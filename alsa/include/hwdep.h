#ifndef __HWDEP_H
#define __HWDEP_H

/*
 *  Hardware dependent layer 
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
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

typedef struct snd_stru_hwdep_ops {
	long long (*lseek) (snd_hwdep_t *hw, struct file * file, long long offset, int orig);
	long (*read) (snd_hwdep_t * hw, char *buf, long count);
	long (*write) (snd_hwdep_t * hw, const char *buf, long count);
	int (*open) (snd_hwdep_t * hw, struct file * file);
	int (*release) (snd_hwdep_t * hw, struct file * file);
	unsigned int (*poll) (snd_hwdep_t * hw, struct file * file, poll_table * wait);
	int (*ioctl) (snd_hwdep_t * hw, struct file * file, unsigned int cmd, unsigned long arg);
	int (*mmap) (snd_hwdep_t * hw, struct inode * inode, struct file * file, struct vm_area_struct * vma);
} snd_hwdep_ops_t;

struct snd_stru_hwdep {
	snd_card_t *card;
	int device;
	char id[32];
	char name[80];
	int type;
	snd_hwdep_ops_t ops;
	void *private_data;
	void (*private_free) (void *private_data);
};

extern int snd_hwdep_new(snd_card_t * card, char *id, int device, snd_hwdep_t ** rhwdep);

#endif				/* __HWDEP_H */
