#ifndef __EMU10K1_H
#define __EMU10K1_H

/*
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>,
 *		     Creative Labs, Inc.
 *  Definitions for EMU10K1 (SB Live!) chips
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
#include "midi.h"
#include "ak4531_codec.h"
#include "ac97_codec.h"

#ifndef PCI_VENDOR_ID_CREATIVE
#define PCI_VENDOR_ID_CREATIVE		0x1102
#endif
#ifndef PCI_DEVICE_ID_CREATIVE_EMU10K1
#define PCI_DEVICE_ID_CREATIVE_EMU10K1	0x0002
#endif

/* ------------------- DEFINES -------------------- */

#define EMUPAGESIZE     4096
#define MAXREQVOICES    8
#define MAXPAGES        8192
#define RESERVED        0
#define NUM_MIDI        16
#define NUM_G           64              /* use all channels */
#define NUM_FXSENDS     4


#define TMEMSIZE        256*1024
#define TMEMSIZEREG     4

#define IP_TO_CP(ip) ((ip == 0) ? 0 : (((0x00001000uL | (ip & 0x00000FFFL)) << (((ip >> 12) & 0x000FL) + 4)) & 0xFFFF0000uL))

/**** Bit Patterns ****/
#define ENV_ON          0x80            /* envelope on */
#define ENV_OFF         0x00            /* envelope off */
#define STEREO          0x00008000L     /* stereo on even channel */
#define LOCKED          0x00008000L     /* locked on odd channel */
#define BYTESIZE        0x01000000L     /* byte sound memory */
#define ROM0            0x00000000L     /* interpolation ROM 0 */
#define ROM1            0x02000000L     /* interpolation ROM 1 */
#define ROM2            0x04000000L     /* interpolation ROM 2 */
#define ROM3            0x06000000L     /* interpolation ROM 3 */
#define ROM4            0x08000000L     /* interpolation ROM 4 */
#define ROM5            0x0A000000L     /* interpolation ROM 5 */
#define ROM6            0x0C000000L     /* interpolation ROM 6 */
#define ROM7            0x0E000000L     /* interpolation ROM 7 */

/**** Interrupt enable ****/
#define ENB_PCI         0x00000800L     /* PCI error */
#define ENB_VINC        0x00000400L     /* volume inc */
#define ENB_VDEC        0x00000200L     /* volume dec */
#define ENB_MUTE        0x00000100L     /* volume mute */
#define ENB_ADC         0x00000040L     /* ADC recording */
#define ENB_TIMER       0x00000004L     /* timer */
#define ENB_TX          0x00000002L     /* TX fifo empty */
#define ENB_RX          0x00000001L     /* RX fifo data */
#define ENB_MIC         0x00000080L     /* MIC recording */
#define ENB_EFX         0x00000020L     /* Efx recording */
#define ENB_GPSCS       0x00000010L     /* SPDIF channel status */
#define ENB_CDCS        0x00000008L     /* CD in channel status */
#define ENB_SRC         0x00002000L     /* sample rate tracker */
#define ENB_DSP         0x00001000L     /* DSP interrupt */

/**** Interrupt status ****/
#define INT_PCI         0x00200000L     /* PCI error */
#define INT_VINC        0x00100000L     /* volume inc */
#define INT_VDEC        0x00080000L     /* volume dec */
#define INT_MUTE        0x00040000L     /* volume mute */
#define INT_FULL        0x00008000L     /* ADC buffer full */
#define INT_HALF        0x00004000L     /* ADC half full */
#define INT_TIMER       0x00000200L     /* timer */
#define INT_TX          0x00000100L     /* TX fifo empty */
#define INT_RX          0x00000080L     /* RX fifo data */
#define INT_LOOP        0x00000040L     /* channel loop */
#define INT_CIN         0x0000003FL     /* channel interrupt loop number*/
#define INT_MODEM       0x00400000L     /* host modem */
#define INT_MICFULL     0x00020000L     /* MIC buffer full */
#define INT_MICHALF     0x00010000L     /* MIC half full */
#define INT_EFXFULL     0x00002000L     /* Efx buffer full */
#define INT_EFXHALF     0x00001000L     /* Efx half full */
#define INT_GSPCS       0x00000800L     /* SPDIF status changed */
#define INT_CDCS        0x00000400L     /* CD in status changed */
#define INT_DSP         0x00800000L     /* DSP interrupt */

/**** Recording controls ****/
#define ADCLE           0x00000008L     /* record left channel */
#define ADCRE           0x00000010L     /* record right channel */
#define ADC48K          0x00000000L     /* record at 48kHz */
#define ADC44K          0x00000001L     /* record at 44.1kHz */
#define ADC32K          0x00000002L     /* record at 32kHz */
#define ADC24K          0x00000003L     /* record at 24kHz */
#define ADC22K          0x00000004L     /* record at 22.05kHz */
#define ADC16K          0x00000005L     /* record at 16kHz */
#define ADC11K          0x00000006L     /* record at 11.025kHz */
#define ADC8K           0x00000007L     /* record at 8kHz */

/**** SPDIF output controls ****/
#define SPDIF_DEFAULT   0x00108500L     /* default */
#define SPDIF_CP        0x00000004L     /* copy permitted */
#define SPDIF_AES       0x00000001L     /* professional AES3-1992 */
#define SPDIF_44K       0x00000000L     /* 44.1kHz */
#define SPDIF_48K       0x02000000L     /* 48kHz */
#define SPDIF_32K       0x03000000L     /* 32kHz */
#define SPDIF_CNL       0x00100000L     /* left of 2 */
#define SPDIF_SRC       0x00000000L     /* unspecified */
#define SPDIF_CAT       0x00000500L     /* music synthesizer */
#define SPDIF_ORG       0x00008000L     /* original */

/**** Per Chanel G Registers ****/
#define CPF             0x00000000      /* Current Pitch and Fraction */
#define CPF_CP          0x1F001000
#define CPF_S           0x0F000F00
#define CPF_F           0x0D000000
#define PTRX            0x00010000      /* Pitch Target, Reverb send, auX data */
#define PTRX_PT         0x1F011000
#define PTRX_R          0x0F010800
#define PTRX_X          0x07010000
#define CVCF            0x00020000      /* Current Volume, Filter Cutoff */
#define CVCF_CV         0x1F021000
#define CVCF_CF         0x0F020000
#define VTFT            0x00030000      /* Volume Target, Filter cutoff Target */
#define VTFT_VT         0x1F031000
#define VTFT_FT         0x0F030000
#define Z1              0x00050000      /* delay memory */
#define Z2              0x00040000      /* delay memory */
#define PSST            0x00060000      /* Pan Send, STart address */
#define PSST_PS         0x1F061800
#define PSST_ST         0x17060000
#define CSL             0x00070000      /* Chorus Send, Loop address */
#define CSL_CS          0x1F071800
#define CSL_L           0x17070000
#define CCCA            0x00080000      /* Coef, Cntl, Current address */
#define CCCA_COEF       0x1F081C00
#define CCCA_ROM        0x1B081900
#define CCCA_BYTE       0x18081800
#define CCCA_CA         0x17080000

/**** Per Chanel Envelope Registers ****/
#define ENVVOL          0x00100040      /* ENVelope VOLume Value */
#define ATKHLDV         0x00110040      /* volume envelope ATtack and HoLD Value */
#define ATKHLDV_HLD     0x0E110840
#define ATKHLDV_ATK     0x06110040
#define DCYSUSV         0x00120040      /* volume envelope DeCaY and SUStain Value */
#define DCYSUSV_SUS     0x0E120840
#define DCYSUSV_DCY     0x06120040
#define LFOVAL1         0x00130040      /* LFO #1 VALue */
#define ENVVAL          0x00140040      /* pitch/filter ENVelope VALue */
#define ATKHLD          0x00150040      /* pitch/filter envelope ATacK and Hold */
#define ATKHLD_HLD      0x0E150840
#define ATKHLD_ATK      0x06150040
#define DCYSUS          0x00160040      /* pitch/filter envelope Decay and Sustain */
#define DCYSUS_SUS      0x0E160840
#define DCYSUS_DCY      0x06160040
#define LFOVAL2         0x00170040      /* LFO #2 VALue */

#define IP              0x00180040      /* Initial Pitch */
#define IFATN           0x00190040      /* Initial Filter cutoff And ATteNuation */
#define IFATN_IF        0x0F190840
#define IFATN_ATN       0x07190040
#define PEFE            0x001A0040      /* Pitch Envelope and Filter Enveolpe amount */
#define PEFE_PE         0x0F1A0840
#define PEFE_FE         0x071A0040
#define FMMOD           0x001B0040      /* FM from LFO#1 and filter MOD amount */
#define FMMOD_FM        0x0F1B0840
#define FMMOD_MOD       0x071B0040
#define TREMFRQ         0x001C0040      /* TREMelo amount and lfo#1 FReQuency */
#define TREMFRQ_TREM    0x0F1C0840
#define TREMFRQ_FRQ     0x071C0040
#define FM2FRQ2         0x001D0040      /* FM from lfo#2 amount and lfo#2 FReQuency */
#define FM2FRQ2_FM2     0x0F1D0840
#define FM2FRQ2_FRQ2    0x071D0040
#define TEMPENV         0x001E0040      /* Tempory Envelope Register */

/**** 8010 registers ****/
#define PTB             0x00400000L     /* Page Table Base register */

/**** Registers not using PTR */
#define WC              0x00000010L     /* Wall Clock */
#define HCFG            0x00000014L     /* Hardware ConFig register */
#define IPR             0x00000008L     /* Interrupt Pending Register */
#define INTE            0x0000000CL     /* INTerrupt Enable register */
#define TIMR            0x0000001AL     /* TIMeR terminal count */
#define MUDATA          0x00000018L     /* MpU401 DATA */
#define MUSTAT          0x00000019L     /* MpU401 STATus */
#define AC97D           0x0000001CL     /* AC97 Data */
#define AC97A           0x0000001EL     /* AC97 Address */

/**** Per Channel Cache Registers ****/
#define MAPA            0x000C0000L     /* cache MAP A */
#define MAPB            0x000D0000L     /* cache MAP B */
#define CDATA           0x00200000L     /* Cache DATA */
#define CCTRL           0x00090000L     /* Cache ConTRoL */
#define CLOOP           0x000A0040L     /* Cache LOOP */

/**** SPDIF Output Status ****/
#define SPCS0           0x00540000L     /* SPdif output Channel Status 0 */
#define SPCS1           0x00550000L     /* SPdif output Channel Status 1 */
#define SPCS2           0x00560000L     /* SPdif output Channel Status 2 */
#define SPBYPASS        0x005E0000L     /* SPdif BYPASS mode */

/**** Voice Channel Registers ****/
#define CLIE            0x00580000L     /* Channel Loop Interrupt Enable */
#define CLIEL           0x00580000L     /* Channel Loop Interrupt Enable Low */
#define CLIEH           0x00590000L     /* Channel Loop Interrupt Enable High */
#define CLIP            0x005A0000L     /* Channel Loop Interrupt Pending */
#define CLIPL           0x005A0000L     /* Channel Loop Interrupt Pending Low */
#define CLIPH           0x005B0000L     /* Channel Loop Interrupt Pending High */
#define SOLEL           0x005C0000L     /* Stop On Loop Enable Low */
#define SOLEH           0x005D0000L     /* Stop On Loop Enable High */

/**** SRC and Channel Status Registers ****/
#define CDCS            0x00500000L     /* CD-ROM digital Channel Status */
#define CDSRCS          0x00600000L     /* CD-ROM Sample Rate Converter Status */
#define GPSCS           0x00510000L     /* General Purpose Spdif Channel Status */
#define GPSRCS          0x00610000L     /* Generap Purpose Spdif sample Rate Converter Status */
#define ZVSRCS          0x00620000L     /* ZVideo Sample Rate Converter Status */

/**** ADC Registers ****/
#define ADCCR           0x00420080L     /* ADC Control Registers */
#define ADCBA           0x00460000L     /* ADC Buffer Address */
#define ADCBS           0x004A0080L     /* ADC Buffer Size */
#define ADCIDX          0x00640000L     /* ADC recording buffer InDeX */

/**** MIC Recording Registers ****/
#define MICBA           0x00450000L     /* MICrophone Buffer Address */
#define MICBS           0x00490080L     /* MICrophone Buffer Size */
#define MICIDX          0x00630000L     /* MICrophone recording buffer InDeX */

/**** FX Recording Registers ****/
#define FXBA            0x00470000L     /* FX Buffer Address */
#define FXBS            0x004B0080L     /* FX Buffer Size */
#define FXIDX           0x00650000L     /* FX recording buffer InDeX */


/**** Tank Memory Registers ****/
#define TMBA            0x00410000L     /* Tank Memory Base Address */
#define TMBS            0x00440080L     /* Tank Memory Buffer Size */
#define TMDR            0x00000200L     /* Tank Memory Data Register */
#define TMAR            0x00000300L     /* Tank Memory Address Register */


/* ------------------- STRUCTURES -------------------- */

typedef struct snd_stru_emu10k1 emu10k1_t;

struct snd_stru_emu10k1 {
	snd_dma_t * dma1ptr;	/* DAC1 */
	snd_dma_t * dma2ptr;	/* ADC */
	snd_irq_t * irqptr;

	unsigned short port;	/* I/O port number */
	int APS: 1;		/* APS flag */
	unsigned int revision;	/* chip revision */
	unsigned int serial;	/* serial number */
	unsigned int ecard_ctrl; /* ecard control bits */

	ac97_t *ac97;

	struct pci_dev *pci;
	snd_card_t *card;
	snd_pcm_t *pcm;
	snd_kmixer_t *mixer;
	snd_rawmidi_t *rmidi;

	spinlock_t reg_lock;
	spinlock_t emu_lock;
	snd_info_entry_t *proc_entry;
};

int snd_emu10k1_create(snd_card_t * card,
		       struct pci_dev *pci,
		       snd_dma_t * dma1ptr,
		       snd_dma_t * dma2ptr,
		       snd_irq_t * irqptr,
		       emu10k1_t ** remu);
int snd_emu10k1_free(emu10k1_t * emu);
// void snd_emu10k1_interrupt(emu10k1_t * emu, unsigned short status);

int snd_emu10k1_pcm(emu10k1_t * emu, int device, snd_pcm_t ** rpcm);
int snd_emu10k1_mixer(emu10k1_t * emu, int device, snd_pcm_t * pcm, snd_kmixer_t ** rmixer);
int snd_emu10k1_midi(emu10k1_t * emu, int device, snd_rawmidi_t ** rrawmidi);

/* I/O functions */
unsigned int snd_emu10k1_synth_read(emu10k1_t * emu, unsigned int reg);
void snd_emu10k1_synth_write(emu10k1_t *emu, unsigned int reg, unsigned int ddata);
unsigned int snd_emu10k1_efx_read(emu10k1_t *emu, unsigned int reg);
void snd_emu10k1_efx_write(emu10k1_t *emu, unsigned int reg, unsigned int ddata);
void snd_emu10k1_intr_enable(emu10k1_t *emu, unsigned int intrenb);
void snd_emu10k1_intr_disable(emu10k1_t *emu, unsigned int intrenb);
void snd_emu10k1_voice_intr_enable(emu10k1_t *emu, unsigned int voicenum);
void snd_emu10k1_voice_set_loop_stop(emu10k1_t *emu, unsigned int voicenum);
void snd_emu10k1_voice_clear_loop_stop(emu10k1_t *emu, unsigned int voicenum);
void snd_emu10k1_voice_intr_disable(emu10k1_t *emu, unsigned int voicenum);
void snd_emu10k1_wait(emu10k1_t *emu, unsigned int wait);
unsigned short snd_emu10k1_ac97_read(void *private_data, unsigned short reg);
void snd_emu10k1_ac97_write(void *private_data, unsigned short reg, unsigned short data);

#endif	/* __EMU10K1_H */
