#ifndef __TRID4DWAVE_H
#define __TRID4DWAVE_H

/*
 *  audio@tridentmicro.com
 *  Fri Feb 19 15:55:28 MST 1999
 *  Definitions for Trident 4DWave DX/NX chips
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

#include "pcm1.h"
#include "mixer.h"
#include "midi.h"
#include "mpu401.h"
#include "ac97_codec.h"
#include "seq_midi_emul.h"
#include "seq_device.h"
#ifndef ALSA_BUILD
//#include <linux/ainstr_iw.h>
//#include <linux/ainstr_gf1.h>
#include <linux/ainstr_simple.h>
#else
//#include "ainstr_iw.h"
//#include "ainstr_gf1.h"
#include "ainstr_simple.h"
#endif

#ifndef PCI_VENDOR_ID_TRIDENT
#define PCI_VENDOR_ID_TRIDENT          0x1023
#endif
#ifndef PCI_DEVICE_ID_TRIDENT_4DWAVE_DX 
#define PCI_DEVICE_ID_TRIDENT_4DWAVE_DX 0x2000
#endif
#ifndef PCI_DEVICE_ID_TRIDENT_4DWAVE_NX 
#define PCI_DEVICE_ID_TRIDENT_4DWAVE_NX 0x2001
#endif

/* Trident chipsets have 1GB memory limit */
#ifdef __alpha__
#define TRIDENT_DMA_TYPE        SND_DMA_TYPE_PCI_16MB
#define TRIDENT_GFP_FLAGS	GFP_DMA
#else
#define TRIDENT_DMA_TYPE        SND_DMA_TYPE_PCI
#if defined(__i386__) && !defined(CONFIG_1GB)
#define TRIDENT_GFP_FLAGS	GFP_DMA
#else
#define TRIDENT_GFP_FLAGS	0
#endif
#endif

#define SND_SEQ_DEV_TRIDENT			"synth-trident"

#define SND_TRIDENT_VOICE_TYPE_PCM		0
#define SND_TRIDENT_VOICE_TYPE_SYNTH		1
#define SND_TRIDENT_VOICE_TYPE_MIDI		2

#define SND_TRIDENT_VFLG_RUNNING		(1<<0)

/*
 * Direct registers
 */

#define FALSE 0
#define TRUE  1

#define TRID_REG(trident, x) ((trident)->port + (x))

#define CHANNEL_REGS    5
#define CHANNEL_START   0xe0   // The first bytes of the contiguous register space.

#define ID_4DWAVE_DX        0x2000
#define ID_4DWAVE_NX        0x2001

#define IWriteAinten( x ) \
        {int i; \
         for( i= 0; i < ChanDwordCount; i++) \
         outl((x)->lpChAinten[i], TRID_REG(trident, (x)->lpAChAinten[i]));}

#define IReadAinten( x ) \
        {int i; \
         for( i= 0; i < ChanDwordCount; i++) \
         (x)->lpChAinten[i] = inl(TRID_REG(trident, (x)->lpAChAinten[i]));}

#define ReadAint( x ) \
        IReadAint( x ) 

#define WriteAint( x ) \
        IWriteAint( x ) 

#define IWriteAint( x ) \
        {int i; \
         for( i= 0; i < ChanDwordCount; i++) \
         outl((x)->lpChAint[i], TRID_REG(trident, (x)->lpAChAint[i]));}

#define IReadAint( x ) \
        {int i; \
         for( i= 0; i < ChanDwordCount; i++) \
         (x)->lpChAint[i] = inl(TRID_REG(trident, (x)->lpAChAint[i]));}


// Register definitions

// Global registers

// T2 legacy dma control registers.
#define LEGACY_DMAR0                0x00  // ADR0
#define LEGACY_DMAR4                0x04  // CNT0
#define LEGACY_DMAR11               0x0b  // MOD 
#define LEGACY_DMAR15               0x0f  // MMR 

#define T4D_START_A		     0x80
#define T4D_STOP_A		     0x84
#define T4D_DLY_A		     0x88
#define T4D_SIGN_CSO_A		     0x8c
#define T4D_CSPF_A		     0x90
#define T4D_CEBC_A		     0x94
#define T4D_AINT_A		     0x98
#define T4D_AINTEN_A		     0x9c
#define T4D_LFO_GC_CIR               0xa0
#define T4D_MUSICVOL_WAVEVOL         0xa8
#define T4D_SBDELTA_DELTA_R          0xac
#define T4D_MISCINT                  0xb0
#define T4D_START_B                  0xb4
#define T4D_STOP_B                   0xb8
#define T4D_SBBL_SBCL                0xc0
#define T4D_SBCTRL_SBE2R_SBDD        0xc4
#define T4D_AINT_B                   0xd8
#define T4D_AINTEN_B                 0xdc

// MPU-401 UART
#define T4D_MPU401_BASE             0x20
#define T4D_MPUR0                   0x20
#define T4D_MPUR1                   0x21
#define T4D_MPUR2                   0x22
#define T4D_MPUR3                   0x23

// S/PDIF Registers
#define NX_SPCTRL_SPCSO             0x24
#define NX_SPLBA                    0x28
#define NX_SPESO                    0x2c
#define NX_SPCSTATUS                0x64

// Channel Registers

#define CH_DX_CSO_ALPHA_FMS         0xe0
#define CH_DX_ESO_DELTA             0xe8
#define CH_DX_FMC_RVOL_CVOL         0xec

#define CH_NX_DELTA_CSO             0xe0
#define CH_NX_DELTA_ESO             0xe8
#define CH_NX_ALPHA_FMS_FMC_RVOL_CVOL 0xec

#define CH_LBA                      0xe4
#define CH_GVSEL_PAN_VOL_CTRL_EC    0xf0

// AC-97 Registers

#define DX_ACR0_AC97_W              0x40
#define DX_ACR1_AC97_R              0x44
#define DX_ACR2_AC97_COM_STAT       0x48

#define NX_ACR0_AC97_COM_STAT       0x40
#define NX_ACR1_AC97_W              0x44
#define NX_ACR2_AC97_R_PRIMARY      0x48
#define NX_ACR3_AC97_R_SECONDARY    0x4c

#define AC97_SIGMATEL_DAC2INVERT    0x6E
#define AC97_SIGMATEL_BIAS1         0x70
#define AC97_SIGMATEL_BIAS2         0x72
#define AC97_SIGMATEL_CIC1          0x76
#define AC97_SIGMATEL_CIC2          0x78

typedef struct tChannelControl
{
    // register data
    unsigned int *  lpChStart;
    unsigned int *  lpChStop;
    unsigned int *  lpChAint;
    unsigned int *  lpChAinten;

    // register addresses
    unsigned int *  lpAChStart;
    unsigned int *  lpAChStop;
    unsigned int *  lpAChAint;
    unsigned int *  lpAChAinten;

}CHANNELCONTROL, *LPCHANNELCONTROL;

typedef struct snd_stru_trident trident_t;
typedef struct snd_trident_stru_voice snd_trident_voice_t;
typedef struct snd_stru_trident_pcm_mixer snd_trident_pcm_mixer_t;

typedef struct {
	void (*sample_start)(trident_t *gus, snd_trident_voice_t *voice, snd_seq_position_t position);
	void (*sample_stop)(trident_t *gus, snd_trident_voice_t *voice, snd_seq_stop_mode_t mode);
	void (*sample_freq)(trident_t *gus, snd_trident_voice_t *voice, snd_seq_frequency_t freq);
	void (*sample_volume)(trident_t *gus, snd_trident_voice_t *voice, snd_seq_ev_volume *volume);
	void (*sample_loop)(trident_t *card, snd_trident_voice_t *voice, snd_seq_ev_loop *loop);
	void (*sample_pos)(trident_t *card, snd_trident_voice_t *voice, snd_seq_position_t position);
	void (*sample_private1)(trident_t *card, snd_trident_voice_t *voice, unsigned char *data);
} snd_trident_sample_ops_t;

typedef struct {
	snd_midi_channel_set_t * chset;
	trident_t * trident;
	int mode;		/* operation mode */
	int client;		/* sequencer client number */
	int port;		/* sequencer port number */
	int midi_has_voices: 1;
} snd_trident_port_t;

struct snd_trident_stru_voice {
	unsigned int number;
	int use: 1,
	    pcm: 1,
	    synth:1,
	    midi: 1;
	unsigned int flags;
	unsigned char client;
	unsigned char port;
	unsigned char index;

	snd_seq_instr_t instr;
	snd_trident_sample_ops_t *sample_ops;

	/* channel parameters */
	unsigned short Delta;		/* 16 bits */
	unsigned char Vol;		/* 8 bits */
	unsigned char Pan;		/* 7 bits */
	unsigned char GVSel;		/* 1 bit */
	unsigned char RVol;		/* 7 bits */
	unsigned char CVol;		/* 7 bits */
	unsigned char FMC;		/* 2 bits */
	unsigned int LBA;		/* 30 bits */
	unsigned char CTRL;		/* 4 bits */
	unsigned short EC;		/* 12 bits */
	unsigned short Alpha_FMS;	/* 16 bits */
	unsigned int CSO;		/* 24 bits (16 on DX) */
	unsigned int ESO;		/* 24 bits (16 on DX) */

	unsigned int negCSO;	/* nonzero - use negative CSO */


	/* PCM data */

	trident_t *trident;
	snd_pcm_subchn_t *subchn;
	snd_pcm1_subchn_t *subchn1;
	int running;
	int eso;                /* final ESO value for channel */
	int count;              /* count between interrupts */
	int csoint;             /* CSO value for next expected interrupt */

	/* --- */

	void *private_data;
	void (*private_free)(void *private_data);
};

struct snd_stru_4dwave {

	int seq_client;

	snd_trident_port_t seq_ports[4];
	snd_simple_ops_t simple_ops;
	snd_seq_kinstr_list_t *ilist;

	snd_trident_voice_t voices[64];	

	int ChanSynthCount;		/* number of allocated synth channels */
	int max_size;			/* maximum synth memory size in bytes */
	int current_size;		/* current allocated synth mem in bytes */

};

typedef struct {
	snd_kmixer_group_control_t *group;
	snd_mixer_volume1_control_t *vol;
	snd_mixer_pan_control1_control_t *pan;
	snd_mixer_volume1_control_t *rvol;
	snd_mixer_volume1_control_t *cvol;
} snd_trident_mixer_ops_t;

struct snd_stru_trident_pcm_mixer {
	int number;			/* hardware index 32 - 63 */

	snd_kmixer_group_t *group;
	snd_kmixer_element_t *me_pcm;
	snd_kmixer_element_t *me_vol_vol;
	snd_kmixer_element_t *me_pan;
	snd_kmixer_element_t *me_vol_rvol;
	snd_kmixer_element_t *me_vol_cvol;
};

struct snd_stru_trident {
	snd_dma_t * dma1ptr;	/* DAC Channel */
	snd_dma_t * dma2ptr;	/* ADC Channel */
	snd_irq_t * irqptr;

	int isNX;		/* NX chip present */

        unsigned char  bDMAStart;

	unsigned int port;
	unsigned int midi_port;

        LPCHANNELCONTROL ChRegs;
        int ChanDwordCount;

	unsigned char spdif_ctrl;
	unsigned int ac97_ctrl;
        
        unsigned int ChanMap[2];	/* allocation map for hardware channels */
        
        int ChanPCM;			/* max number of PCM channels */
	int ChanPCMcnt;			/* actual number of PCM channels */

	struct snd_stru_4dwave synth;	/* synth specific variables */

	spinlock_t event_lock;
	spinlock_t voice_alloc;

	struct pci_dev *pci;
	snd_card_t *card;
	snd_pcm_t *pcm;		/* ADC/DAC PCM */
	snd_kmixer_t *mixer;
	snd_rawmidi_t *rmidi;
	snd_seq_device_t *seq_dev;

	unsigned int musicvol_wavevol;

	snd_trident_pcm_mixer_t pcm_mixer[32];

	snd_kmixer_element_t *me_wave;
	snd_kmixer_element_t *me_vol_wave;
	snd_kmixer_element_t *me_music;
	snd_kmixer_element_t *me_vol_music;
	snd_kmixer_element_t *me_reverb;
	snd_kmixer_element_t *me_sw2_reverb;

	spinlock_t reg_lock;
	snd_info_entry_t *proc_entry;
};

trident_t *snd_trident_create(snd_card_t * card, struct pci_dev *pci,
			      snd_dma_t * dma1ptr,
			      snd_dma_t * dma2ptr,
			      snd_irq_t * irqptr,
			      int pcm_channels,
			      int max_wavetable_size);
void snd_trident_free(trident_t * trident);
void snd_trident_interrupt(trident_t * trident);

snd_pcm_t *snd_trident_pcm(trident_t * trident);
snd_kmixer_t *snd_trident_mixer(trident_t * trident);
void snd_trident_rawmidi(trident_t * trident, mpu401_t * mpu);
int snd_trident_attach_synthesizer(trident_t * trident);
int snd_trident_detach_synthesizer(trident_t * trident);
snd_trident_voice_t *snd_trident_alloc_voice(trident_t * trident, int type, int client, int port);
void snd_trident_free_voice(trident_t * trident, snd_trident_voice_t *voice);
int snd_trident_write_voice_regs(trident_t * trident, unsigned int Channel,
			 unsigned int LBA, unsigned int CSO, unsigned int ESO,
			 unsigned int DELTA, unsigned int ALPHA_FMS, unsigned int FMC_RVOL_CVOL,
			 unsigned int GVSEL, unsigned int PAN, unsigned int VOL,
			 unsigned int CTRL, unsigned int EC);
void snd_trident_start_voice(trident_t * trident, unsigned int HwChannel);
void snd_trident_stop_voice(trident_t * trident, unsigned int HwChannel);
void snd_trident_enable_voice_irq(trident_t * trident, unsigned int HwChannel);
void snd_trident_disable_voice_irq(trident_t * trident, unsigned int HwChannel);
void snd_trident_clear_voices(trident_t * trident, unsigned short v_min, unsigned short v_max);
#endif				/* __TRID4DWAVE_H */

