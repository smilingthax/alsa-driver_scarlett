/*
 * Driver for Digigram VXpocket soundcards
 *
 * Interrupt handler
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
#include <sound/core.h>
#include "vxpocket.h"
#include "vx_cmd.h"


/*
 * vx_test_and_ack - test and acknowledge interrupt
 *
 * called from irq hander, too
 *
 * spinlock held!
 */
int vx_test_and_ack(vxpocket_t *chip)
{
	//unsigned long flags;

	/* not booted yet? */
	if (! chip->xilinx_tested)
		return -ENXIO;

	if (! (vx_inb(chip, DIALOG) & VXP_DLG_MEMIRQ_MASK))
		return -EIO;
	
	//spin_lock_irqsave(&chip->irq_lock, flags);
	/* ok, interrupts generated, now ack it */
	/* set ACQUIT bit up and down */
	vx_outb(chip, DIALOG, chip->regDIALOG | VXP_DLG_ACK_MEMIRQ_MASK);
	/* useless read just to spend some time and maintain
	 * the ACQUIT signal up for a while ( a bus cycle )
	 */
	vx_inb(chip, DIALOG);
	vx_outb(chip, DIALOG, chip->regDIALOG & ~VXP_DLG_ACK_MEMIRQ_MASK);
	//spin_unlock_irqrestore(&chip->irq_lock, flags);
	return 0;
}


/*
 * vx_validate_irq - enable/disable IRQ
 */
void vx_validate_irq(vxpocket_t *chip, int enable)
{
	/* Set the interrupt enable bit to 1 in CDSP register */
	if (enable)
		chip->regCDSP |= VXP_CDSP_VALID_IRQ_MASK;
	else
		chip->regCDSP &= ~VXP_CDSP_VALID_IRQ_MASK;
	vx_outb(chip, CDSP, chip->regCDSP);
}


/*
 * vx_test_irq_src - query the source of interrupts
 *
 * called from irq handler only
 */
static int vx_test_irq_src(vxpocket_t *chip, int *ret)
{
	int err;

	vx_init_rmh(&chip->irq_rmh, CMD_TEST_IT);
	spin_lock(&chip->lock);
	err = vx_send_msg_nolock(chip, &chip->irq_rmh);
	if (err < 0)
		*ret = 0;
	else
		*ret = chip->irq_rmh.Stat[0];
	spin_unlock(&chip->lock);
	return err;
}

/*
 * vx_interrupt - soft irq handler
 */
void vx_interrupt(unsigned long private_data)
{
	vxpocket_t *chip = snd_magic_cast(vxpocket_t, (void*)private_data, return);
	int i, events;
	vx_pipe_t *pipe;
		
	if (chip->is_stale)
		return;

	if (vx_test_irq_src(chip, &events) < 0)
		return;
    
	// printk(KERN_DEBUG "IRQ events = 0x%x\n", events);

	/* We must prevent any application using this DSP
	 * and block any further request until the application
	 * either unregisters or reloads the DSP
	 */
	if (events & FATAL_DSP_ERROR) {
		snd_printk(KERN_ERR "vxpocket: fatal DSP error!!\n");
		return;
	}

	/* The start on time code conditions are filled (ie the time code
	 * received by the board is equal to one of those given to it).
	 */
	if (events & TIME_CODE_EVENT_PENDING)
		;

	/* The frequency has changed on the board (UER mode). */
	if (events & FREQUENCY_CHANGE_EVENT_PENDING)
		vx_change_frequency(chip);

	/* update the pcm pointers as frequently as possible */
	for (i = 0; i < chip->audio_outs; i++) {
		pipe = chip->playback_pipes[i];
		if (pipe && pipe->substream) {
			vx_pcm_playback_update_buffer(chip, pipe->substream, pipe);
			vx_pcm_playback_update(chip, pipe->substream, pipe);
		}
	}
	for (i = 0; i < chip->audio_ins; i++) {
		pipe = chip->capture_pipes[i];
		if (pipe && pipe->substream)
			vx_pcm_capture_update(chip, pipe->substream, pipe);
	}
}


/**
 * snd_vx_irq_handler - interrupt handler
 */
void snd_vx_irq_handler(int irq, void *dev, struct pt_regs *regs)
{
	vxpocket_t *chip = snd_magic_cast(vxpocket_t, dev, return);

	if (! chip->initialized || chip->is_stale)
		return;
	if (! vx_test_and_ack(chip))
		tasklet_hi_schedule(&chip->tq);
}
