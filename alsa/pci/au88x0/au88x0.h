/*
    Aureal Vortex Soundcard driver.

    IO addr collected from asp4core.vxd:
    function    address
    0005D5A0    13004
    00080674    14004
    00080AFF    12818

 */

#ifndef __SOUND_AU88X0_H
#define __SOUND_AU88X0_H

#ifdef __KERNEL__
#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/rawmidi.h>
#include <sound/mpu401.h>
#include <sound/hwdep.h>
#include <sound/ac97_codec.h>
#include <asm/io.h>
#include <linux/init.h>
#include "au88x0_eq.h"

/*
#ifndef	PCI_VENDOR_ID_AUREAL
#define	PCI_VENDOR_ID_AUREAL 0x12eb
#endif
#ifndef	PCI_VENDOR_ID_AUREAL_VORTEX
#define	PCI_DEVICE_ID_AUREAL_VORTEX 0x0001
#endif
#ifndef	PCI_VENDOR_ID_AUREAL_VORTEX2
#define	PCI_DEVICE_ID_AUREAL_VORTEX2 0x0002
#endif
#ifndef	PCI_VENDOR_ID_AUREAL_ADVANTAGE
#define	PCI_DEVICE_ID_AUREAL_ADVANTAGE 0x0003
#endif
*/
#endif

#define	VORTEX_DMA_MASK	0xffffffff

#define	hwread(x,y) readl((x)+((y)>>2))
#define	hwwrite(x,y,z) writel((z),(x)+((y)>>2))

/* Vortex MPU401 defines. */
#define	MIDI_CLOCK_DIV		0x61
/* Standart MPU401 defines. */
#define	MPU401_RESET		0xff
#define	MPU401_ENTER_UART	0x3f
#define	MPU401_ACK		0xfe

// Get src register value to convert from x to y.
#define	SRC_RATIO(x,y)		((((x<<15)/y) + 1)/2)

/* FIFO software state constants. */
#define FIFO_STOP 0
#define FIFO_START 1
#define FIFO_PAUSE 2

/* ADB Resource */
#define VORTEX_RESOURCE_DMA		0x00000000
#define VORTEX_RESOURCE_SRC		0x00000001
#define VORTEX_RESOURCE_MIXIN	0x00000002
#define VORTEX_RESOURCE_MIXOUT	0x00000003
#define VORTEX_RESOURCE_A3D		0x00000004
#define VORTEX_RESOURCE_LAST	0x00000005

/* Check for SDAC bit in "Extended audio ID" AC97 register */
#define VORTEX_IS_QUAD(x) ((x->codec == NULL) ?  0 : (x->codec->ext_id|0x80))

/* PCM devices */
#define VORTEX_PCM_ADB		0
#define VORTEX_PCM_SPDIF	1
#define VORTEX_PCM_I2S		2
#define VORTEX_PCM_A3D		3
#define VORTEX_PCM_WT		4
#define VORTEX_PCM_LAST		5

#define MIX_CAPT(x) (vortex->mixcapt[x])
#define MIX_PLAYB(x) (vortex->mixplayb[x])
#define MIX_SPDIF(x) (vortex->mixspdif[x])

/* Structs */
typedef struct {
    int fifo_enabled;   /* this_24 */
    int fifo_status;    /* this_1c */
    int dma_ctrl;       /* this_78 (ADB), this_7c (WT) */
    int dma_unknown;    /* this_74 (ADB), this_78 (WT) */
    int cfg0;
    int cfg1;

    int nr_ch;		/* Nr of PCM channels */
	int type;		/* Output type (ac97, spdif, i2s, dsp)*/
	int dma;		/* Hardware DMA index. */
	int dir;		/* Stream Direction. */
	u32 resources[5];
	
	/* Virtual page extender stuff */
	int nr_periods;
	int period_bytes;
	unsigned long buf_addr;
	int period_real;
	int period_virt;

    snd_pcm_substream_t *substream;
} stream_t;

typedef struct snd_vortex vortex_t;
struct snd_vortex {
    /* ALSA structs. */
	snd_card_t *card;
	snd_pcm_t *pcm[VORTEX_PCM_LAST];
	
    snd_rawmidi_t *rmidi;   /* Legacy Midi interface. */
	ac97_t *codec;

	/* Stream structs. */
    stream_t dma_adb[NR_ADB];
	int spdif_sr;
#ifndef CHIP_AU8810
    stream_t dma_wt[NR_WT];
	unsigned char mixwt[6];	/* WT mixin objects */
#endif
	/* Global resources */
	unsigned char mixcapt[2];
	unsigned char mixplayb[4];
	unsigned char mixspdif[2];
	u32 fixed_res[5];
	
	/* Hardware equalizer structs */
	eqlzr_t eq; 
	
	/* Extra controls */
	//snd_kcontrol_t *eqctrl[20];
	
	/* Gameport stuff. */
	struct gameport *gameport;

	/* PCI hardware resources */
	unsigned long io;
	unsigned long *mmio;
	unsigned int irq;
    spinlock_t lock;

	/* PCI device */
	struct pci_dev * pci_dev;
	u16 vendor;
	u16 device;
	u8 rev;
};

#define chip_t vortex_t

/* Functions. */

/* SRC */
void vortex_adb_setsrc(vortex_t *vortex, int adbdma, unsigned int cvrt, int dir);

/* DMA Engines. */
void vortex_adbdma_setbuffers(vortex_t *vortex, int adbdma, unsigned int addr, int size, int count);
void vortex_wtdma_setbuffers(vortex_t *vortex, int wtdma, unsigned int addr, int size, int count);
void vortex_adbdma_setmode(vortex_t *vortex, int adbdma, int ie, int b, int fmt, int d, unsigned long offset);
void vortex_wtdma_setmode(vortex_t *vortex, int wtdma, int ie, int b, int fmt, int d, unsigned long offset);
void vortex_adbdma_setstartbuffer(vortex_t *vortex, int adbdma, int sb);
void vortex_wtdma_setstartbuffer(vortex_t *vortex, int wtdma, int sb);

void vortex_adbdma_startfifo(vortex_t *vortex, int adbdma);
void vortex_adbdma_stopfifo(vortex_t *vortex, int adbdma);
void vortex_wtdma_startfifo(vortex_t *vortex, int wtdma);
void vortex_wtdma_stopfifo(vortex_t *vortex, int wtdma);
void vortex_adbdma_pausefifo(vortex_t *vortex, int adbdma);
void vortex_adbdma_resumefifo(vortex_t *vortex, int adbdma);
void vortex_wtdma_pausefifo(vortex_t *vortex, int wtdma);
void vortex_wtdma_resumefifo(vortex_t *vortex, int wtdma);

int inline vortex_adbdma_getlinearpos(vortex_t *vortex, int adbdma);
int inline vortex_wtdma_getlinearpos(vortex_t *vortex, int wtdma);

/* global stuff. */
void vortex_codec_init(vortex_t *vortex);
void vortex_codec_write(ac97_t *codec, unsigned short addr,unsigned short data);
unsigned short vortex_codec_read(ac97_t *codec, unsigned short addr);
void vortex_spdif_init(vortex_t *vortex, int spdif_sr, int spdif_mode);

int  vortex_core_init(vortex_t *card);
int  vortex_core_shutdown(vortex_t *card);
void vortex_enable_int(vortex_t *card);
irqreturn_t vortex_interrupt(int irq, void *dev_id, struct pt_regs *regs);
int  vortex_alsafmt_aspfmt(int alsafmt);

/* Connection  stuff. */
void vortex_connect_default(vortex_t *vortex, int en);
int  vortex_adb_allocroute(vortex_t *vortex, int dma, int nr_ch, int dir, int type);
int  vortex_wt_allocroute(vortex_t *vortex, int dma, int nr_ch);
int  vortex_adb_checkinout(vortex_t *vortex, int resmap[], int out, int restype);
void vortex_wt_connect(vortex_t *vortex, int en, unsigned char mixers[]);
void vortex_wt_InitializeWTRegs(vortex_t * vortex);

/* Driver stuff. */
int vortex_gameport_register(vortex_t *card);
int vortex_gameport_unregister(vortex_t *card);
int vortex_eq_init(vortex_t *vortex);
int vortex_eq_free(vortex_t *vortex);
/* ALSA stuff. */
int snd_vortex_new_pcm(vortex_t *vortex, int idx, int nr);
int snd_vortex_mixer(vortex_t *vortex);
int snd_vortex_midi(vortex_t *vortex);
#endif
