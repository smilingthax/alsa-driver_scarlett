/*
 *  Loopback soundcard
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
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
 *
 */

#include <sound/driver.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/moduleparam.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <sound/initval.h>

MODULE_AUTHOR("Jaroslav Kysela <perex@suse.cz>");
MODULE_DESCRIPTION("A loopback soundcard");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("{{ALSA,Loopback soundcard}}");

#define MAX_PCM_SUBSTREAMS	256

static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;	/* Index 0-MAX */
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;	/* ID for this card */
static int enable[SNDRV_CARDS] = {1, [1 ... (SNDRV_CARDS - 1)] = 0};
static int pcm_devs[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = 1};
static int pcm_substreams[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = 8};
//static int midi_devs[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = 2};

module_param_array(index, int, NULL, 0444);
MODULE_PARM_DESC(index, "Index value for loopback soundcard.");
module_param_array(id, charp, NULL, 0444);
MODULE_PARM_DESC(id, "ID string for loopback soundcard.");
module_param_array(enable, bool, NULL, 0444);
MODULE_PARM_DESC(enable, "Enable this loopback soundcard.");
module_param_array(pcm_devs, int, NULL, 0444);
MODULE_PARM_DESC(pcm_devs, "PCM devices # (0-4) for loopback driver.");
module_param_array(pcm_substreams, int, NULL, 0444);
MODULE_PARM_DESC(pcm_substreams, "PCM substreams # (1-16) for loopback driver.");
//module_param_array(midi_devs, int, NULL, 0444);
//MODULE_PARM_DESC(midi_devs, "MIDI devices # (0-2) for loopback driver.");

typedef struct snd_card_loopback {
	snd_card_t *card;
} snd_card_loopback_t;

typedef struct snd_card_loopback_pcm {
	snd_card_loopback_t *loopback;
	spinlock_t lock;
	struct timer_list timer;
	unsigned int pcm_size;
	unsigned int pcm_count;
	unsigned int pcm_bps;		/* bytes per second */
	unsigned int pcm_jiffie;	/* bytes per one jiffie */
	unsigned int pcm_irq_pos;	/* IRQ position */
	unsigned int pcm_buf_pos;	/* position in buffer */
	snd_pcm_substream_t *substream;
} snd_card_loopback_pcm_t;

static snd_card_t *snd_loopback_cards[SNDRV_CARDS] = SNDRV_DEFAULT_PTR;


static void snd_card_loopback_pcm_timer_start(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_loopback_pcm_t *dpcm = runtime->private_data;

	dpcm->timer.expires = 1 + jiffies;
	add_timer(&dpcm->timer);
}

static void snd_card_loopback_pcm_timer_stop(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_loopback_pcm_t *dpcm = runtime->private_data;

	del_timer(&dpcm->timer);
}

static int snd_card_loopback_playback_trigger(snd_pcm_substream_t * substream,
					   int cmd)
{
	if (cmd == SNDRV_PCM_TRIGGER_START) {
		snd_card_loopback_pcm_timer_start(substream);
	} else if (cmd == SNDRV_PCM_TRIGGER_STOP) {
		snd_card_loopback_pcm_timer_stop(substream);
	} else {
		return -EINVAL;
	}
	return 0;
}

static int snd_card_loopback_capture_trigger(snd_pcm_substream_t * substream,
					  int cmd)
{
	if (cmd == SNDRV_PCM_TRIGGER_START) {
		snd_card_loopback_pcm_timer_start(substream);
	} else if (cmd == SNDRV_PCM_TRIGGER_STOP) {
		snd_card_loopback_pcm_timer_stop(substream);
	} else {
		return -EINVAL;
	}
	return 0;
}

static int snd_card_loopback_pcm_prepare(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_loopback_pcm_t *dpcm = runtime->private_data;
	unsigned int bps;

	bps = runtime->rate * runtime->channels;
	bps *= snd_pcm_format_width(runtime->format);
	bps /= 8;
	if (bps <= 0)
		return -EINVAL;
	dpcm->pcm_bps = bps;
	dpcm->pcm_jiffie = bps / HZ;
	dpcm->pcm_size = snd_pcm_lib_buffer_bytes(substream);
	dpcm->pcm_count = snd_pcm_lib_period_bytes(substream);
	dpcm->pcm_irq_pos = 0;
	dpcm->pcm_buf_pos = 0;
	return 0;
}

static int snd_card_loopback_playback_prepare(snd_pcm_substream_t * substream)
{
	return snd_card_loopback_pcm_prepare(substream);
}

static int snd_card_loopback_capture_prepare(snd_pcm_substream_t * substream)
{
	return snd_card_loopback_pcm_prepare(substream);
}

static void snd_card_loopback_pcm_timer_function(unsigned long data)
{
	snd_card_loopback_pcm_t *dpcm = (snd_card_loopback_pcm_t *)data;
	
	dpcm->timer.expires = 1 + jiffies;
	add_timer(&dpcm->timer);
	spin_lock_irq(&dpcm->lock);
	dpcm->pcm_irq_pos += dpcm->pcm_jiffie;
	dpcm->pcm_buf_pos += dpcm->pcm_jiffie;
	dpcm->pcm_buf_pos %= dpcm->pcm_size;
	if (dpcm->pcm_irq_pos >= dpcm->pcm_count) {
		dpcm->pcm_irq_pos %= dpcm->pcm_count;
		snd_pcm_period_elapsed(dpcm->substream);
	}
	spin_unlock_irq(&dpcm->lock);	
}

static snd_pcm_uframes_t snd_card_loopback_playback_pointer(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_loopback_pcm_t *dpcm = runtime->private_data;

	return bytes_to_frames(runtime, dpcm->pcm_buf_pos);
}

static snd_pcm_uframes_t snd_card_loopback_capture_pointer(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_loopback_pcm_t *dpcm = runtime->private_data;

	return bytes_to_frames(runtime, dpcm->pcm_buf_pos);
}

static snd_pcm_hardware_t snd_card_loopback_info =
{
	.info =			(SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
				 SNDRV_PCM_INFO_MMAP_VALID),
	.formats =		(SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S32_LE),
	.rates =		SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_192000,
	.rate_min =		5500,
	.rate_max =		192000,
	.channels_min =		1,
	.channels_max =		32,
	.buffer_bytes_max =	1024 * 1024,
	.period_bytes_min =	32,
	.period_bytes_max =	128 * 1024,
	.periods_min =		1,
	.periods_max =		16,
	.fifo_size =		0,
};

static void snd_card_loopback_runtime_free(snd_pcm_runtime_t *runtime)
{
	snd_card_loopback_pcm_t *dpcm = runtime->private_data;
	kfree(dpcm);
}

static int snd_card_loopback_hw_params(snd_pcm_substream_t * substream,
				    snd_pcm_hw_params_t * hw_params)
{
	return snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
}

static int snd_card_loopback_hw_free(snd_pcm_substream_t * substream)
{
	return snd_pcm_lib_free_pages(substream);
}

static int snd_card_loopback_pcm_open(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_loopback_pcm_t *dpcm;

	dpcm = kcalloc(1, sizeof(*dpcm), GFP_KERNEL);
	if (dpcm == NULL)
		return -ENOMEM;
	init_timer(&dpcm->timer);
	dpcm->timer.data = (unsigned long) dpcm;
	dpcm->timer.function = snd_card_loopback_pcm_timer_function;
	spin_lock_init(&dpcm->lock);
	dpcm->substream = substream;
	runtime->private_data = dpcm;
	runtime->private_free = snd_card_loopback_runtime_free;
	runtime->hw = snd_card_loopback_info;
	return 0;
}

static snd_pcm_ops_t snd_card_loopback_playback_ops = {
	.open =			snd_card_loopback_pcm_open,
	.close =		NULL,
	.ioctl =		snd_pcm_lib_ioctl,
	.hw_params =		snd_card_loopback_hw_params,
	.hw_free =		snd_card_loopback_hw_free,
	.prepare =		snd_card_loopback_playback_prepare,
	.trigger =		snd_card_loopback_playback_trigger,
	.pointer =		snd_card_loopback_playback_pointer,
};

static snd_pcm_ops_t snd_card_loopback_capture_ops = {
	.open =			snd_card_loopback_pcm_open,
	.close =		NULL,
	.ioctl =		snd_pcm_lib_ioctl,
	.hw_params =		snd_card_loopback_hw_params,
	.hw_free =		snd_card_loopback_hw_free,
	.prepare =		snd_card_loopback_capture_prepare,
	.trigger =		snd_card_loopback_capture_trigger,
	.pointer =		snd_card_loopback_capture_pointer,
};

static int __init snd_card_loopback_pcm(snd_card_loopback_t *loopback, int device, int substreams)
{
	snd_pcm_t *pcm;
	int err;

	if ((err = snd_pcm_new(loopback->card, "Loopback PCM", device, substreams, substreams, &pcm)) < 0)
		return err;
	if (device == 0)
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_card_loopback_playback_ops);
	else
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &snd_card_loopback_capture_ops);
	pcm->private_data = loopback;
	pcm->info_flags = 0;
	strcpy(pcm->name, "Loopback PCM");
	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_CONTINUOUS,
					      snd_dma_continuous_data(GFP_KERNEL),
					      0, 64*1024);
	return 0;
}

static int __init snd_card_loopback_new_mixer(snd_card_loopback_t * loopback)
{
	snd_card_t *card = loopback->card;

	snd_assert(loopback != NULL, return -EINVAL);
	strcpy(card->mixername, "Loopback Mixer");
	return 0;
}

static int __init snd_card_loopback_probe(int dev)
{
	snd_card_t *card;
	struct snd_card_loopback *loopback;
	int err;

	if (!enable[dev])
		return -ENODEV;
	card = snd_card_new(index[dev], id[dev], THIS_MODULE,
			    sizeof(struct snd_card_loopback));
	if (card == NULL)
		return -ENOMEM;
	loopback = (struct snd_card_loopback *)card->private_data;
	loopback->card = card;
	if (pcm_substreams[dev] < 1)
		pcm_substreams[dev] = 1;
	if (pcm_substreams[dev] > MAX_PCM_SUBSTREAMS)
		pcm_substreams[dev] = MAX_PCM_SUBSTREAMS;
	if ((err = snd_card_loopback_pcm(loopback, 0, pcm_substreams[dev])) < 0)
		goto __nodev;
	if ((err = snd_card_loopback_pcm(loopback, 1, pcm_substreams[dev])) < 0)
		goto __nodev;
	if ((err = snd_card_loopback_new_mixer(loopback)) < 0)
		goto __nodev;
	strcpy(card->driver, "Loopback");
	strcpy(card->shortname, "Loopback");
	sprintf(card->longname, "Loopback %i", dev + 1);
	if ((err = snd_card_register(card)) == 0) {
		snd_loopback_cards[dev] = card;
		return 0;
	}
      __nodev:
	snd_card_free(card);
	return err;
}

static int __init alsa_card_loopback_init(void)
{
	int dev, cards;

	for (dev = cards = 0; dev < SNDRV_CARDS && enable[dev]; dev++) {
		if (snd_card_loopback_probe(dev) < 0) {
#ifdef MODULE
			printk(KERN_ERR "Loopback soundcard #%i not found or device busy\n", dev + 1);
#endif
			break;
		}
		cards++;
	}
	if (!cards) {
#ifdef MODULE
		printk(KERN_ERR "Loopback soundcard not found or device busy\n");
#endif
		return -ENODEV;
	}
	return 0;
}

static void __exit alsa_card_loopback_exit(void)
{
	int idx;

	for (idx = 0; idx < SNDRV_CARDS; idx++)
		snd_card_free(snd_loopback_cards[idx]);
}

module_init(alsa_card_loopback_init)
module_exit(alsa_card_loopback_exit)
