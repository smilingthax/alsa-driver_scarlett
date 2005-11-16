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
#include <linux/input.h>

#include <linux/interrupt.h>
#include <linux/timex.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/bitops.h>
#include <asm/i8253.h>
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
static int no_test_speed = 0;

module_param(id, charp, 0444);
MODULE_PARM_DESC(id, "ID string for pcsp soundcard.");
module_param(enable, bool, 0444);
MODULE_PARM_DESC(enable, "dummy");
module_param(no_test_speed, int, 0444);
MODULE_PARM_DESC(no_test_speed, "dont test the CPU speed on startup");

static snd_card_t *snd_pcsp_card = SNDRV_DEFAULT_PTR1;
static pcsp_t *snd_pcsp_chip = NULL;

#ifdef CONFIG_APM_CPU_IDLE
static void (*saved_pm_idle)(void);
#endif

static char *pcsp_input_name = "pcsp-input";

static int (*pcsp_timer_func)(chip_t *chip) = NULL;
void *pcsp_timer_hook;

static void pcsp_input_event(struct input_handle *handle, unsigned int event_type, 
		      unsigned int event_code, int value)
{
}

static struct input_handle *pcsp_input_connect(struct input_handler *handler, 
					struct input_dev *dev,
					struct input_device_id *id)
{
	struct input_handle *handle;

	if (!(handle = kmalloc(sizeof(struct input_handle), GFP_KERNEL))) 
		return NULL;
	memset(handle, 0, sizeof(struct input_handle));

	handle->dev = dev;
	handle->handler = handler;
	handle->name = pcsp_input_name;

	input_open_device(handle);

	if (pcsp_timer_func)
		input_event(dev, EV_SND, SND_SILENT, 1);

	return handle;
}

static void pcsp_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	kfree(handle);
}

static struct input_device_id pcsp_input_ids[] = {
	{
                .flags = INPUT_DEVICE_ID_MATCH_EVBIT | INPUT_DEVICE_ID_MATCH_SNDBIT,
                .evbit = { BIT(EV_SND) },
                .sndbit = { BIT(SND_SILENT) },
        },	

	{ },    /* Terminating entry */
};

static struct input_handler pcsp_input_handler = {
	.event		= pcsp_input_event,
	.connect	= pcsp_input_connect,
	.disconnect	= pcsp_input_disconnect,
	.name		= "pcsp-input",
	.id_table	= pcsp_input_ids,
};

void pcsp_lock_input(int lock)
{
	struct list_head * node;
	list_for_each(node, &pcsp_input_handler.h_list) {
		input_event(to_handle_h(node)->dev, EV_SND, SND_SILENT, lock);
	}
}

/*
 * this is the PCSP IRQ handler
 */
static int pcsp_timer(struct pt_regs *regs)
{
	if (pcsp_timer_func)
		return pcsp_timer_func(snd_pcsp_chip);
	return 0;
}

int pcsp_set_timer_hook(chip_t *chip, int (*func)(chip_t *chip))
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

void pcsp_release_timer_hook(chip_t *chip)
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
static int __init pcsp_test_intspeed(chip_t *chip)
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
static int __init pcsp_measurement(chip_t *chip, int addon, int *rticks)
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

static int __init pcsp_test_speed(chip_t *chip, int *rmin_div)
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

static int snd_pcsp_free(pcsp_t *chip)
{
	unregister_timer_hook(pcsp_timer_hook);
	kfree(chip);
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
		.dev_free = snd_pcsp_dev_free,
	};
	pcsp_t *chip;
	int err;
	int div, min_div, order;
	int pcsp_enabled = 1;

	chip = kcalloc(1, sizeof(pcsp_t), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;
	spin_lock_init(&chip->lock);

	chip->card = card;
	chip->port = 0x61;
	chip->irq = TIMER_IRQ;
	chip->dma = -1;

	snd_pcsp_chip = chip;
	if (!(pcsp_timer_hook = register_timer_hook(pcsp_timer))) {
		printk(KERN_WARNING "PCSP could not register timer hook!");
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
	chip->enable       = 1;

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

	input_register_handler(&pcsp_input_handler);

	return 0;
}

static void __exit alsa_card_pcsp_exit(void)
{
	input_unregister_handler(&pcsp_input_handler);
	snd_card_free(snd_pcsp_card);
}

module_init(alsa_card_pcsp_init);
module_exit(alsa_card_pcsp_exit);
