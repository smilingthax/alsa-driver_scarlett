#ifndef __RME96_H
#define __RME96_H

/*
 *
 *   Defines for RME Digi96 series, from internal RME reference documents
 *   dated 12.01.00
 *
 *   Copyright (c) 2000 Anders Torger <torger@ludd.luth.se>
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

#define RME96_SPDIF_NCHANNELS 2

/* Playback and capture buffer size */
#define RME96_BUFFER_SIZE 0x10000

/* IO area size */
#define RME96_IO_SIZE 0x60000

/* IO area offsets (given in 32 bit words, not bytes) */
#define RME96_IO_PLAY_BUFFER      (0x0 / 4)
#define RME96_IO_REC_BUFFER       (0x10000 / 4)
#define RME96_IO_CONTROL_REGISTER (0x20000 / 4)
#define RME96_IO_ADDITIONAL_REG   (0x20004 / 4)
#define RME96_IO_CONFIRM_PLAY_IRQ (0x20008 / 4)
#define RME96_IO_CONFIRM_REC_IRQ  (0x2000C / 4)
#define RME96_IO_SET_PLAY_POS     (0x40000 / 4)
#define RME96_IO_RESET_PLAY_POS   (0x4FFFC / 4)
#define RME96_IO_SET_REC_POS      (0x50000 / 4)
#define RME96_IO_RESET_REC_POS    (0x5FFFC / 4)
#define RME96_IO_GET_PLAY_POS     (0x20000 / 4)
#define RME96_IO_GET_REC_POS      (0x30000 / 4)

/* Write control register bits */
#define RME96_WCR_START     (1 << 0)
#define RME96_WCR_START_2   (1 << 1)
#define RME96_WCR_GAIN_0    (1 << 2)
#define RME96_WCR_GAIN_1    (1 << 3)
#define RME96_WCR_MODE24    (1 << 4)
#define RME96_WCR_MODE24_2  (1 << 5)
#define RME96_WCR_BM        (1 << 6)
#define RME96_WCR_BM_2      (1 << 7)
#define RME96_WCR_ADAT      (1 << 8)
#define RME96_WCR_FREQ_0    (1 << 9)
#define RME96_WCR_FREQ_1    (1 << 10)
#define RME96_WCR_DS        (1 << 11)
#define RME96_WCR_PRO       (1 << 12)
#define RME96_WCR_EMP       (1 << 13)
#define RME96_WCR_SEL       (1 << 14)
#define RME96_WCR_MASTER    (1 << 15)
#define RME96_WCR_PD        (1 << 16)
#define RME96_WCR_INP_0     (1 << 17)
#define RME96_WCR_INP_1     (1 << 18)
#define RME96_WCR_THRU_0    (1 << 19)
#define RME96_WCR_THRU_1    (1 << 20)
#define RME96_WCR_THRU_2    (1 << 21)
#define RME96_WCR_THRU_3    (1 << 22)
#define RME96_WCR_THRU_4    (1 << 23)
#define RME96_WCR_THRU_5    (1 << 24)
#define RME96_WCR_THRU_6    (1 << 25)
#define RME96_WCR_THRU_7    (1 << 26)
#define RME96_WCR_DOLBY     (1 << 27)
#define RME96_WCR_MONITOR_0 (1 << 28)
#define RME96_WCR_MONITOR_1 (1 << 29)
#define RME96_WCR_ISEL      (1 << 30)
#define RME96_WCR_IDIS      (1 << 31)

#define RME96_WCR_BITPOS_FREQ_0 9
#define RME96_WCR_BITPOS_FREQ_1 10
#define RME96_WCR_BITPOS_INP_0 17
#define RME96_WCR_BITPOS_INP_1 18

/* Read control register bits */
#define RME96_RCR_AUDIO_ADDR_MASK 0xFFFF
#define RME96_RCR_IRQ_2     (1 << 16)
#define RME96_RCR_T_OUT     (1 << 17)
#define RME96_RCR_DEV_ID_0  (1 << 21)
#define RME96_RCR_DEV_ID_1  (1 << 22)
#define RME96_RCR_LOCK      (1 << 23)
#define RME96_RCR_VERF      (1 << 26)
#define RME96_RCR_F0        (1 << 27)
#define RME96_RCR_F1        (1 << 28)
#define RME96_RCR_F2        (1 << 29)
#define RME96_RCR_AUTOSYNC  (1 << 30)
#define RME96_RCR_IRQ       (1 << 31)

#define RME96_RCR_BITPOS_F0 27
#define RME96_RCR_BITPOS_F1 28
#define RME96_RCR_BITPOS_F2 29

/* Additonal register bits */
#define RME96_AR_WSEL       (1 << 0)
#define RME96_AR_ANALOG     (1 << 1)
#define RME96_AR_FREQPAD_0  (1 << 2)
#define RME96_AR_FREQPAD_1  (1 << 3)
#define RME96_AR_FREQPAD_2  (1 << 4)
#define RME96_AR_PD2        (1 << 5)
#define RME96_AR_DAC_EN     (1 << 6)
#define RME96_AR_CLATCH     (1 << 7)
#define RME96_AR_CCLK       (1 << 8)
#define RME96_AR_CDATA      (1 << 9)

/* Input types */
#define RME96_INPUT_OPTICAL 0
#define RME96_INPUT_COAXIAL 1
#define RME96_INPUT_INTERNAL 2
#define RME96_INPUT_XLR 3

/* Clock modes */
#define RME96_CLOCKMODE_SLAVE 0
#define RME96_CLOCKMODE_MASTER 1
#define RME96_CLOCKMODE_WORDCLOCK 2

/* Block sizes in bytes */
#define RME96_SPDIF_SMALL_BLOCK 2048
#define RME96_SPDIF_LARGE_BLOCK 8192

/*
 * PCI vendor/device ids, could in the future be defined in <linux/pci.h>,
 * therefore #ifndef is used.
 */
#ifndef PCI_VENDOR_ID_XILINX
#define PCI_VENDOR_ID_XILINX 0x10ee
#endif
#ifndef PCI_DEVICE_ID_DIGI96
#define PCI_DEVICE_ID_DIGI96 0x3fc0
#endif
#ifndef PCI_DEVICE_ID_DIGI96_8
#define PCI_DEVICE_ID_DIGI96_8 0x3fc1
#endif
#ifndef PCI_DEVICE_ID_DIGI96_8_PRO
#define PCI_DEVICE_ID_DIGI96_8_PRO 0x3fc2
#endif

/* this struct will have to be extended to support Digi96/8 PRO/PAD/PST */
typedef struct snd_rme96 {
	spinlock_t    lock;
	snd_irq_t    *irqptr;
	unsigned long port;
	u32          *iobase;
	
	u32 wcreg;    /* cached write control register value */
	u32 rcreg;    /* cached read control register value */
	u32 areg;     /* cached additional register value */
	
	snd_pcm_subchn_t *playback_subchn;
	snd_pcm_subchn_t *capture_subchn;

	int playback_prepared;
	int capture_prepared;
	
	snd_card_t         *card;
	snd_pcm_t          *pcm;
	struct pci_dev     *pci;
	snd_info_entry_t   *proc_entry;
} rme96_t;

int  snd_rme96_create(int, rme96_t *);
void snd_rme96_interrupt(int, void *, struct pt_regs *);
void snd_rme96_free(void *);

#endif
