#ifndef RME9652_H
#define RME9652_H

/*
 *   Copyright (c) 1999 IEM - Winfried Ritsch
 *   portions are Copyright (c) 1999 Paul Barton-Davis (ported to ALSA)
 *
 *   defines for configuration and memory registers 
 *   for RME Card 9653 alias Hammerschlag 
 *   from developer documents from RME from 19.7.99
 *
 *   THIS Info is copyrighted by RME und secret unless released 
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

/* The Hammerfall has two sets of 24 ADAT + 2 S/PDIF channels, one for
   capture, one for playback. Both the ADAT and S/PDIF channels appear
   to the host CPU in the same block of memory. There is no functional
   difference between them in terms of access.
   
   The Hammerfall Light is identical to the Hammerfall, except that it
   has 2 sets 18 channels (16 ADAT + 2 S/PDIF) for capture and playback.
*/

#define RME9652_HAMMERFALL_NCHANNELS       26
#define RME9652_HAMMERFALL_LIGHT_NCHANNELS 18

/* Latency values - used by "latency" control switch */

#define RME9652_LATENCY_64     0
#define RME9652_LATENCY_128    1
#define RME9652_LATENCY_256    2
#define RME9652_LATENCY_512    3
#define RME9652_LATENCY_1024   4
#define RME9652_LATENCY_2048   5
#define RME9652_LATENCY_4096   6
#define RME9652_LATENCY_8192   7

/* S/PDIF output choices - used by "spdif_out" control switch */

#define RME9652_SPDIF_OUT_COAX  0
#define RME9652_SPDIF_OUT_ADAT1 1

/* Preferred sync source choices - used by "sync_pref" control switch */

#define RME9652_SYNC_FROM_ADAT1 0
#define RME9652_SYNC_FROM_ADAT2 1
#define RME9652_SYNC_FROM_ADAT3 2
#define RME9652_SYNC_FROM_SPDIF 3

/* ----------- IOCTL ----------------- */

#define RME9652_IOCTL_MAGIC 0xA9

/* Read */

#define RME9652_IOCTL_LOCK     _IOR(RME9652_IOCTL_MAGIC,1,int)
#define RME9652_IOCTL_SYNC     _IOR(RME9652_IOCTL_MAGIC,2,int)
#define RME9652_IOCTL_WS       _IOR(RME9652_IOCTL_MAGIC,4,int)
#define RME9652_IOCTL_BUFPOS   _IOR(RME9652_IOCTL_MAGIC,5,int)
#define RME9652_IOCTL_ERF      _IOR(RME9652_IOCTL_MAGIC,6,int)
#define RME9652_IOCTL_BUF_HALF _IOR(RME9652_IOCTL_MAGIC,7,int)
#define RME9652_IOCTL_TC_VALID _IOR(RME9652_IOCTL_MAGIC,8,int)
#define RME9652_IOCTL_SPDIF_SR _IOR(RME9652_IOCTL_MAGIC,9,int)
#define RME9652_IOCTL_TIMECODE _IOR(RME9652_IOCTL_MAGIC,98,int)
#define RME9652_IOCTL_STATUS   _IOR(RME9652_IOCTL_MAGIC,99,int)

/* Read/write */

#define RME9652_SPDIFIN_OPTICAL 0 /* SPDIF-IN: optical (ADAT1) */
#define RME9652_SPDIFIN_COAXIAL 1 /* coaxial (Cinch) */
#define RME9652_SPDIFIN_INTERN  2 /* Internal CDROM */

#define RME9652_SPDIFOUT_PROF  0x1
#define RME9652_SPDIFOUT_EMPH  0x2
#define RME9652_SPDIFOUT_DOLBY 0x4
#define RME9652_SPDIFOUT_ADAT1 0x8

/* ------------- Status-Register bits --------------------- */

#define RME9652_IRQ	  0x0000001 /* IRQ is High if not reset by irq_clear*/

#define RME9652_lock_2	  0x0000002 /* ADAT 3-PLL: 1=locked, 0=unlocked */
#define RME9652_lock_1	  0x0000004 /* ADAT 2-PLL: 1=locked, 0=unlocked */
#define RME9652_lock_0	  0x0000008 /* ADAT 1-PLL: 1=locked, 0=unlocked */
#define RME9652_lock	  (RME9652_lock_0|RME9652_lock_1|RME9652_lock_2)

#define RME9652_fs48	  0x0000010 /* sample rate is 0=44.1/88.2,1=48/96 Khz */

#define RME9652_wsel_rd	  0x0000020 /* if Word-Clock is used and valid then 1 */

#define RME9652_buf_pos	  0x000FFC0 /* Bit 6..15 : Position of buffer-pointer in 64Bytes-blocks
				       resolution +/- 1 64Byte/block (since 64Bytes bursts) */

#define RME9652_sync_0	  0x0040000 /* if ADAT-IN 1 in sync to system clock */
#define RME9652_sync_1	  0x0020000 /* if ADAT-IN 2 in sync to system clock */
#define RME9652_sync_2	  0x0010000 /* if ADAT-IN 3 in sync to system clock */
#define RME9652_sync	  (RME9652_sync_0|RME9652_sync_1|RME9652_sync_2)

#define RME9652_DS_rd	  0x0080000 /* 1=Double Speed Mode, 0=Normal Speed */

#define RME9652_tc_busy	  0x0100000 /* 1=time-code copy in progress (960ms) */
#define RME9652_tc_out	  0x0200000 /* time-code out bit */

#define RME9652_F_2	  0x1000000 /*  od external Crystal Chip if ERF=1*/
#define RME9652_F_1	  0x0800000 /*  111=32kHz, 110=44.1kHz, 101=48kHz, */
#define RME9652_F_0	  0x0400000 /*  000=64kHz, 100=88.2kHz, 011=96kHz  */
#define RME9652_F	  (RME9652_F_0|RME9652_F_1|RME9652_F_2)
#define rme9652_decode_spdif_rate(x) ((x)>>22)

#define RME9652_ERF	  0x2000000 /* Error-Flag of SDPIF Receiver (1=No Lock)*/

#define RME9652_buffer_id 0x4000000 /* toggles by each interrupt on rec/play */

#define RME9652_tc_valid  0x8000000 /* 1 = a signal is detected on time-code input */

/*-------------------------- End of Userspace stuff --------------------------- */

#ifdef __KERNEL__

#include "driver.h"
#include "pcm.h"

#ifdef DEBUG
#define PRINTK(format, a...) printk(format, ## a)
#else
#define PRINTK(format, a...) 
#endif

/* for a Hammerfall, /proc/pci says:

   Bus  0, device   9, function  0:
   Multimedia audio controller: Unknown vendor Unknown device (rev 3).
   Vendor id=10ee. Device id=3fc4.
   Slow devsel.  IRQ 9.  Master Capable.  Latency=32.  
   Non-prefetchable 32 bit memory at 0xfd000000 [0xfd000000].
*/

#ifndef PCI_VENDOR_ID_RME
#define PCI_VENDOR_ID_RME		0x10ee
#define PCI_DEVICE_ID_HAMMERFALL	0x3fc4
#endif

#define CARD_NAME	"RME Digi9652"

/* Registers-Space in offsets from base address with size of 16MByte-Space */
/* #define RME9652_IO_EXTENT     16l*1024l*1024l */
#define RME9652_IO_EXTENT     1024

/* Write-only registers */

#define RME9652_num_of_init_regs   8

#define RME9652_init_buffer       (0/4)
#define RME9652_play_buffer       (32/4)  /* holds ptr to 26x64kBit host RAM */
#define RME9652_rec_buffer        (36/4)  /* holds ptr to 26x64kBit host RAM */
#define RME9652_control_register  (64/4)
#define RME9652_irq_clear         (96/4)
#define RME9652_time_code         (100/4) /* useful if used with alesis adat */
#define RME9652_thru_base         (128/4) /* 132...228 Thru for 26 channels */

/* Read-only registers */

/* Writing to any of the register locations writes to the status
   register. We'll use the first location as our point of access.
*/

#define RME9652_status_register    0

/* --------- Control-Register Bits ---------------- */

#define RME9652_start_bit	   0x01    /* start record/play */

/* latency = 512Bytes * 2^n, where n is made from Bit2 ... Bit0 */

#define RME9652_latency0	   0x02  /* latency bit 0 */
#define RME9652_latency1	   0x04  /* latency bit 1 */
#define RME9652_latency2	   0x08  /* latency bit 2 */
#define RME9652_latency            (RME9652_latency0|RME9652_latency1|RME9652_latency2)
#define rme9652_encode_latency(x)  (((x)&0x7)<<1)
#define rme9652_decode_latency(x)  (((x)>>1)&0x7)

#define RME9652_Master		   0x10   /* Clock Mode Master=1,Slave/Auto=0 */
#define RME9652_IE		   0x20   /* Interupt Enable */

#define RME9652_freq		   0x40   /* samplerate 0=44.1/88.2, 1=48/96 kHz*/
#define RME9652_DS                 0x100  /* Doule Speed 0=44.1/48, 1=88.2/96 Khz */

#define RME9652_PRO		   0x200  /* S/PDIF out: 0=consumer, 1=professional */
#define RME9652_EMP		   0x400  /*  Emphasis 0=None, 1=ON */
#define RME9652_Dolby		   0x800  /*  Non-audio bit 1=set, 0=unset */
#define RME9652_opt_out	           0x1000  /* Use 1st optical OUT as SPDIF: 1=yes,0=no */

#define RME9652_wsel		   0x2000  /* use Wordclock as sync (overwrites master)*/

#define RME9652_inp_0		   0x4000  /* SPDIF-IN: 00=optical (ADAT1),     */
#define RME9652_inp_1		   0x8000  /* 01=koaxial (Cinch), 10=Internal CDROM*/
#define RME9652_inp                (RME9652_inp_0|RME9652_inp_1)
#define rme9652_encode_spdif_in(x) (((x)&0x3)<<14)
#define rme9652_decode_spdif_in(x) (((x)>>14)&0x3)

#define RME9652_SyncRef0	   0x10000  /* preferred sync-source in autosync */
#define RME9652_SyncRef1	   0x20000  /* 00=ADAT1,01=ADAT2,10=ADAT3,11=SPDIF */
#define RME9652_SyncRef            (RME9652_SyncRef0|RME9652_SyncRef1)
#define rme9652_encode_sync_src(x) ((((x)&0x3)<<17)
#define rme9652_decode_sync_src(x) (((x)>>17)&0x3)

#define RME9652_PREALLOCATE_MEMORY /* via module or init/main.c hack */

#ifdef RME9652_PREALLOCATE_MEMORY
extern void *			 rme9652_rec_busmaster_memory;
extern void *			 rme9652_play_busmaster_memory;
#endif

/* the size of a subchannel (1 mono data stream) */

#define RME9652_CHANNEL_BUFFER_SAMPLES  (16*1024)
#define RME9652_CHANNEL_BUFFER_BYTES    (4*RME9652_CHANNEL_BUFFER_SAMPLES)

/* the size of the area we need to allocate for DMA transfers on a
   single channel (NOT a subchannel, a channel). 

   Note that we allocate 1 more channel than is apparently needed
   because the h/w seems to write 1 byte beyond the end of the last
   page. Sigh.
*/

#define RME9652_DMA_AREA_BYTES(s) ((s->nchannels+1) * RME9652_CHANNEL_BUFFER_BYTES)
#define RME9652_DMA_AREA_KILOBYTES(s) (RME9652_DMA_AREA_BYTES(s)/1024)

/* we manage DMA memory ourselves */

#define RME9652_DMA_TYPE           SND_DMA_TYPE_PCI

#define RME9652_NUM_OF_FRAGMENTS     2
#define RME9652_MAX_FRAGMENT_SIZE    (RME9652_CHANNEL_BUFFER_BYTES/2)

typedef struct snd_rme9652

{
    spinlock_t    lock;
    snd_irq_t    *irqptr;
    unsigned long port;
    u32          *iobase;	

    u32 control_register;    /* cached value */
    u32 thru_bits;           /* thru 1=on, 0=off channel 1=Bit1... channel 26= Bit26 */

    unsigned int  hw_offsetmask;    /* &-with status register to get real hw_offset */
    int           hw_offset;        /* effective hw-offset due to 64 byte PCI DMA */
    unsigned char hw_fragment:1;    /* which fragment the hw is using, 0 or 1 */
    unsigned int  fragment_bytes;   /* guess ... */

    unsigned char nchannels;        /* different for hammerfall/hammerfall-light */

    unsigned char *capture_buffer;  /* where the big chunk of memory is */
    unsigned char *playback_buffer; /* where the big chunk of memory is */

    unsigned int capture_open_mask;  /* which channels are open for capture */
    unsigned int playback_open_mask; /* which channels are open for playback */

    snd_dma_area_t *playback_dma_areas[RME9652_HAMMERFALL_NCHANNELS];
    snd_dma_area_t *capture_dma_areas[RME9652_HAMMERFALL_NCHANNELS];

    snd_pcm_subchn_t *master_playback_subchn;
    snd_pcm_subchn_t *master_capture_subchn;

    int last_spdif_sample_rate;     /* so that we can catch externally ... */
    int last_adat_sample_rate;      /* ... induced rate changes            */

    snd_card_t         *card;
    snd_pcm_t          *pcm;
    struct pci_dev     *pci;
    snd_info_entry_t   *proc_entry;

} rme9652_t;


#define rme9652_running(s) ((s)->control_register & RME9652_start_bit)

int  snd_rme9652_create (snd_card_t *, int, rme9652_t *);
void snd_rme9652_interrupt (int, void *, struct pt_regs *);
int  snd_rme9652_free (rme9652_t *);

#endif  /* KERNEL */
#endif  /* ifdef RME9652_H */




