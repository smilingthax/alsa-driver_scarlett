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
#include <linux/pci.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include "cmi8788.h"


/*
 * TODO:
 * - spdif
 * - second ac97
 * - sync start
 * - suspend/resume
 */

static struct snd_pcm_hardware snd_cmi_pcm_playback_hw = {
	.info = SNDRV_PCM_INFO_MMAP |
		SNDRV_PCM_INFO_MMAP_VALID |
		SNDRV_PCM_INFO_INTERLEAVED |
		SNDRV_PCM_INFO_PAUSE,
	.formats = SNDRV_PCM_FMTBIT_S16_LE |
		   SNDRV_PCM_FMTBIT_S32_LE,
	.rates = SNDRV_PCM_RATE_32000 |
		 SNDRV_PCM_RATE_44100 |
		 SNDRV_PCM_RATE_48000 |
		 SNDRV_PCM_RATE_44100 |
		 SNDRV_PCM_RATE_64000 |
		 SNDRV_PCM_RATE_88200 |
		 SNDRV_PCM_RATE_176400 |
		 SNDRV_PCM_RATE_192000,
	.rate_min = 32000,
	.rate_max = 192000,
	.channels_min = 2,
	.channels_max = 8,
	.buffer_bytes_max = 1024 * 1024,
	.period_bytes_min = 128,
	.period_bytes_max = 256 * 1024,
	.periods_min = 2,
	.periods_max = 1024,
};

static struct snd_pcm_hardware snd_cmi_pcm_capture_hw = {
	.info = SNDRV_PCM_INFO_MMAP |
		SNDRV_PCM_INFO_MMAP_VALID |
		SNDRV_PCM_INFO_INTERLEAVED |
		SNDRV_PCM_INFO_PAUSE,
	.formats = SNDRV_PCM_FMTBIT_S16_LE |
		   SNDRV_PCM_FMTBIT_S32_LE,
	.rates = SNDRV_PCM_RATE_32000 |
		 SNDRV_PCM_RATE_44100 |
		 SNDRV_PCM_RATE_48000 |
		 SNDRV_PCM_RATE_44100 |
		 SNDRV_PCM_RATE_64000 |
		 SNDRV_PCM_RATE_88200 |
		 SNDRV_PCM_RATE_176400 |
		 SNDRV_PCM_RATE_192000,
	.rate_min = 32000,
	.rate_max = 192000,
	.channels_min = 2,
	.channels_max = 2,
	.buffer_bytes_max = 1024 * 1024,
	.period_bytes_min = 128,
	.period_bytes_max = 256 * 1024,
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
static void cmi_pcm_open(struct snd_pcm_substream *substream, int cmi_pcm_no, int subs_no)
{
	struct cmi8788 *chip = snd_pcm_substream_chip(substream);
	struct cmi_substream *cmi_subs = &chip->cmi_pcm[cmi_pcm_no].cmi_subs[subs_no];

	cmi_subs->substream = substream;
	substream->runtime->private_data = cmi_subs;
}

static int snd_cmi_pcm_playback_open(struct snd_pcm_substream *substream)
{
	struct cmi8788 *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct cmi_substream *cmi_subs;

	cmi_pcm_open(substream, NORMAL_PCMS, CMI_PLAYBACK);
	chip->playback_volume_init = 1;
	cmi_subs = runtime->private_data;
	cmi_subs->dma_mask = 0x0010;
	cmi_subs->int_mask = 0x0010;
	runtime->hw = snd_cmi_pcm_playback_hw;
	return 0;
}

static int snd_cmi_pcm_capture_open(struct snd_pcm_substream *substream)
{
	struct cmi8788 *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct cmi_substream *cmi_subs;

	cmi_pcm_open(substream, NORMAL_PCMS, CMI_CAPTURE);
	chip->capture_volume_init = 1;
	cmi_subs = runtime->private_data;
	cmi_subs->dma_mask = 0x0001;
	cmi_subs->int_mask = 0x0001;
	runtime->hw = snd_cmi_pcm_capture_hw;
	return 0;
}

static int snd_cmi_pcm_ac97_playback_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct cmi_substream *cmi_subs;

	cmi_pcm_open(substream, AC97_PCMS, CMI_PLAYBACK);
	cmi_subs = runtime->private_data;
	cmi_subs->dma_mask = 0x0020;
	cmi_subs->int_mask = 0x4020;
	runtime->hw = snd_cmi_pcm_playback_hw;
	return 0;
}

static int snd_cmi_pcm_close(struct snd_pcm_substream *substream)
{
	struct cmi_substream *cmi_subs = substream->runtime->private_data;

	cmi_subs->substream = NULL;
	cmi_subs->dma_mask = 0x0000;
	cmi_subs->int_mask = 0x0000;
	return 0;
}

static inline u32 cmi_sample_format(unsigned int sample_bits)
{
	return (sample_bits - 16) >> 3;
}

static u32 i2s_sample_format(unsigned int sample_bits)
{
	switch (sample_bits) {
	default: /* 16 */
		return 0;
	case 24:
		return 2;
	case 32:
		return 3;
	}
}

static u32 cmi_channel_bits(unsigned int channels)
{
	switch (channels) {
	default: /* 2 */
		return 0;
	case 4:
		return 1;
	case 6:
		return 2;
	case 8:
		return 3;
	}
}

static u32 i2s_rate_bits(unsigned int rate)
{
	switch (rate) {
	case 32000:
		return 0;
	case 44100:
		return 1;
	default: /* 48000 */
		return 2;
	case 64000:
		return 3;
	case 88200:
		return 4;
	case 96000:
		return 5;
	case 176400:
		return 6;
	case 192000:
		return 7;
	}
}

static int snd_cmi_pcm_playback_hw_params(struct snd_pcm_substream *substream,
					  struct snd_pcm_hw_params *hw_params)
{
	struct cmi8788 *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	u16 play_routing;
	u16 I2SFmt;
	u8 fmt;
	u8 PlyDmaMode;
	int err;

	err = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
	if (err < 0)
		return err;

	/* Digital Routing and Monitoring Registers */
	play_routing = snd_cmipci_read_w(chip, Mixer_PlayRouting);
	snd_cmipci_write_w(chip, play_routing & ~0x10, Mixer_PlayRouting);

	/* set DMA Multi-Channel Playback DMA buffer addr length and fragsize */
	snd_cmipci_write(chip, (u32)runtime->dma_addr,
			 PCI_DMAPlay_MULTI_BaseAddr);
	snd_cmipci_write(chip, runtime->dma_bytes / 4 - 1,
			 PCI_DMAPlay_MULTI_BaseCount); /* 32-bit units */
	snd_cmipci_write(chip, snd_pcm_lib_period_bytes(substream) / 4 - 1,
			 PCI_DMAPlay_MUTLI_BaseTCount); /* 32-bit units */

	/* Sample Format Convert for Playback Channels */
	fmt = snd_cmipci_read_b(chip, PCI_PlaySampleFmCvt) & ~0x0c;
	fmt |= cmi_sample_format(runtime->sample_bits) << 2;
	snd_cmipci_write_b(chip, fmt, PCI_PlaySampleFmCvt);

	/* I2S Multi-Channel DAC Format Register */
	I2SFmt = snd_cmipci_read_w(chip, I2S_Multi_DAC_Fmt) & ~0x00c7;
	I2SFmt |= i2s_sample_format(runtime->sample_bits) << 6;
	I2SFmt |= i2s_rate_bits(runtime->rate);
	snd_cmipci_write_w(chip, I2SFmt, I2S_Multi_DAC_Fmt);

	/* Multi-Channel DMA Mode */
	PlyDmaMode = snd_cmipci_read_b(chip, PCI_MULTI_DMA_MODE) & ~0x03;
	PlyDmaMode |= cmi_channel_bits(runtime->channels);
	snd_cmipci_write_b(chip, PlyDmaMode, PCI_MULTI_DMA_MODE);
	return 0;
}

static int snd_cmi_pcm_capture_hw_params(struct snd_pcm_substream *substream,
					 struct snd_pcm_hw_params *hw_params)
{
	struct cmi8788 *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	u8 fmt;
	u16 I2SFmt;
	u8 RecDmaMode;
	int err;

	err = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
	if (err < 0)
		return err;

	/* set DMA Recording Channel A DMA buffer addr length and fragsize */
	snd_cmipci_write(chip, (u32)runtime->dma_addr,
			 PCI_DMARec_A_BaseAddr);
	snd_cmipci_write_w(chip, runtime->dma_bytes / 4 - 1,
			   PCI_DMARec_A_BaseCount); /* 32-bit units */
	snd_cmipci_write_w(chip, snd_pcm_lib_period_bytes(substream) / 4 - 1,
			   PCI_DMARec_A_BaseTCount); /* 32-bit units */

	/* Sample Format Convert for Recording Channels */
	fmt = snd_cmipci_read_b(chip, PCI_RecSampleFmtCvt) & ~0x03;
	fmt |= cmi_sample_format(runtime->sample_bits);
	snd_cmipci_write_b(chip, fmt, PCI_RecSampleFmtCvt);

	/* I2S ADC 1 Format Register */
	I2SFmt = snd_cmipci_read_w(chip, I2S_ADC1_Fmt) & ~0x00c7;
	I2SFmt |= i2s_sample_format(runtime->sample_bits) << 6;
	I2SFmt |= i2s_rate_bits(runtime->rate);
	snd_cmipci_write_w(chip, I2SFmt, I2S_ADC1_Fmt);

	RecDmaMode = snd_cmipci_read_b(chip, PCI_RecDMA_Mode) & ~0x07;
	RecDmaMode |= cmi_channel_bits(runtime->channels);
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

static int snd_cmi_pcm_ac97_playback_hw_params(struct snd_pcm_substream *substream,
					       struct snd_pcm_hw_params *hw_params)
{
	struct cmi8788 *chip = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	u16 play_routing;
	u32 ch_cfg;
	int err;

	err = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
	if (err < 0)
		return err;

	/* Digital Routing and Monitoring Registers */
	play_routing = snd_cmipci_read_w(chip, Mixer_PlayRouting);
	snd_cmipci_write_w(chip, play_routing | 0x10, Mixer_PlayRouting);

	/* AC'97 Output Channel Configuration Register */
	ch_cfg = snd_cmipci_read(chip, AC97OutChanCfg);
	snd_cmipci_write(chip, ch_cfg | 0x0000ff00, AC97OutChanCfg);

	/* set DMA Front Panel Playback */
	snd_cmipci_write(chip, (u32)runtime->dma_addr,
			 PCI_DMAPlay_Front_BaseAddr);
	snd_cmipci_write_w(chip, runtime->dma_bytes / 4 - 1,
			   PCI_DMAPlay_Front_BaseCount); /* 32-bit units */
	snd_cmipci_write_w(chip, snd_pcm_lib_period_bytes(substream) / 4 - 1,
			   PCI_DMAPlay_Front_BaseTCount); /* 32-bit units */
	return 0;
}

static int snd_cmi_pcm_hw_free(struct snd_pcm_substream *substream)
{
	return snd_pcm_lib_free_pages(substream);
}

static int snd_cmi_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct cmi8788 *chip = snd_pcm_substream_chip(substream);
	struct cmi_substream *cmi_subs = substream->runtime->private_data;
	unsigned int DMARestRegister;
	u8 reset;

	if (chip->CMI8788IC_revision == CMI8788IC_Revision1)
		DMARestRegister = PCI_DMA_Reset;
	else
		DMARestRegister = PCI_DMA_FLUSH;

	/* Reset DMA Channel*/
	reset = snd_cmipci_read_b(chip, DMARestRegister);
	reset |= cmi_subs->dma_mask; /* set bit */
	snd_cmipci_write_b(chip, reset, DMARestRegister);
	reset &= ~cmi_subs->dma_mask; /* clear bit */
	snd_cmipci_write_b(chip, reset, DMARestRegister);
	return 0;
}

static int snd_cmi_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct cmi8788 *chip = snd_pcm_substream_chip(substream);
	struct cmi_substream *cmi_subs = substream->runtime->private_data;
	int err = 0;
	u16 int_val;
	u16 int_stat;

	int_val = snd_cmipci_read_w(chip, PCI_IntMask);
	int_stat = snd_cmipci_read_w(chip, PCI_DMA_SetStatus);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		cmi_subs->running = 1;

		/* enable Interrupt */
		int_val |= cmi_subs->int_mask;
		snd_cmipci_write_w(chip, int_val, PCI_IntMask);

		/* Set PCI DMA Channel state -- Start */
		int_stat |= cmi_subs->dma_mask;
		snd_cmipci_write_w(chip, int_stat, PCI_DMA_SetStatus);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		cmi_subs->running = 0;

		/* Set PCI DMA Channel state -- Stop */
		int_stat &= ~cmi_subs->dma_mask;
		snd_cmipci_write_w(chip, int_stat, PCI_DMA_SetStatus);

		/* disable interrupt */
		int_val &= ~cmi_subs->int_mask;
		snd_cmipci_write_w(chip, int_val, PCI_IntMask);
		break;
	default:
		err = -EINVAL;
	}
	return err;
}

static snd_pcm_uframes_t cmi_pcm_pointer(struct snd_pcm_substream *substream,
					 unsigned int reg)
{
	struct cmi8788 *chip = snd_pcm_substream_chip(substream);
	u32 pos;

	pos = snd_cmipci_read(chip, reg) - (u32)substream->runtime->dma_addr;
	return bytes_to_frames(substream->runtime, pos);
}

static snd_pcm_uframes_t snd_cmi_pcm_playback_pointer(struct snd_pcm_substream *substream)
{
	return cmi_pcm_pointer(substream, PCI_DMAPlay_MULTI_BaseAddr);
}

static snd_pcm_uframes_t snd_cmi_pcm_capture_pointer(struct snd_pcm_substream *substream)
{
	return cmi_pcm_pointer(substream, PCI_DMARec_A_BaseAddr);
}

static snd_pcm_uframes_t snd_cmi_pcm_ac97_playback_pointer(struct snd_pcm_substream *substream)
{
	return cmi_pcm_pointer(substream, PCI_DMAPlay_Front_BaseAddr);
}

static struct snd_pcm_ops snd_cmi_pcm_playback_ops = {
	.open      = snd_cmi_pcm_playback_open,
	.close     = snd_cmi_pcm_close,
	.ioctl     = snd_pcm_lib_ioctl,
	.hw_params = snd_cmi_pcm_playback_hw_params,
	.hw_free   = snd_cmi_pcm_hw_free,
	.prepare   = snd_cmi_pcm_prepare,
	.trigger   = snd_cmi_pcm_trigger,
	.pointer   = snd_cmi_pcm_playback_pointer,
};

static struct snd_pcm_ops snd_cmi_pcm_capture_ops = {
	.open      = snd_cmi_pcm_capture_open,
	.close     = snd_cmi_pcm_close,
	.ioctl     = snd_pcm_lib_ioctl,
	.hw_params = snd_cmi_pcm_capture_hw_params,
	.hw_free   = snd_cmi_pcm_hw_free,
	.prepare   = snd_cmi_pcm_prepare,
	.trigger   = snd_cmi_pcm_trigger,
	.pointer   = snd_cmi_pcm_capture_pointer,
};

static struct snd_pcm_ops snd_cmi_pcm_ac97_playback_ops = {
	.open      = snd_cmi_pcm_ac97_playback_open,
	.close     = snd_cmi_pcm_close,
	.ioctl     = snd_pcm_lib_ioctl,
	.hw_params = snd_cmi_pcm_ac97_playback_hw_params,
	.hw_free   = snd_cmi_pcm_hw_free,
	.prepare   = snd_cmi_pcm_prepare,
	.trigger   = snd_cmi_pcm_trigger,
	.pointer   = snd_cmi_pcm_ac97_playback_pointer,
};

static void snd_cmi_pcm_free(struct snd_pcm *pcm)
{
	snd_pcm_lib_preallocate_free_for_all(pcm);
}


/*
 * create pcm devices
 */
int __devinit snd_cmi8788_pcm_create(struct cmi8788 *chip)
{
	struct snd_pcm *pcm;
	int err;

	/* 1 create normal PCM */
	err = snd_pcm_new(chip->card, "CMI8788", 0, 1, 1, &pcm);
	if (err < 0)
		return err;

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_cmi_pcm_playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE,  &snd_cmi_pcm_capture_ops);

	pcm->private_data = chip;
	pcm->private_free = snd_cmi_pcm_free;

	strcpy(pcm->name, "C-Media PCI8788 Multichannel");

	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
					      snd_dma_pci_data(chip->pci),
					      1024 * 256, 1024 * 1024);

#if 0
	/* 2 create AC97 PCM */
	err = snd_pcm_new(chip->card, "CMI8788 FP", 2, 1, 0, &pcm);
	if (err < 0)
		return err;

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_cmi_pcm_ac97_playback_ops);

	pcm->private_data = chip;
	pcm->private_free = snd_cmi_pcm_free;

	strcpy(pcm->name, "C-Media PCI8788 Front Panel");

	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
					      snd_dma_pci_data(chip->pci),
					      1024 * 64, 1024 * 128);
#endif

	/* 3 create SPDIF PCM */

	chip->PCM_Count = 3;
	return 0;
}

