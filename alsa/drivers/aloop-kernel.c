/*
 *  Loopback soundcard
 *
 *  Original code:
 *  Copyright (c) by Jaroslav Kysela <perex@perex.cz>
 *
 *  More accurate positioning and full-duplex support:
 *  Copyright (c) Ahmet İnan <ainan at mathematik.uni-freiburg.de>
 *
 *  Major (almost complete) rewrite:
 *  Copyright (c) by Takashi Iwai <tiwai@suse.de>
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

#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <sound/initval.h>

MODULE_AUTHOR("Jaroslav Kysela <perex@perex.cz>");
MODULE_DESCRIPTION("A loopback soundcard");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("{{ALSA,Loopback soundcard}}");

#define MAX_PCM_SUBSTREAMS	8

static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;	/* Index 0-MAX */
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;	/* ID for this card */
static int enable[SNDRV_CARDS] = {1, [1 ... (SNDRV_CARDS - 1)] = 0};
static int pcm_substreams[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = 8};

module_param_array(index, int, NULL, 0444);
MODULE_PARM_DESC(index, "Index value for loopback soundcard.");
module_param_array(id, charp, NULL, 0444);
MODULE_PARM_DESC(id, "ID string for loopback soundcard.");
module_param_array(enable, bool, NULL, 0444);
MODULE_PARM_DESC(enable, "Enable this loopback soundcard.");
module_param_array(pcm_substreams, int, NULL, 0444);
MODULE_PARM_DESC(pcm_substreams, "PCM substreams # (1-8) for loopback driver.");

struct loopback_pcm;

struct loopback_cable {
	struct loopback_pcm *streams[2];
	struct snd_pcm_hardware hw;
	/* PCM parameters */
	unsigned int pcm_period_size;
	unsigned int pcm_bps;		/* bytes per second */
	/* flags */
	unsigned int valid;
	unsigned int running;
	unsigned int period_update_pending :1;
	/* timer stuff */
	unsigned int irq_pos;		/* fractional IRQ position */
	unsigned int period_size_frac;
	unsigned long last_jiffies;
	struct timer_list timer;
};

struct loopback {
	struct snd_card *card;
	struct mutex cable_lock;
	struct loopback_cable *cables[MAX_PCM_SUBSTREAMS][2];
	struct snd_pcm *pcm[2];
};

struct loopback_pcm {
	struct loopback *loopback;
	struct snd_pcm_substream *substream;
	struct loopback_cable *cable;
	unsigned int pcm_buffer_size;
	unsigned int buf_pos;	/* position in buffer */
	unsigned int silent_size;
};

static struct platform_device *devices[SNDRV_CARDS];

#define byte_pos(x)	((x) / HZ)
#define frac_pos(x)	((x) * HZ)

static void loopback_timer_start(struct loopback_cable *cable)
{
	unsigned long tick;

	tick = cable->period_size_frac - cable->irq_pos;
	tick = (tick + cable->pcm_bps - 1) / cable->pcm_bps;
	cable->timer.expires = jiffies + tick;
	add_timer(&cable->timer);
}

static void loopback_timer_stop(struct loopback_cable *cable)
{
	del_timer(&cable->timer);
}

static int loopback_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct loopback_pcm *dpcm = runtime->private_data;
	struct loopback_cable *cable = dpcm->cable;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		if (!cable->running) {
			cable->last_jiffies = jiffies;
			loopback_timer_start(cable);
		}
		cable->running |= (1 << substream->stream);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		cable->running &= ~(1 << substream->stream);
		if (!cable->running)
			loopback_timer_stop(cable);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int loopback_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct loopback_pcm *dpcm = runtime->private_data;
	struct loopback_cable *cable = dpcm->cable;
	unsigned int bps;

	bps = runtime->rate * runtime->channels;
	bps *= snd_pcm_format_width(runtime->format);
	bps /= 8;
	if (bps <= 0)
		return -EINVAL;

	dpcm->buf_pos = 0;
	dpcm->pcm_buffer_size = frames_to_bytes(runtime, runtime->buffer_size);
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		/* clear capture buffer */
		dpcm->silent_size = dpcm->pcm_buffer_size;
		memset(runtime->dma_area, 0, dpcm->pcm_buffer_size);
	}

	if (!cable->running) {
		cable->irq_pos = 0;
		cable->period_update_pending = 0;
	}

	mutex_lock(&dpcm->loopback->cable_lock);
	if (!(cable->valid & ~(1 << substream->stream))) {
		cable->pcm_bps = bps;
		cable->pcm_period_size =
			frames_to_bytes(runtime, runtime->period_size);
		cable->period_size_frac = frac_pos(cable->pcm_period_size);

		cable->hw.formats = (1ULL << runtime->format);
		cable->hw.rate_min = runtime->rate;
		cable->hw.rate_max = runtime->rate;
		cable->hw.channels_min = runtime->channels;
		cable->hw.channels_max = runtime->channels;
		cable->hw.period_bytes_min = cable->pcm_period_size;
		cable->hw.period_bytes_max = cable->pcm_period_size;
	}
	cable->valid |= 1 << substream->stream;
	mutex_unlock(&dpcm->loopback->cable_lock);

	return 0;
}

static void copy_play_buf(struct loopback_pcm *play,
			  struct loopback_pcm *capt,
			  unsigned int bytes)
{
	char *src = play->substream->runtime->dma_area;
	char *dst = capt->substream->runtime->dma_area;
	unsigned int src_off = play->buf_pos;
	unsigned int dst_off = capt->buf_pos;

	for (;;) {
		unsigned int size = bytes;
		if (src_off + size > play->pcm_buffer_size)
			size = play->pcm_buffer_size - src_off;
		if (dst_off + size > capt->pcm_buffer_size)
			size = capt->pcm_buffer_size - dst_off;
		memcpy(dst + dst_off, src + src_off, size);
		if (size < capt->silent_size)
			capt->silent_size -= size;
		else
			capt->silent_size = 0;
		bytes -= size;
		if (!bytes)
			break;
		src_off = (src_off + size) % play->pcm_buffer_size;
		dst_off = (dst_off + size) % capt->pcm_buffer_size;
	}
}

static void clear_capture_buf(struct loopback_pcm *dpcm, unsigned int bytes)
{
	char *dst = dpcm->substream->runtime->dma_area;
	unsigned int dst_off = dpcm->buf_pos;

	if (dpcm->silent_size >= dpcm->pcm_buffer_size)
		return;
	if (dpcm->silent_size + bytes > dpcm->pcm_buffer_size)
		bytes = dpcm->pcm_buffer_size - dpcm->silent_size;

	for (;;) {
		unsigned int size = bytes;
		if (dst_off + size > dpcm->pcm_buffer_size)
			size = dpcm->pcm_buffer_size - dst_off;
		memset(dst + dst_off, 0, size);
		dpcm->silent_size += size;
		bytes -= size;
		if (!bytes)
			break;
		dst_off = 0;
	}
}

#define CABLE_PLAYBACK	(1 << SNDRV_PCM_STREAM_PLAYBACK)
#define CABLE_CAPTURE	(1 << SNDRV_PCM_STREAM_CAPTURE)
#define CABLE_BOTH	(CABLE_PLAYBACK | CABLE_CAPTURE)

static void loopback_xfer_buf(struct loopback_cable *cable, unsigned int count)
{
	int i;

	switch (cable->running) {
	case CABLE_CAPTURE:
		clear_capture_buf(cable->streams[SNDRV_PCM_STREAM_CAPTURE],
				  count);
		break;
	case CABLE_BOTH:
		copy_play_buf(cable->streams[SNDRV_PCM_STREAM_PLAYBACK],
			      cable->streams[SNDRV_PCM_STREAM_CAPTURE],
			      count);
		break;
	}

	for (i = 0; i < 2; i++) {
		if (cable->running & (1 << i)) {
			struct loopback_pcm *dpcm = cable->streams[i];
			dpcm->buf_pos += count;
			dpcm->buf_pos %= dpcm->pcm_buffer_size;
		}
	}
}

static void loopback_pos_update(struct loopback_cable *cable)
{
	unsigned int last_pos, count;
	unsigned long delta;

	if (!cable->running)
		return;
	delta = jiffies - cable->last_jiffies;
	if (!delta)
		return;
	cable->last_jiffies += delta;

	last_pos = byte_pos(cable->irq_pos);
	cable->irq_pos += delta * cable->pcm_bps;
	count = byte_pos(cable->irq_pos) - last_pos;
	if (!count)
		return;
	loopback_xfer_buf(cable, count);
	if (cable->irq_pos >= cable->period_size_frac) {
		cable->irq_pos %= cable->period_size_frac;
		cable->period_update_pending = 1;
	}
}

static void loopback_timer_function(unsigned long data)
{
	struct loopback_cable *cable = (struct loopback_cable *)data;
	int i;

	if (!cable->running)
		return;
	loopback_pos_update(cable);
	loopback_timer_start(cable);
	if (cable->period_update_pending) {
		cable->period_update_pending = 0;
		for (i = 0; i < 2; i++) {
			if (cable->running & (1 << i)) {
				struct loopback_pcm *dpcm = cable->streams[i];
				snd_pcm_period_elapsed(dpcm->substream);
			}
		}
	}
}

static snd_pcm_uframes_t loopback_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct loopback_pcm *dpcm = runtime->private_data;

	loopback_pos_update(dpcm->cable);
	return bytes_to_frames(runtime, dpcm->buf_pos);
}

static struct snd_pcm_hardware loopback_pcm_hardware =
{
	.info =		(SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_MMAP |
			 SNDRV_PCM_INFO_MMAP_VALID),
	.formats =	(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE |
			 SNDRV_PCM_FMTBIT_S32_LE | SNDRV_PCM_FMTBIT_S32_BE |
			 SNDRV_PCM_FMTBIT_FLOAT_LE | SNDRV_PCM_FMTBIT_FLOAT_BE),
	.rates =	SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_192000,
	.rate_min =		8000,
	.rate_max =		192000,
	.channels_min =		1,
	.channels_max =		32,
	.buffer_bytes_max =	2 * 1024 * 1024,
	.period_bytes_min =	64,
	.period_bytes_max =	2 * 1024 * 1024,
	.periods_min =		1,
	.periods_max =		1024,
	.fifo_size =		0,
};

static void loopback_runtime_free(struct snd_pcm_runtime *runtime)
{
	struct loopback_pcm *dpcm = runtime->private_data;
	kfree(dpcm);
}

static int loopback_hw_params(struct snd_pcm_substream *substream,
			      struct snd_pcm_hw_params *params)
{
	return snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
}

static int loopback_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct loopback_pcm *dpcm = runtime->private_data;
	struct loopback_cable *cable = dpcm->cable;

	mutex_lock(&dpcm->loopback->cable_lock);
	cable->valid &= ~(1 << substream->stream);
	mutex_unlock(&dpcm->loopback->cable_lock);
	return snd_pcm_lib_free_pages(substream);
}

static unsigned int get_cable_index(struct snd_pcm_substream *substream)
{
	if (!substream->pcm->device)
		return substream->stream;
	else
		return !substream->stream;
}

static int loopback_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct loopback *loopback = substream->private_data;
	struct loopback_pcm *dpcm;
	struct loopback_cable *cable;
	int err = 0;
	int dev = get_cable_index(substream);

	mutex_lock(&loopback->cable_lock);
	dpcm = kzalloc(sizeof(*dpcm), GFP_KERNEL);
	if (!dpcm) {
		err = -ENOMEM;
		goto unlock;
	}
	dpcm->loopback = loopback;
	dpcm->substream = substream;

	cable = loopback->cables[substream->number][dev];
	if (!cable) {
		cable = kzalloc(sizeof(*cable), GFP_KERNEL);
		if (!cable) {
			kfree(dpcm);
			err = -ENOMEM;
			goto unlock;
		}
		cable->hw = loopback_pcm_hardware;
		setup_timer(&cable->timer, loopback_timer_function,
			    (unsigned long)cable);
		loopback->cables[substream->number][dev] = cable;
	}
	dpcm->cable = cable;
	cable->streams[substream->stream] = dpcm;

	snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);

	runtime->private_data = dpcm;
	runtime->private_free = loopback_runtime_free;
	runtime->hw = cable->hw;
 unlock:
	mutex_unlock(&loopback->cable_lock);
	return err;
}

static int loopback_close(struct snd_pcm_substream *substream)
{
	struct loopback *loopback = substream->private_data;
	struct loopback_cable *cable;
	int dev = get_cable_index(substream);

	mutex_lock(&loopback->cable_lock);
	cable = loopback->cables[substream->number][dev];
	if (cable->streams[!substream->stream]) {
		/* other stream is still alive */
		cable->streams[substream->stream] = NULL;
	} else {
		/* free the cable */
		del_timer(&cable->timer);
		loopback->cables[substream->number][dev] = NULL;
		kfree(cable);
	}
	mutex_unlock(&loopback->cable_lock);
	return 0;
}

static struct snd_pcm_ops loopback_playback_ops = {
	.open =		loopback_open,
	.close =	loopback_close,
	.ioctl =	snd_pcm_lib_ioctl,
	.hw_params =	loopback_hw_params,
	.hw_free =	loopback_hw_free,
	.prepare =	loopback_prepare,
	.trigger =	loopback_trigger,
	.pointer =	loopback_pointer,
};

static struct snd_pcm_ops loopback_capture_ops = {
	.open =		loopback_open,
	.close =	loopback_close,
	.ioctl =	snd_pcm_lib_ioctl,
	.hw_params =	loopback_hw_params,
	.hw_free =	loopback_hw_free,
	.prepare =	loopback_prepare,
	.trigger =	loopback_trigger,
	.pointer =	loopback_pointer,
};

static int __devinit loopback_pcm_new(struct loopback *loopback,
				      int device, int substreams)
{
	struct snd_pcm *pcm;
	int err;

	err = snd_pcm_new(loopback->card, "Loopback PCM", device,
			  substreams, substreams, &pcm);
	if (err < 0)
		return err;
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &loopback_playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &loopback_capture_ops);

	pcm->private_data = loopback;
	pcm->info_flags = 0;
	strcpy(pcm->name, "Loopback PCM");

	loopback->pcm[device] = pcm;

	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_CONTINUOUS,
			snd_dma_continuous_data(GFP_KERNEL),
			0, 2 * 1024 * 1024);
	return 0;
}

static int __devinit loopback_mixer_new(struct loopback *loopback)
{
	struct snd_card *card = loopback->card;

	strcpy(card->mixername, "Loopback Mixer");
	return 0;
}

static int __devinit loopback_probe(struct platform_device *devptr)
{
	struct snd_card *card;
	struct loopback *loopback;
	int dev = devptr->id;
	int err;

	card = snd_card_new(index[dev], id[dev], THIS_MODULE,
			    sizeof(struct loopback));
	if (!card)
		return -ENOMEM;
	loopback = card->private_data;

	if (pcm_substreams[dev] < 1)
		pcm_substreams[dev] = 1;
	if (pcm_substreams[dev] > MAX_PCM_SUBSTREAMS)
		pcm_substreams[dev] = MAX_PCM_SUBSTREAMS;
	
	loopback->card = card;
	mutex_init(&loopback->cable_lock);

	err = loopback_pcm_new(loopback, 0, pcm_substreams[dev]);
	if (err < 0)
		goto __nodev;
	err = loopback_pcm_new(loopback, 1, pcm_substreams[dev]);
	if (err < 0)
		goto __nodev;
	err = loopback_mixer_new(loopback);
	if (err < 0)
		goto __nodev;
	strcpy(card->driver, "Loopback");
	strcpy(card->shortname, "Loopback");
	sprintf(card->longname, "Loopback %i", dev + 1);
	err = snd_card_register(card);
	if (!err) {
		platform_set_drvdata(devptr, card);
		return 0;
	}
      __nodev:
	snd_card_free(card);
	return err;
}

static int __devexit loopback_remove(struct platform_device *devptr)
{
	snd_card_free(platform_get_drvdata(devptr));
	platform_set_drvdata(devptr, NULL);
	return 0;
}

#ifdef CONFIG_PM
static int loopback_suspend(struct platform_device *pdev,
				pm_message_t state)
{
	struct snd_card *card = platform_get_drvdata(pdev);
	struct loopback *loopback = card->private_data;

	snd_power_change_state(card, SNDRV_CTL_POWER_D3hot);

	snd_pcm_suspend_all(loopback->pcm[0]);
	snd_pcm_suspend_all(loopback->pcm[1]);
	return 0;
}
	
static int loopback_resume(struct platform_device *pdev)
{
	struct snd_card *card = platform_get_drvdata(pdev);

	snd_power_change_state(card, SNDRV_CTL_POWER_D0);
	return 0;
}
#endif

#define SND_LOOPBACK_DRIVER	"snd_aloop"

static struct platform_driver loopback_driver = {
	.probe		= loopback_probe,
	.remove		= __devexit_p(loopback_remove),
#ifdef CONFIG_PM
	.suspend	= loopback_suspend,
	.resume		= loopback_resume,
#endif
	.driver		= {
		.name	= SND_LOOPBACK_DRIVER
	},
};

static void loopback_unregister_all(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(devices); ++i)
		platform_device_unregister(devices[i]);
	platform_driver_unregister(&loopback_driver);
}

static int __init alsa_card_loopback_init(void)
{
	int i, err, cards;

	err = platform_driver_register(&loopback_driver);
	if (err < 0)
		return err;


	cards = 0;
	for (i = 0; i < SNDRV_CARDS; i++) {
		struct platform_device *device;
		if (!enable[i])
			continue;
		device = platform_device_register_simple(SND_LOOPBACK_DRIVER,
							 i, NULL, 0);
		if (IS_ERR(device))
			continue;
		if (!platform_get_drvdata(device)) {
			platform_device_unregister(device);
			continue;
		}
		devices[i] = device;
		cards++;
	}
	if (!cards) {
#ifdef MODULE
		printk(KERN_ERR "aloop: No loopback enabled\n");
#endif
		loopback_unregister_all();
		return -ENODEV;
	}
	return 0;
}

static void __exit alsa_card_loopback_exit(void)
{
	loopback_unregister_all();
}

module_init(alsa_card_loopback_init)
module_exit(alsa_card_loopback_exit)
