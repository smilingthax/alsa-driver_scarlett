/*
 *   ALSA driver for VIA VT82xx (South Bridge)
 *
 *   VT82C686A/B/C, VT8233A/C, VT8235
 *
 *	Copyright (c) 2000 Jaroslav Kysela <perex@suse.cz>
 *	                   Tjeerd.Mulder <Tjeerd.Mulder@fujitsu-siemens.com>
 *                    2002 Takashi Iwai <tiwai@suse.de>
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

/*
 * Changes:
 *
 * Dec. 19, 2002	Takashi Iwai <tiwai@suse.de>
 *	- use the DSX channels for the first pcm playback.
 *	  (on VIA8233, 8233C and 8235 only)
 *	  this will allow you play simultaneously up to 4 streams.
 *	  multi-channel playback is assigned to the second device
 *	  on these chips.
 *	- support the secondary capture (on VIA8233/C,8235)
 *	- SPDIF support
 *	  the DSX3 channel can be used for SPDIF output.
 *	  on VIA8233A, this channel is assigned to the second pcm
 *	  playback.
 *	  the card config of alsa-lib will assign the correct
 *	  device for applications.
 *	- clean up the code, separate low-level initialization
 *	  routines for each chipset.
 */

#include <sound/driver.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_sgbuf.h>
#include <sound/pcm_params.h>
#include <sound/info.h>
#include <sound/ac97_codec.h>
#include <sound/mpu401.h>
#define SNDRV_GET_ID
#include <sound/initval.h>

#if 0
#define POINTER_DEBUG
#endif

MODULE_AUTHOR("Jaroslav Kysela <perex@suse.cz>");
MODULE_DESCRIPTION("VIA VT82xx audio");
MODULE_LICENSE("GPL");
MODULE_CLASSES("{sound}");
MODULE_DEVICES("{{VIA,VT82C686A/B/C,pci},{VIA,VT8233A/C,8235}}");

static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;	/* Index 0-MAX */
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;	/* ID for this card */
static int enable[SNDRV_CARDS] = SNDRV_DEFAULT_ENABLE_PNP;	/* Enable this card */
static long mpu_port[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = -1};
static int ac97_clock[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = 48000};

MODULE_PARM(index, "1-" __MODULE_STRING(SNDRV_CARDS) "i");
MODULE_PARM_DESC(index, "Index value for VIA 82xx bridge.");
MODULE_PARM_SYNTAX(index, SNDRV_INDEX_DESC);
MODULE_PARM(id, "1-" __MODULE_STRING(SNDRV_CARDS) "s");
MODULE_PARM_DESC(id, "ID string for VIA 82xx bridge.");
MODULE_PARM_SYNTAX(id, SNDRV_ID_DESC);
MODULE_PARM(enable, "1-" __MODULE_STRING(SNDRV_CARDS) "i");
MODULE_PARM_DESC(enable, "Enable audio part of VIA 82xx bridge.");
MODULE_PARM_SYNTAX(enable, SNDRV_ENABLE_DESC);
MODULE_PARM(mpu_port, "1-" __MODULE_STRING(SNDRV_CARDS) "l");
MODULE_PARM_DESC(mpu_port, "MPU-401 port.");
MODULE_PARM_SYNTAX(mpu_port, SNDRV_PORT_DESC);
MODULE_PARM(ac97_clock, "1-" __MODULE_STRING(SNDRV_CARDS) "l");
MODULE_PARM_DESC(ac97_clock, "AC'97 codec clock (default 48000Hz).");
MODULE_PARM_SYNTAX(ac97_clock, SNDRV_ENABLED ",default:48000");


/* pci ids */
#ifndef PCI_DEVICE_ID_VIA_82C686_5
#define PCI_DEVICE_ID_VIA_82C686_5	0x3058
#endif
#ifndef PCI_DEVICE_ID_VIA_8233_5
#define PCI_DEVICE_ID_VIA_8233_5	0x3059
#endif

/* revision numbers for via8233 */
#define VIA_REV_PRE_8233	0x10	/* not in market */
#define VIA_REV_8233C		0x20	/* 2 rec, 4 pb, 1 multi-pb */
#define VIA_REV_8233		0x30	/* 2 rec, 4 pb, 1 multi-pb, spdif */
#define VIA_REV_8233A		0x40	/* 1 rec, 1 multi-pb, spdf */
#define VIA_REV_8235		0x50	/* 2 rec, 4 pb, 1 multi-pb, spdif */

/*
 *  Direct registers
 */

#define VIAREG(via, x) ((via)->port + VIA_REG_##x)

/* common offsets */
#define VIA_REG_OFFSET_STATUS		0x00	/* byte - channel status */
#define   VIA_REG_STAT_ACTIVE		0x80	/* RO */
#define   VIA_REG_STAT_PAUSED		0x40	/* RO */
#define   VIA_REG_STAT_TRIGGER_QUEUED	0x08	/* RO */
#define   VIA_REG_STAT_STOPPED		0x04	/* RWC */
#define   VIA_REG_STAT_EOL		0x02	/* RWC */
#define   VIA_REG_STAT_FLAG		0x01	/* RWC */
#define VIA_REG_OFFSET_CONTROL		0x01	/* byte - channel control */
#define   VIA_REG_CTRL_START		0x80	/* WO */
#define   VIA_REG_CTRL_TERMINATE	0x40	/* WO */
#define   VIA_REG_CTRL_AUTOSTART	0x20
#define   VIA_REG_CTRL_PAUSE		0x08	/* RW */
#define   VIA_REG_CTRL_INT_STOP		0x04		
#define   VIA_REG_CTRL_INT_EOL		0x02
#define   VIA_REG_CTRL_INT_FLAG		0x01
#define   VIA_REG_CTRL_RESET		0x01	/* RW - probably reset? undocumented */
#define   VIA_REG_CTRL_INT (VIA_REG_CTRL_INT_FLAG | VIA_REG_CTRL_INT_EOL | VIA_REG_CTRL_AUTOSTART)
#define VIA_REG_OFFSET_TYPE		0x02	/* byte - channel type (686 only) */
#define   VIA_REG_TYPE_AUTOSTART	0x80	/* RW - autostart at EOL */
#define   VIA_REG_TYPE_16BIT		0x20	/* RW */
#define   VIA_REG_TYPE_STEREO		0x10	/* RW */
#define   VIA_REG_TYPE_INT_LLINE	0x00
#define   VIA_REG_TYPE_INT_LSAMPLE	0x04
#define   VIA_REG_TYPE_INT_LESSONE	0x08
#define   VIA_REG_TYPE_INT_MASK		0x0c
#define   VIA_REG_TYPE_INT_EOL		0x02
#define   VIA_REG_TYPE_INT_FLAG		0x01
#define VIA_REG_OFFSET_TABLE_PTR	0x04	/* dword - channel table pointer */
#define VIA_REG_OFFSET_CURR_PTR		0x04	/* dword - channel current pointer */
#define VIA_REG_OFFSET_STOP_IDX		0x08	/* dword - stop index, channel type, sample rate */
#define   VIA8233_REG_TYPE_16BIT	0x00200000	/* RW */
#define   VIA8233_REG_TYPE_STEREO	0x00100000	/* RW */
#define VIA_REG_OFFSET_CURR_COUNT	0x0c	/* dword - channel current count (24 bit) */
#define VIA_REG_OFFSET_CURR_INDEX	0x0f	/* byte - channel current index (for via8233 only) */

#define DEFINE_VIA_REGSET(name,val) \
enum {\
	VIA_REG_##name##_STATUS		= (val),\
	VIA_REG_##name##_CONTROL	= (val) + 0x01,\
	VIA_REG_##name##_TYPE		= (val) + 0x02,\
	VIA_REG_##name##_TABLE_PTR	= (val) + 0x04,\
	VIA_REG_##name##_CURR_PTR	= (val) + 0x04,\
	VIA_REG_##name##_STOP_IDX	= (val) + 0x08,\
	VIA_REG_##name##_CURR_COUNT	= (val) + 0x0c,\
}

/* playback block */
DEFINE_VIA_REGSET(PLAYBACK, 0x00);
DEFINE_VIA_REGSET(CAPTURE, 0x10);
DEFINE_VIA_REGSET(FM, 0x20);

/* AC'97 */
#define VIA_REG_AC97			0x80	/* dword */
#define   VIA_REG_AC97_CODEC_ID_MASK	(3<<30)
#define   VIA_REG_AC97_CODEC_ID_SHIFT	30
#define   VIA_REG_AC97_CODEC_ID_PRIMARY	0x00
#define   VIA_REG_AC97_CODEC_ID_SECONDARY 0x01
#define   VIA_REG_AC97_SECONDARY_VALID	(1<<27)
#define   VIA_REG_AC97_PRIMARY_VALID	(1<<25)
#define   VIA_REG_AC97_BUSY		(1<<24)
#define   VIA_REG_AC97_READ		(1<<23)
#define   VIA_REG_AC97_CMD_SHIFT	16
#define   VIA_REG_AC97_CMD_MASK		0x7e
#define   VIA_REG_AC97_DATA_SHIFT	0
#define   VIA_REG_AC97_DATA_MASK	0xffff
#define VIA_REG_SGD_SHADOW		0x84	/* dword */

/* multi-channel and capture registers for via8233 */
DEFINE_VIA_REGSET(MULTPLAY, 0x40);
DEFINE_VIA_REGSET(CAPTURE_8233, 0x60);

/* via8233-specific registers */
#define VIA_REG_PLAYBACK_VOLUME_L	0x02	/* byte */
#define VIA_REG_PLAYBACK_VOLUME_R	0x03	/* byte */
#define VIA_REG_MULTPLAY_FORMAT		0x42	/* byte - format and channels */
#define   VIA_REG_MULTPLAY_FMT_8BIT	0x00
#define   VIA_REG_MULTPLAY_FMT_16BIT	0x80
#define   VIA_REG_MULTPLAY_FMT_CH_MASK	0x70	/* # channels << 4 (valid = 1,2,4,6) */
#define VIA_REG_CAPTURE_FIFO		0x62	/* byte - bit 6 = fifo  enable */
#define   VIA_REG_CAPTURE_FIFO_ENABLE	0x40
#define VIA_REG_CAPTURE_CHANNEL		0x63	/* byte - input select */
#define   VIA_REG_CAPTURE_CHANNEL_MIC	0x4
#define   VIA_REG_CAPTURE_CHANNEL_LINE	0
#define   VIA_REG_CAPTURE_SELECT_CODEC	0x03	/* recording source codec (0 = primary) */

#define VIA_TBL_BIT_FLAG	0x40000000
#define VIA_TBL_BIT_EOL		0x80000000

/*
 */

typedef struct _snd_via82xx via82xx_t;
typedef struct via_dev viadev_t;
#define chip_t via82xx_t

/*
 * pcm stream
 */

struct snd_via_sg_table {
	unsigned int offset;
	unsigned int size;
} ;

#define VIA_TABLE_SIZE	255

struct via_dev {
	unsigned int reg_offset;
	int direction;	/* playback = 0, capture = 1 */
        snd_pcm_substream_t *substream;
	int running;
	unsigned int tbl_entries; /* # descriptors */
	u32 *table; /* physical address + flag */
	dma_addr_t table_addr;
	struct snd_via_sg_table *idx_table;
	/* for recovery from the unexpected pointer */
	unsigned int lastpos;
	unsigned int bufsize;
	unsigned int bufsize2;
};


/*
 * allocate and initialize the descriptor buffers
 * periods = number of periods
 * fragsize = period size in bytes
 */
static int build_via_table(viadev_t *dev, snd_pcm_substream_t *substream,
			   struct pci_dev *pci,
			   unsigned int periods, unsigned int fragsize)
{
	unsigned int i, idx, ofs, rest;
	struct snd_sg_buf *sgbuf = snd_pcm_substream_sgbuf(substream);

	if (! dev->table) {
		/* the start of each lists must be aligned to 8 bytes,
		 * but the kernel pages are much bigger, so we don't care
		 */
		dev->table = (u32*)snd_malloc_pci_pages(pci, PAGE_ALIGN(VIA_TABLE_SIZE * 2 * 8), &dev->table_addr);
		if (! dev->table)
			return -ENOMEM;
	}
	if (! dev->idx_table) {
		dev->idx_table = kmalloc(sizeof(*dev->idx_table) * VIA_TABLE_SIZE, GFP_KERNEL);
		if (! dev->idx_table)
			return -ENOMEM;
	}

	/* fill the entries */
	idx = 0;
	ofs = 0;
	for (i = 0; i < periods; i++) {
		rest = fragsize;
		/* fill descriptors for a period.
		 * a period can be split to several descriptors if it's
		 * over page boundary.
		 */
		do {
			unsigned int r;
			unsigned int flag;

			if (idx >= VIA_TABLE_SIZE) {
				snd_printk(KERN_ERR "via82xx: too much table size!\n");
				return -EINVAL;
			}
			dev->table[idx << 1] = cpu_to_le32((u32)snd_pcm_sgbuf_get_addr(sgbuf, ofs));
			r = PAGE_SIZE - (ofs % PAGE_SIZE);
			if (rest < r)
				r = rest;
			rest -= r;
			if (! rest) {
				if (i == periods - 1)
					flag = VIA_TBL_BIT_EOL; /* buffer boundary */
				else
					flag = VIA_TBL_BIT_FLAG; /* period boundary */
			} else
				flag = 0; /* period continues to the next */
			// printk("via: tbl %d: at %d  size %d (rest %d)\n", idx, ofs, r, rest);
			dev->table[(idx<<1) + 1] = cpu_to_le32(r | flag);
			dev->idx_table[idx].offset = ofs;
			dev->idx_table[idx].size = r;
			ofs += r;
			idx++;
		} while (rest > 0);
	}
	dev->tbl_entries = idx;
	dev->bufsize = periods * fragsize;
	dev->bufsize2 = dev->bufsize / 2;
	return 0;
}


static void clean_via_table(viadev_t *dev, snd_pcm_substream_t *substream,
			    struct pci_dev *pci)
{
	if (dev->table) {
		snd_free_pci_pages(pci, PAGE_ALIGN(VIA_TABLE_SIZE * 2 * 8), dev->table, dev->table_addr);
		dev->table = NULL;
	}
	if (dev->idx_table) {
		kfree(dev->idx_table);
		dev->idx_table = NULL;
	}
}


/*
 */

enum { TYPE_VIA686 = 1, TYPE_VIA8233 };

#define VIA_MAX_DEVS	7	/* 4 playback, 1 multi, 2 capture */

struct via_rate_lock {
	spinlock_t lock;
	int rate;
	int used;
};

struct _snd_via82xx {
	int irq;

	unsigned long port;
	struct resource *res_port;
	int chip_type;
	unsigned char revision;

	unsigned char old_legacy;
	unsigned char old_legacy_cfg;

	struct pci_dev *pci;
	snd_card_t *card;

	int num_devs;
	int playback_devno, multi_devno, capture_devno;
	viadev_t devs[VIA_MAX_DEVS];
	struct via_rate_lock rates[2]; /* playback and capture */

	snd_rawmidi_t *rmidi;

	ac97_t *ac97;
	unsigned int ac97_clock;
	unsigned int ac97_secondary;	/* secondary AC'97 codec is present */

	spinlock_t reg_lock;
	spinlock_t ac97_lock;
	snd_info_entry_t *proc_entry;
};

static struct pci_device_id snd_via82xx_ids[] __devinitdata = {
	{ 0x1106, 0x3058, PCI_ANY_ID, PCI_ANY_ID, 0, 0, TYPE_VIA686, },	/* 686A */
	{ 0x1106, 0x3059, PCI_ANY_ID, PCI_ANY_ID, 0, 0, TYPE_VIA8233, },	/* VT8233 */
	{ 0, }
};

MODULE_DEVICE_TABLE(pci, snd_via82xx_ids);

/*
 *  Basic I/O
 */

static inline unsigned int snd_via82xx_codec_xread(via82xx_t *chip)
{
	return inl(VIAREG(chip, AC97));
}
 
static inline void snd_via82xx_codec_xwrite(via82xx_t *chip, unsigned int val)
{
	outl(val, VIAREG(chip, AC97));
}
 
static int snd_via82xx_codec_ready(via82xx_t *chip, int secondary)
{
	unsigned int timeout = 1000;	/* 1ms */
	unsigned int val;
	
	while (timeout-- > 0) {
		udelay(1);
		if (!((val = snd_via82xx_codec_xread(chip)) & VIA_REG_AC97_BUSY))
			return val & 0xffff;
	}
	snd_printk(KERN_ERR "codec_ready: codec %i is not ready [0x%x]\n", secondary, snd_via82xx_codec_xread(chip));
	return -EIO;
}
 
static int snd_via82xx_codec_valid(via82xx_t *chip, int secondary)
{
	unsigned int timeout = 1000;	/* 1ms */
	unsigned int val;
	unsigned int stat = !secondary ? VIA_REG_AC97_PRIMARY_VALID :
					 VIA_REG_AC97_SECONDARY_VALID;
	
	while (timeout-- > 0) {
		udelay(1);
		if ((val = snd_via82xx_codec_xread(chip)) & stat)
			return val & 0xffff;
	}
	snd_printk(KERN_ERR "codec_valid: codec %i is not valid [0x%x]\n", secondary, snd_via82xx_codec_xread(chip));
	return -EIO;
}
 
static void snd_via82xx_codec_wait(ac97_t *ac97)
{
	via82xx_t *chip = snd_magic_cast(via82xx_t, ac97->private_data, return);
	int err;
	err = snd_via82xx_codec_ready(chip, ac97->num);
	/* here we need to wait fairly for long time.. */
	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_timeout(HZ/2);
}

static void snd_via82xx_codec_write(ac97_t *ac97,
				    unsigned short reg,
				    unsigned short val)
{
	via82xx_t *chip = snd_magic_cast(via82xx_t, ac97->private_data, return);
	unsigned int xval;
	
	xval = !ac97->num ? VIA_REG_AC97_CODEC_ID_PRIMARY : VIA_REG_AC97_CODEC_ID_SECONDARY;
	xval <<= VIA_REG_AC97_CODEC_ID_SHIFT;
	xval |= reg << VIA_REG_AC97_CMD_SHIFT;
	xval |= val << VIA_REG_AC97_DATA_SHIFT;
	spin_lock(&chip->ac97_lock);
	snd_via82xx_codec_xwrite(chip, xval);
	snd_via82xx_codec_ready(chip, ac97->num);
	spin_unlock(&chip->ac97_lock);
}

static unsigned short snd_via82xx_codec_read(ac97_t *ac97, unsigned short reg)
{
	via82xx_t *chip = snd_magic_cast(via82xx_t, ac97->private_data, return ~0);
	unsigned int xval, val = 0xffff;
	int again = 0;

	xval = ac97->num << VIA_REG_AC97_CODEC_ID_SHIFT;
	xval |= ac97->num ? VIA_REG_AC97_SECONDARY_VALID : VIA_REG_AC97_PRIMARY_VALID;
	xval |= VIA_REG_AC97_READ;
	xval |= (reg & 0x7f) << VIA_REG_AC97_CMD_SHIFT;
	spin_lock(&chip->ac97_lock);
      	while (1) {
      		if (again++ > 3) {
		        spin_unlock(&chip->ac97_lock);
		      	return 0xffff;
		}
		snd_via82xx_codec_xwrite(chip, xval);
		if (snd_via82xx_codec_ready(chip, ac97->num) < 0)
			continue;
		if (snd_via82xx_codec_valid(chip, ac97->num) >= 0) {
			udelay(25);
			val = snd_via82xx_codec_xread(chip);
			break;
		}
	}
	spin_unlock(&chip->ac97_lock);
	return val & 0xffff;
}

static void snd_via82xx_channel_reset(via82xx_t *chip, viadev_t *viadev)
{
	unsigned long port = chip->port + viadev->reg_offset;

	outb(VIA_REG_CTRL_PAUSE | VIA_REG_CTRL_TERMINATE | VIA_REG_CTRL_RESET, port + VIA_REG_OFFSET_CONTROL);
	udelay(50);
	/* disable interrupts */
	outb(0x00, port + VIA_REG_OFFSET_CONTROL);
	/* clear interrupts */
	outb(0x03, port + VIA_REG_OFFSET_STATUS);
	outb(0x00, port + VIA_REG_OFFSET_TYPE); /* for via686 */
	outl(0, port + VIA_REG_OFFSET_CURR_PTR);
	viadev->lastpos = 0;
}


/*
 *  Interrupt handler
 */

static void snd_via82xx_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	via82xx_t *chip = snd_magic_cast(via82xx_t, dev_id, return);
	unsigned int status;
	int i;

	spin_lock(&chip->reg_lock);
	if (chip->chip_type == TYPE_VIA686) {
		/* check mpu401 interrupt */
		status = inl(VIAREG(chip, SGD_SHADOW));
		if ((status & 0x00000077) == 0) {
			spin_unlock(&chip->reg_lock);
			if (chip->rmidi != NULL)
				snd_mpu401_uart_interrupt(irq, chip->rmidi->private_data, regs);
			return;
		}
	}
	/* check status for each stream */
	for (i = 0; i < chip->num_devs; i++) {
		viadev_t *viadev = &chip->devs[i];
		if (inb(chip->port + viadev->reg_offset) & (VIA_REG_STAT_EOL|VIA_REG_STAT_FLAG)) {
			outb(VIA_REG_STAT_FLAG | VIA_REG_STAT_EOL, VIAREG(chip, OFFSET_STATUS) + viadev->reg_offset);
			if (viadev->substream && viadev->running) {
				spin_unlock(&chip->reg_lock);
				snd_pcm_period_elapsed(viadev->substream);
				spin_lock(&chip->reg_lock);
			}
		}
	}
	spin_unlock(&chip->reg_lock);
}

/*
 *  PCM callbacks
 */

/*
 * trigger callback
 */
static int snd_via82xx_pcm_trigger(snd_pcm_substream_t * substream, int cmd)
{
	via82xx_t *chip = snd_pcm_substream_chip(substream);
	viadev_t *viadev = (viadev_t *)substream->runtime->private_data;
	unsigned char val;
	unsigned long port = chip->port + viadev->reg_offset;

	if (chip->chip_type == TYPE_VIA8233)
		val = VIA_REG_CTRL_INT;
	else
		val = 0;
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		val |= VIA_REG_CTRL_START;
		viadev->running = 1;
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		val = VIA_REG_CTRL_TERMINATE;
		viadev->running = 0;
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		val |= VIA_REG_CTRL_PAUSE;
		viadev->running = 0;
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		viadev->running = 1;
		break;
	default:
		return -EINVAL;
	}
	outb(val, port + VIA_REG_OFFSET_CONTROL);
	if (cmd == SNDRV_PCM_TRIGGER_STOP)
		snd_via82xx_channel_reset(chip, viadev);
	return 0;
}


/*
 * pointer callbacks
 */

/*
 * calculate the linear position at the given sg-buffer index and the rest count
 */
static inline unsigned int calc_linear_pos(viadev_t *viadev, unsigned int idx, unsigned int count)
{
	unsigned int size, res;

	size = viadev->idx_table[idx].size;
	res = viadev->idx_table[idx].offset + size - count;

	/* check the validity of the calculated position */
	if (size < count || (res < viadev->lastpos && (res >= viadev->bufsize2 || viadev->lastpos < viadev->bufsize2))) {
#ifdef POINTER_DEBUG
		printk("fail: idx = %i/%i, lastpos = 0x%x, bufsize2 = 0x%x, offsize = 0x%x, size = 0x%x, count = 0x%x\n", idx, viadev->tbl_entries, viadev->lastpos, viadev->bufsize2, viadev->idx_table[idx].offset, viadev->idx_table[idx].size, count);
#endif
		/* count register returns full size when end of buffer is reached */
		if (size != count) {
			snd_printd(KERN_ERR "invalid via82xx_cur_ptr, using last valid pointer\n");
			res = viadev->lastpos;
		} else {
			res = viadev->idx_table[idx].offset + size;
			if (res < viadev->lastpos && (res >= viadev->bufsize2 || viadev->lastpos < viadev->bufsize2)) {
				snd_printd(KERN_ERR "invalid via82xx_cur_ptr (2), using last valid pointer\n");
				res = viadev->lastpos;
			}
		}
	}
	viadev->lastpos = res; /* remember the last positiion */
	if (res >= viadev->bufsize)
		res -= viadev->bufsize;
	return res;
}

/*
 * get the current pointer on via686
 */
static snd_pcm_uframes_t snd_via686_pcm_pointer(snd_pcm_substream_t *substream)
{
	via82xx_t *chip = snd_pcm_substream_chip(substream);
	viadev_t *viadev = (viadev_t *)substream->runtime->private_data;
	unsigned int idx, ptr, count, res;

	snd_assert(viadev->tbl_entries, return 0);
	if (!(inb(VIAREG(chip, OFFSET_STATUS) + viadev->reg_offset) & VIA_REG_STAT_ACTIVE))
		return 0;

	spin_lock(&chip->reg_lock);
	count = inl(VIAREG(chip, OFFSET_CURR_COUNT) + viadev->reg_offset) & 0xffffff;
	/* The via686a does not have the current index register,
	 * so we need to calculate the index from CURR_PTR.
	 */
	ptr = inl(VIAREG(chip, OFFSET_CURR_PTR) + viadev->reg_offset);
	if (ptr <= (unsigned int)viadev->table_addr)
		idx = 0;
	else /* CURR_PTR holds the address + 8 */
		idx = ((ptr - (unsigned int)viadev->table_addr) / 8 - 1) % viadev->tbl_entries;
	res = calc_linear_pos(viadev, idx, count);
	spin_unlock(&chip->reg_lock);

	return bytes_to_frames(substream->runtime, res);
}

/*
 * get the current pointer on via823x
 */
static snd_pcm_uframes_t snd_via8233_pcm_pointer(snd_pcm_substream_t *substream)
{
	via82xx_t *chip = snd_pcm_substream_chip(substream);
	viadev_t *viadev = (viadev_t *)substream->runtime->private_data;
	unsigned int idx, count, res;
	
	snd_assert(viadev->tbl_entries, return 0);
	if (!(inb(VIAREG(chip, OFFSET_STATUS) + viadev->reg_offset) & VIA_REG_STAT_ACTIVE))
		return 0;
	spin_lock(&chip->reg_lock);
	count = inl(VIAREG(chip, OFFSET_CURR_COUNT) + viadev->reg_offset);
	idx = count >> 24;
	count &= 0xffffff;
	res = calc_linear_pos(viadev, idx, count);
	spin_unlock(&chip->reg_lock);

	return bytes_to_frames(substream->runtime, res);
}


/*
 * hw_params callback:
 * allocate the buffer and build up the buffer description table
 */
static int snd_via82xx_hw_params(snd_pcm_substream_t * substream,
				 snd_pcm_hw_params_t * hw_params)
{
	via82xx_t *chip = snd_pcm_substream_chip(substream);
	viadev_t *viadev = (viadev_t *)substream->runtime->private_data;
	int err;

	err = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
	if (err < 0)
		return err;
	err = build_via_table(viadev, substream, chip->pci,
			      params_periods(hw_params),
			      params_period_bytes(hw_params));
	if (err < 0)
		return err;

	return 0;
}

/*
 * hw_free callback:
 * clean up the buffer description table and release the buffer
 */
static int snd_via82xx_hw_free(snd_pcm_substream_t * substream)
{
	via82xx_t *chip = snd_pcm_substream_chip(substream);
	viadev_t *viadev = (viadev_t *)substream->runtime->private_data;

	clean_via_table(viadev, substream, chip->pci);
	snd_pcm_lib_free_pages(substream);
	return 0;
}


/*
 * prepare callback for playback and capture on via686
 */
static void via686_setup_format(via82xx_t *chip, viadev_t *viadev, snd_pcm_runtime_t *runtime)
{
	unsigned long port = chip->port + viadev->reg_offset;

	snd_via82xx_channel_reset(chip, viadev);
	/* this must be set after channel_reset */
	outl((u32)viadev->table_addr, port + VIA_REG_OFFSET_TABLE_PTR);
	outb(VIA_REG_TYPE_AUTOSTART |
	     (runtime->format == SNDRV_PCM_FORMAT_S16_LE ? VIA_REG_TYPE_16BIT : 0) |
	     (runtime->channels > 1 ? VIA_REG_TYPE_STEREO : 0) |
	     ((viadev->reg_offset & 0x10) == 0 ? VIA_REG_TYPE_INT_LSAMPLE : 0) |
	     VIA_REG_TYPE_INT_EOL |
	     VIA_REG_TYPE_INT_FLAG, port + VIA_REG_OFFSET_TYPE);
}

static int snd_via686_playback_prepare(snd_pcm_substream_t *substream)
{
	via82xx_t *chip = snd_pcm_substream_chip(substream);
	viadev_t *viadev = (viadev_t *)substream->runtime->private_data;
	snd_pcm_runtime_t *runtime = substream->runtime;

	snd_ac97_set_rate(chip->ac97, AC97_PCM_FRONT_DAC_RATE, runtime->rate);
	via686_setup_format(chip, viadev, runtime);
	return 0;
}

static int snd_via686_capture_prepare(snd_pcm_substream_t *substream)
{
	via82xx_t *chip = snd_pcm_substream_chip(substream);
	viadev_t *viadev = (viadev_t *)substream->runtime->private_data;
	snd_pcm_runtime_t *runtime = substream->runtime;

	snd_ac97_set_rate(chip->ac97, AC97_PCM_LR_ADC_RATE, runtime->rate);
	via686_setup_format(chip, viadev, runtime);
	return 0;
}

/*
 * lock the current rate
 */
static int via_lock_rate(struct via_rate_lock *rec, int rate)
{
	int changed = 0;

	spin_lock(&rec->lock);
	if (rec->rate) {
		if (rec->rate != rate && rec->used > 1) {
			spin_unlock(&rec->lock);
			return -EINVAL;
		}
	} else {
		rec->rate = rate;
		changed = 1;
	}
	spin_unlock(&rec->lock);
	return changed;
}

/*
 * prepare callback for DSX playback on via823x
 */
static int snd_via8233_playback_prepare(snd_pcm_substream_t *substream)
{
	via82xx_t *chip = snd_pcm_substream_chip(substream);
	viadev_t *viadev = (viadev_t *)substream->runtime->private_data;
	unsigned long port = chip->port + viadev->reg_offset;
	snd_pcm_runtime_t *runtime = substream->runtime;
	int rate_changed;
	u32 rbits;

	if ((rate_changed = via_lock_rate(&chip->rates[0], runtime->rate)) < 0)
		return rate_changed;
	if (rate_changed) {
		snd_ac97_set_rate(chip->ac97, AC97_PCM_FRONT_DAC_RATE, runtime->rate);
		if (viadev->reg_offset == 0x30) /* DSX3 */
			snd_ac97_set_rate(chip->ac97, AC97_SPDIF, runtime->rate);
	}
	rbits = (0xfffff / 48000) * runtime->rate + ((0xfffff % 48000) * runtime->rate) / 48000;
	snd_assert((rbits & ~0xfffff) == 0, return -EINVAL);
	snd_via82xx_channel_reset(chip, viadev);
	outl((u32)viadev->table_addr, port + VIA_REG_OFFSET_TABLE_PTR);
	outb(0 , VIAREG(chip, PLAYBACK_VOLUME_L));
	outb(0 , VIAREG(chip, PLAYBACK_VOLUME_R));
	outl((runtime->format == SNDRV_PCM_FORMAT_S16_LE ? VIA8233_REG_TYPE_16BIT : 0) | /* format */
	     (runtime->channels > 1 ? VIA8233_REG_TYPE_STEREO : 0) | /* stereo */
	     rbits | /* rate */
	     0xff000000,    /* STOP index is never reached */
	     port + VIA_REG_OFFSET_STOP_IDX);
	return 0;
}

/*
 * prepare callback for multi-channel playback on via823x
 */
static int snd_via8233_multi_prepare(snd_pcm_substream_t *substream)
{
	via82xx_t *chip = snd_pcm_substream_chip(substream);
	viadev_t *viadev = (viadev_t *)substream->runtime->private_data;
	unsigned long port = chip->port + viadev->reg_offset;
	snd_pcm_runtime_t *runtime = substream->runtime;
	unsigned int slots;
	int fmt;

	if (via_lock_rate(&chip->rates[0], runtime->rate) < 0)
		return -EINVAL;
	snd_ac97_set_rate(chip->ac97, AC97_PCM_FRONT_DAC_RATE, runtime->rate);
	snd_ac97_set_rate(chip->ac97, AC97_PCM_SURR_DAC_RATE, runtime->rate);
	snd_ac97_set_rate(chip->ac97, AC97_PCM_LFE_DAC_RATE, runtime->rate);
	snd_ac97_set_rate(chip->ac97, AC97_SPDIF, runtime->rate);
	snd_via82xx_channel_reset(chip, viadev);
	outl((u32)viadev->table_addr, port + VIA_REG_OFFSET_TABLE_PTR);

	fmt = (runtime->format == SNDRV_PCM_FORMAT_S16_LE) ? VIA_REG_MULTPLAY_FMT_16BIT : VIA_REG_MULTPLAY_FMT_8BIT;
	fmt |= runtime->channels << 4;
	outb(fmt, port + VIA_REG_OFFSET_TYPE);
	/* set sample number to slot 3, 4, 7, 8, 6, 9 */
	/* corresponding to FL, FR, RL, RR, C, LFE ?? */
	switch (runtime->channels) {
	case 1: slots = (1<<0) | (1<<4); break;
	case 2: slots = (1<<0) | (2<<4); break;
	case 3: slots = (1<<0) | (2<<4) | (5<<8); break;
	case 4: slots = (1<<0) | (2<<4) | (3<<8) | (4<<12); break;
	case 5: slots = (1<<0) | (2<<4) | (5<<8) | (3<<12) | (4<<16); break;
	case 6: slots = (1<<0) | (2<<4) | (5<<8) | (6<<12) | (3<<16) | (4<<20); break;
	default: slots = 0; break;
	}
	/* STOP index is never reached */
	outl(0xff000000 | slots, port + VIA_REG_OFFSET_STOP_IDX);
	return 0;
}

/*
 * prepare callback for capture on via823x
 */
static int snd_via8233_capture_prepare(snd_pcm_substream_t *substream)
{
	via82xx_t *chip = snd_pcm_substream_chip(substream);
	viadev_t *viadev = (viadev_t *)substream->runtime->private_data;
	unsigned long port = chip->port + viadev->reg_offset;
	snd_pcm_runtime_t *runtime = substream->runtime;

	if (via_lock_rate(&chip->rates[1], runtime->rate) < 0)
		return -EINVAL;
	snd_ac97_set_rate(chip->ac97, AC97_PCM_LR_ADC_RATE, runtime->rate);
	snd_via82xx_channel_reset(chip, viadev);
	outl((u32)viadev->table_addr, port + VIA_REG_OFFSET_TABLE_PTR);
	outb(VIA_REG_CAPTURE_FIFO_ENABLE, VIAREG(chip, CAPTURE_FIFO));
	outl((runtime->format == SNDRV_PCM_FORMAT_S16_LE ? VIA8233_REG_TYPE_16BIT : 0) |
	     (runtime->channels > 1 ? VIA8233_REG_TYPE_STEREO : 0) |
	     0xff000000,    /* STOP index is never reached */
	     port + VIA_REG_OFFSET_STOP_IDX);
	return 0;
}


/*
 * pcm hardware definition, identical for both playback and capture
 */
static snd_pcm_hardware_t snd_via82xx_hw =
{
	.info =			(SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
				 SNDRV_PCM_INFO_BLOCK_TRANSFER |
				 SNDRV_PCM_INFO_MMAP_VALID |
				 SNDRV_PCM_INFO_PAUSE),
	.formats =		SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE,
	.rates =		SNDRV_PCM_RATE_48000,
	.rate_min =		48000,
	.rate_max =		48000,
	.channels_min =		1,
	.channels_max =		2,
	.buffer_bytes_max =	128 * 1024,
	.period_bytes_min =	32,
	.period_bytes_max =	128 * 1024,
	.periods_min =		2,
	.periods_max =		VIA_TABLE_SIZE / 2,
	.fifo_size =		0,
};


/*
 * open callback skeleton
 */
static int snd_via82xx_pcm_open(via82xx_t *chip, viadev_t *viadev, snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	int err;
	unsigned long flags;
	struct via_rate_lock *ratep;

	runtime->hw = snd_via82xx_hw;
	
	/* set the hw rate condition */
	ratep = &chip->rates[viadev->direction];
	spin_lock_irqsave(&ratep->lock, flags);
	ratep->used++;
	if (! ratep->rate) {
		int idx = viadev->direction ? AC97_RATES_ADC : AC97_RATES_FRONT_DAC;
		runtime->hw.rates = chip->ac97->rates[idx];
		if (runtime->hw.rates & SNDRV_PCM_RATE_8000)
			runtime->hw.rate_min = 8000;
	} else {
		/* a fixed rate */
		runtime->hw.rates = SNDRV_PCM_RATE_KNOT;
		runtime->hw.rate_max = runtime->hw.rate_min = ratep->rate;
	}
	spin_unlock_irqrestore(&ratep->lock, flags);

	/* we may remove following constaint when we modify table entries
	   in interrupt */
	if ((err = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS)) < 0)
		return err;

	runtime->private_data = viadev;
	viadev->substream = substream;

	return 0;
}


/*
 * open callback for playback on via686 and via823x DSX
 */
static int snd_via82xx_playback_open(snd_pcm_substream_t * substream)
{
	via82xx_t *chip = snd_pcm_substream_chip(substream);
	viadev_t *viadev = &chip->devs[chip->playback_devno + substream->number];

	return snd_via82xx_pcm_open(chip, viadev, substream);
}

/*
 * open callback for playback on via823x multi-channel
 */
static int snd_via8233_multi_open(snd_pcm_substream_t * substream)
{
	via82xx_t *chip = snd_pcm_substream_chip(substream);
	viadev_t *viadev = &chip->devs[chip->multi_devno];
	int err;
	/* channels constraint for VIA8233A
	 * 3 and 5 channels are not supported
	 */
	static unsigned int channels[] = {
		1, 2, 4, 6
	};
	static snd_pcm_hw_constraint_list_t hw_constraints_channels = {
		.count = ARRAY_SIZE(channels),
		.list = channels,
		.mask = 0,
	};

	if ((err = snd_via82xx_pcm_open(chip, viadev, substream)) < 0)
		return err;
	substream->runtime->hw.channels_max = 6;
	if (chip->revision == VIA_REV_8233A)
		snd_pcm_hw_constraint_list(substream->runtime, 0, SNDRV_PCM_HW_PARAM_CHANNELS, &hw_constraints_channels);
	return 0;
}

/*
 * open callback for capture on via686 and via823x
 */
static int snd_via82xx_capture_open(snd_pcm_substream_t * substream)
{
	via82xx_t *chip = snd_pcm_substream_chip(substream);
	viadev_t *viadev = &chip->devs[chip->capture_devno + substream->pcm->device];

	return snd_via82xx_pcm_open(chip, viadev, substream);
}

/*
 * close callback
 */
static int snd_via82xx_pcm_close(snd_pcm_substream_t * substream)
{
	via82xx_t *chip = snd_pcm_substream_chip(substream);
	viadev_t *viadev = (viadev_t *)substream->runtime->private_data;
	unsigned long flags;
	struct via_rate_lock *ratep;

	/* release the rate lock */
	ratep = &chip->rates[viadev->direction];
	spin_lock_irqsave(&ratep->lock, flags);
	ratep->used--;
	if (! ratep->used)
		ratep->rate = 0;
	spin_unlock_irqrestore(&ratep->lock, flags);

	viadev->substream = NULL;
	return 0;
}


/* via686 playback callbacks */
static snd_pcm_ops_t snd_via686_playback_ops = {
	.open =		snd_via82xx_playback_open,
	.close =	snd_via82xx_pcm_close,
	.ioctl =	snd_pcm_lib_ioctl,
	.hw_params =	snd_via82xx_hw_params,
	.hw_free =	snd_via82xx_hw_free,
	.prepare =	snd_via686_playback_prepare,
	.trigger =	snd_via82xx_pcm_trigger,
	.pointer =	snd_via686_pcm_pointer,
	.page =		snd_pcm_sgbuf_ops_page,
};

/* via686 capture callbacks */
static snd_pcm_ops_t snd_via686_capture_ops = {
	.open =		snd_via82xx_capture_open,
	.close =	snd_via82xx_pcm_close,
	.ioctl =	snd_pcm_lib_ioctl,
	.hw_params =	snd_via82xx_hw_params,
	.hw_free =	snd_via82xx_hw_free,
	.prepare =	snd_via686_capture_prepare,
	.trigger =	snd_via82xx_pcm_trigger,
	.pointer =	snd_via686_pcm_pointer,
	.page =		snd_pcm_sgbuf_ops_page,
};

/* via823x DSX playback callbacks */
static snd_pcm_ops_t snd_via8233_playback_ops = {
	.open =		snd_via82xx_playback_open,
	.close =	snd_via82xx_pcm_close,
	.ioctl =	snd_pcm_lib_ioctl,
	.hw_params =	snd_via82xx_hw_params,
	.hw_free =	snd_via82xx_hw_free,
	.prepare =	snd_via8233_playback_prepare,
	.trigger =	snd_via82xx_pcm_trigger,
	.pointer =	snd_via8233_pcm_pointer,
	.page =		snd_pcm_sgbuf_ops_page,
};

/* via823x multi-channel playback callbacks */
static snd_pcm_ops_t snd_via8233_multi_ops = {
	.open =		snd_via8233_multi_open,
	.close =	snd_via82xx_pcm_close,
	.ioctl =	snd_pcm_lib_ioctl,
	.hw_params =	snd_via82xx_hw_params,
	.hw_free =	snd_via82xx_hw_free,
	.prepare =	snd_via8233_multi_prepare,
	.trigger =	snd_via82xx_pcm_trigger,
	.pointer =	snd_via8233_pcm_pointer,
	.page =		snd_pcm_sgbuf_ops_page,
};

/* via823x capture callbacks */
static snd_pcm_ops_t snd_via8233_capture_ops = {
	.open =		snd_via82xx_capture_open,
	.close =	snd_via82xx_pcm_close,
	.ioctl =	snd_pcm_lib_ioctl,
	.hw_params =	snd_via82xx_hw_params,
	.hw_free =	snd_via82xx_hw_free,
	.prepare =	snd_via8233_capture_prepare,
	.trigger =	snd_via82xx_pcm_trigger,
	.pointer =	snd_via8233_pcm_pointer,
	.page =		snd_pcm_sgbuf_ops_page,
};


/*
 * create pcm instances for VIA8233, 8233C and 8235 (not 8233A)
 */
static int __devinit snd_via8233_pcm_new(via82xx_t *chip)
{
	snd_pcm_t *pcm;
	int i, err;

	chip->playback_devno = 0;	/* x 4 */
	chip->multi_devno = 4;		/* x 1 */
	chip->capture_devno = 5;	/* x 2 */
	chip->num_devs = 7;

	/* PCM #0:  4 DSX playbacks and 1 capture */
	err = snd_pcm_new(chip->card, chip->card->shortname, 0, 4, 1, &pcm);
	if (err < 0)
		return err;
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_via8233_playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &snd_via8233_capture_ops);
	pcm->private_data = chip;
	strcpy(pcm->name, chip->card->shortname);
	/* set up playbacks */
	for (i = 0; i < 4; i++) {
		chip->devs[i].reg_offset = 0x10 * i;
		chip->devs[i].direction = 0;
	}
	/* capture */
	chip->devs[chip->capture_devno].reg_offset = VIA_REG_CAPTURE_8233_STATUS;
	chip->devs[chip->capture_devno].direction = 1;

	if ((err = snd_pcm_lib_preallocate_sg_pages_for_all(chip->pci, pcm)) < 0)
		return err;

	/* PCM #1:  multi-channel playback and 2nd capture */
	err = snd_pcm_new(chip->card, chip->card->shortname, 1, 1, 1, &pcm);
	if (err < 0)
		return err;
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_via8233_multi_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &snd_via8233_capture_ops);
	pcm->private_data = chip;
	strcpy(pcm->name, chip->card->shortname);
	/* set up playback */
	chip->devs[chip->multi_devno].reg_offset = VIA_REG_MULTPLAY_STATUS;
	chip->devs[chip->multi_devno].direction = 0;
	/* set up capture */
	chip->devs[chip->capture_devno + 1].reg_offset = VIA_REG_CAPTURE_8233_STATUS + 0x10;
	chip->devs[chip->capture_devno + 1].direction = 1;

	if ((err = snd_pcm_lib_preallocate_sg_pages_for_all(chip->pci, pcm)) < 0)
		return err;

	return 0;
}

/*
 * create pcm instances for VIA8233A
 */
static int __devinit snd_via8233a_pcm_new(via82xx_t *chip)
{
	snd_pcm_t *pcm;
	int err;

	chip->playback_devno = 0;
	chip->multi_devno = 1;
	chip->capture_devno = 2;
	chip->num_devs = 3;

	/* PCM #0:  multi-channel playback and capture */
	err = snd_pcm_new(chip->card, chip->card->shortname, 0, 1, 1, &pcm);
	if (err < 0)
		return err;
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_via8233_multi_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &snd_via8233_capture_ops);
	pcm->private_data = chip;
	strcpy(pcm->name, chip->card->shortname);
	/* set up playback */
	chip->devs[chip->multi_devno].reg_offset = VIA_REG_MULTPLAY_STATUS;
	chip->devs[chip->multi_devno].direction = 0;
	/* capture */
	chip->devs[chip->capture_devno].reg_offset = VIA_REG_CAPTURE_8233_STATUS;
	chip->devs[chip->capture_devno].direction = 1;

	/* PCM #1:  DXS3 playback (for spdif) */
	err = snd_pcm_new(chip->card, chip->card->shortname, 1, 1, 0, &pcm);
	if (err < 0)
		return err;
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_via8233_playback_ops);
	pcm->private_data = chip;
	strcpy(pcm->name, chip->card->shortname);
	/* set up playback */
	chip->devs[chip->playback_devno].reg_offset = 0x30;
	chip->devs[chip->playback_devno].direction = 0;

	if ((err = snd_pcm_lib_preallocate_sg_pages_for_all(chip->pci, pcm)) < 0)
		return err;

	return 0;
}

/*
 * create a pcm instance for via686a/b
 */
static int __devinit snd_via686_pcm_new(via82xx_t *chip)
{
	snd_pcm_t *pcm;
	int err;

	chip->playback_devno = 0;
	chip->capture_devno = 1;
	chip->num_devs = 2;

	err = snd_pcm_new(chip->card, chip->card->shortname, 0, 1, 1, &pcm);
	if (err < 0)
		return err;
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_via686_playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &snd_via686_capture_ops);
	pcm->private_data = chip;
	strcpy(pcm->name, chip->card->shortname);
	chip->devs[0].reg_offset = VIA_REG_PLAYBACK_STATUS;
	chip->devs[0].direction = 0;
	chip->devs[1].reg_offset = VIA_REG_CAPTURE_STATUS;
	chip->devs[1].direction = 1;

	if ((err = snd_pcm_lib_preallocate_sg_pages_for_all(chip->pci, pcm)) < 0)
		return err;

	return 0;
}


/*
 *  Mixer part
 */

static int snd_via8233_capture_source_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	static char *texts[2] = {
		"Line", "Mic"
	};
	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = 2;
	if (uinfo->value.enumerated.item >= 2)
		uinfo->value.enumerated.item = 1;
	strcpy(uinfo->value.enumerated.name, texts[uinfo->value.enumerated.item]);
	return 0;
}

static int snd_via8233_capture_source_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	via82xx_t *chip = snd_kcontrol_chip(kcontrol);
	unsigned long port = chip->port + kcontrol->id.index ? (VIA_REG_CAPTURE_CHANNEL + 0x10) : VIA_REG_CAPTURE_CHANNEL;
	ucontrol->value.enumerated.item[0] = inb(port) & VIA_REG_CAPTURE_CHANNEL_MIC ? 1 : 0;
	return 0;
}

static int snd_via8233_capture_source_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	via82xx_t *chip = snd_kcontrol_chip(kcontrol);
	unsigned long port = chip->port + kcontrol->id.index ? (VIA_REG_CAPTURE_CHANNEL + 0x10) : VIA_REG_CAPTURE_CHANNEL;
	unsigned long flags;
	u8 val, oval;

	spin_lock_irqsave(&chip->reg_lock, flags);
	oval = inb(port);
	val = oval & ~VIA_REG_CAPTURE_CHANNEL_MIC;
	if (ucontrol->value.enumerated.item[0])
		val |= VIA_REG_CAPTURE_CHANNEL_MIC;
	if (val != oval)
		outb(val, port);
	spin_unlock_irqrestore(&chip->reg_lock, flags);
	return val != oval;
}

static snd_kcontrol_new_t snd_via8233_capture_source __devinitdata = {
	.name = "Input Source Select",
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.info = snd_via8233_capture_source_info,
	.get = snd_via8233_capture_source_get,
	.put = snd_via8233_capture_source_put,
};

static int snd_via8233_dxs3_spdif_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

static int snd_via8233_dxs3_spdif_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	via82xx_t *chip = snd_kcontrol_chip(kcontrol);
	u8 val;

	pci_read_config_byte(chip->pci, 0x49, &val);
	ucontrol->value.integer.value[0] = (val & 0x08) ? 1 : 0;
	return 0;
}

static int snd_via8233_dxs3_spdif_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	via82xx_t *chip = snd_kcontrol_chip(kcontrol);
	u8 val, oval;

	pci_read_config_byte(chip->pci, 0x49, &oval);
	val = oval & ~0x08;
	if (ucontrol->value.integer.value[0])
		val |= 0x08;
	if (val != oval) {
		pci_write_config_byte(chip->pci, 0x49, val);
		return 1;
	}
	return 0;
}

static snd_kcontrol_new_t snd_via8233_dxs3_spdif_control __devinitdata = {
	.name = "IEC958 Output Switch",
	.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
	.info = snd_via8233_dxs3_spdif_info,
	.get = snd_via8233_dxs3_spdif_get,
	.put = snd_via8233_dxs3_spdif_put,
};

/*
 */

static void snd_via82xx_mixer_free_ac97(ac97_t *ac97)
{
	via82xx_t *chip = snd_magic_cast(via82xx_t, ac97->private_data, return);
	chip->ac97 = NULL;
}

static int __devinit snd_via82xx_mixer_new(via82xx_t *chip)
{
	ac97_t ac97;
	int err;

	memset(&ac97, 0, sizeof(ac97));
	ac97.write = snd_via82xx_codec_write;
	ac97.read = snd_via82xx_codec_read;
	ac97.wait = snd_via82xx_codec_wait;
	ac97.private_data = chip;
	ac97.private_free = snd_via82xx_mixer_free_ac97;
	ac97.clock = chip->ac97_clock;
	if ((err = snd_ac97_mixer(chip->card, &ac97, &chip->ac97)) < 0)
		return err;
	return 0;
}

/*
 * joystick
 */

static int snd_via82xx_joystick_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

static int snd_via82xx_joystick_get(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	via82xx_t *chip = snd_kcontrol_chip(kcontrol);
	u16 val;

	pci_read_config_word(chip->pci, 0x42, &val);
	ucontrol->value.integer.value[0] = (val & 0x08) ? 1 : 0;
	return 0;
}

static int snd_via82xx_joystick_put(snd_kcontrol_t *kcontrol, snd_ctl_elem_value_t *ucontrol)
{
	via82xx_t *chip = snd_kcontrol_chip(kcontrol);
	u16 val, oval;

	pci_read_config_word(chip->pci, 0x42, &oval);
	val = oval & ~0x08;
	if (ucontrol->value.integer.value[0])
		val |= 0x08;
	if (val != oval) {
		pci_write_config_word(chip->pci, 0x42, val);
		return 1;
	}
	return 0;
}

static snd_kcontrol_new_t snd_via82xx_joystick_control __devinitdata = {
	.name = "Joystick",
	.iface = SNDRV_CTL_ELEM_IFACE_CARD,
	.info = snd_via82xx_joystick_info,
	.get = snd_via82xx_joystick_get,
	.put = snd_via82xx_joystick_put,
};

/*
 *
 */

static int snd_via8233_init_misc(via82xx_t *chip, int dev)
{
	int i, err, caps;
	unsigned char val;

	caps = chip->revision == VIA_REV_8233A ? 1 : 2;
	for (i = 0; i < caps; i++) {
		snd_via8233_capture_source.index = i;
		err = snd_ctl_add(chip->card, snd_ctl_new1(&snd_via8233_capture_source, chip));
		if (err < 0)
			return err;
	}
	err = snd_ctl_add(chip->card, snd_ctl_new1(&snd_via8233_dxs3_spdif_control, chip));
	if (err < 0)
		return err;

	/* select spdif data slot 10/11 */
	pci_read_config_byte(chip->pci, 0x49, &val);
	val &= ~0x03;
	pci_write_config_byte(chip->pci, 0x49, val);

	return 0;
}

static int snd_via686_init_misc(via82xx_t *chip, int dev)
{
	unsigned char legacy, legacy_cfg;
	int rev_h = 0;

	legacy = chip->old_legacy;
	legacy_cfg = chip->old_legacy_cfg;
	legacy |= 0x40;		/* disable MIDI */
	legacy &= ~0x08;	/* disable joystick */
	if (chip->revision >= 0x20) {
		if (check_region(pci_resource_start(chip->pci, 2), 4)) {
			rev_h = 0;
			legacy &= ~0x80;	/* disable PCI I/O 2 */
		} else {
			rev_h = 1;
			legacy |= 0x80;		/* enable PCI I/O 2 */
		}
	}
	pci_write_config_byte(chip->pci, 0x42, legacy);
	pci_write_config_byte(chip->pci, 0x43, legacy_cfg);
	if (rev_h && mpu_port[dev] >= 0x200) {	/* force MIDI */
		legacy |= 0x02;	/* enable MPU */
		pci_write_config_dword(chip->pci, 0x18, (mpu_port[dev] & 0xfffc) | 0x01);
	} else {
		if (rev_h && (legacy & 0x02)) {
			mpu_port[dev] = pci_resource_start(chip->pci, 2);
			if (mpu_port[dev] < 0x200)	/* bad value */
				legacy &= ~0x02;	/* disable MIDI */
		} else {
			switch (mpu_port[dev]) {	/* force MIDI */
			case 0x300:
			case 0x310:
			case 0x320:
			case 0x330:
				legacy_cfg &= ~(3 << 2);
				legacy_cfg |= (mpu_port[dev] & 0x0030) >> 2;
				legacy |= 0x02;
				break;
			default:			/* no, use BIOS settings */
				if (legacy & 0x02)
					mpu_port[dev] = 0x300 + ((legacy_cfg & 0x000c) << 2);
			}
		}
	}
	pci_write_config_byte(chip->pci, 0x42, legacy);
	pci_write_config_byte(chip->pci, 0x43, legacy_cfg);
	if (legacy & 0x02) {
		if (check_region(mpu_port[dev], 2)) {
			printk(KERN_WARNING "unable to get MPU-401 port at 0x%lx, skipping\n", mpu_port[dev]);
			legacy &= ~0x02;
			pci_write_config_byte(chip->pci, 0x42, legacy);
			goto __skip_mpu;
		}
		if (snd_mpu401_uart_new(chip->card, 0, MPU401_HW_VIA686A,
					mpu_port[dev], 0,
					chip->irq, 0,
					&chip->rmidi) < 0) {
			printk(KERN_WARNING "unable to initialize MPU-401 at 0x%lx, skipping\n", mpu_port[dev]);
			legacy &= ~0x02;
			pci_write_config_byte(chip->pci, 0x42, legacy);
			goto __skip_mpu;
		}
		legacy &= ~0x40;	/* enable MIDI interrupt */
		pci_write_config_byte(chip->pci, 0x42, legacy);
	__skip_mpu:
		;
	}
	
	/* card switches */
	return snd_ctl_add(chip->card, snd_ctl_new1(&snd_via82xx_joystick_control, chip));
}


/*
 *
 */

static int __devinit snd_via82xx_chip_init(via82xx_t *chip)
{
	ac97_t ac97;
	unsigned int val;
	int max_count;
	unsigned char pval;

	memset(&ac97, 0, sizeof(ac97));
	ac97.private_data = chip;

#if 0 /* broken on K7M? */
	if (chip->chip_type == TYPE_VIA686)
		/* disable all legacy ports */
		pci_write_config_byte(chip->pci, 0x42, 0);
#endif
	pci_read_config_byte(chip->pci, 0x40, &pval);
	if (! (pval & 0x01)) { /* codec not ready? */
		/* deassert ACLink reset, force SYNC */
		pci_write_config_byte(chip->pci, 0x41, 0xe0);
		udelay(100);
		/* deassert ACLink reset, force SYNC (warm AC'97 reset) */
		pci_write_config_byte(chip->pci, 0x41, 0x60);
		udelay(2);
		/* pci_write_config_byte(chip->pci, 0x41, 0x00);
		   udelay(100);
		*/
		/* ACLink on, deassert ACLink reset, VSR, SGD data out */
		/* note - FM data out has trouble with non VRA codecs !! */
		pci_write_config_byte(chip->pci, 0x41, 0xcc);
		udelay(100);
	}
	
	/* Make sure VRA is enabled, in case we didn't do a
	 * complete codec reset, above */
	pci_read_config_byte(chip->pci, 0x41, &pval);
	if ((pval & 0xcc) != 0xcc) {
		/* ACLink on, deassert ACLink reset, VSR, SGD data out */
		/* note - FM data out has trouble with non VRA codecs !! */
		pci_write_config_byte(chip->pci, 0x41, 0xcc);
		udelay(100);
	}

	/* wait until codec ready */
	max_count = ((3 * HZ) / 4) + 1;
	do {
		pci_read_config_byte(chip->pci, 0x40, &pval);
		if (pval & 0x01) /* primary codec ready */
			break;
		set_current_state(TASK_UNINTERRUPTIBLE);
		schedule_timeout(1);
	} while (--max_count > 0);

	if ((val = snd_via82xx_codec_xread(chip)) & VIA_REG_AC97_BUSY)
		snd_printk("AC'97 codec is not ready [0x%x]\n", val);

	/* and then reset codec.. */
	snd_via82xx_codec_ready(chip, 0);
	snd_via82xx_codec_write(&ac97, AC97_RESET, 0x0000);
	snd_via82xx_codec_read(&ac97, 0);

#if 0 /* FIXME: we don't support the second codec yet so skip the detection now.. */
	snd_via82xx_codec_xwrite(chip, VIA_REG_AC97_READ |
				 VIA_REG_AC97_SECONDARY_VALID |
				 (VIA_REG_AC97_CODEC_ID_SECONDARY << VIA_REG_AC97_CODEC_ID_SHIFT));
	max_count = ((3 * HZ) / 4) + 1;
	snd_via82xx_codec_xwrite(chip, VIA_REG_AC97_READ |
				 VIA_REG_AC97_SECONDARY_VALID |
				 (VIA_REG_AC97_CODEC_ID_SECONDARY << VIA_REG_AC97_CODEC_ID_SHIFT));
	do {
		if ((val = snd_via82xx_codec_xread(chip)) & VIA_REG_AC97_SECONDARY_VALID) {
			chip->ac97_secondary = 1;
			goto __ac97_ok2;
		}
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(1);
	} while (--max_count > 0);
	/* This is ok, the most of motherboards have only one codec */

      __ac97_ok2:
#endif

	if (chip->chip_type == TYPE_VIA686) {
		/* route FM trap to IRQ, disable FM trap */
		pci_write_config_byte(chip->pci, 0x48, 0);
		/* disable all GPI interrupts */
		outl(0, chip->port + 0x8c);
	}

	return 0;
}

static int snd_via82xx_free(via82xx_t *chip)
{
	int i;

	if (chip->irq < 0)
		goto __end_hw;
	/* disable interrupts */
	for (i = 0; i < chip->num_devs; i++)
		snd_via82xx_channel_reset(chip, &chip->devs[i]);
	synchronize_irq(chip->irq);
      __end_hw:
	if (chip->res_port) {
		release_resource(chip->res_port);
		kfree_nocheck(chip->res_port);
	}
	if (chip->irq >= 0)
		free_irq(chip->irq, (void *)chip);
	if (chip->chip_type == TYPE_VIA686) {
		pci_write_config_byte(chip->pci, 0x42, chip->old_legacy);
		pci_write_config_byte(chip->pci, 0x43, chip->old_legacy_cfg);
	}
	snd_magic_kfree(chip);
	return 0;
}

static int snd_via82xx_dev_free(snd_device_t *device)
{
	via82xx_t *chip = snd_magic_cast(via82xx_t, device->device_data, return -ENXIO);
	return snd_via82xx_free(chip);
}

static int __devinit snd_via82xx_create(snd_card_t * card,
					struct pci_dev *pci,
					int chip_type,
					unsigned int ac97_clock,
					via82xx_t ** r_via)
{
	via82xx_t *chip;
	int err;
        static snd_device_ops_t ops = {
		.dev_free =	snd_via82xx_dev_free,
        };

	if ((err = pci_enable_device(pci)) < 0)
		return err;

	if ((chip = snd_magic_kcalloc(via82xx_t, 0, GFP_KERNEL)) == NULL)
		return -ENOMEM;

	chip->chip_type = chip_type;

	spin_lock_init(&chip->reg_lock);
	spin_lock_init(&chip->ac97_lock);
	spin_lock_init(&chip->rates[0].lock);
	spin_lock_init(&chip->rates[1].lock);
	chip->card = card;
	chip->pci = pci;
	chip->irq = -1;

	pci_read_config_byte(pci, 0x42, &chip->old_legacy);
	pci_read_config_byte(pci, 0x43, &chip->old_legacy_cfg);

	chip->port = pci_resource_start(pci, 0);
	if ((chip->res_port = request_region(chip->port, 256, card->driver)) == NULL) {
		snd_via82xx_free(chip);
		snd_printk("unable to grab ports 0x%lx-0x%lx\n", chip->port, chip->port + 256 - 1);
		return -EBUSY;
	}
	if (request_irq(pci->irq, snd_via82xx_interrupt, SA_INTERRUPT|SA_SHIRQ,
			card->driver, (void *)chip)) {
		snd_via82xx_free(chip);
		snd_printk("unable to grab IRQ %d\n", chip->irq);
		return -EBUSY;
	}
	chip->irq = pci->irq;
	if (ac97_clock >= 8000 && ac97_clock <= 48000)
		chip->ac97_clock = ac97_clock;
	pci_read_config_byte(pci, PCI_REVISION_ID, &chip->revision);
	synchronize_irq(chip->irq);

	if ((err = snd_via82xx_chip_init(chip)) < 0) {
		snd_via82xx_free(chip);
		return err;
	}

	if ((err = snd_device_new(card, SNDRV_DEV_LOWLEVEL, chip, &ops)) < 0) {
		snd_via82xx_free(chip);
		return err;
	}

	/* The 8233 ac97 controller does not implement the master bit
	 * in the pci command register. IMHO this is a violation of the PCI spec.
	 * We call pci_set_master here because it does not hurt. */
	pci_set_master(pci);

	*r_via = chip;
	return 0;
}

static int __devinit snd_via82xx_probe(struct pci_dev *pci,
				       const struct pci_device_id *pci_id)
{
	static int dev;
	snd_card_t *card;
	via82xx_t *chip;
	unsigned char revision;
	int chip_type;
	int i, err;

	if (dev >= SNDRV_CARDS)
		return -ENODEV;
	if (!enable[dev]) {
		dev++;
		return -ENOENT;
	}

	card = snd_card_new(index[dev], id[dev], THIS_MODULE, 0);
	if (card == NULL)
		return -ENOMEM;

	chip_type = pci_id->driver_data;
	pci_read_config_byte(pci, PCI_REVISION_ID, &revision);
	switch (chip_type) {
	case TYPE_VIA686:
		strcpy(card->driver, "VIA686A");
		strcpy(card->shortname, "VIA 82C686A/B");
		break;
	case TYPE_VIA8233:
		if (revision == VIA_REV_8233A) {
			strcpy(card->driver, "VIA8233A");
			strcpy(card->shortname, "VIA 8233A");
		} else {
			strcpy(card->driver, "VIA8233");
			strcpy(card->shortname, "VIA 8233/C");
		}
		break;
	default:
		snd_printk(KERN_ERR "invalid chip type %d\n", chip_type);
		err = -EINVAL;
		goto __error;
	}
		
	if ((err = snd_via82xx_create(card, pci, chip_type, ac97_clock[dev], &chip)) < 0)
		goto __error;

	if ((err = snd_via82xx_mixer_new(chip)) < 0)
		goto __error;

	if (chip_type == TYPE_VIA686) {
		if ((err = snd_via686_pcm_new(chip)) < 0 ||
		    (err = snd_via686_init_misc(chip, dev)) < 0)
			goto __error;
	} else {
		if (revision == VIA_REV_8233A) {
			if ((err = snd_via8233a_pcm_new(chip)) < 0)
				goto __error;
		} else {
			if ((err = snd_via8233_pcm_new(chip)) < 0)
				goto __error;
		}
		if ((err = snd_via8233_init_misc(chip, dev)) < 0)
			goto __error;
	}
	/* disable interrupts */
	for (i = 0; i < chip->num_devs; i++)
		snd_via82xx_channel_reset(chip, &chip->devs[i]);

	sprintf(card->longname, "%s at 0x%lx, irq %d",
		card->shortname, chip->port, chip->irq);

	if ((err = snd_card_register(card)) < 0) {
		snd_card_free(card);
		return err;
	}
	pci_set_drvdata(pci, card);
	dev++;
	return 0;

 __error:
	snd_card_free(card);
	return err;
}

static void __devexit snd_via82xx_remove(struct pci_dev *pci)
{
	snd_card_free(pci_get_drvdata(pci));
	pci_set_drvdata(pci, NULL);
}

static struct pci_driver driver = {
	.name = "VIA 82xx Audio",
	.id_table = snd_via82xx_ids,
	.probe = snd_via82xx_probe,
	.remove = __devexit_p(snd_via82xx_remove),
};

static int __init alsa_card_via82xx_init(void)
{
	int err;

	if ((err = pci_module_init(&driver)) < 0) {
#ifdef MODULE
		printk(KERN_ERR "VIA 82xx soundcard not found or device busy\n");
#endif
		return err;
	}
	return 0;
}

static void __exit alsa_card_via82xx_exit(void)
{
	pci_unregister_driver(&driver);
}

module_init(alsa_card_via82xx_init)
module_exit(alsa_card_via82xx_exit)

#ifndef MODULE

/* format is: snd-via82xx=enable,index,id,
			  mpu_port,ac97_clock */

static int __init alsa_card_via82xx_setup(char *str)
{
	static unsigned __initdata nr_dev = 0;

	if (nr_dev >= SNDRV_CARDS)
		return 0;
	(void)(get_option(&str,&enable[nr_dev]) == 2 &&
	       get_option(&str,&index[nr_dev]) == 2 &&
	       get_id(&str,&id[nr_dev]) == 2 &&
	       get_option(&str,(int *)&mpu_port[nr_dev]) == 2 &&
	       get_option(&str,&ac97_clock[nr_dev]) == 2);
	nr_dev++;
	return 1;
}

__setup("snd-via82xx=", alsa_card_via82xx_setup);

#endif /* ifndef MODULE */
 
