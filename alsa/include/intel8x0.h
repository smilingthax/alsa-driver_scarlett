#ifndef __INTEL8X0_H
#define __INTEL8X0_H

/*
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
 *  Definitions for Intel 82801AA,82901AB,i810,i820,i830,i840,MX440 chips
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
#include "rawmidi.h"
#include "ac97_codec.h"

#ifndef PCI_DEVICE_ID_INTEL_82801
#define PCI_DEVICE_ID_INTEL_82801	0x2415
#endif
#ifndef PCI_DEVICE_ID_INTEL_82901
#define PCI_DEVICE_ID_INTEL_82901	0x2425
#endif
#ifndef PCI_DEVICE_ID_INTEL_440MX
#define PCI_DEVICE_ID_INTEL_440MX	0x7195
#endif

/*
 *  Direct registers
 */

#define ICHREG(ice, x) ((ice)->bmport + ICH_REG_##x)

/* capture block */
#define ICH_REG_PI_BDBAR		0x00	/* dword - buffer descriptor list base address */
#define ICH_REG_PI_CIV			0x04	/* byte - current index value */
#define ICH_REG_PI_LVI			0x05	/* byte - last valid index */
#define   ICH_REG_LVI_MASK		0x1f
#define ICH_REG_PI_SR			0x06	/* byte - status register */
#define   ICH_FIFOE			0x10	/* FIFO error */
#define   ICH_BCIS			0x08	/* buffer completion interrupt status */
#define   ICH_LVBCI			0x04	/* last valid buffer completion interrupt */
#define   ICH_CELV			0x02	/* current equals last valid */
#define   ICH_DCH			0x01	/* DMA controller halted */
#define ICH_REG_PI_PICB			0x08	/* word - position in current buffer */
#define ICH_REG_PI_PIV			0x0a	/* byte - prefetched index value */
#define   ICH_REG_PIV_MASK		0x1f	/* mask */
#define ICH_REG_PI_CR			0x0b	/* byte - control register */
#define   ICH_IOCE			0x10	/* interrupt on completion enable */
#define   ICH_FEIE			0x08	/* fifo error interrupt enable */
#define   ICH_LVBIE			0x04	/* last valid buffer interrupt enable */
#define   ICH_RESETREGS			0x02	/* reset busmaster registers */
#define   ICH_STARTBM			0x01	/* start busmaster operation */
/* playback block */
#define ICH_REG_PO_BDBAR		0x10	/* dword - buffer descriptor list base address */
#define ICH_REG_PO_CIV			0x14	/* byte - current index value */
#define ICH_REG_PO_LVI			0x15	/* byte - last valid command */
#define ICH_REG_PO_SR			0x16	/* byte - status register */
#define ICH_REG_PO_PICB			0x18	/* word - position in current buffer */
#define ICH_REG_PO_PIV			0x1a	/* byte - prefetched index value */
#define ICH_REG_PO_CR			0x1b	/* byte - control register */
/* mic capture block */
#define ICH_REG_MC_BDBAR		0x20	/* dword - buffer descriptor list base address */
#define ICH_REG_MC_CIV			0x24	/* byte - current index value */
#define ICH_REG_MC_LVI			0x25	/* byte - last valid command */
#define ICH_REG_MC_SR			0x26	/* byte - status register */
#define ICH_REG_MC_PICB			0x28	/* word - position in current buffer */
#define ICH_REG_MC_PIV			0x2a	/* byte - prefetched index value */
#define ICH_REG_MC_CR			0x2b	/* byte - control register */
/* global block */
#define ICH_REG_GLOB_CNT		0x2c	/* dword - global control */
#define   ICH_SRIE		0x00000020	/* secondary resume interrupt enable */
#define   ICH_PRIE		0x00000010	/* primary resume interrupt enable */
#define   ICH_ACLINK		0x00000008	/* AClink shut off */
#define   ICH_AC97WARM		0x00000004	/* AC'97 warm reset */
#define   ICH_AC97COLD		0x00000002	/* AC'97 cold reset */
#define   ICH_GIE		0x00000001	/* GPI interrupt enable */
#define ICH_REG_GLOB_STA		0x30	/* dword - global status */
#define   ICH_MD3		0x00020000	/* modem power down semaphore */
#define   ICH_AD3		0x00010000	/* audio power down semaphore */
#define   ICH_RCS		0x00008000	/* read completion status */
#define   ICH_BIT3		0x00004000	/* bit 3 slot 12 */
#define   ICH_BIT2		0x00002000	/* bit 2 slot 12 */
#define   ICH_BIT1		0x00001000	/* bit 1 slot 12 */
#define   ICH_SRI		0x00000800	/* secondary resume interrupt */
#define   ICH_PRI		0x00000400	/* primary resume interrupt */
#define   ICH_SCR		0x00000200	/* secondary codec ready */
#define   ICH_PCR		0x00000100	/* primary codec ready */
#define   ICH_MCINT		0x00000080	/* MIC capture interrupt */
#define   ICH_POINT		0x00000040	/* playback interrupt */
#define   ICH_PIINT		0x00000020	/* capture interrupt */
#define   ICH_MOINT		0x00000004	/* modem playback interrupt */
#define   ICH_MIINT		0x00000002	/* modem capture interrupt */
#define   ICH_GSCI		0x00000001	/* GPI status change interrupt */
#define ICH_REG_ACC_SEMA		0x34	/* byte - codec write semaphore */
#define   ICH_CAS			0x01	/* codec access semaphore */

/*
 *  
 */

typedef struct {
	unsigned int reg_offset;
	unsigned int *bdbar;
	unsigned int rates;
        snd_pcm_subchn_t *subchn;
        unsigned long physbuf;
        unsigned int size;
        unsigned int fragsize;
        unsigned int fragsize1;
        unsigned int position;
        int frags;
        int lvi;
        int lvi_frag;
	int ack;
	int ack_reload;
} ichdev_t;

typedef struct snd_stru_intel8x0 intel8x0_t;

struct snd_stru_intel8x0 {
	snd_dma_t * dma_pbk;	/* playback */
	snd_dma_t * dma_cap;	/* capture */
	snd_dma_t * dma_mic;	/* mic capture */
	snd_irq_t * irqptr;

	unsigned int port;
	unsigned int bmport;

	struct pci_dev *pci;
	snd_card_t *card;

	snd_pcm_t *pcm;
	snd_pcm_t *pcm_mic;
	ichdev_t playback;
	ichdev_t capture;
	ichdev_t capture_mic;

	ac97_t *ac97;
	unsigned short ac97_ext_id;
	snd_kmixer_t *mixer;

	spinlock_t reg_lock;
	snd_info_entry_t *proc_entry;

	unsigned int *bdbars;
};

int snd_intel8x0_create(snd_card_t * card,
		       struct pci_dev *pci,
		       snd_dma_t * dma_pbk,
		       snd_dma_t * dma_cap,
		       snd_dma_t * dma_mic,
		       snd_irq_t * irqptr,
		       intel8x0_t ** codec);
int snd_intel8x0_free(intel8x0_t * codec);
void snd_intel8x0_interrupt(intel8x0_t * codec);

int snd_intel8x0_pcm(intel8x0_t * codec, int device, snd_pcm_t ** rpcm);
int snd_intel8x0_pcm_mic(intel8x0_t * codec, int device, snd_pcm_t ** rpcm);
int snd_intel8x0_mixer(intel8x0_t * codec, int device, int pcm_count, int *pcm_devs, snd_kmixer_t ** rmixer);

#endif				/* __INTEL8X0_H */
