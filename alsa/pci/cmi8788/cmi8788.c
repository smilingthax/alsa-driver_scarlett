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
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/control.h>

#include "cmi8788.h"
#include "codec.h"
#include "cmi_controller.h"


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


extern int __devinit snd_cmi8788_mixer_create(snd_cmi8788 *chip);

/* read/write operations for dword register */
static inline void snd_cmipci_write(snd_cmi8788 *chip, unsigned int data, unsigned int cmd)
{
	outl(data, chip->addr + cmd);
}

static inline unsigned int snd_cmipci_read(snd_cmi8788 *chip, unsigned int cmd)
{
	return inl(chip->addr + cmd);
}

/* read/write operations for word register */
static inline void snd_cmipci_write_w(snd_cmi8788 *chip, unsigned short data, unsigned int cmd)
{
	outw(data, chip->addr + cmd);
}

static inline unsigned short snd_cmipci_read_w(snd_cmi8788 *chip, unsigned int cmd)
{
	return inw(chip->addr + cmd);
}

/* read/write operations for byte register */
static inline void snd_cmipci_write_b(snd_cmi8788 *chip, unsigned char data, unsigned int cmd)
{
	outb(data, chip->addr + cmd);
}

static inline unsigned char snd_cmipci_read_b(snd_cmi8788 *chip, unsigned int cmd)
{
	return inb(chip->addr + cmd);
}


/*
 * initialize the CMI8788 controller chip
 */
static int cmi8788_init_controller_chip(snd_cmi8788 *chip)
{
	int i, codec_num;
	cmi_codec *codec = NULL;
	u32 val32 = 0;
	u16 val16 = 0;
	u8 val8  = 0;
	int err;

	if (!chip)
		return 0;

	chip->playback_volume_init = 0;
	chip->capture_volume_init = 0;
	chip->first_set_playback_volume = 1;
	chip->first_set_capture_volume = 1;

	chip->capture_source = CAPTURE_AC97_MIC;
	chip->CMI8788IC_revision = CMI8788IC_Revision1;

	/* CMI878 IC Revision */
	val16 = snd_cmipci_read_w(chip, PCI_RevisionRegister);
	if (val16 & 0x0008)
		chip->CMI8788IC_revision = CMI8788IC_Revision2;

	if (chip->CMI8788IC_revision == CMI8788IC_Revision1) {
		val8 = snd_cmipci_read_b(chip, PCI_Misc);
		val8 = val8 | 0x20;
		snd_cmipci_write_b(chip, val8, PCI_Misc);
	}

	/* Function Register */
	/* reset CODEC */
	val8 = snd_cmipci_read_b(chip, PCI_Fun);
	val8 = val8 | 0x02; /* Bit1 set 1, RST_CODEC */
	val8 = val8 | 0x80; /* Bit7 set 1, The function switch of pins, 1: select SPI chip 4, 5 enable function */
	snd_cmipci_write_b(chip, val8, PCI_Fun);

	/* initialize registers */
	val16 = 0x010A; /* I2S PCM Resolution 16 Bit 48k */
	snd_cmipci_write_w(chip, val16, I2S_Multi_DAC_Fmt);
	val16 = 0x010A; /* I2S PCM Resolution 16 Bit 48k */
	snd_cmipci_write_w(chip, val16, I2S_ADC1_Fmt);
	val16 = 0x010A; /* I2S PCM Resolution 16 Bit 48k */
	snd_cmipci_write_w(chip, val16, I2S_ADC2_Fmt);
	val16 = 0x010A; /* I2S PCM Resolution 16 Bit 48k */
	snd_cmipci_write_w(chip, val16, I2S_ADC3_Fmt);

	/* Digital Routing and Monitoring Registers */
	/* Playback Routing Register C0 */
	val16 = 0xE400;
	snd_cmipci_write_w(chip, val16, Mixer_PlayRouting);

	/* Recording Routing Register C2 */
	val8 = 0x00;
	snd_cmipci_write_b(chip, val8, Mixer_RecRouting);
	/* ADC Monitoring Control Register C3 */
	val8 = 0x00;
	snd_cmipci_write_b(chip, val8, Mixer_ADCMonitorCtrl);
	/* Routing of Monitoring of Recording Channel A Register C4 */
	val8 = 0xe4;
	snd_cmipci_write_b(chip, val8, Mixer_RoutOfRecMoniter);

	/* AC97 */
	val32 = 0x00000000;
	snd_cmipci_write_b(chip, val32, AC97InChanCfg1);

	/* initialize CODEC */
	codec_num = chip->controller->codec_num;

	/* codec callback routine */
	for (i = 0; i < codec_num; i++) {
		codec = &(chip->controller->codec_list[i]);
		if (!codec->patch_ops.init)
			continue;
		err = codec->patch_ops.init(codec);
		if (err < 0)
			return err;
	}

	/* for AC97 codec */
	codec = &(chip->controller->ac97_codec_list[0]);
	if (codec->patch_ops.init)
		err = codec->patch_ops.init(codec);

	/* record route, AC97InChanCfg2 */
	/* Gpio #0 programmed as output, set CMI9780 Reg0x70 */
	val32 = 0x00F00000;
	snd_cmipci_write(chip, val32, AC97InChanCfg2);
	udelay(150);

	val32 = 0; /* Bit 31-24 */
	val32 &= 0xff000000; /* Bit-23: 0 write */
	val32 |= 0x70 << 16; /* Register 0x70 */
	val32 |= 0x0100; /* Bit-8 set 1: record by MIC */
	snd_cmipci_write(chip, val32, AC97InChanCfg2);
	udelay(150);

	/* LI2LI,MIC2MIC; let them always on, FOE on, ROE/BKOE/CBOE off */
	val32 = 0; /* Bit 31-24 */
	val32 &= 0xff000000; /* Bit-23: 0 write */
	val32 |= 0x62 << 16; /* Register 0x62 */
	val32 |= 0x1808; /* 0x180f */
	snd_cmipci_write(chip, val32, AC97InChanCfg2);
	udelay(150);

	/* unmute Master Volume */
	val32 = 0; /* Bit 31-24 */
	val32 &= 0xff000000; /*Bit-23: 0 write */
	val32 |= 0x02 << 16; /* Register 0x02 */
	val32 |= 0x0000;
	snd_cmipci_write(chip, val32, AC97InChanCfg2);
	udelay(150);

	/* change PCBeep path, set Mix2FR on, option for quality issue */
	val32 = 0; /* Bit 31-24 */
	val32 &= 0xff000000; /* Bit-23: 0 write */
	val32 |= 0x64 << 16; /* Register 0x64 */
	val32 |= 0x8040;
	snd_cmipci_write(chip, val32, AC97InChanCfg2);
	udelay(150);

	/* mute PCBeep, option for quality issue */
	val32 = 0; /* Bit 31-24 */
	val32 &= 0xff000000; /* Bit-23: 0 write */
	val32 |= 0x0A << 16; /* Register 0x0A */
	val32 |= 0x8000;
	snd_cmipci_write(chip, val32, AC97InChanCfg2);
	udelay(150);

	/* Record Select Control Register (Index 1Ah) */
#if 1
	val32 = 0; /* Bit 31-24 */
	val32 &= 0xff000000; /* Bit-23: 0 write */
	val32 |= 0x1A << 16; /* Register 0x1A */
	val32 |= 0x0000; /* 0000 : Mic in */
	snd_cmipci_write(chip, val32, AC97InChanCfg2);
	udelay(150);

	/* set Mic Volume Register 0x0Eh umute */
	val32 = 0; /* Bit 31-24 */
	val32 &= 0xff000000; /* Bit-23: 0 write */
	val32 |= 0x0E << 16; /* Register 0x0E */
	val32 |= 0x0808; /* 0x0808 : 0dB */
	snd_cmipci_write(chip, val32, AC97InChanCfg2);
	udelay(150);

	/* set CD Volume Register 0x12h mute */
	val32 = 0; /* Bit 31-24 */
	val32 &= 0xff000000; /* Bit-23: 0 write */
	val32 |= 0x12 << 16; /* Register 0x12 */
	val32 |= 0x8808; /* 0x0808 : 0dB */
	snd_cmipci_write(chip, val32, AC97InChanCfg2);
	udelay(150);

	/* set Line in Volume Register 0x10h mute */
	val32 = 0; /* Bit 31-24 */
	val32 &= 0xff000000; /* Bit-23: 0 write */
	val32 |= 0x10 << 16; /* Register 0x10 */
	val32 |= 0x8808; /* 0x0808 : 0dB */
	snd_cmipci_write(chip, val32, AC97InChanCfg2);
	udelay(150);

	/* set AUX Volume Register 0x10h mute */
	val32 = 0; /* Bit 31-24 */
	val32 &= 0xff000000; /* Bit-23: 0 write */
	val32 |= 0x16 << 16;/* Register 0x16 */
	val32 |= 0x8808; /* 0x0808 : 0dB */
	snd_cmipci_write(chip, val32, AC97InChanCfg2);
	udelay(150);

	/* */
	val32 = 0; /* Bit 31-24 */
	val32 &= 0xff000000; /* Bit-23: 0 write */
	val32 |= 0x72 << 16; /* Register 0x72 */
	val32 |= 0x0000;
	snd_cmipci_write(chip, val32, AC97InChanCfg2);
	udelay(150);

	val32 = 0x00720001; /* Record throug Mic */
	snd_cmipci_write(chip, val32, AC97InChanCfg2);
	udelay(150);

	val32 = 0x00720000; /* Record throug Line in */
	/* snd_cmipci_write(chip, val32, AC97InChanCfg2); */
	/* udelay(150); */
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
static int snd_cmi_send_spi_cmd(cmi_codec *codec, u8 *data)
{
	snd_cmi8788 *chip;
	u8 ctrl = 0;

	if (!codec || !data)
		return -1;

	chip = codec->controller->private_data;

	switch (codec->reg_len_flag) {
	case 0: /* 2bytes */
		snd_cmipci_write_b(chip, data[0], SPI_Data + 0); /* byte */
		snd_cmipci_write_b(chip, data[1], SPI_Data + 1); /* byte */
		break;
	case 1: /* 3bytes */
		snd_cmipci_write_b(chip, data[0], SPI_Data + 0); /* byte */
		snd_cmipci_write_b(chip, data[1], SPI_Data + 1); /* byte */
		snd_cmipci_write_b(chip, data[2], SPI_Data + 2); /* byte */
		break;
	default:
		return -1;
	}

	ctrl = snd_cmipci_read_b(chip, SPI_Ctrl);

	/* codec select Bit 6:4 (0-5 XSPI_CEN0 - XSPI_CEN5) */
	ctrl &= 0x8f; /* Bit6:4 clear 0 */
	ctrl |= (codec->addr << 4) & 0x70; /* set Bit6:4 codec->addr. codec index XSPI_CEN 0-5 */

	/* SPI clock period */
	/* The data length of read/write */
	ctrl &= 0xfd; /* 1101 Bit-2 */
	if (1 == codec->reg_len_flag) /* 3Byte */
		ctrl |= 0x02;

	/* Bit 0 Write 1 to trigger read/write operation */
	ctrl &= 0xfe;
	ctrl |= 0x01;

	snd_cmipci_write_b(chip, ctrl, SPI_Ctrl);

	udelay(50);
	return 0;
}

/*
 * send a command by 2-wire interface
 */
static int snd_cmi_send_2wire_cmd(cmi_codec *codec, u8 reg_addr, u16 reg_data)
{
	snd_cmi8788 *chip;
	u8 Status = 0;
	u8 reg = 0, slave_addr = 0;

	if (!codec)
		return -1;

	chip = codec->controller->private_data;

	Status = snd_cmipci_read_b(chip, BusCtrlStatus);
	if ((Status & 0x01) == 1) /* busy */
		return -1;

	slave_addr = codec->addr;
	reg = reg_addr;

	snd_cmipci_write_b(chip, reg_addr, MAPReg);
	snd_cmipci_write_w(chip, reg_data, DataReg);

	slave_addr = slave_addr << 1; /* bit7-1 The target slave device address */
	slave_addr = slave_addr & 0xfe; /* bit0 1: read, 0: write */

	snd_cmipci_write_b(chip, slave_addr, SlaveAddrCtrl);

	return 0;
}


/*
 * send AC'97 command, control AC'97 CODEC register
 */
static int snd_cmi_send_AC97_cmd(cmi_codec *codec, u8 reg_addr, u16 reg_data)
{
	snd_cmi8788 *chip;
	u32 val32 = 0;

	if (!codec)
		return -1;

	chip = codec->controller->private_data;

	/* 31:25 Reserved */
	/* 24    R/W  Codec Command Select 1: Front-Panel Codec 0: On-Card Codec */
	/* 23    R/W  Read/Write Select 1:Read 0:Write */
	/* 22:16 R/W  CODEC Register Address */
	/* 15:0  R/W  CODEC Register Data This is the data that is written to the selected CODEC register */
	/*            when the write operation is selected. Reading this field will return the last value received from CODEC. */
	val32 &= 0xff000000; /* Bit-23: 0 write */
	val32 |= reg_addr << 16; /* register address */
	val32 |= reg_data;

	snd_cmipci_write(chip, val32, AC97InChanCfg2);
	udelay(150);

	return 0;
}

/* receive a response */
static unsigned int snd_cmi_get_response(cmi_codec *codec)
{
	/* ŽýÍêÉÆ */
	return 0;
}

static struct snd_pcm_hardware snd_cmi_pcm_playback_hw = {
	.info =	SNDRV_PCM_INFO_MMAP |
		SNDRV_PCM_INFO_INTERLEAVED |
		SNDRV_PCM_INFO_BLOCK_TRANSFER |
		SNDRV_PCM_INFO_MMAP_VALID |
		SNDRV_PCM_INFO_PAUSE,
	.formats = SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE,
	.rates = SNDRV_PCM_RATE_5512 | SNDRV_PCM_RATE_8000_48000,
	.rate_min = 5512,
	.rate_max = 48000,
	.channels_min = 2,
	.channels_max = 8,
	.buffer_bytes_max = 128 * 1024,
	.period_bytes_min = 128,
	.period_bytes_max = 128 * 1024,
	.periods_min = 2,
	.periods_max = 1024,
};

static struct snd_pcm_hardware snd_cmi_pcm_capture_hw = {
	.info =	SNDRV_PCM_INFO_MMAP |
		SNDRV_PCM_INFO_INTERLEAVED |
		SNDRV_PCM_INFO_BLOCK_TRANSFER |
		SNDRV_PCM_INFO_MMAP_VALID  |
		SNDRV_PCM_INFO_PAUSE,
	.formats =     SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE,
	.rates = SNDRV_PCM_RATE_5512 | SNDRV_PCM_RATE_8000_48000,
	.rate_min = 5512,
	.rate_max = 48000,
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = 128 * 1024,
	.period_bytes_min = 128,
	.period_bytes_max = 128 * 1024,
	.periods_min = 2,
	.periods_max = 1024,
};

/*
 * int cmi_pcm_no :
 *   NORMAL_PCMS 0
 *   AC97_PCMS   1
 *   SPDIF_PCMS  2
 * int subs_no : CMI_PLAYBACK 0; CMI_CAPTURE 1
 *
 */
static int cmi_pcm_open(struct snd_pcm_substream *substream, int cmi_pcm_no, int subs_no)
{
	snd_cmi8788 *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	cmi_substream *cmi_subs = &chip->cmi_pcm[cmi_pcm_no].cmi_subs[subs_no];

	cmi_subs->substream = substream;

	switch(cmi_pcm_no) {
	case NORMAL_PCMS:
		switch(subs_no) {
		case CMI_PLAYBACK:
			chip->playback_volume_init = 1;
			cmi_subs->DMA_sta_mask   = 0x0010; /* PCI 40: PCI DMA Channel Start/Pause/Stop 2Byte -- 1 start*/
			cmi_subs->DMA_chan_reset = 0x0010; /* PCI 42: PCI DMA Channel Reset 1Byte --1 Reset*/
			cmi_subs->int_mask       = 0x0010; /* PCI 44: Interrupt Mask Register  2Byte -- 1 is enable*/
			cmi_subs->int_sta_mask   = 0x0010; /* PCI 46: interrupt status mask    2Byte-- Bit 4  Interrupt for Multi-Channel Playback DMA is pending*/
			runtime->hw = snd_cmi_pcm_playback_hw;
			break;
		case CMI_CAPTURE:
			chip->capture_volume_init = 1;
			cmi_subs->DMA_sta_mask   = 0x0001; /* PCI 40: PCI DMA Channel Start/Pause/Stop 2Byte -- 1 start*/
			cmi_subs->DMA_chan_reset = 0x0001; /* PCI 42: PCI DMA Channel Reset 1Byte --1 Reset*/
			cmi_subs->int_mask       = 0x0001; /* PCI 44: Interrupt Mask Register  2Byte -- 1 is enable*/
			cmi_subs->int_sta_mask   = 0x0001; /* PCI 46: interrupt status mask 2Byte -- Bit 0 Interrupt for Recording Channel A DMA is pending*/
			runtime->hw = snd_cmi_pcm_capture_hw;
			break;
		}
		break;
	case AC97_PCMS:
		switch(subs_no) {
		case CMI_PLAYBACK:
			cmi_subs->DMA_sta_mask   = 0x0020; /* PCI 40: PCI DMA Channel Start/Pause/Stop 2Byte -- 1 start*/
			cmi_subs->DMA_chan_reset = 0x0020; /* PCI 42: PCI DMA Channel Reset 1Byte --1 Reset*/
			cmi_subs->int_mask       = 0x4020; /* PCI 44: Interrupt Mask Register  2Byte -- 1 is enable*/
			cmi_subs->int_sta_mask   = 0x4020; /* PCI 46: interrupt status mask    2Byte-- Bit 4  Interrupt for Multi-Channel Playback DMA is pending*/
			runtime->hw = snd_cmi_pcm_playback_hw;
			break;
		}
		break;
	case SPDIF_PCMS:
		break;
	}
	snd_pcm_hw_constraint_minmax(runtime, SNDRV_PCM_HW_PARAM_BUFFER_SIZE, 0, 0x10000);

	runtime->private_data = chip;

	return 0;
}

static int cmi_pcm_close(struct snd_pcm_substream *substream, int cmi_pcm_no, int subs_no)
{
	snd_cmi8788 *chip = snd_pcm_substream_chip(substream);
	cmi_substream *cmi_subs = &chip->cmi_pcm[cmi_pcm_no].cmi_subs[subs_no];

	cmi_subs->substream    = NULL;

	cmi_subs->DMA_sta_mask   = 0x0000;
	cmi_subs->DMA_chan_reset = 0x00;
	cmi_subs->int_mask       = 0x0000;
	cmi_subs->int_sta_mask   = 0x0000;

	return 0;
}

static int snd_cmi_pcm_playback_open(struct snd_pcm_substream *substream)
{
	cmi_pcm_open(substream, NORMAL_PCMS, CMI_PLAYBACK);
	return 0;
}

static int snd_cmi_pcm_capture_open(struct snd_pcm_substream *substream)
{
	cmi_pcm_open(substream, NORMAL_PCMS, CMI_CAPTURE);
	return 0;
}

static int snd_cmi_pcm_ac97_playback_open(struct snd_pcm_substream *substream)
{
	cmi_pcm_open(substream, AC97_PCMS, CMI_PLAYBACK);
	return 0;
}

static int snd_cmi_pcm_playback_close(struct snd_pcm_substream *substream)
{
	cmi_pcm_close(substream, NORMAL_PCMS, CMI_PLAYBACK);
	return 0;
}

static int snd_cmi_pcm_capture_close(struct snd_pcm_substream *substream)
{
	cmi_pcm_close(substream, NORMAL_PCMS, CMI_CAPTURE);
	return 0;
}

static int snd_cmi_pcm_ac97_playback_close(struct snd_pcm_substream *substream)
{
	cmi_pcm_close(substream, AC97_PCMS, CMI_PLAYBACK);
	return 0;
}

static int snd_cmi_pcm_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *hw_params)
{
	return snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
}

static int snd_cmi_pcm_hw_free(struct snd_pcm_substream *substream)
{
	return snd_pcm_lib_free_pages(substream);
}

static int snd_cmi_pcm_playback_prepare(struct snd_pcm_substream *substream)
{
	snd_cmi8788 *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	cmi_substream *cmi_subs = &(chip->cmi_pcm[NORMAL_PCMS].cmi_subs[CMI_PLAYBACK]);
	cmi8788_controller *controller = chip->controller;
	cmi_codec *codec = controller->codec_list;
	u16 val16 = 0;
	u8 fmt;
	u16 I2SFmt;
	u8 PlyDmaMode;

	cmi_subs->dma_area  = (u32)runtime->dma_area;
	cmi_subs->dma_addr  = runtime->dma_addr;
	cmi_subs->dma_bytes = runtime->dma_bytes;
	cmi_subs->fragsize  = snd_pcm_lib_period_bytes(substream);

	cmi_subs->channels  = runtime->channels;
	cmi_subs->rate      = runtime->rate;
	cmi_subs->frame_bits= runtime->frame_bits;
	cmi_subs->sample_bits=runtime->sample_bits;

	/* Digital Routing and Monitoring Registers */
	val16 = snd_cmipci_read_w(chip, Mixer_PlayRouting);
	val16 &= ~0x0010; /* Bit4 clear 0 */
	snd_cmipci_write_w(chip, val16, Mixer_PlayRouting);

	/* set DMA Multi-Channel Playback DMA buffer addr length and fragsize */
	snd_cmipci_write(chip, cmi_subs->dma_addr , PCI_DMAPlay_MULTI_BaseAddr);
	snd_cmipci_write(chip, cmi_subs->dma_bytes / 4 - 1, PCI_DMAPlay_MULTI_BaseCount); /* d-word units */
	/* snd_cmipci_write(chip, cmi_subs->fragsize / 4 - 1 , PCI_DMAPlay_MUTLI_BaseTCount);// d-word units */
	snd_cmipci_write(chip, cmi_subs->dma_bytes / 8 - 1 , PCI_DMAPlay_MUTLI_BaseTCount);/* d-word units */

	/* Sample Format Convert for Playback Channels */
	/* I2S Multi-Channel DAC Format Register */
	val16 = 0;
	fmt = snd_cmipci_read_b(chip, PCI_PlaySampleFmCvt);
	I2SFmt = snd_cmipci_read_w(chip, I2S_Multi_DAC_Fmt);

	switch (cmi_subs->sample_bits) {
	case 16:
		fmt &= 0xf3;
		fmt |= 0x00; /* Bit 3:2 00 */
		I2SFmt &= 0xff3f;
		I2SFmt |= 0x0000; /* Bit 7:6  00 */
		break;
	case 20:
		I2SFmt &= 0xff3f;
		I2SFmt |= 0x0040; /* Bit 7:6  01 */
		break;
	case 24:
		fmt &= 0xf3;
		fmt |= 0xf4; /* Bit 3:2 01 */
		I2SFmt &= 0xff3f;
		I2SFmt |= 0x0080; /* Bit 7:6  10 */
		break;
	case 32:
		fmt &= 0xf3;
		fmt |= 0xf8; /* Bit 3:2 10 */
		I2SFmt &= 0xff3f;
		I2SFmt |= 0x00c0; /* Bit 7:6  11 */
		break;
	}
	snd_cmipci_write_b(chip, fmt, PCI_PlaySampleFmCvt);

	snd_cmipci_write_w(chip, I2SFmt, I2S_Multi_DAC_Fmt);

	/* set I2S sample rate */

	/* Multi-Channel DMA Mode */
	PlyDmaMode = snd_cmipci_read_b(chip, PCI_MULTI_DMA_MODE);
	switch (cmi_subs->channels) {
	case 2:
		/* Bit 1:0  00 */
		PlyDmaMode &= 0xfc;
		PlyDmaMode |= 0x00;
		codec[0].pcm_substream[0].ops.prepare(&codec[0].pcm_substream[0], &codec[0], substream);
		break;
	case 4:
		/* Bit 1:0  01 */
		PlyDmaMode &= 0xfc;
		PlyDmaMode |= 0x01;
		codec[0].pcm_substream[0].ops.prepare(&codec[0].pcm_substream[0], &codec[0], substream);
		codec[1].pcm_substream[0].ops.prepare(&codec[1].pcm_substream[0], &codec[1], substream);
		break;
	case 6:
		/* Bit 1:0  10 */
		PlyDmaMode &= 0xfc;
		PlyDmaMode |= 0x02;
		codec[0].pcm_substream[0].ops.prepare(&codec[0].pcm_substream[0], &codec[0], substream);
		codec[1].pcm_substream[0].ops.prepare(&codec[1].pcm_substream[0], &codec[1], substream);
		codec[2].pcm_substream[0].ops.prepare(&codec[2].pcm_substream[0], &codec[2], substream);
		break;
	case 8:
		/* Bit 1:0  11 */
		PlyDmaMode &= 0xfc;
		PlyDmaMode |= 0x03;
		codec[0].pcm_substream[0].ops.prepare(&codec[0].pcm_substream[0], &codec[0], substream);
		codec[1].pcm_substream[0].ops.prepare(&codec[1].pcm_substream[0], &codec[1], substream);
		codec[2].pcm_substream[0].ops.prepare(&codec[2].pcm_substream[0], &codec[2], substream);
		codec[3].pcm_substream[0].ops.prepare(&codec[3].pcm_substream[0], &codec[3], substream);
		break;
	default:
		break;
	}
	snd_cmipci_write_b(chip, PlyDmaMode, PCI_MULTI_DMA_MODE);

	return 0;
}


static int snd_cmi_pcm_capture_prepare(struct snd_pcm_substream *substream)
{
	snd_cmi8788 *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	cmi_substream *cmi_subs= &chip->cmi_pcm[NORMAL_PCMS].cmi_subs[CMI_CAPTURE];
	u32 val32 = 0;
	u8 fmt = 0;
	u16 I2SFmt = 0;
	u8 RecDmaMode;

	cmi_subs->dma_area  = (u32)runtime->dma_area;
	cmi_subs->dma_addr  = runtime->dma_addr;
	cmi_subs->dma_bytes = runtime->dma_bytes;
	cmi_subs->fragsize  = snd_pcm_lib_period_bytes(substream);

	cmi_subs->channels  = runtime->channels;
	cmi_subs->rate      = runtime->rate;
	cmi_subs->frame_bits= runtime->frame_bits;
	cmi_subs->sample_bits=runtime->sample_bits;

	/* set DMA Recording Channel A DMA buffer addr length and fragsize */
	snd_cmipci_write(chip, cmi_subs->dma_addr, PCI_DMARec_A_BaseAddr);
	snd_cmipci_write_w(chip, cmi_subs->dma_bytes / 4 - 1, PCI_DMARec_A_BaseCount); /* d-word units */
	snd_cmipci_write_w(chip, cmi_subs->fragsize / 4 - 1, PCI_DMARec_A_BaseTCount); /* d-word units old */
	/* snd_cmipci_write_w(chip, cmi_subs->dma_bytes / 8 - 1, PCI_DMARec_A_BaseTCount); // d-word units */

	/* Sample Format Convert for Recording Channels */
	fmt = snd_cmipci_read_b(chip, PCI_RecSampleFmtCvt);
	/* I2S ADC 1 Format Register */
	I2SFmt = snd_cmipci_read_w(chip, I2S_ADC1_Fmt);

	switch (cmi_subs->sample_bits) {
	case 16:
		fmt &= 0xfc;
		fmt |= 0x00; /* Bit 1:0 00 */
		I2SFmt &= 0xff3f;
		I2SFmt |= 0x0000; /* Bit 7:6 00 */
		break;
	case 20:
		I2SFmt &= 0xff3f;
		I2SFmt |= 0x0040; /* Bit 7:6 01 */
		break;
	case 24:
		fmt &= 0xfc;
		fmt |= 0x01; /* Bit 1:0 01 */
		I2SFmt &= 0xff3f;
		I2SFmt |= 0x0080; /* Bit 7:6 10 */
		break;
	case 32:
		fmt &= 0xfc;
		fmt |= 0x02; /* Bit 1:0 10 */
		I2SFmt &= 0xff3f;
		I2SFmt |= 0x00c0; /* Bit 7:6 11 */
		break;
	}

	snd_cmipci_write_b(chip, fmt,PCI_RecSampleFmtCvt);
	snd_cmipci_write_w(chip, I2SFmt,I2S_ADC1_Fmt);

	/* set I2S sample rate */

	RecDmaMode = snd_cmipci_read_b(chip, PCI_RecDMA_Mode);
	switch (cmi_subs->channels) {
	case 2:
		/* Bit 2:0 000 */
		RecDmaMode &= 0xf8;
		RecDmaMode |= 0x00;
		break;
	case 4:
		/* Bit 2:0 001 */
		RecDmaMode &= 0xf8;
		RecDmaMode |= 0x01;
		break;
	case 6:
		/* Bit 2:0 011 */
		RecDmaMode &= 0xf8;
		RecDmaMode |= 0x03; /* or 0x02 */
		break;
	case 8:
		/* Bit 2:0 100 */
		RecDmaMode &= 0xf8;
		RecDmaMode |= 0x04;
		break;
	default:
		break;
	}
	snd_cmipci_write_b(chip, RecDmaMode, PCI_RecDMA_Mode);

	switch (chip->capture_source) {
	case CAPTURE_AC97_MIC:
		/* set Mic Volume Register 0x0Eh umute */
		val32 = 0; /* Bit 31-24 */
		val32 &= 0xff000000; /* Bit-23: 0 write */
		val32 |= 0x0E << 16; /* Register 0x0E */
		val32 |= 0x0808; /* 0x0808 : 0dB */
		snd_cmipci_write(chip, val32, AC97InChanCfg2);
		udelay(150);

		/* set Line in Volume Register 0x10h mute */
		val32 = 0; /* Bit 31-24 */
		val32 &= 0xff000000; /* Bit-23: 0 write */
		val32 |= 0x10 << 16; /* Register 0x10 */
		val32 |= 0x8808; /* 0x0808 : 0dB */
		snd_cmipci_write(chip, val32, AC97InChanCfg2);
		udelay(150);

		/* slect record source */
		val32 = 0; /* Bit 31-24 */
		val32 &= 0xff000000; /* Bit-23: 0 write */
		val32 |= 0x1A << 16; /* Register 0x1A */
		val32 |= 0x0000; /* 0000 : Mic in */
		snd_cmipci_write(chip, val32, AC97InChanCfg2);

		val32 = 0x00720001; /* Record throug Mic */
		snd_cmipci_write(chip, val32, AC97InChanCfg2);
		udelay(150);
		break;
	case CAPTURE_AC97_LINEIN:
		/* set Mic Volume Register 0x0Eh mute */
		val32 = 0; /* Bit 31-24 */
		val32 &= 0xff000000; /* Bit-23: 0 write */
		val32 |= 0x0E << 16; /* Register 0x0E */
		val32 |= 0x8808; /* 0x0808 : 0dB */
		snd_cmipci_write(chip, val32, AC97InChanCfg2);
		udelay(150);

		/* set Line in Volume Register 0x10h umute */
		val32 = 0; /* Bit 31-24 */
		val32 &= 0xff000000; /* Bit-23: 0 write */
		val32 |= 0x10 << 16; /* Register 0x10 */
		val32 |= 0x0808; /* 0x0808 : 0dB */
		snd_cmipci_write(chip, val32, AC97InChanCfg2);
		udelay(150);

		/* slect record source */
		val32 = 0; /* Bit 31-24 */
		val32 &= 0xff000000; /* Bit-23: 0 write */
		val32 |= 0x1A << 16; /* Register 0x1A */
		val32 |= 0x0000; /* 0404 : Ac97 Line in */
		snd_cmipci_write(chip, val32, AC97InChanCfg2);

		val32 = 0x00720001; /* Record throug AC97 Line in */
		snd_cmipci_write(chip, val32, AC97InChanCfg2);
		udelay(150);
		break;
	case CAPTURE_DIRECT_LINE_IN:
		/* set Mic Volume Register 0x0Eh mute */
		val32 = 0; /* Bit 31-24 */
		val32 &= 0xff000000; /* Bit-23: 0 write */
		val32 |= 0x0E << 16;/* Register 0x0E */
		val32 |= 0x80808; /* 0x0808 : 0dB */
		/* FIXME: value should be 0x8808? */
		snd_cmipci_write(chip, val32, AC97InChanCfg2);
		udelay(150);

		/* set Line in Volume Register 0x10h mute */
		val32 = 0; /* Bit 31-24 */
		val32 &= 0xff000000; /* Bit-23: 0 write */
		val32 |= 0x10 << 16; /* Register 0x10 */
		val32 |= 0x8808; /* 0x0808 : 0dB */
		snd_cmipci_write(chip, val32, AC97InChanCfg2);
		udelay(150);

		val32 = 0x00720000; /* Record throug Line in */
		snd_cmipci_write(chip, val32, AC97InChanCfg2);
		udelay(150);
		break;
	}

	return 0;
}

static int snd_cmi_pcm_ac97_playback_prepare(struct snd_pcm_substream *substream)
{
	snd_cmi8788 *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	cmi_substream *cmi_subs= &chip->cmi_pcm[AC97_PCMS].cmi_subs[CMI_PLAYBACK];
	u16 val16 = 0;
	u32 val32 = 0;

	cmi_subs->dma_area  = (u32)runtime->dma_area;
	cmi_subs->dma_addr  = runtime->dma_addr;
	cmi_subs->dma_bytes = runtime->dma_bytes;
	cmi_subs->fragsize  = snd_pcm_lib_period_bytes(substream);

	cmi_subs->channels  = runtime->channels;
	cmi_subs->rate      = runtime->rate;
	cmi_subs->frame_bits= runtime->frame_bits;
	cmi_subs->sample_bits=runtime->sample_bits;

	/* Digital Routing and Monitoring Registers */
	val16 = snd_cmipci_read_w(chip, Mixer_PlayRouting);
	val16 |= 0x0010; /* Bit4 set 1 */
	snd_cmipci_write_w(chip, val16, Mixer_PlayRouting);

	/* AC'97 Output Channel Configuration Register */
	val32 = snd_cmipci_read(chip, AC97OutChanCfg);
	val32 |= 0x0000ff00; /* Bit15-8 set 1 */
	snd_cmipci_write(chip, val32, AC97OutChanCfg);

	/* set DMA Front Panel Playback */
	snd_cmipci_write(chip, cmi_subs->dma_addr , PCI_DMAPlay_Front_BaseAddr);
	snd_cmipci_write_w(chip, cmi_subs->dma_bytes / 4 - 1, PCI_DMAPlay_Front_BaseCount); /* d-word units */
	snd_cmipci_write_w(chip, cmi_subs->fragsize / 4 - 1, PCI_DMAPlay_Front_BaseTCount); /* d-word units */
	return 0;
}

static int cmi_pcm_trigger(struct snd_pcm_substream *substream, int cmi_pcm_no, int subs_no, int cmd)
{
	snd_cmi8788 *chip = snd_pcm_substream_chip(substream);
	cmi_substream *cmi_subs= &chip->cmi_pcm[cmi_pcm_no].cmi_subs[subs_no];

	int err = 0;
	u8 reset = 0;
	u16 int_val = 0;
	u16 int_stat= 0;
	int DMARestRegister = PCI_DMA_Reset;

	int_val = snd_cmipci_read_w(chip, PCI_IntMask);
	int_stat = snd_cmipci_read_w(chip, PCI_DMA_SetStatus);

	if (chip->CMI8788IC_revision == CMI8788IC_Revision1)
		DMARestRegister = PCI_DMA_Reset;
	if (chip->CMI8788IC_revision == CMI8788IC_Revision2)
		DMARestRegister = PCI_DMA_FLUSH;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_START:
		cmi_subs->running = 1;

		/* Reset DMA Channel*/
		reset = snd_cmipci_read_b(chip, DMARestRegister);
		reset |= cmi_subs->DMA_chan_reset; /* set bit */
		snd_cmipci_write_b(chip, reset, DMARestRegister);
		reset &= ~cmi_subs->DMA_chan_reset; /*clear bit */
		snd_cmipci_write_b(chip, reset, DMARestRegister);

		/* enable Interrupt */
		int_val |= cmi_subs->int_mask; /* Set Bit-1 Interrupt for Recording DMA Channel A is enabled */
		snd_cmipci_write_w(chip, int_val, PCI_IntMask);

		/* Set PCI DMA Channel state -- Start*/
		int_stat |= cmi_subs->DMA_sta_mask; /* PCI DMA Channel Start/Pause/Stop Bit-0 set 1 */
		snd_cmipci_write_w(chip, int_stat, PCI_DMA_SetStatus);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	case SNDRV_PCM_TRIGGER_STOP:
		cmi_subs->running = 0;

		/* Set PCI DMA Channel state -- Stop*/
		int_stat &= ~cmi_subs->DMA_sta_mask; /* PCI DMA Channel Start/Pause/Stop Bit-0 clear */
		snd_cmipci_write_w(chip, int_stat, PCI_DMA_SetStatus);

		/* disable interrupt */
		int_val &= ~cmi_subs->int_mask; /* Clear Interrupt for Recording DMA Channel A is disabled */
		snd_cmipci_write_w(chip, int_val, PCI_IntMask);
		break;
	default:
		err = -EINVAL;
	}

	return err;
}

static int snd_cmi_pcm_playback_trigger(struct snd_pcm_substream *substream, int cmd)
{
	return cmi_pcm_trigger(substream, NORMAL_PCMS, CMI_PLAYBACK, cmd);
}

static int snd_cmi_pcm_capture_trigger(struct snd_pcm_substream *substream, int cmd)
{
	return cmi_pcm_trigger(substream, NORMAL_PCMS, CMI_CAPTURE, cmd);
}

static int snd_cmi_pcm_ac97_playback_trigger(struct snd_pcm_substream *substream, int cmd)
{
	return cmi_pcm_trigger(substream, AC97_PCMS, CMI_PLAYBACK, cmd);
}

static snd_pcm_uframes_t snd_cmi_pcm_playback_pointer(struct snd_pcm_substream *substream)
{
	snd_cmi8788 *chip = snd_pcm_substream_chip(substream);
	u32 addr = 0, pos = 0;
	cmi_substream *cmi_subs = &chip->cmi_pcm[NORMAL_PCMS].cmi_subs[CMI_PLAYBACK];

	addr = snd_cmipci_read(chip, PCI_DMAPlay_MULTI_BaseAddr);
	pos = addr - cmi_subs->dma_addr;
	return bytes_to_frames(substream->runtime, pos);
}

static snd_pcm_uframes_t snd_cmi_pcm_capture_pointer(struct snd_pcm_substream *substream)
{
	snd_cmi8788 *chip = snd_pcm_substream_chip(substream);
	u32 addr = 0, pos = 0;
	cmi_substream *cmi_subs = &chip->cmi_pcm[NORMAL_PCMS].cmi_subs[CMI_CAPTURE];

	addr = snd_cmipci_read(chip, PCI_DMARec_A_BaseAddr);
	pos = addr - cmi_subs->dma_addr;
	return bytes_to_frames(substream->runtime, pos);
}

static snd_pcm_uframes_t snd_cmi_pcm_ac97_playback_pointer(struct snd_pcm_substream *substream)
{
	snd_cmi8788 *chip = snd_pcm_substream_chip(substream);
	u32 addr = 0, pos = 0;
	cmi_substream *cmi_subs = &chip->cmi_pcm[AC97_PCMS].cmi_subs[CMI_PLAYBACK];

	addr = snd_cmipci_read(chip, PCI_DMAPlay_Front_BaseAddr);
	pos = addr - cmi_subs->dma_addr;
	return bytes_to_frames(substream->runtime, pos);
}

static struct snd_pcm_ops snd_cmi_pcm_playback_ops = {
	.open      = snd_cmi_pcm_playback_open,
	.close     = snd_cmi_pcm_playback_close,
	.ioctl     = snd_pcm_lib_ioctl,
	.hw_params = snd_cmi_pcm_hw_params,
	.hw_free   = snd_cmi_pcm_hw_free,
	.prepare   = snd_cmi_pcm_playback_prepare,
	.trigger   = snd_cmi_pcm_playback_trigger,
	.pointer   = snd_cmi_pcm_playback_pointer,
};

static struct snd_pcm_ops snd_cmi_pcm_capture_ops = {
	.open      = snd_cmi_pcm_capture_open,
	.close     = snd_cmi_pcm_capture_close,
	.ioctl     = snd_pcm_lib_ioctl,
	.hw_params = snd_cmi_pcm_hw_params,
	.hw_free   = snd_cmi_pcm_hw_free,
	.prepare   = snd_cmi_pcm_capture_prepare,
	.trigger   = snd_cmi_pcm_capture_trigger,
	.pointer   = snd_cmi_pcm_capture_pointer,
};

static struct snd_pcm_ops snd_cmi_pcm_ac97_playback_ops = {
	.open      = snd_cmi_pcm_ac97_playback_open,
	.close     = snd_cmi_pcm_ac97_playback_close,
	.ioctl     = snd_pcm_lib_ioctl,
	.hw_params = snd_cmi_pcm_hw_params,
	.hw_free   = snd_cmi_pcm_hw_free,
	.prepare   = snd_cmi_pcm_ac97_playback_prepare,
	.trigger   = snd_cmi_pcm_ac97_playback_trigger,
	.pointer   = snd_cmi_pcm_ac97_playback_pointer,
};

static void snd_cmi_pcm_free(struct snd_pcm *pcm)
{
	snd_pcm_lib_preallocate_free_for_all(pcm);
}


/*
 * interrupt handler
 */
static irqreturn_t snd_cmi8788_interrupt(int irq, void *dev_id)
{
	snd_cmi8788 *chip = dev_id;
	int i;
	u16 status;
	u16 old_int_val = 0, int_val = 0;

	status = snd_cmipci_read_w(chip, PCI_IntStatus);

	if (0 == PCI_IntStatus)
		return IRQ_NONE;

	for (i = 0; i < chip->PCM_Count; i++) {
		cmi_substream *cmi_subs = NULL;

		/* playback */
		cmi_subs = &chip->cmi_pcm[i].cmi_subs[CMI_PLAYBACK];
		if (cmi_subs->running) {
			if (status & cmi_subs->int_sta_mask) {
				old_int_val = 0x0; /*  Set Bit-4 Interrupt for Multi-Channel Playback DMA is enabled */
				old_int_val = snd_cmipci_read_w(chip, PCI_IntMask);

				/* disable interrupt */
				int_val = old_int_val & ~cmi_subs->int_mask;
				snd_cmipci_write_w(chip, int_val, PCI_IntMask);

				/* enable interrupt */
				int_val = old_int_val | cmi_subs->int_mask; /*  Set Bit-4 Interrupt for Multi-Channel Playback DMA is enabled */
				snd_cmipci_write_w(chip, int_val, PCI_IntMask);

				snd_pcm_period_elapsed(cmi_subs->substream);
			}
		}

		/* capture */
		cmi_subs = &chip->cmi_pcm[i].cmi_subs[CMI_CAPTURE];
		if (cmi_subs->running) {
			if (status & cmi_subs->int_sta_mask) {
				old_int_val = 0x0; /*  Set Bit-0 Interrupt for DMA Channel A is disenabled */
				old_int_val = snd_cmipci_read_w(chip, PCI_IntMask);

				/* disable interrupt */
				int_val = old_int_val & ~cmi_subs->int_mask;
				snd_cmipci_write_w(chip, int_val, PCI_IntMask);

				/* enable interrupt */
				int_val = int_val | cmi_subs->int_mask;/*  Set Bit-0 Interrupt for DMA Channel A */
				snd_cmipci_write_w(chip, int_val, PCI_IntMask);

				snd_pcm_period_elapsed(cmi_subs->substream);
			}
		}
	}

	return IRQ_HANDLED;
}


/*
 * create pcm DAC/ADC, SPDIF
 */
static int __devinit snd_cmi8788_pcm_create(snd_cmi8788 *chip)
{
	struct snd_pcm *pcm = NULL;
	int err = 0, pcm_dev = 0;
	cmi8788_controller *controller = NULL;
	int i = 0, codec_num =0;
	cmi_codec *codec = NULL;
	int iRet = 0;

	if (!chip)
		return 0;

	controller = chip->controller;
	if (!controller)
		return 0;

#if 1 /* swf 2005-04-25 */
	/* 1 create normal PCM */
	err = snd_pcm_new(chip->card, "C-Media PCI8788 DAC/ADC",
			  pcm_dev, 1, 1, &chip->pcm[pcm_dev]);
	if (err < 0)
		return err;

	pcm = chip->pcm[pcm_dev];

	codec_num = chip->controller->codec_num;
	codec = NULL;

	/* ³õÊŒ»¯Ò»Ð© callback routine */
	for (i = 0; i < codec_num; i++) {
		codec = &controller->codec_list[i];
		if (!codec->patch_ops.build_pcms)
			continue;

		err = codec->patch_ops.build_pcms(codec);
		if (err < 0)
			return err;
	}

	/* for AC97 codec */
	codec_num = chip->controller->ac97_cocde_num;
	codec = NULL;
	for (i = 0; i < codec_num; i++) {
		codec = &controller->ac97_codec_list[i];
		if (!codec->patch_ops.build_pcms)
			continue;

		err = codec->patch_ops.build_pcms(codec);
		if (err < 0)
			return err;
	}

	/* create pcm stream by controller supported */
	/* The PCM use Multi-Channel Playback DMA for playback and */
	/*         use Recording Channel A DMA for record */
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_cmi_pcm_playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE,  &snd_cmi_pcm_capture_ops);

	pcm->private_data = chip;
	pcm->private_free = snd_cmi_pcm_free;
	pcm->info_flags = 0;

	strcpy(pcm->name, "C-Media PCI8788 DAC/ADC");

	iRet = snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
						     snd_dma_pci_data(chip->pci),
						     1024 * 64, 1024 * 128     );

	pcm_dev++;

#endif /* swf 2005-04-25 */

#if 0
	/* create the other pcm stream. AC97 SPDIF */
	/* 2 create AC97 PCM */
	err = snd_pcm_new(chip->card, "C-Media PCI8788 DAC/ADC",
			  pcm_dev, 1, 0, &chip->pcm[pcm_dev]);
	if (err < 0)
		return err;

	pcm = chip->pcm[pcm_dev];
	codec = &controller->ac97_codec_list[0];
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_cmi_pcm_ac97_playback_ops);

	pcm->private_data = chip;
	pcm->private_free = snd_cmi_pcm_free;
	pcm->info_flags = 0;

	strcpy(pcm->name, "C-Media PCI8788 AC97 DAC/ADC");

	iRet = snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
						     snd_dma_pci_data(chip->pci),
						     1024 * 64, 1024 * 128     );
	pcm_dev++;

#endif
	/* 3 create SPDIF PCM */

	chip->PCM_Count = pcm_dev;

	return 0;
}

extern codec_preset snd_preset_ak4396[];
extern codec_preset snd_preset_wm8785[];
extern codec_preset snd_preset_cmi9780[];

static int __devinit snd_cmi8788_codec_create(snd_cmi8788 *chip)
{
	cmi8788_controller *controller;

	if (!chip)
		return 0;

	controller = chip->controller;

	/* ŽýÍêÉÆ£¬ÐèÒªÈ·¶š²»Í¬µÄ CODEC µ÷ÓÃ snd_cmi8788_codec_new Ê±ÒªŽ«µÝÊ²ÃŽ²ÎÊý */
	snd_cmi8788_codec_new(controller, &controller->codec_list[0], 0, snd_preset_ak4396); /* DAC */
	snd_cmi8788_codec_new(controller, &controller->codec_list[1], 1, snd_preset_ak4396); /* DAC */
	snd_cmi8788_codec_new(controller, &controller->codec_list[2], 2, snd_preset_ak4396); /* DAC */
	snd_cmi8788_codec_new(controller, &controller->codec_list[3], 4, snd_preset_ak4396); /* ÒÔºóÒªÓÃ snd_preset_akm4620); // DAC+ADC */
	snd_cmi8788_codec_new(controller, &controller->codec_list[4], 3, snd_preset_wm8785); /* ADC */

	/* for CMI9780 AC97 */
	snd_cmi8788_codec_new(controller, &controller->ac97_codec_list[0], 0, snd_preset_cmi9780); /* CMI9780 AC97 */

	/* initialize chip */
	cmi8788_init_controller_chip(chip);

	return 0;
}

static int __devinit snd_cmi8788_controller_create(snd_cmi8788 *chip)
{
	cmi_bus_template bus_temp;
	int err;

	if (!chip)
		return 0;

	memset(&bus_temp, 0, sizeof(bus_temp));
	bus_temp.private_data     = chip;
	bus_temp.pci              = chip->pci;
	bus_temp.ops.spi_cmd      = snd_cmi_send_spi_cmd;
	bus_temp.ops.twowire_cmd  = snd_cmi_send_2wire_cmd;
	bus_temp.ops.ac97_cmd     = snd_cmi_send_AC97_cmd;
	bus_temp.ops.get_response = snd_cmi_get_response;

	if ((err = snd_cmi8788_controller_new(chip, &bus_temp, &chip->controller)) < 0)
		return err;

	/* ŽýÍêÉÆ£¬³õÊŒ»¯ CODEC µÄžöÊý */
	chip->controller->codec_num = 5; /* */
	chip->controller->ac97_cocde_num = 1;

	return 0;
}

/*
 * destructor
 */
static int snd_cmi8788_free(snd_cmi8788 *chip)
{
	if (!chip)
		return 0;

	if (chip->irq >= 0)
		free_irq(chip->irq, chip);

	kfree(chip->controller);
	chip->controller = NULL;

	pci_release_regions(chip->pci);
	pci_disable_device(chip->pci);

	kfree(chip);

	return 0;
}

static int snd_cmi8788_device_free(struct snd_device *device)
{
	return snd_cmi8788_free(device->device_data);
}

/*
 * constructor
 */
static int __devinit snd_cmi8788_create(struct snd_card *card, struct pci_dev *pci, snd_cmi8788 **rchip)
{
	snd_cmi8788 *chip = NULL;
	int err = 0;

	*rchip = NULL;

	if ((err = pci_enable_device(pci)) < 0)
		return err;

	chip = kzalloc(sizeof(snd_cmi8788), GFP_KERNEL);
	if (!chip) {
		snd_printk(KERN_ERR "cmi8788: cannot allocate chip\n");
		pci_disable_device(pci);
		return -ENOMEM;
	}

	/* spin_lock_init(&chip->reg_lock); */
	/* init_MUTEX(&chip->open_mutex); */

	chip->card = card;
	chip->pci = pci;
	chip->irq = -1;

	if ((err = pci_request_regions(pci, chip->card->driver)) < 0) {
		kfree(chip);
		pci_disable_device(pci);
		return err;
	}

	chip->addr = pci_resource_start(pci, 0);

	if (request_irq(pci->irq, snd_cmi8788_interrupt, SA_INTERRUPT | SA_SHIRQ, chip->card->driver, chip)) {
		snd_printk(KERN_ERR "cmi8788: unable to grab IRQ %d\n", pci->irq);
		err = -EBUSY;
		goto errout;
	}
	chip->irq = pci->irq;

	pci_set_master(pci);
	synchronize_irq(chip->irq);

	*rchip = chip;
	return 0;

errout:
	snd_cmi8788_free(chip);
	return err;
}

static int __devinit snd_cmi8788_probe(struct pci_dev *pci, const struct pci_device_id *pci_id)
{
	static struct snd_device_ops ops = {
		.dev_free = snd_cmi8788_device_free,
	};
	static int dev = 0;
	struct snd_card *card = NULL;
	snd_cmi8788 *chip = NULL;
	int err = 0;

	if (dev >= SNDRV_CARDS)
		return -ENODEV;
	if (!enable[dev]) {
		dev++;
		return -ENOENT;
	}

	card = snd_card_new(index[dev], id[dev], THIS_MODULE, 0);
	if (NULL == card) {
		printk(KERN_ERR "cmi8788: Error creating card!\n");
		return -ENOMEM;
	}

	strcpy(card->driver, "CMI8788");

	if ((err = snd_cmi8788_create(card, pci, &chip)) < 0) {
		snd_card_free(card);
		return err;
	}
	sprintf(card->shortname, "C-Media PCI %s", card->driver);
	sprintf(card->longname, "%s at 0x%lx, irq %i",
		card->shortname, chip->addr, chip->irq);

	/* create cmi8788 controller instances */
	if ((err = snd_cmi8788_controller_create(chip)) < 0) {
		snd_card_free(card);
		return err;
	}

	/* create codec instances */
	if ((err = snd_cmi8788_codec_create(chip)) < 0) {
		snd_card_free(card);
		return err;
	}

	if ((err = snd_device_new(card, SNDRV_DEV_LOWLEVEL, chip, &ops)) <0) {
		printk(KERN_ERR "Error creating device [card]!\n");
		snd_cmi8788_free(chip);
		goto errout;
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
	snd_card_set_dev(card, &pci->dev);

	if ((err = snd_card_register(card)) < 0) {
		snd_card_free(card);
		return err;
	}

	pci_set_drvdata(pci, card);
	dev++;

errout:
	return err;
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
