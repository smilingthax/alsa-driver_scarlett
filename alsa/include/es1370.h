#ifndef __ES1370_H
#define __ES1370_H

/*
 *  Copyright (c) by Jaroslav Kysela <perex@jcu.cz>
 *  Definitions for Ensoniq ES1370/1371 chips
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

#include "sndpci.h"
#include "pcm1.h"
#include "mixer.h"
#include "midi.h"

/*
 * Direct registers
 */

#define ES_REG( ensoniq, x ) ( (ensoniq) -> port + ES_REG_##x )

#define ES_REG_CONTROL	0x00	/* R/W: Interrupt/Chip select control register */ 
#define   ES_1370_ADC_STOP	(1<<31)	  /* disable record buffer transfers */
#define   ES_1370_XCTL1 	(1<<30)	  /* general purpose output bit */
#define   ES_1371_JOY_ASEL(o)	(((o)&0x03)<<24) /* joystick port mapping */
#define   ES_1371_JOY_ASELM	(0x03<<24)	 /* mask for above */
#define   ES_1371_GPIO_IN(i)	(((i)>>20)&0x0f) /* GPIO in [3:0] pins - R/O */
#define   ES_1370_PCLKDIV(o)	(((o)&0x1fff)<<16) /* clock divide ratio for DAC2 */
#define   ES_1370_PCLKDIVM	((0x1fff)<<16)   /* mask for above */
#define   ES_1371_GPIO_OUT(o)	(((o)&0x0f)<<16) /* GPIO out [3:0] pins - W/R */
#define   ES_1371_GPIO_OUTM     (0x0f<<16)       /* mask for above */
#define   ES_MSFMTSEL		(1<<15)	  /* MPEG serial data format; 0 = SONY, 1 = I2S */
#define   ES_1370_M_SBB		(1<<14)	  /* clock source for DAC - 0 = clock generator; 1 = MPEG clocks */
#define   ES_1371_SYNC_RES	(1<<14)	  /* Warm AC97 reset */
#define   ES_1370_WTSRSEL(o)	(((o)&0x03)<<12) /* fixed frequency clock for DAC1 */
#define   ES_1370_WTSRSELM	(0x03<<12)	 /* mask for above */
#define   ES_1371_ADC_STOP	(1<<13)	  /* disable CCB transfer record information */
#define   ES_1371_PWR_INTRM	(1<<12)   /* power level change interrupts enable */
#define   ES_1370_DAC_SYNC	(1<<11)	  /* DAC's are synchronous */
#define   ES_1371_M_CB		(1<<11)	  /* record clock source; 0 = ADC; 1 = I2S */
#define   ES_CCB_INTRM		(1<<10)   /* CCB voice interrupts enable */
#define   ES_1370_M_CB		(1<<9)	  /* record clock source; 0 = ADC; 1 = MPEG */
#define   ES_1370_XCTL0		(1<<8)	  /* generap purpose output bit */
#define   ES_1371_PDLEV(o)	(((o)&0x03)<<8)  /* current power down level */
#define   ES_1371_PDLEVM	(0x03<<8)        /* mask for above */
#define   ES_BREQ		(1<<7)	  /* memory bus request enable */
#define   ES_DAC1_EN		(1<<6)	  /* DAC1 playback channel enable */
#define   ES_DAC2_EN		(1<<5)	  /* DAC2 playback channel enable */
#define   ES_ADC_EN		(1<<4)    /* ADC record channel enable */
#define   ES_UART_EN		(1<<3)	  /* UART enable */
#define   ES_JYSTK_EN		(1<<2)	  /* Joystick module enable */
#define   ES_1370_CDC_EN	(1<<1)	  /* Codec interface enable */
#define   ES_1371_XTALCKDIS	(1<<1)	  /* Xtal clock disable */ 
#define   ES_1370_SERR_DISABLE	(1<<0)    /* PCI serr signal disable */
#define   ES_1371_PCICLKDIS     (1<<0)    /* PCI clock disable */
#define ES_REG_STATUS	0x04	/* R/O: Interrupt/Chip select status register */
#define   ES_INTR               (1<<31)   /* Interrupt is pending */
#define   ES_1370_CSTAT		(1<<10)	  /* CODEC is busy or register write in progress */
#define   ES_1370_CBUSY         (1<<9)    /* CODEC is busy */
#define   ES_1370_CWRIP		(1<<8)    /* CODEC register write in progress */
#define   ES_1371_SYNC_ERR	(1<<8)	  /* CODEC synchronization error occured */
#define   ES_1371_VC(i)         (((i)>>6)&0x03)  /* voice code from CCB module */
#define   ES_1370_VC(i)		(((i)>>5)&0x03)  /* voice code from CCB module */
#define   ES_1371_MPWR          (1<<5)    /* power level interrupt pending */
#define   ES_MCCB		(1<<4)	  /* CCB interrupt pending */
#define   ES_UART		(1<<3)	  /* UART interrupt pending */
#define   ES_DAC1		(1<<2)	  /* DAC1 channel interrupt pending */
#define   ES_DAC2		(1<<1)	  /* DAC2 channel interrupt pending */
#define   ES_ADC		(1<<0)	  /* ADC channel interrupt pending */
#define ES_REG_UART_DATA 0x08	/* R/W: UART data register */
#define ES_REG_UART_STATUS 0x09 /* R/O: UART status register */
#define   ES_RXINT		(1<<7)	  /* RX interrupt occured */
#define   ES_TXINT		(1<<2)	  /* TX interrupt occured */
#define   ES_TXRDY		(1<<1)	  /* transmitter ready */
#define   ES_RXRDY		(1<<0)	  /* receiver ready */
#define ES_REG_UART_CONTROL 0x09 /* W/O: UART control register */
#define   ES_RXINTEN		(1<<7)	  /* RX interrupt enable */
#define   ES_TXINTEN(o)		(((o)&0x03)<<5)  /* TX interrupt enable */
#define   ES_TXINTENM		(0x03<<5)        /* mask for above */
#define   ES_CNTRL(o)		(((o)&0x03)<<0)  /* control */
#define   ES_CNTRLM		(0x03<<0)	 /* mask for above */
#define ES_REG_UART_RES	0x0a	/* R/W: UART reserver register */
#define   ES_TEST_MODE		(1<<0)		 /* test mode enabled */
#define ES_REG_MEM_PAGE	0x0c	/* R/W: Memory page register */
#define   ES_MEM_PAGEO(o)	(((o)&0x0f)<<0)	 /* memory page select - out */
#define   ES_MEM_PAGEM		(0x0f<<0)	 /* mask for above */
#define   ES_MEM_PAGEI(i)       (((i)>>0)&0x0f)  /* memory page select - in */
#define ES_REG_CODEC	0x10	/* W/O: Codec write register address */
#define   ES_CODEC_WRITE(a,d)	((((a)&0xff)<<8)|(((d)&0xff)<<0))

/*
 *
 */

typedef struct snd_stru_ensoniq ensoniq_t;

struct snd_stru_ensoniq {
  int dma1num;		/* DAC1 */
  int dma2num;		/* ADC */
  int dma3num;		/* DAC2 */
  int irqnum;

  unsigned int port;

  unsigned int mode;

  struct snd_pci_dev *pci;
  snd_card_t *card;
  snd_pcm_t *pcm;		/* DAC1 PCM */
  snd_pcm_t *pcm2;		/* DAC2 PCM */
  snd_kmixer_t *mixer;
  snd_rawmidi_t *rmidi;

  snd_spin_define( reg );
  snd_info_entry_t *proc_entry;
};

ensoniq_t *snd_ensoniq_create( snd_card_t *card, struct snd_pci_dev *pci, int dma1num, int dma2num, int dma3num, int irqnum );
void snd_ensoniq_free( ensoniq_t *ensoniq );
void snd_ensoniq_interrupt( ensoniq_t *ensoniq );

snd_pcm_t *snd_ensoniq_pcm( ensoniq_t *ensoniq );
snd_pcm_t *snd_ensoniq_pcm2( ensoniq_t *ensoniq );
snd_kmixer_t *snd_ensoniq_mixer( ensoniq_t *ensoniq );
void snd_ensoniq_midi( ensoniq_t *ensoniq, mpu401_t *mpu );

#endif /* __ES1370_H */
