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
/* >0: print Hw params, timer vars. >1: print stream write/copy sizes  */
#define REALLY_VERBOSE_LOGGING 0

#if REALLY_VERBOSE_LOGGING
#define VPRINTK1 printk
#else
#define VPRINTK1(...)
#endif

#if REALLY_VERBOSE_LOGGING > 1
#define VPRINTK2 printk
#else
#define VPRINTK2(...)
#endif

#if REALLY_VERBOSE_LOGGING > 2
#define VPRINTK3 printk
#else
#define VPRINTK3(...)
#endif

#ifndef ASI_STYLE_NAMES
/* not sure how ALSA style name should look */
#define ASI_STYLE_NAMES 1
#endif

#include "hpi_internal.h"
#include "hpimsginit.h"
#include "hpioctl.h"

#ifndef KERNEL_ALSA_BUILD
/* building in ALSA source tree */
#include "adriver.h"
#else
/* building in kernel tree */
#include <sound/driver.h>
#endif

#include <linux/pci.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/info.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/hwdep.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("AudioScience Inc. <support@audioscience.com>");
MODULE_DESCRIPTION("AudioScience ALSA ASI5000 ASI6000 ASI87xx ASI89xx");

static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;	/* Index 0-MAX */
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;	/* ID for this card */
static int enable[SNDRV_CARDS] = SNDRV_DEFAULT_ENABLE_PNP;
static int enable_hpi_hwdep = 1;

module_param_array(index, int, NULL, S_IRUGO);
MODULE_PARM_DESC(index, "ALSA Index value for AudioScience soundcard.");

module_param_array(id, charp, NULL, S_IRUGO);
MODULE_PARM_DESC(id, "ALSA ID string for AudioScience soundcard.");

module_param_array(enable, bool, NULL, S_IRUGO);
MODULE_PARM_DESC(enable, "ALSA Enable AudioScience soundcard.");

module_param(enable_hpi_hwdep, bool, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(enable_hpi_hwdep,
		"ALSA Enable HPI hwdep for AudioScience soundcard ");

/* identify driver */
#ifdef KERNEL_ALSA_BUILD
static char *build_info = "Built using headers from kernel source";
module_param(build_info, charp, S_IRUGO);
MODULE_PARM_DESC(build_info, "Built using headers from kernel source");
#else
static char *build_info = "Built within ALSA source";
module_param(build_info, charp, S_IRUGO);
MODULE_PARM_DESC(build_info, "Built within ALSA source");
#endif

/* set to 1 to dump every control from adapter to log */
static const int mixer_dump;

#define DEFAULT_SAMPLERATE 44100
static int adapter_fs = DEFAULT_SAMPLERATE;

static struct hpi_hsubsys *phSubSys;	/* handle to HPI audio subsystem */

/* defaults */
#define PERIODS_MIN 2
#define PERIOD_BYTES_MIN  2304
#define BUFFER_BYTES_MAX (512 * 1024)

/*#define TIMER_MILLISECONDS 20
#define FORCE_TIMER_JIFFIES ((TIMER_MILLISECONDS * HZ + 999)/1000)
*/

#define MAX_CLOCKSOURCES (HPI_SAMPLECLOCK_SOURCE_LAST + 1 + 7)

struct clk_source {
	int source;
	int index;
	char *name;
};

struct clk_cache {
	int count;
	int has_local;
	struct clk_source s[MAX_CLOCKSOURCES];
};

/* Per card data */
struct snd_card_asihpi {
	struct snd_card *card;
	struct pci_dev *pci;
	u16 wAdapterIndex;
	u32 dwSerialNumber;
	u16 wType;
	u16 wVersion;
	u16 wNumOutStreams;
	u16 wNumInStreams;

	u32 hMixer;
	struct clk_cache cc;

	int support_mmap;
	int support_grouping;
	int support_mrx;
	u16 in_max_chans;
	u16 out_max_chans;
};

/* Per stream data */
struct snd_card_asihpi_pcm {
	struct timer_list timer;
	unsigned int respawn_timer;
	unsigned int hpi_buffer_attached;
	unsigned int pcm_size;
	unsigned int pcm_count;
	unsigned int bytes_per_sec;
	unsigned int pcm_irq_pos;	/* IRQ position */
	unsigned int pcm_buf_pos;	/* position in buffer */
	struct snd_pcm_substream *substream;
	u32 hStream;
	struct hpi_format Format;
};

/* universal stream verbs work with out or in stream handles */

/* Functions to allow driver to give a buffer to HPI for busmastering */

static u16 HPI_StreamHostBufferAttach(
	struct hpi_hsubsys *hS,
	u32 hStream,   /* Handle to OutStream. */
	u32 dwSizeInBytes, /* Size in bytes of bus mastering buffer */
	u32 dwPciAddress
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	unsigned int obj = HPI_HandleObject(hStream);

	if (!hStream)
		return HPI_ERROR_INVALID_OBJ;
	HPI_InitMessageResponse(&hm, &hr, obj,
			obj == HPI_OBJ_OSTREAM ?
				HPI_OSTREAM_HOSTBUFFER_ALLOC :
				HPI_ISTREAM_HOSTBUFFER_ALLOC);

	HPI_HandleToIndexes(hStream, &hm.wAdapterIndex,
				&hm.wObjIndex);

	hm.u.d.u.Buffer.dwBufferSize = dwSizeInBytes;
	hm.u.d.u.Buffer.dwPciAddress = dwPciAddress;
	hm.u.d.u.Buffer.dwCommand = HPI_BUFFER_CMD_INTERNAL_GRANTADAPTER;
	HPI_Message(&hm, &hr);
	return(hr.wError);
}

static u16 HPI_StreamHostBufferDetach(
	struct hpi_hsubsys *hS,
	u32  hStream
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	unsigned int obj = HPI_HandleObject(hStream);

	if (!hStream)
		return HPI_ERROR_INVALID_OBJ;

	HPI_InitMessageResponse(&hm, &hr,  obj,
			obj == HPI_OBJ_OSTREAM ?
				HPI_OSTREAM_HOSTBUFFER_FREE :
				HPI_ISTREAM_HOSTBUFFER_FREE);

	HPI_HandleToIndexes(hStream, &hm.wAdapterIndex,
				&hm.wObjIndex);
	hm.u.d.u.Buffer.dwCommand = HPI_BUFFER_CMD_INTERNAL_REVOKEADAPTER;
	HPI_Message(&hm, &hr);
	return(hr.wError);
}

static inline u16 HPI_StreamStart(struct hpi_hsubsys *hS, u32 hStream)
{
	if (HPI_HandleObject(hStream) ==  HPI_OBJ_OSTREAM)
		return HPI_OutStreamStart(hS, hStream);
	else
		return HPI_InStreamStart(hS, hStream);
}

static inline u16 HPI_StreamStop(struct hpi_hsubsys *hS, u32 hStream)
{
	if (HPI_HandleObject(hStream) ==  HPI_OBJ_OSTREAM)
		return HPI_OutStreamStop(hS, hStream);
	else
		return HPI_InStreamStop(hS, hStream);
}

static inline u16 HPI_StreamGetInfoEx(
    struct hpi_hsubsys *hS,
    u32 hStream,
    u16        *pwState,
    u32        *pdwBufferSize,
    u32        *pdwDataInBuffer,
    u32        *pdwSampleCount,
    u32        *pdwAuxiliaryData
)
{
	if (HPI_HandleObject(hStream)  ==  HPI_OBJ_OSTREAM)
		return HPI_OutStreamGetInfoEx(hS, hStream, pwState,
					pdwBufferSize, pdwDataInBuffer,
					pdwSampleCount, pdwAuxiliaryData);
	else
		return HPI_InStreamGetInfoEx(hS, hStream, pwState,
					pdwBufferSize, pdwDataInBuffer,
					pdwSampleCount, pdwAuxiliaryData);
}

static inline u16 HPI_StreamGroupAdd(struct hpi_hsubsys *hS,
					u32 hMaster,
					u32 hStream)
{
	if (HPI_HandleObject(hMaster) ==  HPI_OBJ_OSTREAM)
		return HPI_OutStreamGroupAdd(hS, hMaster, hStream);
	else
		return HPI_InStreamGroupAdd(hS, hMaster, hStream);
}

static inline u16 HPI_StreamGroupReset(struct hpi_hsubsys *hS,
						u32 hStream)
{
	if (HPI_HandleObject(hStream) ==  HPI_OBJ_OSTREAM)
		return HPI_OutStreamGroupReset(hS, hStream);
	else
		return HPI_InStreamGroupReset(hS, hStream);
}

static inline u16 HPI_StreamGroupGetMap(struct hpi_hsubsys *hS,
				u32 hStream, u32 *mo, u32 *mi)
{
	if (HPI_HandleObject(hStream) ==  HPI_OBJ_OSTREAM)
		return HPI_OutStreamGroupGetMap(hS, hStream, mo, mi);
	else
		return HPI_InStreamGroupGetMap(hS, hStream, mo, mi);
}

static u16 _HandleError(u16 err, int line, char *filename)
{
	if (err)
		printk(KERN_WARNING
			"in file %s, line %d: HPI error %d\n",
			filename, line, err);
	return err;
}

#define HPI_HandleError(x)  _HandleError(x, __LINE__, __FILE__)

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
}
#else
#define print_hwparams(x)
#endif

static snd_pcm_format_t hpi_to_alsa_formats[] = {
	-1,			/* INVALID */
	SNDRV_PCM_FORMAT_U8,	/* HPI_FORMAT_PCM8_UNSIGNED        1 */
	SNDRV_PCM_FORMAT_S16,	/* HPI_FORMAT_PCM16_SIGNED         2 */
	-1,			/* HPI_FORMAT_MPEG_L1              3 */
	SNDRV_PCM_FORMAT_MPEG,	/* HPI_FORMAT_MPEG_L2              4 */
	SNDRV_PCM_FORMAT_MPEG,	/* HPI_FORMAT_MPEG_L3              5 */
	-1,			/* HPI_FORMAT_DOLBY_AC2            6 */
	-1,			/* HPI_FORMAT_DOLBY_AC3            7 */
	SNDRV_PCM_FORMAT_S16_BE,/* HPI_FORMAT_PCM16_BIGENDIAN      8 */
	-1,			/* HPI_FORMAT_AA_TAGIT1_HITS       9 */
	-1,			/* HPI_FORMAT_AA_TAGIT1_INSERTS   10 */
	SNDRV_PCM_FORMAT_S32,	/* HPI_FORMAT_PCM32_SIGNED        11 */
	-1,			/* HPI_FORMAT_RAW_BITSTREAM       12 */
	-1,			/* HPI_FORMAT_AA_TAGIT1_HITS_EX1  13 */
	SNDRV_PCM_FORMAT_FLOAT,	/* HPI_FORMAT_PCM32_FLOAT         14 */
#if 1
	/* ALSA can't handle 3 byte sample size together with power-of-2
	 *  constraint on buffer_bytes, so disable this format
	 */
	-1
#else
	/* SNDRV_PCM_FORMAT_S24_3LE */	/* { HPI_FORMAT_PCM24_SIGNED        15 */
#endif
};


static int snd_card_asihpi_format_alsa2hpi(snd_pcm_format_t alsaFormat,
					   u16 *hpiFormat)
{
	u16 wFormat;

	for (wFormat = HPI_FORMAT_PCM8_UNSIGNED;
	     wFormat <= HPI_FORMAT_PCM24_SIGNED; wFormat++) {
		if (hpi_to_alsa_formats[wFormat] == alsaFormat) {
			*hpiFormat = wFormat;
			return 0;
		}
	}

	snd_printd(KERN_WARNING "Failed match for alsa format %d\n",
		   alsaFormat);
	*hpiFormat = 0;
	return -EINVAL;
}

static void snd_card_asihpi_pcm_samplerates(struct snd_card_asihpi *asihpi,
					 struct snd_pcm_hardware *pcmhw)
{
	u16 err;
	u32 hControl;
	u32 sampleRate;
	int idx;
	unsigned int rate_min = 200000;
	unsigned int rate_max = 0;
	unsigned int rates = 0;

	if (asihpi->support_mrx) {
		rates |= SNDRV_PCM_RATE_CONTINUOUS;
		rates |= SNDRV_PCM_RATE_8000_96000;
		rate_min = 8000;
		rate_max = 100000;
	} else {
		/* on cards without SRC,
		   valid rates are determined by sampleclock */
		err = HPI_MixerGetControl(phSubSys, asihpi->hMixer,
					  HPI_SOURCENODE_CLOCK_SOURCE, 0, 0, 0,
					  HPI_CONTROL_SAMPLECLOCK, &hControl);
		if (err) {
			printk(KERN_ERR "No local sampleclock, err %d\n", err);
		}

		for (idx = 0; idx < 100; idx++) {
			if (HPI_SampleClock_QueryLocalRate(phSubSys, hControl,
					idx, &sampleRate)) {
				if (!idx)
					printk(KERN_ERR "Local rate query failed\n");

				break;
			}

			rate_min = min(rate_min, sampleRate);
			rate_max = max(rate_max, sampleRate);

			switch (sampleRate) {
			case 5512:
				rates |= SNDRV_PCM_RATE_5512;
				break;
			case 8000:
				rates |= SNDRV_PCM_RATE_8000;
				break;
			case 11025:
				rates |= SNDRV_PCM_RATE_11025;
				break;
			case 16000:
				rates |= SNDRV_PCM_RATE_16000;
				break;
			case 22050:
				rates |= SNDRV_PCM_RATE_22050;
				break;
			case 32000:
				rates |= SNDRV_PCM_RATE_32000;
				break;
			case 44100:
				rates |= SNDRV_PCM_RATE_44100;
				break;
			case 48000:
				rates |= SNDRV_PCM_RATE_48000;
				break;
			case 64000:
				rates |= SNDRV_PCM_RATE_64000;
				break;
			case 88200:
				rates |= SNDRV_PCM_RATE_88200;
				break;
			case 96000:
				rates |= SNDRV_PCM_RATE_96000;
				break;
			case 176400:
				rates |= SNDRV_PCM_RATE_176400;
				break;
			case 192000:
				rates |= SNDRV_PCM_RATE_192000;
				break;
			default: /* some other rate */
				rates |= SNDRV_PCM_RATE_KNOT;
			}
		}
	}

	/* printk(KERN_INFO "Supported rates %X %d %d\n",
	   rates, rate_min, rate_max); */
	pcmhw->rates = rates;
	pcmhw->rate_min = rate_min;
	pcmhw->rate_max = rate_max;

}

static int snd_card_asihpi_pcm_hw_params(struct snd_pcm_substream *substream,
					 struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_card_asihpi_pcm *dpcm = runtime->private_data;
	struct snd_card_asihpi *card = snd_pcm_substream_chip(substream);
	int err;
	u16 wFormat;
	unsigned int bytes_per_sec;

	print_hwparams(params);
	err = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
	if (err < 0)
		return err;
	err = snd_card_asihpi_format_alsa2hpi(params_format(params), &wFormat);
	if (err)
		return err;

	VPRINTK1(KERN_INFO "Format %d, %d chans, %dHz\n",
				wFormat,params_channels(params),
				params_rate(params));

	HPI_HandleError(HPI_FormatCreate(&dpcm->Format, params_channels(params),
			     wFormat, params_rate(params), 0, 0));

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (HPI_InStreamReset(phSubSys, dpcm->hStream) != 0)
			return -EINVAL;

		if (HPI_InStreamSetFormat(phSubSys,
			dpcm->hStream, &dpcm->Format) != 0)
			return -EINVAL;
	}

	dpcm->hpi_buffer_attached = 0;
	if (card->support_mmap) {

		err = HPI_StreamHostBufferAttach(phSubSys, dpcm->hStream,
			params_buffer_bytes(params),  runtime->dma_addr);
		if (err == 0) {
			snd_printd(KERN_INFO
				"StreamHostBufferAttach succeeded %u %lu\n",
				params_buffer_bytes(params),
				(unsigned long)runtime->dma_addr);
		} else {
			snd_printd(KERN_INFO
					"StreamHostBufferAttach error %d\n",
					err);
			return -ENOMEM;
		}

		err = HPI_StreamGetInfoEx(phSubSys, dpcm->hStream, NULL,
						&dpcm->hpi_buffer_attached,
						NULL, NULL, NULL);

		snd_printd(KERN_INFO "StreamHostBufferAttach status 0x%x\n",
				dpcm->hpi_buffer_attached);
	}
	bytes_per_sec = params_rate(params) * params_channels(params);
	bytes_per_sec *= snd_pcm_format_width(params_format(params));
	bytes_per_sec /= 8;
	if (bytes_per_sec <= 0)
		return -EINVAL;

	dpcm->bytes_per_sec = bytes_per_sec;
	dpcm->pcm_size = params_buffer_bytes(params);
	dpcm->pcm_count = params_period_bytes(params);
	snd_printd(KERN_INFO "pcm_size x%x, pcm_count x%x, Bps %d\n",
			dpcm->pcm_size, dpcm->pcm_count, bytes_per_sec);

	dpcm->pcm_irq_pos = 0;
	dpcm->pcm_buf_pos = 0;
	return 0;
}

static void snd_card_asihpi_pcm_timer_start(struct snd_pcm_substream *
					    substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_card_asihpi_pcm *dpcm = runtime->private_data;
	int expiry;

	/* wait longer the first time, for samples to propagate */
	expiry = (dpcm->pcm_count * HZ / dpcm->bytes_per_sec);
	expiry = max(expiry, 20);
	dpcm->timer.expires = jiffies + expiry;
	dpcm->respawn_timer = 1;
	add_timer(&dpcm->timer);
}

static void snd_card_asihpi_pcm_timer_stop(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_card_asihpi_pcm *dpcm = runtime->private_data;

	dpcm->respawn_timer = 0;
	del_timer(&dpcm->timer);
}

static int snd_card_asihpi_trigger(struct snd_pcm_substream *substream,
					   int cmd)
{
	struct snd_card_asihpi_pcm *dpcm = substream->runtime->private_data;
	struct snd_card_asihpi *card = snd_pcm_substream_chip(substream);
	struct snd_pcm_substream *s;
	u16 e;

	snd_printd("Trigger %dstream %d\n",
			substream->stream, substream->number);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		snd_pcm_group_for_each_entry(s, substream) {
			struct snd_card_asihpi_pcm *ds;
			ds = s->runtime->private_data;

			if (snd_pcm_substream_chip(s) != card)
				continue;

			if ((s->stream == SNDRV_PCM_STREAM_PLAYBACK) &&
				(card->support_mmap)) {
				/* How do I know how much valid data is present
				* in buffer? Just guessing 2 periods, but if
				* buffer is bigger it may contain even more
				* data??
				*/
				unsigned int preload = ds->pcm_count*2;
				VPRINTK2("Preload x%x\n", preload);
				HPI_HandleError(HPI_OutStreamWriteBuf(phSubSys, ds->hStream,
						&s->runtime->dma_area[0],
						preload,
						&ds->Format));
				ds->pcm_irq_pos = ds->pcm_irq_pos + preload;
			}

			if (card->support_grouping) {
				VPRINTK1("\tGroup %dstream %d\n", s->stream,
						s->number);
				e = HPI_StreamGroupAdd(phSubSys, dpcm->hStream,
							ds->hStream);
				if (!e) {
					snd_pcm_trigger_done(s, substream);
				} else {
					HPI_HandleError(e);
					break;
				}
			} else
				break;
		}
		snd_printd("Start\n");
		/* start the master stream */
		snd_card_asihpi_pcm_timer_start(substream);
		HPI_HandleError(HPI_StreamStart(phSubSys, dpcm->hStream));
		break;

	case SNDRV_PCM_TRIGGER_STOP:
		snd_card_asihpi_pcm_timer_stop(substream);
		snd_pcm_group_for_each_entry(s, substream) {
			if (snd_pcm_substream_chip(s) != card)
				continue;

			/*? workaround linked streams don't
			transition to SETUP 20070706*/
			s->runtime->status->state = SNDRV_PCM_STATE_SETUP;

			if (card->support_grouping) {
				VPRINTK1("\tGroup %dstream %d\n", s->stream,
					s->number);
				snd_pcm_trigger_done(s, substream);
			} else
				break;
		}
		snd_printd("Stop\n");

		/* _prepare and _hwparams reset the stream */
		HPI_HandleError(HPI_StreamStop(phSubSys, dpcm->hStream));
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			HPI_HandleError(HPI_OutStreamReset(phSubSys, dpcm->hStream));

		if (card->support_grouping)
			HPI_HandleError(HPI_StreamGroupReset(phSubSys,
						dpcm->hStream));
		break;

	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		snd_printd("Pause release\n");
		HPI_HandleError(HPI_StreamStart(phSubSys, dpcm->hStream));
		snd_card_asihpi_pcm_timer_start(substream);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		snd_printd("Pause\n");
		snd_card_asihpi_pcm_timer_stop(substream);
		HPI_HandleError(HPI_StreamStop(phSubSys, dpcm->hStream));
		break;
	default:
		snd_printd("\tINVALID\n");
		return -EINVAL;
	}

	return 0;
}

static int
snd_card_asihpi_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_card_asihpi_pcm *dpcm = runtime->private_data;
	if (dpcm->hpi_buffer_attached)
		HPI_StreamHostBufferDetach(phSubSys, dpcm->hStream);

	snd_pcm_lib_free_pages(substream);
	return 0;
}

static void snd_card_asihpi_runtime_free(struct snd_pcm_runtime *runtime)
{
	struct snd_card_asihpi_pcm *dpcm = runtime->private_data;
	kfree(dpcm);
}

/*algorithm outline
 Without linking degenerates to getting single stream pos etc
 Without mmap 2nd loop degenerates to snd_pcm_period_elapsed
*/
/*
buf_pos=get_buf_pos(s);
for_each_linked_stream(s) {
	buf_pos=get_buf_pos(s);
	min_buf_pos = modulo_min(min_buf_pos, buf_pos, pcm_size)
	new_data = min(new_data, calc_new_data(buf_pos,irq_pos)
}
timer.expires = jiffies + predict_next_period_ready(min_buf_pos);
for_each_linked_stream(s) {
	s->buf_pos = min_buf_pos;
	if (new_data > pcm_count) {
		if (mmap) {
			irq_pos = (irq_pos + pcm_count) % pcm_size;
			if (playback) {
				write(pcm_count);
			} else {
				read(pcm_count);
			}
		}
		snd_pcm_period_elapsed(s);
	}
}
*/

/** Minimum of 2 modulo values.  Works correctly when the difference between
* the values is less than half the modulus
*/
static inline unsigned int modulo_min(unsigned int a, unsigned int b,
					unsigned long int modulus)
{
	unsigned int result;
	if (((a-b) % modulus) < (modulus/2))
		result = b;
	else
		result = a;

	return result;
}

/** Timer function, equivalent to interrupt service routine for cards
*/
static void snd_card_asihpi_timer_function(unsigned long data)
{
	struct snd_card_asihpi_pcm *dpcm = (struct snd_card_asihpi_pcm *)data;
	struct snd_card_asihpi *card = snd_pcm_substream_chip(dpcm->substream);
	struct snd_pcm_runtime *runtime;
	struct snd_pcm_substream *s;
	unsigned int newdata = 0;
	unsigned int buf_pos, min_buf_pos = 0;
	unsigned int remdata, xfercount, next_jiffies;
	int first = 1;
	u16 wState;
	u32 dwBufferSize, dwDataAvail, dwSamplesPlayed, dwAux;

	/* find minimum newdata and buffer pos in group */
	snd_pcm_group_for_each_entry(s, dpcm->substream) {
		struct snd_card_asihpi_pcm *ds = s->runtime->private_data;
		runtime = s->runtime;

		if (snd_pcm_substream_chip(s) != card)
			continue;

		HPI_HandleError(HPI_StreamGetInfoEx(phSubSys,
					ds->hStream, &wState,
					&dwBufferSize, &dwDataAvail,
					&dwSamplesPlayed, &dwAux));

		if (wState == HPI_STATE_DRAINED) {
			snd_printd(KERN_WARNING  "OStream %d drained\n",
					s->number);
			snd_pcm_stop(s, SNDRV_PCM_STATE_XRUN);
			return;
		}

		if (s->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			buf_pos = frames_to_bytes(runtime, dwSamplesPlayed);
		} else {
			buf_pos = dwDataAvail + ds->pcm_irq_pos;
		}

		if (first) {
			/* can't statically init min when wrap is involved */
			min_buf_pos = buf_pos;
			newdata = (buf_pos - ds->pcm_irq_pos) % ds->pcm_size;
			first = 0;
		} else {
			min_buf_pos =
				modulo_min(min_buf_pos, buf_pos, UINT_MAX+1L);
			newdata = min(
				(buf_pos - ds->pcm_irq_pos) % ds->pcm_size,
				newdata);
		}

		VPRINTK1("PB timer hw_ptr x%04lX, appl_ptr x%04lX ",
			(unsigned long)frames_to_bytes(runtime,
						runtime->status->hw_ptr),
			(unsigned long)frames_to_bytes(runtime,
						runtime->control->appl_ptr));
		VPRINTK1("%d S=%d, irq=%04X, pos=x%04X, left=x%04X,"
			" aux=x%04X space=x%04X\n", s->number,
			wState,	ds->pcm_irq_pos, buf_pos, (int)dwDataAvail,
			(int)dwAux, dwBufferSize-dwDataAvail);
	}

	remdata = newdata % dpcm->pcm_count;
	xfercount = newdata - remdata; /* a multiple of pcm_count */
	next_jiffies = ((dpcm->pcm_count-remdata) * HZ / dpcm->bytes_per_sec)+1;
	next_jiffies = max(next_jiffies, 2U * HZ / 1000U);
	dpcm->timer.expires = jiffies + next_jiffies;
	VPRINTK1("jif %d buf pos x%04X newdata x%04X\n",
			next_jiffies, min_buf_pos, newdata);

	snd_pcm_group_for_each_entry(s, dpcm->substream) {
		struct snd_card_asihpi_pcm *ds = s->runtime->private_data;
		ds->pcm_buf_pos = min_buf_pos;

		if (xfercount) {
			if (card->support_mmap) {
				ds->pcm_irq_pos = ds->pcm_irq_pos + xfercount;
				if (s->stream == SNDRV_PCM_STREAM_PLAYBACK) {
					VPRINTK2("Write OS%d x%04x\n",
							s->number,
							ds->pcm_count);
					HPI_HandleError(
						HPI_OutStreamWriteBuf(
							phSubSys, ds->hStream,
							&s->runtime->
								dma_area[0],
							xfercount,
							&ds->Format));
				} else {
					VPRINTK2("Read IS%d x%04x\n",
						s->number,
						dpcm->pcm_count);
					HPI_HandleError(
						HPI_InStreamReadBuf(
							phSubSys, ds->hStream,
							NULL, xfercount));
				}
			} /* else R/W will be handled by read/write callbacks */
			snd_pcm_period_elapsed(s);
		}
	}

	if (dpcm->respawn_timer)
		add_timer(&dpcm->timer);
}

/***************************** PLAYBACK OPS ****************/
static int snd_card_asihpi_playback_ioctl(struct snd_pcm_substream *substream,
					  unsigned int cmd, void *arg)
{
	/* snd_printd(KERN_INFO "Playback ioctl %d\n", cmd); */
	return snd_pcm_lib_ioctl(substream, cmd, arg);
}

static int snd_card_asihpi_playback_prepare(struct snd_pcm_substream *
					    substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_card_asihpi_pcm *dpcm = runtime->private_data;

	snd_printd(KERN_INFO "Playback prepare %d\n", substream->number);

	HPI_HandleError(HPI_OutStreamReset(phSubSys, dpcm->hStream));
	dpcm->pcm_irq_pos = 0;
	dpcm->pcm_buf_pos = 0;

	return 0;
}

static snd_pcm_uframes_t
snd_card_asihpi_playback_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_card_asihpi_pcm *dpcm = runtime->private_data;
	snd_pcm_uframes_t ptr;
	u32 dwSamplesPlayed;
	u16 err;

	if (!snd_pcm_stream_linked(substream)) {
		/* NOTE, can use samples played for playback position here and
		* in timer fn because it LAGS the actual read pointer, and is a
		* better representation of actual playout position
		*/
		err = HPI_OutStreamGetInfoEx(phSubSys, dpcm->hStream, NULL,
					NULL, NULL,
					&dwSamplesPlayed, NULL);
		HPI_HandleError(err);

		dpcm->pcm_buf_pos = frames_to_bytes(runtime, dwSamplesPlayed);
	}
	/* else must return most conservative value found in timer func
	 * by looping over all streams
	 */

	ptr = bytes_to_frames(runtime, dpcm->pcm_buf_pos  % dpcm->pcm_size);
	VPRINTK2("Playback ptr x%04lx\n", (unsigned long)ptr);
	return ptr;
}

static void snd_card_asihpi_playback_format(struct snd_card_asihpi *asihpi,
						u32 hStream,
						struct snd_pcm_hardware *pcmhw)
{
	struct hpi_format hpi_format;
	u16 wFormat;
	u16 err;
	u32 hControl;
	u32 dwSampleRate = 48000;

	/* on cards without SRC, must query at valid rate,
	* maybe set by external sync
	*/
	err = HPI_MixerGetControl(phSubSys, asihpi->hMixer,
				  HPI_SOURCENODE_CLOCK_SOURCE, 0, 0, 0,
				  HPI_CONTROL_SAMPLECLOCK, &hControl);

	if (!err)
		err = HPI_SampleClock_GetSampleRate(phSubSys, hControl,
				&dwSampleRate);

	for (wFormat = HPI_FORMAT_PCM8_UNSIGNED;
	     wFormat <= HPI_FORMAT_PCM24_SIGNED; wFormat++) {
		err = HPI_FormatCreate(&hpi_format,
					2, wFormat, dwSampleRate, 128000, 0);
		if (!err)
			err = HPI_OutStreamQueryFormat(phSubSys, hStream,
							&hpi_format);
		if (!err && (hpi_to_alsa_formats[wFormat] != -1))
			pcmhw->formats |=
				(1ULL << hpi_to_alsa_formats[wFormat]);
	}
}

static struct snd_pcm_hardware snd_card_asihpi_playback = {
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = BUFFER_BYTES_MAX,
	.period_bytes_min = PERIOD_BYTES_MIN,
	.period_bytes_max = BUFFER_BYTES_MAX / PERIODS_MIN,
	.periods_min = PERIODS_MIN,
	.periods_max = BUFFER_BYTES_MAX / PERIOD_BYTES_MIN,
	.fifo_size = 0,
};

static int snd_card_asihpi_playback_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_card_asihpi_pcm *dpcm;
	struct snd_card_asihpi *card = snd_pcm_substream_chip(substream);
	int err;

	dpcm = kzalloc(sizeof(*dpcm), GFP_KERNEL);
	if (dpcm == NULL)
		return -ENOMEM;

	err =
	    HPI_OutStreamOpen(phSubSys, card->wAdapterIndex,
			      substream->number, &dpcm->hStream);
	HPI_HandleError(err);
	if (err)
		kfree(dpcm);
	if (err == HPI_ERROR_OBJ_ALREADY_OPEN)
		return -EBUSY;
	if (err)
		return -EIO;

	/*? also check ASI5000 samplerate source
	    If external, only support external rate.
	    If internal and other stream playing, cant switch
	*/

	init_timer(&dpcm->timer);
	dpcm->timer.data = (unsigned long) dpcm;
	dpcm->timer.function = snd_card_asihpi_timer_function;
	dpcm->substream = substream;
	runtime->private_data = dpcm;
	runtime->private_free = snd_card_asihpi_runtime_free;

	snd_card_asihpi_playback.channels_max = card->out_max_chans;
	/*?snd_card_asihpi_playback.period_bytes_min =
	card->out_max_chans * 4096; */

	snd_card_asihpi_playback_format(card, dpcm->hStream,
					&snd_card_asihpi_playback);

	snd_card_asihpi_pcm_samplerates(card,  &snd_card_asihpi_playback);

	snd_card_asihpi_playback.info = SNDRV_PCM_INFO_INTERLEAVED |
					SNDRV_PCM_INFO_PAUSE;

	if (card->support_mmap)
		snd_card_asihpi_playback.info |= SNDRV_PCM_INFO_MMAP |
						SNDRV_PCM_INFO_MMAP_VALID;

	if (card->support_grouping)
		snd_card_asihpi_playback.info |= SNDRV_PCM_INFO_SYNC_START;

	/* struct is copied, so can create initializer dynamically */
	runtime->hw = snd_card_asihpi_playback;

	if (card->support_mmap)
		err = snd_pcm_hw_constraint_pow2(runtime, 0,
					SNDRV_PCM_HW_PARAM_BUFFER_BYTES);
	if (err < 0)
		return err;

	/* period must be at least 50ms.
	   This means more bytes for more channels
	    ?fixes asi5044 multichannel playback xruns
	*/
	/*
	snd_pcm_hw_constraint_minmax(runtime,
		SNDRV_PCM_HW_PARAM_PERIOD_TIME,
		44000, UINT_MAX);
	*/

	snd_pcm_set_sync(substream);

	snd_printd(KERN_INFO "Playback open\n");

	return 0;
}

static int snd_card_asihpi_playback_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_card_asihpi_pcm *dpcm = runtime->private_data;

	HPI_HandleError(HPI_OutStreamClose(phSubSys, dpcm->hStream));
	snd_printd(KERN_INFO "Playback close\n");

	return 0;
}

static int snd_card_asihpi_playback_copy(struct snd_pcm_substream *substream,
					int channel,
					snd_pcm_uframes_t pos,
					void __user *src,
					snd_pcm_uframes_t count)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_card_asihpi_pcm *dpcm = runtime->private_data;
	unsigned int len;

	len = frames_to_bytes(runtime, count);

	if (copy_from_user(runtime->dma_area, src, len))
		return -EFAULT;

	VPRINTK2(KERN_DEBUG "Playback copy%d %u bytes\n",
			substream->number, len);

	HPI_HandleError(HPI_OutStreamWriteBuf(phSubSys, dpcm->hStream,
				runtime->dma_area, len, &dpcm->Format));

	return 0;
}

static int snd_card_asihpi_playback_silence(struct snd_pcm_substream *
					    substream, int channel,
					    snd_pcm_uframes_t pos,
					    snd_pcm_uframes_t count)
{
	unsigned int len;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_card_asihpi_pcm *dpcm = runtime->private_data;

	len = frames_to_bytes(runtime, count);
	snd_printd(KERN_INFO "Playback silence  %u bytes\n", len);

	memset(runtime->dma_area, 0, len);
	HPI_HandleError(HPI_OutStreamWriteBuf(phSubSys, dpcm->hStream,
				runtime->dma_area, len, &dpcm->Format));
	return 0;
}

static struct snd_pcm_ops snd_card_asihpi_playback_ops = {
	.open = snd_card_asihpi_playback_open,
	.close = snd_card_asihpi_playback_close,
	.ioctl = snd_card_asihpi_playback_ioctl,
	.hw_params = snd_card_asihpi_pcm_hw_params,
	.hw_free = snd_card_asihpi_hw_free,
	.prepare = snd_card_asihpi_playback_prepare,
	.trigger = snd_card_asihpi_trigger,
	.pointer = snd_card_asihpi_playback_pointer,
	.copy = snd_card_asihpi_playback_copy,
	.silence = snd_card_asihpi_playback_silence,
};

static struct snd_pcm_ops snd_card_asihpi_playback_mmap_ops = {
	.open = snd_card_asihpi_playback_open,
	.close = snd_card_asihpi_playback_close,
	.ioctl = snd_card_asihpi_playback_ioctl,
	.hw_params = snd_card_asihpi_pcm_hw_params,
	.hw_free = snd_card_asihpi_hw_free,
	.prepare = snd_card_asihpi_playback_prepare,
	.trigger = snd_card_asihpi_trigger,
	.pointer = snd_card_asihpi_playback_pointer,
};

/***************************** CAPTURE OPS ****************/
static snd_pcm_uframes_t
snd_card_asihpi_capture_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_card_asihpi_pcm *dpcm = runtime->private_data;

	VPRINTK3("Capture pointer%d x%04x\n",
			substream->number, dpcm->pcm_buf_pos);
	/* NOTE Unlike playback can't use actual dwSamplesPlayed
		for the capture position, because those samples aren't yet in
		the local buffer available for reading.
	*/
	return bytes_to_frames(runtime, dpcm->pcm_buf_pos % dpcm->pcm_size);
}

static int snd_card_asihpi_capture_ioctl(struct snd_pcm_substream *substream,
					 unsigned int cmd, void *arg)
{
	return snd_pcm_lib_ioctl(substream, cmd, arg);
}

static int snd_card_asihpi_capture_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_card_asihpi_pcm *dpcm = runtime->private_data;

	HPI_HandleError(HPI_InStreamReset(phSubSys, dpcm->hStream));
	dpcm->pcm_irq_pos = 0;
	dpcm->pcm_buf_pos = 0;

	snd_printd("Capture Prepare %d\n", substream->number);
	return 0;
}



static void snd_card_asihpi_capture_format(struct snd_card_asihpi *asihpi,
					u32 hStream,
					 struct snd_pcm_hardware *pcmhw)
{
  struct hpi_format hpi_format;
	u16 wFormat;
	u16 err;
	u32 hControl;
	u32 dwSampleRate = 48000;

	/* on cards without SRC, must query at valid rate,
		maybe set by external sync */
	err = HPI_MixerGetControl(phSubSys, asihpi->hMixer,
				  HPI_SOURCENODE_CLOCK_SOURCE, 0, 0, 0,
				  HPI_CONTROL_SAMPLECLOCK, &hControl);

	if (!err)
		err = HPI_SampleClock_GetSampleRate(phSubSys, hControl,
			&dwSampleRate);

	for (wFormat = HPI_FORMAT_PCM8_UNSIGNED;
		wFormat <= HPI_FORMAT_PCM24_SIGNED; wFormat++) {

		err = HPI_FormatCreate(&hpi_format, 2, wFormat,
				dwSampleRate, 128000, 0);
		if (!err)
			err = HPI_InStreamQueryFormat(phSubSys, hStream,
					    &hpi_format);
		if (!err)
			pcmhw->formats |=
				(1ULL << hpi_to_alsa_formats[wFormat]);
	}
}


static struct snd_pcm_hardware snd_card_asihpi_capture = {
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = BUFFER_BYTES_MAX,
	.period_bytes_min = PERIOD_BYTES_MIN,
	.period_bytes_max = BUFFER_BYTES_MAX / PERIODS_MIN,
	.periods_min = PERIODS_MIN,
	.periods_max = BUFFER_BYTES_MAX / PERIOD_BYTES_MIN,
	.fifo_size = 0,
};

static int snd_card_asihpi_capture_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_card_asihpi *card = snd_pcm_substream_chip(substream);
	struct snd_card_asihpi_pcm *dpcm;
	int err;

	dpcm = kzalloc(sizeof(*dpcm), GFP_KERNEL);
	if (dpcm == NULL)
		return -ENOMEM;

	snd_printd("HPI_InStreamOpen Adapter %d Stream %d\n",
		   card->wAdapterIndex, substream->number);

	err = HPI_HandleError(
	    HPI_InStreamOpen(phSubSys, card->wAdapterIndex,
			     substream->number, &dpcm->hStream));
	if (err)
		kfree(dpcm);
	if (err == HPI_ERROR_OBJ_ALREADY_OPEN)
		return -EBUSY;
	if (err)
		return -EIO;


	init_timer(&dpcm->timer);
	dpcm->timer.data = (unsigned long) dpcm;
	dpcm->timer.function = snd_card_asihpi_timer_function;
	dpcm->substream = substream;
	runtime->private_data = dpcm;
	runtime->private_free = snd_card_asihpi_runtime_free;

	snd_card_asihpi_capture.channels_max = card->in_max_chans;
	snd_card_asihpi_capture_format(card, dpcm->hStream,
				       &snd_card_asihpi_capture);
	snd_card_asihpi_pcm_samplerates(card,  &snd_card_asihpi_capture);
	snd_card_asihpi_capture.info = SNDRV_PCM_INFO_INTERLEAVED;

	if (card->support_mmap)
		snd_card_asihpi_capture.info |= SNDRV_PCM_INFO_MMAP |
						SNDRV_PCM_INFO_MMAP_VALID;

	runtime->hw = snd_card_asihpi_capture;

	if (card->support_mmap)
		err = snd_pcm_hw_constraint_pow2(runtime, 0,
					SNDRV_PCM_HW_PARAM_BUFFER_BYTES);
	if (err < 0)
		return err;

	snd_pcm_set_sync(substream);

	return 0;
}

static int snd_card_asihpi_capture_close(struct snd_pcm_substream *substream)
{
	struct snd_card_asihpi_pcm *dpcm = substream->runtime->private_data;

	HPI_HandleError(HPI_InStreamClose(phSubSys, dpcm->hStream));
	return 0;
}

static int snd_card_asihpi_capture_copy(struct snd_pcm_substream *substream,
				int channel, snd_pcm_uframes_t pos,
				void __user *dst, snd_pcm_uframes_t count)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_card_asihpi_pcm *dpcm = runtime->private_data;
	u32 dwDataSize;

	dwDataSize = frames_to_bytes(runtime, count);

	VPRINTK2("Capture copy%d %d bytes\n", substream->number, dwDataSize);
	HPI_HandleError(HPI_InStreamReadBuf(phSubSys, dpcm->hStream,
				runtime->dma_area, dwDataSize));

	/* Used by capture_pointer */
	dpcm->pcm_irq_pos = dpcm->pcm_irq_pos + dwDataSize;

	if (copy_to_user(dst, runtime->dma_area, dwDataSize))
		return -EFAULT;

	return 0;
}

static struct snd_pcm_ops snd_card_asihpi_capture_mmap_ops = {
	.open = snd_card_asihpi_capture_open,
	.close = snd_card_asihpi_capture_close,
	.ioctl = snd_card_asihpi_capture_ioctl,
	.hw_params = snd_card_asihpi_pcm_hw_params,
	.hw_free = snd_card_asihpi_hw_free,
	.prepare = snd_card_asihpi_capture_prepare,
	.trigger = snd_card_asihpi_trigger,
	.pointer = snd_card_asihpi_capture_pointer,
};

static struct snd_pcm_ops snd_card_asihpi_capture_ops = {
	.open = snd_card_asihpi_capture_open,
	.close = snd_card_asihpi_capture_close,
	.ioctl = snd_card_asihpi_capture_ioctl,
	.hw_params = snd_card_asihpi_pcm_hw_params,
	.hw_free = snd_card_asihpi_hw_free,
	.prepare = snd_card_asihpi_capture_prepare,
	.trigger = snd_card_asihpi_trigger,
	.pointer = snd_card_asihpi_capture_pointer,
	.copy = snd_card_asihpi_capture_copy
};

static int __devinit snd_card_asihpi_pcm_new(struct snd_card_asihpi *asihpi,
				      int device, int substreams)
{
	struct snd_pcm *pcm;
	int err;

	err = snd_pcm_new(asihpi->card, "Asihpi PCM", device,
			 asihpi->wNumOutStreams, asihpi->wNumInStreams,
			 &pcm);
	if (err < 0)
		return err;
	/* pointer to ops struct is stored, dont change ops afterwards! */
	if (asihpi->support_mmap) {
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK,
				&snd_card_asihpi_playback_mmap_ops);
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE,
				&snd_card_asihpi_capture_mmap_ops);
	} else {
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK,
				&snd_card_asihpi_playback_ops);
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE,
				&snd_card_asihpi_capture_ops);
	}

	pcm->private_data = asihpi;
	pcm->info_flags = 0;
	strcpy(pcm->name, "Asihpi PCM");

/* ? do we want to emulate MMAP for non-BBM cards? Jack doesn't work with ALSAs
   MMAP emulation - WHY NOT? */
	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
						snd_dma_pci_data(asihpi->pci),
						64*1024, BUFFER_BYTES_MAX);

	return 0;
}

/***************************** MIXER CONTROLS ****************/
struct hpi_control {
	u32 hControl;
	u16 wControlType;
	u16 wSrcNodeType;
	u16 wSrcNodeIndex;
	u16 wDstNodeType;
	u16 wDstNodeIndex;
	u16 wBand;
	char name[44]; /* copied to snd_ctl_elem_id.name[44]; */
};

static char *asihpi_tuner_band_names[] =
{
	"invalid",
	"AM",
	"FM mono",
	"TV NTSC-M",
	"FM stereo",
	"AUX",
	"TV PAL BG",
	"TV PAL I",
	"TV PAL DK",
	"TV SECAM",
};

compile_time_assert(
	(ARRAY_SIZE(asihpi_tuner_band_names) ==
		(HPI_TUNER_BAND_LAST+1)),
	assert_tuner_band_names_size);

#if ASI_STYLE_NAMES
static char *asihpi_src_names[] =
{
	"no source",
	"OutStream",
	"LineIn",
	"AesIn",
	"Tuner",
	"RF",
	"Clock",
	"Bitstr",
	"Mic",
	"Cobranet",
	"AnalogIn",
	"Adapter",
};
#else
static char *asihpi_src_names[] =
{
	"no source",
	"PCM Playback",
	"Line in",
	"Digital in",
	"Tuner",
	"RF",
	"Clock",
	"Bitstream",
	"Mic",
	"Cobranet in",
	"Analog in",
	"Adapter",
};
#endif

compile_time_assert(
	(ARRAY_SIZE(asihpi_src_names) ==
		(HPI_SOURCENODE_LAST_INDEX-HPI_SOURCENODE_BASE+1)),
	assert_src_names_size);

#if ASI_STYLE_NAMES
static char *asihpi_dst_names[] =
{
	"no destination",
	"InStream",
	"LineOut",
	"AesOut",
	"RF",
	"Speaker" ,
	"Cobranet",
	"AnalogOut",
};
#else
static char *asihpi_dst_names[] =
{
	"no destination",
	"PCM Capture",
	"Line out",
	"Digital out",
	"RF",
	"Speaker",
	"Cobranet out",
	"Analog out"
};
#endif

compile_time_assert(
	(ARRAY_SIZE(asihpi_dst_names) ==
		(HPI_DESTNODE_LAST_INDEX-HPI_DESTNODE_BASE+1)),
	assert_dst_names_size);

static inline int ctl_add(struct snd_card *card, struct snd_kcontrol_new *ctl,
				struct snd_card_asihpi *asihpi)
{
	int err;

	err = snd_ctl_add(card, snd_ctl_new1(ctl, asihpi));
	if (err < 0)
		return err;
	else if (mixer_dump)
		snd_printk(KERN_INFO "Added %s(%d)\n", ctl->name, ctl->index);

	return 0;
}

/* Convert HPI control name and location into ALSA control name */
static void asihpi_ctl_init(struct snd_kcontrol_new *snd_control,
				struct hpi_control *asihpi_control,
				char *name)
{
	memset(snd_control, 0, sizeof(*snd_control));
	snd_control->name = asihpi_control->name;
	snd_control->private_value = asihpi_control->hControl;
	snd_control->iface = SNDRV_CTL_ELEM_IFACE_MIXER;
	snd_control->index = 0;

	if (asihpi_control->wSrcNodeType && asihpi_control->wDstNodeType)
		sprintf(asihpi_control->name, "%s%d to %s%d %s",
			asihpi_src_names[asihpi_control->wSrcNodeType],
			asihpi_control->wSrcNodeIndex,
			asihpi_dst_names[asihpi_control->wDstNodeType],
			asihpi_control->wDstNodeIndex,
			name);
	else if (asihpi_control->wDstNodeType) {
		sprintf(asihpi_control->name, "%s%d %s",
		asihpi_dst_names[asihpi_control->wDstNodeType],
		asihpi_control->wDstNodeIndex,
		name);
	} else {
		sprintf(asihpi_control->name, "%s%d %s",
		asihpi_src_names[asihpi_control->wSrcNodeType],
		asihpi_control->wSrcNodeIndex,
		name);
	}
}

/*------------------------------------------------------------
   Volume controls
 ------------------------------------------------------------*/
#define VOL_STEP_mB 1
static int snd_asihpi_volume_info(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_info *uinfo)
{
	u32 hControl = kcontrol->private_value;
	u16 err;
	/* native gains are in millibels */
	short nMinGain_mB;
	short nMaxGain_mB;
	short nStepGain_mB;

	err = HPI_VolumeQueryRange(phSubSys, hControl,
			&nMinGain_mB, &nMaxGain_mB, &nStepGain_mB);
	if (err) {
		nMaxGain_mB = 0;
		nMinGain_mB = -10000;
		nStepGain_mB = VOL_STEP_mB;
	}

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = nMinGain_mB / VOL_STEP_mB;
	uinfo->value.integer.max = nMaxGain_mB / VOL_STEP_mB;
	uinfo->value.integer.step = nStepGain_mB / VOL_STEP_mB;
	return 0;
}

static int snd_asihpi_volume_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	u32 hControl = kcontrol->private_value;
	short anGain_mB[HPI_MAX_CHANNELS];

	HPI_HandleError(HPI_VolumeGetGain(phSubSys, hControl, anGain_mB));
	ucontrol->value.integer.value[0] = anGain_mB[0] / VOL_STEP_mB;
	ucontrol->value.integer.value[1] = anGain_mB[1] / VOL_STEP_mB;

	return 0;
}

static int snd_asihpi_volume_put(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	int change;
	u32 hControl = kcontrol->private_value;
	short anGain_mB[HPI_MAX_CHANNELS];

	anGain_mB[0] =
	    (ucontrol->value.integer.value[0]) * VOL_STEP_mB;
	anGain_mB[1] =
	    (ucontrol->value.integer.value[1]) * VOL_STEP_mB;
	/*  change = asihpi->mixer_volume[addr][0] != left ||
	   asihpi->mixer_volume[addr][1] != right;
	 */
	change = 1;
	HPI_HandleError(HPI_VolumeSetGain(phSubSys, hControl, anGain_mB));
	return change;
}

static const DECLARE_TLV_DB_SCALE(db_scale_100, -10000, VOL_STEP_mB, 0);

static int __devinit snd_asihpi_volume_add(struct snd_card_asihpi *asihpi,
					struct hpi_control *asihpi_control)
{
	struct snd_card *card = asihpi->card;
	struct snd_kcontrol_new snd_control;

	asihpi_ctl_init(&snd_control, asihpi_control, "Volume");
	snd_control.access = SNDRV_CTL_ELEM_ACCESS_READWRITE |
				SNDRV_CTL_ELEM_ACCESS_TLV_READ;
	snd_control.info = snd_asihpi_volume_info;
	snd_control.get = snd_asihpi_volume_get;
	snd_control.put = snd_asihpi_volume_put;
	snd_control.tlv.p = db_scale_100;

	return (ctl_add(card, &snd_control, asihpi));
}

/*------------------------------------------------------------
   Level controls
 ------------------------------------------------------------*/
static int snd_asihpi_level_info(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_info *uinfo)
{
	u32 hControl = kcontrol->private_value;
	u16 err;
	short nMinGain_mB;
	short nMaxGain_mB;
	short nStepGain_mB;

	err =
	    HPI_LevelQueryRange(phSubSys, hControl, &nMinGain_mB,
			       &nMaxGain_mB, &nStepGain_mB);
	if (err) {
		nMaxGain_mB = 2400;
		nMinGain_mB = -1000;
		nStepGain_mB = 100;
	}

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = nMinGain_mB / HPI_UNITS_PER_dB;
	uinfo->value.integer.max = nMaxGain_mB / HPI_UNITS_PER_dB;
	uinfo->value.integer.step = nStepGain_mB / HPI_UNITS_PER_dB;
	return 0;
}

static int snd_asihpi_level_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	u32 hControl = kcontrol->private_value;
	short anGain_mB[HPI_MAX_CHANNELS];

	HPI_HandleError(HPI_LevelGetGain(phSubSys, hControl, anGain_mB));
	ucontrol->value.integer.value[0] =
	    anGain_mB[0] / HPI_UNITS_PER_dB;
	ucontrol->value.integer.value[1] =
	    anGain_mB[1] / HPI_UNITS_PER_dB;

	return 0;
}

static int snd_asihpi_level_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int change;
	u32 hControl = kcontrol->private_value;
	short anGain_mB[HPI_MAX_CHANNELS];

	anGain_mB[0] =
	    (ucontrol->value.integer.value[0]) * HPI_UNITS_PER_dB;
	anGain_mB[1] =
	    (ucontrol->value.integer.value[1]) * HPI_UNITS_PER_dB;
	/*  change = asihpi->mixer_level[addr][0] != left ||
	   asihpi->mixer_level[addr][1] != right;
	 */
	change = 1;
	HPI_HandleError(HPI_LevelSetGain(phSubSys, hControl, anGain_mB));
	return change;
}

static const DECLARE_TLV_DB_SCALE(db_scale_level, -1000, 100, 0);

static int __devinit snd_asihpi_level_add(struct snd_card_asihpi *asihpi,
					struct hpi_control *asihpi_control)
{
	struct snd_card *card = asihpi->card;
	struct snd_kcontrol_new snd_control;

	/* can't use 'volume' cos some nodes have volume as well */
	asihpi_ctl_init(&snd_control, asihpi_control, "Level");
	snd_control.access = SNDRV_CTL_ELEM_ACCESS_READWRITE |
				SNDRV_CTL_ELEM_ACCESS_TLV_READ;
	snd_control.info = snd_asihpi_level_info;
	snd_control.get = snd_asihpi_level_get;
	snd_control.put = snd_asihpi_level_put;
	snd_control.tlv.p = db_scale_level;

	return (ctl_add(card, &snd_control, asihpi));
}

/*------------------------------------------------------------
   AESEBU controls
 ------------------------------------------------------------*/

/* AESEBU format */
static char *asihpi_aesebu_format_names[] =
{
	"N/A",
	"S/PDIF",
	"AES/EBU",
};

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
			u16 (*func)(struct hpi_hsubsys *, u32, u16 *))
{
	u32 hControl = kcontrol->private_value;
	u16 source, err;

	err = func(phSubSys, hControl, &source);

	/* default to N/A */
	ucontrol->value.enumerated.item[0] = 0;
	/* return success but set the control to N/A */
	if (err)
		return 0;
	if (source == HPI_AESEBU_FORMAT_SPDIF)
		ucontrol->value.enumerated.item[0] = 1;
	if (source == HPI_AESEBU_FORMAT_AESEBU)
		ucontrol->value.enumerated.item[0] = 2;

	return 0;
}

static int snd_asihpi_aesebu_format_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol,
			 u16 (*func)(struct hpi_hsubsys *, u32, u16))
{
	u32 hControl = kcontrol->private_value;

	/* default to S/PDIF */
	u16 source = HPI_AESEBU_FORMAT_SPDIF;

	if (ucontrol->value.enumerated.item[0] == 1)
		source = HPI_AESEBU_FORMAT_SPDIF;
	if (ucontrol->value.enumerated.item[0] == 2)
		source = HPI_AESEBU_FORMAT_AESEBU;

	if (func(phSubSys, hControl, source) != 0)
		return -EINVAL;

	return 1;
}

static int snd_asihpi_aesebu_rx_format_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol) {
	return snd_asihpi_aesebu_format_get(kcontrol, ucontrol,
					HPI_AESEBU_Receiver_GetFormat);
}

static int snd_asihpi_aesebu_rx_format_put(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol) {
	return snd_asihpi_aesebu_format_put(kcontrol, ucontrol,
					HPI_AESEBU_Receiver_SetFormat);
}

static int snd_asihpi_aesebu_rxstatus_info(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;

	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 0X1F;
	uinfo->value.integer.step = 1;

	return 0;
}

static int snd_asihpi_aesebu_rxstatus_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol) {

	u32 hControl = kcontrol->private_value;
	u16 status;

	HPI_HandleError(HPI_AESEBU_Receiver_GetErrorStatus(phSubSys, hControl, &status));
	ucontrol->value.integer.value[0] = status;
	return 0;
}

static int __devinit snd_asihpi_aesebu_rx_add(struct snd_card_asihpi *asihpi,
					struct hpi_control *asihpi_control)
{
	struct snd_card *card = asihpi->card;
	struct snd_kcontrol_new snd_control;

	asihpi_ctl_init(&snd_control, asihpi_control, "Format");
	snd_control.access = SNDRV_CTL_ELEM_ACCESS_READWRITE;
	snd_control.info = snd_asihpi_aesebu_format_info;
	snd_control.get = snd_asihpi_aesebu_rx_format_get;
	snd_control.put = snd_asihpi_aesebu_rx_format_put;


	if (ctl_add(card, &snd_control, asihpi) < 0)
		return -EINVAL;

	asihpi_ctl_init(&snd_control, asihpi_control, "Status");
	snd_control.access =
	    SNDRV_CTL_ELEM_ACCESS_VOLATILE | SNDRV_CTL_ELEM_ACCESS_READ;
	snd_control.info = snd_asihpi_aesebu_rxstatus_info;
	snd_control.get = snd_asihpi_aesebu_rxstatus_get;

	return ctl_add(card, &snd_control, asihpi);
}

static int snd_asihpi_aesebu_tx_format_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol) {
	return snd_asihpi_aesebu_format_get(kcontrol, ucontrol,
					HPI_AESEBU_Transmitter_GetFormat);
}

static int snd_asihpi_aesebu_tx_format_put(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol) {
	return snd_asihpi_aesebu_format_put(kcontrol, ucontrol,
					HPI_AESEBU_Transmitter_SetFormat);
}


static int __devinit snd_asihpi_aesebu_tx_add(struct snd_card_asihpi *asihpi,
					struct hpi_control *asihpi_control)
{
	struct snd_card *card = asihpi->card;
	struct snd_kcontrol_new snd_control;

	asihpi_ctl_init(&snd_control, asihpi_control, "Format");
	snd_control.access = SNDRV_CTL_ELEM_ACCESS_READWRITE;
	snd_control.info = snd_asihpi_aesebu_format_info;
	snd_control.get = snd_asihpi_aesebu_tx_format_get;
	snd_control.put = snd_asihpi_aesebu_tx_format_put;

	return ctl_add(card, &snd_control, asihpi);
}

/*------------------------------------------------------------
   Tuner controls
 ------------------------------------------------------------*/

/* Gain */

static int snd_asihpi_tuner_gain_info(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_info *uinfo)
{
	u32 hControl = kcontrol->private_value;
	u16 err;
	short idx;
	u16 gainRange[3];

	for (idx = 0; idx < 3; idx++) {
	err = HPI_Tuner_QueryGain(phSubSys, hControl, idx, &gainRange[idx]);
		if (err != 0)
			return err;
	}

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = ((int)gainRange[0]) / HPI_UNITS_PER_dB;
	uinfo->value.integer.max = ((int)gainRange[1]) / HPI_UNITS_PER_dB;
	uinfo->value.integer.step = ((int) gainRange[2]) / HPI_UNITS_PER_dB;
	return 0;
}

static int snd_asihpi_tuner_gain_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	/*
	struct snd_card_asihpi *asihpi = snd_kcontrol_chip(kcontrol);
	*/
	u32 hControl = kcontrol->private_value;
	short gain;

	HPI_HandleError(HPI_Tuner_GetGain(phSubSys, hControl, &gain));
	ucontrol->value.integer.value[0] = gain / HPI_UNITS_PER_dB;

	return 0;
}

static int snd_asihpi_tuner_gain_put(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	/*
	struct snd_card_asihpi *asihpi = snd_kcontrol_chip(kcontrol);
	*/
	u32 hControl = kcontrol->private_value;
	short gain;

	gain = (ucontrol->value.integer.value[0]) * HPI_UNITS_PER_dB;
	HPI_HandleError(HPI_Tuner_SetGain(phSubSys, hControl, gain));

	return 1;
}

/* Band  */

static int asihpi_tuner_band_query(struct snd_kcontrol *kcontrol,
					u16 *bandList, u32 len) {
	u32 hControl = kcontrol->private_value;
	u16 err = 0;
	u32 i;

	for (i = 0; i < len; i++) {
		err = HPI_Tuner_QueryBand(phSubSys, hControl, i, &bandList[i]);
		if (err != 0)
			break;
	}

	if (err && (err != HPI_ERROR_INVALID_OBJ_INDEX))
		return -EIO;

	return i;
}

static int snd_asihpi_tuner_band_info(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_info *uinfo)
{
	u16 tunerBands[HPI_TUNER_BAND_LAST];
	int numBands = 0;

	numBands = asihpi_tuner_band_query(kcontrol, tunerBands,
				HPI_TUNER_BAND_LAST);

	if (numBands < 0)
		return numBands;

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = numBands;

	if (numBands > 0) {
		if (uinfo->value.enumerated.item >=
					uinfo->value.enumerated.items)
			uinfo->value.enumerated.item =
				uinfo->value.enumerated.items - 1;

		strcpy(uinfo->value.enumerated.name,
			asihpi_tuner_band_names[
				tunerBands[uinfo->value.enumerated.item]]);

	}
	return 0;
}

static int snd_asihpi_tuner_band_get(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	u32 hControl = kcontrol->private_value;
	/*
	struct snd_card_asihpi *asihpi = snd_kcontrol_chip(kcontrol);
	*/
	u16 band, idx;
	u16 tunerBands[HPI_TUNER_BAND_LAST];
	u32 numBands = 0;

	numBands = asihpi_tuner_band_query(kcontrol, tunerBands,
				HPI_TUNER_BAND_LAST);

	HPI_HandleError(HPI_Tuner_GetBand(phSubSys, hControl, &band));

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
	struct snd_card_asihpi *asihpi = snd_kcontrol_chip(kcontrol);
	*/
	u32 hControl = kcontrol->private_value;
	u16 band;
	u16 tunerBands[HPI_TUNER_BAND_LAST];
	u32 numBands = 0;

	numBands = asihpi_tuner_band_query(kcontrol, tunerBands,
			HPI_TUNER_BAND_LAST);

	band = tunerBands[ucontrol->value.enumerated.item[0]];
	HPI_HandleError(HPI_Tuner_SetBand(phSubSys, hControl, band));

	return 1;
}

/* Freq */

static int snd_asihpi_tuner_freq_info(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_info *uinfo)
{
	u32 hControl = kcontrol->private_value;
	u16 err;
	u16 tunerBands[HPI_TUNER_BAND_LAST];
	u16 numBands = 0, band_iter, idx;
	u32 freqRange[3], tempFreqRange[3];

	numBands = asihpi_tuner_band_query(kcontrol, tunerBands,
			HPI_TUNER_BAND_LAST);

	freqRange[0] = INT_MAX;
	freqRange[1] = 0;
	freqRange[2] = INT_MAX;

	for (band_iter = 0; band_iter < numBands; band_iter++) {
		for (idx = 0; idx < 3; idx++) {
			err = HPI_Tuner_QueryFrequency(phSubSys, hControl,
				idx, tunerBands[band_iter],
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
	u32 hControl = kcontrol->private_value;
	u32 freq;

	HPI_HandleError(HPI_Tuner_GetFrequency(phSubSys, hControl, &freq));
	ucontrol->value.integer.value[0] = freq;

	return 0;
}

static int snd_asihpi_tuner_freq_put(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	u32 hControl = kcontrol->private_value;
	u32 freq;

	freq = ucontrol->value.integer.value[0];
	HPI_HandleError(HPI_Tuner_SetFrequency(phSubSys, hControl, freq));

	return 1;
}

/* Tuner control group initializer  */
static int __devinit snd_asihpi_tuner_add(struct snd_card_asihpi *asihpi,
					struct hpi_control *asihpi_control)
{
	struct snd_card *card = asihpi->card;
	struct snd_kcontrol_new snd_control;

	snd_control.private_value = asihpi_control->hControl;
	snd_control.access = SNDRV_CTL_ELEM_ACCESS_READWRITE;

	if (HPI_Tuner_GetGain(phSubSys, asihpi_control->hControl, NULL) == 0) {
		asihpi_ctl_init(&snd_control, asihpi_control, "Gain");
		snd_control.info = snd_asihpi_tuner_gain_info;
		snd_control.get = snd_asihpi_tuner_gain_get;
		snd_control.put = snd_asihpi_tuner_gain_put;

		if (ctl_add(card, &snd_control, asihpi) < 0)
			return -EINVAL;
	}

	asihpi_ctl_init(&snd_control, asihpi_control, "Band");
	snd_control.info = snd_asihpi_tuner_band_info;
	snd_control.get = snd_asihpi_tuner_band_get;
	snd_control.put = snd_asihpi_tuner_band_put;

	if (ctl_add(card, &snd_control, asihpi) < 0)
		return -EINVAL;

	asihpi_ctl_init(&snd_control, asihpi_control, "Freq");
	snd_control.info = snd_asihpi_tuner_freq_info;
	snd_control.get = snd_asihpi_tuner_freq_get;
	snd_control.put = snd_asihpi_tuner_freq_put;

	return (ctl_add(card, &snd_control, asihpi));
}

/*------------------------------------------------------------
   Meter controls
 ------------------------------------------------------------*/
static int snd_asihpi_meter_info(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = HPI_MAX_CHANNELS;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 0x7FFFFFFF;
	return 0;
}

/* linear values for 10dB steps */
static int log2lin[] = {
	0x7FFFFFFF, /* 0dB */
	679093956,
	214748365,
	 67909396,
	 21474837,
	  6790940,
	  2147484, /* -60dB */
	   679094,
	   214748, /* -80 */
	    67909,
	    21475, /* -100 */
	     6791,
	     2147,
	      679,
	      214,
	       68,
	       21,
		7,
		2
};

static int snd_asihpi_meter_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	u32 hControl = kcontrol->private_value;
	short anGain_mB[HPI_MAX_CHANNELS], i;
	u16 err;

	err = HPI_MeterGetPeak(phSubSys, hControl, anGain_mB);

	for (i = 0; i < HPI_MAX_CHANNELS; i++) {
		if (err) {
			ucontrol->value.integer.value[i] = 0;
		} else if (anGain_mB[i] >= 0) {
			ucontrol->value.integer.value[i] =
				anGain_mB[i] << 16;
		} else {
			/* -ve is log value in millibels < -60dB,
			* convert to (roughly!) linear,
			*/
			ucontrol->value.integer.value[i] =
					log2lin[anGain_mB[i] / -1000];
		}
	}
	return 0;
}

static int __devinit snd_asihpi_meter_add(struct snd_card_asihpi *asihpi,
					struct hpi_control *asihpi_control, int subidx)
{
	struct snd_card *card = asihpi->card;
	struct snd_kcontrol_new snd_control;

	asihpi_ctl_init(&snd_control, asihpi_control, "Meter");
	snd_control.access =
	    SNDRV_CTL_ELEM_ACCESS_VOLATILE | SNDRV_CTL_ELEM_ACCESS_READ;
	snd_control.info = snd_asihpi_meter_info;
	snd_control.get = snd_asihpi_meter_get;

	snd_control.index = subidx;

	return ctl_add(card, &snd_control, asihpi);
}

/*------------------------------------------------------------
   Multiplexer controls
 ------------------------------------------------------------*/
static int snd_card_asihpi_mux_count_sources(struct snd_kcontrol *snd_control)
{
	u32 hControl = snd_control->private_value;
	struct hpi_control asihpi_control;
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
	u32 hControl = kcontrol->private_value;

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
		wSrcNodeIndex);
	return 0;
}

static int snd_asihpi_mux_get(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	u32 hControl = kcontrol->private_value;
	u16 wSourceType, wSourceIndex;
	u16 wSrcNodeType, wSrcNodeIndex;
	int s;

	HPI_HandleError(HPI_Multiplexer_GetSource(phSubSys, hControl,
				&wSourceType, &wSourceIndex));
	/* Should cache this search result! */
	for (s = 0; s < 256; s++) {
		if (HPI_Multiplexer_QuerySource(phSubSys, hControl, s,
					    &wSrcNodeType, &wSrcNodeIndex))
			break;

		if ((wSourceType == wSrcNodeType)
		    && (wSourceIndex == wSrcNodeIndex)) {
			ucontrol->value.enumerated.item[0] = s;
			return 0;
		}
	}
	snd_printd(KERN_WARNING "Control %x failed to match mux source %hu %hu\n",
			hControl, wSourceType, wSourceIndex);
	ucontrol->value.enumerated.item[0] = 0;
	return 0;
}

static int snd_asihpi_mux_put(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	int change;
	u32 hControl = kcontrol->private_value;
	u16 wSourceType, wSourceIndex;
	u16 e;

	change = 1;

	e = HPI_Multiplexer_QuerySource(phSubSys, hControl,
				    ucontrol->value.enumerated.item[0],
				    &wSourceType, &wSourceIndex);
	if (!e)
		HPI_HandleError(HPI_Multiplexer_SetSource(phSubSys, hControl,
						wSourceType, wSourceIndex));
	return change;
}


static int  __devinit snd_asihpi_mux_add(struct snd_card_asihpi *asihpi,
					struct hpi_control *asihpi_control)
{
	struct snd_card *card = asihpi->card;
	struct snd_kcontrol_new snd_control;

#if ASI_STYLE_NAMES
	asihpi_ctl_init(&snd_control, asihpi_control, "Multiplexer");
#else
	asihpi_ctl_init(&snd_control, asihpi_control, "Route");
#endif
	snd_control.access = SNDRV_CTL_ELEM_ACCESS_READWRITE;
	snd_control.info = snd_asihpi_mux_info;
	snd_control.get = snd_asihpi_mux_get;
	snd_control.put = snd_asihpi_mux_put;

	return (ctl_add(card, &snd_control, asihpi));

}

/*------------------------------------------------------------
   Channel mode controls
 ------------------------------------------------------------*/
static int snd_asihpi_cmode_info(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_info *uinfo)
{
	static char *mode_names[HPI_CHANNEL_MODE_LAST] = {
		"Normal", "Swap", "FromLeft", "FromRight", "ToLeft", "ToRight"
	};

	u32 hControl = kcontrol->private_value;
	u16 wMode;
	int i;

	/* HPI channel mode values can be from 1 to 6
	Some adapters only support a contiguous subset
	*/
	for (i = 0; i < HPI_CHANNEL_MODE_LAST; i++)
		if (HPI_ChannelMode_QueryMode(phSubSys,  hControl, i, &wMode))
			break;

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = i;

	if (uinfo->value.enumerated.item >= i)
		uinfo->value.enumerated.item = i - 1;

	strcpy(uinfo->value.enumerated.name,
	       mode_names[uinfo->value.enumerated.item]);

	return 0;
}

static int snd_asihpi_cmode_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	u32 hControl = kcontrol->private_value;
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
	u32 hControl = kcontrol->private_value;

	change = 1;

	HPI_HandleError(HPI_ChannelModeSet(phSubSys, hControl,
			   ucontrol->value.enumerated.item[0] + 1));
	return change;
}


static int __devinit snd_asihpi_cmode_add(struct snd_card_asihpi *asihpi,
					struct hpi_control *asihpi_control)
{
	struct snd_card *card = asihpi->card;
	struct snd_kcontrol_new snd_control;

	asihpi_ctl_init(&snd_control, asihpi_control, "Channel Mode");
	snd_control.access = SNDRV_CTL_ELEM_ACCESS_READWRITE;
	snd_control.info = snd_asihpi_cmode_info;
	snd_control.get = snd_asihpi_cmode_get;
	snd_control.put = snd_asihpi_cmode_put;

	return (ctl_add(card, &snd_control, asihpi));
}

/*------------------------------------------------------------
   Sampleclock source  controls
 ------------------------------------------------------------*/

static char *sampleclock_sources[MAX_CLOCKSOURCES] =
    { "N/A", "Local PLL", "AES/EBU Sync", "Word External", "Word Header",
	  "SMPTE", "AES/EBU In1", "Auto", "Network", "Invalid",
	  "Prev Module",
	  "AES/EBU In2", "AES/EBU In3", "AES/EBU In4", "AES/EBU In5",
	  "AES/EBU In6", "AES/EBU In7", "AES/EBU In8"};



static int snd_asihpi_clksrc_info(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_info *uinfo)
{
	struct snd_card_asihpi *asihpi =
			(struct snd_card_asihpi *)(kcontrol->private_data);
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
	struct snd_card_asihpi *asihpi =
			(struct snd_card_asihpi *)(kcontrol->private_data);
	struct clk_cache *clkcache = &asihpi->cc;
	u32 hControl = kcontrol->private_value;
	u16 wSource, wIndex = 0;
	int i;

	ucontrol->value.enumerated.item[0] = 0;
	if (HPI_SampleClock_GetSource(phSubSys, hControl, &wSource))
		wSource = 0;

	if (wSource == HPI_SAMPLECLOCK_SOURCE_AESEBU_INPUT)
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
	struct snd_card_asihpi *asihpi =
			(struct snd_card_asihpi *)(kcontrol->private_data);
	struct clk_cache *clkcache = &asihpi->cc;
	int change, item;
	u32 hControl = kcontrol->private_value;

	change = 1;
	item = ucontrol->value.enumerated.item[0];
	if (item >= clkcache->count)
		item = clkcache->count-1;

	HPI_HandleError(HPI_SampleClock_SetSource(phSubSys,
				hControl, clkcache->s[item].source));

	if (clkcache->s[item].source == HPI_SAMPLECLOCK_SOURCE_AESEBU_INPUT)
		HPI_HandleError(HPI_SampleClock_SetSourceIndex(phSubSys,
				hControl, clkcache->s[item].index));
	return change;
}

/*------------------------------------------------------------
   Clkrate controls
 ------------------------------------------------------------*/
/* Need to change this to enumerated control with list of rates */
static int snd_asihpi_clklocal_info(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 8000;
	uinfo->value.integer.max = 192000;
	uinfo->value.integer.step = 100;

	return 0;
}

static int snd_asihpi_clklocal_get(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	u32 hControl = kcontrol->private_value;
	u32 dwRate;
	u16 e;

	e = HPI_SampleClock_GetLocalRate(phSubSys, hControl, &dwRate);
	if (!e)
		ucontrol->value.integer.value[0] = dwRate;
	else
		ucontrol->value.integer.value[0] = 0;
	return 0;
}

static int snd_asihpi_clklocal_put(struct snd_kcontrol *kcontrol,
				  struct snd_ctl_elem_value *ucontrol)
{
	int change;
	u32 hControl = kcontrol->private_value;

	/*  change = asihpi->mixer_clkrate[addr][0] != left ||
	   asihpi->mixer_clkrate[addr][1] != right;
	 */
	change = 1;
	HPI_HandleError(HPI_SampleClock_SetLocalRate(phSubSys, hControl,
				      ucontrol->value.integer.value[0]));
	return change;
}

static int snd_asihpi_clkrate_info(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_info *uinfo)
{
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
	u32 hControl = kcontrol->private_value;
	u32 dwRate;
	u16 e;

	e = HPI_SampleClock_GetSampleRate(phSubSys, hControl, &dwRate);
	if (!e)
		ucontrol->value.integer.value[0] = dwRate;
	else
		ucontrol->value.integer.value[0] = 0;
	return 0;
}

static int __devinit snd_asihpi_sampleclock_add(struct snd_card_asihpi *asihpi,
					struct hpi_control *asihpi_control)
{
	struct snd_card *card = asihpi->card;
	struct snd_kcontrol_new snd_control;

	struct clk_cache *clkcache = &asihpi->cc;
	u32 hSC =  asihpi_control->hControl;
	int hasAesIn = 0;
	int i, j;
	u16 wSource;

	snd_control.private_value = asihpi_control->hControl;

	clkcache->has_local = 0;

	for (i = 0; i <= HPI_SAMPLECLOCK_SOURCE_LAST; i++) {
		if  (HPI_SampleClock_QuerySource(phSubSys, hSC,
				i, &wSource))
			break;
		clkcache->s[i].source = wSource;
		clkcache->s[i].index = 0;
		clkcache->s[i].name = sampleclock_sources[wSource];
		if (wSource == HPI_SAMPLECLOCK_SOURCE_AESEBU_INPUT)
			hasAesIn = 1;
		if (wSource == HPI_SAMPLECLOCK_SOURCE_LOCAL)
			clkcache->has_local = 1;
	}
	if (hasAesIn)
		/* already will have picked up index 0 above */
		for (j = 1; j < 8; j++) {
			if (HPI_SampleClock_QuerySourceIndex(phSubSys, hSC,
				j, HPI_SAMPLECLOCK_SOURCE_AESEBU_INPUT,
				&wSource))
				break;
			clkcache->s[i].source =
				HPI_SAMPLECLOCK_SOURCE_AESEBU_INPUT;
			clkcache->s[i].index = j;
			clkcache->s[i].name = sampleclock_sources[
					j+HPI_SAMPLECLOCK_SOURCE_LAST];
			i++;
		}
	clkcache->count = i;

	asihpi_ctl_init(&snd_control, asihpi_control, "Source");
	snd_control.access = SNDRV_CTL_ELEM_ACCESS_READWRITE ;
	snd_control.info = snd_asihpi_clksrc_info;
	snd_control.get = snd_asihpi_clksrc_get;
	snd_control.put = snd_asihpi_clksrc_put;
	if (ctl_add(card, &snd_control, asihpi) < 0)
		return -EINVAL;


	if (clkcache->has_local) {
		asihpi_ctl_init(&snd_control, asihpi_control, "LocalRate");
		snd_control.access = SNDRV_CTL_ELEM_ACCESS_READWRITE ;
		snd_control.info = snd_asihpi_clklocal_info;
		snd_control.get = snd_asihpi_clklocal_get;
		snd_control.put = snd_asihpi_clklocal_put;


		if (ctl_add(card, &snd_control, asihpi) < 0)
			return -EINVAL;
	}

	asihpi_ctl_init(&snd_control, asihpi_control, "Rate");
	snd_control.access =
	    SNDRV_CTL_ELEM_ACCESS_VOLATILE | SNDRV_CTL_ELEM_ACCESS_READ;
	snd_control.info = snd_asihpi_clkrate_info;
	snd_control.get = snd_asihpi_clkrate_get;

	return (ctl_add(card, &snd_control, asihpi));
}
/*------------------------------------------------------------
   Mixer
 ------------------------------------------------------------*/

static int __devinit snd_card_asihpi_mixer_new(struct snd_card_asihpi *asihpi)
{
	struct snd_card *card = asihpi->card;
	unsigned int idx = 0;
	unsigned int subindex = 0;
	int err;
	struct hpi_control asihpi_control, prev_control;

	if (snd_BUG_ON(!asihpi))
		return -EINVAL;
	strcpy(card->mixername, "Asihpi Mixer");

	err =
	    HPI_MixerOpen(phSubSys, asihpi->wAdapterIndex,
			  &asihpi->hMixer);
	HPI_HandleError(err);
	if (err)
		return -err;

	for (idx = 0; idx < 2000; idx++) {
		err = HPI_MixerGetControlByIndex(phSubSys, asihpi->hMixer,
						 idx,
						 &asihpi_control.wSrcNodeType,
						 &asihpi_control.wSrcNodeIndex,
						 &asihpi_control.wDstNodeType,
						 &asihpi_control.wDstNodeIndex,
						 &asihpi_control.wControlType,
						 &asihpi_control.hControl);
		if (err) {
			if (err == HPI_ERROR_CONTROL_DISABLED) {
				if (mixer_dump)
					snd_printk(KERN_INFO
						   "Disabled HPI Control(%d)\n",
						   idx);
				continue;
			} else
				break;

		}

		asihpi_control.wSrcNodeType -= HPI_SOURCENODE_BASE;
		asihpi_control.wDstNodeType -= HPI_DESTNODE_BASE;

		/* ASI50xx in SSX mode has multiple meters on the same node.
		   Use subindex to create distinct ALSA controls
		   for any duplicated controls.
		*/
		if ((asihpi_control.wControlType == prev_control.wControlType) &&
		    (asihpi_control.wSrcNodeType == prev_control.wSrcNodeType) &&
		    (asihpi_control.wSrcNodeIndex == prev_control.wSrcNodeIndex) &&
		    (asihpi_control.wDstNodeType == prev_control.wDstNodeType) &&
		    (asihpi_control.wDstNodeIndex == prev_control.wDstNodeIndex))
			subindex++;
		else
			subindex = 0;

		prev_control = asihpi_control;

		switch (asihpi_control.wControlType) {
		case HPI_CONTROL_VOLUME:
			err = snd_asihpi_volume_add(asihpi, &asihpi_control);
			break;
		case HPI_CONTROL_LEVEL:
			err = snd_asihpi_level_add(asihpi, &asihpi_control);
			break;
		case HPI_CONTROL_MULTIPLEXER:
			err = snd_asihpi_mux_add(asihpi, &asihpi_control);
			break;
		case HPI_CONTROL_CHANNEL_MODE:
			err = snd_asihpi_cmode_add(asihpi, &asihpi_control);
			break;
		case HPI_CONTROL_METER:
			err = snd_asihpi_meter_add(asihpi, &asihpi_control, subindex);
			break;
		case HPI_CONTROL_SAMPLECLOCK:
			err = snd_asihpi_sampleclock_add(
						asihpi, &asihpi_control);
			break;
		case HPI_CONTROL_CONNECTION:	/* ignore these */
			continue;
		case HPI_CONTROL_TUNER:
			err = snd_asihpi_tuner_add(asihpi, &asihpi_control);
			break;
		case HPI_CONTROL_AESEBU_TRANSMITTER:
			err = snd_asihpi_aesebu_tx_add(asihpi, &asihpi_control);
			break;
		case HPI_CONTROL_AESEBU_RECEIVER:
			err = snd_asihpi_aesebu_rx_add(asihpi, &asihpi_control);
			break;
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
		if (err < 0)
			return err;
	}
	if (HPI_ERROR_INVALID_OBJ_INDEX != err)
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
	struct snd_card_asihpi *asihpi = entry->private_data;
	u16 wVersion;
	u32 hControl;
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
		err = HPI_SampleClock_GetSampleRate(phSubSys,
					hControl, &dwRate);
		err += HPI_SampleClock_GetSource(phSubSys, hControl, &wSource);

		if (!err)
			snd_iprintf(buffer, "SampleClock=%dHz, source %s\n",
			dwRate, sampleclock_sources[wSource]);
	}

}


static void __devinit snd_asihpi_proc_init(struct snd_card_asihpi *asihpi)
{
	struct snd_info_entry *entry;

	if (!snd_card_proc_new(asihpi->card, "info", &entry))
		snd_info_set_text_ops(entry, asihpi, snd_asihpi_proc_read);
}

/*------------------------------------------------------------
   HWDEP
 ------------------------------------------------------------*/

static int snd_asihpi_hpi_open(struct snd_hwdep *hw, struct file *file)
{
	if (enable_hpi_hwdep)
		return 0;
	else
		return -ENODEV;

}

static int snd_asihpi_hpi_release(struct snd_hwdep *hw, struct file *file)
{
	if (enable_hpi_hwdep)
		return asihpi_hpi_release(file);
	else
		return -ENODEV;
}

static int snd_asihpi_hpi_ioctl(struct snd_hwdep *hw, struct file *file,
				unsigned int cmd, unsigned long arg)
{
	if (enable_hpi_hwdep)
		return asihpi_hpi_ioctl(file, cmd, arg);
	else
		return -ENODEV;
}


/* results in /dev/snd/hwC#D0 file for each card with index #
   also /proc/asound/hwdep will contain '#-00: asihpi (HPI) for each card'
*/
static int __devinit snd_asihpi_hpi_new(struct snd_card_asihpi *asihpi,
	int device, struct snd_hwdep **rhwdep)
{
	struct snd_hwdep *hw;
	int err;

	if (rhwdep)
		*rhwdep = NULL;
	err = snd_hwdep_new(asihpi->card, "HPI", device, &hw);
	if (err < 0)
		return err;
	strcpy(hw->name, "asihpi (HPI)");
	hw->iface = SNDRV_HWDEP_IFACE_LAST;
	hw->ops.open = snd_asihpi_hpi_open;
	hw->ops.ioctl = snd_asihpi_hpi_ioctl;
	hw->ops.release = snd_asihpi_hpi_release;
	hw->private_data = asihpi;
	if (rhwdep)
		*rhwdep = hw;
	return 0;
}

/*------------------------------------------------------------
   CARD
 ------------------------------------------------------------*/
static int __devinit snd_asihpi_probe(struct pci_dev *pci_dev,
				       const struct pci_device_id *pci_id)
{
	int err;

	u16 wVersion;
	int pcm_substreams;

	struct hpi_adapter *hpi_card;
	struct snd_card *card;
	struct snd_card_asihpi *asihpi;

	u32 hControl;
	u32 hStream;

	static int dev;
	if (dev >= SNDRV_CARDS)
		return -ENODEV;

	/* Should this be enable[hpi_card->index] ? */
	if (!enable[dev]) {
		dev++;
		return -ENOENT;
	}

	err = asihpi_adapter_probe(pci_dev, pci_id);
	if (err < 0)
		return err;

	hpi_card = pci_get_drvdata(pci_dev);
	/* first try to give the card the same index as its hardware index */
	err = snd_card_create(hpi_card->index,
			      id[hpi_card->index], THIS_MODULE,
			      sizeof(struct snd_card_asihpi),
			      &card);
	if (err < 0) {
		/* if that fails, try the default index==next available */
		err =
		    snd_card_create(index[dev], id[dev],
				    THIS_MODULE,
				    sizeof(struct snd_card_asihpi),
				    &card);
		if (err < 0)
			return err;
		snd_printk(KERN_WARNING
			"**** WARNING **** Adapter index %d->ALSA index %d\n",
			hpi_card->index, card->number);
	}

	asihpi = (struct snd_card_asihpi *) card->private_data;
	asihpi->card = card;
	asihpi->pci = hpi_card->pci;
	asihpi->wAdapterIndex = hpi_card->index;
	HPI_HandleError(HPI_AdapterGetInfo(phSubSys,
				 asihpi->wAdapterIndex,
				 &asihpi->wNumOutStreams,
				 &asihpi->wNumInStreams,
				 &asihpi->wVersion,
				 &asihpi->dwSerialNumber, &asihpi->wType));

	wVersion = asihpi->wVersion;
	snd_printk(KERN_INFO "Adapter ID=%4X Index=%d NumOutStreams=%d "
			"NumInStreams=%d S/N=%d\n"
			"Hw Version %c%d DSP code version %03d\n",
			asihpi->wType, asihpi->wAdapterIndex,
			asihpi->wNumOutStreams,
			asihpi->wNumInStreams, asihpi->dwSerialNumber,
			((wVersion >> 3) & 0xf) + 'A',
			wVersion & 0x7,
			((wVersion >> 13) * 100) + ((wVersion >> 7) & 0x3f));

	pcm_substreams = asihpi->wNumOutStreams;
	if (pcm_substreams < asihpi->wNumInStreams)
		pcm_substreams = asihpi->wNumInStreams;

	HPI_HandleError(HPI_InStreamOpen(phSubSys, asihpi->wAdapterIndex,
			     0, &hStream));

	err = HPI_InStreamGroupAdd(phSubSys, hStream, hStream);
	asihpi->support_grouping = (!err);

	err = HPI_InStreamHostBufferFree(phSubSys, hStream);
	asihpi->support_mmap = (!err);

	asihpi->support_mrx = (((asihpi->wType & 0xFF00) == 0x8900) ||
			((asihpi->wType & 0xF000) == 0x6000));


	err = HPI_AdapterGetProperty(phSubSys, asihpi->wAdapterIndex,
		HPI_ADAPTER_PROPERTY_CURCHANNELS,
		&asihpi->in_max_chans, &asihpi->out_max_chans);
	if (err) {
		asihpi->in_max_chans = 2;
		asihpi->out_max_chans = 2;
	}

	printk(KERN_INFO "Supports mmap:%d grouping:%d mrx:%d\n",
			asihpi->support_mmap,
			asihpi->support_grouping,
			asihpi->support_mrx
	      );

	HPI_HandleError(HPI_InStreamClose(phSubSys, hStream));

	err = snd_card_asihpi_pcm_new(asihpi, 0, pcm_substreams);
	if (err < 0) {
		printk(KERN_ERR "pcm_new failed\n");
		goto __nodev;
	}
	err = snd_card_asihpi_mixer_new(asihpi);
	if (err < 0) {
		printk(KERN_ERR "mixer_new failed\n");
		goto __nodev;
	}

	err = HPI_MixerGetControl(phSubSys, asihpi->hMixer,
				  HPI_SOURCENODE_CLOCK_SOURCE, 0, 0, 0,
				  HPI_CONTROL_SAMPLECLOCK, &hControl);

	if (!err)
		err = HPI_SampleClock_SetLocalRate(phSubSys,
				hControl, adapter_fs);

	snd_asihpi_proc_init(asihpi);

	/* always create, can be enabled or disabled dynamically
	    by enable_hwdep  module param*/
	snd_asihpi_hpi_new(asihpi, 0, NULL);

	if (asihpi->support_mmap)
		strcpy(card->driver, "ASIHPI-MMAP");
	else
		strcpy(card->driver, "ASIHPI");

	sprintf(card->shortname, "AudioScience ASI%4X", asihpi->wType);
	sprintf(card->longname, "%s %i",
			card->shortname, asihpi->wAdapterIndex);
	err = snd_card_register(card);
	if (!err) {
		hpi_card->snd_card_asihpi = card;
		dev++;
		return 0;
	}
__nodev:
	snd_card_free(card);
	printk(KERN_ERR "snd_asihpi_probe error %d\n", err);
	return err;

}

static void __devexit snd_asihpi_remove(struct pci_dev *pci_dev)
{
	struct hpi_adapter *hpi_card = pci_get_drvdata(pci_dev);

	snd_card_free(hpi_card->snd_card_asihpi);
	hpi_card->snd_card_asihpi = NULL;
	asihpi_adapter_remove(pci_dev);
}

static struct pci_device_id  asihpi_pci_tbl[] = {
	{HPI_PCI_VENDOR_ID_TI, HPI_PCI_DEV_ID_DSP6205,
		HPI_PCI_VENDOR_ID_AUDIOSCIENCE, PCI_ANY_ID, 0, 0,
		(kernel_ulong_t)HPI_6205},
	{HPI_PCI_VENDOR_ID_TI, HPI_PCI_DEV_ID_PCI2040,
		HPI_PCI_VENDOR_ID_AUDIOSCIENCE, PCI_ANY_ID, 0, 0,
		(kernel_ulong_t)HPI_6000},
	{0,}
};
MODULE_DEVICE_TABLE(pci, asihpi_pci_tbl);

static struct pci_driver driver = {
	.name = "asihpi",
	.id_table = asihpi_pci_tbl,
	.probe = snd_asihpi_probe,
	.remove = __devexit_p(snd_asihpi_remove),
#ifdef CONFIG_PM
/*	.suspend = snd_asihpi_suspend,
	.resume = snd_asihpi_resume, */
#endif
};

static int __init snd_asihpi_init(void)
{
	asihpi_init();
	return pci_register_driver(&driver);
}

static void __exit snd_asihpi_exit(void)
{

	pci_unregister_driver(&driver);
	asihpi_exit();
}

module_init(snd_asihpi_init)
module_exit(snd_asihpi_exit)

