
#ifndef __OPTi9XX_H
#define __OPTi9XX_H

/*
    opti9xx.h - definitions for OPTi 82c9xx chips.
    Copyright (C) 1998-99 by Massimo Piccioni <dafastidio@libero.it>

    Part of this code was developed at the Italian Ministry of Air Defence,
    Sixth Division (oh, che pace ...), Rome.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#define OPTi9XX_HW_DETECT	0
#define OPTi9XX_HW_82C928	1
#define OPTi9XX_HW_82C929	2
#define OPTi9XX_HW_82C924	3
#define OPTi9XX_HW_82C925	4
#define OPTi9XX_HW_82C930	5
#define OPTi9XX_HW_82C931	6
#define OPTi9XX_HW_82C933	7
#define OPTi9XX_HW_LAST		OPTi9XX_HW_82C933

#define OPTi9XX_MC_REG(n)	n

struct snd_stru_opti9xx {
	unsigned short hardware;
	unsigned char password;
	char name[7];

	unsigned short mc_base;
#ifdef OPTi93X
	unsigned short mc_indir_index;
#endif	/* OPTi93X */
	unsigned short pwd_reg;
	spinlock_t lock;

	short wss_base;
	short irq;
	short dma1;
#if defined(CS4231) || defined(OPTi93X)
	short dma2;
#endif	/* CS4231 || OPTi93X */
	short fm_port;
	short mpu_port;
	short mpu_irq;
};
typedef struct snd_stru_opti9xx opti9xx_t;

#define snd_opti9xx_printk(args...)	snd_printk(__FILE__": " ##args)


extern inline void snd_opti9xx_free(opti9xx_t *chip)
{
	snd_kfree(chip);
}

extern opti9xx_t *snd_opti9xx_new_device(unsigned short hardware) {
	opti9xx_t *chip;

	if (!(chip = (opti9xx_t *) snd_kcalloc(sizeof(opti9xx_t), GFP_KERNEL)))
		return NULL;

	chip->hardware = hardware;
	chip->lock = SPIN_LOCK_UNLOCKED;
	chip->wss_base = chip->irq = chip->dma1 = -1;
#if defined(CS4231) || defined (OPTi93X)
	chip->dma2 = -1;
#endif 	/* CS4231 || OPTi93X */
	chip->fm_port = chip->mpu_port = chip->mpu_irq = -1;
	switch (hardware) {
#ifndef OPTi93X
	case OPTi9XX_HW_82C928:
	case OPTi9XX_HW_82C929:
		strcpy(chip->name, 
			(hardware == OPTi9XX_HW_82C928) ? "82C928" : "82C929");
		chip->mc_base = 0xf8c;
		chip->password = (hardware == OPTi9XX_HW_82C928) ? 0xe2 : 0xe3;
		chip->pwd_reg = 3;
		break;
	case OPTi9XX_HW_82C924:
	case OPTi9XX_HW_82C925:
		strcpy(chip->name, 
			(hardware == OPTi9XX_HW_82C924) ? "82C924" : "82C925");
		chip->mc_base = 0xf8c;
		chip->password = 0xe5;
		chip->pwd_reg = 3;
		break;
#else
	case OPTi9XX_HW_82C930:
	case OPTi9XX_HW_82C931:
	case OPTi9XX_HW_82C933:
		strcpy(chip->name, 
			(hardware == OPTi9XX_HW_82C930) ? "82C930" :
			(hardware == OPTi9XX_HW_82C931) ? "82C931" : "82C933");
		chip->mc_base = (hardware == OPTi9XX_HW_82C930) ? 0xf8f : 0xf8d;
		chip->mc_indir_index = 0xe0e;
		chip->password = 0xe4;
		/* chip->pwd_reg = 0; */
		break;
#endif	/* OPTi93X */
	default:
		snd_opti9xx_printk("chip %d not supported.\n", hardware);
		snd_opti9xx_free(chip);
		chip = NULL;
	}
	return chip;
}

extern inline unsigned char snd_opti9xx_read(opti9xx_t *chip,
							unsigned char reg) {
	unsigned long flags;
	unsigned char retval = 0xff;

	spin_lock_irqsave(&chip->lock, flags);
	outb(chip->password, chip->mc_base + chip->pwd_reg);
	switch (chip->hardware) {
#ifndef OPTi93X
	case OPTi9XX_HW_82C924:
	case OPTi9XX_HW_82C925:
		if (reg > 7) {
			outb(reg, chip->mc_base + 8);
			outb(chip->password, chip->mc_base + chip->pwd_reg);
			retval = inb(chip->mc_base + 9);
			break;
		}
	case OPTi9XX_HW_82C928:
	case OPTi9XX_HW_82C929:
		retval = inb(chip->mc_base + reg);
		break;
#else
	case OPTi9XX_HW_82C930:
	case OPTi9XX_HW_82C931:
	case OPTi9XX_HW_82C933:
		outb(reg, chip->mc_indir_index);
		outb(chip->password, chip->mc_base + chip->pwd_reg);
		retval = inb(chip->mc_indir_index + 1);
		break;
#endif	/* OPTi93X */
	default:
		snd_opti9xx_printk("chip %d not supported.\n", chip->hardware);
	}
	spin_unlock_irqrestore(&chip->lock, flags);
	return retval;
}
	
extern inline void snd_opti9xx_write(opti9xx_t *chip, unsigned char reg,
							unsigned char value) {
	unsigned long flags;

	spin_lock_irqsave(&chip->lock, flags);
	outb(chip->password, chip->mc_base + chip->pwd_reg);
	switch (chip->hardware) {
#ifndef OPTi93X
	case OPTi9XX_HW_82C924:
	case OPTi9XX_HW_82C925:
		if (reg > 7) {
			outb(reg, chip->mc_base + 8);
			outb(chip->password, chip->mc_base + chip->pwd_reg);
			outb(value, chip->mc_base + 9);
			break;
		}
	case OPTi9XX_HW_82C928:
	case OPTi9XX_HW_82C929:
		outb(value, chip->mc_base + reg);
		break;
#else
	case OPTi9XX_HW_82C930:
	case OPTi9XX_HW_82C931:
	case OPTi9XX_HW_82C933:
		outb(reg, chip->mc_indir_index);
		outb(chip->password, chip->mc_base + chip->pwd_reg);
		outb(value, chip->mc_indir_index + 1);
		break;
#endif	/* OPTi93X */
	default:
		snd_opti9xx_printk("chip %d not supported.\n", chip->hardware);
	}
	spin_unlock_irqrestore(&chip->lock, flags);
}


#define snd_opti9xx_mask(chip, reg, value, mask)	\
	snd_opti9xx_write(chip, reg,			\
		(snd_opti9xx_read(chip, reg) & ~(mask)) | ((value) & (mask)))

extern int snd_opti9xx_configure(opti9xx_t *chip) {
	unsigned char wss_base_bits;
	unsigned char irq_bits;
	unsigned char dma_bits;
	unsigned char mpu_port_bits = 0;
	unsigned char mpu_irq_bits;
	unsigned long flags;

	switch (chip->hardware) {
#ifndef OPTi93X
	case OPTi9XX_HW_82C924:
		snd_opti9xx_mask(chip, OPTi9XX_MC_REG(4), 0xf0, 0xfc);
		snd_opti9xx_mask(chip, OPTi9XX_MC_REG(6), 0x02, 0x02);
	case OPTi9XX_HW_82C925:
		snd_opti9xx_mask(chip, OPTi9XX_MC_REG(1), 0x80, 0x80);
		snd_opti9xx_mask(chip, OPTi9XX_MC_REG(2), 0x00, 0x20);
		snd_opti9xx_mask(chip, OPTi9XX_MC_REG(3), 0xf0, 0xff);
#ifdef CS4231
		snd_opti9xx_mask(chip, OPTi9XX_MC_REG(5), 0x02, 0x02);
#else
		snd_opti9xx_mask(chip, OPTi9XX_MC_REG(5), 0x00, 0x02);
#endif	/* CS4231 */
		break;
	case OPTi9XX_HW_82C928:
	case OPTi9XX_HW_82C929:
		snd_opti9xx_mask(chip, OPTi9XX_MC_REG(1), 0x80, 0x80);
		snd_opti9xx_mask(chip, OPTi9XX_MC_REG(2), 0x00, 0x20);
		/* snd_opti9xx_mask(chip, OPTi9XX_MC_REG(3), 0xa2, 0xae); */
		snd_opti9xx_mask(chip, OPTi9XX_MC_REG(4), 0x00, 0x0c);
#ifdef CS4231
		snd_opti9xx_mask(chip, OPTi9XX_MC_REG(5), 0x02, 0x02);
#else
		snd_opti9xx_mask(chip, OPTi9XX_MC_REG(5), 0x00, 0x02);
#endif	/* CS4231 */
		break;
#else	/* OPTi93X */
	case OPTi9XX_HW_82C930:
	case OPTi9XX_HW_82C931:
	case OPTi9XX_HW_82C933:
		snd_opti9xx_mask(chip, OPTi9XX_MC_REG(6), 0x02, 0x03);
		snd_opti9xx_mask(chip, OPTi9XX_MC_REG(3), 0x00, 0xff);
		snd_opti9xx_mask(chip, OPTi9XX_MC_REG(4), 0x10 |
			(chip->hardware == OPTi9XX_HW_82C930 ? 0x00 : 0x04),
			0x34);
		snd_opti9xx_mask(chip, OPTi9XX_MC_REG(5), 0x20, 0xbf);
		break;
#endif	/* OPTi93X */
	default:
		snd_opti9xx_printk("chip %d not supported.\n", chip->hardware);
		return -EINVAL;
	}

	switch (chip->wss_base) {
	case 0x530:
		wss_base_bits = 0x00;
		break;
	case 0x604:
		wss_base_bits = 0x03;
		break;
	case 0xe80:
		wss_base_bits = 0x01;
		break;
	case 0xf40:
		wss_base_bits = 0x02;
		break;
	default:
		snd_opti9xx_printk("WSS port 0x%x not valid.\n",
			chip->wss_base);
		goto __skip_base;
	}
	snd_opti9xx_mask(chip, OPTi9XX_MC_REG(1), wss_base_bits << 4, 0x30);

__skip_base:
	switch (chip->irq) {
#ifdef OPTi93X
	case 5:
		irq_bits = 0x05;
		break;
#endif	/* OPTi93X */
	case 7:
		irq_bits = 0x01;
		break;
	case 9:
		irq_bits = 0x02;
		break;
	case 10:
		irq_bits = 0x03;
		break;
	case 11:
		irq_bits = 0x04;
		break;
	default:
		snd_opti9xx_printk("WSS irq # %d not valid.\n", chip->irq);
		goto __skip_resources;
	}
	switch (chip->dma1) {
	case 0:
		dma_bits = 0x01;
		break;
	case 1:
		dma_bits = 0x02;
		break;
	case 3:
		dma_bits = 0x03;
		break;
	default:
		snd_opti9xx_printk("WSS dma1 # %d not valid.\n", chip->dma1);
		goto __skip_resources;
	}
#if defined(CS4231) || defined(OPTi93X)
	if (chip->dma1 == chip->dma2) {
		snd_opti9xx_printk("don't want to share dmas.\n");
		return -EBUSY;
	}
	switch (chip->dma2) {
	case 0:
	case 1:
		break;
	default:
		snd_opti9xx_printk("WSS dma2 # %d not valid.\n", chip->dma2);
		goto __skip_resources;
	}
	dma_bits |= 0x04;
#endif	/* CS4231 || OPTi93X */
	spin_lock_irqsave(&chip->lock, flags);
	outb(irq_bits << 3 | dma_bits, chip->wss_base);
	spin_unlock_irqrestore(&chip->lock, flags);

__skip_resources:
	if (chip->hardware > OPTi9XX_HW_82C928) {
		switch (chip->mpu_port) {
		case -1:
			break;
		case 0x300:
			mpu_port_bits = 0x03;
			break;
		case 0x310:
			mpu_port_bits = 0x02;
			break;
		case 0x320:
			mpu_port_bits = 0x01;
			break;
		case 0x330:
			mpu_port_bits = 0x00;
			break;
		default:
			snd_opti9xx_printk("MPU-401 port 0x%x not valid.\n",
				chip->mpu_port);
			goto __skip_mpu;
		}
		switch (chip->mpu_irq) {
		case 5:
			mpu_irq_bits = 0x02;
			break;
		case 7:
			mpu_irq_bits = 0x03;
			break;
		case 9:
			mpu_irq_bits = 0x00;
			break;
		case 10:
			mpu_irq_bits = 0x01;
			break;
		default:
			snd_opti9xx_printk("MPU-401 irq # %d not valid.\n",
				chip->mpu_irq);
			goto __skip_mpu;
		}
		snd_opti9xx_mask(chip, OPTi9XX_MC_REG(6),
			(chip->mpu_port == -1) ? 0x00 :
				0x80 | mpu_port_bits << 5 | mpu_irq_bits << 3,
			0xf8);
	}
__skip_mpu:

	return 0;
}

#endif	/* __OPTi9XX_H */

