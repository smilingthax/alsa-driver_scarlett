/*
 *  Copyright (c) 2004 James Courtier-Dutton <James@superbug.demon.co.uk>
 *  Driver CA0106 chips. e.g. Sound Blaster Audigy LS and Live 24bit
 *  Version: 0.0.16
 *
 *  FEATURES currently supported:
 *    See ca0106_main.c for features.
 * 
 *  Changelog:
 *    Support interrupts per period.
 *    Removed noise from Center/LFE channel when in Analog mode.
 *    Rename and remove mixer controls.
 *  0.0.6
 *    Use separate card based DMA buffer for periods table list.
 *  0.0.7
 *    Change remove and rename ctrls into lists.
 *  0.0.8
 *    Try to fix capture sources.
 *  0.0.9
 *    Fix AC3 output.
 *    Enable S32_LE format support.
 *  0.0.10
 *    Enable playback 48000 and 96000 rates. (Rates other that these do not work, even with "plug:front".)
 *  0.0.11
 *    Add Model name recognition.
 *  0.0.12
 *    Correct interrupt timing. interrupt at end of period, instead of in the middle of a playback period.
 *    Remove redundent "voice" handling.
 *  0.0.13
 *    Single trigger call for multi channels.
 *  0.0.14
 *    Set limits based on what the sound card hardware can do.
 *    playback periods_min=2, periods_max=8
 *    capture hw constraints require period_size = n * 64 bytes.
 *    playback hw constraints require period_size = n * 64 bytes.
 *  0.0.15
 *    Separate ca0106.c into separate functional .c files.
 *  0.0.16
 *    Modified Copyright message.
 *
 *  This code was initally based on code from ALSA's emu10k1x.c which is:
 *  Copyright (c) by Francisco Moraes <fmoraes@nc.rr.com>
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
 *
 */
#include <sound/driver.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/moduleparam.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/pcm.h>
#include <sound/ac97_codec.h>
#include <sound/info.h>

#include "ca0106.h"

static void snd_ca0106_proc_reg_write32(snd_info_entry_t *entry, 
				       snd_info_buffer_t * buffer)
{
	ca0106_t *emu = entry->private_data;
	unsigned long flags;
        char line[64];
        u32 reg, val;
        while (!snd_info_get_line(buffer, line, sizeof(line))) {
                if (sscanf(line, "%x %x", &reg, &val) != 2)
                        continue;
                if ((reg < 0x40) && (reg >=0) && (val <= 0xffffffff) ) {
			spin_lock_irqsave(&emu->emu_lock, flags);
			outl(val, emu->port + (reg & 0xfffffffc));
			spin_unlock_irqrestore(&emu->emu_lock, flags);
		}
        }
}

static void snd_ca0106_proc_reg_read32(snd_info_entry_t *entry, 
				       snd_info_buffer_t * buffer)
{
	ca0106_t *emu = entry->private_data;
	unsigned long value;
	unsigned long flags;
	int i;
	snd_iprintf(buffer, "Registers:\n\n");
	for(i = 0; i < 0x20; i+=4) {
		spin_lock_irqsave(&emu->emu_lock, flags);
		value = inl(emu->port + i);
		spin_unlock_irqrestore(&emu->emu_lock, flags);
		snd_iprintf(buffer, "Register %02X: %08lX\n", i, value);
	}
}

static void snd_ca0106_proc_reg_read16(snd_info_entry_t *entry, 
				       snd_info_buffer_t * buffer)
{
	ca0106_t *emu = entry->private_data;
        unsigned int value;
	unsigned long flags;
	int i;
	snd_iprintf(buffer, "Registers:\n\n");
	for(i = 0; i < 0x20; i+=2) {
		spin_lock_irqsave(&emu->emu_lock, flags);
		value = inw(emu->port + i);
		spin_unlock_irqrestore(&emu->emu_lock, flags);
		snd_iprintf(buffer, "Register %02X: %04X\n", i, value);
	}
}

static void snd_ca0106_proc_reg_read8(snd_info_entry_t *entry, 
				       snd_info_buffer_t * buffer)
{
	ca0106_t *emu = entry->private_data;
	unsigned int value;
	unsigned long flags;
	int i;
	snd_iprintf(buffer, "Registers:\n\n");
	for(i = 0; i < 0x20; i+=1) {
		spin_lock_irqsave(&emu->emu_lock, flags);
		value = inb(emu->port + i);
		spin_unlock_irqrestore(&emu->emu_lock, flags);
		snd_iprintf(buffer, "Register %02X: %02X\n", i, value);
	}
}

static void snd_ca0106_proc_reg_read1(snd_info_entry_t *entry, 
				       snd_info_buffer_t * buffer)
{
	ca0106_t *emu = entry->private_data;
	unsigned long value;
	int i,j;

	snd_iprintf(buffer, "Registers\n");
	for(i = 0; i < 0x40; i++) {
		snd_iprintf(buffer, "%02X: ",i);
		for (j = 0; j < 4; j++) {
                  value = snd_ca0106_ptr_read(emu, i, j);
		  snd_iprintf(buffer, "%08lX ", value);
                }
	        snd_iprintf(buffer, "\n");
	}
}

static void snd_ca0106_proc_reg_read2(snd_info_entry_t *entry, 
				       snd_info_buffer_t * buffer)
{
	ca0106_t *emu = entry->private_data;
	unsigned long value;
	int i,j;

	snd_iprintf(buffer, "Registers\n");
	for(i = 0x40; i < 0x80; i++) {
		snd_iprintf(buffer, "%02X: ",i);
		for (j = 0; j < 4; j++) {
                  value = snd_ca0106_ptr_read(emu, i, j);
		  snd_iprintf(buffer, "%08lX ", value);
                }
	        snd_iprintf(buffer, "\n");
	}
}

static void snd_ca0106_proc_reg_write(snd_info_entry_t *entry, 
				       snd_info_buffer_t * buffer)
{
	ca0106_t *emu = entry->private_data;
        char line[64];
        unsigned int reg, channel_id , val;
        while (!snd_info_get_line(buffer, line, sizeof(line))) {
                if (sscanf(line, "%x %x %x", &reg, &channel_id, &val) != 3)
                        continue;
                if ((reg < 0x80) && (reg >=0) && (val <= 0xffffffff) && (channel_id >=0) && (channel_id <= 3) )
                        snd_ca0106_ptr_write(emu, reg, channel_id, val);
        }
}


int __devinit snd_ca0106_proc_init(ca0106_t * emu)
{
	snd_info_entry_t *entry;
	
	if(! snd_card_proc_new(emu->card, "ca0106_reg32", &entry)) {
		snd_info_set_text_ops(entry, emu, 1024, snd_ca0106_proc_reg_read32);
		entry->c.text.write_size = 64;
		entry->c.text.write = snd_ca0106_proc_reg_write32;
	}
	if(! snd_card_proc_new(emu->card, "ca0106_reg16", &entry))
		snd_info_set_text_ops(entry, emu, 1024, snd_ca0106_proc_reg_read16);
	if(! snd_card_proc_new(emu->card, "ca0106_reg8", &entry))
		snd_info_set_text_ops(entry, emu, 1024, snd_ca0106_proc_reg_read8);
	if(! snd_card_proc_new(emu->card, "ca0106_regs1", &entry)) {
		snd_info_set_text_ops(entry, emu, 1024, snd_ca0106_proc_reg_read1);
		entry->c.text.write_size = 64;
		entry->c.text.write = snd_ca0106_proc_reg_write;
//		entry->private_data = emu;
	}
	if(! snd_card_proc_new(emu->card, "ca0106_regs2", &entry)) 
		snd_info_set_text_ops(entry, emu, 1024, snd_ca0106_proc_reg_read2);
	return 0;
}

