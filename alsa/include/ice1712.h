#ifndef __ICE1712_H
#define __ICE1712_H

/*
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
 *  Portions (c) IC Ensemble, Inc.
 *  Definitions for ICEnsemble ICE1712 chip
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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "pcm.h"
#include "rawmidi.h"
#include "ac97_codec.h"

#ifndef PCI_VENDOR_ID_ICE
#define PCI_VENDOR_ID_ICE		0x1412
#endif
#ifndef PCI_DEVICE_ID_ICE_1712
#define PCI_DEVICE_ID_ICE_1712		0x1712
#endif

#define ICE1712_SUBDEVICE_STDSP24	0x12141217	/* Hoontech SoundTrack Audio DSP 24 */
#define ICE1712_SUBDEVICE_DELTA1010	0x121430d6
#define ICE1712_SUBDEVICE_DELTADIO2496	0x121431d6
#define ICE1712_SUBDEVICE_DELTA66	0x121432d6
#define ICE1712_SUBDEVICE_DELTA44	0x121433d6

/*
 *  Direct registers
 */

#define ICEREG(ice, x) ((ice)->port + ICE1712_REG_##x)

#define ICE1712_REG_CONTROL		0x00	/* byte */
#define   ICE1712_RESET			0x80	/* reset whole chip */
#define   ICE1712_SERR_LEVEL		0x04	/* SERR# level otherwise edge */
#define   ICE1712_NATIVE		0x01	/* native mode otherwise SB */
#define ICE1712_REG_IRQMASK		0x01	/* byte */
#define   ICE1712_IRQ_MPU1		0x80
#define   ICE1712_IRQ_TIMER		0x40
#define   ICE1712_IRQ_MPU2		0x20
#define   ICE1712_IRQ_PROPCM		0x10
#define   ICE1712_IRQ_FM		0x08	/* FM/MIDI - legacy */
#define   ICE1712_IRQ_PBKDS		0x04	/* playback DS channels */
#define   ICE1712_IRQ_CONCAP		0x02	/* consumer capture */
#define   ICE1712_IRQ_CONPBK		0x01	/* consumer playback */
#define ICE1712_REG_IRQSTAT		0x02	/* byte */
/* look to ICE1712_IRQ_* */
#define ICE1712_REG_INDEX		0x03	/* byte - indirect CCIxx regs */
#define ICE1712_REG_DATA		0x04	/* byte - indirect CCIxx regs */
#define ICE1712_REG_NMI_STAT1		0x05	/* byte */
#define ICE1712_REG_NMI_DATA		0x06	/* byte */
#define ICE1712_REG_NMI_INDEX		0x07	/* byte */
#define ICE1712_REG_AC97_INDEX		0x08	/* byte */
#define ICE1712_REG_AC97_CMD		0x09	/* byte */
#define   ICE1712_AC97_COLD		0x80	/* cold reset */
#define   ICE1712_AC97_WARM		0x40	/* warm reset */
#define   ICE1712_AC97_WRITE		0x20	/* W: write, R: write in progress */
#define   ICE1712_AC97_READ		0x10	/* W: read, R: read in progress */
#define   ICE1712_AC97_READY		0x08	/* codec ready status bit */
#define   ICE1712_AC97_PBK_VSR		0x02	/* playback VSR */
#define   ICE1712_AC97_CAP_VSR		0x01	/* capture VSR */
#define ICE1712_REG_AC97_DATA		0x0a	/* word (little endian) */
#define ICE1712_REG_MPU1_CTRL		0x0c	/* byte */
#define ICE1712_REG_MPU1_DATA		0x0d	/* byte */
#define ICE1712_REG_I2C_DEV_ADDR	0x10	/* byte */
#define   ICE1712_I2C_WRITE		0x01	/* write direction */
#define ICE1712_REG_I2C_BYTE_ADDR	0x11	/* byte */
#define ICE1712_REG_I2C_DATA		0x12	/* byte */
#define ICE1712_REG_I2C_CTRL		0x13	/* byte */
#define   ICE1712_I2C_EEPROM		0x80	/* EEPROM exists */
#define   ICE1712_I2C_BUSY		0x01	/* busy bit */
#define ICE1712_REG_CONCAP_ADDR		0x14	/* dword - consumer capture */
#define ICE1712_REG_CONCAP_COUNT	0x18	/* word - current/base count */
#define ICE1712_REG_SERR_SHADOW		0x1b	/* byte */
#define ICE1712_REG_MPU2_CTRL		0x1c	/* byte */
#define ICE1712_REG_MPU2_DATA		0x1d	/* byte */
#define ICE1712_REG_TIMER		0x1e	/* word */

/*
 *  Indirect registers
 */

#define ICE1712_IREG_PBK_COUNT_HI	0x00
#define ICE1712_IREG_PBK_COUNT_LO	0x00
#define ICE1712_IREG_PBK_CTRL		0x02
#define ICE1712_IREG_PBK_LEFT		0x03	/* left volume */
#define ICE1712_IREG_PBK_RIGHT		0x04	/* right volume */
#define ICE1712_IREG_PBK_SOFT		0x05	/* soft volume */
#define ICE1712_IREG_PBK_RATE_LO	0x06
#define ICE1712_IREG_PBK_RATE_MID	0x07
#define ICE1712_IREG_PBK_RATE_HI	0x08
#define ICE1712_IREG_CAP_COUNT_HI	0x10
#define ICE1712_IREG_CAP_COUNT_LO	0x11
#define ICE1712_IREG_CAP_CTRL		0x12
#define ICE1712_IREG_GPIO_DATA		0x20
#define ICE1712_IREG_GPIO_WRITE_MASK	0x21
#define ICE1712_IREG_GPIO_DIRECTION	0x22
#define ICE1712_IREG_CONSUMER_POWERDOWN	0x30
#define ICE1712_IREG_PRO_POWERDOWN	0x31

/*
 *  Consumer section direct DMA registers
 */

#define ICEDS(ice, x) ((ice)->dmapath_port + ICE1712_DS_##x)
 
#define ICE1712_DS_INTMASK		0x00	/* word - interrupt mask */
#define ICE1712_DS_INTSTAT		0x02	/* word - interrupt status */
#define ICE1712_DS_DATA			0x04	/* dword - channel data */
#define ICE1712_DS_INDEX		0x08	/* dword - channel index */

/*
 *  Consumer section channel registers
 */
 
#define ICE1712_DSC_ADDR0		0x00	/* dword - base address 0 */
#define ICE1712_DSC_COUNT0		0x01	/* word - count 0 */
#define ICE1712_DSC_ADDR1		0x02	/* dword - base address 1 */
#define ICE1712_DSC_COUNT1		0x03	/* word - count 1 */
#define ICE1712_DSC_CONTROL		0x04	/* byte - control & status */
#define   ICE1712_BUFFER1		0x80	/* buffer1 is active */
#define   ICE1712_BUFFER1_AUTO		0x40	/* buffer1 auto init */
#define   ICE1712_BUFFER0_AUTO		0x20	/* buffer0 auto init */
#define   ICE1712_FLUSH			0x10	/* flush FIFO */
#define   ICE1712_STEREO		0x08	/* stereo */
#define   ICE1712_16BIT			0x04	/* 16-bit data */
#define   ICE1712_PAUSE			0x02	/* pause */
#define   ICE1712_START			0x01	/* start */
#define ICE1712_DSC_RATE		0x05	/* dword - rate */
#define ICE1712_DSC_VOLUME		0x06	/* word - volume control */

/* 
 *  Professional multi-track direct control registers
 */

#define ICEMT(ice, x) ((ice)->profi_port + ICE1712_MT_##x)

#define ICE1712_MT_IRQ			0x00	/* byte - interrupt mask */
#define   ICE1712_MULTI_CAPTURE		0x80	/* capture IRQ */
#define   ICE1712_MULTI_PLAYBACK	0x40	/* playback IRQ */
#define   ICE1712_MULTI_CAPSTATUS	0x02	/* capture IRQ status */
#define   ICE1712_MULTI_PBKSTATUS	0x01	/* playback IRQ status */
#define ICE1712_MT_RATE			0x01	/* byte - sampling rate select */
#define   ICE1712_SPDIF_MASTER		0x10	/* S/PDIF input is master clock */
#define ICE1712_MT_I2S_FORMAT		0x02	/* byte - I2S data format */
#define ICE1712_MT_AC97_INDEX		0x04	/* byte - AC'97 index */
#define ICE1712_MT_AC97_CMD		0x05	/* byte - AC'97 command & status */
/* look to ICE1712_AC97_* */
#define ICE1712_MT_DATA			0x06	/* word - AC'97 data */
#define ICE1712_MT_PLAYBACK_ADDR	0x10	/* dword - playback address */
#define ICE1712_MT_PLAYBACK_SIZE	0x14	/* word - playback size */
#define ICE1712_MT_PLAYBACK_COUNT	0x16	/* word - playback count */
#define ICE1712_MT_PLAYBACK_CONTROL	0x18	/* byte - control */
#define   ICE1712_CAPTURE_START_SHADOW	0x04	/* capture start */
#define   ICE1712_PLAYBACK_PAUSE	0x02	/* playback pause */
#define   ICE1712_PLAYBACK_START	0x01	/* playback start */
#define ICE1712_MT_CAPTURE_ADDR		0x20	/* dword - capture address */
#define ICE1712_MT_CAPTURE_SIZE		0x24	/* word - capture size */
#define ICE1712_MT_CAPTURE_COUNT	0x26	/* word - capture count */
#define ICE1712_MT_CAPTURE_CONTROL	0x28	/* byte - control */
#define   ICE1712_CAPTURE_START		0x01	/* capture start */
#define ICE1712_MT_ROUTE_PSDOUT03	0x30	/* word */
#define ICE1712_MT_ROUTE_SPDOUT		0x32	/* word */
#define ICE1712_MT_ROUTE_CAPTURE	0x34	/* dword */
#define ICE1712_MT_MONITOR_VOLUME	0x38	/* word */
#define ICE1712_MT_MONITOR_INDEX	0x3a	/* byte */
#define ICE1712_MT_MONITOR_RATE		0x3b	/* byte */
#define ICE1712_MT_MONITOR_ROUTECTRL	0x3c	/* byte */
#define   ICE1712_ROUTE_AC97		0x01	/* route digital mixer output to AC'97 */
#define ICE1712_MT_MONITOR_PEAKINDEX	0x3e	/* byte */
#define ICE1712_MT_MONITOR_PEAKDATA	0x3f	/* byte */

/*
 *  Codec configuration bits
 */

/* PCI[60] System Configuration */
#define ICE1712_CFG_CLOCK	0xc0
#define   ICE1712_CFG_CLOCK512	0x00	/* 22.5692Mhz, 44.1kHz*512 */
#define   ICE1712_CFG_CLOCK384  0x40	/* 16.9344Mhz, 44.1kHz*384 */
#define   ICE1712_CFG_EXT	0x80	/* external clock */
#define ICE1712_CFG_2xMPU401	0x20	/* two MPU401 UARTs */
#define ICE1712_CFG_NO_CON_AC97 0x10	/* consumer AC'97 codec is not present */
#define ICE1712_CFG_ADC_MASK	0x0c	/* one, two, three, four stereo ADCs */
#define ICE1712_CFG_DAC_MASK	0x03	/* one, two, three, four stereo DACs */
/* PCI[61] AC-Link Configuration */
#define ICE1712_CFG_PRO_I2S	0x80	/* multitrack converter: I2S or AC'97 */
#define ICE1712_CFG_AC97_PACKED	0x01	/* split or packed mode - AC'97 */
/* PCI[62] I2S Features */
#define ICE1712_CFG_I2S_VOLUME	0x80	/* volume/mute capability */
#define ICE1712_CFG_I2S_96KHZ	0x40	/* supports 96kHz sampling */
#define ICE1712_CFG_I2S_RESMASK	0x30	/* resolution mask, 16,18,20,24-bit */
#define ICE1712_CFG_I2S_OTHER	0x0f	/* other I2S IDs */
/* PCI[63] S/PDIF Configuration */
#define ICE1712_CFG_I2S_CHIPID	0xfc	/* I2S chip ID */
#define ICE1712_CFG_SPDIF_IN	0x02	/* S/PDIF input is present */
#define ICE1712_CFG_SPDIF_OUT	0x01	/* S/PDIF output is present */

/* MidiMan Delta GPIO definitions */

#define ICE1712_DELTA_DFS	0x01	/* fast/slow sample rate mode */
					/* (>48kHz must be 1) */
					/* all cards */
#define ICE1712_DELTA_SPDIF_IN_STAT 0x02
					/* S/PDIF input status */
					/* 0 = valid signal is present */
					/* all except Delta44 */
					/* look to CS8414 datasheet */
#define ICE1712_DELTA_SPDIF_OUT_STAT_CLOCK 0x04
					/* S/PDIF output status clock */
					/* (writting on rising edge - 0->1) */
					/* all except Delta44 */
					/* look to CS8404A datasheet */
#define ICE1712_DELTA_SPDIF_OUT_STAT_DATA 0x08
					/* S/PDIF output status data */
					/* all except Delta44 */
					/* look to CS8404A datasheet */
#define ICE1712_DELTA_SPDIF_INPUT_SELECT 0x10
					/* coaxial (0), optical (1) */
					/* S/PDIF input select*/
					/* DeltaDiO only */
#define ICE1712_DELTA_WORD_CLOCK_SELECT 0x10
					/* 1 - clock are taken from S/PDIF input */
					/* 0 - clock are taken from Word Clock input */
					/* Delta1010 only (affected SPMCLKIN pin of Envy24) */
#define ICE1712_DELTA_CODEC_SERIAL_DATA 0x10
					/* AKM4524 serial data */
					/* Delta66 and Delta44 */
#define ICE1712_DELTA_WORD_CLOCK_STATUS	0x20
					/* 0 = valid word clock signal is present */
					/* Delta1010 only */
#define ICE1712_DELTA_CODEC_SERIAL_CLOCK 0x20
					/* AKM4524 serial clock */
					/* (writting on rising edge - 0->1 */
					/* Delta66 and Delta44 */
#define ICE1712_DELTA_CODEC_CHIP_A	0x40
#define ICE1712_DELTA_CODEC_CHIP_B	0x80
					/* 1 - select chip A or B */
					/* Delta66 and Delta44 */

/* Hoontech SoundTrack Audio DSP 24 GPIO definitions */

#define ICE1712_STDSP24_0_BOX(r, x)	r[0] = ((r[0] & ~3) | ((x)&3))
#define ICE1712_STDSP24_0_DAREAR(r, x)	r[0] = ((r[0] & ~4) | (((x)&1)<<2))
#define ICE1712_STDSP24_1_CHN1(r, x)	r[1] = ((r[1] & ~1) | ((x)&1))
#define ICE1712_STDSP24_1_CHN2(r, x)	r[1] = ((r[1] & ~2) | (((x)&1)<<1))
#define ICE1712_STDSP24_1_CHN3(r, x)	r[1] = ((r[1] & ~4) | (((x)&1)<<2))
#define ICE1712_STDSP24_2_CHN4(r, x)	r[2] = ((r[2] & ~1) | ((x)&1))
#define ICE1712_STDSP24_2_MIDIIN(r, x)	r[2] = ((r[2] & ~2) | (((x)&1)<<1))
#define ICE1712_STDSP24_2_MIDI1(r, x)	r[2] = ((r[2] & ~4) | (((x)&2)<<2))
#define ICE1712_STDSP24_3_MIDI2(r, x)	r[3] = ((r[3] & ~1) | ((x)&2))
#define ICE1712_STDSP24_3_MUTE(r, x)	r[3] = ((r[3] & ~2) | (((x)&2)<<1))
#define ICE1712_STDSP24_3_INSEL(r, x)	r[3] = ((r[3] & ~4) | (((x)&2)<<2))
#define ICE1712_STDSP24_SET_ADDR(r, a)	r[a&3] = ((r[a&3] & ~0x18) | (((a)&3)<<3))
#define ICE1712_STDSP24_CLOCK(r, a, c)	r[a&3] = ((r[a&3] & ~0x20) | (((c)&1)<<5))

/* Hoontech SoundTrack Audio DSP 24 box configuration definitions */

#define ICE1712_STDSP24_DAREAR		(1<<0)
#define ICE1712_STDSP24_MUTE		(1<<1)
#define ICE1712_STDSP24_INSEL		(1<<2)

#define ICE1712_STDSP24_BOX_CHN1	(1<<0)	/* input channel 1 */
#define ICE1712_STDSP24_BOX_CHN2	(1<<1)	/* input channel 2 */
#define ICE1712_STDSP24_BOX_CHN3	(1<<2)	/* input channel 3 */
#define ICE1712_STDSP24_BOX_CHN4	(1<<3)	/* input channel 4 */
#define ICE1712_STDSP24_BOX_MIDI1	(1<<8)
#define ICE1712_STDSP24_BOX_MIDI2	(1<<9)

/*
 *  
 */

typedef struct snd_stru_ice1712 ice1712_t;

typedef struct {
	unsigned int subvendor;	/* PCI[2c-2f] */
	unsigned char size;	/* size of EEPROM image in bytes */
	unsigned char version;	/* must be 1 */
	unsigned char codec;	/* codec configuration PCI[60] */
	unsigned char aclink;	/* ACLink configuration PCI[61] */
	unsigned char i2sID;	/* PCI[62] */
	unsigned char spdif;	/* S/PDIF configuration PCI[63] */
	unsigned char gpiomask;	/* GPIO initial mask, 0 = write, 1 = don't */
	unsigned char gpiostate; /* GPIO initial state */
	unsigned char gpiodir;	/* GPIO direction state */
	unsigned short ac97main;
	unsigned short ac97pcm;
	unsigned short ac97rec;
	unsigned char ac97recsrc;
	unsigned char dacID[4];	/* I2S IDs for DACs */
	unsigned char adcID[4];	/* I2S IDs for ADCs */
	unsigned char extra[4];
} ice1712_eeprom_t;

struct snd_stru_ice1712 {
	unsigned long conp_dma_size;
	unsigned long conc_dma_size;
	unsigned long prop_dma_size;
	unsigned long proc_dma_size;
	int irq;

	unsigned long port;
	struct resource *res_port;
	unsigned long ddma_port;
	struct resource *res_ddma_port;
	unsigned long dmapath_port;
	struct resource *res_dmapath_port;
	unsigned long profi_port;
	struct resource *res_profi_port;

	unsigned int config;	/* system configuration */

	struct pci_dev *pci;
	snd_card_t *card;
	snd_pcm_t *pcm;
	snd_pcm_t *pcm_pro;
        snd_pcm_substream_t *playback_con_substream[6];
        snd_pcm_substream_t *capture_con_substream;
        snd_pcm_substream_t *playback_pro_substream;
        snd_pcm_substream_t *capture_pro_substream;
	unsigned int playback_pro_size;
	unsigned int capture_pro_size;
	unsigned int playback_con_virt_addr[6];
	unsigned int playback_con_active_buf[6];
	unsigned int capture_con_virt_addr;
	unsigned int ac97_ext_id;
	ac97_t *ac97;
	snd_rawmidi_t *rmidi[2];

	spinlock_t reg_lock;
	struct semaphore gpio_mutex;
	snd_info_entry_t *proc_entry;

	ice1712_eeprom_t eeprom;

	unsigned int pro_volumes[20];
	unsigned char ak4524_adc_volume[4];
	unsigned char ak4524_dac_volume[4];
	unsigned char hoontech_boxbits[4];
	unsigned int hoontech_config;
	unsigned short hoontech_boxconfig[4];

	unsigned int spdif_defaults;
};

int snd_ice1712_create(snd_card_t * card,
		       struct pci_dev *pci,
		       unsigned long conp_dma_size,
		       unsigned long conc_dma_size,
		       unsigned long prop_dma_size,
		       unsigned long proc_dma_size,
		       ice1712_t ** ice1712);

int snd_ice1712_pcm(ice1712_t * ice1712, int device, snd_pcm_t ** rpcm);
int snd_ice1712_pcm_profi(ice1712_t * ice1712, int device, snd_pcm_t ** rpcm);
int snd_ice1712_mixer(ice1712_t * ice1712);

#endif				/* __ICE1712_H */
