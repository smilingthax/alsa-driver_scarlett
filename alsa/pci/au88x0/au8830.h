/*
    Aureal Vortex Soundcard driver.

    IO addr collected from asp4core.vxd:
    function    address
    0005D5A0    13004
    00080674    14004
    00080AFF    12818

 */

#define CHIP_AU8830

#define CARD_NAME "Aureal Vortex 2 3D Sound Processor"
#define CARD_NAME_SHORT "au8830"

#ifndef PCI_VENDOR_ID_AUREAL
#define PCI_VENDOR_ID_AUREAL 0x12eb
#endif
#ifndef PCI_VENDOR_ID_AUREAL_VORTEX2
#define PCI_DEVICE_ID_AUREAL_VORTEX2 0x0002
#endif

#define hwread(x,y) readl((x)+((y)>>2))
#define hwwrite(x,y,z) writel((z),(x)+((y)>>2))

#define NR_ADB 0x20
#define NR_SRC 0x10
#define NR_A3D 0x10
#define NR_MIXIN 0x20
#define NR_MIXOUT 0x10
#define NR_WT 0x40

/* ADBDMA */
#define VORTEX_ADBDMA_STAT 0x27e00 /* read only, subbuffer, DMA pos */
#define		POS_MASK 0x00000fff
#define     POS_SHIFT 0x0
#define 	ADB_SUBBUF_MASK 0x00003000  /* ADB only. */
#define     ADB_SUBBUF_SHIFT 0xc        /* ADB only. */
#define VORTEX_ADBDMA_CTRL 0x27a00 		/* write only; format, flags, DMA pos */
#define		OFFSET_MASK 0x00000fff
#define     OFFSET_SHIFT 0x0
#define		IE_MASK 0x00001000 /* interrupt enable. */
#define     IE_SHIFT 0xc
#define     U_MASK 0x00002000  /* Unknown. If you know, tell me. */
#define     U_SHIFT 0xd
#define		FMT_MASK 0x0003c000
#define		FMT_SHIFT 0xe
#define		ADB_FIFO_EN_SHIFT	0x15
#define		ADB_FIFO_EN			(1 << 0x15)
// The ADB masks and shift also are valid for the wtdma, except if specified otherwise.
#define VORTEX_ADBDMA_BUFCFG0 0x27800
#define VORTEX_ADBDMA_BUFCFG1 0x27804
#define VORTEX_ADBDMA_BUFBASE 0x27400
#define VORTEX_ADBDMA_START 0x27c00 /* Which subbuffer starts */

/* WTDMA */
#define VORTEX_WTDMA_CTRL 0x27900 /* format, DMA pos */
#define VORTEX_WTDMA_STAT 0x27d00 /* DMA subbuf, DMA pos */
#define     WT_SUBBUF_MASK 0x3
#define     WT_SUBBUF_SHIFT 0xc
#define VORTEX_WTDMA_BUFBASE 0x27000
#define VORTEX_WTDMA_BUFCFG0 0x27600
#define VORTEX_WTDMA_BUFCFG1 0x27604
#define VORTEX_WTDMA_START 0x27b00 /* which subbuffer is first */

#define VORTEX_WT_BASE (0x420 << 7)

/* ADB */
#define VORTEX_ADB_SR 0x28400 /* Samplerates enable/disable */
#define VORTEX_ADB_RTBASE 0x28000
#define VORTEX_ADB_RTBASE_SIZE (VORTEX_ADB_CHNBASE - VORTEX_ADB_RTBASE)
#define VORTEX_ADB_CHNBASE 0x282b4
#define VORTEX_ADB_CHNBASE_SIZE (ADB_MASK - VORTEX_ADB_RTBASE_SIZE)
#define 	ROUTE_MASK	0xffff
#define		SOURCE_MASK	0xff00
#define     ADB_MASK   0xff
#define		ADB_SHIFT 0x8
/* ADB address */
#define		OFFSET_ADBDMA	0x00
#define		OFFSET_SRCIN	0x40
#define		OFFSET_SRCOUT	0x20 /* ch 0x11 */
#define		OFFSET_MIXIN	0x50 /* ch 0x11 */
#define		OFFSET_MIXOUT	0x30 /* ch 0x11 */
#define		OFFSET_CODECIN	0x70 /* ch 0x11 */ /* adb source */
#define		OFFSET_CODECOUT	0x88 /* ch 0x11 */ /* adb target */
#define		OFFSET_SPORTIN	0x78 /* ch 0x13 */
#define		OFFSET_SPORTOUT	0x90 /* ch 0x13 */
#define		OFFSET_SPDIFOUT	0x92 /* ch 0x14 */
#define		OFFSET_EQIN		0xa0 /* ch 0x11 */
#define		OFFSET_EQOUT	0x7e /* ch 0x11 */ /* 2 routes on ch 0x11 */
#define		OFFSET_WTOUT	0x62 /* 0x64, 0x65, 0xA2, 0xA4, 0xA5 */
#define		OFFSET_UNKNOWNOUT0	0x66 /* ch 0x11 */
#define		OFFSET_UNKNOWNOUT1	0x67 /* ch 0x11 */
#define		OFFSET_UNKNOWNIN0	0x96 /* ch 0x11 */
#define		OFFSET_UNKNOWNIN1	0x9b /* ch 0x11 */

/* ADB route translate helper */
#define ADB_DMA(x) (x)
#define ADB_SRCOUT(x) (x + OFFSET_SRCOUT)
#define ADB_SRCIN(x) (x + OFFSET_SRCIN)
#define ADB_MIXOUT(x) (x + OFFSET_MIXOUT)
#define ADB_MIXIN(x) (x + OFFSET_MIXIN)
#define ADB_CODECIN(x) (x + OFFSET_CODECIN)
#define ADB_CODECOUT(x) (x + OFFSET_CODECOUT)
#define ADB_SPORTIN(x) (x + OFFSET_SPORTIN)
#define ADB_SPORTOUT(x) (x + OFFSET_SPORTOUT)
#define ADB_SPDIFOUT(x)	(x + OFFSET_SPDIFOUT)
#define ADB_EQIN(x) (x + OFFSET_EQIN)
#define ADB_EQOUT(x) (x + OFFSET_EQOUT)
#define ADB_A3DOUT(x) (x + 0x50) /* 0x10 A3D blocks */
#define ADB_A3DIN(x) (x + 0x70)
#define ADB_WTOUT(x) (x + OFFSET_WTOUT)

#define MIX_DEFIGAIN 0x00
#define MIX_DEFOGAIN 0x00 /* 0x8->6dB  (6dB = x4) 16 to 18 bit conversion? */

/* MIXER */
#define VORTEX_MIXER_SR 0x21f00
#define VORTEX_MIXER_CHNBASE 0x21e40
#define VORTEX_MIXER_RTBASE 0x21e00
#define 	MIXER_RTBASE_SIZE 0x38
#define VORTEX_MIX_U0 0x21c00 /* AU8820: 0x9c00 */

/* MIX */
#define VORTEX_MIX_INVOL_A 0x21000 /* in? */
#define VORTEX_MIX_INVOL_B 0x20000 /* out? */
#define VORTEX_MIX_VOL_A 0x21800
#define VORTEX_MIX_VOL_B 0x20800
#define VORTEX_MIX_ENIN 0x21a00 /* Input enable bits. 4 bits wide. */
#define 	VOL_MIN 0x80 /* Input volume when muted. */
#define		VOL_MAX 0x7f /* FIXME: Not confirmed! Just guessed. */

/* SRC */
#define VORTEX_SRCBLOCK_SR	0x26cc0
#define VORTEX_SRC_CHNBASE	0x26c40
#define VORTEX_SRC_RTBASE	0x26c00
#define VORTEX_SRC_SOURCE	0x26cc4
#define VORTEX_SRC_CONVRATIO 0x26e40
#define VORTEX_SRC_DRIFT0	0x26e80
#define VORTEX_SRC_DRIFT1	0x26ec0
#define VORTEX_SRC_DRIFT2	0x26f40
#define VORTEX_SRC_U0		0x26e00
#define		U0_SLOWLOCK		0x200
#define VORTEX_SRC_U1		0x26f00
#define VORTEX_SRC_U3		0x26f80
#define VORTEX_SRC_DATA		0x26800 /* 0xc800 */
#define VORTEX_SRC_DATA0	0x26000

/* FIFO */
#define VORTEX_FIFO_ADBCTRL 0x16100 /* Control bits. */
#define VORTEX_FIFO_WTCTRL 0x16000
#define		FIFO_RDONLY	0x00000001
#define		FIFO_CTRL	0x00000002 /* Allow ctrl. ? */
#define		FIFO_VALID	0x00000010
#define 	FIFO_EMPTY	0x00000020
#define		FIFO_U0		0x00002000 /* Unknown. */
#define		FIFO_U1		0x00040000
#define		FIFO_SIZE_BITS 6
#define		FIFO_SIZE	(1<<(FIFO_SIZE_BITS)) // 0x40
#define 	FIFO_MASK	(FIFO_SIZE-1) //0x3f	/* at shift left 0xc */
#define 	FIFO_BITS	0x1c400000
#define VORTEX_FIFO_ADBDATA 0x14000
#define VORTEX_FIFO_WTDATA 0x10000

/* CODEC */
#define VORTEX_CODEC_CTRL 0x29184
#define VORTEX_CODEC_EN 0x29190
#define		EN_AUDIO0		0x00000300
#define		EN_MODEM		0x00000c00
#define		EN_AUDIO1		0x00003000
#define		EN_SPORT		0x00030000
#define		EN_SPDIF		0x000c0000
#define		EN_CODEC		(EN_AUDIO1 | EN_AUDIO0)
#define VORTEX_CODEC_CHN 0x29080 /* The name "CHN" is wrong. */
#define VORTEX_CODEC_WRITE 0x00800000
#define VORTEX_CODEC_ADDSHIFT 16
#define VORTEX_CODEC_ADDMASK 0x7f0000 /* 0x000f0000*/
#define VORTEX_CODEC_DATSHIFT 0
#define VORTEX_CODEC_DATMASK 0xffff
#define VORTEX_CODEC_IO 0x29188

#define VORTEX_SPDIF_FLAGS		0x2205c
#define VORTEX_SPDIF_CFG0		0x291D0
#define VORTEX_SPDIF_CFG1		0x291D4
#define VORTEX_SPDIF_SMPRATE	0x29194

/* Sample timer */
#define VORTEX_SMP_TIME  0x29198

/* IRQ */
#define VORTEX_IRQ_SOURCE 0x2a000 /* Interrupt source flags. */
#define 	IRQ_FATAL    0x0001
#define 	IRQ_PARITY   0x0002
#define 	IRQ_PCMOUT   0x0020 /* ?? */
#define 	IRQ_TIMER    0x1000
#define 	IRQ_MIDI     0x2000
#define		IRQ_MODEM	0x4000

#define VORTEX_IRQ_U0	0x2a008 /* ?? */

#define VORTEX_CTRL		0x2a00c
#define 	CTRL_MIDI_EN	0x00000001
#define 	CTRL_MIDI_PORT	0x00000060
#define 	CTRL_GAME_EN	0x00000008
#define 	CTRL_GAME_PORT	0x00000e00
#define		CTRL_SPDIF		0x00000000 /* unknown. Please find this value*/
#define 	CTRL_SPORT		0x00200000
#define 	CTRL_UNKNOWN	0x01000000
#define 	CTRL_IRQ_ENABLE	0x00004000

#define VORTEX_IRQ_CTRL 0x2a004 /* Interrupt source mask. */
#define 	IRQ_FATAL	0x0001
#define 	IRQ_PARITY	0x0002
#define 	IRQ_PCMOUT	0x0020 /* PCM OUT page crossing */
#define 	IRQ_TIMER	0x1000
#define 	IRQ_MIDI	0x2000
#define		IRQ_MODEM	0x4000

/* write: Timer period config / read: TIMER IRQ ack. */
#define VORTEX_IRQ_STAT 0x2919c

/* DMA */
#define VORTEX_ENGINE_CTRL 0x27ae8
#define 	ENGINE_INIT 0x1380000

/* MIDI */ /* GAME. */
#define VORTEX_MIDI_DATA 0x28800
#define VORTEX_MIDI_CMD 0x28804  /* Write command / Read status */

#define VORTEX_CTRL2 0x2880c
#define		CTRL2_GAME_ADCMODE 0x40
#define VORTEX_GAME_LEGACY 0x28808
#define VORTEX_GAME_AXIS 0x28810
#define		AXIS_SIZE 4
#define		AXIS_RANGE 0x1fff
