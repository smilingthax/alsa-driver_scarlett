/*
 * Driver for Digigram VXpocket soundcards
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

#ifndef __VXPOCKET_H
#define __VXPOCKET_H

#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>

typedef struct snd_vxpocket vxpocket_t;
typedef struct vx_pipe vx_pipe_t;

#define vxpocket_t_magic	0xa15a4101
#define vx_pipe_t_magic		0xa15a4102

struct snd_vxp_image {
	unsigned char *image;
	unsigned int length;
};

struct snd_vxp_entry {
	const char *name;
	int type;	/* VXP_TYPE_XXX */
	dev_info_t *dev_info;

	/* module parameters */
	int *index_table;
	char **id_table;
	int *enable_table;
	unsigned int *irq_mask_p;
	int *irq_list;

	/* images */
	struct snd_vxp_image boot;
	struct snd_vxp_image xilinx;
	struct snd_vxp_image dsp_boot;
	struct snd_vxp_image dsp;

	/* hardware specs */
	int num_codecs;
	int num_ins;
	int num_outs;

	/* slots */
	vxpocket_t *card_list[SNDRV_CARDS];
	dev_link_t *dev_list;		/* Linked list of devices */
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

#define VXP_MAX_PIPES	16
#define VXP_MAX_CODECS	2

#define VXP_MAX_PERIODS	32

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

struct snd_vxpocket {
	/* pcmcia stuff */
	dev_link_t link;
	dev_node_t node;

	/* board-speicific entry */
	struct snd_vxp_entry *hw;

	/* ALSA stuff */
	snd_card_t *card;
	snd_pcm_t *pcm[VXP_MAX_CODECS];
	snd_info_entry_t *proc_entry;

	int index;
	unsigned long port;
	int irq;

	spinlock_t lock;
	spinlock_t irq_lock;
	struct tasklet_struct tq;

	unsigned int initialized: 1;
	unsigned int in_suspend: 1;
	unsigned int xilinx_tested: 1;
	unsigned int is_stale: 1;

	unsigned int pcm_running;

	int regCDSP;	/* current CDSP register */
	int regDIALOG;	/* current DIALOG register */

	struct vx_rmh irq_rmh;	/* RMH used in interrupts */

	int audio_ins;
	int audio_outs;
	struct vx_pipe **playback_pipes;
	struct vx_pipe **capture_pipes;

	int output_level[VXP_MAX_CODECS][2];	/* analog output level */
	int mic_level;				/* analog mic level (or boost) */
	int audio_gain[2][4];			/* digital audio level (playback/capture) */
	unsigned char audio_active[4];		/* mute/unmute on digital playback */
	int audio_monitor[4];			/* playback hw-monitor level */
	unsigned char audio_monitor_active[4];	/* playback hw-monitor mute/unmute */

	int audio_source;	/* current audio input source */
	int audio_source_target;
	int clock_source;	/* current clock source (INTERNAL_QUARTZ or UER_SYNC) */
	int freq;		/* current frequency */
	int freq_detected;	/* detected frequency from digital in */
	int uer_detected;	/* VXP_UER_MODE_XXX */
	unsigned int uer_bits;	/* IEC958 status bits */
	struct vx_ibl_info ibl;	/* IBL information */
};

/*
 * pcmcia stuff
 */
dev_link_t *snd_vxpocket_attach(struct snd_vxp_entry *hw);
void snd_vxpocket_detach(struct snd_vxp_entry *hw, dev_link_t *link);
void snd_vxpocket_detach_all(struct snd_vxp_entry *hw);

/*
 * core constructor/destructor, exported
 */
vxpocket_t *snd_vxpocket_create_chip(struct snd_vxp_entry *hw);
void snd_vxpocket_free_chip(vxpocket_t *chip);
int snd_vxpocket_assign_resources(vxpocket_t *chip, int port, int irq);
int snd_vxpocket_card_busy(vxpocket_t *chip);

/*
 * mixer
 */
int snd_vxpocket_mixer_new(vxpocket_t *chip);

/*
 * pcm stuff
 */
int snd_vxpocket_pcm_new(vxpocket_t *chip);
void vx_pcm_playback_update(vxpocket_t *chip, snd_pcm_substream_t *subs, vx_pipe_t *pipe);
void vx_pcm_playback_update_buffer(vxpocket_t *chip, snd_pcm_substream_t *subs, vx_pipe_t *pipe);
void vx_pcm_capture_update(vxpocket_t *chip, snd_pcm_substream_t *subs, vx_pipe_t *pipe);

/*
 * interrupt handler
 */
void snd_vx_irq_handler(int irq, void *dev, struct pt_regs *regs); /* exported */
int vx_test_and_ack(vxpocket_t *chip);
void vx_validate_irq(vxpocket_t *chip, int enable);
void vx_interrupt(unsigned long private_data);

/*
 * lowlevel functions
 */
void vx_delay(vxpocket_t *chip, int msec);
int snd_vx_inb(vxpocket_t *chip, int offset);
void snd_vx_outb(vxpocket_t *chip, int offset, int val);
#define vx_inb(chip,reg)	snd_vx_inb(chip, VXP_##reg)
#define vx_outb(chip,reg,val)	snd_vx_outb(chip, VXP_##reg, val)

int vx_send_msg(vxpocket_t *chip, struct vx_rmh *rmh);
int vx_send_msg_nolock(vxpocket_t *chip, struct vx_rmh *rmh);
int vx_send_rih(vxpocket_t *chip, int cmd);
int vx_send_rih_nolock(vxpocket_t *chip, int cmd);

void vx_reset_codec(vxpocket_t *chip);
void vx_toggle_dac_mute(vxpocket_t *chip, int mute);
int vx_sync_audio_source(vxpocket_t *chip);
void vx_reset_audio_levels(vxpocket_t *chip);

void vx_set_iec958_status(vxpocket_t *chip, unsigned int bits);
int vx_set_clock(vxpocket_t *chip, int freq);
void vx_set_internal_clock(vxpocket_t *chip, int freq);

int vx_change_frequency(vxpocket_t *chip);


/* error with hardware code,
 * the return value is -(VXP_ERR_MASK | actual-hw-error-code)
 */
#define VXP_ERR_MASK	0x1000000
#define vx_get_error(err)	(-(err) & ~VXP_ERR_MASK)


/*
 * hardware constants
 */

/* hardware type */
enum {
	VXP_TYPE_VXPOCKET,
	VXP_TYPE_VXP440,
};

/* audio input source */
enum {
	VXP_AUDIO_SRC_LINE = 0x00,
	VXP_AUDIO_SRC_MIC = 0x01,
	VXP_AUDIO_SRC_DIGITAL = 0x02,
};

/* clock source */
enum {
	INTERNAL_QUARTZ,
	UER_SYNC
};

/* SPDIF/UER type */
enum {
	VXP_UER_MODE_CONSUMER,
	VXP_UER_MODE_PROFESSIONAL,
	VXP_UER_MODE_NOT_PRESENT,
};

/* register offsets */
enum {
	VXP_ICR		= 0x00,
	VXP_CVR		= 0x01,
	VXP_ISR		= 0x02,
	VXP_IVR		= 0x03,
	VXP_DMA		= 0x04,
	VXP_RXH		= 0x05,
	VXP_RXM		= 0x06,
	VXP_RXL		= 0x07,
	VXP_TXH		= 0x05,
	VXP_TXM		= 0x06,
	VXP_TXL		= 0x07,
	VXP_CDSP	= 0x08,
	VXP_LOFREQ	= 0x09,
	VXP_HIFREQ	= 0x0a,
	VXP_DATA	= 0x0b,
	VXP_MICRO	= 0x0c,
	VXP_CODEC2	= VXP_MICRO,	/* 2nd codec, VXP440 only */
	VXP_DIALOG	= 0x0d,
	VXP_CSUER	= 0x0e,
	VXP_RUER	= 0x0f,
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

/* DSP Interrupt Request values */
/* add 0x40 offset for vxpocket */
#define VXP_IRQ_OFFSET		0x40
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

/* VXPOCKET */
/* Constants used to access the CDSP register (0x08). */
#define CDSP_MAGIC	0xA7	/* magic value (for read) */
/* for write */
#define VXP_CDSP_CLOCKIN_SEL_MASK	0x80	/* 0 (internal), 1 (AES/EBU) */
#define VXP_CDSP_DATAIN_SEL_MASK	0x40	/* 0 (analog), 1 (UER) */
#define VXP_CDSP_SMPTE_SEL_MASK		0x20
#define VXP_CDSP_RESERVED_MASK		0x10
#define VXP_CDSP_MIC_SEL_MASK		0x08
#define VXP_CDSP_VALID_IRQ_MASK		0x04
#define VXP_CDSP_CODEC_RESET_MASK	0x02
#define VXP_CDSP_DSP_RESET_MASK		0x01
/* VXPOCKET 240/440 */
#define P24_CDSP_MICS_SEL_MASK		0x18
#define P24_CDSP_MIC20_SEL_MASK		0x10
#define P24_CDSP_MIC38_SEL_MASK		0x08

/* Constants used to access the MEMIRQ register (0x0C). */
#define P44_MEMIRQ_MASTER_SLAVE_SEL_MASK 0x08
#define P44_MEMIRQ_SYNCED_ALONE_SEL_MASK 0x04
#define P44_MEMIRQ_WCLK_OUT_IN_SEL_MASK  0x02 /* Not used */
#define P44_MEMIRQ_WCLK_UER_SEL_MASK     0x01 /* Not used */

/* Constants used to access the LOFREQ and HIFREQ registers (0x09 et 0x0A). */
#define VXP_LOFREQ_VAL(v)	((v) & 0x0000ff)
#define VXP_HIFREQ_VAL(v)	(((v) & 0x000f00)>>8)

/* Constants used to access the DATA register (0x0B). */
#define VXP_DATA_CODEC_MASK	0x80
#define VXP_DATA_XICOR_MASK	0x80

/* Micro levels (0x0C) */

/* Constants used to access the DIALOG register (0x0D). */
#define VXP_DLG_XILINX_REPROG_MASK	0x80	/* W */
#define VXP_DLG_DATA_XICOR_MASK		0x80	/* R */
#define VXP_DLG_RESERVED4_0_MASK	0x40
#define VXP_DLG_RESERVED2_0_MASK	0x20
#define VXP_DLG_RESERVED1_0_MASK	0x10
#define VXP_DLG_DMAWRITE_SEL_MASK	0x08	/* W */
#define VXP_DLG_DMAREAD_SEL_MASK	0x04	/* W */
#define VXP_DLG_MEMIRQ_MASK		0x02	/* R */
#define VXP_DLG_DMA16_SEL_MASK		0x02	/* W */
#define VXP_DLG_ACK_MEMIRQ_MASK		0x01	/* R/W */

/* Constants used to access the CSUER register (0x0E) */
#define VXP_SUER_FREQ_MASK		0x0C
#define VXP_SUER_FREQ_32KHz_MASK	0x0C
#define VXP_SUER_FREQ_44KHz_MASK	0x00
#define VXP_SUER_FREQ_48KHz_MASK	0x04
#define VXP_SUER_DATA_PRESENT_MASK	0x02
#define VXP_SUER_CLOCK_PRESENT_MASK	0x01

#define VXP_CUER_HH_BITC_SEL_MASK	0x00000008    // then read data in RUER
#define VXP_CUER_MH_BITC_SEL_MASK	0x00000004    // then read data in RUER
#define VXP_CUER_ML_BITC_SEL_MASK	0x00000002    // then read data in RUER
#define VXP_CUER_LL_BITC_SEL_MASK	0x00000001    // then read data in RUER

#define XX_UER_CBITS_OFFSET_MASK	0x1F


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


#endif /* __VXPOCKET_H */
