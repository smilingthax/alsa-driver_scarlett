#ifndef __ESSM_H
#define __ESSM_H

/*
 *  Copyright (c) by Matze Braun <MatzeBraun@gmx.de>
 *  Definitions for ESS Maestro 2E
 *
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
#include "mixer.h"
#include "mpu401.h"
#include "ac97_codec.h"
#include "driver.h"
#include "ainstr_simple.h"

/* PCI Dev ID's */

#ifndef PCI_VENDOR_ID_ESS
#define PCI_VENDOR_ID_ESS	0x125D
#endif

#define PCI_VENDOR_ID_ESS_OLD	0x1285	/* Platform Tech, the people the ESS
					   was bought form */

#ifndef PCI_DEVICE_ID_ESS_M2E
#define PCI_DEVICE_ID_ESS_M2E	0x1978
#endif
#ifndef PCI_DEVICE_ID_ESS_M2
#define PCI_DEVICE_ID_ESS_M2	0x1968
#endif
#ifndef PCI_DEVICE_ID_ESS_M1
#define PCI_DEVICE_ID_ESS_M1	0x0100
#endif

#define NR_APUS		64
#define NR_APU_REGS	16

/* NEC Versas ? */
#define NEC_VERSA_SUBID1	0x80581033
#define NEC_VERSA_SUBID2	0x803c1033

/* Mode Flags */
#define ESS_CFMT_STEREO     	0x01
#define ESS_CFMT_16BIT      	0x02
#define ESS_CFMT_MASK       	0x03

#define DAC_RUNNING		1
#define ADC_RUNNING		2

#define ESM_MODE_NONE		0x00
#define ESM_MODE_PLAY		0x01
#define ESM_MODE_REC		0x02

/* Values for the ESM_LEGACY_AUDIO_CONTROL */

#define ESS_ENABLE_AUDIO	0x8000
#define ESS_ENABLE_SERIAL_IRQ	0x4000
#define IO_ADRESS_ALIAS		0x0020
#define MPU401_IRQ_ENABLE	0x0010
#define MPU401_IO_ENABLE	0x0008
#define GAME_IO_ENABLE		0x0004
#define FM_IO_ENABLE		0x0002
#define SB_IO_ENABLE		0x0001

/* Values for the ESM_CONFIG_A */

#define PIC_SNOOP1		0x4000
#define PIC_SNOOP2		0x2000
#define SAFEGUARD		0x0800
#define DMA_CLEAR		0x0700
#define DMA_DDMA		0x0000
#define DMA_TDMA		0x0100
#define DMA_PCPCI		0x0200
#define POST_WRITE		0x0080
#define ISA_TIMING		0x0040
#define SWAP_LR			0x0020
#define SUBTR_DECODE		0x0002

/* Values for the ESM_CONFIG_B */

#define SPDIF_CONFB		0x0100
#define HWV_CONFB		0x0080
#define DEBOUNCE		0x0040
#define GPIO_CONFB		0x0020
#define CHI_CONFB		0x0010
#define IDMA_CONFB		0x0008	/*undoc */
#define MIDI_FIX		0x0004	/*undoc */
#define IRQ_TO_ISA		0x0001	/*undoc */

/* Values for Ring Bus Control B */
#define	RINGB_2CODEC_ID_MASK	0x0003
#define RINGB_DIS_VALIDATION	0x0008
#define RINGB_EN_SPDIF		0x0010
#define	RINGB_EN_2CODEC		0x0020
#define RINGB_SING_BIT_DUAL	0x0040

/* ****Port Adresses**** */

/*   Write & Read */
#define ESM_INDEX		0x02
#define ESM_DATA		0x00

/*   AC97 + RingBus */
#define AC97_INDEX		0x30
#define AC97_STATUS		0x30
#define	AC97_DATA		0x32
#define ESM_RING_BUS_CONTR_A	0x36
#define ESM_RING_BUS_CONTR_B	0x38
#define ESM_RING_BUS_SDO	0x3A

/*   WaveCache*/
#define WC_INDEX		0x10
#define WC_DATA			0x12
#define WC_CONTROL		0x14

/*   ASSP*/
#define ASSP_INDEX		0x80
#define ASSP_MEMORY		0x82
#define ASSP_DATA		0x84
#define ASSP_CONTROL_A		0xA2
#define ASSP_CONTROL_B		0xA4
#define ASSP_CONTROL_C		0xA6
#define ASSP_HOSTW_INDEX	0xA8
#define ASSP_HOSTW_DATA		0xAA
#define ASSP_HOSTW_IRQ		0xAC
/* Midi */
#define ESM_MPU401_PORT		0x98
/* Others */
#define ESM_PORT_HOST_IRQ	0x18

#define IDR0_DATA_PORT		0x00
#define IDR1_CRAM_POINTER	0x01
#define IDR2_CRAM_DATA		0x02
#define IDR3_WAVE_DATA		0x03
#define IDR4_WAVE_PTR_LOW	0x04
#define IDR5_WAVE_PTR_HI	0x05
#define IDR6_TIMER_CTRL		0x06
#define IDR7_WAVE_ROMRAM	0x07

#define WRITEABLE_MAP		0xEFFFFF
#define READABLE_MAP		0x64003F

/* PCI Register */

#define ESM_LEGACY_AUDIO_CONTROL 0x40
#define ESM_ACPI_COMMAND	0x54
#define ESM_CONFIG_A		0x50
#define ESM_CONFIG_B		0x52

/* Bob Bits */
#define ESM_BOB_ENABLE		0x0001
#define ESM_BOB_START		0x0001

/* Host IRQ Control Bits */
#define ESM_RESET_MAESTRO	0x8000
#define ESM_RESET_DIRECTSOUND   0x4000
#define ESM_HIRQ_ClkRun		0x0100
#define ESM_HIRQ_HW_VOLUME	0x0040
#define ESM_HIRQ_HARPO		0x0030	/* What's that? */
#define ESM_HIRQ_ASSP		0x0010
#define	ESM_HIRQ_DSIE		0x0004
#define ESM_HIRQ_MPU401		0x0002
#define ESM_HIRQ_SB		0x0001

/* Host IRQ Status Bits */
#define ESM_MPU401_IRQ		0x02
#define ESM_SB_IRQ		0x01
#define ESM_SOUND_IRQ		0x04
#define	ESM_ASSP_IRQ		0x10
#define ESM_HWVOL_IRQ		0x40

#define ESM_BOB_FREQ 	150

#define ESM_FREQ_ESM1  (49152000L / 1024L)
#define ESM_FREQ_ESM2  (50000000L / 1024L)
#define ESM_FREQ_ESM2E (50000000L / 1024L)

struct snd_stru_maestro;
struct snd_stru_channel;
struct snd_stru_channelp;
struct snd_stru_channelc;
struct snd_stru_esmdma;
enum snd_enum_apu_type;

typedef struct snd_stru_es1968 es1968_t;
typedef struct snd_stru_channelp esschanp_t;	/* Playback Channel */
typedef struct snd_stru_channelc esschanc_t;	/* Record Channel */
typedef struct snd_stru_esmdma esmdma_t;
typedef enum snd_enum_apu_type esmaput;

/* APU use in the driver */
enum snd_enum_apu_type {
	pcm_play,
	pcm_capture,
	pcm_rateconv,
	wavetable,
	free
};

/* DMA Hack! */
struct snd_stru_esmdma {
	es1968_t *card;
	char *buf;
	int size;

	esmdma_t *next;
};

/* Playback Channel */
struct snd_stru_channelp {
	es1968_t *card;
	int num;

	u8 apu[2];
	u8 apu_mode[2];

	spinlock_t reg_lock;

	unsigned int hwptr;	/* Old HWPtr */
	unsigned int count;	/* Sample Count */
	unsigned int transs;	/* Transfer Size */
	unsigned int transf;	/* Transfer Fragment */
	u16 base;		/* Offset for ptr */

	/* PCM stuff */
	unsigned int mode;
	unsigned char fmt, enable;
};

/* Record Channel */
struct snd_stru_channelc {
	es1968_t *card;
	int num;

	u8 apu[4];
	u8 apu_mode[4];

	spinlock_t reg_lock;

	char *mixbuf;

	unsigned int hwptr;	/* Old HWPtr */
	unsigned int count;	/* Sample Count */
	unsigned int transs;	/* Transfer Size */
	unsigned int transf;	/* Transfer Fragment */
	u16 base;		/* Offset for ptr */

	/* PCM stuff */
	unsigned int mode;
	unsigned char fmt, enable;
};

struct snd_stru_es1968 {
	/* Module Config */
	int midi;
	int gesbuf;
	int pcmp, pcmc;

	/* Resources... */
	snd_dma_t *dma1ptr;
	snd_dma_t *dma2ptr;
	snd_irq_t *irqptr;

	unsigned int io_port;

	int type;
	struct pci_dev *pci;
	snd_card_t *card;
	snd_pcm_t *pcm;

	/* DMA Hack! */
	char *firstbuf;		/* The Main Buffer */
	esmdma_t *bufs;

	/* ALSA Stuff */
	ac97_t *ac97;
	snd_kmixer_t *mixer;
	snd_rawmidi_t *rmidi;

	spinlock_t reg_lock;
	snd_info_entry_t *proc_entry;

	/* Maestro Stuff */
	u16 maestro_map[32];
	int bobclient;

	/* APU's */
	esmaput apu[NR_APUS];
};

int snd_es1968_create(snd_card_t * card, struct pci_dev *pci,
		   snd_dma_t * dma1ptr, snd_dma_t * dma2ptr,
		   snd_irq_t * irqptr, int midi_enable, int gesbuf,
		   int pcmp, int pcmc, es1968_t ** resm);

int snd_es1968_free(es1968_t * esm);
void snd_es1968_interrupt(es1968_t * esm);

int snd_es1968_pcm(es1968_t * esm, int device, snd_pcm_t ** rpcm);

int snd_es1968_mixer(es1968_t * esm, int device, snd_pcm_t * pcm,
		     snd_kmixer_t ** rmixer);

int snd_es1968_midi(es1968_t * esm, int device, snd_rawmidi_t **);

#endif				/* __ESSM_H__ */
