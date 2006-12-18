/*
 * PC-Speaker driver for Linux
 *
 * Copyright (C) 1997-2001  David Woodhouse
 * Copyright (C) 2001-2004  Stas Sergeev
 */

#include <sound/driver.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/pcm.h>

#include <linux/interrupt.h>
#include <linux/timex.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/bitops.h>
#include <asm/i8253.h>
#ifdef CONFIG_APM_CPU_IDLE
#include <linux/pm.h>
#endif
#include "pcsp.h"
#include "pcsp_input.h"

MODULE_AUTHOR("Stas Sergeev <stsp@users.sourceforge.net>");
MODULE_DESCRIPTION("PC-Speaker driver");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("{{PC-Speaker, pcsp}}");

static int index = SNDRV_DEFAULT_IDX1;	/* Index 0-MAX */
static char *id = SNDRV_DEFAULT_STR1;	/* ID for this card */
static int enable = SNDRV_DEFAULT_ENABLE1;	/* Enable this card */
static int no_test_speed = 0;

module_param(id, charp, 0444);
MODULE_PARM_DESC(id, "ID string for pcsp soundcard.");
module_param(enable, bool, 0444);
MODULE_PARM_DESC(enable, "dummy");
module_param(no_test_speed, int, 0444);
MODULE_PARM_DESC(no_test_speed, "dont test the CPU speed on startup");

struct snd_pcsp *snd_pcsp_chip = NULL;

#ifdef CONFIG_APM_CPU_IDLE
static void (*saved_pm_idle)(void);
#endif

static int (*pcsp_timer_func)(struct snd_pcsp *chip) = NULL;
void *pcsp_timer_hook;


/*
 * this is the PCSP IRQ handler
 */
static int pcsp_timer(void)
{
	if (pcsp_timer_func)
		return pcsp_timer_func(snd_pcsp_chip);
	return 0;
}

int pcsp_set_timer_hook(struct snd_pcsp *chip, int (*func)(struct snd_pcsp *chip))
{
	unsigned long flags;
	int err;
	if (!func)
		return -1;
	spin_lock_irqsave(&chip->lock, flags);
	if ((err = grab_timer_hook(pcsp_timer_hook))) {
		spin_unlock_irqrestore(&chip->lock, flags);
		printk(KERN_WARNING "PCSP: unable to grab timer!\n");
		return err;
	}
	pcsp_timer_func = func;
#ifdef CONFIG_APM_CPU_IDLE
	saved_pm_idle = pm_idle;
	pm_idle = NULL;
#endif
	spin_unlock_irqrestore(&chip->lock, flags);
	return 0;
}

void pcsp_release_timer_hook(struct snd_pcsp *chip)
{
	unsigned long flags;
	spin_lock_irqsave(&chip->lock, flags);
	pcsp_timer_func = NULL;
#ifdef CONFIG_APM_CPU_IDLE
	pm_idle = saved_pm_idle;
#endif
	ungrab_timer_hook(pcsp_timer_hook);
	spin_unlock_irqrestore(&chip->lock, flags);
}

static int tst_len[2];
volatile static int pcsp_test_running;

/*
   the timer-int for testing cpu-speed, mostly the same as
   for PC-Speaker
 */
static int pcsp_test_intspeed(struct snd_pcsp *chip)
{
	unsigned char test_buffer[256];
	if (chip->index < tst_len[chip->cur_buf]) {
		outb(chip->e,     0x61);
		outb(test_buffer[chip->index], 0x42);
		outb(chip->e ^ 1, 0x61);

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
static int pcsp_measurement(struct snd_pcsp *chip, int addon, int *rticks)
{
	int count, err;
	unsigned long flags;

	chip->index	  = 0;
	tst_len[0]        = 5 + addon;
	tst_len[1] 	  = 0;
	chip->cur_buf     = 0;
	chip->e 	  = inb(0x61) & 0xFC;

	pcsp_test_running = 0;

	if ((err = pcsp_set_timer_hook(chip, pcsp_test_intspeed)))
		return err;
	/*
	  Perhaps we need some sort of timeout here, but if IRQ0
	  isn't working the system hangs later ...
	*/
	while (pcsp_test_running < 5);
	pcsp_release_timer_hook(chip);

	spin_lock_irqsave(&i8253_lock, flags);
	outb_p(0x00, 0x43);		/* latch the count ASAP */
	count = inb_p(0x40);		/* read the latched count */
	count |= inb(0x40) << 8;
	spin_unlock_irqrestore(&i8253_lock, flags);

	*rticks = LATCH - count;
	return 0;
}

static int __init pcsp_test_speed(struct snd_pcsp *chip, int *rmin_div)
{
	int worst, worst1, best, best1, min_div;

	if (pcsp_measurement(chip, 0, &worst))
		return 0;
	if (pcsp_measurement(chip, 0, &worst1))
		return 0;
	if (pcsp_measurement(chip, 5, &best))
		return 0;
	if (pcsp_measurement(chip, 5, &best1))
		return 0;

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
		return 0;
	}
	return 1;
}

static int snd_pcsp_free(struct snd_pcsp *chip)
{
	unregister_timer_hook(pcsp_timer_hook);
	kfree(chip);
	return 0;
}

static int snd_pcsp_dev_free(struct snd_device *device)
{
	struct snd_pcsp *chip = device->device_data;
	return snd_pcsp_free(chip);
}

static int __init snd_pcsp_create(struct snd_card *card)
{
	static struct snd_device_ops ops = {
		.dev_free = snd_pcsp_dev_free,
	};
	int err;
	int div, min_div, order;
	int pcsp_enabled = 1;

	snd_pcsp_chip = kcalloc(1, sizeof(struct snd_pcsp), GFP_KERNEL);
	if (!snd_pcsp_chip)
		return -ENOMEM;
	spin_lock_init(&snd_pcsp_chip->lock);

	snd_pcsp_chip->card = card;
	snd_pcsp_chip->port = 0x61;
	snd_pcsp_chip->irq = TIMER_IRQ;
	snd_pcsp_chip->dma = -1;

	if (!(pcsp_timer_hook = register_timer_hook(pcsp_timer))) {
		printk(KERN_WARNING "PCSP could not register timer hook!");
		snd_pcsp_free(snd_pcsp_chip);
		return -EBUSY;
	}

	if (!no_test_speed) {
		pcsp_enabled = pcsp_test_speed(snd_pcsp_chip, &min_div);
	} else {
		min_div = MAX_DIV;
	}
	if (!pcsp_enabled) {
		snd_pcsp_free(snd_pcsp_chip);
		return -EIO;
	}

	div = MAX_DIV / min_div;
	order = fls(div) - 1;

	snd_pcsp_chip->max_treble   = min(order, PCSP_MAX_POSS_TREBLE);
	snd_pcsp_chip->treble       = min(snd_pcsp_chip->max_treble, 1);
	snd_pcsp_chip->gain         = PCSP_DEFAULT_GAIN;
	snd_pcsp_chip->index	    = 0;
	snd_pcsp_chip->bass         = 0;
	snd_pcsp_chip->cur_buf	    = 0;
	snd_pcsp_chip->timer_active = 0;
	snd_pcsp_chip->last_clocks  =
	snd_pcsp_chip->timer_latch  =
	snd_pcsp_chip->clockticks   = LATCH;
	snd_pcsp_chip->volume       = PCSP_MAX_VOLUME;
	snd_pcsp_chip->enable       = 1;
	snd_pcsp_chip->pcspkr       = 1;

	/* Register device */
	if ((err = snd_device_new(card, SNDRV_DEV_LOWLEVEL, snd_pcsp_chip, &ops)) < 0) {
		snd_pcsp_free(snd_pcsp_chip);
		return err;
	}

	return 0;
}

static int __init snd_card_pcsp_probe(int dev)
{
	struct snd_card *card;
	int err;

	if (dev != 0)
		return -EINVAL;

	card = snd_card_new(index, id, THIS_MODULE, 0);
	if (!card)
		return -ENOMEM;

	if ((err = snd_pcsp_create(card)) < 0) {
		snd_card_free(card);
		return err;
	}
	if ((err = snd_pcsp_new_pcm(snd_pcsp_chip)) < 0) {
		snd_card_free(card);
		return err;
	}
	if ((err = snd_pcsp_new_mixer(snd_pcsp_chip)) < 0) {
		snd_card_free(card);
		return err;
	}

	strcpy(card->driver, "PC-Speaker");
	strcpy(card->shortname, snd_pcsp_chip->pcm->name);
	sprintf(card->longname, "Internal PC-Speaker at port 0x%x, irq %d",
	    snd_pcsp_chip->port, snd_pcsp_chip->irq);

	if ((err = snd_card_register(card)) < 0) {
		snd_card_free(card);
		return err;
	}

	pcspkr_input_init(snd_pcsp_chip);

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
	pcspkr_input_remove(snd_pcsp_chip);
	snd_card_free(snd_pcsp_chip->card);
}

module_init(alsa_card_pcsp_init);
module_exit(alsa_card_pcsp_exit);
