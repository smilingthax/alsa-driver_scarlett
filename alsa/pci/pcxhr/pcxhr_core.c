#define __NO_VERSION__
/*
 * Driver for Digigram pcxhr compatible soundcards
 *
 * low level interface with interrupt and message handling implementation
 *
 * Copyright (c) 2004 by Digigram <alsa@digigram.com>
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
 */

#include <sound/driver.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <sound/core.h>
#include "pcxhr.h"
#include "pcxhr_mixer.h"
#include "pcxhr_hwdep.h"
#include "pcxhr_core.h"


/* registers used on the PLX (port 1) */
#define PCXHR_PLX_OFFSET_MIN	0x40
#define PCXHR_PLX_MBOX0		0x40
#define PCXHR_PLX_MBOX1		0x44
#define PCXHR_PLX_MBOX2		0x48
#define PCXHR_PLX_MBOX3		0x4C
#define PCXHR_PLX_MBOX4		0x50
#define PCXHR_PLX_MBOX5		0x54
#define PCXHR_PLX_MBOX6		0x58
#define PCXHR_PLX_MBOX7		0x5C
#define PCXHR_PLX_L2PCIDB	0x64
#define PCXHR_PLX_IRQCS		0x68
#define PCXHR_PLX_CHIPSC	0x6C

/* registers used on the DSP (port 2) */
#define PCXHR_DSP_ICR		0x00
#define PCXHR_DSP_CVR		0x04
#define PCXHR_DSP_ISR		0x08
#define PCXHR_DSP_IVR		0x0C
#define PCXHR_DSP_RXH		0x14
#define PCXHR_DSP_TXH		0x14
#define PCXHR_DSP_RXM		0x18
#define PCXHR_DSP_TXM		0x18
#define PCXHR_DSP_RXL		0x1C
#define PCXHR_DSP_TXL		0x1C
#define PCXHR_DSP_RESET		0x20
#define PCXHR_DSP_OFFSET_MAX	0x20

/* access to the card */
#define PCXHR_PLX 1
#define PCXHR_DSP 2

#if (PCXHR_DSP_OFFSET_MAX > PCXHR_PLX_OFFSET_MIN)
#undef  PCXHR_REG_TO_PORT(x)
#else
#define PCXHR_REG_TO_PORT(x)	((x)>PCXHR_DSP_OFFSET_MAX ? PCXHR_PLX : PCXHR_DSP)
#endif
#define PCXHR_INPB(mgr,x)	inb((mgr)->port[PCXHR_REG_TO_PORT(x)] + (x))
#define PCXHR_INPL(mgr,x)	inl((mgr)->port[PCXHR_REG_TO_PORT(x)] + (x))
#define PCXHR_OUTPB(mgr,x,data)	outb((data), (mgr)->port[PCXHR_REG_TO_PORT(x)] + (x))
#define PCXHR_OUTPL(mgr,x,data)	outl((data), (mgr)->port[PCXHR_REG_TO_PORT(x)] + (x))
/* attention : access the PCXHR_DSP_* registers with inb and outb only ! */

/* params used with PCXHR_PLX_MBOX0 */
#define PCXHR_MBOX0_HF5			(1 << 0)
#define PCXHR_MBOX0_HF4			(1 << 1)
#define PCXHR_MBOX0_BOOT_HERE		(1 << 23)
/* params used with PCXHR_PLX_IRQCS */
#define PCXHR_IRQCS_ENABLE_PCIIRQ	(1 << 8)
#define PCXHR_IRQCS_ENABLE_PCIDB	(1 << 9)
#define PCXHR_IRQCS_ACTIVE_PCIDB	(1 << 13)
/* params used with PCXHR_PLX_CHIPSC */
#define PCXHR_CHIPSC_INIT_VALUE		0x100D767E
#define PCXHR_CHIPSC_RESET_XILINX	(1 << 16)
#define PCXHR_CHIPSC_GPI_USERI		(1 << 17)
#define PCXHR_CHIPSC_DATA_CLK		(1 << 24)
#define PCXHR_CHIPSC_DATA_IN		(1 << 26)

/* params used with PCXHR_DSP_ICR */
#define PCXHR_ICR_HI08_RREQ		0x01
#define PCXHR_ICR_HI08_TREQ		0x02
#define PCXHR_ICR_HI08_HDRQ		0x04
#define PCXHR_ICR_HI08_HF0		0x08
#define PCXHR_ICR_HI08_HF1		0x10
#define PCXHR_ICR_HI08_HLEND		0x20
#define PCXHR_ICR_HI08_INIT		0x80
/* params used with PCXHR_DSP_CVR */
#define PCXHR_CVR_HI08_HC		0x80
/* params used with PCXHR_DSP_ISR */
#define PCXHR_ISR_HI08_RXDF		0x01
#define PCXHR_ISR_HI08_TXDE		0x02
#define PCXHR_ISR_HI08_TRDY		0x04
#define PCXHR_ISR_HI08_ERR		0x08
#define PCXHR_ISR_HI08_CHK		0x10
#define PCXHR_ISR_HI08_HREQ		0x80


/*
 * pcxhr_delay - delay for the specified time
 * @xmsec: the time to delay in msec
 */
void pcxhr_delay(int xmsec)
{
	if (! in_interrupt() && xmsec >= 1000 / HZ) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout((xmsec * HZ + 999) / 1000);
	} else {
		mdelay(xmsec);
	}
}
/* constants used with pcxhr_delay() */
#define PCXHR_WAIT_DEFAULT		2
#define PCXHR_WAIT_IT			25
#define PCXHR_WAIT_IT_EXTRA		65

/*
 * pcxhr_check_reg_bit - wait for the specified bit is set/reset on a register
 * @reg: register to check
 * @mask: bit mask
 * @bit: resultant bit to be checked
 * @time: time-out of loop in msec
 *
 * returns zero if a bit matches, or a negative error code.
 */
static int pcxhr_check_reg_bit(pcxhr_mgr_t *mgr, unsigned int reg, unsigned char mask, unsigned char bit, int time, unsigned char* read)
{
	int i=0;
	unsigned long end_time = jiffies + (time * HZ + 999) / 1000;
	do {
		*read = PCXHR_INPB(mgr, reg);
		if ((*read & mask) == bit) {
			if(i>100) snd_printdd("ATTENTION! check_reg(%x) loopcount=%d\n", reg, i);
			return 0;
		}
		i ++;
	} while (time_after_eq(end_time, jiffies));
	snd_printk(KERN_ERR "pcxhr_check_reg_bit: timeout, reg=%x, mask=0x%x, val=0x%x\n", reg, mask, *read);
	return -EIO;
}
/* constants used with pcxhr_check_reg_bit() */
#define PCXHR_TIMEOUT_DSP		200


#define PCXHR_MASK_EXTRA_INFO		0x0000FE
#define PCXHR_MASK_IT_HF0		0x000100
#define PCXHR_MASK_IT_HF1		0x000200
#define PCXHR_MASK_IT_NO_HF0_HF1	0x000400
#define PCXHR_MASK_IT_MANAGE_HF5	0x000800
#define PCXHR_MASK_IT_WAIT		0x010000
#define PCXHR_MASK_IT_WAIT_EXTRA	0x020000

#define PCXHR_IT_SEND_BYTE_XILINX	(0x0000003C | PCXHR_MASK_IT_HF0)
#define PCXHR_IT_TEST_XILINX		(0x0000003C | PCXHR_MASK_IT_HF1 | PCXHR_MASK_IT_MANAGE_HF5)
#define PCXHR_IT_DOWNLOAD_BOOT		(0x0000000C | PCXHR_MASK_IT_HF1 | PCXHR_MASK_IT_MANAGE_HF5 | PCXHR_MASK_IT_WAIT)
#define PCXHR_IT_RESET_BOARD_FUNC	(0x0000000C | PCXHR_MASK_IT_HF0 | PCXHR_MASK_IT_MANAGE_HF5 | PCXHR_MASK_IT_WAIT_EXTRA)
#define PCXHR_IT_DOWNLOAD_DSP		(0x0000000C |                     PCXHR_MASK_IT_MANAGE_HF5 | PCXHR_MASK_IT_WAIT)
#define PCXHR_IT_DEBUG			(0x0000005A | PCXHR_MASK_IT_NO_HF0_HF1)
#define PCXHR_IT_RESET_SEMAPHORE	(0x0000005C | PCXHR_MASK_IT_NO_HF0_HF1)
#define PCXHR_IT_MESSAGE		(0x00000074 | PCXHR_MASK_IT_NO_HF0_HF1)
#define PCXHR_IT_RESET_CHK		(0x00000076 | PCXHR_MASK_IT_NO_HF0_HF1)
#define PCXHR_IT_UPDATE_RBUFFER		(0x00000078 | PCXHR_MASK_IT_NO_HF0_HF1)

static int pcxhr_send_it_dsp(pcxhr_mgr_t *mgr, unsigned int itdsp)
{
	int err;
	unsigned char reg;

	if(itdsp & PCXHR_MASK_IT_MANAGE_HF5) {
		/* clear hf5 bit */
		PCXHR_OUTPL(mgr, PCXHR_PLX_MBOX0, PCXHR_INPL(mgr, PCXHR_PLX_MBOX0) & ~PCXHR_MBOX0_HF5);
	}
	if( (itdsp & PCXHR_MASK_IT_NO_HF0_HF1) == 0) {
		reg = PCXHR_ICR_HI08_RREQ | PCXHR_ICR_HI08_TREQ | PCXHR_ICR_HI08_HDRQ;
		if(itdsp & PCXHR_MASK_IT_HF0) reg |= PCXHR_ICR_HI08_HF0;
		if(itdsp & PCXHR_MASK_IT_HF1) reg |= PCXHR_ICR_HI08_HF1;
		PCXHR_OUTPB(mgr, PCXHR_DSP_ICR, reg);
	}
	reg = (unsigned char)(((itdsp & PCXHR_MASK_EXTRA_INFO) >> 1) | PCXHR_CVR_HI08_HC);
	PCXHR_OUTPB(mgr, PCXHR_DSP_CVR, reg);
	if(itdsp & PCXHR_MASK_IT_WAIT) {
		pcxhr_delay(PCXHR_WAIT_IT);
	}
	if(itdsp & PCXHR_MASK_IT_WAIT_EXTRA) {
		pcxhr_delay(PCXHR_WAIT_IT_EXTRA);
	}
	/* wait for CVR_HI08_HC == 0 */
	err = pcxhr_check_reg_bit(mgr, PCXHR_DSP_CVR,  PCXHR_CVR_HI08_HC, 0, PCXHR_TIMEOUT_DSP, &reg);
	if(err) {
		snd_printk(KERN_ERR "pcxhr_send_it_dsp : TIMEOUT CVR\n");
		return err;
	}
	if(itdsp & PCXHR_MASK_IT_MANAGE_HF5) {
		/* wait for hf5 bit */
		err = pcxhr_check_reg_bit(mgr, PCXHR_PLX_MBOX0, PCXHR_MBOX0_HF5, PCXHR_MBOX0_HF5, PCXHR_TIMEOUT_DSP, &reg);
		if(err) {
			snd_printk(KERN_ERR "pcxhr_send_it_dsp : TIMEOUT HF5\n");
			return err;
		}
	}
	return 0; /* retry not handled here */
}

void pcxhr_reset_xilinx_com(pcxhr_mgr_t *mgr)
{
	/* reset second xilinx */
	PCXHR_OUTPL(mgr, PCXHR_PLX_CHIPSC, PCXHR_CHIPSC_INIT_VALUE & ~PCXHR_CHIPSC_RESET_XILINX);
}

static void pcxhr_enable_irq(pcxhr_mgr_t *mgr, int enable)
{
	unsigned int reg = PCXHR_INPL(mgr, PCXHR_PLX_IRQCS);
	/* enable/disable interrupts */
	if(enable)	reg |=  (PCXHR_IRQCS_ENABLE_PCIIRQ | PCXHR_IRQCS_ENABLE_PCIDB);
	else		reg &= ~(PCXHR_IRQCS_ENABLE_PCIIRQ | PCXHR_IRQCS_ENABLE_PCIDB);
	PCXHR_OUTPL(mgr, PCXHR_PLX_IRQCS, reg);
}

void pcxhr_reset_dsp(pcxhr_mgr_t *mgr)
{
	/* disable interrupts */
	pcxhr_enable_irq(mgr, 0);

	/* let's reset the DSP */
	PCXHR_OUTPB(mgr, PCXHR_DSP_RESET, 0);
	pcxhr_delay( PCXHR_WAIT_DEFAULT ); /* wait 2 msec */
	PCXHR_OUTPB(mgr, PCXHR_DSP_RESET, 3);
	pcxhr_delay( PCXHR_WAIT_DEFAULT ); /* wait 2 msec */

	/* reset mailbox */
	PCXHR_OUTPL(mgr, PCXHR_PLX_MBOX0, 0);
}
void pcxhr_enable_dsp(pcxhr_mgr_t *mgr)
{
	/* enable interrupts */
	pcxhr_enable_irq(mgr, 1);
}

/*
 * load the xilinx image
 */
int pcxhr_load_xilinx_binary(pcxhr_mgr_t *mgr, const struct firmware *xilinx, int second)
{
	unsigned int i;
	unsigned int chipsc;
	unsigned char data;
	unsigned char mask;
	unsigned char *image;

	/* test first xilinx */
	chipsc = PCXHR_INPL(mgr, PCXHR_PLX_CHIPSC);
	if( !second ) {
		if( chipsc & PCXHR_CHIPSC_GPI_USERI ) {
			snd_printdd("no need to load first xilinx\n");
			return 0; /* first xilinx is already present and cannot be reset */
		}
	} else {
		if( (chipsc & PCXHR_CHIPSC_GPI_USERI) == 0 ) {
			snd_printk(KERN_ERR "error loading first xilinx\n");
			return -EINVAL;
		}
		/* activate second xilinx */
		chipsc |= PCXHR_CHIPSC_RESET_XILINX;
		PCXHR_OUTPL(mgr, PCXHR_PLX_CHIPSC, chipsc);
		pcxhr_delay( PCXHR_WAIT_DEFAULT ); /* wait 2 msec */
	}
	image = xilinx->data;
	for (i = 0; i < xilinx->size; i++, image++) {
		data = *image;
		mask = 0x80;
		while (mask) {
			chipsc &= ~(PCXHR_CHIPSC_DATA_CLK | PCXHR_CHIPSC_DATA_IN);
			if(data & mask) chipsc |= PCXHR_CHIPSC_DATA_IN;
			PCXHR_OUTPL(mgr, PCXHR_PLX_CHIPSC, chipsc);
			chipsc |= PCXHR_CHIPSC_DATA_CLK;
			PCXHR_OUTPL(mgr, PCXHR_PLX_CHIPSC, chipsc);
			mask >>= 1;
		}
		/* don't take too much time in this loop... */
		cond_resched();
	}
	chipsc &= ~(PCXHR_CHIPSC_DATA_CLK | PCXHR_CHIPSC_DATA_IN);
	PCXHR_OUTPL(mgr, PCXHR_PLX_CHIPSC, chipsc);
	pcxhr_delay( PCXHR_WAIT_DEFAULT ); /* wait 2 msec (time to boot the xilinx before any access) */
	return 0;
}

/*
 * send an executable file to the DSP
 */
static int pcxhr_download_dsp(pcxhr_mgr_t *mgr, const struct firmware *dsp)
{
	int err;
	unsigned int i;
	unsigned int len;
	unsigned char *data;
	unsigned char dummy;
	/* check the length of boot image */
	snd_assert(dsp->size > 0, return -EINVAL);
	snd_assert(dsp->size % 3 == 0, return -EINVAL);
	snd_assert(dsp->data, return -EINVAL);
	/* transfert data buffer from PC to DSP */
	for (i = 0; i < dsp->size; i += 3) {
		data = dsp->data + i;
		if( i == 0 ) {
			/* test data header consistency */
			len = (unsigned int)((data[0]<<16) + (data[1]<<8) + data[2]);
			snd_assert((len==0) || (dsp->size == (len+2)*3), return -EINVAL);
		}
		/* wait DSP ready for new transfer */
		err = pcxhr_check_reg_bit(mgr, PCXHR_DSP_ISR, PCXHR_ISR_HI08_TRDY, PCXHR_ISR_HI08_TRDY, PCXHR_TIMEOUT_DSP, &dummy);
		if(err) {
			snd_printk(KERN_ERR "dsp loading error at position %d\n", i);
			return err;
		}
		/* send host data */
		PCXHR_OUTPB(mgr, PCXHR_DSP_TXH, data[0]);
		PCXHR_OUTPB(mgr, PCXHR_DSP_TXM, data[1]);
		PCXHR_OUTPB(mgr, PCXHR_DSP_TXL, data[2]);

		/* don't take too much time in this loop... */
		cond_resched();
	}
	/* give some time to boot the DSP */
	pcxhr_delay(PCXHR_WAIT_DEFAULT);
	return 0;
}

/*
 * load the eeprom image
 */
int pcxhr_load_eeprom_binary(pcxhr_mgr_t *mgr, const struct firmware *eeprom)
{
	int err;
	unsigned char reg;

	/* init value of the ICR register */
	reg = PCXHR_ICR_HI08_RREQ | PCXHR_ICR_HI08_TREQ | PCXHR_ICR_HI08_HDRQ;
	if( PCXHR_INPL(mgr, PCXHR_PLX_MBOX0) & PCXHR_MBOX0_BOOT_HERE) {
		/* no need to load the eeprom binary, but init the HI08 interface */
		PCXHR_OUTPB(mgr, PCXHR_DSP_ICR, reg | PCXHR_ICR_HI08_INIT);
		pcxhr_delay(PCXHR_WAIT_DEFAULT);
		PCXHR_OUTPB(mgr, PCXHR_DSP_ICR, reg);
		pcxhr_delay(PCXHR_WAIT_DEFAULT);
		snd_printdd("no need to load eeprom boot\n");
		return 0;
	}
	PCXHR_OUTPB(mgr, PCXHR_DSP_ICR, reg);

	err = pcxhr_download_dsp(mgr, eeprom);
	if(err) return err;
	/* wait for chk bit */
	return pcxhr_check_reg_bit(mgr, PCXHR_DSP_ISR, PCXHR_ISR_HI08_CHK, PCXHR_ISR_HI08_CHK, PCXHR_TIMEOUT_DSP, &reg);
}

/*
 * load the boot image
 */
int pcxhr_load_boot_binary(pcxhr_mgr_t *mgr, const struct firmware *boot)
{
	int err;
	unsigned int physaddr = mgr->hostport.addr;
	unsigned char dummy;

	/* send the hostport address to the DSP (only the upper 24 bit !) */
	snd_assert((physaddr & 0xff) == 0, return -EINVAL);
	PCXHR_OUTPL(mgr, PCXHR_PLX_MBOX1, (physaddr >> 8));

	err = pcxhr_send_it_dsp(mgr, PCXHR_IT_DOWNLOAD_BOOT);
	if(err) return err;
	/* clear hf5 bit */
	PCXHR_OUTPL(mgr, PCXHR_PLX_MBOX0, PCXHR_INPL(mgr, PCXHR_PLX_MBOX0) & ~PCXHR_MBOX0_HF5);

	err = pcxhr_download_dsp(mgr, boot);
	if(err) return err;
	/* wait for hf5 bit */
	return pcxhr_check_reg_bit(mgr, PCXHR_PLX_MBOX0, PCXHR_MBOX0_HF5, PCXHR_MBOX0_HF5, PCXHR_TIMEOUT_DSP, &dummy);
}

/*
 * load the final dsp image
 */
int pcxhr_load_dsp_binary(pcxhr_mgr_t *mgr, const struct firmware *dsp)
{
	int err;
	unsigned char dummy;
	err = pcxhr_send_it_dsp(mgr, PCXHR_IT_RESET_BOARD_FUNC);
	if(err) return err;
	err = pcxhr_send_it_dsp(mgr, PCXHR_IT_DOWNLOAD_DSP);
	if(err) return err;
	err = pcxhr_download_dsp(mgr, dsp);
	if(err) return err;
	/* wait for chk bit */
	return pcxhr_check_reg_bit(mgr, PCXHR_DSP_ISR, PCXHR_ISR_HI08_CHK, PCXHR_ISR_HI08_CHK, PCXHR_TIMEOUT_DSP, &dummy);
}


struct pcxhr_cmd_info {
	u32 opcode;		/* command word */
	u16 st_length;		/* status length */
	u16 st_type;		/* status type (RMH_SSIZE_XXX) */
};
/* RMH status type */
enum {
	RMH_SSIZE_FIXED = 0,	/* status size fix (st_length = 0..x) */
	RMH_SSIZE_ARG = 1,	/* status size given in the LSB byte (used with st_length = 1) */
	RMH_SSIZE_MASK = 2,	/* status size given in bitmask  (used with st_length = 1) */
};

/*
 * Array of DSP commands
 */
static struct pcxhr_cmd_info pcxhr_dsp_cmds[] = {
[CMD_VERSION] =				{ 0x010000, 1, RMH_SSIZE_FIXED },
[CMD_SUPPORTED] =			{ 0x020000, 4, RMH_SSIZE_FIXED },
[CMD_TEST_IT] =				{ 0x040000, 1, RMH_SSIZE_FIXED },
[CMD_SEND_IRQA] =			{ 0x070001, 0, RMH_SSIZE_FIXED },
[CMD_ACCESS_IO_WRITE] =			{ 0x090000, 1, RMH_SSIZE_ARG },
[CMD_ACCESS_IO_READ] =			{ 0x094000, 1, RMH_SSIZE_ARG },
[CMD_ASYNC] =				{ 0x0a0000, 1, RMH_SSIZE_ARG },
[CMD_MODIFY_CLOCK] =			{ 0x0d0000, 0, RMH_SSIZE_FIXED },
[CMD_GET_DSP_RESOURCES] =		{ 0x100000, 4, RMH_SSIZE_FIXED },
[CMD_SET_TIMER_INTERRUPT] =		{ 0x110000, 0, RMH_SSIZE_FIXED },
[CMD_RES_PIPE] =			{ 0x400000, 0, RMH_SSIZE_FIXED },
[CMD_FREE_PIPE] =			{ 0x410000, 0, RMH_SSIZE_FIXED },
[CMD_CONF_PIPE] =			{ 0x422101, 0, RMH_SSIZE_FIXED },
[CMD_STOP_PIPE] =			{ 0x470004, 0, RMH_SSIZE_FIXED },
[CMD_PIPE_SAMPLE_COUNT] =		{ 0x49a000, 2, RMH_SSIZE_FIXED },
[CMD_CAN_START_PIPE] =			{ 0x4b0000, 1, RMH_SSIZE_FIXED },
[CMD_START_STREAM] =			{ 0x802000, 0, RMH_SSIZE_FIXED },
[CMD_STREAM_OUT_LEVEL_ADJUST] =		{ 0x822000, 0, RMH_SSIZE_FIXED },
[CMD_STOP_STREAM] =			{ 0x832000, 0, RMH_SSIZE_FIXED },
[CMD_UPDATE_R_BUFFERS] =		{ 0x840000, 0, RMH_SSIZE_FIXED },
[CMD_FORMAT_STREAM_OUT] =		{ 0x860000, 0, RMH_SSIZE_FIXED },
[CMD_FORMAT_STREAM_IN] =		{ 0x870000, 0, RMH_SSIZE_FIXED },
[CMD_STREAM_SAMPLE_COUNT] =		{ 0x902000, 2, RMH_SSIZE_FIXED },	/* stat_len = nb_streams * 2 */
[CMD_AUDIO_LEVEL_ADJUST] =		{ 0xc22000, 0, RMH_SSIZE_FIXED },
};
#ifdef CONFIG_SND_DEBUG
static char* cmd_names[] = {
[CMD_VERSION] =				"CMD_VERSION",
[CMD_SUPPORTED] =			"CMD_SUPPORTED",
[CMD_TEST_IT] =				"CMD_TEST_IT",
[CMD_SEND_IRQA] =			"CMD_SEND_IRQA",
[CMD_ACCESS_IO_WRITE] =			"CMD_ACCESS_IO_WRITE",
[CMD_ACCESS_IO_READ] =			"CMD_ACCESS_IO_READ",
[CMD_ASYNC] =				"CMD_ASYNC",
[CMD_MODIFY_CLOCK] =			"CMD_MODIFY_CLOCK",
[CMD_GET_DSP_RESOURCES] =		"CMD_GET_DSP_RESOURCES",
[CMD_SET_TIMER_INTERRUPT] =		"CMD_SET_TIMER_INTERRUPT",
[CMD_RES_PIPE] =			"CMD_RES_PIPE",
[CMD_FREE_PIPE] =			"CMD_FREE_PIPE",
[CMD_CONF_PIPE] =			"CMD_CONF_PIPE",
[CMD_STOP_PIPE] =			"CMD_STOP_PIPE",
[CMD_PIPE_SAMPLE_COUNT] =		"CMD_PIPE_SAMPLE_COUNT",
[CMD_CAN_START_PIPE] =			"CMD_CAN_START_PIPE",
[CMD_START_STREAM] =			"CMD_START_STREAM",
[CMD_STREAM_OUT_LEVEL_ADJUST] =		"CMD_STREAM_OUT_LEVEL_ADJUST",
[CMD_STOP_STREAM] =			"CMD_STOP_STREAM",
[CMD_UPDATE_R_BUFFERS] =		"CMD_UPDATE_R_BUFFERS",
[CMD_FORMAT_STREAM_OUT] =		"CMD_FORMAT_STREAM_OUT",
[CMD_FORMAT_STREAM_IN] =		"CMD_FORMAT_STREAM_IN",
[CMD_STREAM_SAMPLE_COUNT] =		"CMD_STREAM_SAMPLE_COUNT",
[CMD_AUDIO_LEVEL_ADJUST] =		"CMD_AUDIO_LEVEL_ADJUST",
};
#endif


static int pcxhr_read_rmh_status(pcxhr_mgr_t *mgr, pcxhr_rmh_t *rmh)
{
	int err;
	int i;
	u32 data;
	u32 size_mask;
	unsigned char reg;
	for(i=0; i<rmh->stat_len; i++) {
		/* wait for receiver full */
		err = pcxhr_check_reg_bit(mgr, PCXHR_DSP_ISR, PCXHR_ISR_HI08_RXDF, PCXHR_ISR_HI08_RXDF, PCXHR_TIMEOUT_DSP, &reg);
		if(err) {
			snd_printk(KERN_ERR "ERROR RMH stat: ISR:RXDF=1 (ISR = %x; i=%d )\n", reg, i);
			return err;
		}
		/* read data */
		data  = PCXHR_INPB(mgr, PCXHR_DSP_TXH) << 16;
		data |= PCXHR_INPB(mgr, PCXHR_DSP_TXM) << 8;
		data |= PCXHR_INPB(mgr, PCXHR_DSP_TXL);

		/* need to update rmh->stat_len on the fly ?? */
		if(i==0) {
			if(rmh->dsp_stat != RMH_SSIZE_FIXED) {
				if(rmh->dsp_stat == RMH_SSIZE_ARG) {
					rmh->stat_len = (u16)(data & 0x0000ff) + 1;
					data &= 0xffff00;
				} else {
					/* rmh->dsp_stat == RMH_SSIZE_MASK */
					rmh->stat_len = 1;
					size_mask = data;
					while(size_mask) {
						if(size_mask & 1) rmh->stat_len++;
						size_mask >>= 1;
					}
				}
			}
		}
#ifdef CONFIG_SND_DEBUG
		if(rmh->cmd_idx < CMD_LAST_INDEX)
			snd_printdd("    stat[%d]=%x\n", i, data);
#endif
		if(i < PCXHR_SIZE_MAX_STATUS) rmh->stat[i] = data;
	}
	if(rmh->stat_len > PCXHR_SIZE_MAX_STATUS) {
		snd_printdd("PCXHR : rmh->stat_len=%x too big\n", rmh->stat_len);
		rmh->stat_len = PCXHR_SIZE_MAX_STATUS;
	}
	return 0;
}

static int pcxhr_send_msg_nolock(pcxhr_mgr_t *mgr, pcxhr_rmh_t *rmh)
{
	int err;
	int i;
	u32 data;
	unsigned char reg;

	snd_assert(rmh->cmd_len<PCXHR_SIZE_MAX_CMD, return -EINVAL);
	err = pcxhr_send_it_dsp(mgr, PCXHR_IT_MESSAGE);
	if(err) {
		snd_printk(KERN_ERR "pcxhr_send_message : ED_DSP_CRASHED\n");
		return err;
	}
	/* wait for chk bit */
	err = pcxhr_check_reg_bit(mgr, PCXHR_DSP_ISR, PCXHR_ISR_HI08_CHK, PCXHR_ISR_HI08_CHK, PCXHR_TIMEOUT_DSP, &reg);
	if(err) return err;
	/* reset irq chk */
	err = pcxhr_send_it_dsp(mgr, PCXHR_IT_RESET_CHK);
	if(err) return err;
	/* wait for chk bit == 0*/
	err = pcxhr_check_reg_bit(mgr, PCXHR_DSP_ISR, PCXHR_ISR_HI08_CHK, 0, PCXHR_TIMEOUT_DSP, &reg);
	if(err) return err;

	data = rmh->cmd[0];

	if(rmh->cmd_len > 1)	data |= 0x008000;	/* MASK_MORE_THAN_1_WORD_COMMAND */
	else			data &= 0xff7fff;	/* MASK_1_WORD_COMMAND */
#ifdef CONFIG_SND_DEBUG
	if(rmh->cmd_idx < CMD_LAST_INDEX)
		snd_printdd("MSG cmd[0]=%x (%s)\n", data, cmd_names[rmh->cmd_idx]);
#endif

	err = pcxhr_check_reg_bit(mgr, PCXHR_DSP_ISR, PCXHR_ISR_HI08_TRDY, PCXHR_ISR_HI08_TRDY, PCXHR_TIMEOUT_DSP, &reg);
	if(err) return err;
	PCXHR_OUTPB(mgr, PCXHR_DSP_TXH, (data>>16)&0xFF);
	PCXHR_OUTPB(mgr, PCXHR_DSP_TXM, (data>>8)&0xFF);
	PCXHR_OUTPB(mgr, PCXHR_DSP_TXL, (data&0xFF));

	if(rmh->cmd_len > 1) {
		/* send length */
		data = rmh->cmd_len - 1;
		err = pcxhr_check_reg_bit(mgr, PCXHR_DSP_ISR, PCXHR_ISR_HI08_TRDY, PCXHR_ISR_HI08_TRDY, PCXHR_TIMEOUT_DSP, &reg);
		if(err) return err;
		PCXHR_OUTPB(mgr, PCXHR_DSP_TXH, (data>>16)&0xFF);
		PCXHR_OUTPB(mgr, PCXHR_DSP_TXM, (data>>8)&0xFF);
		PCXHR_OUTPB(mgr, PCXHR_DSP_TXL, (data&0xFF));

		for(i=1; i<rmh->cmd_len; i++) {
			/* send other words */
			data = rmh->cmd[i];
#ifdef CONFIG_SND_DEBUG
			if(rmh->cmd_idx < CMD_LAST_INDEX)
				snd_printdd("    cmd[%d]=%x\n", i, data);
#endif
			err = pcxhr_check_reg_bit(mgr, PCXHR_DSP_ISR, PCXHR_ISR_HI08_TRDY, PCXHR_ISR_HI08_TRDY, PCXHR_TIMEOUT_DSP, &reg);
			if(err) return err;
			PCXHR_OUTPB(mgr, PCXHR_DSP_TXH, (data>>16)&0xFF);
			PCXHR_OUTPB(mgr, PCXHR_DSP_TXM, (data>>8)&0xFF);
			PCXHR_OUTPB(mgr, PCXHR_DSP_TXL, (data&0xFF));
		}
	}
	/* wait for chk bit */
	err = pcxhr_check_reg_bit(mgr, PCXHR_DSP_ISR, PCXHR_ISR_HI08_CHK, PCXHR_ISR_HI08_CHK, PCXHR_TIMEOUT_DSP, &reg);
	if(err) return err;
	/* test status ISR */
	if(reg & PCXHR_ISR_HI08_ERR) {
		/* ERROR, wait for receiver full */
		err = pcxhr_check_reg_bit(mgr, PCXHR_DSP_ISR, PCXHR_ISR_HI08_RXDF, PCXHR_ISR_HI08_RXDF, PCXHR_TIMEOUT_DSP, &reg);
		if(err) {
			snd_printk(KERN_ERR "ERROR RMH: ISR:RXDF=1 (ISR = %x)\n", reg);
			return err;
		}
		/* read error code */
		data  = PCXHR_INPB(mgr, PCXHR_DSP_TXH) << 16;
		data |= PCXHR_INPB(mgr, PCXHR_DSP_TXM) << 8;
		data |= PCXHR_INPB(mgr, PCXHR_DSP_TXL);
		snd_printk(KERN_ERR "ERROR RMH(%d): 0x%x\n", rmh->cmd_idx, data);
		err = -EINVAL;
	} else {
		/* read the response data */
		err = pcxhr_read_rmh_status(mgr, rmh);
	}
	/* reset semaphore */
	if( pcxhr_send_it_dsp(mgr, PCXHR_IT_RESET_SEMAPHORE) < 0 ) {
		return -EIO;
	}
	return err;
}


/**
 * pcxhr_init_rmh - initialize the RMH instance
 * @rmh: the rmh pointer to be initialized
 * @cmd: the rmh command to be set
 */
void pcxhr_init_rmh(pcxhr_rmh_t *rmh, int cmd)
{
	snd_assert(cmd < CMD_LAST_INDEX, return);
	rmh->cmd[0] = pcxhr_dsp_cmds[cmd].opcode;
	rmh->cmd_len = 1;
	rmh->stat_len = pcxhr_dsp_cmds[cmd].st_length;
	rmh->dsp_stat = pcxhr_dsp_cmds[cmd].st_type;
	rmh->cmd_idx = cmd;
}


void pcxhr_set_pipe_cmd_params(pcxhr_rmh_t* rmh, int capture, unsigned int param1, unsigned int param2, unsigned int param3)
{
	snd_assert(param1 <= MASK_FIRST_FIELD);
	if(capture) rmh->cmd[0] |= 0x800;		/* COMMAND_RECORD_MASK */
	if(param1)  rmh->cmd[0] |= (param1 << FIELD_SIZE);
	if(param2) {
		snd_assert(param2 <= MASK_FIRST_FIELD);
		rmh->cmd[0] |= param2;
	}
	if(param3) {
		snd_assert(param3 <= MASK_DSP_WORD);
		rmh->cmd[1] = param3;
		rmh->cmd_len = 2;
	}
}

/*
 * pcxhr_send_msg - send a DSP message with spinlock
 * @rmh: the rmh record to send and receive
 *
 * returns 0 if successful, or a negative error code.
 */
int pcxhr_send_msg(pcxhr_mgr_t *mgr, pcxhr_rmh_t *rmh)
{
	unsigned long flags;
	int err;
	spin_lock_irqsave(&mgr->msg_lock, flags);
	err = pcxhr_send_msg_nolock(mgr, rmh);
	spin_unlock_irqrestore(&mgr->msg_lock, flags);
	return err;
}

int pcxhr_is_pipe_running(pcxhr_mgr_t *mgr, int is_capture, int audio)
{
	unsigned int start_mask = PCXHR_INPL(mgr, PCXHR_PLX_MBOX2);
	snd_printdd("CMD_PIPE_STATE MBOX2=0x%06x audio %c %d\n", start_mask & 0xffffff, is_capture?'C':'P', audio);
	if(is_capture) start_mask >>= 12;
	return (start_mask & (1<<audio));
}

int pcxhr_write_io_num_reg_cont(pcxhr_mgr_t *mgr, unsigned int mask, unsigned int value, int *changed)
{
	pcxhr_rmh_t rmh;
	int err;
	if((mgr->io_num_reg_cont & mask) == value) {
		snd_printdd("IO_NUM_REG_CONT mask %x already is set to %x\n", mask, value);
		if(changed) *changed = 0;
		return 0;	/* already programmed */
	}
	pcxhr_init_rmh(&rmh, CMD_ACCESS_IO_WRITE);
	rmh.cmd[0] |= IO_NUM_REG_CONT;
	rmh.cmd[1]  = mask;
	rmh.cmd[2]  = value;
	rmh.cmd_len = 3;
	err = pcxhr_send_msg(mgr, &rmh);
	if(err == 0) {
		mgr->io_num_reg_cont &= ~mask;
		mgr->io_num_reg_cont |= value;
		if(changed) *changed = 1;
	}
	return err;
}

#define PCXHR_IRQ_TIMER		0x000300
#define PCXHR_IRQ_FREQ_CHANGE	0x000800
#define PCXHR_IRQ_TIME_CODE	0x001000
#define PCXHR_IRQ_NOTIFY	0x002000
#define PCXHR_IRQ_ASYNC		0x008000
#define PCXHR_IRQ_MASK		0x00bb00
#define PCXHR_FATAL_DSP_ERR	0xff0000

void pcxhr_msg_tasklet( unsigned long arg)
{
	pcxhr_mgr_t *mgr = ( pcxhr_mgr_t*)(arg);
	pcxhr_rmh_t rmh;
	int err;
#ifdef CONFIG_SND_DEBUG
	int i, j;
#endif
	if(mgr->src_it_dsp & PCXHR_IRQ_FREQ_CHANGE)
		snd_printdd("TASKLET : PCXHR_IRQ_FREQ_CHANGE event occured\n");
	if(mgr->src_it_dsp & PCXHR_IRQ_TIME_CODE)
		snd_printdd("TASKLET : PCXHR_IRQ_TIME_CODE event occured\n");
	if(mgr->src_it_dsp & PCXHR_IRQ_NOTIFY)
		snd_printdd("TASKLET : PCXHR_IRQ_NOTIFY event occured\n");
	if(mgr->src_it_dsp & PCXHR_IRQ_ASYNC) {
		snd_printdd("TASKLET : PCXHR_IRQ_ASYNC event occured\n");

		pcxhr_init_rmh(&rmh, CMD_ASYNC);
		rmh.cmd[0] |= 1;	/* add SEL_ASYNC_EVENTS */
		err = pcxhr_send_msg(mgr, &rmh);
		if(err) {
			snd_printk(KERN_ERR "ERROR pcxhr_msg_tasklet=%x;\n", err);
		}
#ifdef CONFIG_SND_DEBUG
		i=1;
		while(i<rmh.stat_len) {
			int nb_audio = (rmh.stat[i] >> FIELD_SIZE) & MASK_FIRST_FIELD;
			int nb_stream = (rmh.stat[i] >> (2*FIELD_SIZE)) & MASK_FIRST_FIELD;
#ifdef CONFIG_SND_DEBUG_DETECT
			int pipe = rmh.stat[i] & MASK_FIRST_FIELD;
			int is_capture = rmh.stat[i] & 0x400000;
#endif
			if(rmh.stat[i] & 0x800000) {	/* if BIT_END */
				snd_printdd("TASKLET : End%sPipe %d\n", is_capture ? "Record" : "Play", pipe);
			}
			i++;
			if( rmh.stat[i] || rmh.stat[i+1] ) {
				snd_printdd("TASKLET : Error %s Pipe %d err=%x-%x\n", is_capture ? "Record" : "Play", pipe, rmh.stat[i], rmh.stat[i+1]);
			}
			i+=2;
			for(j=0; j<nb_stream; j++) {
				if( rmh.stat[i] || rmh.stat[i+1] ) {
					snd_printdd("TASKLET : Error %s Pipe %d Stream %d err=%x-%x\n",
						    is_capture ? "Record" : "Play", pipe, j,  rmh.stat[i],  rmh.stat[i+1]);
				}
				i+=2;
			}
			for(j=0; j<nb_audio; j++) {
				if( rmh.stat[i] || rmh.stat[i+1] ) {
					snd_printdd("TASKLET : Error %s Pipe %d Audio %d err=%x- %x\n",
						    is_capture ? "Record" : "Play", pipe, j,  rmh.stat[i], rmh.stat[i+1]);
				}
				i+=2;
			}
		}
#endif
	}
}

static int pcxhr_stream_read_position(pcxhr_mgr_t *mgr, pcxhr_stream_t *stream)
{
	snd_pcm_uframes_t sample_count;
	pcxhr_rmh_t rmh;
	int err, stream_mask;

	stream_mask = stream->pipe->is_capture ? 1 : 1<<stream->substream->number;

	/* get sample count for one stream */
	pcxhr_init_rmh(&rmh, CMD_STREAM_SAMPLE_COUNT);
	pcxhr_set_pipe_cmd_params(&rmh, stream->pipe->is_capture, stream->pipe->first_audio, 0, stream_mask);
	/* rmh.stat_len = 2; */		/* 2 resp data for each stream of the pipe */

	err = pcxhr_send_msg(mgr, &rmh);
	if( err ) return err;

	sample_count = ((snd_pcm_uframes_t)rmh.stat[0]) << 24;
	sample_count += (snd_pcm_uframes_t)rmh.stat[1];

	snd_printdd("stream abs samples real(%lx) timer(%lx)\n", sample_count, stream->timer_abs_samples);

	/* adjust the absolute timer position */
	stream->timer_abs_samples = sample_count;
	if( sample_count != 0 )  stream->timer_abs_adjusted = 1;
	return 0;
}

static void pcxhr_update_timer_pos(pcxhr_mgr_t *mgr, pcxhr_stream_t*stream)
{
	int updated = 0;
	if(stream->substream && (stream->status == PCXHR_STREAM_STATUS_RUNNING)) {
		stream->timer_elapsed += PCXHR_GRANULARITY;
		stream->timer_abs_samples += PCXHR_GRANULARITY;
		if(stream->timer_elapsed >= stream->substream->runtime->period_size ) {
			/* first time, lets adjust stream->timer_abs_samples to its real value */
			if(!stream->timer_abs_adjusted) {
				if(pcxhr_stream_read_position(mgr, stream) == 0)
					updated = 1;
			}
			/* adjust the timer position */
			stream->timer_elapsed = stream->timer_abs_samples % stream->substream->runtime->period_size;

			if(updated) {
				if(stream->timer_abs_samples < stream->substream->runtime->period_size) {
					/* it is too early to call snd_pcm_period_elapsed ! */
					return;
				}
			}
			spin_unlock(&mgr->lock);
			snd_pcm_period_elapsed(stream->substream);
			spin_lock(&mgr->lock);
		}
	}
}

irqreturn_t pcxhr_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	pcxhr_mgr_t *mgr = dev_id;
	unsigned int reg;
	int i, j;
	pcxhr_t *chip;

	spin_lock(&mgr->lock);

	reg = PCXHR_INPL(mgr, PCXHR_PLX_IRQCS);
	if( reg & PCXHR_IRQCS_ACTIVE_PCIDB ) {

		/* clear interrupt */
		reg = PCXHR_INPL(mgr, PCXHR_PLX_L2PCIDB);
		PCXHR_OUTPL(mgr, PCXHR_PLX_L2PCIDB, reg);

		/* timer irq occured */
		if(reg & PCXHR_IRQ_TIMER) {
			int timer_toggle = reg & PCXHR_IRQ_TIMER;
			if(timer_toggle == mgr->timer_toggle) {
				snd_printk(KERN_DEBUG "ERROR TIMER TOGGLE\n");
				mgr->timer_err++;
			}
			mgr->timer_toggle = timer_toggle;
			reg &= ~PCXHR_IRQ_TIMER;
			for(i=0; i<mgr->num_cards; i++) {
				chip = mgr->chip[i];
				for(j=0; j<chip->nb_streams_play; j++) {
					pcxhr_update_timer_pos(mgr, &chip->playback_stream[j]);
				}
				for(j=0; j<chip->nb_streams_capt; j++) {
					pcxhr_update_timer_pos(mgr, &chip->capture_stream[j]);
				}
			}
		}
		/* other irq's handled in the tasklet */
		if( reg & PCXHR_IRQ_MASK ) {
			mgr->src_it_dsp = reg;
			tasklet_hi_schedule(&mgr->msg_taskq);
		}
		if( reg & PCXHR_FATAL_DSP_ERR ) {
			snd_printdd("FATAL DSP ERROR : %x\n", reg);
		}
		spin_unlock(&mgr->lock);
		return IRQ_HANDLED;	/* this device caused the interrupt */
	}
	spin_unlock(&mgr->lock);
	return IRQ_NONE;		/* this device did not cause the interrupt */
}
