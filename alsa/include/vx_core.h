/*
 * Driver for Digigram VX soundcards
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

#ifndef __VX_COMMON_H
#define __VX_COMMON_H

#include <sound/pcm.h>

typedef struct snd_vx_core vx_core_t;
typedef struct vx_pipe vx_pipe_t;

#define vx_core_t_magic		0xa15a4110
#define vx_pipe_t_magic		0xa15a4112

struct snd_vx_image {
	unsigned char *image;
	unsigned int length;
};

/*
 */
#define SIZE_MAX_CMD    0x10
#define SIZE_MAX_STATUS 0x10

struct vx_rmh {
	u16	LgCmd;		/* length of the command to send (WORDs) */
	u16	LgStat;		/* length of the status received (WORDs) */
	u32	Cmd[SIZE_MAX_CMD];
	u32	Stat[SIZE_MAX_STATUS];
	u16	DspStat;	/* status type, RMP_SSIZE_XXX */
};
	
typedef u64 pcx_time_t;

#define VX_MAX_PIPES	16
#define VX_MAX_PERIODS	32
#define VX_MAX_CODECS	2

struct vx_ibl_info {
	int size;	/* the current IBL size (0 = query) in bytes */
	int max_size;	/* max. IBL size in bytes */
	int min_size;	/* min. IBL size in bytes */
	int granularity;	/* granularity */
};

struct vx_pipe {
	int number;
	unsigned int is_capture: 1;
	unsigned int data_mode: 1;
	unsigned int running: 1;
	unsigned int prepared: 1;
	int channels;
	unsigned int differed_type;
	pcx_time_t pcx_time;
	snd_pcm_substream_t *substream;

	int hbuf_size;		/* H-buffer size in bytes */
	int buffer_bytes;	/* the ALSA pcm buffer size in bytes */
	int hw_ptr;		/* the current hardware pointer in frames */
	int position;		/* the current position in frames (playback only) */
	int transferred;	/* the transferred size (per period) in frames */
	int chunk_transferred;	/* the transferred size (per period) in frames */
	int align;		/* size of alignment */
	u64 cur_count;		/* current sample position (for playback) */

	struct tasklet_struct start_tq;
};

struct snd_vx_ops {
	/* low-level i/o */
	unsigned char (*in8)(vx_core_t *chip, int reg);
	unsigned int (*in32)(vx_core_t *chip, int reg);
	void (*out8)(vx_core_t *chip, int reg, unsigned char val);
	void (*out32)(vx_core_t *chip, int reg, unsigned int val);
	/* irq */
	int (*test_and_ack)(vx_core_t *chip);
	void (*validate_irq)(vx_core_t *chip, int enable);
	/* codec */
	void (*write_codec)(vx_core_t *chip, int codec, unsigned int data);
	void (*reset_codec)(vx_core_t *chip);
	void (*change_audio_source)(vx_core_t *chip, int src);
	void (*set_clock_source)(vx_core_t *chp, int src);
	/* chip init */
	int (*test_xilinx)(vx_core_t *chip);
	int (*load_xilinx)(vx_core_t *chip);
	void (*reset_dsp)(vx_core_t *chip);
	void (*reset_board)(vx_core_t *chip);
	/* pcm */
	void (*dma_write)(vx_core_t *chip, snd_pcm_runtime_t *runtime,
			  vx_pipe_t *pipe, int count);
	void (*dma_read)(vx_core_t *chip, snd_pcm_runtime_t *runtime,
			  vx_pipe_t *pipe, int count);
};

struct snd_vx_hardware {
	const char *name;
	int type;	/* VX_TYPE_XXX */

	/* xilinx and dsp images */
	struct snd_vx_image boot;
	struct snd_vx_image xilinx;
	struct snd_vx_image dsp_boot;
	struct snd_vx_image dsp;

	/* hardware specs */
	int num_codecs;
	int num_ins;
	int num_outs;
};

struct snd_vx_core {
	/* ALSA stuff */
	snd_card_t *card;
	snd_pcm_t *pcm[VX_MAX_CODECS];
	int type;	/* VX_TYPE_XXX */

	int irq;
	/* ports are defined externally */

	/* low-level functions */
	struct snd_vx_hardware *hw;
	struct snd_vx_ops *ops;

	spinlock_t lock;
	spinlock_t irq_lock;
	struct tasklet_struct tq;

	unsigned int initialized: 1;
	unsigned int in_suspend: 1;
	unsigned int xilinx_tested: 1;
	unsigned int is_stale: 1;

	unsigned int pcm_running;

	struct vx_rmh irq_rmh;	/* RMH used in interrupts */

	unsigned int audio_info; /* see VX_AUDIO_INFO */
	int audio_ins;
	int audio_outs;
	struct vx_pipe **playback_pipes;
	struct vx_pipe **capture_pipes;

	/* clock and audio sources */
	int audio_source;	/* current audio input source */
	int audio_source_target;
	int clock_source;	/* current clock source (INTERNAL_QUARTZ or UER_SYNC) */
	int freq;		/* current frequency */
	int freq_detected;	/* detected frequency from digital in */
	int uer_detected;	/* VX_UER_MODE_XXX */
	unsigned int uer_bits;	/* IEC958 status bits */
	struct vx_ibl_info ibl;	/* IBL information */

	/* mixer setting */
	int output_level[VX_MAX_CODECS][2];	/* analog output level */
	int audio_gain[2][4];			/* digital audio level (playback/capture) */
	unsigned char audio_active[4];		/* mute/unmute on digital playback */
	int audio_monitor[4];			/* playback hw-monitor level */
	unsigned char audio_monitor_active[4];	/* playback hw-monitor mute/unmute */
};


/*
 * constructor
 */
vx_core_t *snd_vx_create(snd_card_t *card, struct snd_vx_hardware *hw,
			 struct snd_vx_ops *ops, int extra_size);
int snd_vx_init(vx_core_t *chip);
int snd_vx_load_boot_image(vx_core_t *chip, const struct snd_vx_image *boot);

/*
 * interrupt handler; exported for pcmcia
 */
void snd_vx_irq_handler(int irq, void *dev, struct pt_regs *regs);

/*
 * power-management routines
 */
#ifdef CONFIG_PM
void snd_vx_suspend(vx_core_t *chip);
void snd_vx_resume(vx_core_t *chip);
#endif


/*
 * lowlevel functions
 */
inline static int vx_test_and_ack(vx_core_t *chip)
{
	snd_assert(chip->ops->test_and_ack, return -ENXIO);
	return chip->ops->test_and_ack(chip);
}

inline static void vx_validate_irq(vx_core_t *chip, int enable)
{
	snd_assert(chip->ops->validate_irq, return);
	chip->ops->validate_irq(chip, enable);
}

inline static unsigned char snd_vx_inb(vx_core_t *chip, int reg)
{
	snd_assert(chip->ops->in8, return 0);
	return chip->ops->in8(chip, reg);
}

inline static unsigned int snd_vx_inl(vx_core_t *chip, int reg)
{
	snd_assert(chip->ops->in32, return 0);
	return chip->ops->in32(chip, reg);
}

inline static void snd_vx_outb(vx_core_t *chip, int reg, unsigned char val)
{
	snd_assert(chip->ops->out8, return);
	return chip->ops->out8(chip, reg, val);
}

inline static void snd_vx_outl(vx_core_t *chip, int reg, unsigned int val)
{
	snd_assert(chip->ops->out32, return);
	return chip->ops->out32(chip, reg, val);
}

#define vx_inb(chip,reg)	snd_vx_inb(chip, VX_##reg)
#define vx_outb(chip,reg,val)	snd_vx_outb(chip, VX_##reg,val)
#define vx_inl(chip,reg)	snd_vx_inl(chip, VX_##reg)
#define vx_outl(chip,reg,val)	snd_vx_outl(chip, VX_##reg,val)

void snd_vx_delay(vx_core_t *chip, int msec);

static inline void vx_reset_dsp(vx_core_t *chip)
{
	snd_assert(chip->ops->reset_dsp, return);
	chip->ops->reset_dsp(chip);
}

int vx_send_msg(vx_core_t *chip, struct vx_rmh *rmh);
int vx_send_msg_nolock(vx_core_t *chip, struct vx_rmh *rmh);
int vx_send_rih(vx_core_t *chip, int cmd);
int vx_send_rih_nolock(vx_core_t *chip, int cmd);

void vx_reset_codec(vx_core_t *chip);

/*
 * check the bit on the specified register
 * returns zero if a bit matches, or a negative error code.
 * exported for vxpocket driver
 */
int snd_vx_check_reg_bit(vx_core_t *chip, int reg, int mask, int bit, int time);
#define vx_check_isr(chip,mask,bit,time) snd_vx_check_reg_bit(chip, VX_ISR, mask, bit, time)
#define vx_wait_isr_bit(chip,bit) vx_check_isr(chip, bit, bit, 200)
#define vx_wait_for_rx_full(chip) vx_wait_isr_bit(chip, ISR_RX_FULL)


/*
 * pseudo-DMA transfer
 */
inline static void vx_pseudo_dma_write(vx_core_t *chip, snd_pcm_runtime_t *runtime,
				       vx_pipe_t *pipe, int count)
{
	snd_assert(chip->ops->dma_write, return);
	chip->ops->dma_write(chip, runtime, pipe, count);
}

inline static void vx_pseudo_dma_read(vx_core_t *chip, snd_pcm_runtime_t *runtime,
				      vx_pipe_t *pipe, int count)
{
	snd_assert(chip->ops->dma_read, return);
	chip->ops->dma_read(chip, runtime, pipe, count);
}



/* error with hardware code,
 * the return value is -(VX_ERR_MASK | actual-hw-error-code)
 */
#define VX_ERR_MASK	0x1000000
#define vx_get_error(err)	(-(err) & ~VX_ERR_MASK)


/*
 * pcm stuff
 */
int snd_vx_pcm_new(vx_core_t *chip);
void vx_pcm_playback_update_buffer(vx_core_t *chip, snd_pcm_substream_t *subs, vx_pipe_t *pipe);
void vx_pcm_playback_update(vx_core_t *chip, snd_pcm_substream_t *subs, vx_pipe_t *pipe);
void vx_pcm_capture_update(vx_core_t *chip, snd_pcm_substream_t *subs, vx_pipe_t *pipe);

/*
 * mixer stuff
 */
int snd_vx_mixer_new(vx_core_t *chip);
void vx_toggle_dac_mute(vx_core_t *chip, int mute);
int vx_sync_audio_source(vx_core_t *chip);

/*
 * IEC958 & clock stuff
 */
void vx_set_iec958_status(vx_core_t *chip, unsigned int bits);
int vx_set_clock(vx_core_t *chip, int freq);
void vx_set_internal_clock(vx_core_t *chip, int freq);
int vx_change_frequency(vx_core_t *chip);


/*
 * hardware constants
 */

/* hardware type */
enum {
	/* VX222 PCI */
	VX_TYPE_BOARD,
	VX_TYPE_V2,
	VX_TYPE_MIC,
	/* VX-pocket */
	VX_TYPE_VXPOCKET,
	VX_TYPE_VXP440
};

/* audio input source */
enum {
	VX_AUDIO_SRC_DIGITAL,
	VX_AUDIO_SRC_LINE,
	VX_AUDIO_SRC_MIC
};

/* clock source */
enum {
	INTERNAL_QUARTZ,
	UER_SYNC
};

/* SPDIF/UER type */
enum {
	VX_UER_MODE_CONSUMER,
	VX_UER_MODE_PROFESSIONAL,
	VX_UER_MODE_NOT_PRESENT,
};

/* register indices */
enum {
	VX_ICR,
	VX_CVR,
	VX_ISR,
	VX_IVR,
	VX_RXH,
	VX_TXH = VX_RXH,
	VX_RXM,
	VX_TXM = VX_RXM,
	VX_RXL,
	VX_TXL = VX_RXL,
	VX_DMA,
	VX_CDSP,
	VX_RFREQ,
	VX_RUER_V2,
	VX_GAIN,
	VX_DATA = VX_GAIN,
	VX_MEMIRQ,
	VX_ACQ,
	VX_BIT0,
	VX_BIT1,
	VX_MIC0,
	VX_MIC1,
	VX_MIC2,
	VX_MIC3,
	VX_PLX0,
	VX_PLX1,
	VX_PLX2,

	VX_LOFREQ,  // V2: ACQ, VP: RFREQ
	VX_HIFREQ,  // V2: BIT0, VP: RUER_V2
	VX_CSUER,   // V2: BIT1, VP: BIT0
	VX_RUER,    // V2: RUER_V2, VP: BIT1

	VX_REG_MAX,

	/* aliases for VX board */
	VX_RESET_DMA = VX_ISR,
	VX_CFG = VX_RFREQ,
	VX_STATUS = VX_MEMIRQ,
	VX_SELMIC = VX_MIC0,
	VX_COMPOT = VX_MIC1,
	VX_SCOMPR = VX_MIC2,
	VX_GLIMIT = VX_MIC3,
	VX_INTCSR = VX_PLX0,
	VX_CNTRL = VX_PLX1,
	VX_GPIOC = VX_PLX2,

	/* aliases for VXPOCKET board */
	VX_MICRO = VX_MEMIRQ,
	VX_CODEC2 = VX_MEMIRQ,
	VX_DIALOG = VX_ACQ,

};

/* RMH status type */
enum {
	RMH_SSIZE_FIXED = 0,	/* status size given by the driver (in LgStat) */
	RMH_SSIZE_ARG = 1,	/* status size given in the LSB byte */
	RMH_SSIZE_MASK = 2,	/* status size given in bitmask */
};


/* bits for ICR register */
#define ICR_HF1		0x10
#define ICR_HF0		0x08
#define ICR_TREQ	0x02	/* Interrupt mode + HREQ set on for transfer (->DSP) request */
#define ICR_RREQ	0x01	/* Interrupt mode + RREQ set on for transfer (->PC) request */

/* bits for CVR register */
#define CVR_HC		0x80

/* bits for ISR register */
#define ISR_HF3		0x10
#define ISR_HF2		0x08
#define ISR_CHK		0x10
#define ISR_ERR		0x08
#define ISR_TX_READY	0x04
#define ISR_TX_EMPTY	0x02
#define ISR_RX_FULL	0x01

/* Constants used to access the DATA register */
#define VX_DATA_CODEC_MASK	0x80
#define VX_DATA_XICOR_MASK	0x80

/* Constants used to access the CSUER register (both for VX2 and VXP) */
#define VX_SUER_FREQ_MASK		0x0c
#define VX_SUER_FREQ_32KHz_MASK		0x0c
#define VX_SUER_FREQ_44KHz_MASK		0x00
#define VX_SUER_FREQ_48KHz_MASK		0x04
#define VX_SUER_DATA_PRESENT_MASK	0x02
#define VX_SUER_CLOCK_PRESENT_MASK	0x01

#define VX_CUER_HH_BITC_SEL_MASK	0x08
#define VX_CUER_MH_BITC_SEL_MASK	0x04
#define VX_CUER_ML_BITC_SEL_MASK	0x02
#define VX_CUER_LL_BITC_SEL_MASK	0x01

#define XX_UER_CBITS_OFFSET_MASK	0x1f


/* bits for audio_info */
#define VX_AUDIO_INFO_REAL_TIME	(1<<0)	/* real-time processing available */
#define VX_AUDIO_INFO_OFFLINE	(1<<1)	/* offline processing available */
#define VX_AUDIO_INFO_MPEG1	(1<<5)
#define VX_AUDIO_INFO_MPEG2	(1<<6)
#define VX_AUDIO_INFO_LINEAR_8	(1<<7)
#define VX_AUDIO_INFO_LINEAR_16	(1<<8)
#define VX_AUDIO_INFO_LINEAR_24	(1<<9)

/* DSP Interrupt Request values */
#define VXP_IRQ_OFFSET		0x40 /* add 0x40 offset for vxpocket */
/* call with vx_send_irq_dsp() */
#define IRQ_MESS_WRITE_END          0x30
#define IRQ_MESS_WRITE_NEXT         0x32
#define IRQ_MESS_READ_NEXT          0x34
#define IRQ_MESS_READ_END           0x36
#define IRQ_MESSAGE                 0x38
#define IRQ_RESET_CHK               0x3A
#define IRQ_CONNECT_STREAM_NEXT     0x26
#define IRQ_CONNECT_STREAM_END      0x28
#define IRQ_PAUSE_START_CONNECT     0x2A
#define IRQ_END_CONNECTION          0x2C

/* Is there async. events pending ( IT Source Test ) */
#define ASYNC_EVENTS_PENDING            0x008000
#define HBUFFER_EVENTS_PENDING          0x004000   // Not always accurate
#define NOTIF_EVENTS_PENDING            0x002000
#define TIME_CODE_EVENT_PENDING         0x001000
#define FREQUENCY_CHANGE_EVENT_PENDING  0x000800
#define END_OF_BUFFER_EVENTS_PENDING    0x000400
#define FATAL_DSP_ERROR                 0xff0000

/* Stream Format Header Defines */ 
#define HEADER_FMT_BASE			0xFED00000
#define HEADER_FMT_MONO			0x000000C0
#define HEADER_FMT_INTEL		0x00008000
#define HEADER_FMT_16BITS		0x00002000
#define HEADER_FMT_24BITS		0x00004000
#define HEADER_FMT_UPTO11		0x00000200	/* frequency is less or equ. to 11k.*/
#define HEADER_FMT_UPTO32		0x00000100	/* frequency is over 11k and less then 32k.*/

/* Constants used to access the Codec */
#define XX_CODEC_SELECTOR               0x20
/* codec commands */
#define XX_CODEC_ADC_CONTROL_REGISTER   0x01
#define XX_CODEC_DAC_CONTROL_REGISTER   0x02
#define XX_CODEC_LEVEL_LEFT_REGISTER    0x03
#define XX_CODEC_LEVEL_RIGHT_REGISTER   0x04
#define XX_CODEC_PORT_MODE_REGISTER     0x05
#define XX_CODEC_STATUS_REPORT_REGISTER 0x06
#define XX_CODEC_CLOCK_CONTROL_REGISTER 0x07

/*
 * Audio-level control values
 */
#define CVAL_M110DB		0x000	/* -110dB */
#define CVAL_M99DB		0x02C
#define CVAL_M21DB		0x163
#define CVAL_M18DB		0x16F
#define CVAL_M10DB		0x18F
#define CVAL_0DB		0x1B7
#define CVAL_18DB		0x1FF	/* +18dB */
#define CVAL_MAX		0x1FF

#define AUDIO_IO_HAS_MUTE_LEVEL			0x400000
#define AUDIO_IO_HAS_MUTE_MONITORING_1		0x200000
#define AUDIO_IO_HAS_MUTE_MONITORING_2		0x100000
#define VALID_AUDIO_IO_DIGITAL_LEVEL		0x01
#define VALID_AUDIO_IO_MONITORING_LEVEL		0x02
#define VALID_AUDIO_IO_MUTE_LEVEL		0x04
#define VALID_AUDIO_IO_MUTE_MONITORING_1	0x08
#define VALID_AUDIO_IO_MUTE_MONITORING_2	0x10


#endif /* __VX_COMMON_H */
