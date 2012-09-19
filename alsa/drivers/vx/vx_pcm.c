/* to be in alsa-driver-specfici code */
#include <linux/config.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,18)
static struct page *vmalloc_to_page(void *pageptr);
#endif
#define __NO_VERSION__

/*
 * Driver for Digigram VX soundcards
 *
 * PCM part
 *
 * Copyright (c) 2002 by Takashi Iwai <tiwai@suse.de>
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
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <sound/core.h>
#include <sound/asoundef.h>
#include <sound/pcm.h>
#include "vx_core.h"
#include "vx_cmd.h"

#define chip_t	vx_core_t


/*
 * we use a vmalloc'ed (sg-)buffer
 */

/* get the physical page pointer on the given offset */
static struct page *snd_pcm_get_vmalloc_page(snd_pcm_substream_t *subs, unsigned long offset)
{
	void *pageptr = subs->runtime->dma_area + offset;
	return vmalloc_to_page(pageptr);
}

/*
 * hw_params callback
 * NOTE: this may be called not only once per pcm open!
 */
static int snd_pcm_alloc_vmalloc_buffer(snd_pcm_substream_t *subs, int size)
{
	snd_pcm_runtime_t *runtime = subs->runtime;
	if (runtime->dma_area) {
		if (runtime->dma_bytes >= size)
			return 0; /* already enough large */
		vfree_nocheck(runtime->dma_area);
	}
	runtime->dma_area = vmalloc_nocheck(size);
	if (! runtime->dma_area)
		return -ENOMEM;
	runtime->dma_bytes = size;
	return 0;
}

/*
 * hw_free callback
 * NOTE: this may be called not only once per pcm open!
 */
static int snd_pcm_free_vmalloc_buffer(snd_pcm_substream_t *subs)
{
	snd_pcm_runtime_t *runtime = subs->runtime;
	if (runtime->dma_area) {
		vfree_nocheck(runtime->dma_area);
		runtime->dma_area = NULL;
	}
	return 0;
}


#if 0 /* NOT USED */
/*
 * vx_flush_read - read rest of bytes via normal transfer mode
 * @count: frames to read
 *
 * the data size must be aligned to 3 bytes, though.
 * NB: call with a certain lock!
 */
static void vx_flush_read(vx_core_t *chip, snd_pcm_runtime_t *runtime,
			  vx_pipe_t *pipe, int count)
{
	int offset = frames_to_bytes(runtime, pipe->hw_ptr);
	unsigned char *buf = (unsigned char *)(runtime->dma_area + offset);

	snd_assert(count % 3 == 0, return);
	pipe->hw_ptr += count;
	if (pipe->hw_ptr >= runtime->buffer_size)
		pipe->hw_ptr -= runtime->buffer_size;
	while (count > 0) {
		*buf++ = vx_inb(chip, RXH);
		if (++offset >= pipe->buffer_bytes) {
			offset = 0;
			buf = (unsigned char *)runtime->dma_area;
		}
		*buf++ = vx_inb(chip, RXM);
		if (++offset >= pipe->buffer_bytes) {
			offset = 0;
			buf = (unsigned char *)runtime->dma_area;
		}
		*buf++ = vx_inb(chip, RXL);
		if (++offset >= pipe->buffer_bytes) {
			offset = 0;
			buf = (unsigned char *)runtime->dma_area;
		}
		count -= 3;
	}
}
#endif /* NOT USED */

/*
 * vx_set_pcx_time - convert from the PC time to the RMH status time.
 * @pc_time: the pointer for the PC-time to set
 * @dsp_time: the pointer for RMH status time array
 */
static void vx_set_pcx_time(vx_core_t *chip, pcx_time_t *pc_time, unsigned int *dsp_time)
{
	dsp_time[0] = (unsigned int)((*pc_time) >> 24) & PCX_TIME_HI_MASK;
	dsp_time[1] = (unsigned int)(*pc_time) &  MASK_DSP_WORD;
}

/*
 * vx_set_differed_time - set the differed time if specified
 * @rmh: the rmh record to modify
 * @pipe: the pipe to be checked
 *
 * if the pipe is programmed with the differed time, set the DSP time
 * on the rmh and changes its command length.
 *
 * returns the increase of the command length.
 */
static int vx_set_differed_time(vx_core_t *chip, struct vx_rmh *rmh, vx_pipe_t *pipe)
{
	/* Update The length added to the RMH command by the timestamp */
	if (! (pipe->differed_type & DC_DIFFERED_DELAY))
		return 0;
		
	/* Set the T bit */
	rmh->Cmd[0] |= DSP_DIFFERED_COMMAND_MASK;

	/* Time stamp is the 1st following parameter */
	vx_set_pcx_time(chip, &pipe->pcx_time, &rmh->Cmd[1]);

	/* Add the flags to a notified differed command */
	if (pipe->differed_type & DC_NOTIFY_DELAY)
		rmh->Cmd[1] |= NOTIFY_MASK_TIME_HIGH ;

	/* Add the flags to a multiple differed command */
	if (pipe->differed_type & DC_MULTIPLE_DELAY)
		rmh->Cmd[1] |= MULTIPLE_MASK_TIME_HIGH;

	/* Add the flags to a stream-time differed command */
	if (pipe->differed_type & DC_STREAM_TIME_DELAY)
		rmh->Cmd[1] |= STREAM_MASK_TIME_HIGH;
		
	rmh->LgCmd += 2;
	return 2;
}

/*
 * vx_set_stream_format - send the stream format command
 * @pipe: the affected pipe
 * @data: format bitmask
 */
static int vx_set_stream_format(vx_core_t *chip, vx_pipe_t *pipe, unsigned int data)
{
	struct vx_rmh rmh;

	vx_init_rmh(&rmh, pipe->is_capture ?
		    CMD_FORMAT_STREAM_IN : CMD_FORMAT_STREAM_OUT);
	rmh.Cmd[0] |= pipe->number << FIELD_SIZE;

        /* Command might be longer since we may have to add a timestamp */
	vx_set_differed_time(chip, &rmh, pipe);

	rmh.Cmd[rmh.LgCmd] = (data & 0xFFFFFF00) >> 8;
	rmh.Cmd[rmh.LgCmd + 1] = (data & 0xFF) << 16 /*| (datal & 0xFFFF00) >> 8*/;
	rmh.LgCmd += 2;
    
	return vx_send_msg(chip, &rmh);
}


/*
 * vx_set_format - set the format of a pipe
 * @pipe: the affected pipe
 * @runtime: pcm runtime instance to be referred
 *
 * returns 0 if successful, or a negative error code.
 */
static int vx_set_format(vx_core_t *chip, vx_pipe_t *pipe,
			 snd_pcm_runtime_t *runtime)
{
	unsigned int header = HEADER_FMT_BASE;

	if (runtime->channels == 1)
		header |= HEADER_FMT_MONO;
	if (snd_pcm_format_little_endian(runtime->format))
		header |= HEADER_FMT_INTEL;
	if (runtime->rate < 32000 && runtime->rate > 11025)
		header |= HEADER_FMT_UPTO32;
	else if (runtime->rate <= 11025)
		header |= HEADER_FMT_UPTO11;

	switch (snd_pcm_format_physical_width(runtime->format)) {
	// case 8: break;
	case 16: header |= HEADER_FMT_16BITS; break;
	case 24: header |= HEADER_FMT_24BITS; break;
	default : 
		snd_BUG();
		return -EINVAL;
        };

	return vx_set_stream_format(chip, pipe, header);
}

static int vx_set_ibl(vx_core_t *chip, struct vx_ibl_info *info)
{
	int err;
	struct vx_rmh rmh;

	vx_init_rmh(&rmh, CMD_IBL);
	rmh.Cmd[0] |= info->size & 0x03ffff;
	err = vx_send_msg(chip, &rmh);
	if (err < 0)
		return err;
	info->size = rmh.Stat[0];
	info->max_size = rmh.Stat[1];
	info->min_size = rmh.Stat[2];
	info->granularity = rmh.Stat[3];
	snd_printdd(KERN_DEBUG "vx_set_ibl: size = %d, max = %d, min = %d, gran = %d\n",
		   info->size, info->max_size, info->min_size, info->granularity);
	return 0;
}


/*
 * vx_get_pipe_state - get the state of a pipe
 * @pipe: the pipe to be checked
 * @state: the pointer for the returned state
 *
 * checks the state of a given pipe, and stores the state (1 = running,
 * 0 = paused) on the given pointer.
 *
 * called from trigger callback only
 */
static int vx_get_pipe_state(vx_core_t *chip, vx_pipe_t *pipe, int *state)
{
	int err;
	struct vx_rmh rmh;

	vx_init_rmh(&rmh, CMD_PIPE_STATE);
	vx_set_pipe_cmd_params(&rmh, pipe->is_capture, pipe->number, 0);
	err = vx_send_msg_nolock(chip, &rmh); /* no lock needed for trigger */ 
	if (! err)
		*state = (rmh.Stat[0] & (1 << pipe->number)) ? 1 : 0;
	return err;
}


/*
 * vx_query_hbuffer_size - query available h-buffer size in bytes
 * @pipe: the pipe to be checked
 *
 * return the available size on h-buffer in bytes,
 * or a negative error code.
 */
static int vx_query_hbuffer_size(vx_core_t *chip, vx_pipe_t *pipe)
{
	int result;
	struct vx_rmh rmh;

	vx_init_rmh(&rmh, CMD_SIZE_HBUFFER);
	vx_set_pipe_cmd_params(&rmh, pipe->is_capture, pipe->number, 0);
	if (pipe->is_capture)
		rmh.Cmd[0] |= 0x00000001;
	result = vx_send_msg(chip, &rmh);
	if (! result)
		result = rmh.Stat[0] & 0xffff;
	return result;
}


/*
 * vx_pipe_can_start - query whether a pipe is ready for start
 * @pipe: the pipe to be checked
 *
 * return 1 if ready, 0 if not ready, and negative value on error.
 *
 * called from trigger callback only
 */
static int vx_pipe_can_start(vx_core_t *chip, vx_pipe_t *pipe)
{
	int err;
	struct vx_rmh rmh;
        
	vx_init_rmh(&rmh, CMD_CAN_START_PIPE);
	vx_set_pipe_cmd_params(&rmh, pipe->is_capture, pipe->number, 0);
	rmh.Cmd[0] |= 1;

	err = vx_send_msg_nolock(chip, &rmh); /* no lock needed for trigger */ 
	if (! err) {
		if (rmh.Stat[0])
			err = 1;
	}
	return err;
}

/*
 * vx_vonf_pipe - tell the pipe to stand by and wait for IRQA.
 * @pipe: the pipe to be configured
 */
static int vx_conf_pipe(vx_core_t *chip, vx_pipe_t *pipe)
{
	struct vx_rmh rmh;

	vx_init_rmh(&rmh, CMD_CONF_PIPE);
	if (pipe->is_capture)
		rmh.Cmd[0] |= COMMAND_RECORD_MASK;
	rmh.Cmd[1] = 1 << pipe->number;
	return vx_send_msg_nolock(chip, &rmh); /* no lock needed for trigger */
}

/*
 * vx_send_irqa - trigger IRQA
 */
static int vx_send_irqa(vx_core_t *chip)
{
	struct vx_rmh rmh;

	vx_init_rmh(&rmh, CMD_SEND_IRQA);
	return vx_send_msg_nolock(chip, &rmh); /* no lock needed for trigger */ 
}

#define MAX_WAIT_FOR_DSP        250
#define CAN_START_DELAY         10
#define WAIT_STATE_DELAY        10

/*
 * vx_toggle_pipe - start / pause a pipe
 * @pipe: the pipe to be triggered
 * @state: start = 1, pause = 0
 *
 * called from trigger callback only
 *
 */
static int vx_toggle_pipe(vx_core_t *chip, vx_pipe_t *pipe, int state)
{
	int err, i, cur_state, delay;

	/* Check the pipe is not already in the requested state */
	if (vx_get_pipe_state(chip, pipe, &cur_state) < 0)
		return -EBADFD;
	if (state == cur_state)
		return 0;

	/* If a start is requested, ask the DSP to get prepared
	 * and wait for a positive acknowledge (when there are
	 * enough sound buffer for this pipe)
	 */
	if (state) {
		int delay = CAN_START_DELAY;
		for (i = 0 ; i < MAX_WAIT_FOR_DSP; i++) {
			err = vx_pipe_can_start(chip, pipe);
			if (err > 0)
				break;
			/* Wait for a few, before asking again
			 * to avoid flooding the DSP with our requests
			 */
			if ((i % 4 ) == 0)
				delay <<= 1;
			snd_vx_delay(chip, delay);
		}
	}
    
	if ((err = vx_conf_pipe(chip, pipe)) < 0)
		return err;

	if ((err = vx_send_irqa(chip)) < 0)
		return err;
    
	/* If it completes successfully, wait for the pipes
	 * reaching the expected state before returning
	 * Check one pipe only (since they are synchronous)
	 */
	delay = WAIT_STATE_DELAY;
	for (i = 0; i < MAX_WAIT_FOR_DSP; i++) {
		err = vx_get_pipe_state(chip, pipe, &cur_state);
		if (err < 0 || cur_state == state)
			break;
		err = -EIO;
		if ((i % 4 ) == 0)
			delay <<= 1;
		snd_vx_delay(chip, delay);
	}
	return err < 0 ? -EIO : 0;
}

    
/*
 * vx_stop_pipe - stop a pipe
 * @pipe: the pipe to be stopped
 *
 * called from trigger callback only
 */
static int vx_stop_pipe(vx_core_t *chip, vx_pipe_t *pipe)
{
	struct vx_rmh rmh;
	vx_init_rmh(&rmh, CMD_STOP_PIPE);
	vx_set_pipe_cmd_params(&rmh, pipe->is_capture, pipe->number, 0);
	return vx_send_msg_nolock(chip, &rmh); /* no lock needed for trigger */ 
}


/*
 * vx_alloc_pipe - allocate a pipe and initialize the pipe instance
 * @capture: 0 = playback, 1 = capture operation
 * @audioid: the audio id to be assigned
 * @num_audio: number of audio channels
 * @pipep: the returned pipe instance
 *
 * return 0 on success, or a negative error code.
 */
static int vx_alloc_pipe(vx_core_t *chip, int capture,
			 int audioid, int num_audio,
			 vx_pipe_t **pipep)
{
	int err;
	vx_pipe_t *pipe;
	struct vx_rmh rmh;
	int data_mode;

	*pipep = 0;
	vx_init_rmh(&rmh, CMD_RES_PIPE);
	vx_set_pipe_cmd_params(&rmh, capture, audioid, num_audio);
#if 0	// NYI
	if (underrun_skip_sound)
		rmh.Cmd[0] |= BIT_SKIP_SOUND;
#endif	// NYI
	data_mode = (chip->uer_bits & IEC958_AES0_NONAUDIO) != 0;
	if (! capture && data_mode)
		rmh.Cmd[0] |= BIT_DATA_MODE;
	err = vx_send_msg(chip, &rmh);
	if (err < 0)
		return err;

	/* initialize the pipe record */
	pipe = snd_magic_kcalloc(vx_pipe_t, 0, GFP_KERNEL);
	if (! pipe) {
		/* release the pipe */
		vx_init_rmh(&rmh, CMD_FREE_PIPE);
		vx_set_pipe_cmd_params(&rmh, capture, audioid, 0);
		vx_send_msg(chip, &rmh);
		return -ENOMEM;
	}

	/* the pipe index should be identical with the audio index */
	pipe->number = audioid;
	pipe->is_capture = capture;
	pipe->channels = num_audio;
	pipe->differed_type = 0;
	pipe->pcx_time = 0;
	pipe->data_mode = data_mode;
	*pipep = pipe;

	return 0;
}


/*
 * vx_free_pipe - release a pipe
 * @pipe: pipe to be released
 */
static int vx_free_pipe(vx_core_t *chip, vx_pipe_t *pipe)
{
	struct vx_rmh rmh;

	vx_init_rmh(&rmh, CMD_FREE_PIPE);
	vx_set_pipe_cmd_params(&rmh, pipe->is_capture, pipe->number, 0);
	vx_send_msg(chip, &rmh);

	snd_magic_kfree(pipe);
	return 0;
}


/*
 * vx_start_stream - start the stream
 *
 * called from trigger callback only
 */
static int vx_start_stream(vx_core_t *chip, vx_pipe_t *pipe)
{
	struct vx_rmh rmh;

	vx_init_rmh(&rmh, CMD_START_ONE_STREAM);
	vx_set_stream_cmd_params(&rmh, pipe->is_capture, pipe->number);
	vx_set_differed_time(chip, &rmh, pipe);
	return vx_send_msg_nolock(chip, &rmh); /* no lock needed for trigger */ 
}


/*
 * vx_stop_stream - stop the stream
 *
 * called from trigger callback only
 */
static int vx_stop_stream(vx_core_t *chip, vx_pipe_t *pipe)
{
	struct vx_rmh rmh;

	vx_init_rmh(&rmh, CMD_STOP_STREAM);
	vx_set_stream_cmd_params(&rmh, pipe->is_capture, pipe->number);
	return vx_send_msg_nolock(chip, &rmh); /* no lock needed for trigger */ 
}


/*
 * playback hw information
 */

static snd_pcm_hardware_t vx_pcm_playback_hw = {
	.info =			(SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
				 SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_MMAP_VALID),
	.formats =		/*SNDRV_PCM_FMTBIT_U8 |*/ SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_3LE,
	.rates =		SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_48000,
	.rate_min =		5000,
	.rate_max =		48000,
	.channels_min =		2,
	.channels_max =		2,
	.buffer_bytes_max =	(128*1024),
	.period_bytes_min =	126,
	.period_bytes_max =	(128*1024),
	.periods_min =		2,
	.periods_max =		VX_MAX_PERIODS,
	.fifo_size =		126,
};


static void vx_pcm_delayed_start(unsigned long arg);

/*
 * vx_pcm_playback_open - open callback for playback
 */
static int vx_pcm_playback_open(snd_pcm_substream_t *subs)
{
	snd_pcm_runtime_t *runtime = subs->runtime;
	vx_core_t *chip = snd_pcm_substream_chip(subs);
	vx_pipe_t *pipe;
	int audio, err;

	if (chip->is_stale)
		return -EBUSY;

	audio = subs->pcm->device * 2;
	snd_assert(audio < chip->audio_outs, return -EINVAL);
	err = vx_alloc_pipe(chip, 0, audio, 2, &pipe); /* stereo capture */
	if (err < 0)
		return err;
	pipe->substream = subs;
	tasklet_init(&pipe->start_tq, vx_pcm_delayed_start, (unsigned long)subs);
	chip->playback_pipes[audio] = pipe;

	runtime->hw = vx_pcm_playback_hw;
	runtime->hw.period_bytes_min = chip->ibl.size;
	runtime->private_data = pipe;

	return 0;
}

/*
 * vx_pcm_playback_close - close callback for playback
 */
static int vx_pcm_playback_close(snd_pcm_substream_t *subs)
{
	vx_core_t *chip = snd_pcm_substream_chip(subs);
	vx_pipe_t *pipe;

	if (! subs->runtime->private_data)
		return -EINVAL;
	pipe = snd_magic_cast(vx_pipe_t, subs->runtime->private_data, return -EINVAL);
	chip->playback_pipes[pipe->number] = 0;
	vx_free_pipe(chip, pipe);
	return 0;
}


/*
 * vx_notify_end_of_buffer - send "end-of-buffer" notifier at the given pipe
 * @pipe: the pipe to notify
 *
 * NB: call with a certain lock.
 */
static int vx_notify_end_of_buffer(vx_core_t *chip, vx_pipe_t *pipe)
{
	int err;
	struct vx_rmh rmh;  /* use a temporary rmh here */

	/* Toggle Dsp Host Interface into Message mode */
	vx_send_rih_nolock(chip, IRQ_PAUSE_START_CONNECT);
	vx_init_rmh(&rmh, CMD_NOTIFY_END_OF_BUFFER);
	vx_set_stream_cmd_params(&rmh, 0, pipe->number);
	err = vx_send_msg_nolock(chip, &rmh);
	if (err < 0)
		return err;
	/* Toggle Dsp Host Interface back to sound transfer mode */
	vx_send_rih_nolock(chip, IRQ_PAUSE_START_CONNECT);
	return 0;
}

/*
 * vx_update_playback_buffer - update the playback buffer
 * @subs: substream
 * @pipe: the pipe to transfer
 *
 * transfer as much data as possible.
 * called from the interrupt handler, too.
 *
 * return 0 if ok.
 */
static int vx_update_playback_buffer(vx_core_t *chip, snd_pcm_runtime_t *runtime, vx_pipe_t *pipe)
{
	int size, space, err = 0;

	size = (int)runtime->control->appl_ptr - pipe->hw_ptr;
	if (size < 0)
		size += runtime->buffer_size;
	size = frames_to_bytes(runtime, size);
	if (! size) /* nothing to copy */
		return 0;
	space = vx_query_hbuffer_size(chip, pipe);
	if (space < 0) {
		/* disconnect the host, SIZE_HBUF command always switches to the stream mode */
		vx_send_rih(chip, IRQ_CONNECT_STREAM_NEXT);
		return space;
	}
	/* we don't need irqsave here, because this function
	 * is called from either trigger callback or irq handler
	 */
	spin_lock(&chip->lock); 
	if (space > size)
		space = size;
	space = (space / pipe->align) * pipe->align;
	if (! space)
		goto _finish;
	size = bytes_to_frames(runtime, space);
	vx_pseudo_dma_write(chip, runtime, pipe, space);
	pipe->chunk_transferred += size;
	if (pipe->chunk_transferred >= chip->ibl.min_size) {
		pipe->chunk_transferred = 0;
		err = vx_notify_end_of_buffer(chip, pipe);
	}
 _finish:
	/* disconnect the host, SIZE_HBUF command always switches to the stream mode */
	vx_send_rih_nolock(chip, IRQ_CONNECT_STREAM_NEXT);
	spin_unlock(&chip->lock);
	return err;
}

/*
 * update the position of the given pipe.
 * pipe->position is updated and wrapped within the buffer size.
 * pipe->transferred is updated, too, but the size is not wrapped,
 * so that the caller can check the total transferred size later
 * (to call snd_pcm_period_elapsed).
 */
static int vx_update_pipe_position(vx_core_t *chip, snd_pcm_runtime_t *runtime, vx_pipe_t *pipe)
{
	struct vx_rmh rmh;
	int err, update;
	u64 count;

	vx_init_rmh(&rmh, CMD_STREAM_SAMPLE_COUNT);
	vx_set_pipe_cmd_params(&rmh, pipe->is_capture, pipe->number, 0);
	err = vx_send_msg(chip, &rmh);
	if (err < 0)
		return err;

	count = ((u64)(rmh.Stat[0] & 0xfffff) << 24) | (u64)rmh.Stat[1];
	update = (int)(count - pipe->cur_count);
	pipe->cur_count = count;
	pipe->position += update;
	if (pipe->position >= runtime->buffer_size)
		pipe->position %= runtime->buffer_size;
	pipe->transferred += update;
	return 0;
}

/*
 * transfer the pending playback buffer data to DSP
 * called from interrupt handler
 */
void vx_pcm_playback_update_buffer(vx_core_t *chip, snd_pcm_substream_t *subs, vx_pipe_t *pipe)
{
	int err;
	snd_pcm_runtime_t *runtime = subs->runtime;

	if (! pipe->prepared || chip->is_stale)
		return;
	if ((err = vx_update_playback_buffer(chip, runtime, pipe)) < 0)
		return;
}

/*
 * update the playback position and call snd_pcm_period_elapsed() if necessary
 * called from interrupt handler
 */
void vx_pcm_playback_update(vx_core_t *chip, snd_pcm_substream_t *subs, vx_pipe_t *pipe)
{
	int err;
	snd_pcm_runtime_t *runtime = subs->runtime;

	if (pipe->running && ! chip->is_stale) {
		if ((err = vx_update_pipe_position(chip, runtime, pipe)) < 0)
			return;
		if (pipe->transferred >= runtime->period_size) {
			pipe->transferred %= runtime->period_size;
			snd_pcm_period_elapsed(subs);
		}
	}
}

/*
 * start the stream and pipe.
 * this function is called from tasklet, which is invoked by the trigger
 * START callback.
 */
static void vx_pcm_delayed_start(unsigned long arg)
{
	snd_pcm_substream_t *subs = (snd_pcm_substream_t *)arg;
	vx_core_t *chip = snd_magic_cast(vx_core_t, subs->pcm->private_data, return);
	vx_pipe_t *pipe = snd_magic_cast(vx_pipe_t, subs->runtime->private_data, return);
	int err;

	if ((err = vx_start_stream(chip, pipe)) < 0) {
		snd_printk(KERN_ERR "vx: cannot start stream\n");
		return;
	}
	if ((err = vx_toggle_pipe(chip, pipe, 1)) < 0) {
		snd_printk(KERN_ERR "vx: cannot start pipe\n");
		return;
	}
}

/*
 * vx_pcm_playback_trigger - trigger callback for playback
 */
static int vx_pcm_trigger(snd_pcm_substream_t *subs, int cmd)
{
	vx_core_t *chip = snd_pcm_substream_chip(subs);
	vx_pipe_t *pipe = snd_magic_cast(vx_pipe_t, subs->runtime->private_data, return -EINVAL);
	int err;

	if (chip->is_stale)
		return -EBUSY;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		if (! pipe->is_capture)
			vx_update_playback_buffer(chip, subs->runtime, pipe);
		/* FIXME:
		 * we trigger the pipe using tasklet, so that the interrupts are
		 * issued surely after the trigger is completed.
		 */ 
		tasklet_hi_schedule(&pipe->start_tq);
		chip->pcm_running++;
		pipe->running = 1;
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		vx_toggle_pipe(chip, pipe, 0);
		vx_stop_pipe(chip, pipe);
		vx_stop_stream(chip, pipe);
		chip->pcm_running--;
		pipe->running = 0;
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if ((err = vx_toggle_pipe(chip, pipe, 0)) < 0)
			return err;
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (! pipe->is_capture)
			vx_update_playback_buffer(chip, subs->runtime, pipe);
		if ((err = vx_toggle_pipe(chip, pipe, 1)) < 0)
			return err;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/*
 * vx_pcm_playback_pointer - pointer callback for playback
 */
static snd_pcm_uframes_t vx_pcm_playback_pointer(snd_pcm_substream_t *subs)
{
	snd_pcm_runtime_t *runtime = subs->runtime;
	vx_pipe_t *pipe = snd_magic_cast(vx_pipe_t, runtime->private_data, return -EINVAL);
	return pipe->position;
}

/*
 * vx_pcm_hw_params - hw_params callback for playback and capture
 */
static int vx_pcm_hw_params(snd_pcm_substream_t *subs,
				     snd_pcm_hw_params_t *hw_params)
{
	return snd_pcm_alloc_vmalloc_buffer(subs, params_buffer_bytes(hw_params));
}

/*
 * vx_pcm_hw_free - hw_free callback for playback and capture
 */
static int vx_pcm_hw_free(snd_pcm_substream_t *subs)
{
	return snd_pcm_free_vmalloc_buffer(subs);
}

/*
 * vx_pcm_prepare - prepare callback for playback and capture
 */
static int vx_pcm_prepare(snd_pcm_substream_t *subs)
{
	vx_core_t *chip = snd_pcm_substream_chip(subs);
	snd_pcm_runtime_t *runtime = subs->runtime;
	vx_pipe_t *pipe = snd_magic_cast(vx_pipe_t, runtime->private_data, return -EINVAL);
	int err, data_mode;
	// int max_size, nchunks;

	if (chip->is_stale)
		return -EBUSY;

	data_mode = (chip->uer_bits & IEC958_AES0_NONAUDIO) != 0;
	if (data_mode != pipe->data_mode && ! pipe->is_capture) {
		/* IEC958 status (raw-mode) was changed */
		/* we reopen the pipe */
		struct vx_rmh rmh;
		snd_printdd(KERN_DEBUG "reopen the pipe with data_mode = %d\n", data_mode);
		vx_init_rmh(&rmh, CMD_FREE_PIPE);
		vx_set_pipe_cmd_params(&rmh, 0, pipe->number, 0);
		if ((err = vx_send_msg(chip, &rmh)) < 0)
			return err;
		vx_init_rmh(&rmh, CMD_RES_PIPE);
		vx_set_pipe_cmd_params(&rmh, 0, pipe->number, pipe->channels);
		if (data_mode)
			rmh.Cmd[0] |= BIT_DATA_MODE;
		if ((err = vx_send_msg(chip, &rmh)) < 0)
			return err;
		pipe->data_mode = data_mode;
	}

	if (chip->pcm_running && chip->freq != runtime->rate) {
		snd_printk(KERN_ERR "vx: cannot set different clock %d from the current %d\n", runtime->rate, chip->freq);
		return -EINVAL;
	}
	vx_set_clock(chip, runtime->rate);

	if ((err = vx_set_format(chip, pipe, runtime)) < 0)
		return err;

	if (chip->type >= VX_TYPE_VXPOCKET) {
		/* minimal DMA alignment = 6 */
		if (snd_pcm_format_physical_width(runtime->format) == 16)
			pipe->align = runtime->channels * 6;
		else
			pipe->align = 6;
	} else {
		/* minimal DMA alignment = 12 */
		pipe->align = 12;
	}

	pipe->buffer_bytes = frames_to_bytes(runtime, runtime->buffer_size);
	pipe->hw_ptr = 0;
	pipe->chunk_transferred = 0;

	/* set the timestamp */
	vx_update_pipe_position(chip, runtime, pipe);
	/* clear again */
	pipe->transferred = 0;
	pipe->position = 0;

	pipe->prepared = 1;

	return 0;
}


/*
 * operators for PCM playback
 */
static snd_pcm_ops_t vx_pcm_playback_ops = {
	.open =		vx_pcm_playback_open,
	.close =	vx_pcm_playback_close,
	.ioctl =	snd_pcm_lib_ioctl,
	.hw_params =	vx_pcm_hw_params,
	.hw_free =	vx_pcm_hw_free,
	.prepare =	vx_pcm_prepare,
	.trigger =	vx_pcm_trigger,
	.pointer =	vx_pcm_playback_pointer,
	.page =		snd_pcm_get_vmalloc_page,
};


/*
 * playback hw information
 */

static snd_pcm_hardware_t vx_pcm_capture_hw = {
	.info =			(SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
				 SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_MMAP_VALID),
	.formats =		/*SNDRV_PCM_FMTBIT_U8 |*/ SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_3LE,
	.rates =		SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_48000,
	.rate_min =		5000,
	.rate_max =		48000,
	.channels_min =		2,
	.channels_max =		2,
	.buffer_bytes_max =	(128*1024),
	.period_bytes_min =	126,
	.period_bytes_max =	(128*1024),
	.periods_min =		2,
	.periods_max =		VX_MAX_PERIODS,
	.fifo_size =		126,
};


/*
 * vx_pcm_capture_open - open callback for capture
 */
static int vx_pcm_capture_open(snd_pcm_substream_t *subs)
{
	snd_pcm_runtime_t *runtime = subs->runtime;
	vx_core_t *chip = snd_pcm_substream_chip(subs);
	vx_pipe_t *pipe;
	int audio, err;

	if (chip->is_stale)
		return -EBUSY;

	audio = subs->pcm->device * 2;
	snd_assert(audio < chip->audio_ins, return -EINVAL);
	err = vx_alloc_pipe(chip, 1, audio, 2, &pipe);
	if (err < 0)
		return err;
	pipe->substream = subs;
	tasklet_init(&pipe->start_tq, vx_pcm_delayed_start, (unsigned long)subs);
	chip->capture_pipes[audio] = pipe;

	runtime->hw = vx_pcm_capture_hw;
	runtime->hw.period_bytes_min = chip->ibl.size;
	runtime->private_data = pipe;

	return 0;
}

/*
 * vx_pcm_capture_close - close callback for capture
 */
static int vx_pcm_capture_close(snd_pcm_substream_t *subs)
{
	vx_core_t *chip = snd_pcm_substream_chip(subs);
	vx_pipe_t *pipe;

	if (! subs->runtime->private_data)
		return -EINVAL;
	pipe = snd_magic_cast(vx_pipe_t, subs->runtime->private_data, return -EINVAL);
	chip->capture_pipes[pipe->number] = 0;
	vx_free_pipe(chip, pipe);
	return 0;
}


/*
 * vx_pcm_capture_update - update the capture buffer
 */
void vx_pcm_capture_update(vx_core_t *chip, snd_pcm_substream_t *subs, vx_pipe_t *pipe)
{
	int size, space;
	snd_pcm_runtime_t *runtime = subs->runtime;

	if (! pipe->prepared || chip->is_stale)
		return;

	size = runtime->buffer_size - snd_pcm_capture_avail(runtime);
	if (! size)
		return;
	size = frames_to_bytes(runtime, size);
	space = vx_query_hbuffer_size(chip, pipe);
	if (space < 0) {
		/* disconnect the host, SIZE_HBUF command always switches to the stream mode */
		vx_send_rih(chip, IRQ_CONNECT_STREAM_NEXT);
		return;
	}
	if (size > space)
		size = space;
	size = (size / pipe->align) * pipe->align;
	if (! size)
		goto _finish;
#if 0
	if (size > pipe->align)
		vx_pseudo_dma_read(chip, runtime, pipe, size - pipe->align);
 _finish:
	/* disconnect the host, SIZE_HBUF command always switches to the stream mode */
	vx_send_rih_nolock(chip, IRQ_CONNECT_STREAM_NEXT);
	vx_flush_read(chip, runtime, pipe, pipe->align);
#else
	vx_pseudo_dma_read(chip, runtime, pipe, size);
 _finish:
	vx_send_rih_nolock(chip, IRQ_CONNECT_STREAM_NEXT);
#endif
	pipe->transferred += bytes_to_frames(runtime, size);
	if (pipe->transferred >= runtime->period_size) {
		pipe->transferred %= runtime->period_size;
		snd_pcm_period_elapsed(subs);
	}
}

/*
 * vx_pcm_capture_pointer - pointer callback for capture
 */
static snd_pcm_uframes_t vx_pcm_capture_pointer(snd_pcm_substream_t *subs)
{
	snd_pcm_runtime_t *runtime = subs->runtime;
	vx_pipe_t *pipe = snd_magic_cast(vx_pipe_t, runtime->private_data, return -EINVAL);
	return pipe->hw_ptr;
}

/*
 * operators for PCM capture
 */
static snd_pcm_ops_t vx_pcm_capture_ops = {
	.open =		vx_pcm_capture_open,
	.close =	vx_pcm_capture_close,
	.ioctl =	snd_pcm_lib_ioctl,
	.hw_params =	vx_pcm_hw_params,
	.hw_free =	vx_pcm_hw_free,
	.prepare =	vx_pcm_prepare,
	.trigger =	vx_pcm_trigger,
	.pointer =	vx_pcm_capture_pointer,
	.page =		snd_pcm_get_vmalloc_page,
};


/*
 * vx_init_audio_io - check the availabe audio i/o and allocate pipe arrays
 */
static int vx_init_audio_io(vx_core_t *chip)
{
	struct vx_rmh rmh;

	vx_init_rmh(&rmh, CMD_SUPPORTED);
	if (vx_send_msg(chip, &rmh) < 0) {
		snd_printk(KERN_ERR "vx: cannot get the supported audio data\n");
		return -ENXIO;
	}

	chip->audio_outs = rmh.Stat[0] & MASK_FIRST_FIELD;
	chip->audio_ins = (rmh.Stat[0] >> (FIELD_SIZE*2)) & MASK_FIRST_FIELD;
	chip->audio_info = rmh.Stat[1];

	/* allocate pipes */
	chip->playback_pipes = kmalloc(sizeof(vx_pipe_t *) * chip->audio_outs, GFP_KERNEL);
	chip->capture_pipes = kmalloc(sizeof(vx_pipe_t *) * chip->audio_ins, GFP_KERNEL);
	if (! chip->playback_pipes || ! chip->capture_pipes)
		return -ENOMEM;

	memset(chip->playback_pipes, 0, sizeof(vx_pipe_t *) * chip->audio_outs);
	memset(chip->capture_pipes, 0, sizeof(vx_pipe_t *) * chip->audio_ins);

	vx_set_ibl(chip, &chip->ibl); /* query the info */
	chip->ibl.size = chip->ibl.min_size;
	vx_set_ibl(chip, &chip->ibl); /* set to the minimum */

	return 0;
}


/*
 * free callback for pcm
 */
static void snd_vxpocket_pcm_free(snd_pcm_t *pcm)
{
	vx_core_t *chip = snd_magic_cast(vx_core_t, pcm->private_data, return);
	chip->pcm[pcm->device] = NULL;
	if (chip->playback_pipes) {
		kfree(chip->playback_pipes);
		chip->playback_pipes = 0;
	}
	if (chip->capture_pipes) {
		kfree(chip->capture_pipes);
		chip->capture_pipes = 0;
	}
}

/*
 * snd_vx_pcm_new - create and initialize a pcm
 */
int snd_vx_pcm_new(vx_core_t *chip)
{
	snd_pcm_t *pcm;
	int i, err;

	if ((err = vx_init_audio_io(chip)) < 0)
		return err;

	for (i = 0; i < chip->hw->num_codecs; i++) {
		int outs, ins;
		outs = chip->audio_outs > i * 2 ? 1 : 0;
		ins = chip->audio_ins > i * 2 ? 1 : 0;
		if (! outs && ! ins)
			break;
		err = snd_pcm_new(chip->card, "VXPocket", i,
				  outs, ins, &pcm);
		if (err < 0)
			return err;
		if (outs)
			snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &vx_pcm_playback_ops);
		if (ins)
			snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &vx_pcm_capture_ops);

		pcm->private_data = chip;
		pcm->private_free = snd_vxpocket_pcm_free;
		pcm->info_flags = 0;
		strcpy(pcm->name, chip->card->shortname);
		chip->pcm[i] = pcm;
	}

	return 0;
}


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,18)

static struct page *vmalloc_to_page(void *pageptr)
{
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;
	unsigned long lpage;
	struct page *page = (struct page *)NOPAGE_SIGBUS;

	lpage = VMALLOC_VMADDR(pageptr);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,18)
	spin_lock(&init_mm.page_table_lock);
#endif
	pgd = pgd_offset(&init_mm, lpage);
	pmd = pmd_offset(pgd, lpage);
	pte = pte_offset(pmd, lpage);
	page = (struct page *)pte_page(*pte);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,18)
	spin_unlock(&init_mm.page_table_lock);
#endif

	return page;
}    
#endif
