#ifndef __SND_CMI8788__H__
#define __SND_CMI8788__H__

/* #define  CMI_DEBUG */

#ifdef CMI_DEBUG
#define  cmi_printk(_x_)  printk _x_
#else
#define  cmi_printk(_x_)
#endif


/* max number of PCM devics per card */
#define CMI8788_MAX_PCMS 4

#define NORMAL_PCMS      0
#define AC97_PCMS        1
#define SPDIF_PCMS       2

#define CMI_PLAYBACK     0
#define CMI_CAPTURE      1

/* playback volume */
#define PLAYBACK_MASTER_VOL    0
#define PLAYBACK_FRONT_VOL     1
#define PLAYBACK_SIDE_VOL      2
#define PLAYBACK_CENTER_VOL    3
#define PLAYBACK_BACK_VOL      4

/* capture volume */
#define CAPTURE_MIC_VOL        0
#define CAPTURE_LINEIN_VOL     1
#define CAPTURE_AC97LINE_VOL   2

/* capture source */
#define CAPTURE_AC97_MIC        0
#define CAPTURE_DIRECT_LINE_IN  1
#define CAPTURE_AC97_LINEIN     2
#define CAPTURE_MAX_SOURCE      2 /* old 3 */

/* ac97 volumes */
#define MASTER_VOL_SLIDER     0
#define PCBEEP_VOL_SLIDER     1
#define MIC_VOL_SLIDER        2
#define LINEIN_VOL_SLIDER     3
#define CD_VOL_SLIDER         4
#define VIDEO_VOL_SLIDER      5
#define AUX_VOL_SLIDER        6
#define MAX_VOL_SLIDER        7


/* CMI8788 IC_revision */
#define CMI8788IC_Revision1     1
#define CMI8788IC_Revision2     2

/* register map */

/* PCI Application Register Map */

/* PCI DMA Control Registers */
#define PCI_DMARec_A_BaseAddr    0x00   /* PCI DMA Recording Channel A Base/Current Address Register        4Byte*/
#define PCI_DMARec_A_BaseCount   0x04   /* PCI DMA Recording Channel A Base/Current Count Register          2Byte*/
#define PCI_DMARec_A_BaseTCount  0x06   /* PCI DMA Recording Channel A Base/Current Terminal Count Register 2Byte*/

#define PCI_DMARec_B_BaseAddr    0x08   /* PCI DMA Recording Channel B Base/Current Address Register        4Byte*/
#define PCI_DMARec_B_BaseCount   0x0C   /* PCI DMA Recording Channel B Base/Current Count Register          2Byte*/
#define PCI_DMARec_B_BaseTCount  0x0E   /* PCI DMA Recording Channel B Base/Current Terminal Count Register 2Byte*/

#define PCI_DMARec_C_BaseAddr    0x10   /* PCI DMA Recording Channel C Base/Current Address Register        4Byte*/
#define PCI_DMARec_C_BaseCount   0x14   /* PCI DMA Recording Channel C Base/Current Count Register          2Byte*/
#define PCI_DMARec_C_BaseTCount  0x16   /* PCI DMA Recording Channel C Base/Current Terminal Count Register 2Byte*/

#define PCI_DMAPlay_SPDIF_BaseAddr   0x18 /* PCI DMA S/PDIF Playback Base/Current Address Register        4Byte*/
#define PCI_DMAPlay_SPDIF_BaseCount  0x1C /* PCI DMA S/PDIF Playback Base/Current Count Register          2Byte*/
#define PCI_DMAPlay_SPDIF_BaseTCount 0x1E /* PCI DMA S/PDIF Playback Base/Current Terminal Count Register 2Byte*/

#define PCI_DMAPlay_MULTI_BaseAddr   0x20 /* PCI DMA Multi-Channel Playback Base/Current Address Register        4Byte*/
#define PCI_DMAPlay_MULTI_BaseCount  0x24 /* PCI DMA Multi-Channel Playback Base/Current Count Register          4Byte*/
#define PCI_DMAPlay_MUTLI_BaseTCount 0x28 /* PCI DMA Multi-Channel Playback Base/Current Terminal Count Register 4Byte*/

#define PCI_DMAPlay_Front_BaseAddr   0x30 /* PCI DMA Front Panel Playback Base/Current Address Register        4Byte*/
#define PCI_DMAPlay_Front_BaseCount  0x34 /* PCI DMA Front Panel layback Base/Current Count Register           2Byte*/
#define PCI_DMAPlay_Front_BaseTCount 0x36 /* PCI DMA Front Panel Playback Base/Current Terminal Count Register 2Byte*/

#define PCI_DMA_SetStatus    0x40   /* PCI DMA Channel Start/Pause/Stop  2Byte  ÉèÖÃ 6žö DMA Channel µÄ×ŽÌ¬ */
#define PCI_DMA_Reset        0x42   /* PCI DMA Channel Reset 1Byte  reset 6žö DMA Channel µÄ×ŽÌ¬ */
#define PCI_MULTI_DMA_MODE   0x43   /* Multi-Channel DMA Mode 1Byte  set Multi DMA n(2-8) Channel mode */

#define PCI_IntMask          0x44   /* Interrupt Mask Register 2Byte*/
#define PCI_IntStatus        0x46   /* Interrupt Status Register 2Byte*/

#define PCI_Misc             0x48   /* Miscellaneous Register 1Byte*/

#define PCI_RecSampleFmtCvt  0x4A   /* Sample Format Convert for Recording Channels (A,B,C) 1Byte*/
#define PCI_PlaySampleFmCvt  0x4B   /* Sample Format Convert for Playback Channels(Multi and SPDIF) 1Byte*/
#define PCI_RecDMA_Mode      0x4C   /* Recording Channels(A,B,C) DMA Mode 1Byte  Set Recording Channels(A,B,C) DMA 6-2 Channels */

#define PCI_Fun              0x50   /* Function Register 1Byte*/

/* I2S Bus Control Registers */
#define I2S_Multi_DAC_Fmt    0x60   /* I2S Multi-Channel DAC Format Register 2Byte*/
#define I2S_ADC1_Fmt         0x62   /* I2S ADC 1 Format Register 2Byte*/
#define I2S_ADC2_Fmt         0x64   /* I2S ADC 2 Format Register 2Byte*/
#define I2S_ADC3_Fmt         0x66   /* I2S ADC 3 Format Register 2Byte*/

/* 2-Wire Master Serial Bus for Codec */
#define SlaveAddrCtrl        0x90   /* Slave Device Address and Read/Write Control Register 1Byte*/
#define MAPReg               0x91   /* Memory Address Pointer (MAP) of Slave Device 1Byte*/
#define DataReg              0x92   /* Data Register 2Byte*/
#define BusCtrlStatus        0x94   /* 2-Wire Serial Bus Control and Status Register 2Byte*/

/* SPI */
#define SPI_Ctrl             0x98   /* SPI Control Register 1Byte*/
#define SPI_Data             0x99   /* SPI Data Register 3Byte */
                                    /*   The data (which include address, r/w, and data bits)*/
                                    /*   written to or read from the codec*/


/* Mixer */
#define Mixer_PlayRouting      0xC0   /* Playback Routing Register 2Byte*/
#define Mixer_RecRouting       0xC2   /* Recording Routing Register 2Byte*/
#define Mixer_ADCMonitorCtrl   0xC3   /* ADC Monitoring Control Register 1 Byte*/
#define Mixer_RoutOfRecMoniter 0xC4   /* Routing of Monitoring of Recording Channel A Register 1Byte*/

/* AC'97 Controller Interface */
#define AC97StatuCtrl        0xD0   /* AC'97 Controller Status/Control Register 2Byte*/
#define AC97IntMask          0xD2   /* AC'97 CODEC Interrupt Mask Register 1Byte*/
#define AC97IntStatus        0xD3   /* AC'97 Interrupt Status Register 1Byte*/
#define AC97OutChanCfg       0xD4   /* AC'97 Output Channel Configuration Register 4Byte*/
#define AC97InChanCfg1       0xD8   /* AC'97 Input Channel Configuration Register 4Byte*/
#define AC97InChanCfg2       0xDC   /* AC'97 Input Channel Configuration Register 4Byte*/

/* Miscellaneous Registers */
#define PCI_DMA_FLUSH        0xE1   /* 1Byte*/
#define PCI_RevisionRegister 0xE6   /* PCI E6: Revision Register 2Byte*/

#define CODEC_ADR_AK4396     0x00  /* Žý¶š */
#define CODEC_ADR_WM8776     0x34  /* or 0x36 */
#define CODEC_ADR_WM8785     0x1A
#define CODEC_ADR_AK5385A    0x00  /* Žý¶š */


/* MPU401 Interface */


struct stru_cmi8788_controller;
typedef struct stru_cmi8788_controller cmi8788_controller;

typedef struct stru_cmi_substream {
	struct snd_pcm_substream *substream;
	int running;      /* dac/adc running? */

	u32 dma_area;     /* virtual address of the dma buffer */
	u32 dma_addr;     /* physical address of the dma buffer */
	u32 dma_bytes;    /* size of dma buffer */
	u32 fragsize;     /* in Byte */

	u32 rate;         /* sample rate */
	u16 channels;     /* channel number */
	u8  frame_bits;   /* sample size */
	u8  sample_bits;  /* */

	int DMA_sta_mask;  /* PCI 40: PCI DMA Channel Start/Pause/Stop 2Byte*/
	int DMA_chan_reset;/* PCI 42: PCI DMA Channel Reset            1Byte*/
	int int_mask;      /* PCI 44: Interrupt Mask Register          2Byte*/
	int int_sta_mask;  /* PCI 46: interrupt status mask            2Byte*/
} cmi_substream;

typedef struct snd_stru_cmipci_pcm {
	cmi_substream  cmi_subs[2]; /* 0 playback; 1 record; */
} cmipci_pcm_t;

/* for record */
#define CMI8788_MAX_NUM_INPUTS	4
struct cmi8788_input_mux_item {
	const char *label;
	unsigned int index;
};

struct cmi8788_input_mux {
	unsigned int num_items;
	struct cmi8788_input_mux_item items[CMI8788_MAX_NUM_INPUTS];
};

typedef struct stru_snd_cmi8788 {
	struct snd_card *card;
	struct pci_dev *pci;

	/* pci resources */
	unsigned long   addr;
	int             irq;

	/* locks */
	spinlock_t reg_lock;
	struct semaphore open_mutex;

	/* PCM */
	int PCM_Count;
	struct snd_pcm *pcm[CMI8788_MAX_PCMS];
	cmipci_pcm_t  cmi_pcm[CMI8788_MAX_PCMS];


	/* CMI8788 controller */
	cmi8788_controller *controller;

	u16 codec_mask;
	u8  playback_volume_init;
	u8  capture_volume_init;
	u8  first_set_playback_volume;
	u8  first_set_capture_volume;

	u8  capture_source; /* 0-AC97 Mic; 1-Direct Line in; 2-AC97 Line In; */

	/* CMI8788 IC revision*/
	u8  CMI8788IC_revision;
} snd_cmi8788;

#endif
