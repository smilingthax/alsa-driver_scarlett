/*
 *  cmi8788.c - Driver for C-Media CMI8788 PCI soundcards.
 *
 *      Copyright (C) 2005  C-media support
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
 *  Revision history
 *
 *    Weifeng Sui <weifengsui@163.com>
 */

#include <sound/driver.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/control.h>
#include "cmi8788.h"


static struct snd_pcm_hardware snd_cmi_pcm_playback_hw = {
	.info =	SNDRV_PCM_INFO_MMAP |
		SNDRV_PCM_INFO_INTERLEAVED |
		SNDRV_PCM_INFO_BLOCK_TRANSFER |
		SNDRV_PCM_INFO_MMAP_VALID |
		SNDRV_PCM_INFO_PAUSE,
	.formats = SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE,
	.rates = SNDRV_PCM_RATE_5512 | SNDRV_PCM_RATE_8000_48000,
	.rate_min = 5512,
	.rate_max = 48000,
	.channels_min = 2,
	.channels_max = 8,
	.buffer_bytes_max = 128 * 1024,
	.period_bytes_min = 128,
	.period_bytes_max = 128 * 1024,
	.periods_min = 2,
	.periods_max = 1024,
};

static struct snd_pcm_hardware snd_cmi_pcm_capture_hw = {
	.info =	SNDRV_PCM_INFO_MMAP |
		SNDRV_PCM_INFO_INTERLEAVED |
		SNDRV_PCM_INFO_BLOCK_TRANSFER |
		SNDRV_PCM_INFO_MMAP_VALID  |
		SNDRV_PCM_INFO_PAUSE,
	.formats =     SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE,
	.rates = SNDRV_PCM_RATE_5512 | SNDRV_PCM_RATE_8000_48000,
	.rate_min = 5512,
	.rate_max = 48000,
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = 128 * 1024,
	.period_bytes_min = 128,
	.period_bytes_max = 128 * 1024,
	.periods_min = 2,
	.periods_max = 1024,
};

/*
 * int cmi_pcm_no :
 *   NORMAL_PCMS 0
 *   AC97_PCMS   1
 *   SPDIF_PCMS  2
 * int subs_no : CMI_PLAYBACK 0; CMI_CAPTURE 1
 *
 */
static int cmi_pcm_open(struct snd_pcm_substream *substream, int cmi_pcm_no, int subs_no)
{
	struct cmi8788 *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct cmi_substream *cmi_subs = &chip->cmi_pcm[cmi_pcm_no].cmi_subs[subs_no];

	cmi_subs->substream = substream;

	switch(cmi_pcm_no) {
	case NORMAL_PCMS:
		switch(subs_no) {
		case CMI_PLAYBACK:
			chip->playback_volume_init = 1;
			cmi_subs->DMA_sta_mask   = 0x0010; /* PCI 40: PCI DMA Channel Start/Pause/Stop 2Byte -- 1 start*/
			cmi_subs->DMA_chan_reset = 0x0010; /* PCI 42: PCI DMA Channel Reset 1Byte --1 Reset*/
			cmi_subs->int_mask       = 0x0010; /* PCI 44: Interrupt Mask Register  2Byte -- 1 is enable*/
			cmi_subs->int_sta_mask   = 0x0010; /* PCI 46: interrupt status mask    2Byte-- Bit 4  Interrupt for Multi-Channel Playback DMA is pending*/
			runtime->hw = snd_cmi_pcm_playback_hw;
			break;
		case CMI_CAPTURE:
			chip->capture_volume_init = 1;
			cmi_subs->DMA_sta_mask   = 0x0001; /* PCI 40: PCI DMA Channel Start/Pause/Stop 2Byte -- 1 start*/
			cmi_subs->DMA_chan_reset = 0x0001; /* PCI 42: PCI DMA Channel Reset 1Byte --1 Reset*/
			cmi_subs->int_mask       = 0x0001; /* PCI 44: Interrupt Mask Register  2Byte -- 1 is enable*/
			cmi_subs->int_sta_mask   = 0x0001; /* PCI 46: interrupt status mask 2Byte -- Bit 0 Interrupt for Recording Channel A DMA is pending*/
			runtime->hw = snd_cmi_pcm_capture_hw;
			break;
		}
		break;
	case AC97_PCMS:
		switch(subs_no) {
		case CMI_PLAYBACK:
			cmi_subs->DMA_sta_mask   = 0x0020; /* PCI 40: PCI DMA Channel Start/Pause/Stop 2Byte -- 1 start*/
			cmi_subs->DMA_chan_reset = 0x0020; /* PCI 42: PCI DMA Channel Reset 1Byte --1 Reset*/
			cmi_subs->int_mask       = 0x4020; /* PCI 44: Interrupt Mask Register  2Byte -- 1 is enable*/
			cmi_subs->int_sta_mask   = 0x4020; /* PCI 46: interrupt status mask    2Byte-- Bit 4  Interrupt for Multi-Channel Playback DMA is pending*/
			runtime->hw = snd_cmi_pcm_playback_hw;
			break;
		}
		break;
	case SPDIF_PCMS:
		break;
	}
	snd_pcm_hw_constraint_minmax(runtime, SNDRV_PCM_HW_PARAM_BUFFER_SIZE, 0, 0x10000);

	return 0;
}

static int cmi_pcm_close(struct snd_pcm_substream *substream, int cmi_pcm_no, int subs_no)
{
	struct cmi8788 *chip = snd_pcm_substream_chip(substream);
	struct cmi_substream *cmi_subs = &chip->cmi_pcm[cmi_pcm_no].cmi_subs[subs_no];

	cmi_subs->substream    = NULL;

	cmi_subs->DMA_sta_mask   = 0x0000;
	cmi_subs->DMA_chan_reset = 0x00;
	cmi_subs->int_mask       = 0x0000;
	cmi_subs->int_sta_mask   = 0x0000;

	return 0;
}

static int snd_cmi_pcm_playback_open(struct snd_pcm_substream *substream)
{
	cmi_pcm_open(substream, NORMAL_PCMS, CMI_PLAYBACK);
	return 0;
}

static int snd_cmi_pcm_capture_open(struct snd_pcm_substream *substream)
{
	cmi_pcm_open(substream, NORMAL_PCMS, CMI_CAPTURE);
	return 0;
}

static int snd_cmi_pcm_ac97_playback_open(struct snd_pcm_substream *substream)
{
	cmi_pcm_open(substream, AC97_PCMS, CMI_PLAYBACK);
	return 0;
}

static int snd_cmi_pcm_playback_close(struct snd_pcm_substream *substream)
{
	cmi_pcm_close(substream, NORMAL_PCMS, CMI_PLAYBACK);
	return 0;
}

static int snd_cmi_pcm_capture_close(struct snd_pcm_substream *substream)
{
	cmi_pcm_close(substream, NORMAL_PCMS, CMI_CAPTURE);
	return 0;
}

static int snd_cmi_pcm_ac97_playback_close(struct snd_pcm_substream *substream)
{
	cmi_pcm_close(substream, AC97_PCMS, CMI_PLAYBACK);
	return 0;
}

static int snd_cmi_pcm_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *hw_params)
{
	return snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
}

static int snd_cmi_pcm_hw_free(struct snd_pcm_substream *substream)
{
	return snd_pcm_lib_free_pages(substream);
}

static int snd_cmi_pcm_playback_prepare(struct snd_pcm_substream *substream)
{
	struct cmi8788 *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	u32 period_bytes;
	u16 play_routing;
	u16 I2SFmt;
	u8 fmt;
	u8 PlyDmaMode;

	period_bytes = snd_pcm_lib_period_bytes(substream);

	/* Digital Routing and Monitoring Registers */
	play_routing = snd_cmipci_read_w(chip, Mixer_PlayRouting);
	snd_cmipci_write_w(chip, play_routing & ~0x10, Mixer_PlayRouting);

	/* set DMA Multi-Channel Playback DMA buffer addr length and fragsize */
	snd_cmipci_write(chip, (u32)runtime->dma_addr, PCI_DMAPlay_MULTI_BaseAddr);
	snd_cmipci_write(chip, runtime->dma_bytes / 4 - 1, PCI_DMAPlay_MULTI_BaseCount); /* d-word units */
	/* snd_cmipci_write(chip, period_bytes / 4 - 1 , PCI_DMAPlay_MUTLI_BaseTCount);// d-word units */
	snd_cmipci_write(chip, runtime->dma_bytes / 8 - 1 , PCI_DMAPlay_MUTLI_BaseTCount);/* d-word units */

	/* Sample Format Convert for Playback Channels */
	/* I2S Multi-Channel DAC Format Register */
	fmt = snd_cmipci_read_b(chip, PCI_PlaySampleFmCvt) & ~0x0c;
	I2SFmt = snd_cmipci_read_w(chip, I2S_Multi_DAC_Fmt) & ~0x00c0;

	switch (runtime->sample_bits) {
	case 16:
		fmt |= 0x00; /* Bit 3:2 00 */
		I2SFmt |= 0x0000; /* Bit 7:6  00 */
		break;
	case 20:
		I2SFmt |= 0x0040; /* Bit 7:6  01 */
		break;
	case 24:
		fmt |= 0xf4; /* Bit 3:2 01 */
		I2SFmt |= 0x0080; /* Bit 7:6  10 */
		break;
	case 32:
		fmt |= 0xf8; /* Bit 3:2 10 */
		I2SFmt |= 0x00c0; /* Bit 7:6  11 */
		break;
	}
	snd_cmipci_write_b(chip, fmt, PCI_PlaySampleFmCvt);
	snd_cmipci_write_w(chip, I2SFmt, I2S_Multi_DAC_Fmt);

	/* set I2S sample rate */

	/* Multi-Channel DMA Mode */
	PlyDmaMode = snd_cmipci_read_b(chip, PCI_MULTI_DMA_MODE) & ~0x03;
	switch (runtime->channels) {
	case 2:
		/* Bit 1:0  00 */
		PlyDmaMode |= 0x00;
		break;
	case 4:
		/* Bit 1:0  01 */
		PlyDmaMode |= 0x01;
		break;
	case 6:
		/* Bit 1:0  10 */
		PlyDmaMode |= 0x02;
		break;
	case 8:
		/* Bit 1:0  11 */
		PlyDmaMode |= 0x03;
		break;
	default:
		break;
	}
	snd_cmipci_write_b(chip, PlyDmaMode, PCI_MULTI_DMA_MODE);

	return 0;
}


static int snd_cmi_pcm_capture_prepare(struct snd_pcm_substream *substream)
{
	struct cmi8788 *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	u32 period_bytes;
	u8 fmt;
	u16 I2SFmt;
	u8 RecDmaMode;

	period_bytes = snd_pcm_lib_period_bytes(substream);

	/* set DMA Recording Channel A DMA buffer addr length and fragsize */
	snd_cmipci_write(chip, (u32)runtime->dma_addr, PCI_DMARec_A_BaseAddr);
	snd_cmipci_write_w(chip, runtime->dma_bytes / 4 - 1, PCI_DMARec_A_BaseCount); /* d-word units */
	snd_cmipci_write_w(chip, period_bytes / 4 - 1, PCI_DMARec_A_BaseTCount); /* d-word units old */
	/* snd_cmipci_write_w(chip, runtime->dma_bytes / 8 - 1, PCI_DMARec_A_BaseTCount); // d-word units */

	/* Sample Format Convert for Recording Channels */
	fmt = snd_cmipci_read_b(chip, PCI_RecSampleFmtCvt) & ~0x03;
	/* I2S ADC 1 Format Register */
	I2SFmt = snd_cmipci_read_w(chip, I2S_ADC1_Fmt) & ~0x00c0;

	switch (runtime->sample_bits) {
	case 16:
		fmt |= 0x00; /* Bit 1:0 00 */
		I2SFmt |= 0x0000; /* Bit 7:6 00 */
		break;
	case 20:
		I2SFmt |= 0x0040; /* Bit 7:6 01 */
		break;
	case 24:
		fmt |= 0x01; /* Bit 1:0 01 */
		I2SFmt |= 0x0080; /* Bit 7:6 10 */
		break;
	case 32:
		fmt |= 0x02; /* Bit 1:0 10 */
		I2SFmt |= 0x00c0; /* Bit 7:6 11 */
		break;
	}
	snd_cmipci_write_b(chip, fmt, PCI_RecSampleFmtCvt);
	snd_cmipci_write_w(chip, I2SFmt, I2S_ADC1_Fmt);

	/* set I2S sample rate */

	RecDmaMode = snd_cmipci_read_b(chip, PCI_RecDMA_Mode) & ~0x07;
	switch (runtime->channels) {
	case 2:
		/* Bit 2:0 000 */
		RecDmaMode |= 0x00;
		break;
	case 4:
		/* Bit 2:0 001 */
		RecDmaMode |= 0x01;
		break;
	case 6:
		/* Bit 2:0 011 */
		RecDmaMode |= 0x03; /* or 0x02 */
		break;
	case 8:
		/* Bit 2:0 100 */
		RecDmaMode |= 0x04;
		break;
	default:
		break;
	}
	snd_cmipci_write_b(chip, RecDmaMode, PCI_RecDMA_Mode);

	switch (chip->capture_source) {
	case CAPTURE_AC97_MIC:
		/* set Mic Volume Register 0x0Eh umute */
		snd_cmi_send_ac97_cmd(chip, 0x0e, 0x0808); /* 0x0808 : 0dB */

		/* set Line in Volume Register 0x10h mute */
		snd_cmi_send_ac97_cmd(chip, 0x10, 0x8808); /* 0x0808 : 0dB */

		/* slect record source */
		snd_cmi_send_ac97_cmd(chip, 0x1a, 0x0000); /* 0000 : Mic in */

		snd_cmi_send_ac97_cmd(chip, 0x72, 0x0001); /* Record throug Mic */
		break;
	case CAPTURE_AC97_LINEIN:
		/* set Mic Volume Register 0x0Eh mute */
		snd_cmi_send_ac97_cmd(chip, 0x0e, 0x8808); /* 0x0808 : 0dB */

		/* set Line in Volume Register 0x10h umute */
		snd_cmi_send_ac97_cmd(chip, 0x10, 0x0808); /* 0x0808 : 0dB */

		/* slect record source */
		snd_cmi_send_ac97_cmd(chip, 0x1a, 0x0000); /* 0404 : Ac97 Line in */

		snd_cmi_send_ac97_cmd(chip, 0x72, 0x0001); /* Record throug AC97 Line in */
		break;
	case CAPTURE_DIRECT_LINE_IN:
		/* set Mic Volume Register 0x0Eh mute */
		snd_cmi_send_ac97_cmd(chip, 0x0e, 0x8808); /* 0x0808 : 0dB */

		/* set Line in Volume Register 0x10h mute */
		snd_cmi_send_ac97_cmd(chip, 0x10, 0x8808); /* 0x0808 : 0dB */

		snd_cmi_send_ac97_cmd(chip, 0x72, 0x0000); /* Record throug Line in */
		break;
	}

	return 0;
}

static int snd_cmi_pcm_ac97_playback_prepare(struct snd_pcm_substream *substream)
{
	struct cmi8788 *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	u32 period_bytes;
	u16 play_routing;
	u32 ch_cfg;

	period_bytes = snd_pcm_lib_period_bytes(substream);

	/* Digital Routing and Monitoring Registers */
	play_routing = snd_cmipci_read_w(chip, Mixer_PlayRouting);
	snd_cmipci_write_w(chip, play_routing | 0x10, Mixer_PlayRouting);

	/* AC'97 Output Channel Configuration Register */
	ch_cfg = snd_cmipci_read(chip, AC97OutChanCfg);
	snd_cmipci_write(chip, ch_cfg | 0x0000ff00, AC97OutChanCfg);

	/* set DMA Front Panel Playback */
	snd_cmipci_write(chip, (u32)runtime->dma_addr , PCI_DMAPlay_Front_BaseAddr);
	snd_cmipci_write_w(chip, runtime->dma_bytes / 4 - 1, PCI_DMAPlay_Front_BaseCount); /* d-word units */
	snd_cmipci_write_w(chip, period_bytes / 4 - 1, PCI_DMAPlay_Front_BaseTCount); /* d-word units */
	return 0;
}

static int cmi_pcm_trigger(struct snd_pcm_substream *substream, int cmi_pcm_no, int subs_no, int cmd)
{
	struct cmi8788 *chip = snd_pcm_substream_chip(substream);
	struct cmi_substream *cmi_subs= &chip->cmi_pcm[cmi_pcm_no].cmi_subs[subs_no];
	int err = 0;
	u8 reset;
	u16 int_val;
	u16 int_stat;
	int DMARestRegister;

	int_val = snd_cmipci_read_w(chip, PCI_IntMask);
	int_stat = snd_cmipci_read_w(chip, PCI_DMA_SetStatus);

	if (chip->CMI8788IC_revision == CMI8788IC_Revision1)
		DMARestRegister = PCI_DMA_Reset;
	else
		DMARestRegister = PCI_DMA_FLUSH;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_START:
		cmi_subs->running = 1;

		/* Reset DMA Channel*/
		reset = snd_cmipci_read_b(chip, DMARestRegister);
		reset |= cmi_subs->DMA_chan_reset; /* set bit */
		snd_cmipci_write_b(chip, reset, DMARestRegister);
		reset &= ~cmi_subs->DMA_chan_reset; /*clear bit */
		snd_cmipci_write_b(chip, reset, DMARestRegister);

		/* enable Interrupt */
		int_val |= cmi_subs->int_mask; /* Set Bit-1 Interrupt for Recording DMA Channel A is enabled */
		snd_cmipci_write_w(chip, int_val, PCI_IntMask);

		/* Set PCI DMA Channel state -- Start*/
		int_stat |= cmi_subs->DMA_sta_mask; /* PCI DMA Channel Start/Pause/Stop Bit-0 set 1 */
		snd_cmipci_write_w(chip, int_stat, PCI_DMA_SetStatus);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	case SNDRV_PCM_TRIGGER_STOP:
		cmi_subs->running = 0;

		/* Set PCI DMA Channel state -- Stop*/
		int_stat &= ~cmi_subs->DMA_sta_mask; /* PCI DMA Channel Start/Pause/Stop Bit-0 clear */
		snd_cmipci_write_w(chip, int_stat, PCI_DMA_SetStatus);

		/* disable interrupt */
		int_val &= ~cmi_subs->int_mask; /* Clear Interrupt for Recording DMA Channel A is disabled */
		snd_cmipci_write_w(chip, int_val, PCI_IntMask);
		break;
	default:
		err = -EINVAL;
	}

	return err;
}

static int snd_cmi_pcm_playback_trigger(struct snd_pcm_substream *substream, int cmd)
{
	return cmi_pcm_trigger(substream, NORMAL_PCMS, CMI_PLAYBACK, cmd);
}

static int snd_cmi_pcm_capture_trigger(struct snd_pcm_substream *substream, int cmd)
{
	return cmi_pcm_trigger(substream, NORMAL_PCMS, CMI_CAPTURE, cmd);
}

static int snd_cmi_pcm_ac97_playback_trigger(struct snd_pcm_substream *substream, int cmd)
{
	return cmi_pcm_trigger(substream, AC97_PCMS, CMI_PLAYBACK, cmd);
}

static snd_pcm_uframes_t snd_cmi_pcm_playback_pointer(struct snd_pcm_substream *substream)
{
	struct cmi8788 *chip = snd_pcm_substream_chip(substream);
	u32 addr, pos;

	addr = snd_cmipci_read(chip, PCI_DMAPlay_MULTI_BaseAddr);
	pos = addr - (u32)substream->runtime->dma_addr;
	return bytes_to_frames(substream->runtime, pos);
}

static snd_pcm_uframes_t snd_cmi_pcm_capture_pointer(struct snd_pcm_substream *substream)
{
	struct cmi8788 *chip = snd_pcm_substream_chip(substream);
	u32 addr, pos;

	addr = snd_cmipci_read(chip, PCI_DMARec_A_BaseAddr);
	pos = addr - (u32)substream->runtime->dma_addr;
	return bytes_to_frames(substream->runtime, pos);
}

static snd_pcm_uframes_t snd_cmi_pcm_ac97_playback_pointer(struct snd_pcm_substream *substream)
{
	struct cmi8788 *chip = snd_pcm_substream_chip(substream);
	u32 addr, pos;

	addr = snd_cmipci_read(chip, PCI_DMAPlay_Front_BaseAddr);
	pos = addr - (u32)substream->runtime->dma_addr;
	return bytes_to_frames(substream->runtime, pos);
}

static struct snd_pcm_ops snd_cmi_pcm_playback_ops = {
	.open      = snd_cmi_pcm_playback_open,
	.close     = snd_cmi_pcm_playback_close,
	.ioctl     = snd_pcm_lib_ioctl,
	.hw_params = snd_cmi_pcm_hw_params,
	.hw_free   = snd_cmi_pcm_hw_free,
	.prepare   = snd_cmi_pcm_playback_prepare,
	.trigger   = snd_cmi_pcm_playback_trigger,
	.pointer   = snd_cmi_pcm_playback_pointer,
};

static struct snd_pcm_ops snd_cmi_pcm_capture_ops = {
	.open      = snd_cmi_pcm_capture_open,
	.close     = snd_cmi_pcm_capture_close,
	.ioctl     = snd_pcm_lib_ioctl,
	.hw_params = snd_cmi_pcm_hw_params,
	.hw_free   = snd_cmi_pcm_hw_free,
	.prepare   = snd_cmi_pcm_capture_prepare,
	.trigger   = snd_cmi_pcm_capture_trigger,
	.pointer   = snd_cmi_pcm_capture_pointer,
};

static struct snd_pcm_ops snd_cmi_pcm_ac97_playback_ops = {
	.open      = snd_cmi_pcm_ac97_playback_open,
	.close     = snd_cmi_pcm_ac97_playback_close,
	.ioctl     = snd_pcm_lib_ioctl,
	.hw_params = snd_cmi_pcm_hw_params,
	.hw_free   = snd_cmi_pcm_hw_free,
	.prepare   = snd_cmi_pcm_ac97_playback_prepare,
	.trigger   = snd_cmi_pcm_ac97_playback_trigger,
	.pointer   = snd_cmi_pcm_ac97_playback_pointer,
};

static void snd_cmi_pcm_free(struct snd_pcm *pcm)
{
	snd_pcm_lib_preallocate_free_for_all(pcm);
}


void snd_cmi_pcm_interrupt(struct cmi8788 *chip, struct cmi_substream *cmi_subs)
{
	u16 int_mask;

	int_mask = snd_cmipci_read_w(chip, PCI_IntMask);
	/* disable interrupt */
	snd_cmipci_write_w(chip, int_mask & ~cmi_subs->int_mask, PCI_IntMask);
	/* enable interrupt */
	snd_cmipci_write_w(chip, int_mask | cmi_subs->int_mask, PCI_IntMask);

	snd_pcm_period_elapsed(cmi_subs->substream);
}


/*
 * create pcm DAC/ADC, SPDIF
 */
int __devinit snd_cmi8788_pcm_create(struct cmi8788 *chip)
{
	struct snd_pcm *pcm;
	int err, pcm_dev = 0;

#if 1 /* swf 2005-04-25 */
	/* 1 create normal PCM */
	err = snd_pcm_new(chip->card, "C-Media PCI8788 DAC/ADC",
			  pcm_dev, 1, 1, &chip->pcm[pcm_dev]);
	if (err < 0)
		return err;

	pcm = chip->pcm[pcm_dev];

	/* create pcm stream by controller supported */
	/* The PCM use Multi-Channel Playback DMA for playback and */
	/*         use Recording Channel A DMA for record */
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_cmi_pcm_playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE,  &snd_cmi_pcm_capture_ops);

	pcm->private_data = chip;
	pcm->private_free = snd_cmi_pcm_free;
	pcm->info_flags = 0;

	strcpy(pcm->name, "C-Media PCI8788 DAC/ADC");

	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
					      snd_dma_pci_data(chip->pci),
					      1024 * 64, 1024 * 128);

	pcm_dev++;

#endif /* swf 2005-04-25 */

#if 0
	/* create the other pcm stream. AC97 SPDIF */
	/* 2 create AC97 PCM */
	err = snd_pcm_new(chip->card, "C-Media PCI8788 DAC/ADC",
			  pcm_dev, 1, 0, &chip->pcm[pcm_dev]);
	if (err < 0)
		return err;

	pcm = chip->pcm[pcm_dev];
	codec = &chip->ac97_codec_list[0];
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_cmi_pcm_ac97_playback_ops);

	pcm->private_data = chip;
	pcm->private_free = snd_cmi_pcm_free;
	pcm->info_flags = 0;

	strcpy(pcm->name, "C-Media PCI8788 AC97 DAC/ADC");

	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
					      snd_dma_pci_data(chip->pci),
					      1024 * 64, 1024 * 128);
	pcm_dev++;

#endif
	/* 3 create SPDIF PCM */

	chip->PCM_Count = pcm_dev;

	return 0;
}

