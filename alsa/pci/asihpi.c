/*   -*- linux-c -*-
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
	
// Mixer control
#ifndef ASI_STYLE_NAMES
#define ASI_STYLE_NAMES 1
#endif

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

#define HPI_OS_LINUX
#include "hpi.h"

static int mixer_dump;
module_param(mixer_dump, int, 0444);
MODULE_PARM_DESC(mixer_dump, "Enable mixer control dump");

#define DEFAULT_SAMPLERATE 44100
static int adapter_fs = DEFAULT_SAMPLERATE;
module_param(adapter_fs, int, 0444);
MODULE_PARM_DESC(adapter_fs, "Adapter HW samplerate (if supported)");

static int enable_bbm = 1;
module_param(enable_bbm, int, 0444);
MODULE_PARM_DESC(enable_bbm, "Enable background bus master, if available");

MODULE_AUTHOR("Eliot Blennerhassett <EBlennerhassett@audioscience.com>");
MODULE_DESCRIPTION("AudioScience soundcard (HPI)");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("{{AudioScience,HPI soundcard}}");

#define MAX_PCM_DEVICES	 4
#define MAX_PCM_SUBSTREAMS	16
#define MAX_MIDI_DEVICES	 0

/* defaults */
#ifndef MAX_BUFFER_SIZE
#define MAX_BUFFER_SIZE		(64*1024)
#endif

#define TIMER_JIFFIES 15

typedef struct snd_card_asihpi {
	snd_card_t *card;
	spinlock_t mixer_lock;
	// ASI 
	HW16 wAdapterIndex;
	HW32 dwSerialNumber;
	HW16 wType;
	HW16 wVersion;
	HW16 wNumOutStreams;
	HW16 wNumInStreams;

	HPI_HMIXER hMixer;
} snd_card_asihpi_t;

typedef struct snd_card_asihpi_pcm {
	snd_card_asihpi_t *asihpi;
	spinlock_t lock;
	struct timer_list timer;
	unsigned int hpi_in_buffer_allocated;
	unsigned int hpi_out_buffer_allocated;
	unsigned int pcm_size;
	unsigned int pcm_count;
	unsigned int pcm_jiffie_per_period;
	unsigned int pcm_irq_pos;	/* IRQ position */
	unsigned int pcm_buf_pos;	/* position in buffer */
	snd_pcm_substream_t *substream;
	HPI_HOSTREAM hStream;
	HPI_DATA Data;
	unsigned int pcm_hpi_pos;	/* position in buffer */

} snd_card_asihpi_pcm_t;

static HPI_HSUBSYS *phSubSys;	/* handle to HPI audio subsystem */
static snd_card_t *snd_asihpi_cards[SNDRV_CARDS] = SNDRV_DEFAULT_PTR;

static void _HandleError(HW16 err, int line, char *filename)
{
	char ErrorText[80];

	if (err) {
		HPI_GetErrorText(err, ErrorText);
		printk(KERN_WARNING "in file %s, line %d: %s\n", filename, line, ErrorText);
	}
}

#define HPI_HandleError(x)  _HandleError(x,__LINE__,__FILE__)
/***************************** GENERAL PCM ****************/
static int snd_card_asihpi_pcm_hw_params(snd_pcm_substream_t * substream,
					 snd_pcm_hw_params_t * params)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	unsigned int bps;

	bps = params_rate(params) * params_channels(params);
	bps *= snd_pcm_format_width(params_format(params));
	bps /= 8;
	if (bps <= 0)
		return -EINVAL;
	dpcm->pcm_size = params_buffer_bytes(params);
	dpcm->pcm_count = params_period_bytes(params);
	dpcm->pcm_jiffie_per_period = (dpcm->pcm_count * HZ / bps);
	snd_printd("Jiffies per period %ld, pcm_size %d, pcm_count %d\n",
			   (long)dpcm->pcm_jiffie_per_period,dpcm->pcm_size,    dpcm->pcm_count);
	dpcm->pcm_irq_pos = 0;
	dpcm->pcm_buf_pos = 0;
	return 0;
}

static void snd_card_asihpi_pcm_timer_start(snd_pcm_substream_t *
					    substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;

	dpcm->timer.expires = dpcm->pcm_jiffie_per_period + jiffies;
	add_timer(&dpcm->timer);
}

static void snd_card_asihpi_pcm_timer_stop(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;

	del_timer(&dpcm->timer);
}

static void snd_card_asihpi_runtime_free(snd_pcm_runtime_t * runtime)
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
					   HW16 * hpiFormat)
{
	HW16 wFormat;

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
	snd_card_asihpi_pcm_t *dpcm = (snd_card_asihpi_pcm_t *)data;
	snd_pcm_runtime_t *runtime = dpcm->substream->runtime;

	unsigned int pos;
	int delta;
	HW16 wState, err;
	HW32 dwBufferSize;
	HW32 dwDataToPlay;
	HW32 dwSamplesPlayed;

	dpcm->timer.expires = dpcm->pcm_jiffie_per_period + jiffies;
	add_timer(&dpcm->timer);
	spin_lock_irq(&dpcm->lock);

	err =
	    HPI_OutStreamGetInfoEx(phSubSys, dpcm->hStream, &wState,
				   &dwBufferSize, &dwDataToPlay,
				   &dwSamplesPlayed, NULL);
	HPI_HandleError(err);
	snd_printd(KERN_INFO
		   "Playback timer state=%d, played=%d, left=%d\n", wState,
		   (int)dwSamplesPlayed, (int)dwDataToPlay);

	if ((wState == HPI_STATE_DRAINED))	// pretend to keep moving, so alsa thinks all its data has been consumed.
		pos = dpcm->pcm_buf_pos + dpcm->pcm_count;
	else
		pos = frames_to_bytes(runtime, dwSamplesPlayed);

	pos %= dpcm->pcm_size;
	delta = pos - dpcm->pcm_buf_pos;
	if (delta < 0)
		delta += dpcm->pcm_size;
	dpcm->pcm_irq_pos += delta;
	dpcm->pcm_buf_pos = pos;

	if (dpcm->pcm_irq_pos >= dpcm->pcm_count) {
		dpcm->pcm_irq_pos %= dpcm->pcm_count;
		snd_pcm_period_elapsed(dpcm->substream);
	}
	spin_unlock_irq(&dpcm->lock);
}

static int snd_card_asihpi_playback_ioctl(snd_pcm_substream_t * substream,
					  unsigned int cmd, void *arg)
{
	snd_printd(KERN_INFO "playback ioctl %d\n", cmd);
	return snd_pcm_lib_ioctl(substream, cmd, arg);
}

static int snd_card_asihpi_playback_trigger(snd_pcm_substream_t *
					    substream, int cmd)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	HW16 err;

	snd_printd(KERN_INFO "playback trigger\n");
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		snd_printd("\tstart\n");
		err = HPI_OutStreamStart(phSubSys, dpcm->hStream);
		snd_card_asihpi_pcm_timer_start(substream);
		HPI_HandleError(err);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		snd_printd("\tstop\n");
		snd_card_asihpi_pcm_timer_stop(substream);
		//?     err = HPI_OutStreamStop(phSubSys,dpcm->hStream );
		err = HPI_OutStreamReset(phSubSys, dpcm->hStream);
		HPI_HandleError(err);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		snd_printd("\tpause release\n");
		err = HPI_OutStreamStart(phSubSys, dpcm->hStream);
		snd_card_asihpi_pcm_timer_start(substream);
		HPI_HandleError(err);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		snd_printd("\tpause push\n");
		snd_card_asihpi_pcm_timer_stop(substream);
		err = HPI_OutStreamStop(phSubSys, dpcm->hStream);
		HPI_HandleError(err);
	default:
		snd_printd("\tINVALID\n");
		return -EINVAL;
	}
	return 0;
}

static int snd_card_asihpi_playback_hw_params(snd_pcm_substream_t *
					      substream,
					      snd_pcm_hw_params_t * params)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	int err;
	HW16 wFormat;
	HW32 minBufSize;

	if ((err = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params))) < 0)
		return err;

	if ((err =
	     snd_card_asihpi_format_alsa2hpi(params_format(params),
					     &wFormat)) != 0) {
		return err;
	}

	err =
	    HPI_FormatCreate(&dpcm->Data.Format, params_channels(params),
			     wFormat, params_rate(params), 0, 0);
	HPI_HandleError(err);

	dpcm->Data.dwpbData = (HW32) runtime->dma_area;
	dpcm->Data.dwDataSize = MAX_BUFFER_SIZE;

	if (enable_bbm) {
		dpcm->hpi_out_buffer_allocated = 0;
		minBufSize = 0;

		err = HPI_StreamEstimateBufferSize( &dpcm->Data.Format, (HW32)( 1000 / HZ ), &minBufSize );
		snd_printd(KERN_INFO "OutStreamHostBuffer estimated size: %d bytes\n", (int)minBufSize);

		if ( minBufSize < MAX_BUFFER_SIZE )
			minBufSize = MAX_BUFFER_SIZE;

		if ( err == 0 ) {
			err = HPI_OutStreamHostBufferAllocate( phSubSys, dpcm->hStream, minBufSize );
			if ( err == 0 ) {
				dpcm->hpi_out_buffer_allocated = minBufSize;
				snd_printd(KERN_INFO "OutStreamHostBufferAllocate succeded\n");
			} else if ( err != 0 && err != HPI_ERROR_INVALID_FUNC ) {
				snd_printd(KERN_INFO "OutStreamHostBufferAllocate error(%d)\n", err);
				return err;
			}
		} else {
			snd_printd(KERN_INFO "Cannot estimate minimum bus master buffer size\n");
		}
		snd_printd(KERN_INFO "OutStreamHostBufferAllocate status(%d)\n", dpcm->hpi_out_buffer_allocated);
	}

	snd_printd(KERN_INFO "playback hwparams\n");

	return snd_card_asihpi_pcm_hw_params(substream, params);
}

static int
snd_card_asihpi_playback_hw_free(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	if ( dpcm->hpi_out_buffer_allocated ) {
		HPI_OutStreamHostBufferFree( phSubSys, dpcm->hStream );
	}

	snd_pcm_lib_free_pages(substream);
	return 0;
}


static int snd_card_asihpi_playback_prepare(snd_pcm_substream_t *
					    substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
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
snd_card_asihpi_playback_pointer(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;

	return bytes_to_frames(runtime, dpcm->pcm_buf_pos);
}

static snd_pcm_hardware_t snd_card_asihpi_playback = {
	.info = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_PAUSE,
	.rates = SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_48000,
	.rate_min = 5500,
	.rate_max = 48000,
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = MAX_BUFFER_SIZE,
	.period_bytes_min = MAX_BUFFER_SIZE / 64,
	.period_bytes_max = MAX_BUFFER_SIZE / 2,
	.periods_min = 2,
	.periods_max = 64,
	.fifo_size = 0,
};

static void snd_card_asihpi_playback_format(HPI_HOSTREAM hStream,
					    u64 * formats)
{
	HPI_FORMAT hpi_format = { 48000, 128000, 0, 0, 0, 2, 0 };
	HW16 wFormat;
	HW16 err;

	for (wFormat = HPI_FORMAT_PCM8_UNSIGNED;
	     wFormat <= HPI_FORMAT_PCM24_SIGNED; wFormat++) {
		hpi_format.wFormat = wFormat;
		err =
		    HPI_OutStreamQueryFormat(phSubSys, hStream,
					     &hpi_format);
		if (!err && (hpi_to_alsa_formats[wFormat] != -1)) {
			*formats |= (1ULL << hpi_to_alsa_formats[wFormat]);
			//snd_printk(KERN_INFO "HPI format %d, alsa %d\n",wFormat,hpi_to_alsa_formats[wFormat]);
		}
	}
}

static int snd_card_asihpi_playback_open(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm;
	snd_card_asihpi_t *asihpi = snd_pcm_substream_chip(substream);
	HW16 err;

	dpcm = kcalloc(1, sizeof(*dpcm), GFP_KERNEL);
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

	snd_card_asihpi_playback_format(dpcm->hStream,
					&snd_card_asihpi_playback.formats);

	init_timer(&dpcm->timer);
	dpcm->timer.data = (unsigned long) dpcm;
	dpcm->timer.function = snd_card_asihpi_playback_timer_function;
	spin_lock_init(&dpcm->lock);
	dpcm->substream = substream;
	runtime->private_data = dpcm;
	runtime->private_free = snd_card_asihpi_runtime_free;
	runtime->hw = snd_card_asihpi_playback;

	snd_printd(KERN_INFO "playback open\n");

	return 0;
}

static int snd_card_asihpi_playback_close(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	HW16 err;

	err = HPI_OutStreamClose(phSubSys, dpcm->hStream);
	HPI_HandleError(err);

	snd_printd(KERN_INFO "playback close\n");

	return 0;
}

static int snd_card_asihpi_playback_copy(snd_pcm_substream_t * substream,
					 int channel,
					 snd_pcm_uframes_t pos, void *src,
					 snd_pcm_uframes_t count)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	HW16 err;
	unsigned int len;

	len = frames_to_bytes(runtime, count);

	if (copy_from_user(runtime->dma_area, src, len))
		return -EFAULT;

	snd_printd(KERN_INFO "playback_copy %u bytes\n", len);

	dpcm->Data.dwDataSize = len;
	dpcm->Data.dwpbData = (HW32) runtime->dma_area;

	err = HPI_OutStreamWrite(phSubSys, dpcm->hStream, &dpcm->Data);
	HPI_HandleError(err);

	return 0;
}

static int snd_card_asihpi_playback_silence(snd_pcm_substream_t *
					    substream, int channel,
					    snd_pcm_uframes_t pos,
					    snd_pcm_uframes_t count)
{
	unsigned int len;
	HW16 err;
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;

	len = frames_to_bytes(runtime, count);
	snd_printd(KERN_INFO "playback silence  %u bytes\n", len);

	memset(runtime->dma_area, 0, len);
	err = HPI_OutStreamWrite(phSubSys, dpcm->hStream, &dpcm->Data);
	HPI_HandleError(err);
	return 0;
}

static snd_pcm_ops_t snd_card_asihpi_playback_ops = {
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
	snd_card_asihpi_pcm_t *dpcm = (snd_card_asihpi_pcm_t *)data;
	HW16 wState, err;
	HW32 dwBufferSize, dwDataToPlay, dwSamplesPlayed;

	dpcm->timer.expires = dpcm->pcm_jiffie_per_period + jiffies;
	add_timer(&dpcm->timer);
	spin_lock_irq(&dpcm->lock);

	err =
	    HPI_InStreamGetInfoEx(phSubSys, dpcm->hStream, &wState,
				  &dwBufferSize, &dwDataToPlay,
				  &dwSamplesPlayed, NULL);
	HPI_HandleError(err);

	dpcm->pcm_buf_pos =
	    (dpcm->pcm_irq_pos + dwDataToPlay) % dpcm->pcm_size;
	snd_printd("Capture timer %d samples, %d left, pos %d\n",
		   (int)dwSamplesPlayed, (int)dwDataToPlay, dpcm->pcm_buf_pos);
	if (dwDataToPlay >= dpcm->pcm_count) {
		snd_pcm_period_elapsed(dpcm->substream);
	}
	spin_unlock_irq(&dpcm->lock);
}

static int snd_card_asihpi_capture_ioctl(snd_pcm_substream_t * substream,
					 unsigned int cmd, void *arg)
{
	return snd_pcm_lib_ioctl(substream, cmd, arg);
}

static int snd_card_asihpi_capture_prepare(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	int err;

	err = HPI_InStreamReset(phSubSys, dpcm->hStream);
	HPI_HandleError(err);
	dpcm->pcm_irq_pos = 0;
	dpcm->pcm_buf_pos = 0;

	snd_printd("Capture Prepare\n");
	return 0;
}


static int snd_card_asihpi_capture_hw_params(snd_pcm_substream_t *
					     substream,
					     snd_pcm_hw_params_t * params)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	int err;
	HW16 wFormat;
	HW32 minBufSize;

	if ((err = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params))) < 0)
		return err;
	if ((err =
	     snd_card_asihpi_format_alsa2hpi(params_format(params),
					     &wFormat)) != 0)
		return err;

	// HPI_InstreamQueryFormat();

	err =
	    HPI_FormatCreate(&dpcm->Data.Format, params_channels(params),
			     wFormat, params_rate(params), 0, 0);
	HPI_HandleError(err);

	if ((err =
	     HPI_InStreamSetFormat(phSubSys, dpcm->hStream,
				   &dpcm->Data.Format)) != 0)
		return -EINVAL;

	dpcm->Data.dwpbData = (HW32) runtime->dma_area;
	dpcm->Data.dwDataSize = MAX_BUFFER_SIZE;

	if (enable_bbm) {
		dpcm->hpi_in_buffer_allocated = 0;
		minBufSize = 0;

		err = HPI_StreamEstimateBufferSize( &dpcm->Data.Format, (HW32)( 1000 / HZ ), &minBufSize );
		snd_printd(KERN_INFO "OutStreamHostBuffer estimated size: %d bytes\n", (int)minBufSize);

		if ( minBufSize < MAX_BUFFER_SIZE ) /* paranoia check */
			minBufSize = MAX_BUFFER_SIZE;

		if ( err == 0 ) {
			err = HPI_InStreamHostBufferAllocate( phSubSys, dpcm->hStream, minBufSize );
			if ( err == 0 ) {
				dpcm->hpi_in_buffer_allocated = minBufSize;
				snd_printd(KERN_INFO "InStreamHostBufferAllocate succeded");
			} else if ( err != 0 && err != HPI_ERROR_INVALID_FUNC ) {
				snd_printd(KERN_INFO "InStreamHostBufferAllocate error(%d)", err);
				return err;
			}
		} else {
			snd_printd(KERN_INFO "Cannot estimate minimum bus master buffer size\n");
		}
		snd_printd(KERN_INFO "InStreamHostBufferAllocate status(%d)", dpcm->hpi_in_buffer_allocated);
	}

	snd_printd(KERN_INFO "Capture hwparams\n");

	return snd_card_asihpi_pcm_hw_params(substream, params);
}

static int snd_card_asihpi_capture_hw_free(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	if ( dpcm->hpi_in_buffer_allocated ) {
		HPI_InStreamHostBufferFree( phSubSys, dpcm->hStream );
	}

	snd_pcm_lib_free_pages(substream);
	return 0;
}

static int snd_card_asihpi_capture_trigger(snd_pcm_substream_t * substream,
					   int cmd)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	HW16 err;

	if (cmd == SNDRV_PCM_TRIGGER_START) {
		snd_printd("Capture Trigger Start\n");
		err = HPI_InStreamStart(phSubSys, dpcm->hStream);
		snd_card_asihpi_pcm_timer_start(substream);
	} else if (cmd == SNDRV_PCM_TRIGGER_STOP) {
		snd_printd("Capture Trigger Stop\n");
		snd_card_asihpi_pcm_timer_stop(substream);
		err = HPI_InStreamStop(phSubSys, dpcm->hStream);
	} else {
		return -EINVAL;
	}
	return 0;
}

static snd_pcm_uframes_t
snd_card_asihpi_capture_pointer(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;

	//snd_printd("Capture pointer %d\n",dpcm->pcm_buf_pos);
	return bytes_to_frames(runtime, dpcm->pcm_buf_pos);
}

static snd_pcm_hardware_t snd_card_asihpi_capture = {
	.info = SNDRV_PCM_INFO_INTERLEAVED,
	.formats = SNDRV_PCM_FMTBIT_S16,
	.rates = SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_48000,
	.rate_min = 5500,
	.rate_max = 48000,
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = MAX_BUFFER_SIZE,
	.period_bytes_min = MAX_BUFFER_SIZE / 64,
	.period_bytes_max = MAX_BUFFER_SIZE,
	.periods_min = 1,
	.periods_max = 64,
	.fifo_size = 0,
};

static void snd_card_asihpi_capture_format(HPI_HOSTREAM hStream,
					   u64 * formats)
{
	HPI_FORMAT hpi_format = { 48000, 128000, 0, 0, 0, 2, 0 };
	HW16 wFormat;
	HW16 err;

	for (wFormat = HPI_FORMAT_PCM8_UNSIGNED;
	     wFormat <= HPI_FORMAT_PCM24_SIGNED; wFormat++) {
		hpi_format.wFormat = wFormat;
		err =
		    HPI_InStreamQueryFormat(phSubSys, hStream,
					    &hpi_format);
		if (!err)
			*formats |= (1ULL << hpi_to_alsa_formats[wFormat]);
	}
}

static int snd_card_asihpi_capture_open(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm;
	snd_card_asihpi_t *asihpi = snd_pcm_substream_chip(substream);
	HW16 err;

	dpcm = kcalloc(1, sizeof(*dpcm), GFP_KERNEL);
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

	snd_card_asihpi_capture_format(dpcm->hStream,
				       &snd_card_asihpi_capture.formats);

	init_timer(&dpcm->timer);
	dpcm->timer.data = (unsigned long) dpcm;
	dpcm->timer.function = snd_card_asihpi_capture_timer_function;
	spin_lock_init(&dpcm->lock);
	dpcm->substream = substream;
	runtime->private_data = dpcm;
	runtime->private_free = snd_card_asihpi_runtime_free;
	runtime->hw = snd_card_asihpi_capture;
	return 0;
}

static int snd_card_asihpi_capture_close(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	HW16 err;

	err = HPI_InStreamClose(phSubSys, dpcm->hStream);
	HPI_HandleError(err);

	return 0;
}

static int snd_card_asihpi_capture_copy(snd_pcm_substream_t * substream,
					int channel, snd_pcm_uframes_t pos,
					void *dst, snd_pcm_uframes_t count)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_asihpi_pcm_t *dpcm = runtime->private_data;
	HW16 err;

	dpcm->Data.dwpbData = (HW32) runtime->dma_area;
	dpcm->Data.dwDataSize = frames_to_bytes(runtime, count);

	//snd_printd("Capture copy %d bytes\n",dpcm->Data.dwDataSize);

	err = HPI_InStreamRead(phSubSys, dpcm->hStream, &dpcm->Data);
	HPI_HandleError(err);

	dpcm->pcm_irq_pos =
	    (dpcm->pcm_irq_pos + dpcm->Data.dwDataSize) % dpcm->pcm_size;

	if (copy_to_user(dst, runtime->dma_area, dpcm->Data.dwDataSize))
		return -EFAULT;

	return 0;
}

static snd_pcm_ops_t snd_card_asihpi_capture_ops = {
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

static int __init snd_card_asihpi_pcm(snd_card_asihpi_t * asihpi,
				      int device, int substreams)
{
	snd_pcm_t *pcm;
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
					      snd_dma_continuous_data(GFP_KERNEL),
					      0, MAX_BUFFER_SIZE);
	return 0;
}

/***************************** MIXER CONTROLS ****************/
typedef struct {
	HW16 wControlType;	// HPI_CONTROL_METER, _VOLUME etc
	HW16 wSrcNodeType;
	HW16 wSrcNodeIndex;
	HW16 wDstNodeType;
	HW16 wDstNodeIndex;
	HW16 wBand;
} hpi_control_t;

#define TEXT(a) a

#define ASIHPI_TUNER_BAND_STRINGS \
{ \
	TEXT("invalid"), \
	TEXT("AM"), \
	TEXT("FM (mono)"), \
	TEXT("TV"), \
	TEXT("FM (stereo)"), \
	TEXT("AUX") \
}

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

#if ( (NUM_SOURCENODE_STRINGS < (HPI_SOURCENODE_LAST_INDEX-HPI_SOURCENODE_BASE+1)) || (NUM_DESTNODE_STRINGS < (HPI_DESTNODE_LAST_INDEX-HPI_DESTNODE_BASE+1)))
#error "Fewer node strings than #defines - new HPI?"
#endif
#if ( (NUM_SOURCENODE_STRINGS > (HPI_SOURCENODE_LAST_INDEX-HPI_SOURCENODE_BASE+1)) || (NUM_DESTNODE_STRINGS < (HPI_DESTNODE_LAST_INDEX-HPI_DESTNODE_BASE+1)))
#warning "More Node strings than HPI defines - old HPI?"
#endif

static char *asihpi_src_names[] = ASIHPI_SOURCENODE_STRINGS;
static char *asihpi_dst_names[] = ASIHPI_DESTNODE_STRINGS;
static char *asihpi_tuner_band_names[] = ASIHPI_TUNER_BAND_STRINGS;

static inline int ctl_add( snd_card_t * card, snd_kcontrol_new_t * ctl, snd_card_asihpi_t * asihpi ) {
	int err;

	if ((err = snd_ctl_add(card, snd_ctl_new1(ctl, asihpi))) < 0) {
		return err;
	} else if (mixer_dump)
		snd_printk(KERN_INFO "Added %s(%d)\n", ctl->name, ctl->index);

	return 0;
}

static void asihpi_ctl_name_prefix( snd_kcontrol_new_t * snd_control, hpi_control_t * asihpi_control) {

	if (asihpi_control->wDstNodeType)
		sprintf(snd_control->name, "%s %s%d",
			asihpi_src_names[asihpi_control->wSrcNodeType],
			asihpi_dst_names[asihpi_control->wDstNodeType],
			asihpi_control->wDstNodeIndex + 1);
	else {
		strcpy(snd_control->name,
		       asihpi_src_names[asihpi_control->wSrcNodeType]);
	}

}

/*------------------------------------------------------------
   Volume controls
 ------------------------------------------------------------*/
static int snd_asihpi_volume_info(snd_kcontrol_t * kcontrol,
				  snd_ctl_elem_info_t * uinfo)
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	HW16 err;
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

static int snd_asihpi_volume_get(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_value_t * ucontrol)
{
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	HPI_HCONTROL hControl = kcontrol->private_value;
	short anGain0_01dB[HPI_MAX_CHANNELS];

	spin_lock_irqsave(&asihpi->mixer_lock, flags);
	HPI_VolumeGetGain(phSubSys, hControl, anGain0_01dB);
	ucontrol->value.integer.value[0] = anGain0_01dB[0] / 100;
	ucontrol->value.integer.value[1] = anGain0_01dB[1] / 100;

	spin_unlock_irqrestore(&asihpi->mixer_lock, flags);
	return 0;
}

static int snd_asihpi_volume_put(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_value_t * ucontrol)
{
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	int change;
	HPI_HCONTROL hControl = kcontrol->private_value;
	short anGain0_01dB[HPI_MAX_CHANNELS];


	anGain0_01dB[0] =
	    (ucontrol->value.integer.value[0]) * HPI_UNITS_PER_dB;
	anGain0_01dB[1] =
	    (ucontrol->value.integer.value[1]) * HPI_UNITS_PER_dB;
	spin_lock_irqsave(&asihpi->mixer_lock, flags);
	/*  change = asihpi->mixer_volume[addr][0] != left ||
	   asihpi->mixer_volume[addr][1] != right;
	 */
	change = 1;
	HPI_VolumeSetGain(phSubSys, hControl, anGain0_01dB);
	spin_unlock_irqrestore(&asihpi->mixer_lock, flags);
	return change;
}

static void __init snd_asihpi_volume_new(hpi_control_t * asihpi_control,
					 snd_kcontrol_new_t * snd_control)
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
static int snd_asihpi_level_info(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_info_t * uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = -20;
	uinfo->value.integer.max = 26;
	return 0;
}

static int snd_asihpi_level_get(snd_kcontrol_t * kcontrol,
				snd_ctl_elem_value_t * ucontrol)
{
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	HPI_HCONTROL hControl = kcontrol->private_value;
	short anGain0_01dB[HPI_MAX_CHANNELS];

	spin_lock_irqsave(&asihpi->mixer_lock, flags);
	HPI_LevelGetGain(phSubSys, hControl, anGain0_01dB);
	ucontrol->value.integer.value[0] =
	    anGain0_01dB[0] / HPI_UNITS_PER_dB;
	ucontrol->value.integer.value[1] =
	    anGain0_01dB[1] / HPI_UNITS_PER_dB;

	spin_unlock_irqrestore(&asihpi->mixer_lock, flags);
	return 0;
}

static int snd_asihpi_level_put(snd_kcontrol_t * kcontrol,
				snd_ctl_elem_value_t * ucontrol)
{
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	int change;
	HPI_HCONTROL hControl = kcontrol->private_value;
	short anGain0_01dB[HPI_MAX_CHANNELS];

	anGain0_01dB[0] =
	    (ucontrol->value.integer.value[0]) * HPI_UNITS_PER_dB;
	anGain0_01dB[1] =
	    (ucontrol->value.integer.value[1]) * HPI_UNITS_PER_dB;
	spin_lock_irqsave(&asihpi->mixer_lock, flags);
	/*  change = asihpi->mixer_level[addr][0] != left ||
	   asihpi->mixer_level[addr][1] != right;
	 */
	change = 1;
	HPI_LevelSetGain(phSubSys, hControl, anGain0_01dB);
	spin_unlock_irqrestore(&asihpi->mixer_lock, flags);
	return change;
}

static void __init snd_asihpi_level_new(hpi_control_t * asihpi_control,
					snd_kcontrol_new_t * snd_control)
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
	TEXT("No signal"), \
	TEXT("S/PDIF"), \
	TEXT("AES/EBU"), \
}

static char *asihpi_aesebu_format_names[] = ASIHPI_AESEBU_FORMAT_STRINGS;

static int snd_asihpi_aesebu_format_info(snd_kcontrol_t * kcontrol,
				  snd_ctl_elem_info_t * uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = 3;

	if (uinfo->value.enumerated.item >= uinfo->value.enumerated.items)
		uinfo->value.enumerated.item = uinfo->value.enumerated.items - 1;

	strcpy( uinfo->value.enumerated.name, 
		asihpi_aesebu_format_names[uinfo->value.enumerated.item]);

	return 0;
}

static int snd_asihpi_aesebu_format_get(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_value_t * ucontrol,
				 HW16 (*func)( HPI_HSUBSYS*, HPI_HCONTROL, HW16* ) )
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	HW16 source;

	snd_runtime_check( func(phSubSys, hControl, &source) != 0 , return -EINVAL );

	/* default to S/PDIF */
	ucontrol->value.enumerated.item[0] = 0;
	if ( source == HPI_AESEBU_SOURCE_SPDIF )
		ucontrol->value.enumerated.item[0] = 1;
	if ( source == HPI_AESEBU_SOURCE_AESEBU )
		ucontrol->value.enumerated.item[0] = 2;

	return 0;
}

static int snd_asihpi_aesebu_format_put(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_value_t * ucontrol,
				 HW16 (*func)( HPI_HSUBSYS*, HPI_HCONTROL, HW16 ) )
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	/* default to S/PDIF */
	HW16 source = HPI_AESEBU_SOURCE_SPDIF;

	if ( ucontrol->value.enumerated.item[0] == 1 )
		source = HPI_AESEBU_SOURCE_SPDIF;
	if ( ucontrol->value.enumerated.item[0] == 2 )
		source = HPI_AESEBU_SOURCE_AESEBU;

	snd_runtime_check( func(phSubSys, hControl, source) != 0, return -EINVAL );

	return 1;
}

static int snd_asihpi_aesebu_rx_source_get(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_value_t * ucontrol) {
	return snd_asihpi_aesebu_format_get( kcontrol, ucontrol, HPI_AESEBU_Receiver_GetSource );
}

static int snd_asihpi_aesebu_rx_source_put(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_value_t * ucontrol) {
	return snd_asihpi_aesebu_format_put( kcontrol, ucontrol, HPI_AESEBU_Receiver_SetSource );
}

static int snd_asihpi_aesebu_tx_format_get(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_value_t * ucontrol) {
	return snd_asihpi_aesebu_format_get( kcontrol, ucontrol, HPI_AESEBU_Transmitter_GetFormat );
}

static int snd_asihpi_aesebu_tx_format_put(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_value_t * ucontrol) {
	return snd_asihpi_aesebu_format_put( kcontrol, ucontrol, HPI_AESEBU_Transmitter_SetFormat );
}

/* TX clock source */

#define ASIHPI_AESEBU_CLOCKSOURCE_STRINGS \
{ \
	TEXT("Adapter"), \
	TEXT("AES/EBU sync"), \
}

static char *asihpi_aesebu_clocksource_names[] = ASIHPI_AESEBU_CLOCKSOURCE_STRINGS;

static int snd_asihpi_aesebu_tx_clocksource_info(snd_kcontrol_t * kcontrol,
				  snd_ctl_elem_info_t * uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = 2;

	if (uinfo->value.enumerated.item >= uinfo->value.enumerated.items)
		uinfo->value.enumerated.item = uinfo->value.enumerated.items - 1;

	strcpy( uinfo->value.enumerated.name, 
		asihpi_aesebu_clocksource_names[uinfo->value.enumerated.item]);

	return 0;
}

static int snd_asihpi_aesebu_tx_clocksource_get(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_value_t * ucontrol )
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	HW16 sync;

	snd_runtime_check( HPI_AESEBU_Transmitter_GetClockSource(phSubSys, hControl, &sync) );

	/* default to HPI_AESEBU_CLOCKSOURCE_ADAPTER */
	ucontrol->value.enumerated.item[0] = 0;
	if ( sync == HPI_AESEBU_CLOCKSOURCE_ADAPTER )
		ucontrol->value.enumerated.item[0] = 0;
	if ( sync == HPI_AESEBU_CLOCKSOURCE_AESEBU_SYNC )
		ucontrol->value.enumerated.item[0] = 1;

	return 0;
}

static int snd_asihpi_aesebu_tx_clocksource_put(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_value_t * ucontrol )
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	/* default to AES/EBU sync */
	HW16 sync = HPI_AESEBU_CLOCKSOURCE_AESEBU_SYNC;

	if ( ucontrol->value.enumerated.item[0] == 0 )
		sync = HPI_AESEBU_CLOCKSOURCE_ADAPTER;
	if ( ucontrol->value.enumerated.item[0] == 1 )
		sync = HPI_AESEBU_CLOCKSOURCE_AESEBU_SYNC;

	snd_runtime_check( HPI_AESEBU_Transmitter_SetClockSource(phSubSys, hControl, sync) != 0, return -EINVAL );

	return 1;
}


/* AESEBU control group initializers  */

static int __init snd_asihpi_aesebu_rx_new(snd_card_asihpi_t * asihpi, hpi_control_t * asihpi_control,
					snd_kcontrol_new_t * snd_control)
{

	snd_card_t *card = asihpi->card;

/* RX source */

	snd_control->info = snd_asihpi_aesebu_format_info;
	snd_control->get = snd_asihpi_aesebu_rx_source_get;
	snd_control->put = snd_asihpi_aesebu_rx_source_put;
	snd_control->index = asihpi_control->wSrcNodeIndex + 1;

	asihpi_ctl_name_prefix( snd_control, asihpi_control );
	strcat(snd_control->name, " Source");

	snd_runtime_check( ctl_add( card, snd_control, asihpi ) >= 0, return -EINVAL );

	return 0;
}

static int __init snd_asihpi_aesebu_tx_new(snd_card_asihpi_t * asihpi, hpi_control_t * asihpi_control,
					snd_kcontrol_new_t * snd_control)
{

	snd_card_t *card = asihpi->card;

/* TX Format */

	snd_control->info = snd_asihpi_aesebu_format_info;
	snd_control->get = snd_asihpi_aesebu_tx_format_get;
	snd_control->put = snd_asihpi_aesebu_tx_format_put;
	snd_control->index = asihpi_control->wSrcNodeIndex + 1;

	asihpi_ctl_name_prefix( snd_control, asihpi_control );
	strcat(snd_control->name, " Format");

	snd_runtime_check( ctl_add( card, snd_control, asihpi ) >= 0, return -EINVAL );

/* TX Sample rate source*/

	snd_control->info = snd_asihpi_aesebu_tx_clocksource_info;
	snd_control->get = snd_asihpi_aesebu_tx_clocksource_get;
	snd_control->put = snd_asihpi_aesebu_tx_clocksource_put;
	snd_control->index = asihpi_control->wSrcNodeIndex + 1;

	asihpi_ctl_name_prefix( snd_control, asihpi_control );
	strcat(snd_control->name, " Clock Source");

	snd_runtime_check( ctl_add( card, snd_control, asihpi ) >= 0, return -EINVAL );

	return 0;
}

/*------------------------------------------------------------
   Tuner controls
 ------------------------------------------------------------*/

/* Gain */

static int snd_asihpi_tuner_gain_info(snd_kcontrol_t * kcontrol,
				  snd_ctl_elem_info_t * uinfo)
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	HW16 err;
	short idx;
        HW32 gainRange[3];

	for (idx = 0; idx < 3; idx++) {
        	err = HPI_ControlQuery( phSubSys, hControl, HPI_TUNER_GAIN, idx, 0, &gainRange[idx] );
		if (err != 0) return err;
	}

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = ( (int)gainRange[0] ) / HPI_UNITS_PER_dB;
	uinfo->value.integer.max = ( (int)gainRange[1] ) / HPI_UNITS_PER_dB;
	uinfo->value.integer.step = ( (int) gainRange[2] ) / HPI_UNITS_PER_dB;
	return 0;
}

static int snd_asihpi_tuner_gain_get(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_value_t * ucontrol)
{
	/*
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	*/
	HPI_HCONTROL hControl = kcontrol->private_value;
	short gain;

	HPI_Tuner_GetGain(phSubSys, hControl, &gain);
	ucontrol->value.integer.value[0] = gain / HPI_UNITS_PER_dB;

	return 0;
}

static int snd_asihpi_tuner_gain_put(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_value_t * ucontrol)
{
	/*
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	*/
	HPI_HCONTROL hControl = kcontrol->private_value;
	short gain;

	gain = (ucontrol->value.integer.value[0]) * HPI_UNITS_PER_dB;
	HPI_Tuner_SetGain(phSubSys, hControl, gain);

	return 1;
}

/* Band  */

static int asihpi_tuner_band_query( snd_kcontrol_t * kcontrol, HW32 * bandList, HW32 len ) {
	HPI_HCONTROL hControl = kcontrol->private_value;
	HW16 err;
	HW32 idx;

	for (idx=0; idx < len; idx++) {
		err = HPI_ControlQuery( phSubSys, hControl , HPI_TUNER_BAND, idx, 0 , &bandList[idx]);
		if (err !=0) break;
	}

	return idx;
}

static int snd_asihpi_tuner_band_info(snd_kcontrol_t * kcontrol,
				  snd_ctl_elem_info_t * uinfo)
{
        HW32 tunerBands[HPI_TUNER_BAND_LAST];
	HW32 numBands = 0;

        numBands = asihpi_tuner_band_query( kcontrol, tunerBands, HPI_TUNER_BAND_LAST );

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = numBands;

	if (uinfo->value.enumerated.item >= uinfo->value.enumerated.items)
		uinfo->value.enumerated.item = uinfo->value.enumerated.items - 1;

	strcpy( uinfo->value.enumerated.name, 
		asihpi_tuner_band_names[tunerBands[uinfo->value.enumerated.item]]);

	return 0;
}

static int snd_asihpi_tuner_band_get(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_value_t * ucontrol)
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	/*
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	*/
	HW16 band, idx;
        HW32 tunerBands[HPI_TUNER_BAND_LAST];
	HW32 numBands = 0;

        numBands = asihpi_tuner_band_query( kcontrol, tunerBands, HPI_TUNER_BAND_LAST );

	HPI_Tuner_GetBand(phSubSys, hControl, &band);

	ucontrol->value.enumerated.item[0] = -1;
	for (idx = 0; idx < HPI_TUNER_BAND_LAST; idx++)
		if ( tunerBands[ idx ] == band ) {
			ucontrol->value.enumerated.item[0] = idx;
			break;
		}

	snd_runtime_check( ucontrol->value.enumerated.item[0] >= 0, return -EINVAL );

	return 0;
}

static int snd_asihpi_tuner_band_put(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_value_t * ucontrol)
{
	/*
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	*/
	HPI_HCONTROL hControl = kcontrol->private_value;
	HW16 band;
        HW32 tunerBands[HPI_TUNER_BAND_LAST];
	HW32 numBands = 0;

        numBands = asihpi_tuner_band_query( kcontrol, tunerBands, HPI_TUNER_BAND_LAST );

	band = tunerBands[ucontrol->value.enumerated.item[0]];
	HPI_Tuner_SetBand(phSubSys, hControl, band);

	return 1;
}

/* Freq */

static int snd_asihpi_tuner_freq_info(snd_kcontrol_t * kcontrol,
				  snd_ctl_elem_info_t * uinfo)
{
	HPI_HCONTROL hControl = kcontrol->private_value;
	HW16 err;
        HW32 tunerBands[HPI_TUNER_BAND_LAST];
	HW32 numBands = 0, band_iter, idx;
	HW32 freqRange[3], tempFreqRange[3];

        numBands = asihpi_tuner_band_query( kcontrol, tunerBands, HPI_TUNER_BAND_LAST );

	freqRange[0] = INT_MAX;
	freqRange[1] = 0;
	freqRange[2] = INT_MAX;

	for ( band_iter = 0; band_iter < numBands; band_iter++ ) {
		for (idx = 0; idx < 3; idx++) {
        		err = HPI_ControlQuery( phSubSys, hControl, HPI_TUNER_FREQ, idx, tunerBands[band_iter], &tempFreqRange[idx] );
			if (err != 0) return err;
		}

		/* skip band with bogus stepping */
		if ( tempFreqRange[2] <= 0 )
			continue;

		if ( tempFreqRange[0] < freqRange[0] )
			freqRange[0] = tempFreqRange[0];
		if ( tempFreqRange[1] > freqRange[1] )
			freqRange[1] = tempFreqRange[1];
		if ( tempFreqRange[2] < freqRange[2] )
			freqRange[2] = tempFreqRange[2];
	}

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = ( (int)freqRange[0] );
	uinfo->value.integer.max = ( (int)freqRange[1] );
	uinfo->value.integer.step = ( (int)freqRange[2] );
	return 0;
}

static int snd_asihpi_tuner_freq_get(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_value_t * ucontrol)
{
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	HPI_HCONTROL hControl = kcontrol->private_value;
	HW32 freq;

	spin_lock_irqsave(&asihpi->mixer_lock, flags);
	HPI_Tuner_GetFrequency(phSubSys, hControl, &freq);
	ucontrol->value.integer.value[0] = freq;
	spin_unlock_irqrestore(&asihpi->mixer_lock, flags);

	return 0;
}

static int snd_asihpi_tuner_freq_put(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_value_t * ucontrol)
{
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	HPI_HCONTROL hControl = kcontrol->private_value;
	HW32 freq;

	freq = ucontrol->value.integer.value[0];
	spin_lock_irqsave(&asihpi->mixer_lock, flags);
	HPI_Tuner_SetFrequency(phSubSys, hControl, freq);
	spin_unlock_irqrestore(&asihpi->mixer_lock, flags);

	return 1;
}

/* Tuner control group initializer  */

static int __init snd_asihpi_tuner_new(snd_card_asihpi_t * asihpi, hpi_control_t * asihpi_control,
					snd_kcontrol_new_t * snd_control)
{

	snd_card_t *card = asihpi->card;

/* Gain ctl */

	snd_control->info = snd_asihpi_tuner_gain_info;
	snd_control->get = snd_asihpi_tuner_gain_get;
	snd_control->put = snd_asihpi_tuner_gain_put;
	snd_control->index = asihpi_control->wSrcNodeIndex + 1;

	asihpi_ctl_name_prefix( snd_control, asihpi_control );
	strcat(snd_control->name, " Gain");

	snd_runtime_check( ctl_add( card, snd_control, asihpi ) != 0, return -EINVAL );

/* Band ctl */

	snd_control->info = snd_asihpi_tuner_band_info;
	snd_control->get = snd_asihpi_tuner_band_get;
	snd_control->put = snd_asihpi_tuner_band_put;
	snd_control->index = asihpi_control->wSrcNodeIndex + 1;

	asihpi_ctl_name_prefix( snd_control, asihpi_control );
	strcat(snd_control->name, " Band");

	snd_runtime_check( ctl_add( card, snd_control, asihpi ) != 0, return -EINVAL );

/* Freq ctl */

	snd_control->info = snd_asihpi_tuner_freq_info;
	snd_control->get = snd_asihpi_tuner_freq_get;
	snd_control->put = snd_asihpi_tuner_freq_put;
	snd_control->index = asihpi_control->wSrcNodeIndex + 1;

	asihpi_ctl_name_prefix( snd_control, asihpi_control );
	strcat(snd_control->name, " Freq");

	snd_runtime_check( ctl_add( card, snd_control, asihpi ) != 0, return -EINVAL );

/* Level meter */

	return 0;
}


/*------------------------------------------------------------
   Meter controls
 ------------------------------------------------------------*/
static int snd_asihpi_meter_info(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_info_t * uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = -192 * HPI_UNITS_PER_dB;
	uinfo->value.integer.max = 32767;
	return 0;
}

static int snd_asihpi_meter_get(snd_kcontrol_t * kcontrol,
				snd_ctl_elem_value_t * ucontrol)
{
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	HPI_HCONTROL hControl = kcontrol->private_value;
	short anGain0_01dB[HPI_MAX_CHANNELS], i;
	short nLinear = 0;
	HW16 err;

	spin_lock_irqsave(&asihpi->mixer_lock, flags);
	err = HPI_MeterGetRms(phSubSys, hControl, anGain0_01dB);

	// convert 0..32767 level to 0.01dB (20log10), 0 is -100.00dB
	for (i = 0; i < HPI_MAX_CHANNELS; i++) {
		nLinear = anGain0_01dB[i];
		if ((nLinear == 0) || err)
			anGain0_01dB[i] = -192 * HPI_UNITS_PER_dB;
		//      else if(nLinear > 0) { // actually should always be > 32  32768 => 0dB, 32.768=>-60dB
		//anGain0_01dB[i] = (short)((float)(20*log10((float)nLinear/32767.0))*100.0);// units are 0.01dB
		// }  // else don't have to touch the LogValue when it is -ve since it is already a log value
	}
	ucontrol->value.integer.value[0] = anGain0_01dB[0];
	ucontrol->value.integer.value[1] = anGain0_01dB[1];

	spin_unlock_irqrestore(&asihpi->mixer_lock, flags);
	return 0;
}


static void __init snd_asihpi_meter_new(hpi_control_t * asihpi_control,
					snd_kcontrol_new_t * snd_control)
{
	snd_control->info = snd_asihpi_meter_info;
	snd_control->get = snd_asihpi_meter_get;
	snd_control->put = 0;
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
static int snd_card_asihpi_mux_count_sources(snd_kcontrol_t * snd_control)
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

static int snd_asihpi_mux_info(snd_kcontrol_t * kcontrol,
			       snd_ctl_elem_info_t * uinfo)
{
	int err;
	HW16 wSrcNodeType, wSrcNodeIndex;
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

static int snd_asihpi_mux_get(snd_kcontrol_t * kcontrol,
			      snd_ctl_elem_value_t * ucontrol)
{
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	HPI_HCONTROL hControl = kcontrol->private_value;
	HW16 wSourceType, wSourceIndex;
	HW16 wSrcNodeType, wSrcNodeIndex;
	int s;

	spin_lock_irqsave(&asihpi->mixer_lock, flags);

	HPI_Multiplexer_GetSource(phSubSys, hControl, &wSourceType,
				  &wSourceIndex);
	// search to find the index with this source and index.  Should cache it!
	for (s = 0; s < 16; s++) {
		HPI_Multiplexer_QuerySource(phSubSys, hControl, s,
					    &wSrcNodeType, &wSrcNodeIndex);
		if ((wSourceType == wSrcNodeType)
		    && (wSourceIndex == wSrcNodeIndex)) {
			ucontrol->value.enumerated.item[0] = s;
			spin_unlock_irqrestore(&asihpi->mixer_lock, flags);
			return 0;
		}
	}
	spin_unlock_irqrestore(&asihpi->mixer_lock, flags);
	return -EINVAL;
}

static int snd_asihpi_mux_put(snd_kcontrol_t * kcontrol,
			      snd_ctl_elem_value_t * ucontrol)
{
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	int change;
	HPI_HCONTROL hControl = kcontrol->private_value;
	HW16 wSourceType, wSourceIndex;

	spin_lock_irqsave(&asihpi->mixer_lock, flags);
	change = 1;

	HPI_Multiplexer_QuerySource(phSubSys, hControl,
				    ucontrol->value.enumerated.item[0],
				    &wSourceType, &wSourceIndex);
	HPI_Multiplexer_SetSource(phSubSys, hControl, wSourceType,
				  wSourceIndex);
	return change;
}


static void __init snd_asihpi_mux_new(hpi_control_t * asihpi_control,
				      snd_kcontrol_new_t * snd_control)
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
static int snd_asihpi_cmode_info(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_info_t * uinfo)
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

static int snd_asihpi_cmode_get(snd_kcontrol_t * kcontrol,
				snd_ctl_elem_value_t * ucontrol)
{
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	HPI_HCONTROL hControl = kcontrol->private_value;
	HW16 wMode;

	spin_lock_irqsave(&asihpi->mixer_lock, flags);

	if (HPI_ChannelModeGet(phSubSys, hControl, &wMode))
		wMode = 1;

	ucontrol->value.enumerated.item[0] = wMode - 1;;

	spin_unlock_irqrestore(&asihpi->mixer_lock, flags);
	return 0;
}

static int snd_asihpi_cmode_put(snd_kcontrol_t * kcontrol,
				snd_ctl_elem_value_t * ucontrol)
{
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	int change;
	HPI_HCONTROL hControl = kcontrol->private_value;

	spin_lock_irqsave(&asihpi->mixer_lock, flags);
	change = 1;

	HPI_ChannelModeSet(phSubSys, hControl,
			   ucontrol->value.enumerated.item[0] + 1);
	return change;
}


static void __init snd_asihpi_cmode_new(hpi_control_t * asihpi_control,
					snd_kcontrol_new_t * snd_control)
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
static char *sampleclock_sources[] =
    { "bad", "Adapter", "AES/EBU", "Word External", "Word Header",
"SMPTE" };

static int snd_asihpi_clksrc_info(snd_kcontrol_t * kcontrol,
				  snd_ctl_elem_info_t * uinfo)
{

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = 5;

	if (uinfo->value.enumerated.item > 4)
		uinfo->value.enumerated.item = 4;

	strcpy(uinfo->value.enumerated.name,
	       sampleclock_sources[uinfo->value.enumerated.item + 1]);
	return 0;
}

static int snd_asihpi_clksrc_get(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_value_t * ucontrol)
{
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	HPI_HCONTROL hControl = kcontrol->private_value;
	HW16 wSource;

	spin_lock_irqsave(&asihpi->mixer_lock, flags);

	if (HPI_SampleClock_GetSource(phSubSys, hControl, &wSource))
		wSource = 1;

	ucontrol->value.enumerated.item[0] = wSource - 1;;

	spin_unlock_irqrestore(&asihpi->mixer_lock, flags);
	return 0;
}

static int snd_asihpi_clksrc_put(snd_kcontrol_t * kcontrol,
				 snd_ctl_elem_value_t * ucontrol)
{
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	int change;
	HPI_HCONTROL hControl = kcontrol->private_value;

	spin_lock_irqsave(&asihpi->mixer_lock, flags);
	change = 1;

	HPI_SampleClock_SetSource(phSubSys, hControl,
				  ucontrol->value.enumerated.item[0] + 1);
	return change;
}

static void __init snd_asihpi_clksrc_new(hpi_control_t * asihpi_control,
					 snd_kcontrol_new_t * snd_control)
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
	strcat(snd_control->name, " Source");
}

/*------------------------------------------------------------
   Clkrate controls
 ------------------------------------------------------------*/
static int snd_asihpi_clkrate_info(snd_kcontrol_t * kcontrol,
				   snd_ctl_elem_info_t * uinfo)
{
	//HPI_HCONTROL hControl= kcontrol->private_value;
	//HW16 err;

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 8000;
	uinfo->value.integer.max = 96000;
	uinfo->value.integer.step = 100;

	return 0;
}

static int snd_asihpi_clkrate_get(snd_kcontrol_t * kcontrol,
				  snd_ctl_elem_value_t * ucontrol)
{
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	HPI_HCONTROL hControl = kcontrol->private_value;
	HW32 dwRate;

	spin_lock_irqsave(&asihpi->mixer_lock, flags);
	HPI_SampleClock_GetSampleRate(phSubSys, hControl, &dwRate);
	ucontrol->value.integer.value[0] = dwRate;
	spin_unlock_irqrestore(&asihpi->mixer_lock, flags);
	return 0;
}

static int snd_asihpi_clkrate_put(snd_kcontrol_t * kcontrol,
				  snd_ctl_elem_value_t * ucontrol)
{
	snd_card_asihpi_t *asihpi = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	int change;
	HPI_HCONTROL hControl = kcontrol->private_value;

	spin_lock_irqsave(&asihpi->mixer_lock, flags);
	/*  change = asihpi->mixer_clkrate[addr][0] != left ||
	   asihpi->mixer_clkrate[addr][1] != right;
	 */
	change = 1;
	HPI_SampleClock_SetSampleRate(phSubSys, hControl,
				      ucontrol->value.integer.value[0]);
	spin_unlock_irqrestore(&asihpi->mixer_lock, flags);
	return change;
}

static void __init snd_asihpi_clkrate_new(hpi_control_t * asihpi_control,
					  snd_kcontrol_new_t * snd_control)
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

static int __init snd_card_asihpi_new_mixer(snd_card_asihpi_t * asihpi)
{
	snd_card_t *card = asihpi->card;
	unsigned int idx = 0;
	int err;
	HPI_HCONTROL hControl;
	snd_kcontrol_new_t snd_control;
	char snd_control_name[44];	// asound.h line 745 unsigned char name[44];
	hpi_control_t asihpi_control;

	snd_assert(asihpi != NULL, return -EINVAL);
	spin_lock_init(&asihpi->mixer_lock);
	strcpy(card->mixername, "Asihpi Mixer");

	err =
	    HPI_MixerOpen(phSubSys, asihpi->wAdapterIndex,
			  &asihpi->hMixer);
	HPI_HandleError(err);
	if (err)
		return -err;

	memset(&snd_control, 0, sizeof(snd_control));
	snd_control.iface = SNDRV_CTL_ELEM_IFACE_MIXER;
	snd_control.name = snd_control_name;

	/* now iterate through mixer controls */
	for (idx = 0; idx < 512; idx++) {
		err = HPI_MixerGetControlByIndex(phSubSys, asihpi->hMixer,
						 idx,
						 &asihpi_control.wSrcNodeType,
						 &asihpi_control.wSrcNodeIndex,
						 &asihpi_control.wDstNodeType,
						 &asihpi_control.wDstNodeIndex,
						 &asihpi_control.wControlType, 
						 &hControl);
		if (err) {
			if (err==HPI_ERROR_CONTROL_DISABLED) {
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
		    SNDRV_CTL_ELEM_ACCESS_WRITE |
		    SNDRV_CTL_ELEM_ACCESS_READ;

		switch (asihpi_control.wControlType) {
		case HPI_CONTROL_VOLUME:
			snd_asihpi_volume_new(&asihpi_control,
					      &snd_control);
			break;
		case HPI_CONTROL_LEVEL:
			snd_asihpi_level_new(&asihpi_control,
					     &snd_control);
			break;
		case HPI_CONTROL_MULTIPLEXER:
			snd_asihpi_mux_new(&asihpi_control, &snd_control);
			break;
		case HPI_CONTROL_CHANNEL_MODE:
			snd_asihpi_cmode_new(&asihpi_control,
					     &snd_control);
			break;
		case HPI_CONTROL_METER:
			snd_asihpi_meter_new(&asihpi_control,
					     &snd_control);
			break;
		case HPI_CONTROL_SAMPLECLOCK:
			snd_asihpi_clksrc_new(&asihpi_control,
					      &snd_control);
			if ((err =
			     snd_ctl_add(card,
					 snd_ctl_new1(&snd_control,
						      asihpi))) < 0)
				return err;
			snd_asihpi_clkrate_new(&asihpi_control,
					       &snd_control);
			break;
		case HPI_CONTROL_CONNECTION:	// ignore these
			continue;
		case HPI_CONTROL_TUNER:
			snd_runtime_check( snd_asihpi_tuner_new(asihpi, &asihpi_control,
					     &snd_control) >= 0, return -EINVAL );
			continue;
		case HPI_CONTROL_AESEBU_TRANSMITTER:
			snd_runtime_check( snd_asihpi_aesebu_tx_new(asihpi, &asihpi_control,
					     &snd_control) >= 0, return -EINVAL );
			continue;
		case HPI_CONTROL_AESEBU_RECEIVER:
			snd_runtime_check( snd_asihpi_aesebu_rx_new(asihpi, &asihpi_control,
					     &snd_control) >= 0, return -EINVAL );
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
					   "Untranslated HPI Control(%d) %d %d %d %d %d\n",
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
				 snd_ctl_new1(&snd_control,
					      asihpi))) < 0) {
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
snd_asihpi_proc_read(snd_info_entry_t * entry, snd_info_buffer_t * buffer)
{
	snd_card_asihpi_t *asihpi = entry->private_data;
	HW16 wVersion;
	HPI_HCONTROL hControl;
	HW32 dwRate=0;
	HW16 wSource=0;
	int err;

	snd_iprintf(buffer, "ASIHPI driver proc file\n");
	snd_iprintf(buffer,
		    "Adapter ID=%4X\nIndex=%d\nNumOutStreams=%d\nNumInStreams=%d\n",
		    asihpi->wType, asihpi->wAdapterIndex,
		    asihpi->wNumOutStreams, asihpi->wNumInStreams);

	wVersion = asihpi->wVersion;
	snd_iprintf(buffer,
		    "Serial#=%ld\nHw Version %c%d\nDSP code version %03d\n",
		    asihpi->dwSerialNumber, ((wVersion >> 3) & 0xf) + 'A',
		    wVersion & 0x7,
		    ((wVersion >> 13) * 100) + ((wVersion >> 7) & 0x3f));

	err = HPI_MixerGetControl(phSubSys, asihpi->hMixer,
				  HPI_SOURCENODE_CLOCK_SOURCE, 0, 0, 0,
				  HPI_CONTROL_SAMPLECLOCK, &hControl);

	if (!err) {
			err = HPI_SampleClock_GetSampleRate(phSubSys, hControl, &dwRate);
			err += HPI_SampleClock_GetSource(phSubSys, hControl, &wSource);

			if (!err)
					snd_iprintf(buffer, "SampleClock=%ldHz, source %s\n", dwRate,
								sampleclock_sources[wSource]);
	} 

}


static void __devinit snd_asihpi_proc_init(snd_card_asihpi_t * asihpi)
{
	snd_info_entry_t *entry;

	if (!snd_card_proc_new(asihpi->card, "info", &entry))
		snd_info_set_text_ops(entry, asihpi, 1024, snd_asihpi_proc_read);
}

/*------------------------------------------------------------
   CARD
 ------------------------------------------------------------*/
static int __init snd_card_asihpi_probe(int dev)
{
	HW16 wVersion;
	int pcm_substreams;

	snd_card_t *card;
	struct snd_card_asihpi *asihpi;
	int idx, err;

	HPI_HCONTROL hControl;

	//  if (!enable[dev])
	//return -ENODEV;
	err = HPI_AdapterOpen(phSubSys, dev);

	HPI_HandleError(err);
	if (err)
		return -ENODEV;

	// first try to give the card the same index as its hardware index
	card = snd_card_new(dev, SNDRV_DEFAULT_STR1, THIS_MODULE,
			    sizeof(struct snd_card_asihpi));
	if (card == NULL) {
		// if that fails, try the default index==next available
		card =
		    snd_card_new(SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1,
				 THIS_MODULE,
				 sizeof(struct snd_card_asihpi));
		if (card == NULL)
			return -ENOMEM;
		snd_printk(KERN_WARNING
			   "**** WARNING **** Adapter index %d->ALSA index %d\n",
			   dev, card->number);
	}

	asihpi = (struct snd_card_asihpi *) card->private_data;
	asihpi->card = card;
	asihpi->wAdapterIndex = dev;
	err = HPI_AdapterGetInfo(phSubSys,
				 asihpi->wAdapterIndex,
				 &asihpi->wNumOutStreams,
				 &asihpi->wNumInStreams,
				 &asihpi->wVersion,
				 &asihpi->dwSerialNumber, &asihpi->wType);
	HPI_HandleError(err);
	wVersion = asihpi->wVersion;
	snd_printk(KERN_INFO "Adapter ID=%4X Index=%d NumOutStreams=%d NumInStreams=%d S/N=%ld\n" "Hw Version %c%d DSP code version %03d\n", asihpi->wType, asihpi->wAdapterIndex, asihpi->wNumOutStreams, asihpi->wNumInStreams, asihpi->dwSerialNumber, ((wVersion >> 3) & 0xf) + 'A',	// Hw version major
		   wVersion & 0x7,	// Hw version minor
		   ((wVersion >> 13) * 100) + ((wVersion >> 7) & 0x3f)	// DSP code version
	    );

	idx = 0;
	pcm_substreams = asihpi->wNumOutStreams;
	if (pcm_substreams < asihpi->wNumInStreams)
		pcm_substreams = asihpi->wNumInStreams;
	if ((err = snd_card_asihpi_pcm(asihpi, idx, pcm_substreams)) < 0)
		goto __nodev;
	if ((err = snd_card_asihpi_new_mixer(asihpi)) < 0)
		goto __nodev;

	err = HPI_MixerGetControl(phSubSys, asihpi->hMixer,
				  HPI_SOURCENODE_CLOCK_SOURCE, 0, 0, 0,
				  HPI_CONTROL_SAMPLECLOCK, &hControl);

	if (!err)
			err = HPI_SampleClock_SetSampleRate(phSubSys, hControl, adapter_fs);

	snd_asihpi_proc_init(asihpi);

	strcpy(card->driver, "ASIHPI");
	sprintf(card->shortname, "AudioScience ASI%4X", asihpi->wType);
	sprintf(card->longname, "%s %i", card->shortname, dev + 1);
	if ((err = snd_card_register(card)) == 0) {
		snd_asihpi_cards[dev] = card;
		return 0;
	}
      __nodev:
	snd_card_free(card);
	return err;
}


static int __init alsa_card_asihpi_init(void)
{
	int dev, cards;

	HW16 err = 0;		// HPI error
	HW16 wNumAdapters = 0;
	HW16 awAdapterList[HPI_MAX_ADAPTERS];
	HW16 wListLength = HPI_MAX_ADAPTERS;
	HW32 dwVersion;

	snd_printd(KERN_INFO "ASIHPI driver start\n");

	phSubSys = HPI_SubSysCreate();
	if (phSubSys == NULL) {
		snd_printd(KERN_ERR
			   "phSubSys==NULL, ASI HPI driver not loaded?\n");
		return 0;
	}

	err = HPI_SubSysGetVersion(phSubSys, &dwVersion);
	HPI_HandleError(err);
	snd_printd(KERN_INFO "HPI_SubSysGetVersion=%lx\n", dwVersion);

	err = HPI_SubSysFindAdapters(phSubSys,
				     &wNumAdapters,
				     awAdapterList, wListLength);
	HPI_HandleError(err);
	snd_printd(KERN_INFO "HPI_SubSysFindAdapters NumberAdapters=%d \n",
		   wNumAdapters);

	for (dev = cards = 0; dev < HPI_MAX_ADAPTERS; dev++) {
		if (awAdapterList[dev] && snd_card_asihpi_probe(dev) < 0) {
#ifdef MODULE
			snd_printk(KERN_ERR
				   "Asihpi soundcard #%i not found or device busy\n",
				   dev + 1);
#endif
			break;
		}
		cards++;
	}
	if (!cards) {
#ifdef MODULE
		snd_printk(KERN_ERR
			   "Asihpi soundcard not found or device busy\n");
#endif
		return -ENODEV;
	}
	return 0;
}

static void __exit alsa_card_asihpi_exit(void)
{
	int idx;

	for (idx = 0; idx < SNDRV_CARDS; idx++)
		snd_card_free(snd_asihpi_cards[idx]);
}

module_init(alsa_card_asihpi_init)
module_exit(alsa_card_asihpi_exit)
