/*
 * Driver for Digigram VXpocket soundcards
 *
 * Hardware core part
 *
 * Copyright (c) 2002 by Takashi Iwai <tiwai@suse.de>
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
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/asoundef.h>
#include <sound/info.h>
#include "vxpocket.h"
#include <asm/io.h>

MODULE_AUTHOR("Takashi Iwai <tiwai@suse.de>");
MODULE_DESCRIPTION("Common routines for VXPocket drivers");
MODULE_LICENSE("GPL");


/*
 * low-level functions
 */

/*
 * snd_vx_inb - read a byte from the register
 * @offset: register offset
 */
int snd_vx_inb(vxpocket_t *chip, int offset)
{
	return inb(chip->port + offset);
}

/*
 * snd_vx_outb - write a byte on the register
 * @offset: the register offset
 * @val: the value to write
 */
void snd_vx_outb(vxpocket_t *chip, int offset, int val)
{
	outb(val, chip->port + offset);
}


/*
 * vx_delay - delay for the specified time
 * @xmsec: the time to delay in msec
 */
void vx_delay(vxpocket_t *chip, int xmsec)
{
	if (! chip->in_suspend && ! in_interrupt() &&
	    xmsec >= 1000 / HZ) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout((xmsec * HZ + 999) / 1000);
	} else {
		mdelay(xmsec);
	}
}

/*
 * vx_check_reg_bit - wait for the specified bit is set/reset on a register
 * @reg: register to check
 * @mask: bit mask
 * @bit: resultant bit to be checked
 * @time: time-out of loop in msec
 *
 * returns zero if a bit matches, or a negative error code.
 */
int vx_check_reg_bit(vxpocket_t *chip, int reg, int mask, int bit, int time)
{
#if 1 // debug only
	static char *reg_names[16] = { "ICR", "CVR", "ISR", "IVR", "DMA",
				       "R/TXH", "R/TXM", "R/TXL", "CDSP",
				       "LOFREQ", "HIFREQ", "DATA", "MICRO",
				       "DIALOG", "CSUER", "RUER" };
#endif
	long end_time = jiffies + (time * HZ + 999) / 1000;
	do {
		if ((snd_vx_inb(chip, reg) & mask) == bit)
			return 0;
		//vx_delay(chip, 10);
	} while (time_after_eq(end_time, jiffies));
	printk(KERN_DEBUG "vx_check_reg_bit: timeout, reg=%s, mask=0x%x, val=0x%x\n", reg_names[reg], mask, snd_vx_inb(chip, reg));
	return -EIO;
}

/*
 * vx_check_isr - check the ISR bit
 *
 * returns zero if a bit matches, or a negative error code.
 */
#define vx_check_isr(chip,mask,bit,time) vx_check_reg_bit(chip, VXP_ISR, mask, bit, time)


/*
 * vx_check_magic - check the magic word on xilinx
 *
 * returns zero if a magic word is detected, or a negative error code.
 */
static int vx_check_magic(vxpocket_t *chip)
{
	long end_time = jiffies + HZ / 5;
	int c;
	do {
		c = vx_inb(chip, CDSP);
		if (c == CDSP_MAGIC)
			return 0;
		vx_delay(chip, 10);
	} while (time_after_eq(end_time, jiffies));
	printk(KERN_ERR "cannot find xilinx magic word (%x)\n", c);
	return -EIO;
}


/*
 * vx_test_xilinx - check whether the xilinx is initialized
 *
 * returns zero if xilinx was already initialized, or a negative error code.
 */
static int vx_test_xilinx(vxpocket_t *chip)
{
	chip->xilinx_tested = 1; /* always ok */
	return 0;
}


/*
 * vx_reset_dsp - reset the DSP
 */

#define END_OF_RESET_WAIT_TIME		200	/* ms */
#define XX_DSP_RESET_WAIT_TIME		2	/* ms */

static void vx_reset_dsp(vxpocket_t *chip)
{
	/* set the reset dsp bit to 1 */
	vx_outb(chip, CDSP, chip->regCDSP | VXP_CDSP_DSP_RESET_MASK);
	mdelay(XX_DSP_RESET_WAIT_TIME);
	/* reset the bit */
	vx_outb(chip, CDSP, chip->regCDSP & ~VXP_CDSP_DSP_RESET_MASK);
	mdelay(XX_DSP_RESET_WAIT_TIME);
}


/*
 * vx_send_irq_dsp - set command irq bit
 * @num: the requested IRQ type, IRQ_XXX
 *
 * this triggers the specified IRQ request
 * returns 0 if successful, or a negative error code.
 * 
 */
static int vx_send_irq_dsp(vxpocket_t *chip, int num)
{
	int nirq;

	/* wait for Hc = 0 */
	if (vx_check_reg_bit(chip, VXP_CVR, CVR_HC, 0, 200) < 0)
		return -EIO;

	nirq = num + VXP_IRQ_OFFSET;
	vx_outb(chip, CVR, (nirq >> 1) | CVR_HC);
	return 0;
}


/*
 * vx_reset_chk - reset CHK bit on ISR
 *
 * returns 0 if successful, or a negative error code.
 */
static int vx_reset_chk(vxpocket_t *chip)
{
	/* Reset irq CHK */
	if (vx_send_irq_dsp(chip, IRQ_RESET_CHK) < 0)
		return -EIO;
	/* Wait until CHK = 0 */
	if (vx_check_isr(chip, ISR_CHK, 0, 200) < 0)
		return -EIO;
	return 0;
}

#define vx_wait_isr_bit(chip,bit) vx_check_isr(chip, bit, bit, 200)
#define vx_wait_for_rx_full(chip) vx_wait_isr_bit(chip, ISR_RX_FULL)

/*
 * vx_transfer_end - terminate message transfer
 * @cmd: IRQ message to send (IRQ_MESS_XXX_END)
 *
 * returns 0 if successful, or a negative error code.
 * the error code can be VX-specific, retrieved via vx_get_error().
 * NB: call with spinlock held!
 */
static int vx_transfer_end(vxpocket_t *chip, int cmd)
{
	int err;

	if ((err = vx_reset_chk(chip)) < 0)
		return err;

	/* irq MESS_READ/WRITE_END */
	if ((err = vx_send_irq_dsp(chip, cmd)) < 0)
		return err;

	/* Wait CHK = 1 */
	if ((err = vx_wait_isr_bit(chip, ISR_CHK)) < 0)
		return err;

	/* If error, Read RX */
	if ((err = vx_inb(chip, ISR)) & ISR_ERR) {
		if ((err = vx_wait_for_rx_full(chip)) < 0) {
			snd_printd(KERN_DEBUG "transfer_end: error in rx_full\n");
			return err;
		}
		err = vx_inb(chip, RXH) << 16;
		err |= vx_inb(chip, RXM) << 8;
		err |= vx_inb(chip, RXL);
		snd_printd(KERN_DEBUG "transfer_end: error = 0x%x\n", err);
		return -(VXP_ERR_MASK | err);
	}
	return 0;
}

/*
 * vx_read_status - return the status rmh
 * @rmh: rmh record to store the status
 *
 * returns 0 if successful, or a negative error code.
 * the error code can be VX-specific, retrieved via vx_get_error().
 * NB: call with spinlock held!
 */
static int vx_read_status(vxpocket_t *chip, struct vx_rmh *rmh)
{
	int i, err, val, size;

	/* no read necessary? */
	if (rmh->DspStat == RMH_SSIZE_FIXED && rmh->LgStat == 0)
		return 0;

	/* Wait for RX full (with timeout protection)
	 * The first word of status is in RX
	 */
	err = vx_wait_for_rx_full(chip);
	if (err < 0)
		return err;

	/* Read RX */
	val = vx_inb(chip, RXH) << 16;
	val |= vx_inb(chip, RXM) << 8;
	val |= vx_inb(chip, RXL);

	/* If status given by DSP, let's decode its size */
	switch (rmh->DspStat) {
	case RMH_SSIZE_ARG:
		size = val & 0xff;
		rmh->Stat[0] = val & 0xffff00;
		rmh->LgStat = size + 1;
		break;
	case RMH_SSIZE_MASK:
		/* Let's count the arg numbers from a mask */
		rmh->Stat[0] = val;
		size = 0;
		while (val) {
			if (val & 0x01)
				size++;
			val >>= 1;
		}
		rmh->LgStat = size + 1;
		break;
	default:
		/* else retrieve the status length given by the driver */
		size = rmh->LgStat;
		rmh->Stat[0] = val;  /* Val is the status 1st word */
		size--;              /* hence adjust remaining length */
		break;
        }

	if (size < 1)
		return 0;
	snd_assert(size <= SIZE_MAX_STATUS, return -EINVAL);

	for (i = 1; i <= size; i++) {
		/* trigger an irq MESS_WRITE_NEXT */
		err = vx_send_irq_dsp(chip, IRQ_MESS_WRITE_NEXT);
		if (err < 0)
			return err;
		/* Wait for RX full (with timeout protection) */
		err = vx_wait_for_rx_full(chip);
		if (err < 0)
			return err;
		rmh->Stat[i] = vx_inb(chip, RXH) << 16;
		rmh->Stat[i] |= vx_inb(chip, RXM) <<  8;
		rmh->Stat[i] |= vx_inb(chip, RXL);
	}

	return vx_transfer_end(chip, IRQ_MESS_WRITE_END);
}


#define MASK_MORE_THAN_1_WORD_COMMAND   0x00008000
#define MASK_1_WORD_COMMAND             0x00ff7fff

/*
 * vx_send_msg_nolock - send a DSP message and read back the status
 * @rmh: the rmh record to send and receive
 *
 * returns 0 if successful, or a negative error code.
 * the error code can be VX-specific, retrieved via vx_get_error().
 * 
 * this function doesn't call spinlock at all.
 */
int vx_send_msg_nolock(vxpocket_t *chip, struct vx_rmh *rmh)
{
	int i, err;
	
	if (chip->is_stale)
		return -EBUSY;

	if ((err = vx_reset_chk(chip)) < 0) {
		snd_printd(KERN_DEBUG "vx_send_msg: vx_reset_chk error\n");
		return err;
	}

#if 0
	printk(KERN_DEBUG "rmh: cmd = 0x%06x, length = %d, stype = %d\n",
	       rmh->Cmd[0], rmh->LgCmd, rmh->DspStat);
	if (rmh->LgCmd > 1) {
		printk(KERN_DEBUG "  ");
		for (i = 1; i < rmh->LgCmd; i++)
			printk("0x%06x ", rmh->Cmd[i]);
		printk("\n");
	}
#endif
	/* Check bit M is set according to length of the command */
	if (rmh->LgCmd > 1)
		rmh->Cmd[0] |= MASK_MORE_THAN_1_WORD_COMMAND;
	else
		rmh->Cmd[0] &= MASK_1_WORD_COMMAND;

	/* Wait for TX empty */
	if ((err = vx_wait_isr_bit(chip, ISR_TX_EMPTY)) < 0) {
		snd_printd(KERN_DEBUG "vx_send_msg: wait tx empty error\n");
		return err;
	}

	/* Write Cmd[0] */
	vx_outb(chip, TXH, (rmh->Cmd[0] >> 16) & 0xff);
	vx_outb(chip, TXM, (rmh->Cmd[0] >> 8) & 0xff);
	vx_outb(chip, TXL, rmh->Cmd[0] & 0xff);

	/* Trigger irq MESSAGE */
	if ((err = vx_send_irq_dsp(chip, IRQ_MESSAGE)) < 0) {
		snd_printd(KERN_DEBUG "vx_send_msg: send IRQ_MESSAGE error\n");
		return err;
	}

	/* Wait for CHK = 1 */
	if ((err = vx_wait_isr_bit(chip, ISR_CHK)) < 0)
		return err;

	/* If error, get error value from RX */
	if (vx_inb(chip, ISR) & ISR_ERR) {
		if ((err = vx_wait_for_rx_full(chip)) < 0) {
			snd_printd(KERN_DEBUG "vx_send_msg: rx_full read error\n");
			return err;
		}
		err = vx_inb(chip, RXH) << 16;
		err |= vx_inb(chip, RXM) << 8;
		err |= vx_inb(chip, RXL);
		snd_printd(KERN_DEBUG "msg got error = 0x%x at cmd[0]\n", err);
		err = -(VXP_ERR_MASK | err);
		return err;
	}

	/* Send the other words */
	if (rmh->LgCmd > 1) {
		for (i = 1; i < rmh->LgCmd; i++) {
			/* Wait for TX ready */
			if ((err = vx_wait_isr_bit(chip, ISR_TX_READY)) < 0) {
				snd_printd(KERN_DEBUG "vx_send_msg: tx_ready error\n");
				return err;
			}

			/* Write Cmd[i] */
			vx_outb(chip, TXH, (rmh->Cmd[i] >> 16) & 0xff);
			vx_outb(chip, TXM, (rmh->Cmd[i] >> 8) & 0xff);
			vx_outb(chip, TXL, rmh->Cmd[i] & 0xff);

			/* Trigger irq MESS_READ_NEXT */
			if ((err = vx_send_irq_dsp(chip, IRQ_MESS_READ_NEXT)) < 0) {
				snd_printd(KERN_DEBUG "vx_send_msg: IRQ_READ_NEXT error\n");
				return err;
			}
		}
		/* Wait for TX empty */
		if ((err = vx_wait_isr_bit(chip, ISR_TX_READY)) < 0) {
			snd_printd(KERN_DEBUG "vx_send_msg: TX_READY error\n");
			return err;
		}
		/* End of transfer */
		err = vx_transfer_end(chip, IRQ_MESS_READ_END);
		if (err < 0)
			return err;
	}

	return vx_read_status(chip, rmh);
}


/*
 * vx_send_msg - send a DSP message with spinlock
 * @rmh: the rmh record to send and receive
 *
 * returns 0 if successful, or a negative error code.
 * see vx_send_msg_nolock().
 */
int vx_send_msg(vxpocket_t *chip, struct vx_rmh *rmh)
{
	int err;

	spin_lock_bh(&chip->lock);
	err = vx_send_msg_nolock(chip, rmh);
	spin_unlock_bh(&chip->lock);
	return err;
}


/*
 * vx_send_rih_nolock - send an RIH to xilinx
 * @cmd: the command to send
 *
 * returns 0 if successful, or a negative error code.
 * the error code can be VX-specific, retrieved via vx_get_error().
 *
 * this function doesn't call spinlock at all.
 *
 * unlike RMH, no command is sent to DSP.
 */
int vx_send_rih_nolock(vxpocket_t *chip, int cmd)
{
	int err;

	if (chip->is_stale)
		return -EBUSY;

#if 0
	printk(KERN_DEBUG "send_rih: cmd = 0x%x\n", cmd);
#endif
	if ((err = vx_reset_chk(chip)) < 0)
		return err;
	/* send the IRQ */
	if ((err = vx_send_irq_dsp(chip, cmd)) < 0)
		return err;
	/* Wait CHK = 1 */
	if ((err = vx_wait_isr_bit(chip, ISR_CHK)) < 0)
		return err;
	/* If error, read RX */
	if (vx_inb(chip, ISR) & ISR_ERR) {
		if ((err = vx_wait_for_rx_full(chip)) < 0)
			return err;
		err = vx_inb(chip, RXH) << 16;
		err |= vx_inb(chip, RXM) << 8;
		err |= vx_inb(chip, RXL);
		return -(VXP_ERR_MASK | err);
	}
	return 0;
}


/*
 * vx_send_rih - send an RIH with spinlock
 * @cmd: the command to send
 *
 * see vx_send_rih_nolock().
 */
int vx_send_rih(vxpocket_t *chip, int cmd)
{
	int err;

	spin_lock_bh(&chip->lock);
	err = vx_send_rih_nolock(chip, cmd);
	spin_unlock_bh(&chip->lock);
	return err;
}


/*
 * vx_boot_xilinx - boot up the xilinx interface
 * @boot: the boot record to load
 */
static int vx_load_boot_image(vxpocket_t *chip, const struct snd_vxp_image *boot)
{
	int c, i;

	snd_printdd(KERN_DEBUG "loading boot: size = %d\n", boot->length);

	/* check the length of boot image */
	snd_assert(boot->length % 3 == 0, return -EINVAL);
	c = ((u32)boot->image[0] << 16) | ((u32)boot->image[1] << 8) | boot->image[2];
	snd_assert(boot->length == (c + 2) * 3, return -EINVAL);

	/* reset dsp */
	vx_reset_dsp(chip);
	
	vx_delay(chip, END_OF_RESET_WAIT_TIME); /* another wait? */

	/* download boot strap */
	for (i = 0; i < boot->length; i += 3) {
		if (vx_wait_isr_bit(chip, ISR_TX_EMPTY) < 0)
			return -EIO;
		vx_outb(chip, TXH, boot->image[i]);
		vx_outb(chip, TXM, boot->image[i+1]);
		vx_outb(chip, TXL, boot->image[i+2]);
	}
	return 0;
}


/*
 * vx_load_xilinx_binary - load the xilinx binary image
 * the binary image is the binary array converted from the bitstream file.
 */
static int vx_load_xilinx_binary(vxpocket_t *chip)
{
	const struct snd_vxp_image *xilinx = &chip->hw->xilinx;
	int i, c, err;
	int regCSUER, regRUER;

	if ((err = vx_check_magic(chip)) < 0)
		return err;

	if ((err = vx_load_boot_image(chip, &chip->hw->boot)) < 0)
		return err;

	snd_printd(KERN_DEBUG "loading xilinx: size = %d\n", xilinx->length);
	/* Switch to programmation mode */
	chip->regDIALOG |= VXP_DLG_XILINX_REPROG_MASK;
	vx_outb(chip, DIALOG, chip->regDIALOG);

	/* Save register CSUER and RUER */
	regCSUER = vx_inb(chip, CSUER);
	regRUER = vx_inb(chip, RUER);

	/* reset HF0 and HF1 */
	vx_outb(chip, ICR, 0);

	/* Wait for answer HF2 equal to 1 */
	if (vx_check_isr(chip, ISR_HF2, ISR_HF2, 20) < 0)
		goto _error;

	/* set HF1 for loading xilinx binary */
	vx_outb(chip, ICR, ICR_HF1);
	for (i = 0; i < xilinx->length; i++) {
		if (vx_wait_isr_bit(chip, ISR_TX_EMPTY) < 0)
			goto _error;
		if (i == 0)
			printk("first outb: jiffies = %li\n", jiffies);
		vx_outb(chip, TXL, xilinx->image[i]);

		/* wait for reading */
		if (vx_wait_for_rx_full(chip) < 0)
			goto _error;
		c = vx_inb(chip, RXL);
		if (c != xilinx->image[i])
			printk(KERN_ERR "vxpocket: load xilinx mismatch at %d: 0x%x != 0x%x\n", i, c, xilinx->image[i]);
        }

	/* reset HF1 */
	vx_outb(chip, ICR, 0);

	/* wait for HF3 */
	if (vx_check_isr(chip, ISR_HF3, ISR_HF3, 20) < 0)
		goto _error;

	/* read the number of bytes received */
	if (vx_wait_for_rx_full(chip) < 0)
		goto _error;

	c = (int)vx_inb(chip, RXH) << 16;
	c |= (int)vx_inb(chip, RXM) << 8;
	c |= vx_inb(chip, RXL);

	snd_printd(KERN_DEBUG "xilinx: dsp size received 0x%x, orig 0x%x\n", c, xilinx->length);

	vx_outb(chip, ICR, ICR_HF0);

	/* TEMPO 250ms : wait until Xilinx is downloaded */
	vx_delay(chip, 300);

	/* test magical word */
	if (vx_check_magic(chip) < 0)
		goto _error;

	/* Restore register 0x0E and 0x0F (thus replacing COR and FCSR) */
	vx_outb(chip, CSUER, regCSUER);
	vx_outb(chip, RUER, regRUER);

	/* Reset the Xilinx's signal enabling IO access */
	chip->regDIALOG |= VXP_DLG_XILINX_REPROG_MASK;
	vx_outb(chip, DIALOG, chip->regDIALOG);
	vx_delay(chip, 10);
	chip->regDIALOG &= ~VXP_DLG_XILINX_REPROG_MASK;
	vx_outb(chip, DIALOG, chip->regDIALOG);

	/* Reset of the Codec */
	vx_reset_codec(chip);
	vx_reset_dsp(chip);

	return 0;

 _error:
	vx_outb(chip, CSUER, regCSUER);
	vx_outb(chip, RUER, regRUER);
	chip->regDIALOG &= ~VXP_DLG_XILINX_REPROG_MASK;
	vx_outb(chip, DIALOG, chip->regDIALOG);
	return -EIO;
}


/*
 * vx_load_dsp - load the DSP image
 */
static int vx_load_dsp(vxpocket_t *chip)
{
	const struct snd_vxp_image *dsp = &chip->hw->dsp;
	int i, err;
	unsigned int csum = 0;
	unsigned char *cptr;

	snd_assert(dsp->length % 3 == 0, return -EINVAL);

	vx_toggle_dac_mute(chip, 1);

	if ((err = vx_load_boot_image(chip, &chip->hw->dsp_boot)) < 0)
		return err;
	vx_delay(chip, 10);

	snd_printd(KERN_DEBUG "loading dsp: size = %d\n", dsp->length);
	/* Transfert data buffer from PC to DSP */
	cptr = dsp->image;
	for (i = 0; i < dsp->length; i += 3) {
		/* Wait DSP ready for a new read */
		if ((err = vx_wait_isr_bit(chip, ISR_TX_EMPTY)) < 0)
			return err;
		csum ^= *cptr;
		csum = (csum >> 24) | (csum << 8);
		vx_outb(chip, TXH, *cptr++);
		csum ^= *cptr;
		csum = (csum >> 24) | (csum << 8);
		vx_outb(chip, TXM, *cptr++);
		csum ^= *cptr;
		csum = (csum >> 24) | (csum << 8);
		vx_outb(chip, TXL, *cptr++);
	}
	snd_printd(KERN_DEBUG "checksum = 0x%08x\n", csum);

	vx_delay(chip, 200);

	err = vx_wait_isr_bit(chip, ISR_CHK);

	vx_toggle_dac_mute(chip, 0);

	return err;
}


#if 0
/* call CMD_VERSION */
static int vx_call_cmd_version(vxpocket_t *chip)
{
	struct vx_rmh rmh;

	rmh.LgStat = 1;
	rmh.DspStat = 0;
	rmh.LgCmd = 2;
	rmh.Cmd[0] = 0x010000;		/* CMD_VERSION */
	rmh.Cmd[0] |= chip->hw->type == VXP_TYPE_VXP440 ? 104 : 102;
	rmh.Cmd[1] = 2016;		/* DEFAULT_IBL */
	return vx_send_msg(chip, &rmh);
}
#endif


/*
 * vx_reset_board - perform reset
 */
void vx_reset_board(vxpocket_t *chip)
{
	chip->regCDSP = 0;
	chip->regDIALOG = 0;
	chip->audio_source = VXP_AUDIO_SRC_LINE;
	chip->audio_source_target = chip->audio_source;
	chip->clock_source = INTERNAL_QUARTZ;
	chip->freq = 48000;
	chip->uer_detected = VXP_UER_MODE_NOT_PRESENT;

	vx_reset_codec(chip);
	vx_set_internal_clock(chip, 48000);

	/* Reset the DSP */
	vx_reset_dsp(chip);

	/* Acknowledge any pending IRQ and reset the MEMIRQ flag. */
	vx_test_and_ack(chip);
	vx_validate_irq(chip, 1);

	/* init CBits */
	chip->uer_bits = SNDRV_PCM_DEFAULT_CON_SPDIF;
	vx_set_iec958_status(chip, SNDRV_PCM_DEFAULT_CON_SPDIF);
}


/*
 * proc interface
 */

static void vx_proc_read(snd_info_entry_t *entry, snd_info_buffer_t *buffer)
{
	vxpocket_t *chip = snd_magic_cast(vxpocket_t, entry->private_data, return);
	static char *audio_src[] = { "Line", "Mic", "Digital" };
	static char *clock_src[] = { "Internal", "External" };
	static char *uer_type[] = { "Consumer", "Professional", "Not Present" };
	
	snd_iprintf(buffer, "%s\n", chip->card->longname);
	snd_iprintf(buffer, "DSP audio info:");
	if (chip->audio_info & VX_AUDIO_INFO_REAL_TIME)
		snd_iprintf(buffer, " realtime");
	if (chip->audio_info & VX_AUDIO_INFO_OFFLINE)
		snd_iprintf(buffer, " offline");
	if (chip->audio_info & VX_AUDIO_INFO_MPEG1)
		snd_iprintf(buffer, " mpeg1");
	if (chip->audio_info & VX_AUDIO_INFO_MPEG2)
		snd_iprintf(buffer, " mpeg2");
	if (chip->audio_info & VX_AUDIO_INFO_LINEAR_8)
		snd_iprintf(buffer, " linear8");
	if (chip->audio_info & VX_AUDIO_INFO_LINEAR_16)
		snd_iprintf(buffer, " linear16");
	if (chip->audio_info & VX_AUDIO_INFO_LINEAR_24)
		snd_iprintf(buffer, " linear24");
	snd_iprintf(buffer, "\n");
	snd_iprintf(buffer, "Input Source: %s\n", audio_src[chip->audio_source]);
	snd_iprintf(buffer, "Clock Source: %s\n", clock_src[chip->clock_source]);
	snd_iprintf(buffer, "Frequency: %d\n", chip->freq);
	snd_iprintf(buffer, "Detected Frequency: %d\n", chip->freq_detected);
	snd_iprintf(buffer, "Detected UER type: %s\n", uer_type[chip->uer_detected]);
}

static void vxpocket_proc_done(vxpocket_t *chip)
{
	if (chip->proc_entry) {
		snd_info_unregister(chip->proc_entry);
		chip->proc_entry = NULL;
	}
}

static void vxpocket_proc_init(vxpocket_t *chip)
{
	snd_info_entry_t *entry;

	if ((entry = snd_info_create_card_entry(chip->card, "vxpocket", chip->card->proc_root)) != NULL) {
		entry->content = SNDRV_INFO_CONTENT_TEXT;
		entry->private_data = chip;
		entry->mode = S_IFREG | S_IRUGO | S_IWUSR;
		entry->c.text.read_size = 256;
		entry->c.text.read = vx_proc_read;
		if (snd_info_register(entry) < 0) {
			snd_info_free_entry(entry);
			entry = NULL;
		}
	}
	chip->proc_entry = entry;
}


/*
 * snd_vxpocket_init - initialize VXpocket hardware
 */
static int snd_vxpocket_init(vxpocket_t *chip)
{
	int err;

	if ((err = vx_load_xilinx_binary(chip)) < 0)
		return err;
	if ((err = vx_test_xilinx(chip)) < 0)
		return err;

	vx_reset_board(chip);
	vx_validate_irq(chip, 0);
	if ((err = vx_load_dsp(chip)) < 0)
		return err;
	return 0;
}

/**
 * snd_vxpocket_create_chip - constructor for vxpocket_t
 * @hw: hardware specific record
 *
 * this function allocates the instance and prepare for the hardware
 * initialization, which will be done via snd_vxpocket_assign_resources().
 *
 * return the instance pointer if successful, NULL in error.
 */
vxpocket_t *snd_vxpocket_create_chip(struct snd_vxp_entry *hw)
{
	vxpocket_t *chip;
	int i;

	chip = snd_magic_kcalloc(vxpocket_t, 0, GFP_KERNEL);
	if (! chip) {
		snd_printk(KERN_ERR "vxpocket: no memory\n");
		return NULL;
	}
	spin_lock_init(&chip->lock);
	spin_lock_init(&chip->irq_lock);
	chip->irq = -1;
	chip->hw = hw;
	tasklet_init(&chip->tq, vx_interrupt, (unsigned long)chip);

	/* find an empty slot from the card list */
	for (i = 0; i < SNDRV_CARDS; i++) {
		if (! hw->card_list[i])
			break;
	}
	if (i >= SNDRV_CARDS) {
		snd_printk(KERN_ERR "vxpocket: too many cards found\n");
		snd_magic_kfree(chip);
		return NULL;
	}
	if (! hw->enable_table[i])
		return NULL; /* disabled explicitly */

	/* ok, create a card instance */
	chip->index = i;
	chip->card = snd_card_new(hw->index_table[i], hw->id_table[i], THIS_MODULE, 0);
	if (chip->card == NULL) {
		snd_printk(KERN_ERR "vxpocket: cannot create a card instance\n");
		snd_magic_kfree(chip);
		return NULL;
	}
	chip->card->private_data = chip;
	strcpy(chip->card->driver, hw->name);
	hw->card_list[i] = chip;

	return chip;
}

/**
 * snd_vxpocket_free_chip - destructor for vxpocket_t
 */
void snd_vxpocket_free_chip(vxpocket_t *chip)
{
	snd_assert(chip, return);
	chip->initialized = 0;
	vxpocket_proc_done(chip);
	chip->hw->card_list[chip->index] = NULL;
	chip->card = NULL;
}

/**
 * snd_vxpocket_assign_resources - initialize the hardware and card instance.
 * @port: i/o port for the card
 * @irq: irq number for the card
 *
 * this function assigns the specified port and irq, boot the card,
 * create pcm and control instances, and initialize the rest hardware.
 *
 * returns 0 if successful, or a negative error code.
 */
int snd_vxpocket_assign_resources(vxpocket_t *chip, int port, int irq)
{
	int err;
	snd_card_t *card = chip->card;

	snd_printd(KERN_DEBUG "vxpocket assign resources: port = 0x%x, irq = %d\n", port, irq);
	chip->port = port;

	sprintf(card->shortname, "Digigram %s", chip->card->driver);
	sprintf(card->longname, "%s at 0x%x, irq %i",
		card->shortname, port, irq);

	if ((err = snd_vxpocket_init(chip)) < 0)
		return err;

	chip->irq = irq;

	vx_test_and_ack(chip);
	vx_validate_irq(chip, 1);

	//vx_call_cmd_version(chip);

	if ((err = snd_vxpocket_pcm_new(chip)) < 0)
		return err;

	vx_reset_audio_levels(chip);
	if ((err = snd_vxpocket_mixer_new(chip)) < 0)
		return err;

	vxpocket_proc_init(chip);
	chip->initialized = 1;

	if ((err = snd_card_register(chip->card)) < 0)
		return err;

	return 0;
}

EXPORT_SYMBOL(snd_vxpocket_create_chip);
EXPORT_SYMBOL(snd_vxpocket_free_chip);
EXPORT_SYMBOL(snd_vxpocket_assign_resources);
EXPORT_SYMBOL(snd_vx_irq_handler);
