/*
 *  Copyright (c) by Jaroslav Kysela <perex@perex.cz>
 *  Routines for control of AD1848/AD1847/CS4248
 *
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

#define SNDRV_MAIN_OBJECT_FILE
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <sound/core.h>
#include <sound/ad1848.h>
#include <sound/control.h>
#include <sound/tlv.h>
#include <sound/pcm_params.h>

#include <asm/io.h>
#include <asm/dma.h>

MODULE_AUTHOR("Jaroslav Kysela <perex@perex.cz>");
MODULE_DESCRIPTION("Routines for control of AD1848/AD1847/CS4248");
MODULE_LICENSE("GPL");

#if 0
#define SNDRV_DEBUG_MCE
#endif

/*
 *  Some variables
 */

static unsigned char snd_ad1848_original_image[16] =
{
	0x00,			/* 00 - lic */
	0x00,			/* 01 - ric */
	0x9f,			/* 02 - la1ic */
	0x9f,			/* 03 - ra1ic */
	0x9f,			/* 04 - la2ic */
	0x9f,			/* 05 - ra2ic */
	0xbf,			/* 06 - loc */
	0xbf,			/* 07 - roc */
	0x20,			/* 08 - dfr */
	AD1848_AUTOCALIB,	/* 09 - ic */
	0x00,			/* 0a - pc */
	0x00,			/* 0b - ti */
	0x00,			/* 0c - mi */
	0x00,			/* 0d - lbc */
	0x00,			/* 0e - dru */
	0x00,			/* 0f - drl */
};

/*
 *  Basic I/O functions
 */

static void snd_ad1848_wait(struct snd_wss *chip)
{
	int timeout;

	for (timeout = 250; timeout > 0; timeout--) {
		if ((inb(chip->port + CS4231P(REGSEL)) & AD1848_INIT) == 0)
			break;
		udelay(100);
	}
}

void snd_ad1848_out(struct snd_wss *chip,
			   unsigned char reg,
			   unsigned char value)
{
	snd_ad1848_wait(chip);
#ifdef CONFIG_SND_DEBUG
	if (inb(chip->port + CS4231P(REGSEL)) & AD1848_INIT)
		snd_printk(KERN_WARNING "auto calibration time out - "
			   "reg = 0x%x, value = 0x%x\n", reg, value);
#endif
	outb(chip->mce_bit | reg, chip->port + CS4231P(REGSEL));
	outb(chip->image[reg] = value, chip->port + CS4231P(REG));
	mb();
	snd_printdd("codec out - reg 0x%x = 0x%x\n",
			chip->mce_bit | reg, value);
}

EXPORT_SYMBOL(snd_ad1848_out);

static unsigned char snd_ad1848_in(struct snd_wss *chip, unsigned char reg)
{
	snd_ad1848_wait(chip);
#ifdef CONFIG_SND_DEBUG
	if (inb(chip->port + CS4231P(REGSEL)) & AD1848_INIT)
		snd_printk(KERN_WARNING "auto calibration time out - "
			   "reg = 0x%x\n", reg);
#endif
	outb(chip->mce_bit | reg, chip->port + CS4231P(REGSEL));
	mb();
	return inb(chip->port + CS4231P(REG));
}

#if 0

static void snd_ad1848_debug(struct snd_wss *chip)
{
	printk(KERN_DEBUG "AD1848 REGS:      INDEX = 0x%02x  ", inb(chip->port + CS4231P(REGSEL)));
	printk(KERN_DEBUG "                 STATUS = 0x%02x\n", inb(chip->port + CS4231P(STATUS)));
	printk(KERN_DEBUG "  0x00: left input      = 0x%02x  ", snd_ad1848_in(chip, 0x00));
	printk(KERN_DEBUG "  0x08: playback format = 0x%02x\n", snd_ad1848_in(chip, 0x08));
	printk(KERN_DEBUG "  0x01: right input     = 0x%02x  ", snd_ad1848_in(chip, 0x01));
	printk(KERN_DEBUG "  0x09: iface (CFIG 1)  = 0x%02x\n", snd_ad1848_in(chip, 0x09));
	printk(KERN_DEBUG "  0x02: AUXA left       = 0x%02x  ", snd_ad1848_in(chip, 0x02));
	printk(KERN_DEBUG "  0x0a: pin control     = 0x%02x\n", snd_ad1848_in(chip, 0x0a));
	printk(KERN_DEBUG "  0x03: AUXA right      = 0x%02x  ", snd_ad1848_in(chip, 0x03));
	printk(KERN_DEBUG "  0x0b: init & status   = 0x%02x\n", snd_ad1848_in(chip, 0x0b));
	printk(KERN_DEBUG "  0x04: AUXB left       = 0x%02x  ", snd_ad1848_in(chip, 0x04));
	printk(KERN_DEBUG "  0x0c: revision & mode = 0x%02x\n", snd_ad1848_in(chip, 0x0c));
	printk(KERN_DEBUG "  0x05: AUXB right      = 0x%02x  ", snd_ad1848_in(chip, 0x05));
	printk(KERN_DEBUG "  0x0d: loopback        = 0x%02x\n", snd_ad1848_in(chip, 0x0d));
	printk(KERN_DEBUG "  0x06: left output     = 0x%02x  ", snd_ad1848_in(chip, 0x06));
	printk(KERN_DEBUG "  0x0e: data upr count  = 0x%02x\n", snd_ad1848_in(chip, 0x0e));
	printk(KERN_DEBUG "  0x07: right output    = 0x%02x  ", snd_ad1848_in(chip, 0x07));
	printk(KERN_DEBUG "  0x0f: data lwr count  = 0x%02x\n", snd_ad1848_in(chip, 0x0f));
}

#endif

/*
 *  AD1848 detection / MCE routines
 */

static void snd_ad1848_mce_up(struct snd_wss *chip)
{
	unsigned long flags;
	int timeout;

	snd_ad1848_wait(chip);
#ifdef CONFIG_SND_DEBUG
	if (inb(chip->port + CS4231P(REGSEL)) & AD1848_INIT)
		snd_printk(KERN_WARNING "mce_up - auto calibration time out (0)\n");
#endif
	spin_lock_irqsave(&chip->reg_lock, flags);
	chip->mce_bit |= AD1848_MCE;
	timeout = inb(chip->port + CS4231P(REGSEL));
	if (timeout == 0x80)
		snd_printk(KERN_WARNING "mce_up [0x%lx]: serious init problem - codec still busy\n", chip->port);
	if (!(timeout & AD1848_MCE))
		outb(chip->mce_bit | (timeout & 0x1f),
		     chip->port + CS4231P(REGSEL));
	spin_unlock_irqrestore(&chip->reg_lock, flags);
}

static void snd_ad1848_mce_down(struct snd_wss *chip)
{
	unsigned long flags, timeout;
	int reg;

	spin_lock_irqsave(&chip->reg_lock, flags);
	for (timeout = 5; timeout > 0; timeout--)
		inb(chip->port + CS4231P(REGSEL));
	/* end of cleanup sequence */
	for (timeout = 12000;
	     timeout > 0 && (inb(chip->port + CS4231P(REGSEL)) & AD1848_INIT);
	     timeout--)
		udelay(100);

	snd_printdd("(1) timeout = %ld\n", timeout);

#ifdef CONFIG_SND_DEBUG
	if (inb(chip->port + CS4231P(REGSEL)) & AD1848_INIT)
		snd_printk(KERN_WARNING
			   "mce_down [0x%lx] - auto calibration time out (0)\n",
			   chip->port + CS4231P(REGSEL));
#endif

	chip->mce_bit &= ~AD1848_MCE;
	reg = inb(chip->port + CS4231P(REGSEL));
	outb(chip->mce_bit | (reg & 0x1f), chip->port + CS4231P(REGSEL));
	if (reg == 0x80)
		snd_printk(KERN_WARNING "mce_down [0x%lx]: serious init problem - codec still busy\n", chip->port);
	if ((reg & AD1848_MCE) == 0) {
		spin_unlock_irqrestore(&chip->reg_lock, flags);
		return;
	}

	/*
	 * Wait for auto-calibration (AC) process to finish, i.e. ACI to go low.
	 * It may take up to 5 sample periods (at most 907 us @ 5.5125 kHz) for
	 * the process to _start_, so it is important to wait at least that long
	 * before checking.  Otherwise we might think AC has finished when it
	 * has in fact not begun.  It could take 128 (no AC) or 384 (AC) cycles
	 * for ACI to drop.  This gives a wait of at most 70 ms with a more
	 * typical value of 3-9 ms.
	 */
	timeout = jiffies + msecs_to_jiffies(250);
	do {
		spin_unlock_irqrestore(&chip->reg_lock, flags);
		msleep(1);
		spin_lock_irqsave(&chip->reg_lock, flags);
		reg = snd_ad1848_in(chip, AD1848_TEST_INIT) &
		      AD1848_CALIB_IN_PROGRESS;
	} while (reg && time_before(jiffies, timeout));
	spin_unlock_irqrestore(&chip->reg_lock, flags);
	if (reg)
		snd_printk(KERN_ERR
			   "mce_down - auto calibration time out (2)\n");

	snd_printdd("(4) jiffies = %lu\n", jiffies);
	snd_printd("mce_down - exit = 0x%x\n",
		   inb(chip->port + CS4231P(REGSEL)));
}

static irqreturn_t snd_ad1848_interrupt(int irq, void *dev_id)
{
	struct snd_wss *chip = dev_id;

	if ((chip->mode & WSS_MODE_PLAY) && chip->playback_substream)
		snd_pcm_period_elapsed(chip->playback_substream);
	if ((chip->mode & WSS_MODE_RECORD) && chip->capture_substream)
		snd_pcm_period_elapsed(chip->capture_substream);
	outb(0, chip->port + CS4231P(STATUS));	/* clear global interrupt bit */
	return IRQ_HANDLED;
}

/*

 */

static void snd_ad1848_thinkpad_twiddle(struct snd_wss *chip, int on)
{
	int tmp;

	if (!chip->thinkpad_flag) return;

	outb(0x1c, AD1848_THINKPAD_CTL_PORT1);
	tmp = inb(AD1848_THINKPAD_CTL_PORT2);

	if (on)
		/* turn it on */
		tmp |= AD1848_THINKPAD_CS4248_ENABLE_BIT;
	else
		/* turn it off */
		tmp &= ~AD1848_THINKPAD_CS4248_ENABLE_BIT;
	
	outb(tmp, AD1848_THINKPAD_CTL_PORT2);

}

#ifdef CONFIG_PM
static void snd_ad1848_suspend(struct snd_wss *chip)
{
	snd_pcm_suspend_all(chip->pcm);
	if (chip->thinkpad_flag)
		snd_ad1848_thinkpad_twiddle(chip, 0);
}

static void snd_ad1848_resume(struct snd_wss *chip)
{
	int i;

	if (chip->thinkpad_flag)
		snd_ad1848_thinkpad_twiddle(chip, 1);

	/* clear any pendings IRQ */
	inb(chip->port + CS4231P(STATUS));
	outb(0, chip->port + CS4231P(STATUS));
	mb();

	snd_ad1848_mce_down(chip);
	for (i = 0; i < 16; i++)
		snd_ad1848_out(chip, i, chip->image[i]);
	snd_ad1848_mce_up(chip);
	snd_ad1848_mce_down(chip);
}
#endif /* CONFIG_PM */

static int snd_ad1848_probe(struct snd_wss *chip)
{
	unsigned long flags;
	int i, id, rev, ad1847;
	unsigned char *ptr;

#if 0
	snd_ad1848_debug(chip);
#endif
	id = ad1847 = 0;
	for (i = 0; i < 1000; i++) {
		mb();
		if (inb(chip->port + CS4231P(REGSEL)) & AD1848_INIT)
			udelay(500);
		else {
			spin_lock_irqsave(&chip->reg_lock, flags);
			snd_ad1848_out(chip, AD1848_MISC_INFO, 0x00);
			snd_ad1848_out(chip, AD1848_LEFT_INPUT, 0xaa);
			snd_ad1848_out(chip, AD1848_RIGHT_INPUT, 0x45);
			rev = snd_ad1848_in(chip, AD1848_RIGHT_INPUT);
			if (rev == 0x65) {
				spin_unlock_irqrestore(&chip->reg_lock, flags);
				id = 1;
				ad1847 = 1;
				break;
			}
			if (snd_ad1848_in(chip, AD1848_LEFT_INPUT) == 0xaa && rev == 0x45) {
				spin_unlock_irqrestore(&chip->reg_lock, flags);
				id = 1;
				break;
			}
			spin_unlock_irqrestore(&chip->reg_lock, flags);
		}
	}
	if (id != 1)
		return -ENODEV;	/* no valid device found */
	if (chip->hardware == WSS_HW_DETECT) {
		if (ad1847) {
			chip->hardware = WSS_HW_AD1847;
		} else {
			chip->hardware = WSS_HW_AD1848;
			rev = snd_ad1848_in(chip, AD1848_MISC_INFO);
			if (rev & 0x80) {
				chip->hardware = WSS_HW_CS4248;
			} else if ((rev & 0x0f) == 0x0a) {
				snd_ad1848_out(chip, AD1848_MISC_INFO, 0x40);
				for (i = 0; i < 16; ++i) {
					if (snd_ad1848_in(chip, i) != snd_ad1848_in(chip, i + 16)) {
						chip->hardware = WSS_HW_CMI8330;
						break;
					}
				}
				snd_ad1848_out(chip, AD1848_MISC_INFO, 0x00);
			}
		}
	}
	spin_lock_irqsave(&chip->reg_lock, flags);
	inb(chip->port + CS4231P(STATUS));	/* clear any pendings IRQ */
	outb(0, chip->port + CS4231P(STATUS));
	mb();
	spin_unlock_irqrestore(&chip->reg_lock, flags);

	chip->image[AD1848_MISC_INFO] = 0x00;
	chip->image[AD1848_IFACE_CTRL] =
	    (chip->image[AD1848_IFACE_CTRL] & ~AD1848_SINGLE_DMA) | AD1848_SINGLE_DMA;
	ptr = (unsigned char *) &chip->image;
	snd_ad1848_mce_down(chip);
	spin_lock_irqsave(&chip->reg_lock, flags);
	for (i = 0; i < 16; i++)	/* ok.. fill all AD1848 registers */
		snd_ad1848_out(chip, i, *ptr++);
	spin_unlock_irqrestore(&chip->reg_lock, flags);
	snd_ad1848_mce_up(chip);
	/* init needed for WSS pcm */
	spin_lock_irqsave(&chip->reg_lock, flags);
	chip->image[AD1848_IFACE_CTRL] &= ~(AD1848_PLAYBACK_ENABLE |
				AD1848_PLAYBACK_PIO |
				AD1848_CAPTURE_ENABLE |
				AD1848_CAPTURE_PIO |
				AD1848_CALIB_MODE);
	chip->image[AD1848_IFACE_CTRL] |= AD1848_AUTOCALIB;
	snd_ad1848_out(chip, AD1848_IFACE_CTRL, chip->image[AD1848_IFACE_CTRL]);
	spin_unlock_irqrestore(&chip->reg_lock, flags);
	snd_ad1848_mce_down(chip);
	return 0;		/* all things are ok.. */
}

/*

 */

static int snd_ad1848_free(struct snd_wss *chip)
{
	release_and_free_resource(chip->res_port);
	if (chip->irq >= 0)
		free_irq(chip->irq, (void *) chip);
	if (chip->dma1 >= 0) {
		snd_dma_disable(chip->dma1);
		free_dma(chip->dma1);
	}
	kfree(chip);
	return 0;
}

static int snd_ad1848_dev_free(struct snd_device *device)
{
	struct snd_wss *chip = device->device_data;
	return snd_ad1848_free(chip);
}

int snd_ad1848_create(struct snd_card *card,
		      unsigned long port,
		      int irq, int dma,
		      unsigned short hardware,
		      struct snd_wss **rchip)
{
	static struct snd_device_ops ops = {
		.dev_free =	snd_ad1848_dev_free,
	};
	struct snd_wss *chip;
	int err;

	*rchip = NULL;
	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;
	spin_lock_init(&chip->reg_lock);
	chip->card = card;
	chip->port = port;
	chip->irq = -1;
	chip->dma1 = -1;
	chip->dma2 = -1;
	chip->single_dma = 1;
	chip->hardware = hardware;
	memcpy(&chip->image, &snd_ad1848_original_image, sizeof(snd_ad1848_original_image));
	
	if ((chip->res_port = request_region(port, 4, "AD1848")) == NULL) {
		snd_printk(KERN_ERR "ad1848: can't grab port 0x%lx\n", port);
		snd_ad1848_free(chip);
		return -EBUSY;
	}
	if (request_irq(irq, snd_ad1848_interrupt, IRQF_DISABLED, "AD1848", (void *) chip)) {
		snd_printk(KERN_ERR "ad1848: can't grab IRQ %d\n", irq);
		snd_ad1848_free(chip);
		return -EBUSY;
	}
	chip->irq = irq;
	if (request_dma(dma, "AD1848")) {
		snd_printk(KERN_ERR "ad1848: can't grab DMA %d\n", dma);
		snd_ad1848_free(chip);
		return -EBUSY;
	}
	chip->dma1 = dma;
	chip->dma2 = dma;

	if (hardware == WSS_HW_THINKPAD) {
		chip->thinkpad_flag = 1;
		chip->hardware = WSS_HW_DETECT; /* reset */
		snd_ad1848_thinkpad_twiddle(chip, 1);
	}

	if (snd_ad1848_probe(chip) < 0) {
		snd_ad1848_free(chip);
		return -ENODEV;
	}

	/* Register device */
	if ((err = snd_device_new(card, SNDRV_DEV_LOWLEVEL, chip, &ops)) < 0) {
		snd_ad1848_free(chip);
		return err;
	}

#ifdef CONFIG_PM
	chip->suspend = snd_ad1848_suspend;
	chip->resume = snd_ad1848_resume;
#endif

	*rchip = chip;
	return 0;
}

EXPORT_SYMBOL(snd_ad1848_create);

/*
 *  INIT part
 */

static int __init alsa_ad1848_init(void)
{
	return 0;
}

static void __exit alsa_ad1848_exit(void)
{
}

module_init(alsa_ad1848_init)
module_exit(alsa_ad1848_exit)
