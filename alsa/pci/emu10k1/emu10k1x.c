/*
 *  Copyright (c) by Francisco Moraes <fmoraes@nc.rr.com>
 *  Driver EMU10K1X chips
 *
 *  Parts of this code were adapted from audigyls.c driver which is
 *  Copyright (c) by James Courtier-Dutton <James@superbug.demon.co.uk>
 *
 *  BUGS:
 *    --
 *
 *  TODO:
 *    MIDI support
 *
 *  Chips (SB0200 model):
 *    - EMU10K1X-DBQ
 *    - STAC 9708T
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

MODULE_AUTHOR("Francisco Moraes <fmoraes@nc.rr.com>");
MODULE_DESCRIPTION("EMU10K1X");
MODULE_LICENSE("GPL");
MODULE_CLASSES("{sound}");
MODULE_DEVICES("{{Dell Creative Labs,SB Live!}");

// module parameters (see "Module Parameters")
static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;
static int enable[SNDRV_CARDS] = SNDRV_DEFAULT_ENABLE_PNP;
static int boot_devs;

module_param_array(index, int, boot_devs, 0444);
MODULE_PARM_DESC(index, "Index value for the EMU10K1X soundcard.");
MODULE_PARM_SYNTAX(index, SNDRV_INDEX_DESC);
module_param_array(id, charp, boot_devs, 0444);
MODULE_PARM_DESC(id, "ID string for the EMU10K1X soundcard.");
MODULE_PARM_SYNTAX(id, SNDRV_ID_DESC);
module_param_array(enable, bool, boot_devs, 0444);
MODULE_PARM_DESC(enable, "Enable the EMU10K1X soundcard.");
MODULE_PARM_SYNTAX(enable, SNDRV_ENABLE_DESC);


// some definitions were borrowed from emu10k1 driver as they seem to be the same
/************************************************************************************************/
/* PCI function 0 registers, address = <val> + PCIBASE0						*/
/************************************************************************************************/

#define PTR			0x00		/* Indexed register set pointer register	*/
						/* NOTE: The CHANNELNUM and ADDRESS words can	*/
						/* be modified independently of each other.	*/

#define DATA			0x04		/* Indexed register set data register		*/

#define IPR			0x08		/* Global interrupt pending register		*/
						/* Clear pending interrupts by writing a 1 to	*/
						/* the relevant bits and zero to the other bits	*/
#define IPR_CH_0_LOOP           0x00000800      /* Channel 0 loop                               */
#define IPR_CH_0_HALF_LOOP      0x00000100      /* Channel 0 half loop                          */
#define IPR_CAP_0_LOOP          0x00080000      /* Channel capture loop                               */
#define IPR_CAP_0_HALF_LOOP     0x00010000      /* Channel capture half loop                          */

#define INTE			0x0c		/* Interrupt enable register			*/
#define INTE_CH_0_LOOP          0x00000800      /* Channel 0 loop                               */
#define INTE_CH_0_HALF_LOOP     0x00000100      /* Channel 0 half loop                          */
#define INTE_CAP_0_LOOP         0x00080000      /* Channel capture loop                               */
#define INTE_CAP_0_HALF_LOOP    0x00010000      /* Channel capture half loop                          */

#define HCFG			0x14		/* Hardware config register			*/

#define HCFG_LOCKSOUNDCACHE	0x00000008	/* 1 = Cancel bustmaster accesses to soundcache */
						/* NOTE: This should generally never be used.  	*/
#define HCFG_AUDIOENABLE	0x00000001	/* 0 = CODECs transmit zero-valued samples	*/
						/* Should be set to 1 when the EMU10K1 is	*/
						/* completely initialized.			*/


#define AC97DATA		0x1c		/* AC97 register set data register (16 bit)	*/

#define AC97ADDRESS		0x1e		/* AC97 register set address register (8 bit)	*/

/********************************************************************************************************/
/* Emu10k1x pointer-offset register set, accessed through the PTR and DATA registers			*/
/********************************************************************************************************/
#define PLAYBACK_DMA_ADDR	0x04		/* Playback DMA addresss */
#define PLAYBACK_BUFFER_SIZE	0x05		/* Playback buffer size */
#define PLAYBACK_POINTER	0x06		/* Playback buffer pointer. Sample currently in DAC */

/* Only one capture channel supported */
#define CAPTURE_DMA_ADDR	0x10		/* Capture DMA address */
#define CAPTURE_BUFFER_SIZE	0x11		/* Capture buffer size */
#define CAPTURE_POINTER		0x12		/* Capture buffer pointer. Sample currently in ADC */

/* From 0x20 - 0x3f, last samples played on each channel */

#define TRIGGER_CHANNEL         0x40            /* Trigger channel playback                     */
#define TRIGGER_CHANNEL_0       0x00000001      /* Trigger channel 0                            */
#define TRIGGER_CHANNEL_1       0x00000002      /* Trigger channel 1                            */
#define TRIGGER_CHANNEL_2       0x00000004      /* Trigger channel 2                            */
#define TRIGGER_CAPTURE         0x00000100      /* Trigger capture channel                      */

#define ROUTING                 0x41            /* Setup sound routing ?                        */
#define ROUTING_FRONT_LEFT      0x00000001
#define ROUTING_FRONT_RIGHT     0x00000002
#define ROUTING_REAR_LEFT       0x00000004
#define ROUTING_REAR_RIGHT      0x00000008
#define ROUTING_CENTER_LFE      0x00010000

#define SPCS0			0x42		/* SPDIF output Channel Status 0 register	*/

#define SPCS1			0x43		/* SPDIF output Channel Status 1 register	*/

#define SPCS2			0x44		/* SPDIF output Channel Status 2 register	*/

#define SPCS_CLKACCYMASK	0x30000000	/* Clock accuracy				*/
#define SPCS_CLKACCY_1000PPM	0x00000000	/* 1000 parts per million			*/
#define SPCS_CLKACCY_50PPM	0x10000000	/* 50 parts per million				*/
#define SPCS_CLKACCY_VARIABLE	0x20000000	/* Variable accuracy				*/
#define SPCS_SAMPLERATEMASK	0x0f000000	/* Sample rate					*/
#define SPCS_SAMPLERATE_44	0x00000000	/* 44.1kHz sample rate				*/
#define SPCS_SAMPLERATE_48	0x02000000	/* 48kHz sample rate				*/
#define SPCS_SAMPLERATE_32	0x03000000	/* 32kHz sample rate				*/
#define SPCS_CHANNELNUMMASK	0x00f00000	/* Channel number				*/
#define SPCS_CHANNELNUM_UNSPEC	0x00000000	/* Unspecified channel number			*/
#define SPCS_CHANNELNUM_LEFT	0x00100000	/* Left channel					*/
#define SPCS_CHANNELNUM_RIGHT	0x00200000	/* Right channel				*/
#define SPCS_SOURCENUMMASK	0x000f0000	/* Source number				*/
#define SPCS_SOURCENUM_UNSPEC	0x00000000	/* Unspecified source number			*/
#define SPCS_GENERATIONSTATUS	0x00008000	/* Originality flag (see IEC-958 spec)		*/
#define SPCS_CATEGORYCODEMASK	0x00007f00	/* Category code (see IEC-958 spec)		*/
#define SPCS_MODEMASK		0x000000c0	/* Mode (see IEC-958 spec)			*/
#define SPCS_EMPHASISMASK	0x00000038	/* Emphasis					*/
#define SPCS_EMPHASIS_NONE	0x00000000	/* No emphasis					*/
#define SPCS_EMPHASIS_50_15	0x00000008	/* 50/15 usec 2 channel				*/
#define SPCS_COPYRIGHT		0x00000004	/* Copyright asserted flag -- do not modify	*/
#define SPCS_NOTAUDIODATA	0x00000002	/* 0 = Digital audio, 1 = not audio		*/
#define SPCS_PROFESSIONAL	0x00000001	/* 0 = Consumer (IEC-958), 1 = pro (AES3-1992)	*/

#define SPDIF_SELECT		0x45		/* Enables SPDIF or Analogue outputs 0-Analogue, 0x700-SPDIF */

/**
 * The hardware has 3 channels for playback and 1 for capture.
 *  - channel 0 is the front channel
 *  - channel 1 is the rear channel
 *  - channel 2 is the center/lfe chanel
 * Volume is controlled by the AC97 for the front and rear channels by
 * the PCM Playback Volume, Sigmatel Surround Playback Volume and 
 * Surround Playback Volume. The Sigmatel 4-Speaker Stereo switch affects
 * the front/rear channel mixing in the REAR OUT jack. When using the
 * 4-Speaker Stereo, both front and rear channels will be mixed in the
 * REAR OUT.
 * The center/lfe channel has no volume control and cannot be muted during
 * playback.
 */

#define chip_t emu10k1x_t

typedef struct snd_emu10k1x_voice emu10k1x_voice_t;
typedef struct snd_emu10k1x emu10k1x_t;
typedef struct snd_emu10k1x_pcm emu10k1x_pcm_t;

struct snd_emu10k1x_voice {
	emu10k1x_t *emu;
	int number;
	int use;
	void (*interrupt)(emu10k1x_t *emu, emu10k1x_voice_t *pvoice);
  
	emu10k1x_pcm_t *epcm;
};

struct snd_emu10k1x_pcm {
	emu10k1x_t *emu;
	snd_pcm_substream_t *substream;
	emu10k1x_voice_t *voice;
	unsigned short running;
};

// definition of the chip-specific record
struct snd_emu10k1x {
	snd_card_t *card;
	struct pci_dev *pci;

	unsigned long port;
	struct resource *res_port;
	int irq;

	unsigned int revision;		/* chip revision */
	unsigned int serial;            /* serial number */
	unsigned short model;		/* subsystem id */

	spinlock_t emu_lock;
	spinlock_t voice_lock;

	ac97_t *ac97;
	snd_pcm_t *pcm;

	emu10k1x_voice_t voices[3];
	emu10k1x_voice_t capture_voice;
	u32 spdif_bits[3]; // SPDIF out setup

	struct snd_dma_device dma_dev;
	struct snd_dma_buffer buffer;
};

#define emu10k1x_t_magic        0xa15a4501
#define emu10k1x_pcm_t_magic	0xa15a4502

/* hardware definition */
static snd_pcm_hardware_t snd_emu10k1x_playback_hw = {
	.info =			(SNDRV_PCM_INFO_MMAP | 
				 SNDRV_PCM_INFO_INTERLEAVED |
				 SNDRV_PCM_INFO_BLOCK_TRANSFER |
				 SNDRV_PCM_INFO_MMAP_VALID),
	.formats =		SNDRV_PCM_FMTBIT_S16_LE,
	.rates =		SNDRV_PCM_RATE_48000,
	.rate_min =		48000,
	.rate_max =		48000,
	.channels_min =		2,
	.channels_max =		2,
	.buffer_bytes_max =	(32*1024),
	.period_bytes_min =	64,
	.period_bytes_max =	(16*1024),
	.periods_min =		2,
	.periods_max =		2,
	.fifo_size =		0,
};

static snd_pcm_hardware_t snd_emu10k1x_capture_hw = {
	.info =			(SNDRV_PCM_INFO_MMAP | 
				 SNDRV_PCM_INFO_INTERLEAVED |
				 SNDRV_PCM_INFO_BLOCK_TRANSFER |
				 SNDRV_PCM_INFO_MMAP_VALID),
	.formats =		SNDRV_PCM_FMTBIT_S16_LE,
	.rates =		SNDRV_PCM_RATE_48000,
	.rate_min =		48000,
	.rate_max =		48000,
	.channels_min =		2,
	.channels_max =		2,
	.buffer_bytes_max =	(32*1024),
	.period_bytes_min =	64,
	.period_bytes_max =	(16*1024),
	.periods_min =		2,
	.periods_max =		2,
	.fifo_size =		0,
};

static unsigned int snd_emu10k1x_ptr_read(emu10k1x_t * emu, 
					  unsigned int reg, 
					  unsigned int chn)
{
	unsigned long flags;
	unsigned int regptr, val;
  
	regptr = (reg << 16) | chn;

	spin_lock_irqsave(&emu->emu_lock, flags);
	outl(regptr, emu->port + PTR);
	val = inl(emu->port + DATA);
	spin_unlock_irqrestore(&emu->emu_lock, flags);
	return val;
}

static void snd_emu10k1x_ptr_write(emu10k1x_t *emu, 
				   unsigned int reg, 
				   unsigned int chn, 
				   unsigned int data)
{
	unsigned int regptr;
	unsigned long flags;

	regptr = (reg << 16) | chn;

	spin_lock_irqsave(&emu->emu_lock, flags);
	outl(regptr, emu->port + PTR);
	outl(data, emu->port + DATA);
	spin_unlock_irqrestore(&emu->emu_lock, flags);
}

static void snd_emu10k1x_intr_enable(emu10k1x_t *emu, unsigned int intrenb)
{
	unsigned long flags;
	unsigned int enable;
  
	spin_lock_irqsave(&emu->emu_lock, flags);
	enable = inl(emu->port + INTE) | intrenb;
	outl(enable, emu->port + INTE);
	spin_unlock_irqrestore(&emu->emu_lock, flags);
}

static int voice_alloc(emu10k1x_t *emu, emu10k1x_voice_t **rvoice, int id)
{
	emu10k1x_voice_t *voice;
	int idx = id;

	*rvoice = NULL;
	voice = &emu->voices[idx];
	if (!voice->use) {
		voice->use = 1;
		*rvoice = voice;
		return 0;
	}
	return -ENOMEM;
}

static int snd_emu10k1x_voice_alloc(emu10k1x_t *emu, emu10k1x_voice_t **rvoice, int id)
{
  unsigned long flags;
	int result;
  
	snd_assert(rvoice != NULL, return -EINVAL);

	spin_lock_irqsave(&emu->voice_lock, flags);
  
	result = voice_alloc(emu, rvoice, id);

	spin_unlock_irqrestore(&emu->voice_lock, flags);
  
	return result;
}

static int snd_emu10k1x_voice_free(emu10k1x_t *emu, emu10k1x_voice_t *pvoice)
{
	unsigned long flags;
  
	snd_assert(pvoice != NULL, return -EINVAL);
	spin_lock_irqsave(&emu->voice_lock, flags);

	pvoice->interrupt = NULL;
	pvoice->use = 0;
	pvoice->epcm = NULL;

	spin_unlock_irqrestore(&emu->voice_lock, flags);
	return 0;
}

static void snd_emu10k1x_pcm_free_substream(snd_pcm_runtime_t *runtime)
{
	emu10k1x_pcm_t *epcm = snd_magic_cast(emu10k1x_pcm_t, runtime->private_data, return);
  
	if (epcm)
		snd_magic_kfree(epcm);
}

static void snd_emu10k1x_pcm_interrupt(emu10k1x_t *emu, emu10k1x_voice_t *voice)
{
	emu10k1x_pcm_t *epcm;

	if ((epcm = voice->epcm) == NULL)
		return;
	if (epcm->substream == NULL)
		return;
#if 0
	snd_printk("IRQ: position = 0x%x, period = 0x%x, size = 0x%x\n",
		   epcm->substream->ops->pointer(epcm->substream),
		   snd_pcm_lib_period_bytes(epcm->substream),
		   snd_pcm_lib_buffer_bytes(epcm->substream));
#endif
	snd_pcm_period_elapsed(epcm->substream);
}

/* open callback */
static int snd_emu10k1x_playback_open(snd_pcm_substream_t *substream)
{
	emu10k1x_t *chip = snd_pcm_substream_chip(substream);
	emu10k1x_pcm_t *epcm;
	snd_pcm_runtime_t *runtime = substream->runtime;

	epcm = snd_magic_kcalloc(emu10k1x_pcm_t, 0, GFP_KERNEL);
	if (epcm == NULL)
		return -ENOMEM;
	epcm->emu = chip;
	epcm->substream = substream;
  
	runtime->private_data = epcm;
	runtime->private_free = snd_emu10k1x_pcm_free_substream;
  
	runtime->hw = snd_emu10k1x_playback_hw;

	return 0;
}

/* close callback */
static int snd_emu10k1x_playback_close(snd_pcm_substream_t *substream)
{
	return 0;
}

/* hw_params callback */
static int snd_emu10k1x_pcm_hw_params(snd_pcm_substream_t *substream,
				      snd_pcm_hw_params_t * hw_params)
{
	int err;
	snd_pcm_runtime_t *runtime = substream->runtime;
	emu10k1x_pcm_t *epcm = snd_magic_cast(emu10k1x_pcm_t, runtime->private_data, return -ENXIO);

	if (! epcm->voice) {
		if ((err = snd_emu10k1x_voice_alloc(epcm->emu, &epcm->voice, substream->pcm->device)) < 0)
			return err;
		epcm->voice->use = 1;
		epcm->voice->interrupt = snd_emu10k1x_pcm_interrupt;
		epcm->voice->epcm = epcm;
		epcm->voice->interrupt = snd_emu10k1x_pcm_interrupt;
	}

	return snd_pcm_lib_malloc_pages(substream,
					params_buffer_bytes(hw_params));
}

/* hw_free callback */
static int snd_emu10k1x_pcm_hw_free(snd_pcm_substream_t *substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	emu10k1x_pcm_t *epcm;

	if (runtime->private_data == NULL)
		return 0;
	epcm = snd_magic_cast(emu10k1x_pcm_t, runtime->private_data, return -ENXIO);

	if (epcm->voice) {
		snd_emu10k1x_voice_free(epcm->emu, epcm->voice);
		epcm->voice = NULL;
	}

	return snd_pcm_lib_free_pages(substream);
}

/* prepare callback */
static int snd_emu10k1x_pcm_prepare(snd_pcm_substream_t *substream)
{
	emu10k1x_t *emu = snd_pcm_substream_chip(substream);
	snd_pcm_runtime_t *runtime = substream->runtime;
	emu10k1x_pcm_t *epcm = snd_magic_cast(emu10k1x_pcm_t, runtime->private_data, return -ENXIO);
	int voice = epcm->voice->number;

	snd_emu10k1x_ptr_write(emu, 0x00, voice, 0);
	snd_emu10k1x_ptr_write(emu, 0x01, voice, 0);
	snd_emu10k1x_ptr_write(emu, 0x02, voice, 0);
	snd_emu10k1x_ptr_write(emu, PLAYBACK_DMA_ADDR, voice, runtime->dma_addr);
	snd_emu10k1x_ptr_write(emu, PLAYBACK_BUFFER_SIZE, voice, frames_to_bytes(runtime, runtime->buffer_size)<<16); // buffer size in bytes
	snd_emu10k1x_ptr_write(emu, PLAYBACK_POINTER, voice, 0);
	snd_emu10k1x_ptr_write(emu, 0x07, voice, 0);
	snd_emu10k1x_ptr_write(emu, 0x08, voice, 0);
	unsigned int value = snd_emu10k1x_ptr_read(emu, ROUTING, 0);
	value |= 3 << (2*voice);
	if(voice == 2)
	  value |= 0x10000; // enable center/lfe channel this way
	snd_emu10k1x_ptr_write(emu, ROUTING, 0, value);

	return 0;
}

/* trigger callback */
static int snd_emu10k1x_pcm_trigger(snd_pcm_substream_t *substream,
				    int cmd)
{
	emu10k1x_t *emu = snd_pcm_substream_chip(substream);
	snd_pcm_runtime_t *runtime = substream->runtime;
	emu10k1x_pcm_t *epcm = snd_magic_cast(emu10k1x_pcm_t, runtime->private_data, return -ENXIO);
	int channel = epcm->voice->number;
	int result = 0;

//	snd_printk("trigger - emu10k1x = 0x%x, cmd = %i, pointer = %d\n", (int)emu, cmd, (int)substream->ops->pointer(substream));

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		snd_emu10k1x_ptr_write(emu, TRIGGER_CHANNEL, 0, snd_emu10k1x_ptr_read(emu, TRIGGER_CHANNEL, 0)|(TRIGGER_CHANNEL_0<<channel));
		epcm->running = 1;
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		snd_emu10k1x_ptr_write(emu, TRIGGER_CHANNEL, 0, snd_emu10k1x_ptr_read(emu, TRIGGER_CHANNEL, 0) & ~(TRIGGER_CHANNEL_0<<channel));
		epcm->running = 0;
		break;
	default:
		result = -EINVAL;
		break;
	}
	return result;
}

/* pointer callback */
static snd_pcm_uframes_t
snd_emu10k1x_pcm_pointer(snd_pcm_substream_t *substream)
{
	emu10k1x_t *emu = snd_pcm_substream_chip(substream);
	snd_pcm_runtime_t *runtime = substream->runtime;
	emu10k1x_pcm_t *epcm = snd_magic_cast(emu10k1x_pcm_t, runtime->private_data, return -ENXIO);
	unsigned int ptr = 0;
	int channel = epcm->voice->number;

	if (!epcm->running)
		return 0;

//	snd_printk("pointer: %08X\n", 
//		   snd_emu10k1x_ptr_read(emu, PLAYBACK_POINTER, channel));
	ptr = bytes_to_frames(runtime, snd_emu10k1x_ptr_read(emu, PLAYBACK_POINTER, channel));
	if (ptr >= runtime->buffer_size)
		ptr -= runtime->buffer_size;

//	snd_printk("ptr = 0x%x, buffer_size = 0x%x, period_size = 0x%x, bits=%d, rate=%d\n", ptr, (int)runtime->buffer_size, (int)runtime->period_size, (int)runtime->frame_bits, (int)runtime->rate);
	return ptr;
}

/* operators */
static snd_pcm_ops_t snd_emu10k1x_playback_ops = {
	.open =        snd_emu10k1x_playback_open,
	.close =       snd_emu10k1x_playback_close,
	.ioctl =       snd_pcm_lib_ioctl,
	.hw_params =   snd_emu10k1x_pcm_hw_params,
	.hw_free =     snd_emu10k1x_pcm_hw_free,
	.prepare =     snd_emu10k1x_pcm_prepare,
	.trigger =     snd_emu10k1x_pcm_trigger,
	.pointer =     snd_emu10k1x_pcm_pointer,
};

/* open_capture callback */
static int snd_emu10k1x_pcm_open_capture(snd_pcm_substream_t *substream)
{
	emu10k1x_t *chip = snd_pcm_substream_chip(substream);
	emu10k1x_pcm_t *epcm;
	snd_pcm_runtime_t *runtime = substream->runtime;

	epcm = snd_magic_kcalloc(emu10k1x_pcm_t, 0, GFP_KERNEL);
	if (epcm == NULL) {
		return -ENOMEM;
        }
	epcm->emu = chip;
	epcm->substream = substream;
  
	runtime->private_data = epcm;
	runtime->private_free = snd_emu10k1x_pcm_free_substream;
  
	runtime->hw = snd_emu10k1x_capture_hw;

	return 0;
}

/* close callback */
static int snd_emu10k1x_pcm_close_capture(snd_pcm_substream_t *substream)
{
	return 0;
}

/* hw_params callback */
static int snd_emu10k1x_pcm_hw_params_capture(snd_pcm_substream_t *substream,
				      snd_pcm_hw_params_t * hw_params)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	emu10k1x_pcm_t *epcm = snd_magic_cast(emu10k1x_pcm_t, runtime->private_data, return -ENXIO);

	if (! epcm->voice) {
		if (epcm->emu->capture_voice.use)
			return -1;
		epcm->voice = &epcm->emu->capture_voice;
		epcm->voice->epcm = epcm;
		epcm->voice->use = 1;
		epcm->voice->interrupt = snd_emu10k1x_pcm_interrupt;
	}

	return snd_pcm_lib_malloc_pages(substream,
					params_buffer_bytes(hw_params));
}

/* hw_free callback */
static int snd_emu10k1x_pcm_hw_free_capture(snd_pcm_substream_t *substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;

	emu10k1x_pcm_t *epcm;

	if (runtime->private_data == NULL)
		return 0;
	epcm = snd_magic_cast(emu10k1x_pcm_t, runtime->private_data, return -ENXIO);

	if (epcm->voice) {
		epcm->voice->use = 0;
		epcm->voice->interrupt = NULL;
		epcm->voice->epcm = NULL;
		epcm->voice = NULL;
	}

	return snd_pcm_lib_free_pages(substream);
}

/* prepare capture callback */
static int snd_emu10k1x_pcm_prepare_capture(snd_pcm_substream_t *substream)
{
	emu10k1x_t *emu = snd_pcm_substream_chip(substream);
	snd_pcm_runtime_t *runtime = substream->runtime;
	//	snd_printk("prepare: rate=%d, format=0x%x, channels=%d, buffer_size=%ld, period_size=%ld, frames_to_bytes=%d\n",runtime->rate, runtime->format, runtime->channels, runtime->buffer_size, runtime->period_size,  frames_to_bytes(runtime, 1));
	snd_emu10k1x_ptr_write(emu, CAPTURE_DMA_ADDR, 0, runtime->dma_addr);
	snd_emu10k1x_ptr_write(emu, CAPTURE_BUFFER_SIZE, 0, frames_to_bytes(runtime, runtime->buffer_size)<<16); // buffer size in bytes
	snd_emu10k1x_ptr_write(emu, CAPTURE_POINTER, 0, 0);

	return 0;
}

/* trigger_capture callback */
static int snd_emu10k1x_pcm_trigger_capture(snd_pcm_substream_t *substream,
				    int cmd)
{
	emu10k1x_t *emu = snd_pcm_substream_chip(substream);
	snd_pcm_runtime_t *runtime = substream->runtime;
	emu10k1x_pcm_t *epcm = snd_magic_cast(emu10k1x_pcm_t, runtime->private_data, return -ENXIO);
	int result = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		snd_emu10k1x_ptr_write(emu, TRIGGER_CHANNEL, 0, snd_emu10k1x_ptr_read(emu, TRIGGER_CHANNEL, 0)|TRIGGER_CAPTURE);
		epcm->running = 1;
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		snd_emu10k1x_ptr_write(emu, TRIGGER_CHANNEL, 0, snd_emu10k1x_ptr_read(emu, TRIGGER_CHANNEL, 0) & ~(TRIGGER_CAPTURE));
		epcm->running = 0;
		break;
	default:
		result = -EINVAL;
		break;
	}
	return result;
}

/* pointer_capture callback */
static snd_pcm_uframes_t
snd_emu10k1x_pcm_pointer_capture(snd_pcm_substream_t *substream)
{
	emu10k1x_t *emu = snd_pcm_substream_chip(substream);
	snd_pcm_runtime_t *runtime = substream->runtime;
	emu10k1x_pcm_t *epcm = snd_magic_cast(emu10k1x_pcm_t, runtime->private_data, return -ENXIO);
	snd_pcm_uframes_t ptr;

	if (!epcm->running)
		return 0;

	ptr = bytes_to_frames(runtime, snd_emu10k1x_ptr_read(emu, CAPTURE_POINTER, 0));
	if (ptr >= runtime->buffer_size)
		ptr -= runtime->buffer_size;

	return ptr;
}

static snd_pcm_ops_t snd_emu10k1x_capture_ops = {
	.open =        snd_emu10k1x_pcm_open_capture,
	.close =       snd_emu10k1x_pcm_close_capture,
	.ioctl =       snd_pcm_lib_ioctl,
	.hw_params =   snd_emu10k1x_pcm_hw_params_capture,
	.hw_free =     snd_emu10k1x_pcm_hw_free_capture,
	.prepare =     snd_emu10k1x_pcm_prepare_capture,
	.trigger =     snd_emu10k1x_pcm_trigger_capture,
	.pointer =     snd_emu10k1x_pcm_pointer_capture,
};

static unsigned short snd_emu10k1x_ac97_read(ac97_t *ac97,
					     unsigned short reg)
{
	emu10k1x_t *emu = snd_magic_cast(emu10k1x_t, ac97->private_data, return -ENXIO);
	unsigned long flags;
	unsigned short val;
  
	spin_lock_irqsave(&emu->emu_lock, flags);
	outb(reg, emu->port + AC97ADDRESS);
	val = inw(emu->port + AC97DATA);
	spin_unlock_irqrestore(&emu->emu_lock, flags);
	return val;
}

static void snd_emu10k1x_ac97_write(ac97_t *ac97,
				    unsigned short reg, unsigned short val)
{
	emu10k1x_t *emu = snd_magic_cast(emu10k1x_t, ac97->private_data, return);
	unsigned long flags;
  
	spin_lock_irqsave(&emu->emu_lock, flags);
	outb(reg, emu->port + AC97ADDRESS);
	outw(val, emu->port + AC97DATA);
	spin_unlock_irqrestore(&emu->emu_lock, flags);
}

static int snd_emu10k1x_ac97(emu10k1x_t *chip)
{
	ac97_bus_t bus, *pbus;
	ac97_t ac97;
	int err;
  
	memset(&bus, 0, sizeof(bus));
	bus.write = snd_emu10k1x_ac97_write;
	bus.read = snd_emu10k1x_ac97_read;
	if ((err = snd_ac97_bus(chip->card, &bus, &pbus)) < 0)
		return err;
	memset(&ac97, 0, sizeof(ac97));
	ac97.private_data = chip;
	return snd_ac97_mixer(pbus, &ac97, &chip->ac97);
}

static int snd_emu10k1x_free(emu10k1x_t *chip)
{
	snd_emu10k1x_ptr_write(chip, TRIGGER_CHANNEL, 0, 0);
	// disable interrupts
	outl(0, chip->port + INTE);
	// disable audio
	outl(HCFG_LOCKSOUNDCACHE, chip->port + HCFG);

	// release the i/o port
	if (chip->res_port) {
		release_resource(chip->res_port);
		kfree_nocheck(chip->res_port);
	}
	// release the irq
	if (chip->irq >= 0)
		free_irq(chip->irq, (void *)chip);
	// release the data
	snd_magic_kfree(chip);
	return 0;
}

static int snd_emu10k1x_dev_free(snd_device_t *device)
{
	emu10k1x_t *chip = snd_magic_cast(emu10k1x_t,
					  device->device_data, return -ENXIO);
	return snd_emu10k1x_free(chip);
}

static irqreturn_t snd_emu10k1x_interrupt(int irq, void *dev_id,
					  struct pt_regs *regs)
{
	unsigned int status;

	emu10k1x_t *chip = snd_magic_cast(emu10k1x_t, dev_id, return IRQ_NONE);
	int i;
	int mask;

	spin_lock(&chip->emu_lock);

	status = inl(chip->port + IPR);

	// call updater, unlock before it
	spin_unlock(&chip->emu_lock);
  
	if (! status)
		return IRQ_NONE;

	mask = IPR_CH_0_LOOP|IPR_CH_0_HALF_LOOP;
	emu10k1x_voice_t *pvoice = chip->voices;
	for(i = 0; i < 3; i++) {
		if(status & mask) {
			if(pvoice->use && pvoice->interrupt)
				pvoice->interrupt(chip, pvoice);
		}
		pvoice++;
		mask <<= 1;
	}
	// capture interrupt
	if(status & (IPR_CAP_0_LOOP | IPR_CAP_0_HALF_LOOP)) {
		emu10k1x_voice_t *pvoice = &chip->capture_voice;
		if(pvoice->use && pvoice->interrupt)
			pvoice->interrupt(chip, pvoice);
	}

	spin_lock(&chip->emu_lock);
	// acknowledge the interrupt if necessary
	outl(status, chip->port+IPR);

	spin_unlock(&chip->emu_lock);

//	snd_printk("interrupt %08x\n", status);

	return IRQ_HANDLED;
}

static void snd_emu10k1x_pcm_free(snd_pcm_t *pcm)
{
	emu10k1x_t *emu = snd_magic_cast(emu10k1x_t, pcm->private_data, return);
	emu->pcm = NULL;
	snd_pcm_lib_preallocate_free_for_all(pcm);
}

static int __devinit snd_emu10k1x_pcm(emu10k1x_t *emu, int device, snd_pcm_t **rpcm)
{
	snd_pcm_t *pcm;
	snd_pcm_substream_t *substream;
	int err;
	int capture = 0;
  
	if (rpcm)
		*rpcm = NULL;
	if (device == 0)
	  capture = 1;
	
	if ((err = snd_pcm_new(emu->card, "emu10k1x", device, 1, capture, &pcm)) < 0)
		return err;
  
	pcm->private_data = emu;
	pcm->private_free = snd_emu10k1x_pcm_free;
	
	switch(device) {
	case 0:
	  snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_emu10k1x_playback_ops);
	  snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &snd_emu10k1x_capture_ops);
	  break;
	case 1:
	case 2:
	  snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_emu10k1x_playback_ops);
	  break;
	}

	pcm->info_flags = 0;
	pcm->dev_subclass = SNDRV_PCM_SUBCLASS_GENERIC_MIX;
	switch(device) {
	case 0:
	  strcpy(pcm->name, "EMU10K1X Front");
	  break;
	case 1:
	  strcpy(pcm->name, "EMU10K1X Rear");
	  break;
	case 2:
	  strcpy(pcm->name, "EMU10K1X Center/LFE");
	  break;
	}
	emu->pcm = pcm;

	for(substream = pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream; 
	    substream; 
	    substream = substream->next)
		if ((err = snd_pcm_lib_preallocate_pages(substream, 
							 SNDRV_DMA_TYPE_DEV, 
							 snd_dma_pci_data(emu->pci), 
							 32*1024, 32*1024)) < 0)
			return err;

	for (substream = pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream; 
	     substream; 
	     substream = substream->next)
		snd_pcm_lib_preallocate_pages(substream, 
					      SNDRV_DMA_TYPE_DEV, 
					      snd_dma_pci_data(emu->pci), 
					      32*1024, 32*1024);
  
	if (rpcm)
		*rpcm = pcm;
  
	return 0;
}

static int __devinit snd_emu10k1x_create(snd_card_t *card,
					 struct pci_dev *pci,
					 emu10k1x_t **rchip)
{
	emu10k1x_t *chip;
	int err;
	int ch;
	static snd_device_ops_t ops = {
		.dev_free = snd_emu10k1x_dev_free,
	};
  
	*rchip = NULL;
  
	if ((err = pci_enable_device(pci)) < 0)
		return err;
	if (pci_set_dma_mask(pci, 0x0fffffff) < 0 ||
	    pci_set_consistent_dma_mask(pci, 0x0fffffff) < 0) {
		snd_printk(KERN_ERR "error to set 28bit mask DMA\n");
		return -ENXIO;
	}
  
	chip = snd_magic_kcalloc(emu10k1x_t, 0, GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;
  
	chip->card = card;
	chip->pci = pci;
	chip->irq = -1;

	spin_lock_init(&chip->emu_lock);
	spin_lock_init(&chip->voice_lock);
  
	chip->port = pci_resource_start(pci, 0);
	if ((chip->res_port = request_region(chip->port, 8,
					     "My Chip")) == NULL) { 
		snd_emu10k1x_free(chip);
		snd_printk("cannot allocate the port\n");
		return -EBUSY;
	}

	if (request_irq(pci->irq, snd_emu10k1x_interrupt,
			SA_INTERRUPT|SA_SHIRQ, "EMU10K1X",
			(void *)chip)) {
		snd_emu10k1x_free(chip);
		snd_printk("cannot grab irq\n");
		return -EBUSY;
	}
	chip->irq = pci->irq;
  
	memset(&chip->dma_dev, 0, sizeof(chip->dma_dev));
	chip->dma_dev.type = SNDRV_DMA_TYPE_DEV;
	chip->dma_dev.dev = snd_dma_pci_data(pci);
  
	if(snd_dma_alloc_pages(&chip->dma_dev, 32 * 1024, &chip->buffer) < 0) {
		snd_emu10k1x_free(chip);
		return -ENOMEM;
	}

	pci_set_master(pci);
	/* read revision & serial */
	pci_read_config_byte(pci, PCI_REVISION_ID, (char *)&chip->revision);
	pci_read_config_dword(pci, PCI_SUBSYSTEM_VENDOR_ID, &chip->serial);
	pci_read_config_word(pci, PCI_SUBSYSTEM_ID, &chip->model);
	snd_printk("Model %04x Rev %08x Serial %08x\n", chip->model,
		   chip->revision, chip->serial);

	outl(0, chip->port + INTE);	

	for(ch = 0; ch < 3; ch++) {
		chip->voices[ch].emu = chip;
		chip->voices[ch].number = ch;
	}

	/*
	 *  Init to 0x02109204 :
	 *  Clock accuracy    = 0     (1000ppm)
	 *  Sample Rate       = 2     (48kHz)
	 *  Audio Channel     = 1     (Left of 2)
	 *  Source Number     = 0     (Unspecified)
	 *  Generation Status = 1     (Original for Cat Code 12)
	 *  Cat Code          = 12    (Digital Signal Mixer)
	 *  Mode              = 0     (Mode 0)
	 *  Emphasis          = 0     (None)
	 *  CP                = 1     (Copyright unasserted)
	 *  AN                = 0     (Audio data)
	 *  P                 = 0     (Consumer)
	 */
	snd_emu10k1x_ptr_write(chip, SPCS0, 0,
			       chip->spdif_bits[0] = 
			       SPCS_CLKACCY_1000PPM | SPCS_SAMPLERATE_48 |
			       SPCS_CHANNELNUM_LEFT | SPCS_SOURCENUM_UNSPEC |
			       SPCS_GENERATIONSTATUS | 0x00001200 |
			       0x00000000 | SPCS_EMPHASIS_NONE | SPCS_COPYRIGHT);
	snd_emu10k1x_ptr_write(chip, SPCS1, 0,
			       chip->spdif_bits[1] = 
			       SPCS_CLKACCY_1000PPM | SPCS_SAMPLERATE_48 |
			       SPCS_CHANNELNUM_LEFT | SPCS_SOURCENUM_UNSPEC |
			       SPCS_GENERATIONSTATUS | 0x00001200 |
			       0x00000000 | SPCS_EMPHASIS_NONE | SPCS_COPYRIGHT);
	snd_emu10k1x_ptr_write(chip, SPCS2, 0,
			       chip->spdif_bits[2] = 
			       SPCS_CLKACCY_1000PPM | SPCS_SAMPLERATE_48 |
			       SPCS_CHANNELNUM_LEFT | SPCS_SOURCENUM_UNSPEC |
			       SPCS_GENERATIONSTATUS | 0x00001200 |
			       0x00000000 | SPCS_EMPHASIS_NONE | SPCS_COPYRIGHT);

	snd_emu10k1x_ptr_write(chip, SPDIF_SELECT, 0, 0x700); // disable SPDIF
	// verify the next line
	snd_emu10k1x_ptr_write(chip, ROUTING, 0, 0x10000); // routing

	outl(HCFG_LOCKSOUNDCACHE|HCFG_AUDIOENABLE, chip->port+HCFG);

	snd_emu10k1x_intr_enable(chip, (INTE_CH_0_LOOP|INTE_CH_0_HALF_LOOP) |
				 ((INTE_CH_0_LOOP|INTE_CH_0_HALF_LOOP)<<1) |
				 ((INTE_CH_0_LOOP|INTE_CH_0_HALF_LOOP)<<2) |
				 (INTE_CAP_0_LOOP|INTE_CAP_0_HALF_LOOP));

	if ((err = snd_device_new(card, SNDRV_DEV_LOWLEVEL,
				  chip, &ops)) < 0) {
		snd_emu10k1x_free(chip);
		return err;
	}
	*rchip = chip;
	return 0;
}

static void snd_emu10k1x_proc_reg_read(snd_info_entry_t *entry, 
				       snd_info_buffer_t * buffer)
{
	emu10k1x_t *emu = snd_magic_cast(emu10k1x_t, entry->private_data, return);
	unsigned long value,value1,value2;
	unsigned long flags;
	int i;

	snd_iprintf(buffer, "Registers:\n\n");
	for(i = 0; i < 0x20; i+=4) {
		spin_lock_irqsave(&emu->emu_lock, flags);
		value = inl(emu->port + i);
		spin_unlock_irqrestore(&emu->emu_lock, flags);
		snd_iprintf(buffer, "Register %02X: %08lX\n", i, value);
	}
	snd_iprintf(buffer, "\nRegisters\n\n");
	for(i = 0; i <= 0x48; i++) {
		value = snd_emu10k1x_ptr_read(emu, i, 0);
		if(i < 0x10 || (i >= 0x20 && i < 0x40)) {
			value1 = snd_emu10k1x_ptr_read(emu, i, 1);
			value2 = snd_emu10k1x_ptr_read(emu, i, 2);
			snd_iprintf(buffer, "%02X: %08lX %08lX %08lX\n", i, value, value1, value2);
		} else {
			snd_iprintf(buffer, "%02X: %08lX\n", i, value);
		}
	}
}

static void snd_emu10k1x_proc_reg_write(snd_info_entry_t *entry, 
					snd_info_buffer_t *buffer)
{
	emu10k1x_t *emu = snd_magic_cast(emu10k1x_t, entry->private_data, 
					 return);
        char line[64];
        unsigned int reg, channel_id , val;
        while (!snd_info_get_line(buffer, line, sizeof(line))) {
                if (sscanf(line, "%x %x %x", &reg, &channel_id, &val) != 3)
                        continue;
		
                if ((reg < 0x49) && (reg >=0) && (val <= 0xffffffff) 
		    && (channel_id >=0) && (channel_id <= 2) )
			snd_emu10k1x_ptr_write(emu, reg, channel_id, val);
        }
}

static int __devinit snd_emu10k1x_proc_init(emu10k1x_t * emu)
{
	snd_info_entry_t *entry;
	
	if(! snd_card_proc_new(emu->card, "emu10k1x_regs", &entry)) {
		snd_info_set_text_ops(entry, emu, 1024, snd_emu10k1x_proc_reg_read);
		entry->c.text.write_size = 64;
		entry->c.text.write = snd_emu10k1x_proc_reg_write;
		entry->private_data = emu;
	}
	
	return 0;
}

static int snd_emu10k1x_shared_spdif_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t * uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

static int snd_emu10k1x_shared_spdif_get(snd_kcontrol_t * kcontrol,
					snd_ctl_elem_value_t * ucontrol)
{
	emu10k1x_t *emu = snd_kcontrol_chip(kcontrol);

	ucontrol->value.integer.value[0] = (snd_emu10k1x_ptr_read(emu, SPDIF_SELECT, 0) == 0x700) ? 0 : 1;

        return 0;
}

static int snd_emu10k1x_shared_spdif_put(snd_kcontrol_t * kcontrol,
					snd_ctl_elem_value_t * ucontrol)
{
	emu10k1x_t *emu = snd_kcontrol_chip(kcontrol);
	unsigned int val;
	int change = 0;

	val = ucontrol->value.integer.value[0] ;

	if (val) {
		// enable spdif output
		snd_emu10k1x_ptr_write(emu, SPDIF_SELECT, 0, 0x000);
	} else {
		// disable spdif output
		snd_emu10k1x_ptr_write(emu, SPDIF_SELECT, 0, 0x700);
	}
        return change;
}

static snd_kcontrol_new_t snd_emu10k1x_shared_spdif __devinitdata =
{
	.iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
	.name =		"Analog/Digital Output Jack",
	.info =		snd_emu10k1x_shared_spdif_info,
	.get =		snd_emu10k1x_shared_spdif_get,
	.put =		snd_emu10k1x_shared_spdif_put
};

static int snd_emu10k1x_spdif_info(snd_kcontrol_t *kcontrol, snd_ctl_elem_info_t * uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_IEC958;
	uinfo->count = 1;
	return 0;
}

static int snd_emu10k1x_spdif_get(snd_kcontrol_t * kcontrol,
                                 snd_ctl_elem_value_t * ucontrol)
{
	emu10k1x_t *emu = snd_kcontrol_chip(kcontrol);
	unsigned int idx = snd_ctl_get_ioffidx(kcontrol, &ucontrol->id);

	ucontrol->value.iec958.status[0] = (emu->spdif_bits[idx] >> 0) & 0xff;
	ucontrol->value.iec958.status[1] = (emu->spdif_bits[idx] >> 8) & 0xff;
	ucontrol->value.iec958.status[2] = (emu->spdif_bits[idx] >> 16) & 0xff;
	ucontrol->value.iec958.status[3] = (emu->spdif_bits[idx] >> 24) & 0xff;
        return 0;
}

static int snd_emu10k1x_spdif_get_mask(snd_kcontrol_t * kcontrol,
				      snd_ctl_elem_value_t * ucontrol)
{
	ucontrol->value.iec958.status[0] = 0xff;
	ucontrol->value.iec958.status[1] = 0xff;
	ucontrol->value.iec958.status[2] = 0xff;
	ucontrol->value.iec958.status[3] = 0xff;
        return 0;
}

static int snd_emu10k1x_spdif_put(snd_kcontrol_t * kcontrol,
                                 snd_ctl_elem_value_t * ucontrol)
{
	emu10k1x_t *emu = snd_kcontrol_chip(kcontrol);
	unsigned int idx = snd_ctl_get_ioffidx(kcontrol, &ucontrol->id);
	int change;
	unsigned int val;

	val = (ucontrol->value.iec958.status[0] << 0) |
	      (ucontrol->value.iec958.status[1] << 8) |
	      (ucontrol->value.iec958.status[2] << 16) |
	      (ucontrol->value.iec958.status[3] << 24);
	change = val != emu->spdif_bits[idx];
	if (change) {
		snd_emu10k1x_ptr_write(emu, SPCS0 + idx, 0, val);
		emu->spdif_bits[idx] = val;
	}
        return change;
}

static snd_kcontrol_new_t snd_emu10k1x_spdif_mask_control =
{
	.access =	SNDRV_CTL_ELEM_ACCESS_READ,
        .iface =        SNDRV_CTL_ELEM_IFACE_MIXER,
        .name =         SNDRV_CTL_NAME_IEC958("",PLAYBACK,MASK),
	.count =	3,
        .info =         snd_emu10k1x_spdif_info,
        .get =          snd_emu10k1x_spdif_get_mask
};

static snd_kcontrol_new_t snd_emu10k1x_spdif_control =
{
        .iface =	SNDRV_CTL_ELEM_IFACE_MIXER,
        .name =         SNDRV_CTL_NAME_IEC958("",PLAYBACK,DEFAULT),
	.count =	3,
        .info =         snd_emu10k1x_spdif_info,
        .get =          snd_emu10k1x_spdif_get,
        .put =          snd_emu10k1x_spdif_put
};

static int __devinit snd_emu10k1x_mixer(emu10k1x_t *emu)
{
        int err;
        snd_kcontrol_t *kctl;
        snd_card_t *card = emu->card;

	if ((kctl = snd_ctl_new1(&snd_emu10k1x_spdif_mask_control, emu)) == NULL)
		return -ENOMEM;
	if ((err = snd_ctl_add(card, kctl)))
		return err;
	if ((kctl = snd_ctl_new1(&snd_emu10k1x_shared_spdif, emu)) == NULL)
		return -ENOMEM;
	if ((err = snd_ctl_add(card, kctl)))
		return err;
	if ((kctl = snd_ctl_new1(&snd_emu10k1x_spdif_control, emu)) == NULL)
		return -ENOMEM;
	if ((err = snd_ctl_add(card, kctl)))
		return err;

        return 0;
}

static int __devinit snd_emu10k1x_probe(struct pci_dev *pci,
					const struct pci_device_id *pci_id)
{
	static int dev;
	snd_card_t *card;
	emu10k1x_t *chip;
	int err;

	if (dev >= SNDRV_CARDS)
		return -ENODEV;
	if (!enable[dev]) {
		dev++;
		return -ENOENT;
	}

	card = snd_card_new(index[dev], id[dev], THIS_MODULE, 0);
	if (card == NULL)
		return -ENOMEM;

	if ((err = snd_emu10k1x_create(card, pci, &chip)) < 0) {
		snd_card_free(card);
		return err;
	}

	if ((err = snd_emu10k1x_pcm(chip, 0, NULL)) < 0) {
		snd_card_free(card);
		return err;
	}
	if ((err = snd_emu10k1x_pcm(chip, 1, NULL)) < 0) {
		snd_card_free(card);
		return err;
	}
	if ((err = snd_emu10k1x_pcm(chip, 2, NULL)) < 0) {
		snd_card_free(card);
		return err;
	}

	if ((err = snd_emu10k1x_ac97(chip)) < 0) {
		snd_card_free(card);
		return err;
	}

	if ((err = snd_emu10k1x_mixer(chip)) < 0) {
		snd_card_free(card);
		return err;
	}

	snd_emu10k1x_proc_init(chip);

	strcpy(card->driver, "EMU10K1X");
	strcpy(card->shortname, "Dell Sound Blaster Live!");
	sprintf(card->longname, "%s at 0x%lx irq %i",
		card->shortname, chip->port, chip->irq);

	if ((err = snd_card_register(card)) < 0) {
		snd_card_free(card);
		return err;
	}

	pci_set_drvdata(pci, card);
	dev++;
	return 0;
}

static void __devexit snd_emu10k1x_remove(struct pci_dev *pci)
{
	snd_card_free(pci_get_drvdata(pci));
	pci_set_drvdata(pci, NULL);
}

// PCI IDs
static struct pci_device_id snd_emu10k1x_ids[] = {
	{ 0x1102, 0x0006, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },	/* Dell OEM version (EMU10K1) */
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, snd_emu10k1x_ids);

// pci_driver definition
static struct pci_driver driver = {
	.name = "EMU10K1X",
	.id_table = snd_emu10k1x_ids,
	.probe = snd_emu10k1x_probe,
	.remove = __devexit_p(snd_emu10k1x_remove),
};

// initialization of the module
static int __init alsa_card_emu10k1x_init(void)
{
	int err;

	if ((err = pci_module_init(&driver)) > 0)
		return err;

	return 0;
}

// clean up the module
static void __exit alsa_card_emu10k1x_exit(void)
{
	pci_unregister_driver(&driver);
}

module_init(alsa_card_emu10k1x_init)
module_exit(alsa_card_emu10k1x_exit)
     
EXPORT_NO_SYMBOLS; /* for old kernels only */
