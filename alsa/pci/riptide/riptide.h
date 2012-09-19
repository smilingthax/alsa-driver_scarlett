/*
 *   Driver for the Conexant Riptide Soundchip
 *
 *	Copyright (c) 2004 Peter Gruber <nokos@gmx.net>
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
  History:
   - 02/15/2004 first release
   
  This Driver is based on the OSS Driver version from Linuxant (riptide-0.6lnxtbeta03111100)
  credits from the original files:
  
  MODULE NAME:        cnxt_rt.h                       
  AUTHOR:             K. Lazarev  (Transcribed by KNL)
  HISTORY:         Major Revision               Date        By
            -----------------------------     --------     -----
            Created                           02/1/2000     KNL

  MODULE NAME:     int_mdl.c                       
  AUTHOR:          Konstantin Lazarev    (Transcribed by KNL)
  HISTORY:         Major Revision               Date        By
            -----------------------------     --------     -----
            Created                           10/01/99      KNL
	    
  MODULE NAME:        riptide.h                       
  AUTHOR:             O. Druzhinin  (Transcribed by OLD)
  HISTORY:         Major Revision               Date        By
            -----------------------------     --------     -----
            Created                           10/16/97      OLD

  MODULE NAME:        Rp_Cmdif.cpp                       
  AUTHOR:             O. Druzhinin  (Transcribed by OLD)
                      K. Lazarev    (Transcribed by KNL)
  HISTORY:         Major Revision               Date        By
            -----------------------------     --------     -----
            Adopted from NT4 driver            6/22/99      OLD
            Ported to Linux                    9/01/99      KNL

  MODULE NAME:        rt_hw.c                       
  AUTHOR:             O. Druzhinin  (Transcribed by OLD)
                      C. Lazarev    (Transcribed by CNL)
  HISTORY:         Major Revision               Date        By
            -----------------------------     --------     -----
            Created                           11/18/97      OLD
            Hardware functions for RipTide    11/24/97      CNL
            (ES1) are coded
            Hardware functions for RipTide    12/24/97      CNL
            (A0) are coded
            Hardware functions for RipTide    03/20/98      CNL
            (A1) are coded
            Boot loader is included           05/07/98      CNL
            Redesigned for WDM                07/27/98      CNL
            Redesigned for Linux              09/01/99      CNL

  MODULE NAME:        rt_hw.h
  AUTHOR:             C. Lazarev    (Transcribed by CNL)
  HISTORY:         Major Revision               Date        By
            -----------------------------     --------     -----
            Created                           11/18/97      CNL

  MODULE NAME:     rt_mdl.c                       
  AUTHOR:          Konstantin Lazarev    (Transcribed by KNL)
  HISTORY:         Major Revision               Date        By
            -----------------------------     --------     -----
            Created                           10/01/99      KNL

  MODULE NAME:        mixer.h                        
  AUTHOR:             K. Kenney
  HISTORY:         Major Revision                   Date          By
            -----------------------------          --------     -----
            Created from MS W95 Sample             11/28/95      KRS
            RipTide                                10/15/97      KRS
            Adopted for Windows NT driver          01/20/98      CNL
*/

#ifndef _RIPTIDE_H_
#define _RIPTIDE_H_

#if defined(CONFIG_GAMEPORT) || (defined(MODULE) && defined(CONFIG_GAMEPORT_MODULE))
#define SUPPORT_JOYSTICK 1
#endif

#define FALSE 0
#define TRUE  1

#define MPU401_HW_RIPTIDE MPU401_HW_MPU401
#define OPL3_HW_RIPTIDE   OPL3_HW_OPL3

#define PLAYBACK_SUBSTREAMS 3

#define PCI_EXT_CapId       0x40
#define PCI_EXT_NextCapPrt  0x41
#define PCI_EXT_PWMC        0x42
#define PCI_EXT_PWSCR       0x44
#define PCI_EXT_Data00      0x46
#define PCI_EXT_PMSCR_BSE   0x47
#define PCI_EXT_SB_Base     0x48
#define PCI_EXT_FM_Base     0x4a
#define PCI_EXT_MPU_Base    0x4C
#define PCI_EXT_Game_Base   0x4E
#define PCI_EXT_Legacy_Mask 0x50
#define PCI_EXT_AsicRev     0x52
#define PCI_EXT_Reserved3   0x53

#define LEGACY_ENABLE_ALL      0x8000
#define LEGACY_ENABLE_SB       0x4000
#define LEGACY_ENABLE_FM       0x2000
#define LEGACY_ENABLE_MPU_INT  0x1000
#define LEGACY_ENABLE_MPU      0x0800
#define LEGACY_ENABLE_GAMEPORT 0x0400

#define D0_POWER_STATE   0x00
#define D1_POWER_STATE   0x01
#define D2_POWER_STATE   0x02
#define D3_POWER_STATE   0x03
#define POWER_STATE_MASK 0x03

#define TIMEOUT_CMD(a,b,c,d) a=b;while(--a){if(c){break;}udelay(10);d}

#define MAX_WRITE_RETRY  10
#define MAX_ERROR_COUNT  10
#define CMDIF_TIMEOUT    500000

#define READ_PORT_ULONG(p)     inl((unsigned long)&(p))
#define WRITE_PORT_ULONG(p,x)  outl(x,(unsigned long)&(p))

#define READ_AUDIO_CONTROL(p)     READ_PORT_ULONG(p->audio_control)
#define WRITE_AUDIO_CONTROL(p,x)  WRITE_PORT_ULONG(p->audio_control,x)
#define UMASK_AUDIO_CONTROL(p,x)  WRITE_PORT_ULONG(p->audio_control,READ_PORT_ULONG(p->audio_control)|x)
#define MASK_AUDIO_CONTROL(p,x)   WRITE_PORT_ULONG(p->audio_control,READ_PORT_ULONG(p->audio_control)&x)
#define READ_AUDIO_STATUS(p)      READ_PORT_ULONG(p->audio_status)

#define SET_GRESET(p)     UMASK_AUDIO_CONTROL(p,0x0001)
#define UNSET_GRESET(p)   MASK_AUDIO_CONTROL(p,~0x0001)
#define SET_AIE(p)        UMASK_AUDIO_CONTROL(p,0x0004)
#define UNSET_AIE(p)      MASK_AUDIO_CONTROL(p,~0x0004)
#define SET_AIACK(p)      UMASK_AUDIO_CONTROL(p,0x0008)
#define UNSET_AIACKT(p)   MASKAUDIO_CONTROL(p,~0x0008)
#define SET_ECMDAE(p)     UMASK_AUDIO_CONTROL(p,0x0010)
#define UNSET_ECMDAE(p)   MASK_AUDIO_CONTROL(p,~0x0010)
#define SET_ECMDBE(p)     UMASK_AUDIO_CONTROL(p,0x0020)
#define UNSET_ECMDBE(p)   MASK_AUDIO_CONTROL(p,~0x0020)
#define SET_EDATAF(p)     UMASK_AUDIO_CONTROL(p,0x0040)
#define UNSET_EDATAF(p)   MASK_AUDIO_CONTROL(p,~0x0040)
#define SET_EDATBF(p)     UMASK_AUDIO_CONTROL(p,0x0080)
#define UNSET_EDATBF(p)   MASK_AUDIO_CONTROL(p,~0x0080)
#define SET_ESBIRQON(p)   UMASK_AUDIO_CONTROL(p,0x0100)
#define UNSET_ESBIRQON(p) MASK_AUDIO_CONTROL(p,~0x0100)
#define SET_EMPUIRQ(p)    UMASK_AUDIO_CONTROL(p,0x0200)
#define UNSET_EMPUIRQ(p)  MASK_AUDIO_CONTROL(p,~0x0200)
#define IS_CMDE(a)        (READ_PORT_ULONG(a->stat)&0x1)
#define IS_DATF(a)        (READ_PORT_ULONG(a->stat)&0x2)
#define IS_READY(p)       (READ_AUDIO_STATUS(p)&0x0001)
#define IS_DLREADY(p)     (READ_AUDIO_STATUS(p)&0x0002)
#define IS_DLERR(p)       (READ_AUDIO_STATUS(p)&0x0004)
#define IS_GERR(p)        (READ_AUDIO_STATUS(p)&0x0008)
#define IS_CMDAEIRQ(p)    (READ_AUDIO_STATUS(p)&0x0010)
#define IS_CMDBEIRQ(p)    (READ_AUDIO_STATUS(p)&0x0020)
#define IS_DATAFIRQ(p)    (READ_AUDIO_STATUS(p)&0x0040)
#define IS_DATBFIRQ(p)    (READ_AUDIO_STATUS(p)&0x0080)
#define IS_EOBIRQ(p)      (READ_AUDIO_STATUS(p)&0x0100)
#define IS_EOSIRQ(p)      (READ_AUDIO_STATUS(p)&0x0200)
#define IS_EOCIRQ(p)      (READ_AUDIO_STATUS(p)&0x0400)
#define IS_UNSLIRQ(p)     (READ_AUDIO_STATUS(p)&0x0800)
#define IS_SBIRQ(p)       (READ_AUDIO_STATUS(p)&0x1000)
#define IS_MPUIRQ(p)      (READ_AUDIO_STATUS(p)&0x2000)

#define RESP 0x00000001
#define PARM 0x00000002
#define CMDA 0x00000004
#define CMDB 0x00000008
#define NILL 0x00000000

#define LONG0(a)   ((u32)a)
#define BYTE0(a)   (LONG0(a)&0xff)
#define BYTE1(a)   (BYTE0(a)<<8)
#define BYTE2(a)   (BYTE0(a)<<16)
#define BYTE3(a)   (BYTE0(a)<<24)
#define WORD0(a)   (LONG0(a)&0xffff)
#define WORD1(a)   (WORD0(a)<<8)
#define WORD2(a)   (WORD0(a)<<16)
#define TRINIB0(a) (LONG0(a)&0xffffff)
#define TRINIB1(a) (TRINIB0(a)<<8)
#define RET(a)     ((cmdret_t *)(a))

#define SEND_GETV(p,b)             sendcmd(p,RESP,GETV,0,RET(b))
#define SEND_GETC(p,b,c)           sendcmd(p,PARM|RESP,GETC,c,RET(b))
#define SEND_GUNS(p,b)             sendcmd(p,RESP,GUNS,0,RET(b))
#define SEND_SCID(p,b)             sendcmd(p,RESP,SCID,0,RET(b))
#define SEND_RMEM(p,b,c,d)         sendcmd(p,PARM|RESP,RMEM|BYTE1(b),LONG0(c),RET(d))
#define SEND_SMEM(p,b,c)           sendcmd(p,PARM,SMEM|BYTE1(b),LONG0(c),RET(0))
#define SEND_WMEM(p,b,c)           sendcmd(p,PARM,WMEM|BYTE1(b),LONG0(c),RET(0))
#define SEND_SDTM(p,b,c)           sendcmd(p,PARM|RESP,SDTM|TRINIB1(b),0,RET(c))
#define SEND_GOTO(p,b)             sendcmd(p,PARM,GOTO,LONG0(b),RET(0))
#define SEND_SETDPLL(p)	           sendcmd(p,0,ARM_SETDPLL,0,RET(0))
#define SEND_SSTR(p,b,c)           sendcmd(p,PARM,SSTR|BYTE3(b),LONG0(c),RET(0))
#define SEND_PSTR(p,b)             sendcmd(p,PARM,PSTR,BYTE3(b),RET(0))
#define SEND_KSTR(p,b)             sendcmd(p,PARM,KSTR,BYTE3(b),RET(0))
#define SEND_KDMA(p)               sendcmd(p,0,KDMA,0,RET(0))
#define SEND_GPOS(p,b,c,d)         sendcmd(p,PARM|RESP,GPOS,BYTE3(c)|BYTE2(b),RET(d))
#define SEND_SETF(p,b,c,d,e,f,g)   sendcmd(p,PARM,SETF|WORD1(b)|BYTE3(c),d|BYTE1(e)|BYTE2(f)|BYTE3(g),RET(0))
#define SEND_GSTS(p,b,c,d)         sendcmd(p,PARM|RESP,GSTS,BYTE3(c)|BYTE2(b),RET(d))
#define SEND_NGPOS(p,b,c,d)        sendcmd(p,PARM|RESP,NGPOS,BYTE3(c)|BYTE2(b),RET(d))
#define SEND_PSEL(p,b,c)           sendcmd(p,PARM,PSEL,BYTE2(b)|BYTE3(c),RET(0))
#define SEND_PCLR(p,b,c)           sendcmd(p,PARM,PCLR,BYTE2(b)|BYTE3(c),RET(0))
#define SEND_PLST(p,b)             sendcmd(p,PARM,PLST,BYTE3(b),RET(0))
#define SEND_RSSV(p,b,c,d)         sendcmd(p,PARM|RESP,RSSV,BYTE2(b)|BYTE3(c),RET(d))
#define SEND_LSEL(p,b,c,d,e,f,g,h) sendcmd(p,PARM,LSEL|BYTE1(b)|BYTE2(c)|BYTE3(d),BYTE0(e)|BYTE1(f)|BYTE2(g)|BYTE3(h),RET(0))
#define SEND_SSRC(p,b,c,d,e)       sendcmd(p,PARM,SSRC|BYTE1(b)|WORD2(c),WORD0(d)|WORD2(e),RET(0))
#define SEND_SLST(p,b)             sendcmd(p,PARM,SLST,BYTE3(b),RET(0))
#define SEND_RSRC(p,b,c)           sendcmd(p,RESP,RSRC|BYTE1(b),0,RET(c))
#define SEND_SSRB(p,b,c)           sendcmd(p,PARM,SSRB|BYTE1(b),WORD2(c),RET(0))
#define SEND_SDGV(p,b,c,d,e)       sendcmd(p,PARM,SDGV|BYTE2(b)|BYTE3(c),WORD0(d)|WORD2(e),RET(0))
#define SEND_RDGV(p,b,c,d)         sendcmd(p,PARM|RESP,RDGV|BYTE2(b)|BYTE3(c),0,RET(d))
#define SEND_DLST(p,b)             sendcmd(p,PARM,DLST,BYTE3(b),RET(0))
#define SEND_SACR(p,b,c)           sendcmd(p,PARM,SACR,WORD0(b)|WORD2(c),RET(0))
#define SEND_RACR(p,b,c)           sendcmd(p,PARM|RESP,RACR,WORD2(b),RET(c))
#define SEND_ALST(p,b)             sendcmd(p,PARM,ALST,BYTE3(b),RET(0))
#define SEND_TXAC(p,b,c,d,e,f)     sendcmd(p,PARM,TXAC|BYTE1(b)|WORD2(c),WORD0(d)|BYTE2(e)|BYTE3(f),RET(0))
#define SEND_RXAC(p,b,c,d)         sendcmd(p,PARM|RESP,RXAC,BYTE2(b)|BYTE3(c),RET(d))
#define SEND_SI2S(p,b)             sendcmd(p,PARM,SI2S,WORD2(b),RET(0))

#define EOB_STATUS         0x80000000
#define EOS_STATUS         0x40000000
#define EOC_STATUS         0x20000000
#define ERR_STATUS         0x10000000
#define EMPTY_STATUS       0x08000000

#define IEOB_ENABLE        0x1
#define IEOS_ENABLE        0x2
#define IEOC_ENABLE        0x4
#define RDONCE             0x8
#define DESC_MAX_MASK      0xff

#define ST_PLAY  0x1
#define ST_STOP  0x2
#define ST_PAUSE 0x4

#define I2S_INTDEC     3
#define I2S_MERGER     0
#define I2S_SPLITTER   0
#define I2S_MIXER      7
#define I2S_RATE       44100

#define MODEM_INTDEC   4
#define MODEM_MERGER   3
#define MODEM_SPLITTER 0
#define MODEM_MIXER    11

#define FM_INTDEC      3
#define FM_MERGER      0
#define FM_SPLITTER    0
#define FM_MIXER       9

#define SPLIT_PATH  0x80

enum FIRMWARE {
	DATA_REC =
	    0, EXT_END_OF_FILE, EXT_SEG_ADDR_REC, EXT_GOTO_CMD_REC,
	    EXT_LIN_ADDR_REC,
};

enum CMDS {
	GETV = 0x00, GETC, GUNS, SCID, RMEM =
	    0x10, SMEM, WMEM, SDTM, GOTO, SSTR =
	    0x20, PSTR, KSTR, KDMA, GPOS, SETF, GSTS, NGPOS, PSEL =
	    0x30, PCLR, PLST, RSSV, LSEL, SSRC = 0x40, SLST, RSRC, SSRB, SDGV =
	    0x50, RDGV, DLST, SACR = 0x60, RACR, ALST, TXAC, RXAC, SI2S =
	    0x70, ARM_SETDPLL = 0x72,
};

enum E1SOURCE {
	ARM2LBUS_FIFO0, ARM2LBUS_FIFO1, ARM2LBUS_FIFO2, ARM2LBUS_FIFO3,
	ARM2LBUS_FIFO4, ARM2LBUS_FIFO5, ARM2LBUS_FIFO6, ARM2LBUS_FIFO7,
	ARM2LBUS_FIFO8, ARM2LBUS_FIFO9, ARM2LBUS_FIFO10, ARM2LBUS_FIFO11,
	ARM2LBUS_FIFO12, ARM2LBUS_FIFO13, ARM2LBUS_FIFO14, ARM2LBUS_FIFO15,
	INTER0_OUT, INTER1_OUT, INTER2_OUT, INTER3_OUT, INTER4_OUT, INTERM0_OUT,
	INTERM1_OUT, INTERM2_OUT, INTERM3_OUT, INTERM4_OUT, INTERM5_OUT,
	INTERM6_OUT, DECIMM0_OUT, DECIMM1_OUT, DECIMM2_OUT, DECIMM3_OUT,
	DECIM0_OUT, SR3_4_OUT, OPL3_SAMPLE, ASRC0, ASRC1, ACLNK2PADC,
	ACLNK2MODEM0RX, ACLNK2MIC, ACLNK2MODEM1RX, ACLNK2HNDMIC,
	DIGITAL_MIXER_OUT0, GAINFUNC0_OUT, GAINFUNC1_OUT, GAINFUNC2_OUT,
	GAINFUNC3_OUT, GAINFUNC4_OUT, SOFTMODEMTX, SPLITTER0_OUTL,
	    SPLITTER0_OUTR,
	SPLITTER1_OUTL, SPLITTER1_OUTR, SPLITTER2_OUTL, SPLITTER2_OUTR,
	SPLITTER3_OUTL, SPLITTER3_OUTR, MERGER0_OUT, MERGER1_OUT, MERGER2_OUT,
	MERGER3_OUT, ARM2LBUS_FIFO_DIRECT, NO_OUT,
};

enum E2SINK {
	LBUS2ARM_FIFO0, LBUS2ARM_FIFO1, LBUS2ARM_FIFO2, LBUS2ARM_FIFO3,
	LBUS2ARM_FIFO4, LBUS2ARM_FIFO5, LBUS2ARM_FIFO6, LBUS2ARM_FIFO7,
	    INTER0_IN,
	INTER1_IN, INTER2_IN, INTER3_IN, INTER4_IN, INTERM0_IN, INTERM1_IN,
	INTERM2_IN, INTERM3_IN, INTERM4_IN, INTERM5_IN, INTERM6_IN, DECIMM0_IN,
	DECIMM1_IN, DECIMM2_IN, DECIMM3_IN, DECIM0_IN, SR3_4_IN, PDAC2ACLNK,
	MODEM0TX2ACLNK, MODEM1TX2ACLNK, HNDSPK2ACLNK, DIGITAL_MIXER_IN0,
	DIGITAL_MIXER_IN1, DIGITAL_MIXER_IN2, DIGITAL_MIXER_IN3,
	DIGITAL_MIXER_IN4, DIGITAL_MIXER_IN5, DIGITAL_MIXER_IN6,
	DIGITAL_MIXER_IN7, DIGITAL_MIXER_IN8, DIGITAL_MIXER_IN9,
	DIGITAL_MIXER_IN10, DIGITAL_MIXER_IN11, GAINFUNC0_IN, GAINFUNC1_IN,
	GAINFUNC2_IN, GAINFUNC3_IN, GAINFUNC4_IN, SOFTMODEMRX, SPLITTER0_IN,
	SPLITTER1_IN, SPLITTER2_IN, SPLITTER3_IN, MERGER0_INL, MERGER0_INR,
	MERGER1_INL, MERGER1_INR, MERGER2_INL, MERGER2_INR, MERGER3_INL,
	MERGER3_INR, E2SINK_MAX
};

enum LBUS_SINK {
	LS_SRC_INTERPOLATOR, LS_SRC_INTERPOLATORM, LS_SRC_DECIMATOR,
	LS_SRC_DECIMATORM, LS_MIXER_IN, LS_MIXER_GAIN_FUNCTION, LS_SRC_SPLITTER,
	LS_SRC_MERGER, LS_NONE1, LS_NONE2,
};

enum RT_CHANNEL_IDS {
	M0TX =
	    0, M1TX, TAMTX, HSSPKR, PDAC, DSNDTX0, DSNDTX1, DSNDTX2, DSNDTX3,
	    DSNDTX4,
	DSNDTX5, DSNDTX6, DSNDTX7, WVSTRTX, COP3DTX, SPARE, M0RX, HSMIC, M1RX,
	CLEANRX, MICADC, PADC, COPRX1, COPRX2, CHANNEL_ID_COUNTER
};

enum { SB_CMD, MODEM_CMD, I2S_CMD0, I2S_CMD1, FM_CMD, MAX_CMD };

typedef void (*voidfunc) (void);
typedef struct _snd_riptide riptide_t;
typedef struct _snd_cmdif cmdif_t;
typedef struct _snd_pcmhw pcmhw_t;
typedef struct _snd_lbuspath lbuspath_t;
typedef struct _snd_sgd sgd_t;
typedef union _snd_cmdret cmdret_t;
typedef struct _snd_riptideport riptideport_t;
typedef struct _snd_cmdport cmdport_t;
typedef struct gameport gameport_t;

struct _snd_lbuspath {
	unsigned char *noconv;
	unsigned char *stereo;
	unsigned char *mono;
};

struct _snd_cmdport {
	u32 data1;
	u32 data2;
	u32 stat;
	u32 pad[5];
};

struct _snd_riptideport {
	u32 audio_control;
	u32 audio_status;
	u32 pad[2];
	cmdport_t port[2];
};

struct _snd_cmdif {
	riptideport_t *hwport;
	spinlock_t lock;
	unsigned int cmdcnt;
	unsigned int cmdtime;
	unsigned int cmdtimemax;
	unsigned int cmdtimemin;
	unsigned int errcnt;
	int is_reset;
};

#define get_pcmhwdev(substream) (pcmhw_t *)(substream->runtime->private_data)

struct _snd_riptide {
	snd_card_t *card;
	struct pci_dev *pci;

	cmdif_t *cif;

	snd_pcm_t *pcm;
	snd_pcm_t *pcm_i2s;
	snd_rawmidi_t *rmidi;
	opl3_t *opl3;
	ac97_t *ac97;
	ac97_bus_t *ac97_bus;

	snd_pcm_substream_t *playback_substream[PLAYBACK_SUBSTREAMS];
	snd_pcm_substream_t *capture_substream;

	int openstreams;

	int irq;
	unsigned long port;
	unsigned short mpuaddr;
	unsigned short opladdr;
#ifdef SUPPORT_JOYSTICK
	unsigned short gameaddr;
#endif
	struct resource *res_port;

	unsigned short device_id;

	spinlock_t lock;
	struct tasklet_struct riptide_tq;
	snd_info_entry_t *proc_entry;

	unsigned long received_irqs;
	unsigned long handled_irqs;
#ifdef CONFIG_PM
	int in_suspend;
#endif
};

struct _snd_sgd {
	dma_addr_t dwNextLink;
	dma_addr_t dwSegPtrPhys;
	u32 dwSegLen;
	u32 dwStat_Ctl;
};

struct _snd_pcmhw {
	lbuspath_t paths;
	unsigned char *lbuspath;
	unsigned char source;
	unsigned char intdec[2];
	unsigned char mixer;
	unsigned char id;
	unsigned char state;
	unsigned int rate;
	unsigned int channels;
	snd_pcm_format_t format;
	struct snd_dma_buffer sgdlist;
	unsigned int size;
	unsigned int pages;
	unsigned int oldpos;
	unsigned int pointer;
};

union _snd_cmdret {
	u8 retbytes[8];
	u16 retwords[4];
	u32 retlongs[2];
};

static u32 atoh(unsigned char *, unsigned int);
static int senddata(cmdif_t *, unsigned char *, u32);
static int loadfirmware(cmdif_t *, unsigned char *, unsigned int);
static void alloclbuspath(cmdif_t *, unsigned char,
			  unsigned char *, unsigned char *, unsigned char *);
static void freelbuspath(cmdif_t *, unsigned char, unsigned char *);
static int writearm(cmdif_t *, u32, u32, u32);
static int sendcmd(cmdif_t *, u32, u32, u32, cmdret_t *);
static int getmixer(cmdif_t *, short, unsigned short *, unsigned short *);
static int getsamplerate(cmdif_t *, unsigned char *, unsigned int *);
static int setsampleformat(cmdif_t *, unsigned char,
			   unsigned char, unsigned char, unsigned char);
static int setmixer(cmdif_t *, short, unsigned short, unsigned short);
static int getpaths(cmdif_t *, unsigned char *);
static int getsourcesink(cmdif_t *, unsigned char, unsigned char,
			 unsigned char *, unsigned char *);

static void riptide_handleirq(unsigned long);
#ifdef CONFIG_PM
static void riptide_suspend(riptide_t *);
static void riptide_resume(riptide_t *);
#endif
static int riptide_reset(cmdif_t *, riptide_t *);

static snd_pcm_uframes_t snd_riptide_pointer(snd_pcm_substream_t *);
static int snd_riptide_trigger(snd_pcm_substream_t *, int);
static int snd_riptide_prepare(snd_pcm_substream_t *);
static int snd_riptide_hw_params(snd_pcm_substream_t *, snd_pcm_hw_params_t *);
static int snd_riptide_hw_free(snd_pcm_substream_t *);
static int snd_riptide_playback_open(snd_pcm_substream_t *);
static int snd_riptide_capture_open(snd_pcm_substream_t *);
static int snd_riptide_playback_close(snd_pcm_substream_t *);
static int snd_riptide_capture_close(snd_pcm_substream_t *);
static void snd_riptide_pcm_free(snd_pcm_t *);
static int __devinit snd_riptide_pcm(riptide_t *, int, snd_pcm_t **);
static irqreturn_t snd_riptide_interrupt(int, void *, struct pt_regs *);
static void snd_riptide_codec_write(ac97_t *, unsigned short, unsigned short);
static unsigned short snd_riptide_codec_read(ac97_t *, unsigned short);
#ifdef CONFIG_PM
static int snd_riptide_suspend(snd_card_t *, pm_message_t);
static int snd_riptide_resume(snd_card_t *);
#endif
static int snd_riptide_initialize(riptide_t *);
static int snd_riptide_hw_close(riptide_t *);
static int snd_riptide_free(riptide_t *);
static int snd_riptide_dev_free(snd_device_t *);
static int __devinit snd_riptide_create(snd_card_t *, struct pci_dev *,
					riptide_t **);
static void snd_riptide_proc_read(snd_info_entry_t *, snd_info_buffer_t *);
static void __devinit snd_riptide_proc_init(riptide_t *);
static void snd_riptide_mixer_free_ac97(ac97_t *);
static void snd_riptide_mixer_free_ac97_bus(ac97_bus_t *);
static int __devinit snd_riptide_mixer(riptide_t *);
#ifdef SUPPORT_JOYSTICK
static int __devinit snd_riptide_joystick_probe(struct pci_dev *, const struct pci_device_id
						*);
static void __devexit snd_riptide_joystick_remove(struct pci_dev *);
#endif
static int __devinit snd_card_riptide_probe(struct pci_dev *,
					    const struct pci_device_id *);
static void __devexit snd_card_riptide_remove(struct pci_dev *);
static int __init alsa_card_riptide_init(void);
static void __exit alsa_card_riptide_exit(void);

#endif
