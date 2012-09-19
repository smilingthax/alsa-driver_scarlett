/*
 * Driver for Digigram VX soundcards
 *
 * Copyright (c) 2002-2003 by Takashi Iwai <tiwai@suse.de>
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

#ifndef __SOUND_VX_LOAD_H
#define __SOUND_VX_LOAD_H

struct snd_vx_version {
	int type;		/* VX_TYPE_XXX */
	unsigned int status;	/* VX_STAT_XXX bits */
	char name[64];		/* card name */
	unsigned int version;	/* driver version */
	/* hardware specs */
	unsigned int num_codecs;
	unsigned int num_ins;
	unsigned int num_outs;
};

/* hardware type */
enum {
	/* VX222 PCI */
	VX_TYPE_BOARD,		/* old VX222 PCI */
	VX_TYPE_V2,		/* VX222 V2 PCI */
	VX_TYPE_MIC,		/* VX222 Mic PCI */
	/* VX-pocket */
	VX_TYPE_VXPOCKET,	/* VXpocket V2 */
	VX_TYPE_VXP440		/* VXpocket 440 */
};

enum {
	VX_STAT_XILINX_LOADED	= (1 << 0),	/* xilinx image is loaded */
	VX_STAT_XILINX_TESTED	= (1 << 1),	/* xilinx is booted */
	VX_STAT_DSP_LOADED	= (1 << 2),	/* dsp is loaded */
	VX_STAT_DEVICE_INIT	= (1 << 3),	/* devices are registered */
	VX_STAT_CHIP_INIT	= (1 << 4),	/* all operational */
	VX_STAT_IN_SUSPEND	= (1 << 10),	/* in suspend phase */
	VX_STAT_RESUMING	= (1 << 11),	/* in resume phase */
	VX_STAT_IS_STALE	= (1 << 15)	/* device is stale */
};

struct snd_vx_image {
	unsigned char name[64];		/* ID (e.g. file name) */
	unsigned char *image;		/* binary image */
	unsigned int length;		/* size of image in bytes */
};

struct snd_vx_loader {
	struct snd_vx_image boot;	/* boot image (if necessary) */
	struct snd_vx_image binary;	/* xilinx or DSP image */
};


enum {
	SND_VX_HWDEP_IOCTL_VERSION	= _IOR('V', 0x00, struct snd_vx_version),
	SND_VX_HWDEP_IOCTL_LOAD_XILINX	= _IOW('V', 0x01, struct snd_vx_loader),
	SND_VX_HWDEP_IOCTL_LOAD_DSP	= _IOW('V', 0x02, struct snd_vx_loader),
	SND_VX_HWDEP_IOCTL_INIT_DEVICE	= _IO('V', 0x03),
	SND_VX_HWDEP_IOCTL_RESUME	= _IO('V', 0x04),
};


#endif /* __SOUND_VX_LOAD_H */
