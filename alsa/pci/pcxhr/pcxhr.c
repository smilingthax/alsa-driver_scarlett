/*
 * Driver for Digigram pcxhr compatible soundcards
 *
 * main file with alsa callbacks
 *
 * Copyright (c) 2004 by Digigram <alsa@digigram.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */


#include <sound/driver.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/moduleparam.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/info.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include "pcxhr.h"
#include "pcxhr_mixer.h"
#include "pcxhr_hwdep.h"
#include "pcxhr_core.h"

#define DRIVER_NAME "pcxhr"

MODULE_AUTHOR("Markus Bollinger <bollinger@digigram.com>");
MODULE_DESCRIPTION("Digigram " DRIVER_NAME);
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("{{Digigram," DRIVER_NAME "}}");

static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;		/* Index 0-MAX */
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;		/* ID for this card */
static int enable[SNDRV_CARDS] = SNDRV_DEFAULT_ENABLE_PNP;	/* Enable this card */
static int mono_capture[SNDRV_CARDS];				/* capture in mono only */

module_param_array(index, int, NULL, 0444);
MODULE_PARM_DESC(index, "Index value for Digigram " DRIVER_NAME " soundcard.");
module_param_array(id, charp, NULL, 0444);
MODULE_PARM_DESC(id, "ID string for Digigram " DRIVER_NAME " soundcard.");
module_param_array(enable, bool, NULL, 0444);
MODULE_PARM_DESC(enable, "Enable Digigram " DRIVER_NAME " soundcard.");
module_param_array(mono_capture, bool, NULL, 0444);
MODULE_PARM_DESC(mono_capture, "Mono Capture.");


enum {
	PCI_ID_VX882HR,
	PCI_ID_PCX882HR,
	PCI_ID_VX881HR,
	PCI_ID_PCX881HR,
	PCI_ID_LAST
};

/*
 */
static struct pci_device_id pcxhr_ids[] = {
	{ 0x10b5, 0x9656, 0x1369, 0xb001, 0, 0, PCI_ID_VX882HR, },  /* VX882HR */
	{ 0x10b5, 0x9656, 0x1369, 0xb101, 0, 0, PCI_ID_PCX882HR, }, /* PCX882HR */
	{ 0x10b5, 0x9656, 0x1369, 0xb201, 0, 0, PCI_ID_VX881HR, },  /* VX881HR */
	{ 0x10b5, 0x9656, 0x1369, 0xb301, 0, 0, PCI_ID_PCX881HR, }, /* PCX881HR */
	{ 0, }
};

MODULE_DEVICE_TABLE(pci, pcxhr_ids);

#define MAX_WAIT_FOR_DSP	20

static char* pcxhr_board_names[] = {
[PCI_ID_VX882HR] =	"VX882HR",
[PCI_ID_PCX882HR] =	"PCX882HR",
[PCI_ID_VX881HR] =	"VX881HR",
[PCI_ID_PCX881HR] =	"PCX881HR",
};


static int pcxhr_set_pipe_state(pcxhr_mgr_t *mgr, pcxhr_pipe_t* pipe, int start_pipe)
{
	int err, i, current_state;
	pcxhr_rmh_t rmh;

	if(pipe->status == PCXHR_PIPE_UNDEFINED) {
		snd_printk(KERN_ERR "error pcxhr_set_pipe_state called with wrong pipe->status!\n");
		return -EINVAL;
	}
	current_state = pcxhr_is_pipe_running(mgr, pipe->is_capture, pipe->first_audio);
	if(!start_pipe) {
		if(current_state == 0) {
			pipe->status = PCXHR_PIPE_STOPPED;
			return 0;
		}
		snd_printdd("pcxhr_set_pipe_state STOP\n");
	} else {
		if(current_state != 0) {
			pipe->status = PCXHR_PIPE_RUNNING;
			return 0;
		}
		snd_printdd("pcxhr_set_pipe_state START\n");
		pcxhr_init_rmh(&rmh, CMD_CAN_START_PIPE);
		pcxhr_set_pipe_cmd_params(&rmh, pipe->is_capture, pipe->first_audio, 0, 0);
		for(i=0; i<MAX_WAIT_FOR_DSP; i++) {
			err = pcxhr_send_msg(mgr, &rmh);
			if(err) {
				snd_printk(KERN_ERR "error pipe start (CMD_CAN_START_PIPE) err=%x!\n", err );
				return err;
			}
			if(rmh.stat[0] != 0) break;
			pcxhr_delay(1);			/* wait 1 millisecond */
		}
		if(rmh.stat[0] == 0) return -EBUSY;
	}
	pcxhr_init_rmh(&rmh, CMD_CONF_PIPE);
	pcxhr_set_pipe_cmd_params(&rmh, pipe->is_capture, 0, 0, 1 << pipe->first_audio);
	err = pcxhr_send_msg(mgr, &rmh);
	if(err) {
		snd_printk(KERN_ERR "error pipe start (CMD_CONF_PIPE) err=%x!\n", err );
		return err;
	}
	pcxhr_init_rmh(&rmh, CMD_SEND_IRQA);
	err = pcxhr_send_msg(mgr, &rmh);
	if(err) {
		snd_printk(KERN_ERR "error pipe start (CMD_SEND_IRQA) err=%x!\n", err );
		return err;
	}
	for(i=0; i<MAX_WAIT_FOR_DSP; i++) {
		if( pcxhr_is_pipe_running(mgr, pipe->is_capture, pipe->first_audio) ) {
			if(start_pipe) break;
		} else {
			if(!start_pipe) break;
		}
		pcxhr_delay(1);			/* wait 1 millisecond */
	}
	if(i==MAX_WAIT_FOR_DSP) {
		snd_printk(KERN_ERR "error pipe start/stop (ED_NO_RESPONSE_AT_IRQA)\n" );
		return -EBUSY;
	}
	if(start_pipe) {
		pipe->status = PCXHR_PIPE_RUNNING;
	} else {
		pcxhr_init_rmh(&rmh, CMD_STOP_PIPE);
		pcxhr_set_pipe_cmd_params(&rmh, pipe->is_capture, pipe->first_audio, 0, 0);
		err = pcxhr_send_msg(mgr, &rmh);
		if(err) {
			snd_printk(KERN_ERR "error pipe stop (CMD_STOP_PIPE) err=%x!\n", err );
			return err;
		}
		pipe->status = PCXHR_PIPE_STOPPED;
	}
	return 0;
}


static int pcxhr_pll_freq_register(unsigned int freq, unsigned int* pllreg, unsigned int* realfreq)
{
	unsigned int reg;
	if((freq < 6900) || (freq > 110250)) return -EINVAL;
	reg = (28224000 * 10) / freq;
	reg = (reg + 5) / 10;
	if(reg<0x200) {
		*pllreg = reg + 0x800;
	} else if(reg<0x400) {
		*pllreg = reg & 0x1ff;
	} else if(reg<0x800) {
		*pllreg = ((reg >> 1) & 0x1ff) + 0x200;
		reg &= ~1;
	} else {
		*pllreg = ((reg >> 2) & 0x1ff) + 0x400;
		reg &= ~3;
	}
	if(realfreq) {
	  *realfreq = ((28224000 * 10) / reg + 5) / 10;
	}
	return 0;
}

#define PCXHR_FREQ_REG_MASK		0x1f
#define PCXHR_FREQ_QUARTZ_48000		0x00
#define PCXHR_FREQ_QUARTZ_24000		0x01
#define PCXHR_FREQ_QUARTZ_12000		0x09
#define PCXHR_FREQ_QUARTZ_32000		0x08
#define PCXHR_FREQ_QUARTZ_16000		0x04
#define PCXHR_FREQ_QUARTZ_8000		0x0c
#define PCXHR_FREQ_QUARTZ_44100		0x02
#define PCXHR_FREQ_QUARTZ_22050		0x0a
#define PCXHR_FREQ_QUARTZ_11025		0x06
#define PCXHR_FREQ_PLL			0x05
#define PCXHR_FREQ_QUARTZ_192000	0x10
#define PCXHR_FREQ_QUARTZ_96000		0x18
#define PCXHR_FREQ_QUARTZ_176400	0x14
#define PCXHR_FREQ_QUARTZ_88200		0x1c
#define PCXHR_FREQ_QUARTZ_128000	0x12
#define PCXHR_FREQ_QUARTZ_64000		0x1a

#define PCXHR_MODIFY_CLOCK_S_BIT	0x04

#define PCXHR_IRQ_TIMER_FREQ		92000
#define PCXHR_IRQ_TIMER_PERIOD		48

static int pcxhr_set_clock(pcxhr_mgr_t *mgr, unsigned int rate)
{
	unsigned int val, pllreg, realfreq, speed;
	pcxhr_rmh_t rmh;
	int err, changed;
	if(rate == 0)	return 0; /* nothing to do */
	realfreq = rate;
	switch(rate) {
	case 48000 :	val = PCXHR_FREQ_QUARTZ_48000;	break;
	case 24000 :	val = PCXHR_FREQ_QUARTZ_24000;	break;
	case 12000 :	val = PCXHR_FREQ_QUARTZ_12000;	break;
	case 32000 :	val = PCXHR_FREQ_QUARTZ_32000;	break;
	case 16000 :	val = PCXHR_FREQ_QUARTZ_16000;	break;
	case 8000 :	val = PCXHR_FREQ_QUARTZ_8000;	break;
	case 44100 :	val = PCXHR_FREQ_QUARTZ_44100;	break;
	case 22050 :	val = PCXHR_FREQ_QUARTZ_22050;	break;
	case 11025 :	val = PCXHR_FREQ_QUARTZ_11025;	break;
	case 192000 :	val = PCXHR_FREQ_QUARTZ_192000;	break;
	case 96000 :	val = PCXHR_FREQ_QUARTZ_96000;	break;
	case 176400 :	val = PCXHR_FREQ_QUARTZ_176400;	break;
	case 88200 :	val = PCXHR_FREQ_QUARTZ_88200;	break;
	case 128000 :	val = PCXHR_FREQ_QUARTZ_128000;	break;
	case 64000 :	val = PCXHR_FREQ_QUARTZ_64000;	break;
	default :
		val = PCXHR_FREQ_PLL;
		/* get the value for the pll register */
		err = pcxhr_pll_freq_register(rate, &pllreg, &realfreq);
		if(err) return err;
		pcxhr_init_rmh(&rmh, CMD_ACCESS_IO_WRITE);
		rmh.cmd[0] |= IO_NUM_REG_GENCLK;
		rmh.cmd[1]  = pllreg & MASK_DSP_WORD;
		rmh.cmd[2]  = pllreg >> 24;
		rmh.cmd_len = 3;
		err = pcxhr_send_msg(mgr, &rmh);
		if( err < 0) {
			snd_printk(KERN_ERR "error pipe allocation (CMD_ACCESS_IO_WRITE) err=%x!\n", err );
			return err;
		}
	}
	/* codec speed modes */
	if(rate<55000)		speed = 0x00;	/* single speed */
	else if(rate<100000)	speed = 0x80;	/* dual speed */
	else			speed = 0xe0;	/* quad speed */
	if(mgr->codec_speed != speed) {
		pcxhr_init_rmh(&rmh, CMD_ACCESS_IO_WRITE);	/* mute outputs */
		rmh.cmd[0] |= IO_NUM_REG_MUTE_OUT;
		err = pcxhr_send_msg(mgr, &rmh);
		if(err) return err;

		pcxhr_init_rmh(&rmh, CMD_ACCESS_IO_WRITE);	/* set speed ratio */
		rmh.cmd[0] |= IO_NUM_SPEED_RATIO;
		rmh.cmd[1] = (CS4271_MODE_CTL_1 & CHIP_SIG_AND_MAP_SPI) | speed;
		rmh.cmd_len = 2;
		err = pcxhr_send_msg(mgr, &rmh);
		if(err) return err;
	}
	/* set the new frequency */
	err = pcxhr_write_io_num_reg_cont(mgr, PCXHR_FREQ_REG_MASK, val, &changed);
	if(err) return err;

	/* unmute after codec speed modes */
	if(mgr->codec_speed != speed) {
		pcxhr_init_rmh(&rmh, CMD_ACCESS_IO_READ);	/* unmute outputs */
		rmh.cmd[0] |= IO_NUM_REG_MUTE_OUT;
		err = pcxhr_send_msg(mgr, &rmh);
		if(err) return err;
		mgr->codec_speed = speed;			/* save new codec speed */
	}

	if(changed) {
		pcxhr_init_rmh(&rmh, CMD_MODIFY_CLOCK);
		rmh.cmd[0] |= PCXHR_MODIFY_CLOCK_S_BIT;		/* resync fifos  */
		if(rate < PCXHR_IRQ_TIMER_FREQ)	rmh.cmd[1] = PCXHR_IRQ_TIMER_PERIOD;
		else				rmh.cmd[1] = PCXHR_IRQ_TIMER_PERIOD * 2;
		rmh.cmd_len = 2;
		err = pcxhr_send_msg(mgr, &rmh);
		if(err) return err;
	}
	snd_printdd("pcxhr_set_clock to %dHz (realfreq=%d)\n", rate, realfreq);
	return 0;
}


/*
 *  allocate or reference playback/capture pipe (pcmp0/pcmc0)
 */
pcxhr_pipe_t* pcxhr_add_ref_pipe( pcxhr_t *chip, int capture, int monitoring)
{
	int stream_count;
	pcxhr_pipe_t *pipe;

	if(capture) {
		pipe = &(chip->capture_pipe);   /* capture */
		stream_count = PCXHR_CAPTURE_STREAMS;
	} else {
		pipe = &(chip->playback_pipe);  /* playback */
		stream_count = PCXHR_PLAYBACK_STREAMS;
	}
	/* a new stream is opened and there are already all streams in use */
	if( (monitoring == 0) && (pipe->references >= stream_count) ) {
		return NULL;
	}
	/* pipe is not yet defined */
	if( pipe->status == PCXHR_PIPE_UNDEFINED ) {
		int err;
		pcxhr_rmh_t rmh;
		snd_printdd("snd_add_ref_pipe chip(%d) pcm%c0\n", chip->chip_idx, capture ? 'c' : 'p');
		snd_assert(stream_count <= MASK_FIRST_FIELD);
 		pipe->is_capture = capture;
 		pipe->first_audio = 2 * chip->chip_idx;
		/* define pipe (stereo only for instance) with flag P_PCM_ONLY_MASK (0x020000) */
		pcxhr_init_rmh(&rmh, CMD_RES_PIPE);
		pcxhr_set_pipe_cmd_params(&rmh, capture, pipe->first_audio, 2, stream_count | 0x020000);
		err = pcxhr_send_msg(chip->mgr, &rmh);
		if( err < 0) {
			snd_printk(KERN_ERR "error pipe allocation (CMD_RES_PIPE) err=%x!\n", err );
			return NULL;
		}
		pipe->stream_count = stream_count;
		pipe->status = PCXHR_PIPE_STOPPED;
		/* capture levels are set here(important for monitoring) */
		/* playback levels are set later on */
		if(capture) {
			pcxhr_update_audio_pipe_level(chip, 1, 0);	/* left channel */
			pcxhr_update_audio_pipe_level(chip, 1, 1);	/* right channel */
		}
	}

	if(monitoring)	pipe->monitoring = 1;
	else		pipe->references++;

	return pipe;
}


/*
 *  dereference or destroy playback/capture pipe (pcmp0/pcmc0)
 */
int pcxhr_kill_ref_pipe( pcxhr_mgr_t *mgr, pcxhr_pipe_t *pipe, int monitoring)
{
	pcxhr_rmh_t rmh;
	int err = 0;

	if( pipe->status == PCXHR_PIPE_UNDEFINED )
		return 0;
	if( monitoring )	pipe->monitoring = 0;
	else			pipe->references--;

	if( (pipe->references <= 0) && (pipe->monitoring == 0) ) {
		/* stop the pipe */
		err = pcxhr_set_pipe_state(mgr, pipe, 0);
		if( err < 0 ) {
			snd_printk(KERN_ERR "error stopping pipe!\n");
		}
		/* release the pipe */
		pcxhr_init_rmh(&rmh, CMD_FREE_PIPE);
		pcxhr_set_pipe_cmd_params(&rmh, pipe->is_capture, pipe->first_audio, 0, 0);
		err = pcxhr_send_msg(mgr, &rmh);
		if( err < 0 ) {
			snd_printk(KERN_ERR "error pipe release (CMD_FREE_PIPE) err(%x)\n", err);
		}
		pipe->stream_count = 0;
		pipe->status = PCXHR_PIPE_UNDEFINED;
	}
	return err;
}

/*
 *  start or stop playback/capture substream
 */
static int pcxhr_set_stream_state(pcxhr_stream_t *stream, int start)
{
	int err;
	pcxhr_t *chip;
	pcxhr_rmh_t rmh;

	if(!stream->substream)
		return -EINVAL;

	stream->timer_in_update = 0;
	stream->timer_elapsed = 0;
	stream->timer_abs_samples = 0;            /* reset theoretical stream pos */

	pcxhr_init_rmh(&rmh, start ? CMD_START_STREAM : CMD_STOP_STREAM);
	pcxhr_set_pipe_cmd_params(&rmh, stream->pipe->is_capture, stream->pipe->first_audio, 0, 1<<stream->substream->number);

	chip = snd_pcm_substream_chip(stream->substream);

	err = pcxhr_send_msg(chip->mgr, &rmh);
	if(err) {
		snd_printk(KERN_ERR "ERROR pcxhr_set_stream_state err=%x;\n", err);
	}
	return err;
}

#define HEADER_FMT_BASE_LIN		0xfed00000
#define HEADER_FMT_BASE_FLOAT		0xfad00000
#define HEADER_FMT_INTEL		0x00008000
#define HEADER_FMT_24BITS		0x00004000
#define HEADER_FMT_16BITS		0x00002000
#define HEADER_FMT_UPTO11		0x00000200
#define HEADER_FMT_UPTO32		0x00000100
#define HEADER_FMT_MONO			0x00000080

static int pcxhr_set_format(pcxhr_stream_t *stream)
{
	int err, is_capture, sample_rate;
	pcxhr_t *chip;
	pcxhr_rmh_t rmh;
	unsigned int header;

	switch(stream->format){
	case SNDRV_PCM_FORMAT_U8:	header = HEADER_FMT_BASE_LIN; break;
	case SNDRV_PCM_FORMAT_S16_LE:	header = HEADER_FMT_BASE_LIN | HEADER_FMT_16BITS | HEADER_FMT_INTEL; break;
	case SNDRV_PCM_FORMAT_S16_BE:	header = HEADER_FMT_BASE_LIN | HEADER_FMT_16BITS; break;
	case SNDRV_PCM_FORMAT_S24_3LE:	header = HEADER_FMT_BASE_LIN | HEADER_FMT_24BITS | HEADER_FMT_INTEL; break;
	case SNDRV_PCM_FORMAT_S24_3BE:	header = HEADER_FMT_BASE_LIN | HEADER_FMT_24BITS; break;
	case SNDRV_PCM_FORMAT_FLOAT_LE:	header = HEADER_FMT_BASE_FLOAT | HEADER_FMT_INTEL; break;
	default:
		snd_printk(KERN_ERR "error pcxhr_set_format() : unknown format\n");
		return -EINVAL;
	}
	chip = snd_pcm_substream_chip(stream->substream);

	sample_rate = chip->mgr->sample_rate;
	if((sample_rate<=32000) && (sample_rate!=0)) {
		if(sample_rate<=11025)	header |= HEADER_FMT_UPTO11;
		else 			header |= HEADER_FMT_UPTO32;
	}
	if(stream->channels == 1)	header |= HEADER_FMT_MONO;

	is_capture = stream->pipe->is_capture;

	pcxhr_init_rmh(&rmh, is_capture ? CMD_FORMAT_STREAM_IN : CMD_FORMAT_STREAM_OUT);
	pcxhr_set_pipe_cmd_params(&rmh, is_capture, stream->pipe->first_audio, stream->substream->number, 0);
	if(is_capture) rmh.cmd[0] |= 1<<12;
	rmh.cmd[1] = 0;
	rmh.cmd[2] = header >> 8;
	rmh.cmd[3] = (header & 0xff) << 16;
	rmh.cmd_len = 4;
	err = pcxhr_send_msg(chip->mgr, &rmh);
	if(err) {
		snd_printk(KERN_ERR "ERROR pcxhr_set_format err=%x;\n", err);
	}
	return err;
}

static int pcxhr_update_r_buffer(pcxhr_stream_t *stream)
{
	int err, is_capture;
	pcxhr_rmh_t rmh;
	snd_pcm_substream_t *subs = stream->substream;
	pcxhr_t *chip = snd_pcm_substream_chip(subs);

	is_capture = (subs->stream == SNDRV_PCM_STREAM_CAPTURE);

	snd_printdd("pcxhr_hw_params(pcm%c0) : addr(%x) bytes(%x) subs(%d)\n", is_capture?'c':'p',
		    subs->runtime->dma_addr, subs->runtime->dma_bytes, subs->number);

	pcxhr_init_rmh(&rmh, CMD_UPDATE_R_BUFFERS);
	pcxhr_set_pipe_cmd_params(&rmh, is_capture, stream->pipe->first_audio, subs->number, 0);

	snd_assert(subs->runtime->dma_bytes < 0x200000);	/* max buffer size is 2 MByte */
	rmh.cmd[1] = subs->runtime->dma_bytes * 8;		/* size in bits */
	rmh.cmd[2] = subs->runtime->dma_addr >> 24;		/* most significant byte */
	rmh.cmd[2] |= 1<<19;					/* this is a circular buffer */
	rmh.cmd[3] = subs->runtime->dma_addr & MASK_DSP_WORD;	/* least 3 significant bytes */
	rmh.cmd_len = 4;
	err = pcxhr_send_msg(chip->mgr, &rmh);
	if(err) {
		snd_printk(KERN_ERR "ERROR CMD_UPDATE_R_BUFFERS err=%x;\n", err);
	}
	return err;
}

/*
 *  trigger callback
 */
static int pcxhr_trigger(snd_pcm_substream_t *subs, int cmd)
{
	pcxhr_stream_t *stream = (pcxhr_stream_t*)subs->runtime->private_data;
	int err;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		snd_printdd("SNDRV_PCM_TRIGGER_START\n");
		/* if the stream was stopped before, format and buffer were reset */
		if(stream->status == PCXHR_STREAM_STATUS_STOPPED) {
			err = pcxhr_set_format(stream);
			if(err) return err;
			err = pcxhr_update_r_buffer(stream);
			if(err) return err;
		}
		/* START_STREAM */
		if( pcxhr_set_stream_state(stream, 1) )
			return -EINVAL;
		stream->status = PCXHR_STREAM_STATUS_RUNNING;
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		/* STOP_STREAM */
		if( pcxhr_set_stream_state(stream, 0) )
			return -EINVAL;
		stream->status = PCXHR_STREAM_STATUS_STOPPED;
		snd_printdd("SNDRV_PCM_TRIGGER_STOP\n");
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		/* TODO */
		stream->status = PCXHR_STREAM_STATUS_PAUSED;
		snd_printdd("SNDRV_PCM_PAUSE_PUSH\n");
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		/* TODO */
		stream->status = PCXHR_STREAM_STATUS_RUNNING;
		snd_printdd("SNDRV_PCM_PAUSE_RELEASE\n");
		break;
	default:
		return -EINVAL;
	}
	return 0;
}


static int pcxhr_hardware_timer(pcxhr_t *chip, int start)
{
	pcxhr_rmh_t rmh;
	int err;
	pcxhr_init_rmh(&rmh, CMD_SET_TIMER_INTERRUPT);
	if(start) rmh.cmd[0] |= PCXHR_GRANULARITY;
	err = pcxhr_send_msg(chip->mgr, &rmh);
	if( err < 0 ) {
		snd_printk(KERN_ERR "error pcxhr_hardware_timer err(%x)\n", err);
	}
	return err;
}

/*
 *  prepare callback for all pcms
 */
static int pcxhr_prepare(snd_pcm_substream_t *subs)
{
	pcxhr_t *chip = snd_pcm_substream_chip(subs);
	// TODO pcxhr_stream_t *stream = (pcxhr_stream_t*)subs->runtime->private_data;

	snd_printdd("pcxhr_prepare : period_size(%lx) periods(%x) buffer_size(%lx)\n",
		    subs->runtime->period_size, subs->runtime->periods, subs->runtime->buffer_size);

	/* only the first stream can choose the sample rate */
	/* the further opened streams will be limited to its frequency (see open) */
	/* set the clock only once (first stream) on the same pipe */
	if(chip->mgr->ref_count_rate == 1) {
		if( pcxhr_set_clock(chip->mgr, subs->runtime->rate) )
			return -EINVAL;
		chip->mgr->sample_rate = subs->runtime->rate;

		pcxhr_hardware_timer(chip, 1);	/* start the DSP-timer */
	}

	return 0;
}


/*
 *  HW_PARAMS callback for all pcms
 */
static int pcxhr_hw_params(snd_pcm_substream_t *subs,
                               snd_pcm_hw_params_t *hw)
{
	pcxhr_t *chip = snd_pcm_substream_chip(subs);
	pcxhr_mgr_t *mgr = chip->mgr;
	pcxhr_stream_t *stream = (pcxhr_stream_t*)subs->runtime->private_data;
	snd_pcm_format_t format;
	int err;
	int channels;

	/* set up channels */
	channels = params_channels(hw);

	/*  set up format for the stream */
	format = params_format(hw);

	down(&mgr->setup_mutex);

	/* update the stream levels for playback */
	/* capture stream levels are already updated in pcxhr_add_ref_pipe() */
	if( subs->stream == SNDRV_PCM_STREAM_PLAYBACK )
		pcxhr_update_playback_stream_level(chip, subs->number, 0);

	stream->channels = channels;
	stream->format = format;

	/* set the format to the board */
	err = pcxhr_set_format(stream);
	if(err) {
		up(&mgr->setup_mutex);
		return err;
	}

	/* allocate buffer */
	err = snd_pcm_lib_malloc_pages(subs, params_buffer_bytes(hw));

	if (err > 0) {
		err = pcxhr_update_r_buffer(stream);
	}
	up(&mgr->setup_mutex);

	return err;
}

static int pcxhr_hw_free(snd_pcm_substream_t *subs)
{
	snd_pcm_lib_free_pages(subs);
	return 0;
}


/*
 *  CONFIGURATION SPACE for all pcms, mono pcm must update channels_max
 */
static snd_pcm_hardware_t pcxhr_caps =
{
	.info             = ( SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
			      SNDRV_PCM_INFO_MMAP_VALID | SNDRV_PCM_INFO_SYNC_START |
			      SNDRV_PCM_INFO_PAUSE),
	.formats	  = ( SNDRV_PCM_FMTBIT_U8 |
			      SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE |
			      SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S24_3BE |
			      SNDRV_PCM_FMTBIT_FLOAT_LE ),
	.rates            = SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_192000,
	.rate_min         = 8000,
	.rate_max         = 192000,
	.channels_min     = 1,
	.channels_max     = 2,
	.buffer_bytes_max = (32*1024),
	.period_bytes_min = PCXHR_GRANULARITY,	/* 1 byte == 1 frame U8 mono (PCXHR_GRANULARITY is frames!) */
	.period_bytes_max = (16*1024),
	.periods_min      = 2,
	.periods_max      = (32*1024/PCXHR_GRANULARITY),
};


static int pcxhr_open(snd_pcm_substream_t *subs)
{
	pcxhr_t             *chip = snd_pcm_substream_chip(subs);
	pcxhr_mgr_t         *mgr = chip->mgr;
	snd_pcm_runtime_t   *runtime = subs->runtime;
	pcxhr_stream_t      *stream;
	pcxhr_pipe_t        *pipe;
	int err, is_capture;

	down(&mgr->setup_mutex);

	/* copy the snd_pcm_hardware_t struct */
	runtime->hw = pcxhr_caps;

	if( subs->stream == SNDRV_PCM_STREAM_PLAYBACK ) {
		snd_printdd("pcxhr_open playback chip%d subs%d\n", chip->chip_idx, subs->number);
		is_capture = 0;
		stream = &(chip->playback_stream[subs->number]);
	} else {
		snd_printdd("pcxhr_open capture chip%d subs%d\n", chip->chip_idx, subs->number);
		is_capture = 1;
		stream = &(chip->capture_stream);
	}

	if (stream->status != PCXHR_STREAM_STATUS_FREE){
		/* streams in use */
		snd_printk(KERN_ERR "pcxhr_open chip%d subs%d in use\n", chip->chip_idx, subs->number);
		err = -EBUSY;
		goto _exit_open;
	}

	/* get pipe pointer (out pipe) */
	pipe = pcxhr_add_ref_pipe(chip, is_capture, 0);

	if (pipe == NULL) {
		err = -EINVAL;
		goto _exit_open;
	}

	/* start the pipe if necessary */
	err = pcxhr_set_pipe_state(chip->mgr, pipe, 1);
	if( err < 0 ) {
		snd_printk(KERN_ERR "error starting pipe!\n");
		pcxhr_kill_ref_pipe(chip->mgr, pipe, 0);
		goto _exit_open;
	}

	stream->pipe        = pipe;
	stream->status      = PCXHR_STREAM_STATUS_OPEN;
	stream->substream   = subs;
	stream->channels    = 0; /* not configured yet */

	runtime->private_data = stream;

	snd_pcm_hw_constraint_step(runtime, 0, SNDRV_PCM_HW_PARAM_BUFFER_BYTES, 4);
	snd_pcm_hw_constraint_step(runtime, 0, SNDRV_PCM_HW_PARAM_PERIOD_BYTES, 4);

	/* if a sample rate is already used, another stream cannot change */
	if(mgr->ref_count_rate++) {
		if(mgr->sample_rate) {
			runtime->hw.rate_min = runtime->hw.rate_max = mgr->sample_rate;
		}
	}

 _exit_open:
	up(&mgr->setup_mutex);
	return err;
}


static int pcxhr_close(snd_pcm_substream_t *subs)
{
	pcxhr_t *chip = snd_pcm_substream_chip(subs);
	pcxhr_mgr_t *mgr = chip->mgr;
	pcxhr_stream_t *stream = (pcxhr_stream_t*)subs->runtime->private_data;

	down(&mgr->setup_mutex);

	snd_printdd("pcxhr_close chip%d subs%d\n", chip->chip_idx, subs->number);

	/* sample rate released */
	if(--mgr->ref_count_rate == 0) {
		mgr->sample_rate = 0;

		pcxhr_hardware_timer(chip, 0);	/* stop the DSP-timer */
	}

	/* delete pipe */
	if (pcxhr_kill_ref_pipe(mgr, stream->pipe, 0 ) < 0) {

		snd_printk(KERN_ERR "error pcxhr_kill_ref_pipe chip%d subs%d\n", chip->chip_idx, subs->number);
	}

	stream->pipe      = NULL;
	stream->status    = PCXHR_STREAM_STATUS_FREE;
	stream->substream = NULL;

	up(&mgr->setup_mutex);

	return 0;
}


static snd_pcm_uframes_t pcxhr_stream_pointer(snd_pcm_substream_t * subs)
{
	snd_pcm_runtime_t *runtime = subs->runtime;
	pcxhr_stream_t *stream  = (pcxhr_stream_t*)runtime->private_data;
	pcxhr_t *chip = snd_pcm_substream_chip(subs);
	pcxhr_rmh_t rmh;
	snd_pcm_uframes_t sample_count = 0;

	if(0 /*stream->timer_in_update*/ ) {
		sample_count = stream->timer_abs_samples;
	} else {
		/* get sample count for one stream */
		pcxhr_init_rmh(&rmh, CMD_STREAM_SAMPLE_COUNT);
		pcxhr_set_pipe_cmd_params(&rmh, stream->pipe->is_capture, stream->pipe->first_audio, 0, 1<<subs->number);
		/* do cmd[0] |= (1<<10) to get a byte count instead of sample count */
		rmh.stat_len = 2;		/* 2 resp data for each stream of the pipe */
		if( ! pcxhr_send_msg(chip->mgr, &rmh) ) {
			sample_count = rmh.stat[0] << 24;
			sample_count |= rmh.stat[1];
			/*snd_printdd("stream pointer abs %lx (%lx), pointer = %lx\n", sample_count, stream->timer_abs_samples,
				    sample_count % runtime->buffer_size);
			*/
			spin_lock(&chip->mgr->lock);
			/* adjust the absolute timer position */
			stream->timer_abs_samples = sample_count;
			spin_unlock(&chip->mgr->lock);
		}
	}
	return sample_count % runtime->buffer_size;
}


static snd_pcm_ops_t pcxhr_ops = {
	.open      = pcxhr_open,
	.close     = pcxhr_close,
	.ioctl     = snd_pcm_lib_ioctl,
	.prepare   = pcxhr_prepare,
	.hw_params = pcxhr_hw_params,
	.hw_free   = pcxhr_hw_free,
	.trigger   = pcxhr_trigger,
	.pointer   = pcxhr_stream_pointer,
};


/*
 */
int pcxhr_create_pcm(pcxhr_t *chip)
{
	int err;
	snd_pcm_t *pcm;
	char name[32];

	sprintf(name, "pcxhr %d", chip->chip_idx);
	if ((err = snd_pcm_new(chip->card, name, 0,
			       PCXHR_PLAYBACK_STREAMS,
			       PCXHR_CAPTURE_STREAMS, &pcm)) < 0) {
		snd_printk(KERN_ERR "cannot create pcm %s\n", name);
		return err;
	}
	pcm->private_data = chip;

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &pcxhr_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &pcxhr_ops);

	pcm->info_flags = 0;
	strcpy(pcm->name, name);

	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
					      snd_dma_pci_data(chip->mgr->pci), 32*1024, 32*1024);
	chip->pcm = pcm;
	return 0;
}

static int pcxhr_chip_free(pcxhr_t *chip)
{
	kfree(chip);
	return 0;
}

static int pcxhr_chip_dev_free(snd_device_t *device)
{
	pcxhr_t *chip = device->device_data;
	return pcxhr_chip_free(chip);
}


/*
 */
static int __devinit pcxhr_create(pcxhr_mgr_t *mgr, snd_card_t *card, int idx)
{
	int err;
	pcxhr_t *chip;
	static snd_device_ops_t ops = {
		.dev_free = pcxhr_chip_dev_free,
	};

	mgr->chip[idx] = chip = kcalloc(1, sizeof(*chip), GFP_KERNEL);
	if (! chip) {
		snd_printk(KERN_ERR "cannot allocate chip\n");
		return -ENOMEM;
	}

	chip->card = card;
	chip->chip_idx = idx;
	chip->mgr = mgr;

	if ((err = snd_device_new(card, SNDRV_DEV_LOWLEVEL, chip, &ops)) < 0) {
		pcxhr_chip_free(chip);
		return err;
	}

	if (idx == 0) {
		/* create a DSP loader only on first card */
	  err = pcxhr_setup_firmware(mgr);
		if (err < 0)
			return err;
	}

	snd_card_set_dev(card, &mgr->pci->dev);

	return 0;
}

/* proc interface */
static void pcxhr_proc_read(snd_info_entry_t *entry, snd_info_buffer_t *buffer)
{
	pcxhr_t *chip = entry->private_data;
	pcxhr_mgr_t *mgr = chip->mgr;

	snd_iprintf(buffer, "\n%s\n", mgr->longname);

	/* stats available when embedded DSP is running */
	if( mgr->dsp_loaded & ( 1 << PCXHR_FIRMWARE_DSP_MAIN_INDEX) ) {
		pcxhr_rmh_t rmh;
		short ver_maj = (mgr->dsp_version>>16)&0xff;
		short ver_min = (mgr->dsp_version>>8)&0xff;
		short ver_build = mgr->dsp_version&0xff;
		snd_iprintf(buffer, "dsp version %d.%d.%d\n", ver_maj, ver_min, ver_build);
		if(mgr->board_has_analog)
			snd_iprintf(buffer, "analog io available\n");
		else
			snd_iprintf(buffer, "digital only board\n");

		/* calc cpu load of the dsp */
		pcxhr_init_rmh(&rmh, CMD_GET_DSP_RESOURCES);
		if( ! pcxhr_send_msg(mgr, &rmh) ) {
			int cur = rmh.stat[0];
			int ref = rmh.stat[1];
			if(ref > 0) {
				if((mgr->sample_rate != 0) && (mgr->sample_rate != 48000)) {
					ref = (ref*48000)/mgr->sample_rate;
					if(mgr->sample_rate >= PCXHR_IRQ_TIMER_FREQ) ref*=2;
				}
				cur = 100 - (100 * cur)/ref;
				snd_iprintf(buffer, "cpu load    %d%%\n", cur);
			}
		}
	} else {
		snd_iprintf(buffer, "no firmware loaded\n");
	}
	snd_iprintf(buffer, "\n");
}

static void __devinit pcxhr_proc_init(pcxhr_t *chip)
{
	snd_info_entry_t *entry;

	if (! snd_card_proc_new(chip->card, "info", &entry)) {
		snd_info_set_text_ops(entry, chip, 1024, pcxhr_proc_read);
	}
}
/* end of proc interface */

/*
 * release all the cards assigned to a manager instance
 */
static int pcxhr_free(pcxhr_mgr_t *mgr)
{
	unsigned int i;

	for (i = 0; i < mgr->num_cards; i++) {
		if (mgr->chip[i])
			snd_card_free(mgr->chip[i]->card);
	}

	/* reset board if some firmware was loaded */
	if(mgr->dsp_loaded) {
		pcxhr_reset_board(mgr);
		snd_printdd("reset pcxhr !\n");
	}

	/* release irq  */
	if (mgr->irq >= 0)
		free_irq(mgr->irq, (void *)mgr);

	pci_release_regions(mgr->pci);

	/* free hostport purgebuffer */
	if(mgr->hostport.area) {
		snd_dma_free_pages(&mgr->hostport);
		mgr->hostport.area = NULL;
	}

	pci_disable_device(mgr->pci);
	kfree(mgr);
	return 0;
}

/*
 *    probe function - creates the card manager
 */
static int __devinit pcxhr_probe(struct pci_dev *pci, const struct pci_device_id *pci_id)
{
	static int dev;
	pcxhr_mgr_t *mgr;
	unsigned int i;
	int err;
	size_t size;
	char *card_name;

	if (dev >= SNDRV_CARDS)
		return -ENODEV;
	if (! enable[dev]) {
		dev++;
		return -ENOENT;
	}

	/* enable PCI device */
	if ((err = pci_enable_device(pci)) < 0)
		return err;
	pci_set_master(pci);

	/* check if we can restrict PCI DMA transfers to 32 bits */
	if (pci_set_dma_mask(pci, 0xffffffff) < 0) {
		snd_printk(KERN_ERR "architecture does not support 32bit PCI busmaster DMA\n");
		pci_disable_device(pci);
		return -ENXIO;
	}

	/* alloc card manager */
	mgr = kcalloc(1, sizeof(*mgr), GFP_KERNEL);
	if (! mgr) {
		pci_disable_device(pci);
		return -ENOMEM;
	}

	snd_assert(pci_id->driver_data < PCI_ID_LAST, return -ENODEV);
	card_name = pcxhr_board_names[pci_id->driver_data];

	/* resource assignment */
	if ((err = pci_request_regions(pci, card_name)) < 0) {
		kfree(mgr);
		pci_disable_device(pci);
		return err;
	}
	for (i = 0; i < 3; i++) {
		mgr->port[i] = pci_resource_start(pci, i);
	}

	mgr->pci = pci;
	mgr->irq = -1;

	if (request_irq(pci->irq, pcxhr_interrupt, SA_INTERRUPT|SA_SHIRQ, card_name, (void *)mgr)) {
		snd_printk(KERN_ERR "unable to grab IRQ %d\n", pci->irq);
		pcxhr_free(mgr);
		return -EBUSY;
		}
	mgr->irq = pci->irq;

	sprintf(mgr->shortname, "Digigram %s", card_name);
	sprintf(mgr->longname, "%s at 0x%lx & 0x%lx, 0x%lx irq %i", mgr->shortname,
		mgr->port[0], mgr->port[1], mgr->port[2], mgr->irq);

	/* ISR spinlock  */
	spin_lock_init(&mgr->lock);

	spin_lock_init(&mgr->msg_lock);

	/* init setup mutex*/
	init_MUTEX(&mgr->setup_mutex);

	/* init taslket */
	tasklet_init( &mgr->msg_taskq, pcxhr_msg_tasklet, (unsigned long) mgr);

	/* card assignment */
	mgr->num_cards = PCXHR_MAX_CARDS; /* 4  FIXME: configurable? */
	mgr->mono_capture = mono_capture[dev];
	for (i = 0; i < mgr->num_cards; i++) {
		snd_card_t *card;
		char tmpid[16];
		int idx;

		if (index[dev] < 0)	idx = index[dev];
		else			idx = index[dev] + i;

		snprintf(tmpid, sizeof(tmpid), "%s-%d", id[dev], i);
		card = snd_card_new(idx, tmpid, THIS_MODULE, 0);

		if (! card) {
			snd_printk(KERN_ERR "cannot allocate the card %d\n", i);
			pcxhr_free(mgr);
			return -ENOMEM;
		}

		strcpy(card->driver, DRIVER_NAME);
		sprintf(card->shortname, "%s [PCM #%d]", mgr->shortname, i);
		sprintf(card->longname, "%s [PCM #%d]", mgr->longname, i);

		if ((err = pcxhr_create(mgr, card, i)) < 0) {
			pcxhr_free(mgr);
			return err;
		}

		if(i==0) {
			/* init proc interface only for chip0 */
			pcxhr_proc_init(mgr->chip[i]);
		}

		if ((err = snd_card_register(card)) < 0) {
			pcxhr_free(mgr);
			return err;
		}
	}

	/* create hostport purgebuffer */
	size = PAGE_ALIGN( sizeof(struct pcxhr_hostport) );
	if (snd_dma_alloc_pages(SNDRV_DMA_TYPE_DEV, snd_dma_pci_data(pci),
				size, &mgr->hostport) < 0) {
		pcxhr_free(mgr);
		return -ENOMEM;
	}
	/* init purgebuffer */
	memset(mgr->hostport.area, 0, size);

	pci_set_drvdata(pci, mgr);
	dev++;
	return 0;
}

static void __devexit pcxhr_remove(struct pci_dev *pci)
{
	pcxhr_free(pci_get_drvdata(pci));
	pci_set_drvdata(pci, NULL);
}

static struct pci_driver driver = {
	.name = "Digigram pcxhr",
	.id_table = pcxhr_ids,
	.probe = pcxhr_probe,
	.remove = __devexit_p(pcxhr_remove),
};

static int __init pcxhr_module_init(void)
{
	return pci_module_init(&driver);
}

static void __exit pcxhr_module_exit(void)
{
	pci_unregister_driver(&driver);
}

module_init(pcxhr_module_init)
module_exit(pcxhr_module_exit)
