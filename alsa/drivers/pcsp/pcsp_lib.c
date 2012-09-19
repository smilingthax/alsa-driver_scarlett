/*
 * PC-Speaker driver for Linux
 *
 * Copyright (C) 1993-1997  Michael Beck
 * Copyright (C) 1997-2001  David Woodhouse
 * Copyright (C) 2001-2004  Stas Sergeev
 */

#include <sound/driver.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <asm/i8253.h>
#include "pcsp.h"
#include "pcsp_tabs.h"

#define DMIX_WANTS_S16		1

/* the timer-int for playing thru PC-Speaker */
static int pcsp_do_timer(struct snd_pcsp *chip)
{
	struct snd_pcm_substream *substream = chip->playback_substream;
	struct snd_pcm_runtime *runtime = substream->runtime;

	if (chip->reset_timer) {
		outb_p(0x34, 0x43);	/* binary, mode 2, LSB/MSB, ch 0 */
		outb_p(PIT_COUNTER, 0x40);
		outb(PIT_COUNTER >> 8, 0x40);
		chip->timer_latch = PIT_COUNTER;
		chip->reset_timer = 0;
		printk(KERN_INFO "PCSP: rate set to %i\n",
			CLOCK_TICK_RATE / PIT_COUNTER);
	}
	if (chip->index < snd_pcm_lib_period_bytes(substream)) {
		unsigned char timer_cnt, val;
		int fmt_size = snd_pcm_format_physical_width(runtime->format) >> 3;
		/* assume it is mono! */
		val = PCSP_CUR_BUF[chip->index + fmt_size - 1];
		if (snd_pcm_format_signed(runtime->format))
			val ^= 0x80;
		timer_cnt = chip->vl_tab[val];
		if (timer_cnt && chip->enable) {
			outb_p(chip->e, 0x61);
			outb_p(timer_cnt, 0x42);
			outb(chip->e ^ 1, 0x61);
		}
		chip->index += PCSP_INDEX_INC * fmt_size;
	}
	if (chip->index >= snd_pcm_lib_period_bytes(substream)) {
		chip->cur_buf = ((chip->cur_buf + 1) % runtime->periods);
		chip->index -= snd_pcm_lib_period_bytes(substream);
		write_sequnlock(&xtime_lock);	// avoid recursive locking
		snd_pcm_period_elapsed(substream);
		write_seqlock(&xtime_lock);
	}

	chip->clockticks -= chip->timer_latch;
	pit_counter0_offset = chip->clockticks - chip->timer_latch;
	if (chip->clockticks < 0) {
		chip->clockticks += LATCH;
		pit_counter0_offset += LATCH;
		return 0;
	}
	return 1;
}

void pcsp_start_timer(struct snd_pcsp *chip)
{
	unsigned long flags;

	if (chip->timer_active) {
		printk(KERN_INFO "PCSP: Timer already active\n");
		return;
	}

	spin_lock_irqsave(&i8253_lock, flags);
	if (pcsp_set_timer_hook(chip, pcsp_do_timer)) {
		spin_unlock_irqrestore(&i8253_lock, flags);
		return;
	}
	chip->e = inb(0x61) | 0x03;
	outb_p(0x92, 0x43);	/* binary, mode 1, LSB only, ch 2 */
	outb_p(0x34, 0x43);	/* binary, mode 2, LSB/MSB, ch 0 */
	outb_p(PIT_COUNTER, 0x40);
	outb(PIT_COUNTER >> 8, 0x40);
	chip->timer_latch = PIT_COUNTER;
	chip->clockticks = chip->last_clocks;
	chip->reset_timer = 0;

	chip->timer_active = 1;
	spin_unlock_irqrestore(&i8253_lock, flags);
}

/* reset the timer to 100 Hz and reset old timer-int */
void pcsp_stop_timer(struct snd_pcsp *chip)
{
	unsigned long flags;
	if (!chip->timer_active)
		return;

	spin_lock_irqsave(&i8253_lock, flags);
	/* restore the timer */
	outb_p(0x34, 0x43);		/* binary, mode 2, LSB/MSB, ch 0 */
	outb_p(0xb6, 0x43);		/* binary, mode 3, LSB/MSB, ch 2 */
	outb_p(LATCH & 0xff, 0x40);	/* LSB */
	outb(LATCH >> 8, 0x40);		/* MSB */
	outb(chip->e & 0xFC, 0x61);

	/* clear clock tick counter */
	chip->last_clocks = chip->clockticks;
	chip->timer_latch = chip->clockticks = LATCH;
	pit_counter0_offset = 0;

	pcsp_release_timer_hook(chip);

	chip->timer_active = 0;
	spin_unlock_irqrestore(&i8253_lock, flags);
}

static int snd_pcsp_playback_close(struct snd_pcm_substream * substream)
{
	struct snd_pcsp *chip = snd_pcm_substream_chip(substream);
#if PCSP_DEBUG
	printk("close called\n");
#endif
	chip->playback_substream = NULL;
	if (chip->timer_active) {
		printk(KERN_INFO "PCSP: timer still active\n");
	}
	return 0;
}

static int snd_pcsp_playback_hw_params(struct snd_pcm_substream * substream,
				       struct snd_pcm_hw_params * hw_params)
{
	int err;
	if ((err = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params))) < 0)
		return err;
	return 0;
}

static int snd_pcsp_playback_hw_free(struct snd_pcm_substream * substream)
{
#if PCSP_DEBUG
	printk("hw_free called\n");
#endif
	return snd_pcm_lib_free_pages(substream);
}

static int snd_pcsp_playback_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcsp *chip = snd_pcm_substream_chip(substream);
#if PCSP_DEBUG
	printk("prepare called, size=%i psize=%i f=%i f1=%i\n",
		snd_pcm_lib_buffer_bytes(substream),
		snd_pcm_lib_period_bytes(substream),
		snd_pcm_lib_buffer_bytes(substream)/
		snd_pcm_lib_period_bytes(substream),
		substream->runtime->periods);
#endif
	chip->cur_buf = chip->index = 0;
	pcsp_calc_voltab(chip);
	return 0;
}

static int snd_pcsp_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcsp *chip = snd_pcm_substream_chip(substream);
#if PCSP_DEBUG
	printk("trigger called\n");
#endif
	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
			pcsp_start_timer(chip);
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
			pcsp_stop_timer(chip);
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

static snd_pcm_uframes_t snd_pcsp_playback_pointer(struct snd_pcm_substream * substream)
{
	struct snd_pcsp *chip = snd_pcm_substream_chip(substream);
	size_t ptr;
	ptr = chip->cur_buf * snd_pcm_lib_period_bytes(substream) + chip->index;
#if PCSP_DEBUG
	printk("pointer %i\n", ptr);
#endif
	return bytes_to_frames(substream->runtime, ptr);
}

static struct snd_pcm_hardware snd_pcsp_playback =
{
	.info =			(SNDRV_PCM_INFO_INTERLEAVED |
				 SNDRV_PCM_INFO_HALF_DUPLEX |
				 SNDRV_PCM_INFO_MMAP |
				 SNDRV_PCM_INFO_MMAP_VALID),
	.formats =		(SNDRV_PCM_FMTBIT_U8
#if DMIX_WANTS_S16
				| SNDRV_PCM_FMTBIT_S16_LE
#endif
				),
	.rates =		SNDRV_PCM_RATE_KNOT,
	.rate_min =		PCSP_DEFAULT_RATE,
	.rate_max =		PCSP_DEFAULT_RATE,
	.channels_min =		1,
	.channels_max =		1,
	.buffer_bytes_max =	PCSP_BUFFER_SIZE,
	.period_bytes_min =	64,
	.period_bytes_max =	PCSP_MAX_PERIOD_SIZE,
	.periods_min =		2,
	.periods_max =		PCSP_MAX_PERIODS,
	.fifo_size =		0,
};

static int snd_pcsp_playback_open(struct snd_pcm_substream * substream)
{
	struct snd_pcsp *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
#if PCSP_DEBUG
	printk("open called\n");
#endif
	if (chip->timer_active) {
		printk(KERN_INFO "PCSP: timer still active!!\n");
		pcsp_stop_timer(chip);
	}
	runtime->hw = snd_pcsp_playback;
	chip->playback_substream = substream;
	return 0;
}

static struct snd_pcm_ops snd_pcsp_playback_ops = {
	.open =		snd_pcsp_playback_open,
	.close =	snd_pcsp_playback_close,
	.ioctl =	snd_pcm_lib_ioctl,
	.hw_params =	snd_pcsp_playback_hw_params,
	.hw_free =	snd_pcsp_playback_hw_free,
	.prepare =	snd_pcsp_playback_prepare,
	.trigger =	snd_pcsp_trigger,
	.pointer =	snd_pcsp_playback_pointer,
};

int __init snd_pcsp_new_pcm(struct snd_pcsp *chip)
{
	struct snd_pcm *pcm;
	int err;

	if ((err = snd_pcm_new(chip->card, "pcspeaker", 0, 1, 0, &pcm)) < 0)
		return err;

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_pcsp_playback_ops);

	pcm->private_data = chip;
	pcm->info_flags = SNDRV_PCM_INFO_HALF_DUPLEX;
	strcpy(pcm->name, "pcsp");

	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_CONTINUOUS,
				      snd_dma_continuous_data(GFP_KERNEL),
				      PCSP_BUFFER_SIZE, PCSP_BUFFER_SIZE);

	chip->pcm = pcm;
	return 0;
}
