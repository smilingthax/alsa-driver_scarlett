#ifndef __AUDIOPCI_H
#define __AUDIOPCI_H

/*
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
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

#include "pcm.h"
#include "mixer.h"
#include "rawmidi.h"
#include "ak4531_codec.h"
#include "ac97_codec.h"

#ifndef PCI_VENDOR_ID_ENSONIQ
#define PCI_VENDOR_ID_ENSONIQ           0x1274
#endif
#ifndef PCI_DEVICE_ID_ENSONIQ_ES1370
#define PCI_DEVICE_ID_ENSONIQ_ES1370    0x5000
#endif
#ifndef PCI_DEVICE_ID_ENSONIQ_ES1371
#define PCI_DEVICE_ID_ENSONIQ_ES1371    0x1371
#endif

/*
 * Direct registers
 */

#define ES_REG( ensoniq, x ) ( (ensoniq) -> port + ES_REG_##x )

#define ES_REG_CONTROL	0x00	/* R/W: Interrupt/Chip select control register */
#define   ES_1370_ADC_STOP	(1<<31)		/* disable capture buffer transfers */
#define   ES_1370_XCTL1 	(1<<30)		/* general purpose output bit */
#define   ES_1371_SPDIF_EN	(1<<26)		/* SPDIF enable */
#define   ES_1371_JOY_ASEL(o)	(((o)&0x03)<<24)	/* joystick port mapping */
#define   ES_1371_JOY_ASELM	(0x03<<24)	/* mask for above */
#define   ES_1371_JOY_ASELI(i)  (((i)>>24)&0x03)
#define   ES_1371_GPIO_IN(i)	(((i)>>20)&0x0f)	/* GPIO in [3:0] pins - R/O */
#define   ES_1370_PCLKDIVO(o)	(((o)&0x1fff)<<16)	/* clock divide ratio for DAC2 */
#define   ES_1370_PCLKDIVM	((0x1fff)<<16)	/* mask for above */
#define   ES_1370_PCLKDIVI(i)	(((i)>>16)&0x1fff)	/* clock divide ratio for DAC2 */
#define   ES_1371_GPIO_OUT(o)	(((o)&0x0f)<<16)	/* GPIO out [3:0] pins - W/R */
#define   ES_1371_GPIO_OUTM     (0x0f<<16)	/* mask for above */
#define   ES_MSFMTSEL		(1<<15)		/* MPEG serial data format; 0 = SONY, 1 = I2S */
#define   ES_1370_M_SBB		(1<<14)		/* clock source for DAC - 0 = clock generator; 1 = MPEG clocks */
#define   ES_1371_SYNC_RES	(1<<14)		/* Warm AC97 reset */
#define   ES_1370_WTSRSEL(o)	(((o)&0x03)<<12)	/* fixed frequency clock for DAC1 */
#define   ES_1370_WTSRSELM	(0x03<<12)	/* mask for above */
#define   ES_1371_ADC_STOP	(1<<13)		/* disable CCB transfer capture information */
#define   ES_1371_PWR_INTRM	(1<<12)		/* power level change interrupts enable */
#define   ES_1370_DAC_SYNC	(1<<11)		/* DAC's are synchronous */
#define   ES_1371_M_CB		(1<<11)		/* capture clock source; 0 = ADC; 1 = I2S */
#define   ES_CCB_INTRM		(1<<10)		/* CCB voice interrupts enable */
#define   ES_1370_M_CB		(1<<9)	/* capture clock source; 0 = ADC; 1 = MPEG */
#define   ES_1370_XCTL0		(1<<8)	/* generap purpose output bit */
#define   ES_1371_PDLEV(o)	(((o)&0x03)<<8)		/* current power down level */
#define   ES_1371_PDLEVM	(0x03<<8)	/* mask for above */
#define   ES_BREQ		(1<<7)	/* memory bus request enable */
#define   ES_DAC1_EN		(1<<6)	/* DAC1 playback channel enable */
#define   ES_DAC2_EN		(1<<5)	/* DAC2 playback channel enable */
#define   ES_ADC_EN		(1<<4)	/* ADC capture channel enable */
#define   ES_UART_EN		(1<<3)	/* UART enable */
#define   ES_JYSTK_EN		(1<<2)	/* Joystick module enable */
#define   ES_1370_CDC_EN	(1<<1)	/* Codec interface enable */
#define   ES_1371_XTALCKDIS	(1<<1)	/* Xtal clock disable */
#define   ES_1370_SERR_DISABLE	(1<<0)	/* PCI serr signal disable */
#define   ES_1371_PCICLKDIS     (1<<0)	/* PCI clock disable */
#define ES_REG_STATUS	0x04	/* R/O: Interrupt/Chip select status register */
#define   ES_INTR               (1<<31)	/* Interrupt is pending */
#define   ES_1371_ST_SPDIF_EN	(1<<18) /* SPDIF enable */
#define   ES_1371_ST_SPDIF_TEST (1<<17) /* SPDIF test */
#define   ES_1371_TEST          (1<<16) /* test ASIC */
#define   ES_1370_CSTAT		(1<<10)	/* CODEC is busy or register write in progress */
#define   ES_1370_CBUSY         (1<<9)	/* CODEC is busy */
#define   ES_1370_CWRIP		(1<<8)	/* CODEC register write in progress */
#define   ES_1371_SYNC_ERR	(1<<8)	/* CODEC synchronization error occured */
#define   ES_1371_VC(i)         (((i)>>6)&0x03)		/* voice code from CCB module */
#define   ES_1370_VC(i)		(((i)>>5)&0x03)		/* voice code from CCB module */
#define   ES_1371_MPWR          (1<<5)	/* power level interrupt pending */
#define   ES_MCCB		(1<<4)	/* CCB interrupt pending */
#define   ES_UART		(1<<3)	/* UART interrupt pending */
#define   ES_DAC1		(1<<2)	/* DAC1 channel interrupt pending */
#define   ES_DAC2		(1<<1)	/* DAC2 channel interrupt pending */
#define   ES_ADC		(1<<0)	/* ADC channel interrupt pending */
#define ES_REG_UART_DATA 0x08	/* R/W: UART data register */
#define ES_REG_UART_STATUS 0x09	/* R/O: UART status register */
#define   ES_RXINT		(1<<7)	/* RX interrupt occured */
#define   ES_TXINT		(1<<2)	/* TX interrupt occured */
#define   ES_TXRDY		(1<<1)	/* transmitter ready */
#define   ES_RXRDY		(1<<0)	/* receiver ready */
#define ES_REG_UART_CONTROL 0x09	/* W/O: UART control register */
#define   ES_RXINTEN		(1<<7)	/* RX interrupt enable */
#define   ES_TXINTENO(o)	(((o)&0x03)<<5)		/* TX interrupt enable */
#define   ES_TXINTENM		(0x03<<5)	/* mask for above */
#define   ES_TXINTENI(i)	(((i)>>5)&0x03)
#define   ES_CNTRL(o)		(((o)&0x03)<<0)		/* control */
#define   ES_CNTRLM		(0x03<<0)	/* mask for above */
#define ES_REG_UART_RES	0x0a	/* R/W: UART reserver register */
#define   ES_TEST_MODE		(1<<0)	/* test mode enabled */
#define ES_REG_MEM_PAGE	0x0c	/* R/W: Memory page register */
#define   ES_MEM_PAGEO(o)	(((o)&0x0f)<<0)		/* memory page select - out */
#define   ES_MEM_PAGEM		(0x0f<<0)	/* mask for above */
#define   ES_MEM_PAGEI(i)       (((i)>>0)&0x0f)		/* memory page select - in */

#define ES_REG_1370_CODEC 0x10	/* W/O: Codec write register address */
#define   ES_1370_CODEC_WRITE(a,d) ((((a)&0xff)<<8)|(((d)&0xff)<<0))
#define ES_REG_1371_CODEC 0x14	/* W/R: Codec Read/Write register address */
#define   ES_1371_CODEC_RDY	   (1<<31)	/* codec ready */
#define   ES_1371_CODEC_WIP	   (1<<30)	/* codec register access in progress */
#define   ES_1371_CODEC_PIRD	   (1<<23)	/* codec read/write select register */
#define   ES_1371_CODEC_WRITE(a,d) ((((a)&0x7f)<<16)|(((d)&0xffff)<<0))
#define   ES_1371_CODEC_READS(a)   ((((a)&0x7f)<<16)|ES_1371_CODEC_PIRD)
#define   ES_1371_CODEC_READ(i)    (((i)>>0)&0xffff)

#define ES_REG_1371_SMPRATE 0x10	/* W/R: Codec rate converter interface register */
#define   ES_1371_SRC_RAM_ADDRO(o) (((o)&0x7f)<<25)	/* address of the sample rate converter */
#define   ES_1371_SRC_RAM_ADDRM	   (0x7f<<25)	/* mask for above */
#define   ES_1371_SRC_RAM_ADDRI(i) (((i)>>25)&0x7f)	/* address of the sample rate converter */
#define   ES_1371_SRC_RAM_WE	   (1<<24)	/* R/W: read/write control for sample rate converter */
#define   ES_1371_SRC_RAM_BUSY     (1<<23)	/* R/O: sample rate memory is busy */
#define   ES_1371_SRC_DISABLE      (1<<22)	/* sample rate converter disable */
#define   ES_1371_DIS_P1	   (1<<21)	/* playback channel 1 accumulator update disable */
#define   ES_1371_DIS_P2	   (1<<20)	/* playback channel 1 accumulator update disable */
#define   ES_1371_DIS_R1	   (1<<19)	/* capture channel accumulator update disable */
#define   ES_1371_SRC_RAM_DATAO(o) (((o)&0xffff)<<0)	/* current value of the sample rate converter */
#define   ES_1371_SRC_RAM_DATAM	   (0xffff<<0)	/* mask for above */
#define   ES_1371_SRC_RAM_DATAI(i) (((i)>>0)&0xffff)	/* current value of the sample rate converter */

#define ES_REG_1371_LEGACY 0x18	/* W/R: Legacy control/status register */
#define   ES_1371_JFAST		(1<<31)		/* fast joystick timing */
#define   ES_1371_HIB		(1<<30)		/* host interrupt blocking enable */
#define   ES_1371_VSB		(1<<29)		/* SB; 0 = addr 0x220xH, 1 = 0x22FxH */
#define   ES_1371_VMPUO(o)	(((o)&0x03)<<27)	/* base register address; 0 = 0x320xH; 1 = 0x330xH; 2 = 0x340xH; 3 = 0x350xH */
#define   ES_1371_VMPUM		(0x03<<27)	/* mask for above */
#define   ES_1371_VMPUI(i)	(((i)>>27)&0x03)	/* base register address */
#define   ES_1371_VCDCO(o)	(((o)&0x03)<<25)	/* CODEC; 0 = 0x530xH; 1 = undefined; 2 = 0xe80xH; 3 = 0xF40xH */
#define   ES_1371_VCDCM		(0x03<<25)	/* mask for above */
#define   ES_1371_VCDCI(i)	(((i)>>25)&0x03)	/* CODEC address */
#define   ES_1371_FIRQ		(1<<24)		/* force an interrupt */
#define   ES_1371_SDMACAP	(1<<23)		/* enable event capture for slave DMA controller */
#define   ES_1371_SPICAP	(1<<22)		/* enable event capture for slave IRQ controller */
#define   ES_1371_MDMACAP	(1<<21)		/* enable event capture for master DMA controller */
#define   ES_1371_MPICAP	(1<<20)		/* enable event capture for master IRQ controller */
#define   ES_1371_ADCAP		(1<<19)		/* enable event capture for ADLIB register; 0x388xH */
#define   ES_1371_SVCAP		(1<<18)		/* enable event capture for SB registers */
#define   ES_1371_CDCCAP	(1<<17)		/* enable event capture for CODEC registers */
#define   ES_1371_BACAP		(1<<16)		/* enable event capture for SoundScape base address */
#define   ES_1371_EXI(i)	(((i)>>8)&0x07)		/* event number */
#define   ES_1371_AI(i)		(((i)>>3)&0x1f)		/* event significant I/O address */
#define   ES_1371_WR		(1<<2)	/* event capture; 0 = read; 1 = write */
#define   ES_1371_LEGINT	(1<<0)	/* interrupt for legacy events; 0 = interrupt did occur */

#define ES_REG_SERIAL	0x20	/* R/W: Serial interface control register */
#define   ES_1371_DAC_TEST	(1<<22)		/* DAC test mode enable */
#define   ES_P2_END_INCO(o)	(((o)&0x07)<<19)	/* binary offset value to increment / loop end */
#define   ES_P2_END_INCM	(0x07<<19)	/* mask for above */
#define   ES_P2_END_INCI(i)	(((i)>>16)&0x07)	/* binary offset value to increment / loop end */
#define   ES_P2_ST_INCO(o)	(((o)&0x07)<<16)	/* binary offset value to increment / start */
#define   ES_P2_ST_INCM		(0x07<<16)	/* mask for above */
#define   ES_P2_ST_INCI(i)	(((i)<<16)&0x07)	/* binary offset value to increment / start */
#define   ES_R1_LOOP_SEL	(1<<15)		/* ADC; 0 - loop mode; 1 = stop mode */
#define   ES_P2_LOOP_SEL	(1<<14)		/* DAC2; 0 - loop mode; 1 = stop mode */
#define   ES_P1_LOOP_SEL	(1<<13)		/* DAC1; 0 - loop mode; 1 = stop mode */
#define   ES_P2_PAUSE		(1<<12)		/* DAC2; 0 - play mode; 1 = pause mode */
#define   ES_P1_PAUSE		(1<<11)		/* DAC1; 0 - play mode; 1 = pause mode */
#define   ES_R1_INT_EN		(1<<10)		/* ADC interrupt enable */
#define   ES_P2_INT_EN		(1<<9)	/* DAC2 interrupt enable */
#define   ES_P1_INT_EN		(1<<8)	/* DAC1 interrupt enable */
#define   ES_P1_SCT_RLD		(1<<7)	/* force sample counter reload for DAC1 */
#define   ES_P2_DAC_SEN		(1<<6)	/* when stop mode: 0 - DAC2 play back zeros; 1 = DAC2 play back last sample */
#define   ES_R1_MODEO(o)	(((o)&0x03)<<4)		/* ADC mode; 0 = 8-bit mono; 1 = 8-bit stereo; 2 = 16-bit mono; 3 = 16-bit stereo */
#define   ES_R1_MODEM		(0x03<<4)	/* mask for above */
#define   ES_R1_MODEI(i)	(((i)>>4)&0x03)
#define   ES_P2_MODEO(o)	(((o)&0x03)<<2)		/* DAC2 mode; -- '' -- */
#define   ES_P2_MODEM		(0x03<<2)	/* mask for above */
#define   ES_P2_MODEI(i)	(((i)>>2)&0x03)
#define   ES_P1_MODEO(o)	(((o)&0x03)<<0)		/* DAC1 mode; -- '' -- */
#define   ES_P1_MODEM		(0x03<<0)	/* mask for above */
#define   ES_P1_MODEI(i)	(((i)>>0)&0x03)

#define ES_REG_DAC1_COUNT 0x24	/* R/W: DAC1 sample count register */
#define ES_REG_DAC2_COUNT 0x28	/* R/W: DAC2 sample count register */
#define ES_REG_ADC_COUNT  0x2c	/* R/W: ADC sample count register */
#define   ES_REG_CURR_COUNT(i)  (((i)>>16)&0xffff)
#define   ES_REG_COUNTO(o)	(((o)&0xffff)<<0)
#define   ES_REG_COUNTM		(0xffff<<0)
#define   ES_REG_COUNTI(i)	(((i)>>0)&0xffff)

#define ES_REG_DAC1_FRAME 0x30	/* R/W: PAGE 0x0c; DAC1 frame address */
#define ES_REG_DAC1_SIZE  0x34	/* R/W: PAGE 0x0c; DAC1 frame size */
#define ES_REG_DAC2_FRAME 0x38	/* R/W: PAGE 0x0c; DAC2 frame address */
#define ES_REG_DAC2_SIZE  0x3c	/* R/W: PAGE 0x0c; DAC2 frame size */
#define ES_REG_ADC_FRAME  0x30	/* R/W: PAGE 0x0d; ADC frame address */
#define ES_REG_ADC_SIZE	  0x34	/* R/W: PAGE 0x0d; ADC frame size */
#define   ES_REG_FCURR_COUNTO(o) (((o)&0xffff)<<16)
#define   ES_REG_FCURR_COUNTM    (0xffff<<16)
#define   ES_REG_FCURR_COUNTI(i) (((i)>>14)&0x3fffc)
#define   ES_REG_FSIZEO(o)	 (((o)&0xffff)<<0)
#define   ES_REG_FSIZEM		 (0xffff<<0)
#define   ES_REG_FSIZEI(i)	 (((i)>>0)&0xffff)
#define ES_REG_PHANTOM_FRAME 0x38 /* R/W: PAGE 0x0d: phantom frame address */
#define ES_REG_PHANTOM_COUNT 0x3c /* R/W: PAGE 0x0d: phantom frame count */

#define ES_REG_UART_FIFO  0x30	/* R/W: PAGE 0x0e; UART FIFO register */
#define   ES_REG_UF_VALID	 (1<<8)
#define   ES_REG_UF_BYTEO(o)	 (((o)&0xff)<<0)
#define   ES_REG_UF_BYTEM	 (0xff<<0)
#define   ES_REG_UF_BYTEI(i)	 (((i)>>0)&0xff)


/*
 *  Pages
 */

#define ES_PAGE_DAC	0x0c
#define ES_PAGE_ADC	0x0d
#define ES_PAGE_UART	0x0e
#define ES_PAGE_UART1	0x0f

/*
 *  Sample rate converter addresses
 */

#define ES_SMPREG_DAC1		0x70
#define ES_SMPREG_DAC2		0x74
#define ES_SMPREG_ADC		0x78
#define ES_SMPREG_VOL_ADC	0x6c
#define ES_SMPREG_VOL_DAC1	0x7c
#define ES_SMPREG_VOL_DAC2	0x7e
#define ES_SMPREG_TRUNC_N	0x00
#define ES_SMPREG_INT_REGS	0x01
#define ES_SMPREG_ACCUM_FRAC	0x02
#define ES_SMPREG_VFREQ_FRAC	0x03

/*
 *  Some contants
 */

#define ES_1370_SRTODIV(x) (((1411200+(x)/2)/(x))-2)
#define ES_1370_DIVTOSR(x) (1411200/((x)+2))

/*
 *  Open modes
 */

#define ES_MODE_PLAY1	0x0001
#define ES_MODE_PLAY2	0x0002
#define ES_MODE_CAPTURE	0x0004

#define ES_MODE_OUTPUT	0x0001	/* for MIDI */
#define ES_MODE_INPUT	0x0002	/* for MIDI */

/*

 */

typedef struct snd_stru_ensoniq ensoniq_t;

struct snd_stru_ensoniq {
	snd_dma_t * dma1ptr;	/* DAC1 */
	snd_dma_t * dma2ptr;	/* ADC */
	snd_dma_t * dma3ptr;	/* DAC2 */
	snd_irq_t * irqptr;

	unsigned int port;
	unsigned int mode;
	unsigned int uartm;	/* UART mode */

	unsigned int ctrl;	/* control register */
	unsigned int sctrl;	/* serial control register */
	unsigned int uartc;	/* uart control register */

	union {
		struct {
			ac97_t *ac97;
		} es1371;
		struct {
			int pclkdiv_lock;
			ak4531_t *ak4531;
		} es1370;
	} u;

	struct pci_dev *pci;
	snd_card_t *card;
	snd_pcm_t *pcm;		/* DAC1/ADC PCM */
	snd_pcm_t *pcm2;	/* DAC2 PCM */
	snd_pcm_subchn_t *playback_subchn;
	snd_pcm_subchn_t *playback2_subchn;
	snd_pcm_subchn_t *capture_subchn;
	unsigned int p1_dma_size;
	unsigned int p2_dma_size;
	unsigned int c_dma_size;
	unsigned int p1_frag_size;
	unsigned int p2_frag_size;
	unsigned int c_frag_size;
	snd_kmixer_t *mixer;
	snd_rawmidi_t *rmidi;

	spinlock_t reg_lock;
	snd_info_entry_t *proc_entry;
};

int snd_ensoniq_1370_create(snd_card_t * card,
			    struct pci_dev *pci,
			    snd_dma_t * dma1ptr,
			    snd_dma_t * dma2ptr,
			    snd_dma_t * dma3ptr,
			    snd_irq_t * irqptr,
			    ensoniq_t ** rensoniq);
int snd_ensoniq_1370_free(ensoniq_t * ensoniq);
void snd_ensoniq_1370_interrupt(ensoniq_t * ensoniq);

int snd_ensoniq_1370_pcm(ensoniq_t * ensoniq, int device, snd_pcm_t ** rpcm);
int snd_ensoniq_1370_pcm2(ensoniq_t * ensoniq, int device, snd_pcm_t ** rpcm);
int snd_ensoniq_1370_mixer(ensoniq_t * ensoniq, int device, int pcm_count, int *pcm_devs, snd_kmixer_t ** rmixer);
int snd_ensoniq_1370_midi(ensoniq_t * ensoniq, int device, snd_rawmidi_t ** rrawmidi);

int snd_ensoniq_1371_create(snd_card_t * card,
			    struct pci_dev *pci,
			    snd_dma_t * dma1ptr,
			    snd_dma_t * dma2ptr,
			    snd_dma_t * dma3ptr,
			    snd_irq_t * irqptr,
			    ensoniq_t ** rensoniq);
int snd_ensoniq_1371_free(ensoniq_t * ensoniq);
void snd_ensoniq_1371_interrupt(ensoniq_t * ensoniq);

int snd_ensoniq_1371_pcm(ensoniq_t * ensoniq, int device, snd_pcm_t ** rpcm);
int snd_ensoniq_1371_pcm2(ensoniq_t * ensoniq, int device, snd_pcm_t ** rpcm);
int snd_ensoniq_1371_mixer(ensoniq_t * ensoniq, int device, int pcm_count, int *pcm_devs, snd_kmixer_t ** rmixer);
int snd_ensoniq_1371_midi(ensoniq_t * ensoniq, int device, snd_rawmidi_t ** rrawmidi);

#endif				/* __AUDIOPCI_H */
