/*
 * PC-Speaker driver for Linux
 *
 * Copyright (C) 1997-2001  David Woodhouse
 * Copyright (C) 2001-2004  Stas Sergeev
 */

#include <linux/config.h>
#include <sound/driver.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/pcm.h>

#include <linux/interrupt.h>
#include <linux/timex.h>
#include <linux/kd.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/bitops.h>
#ifdef CONFIG_APM_CPU_IDLE
#include <linux/pm.h>
#endif
#include "pcsp_defs.h"

MODULE_AUTHOR("Stas Sergeev <stsp@users.sourceforge.net>");
MODULE_DESCRIPTION("PC-Speaker driver");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("{{PC-Speaker, pcsp}}");

static int index = SNDRV_DEFAULT_IDX1;	/* Index 0-MAX */
static char *id = SNDRV_DEFAULT_STR1;	/* ID for this card */
static int enable = SNDRV_DEFAULT_ENABLE1;	/* Enable this card */
static int no_test_speed = 0, no_beeps = 0;

module_param(id, charp, 0444);
MODULE_PARM_DESC(id, "ID string for pcsp soundcard.");
module_param(enable, bool, 0444);
MODULE_PARM_DESC(enable, "dummy");
module_param(no_test_speed, int, 0444);
MODULE_PARM_DESC(no_test_speed, "dont test the CPU speed on startup");
module_param(no_beeps, int, 0444);
MODULE_PARM_DESC(no_beeps, "disable pc-speaker beeps");

snd_card_t *snd_pcsp_card = SNDRV_DEFAULT_PTR1;

#ifdef CONFIG_APM_CPU_IDLE
static void (*saved_pm_idle)(void);
#endif

static int (*pcsp_IRQ)(chip_t *chip) = NULL;

/*
 * this is the PCSP IRQ handler
 */
static irqreturn_t pcsp_run_IRQ(int irq, void *dev_id, struct pt_regs *regs)
{
	pcsp_t *chip = dev_id;
	int ret = IRQ_NONE;
	if (pcsp_IRQ) {
		ret = IRQ_HANDLED;
		if (!pcsp_IRQ(chip))
			ret |= IRQ_DONE;
	}
	return ret;
}

/*
 * Set the function func to be executed as the timer int.
 * if func returns a 0, the old IRQ0-handler(s) is called
 */
void pcsp_set_irq(chip_t *chip, int (*func)(chip_t *chip))
{
	unsigned long flags;
	if (!func)
		return;
	spin_lock_irqsave(&chip->lock, flags);
	pcsp_IRQ = func;
#if defined(CONFIG_INPUT_PCSPKR) || defined(CONFIG_INPUT_PCSPKR_MODULE)
	use_speaker_beep = 0;
#endif
#ifdef CONFIG_APM_CPU_IDLE
	saved_pm_idle = pm_idle;
	pm_idle = NULL;
#endif
	spin_unlock_irqrestore(&chip->lock, flags);
}

/*
 * reset the IRQ0 to the old handling
 */
void pcsp_release_irq(chip_t *chip)
{
	unsigned long flags;
	spin_lock_irqsave(&chip->lock, flags);
	pcsp_IRQ = NULL;
#if defined(CONFIG_INPUT_PCSPKR) || defined(CONFIG_INPUT_PCSPKR_MODULE)
	use_speaker_beep = !no_beeps;
#endif
#ifdef CONFIG_APM_CPU_IDLE
	pm_idle = saved_pm_idle;
#endif
	spin_unlock_irqrestore(&chip->lock, flags);
}

static int tst_len[2];
volatile static int pcsp_test_running;
/*
   this is a stupid beep which occurs if PCSP is disabled;
   it's not needed because we have the message, but who reads it...
   and this is the PC-Speaker driver :-)
*/
void __init pcsp_beep(int count, int cycles)
{
	unsigned long flags;
	spin_lock_irqsave(&i8253_lock, flags);
	/* enable counter 2 */
	outb_p(inb_p(0x61)|3, 0x61);
	/* set command for counter 2, 2 byte write */
	outb_p(0xB6, 0x43);
	/* select desired HZ */
	outb_p(count & 0xff, 0x42);
	outb((count >> 8) & 0xff, 0x42);

	while (cycles--);
		 
	/* disable counter 2 */
	outb(inb_p(0x61)&0xFC, 0x61);
	spin_unlock_irqrestore(&i8253_lock, flags);
}

/*
   the timer-int for testing cpu-speed, mostly the same as
   for PC-Speaker
 */
static int __init pcsp_test_intspeed(chip_t *chip)
{
	unsigned char test_buffer[256];
	if (chip->index < tst_len[chip->cur_buf]) {
		spin_lock(&i8253_lock);
		outb(chip->e,     0x61);
		outb(test_buffer[chip->index], 0x42);
		outb(chip->e ^ 1, 0x61);
		spin_unlock(&i8253_lock);

		chip->index += 2;
	}
	if (chip->index >= tst_len[chip->cur_buf]) {
		chip->index = 0;
		tst_len[chip->cur_buf] = 0;
		chip->cur_buf ^= 1;
		if (tst_len[chip->cur_buf] == 0xFFFF)
			chip->cur_buf ^= 1;
	}

	++pcsp_test_running;
	return 1;
}

/*
   this routine measures the time needed for one timer-int if
   we play thru PC-Speaker. This is kind of ugly but does the
   trick.
 */
static int __init pcsp_measurement(chip_t *chip, int addon)
{
	int count;
	unsigned long flags;

	chip->index	  = 0;
	tst_len[0]        = 5 + addon;
	tst_len[1] 	  = 0;
	chip->cur_buf     = 0;
	chip->e 	  = inb(0x61) & 0xFC;

	pcsp_test_running = 0;

	pcsp_set_irq(chip, pcsp_test_intspeed);
	/*
	  Perhaps we need some sort of timeout here, but if IRQ0
	  isn't working the system hangs later ...
	*/
	while (pcsp_test_running < 5);
	pcsp_release_irq(chip);

	spin_lock_irqsave(&i8253_lock, flags);
	outb_p(0x00, 0x43);		/* latch the count ASAP */
	count = inb_p(0x40);		/* read the latched count */
	count |= inb(0x40) << 8;
	spin_unlock_irqrestore(&i8253_lock, flags);

	return (LATCH - count);
}

static int __init pcsp_test_speed(chip_t *chip, int *rmin_div)
{
	int worst, worst1, best, best1, min_div;

	worst  = pcsp_measurement(chip, 0);
	worst1 = pcsp_measurement(chip, 0);
	best   = pcsp_measurement(chip, 5);
	best1  = pcsp_measurement(chip, 5);

	worst = max(worst, worst1);
	best  = min(best, best1);

#if PCSP_DEBUG
	printk(KERN_INFO "PCSP-Timerint needs %d Ticks in worst case\n", worst);
	printk(KERN_INFO "PCSP-Timerint needs %d Ticks in best case\n", best);
#endif
	/* We allow a CPU-usage of 80 % for the best-case ! */
	*rmin_div = min_div = best * 10 / 8;
	printk(KERN_INFO "PCSP: Measurement: maximal modulation freq %d Hz.\n",
		CLOCK_TICK_RATE / min_div);

	if (min_div > MAX_DIV) {
		printk(KERN_WARNING "This is too SLOW! PCSP-driver DISABLED\n");
		/* very ugly beep, but you hopefully never hear it */
		pcsp_beep(12000,800000);
		pcsp_beep(10000,800000);
		return 0;
	}
	return 1;
}

static int snd_pcsp_free(pcsp_t *chip)
{
	free_irq(chip->irq, chip);
	kfree(chip);
#if defined(CONFIG_INPUT_PCSPKR) || defined(CONFIG_INPUT_PCSPKR_MODULE)
	use_speaker_beep = 1;
#endif
	return 0;
}

static int snd_pcsp_dev_free(snd_device_t *device)
{
	pcsp_t *chip = device->device_data;
	return snd_pcsp_free(chip);
}

static int __init snd_pcsp_create(snd_card_t *card, pcsp_t **rchip)
{
	static snd_device_ops_t ops = {
		.dev_free =	snd_pcsp_dev_free,
	};
	pcsp_t *chip;
	int err;
	int div, min_div, order;
	int pcsp_enabled = 1;

	chip = kzalloc(sizeof(pcsp_t), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;
	spin_lock_init(&chip->lock);

	chip->card = card;
	chip->port = 0x61;
	chip->irq = TIMER_IRQ;
	chip->dma = -1;

	if (request_irq(chip->irq, pcsp_run_IRQ,
			SA_INTERRUPT | SA_SHIRQ | SA_FIRST,
			"pcsp", chip)) {
		printk(KERN_WARNING "PCSP could not modify timer IRQ!");
		snd_pcsp_free(chip);
		return -EBUSY;
	}

	if (!no_test_speed) {
		pcsp_enabled = pcsp_test_speed(chip, &min_div);
	} else {
		min_div = MAX_DIV;
	}
	if (!pcsp_enabled) {
		snd_pcsp_free(chip);
		return -EIO;
	}

	div = MAX_DIV / min_div;
	order = fls(div) - 1;

	chip->max_treble   = min(order, PCSP_MAX_POSS_TREBLE);
	chip->treble       = min(chip->max_treble, 1);
	chip->gain         = PCSP_DEFAULT_GAIN;
	chip->index	   = 0;
	chip->bass         = 0;
	chip->cur_buf	   = 0;
	chip->timer_active = 0;
	chip->last_clocks  =
	chip->timer_latch  =
	chip->clockticks   = LATCH;
	chip->volume       = PCSP_MAX_VOLUME;

	/* Register device */
	if ((err = snd_device_new(card, SNDRV_DEV_LOWLEVEL, chip, &ops)) < 0) {
		snd_pcsp_free(chip);
		return err;
	}

	*rchip = chip;
	return 0;
}

static int __init snd_card_pcsp_probe(int dev)
{
	snd_card_t *card;
	pcsp_t *chip;
	int err;

	if (dev != 0)
		return -EINVAL;

	card = snd_card_new(index, id, THIS_MODULE, 0);
	if (card == NULL)
		return -ENOMEM;

	if ((err = snd_pcsp_create(card, &chip)) < 0) {
		snd_card_free(card);
		return err;
	}
	if ((err = snd_pcsp_new_pcm(chip)) < 0) {
		snd_card_free(card);
		return err;
	}
	if ((err = snd_pcsp_new_mixer(chip)) < 0) {
		snd_card_free(card);
		return err;
	}

	strcpy(card->driver, "PC-Speaker");
	strcpy(card->shortname, chip->pcm->name);
	sprintf(card->longname, "Internal PC-Speaker at port 0x%x, irq %d",
	    chip->port, chip->irq);

	if ((err = snd_card_register(card)) < 0) {
		snd_card_free(card);
		return err;
	}
	snd_pcsp_card = card;

#if defined(CONFIG_INPUT_PCSPKR) || defined(CONFIG_INPUT_PCSPKR_MODULE)
	use_speaker_beep = !no_beeps;
#endif
	return 0;
}

static int __init alsa_card_pcsp_init(void)
{
	int dev = 0, cards = 0;

#ifdef CONFIG_DEBUG_PAGEALLOC
	/* Well, CONFIG_DEBUG_PAGEALLOC makes the sound horrible. Lets alert */
	printk(KERN_WARNING "PCSP: Warning, CONFIG_DEBUG_PAGEALLOC is enabled!\n"
		"You have to disable it if you want to use the PC-Speaker driver.\n"
		"Unless it is disabled, enjoy the horrible, distorted and crackling "
		"noise.\n");
#endif

	if (enable) {
		if (snd_card_pcsp_probe(dev) >= 0)
			cards++;
	}
	if (!cards) {
#ifdef MODULE
		printk(KERN_ERR "PC-Speaker soundcard not found or device busy\n");
#endif
		return -ENODEV;
	}
	return 0;
}

static void __exit alsa_card_pcsp_exit(void)
{

	snd_card_free(snd_pcsp_card);
}

module_init(alsa_card_pcsp_init);
module_exit(alsa_card_pcsp_exit);
