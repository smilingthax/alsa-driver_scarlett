/*
 *  Asihpi soundcard
 *  Copyright (c) by AudioScience Inc <alsa@audioscience.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of version 2 of the GNU General Public License as 
 *   published by the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *
 *  The following is not a condition of use, merely a request:
 *  If you modify this program, particularly if you fix errors, AudioScience Inc
 *  would appreciate it if you grant us the right to use those modifications
 *  for any purpose including commercial applications.
 */
#define REALLY_VERBOSE_LOGGING 0

// Mixer control
#ifndef ASI_STYLE_NAMES
#define ASI_STYLE_NAMES 1
#endif

#include <linux/pci.h>
#include <sound/driver.h>
#include <linux/version.h>
#include <linux/init.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 5, 0)
#include <linux/jiffies.h>
#else
#include <linux/sched.h>
#endif
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/info.h>
#include <sound/initval.h>

#include "hpi.h"

static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;	/* Index 0-MAX */
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;	/* ID for this card */
static int enable[SNDRV_CARDS] = SNDRV_DEFAULT_ENABLE_PNP;	/* Enable this card */

module_param_array(index, int, NULL, 0444);
MODULE_PARM_DESC(index, "ALSA Index value for AudioScience soundcard.");
module_param_array(id, charp, NULL, 0444);
MODULE_PARM_DESC(id, "ALSA ID string for AudioScience soundcard.");
module_param_array(enable, bool, NULL, 0444);
MODULE_PARM_DESC(enable, "ALSA Enable AudioScience soundcard.");

/* used by hpimod.c, stop sparse from complaining */
int snd_asihpi_bind(adapter_t * hpi_card);
void snd_asihpi_unbind(adapter_t * hpi_card);

static int mixer_dump = 0;

#define DEFAULT_SAMPLERATE 44100
static int adapter_fs = DEFAULT_SAMPLERATE;

static int enable_bbm = 1;

#define MAX_PCM_DEVICES	 4
#define MAX_PCM_SUBSTREAMS	16
#define MAX_MIDI_DEVICES	 0

/* defaults */
#ifndef MAX_BUFFER_SIZE
//#define MAX_BUFFER_SIZE               (64*1024)
#define MAX_BUFFER_SIZE		(256*1024)
#endif

#define TIMER_JIFFIES 15

#define MAX_CLOCKSOURCES (HPI_SAMPLECLOCK_SOURCE_LAST + 1 + 7)

struct clk_source {
	int source;
	int index;
	char *name;
};

struct clk_cache {
	int count;
	struct clk_source s[MAX_CLOCKSOURCES];
};

#define MAX_CLOCKSOURCES (HPI_SAMPLECLOCK_SOURCE_LAST + 1 + 7)

typedef struct snd_card_asihpi {
	struct snd_card *card;
//      spinlock_t mixer_lock;
	// ASI 
	u16 wAdapterIndex;
	u32 dwSerialNumber;
	u16 wType;
	u16 wVersion;
	u16 wNumOutStreams;
	u16 wNumInStreams;

	HPI_HMIXER hMixer;
	struct clk_cache cc;
} snd_card_asihpi_t;

typedef struct snd_card_asihpi_pcm {
	snd_card_asihpi_t *asihpi;
//      spinlock_t lock;
	struct timer_list timer;
	unsigned int respawn_timer;
	unsigned int hpi_in_buffer_allocated;
	unsigned int hpi_out_buffer_allocated;
	unsigned int pcm_size;
	unsigned int pcm_count;
	unsigned int pcm_jiffie_per_period;
	unsigned int pcm_irq_pos;	/* IRQ position */
	unsigned int pcm_buf_pos;	/* position in buffer */
	struct snd_pcm_substream *substream;
	HPI_HOSTREAM hStream;
	//HPI_DATA Data;
	HPI_FORMAT Format;
	unsigned int pcm_hpi_pos;	/* position in buffer */

} snd_card_asihpi_pcm_t;

static HPI_HSUBSYS *phSubSys;	/* handle to HPI audio subsystem */

static void _HandleError(u16 err, int line, char *filename)
{
	char ErrorText[80];

	if (err) {
		HPI_GetErrorText(err, ErrorText);
		printk(KERN_WARNING "in file %s, line %d: %s\n",
		       filename, line, ErrorText);
	}
}

#define HPI_HandleError(x)  _HandleError(x,__LINE__,__FILE__)
/***************************** GENERAL PCM ****************/
#if REALLY_VERBOSE_LOGGING
static void print_hwparams(struct snd_pcm_hw_params *p)
{
	snd_printd("HWPARAMS \n");
	snd_printd("samplerate %d \n", params_rate(p));
	snd_printd("Channels %d \n", params_channels(p));
	snd_printd("Format %d \n", params_format(p));
	snd_printd("subformat %d \n", params_subformat(p));
	snd_printd("Buffer bytes %d \n", params_buffer_bytes(p));
	snd_printd("Period bytes %d \n", params_period_bytes(p));
	snd_printd("access %d \n", params_access(p));
	snd_printd("period_size %d \n", params_period_size(p));
	snd_printd("periods %d \n", params_periods(p));
	snd_printd("buffer_size %d \n", params_buffer_size(p));
	snd_printd("tick_time %d \n", params_tick_time(p));
}
#endif

static int snd_card_asihpi_pcm_hw_params(struct snd_pcm_substream *substream,
					 struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	unsigned int bps;

#if REALLY_VERBOSE_LOGGING
	print_hwparams(params);
#endif

	bps = params_rate(params) * params_channels(params);
	bps *= snd_pcm_format_width(params_format(params));
	bps /= 8;
	if (bps <= 0)
		return -EINVAL;
	dpcm->pcm_size = params_buffer_bytes(params);
	dpcm->pcm_count = params_period_bytes(params);
	dpcm->pcm_jiffie_per_period = (dpcm->pcm_count * HZ / bps);
	snd_printd("Jiffies per period %ld, pcm_size %d, pcm_count %d\n",
		   (long)dpcm->pcm_jiffie_per_period, dpcm->pcm_size,
		   dpcm->pcm_count);
	dpcm->pcm_irq_pos = 0;
	dpcm->pcm_buf_pos = 0;
	return 0;
}

static void snd_card_asihpi_pcm_timer_start(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;

	/* wait longer the first time, for samples to propagate */
	dpcm->timer.expires =
	    jiffies + dpcm->pcm_jiffie_per_period +
	    dpcm->pcm_jiffie_per_period / 4;
	dpcm->respawn_timer = 1;
	add_timer(&dpcm->timer);
}

static void snd_card_asihpi_pcm_timer_stop(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;

	dpcm->respawn_timer = 0;
	del_timer(&dpcm->timer);
}

static void snd_card_asihpi_runtime_free(struct snd_pcm_runtime *runtime)
{
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	kfree(dpcm);
}

static snd_pcm_format_t hpi_to_alsa_formats[] = {
	-1,			/* INVALID */
	SNDRV_PCM_FORMAT_U8,	/* { HPI_FORMAT_PCM8_UNSIGNED        1 */
	SNDRV_PCM_FORMAT_S16,	/* { HPI_FORMAT_PCM16_SIGNED         2 */
	-1,			/* { HPI_FORMAT_MPEG_L1              3 */
	SNDRV_PCM_FORMAT_MPEG,	/* { HPI_FORMAT_MPEG_L2              4 */
	SNDRV_PCM_FORMAT_MPEG,	/* { HPI_FORMAT_MPEG_L3              5 */
	-1,			/* { HPI_FORMAT_DOLBY_AC2            6 */
	-1,			/* { HPI_FORMAT_DOLBY_AC3            7 */
	SNDRV_PCM_FORMAT_S16_BE,	/* { HPI_FORMAT_PCM16_BIGENDIAN      8 */
	-1,			/* { HPI_FORMAT_AA_TAGIT1_HITS       9 */
	-1,			/* { HPI_FORMAT_AA_TAGIT1_INSERTS   10 */
	SNDRV_PCM_FORMAT_S32,	/* { HPI_FORMAT_PCM32_SIGNED        11 */
	-1,			/* { HPI_FORMAT_RAW_BITSTREAM       12 */
	-1,			/* { HPI_FORMAT_AA_TAGIT1_HITS_EX1  13 */
	SNDRV_PCM_FORMAT_FLOAT,	/* { HPI_FORMAT_PCM32_FLOAT             14 */
	SNDRV_PCM_FORMAT_S24	/* { HPI_FORMAT_PCM24_SIGNED        15 */
};

static int snd_card_asihpi_format_alsa2hpi(snd_pcm_format_t alsaFormat,
					   u16 * hpiFormat)
{
	u16 wFormat;

	for (wFormat = HPI_FORMAT_PCM8_UNSIGNED;
	     wFormat <= HPI_FORMAT_PCM24_SIGNED; wFormat++) {
		if (hpi_to_alsa_formats[wFormat] == alsaFormat) {
			*hpiFormat = wFormat;
			// snd_printd(KERN_INFO "Matched alsa format %d to asi format %d\n",alsaFormat,wFormat);
			return 0;
		}
	}

	snd_printd(KERN_WARNING "Failed match for alsa format %d\n",
		   alsaFormat);
	*hpiFormat = 0;
	return -EINVAL;
}

/***************************** PLAYBACK OPS ****************/
static void snd_card_asihpi_playback_timer_function(unsigned long data)
{
	snd_card_asihpi_pcm_t *dpcm = (snd_card_asihpi_pcm_t *) data;
	struct snd_pcm_runtime *runtime = dpcm->substream->runtime;

	unsigned int pos;
	int delta;
	u16 wState, err;
	u32 dwBufferSize;
	u32 dwDataToPlay;
	u32 dwSamplesPlayed;

	dpcm->timer.expires = dpcm->pcm_jiffie_per_period + jiffies;

	err = HPI_OutStreamGetInfoEx(phSubSys, dpcm->hStream, &wState,
				     &dwBufferSize, &dwDataToPlay,
				     &dwSamplesPlayed, NULL);
	HPI_HandleError(err);
#if REALLY_VERBOSE_LOGGING
	snd_printd(KERN_DEBUG
		   "Playback timer state=%d, played=%d, left=%d\n", wState,
		   (int)dwSamplesPlayed, (int)dwDataToPlay);
#endif

	if ((wState == HPI_STATE_DRAINED))
		// pretend to keep moving, so alsa thinks all its data has been consumed.
		pos = dpcm->pcm_buf_pos + dpcm->pcm_count;
	else
		pos = frames_to_bytes(runtime, dwSamplesPlayed);

	pos %= dpcm->pcm_size;
	delta = pos - dpcm->pcm_buf_pos;
	if (delta < 0)
		delta += dpcm->pcm_size;
	dpcm->pcm_irq_pos += delta;
	dpcm->pcm_buf_pos = pos;

	if (dpcm->respawn_timer)
		add_timer(&dpcm->timer);

	if (dpcm->pcm_irq_pos >= dpcm->pcm_count) {
		dpcm->pcm_irq_pos %= dpcm->pcm_count;
		snd_pcm_period_elapsed(dpcm->substream);
	}
}

static int snd_card_asihpi_playback_ioctl(struct snd_pcm_substream *substream,
					  unsigned int cmd, void *arg)
{
	snd_printd(KERN_INFO "playback ioctl %d\n", cmd);
	return snd_pcm_lib_ioctl(substream, cmd, arg);
}

static int snd_card_asihpi_playback_trigger(struct snd_pcm_substream *substream,
					    int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;

	snd_printd(KERN_INFO "playback trigger\n");
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		snd_printd("\tstart\n");
		HPI_HandleError(HPI_OutStreamStart(phSubSys, dpcm->hStream));
		snd_card_asihpi_pcm_timer_start(substream);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		snd_printd("\tstop\n");
		snd_card_asihpi_pcm_timer_stop(substream);
		HPI_HandleError(HPI_OutStreamReset(phSubSys, dpcm->hStream));
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		snd_printd("\tpause release\n");
		HPI_HandleError(HPI_OutStreamStart(phSubSys, dpcm->hStream));
		snd_card_asihpi_pcm_timer_start(substream);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		snd_printd("\tpause push\n");
		snd_card_asihpi_pcm_timer_stop(substream);
		HPI_HandleError(HPI_OutStreamStop(phSubSys, dpcm->hStream));
		break;
	default:
		snd_printd("\tINVALID\n");
		return -EINVAL;
	}
	return 0;
}

static int snd_card_asihpi_playback_hw_params(struct snd_pcm_substream
					      *substream,
					      struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	int err;
	u16 wFormat;
	u32 minBufSize;

	err = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
	if (err < 0)
		return err;

	err = snd_card_asihpi_format_alsa2hpi(params_format(params), &wFormat);
	if (err != 0)
		return err;

	err = HPI_FormatCreate(&dpcm->Format, params_channels(params),
			       wFormat, params_rate(params), 0, 0);
	HPI_HandleError(err);

	if (enable_bbm) {
		dpcm->hpi_out_buffer_allocated = 0;
		minBufSize = 0;

		err = HPI_StreamEstimateBufferSize(&dpcm->Format,
						   (u32) (1000 / HZ),
						   &minBufSize);
		snd_printd(KERN_INFO
			   "OutStreamHostBuffer estimated size: %d bytes\n",
			   (int)minBufSize);

		if (minBufSize < params_buffer_bytes(params))	/* paranoia check */
			minBufSize = params_buffer_bytes(params);

		// Need to request 4 bytes extra, because buffer is never allowed
		// to become completely full.
		// ? convert buffers to lockfree like kfifo?
		minBufSize += 4;

		if (err == 0) {
			err = HPI_OutStreamHostBufferAllocate(phSubSys,
							      dpcm->hStream,
							      minBufSize);
			if (err == 0) {
				dpcm->hpi_out_buffer_allocated = minBufSize;
				snd_printd(KERN_INFO
					   "OutStreamHostBufferAllocate succeded\n");
			} else if (err != 0 && err != HPI_ERROR_INVALID_FUNC) {
				snd_printd(KERN_INFO
					   "OutStreamHostBufferAllocate error(%d)\n",
					   err);
				return err;
			}
		} else {
			snd_printd(KERN_INFO
				   "Cannot estimate minimum bus master buffer size\n");
		}
		snd_printd(KERN_INFO
			   "OutStreamHostBufferAllocate status(%d)\n",
			   dpcm->hpi_out_buffer_allocated);
	}

	snd_printd(KERN_INFO "playback hwparams\n");

	return snd_card_asihpi_pcm_hw_params(substream, params);
}

static int snd_card_asihpi_playback_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	if (dpcm->hpi_out_buffer_allocated) {
		HPI_OutStreamHostBufferFree(phSubSys, dpcm->hStream);
	}

	snd_pcm_lib_free_pages(substream);
	return 0;
}

static int snd_card_asihpi_playback_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	int err;

	snd_printd(KERN_INFO "playback prepare\n");

	err = HPI_OutStreamReset(phSubSys, dpcm->hStream);
	HPI_HandleError(err);
	dpcm->pcm_irq_pos = 0;
	dpcm->pcm_buf_pos = 0;

	return 0;
}

static snd_pcm_uframes_t
snd_card_asihpi_playback_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;

	return bytes_to_frames(runtime, dpcm->pcm_buf_pos);
}

/* need to dynamically determine the rate range per card */
static struct snd_pcm_hardware snd_card_asihpi_playback = {
	.info = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_PAUSE,
	.rates = SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_192000,
	.rate_min = 5500,
	.rate_max = 192000,
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = MAX_BUFFER_SIZE,
	.period_bytes_min = MAX_BUFFER_SIZE / 64,
	.period_bytes_max = MAX_BUFFER_SIZE / 2,
	.periods_min = 2,
	.periods_max = 64,
	.fifo_size = 0,
};

static void snd_card_asihpi_playback_format(snd_card_asihpi_t * asihpi,
					    HPI_HOSTREAM hStream, u64 * formats)
{
	HPI_FORMAT hpi_format;
	u16 wFormat;
	u16 err;
	HPI_HCONTROL hControl;

	/* on cards without SRC, must query at valid rate, maybe set by external sync */
	err = HPI_MixerGetControl(phSubSys, asihpi->hMixer,
				  HPI_SOURCENODE_CLOCK_SOURCE, 0, 0, 0,
				  HPI_CONTROL_SAMPLECLOCK, &hControl);

	if (!err) {
		err = HPI_SampleClock_GetSampleRate(phSubSys, hControl,
						    &hpi_format.dwSampleRate);
	}

	for (wFormat = HPI_FORMAT_PCM8_UNSIGNED;
	     wFormat <= HPI_FORMAT_PCM24_SIGNED; wFormat++) {
		HPI_FormatCreate(&hpi_format, 2, wFormat, 48000, 128000, 0);
		err = HPI_OutStreamQueryFormat(phSubSys, hStream, &hpi_format);
		if (!err && (hpi_to_alsa_formats[wFormat] != -1)) {
			*formats |= (1ULL << hpi_to_alsa_formats[wFormat]);
			//snd_printk(KERN_INFO "HPI format %d, alsa %d\n",wFormat,hpi_to_alsa_formats[wFormat]);
		}
	}
}

static int snd_card_asihpi_playback_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm;
	snd_card_asihpi_t *asihpi = snd_pcm_substream_chip(substream);
	u16 err;

	dpcm = kzalloc(sizeof(*dpcm), GFP_KERNEL);
	if (dpcm == NULL)
		return -ENOMEM;

	err =
	    HPI_OutStreamOpen(phSubSys, asihpi->wAdapterIndex,
			      substream->number, &dpcm->hStream);
	HPI_HandleError(err);
	if (err)
		kfree(dpcm);
	if (err == HPI_ERROR_OBJ_ALREADY_OPEN)
		return -EBUSY;
	if (err)
		return -EIO;

	snd_card_asihpi_playback_format(asihpi, dpcm->hStream,
					&snd_card_asihpi_playback.formats);

	/*? also check ASI5000 samplerate source
	   If external, only support external rate.
	   If interneal and other stream playing, cant switch
	 */

	init_timer(&dpcm->timer);
	dpcm->timer.data = (unsigned long)dpcm;
	dpcm->timer.function = snd_card_asihpi_playback_timer_function;
//      spin_lock_init(&dpcm->lock);
	dpcm->substream = substream;
	runtime->private_data = dpcm;
	runtime->private_free = snd_card_asihpi_runtime_free;
	runtime->hw = snd_card_asihpi_playback;

	snd_printd(KERN_INFO "playback open\n");

	return 0;
}

static int snd_card_asihpi_playback_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	u16 err;

	err = HPI_OutStreamClose(phSubSys, dpcm->hStream);
	HPI_HandleError(err);

	snd_printd(KERN_INFO "playback close\n");

	return 0;
}

static int snd_card_asihpi_playback_copy(struct snd_pcm_substream *substream,
					 int channel,
					 snd_pcm_uframes_t pos,
					 void __user * src,
					 snd_pcm_uframes_t count)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	u16 err;
	unsigned int len;

	len = frames_to_bytes(runtime, count);

	if (copy_from_user(runtime->dma_area, src, len))
		return -EFAULT;

#if REALLY_VERBOSE_LOGGING
	snd_printd(KERN_DEBUG "playback_copy %u bytes\n", len);
#endif

	err =
	    HPI_OutStreamWriteBuf(phSubSys, dpcm->hStream, runtime->dma_area,
				  len, &dpcm->Format);
	HPI_HandleError(err);

	return 0;
}

static int snd_card_asihpi_playback_silence(struct snd_pcm_substream *substream,
					    int channel, snd_pcm_uframes_t pos,
					    snd_pcm_uframes_t count)
{
	unsigned int len;
	u16 err;
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;

	len = frames_to_bytes(runtime, count);
	snd_printd(KERN_INFO "playback silence  %u bytes\n", len);

	memset(runtime->dma_area, 0, len);
	err =
	    HPI_OutStreamWriteBuf(phSubSys, dpcm->hStream, runtime->dma_area,
				  len, &dpcm->Format);
	HPI_HandleError(err);
	return 0;
}

static struct snd_pcm_ops snd_card_asihpi_playback_ops = {
	.open = snd_card_asihpi_playback_open,
	.close = snd_card_asihpi_playback_close,
	.ioctl = snd_card_asihpi_playback_ioctl,
	.hw_params = snd_card_asihpi_playback_hw_params,
	.hw_free = snd_card_asihpi_playback_hw_free,
	.prepare = snd_card_asihpi_playback_prepare,
	.trigger = snd_card_asihpi_playback_trigger,
	.pointer = snd_card_asihpi_playback_pointer,
	.copy = snd_card_asihpi_playback_copy,
	.silence = snd_card_asihpi_playback_silence,
};

/***************************** CAPTURE OPS ****************/
static void snd_card_asihpi_capture_timer_function(unsigned long data)
{
	snd_card_asihpi_pcm_t *dpcm = (snd_card_asihpi_pcm_t *) data;
	u16 wState, err;
	u32 dwBufferSize, dwDataToPlay, dwSamplesPlayed;

	err =
	    HPI_InStreamGetInfoEx(phSubSys, dpcm->hStream, &wState,
				  &dwBufferSize, &dwDataToPlay,
				  &dwSamplesPlayed, NULL);
	HPI_HandleError(err);

	/* Used by capture_pointer */
	dpcm->pcm_buf_pos = (dpcm->pcm_irq_pos + dwDataToPlay) % dpcm->pcm_size;
#if REALLY_VERBOSE_LOGGING
	snd_printd("Capture timer %d samples, %d left, pos %d\n",
		   (int)dwSamplesPlayed, (int)dwDataToPlay, dpcm->pcm_buf_pos);
#endif

	/* Future: if data will be left over, make timer expire sooner ? */
	dpcm->timer.expires = dpcm->pcm_jiffie_per_period + jiffies;
	if (dpcm->respawn_timer)
		add_timer(&dpcm->timer);

	if (dwDataToPlay >= dpcm->pcm_count) {
		snd_pcm_period_elapsed(dpcm->substream);
	}
}

static int snd_card_asihpi_capture_ioctl(struct snd_pcm_substream *substream,
					 unsigned int cmd, void *arg)
{
	return snd_pcm_lib_ioctl(substream, cmd, arg);
}

static int snd_card_asihpi_capture_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;

	HPI_HandleError(HPI_InStreamReset(phSubSys, dpcm->hStream));
	dpcm->pcm_irq_pos = 0;
	dpcm->pcm_buf_pos = 0;

	snd_printd("Capture Prepare\n");
	return 0;
}

static int snd_card_asihpi_capture_hw_params(struct snd_pcm_substream
					     *substream,
					     struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	int err;
	u16 wFormat;
	u32 minBufSize;

	if ((err =
	     snd_pcm_lib_malloc_pages(substream,
				      params_buffer_bytes(params))) < 0)
		return err;
	if ((err =
	     snd_card_asihpi_format_alsa2hpi(params_format(params),
					     &wFormat)) != 0)
		return err;

#if REALLY_VERBOSE_LOGGING
	snd_printd(KERN_INFO "Format %d chan, %d format, %dHz\n ",
		   params_channels(params), wFormat, params_rate(params));
#endif
	err =
	    HPI_FormatCreate(&dpcm->Format, params_channels(params),
			     wFormat, params_rate(params), 0, 0);
	HPI_HandleError(err);

	if (HPI_InStreamReset(phSubSys, dpcm->hStream) != 0)
		return -EINVAL;

	if (HPI_InStreamSetFormat(phSubSys, dpcm->hStream, &dpcm->Format) != 0)
		return -EINVAL;

	if (enable_bbm) {
		dpcm->hpi_in_buffer_allocated = 0;
		minBufSize = 0;

		err =
		    HPI_StreamEstimateBufferSize(&dpcm->Format,
						 (u32) (1000 / HZ),
						 &minBufSize);
		snd_printd(KERN_INFO
			   "InStreamHostBuffer estimated size: %d bytes\n",
			   (int)minBufSize);

		if (minBufSize < params_buffer_bytes(params))	/* paranoia check */
			minBufSize = params_buffer_bytes(params);

		if (err == 0) {
			err =
			    HPI_InStreamHostBufferAllocate(phSubSys,
							   dpcm->hStream,
							   minBufSize);
			if (err == 0) {
				dpcm->hpi_in_buffer_allocated = minBufSize;
				snd_printd(KERN_INFO
					   "InStreamHostBufferAllocate succeded\n");
			} else if (err != 0 && err != HPI_ERROR_INVALID_FUNC) {
				snd_printd(KERN_INFO
					   "InStreamHostBufferAllocate error(%d)\n",
					   err);
				return err;
			}
		} else {
			snd_printd(KERN_INFO
				   "Cannot estimate minimum bus master buffer size\n");
		}
		snd_printd(KERN_INFO "InStreamHostBufferAllocate status(%d)\n",
			   dpcm->hpi_in_buffer_allocated);
	}

	snd_printd(KERN_INFO "Capture hwparams\n");

	return snd_card_asihpi_pcm_hw_params(substream, params);
}

static int snd_card_asihpi_capture_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	if (dpcm->hpi_in_buffer_allocated) {
		HPI_InStreamHostBufferFree(phSubSys, dpcm->hStream);
	}

	snd_pcm_lib_free_pages(substream);
	return 0;
}

static int snd_card_asihpi_capture_trigger(struct snd_pcm_substream *substream,
					   int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		snd_printd("Capture Trigger Start\n");
		HPI_HandleError(HPI_InStreamStart(phSubSys, dpcm->hStream));
		snd_card_asihpi_pcm_timer_start(substream);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		snd_printd("Capture Trigger Stop\n");
		snd_card_asihpi_pcm_timer_stop(substream);
		/*???? should this be stream reset???  Prepare and hwparams do reset */
		HPI_HandleError(HPI_InStreamStop(phSubSys, dpcm->hStream));
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		snd_printd("Capture Trigger Pause release\n");
		HPI_HandleError(HPI_InStreamStart(phSubSys, dpcm->hStream));
		snd_card_asihpi_pcm_timer_start(substream);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		snd_printd("Capture Trigger Pause\n");
		snd_card_asihpi_pcm_timer_stop(substream);
		HPI_HandleError(HPI_InStreamStop(phSubSys, dpcm->hStream));
		break;
	default:
		snd_printd("\tINVALID\n");
		return -EINVAL;
	}

	return 0;
}

static snd_pcm_uframes_t
snd_card_asihpi_capture_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;

#if REALLY_VERBOSE_LOGGING
	snd_printd("Capture pointer %d\n", dpcm->pcm_buf_pos);
#endif
	return bytes_to_frames(runtime, dpcm->pcm_buf_pos);
}

static struct snd_pcm_hardware snd_card_asihpi_capture = {
	.info = SNDRV_PCM_INFO_INTERLEAVED,
	.formats = SNDRV_PCM_FMTBIT_S16,
	.rates = SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_192000,
	.rate_min = 5500,
	.rate_max = 192000,
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = MAX_BUFFER_SIZE,
	.period_bytes_min = MAX_BUFFER_SIZE / 64,
	.period_bytes_max = MAX_BUFFER_SIZE / 4,
	.periods_min = 2,
	.periods_max = 64,
	.fifo_size = 0,
};

static void snd_card_asihpi_capture_format(snd_card_asihpi_t * asihpi,
					   HPI_HOSTREAM hStream, u64 * formats)
{
	HPI_FORMAT hpi_format;
	u16 wFormat;
	u16 err;
	HPI_HCONTROL hControl;

	/* on cards without SRC, must query at valid rate, 
	   maybe set by external sync */
	err = HPI_MixerGetControl(phSubSys, asihpi->hMixer,
				  HPI_SOURCENODE_CLOCK_SOURCE, 0, 0, 0,
				  HPI_CONTROL_SAMPLECLOCK, &hControl);

	if (!err) {
		err = HPI_SampleClock_GetSampleRate(phSubSys, hControl,
						    &hpi_format.dwSampleRate);
	}

	for (wFormat = HPI_FORMAT_PCM8_UNSIGNED;
	     wFormat <= HPI_FORMAT_PCM24_SIGNED; wFormat++) {

		HPI_FormatCreate(&hpi_format, 2, wFormat, 48000, 128000, 0);
		err = HPI_InStreamQueryFormat(phSubSys, hStream, &hpi_format);
		if (!err)
			*formats |= (1ULL << hpi_to_alsa_formats[wFormat]);
	}
}

static int snd_card_asihpi_capture_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm;
	snd_card_asihpi_t *asihpi = snd_pcm_substream_chip(substream);
	u16 err;

	dpcm = kzalloc(sizeof(*dpcm), GFP_KERNEL);
	if (dpcm == NULL)
		return -ENOMEM;

	snd_printd("HPI_InStreamOpen Adapter %d Stream %d\n",
		   asihpi->wAdapterIndex, substream->number);

	err =
	    HPI_InStreamOpen(phSubSys, asihpi->wAdapterIndex,
			     substream->number, &dpcm->hStream);
	HPI_HandleError(err);
	if (err)
		kfree(dpcm);
	if (err == HPI_ERROR_OBJ_ALREADY_OPEN)
		return -EBUSY;
	if (err)
		return -EIO;

	snd_card_asihpi_capture_format(asihpi, dpcm->hStream,
				       &snd_card_asihpi_capture.formats);

	init_timer(&dpcm->timer);
	dpcm->timer.data = (unsigned long)dpcm;
	dpcm->timer.function = snd_card_asihpi_capture_timer_function;
//      spin_lock_init(&dpcm->lock);
	dpcm->substream = substream;
	runtime->private_data = dpcm;
	runtime->private_free = snd_card_asihpi_runtime_free;
	runtime->hw = snd_card_asihpi_capture;
	return 0;
}

static int snd_card_asihpi_capture_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	u16 err;

	err = HPI_InStreamClose(phSubSys, dpcm->hStream);
	HPI_HandleError(err);

	return 0;
}

static int snd_card_asihpi_capture_copy(struct snd_pcm_substream *substream,
					int channel, snd_pcm_uframes_t pos,
					void __user * dst,
					snd_pcm_uframes_t count)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	u16 err;
	u32 dwDataSize;

	dwDataSize = frames_to_bytes(runtime, count);

#if REALLY_VERBOSE_LOGGING
	snd_printd("Capture copy %d bytes\n", dwDataSize);
#endif
	err =
	    HPI_InStreamReadBuf(phSubSys, dpcm->hStream, runtime->dma_area,
				dwDataSize);
	HPI_HandleError(err);

	/* Used by capture_pointer */
	dpcm->pcm_irq_pos = (dpcm->pcm_irq_pos + dwDataSize) % dpcm->pcm_size;

	if (copy_to_user(dst, runtime->dma_area, dwDataSize))
		return -EFAULT;

	return 0;
}

static struct snd_pcm_ops snd_card_asihpi_capture_ops = {
	.open = snd_card_asihpi_capture_open,
	.close = snd_card_asihpi_capture_close,
	.ioctl = snd_card_asihpi_capture_ioctl,
	.hw_params = snd_card_asihpi_capture_hw_params,
	.hw_free = snd_card_asihpi_capture_hw_free,
	.prepare = snd_card_asihpi_capture_prepare,
	.trigger = snd_card_asihpi_capture_trigger,
	.pointer = snd_card_asihpi_capture_pointer,
	.copy = snd_card_asihpi_capture_copy
};

static int __devinit snd_card_asihpi_pcm(snd_card_asihpi_t * asihpi,
					 int device, int substreams)
{
	struct snd_pcm *pcm;
	int err;

	if ((err =
	     snd_pcm_new(asihpi->card, "Asihpi PCM", device,
			 asihpi->wNumOutStreams, asihpi->wNumInStreams,
			 &pcm)) < 0)
		return err;
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK,
			&snd_card_asihpi_playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE,
			&snd_card_asihpi_capture_ops);
	pcm->private_data = asihpi;
	pcm->info_flags = 0;
	strcpy(pcm->name, "Asihpi PCM");

	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_CONTINUOUS,
					      snd_dma_continuous_data
					      (GFP_KERNEL), 0, MAX_BUFFER_SIZE);
	return 0;
}

/***************************** MIXER CONTROLS ****************/
typedef struct {
	u16 wControlType;	// HPI_CONTROL_METER, _VOLUME etc
	u16 wSrcNodeType;
	u16 wSrcNodeIndex;
	u16 wDstNodeType;
	u16 wDstNodeIndex;
	u16 wBand;
} hpi_control_t;

#define TEXT(a) a

#define ASIHPI_TUNER_BAND_STRINGS \
{ \
	TEXT("invalid"), \
	TEXT("AM"), \
	TEXT("FM (mono)"), \
	TEXT("TV NTSC-M"), \
	TEXT("FM (stereo)"), \
	TEXT("AUX"), \
	TEXT("TV PAL BG"), \
	TEXT("TV PAL I"), \
	TEXT("TV PAL DK"), \
	TEXT("TV SECAM"), \
}

#define NUM_TUNER_STRINGS 10

#if ( NUM_TUNER_STRINGS != (HPI_TUNER_BAND_LAST+1) )
#error "Tuner band strings don't agree with HPI defines - version mismatch?"
#endif

#ifdef ASI_STYLE_NAMES
#define ASIHPI_SOURCENODE_STRINGS \
{ \
	TEXT("no source"), \
	TEXT("OutStr"), \
	TEXT("LineIn"), \
	TEXT("AesIn"), \
	TEXT("Tuner"), \
	TEXT("RF"), \
	TEXT("Clock"), \
	TEXT("Bitstr"), \
	TEXT("Mic"), \
	TEXT("Cobranet"), \
}
#else
#define ASIHPI_SOURCENODE_STRINGS \
{ \
	TEXT("no source"), \
	TEXT("PCM"), \
	TEXT("Line"), \
	TEXT("Digital"), \
	TEXT("Tuner"), \
	TEXT("RF"), \
	TEXT("Clock"), \
	TEXT("Bitstream"), \
	TEXT("Mic"), \
	TEXT("Cobranet"), \
}
#endif

#define NUM_SOURCENODE_STRINGS 10

#ifdef ASI_STYLE_NAMES
#define ASIHPI_DESTNODE_STRINGS \
{ \
	TEXT("no destination"), \
	TEXT("InStr"), \
	TEXT("LinOut"), \
	TEXT("AesOut"), \
	TEXT("RF"), \
	TEXT("Speak") \
	TEXT("Cobranet"), \
}
#else
#define ASIHPI_DESTNODE_STRINGS \
{ \
	TEXT("no destination"), \
	TEXT("PCM"), \
	TEXT("Line"), \
	TEXT("Digital"), \
	TEXT("RF"), \
	TEXT("Speaker") \
	TEXT("Cobranet"), \
}
#endif
#define NUM_DESTNODE_STRINGS 7

#if ( (NUM_SOURCENODE_STRINGS != (HPI_SOURCENODE_LAST_INDEX-HPI_SOURCENODE_BASE+1)) || (NUM_DESTNODE_STRINGS != (HPI_DESTNODE_LAST_INDEX-HPI_DESTNODE_BASE+1)))
#error "Node strings don't agree with HPI defines - version mismatch?"
#endif

static char *asihpi_src_names[] = ASIHPI_SOURCENODE_STRINGS;
static char *asihpi_dst_names[] = ASIHPI_DESTNODE_STRINGS;
static char *asihpi_tuner_band_names[] = ASIHPI_TUNER_BAND_STRINGS;

static inline int ctl_add(struct snd_card *card, struct snd_kcontrol_new *ctl,
			  snd_card_asihpi_t * asihpi)
{
	int err;

	if ((err = snd_ctl_add(card, snd_ctl_new1(ctl, asihpi))) < 0) {
		return err;
	} else if (mixer_dump)
		snd_printk(KERN_INFO "Added %s(%d)\n", ctl->name, ctl->index);

	return 0;
}

static void asihpi_ctl_name_prefix(struct snd_kcontrol_new *snd_control,
				   hpi_control_t * asihpi_control)
{

	if (asihpi_control->wDstNodeType) {
		if (asihpi_control->wSrcNodeType) {
			sprintf(snd_control->name, "%s %s%d",
				asihpi_src_names[asihpi_control->wSrcNodeType],
				asihpi_dst_names[asihpi_control->wDstNodeType],
				asihpi_control->wDstNodeIndex + 1);
		} else {
			strcpy(snd_control->name,
			       asihpi_dst_names[asihpi_control->wDstNodeType]);
		}
	} else {
		strcpy(snd_control->name,
		       asihpi_src_names[asihpi_control->wSrcNodeType]);
	}

}

/*------------------------------------------------------------
   Volume controls
 ------------------------------------------------------------*/
static int snd_asihpi_volume_info(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_info *uinfo)
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	u16 err;
	short nMinGain_01dB;
	short nMaxGain_01dB;
	short nStepGain_01dB;

	err =
	    HPI_VolumeQueryRange(phSubSys, hControl, &nMinGain_01dB,
				 &nMaxGain_01dB, &nStepGain_01dB);
	if (err) {
		nMaxGain_01dB = 0;
		nMinGain_01dB = -10000;
		nStepGain_01dB = 100;
	}
	//    HPI_HandleError(err);

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = nMinGain_01dB / HPI_UNITS_PER_dB;
	uinfo->value.integer.max = nMaxGain_01dB / HPI_UNITS_PER_dB;
	uinfo->value.integer.step = nStepGain_01dB / HPI_UNITS_PER_dB;
	return 0;
}

static int snd_asihpi_volume_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	short anGain0_01dB[HPI_MAX_CHANNELS];

	HPI_VolumeGetGain(phSubSys, hControl, anGain0_01dB);
	ucontrol->value.integer.value[0] = anGain0_01dB[0] / 100;
	ucontrol->value.integer.value[1] = anGain0_01dB[1] / 100;

	return 0;
}

static int snd_asihpi_volume_put(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	int change;
	HPI_HCONTROL hControl = kcontrol->private_value;
	short anGain0_01dB[HPI_MAX_CHANNELS];

	anGain0_01dB[0] = (ucontrol->value.integer.value[0]) * HPI_UNITS_PER_dB;
	anGain0_01dB[1] = (ucontrol->value.integer.value[1]) * HPI_UNITS_PER_dB;
	/*  change = asihpi->mixer_volume[addr][0] != left ||
	   asihpi->mixer_volume[addr][1] != right;
	 */
	change = 1;
	HPI_VolumeSetGain(phSubSys, hControl, anGain0_01dB);
	return change;
}

static void __devinit snd_asihpi_volume_new(hpi_control_t * asihpi_control,
					    struct snd_kcontrol_new
					    *snd_control)
{
	snd_control->info = snd_asihpi_volume_info;
	snd_control->get = snd_asihpi_volume_get;
	snd_control->put = snd_asihpi_volume_put;
	snd_control->index = asihpi_control->wSrcNodeIndex + 1;

	if (asihpi_control->wDstNodeType)
		sprintf(snd_control->name, "%s %s%d",
			asihpi_src_names[asihpi_control->wSrcNodeType],
			asihpi_dst_names[asihpi_control->wDstNodeType],
			asihpi_control->wDstNodeIndex + 1);
	else {
		strcpy(snd_control->name,
		       asihpi_src_names[asihpi_control->wSrcNodeType]);
	}
	strcat(snd_control->name, " Volume");
}

/*------------------------------------------------------------
   Level controls
 ------------------------------------------------------------*/
static int snd_asihpi_level_info(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = -20;
	uinfo->value.integer.max = 26;
	return 0;
}

static int snd_asihpi_level_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	short anGain0_01dB[HPI_MAX_CHANNELS];

	HPI_LevelGetGain(phSubSys, hControl, anGain0_01dB);
	ucontrol->value.integer.value[0] = anGain0_01dB[0] / HPI_UNITS_PER_dB;
	ucontrol->value.integer.value[1] = anGain0_01dB[1] / HPI_UNITS_PER_dB;

	return 0;
}

static int snd_asihpi_level_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int change;
	HPI_HCONTROL hControl = kcontrol->private_value;
	short anGain0_01dB[HPI_MAX_CHANNELS];

	anGain0_01dB[0] = (ucontrol->value.integer.value[0]) * HPI_UNITS_PER_dB;
	anGain0_01dB[1] = (ucontrol->value.integer.value[1]) * HPI_UNITS_PER_dB;
	/*  change = asihpi->mixer_level[addr][0] != left ||
	   asihpi->mixer_level[addr][1] != right;
	 */
	change = 1;
	HPI_LevelSetGain(phSubSys, hControl, anGain0_01dB);
	return change;
}

static void __devinit snd_asihpi_level_new(hpi_control_t * asihpi_control,
					   struct snd_kcontrol_new *snd_control)
{
	snd_control->info = snd_asihpi_level_info;
	snd_control->get = snd_asihpi_level_get;
	snd_control->put = snd_asihpi_level_put;

	if (asihpi_control->wDstNodeType ==
	    HPI_DESTNODE_LINEOUT - HPI_DESTNODE_BASE) {
		snd_control->index = asihpi_control->wDstNodeIndex + 1;
#ifdef ASI_STYLE_NAMES
		strcpy(snd_control->name, "LineOut Level");
#else
		strcpy(snd_control->name, "Playback Volume");
#endif
	} else if (asihpi_control->wSrcNodeType ==
		   HPI_SOURCENODE_LINEIN - HPI_SOURCENODE_BASE) {
		snd_control->index = asihpi_control->wSrcNodeIndex + 1;
#ifdef ASI_STYLE_NAMES
		strcpy(snd_control->name, "LineIn Level");
#else
		strcpy(snd_control->name, "Capture Volume");
#endif
	}
}

/*------------------------------------------------------------
   AESEBU controls
 ------------------------------------------------------------*/

/* AESEBU format */

#define ASIHPI_AESEBU_FORMAT_STRINGS \
{ \
	TEXT("N/A"), \
	TEXT("S/PDIF"), \
	TEXT("AES/EBU"), \
}

static char *asihpi_aesebu_format_names[] = ASIHPI_AESEBU_FORMAT_STRINGS;

static int snd_asihpi_aesebu_format_info(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = 3;

	if (uinfo->value.enumerated.item >= uinfo->value.enumerated.items)
		uinfo->value.enumerated.item =
		    uinfo->value.enumerated.items - 1;

	strcpy(uinfo->value.enumerated.name,
	       asihpi_aesebu_format_names[uinfo->value.enumerated.item]);

	return 0;
}

static int snd_asihpi_aesebu_format_get(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol,
					u16(*func) (HPI_HSUBSYS *, HPI_HCONTROL,
						    u16 *))
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	u16 source, err;

	err = func(phSubSys, hControl, &source);

	/* default to N/A */
	ucontrol->value.enumerated.item[0] = 0;
	/* return success but set the control to N/A */
	if (err)
		return 0;
	if (source == HPI_AESEBU_SOURCE_SPDIF)
		ucontrol->value.enumerated.item[0] = 1;
	if (source == HPI_AESEBU_SOURCE_AESEBU)
		ucontrol->value.enumerated.item[0] = 2;

	return 0;
}

static int snd_asihpi_aesebu_format_put(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol,
					u16(*func) (HPI_HSUBSYS *, HPI_HCONTROL,
						    u16))
{
	HPI_HCONTROL hControl = kcontrol->private_value;

	/* default to S/PDIF */
	u16 source = HPI_AESEBU_SOURCE_SPDIF;

	if (ucontrol->value.enumerated.item[0] == 1)
		source = HPI_AESEBU_SOURCE_SPDIF;
	if (ucontrol->value.enumerated.item[0] == 2)
		source = HPI_AESEBU_SOURCE_AESEBU;

	if (func(phSubSys, hControl, source) != 0)
		return -EINVAL;

	return 1;
}

static int snd_asihpi_aesebu_rx_source_get(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	return snd_asihpi_aesebu_format_get(kcontrol, ucontrol,
					    HPI_AESEBU_Receiver_GetSource);
}

static int snd_asihpi_aesebu_rx_source_put(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	return snd_asihpi_aesebu_format_put(kcontrol, ucontrol,
					    HPI_AESEBU_Receiver_SetSource);
}

static int snd_asihpi_aesebu_tx_format_get(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	return snd_asihpi_aesebu_format_get(kcontrol, ucontrol,
					    HPI_AESEBU_Transmitter_GetFormat);
}

static int snd_asihpi_aesebu_tx_format_put(struct snd_kcontrol *kcontrol,
					   struct snd_ctl_elem_value *ucontrol)
{
	return snd_asihpi_aesebu_format_put(kcontrol, ucontrol,
					    HPI_AESEBU_Transmitter_SetFormat);
}

/* AESEBU control group initializers  */

static int __devinit snd_asihpi_aesebu_rx_new(snd_card_asihpi_t * asihpi,
					      hpi_control_t * asihpi_control,
					      struct snd_kcontrol_new
					      *snd_control)
{

	struct snd_card *card = asihpi->card;

/* RX source */

	snd_control->info = snd_asihpi_aesebu_format_info;
	snd_control->get = snd_asihpi_aesebu_rx_source_get;
	snd_control->put = snd_asihpi_aesebu_rx_source_put;
	snd_control->index = asihpi_control->wSrcNodeIndex + 1;

	asihpi_ctl_name_prefix(snd_control, asihpi_control);
	strcat(snd_control->name, " AesRx");

	if (ctl_add(card, snd_control, asihpi) < 0)
		return -EINVAL;

	return 0;
}

static int __devinit snd_asihpi_aesebu_tx_new(snd_card_asihpi_t * asihpi,
					      hpi_control_t * asihpi_control,
					      struct snd_kcontrol_new
					      *snd_control)
{

	struct snd_card *card = asihpi->card;

/* TX Format */

	snd_control->info = snd_asihpi_aesebu_format_info;
	snd_control->get = snd_asihpi_aesebu_tx_format_get;
	snd_control->put = snd_asihpi_aesebu_tx_format_put;
	snd_control->index = asihpi_control->wDstNodeIndex + 1;

	asihpi_ctl_name_prefix(snd_control, asihpi_control);
	strcat(snd_control->name, " Format");

	if (ctl_add(card, snd_control, asihpi) < 0)
		return -EINVAL;

	return 0;
}

/*------------------------------------------------------------
   Tuner controls
 ------------------------------------------------------------*/

/* Gain */

static int snd_asihpi_tuner_gain_info(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_info *uinfo)
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	u16 err;
	short idx;
	u32 gainRange[3];

	for (idx = 0; idx < 3; idx++) {
		err = HPI_ControlQuery(phSubSys, hControl, HPI_TUNER_GAIN,
				       idx, 0, &gainRange[idx]);
		if (err != 0)
			return err;
	}

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = ((int)gainRange[0]) / HPI_UNITS_PER_dB;
	uinfo->value.integer.max = ((int)gainRange[1]) / HPI_UNITS_PER_dB;
	uinfo->value.integer.step = ((int)gainRange[2]) / HPI_UNITS_PER_dB;
	return 0;
}

static int snd_asihpi_tuner_gain_get(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	/*
	   snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	 */
	HPI_HCONTROL hControl = kcontrol->private_value;
	short gain;

	HPI_Tuner_GetGain(phSubSys, hControl, &gain);
	ucontrol->value.integer.value[0] = gain / HPI_UNITS_PER_dB;

	return 0;
}

static int snd_asihpi_tuner_gain_put(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	/*
	   snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	 */
	HPI_HCONTROL hControl = kcontrol->private_value;
	short gain;

	gain = (ucontrol->value.integer.value[0]) * HPI_UNITS_PER_dB;
	HPI_Tuner_SetGain(phSubSys, hControl, gain);

	return 1;
}

/* Band  */

static int asihpi_tuner_band_query(struct snd_kcontrol *kcontrol,
				   u32 * bandList, u32 len)
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	u16 err;
	u32 idx;

	for (idx = 0; idx < len; idx++) {
		err = HPI_ControlQuery(phSubSys, hControl, HPI_TUNER_BAND,
				       idx, 0, &bandList[idx]);
		if (err != 0)
			break;
	}

	return idx;
}

static int snd_asihpi_tuner_band_info(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_info *uinfo)
{
	u32 tunerBands[HPI_TUNER_BAND_LAST];
	u32 numBands = 0;

	numBands = asihpi_tuner_band_query(kcontrol, tunerBands,
					   HPI_TUNER_BAND_LAST);

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = numBands;

	if (uinfo->value.enumerated.item >= uinfo->value.enumerated.items)
		uinfo->value.enumerated.item =
		    uinfo->value.enumerated.items - 1;

	strcpy(uinfo->value.enumerated.name,
	       asihpi_tuner_band_names[tunerBands
				       [uinfo->value.enumerated.item]]);

	return 0;
}

static int snd_asihpi_tuner_band_get(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	/*
	   snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	 */
	u16 band, idx;
	u32 tunerBands[HPI_TUNER_BAND_LAST];
	u32 numBands = 0;

	numBands = asihpi_tuner_band_query(kcontrol, tunerBands,
					   HPI_TUNER_BAND_LAST);

	HPI_Tuner_GetBand(phSubSys, hControl, &band);

	ucontrol->value.enumerated.item[0] = -1;
	for (idx = 0; idx < HPI_TUNER_BAND_LAST; idx++)
		if (tunerBands[idx] == band) {
			ucontrol->value.enumerated.item[0] = idx;
			break;
		}

	return 0;
}

static int snd_asihpi_tuner_band_put(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	/*
	   snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	 */
	HPI_HCONTROL hControl = kcontrol->private_value;
	u16 band;
	u32 tunerBands[HPI_TUNER_BAND_LAST];
	u32 numBands = 0;

	numBands = asihpi_tuner_band_query(kcontrol, tunerBands,
					   HPI_TUNER_BAND_LAST);

	band = tunerBands[ucontrol->value.enumerated.item[0]];
	HPI_Tuner_SetBand(phSubSys, hControl, band);

	return 1;
}

/* Freq */

static int snd_asihpi_tuner_freq_info(struct snd_kcontrol *kcontrol,
				      struct snd_ctl_elem_info *uinfo)
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	u16 err;
	u32 tunerBands[HPI_TUNER_BAND_LAST];
	u32 numBands = 0, band_iter, idx;
	u32 freqRange[3], tempFreqRange[3];

	numBands = asihpi_tuner_band_query(kcontrol, tunerBands,
					   HPI_TUNER_BAND_LAST);

	freqRange[0] = INT_MAX;
	freqRange[1] = 0;
	freqRange[2] = INT_MAX;

	for (band_iter = 0; band_iter < numBands; band_iter++) {
		for (idx = 0; idx < 3; idx++) {
			err = HPI_ControlQuery(phSubSys, hControl,
					       HPI_TUNER_FREQ, idx,
					       tunerBands[band_iter],
					       &tempFreqRange[idx]);
			if (err != 0)
				return err;
		}

		/* skip band with bogus stepping */
		if (tempFreqRange[2] <= 0)
			continue;

		if (tempFreqRange[0] < freqRange[0])
			freqRange[0] = tempFreqRange[0];
		if (tempFreqRange[1] > freqRange[1])
			freqRange[1] = tempFreqRange[1];
		if (tempFreqRange[2] < freqRange[2])
			freqRange[2] = tempFreqRange[2];
	}

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = ((int)freqRange[0]);
	uinfo->value.integer.max = ((int)freqRange[1]);
	uinfo->value.integer.step = ((int)freqRange[2]);
	return 0;
}

static int snd_asihpi_tuner_freq_get(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	u32 freq;

	HPI_Tuner_GetFrequency(phSubSys, hControl, &freq);
	ucontrol->value.integer.value[0] = freq;

	return 0;
}

static int snd_asihpi_tuner_freq_put(struct snd_kcontrol *kcontrol,
				     struct snd_ctl_elem_value *ucontrol)
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	u32 freq;

	freq = ucontrol->value.integer.value[0];
	HPI_Tuner_SetFrequency(phSubSys, hControl, freq);

	return 1;
}

/* Tuner control group initializer  */

static int __devinit snd_asihpi_tuner_new(snd_card_asihpi_t * asihpi,
					  hpi_control_t * asihpi_control,
					  struct snd_kcontrol_new *snd_control)
{

	struct snd_card *card = asihpi->card;

/* Gain ctl */

	snd_control->info = snd_asihpi_tuner_gain_info;
	snd_control->get = snd_asihpi_tuner_gain_get;
	snd_control->put = snd_asihpi_tuner_gain_put;
	snd_control->index = asihpi_control->wSrcNodeIndex + 1;

	asihpi_ctl_name_prefix(snd_control, asihpi_control);
	strcat(snd_control->name, " Gain");

	if (ctl_add(card, snd_control, asihpi) < 0)
		return -EINVAL;

/* Band ctl */

	snd_control->info = snd_asihpi_tuner_band_info;
	snd_control->get = snd_asihpi_tuner_band_get;
	snd_control->put = snd_asihpi_tuner_band_put;
	snd_control->index = asihpi_control->wSrcNodeIndex + 1;

	asihpi_ctl_name_prefix(snd_control, asihpi_control);
	strcat(snd_control->name, " Band");

	if (ctl_add(card, snd_control, asihpi) < 0)
		return -EINVAL;

/* Freq ctl */

	snd_control->info = snd_asihpi_tuner_freq_info;
	snd_control->get = snd_asihpi_tuner_freq_get;
	snd_control->put = snd_asihpi_tuner_freq_put;
	snd_control->index = asihpi_control->wSrcNodeIndex + 1;

	asihpi_ctl_name_prefix(snd_control, asihpi_control);
	strcat(snd_control->name, " Freq");

	if (ctl_add(card, snd_control, asihpi) < 0)
		return -EINVAL;

/* Level meter */

	return 0;
}

/*------------------------------------------------------------
   Meter controls
 ------------------------------------------------------------*/
#define ASIHPI_LINEAR_METERS 1
#define ASIHPI_LOG_METERS 0

static int snd_asihpi_meter_info(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = HPI_MAX_CHANNELS;
#if ASIHPI_LINEAR_METERS
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 0x7FFFFFFF;
#else
	uinfo->value.integer.min = HPI_METER_MINIMUM;
#if ASIHPI_LOG_METERS
	uinfo->value.integer.max = 0;
#else
	uinfo->value.integer.max = 32767;
#endif
#endif

	return 0;
}

#if ASIHPI_LINEAR_METERS
/* linear values for 10dB steps */
static int log2lin[] = {
	0x7FFFFFFF,		/* 0dB */
	679093956,
	214748365,
	67909396,
	21474837,
	6790940,
	2147484,		/* -60dB */
	679094,
	214748,
	67909,
	21475,
	6791,
	2147,
	679,
	214,
	68,
	21,
	7,
	2
};
#endif

static int snd_asihpi_meter_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	short anGain0_01dB[HPI_MAX_CHANNELS], i;
	u16 err;

	err = HPI_MeterGetRms(phSubSys, hControl, anGain0_01dB);

	// convert 0..32767 level to 0.01dB (20log10), 0 is -100.00dB
	for (i = 0; i < HPI_MAX_CHANNELS; i++) {
#if ASIHPI_LINEAR_METERS
		if (err) {
			ucontrol->value.integer.value[i] = 0;
		} else if (anGain0_01dB[i] >= 0) {
			ucontrol->value.integer.value[i] =
			    anGain0_01dB[i] << 16;
		} else {
			/* -ve is log value in millibels < -60dB, convert to (roughly!) linear, */
			ucontrol->value.integer.value[i] =
			    log2lin[anGain0_01dB[i] / -1000];
		}
#elif ASIHPI_LOG_METERS
		ucontrol->value.integer.value[i] = anGain0_01dB[i];
		if ((anGain0_01dB[i] == 0) || err)
			ucontrol->value.integer.value[i] = HPI_METER_MINIMUM;
		else if (anGain0_01dB[i] > 0) {
#if 0
			/* need fixed point log calculation convert 32..32767 to -60.00..0.00dB */
			// actually should always be > 32  32767 => 0dB, 32.767=>-60dB
			anGain0_01dB[i] =
			    (short)((float)
				    (20 * log10((float)nLinear / 32767.0)) *
				    100.0);
			// units are 0.01dB
#endif
		}		// else don't have to touch the LogValue when it is -ve since it is already a log value
#else				/* RAW_METERS */
		ucontrol->value.integer.value[i] = anGain0_01dB[i];
		if (err)
			ucontrol->value.integer.value[i] = 0;
#endif
	}

	return 0;
}

static void __devinit snd_asihpi_meter_new(hpi_control_t * asihpi_control,
					   struct snd_kcontrol_new *snd_control)
{
	snd_control->info = snd_asihpi_meter_info;
	snd_control->get = snd_asihpi_meter_get;
	snd_control->put = NULL;
	snd_control->access =
	    SNDRV_CTL_ELEM_ACCESS_VOLATILE | SNDRV_CTL_ELEM_ACCESS_READ;

	if (asihpi_control->wDstNodeType) {
		snd_control->index = asihpi_control->wDstNodeIndex + 1;
		strcpy(snd_control->name,
		       asihpi_dst_names[asihpi_control->wDstNodeType]);
	} else {
		snd_control->index = asihpi_control->wSrcNodeIndex + 1;
		strcpy(snd_control->name,
		       asihpi_src_names[asihpi_control->wSrcNodeType]);
	}
	strcat(snd_control->name, " Meter");
}

/*------------------------------------------------------------
   Multiplexer controls
 ------------------------------------------------------------*/
static int snd_card_asihpi_mux_count_sources(struct snd_kcontrol *snd_control)
{
	HPI_HCONTROL hControl = snd_control->private_value;
	hpi_control_t asihpi_control;
	int s, err;
	for (s = 0; s < 32; s++) {
		err = HPI_Multiplexer_QuerySource(phSubSys, hControl, s,
						  &asihpi_control.
						  wSrcNodeType,
						  &asihpi_control.
						  wSrcNodeIndex);
		if (err)
			break;
	}
	return (s);
}

static int snd_asihpi_mux_info(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_info *uinfo)
{
	int err;
	u16 wSrcNodeType, wSrcNodeIndex;
	HPI_HCONTROL hControl = kcontrol->private_value;

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items =
	    snd_card_asihpi_mux_count_sources(kcontrol);

	if (uinfo->value.enumerated.item >= uinfo->value.enumerated.items)
		uinfo->value.enumerated.item =
		    uinfo->value.enumerated.items - 1;

	err =
	    HPI_Multiplexer_QuerySource(phSubSys, hControl,
					uinfo->value.enumerated.item,
					&wSrcNodeType, &wSrcNodeIndex);

	sprintf(uinfo->value.enumerated.name, "%s %d",
		asihpi_src_names[wSrcNodeType - HPI_SOURCENODE_BASE],
		wSrcNodeIndex + 1);
	return 0;
}

static int snd_asihpi_mux_get(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	u16 wSourceType, wSourceIndex;
	u16 wSrcNodeType, wSrcNodeIndex;
	int s;

	HPI_Multiplexer_GetSource(phSubSys, hControl, &wSourceType,
				  &wSourceIndex);
	// search to find the index with this source and index.  Should cache it!
	for (s = 0; s < 16; s++) {
		HPI_Multiplexer_QuerySource(phSubSys, hControl, s,
					    &wSrcNodeType, &wSrcNodeIndex);
		if ((wSourceType == wSrcNodeType)
		    && (wSourceIndex == wSrcNodeIndex)) {
			ucontrol->value.enumerated.item[0] = s;
			return 0;
		}
	}
	return -EINVAL;
}

static int snd_asihpi_mux_put(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	int change;
	HPI_HCONTROL hControl = kcontrol->private_value;
	u16 wSourceType, wSourceIndex;

	change = 1;

	HPI_Multiplexer_QuerySource(phSubSys, hControl,
				    ucontrol->value.enumerated.item[0],
				    &wSourceType, &wSourceIndex);
	HPI_Multiplexer_SetSource(phSubSys, hControl, wSourceType,
				  wSourceIndex);
	return change;
}

static void __devinit snd_asihpi_mux_new(hpi_control_t * asihpi_control,
					 struct snd_kcontrol_new *snd_control)
{
	snd_control->info = snd_asihpi_mux_info;
	snd_control->get = snd_asihpi_mux_get;
	snd_control->put = snd_asihpi_mux_put;
	if (asihpi_control->wSrcNodeType) {
		strcpy(snd_control->name,
		       asihpi_src_names[asihpi_control->wSrcNodeType]);
		snd_control->index = asihpi_control->wSrcNodeIndex + 1;
	} else {
		strcpy(snd_control->name,
		       asihpi_dst_names[asihpi_control->wDstNodeType]);
		snd_control->index = asihpi_control->wDstNodeIndex + 1;
	}
#ifdef ASI_STYLE_NAMES
	strcat(snd_control->name, " Multiplexer");
#else
	strcat(snd_control->name, " Route");
#endif
}

/*------------------------------------------------------------
   Channel mode controls
 ------------------------------------------------------------*/
static int snd_asihpi_cmode_info(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_info *uinfo)
{
	static char *texts[4] = { "Normal", "Swap", "ToLeft", "ToRight" };

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = 4;

	if (uinfo->value.enumerated.item > 3)
		uinfo->value.enumerated.item = 3;

	strcpy(uinfo->value.enumerated.name,
	       texts[uinfo->value.enumerated.item]);
	return 0;
}

static int snd_asihpi_cmode_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	u16 wMode;

	if (HPI_ChannelModeGet(phSubSys, hControl, &wMode))
		wMode = 1;

	ucontrol->value.enumerated.item[0] = wMode - 1;

	return 0;
}

static int snd_asihpi_cmode_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int change;
	HPI_HCONTROL hControl = kcontrol->private_value;

	change = 1;

	HPI_ChannelModeSet(phSubSys, hControl,
			   ucontrol->value.enumerated.item[0] + 1);
	return change;
}

static void __devinit snd_asihpi_cmode_new(hpi_control_t * asihpi_control,
					   struct snd_kcontrol_new *snd_control)
{
	snd_control->info = snd_asihpi_cmode_info;
	snd_control->get = snd_asihpi_cmode_get;
	snd_control->put = snd_asihpi_cmode_put;
	if (asihpi_control->wSrcNodeType) {
		strcpy(snd_control->name,
		       asihpi_src_names[asihpi_control->wSrcNodeType]);
		snd_control->index = asihpi_control->wSrcNodeIndex + 1;
	} else {
		strcpy(snd_control->name,
		       asihpi_dst_names[asihpi_control->wDstNodeType]);
		snd_control->index = asihpi_control->wDstNodeIndex + 1;
	}
	strcat(snd_control->name, " Channel Mode");
}

/*------------------------------------------------------------
   Sampleclock source  controls
 ------------------------------------------------------------*/

static char *sampleclock_sources[MAX_CLOCKSOURCES] =
    { "N/A", "Adapter", "AES/EBU Sync", "Word External", "Word Header",
	"SMPTE", "AES/EBU In1", "Auto", "Cobranet",
	"AES/EBU In2", "AES/EBU In3", "AES/EBU In4", "AES/EBU In5",
	"AES/EBU In6", "AES/EBU In7", "AES/EBU In8"
};

static int snd_asihpi_clksrc_info(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_info *uinfo)
{
	snd_card_asihpi_t *asihpi =
	    (snd_card_asihpi_t *) (kcontrol->private_data);
	struct clk_cache *clkcache = &asihpi->cc;
	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = clkcache->count;

	if (uinfo->value.enumerated.item >= uinfo->value.enumerated.items)
		uinfo->value.enumerated.item =
		    uinfo->value.enumerated.items - 1;

	strcpy(uinfo->value.enumerated.name,
	       clkcache->s[uinfo->value.enumerated.item].name);
	return 0;
}

static int snd_asihpi_clksrc_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	snd_card_asihpi_t *asihpi =
	    (snd_card_asihpi_t *) (kcontrol->private_data);
	struct clk_cache *clkcache = &asihpi->cc;
	HPI_HCONTROL hControl = kcontrol->private_value;
	u16 wSource, wIndex;
	int i;

	ucontrol->value.enumerated.item[0] = 0;
	/*Get current clock source, return "N/A" if an error occurs */
	if (HPI_SampleClock_GetSource(phSubSys, hControl, &wSource))
		wSource = 0;
	if (HPI_SampleClock_GetSourceIndex(phSubSys, hControl, &wIndex))
		wIndex = 0;

	for (i = 0; i < clkcache->count; i++)
		if ((clkcache->s[i].source == wSource) &&
		    (clkcache->s[i].index == wIndex))
			break;

	ucontrol->value.enumerated.item[0] = i;

	return 0;
}

static int snd_asihpi_clksrc_put(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	snd_card_asihpi_t *asihpi =
	    (snd_card_asihpi_t *) (kcontrol->private_data);
	struct clk_cache *clkcache = &asihpi->cc;
	int change, item;
	HPI_HCONTROL hControl = kcontrol->private_value;

	change = 1;
	item = ucontrol->value.enumerated.item[0];
	if (item >= clkcache->count)
		item = clkcache->count - 1;

	HPI_SampleClock_SetSource(phSubSys, hControl, clkcache->s[item].source);
	HPI_SampleClock_SetSourceIndex(phSubSys,
				       hControl, clkcache->s[item].index);
	return change;
}

static void __devinit snd_asihpi_clksrc_new(snd_card_asihpi_t * asihpi,
					    hpi_control_t * asihpi_control,
					    struct snd_kcontrol_new
					    *snd_control)
{
	snd_control->info = snd_asihpi_clksrc_info;
	snd_control->get = snd_asihpi_clksrc_get;
	snd_control->put = snd_asihpi_clksrc_put;
	if (asihpi_control->wSrcNodeType) {
		strcpy(snd_control->name,
		       asihpi_src_names[asihpi_control->wSrcNodeType]);
		snd_control->index = asihpi_control->wSrcNodeIndex + 1;
	} else {
		strcpy(snd_control->name,
		       asihpi_dst_names[asihpi_control->wDstNodeType]);
		snd_control->index = asihpi_control->wDstNodeIndex + 1;
	}
	strcat(snd_control->name, " ClockSource");

	{
		struct clk_cache *clkcache = &asihpi->cc;
		HPI_HCONTROL hSC = snd_control->private_value;
		int hasAesIn = 0;
		int i, j;
		u32 wSource;

		for (i = 0; i <= HPI_SAMPLECLOCK_SOURCE_LAST; i++) {
			if (HPI_ControlQuery(phSubSys, hSC,
					     HPI_SAMPLECLOCK_SOURCE, i, 0,
					     &wSource))
				break;
			clkcache->s[i].source = wSource;
			clkcache->s[i].index = 0;
			clkcache->s[i].name = sampleclock_sources[wSource];
			if (wSource == HPI_SAMPLECLOCK_SOURCE_AESEBU_INPUT)
				hasAesIn = 1;
		}
		if (hasAesIn)
			/* already will have picked up index 0 above */
			for (j = 1; j < 8; j++) {
				if (HPI_ControlQuery(phSubSys, hSC,
						     HPI_SAMPLECLOCK_SOURCE_INDEX,
						     j,
						     HPI_SAMPLECLOCK_SOURCE_AESEBU_INPUT,
						     &wSource))
					break;
				clkcache->s[i].source =
				    HPI_SAMPLECLOCK_SOURCE_AESEBU_INPUT;
				clkcache->s[i].index = j;
				clkcache->s[i].name =
				    sampleclock_sources[j +
							HPI_SAMPLECLOCK_SOURCE_LAST];
				i++;
			}
		clkcache->count = i;
	}

}

/*------------------------------------------------------------
   Clkrate controls
 ------------------------------------------------------------*/
static int snd_asihpi_clkrate_info(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_info *uinfo)
{
	//HPI_HCONTROL hControl= kcontrol->private_value;
	//u16 err;

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 8000;
	uinfo->value.integer.max = 192000;
	uinfo->value.integer.step = 100;

	return 0;
}

static int snd_asihpi_clkrate_get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	u32 dwRate;

	HPI_SampleClock_GetSampleRate(phSubSys, hControl, &dwRate);
	ucontrol->value.integer.value[0] = dwRate;
	return 0;
}

static int snd_asihpi_clkrate_put(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	int change;
	HPI_HCONTROL hControl = kcontrol->private_value;

	/*  change = asihpi->mixer_clkrate[addr][0] != left ||
	   asihpi->mixer_clkrate[addr][1] != right;
	 */
	change = 1;
	HPI_SampleClock_SetSampleRate(phSubSys, hControl,
				      ucontrol->value.integer.value[0]);
	return change;
}

static void __devinit snd_asihpi_clkrate_new(hpi_control_t * asihpi_control,
					     struct snd_kcontrol_new
					     *snd_control)
{
	snd_control->info = snd_asihpi_clkrate_info;
	snd_control->get = snd_asihpi_clkrate_get;
	snd_control->put = snd_asihpi_clkrate_put;
	snd_control->index = asihpi_control->wSrcNodeIndex + 1;

	if (asihpi_control->wDstNodeType)
		sprintf(snd_control->name, "%s %s%d",
			asihpi_src_names[asihpi_control->wSrcNodeType],
			asihpi_dst_names[asihpi_control->wDstNodeType],
			asihpi_control->wDstNodeIndex + 1);
	else {
		strcpy(snd_control->name,
		       asihpi_src_names[asihpi_control->wSrcNodeType]);
	}
	strcat(snd_control->name, " Rate");
}

/*------------------------------------------------------------
   Mixer
 ------------------------------------------------------------*/

static int __devinit snd_card_asihpi_new_mixer(snd_card_asihpi_t * asihpi)
{
	struct snd_card *card = asihpi->card;
	unsigned int idx = 0;
	int err;
	HPI_HCONTROL hControl;
	struct snd_kcontrol_new snd_control;
	char snd_control_name[44];	// asound.h line 745 unsigned char name[44];
	hpi_control_t asihpi_control;

	snd_assert(asihpi != NULL, return -EINVAL);
//      spin_lock_init(&asihpi->mixer_lock);
	strcpy(card->mixername, "Asihpi Mixer");

	err = HPI_MixerOpen(phSubSys, asihpi->wAdapterIndex, &asihpi->hMixer);
	HPI_HandleError(err);
	if (err)
		return -err;

	memset(&snd_control, 0, sizeof(snd_control));
	snd_control.iface = SNDRV_CTL_ELEM_IFACE_MIXER;
	snd_control.name = snd_control_name;

	/* now iterate through mixer controls */
	for (idx = 0; idx < 2000; idx++) {
		err = HPI_MixerGetControlByIndex(phSubSys, asihpi->hMixer,
						 idx,
						 &asihpi_control.wSrcNodeType,
						 &asihpi_control.wSrcNodeIndex,
						 &asihpi_control.wDstNodeType,
						 &asihpi_control.wDstNodeIndex,
						 &asihpi_control.wControlType,
						 &hControl);
		if (err) {
			if (err == HPI_ERROR_CONTROL_DISABLED) {
				if (mixer_dump)
					snd_printk(KERN_INFO
						   "Disabled HPI Control(%d)\n",
						   idx);
				continue;
			} else {
				break;
			}
		}

		asihpi_control.wSrcNodeType -= HPI_SOURCENODE_BASE;
		asihpi_control.wDstNodeType -= HPI_DESTNODE_BASE;
		snd_control.index = 0;
		snd_control.private_value = hControl;
		snd_control.access =
		    SNDRV_CTL_ELEM_ACCESS_WRITE | SNDRV_CTL_ELEM_ACCESS_READ;
		switch (asihpi_control.wControlType) {
		case HPI_CONTROL_VOLUME:
			snd_asihpi_volume_new(&asihpi_control, &snd_control);
			break;
		case HPI_CONTROL_LEVEL:
			snd_asihpi_level_new(&asihpi_control, &snd_control);
			break;
		case HPI_CONTROL_MULTIPLEXER:
			snd_asihpi_mux_new(&asihpi_control, &snd_control);
			break;
		case HPI_CONTROL_CHANNEL_MODE:
			snd_asihpi_cmode_new(&asihpi_control, &snd_control);
			break;
		case HPI_CONTROL_METER:
			snd_asihpi_meter_new(&asihpi_control, &snd_control);
			break;
		case HPI_CONTROL_SAMPLECLOCK:
			snd_asihpi_clksrc_new(asihpi, &asihpi_control,
					      &snd_control);
			err = snd_ctl_add(card,
					  snd_ctl_new1(&snd_control, asihpi));
			if (err < 0)
				return err;
			snd_asihpi_clkrate_new(&asihpi_control, &snd_control);
			break;
		case HPI_CONTROL_CONNECTION:	// ignore these
			continue;
		case HPI_CONTROL_TUNER:
			if (snd_asihpi_tuner_new(asihpi, &asihpi_control,
						 &snd_control) < 0)
				return -EINVAL;
			continue;
		case HPI_CONTROL_AESEBU_TRANSMITTER:
			if (snd_asihpi_aesebu_tx_new(asihpi, &asihpi_control,
						     &snd_control) < 0)
				return -EINVAL;
			continue;
		case HPI_CONTROL_AESEBU_RECEIVER:
			if (snd_asihpi_aesebu_rx_new(asihpi, &asihpi_control,
						     &snd_control) < 0)
				return -EINVAL;
			continue;
		case HPI_CONTROL_MUTE:
		case HPI_CONTROL_ONOFFSWITCH:
		case HPI_CONTROL_VOX:
		case HPI_CONTROL_BITSTREAM:
		case HPI_CONTROL_MICROPHONE:
		case HPI_CONTROL_PARAMETRIC_EQ:
		case HPI_CONTROL_COMPANDER:
		default:
			if (mixer_dump)
				snd_printk(KERN_INFO
					   "Untranslated HPI Control"
					   "(%d) %d %d %d %d %d\n",
					   idx,
					   asihpi_control.wControlType,
					   asihpi_control.wSrcNodeType,
					   asihpi_control.wSrcNodeIndex,
					   asihpi_control.wDstNodeType,
					   asihpi_control.wDstNodeIndex);
			continue;
		};

		if ((err =
		     snd_ctl_add(card,
				 snd_ctl_new1(&snd_control, asihpi))) < 0) {
			return err;
		} else if (mixer_dump)
			snd_printk(KERN_INFO "Added %s(%d)\n",
				   snd_control.name, snd_control.index);
	}
	if (HPI_ERROR_INVALID_OBJ_INDEX != err)	// this is normal
		HPI_HandleError(err);

	snd_printk(KERN_INFO "%d mixer controls found\n", idx);

	return 0;
}

/*------------------------------------------------------------
   /proc interface 
 ------------------------------------------------------------*/

static void
snd_asihpi_proc_read(struct snd_info_entry *entry,
		     struct snd_info_buffer *buffer)
{
	snd_card_asihpi_t *asihpi = entry->private_data;
	u16 wVersion;
	HPI_HCONTROL hControl;
	u32 dwRate = 0;
	u16 wSource = 0;
	int err;

	snd_iprintf(buffer, "ASIHPI driver proc file\n");
	snd_iprintf(buffer,
		    "Adapter ID=%4X\nIndex=%d\n"
		    "NumOutStreams=%d\nNumInStreams=%d\n",
		    asihpi->wType, asihpi->wAdapterIndex,
		    asihpi->wNumOutStreams, asihpi->wNumInStreams);

	wVersion = asihpi->wVersion;
	snd_iprintf(buffer,
		    "Serial#=%d\nHw Version %c%d\nDSP code version %03d\n",
		    asihpi->dwSerialNumber, ((wVersion >> 3) & 0xf) + 'A',
		    wVersion & 0x7,
		    ((wVersion >> 13) * 100) + ((wVersion >> 7) & 0x3f));

	err = HPI_MixerGetControl(phSubSys, asihpi->hMixer,
				  HPI_SOURCENODE_CLOCK_SOURCE, 0, 0, 0,
				  HPI_CONTROL_SAMPLECLOCK, &hControl);

	if (!err) {
		err =
		    HPI_SampleClock_GetSampleRate(phSubSys, hControl, &dwRate);
		err += HPI_SampleClock_GetSource(phSubSys, hControl, &wSource);

		if (!err)
			snd_iprintf(buffer, "SampleClock=%dHz, source %s\n",
				    dwRate, sampleclock_sources[wSource]);
	}

}

static void __devinit snd_asihpi_proc_init(snd_card_asihpi_t * asihpi)
{
	struct snd_info_entry *entry;

	if (!snd_card_proc_new(asihpi->card, "info", &entry))
		snd_info_set_text_ops(entry, asihpi, snd_asihpi_proc_read);
}

/*------------------------------------------------------------
   CARD
 ------------------------------------------------------------*/
int snd_asihpi_bind(adapter_t * hpi_card)
{
	int err;

	u16 wVersion;
	int pcm_substreams;

	struct snd_card *card;
	struct snd_card_asihpi *asihpi;

	HPI_HCONTROL hControl;

	static int dev;
	if (dev >= SNDRV_CARDS) {
		return -ENODEV;
	}
	// Should this be enable[hpi_card->wAdapterIndex] ?
	if (!enable[dev]) {
		dev++;
		return -ENOENT;
	}
	// first try to give the card the same index as its hardware index
	card = snd_card_new(hpi_card->wAdapterIndex,
			    id[hpi_card->wAdapterIndex], THIS_MODULE,
			    sizeof(struct snd_card_asihpi));
	if (card == NULL) {
		// if that fails, try the default index==next available
		card =
		    snd_card_new(index[dev], id[dev],
				 THIS_MODULE, sizeof(struct snd_card_asihpi));
		if (card == NULL)
			return -ENOMEM;
		snd_printk(KERN_WARNING
			   "**** WARNING **** Adapter index %d->ALSA index %d\n",
			   hpi_card->wAdapterIndex, card->number);
	}

	asihpi = (struct snd_card_asihpi *)card->private_data;
	asihpi->card = card;
	asihpi->wAdapterIndex = hpi_card->wAdapterIndex;
	err = HPI_AdapterGetInfo(phSubSys,
				 asihpi->wAdapterIndex,
				 &asihpi->wNumOutStreams,
				 &asihpi->wNumInStreams,
				 &asihpi->wVersion,
				 &asihpi->dwSerialNumber, &asihpi->wType);
	HPI_HandleError(err);
	wVersion = asihpi->wVersion;
	snd_printk(KERN_INFO "Adapter ID=%4X Index=%d NumOutStreams=%d NumInStreams=%d S/N=%d\n" "Hw Version %c%d DSP code version %03d\n", asihpi->wType, asihpi->wAdapterIndex, asihpi->wNumOutStreams, asihpi->wNumInStreams, asihpi->dwSerialNumber, ((wVersion >> 3) & 0xf) + 'A',	// Hw version major
		   wVersion & 0x7,	// Hw version minor
		   ((wVersion >> 13) * 100) + ((wVersion >> 7) & 0x3f)	// DSP code version
	    );

	pcm_substreams = asihpi->wNumOutStreams;
	if (pcm_substreams < asihpi->wNumInStreams)
		pcm_substreams = asihpi->wNumInStreams;
	if ((err = snd_card_asihpi_pcm(asihpi, 0, pcm_substreams)) < 0)
		goto __nodev;
	if ((err = snd_card_asihpi_new_mixer(asihpi)) < 0)
		goto __nodev;

	err = HPI_MixerGetControl(phSubSys, asihpi->hMixer,
				  HPI_SOURCENODE_CLOCK_SOURCE, 0, 0, 0,
				  HPI_CONTROL_SAMPLECLOCK, &hControl);

	if (!err)
		err = HPI_SampleClock_SetSampleRate(phSubSys,
						    hControl, adapter_fs);

	snd_asihpi_proc_init(asihpi);

	strcpy(card->driver, "ASIHPI");
	sprintf(card->shortname, "AudioScience ASI%4X", asihpi->wType);
	sprintf(card->longname, "%s %i",
		card->shortname, asihpi->wAdapterIndex + 1);
	if ((err = snd_card_register(card)) == 0) {
		hpi_card->snd_card_asihpi = card;
		dev++;
		return 0;
	}
      __nodev:
	snd_card_free(card);
	return err;

}

void snd_asihpi_unbind(adapter_t * hpi_card)
{
	snd_card_free(hpi_card->snd_card_asihpi);
	hpi_card->snd_card_asihpi = NULL;
}
