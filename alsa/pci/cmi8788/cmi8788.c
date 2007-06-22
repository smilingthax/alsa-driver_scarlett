/*
 *  cmi8788.c - Driver for C-Media CMI8788 PCI soundcards.
 *
 *      Copyright (C) 2005  C-media support
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
 *  Revision history
 *
 *    Weifeng Sui <weifengsui@163.com>
 */

#include <sound/driver.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/control.h>
#include "cmi8788.h"


MODULE_AUTHOR("weifeng sui <weifengsui@163.com>");
MODULE_DESCRIPTION("C-Media CMI8788 PCI");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("{{C-Media,CMI8788}}");

static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;
static int enable[SNDRV_CARDS] = SNDRV_DEFAULT_ENABLE_PNP;

module_param_array(index, int, NULL, 0444);
MODULE_PARM_DESC(index, "Index value for C-Media PCI soundcard.");
module_param_array(id, charp, NULL, 0444);
MODULE_PARM_DESC(id, "ID string for C-Media PCI soundcard.");
module_param_array(enable, bool, NULL, 0444);
MODULE_PARM_DESC(enable, "Enable C-Media PCI soundcard.");

/* #define RECORD_LINE_IN */

/*
 * pci ids
 */
#ifndef PCI_VENDOR_ID_CMEDIA
#define PCI_VENDOR_ID_CMEDIA         0x13F6
#endif
#ifndef PCI_DEVICE_ID_CMEDIA_CM8788
#define PCI_DEVICE_ID_CMEDIA_CM8788  0x8788
#endif


/* read/write operations for dword register */
void snd_cmipci_write(struct cmi8788 *chip, unsigned int data, unsigned int cmd)
{
	outl(data, chip->addr + cmd);
}

unsigned int snd_cmipci_read(struct cmi8788 *chip, unsigned int cmd)
{
	return inl(chip->addr + cmd);
}

/* read/write operations for word register */
void snd_cmipci_write_w(struct cmi8788 *chip, unsigned short data, unsigned int cmd)
{
	outw(data, chip->addr + cmd);
}

unsigned short snd_cmipci_read_w(struct cmi8788 *chip, unsigned int cmd)
{
	return inw(chip->addr + cmd);
}

/* read/write operations for byte register */
void snd_cmipci_write_b(struct cmi8788 *chip, unsigned char data, unsigned int cmd)
{
	outb(data, chip->addr + cmd);
}

unsigned char snd_cmipci_read_b(struct cmi8788 *chip, unsigned int cmd)
{
	return inb(chip->addr + cmd);
}


/*
 * initialize the CMI8788 controller chip
 */
static int cmi8788_init_controller_chip(struct cmi8788 *chip)
{
	struct cmi_codec *codec;
	int i;
	int err;
	u8 tmp;

	chip->playback_volume_init = 0;
	chip->capture_volume_init = 0;

	chip->capture_source = CAPTURE_AC97_MIC;
	chip->CMI8788IC_revision = CMI8788IC_Revision1;

	/* CMI878 IC Revision */
	if (snd_cmipci_read_w(chip, PCI_RevisionRegister) & 0x0008)
		chip->CMI8788IC_revision = CMI8788IC_Revision2;

	if (chip->CMI8788IC_revision == CMI8788IC_Revision1) {
		tmp = snd_cmipci_read_b(chip, PCI_Misc);
		snd_cmipci_write_b(chip, tmp | 0x20, PCI_Misc);
	}

	/* Function Register */
	/* reset CODEC */
	tmp = snd_cmipci_read_b(chip, PCI_Fun);
	/* Bit1 set 1, RST_CODEC */
	/* Bit7 set 1, The function switch of pins, 1: select SPI chip 4, 5 enable function */
	snd_cmipci_write_b(chip, tmp | 0x82, PCI_Fun);

	/* initialize registers */
	/* I2S PCM Resolution 16 Bit 48k */
	snd_cmipci_write_w(chip, 0x010a, I2S_Multi_DAC_Fmt);
	snd_cmipci_write_w(chip, 0x010a, I2S_ADC1_Fmt);
	snd_cmipci_write_w(chip, 0x010a, I2S_ADC2_Fmt);
	snd_cmipci_write_w(chip, 0x010a, I2S_ADC3_Fmt);

	/* Digital Routing and Monitoring Registers */
	/* Playback Routing Register C0 */
	snd_cmipci_write_w(chip, 0xe400, Mixer_PlayRouting);

	/* Recording Routing Register C2 */
	snd_cmipci_write_b(chip, 0x00, Mixer_RecRouting);
	/* ADC Monitoring Control Register C3 */
	snd_cmipci_write_b(chip, 0x00, Mixer_ADCMonitorCtrl);
	/* Routing of Monitoring of Recording Channel A Register C4 */
	snd_cmipci_write_b(chip, 0xe4, Mixer_RoutOfRecMoniter);

	/* AC97 */
	snd_cmipci_write_b(chip, 0x00000000, AC97InChanCfg1);

	/* initialize CODEC */
	/* codec callback routine */
	for (i = 0; i < chip->num_codecs; i++) {
		codec = &chip->codec_list[i];
		if (codec->patch_ops.init) {
			err = codec->patch_ops.init(codec);
			if (err < 0)
				return err;
		}
	}

	/* for AC97 codec */
	codec = &chip->ac97_codec_list[0];
	if (codec->patch_ops.init) {
		err = codec->patch_ops.init(codec);
		if (err < 0)
			return err;
	}

	/* record route, AC97InChanCfg2 */
	/* Gpio #0 programmed as output, set CMI9780 Reg0x70 */
	snd_cmipci_write(chip, 0x00f00000, AC97InChanCfg2);
	udelay(150);
	/* FIXME: this is a read? */

	snd_cmi_send_ac97_cmd(chip, 0x70, 0x0100); /* Bit-8 set 1: record by MIC */

	/* LI2LI,MIC2MIC; let them always on, FOE on, ROE/BKOE/CBOE off */
	snd_cmi_send_ac97_cmd(chip, 0x62, 0x1808); /* 0x180f */

	/* unmute Master Volume */
	snd_cmi_send_ac97_cmd(chip, 0x02, 0x0000);

	/* change PCBeep path, set Mix2FR on, option for quality issue */
	snd_cmi_send_ac97_cmd(chip, 0x64, 0x8040);

	/* mute PCBeep, option for quality issue */
	snd_cmi_send_ac97_cmd(chip, 0x0a, 0x8000);

	/* Record Select Control Register (Index 1Ah) */
	snd_cmi_send_ac97_cmd(chip, 0x1a, 0x0000); /* 0000 : Mic in */

	/* set Mic Volume Register 0x0Eh umute */
	snd_cmi_send_ac97_cmd(chip, 0x0e, 0x0808); /* 0x0808 : 0dB */

	/* set CD Volume Register 0x12h mute */
	snd_cmi_send_ac97_cmd(chip, 0x12, 0x8808); /* 0x0808 : 0dB */

	/* set Line in Volume Register 0x10h mute */
	snd_cmi_send_ac97_cmd(chip, 0x10, 0x8808); /* 0x0808 : 0dB */

	/* set AUX Volume Register 0x10h mute */
	snd_cmi_send_ac97_cmd(chip, 0x16, 0x8808); /* 0x0808 : 0dB */

	snd_cmi_send_ac97_cmd(chip, 0x72, 0x0000);

	snd_cmi_send_ac97_cmd(chip, 0x72, 0x0001); /* Record throug Mic */
#if 0
	snd_cmi_send_ac97_cmd(chip, 0x72, 0x0000); /* Record throug Line in */
#endif
	return 0;
}

/*
 * Interface for send controller command to codec
 */

/* send a command by SPI interface
 * The data (which include address, r/w, and data bits) written to or read from the codec.
 * The bits in this register should be interpreted according to the individual codec.
 */
int snd_cmi_send_spi_cmd(struct cmi_codec *codec, u8 *data)
{
	struct cmi8788 *chip = codec->chip;
	u8 ctrl;

	snd_cmipci_write_b(chip, data[0], SPI_Data + 0);
	snd_cmipci_write_b(chip, data[1], SPI_Data + 1);
	if (codec->reg_len_flag) /* 3bytes */
		snd_cmipci_write_b(chip, data[2], SPI_Data + 2);

	ctrl = snd_cmipci_read_b(chip, SPI_Ctrl);

	/* codec select Bit 6:4 (0-5 XSPI_CEN0 - XSPI_CEN5) */
	ctrl &= 0x8f; /* Bit6:4 clear 0 */
	ctrl |= (codec->addr << 4) & 0x70; /* set Bit6:4 codec->addr. codec index XSPI_CEN 0-5 */

	/* SPI clock period */
	/* The data length of read/write */
	ctrl &= 0xfd; /* 1101 Bit-2 */
	if (codec->reg_len_flag) /* 3Byte */
		ctrl |= 0x02;

	/* Bit 0 Write 1 to trigger read/write operation */
	ctrl |= 0x01;

	snd_cmipci_write_b(chip, ctrl, SPI_Ctrl);
	udelay(50);
	return 0;
}

#if 0
/*
 * send a command by 2-wire interface
 */
static int snd_cmi_send_2wire_cmd(struct cmi_codec *codec, u8 reg_addr, u16 reg_data)
{
	struct cmi8788 *chip = codec->chip;
	u8 Status = 0;

	Status = snd_cmipci_read_b(chip, BusCtrlStatus);
	if ((Status & 0x01) == 1) /* busy */
		return -1;

	snd_cmipci_write_b(chip, reg_addr, MAPReg);
	snd_cmipci_write_w(chip, reg_data, DataReg);

	/* bit7-1 The target slave device address */
	/* bit0 1: read, 0: write */
	snd_cmipci_write_b(chip, codec->addr << 1, SlaveAddrCtrl);

	return 0;
}
#endif

/*
 * send AC'97 command, control AC'97 CODEC register
 */
void snd_cmi_send_ac97_cmd(struct cmi8788 *chip, u8 reg, u16 value)
{
	/* 31:25 Reserved */
	/* 24    R/W  Codec Command Select 1: Front-Panel Codec 0: On-Card Codec */
	/* 23    R/W  Read/Write Select 1:Read 0:Write */
	/* 22:16 R/W  CODEC Register Address */
	/* 15:0  R/W  CODEC Register Data This is the data that is written to the selected CODEC register */
	/*            when the write operation is selected. Reading this field will return the last value received from CODEC. */
	snd_cmipci_write(chip, (reg << 16) | value, AC97InChanCfg2);
	udelay(150);
}

#if 0
/* receive a response */
static unsigned int snd_cmi_get_response(struct cmi_codec *codec)
{
	/* ŽýÍêÉÆ */
	return 0;
}
#endif


/*
 * interrupt handler
 */
static irqreturn_t snd_cmi8788_interrupt(int irq, void *dev_id)
{
	struct cmi8788 *chip = dev_id;
	int i;
	u16 status;

	status = snd_cmipci_read_w(chip, PCI_IntStatus);

	if (0 == status)
		return IRQ_NONE;

	for (i = 0; i < chip->PCM_Count; i++) {
		struct cmi_substream *cmi_subs;

		/* playback */
		cmi_subs = &chip->cmi_pcm[i].cmi_subs[CMI_PLAYBACK];
		if (cmi_subs->running) {
			if (status & cmi_subs->int_mask)
				snd_cmi_pcm_interrupt(chip, cmi_subs);
		}

		/* capture */
		cmi_subs = &chip->cmi_pcm[i].cmi_subs[CMI_CAPTURE];
		if (cmi_subs->running) {
			if (status & cmi_subs->int_mask)
				snd_cmi_pcm_interrupt(chip, cmi_subs);
		}
	}

	return IRQ_HANDLED;
}


static inline void snd_cmi8788_codec_new(struct cmi8788 *chip,
					 struct cmi_codec *codec, u32 addr,
					 struct cmi_codec_ops *ops)
{
	codec->chip = chip;
	codec->addr = addr;
	codec->patch_ops = *ops;
}

static int __devinit snd_cmi8788_codec_create(struct cmi8788 *chip)
{
	/* ŽýÍêÉÆ£¬ÐèÒªÈ·¶š²»Í¬µÄ CODEC µ÷ÓÃ snd_cmi8788_codec_new Ê±ÒªŽ«µÝÊ²ÃŽ²ÎÊý */
	snd_cmi8788_codec_new(chip, &chip->codec_list[0], 0, &ak4396_patch_ops); /* DAC */
	snd_cmi8788_codec_new(chip, &chip->codec_list[1], 1, &ak4396_patch_ops); /* DAC */
	snd_cmi8788_codec_new(chip, &chip->codec_list[2], 2, &ak4396_patch_ops); /* DAC */
	snd_cmi8788_codec_new(chip, &chip->codec_list[3], 4, &ak4396_patch_ops); /* ÒÔºóÒªÓÃ akm4620_patch_ops); // DAC+ADC */
	snd_cmi8788_codec_new(chip, &chip->codec_list[4], 3, &wm8785_patch_ops); /* ADC */

	/* for CMI9780 AC97 */
	snd_cmi8788_codec_new(chip, &chip->ac97_codec_list[0], 0, &cmi9780_patch_ops); /* CMI9780 AC97 */

	/* initialize chip */
	cmi8788_init_controller_chip(chip);

	return 0;
}

/*
 * destructor
 */
static void snd_cmi8788_card_free(struct snd_card *card)
{
	struct cmi8788 *chip = card->private_data;

	if (chip->irq >= 0)
		free_irq(chip->irq, chip);

	pci_release_regions(chip->pci);
	pci_disable_device(chip->pci);
}

/*
 * constructor
 */
static int __devinit snd_cmi8788_probe(struct pci_dev *pci, const struct pci_device_id *pci_id)
{
	static int dev = 0;
	struct snd_card *card;
	struct cmi8788 *chip;
	int err;

	if (dev >= SNDRV_CARDS)
		return -ENODEV;
	if (!enable[dev]) {
		dev++;
		return -ENOENT;
	}

	card = snd_card_new(index[dev], id[dev], THIS_MODULE, sizeof *chip);
	if (!card) {
		printk(KERN_ERR "cmi8788: Error creating card!\n");
		return -ENOMEM;
	}
	chip = card->private_data;

	strcpy(card->driver, "CMI8788");

	err = pci_enable_device(pci);
	if (err < 0) {
		snd_card_free(card);
		return err;
	}

	/* spin_lock_init(&chip->reg_lock); */
	/* init_MUTEX(&chip->open_mutex); */

	chip->card = card;
	chip->pci = pci;
	chip->irq = -1;

	if ((err = pci_request_regions(pci, chip->card->driver)) < 0) {
		pci_disable_device(pci);
		snd_card_free(card);
		return err;
	}

	card->private_free = snd_cmi8788_card_free;

	chip->addr = pci_resource_start(pci, 0);

	if (request_irq(pci->irq, snd_cmi8788_interrupt, SA_INTERRUPT | SA_SHIRQ, card->driver, chip)) {
		snd_printk(KERN_ERR "cmi8788: unable to grab IRQ %d\n", pci->irq);
		snd_card_free(card);
		return -EBUSY;
	}
	chip->irq = pci->irq;

	pci_set_master(pci);
	snd_card_set_dev(card, &pci->dev);
	synchronize_irq(chip->irq);

	sprintf(card->shortname, "C-Media PCI %s", card->driver);
	sprintf(card->longname, "%s at 0x%lx, irq %i",
		card->shortname, chip->addr, chip->irq);

	/* init_MUTEX(&chip->codec_mutex); */

	/* ŽýÍêÉÆ£¬³õÊŒ»¯ CODEC µÄžöÊý */
	chip->num_codecs = 5;
	chip->num_ac97_codecs = 1;

	/* create codec instances */
	if ((err = snd_cmi8788_codec_create(chip)) < 0) {
		snd_card_free(card);
		return err;
	}

	/* create PCM streams */
	if ((err = snd_cmi8788_pcm_create(chip)) < 0) {
		snd_card_free(card);
		return err;
	}

	/* create mixer controls */
	if ((err = snd_cmi8788_mixer_create(chip)) < 0) {
		snd_card_free(card);
		return err;
	}

	if ((err = snd_card_register(card)) < 0) {
		snd_card_free(card);
		return err;
	}

	pci_set_drvdata(pci, card);
	dev++;

	return 0;
}

static void __devexit snd_cmi8788_remove(struct pci_dev *pci)
{
	snd_card_free(pci_get_drvdata(pci));
	pci_set_drvdata(pci, NULL);
}

static struct pci_device_id snd_cmi8788_ids[] = {
	{
		.vendor = PCI_VENDOR_ID_CMEDIA,
		.device = PCI_DEVICE_ID_CMEDIA_CM8788,
		.subvendor = PCI_ANY_ID,
		.subdevice = PCI_ANY_ID,
	},
	{ }
};
MODULE_DEVICE_TABLE(pci, snd_cmi8788_ids);


/* pci_driver definition */
static struct pci_driver driver = {
	.name     = "C-Media PCI",
	.id_table = snd_cmi8788_ids,
	.probe    = snd_cmi8788_probe,
	.remove   = __devexit_p(snd_cmi8788_remove),
};

static int __init alsa_card_cmi8788_init(void)
{
	return pci_module_init(&driver);
}

static void __exit alsa_card_cmi8788_exit(void)
{
	pci_unregister_driver(&driver);
}

module_init(alsa_card_cmi8788_init)
module_exit(alsa_card_cmi8788_exit)

EXPORT_NO_SYMBOLS; /* for older kernels */
