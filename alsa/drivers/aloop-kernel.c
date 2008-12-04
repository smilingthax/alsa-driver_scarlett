/*
 *  Loopback soundcard
 *  Copyright (c) by Jaroslav Kysela <perex@perex.cz>
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

struct loopback_cable {
	struct snd_pcm_substream *playback;
	struct snd_pcm_substream *capture;
	struct snd_dma_buffer *dma_buffer;
	struct snd_pcm_hardware hw;
	int playback_valid;
	int capture_valid;
	int playback_running;
	int capture_running;
};

struct loopback {
	struct snd_card *card;
	struct loopback_cable cables[MAX_PCM_SUBSTREAMS][2];
};

struct loopback_pcm {
	struct loopback *loopback;
	struct timer_list timer;
	unsigned int pcm_buffer_size;
	unsigned int pcm_period_size;
	unsigned int pcm_bps;		/* bytes per second */
	unsigned int pcm_hz;		/* HZ */
	unsigned int pcm_irq_pos;	/* IRQ position */
	unsigned int pcm_buf_pos;	/* position in buffer */
	struct snd_pcm_substream *substream;
	struct loopback_cable *cable;
};

static struct platform_device *devices[SNDRV_CARDS];

static void loopback_timer_start(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct loopback_pcm *dpcm = runtime->private_data;

	dpcm->timer.expires = 1 + jiffies;
	add_timer(&dpcm->timer);
}

static void loopback_timer_stop(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct loopback_pcm *dpcm = runtime->private_data;

	del_timer(&dpcm->timer);
}

static int loopback_playback_trigger(struct snd_pcm_substream *substream,
				     int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct loopback_pcm *dpcm = runtime->private_data;

	if (cmd == SNDRV_PCM_TRIGGER_START) {
		dpcm->cable->playback_running = 1;
		loopback_timer_start(substream);
	} else if (cmd == SNDRV_PCM_TRIGGER_STOP) {
		dpcm->cable->playback_running = 0;
		loopback_timer_stop(substream);
		snd_pcm_format_set_silence(runtime->format, runtime->dma_area,
				bytes_to_samples(runtime, runtime->dma_bytes));

	} else {
		return -EINVAL;
	}
	return 0;
}

static int loopback_capture_trigger(struct snd_pcm_substream *substream,
				    int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct loopback_pcm *dpcm = runtime->private_data;

	if (cmd == SNDRV_PCM_TRIGGER_START) {
		dpcm->cable->capture_running = 1;
		loopback_timer_start(substream);
	} else if (cmd == SNDRV_PCM_TRIGGER_STOP) {
		dpcm->cable->capture_running = 0;
		loopback_timer_stop(substream);
	} else {
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
	dpcm->pcm_bps = bps;
	dpcm->pcm_hz = HZ;
	dpcm->pcm_buffer_size = frames_to_bytes(runtime, runtime->buffer_size);
	dpcm->pcm_period_size = frames_to_bytes(runtime, runtime->period_size);
	dpcm->pcm_irq_pos = 0;
	dpcm->pcm_buf_pos = 0;

	cable->hw.formats = (1ULL << runtime->format);
	cable->hw.rate_min = runtime->rate;
	cable->hw.rate_max = runtime->rate;
	cable->hw.channels_min = runtime->channels;
	cable->hw.channels_max = runtime->channels;
	cable->hw.buffer_bytes_max = frames_to_bytes(runtime,
						     runtime->buffer_size);
	cable->hw.period_bytes_min = frames_to_bytes(runtime,
						     runtime->period_size);
	cable->hw.period_bytes_max = frames_to_bytes(runtime,
						     runtime->period_size);
	cable->hw.periods_min = runtime->periods;
	cable->hw.periods_max = runtime->periods;

	if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
		snd_pcm_format_set_silence(runtime->format, runtime->dma_area,
				bytes_to_samples(runtime, runtime->dma_bytes));
		cable->playback_valid = 1;
	} else {
		if (!cable->playback_running) {
			snd_pcm_format_set_silence(runtime->format,
				runtime->dma_area,
				bytes_to_samples(runtime, runtime->dma_bytes));
		}
		cable->capture_valid = 1;
	}

	return 0;
}

static void loopback_timer_function(unsigned long data)
{
	struct loopback_pcm *dpcm = (struct loopback_pcm *)data;
	
	dpcm->timer.expires = 1 + jiffies;
	add_timer(&dpcm->timer);

	dpcm->pcm_irq_pos += dpcm->pcm_bps;
	dpcm->pcm_buf_pos += dpcm->pcm_bps;
	dpcm->pcm_buf_pos %= dpcm->pcm_buffer_size * dpcm->pcm_hz;
	if (dpcm->pcm_irq_pos >= dpcm->pcm_period_size * dpcm->pcm_hz) {
		dpcm->pcm_irq_pos %= dpcm->pcm_period_size * dpcm->pcm_hz;
		snd_pcm_period_elapsed(dpcm->substream);
	}
}

static snd_pcm_uframes_t loopback_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct loopback_pcm *dpcm = runtime->private_data;
	return bytes_to_frames(runtime, dpcm->pcm_buf_pos / dpcm->pcm_hz);
}

static struct snd_pcm_hardware loopback_pcm_hardware =
{
	.info =		(SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
			 SNDRV_PCM_INFO_MMAP_VALID),
	.formats =	(SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE |
			 SNDRV_PCM_FMTBIT_S32_LE | SNDRV_PCM_FMTBIT_FLOAT_LE),
	.rates =	(SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_192000),
	.rate_min =		8000,
	.rate_max =		192000,
	.channels_min =		1,
	.channels_max =		32,
	.buffer_bytes_max =	64 * 1024,
	.period_bytes_min =	64,
	.period_bytes_max =	64 * 1024,
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
			      struct snd_pcm_hw_params *hw_params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct loopback_pcm *dpcm = runtime->private_data;
	struct snd_dma_buffer *dmab;

	if (!dpcm->cable->dma_buffer) {
		dmab = kzalloc(sizeof(*dmab), GFP_KERNEL);
		if (!dmab)
			return -ENOMEM;
		
		if (snd_dma_alloc_pages(SNDRV_DMA_TYPE_CONTINUOUS,
					snd_dma_continuous_data(GFP_KERNEL),
					params_buffer_bytes(hw_params),
					dmab) < 0) {
			kfree(dmab);
			return -ENOMEM;
		}
		dpcm->cable->dma_buffer = dmab;
	}
	snd_pcm_set_runtime_buffer(substream, dpcm->cable->dma_buffer);
	return 0;
}

static int loopback_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct loopback_pcm *dpcm = runtime->private_data;
	struct loopback_cable *cable = dpcm->cable;

	snd_pcm_set_runtime_buffer(substream, NULL);

	if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
		if (cable->capture)
			return 0;
	} else {
		if (cable->playback)
			return 0;
	}

	if (!cable->dma_buffer)
		return 0;

	snd_dma_free_pages(cable->dma_buffer);
	kfree(cable->dma_buffer);
	cable->dma_buffer = NULL;
	return 0;
}

static int loopback_open(struct snd_pcm_substream *substream)
{
	int half;
	struct loopback_pcm *dpcm;
	struct loopback *loopback = substream->private_data;
	struct snd_pcm_runtime *runtime = substream->runtime;

	if (!substream->pcm->device) {
		if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream)
			half = 1;
		else
			half = 0;
	} else {
		if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream)
			half = 0;
		else
			half = 1;
	}
	if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
		if (loopback->cables[substream->number][half].playback)
			return -EBUSY;
	} else {
		if (loopback->cables[substream->number][half].capture)
			return -EBUSY;
	}
	dpcm = kzalloc(sizeof(*dpcm), GFP_KERNEL);
	if (!dpcm)
		return -ENOMEM;
	init_timer(&dpcm->timer);
	dpcm->substream = substream;
	dpcm->loopback = loopback;
	dpcm->timer.data = (unsigned long)dpcm;
	dpcm->timer.function = loopback_timer_function;
	dpcm->cable = &loopback->cables[substream->number][half];
	runtime->private_data = dpcm;
	runtime->private_free = loopback_runtime_free;
	runtime->hw = loopback_pcm_hardware;

	if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
		dpcm->cable->playback = substream;
		dpcm->cable->playback_valid = 0;
		dpcm->cable->playback_running = 0;
		if (dpcm->cable->capture_valid)
			runtime->hw = dpcm->cable->hw;
		else
			dpcm->cable->hw = loopback_pcm_hardware;
	} else {
		dpcm->cable->capture = substream;
		dpcm->cable->capture_valid = 0;
		dpcm->cable->capture_running = 0;
		if (dpcm->cable->playback_valid)
			runtime->hw = dpcm->cable->hw;
		else
			dpcm->cable->hw = loopback_pcm_hardware;
	}
	return 0;
}

static int loopback_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct loopback_pcm *dpcm = runtime->private_data;

	if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
		dpcm->cable->playback_valid = 0;
		dpcm->cable->playback_running = 0;
		dpcm->cable->playback = NULL;
	} else {
		dpcm->cable->capture_valid = 0;
		dpcm->cable->capture_running = 0;
		dpcm->cable->capture = NULL;
	}
	return 0;
}

static struct snd_pcm_ops loopback_playback_ops = {
	.open =			loopback_open,
	.close =		loopback_close,
	.ioctl =		snd_pcm_lib_ioctl,
	.hw_params =		loopback_hw_params,
	.hw_free =		loopback_hw_free,
	.prepare =		loopback_prepare,
	.trigger =		loopback_playback_trigger,
	.pointer =		loopback_pointer,
};

static struct snd_pcm_ops loopback_capture_ops = {
	.open =			loopback_open,
	.close =		loopback_close,
	.ioctl =		snd_pcm_lib_ioctl,
	.hw_params =		loopback_hw_params,
	.hw_free =		loopback_hw_free,
	.prepare =		loopback_prepare,
	.trigger =		loopback_capture_trigger,
	.pointer =		loopback_pointer,
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
	int i, s;

	snd_power_change_state(card, SNDRV_CTL_POWER_D3hot);

	for (i = 0; i < MAX_PCM_SUBSTREAMS; i++) {
		for (s = 0; s < 2; s++) {
			snd_pcm_suspend(loopback->cables[i][s].playback);
			snd_pcm_suspend(loopback->cables[i][s].capture);
		}
	}
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
