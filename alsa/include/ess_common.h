#ifndef __ESSCOMMON_H
#define __ESSCOMMON_H

/*
 *  Copyright (c) by Jaromir Koutek <miri@punknet.cz>,
 *                   Jaroslav Kysela <perex@suse.cz>
 *  Definitions for ESS Solo-1
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

#define ESSIO_REG_AUDIO2DMAADDR 0
#define ESSIO_REG_AUDIO2DMACOUNT 4
#define ESSIO_REG_AUDIO2MODE 6
#define ESSIO_REG_IRQCONTROL 7

#define ESSDM_REG_DMABASE 0
#define ESSDM_REG_DMACOUNT 4
#define ESSDM_REG_DMACOMMAND 8
#define ESSDM_REG_DMASTATUS 8
#define ESSDM_REG_DMAMODE 0x0b
#define ESSDM_REG_DMACLEAR 0x0d
#define ESSDM_REG_DMAMASK 0x0f

#define ESSSB_REG_MIXERADDR 4
#define ESSSB_REG_MIXERDATA 5

#define ESSSB_IREG_AUDIO1 0x14
#define ESSSB_IREG_MICMIX 0x1a
#define ESSSB_IREG_RECSRC 0x1c
#define ESSSB_IREG_MASTER 0x32
#define ESSSB_IREG_FM 0x36
#define ESSSB_IREG_AUXACD 0x38
#define ESSSB_IREG_AUXB 0x3a
#define ESSSB_IREG_PCSPEAKER 0x3c
#define ESSSB_IREG_LINE 0x3e
#define ESSSB_IREG_SPATCONTROL 0x50
#define ESSSB_IREG_SPATLEVEL 0x52
#define ESSSB_IREG_MASTER_LEFT 0x60
#define ESSSB_IREG_MASTER_RIGHT 0x62
#define ESSSB_IREG_MICMIXRECORD 0x68
#define ESSSB_IREG_AUDIO2RECORD 0x69
#define ESSSB_IREG_AUXACDRECORD 0x6a
#define ESSSB_IREG_FMRECORD 0x6b
#define ESSSB_IREG_AUXBRECORD 0x6c
#define ESSSB_IREG_MONO 0x6d
#define ESSSB_IREG_LINERECORD 0x6e
#define ESSSB_IREG_MONORECORD 0x6f
#define ESSSB_IREG_AUDIO2SAMPLE 0x70
#define ESSSB_IREG_AUDIO2MODE 0x71
#define ESSSB_IREG_AUDIO2FILTER 0x72
#define ESSSB_IREG_AUDIO2TCOUNTL 0x74
#define ESSSB_IREG_AUDIO2TCOUNTH 0x76
#define ESSSB_IREG_AUDIO2CONTROL1 0x78
#define ESSSB_IREG_AUDIO2CONTROL2 0x7a
#define ESSSB_IREG_AUDIO2 0x7c

#define ESSSB_REG_RESET 6

#define ESSSB_REG_READDATA 0x0a
#define ESSSB_REG_WRITEDATA 0x0c
#define ESSSB_REG_READSTATUS 0x0c

#define ESSSB_REG_STATUS 0x0e

#define ESS_CMD_EXTSAMPLERATE 0xa1
#define ESS_CMD_FILTERDIV 0xa2
#define ESS_CMD_DMACNTRELOADL 0xa4
#define ESS_CMD_DMACNTRELOADH 0xa5
#define ESS_CMD_ANALOGCONTROL 0xa8
#define ESS_CMD_IRQCONTROL 0xb1
#define ESS_CMD_DRQCONTROL 0xb2
#define ESS_CMD_SETFORMAT 0xb6
#define ESS_CMD_SETFORMAT2 0xb7
#define ESS_CMD_DMACONTROL 0xb8
#define ESS_CMD_DMATYPE 0xb9
#define ESS_CMD_READREG 0xc0
#define ESS_CMD_ENABLEEXT 0xc6
#define ESS_CMD_PAUSEDMA 0xd0
#define ESS_CMD_ENABLEAUDIO1 0xd1
#define ESS_CMD_STOPAUDIO1 0xd3
#define ESS_CMD_AUDIO1STATUS 0xd8
#define ESS_CMD_CONTDMA 0xd4
#define ESS_CMD_TESTIRQ 0xf2

#define ESS_RECSRC_MIC 0
#define ESS_RECSRC_AUXACD 2
#define ESS_RECSRC_AUXB 5
#define ESS_RECSRC_LINE 6
#define ESS_RECSRC_NONE 7

#endif				/* __ESSCOMMON_H */
