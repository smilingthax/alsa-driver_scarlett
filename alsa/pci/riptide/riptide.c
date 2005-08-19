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

#include <sound/driver.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/gameport.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <asm/io.h>
#include <asm/semaphore.h>
#include <sound/core.h>
#include <sound/info.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/ac97_codec.h>
#include <sound/mpu401.h>
#include <sound/opl3.h>
#include <sound/initval.h>

#include "riptide.h"

MODULE_AUTHOR("Peter Gruber <nokos@gmx.net>");
MODULE_DESCRIPTION("riptide");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("{{Conexant,Riptide}}");

static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;
static int enable[SNDRV_CARDS] = SNDRV_DEFAULT_ENABLE;

#ifdef SUPPORT_JOYSTICK
static int joystick_port[SNDRV_CARDS] = { [0 ... (SNDRV_CARDS - 1)] = 0x200 };
#endif
static int mpu_port[SNDRV_CARDS] = { [0 ... (SNDRV_CARDS - 1)] = 0x330 };
static int opl3_port[SNDRV_CARDS] = { [0 ... (SNDRV_CARDS - 1)] = 0x388 };

module_param_array(index, int, NULL, 0444);
MODULE_PARM_DESC(index, "Index value for Riptide soundcard.");
module_param_array(id, charp, NULL, 0444);
MODULE_PARM_DESC(id, "ID string for Riptide soundcard.");
module_param_array(enable, bool, NULL, 0444);
MODULE_PARM_DESC(enable, "Enable Riptide soundcard.");
#ifdef SUPPORT_JOYSTICK
module_param_array(joystick_port, int, NULL, 0444);
MODULE_PARM_DESC(joystick_port, "Joystick port # for Riptide soundcard.");
#endif
module_param_array(mpu_port, int, NULL, 0444);
MODULE_PARM_DESC(mpu_port, "MPU401 port # for Riptide driver.");
module_param_array(opl3_port, int, NULL, 0444);
MODULE_PARM_DESC(opl3_port, "OPL3 port # for Riptide driver.");

static struct pci_device_id snd_riptide_ids[] = {
	{
	 .vendor = 0x127a,.device = 0x4310,
	 .subvendor = PCI_ANY_ID,.subdevice = PCI_ANY_ID,
	 },
	{
	 .vendor = 0x127a,.device = 0x4320,
	 .subvendor = PCI_ANY_ID,.subdevice = PCI_ANY_ID,
	 },
	{
	 .vendor = 0x127a,.device = 0x4330,
	 .subvendor = PCI_ANY_ID,.subdevice = PCI_ANY_ID,
	 },
	{
	 .vendor = 0x127a,.device = 0x4340,
	 .subvendor = PCI_ANY_ID,.subdevice = PCI_ANY_ID,
	 },
	{0,},
};

#ifdef SUPPORT_JOYSTICK
static struct pci_device_id snd_riptide_joystick_ids[] = {
	{
	 .vendor = 0x127a,.device = 0x4312,
	 .subvendor = PCI_ANY_ID,.subdevice = PCI_ANY_ID,
	 },
	{
	 .vendor = 0x127a,.device = 0x4322,
	 .subvendor = PCI_ANY_ID,.subdevice = PCI_ANY_ID,
	 },
	{.vendor = 0x127a,.device = 0x4332,
	 .subvendor = PCI_ANY_ID,.subdevice = PCI_ANY_ID,
	 },
	{.vendor = 0x127a,.device = 0x4342,
	 .subvendor = PCI_ANY_ID,.subdevice = PCI_ANY_ID,
	 },
};

static int have_joystick;
#endif

MODULE_DEVICE_TABLE(pci, snd_riptide_ids);

static struct pci_driver driver = {
	.name = "RIPTIDE",
	.id_table = snd_riptide_ids,
	.probe = snd_card_riptide_probe,
	.remove = __devexit_p(snd_card_riptide_remove),
};

#ifdef SUPPORT_JOYSTICK
static struct pci_driver joystick_driver = {
	.name = "Riptide Joystick",
	.id_table = snd_riptide_joystick_ids,
	.probe = snd_riptide_joystick_probe,
	.remove = __devexit_p(snd_riptide_joystick_remove),
};
static struct pci_dev *riptide_gameport_pci;
static struct gameport *riptide_gameport;
#endif

static unsigned char lbusin2out[E2SINK_MAX + 1][2] = {
	{NO_OUT, LS_NONE1}, {NO_OUT, LS_NONE2}, {NO_OUT, LS_NONE1}, {NO_OUT, LS_NONE2},
	{NO_OUT, LS_NONE1}, {NO_OUT, LS_NONE2}, {NO_OUT, LS_NONE1}, {NO_OUT, LS_NONE2},
	{INTER0_OUT, LS_SRC_INTERPOLATOR}, {INTER1_OUT, LS_SRC_INTERPOLATOR},
	{INTER2_OUT, LS_SRC_INTERPOLATOR}, {INTER3_OUT, LS_SRC_INTERPOLATOR},
	{INTER4_OUT, LS_SRC_INTERPOLATOR}, {INTERM0_OUT, LS_SRC_INTERPOLATORM},
	{INTERM1_OUT, LS_SRC_INTERPOLATORM}, {INTERM2_OUT, LS_SRC_INTERPOLATORM},
	{INTERM3_OUT, LS_SRC_INTERPOLATORM}, {INTERM4_OUT, LS_SRC_INTERPOLATORM},
	{INTERM5_OUT, LS_SRC_INTERPOLATORM}, {INTERM6_OUT, LS_SRC_INTERPOLATORM},
	{DECIMM0_OUT, LS_SRC_DECIMATORM}, {DECIMM1_OUT, LS_SRC_DECIMATORM},
	{DECIMM2_OUT, LS_SRC_DECIMATORM}, {DECIMM3_OUT, LS_SRC_DECIMATORM},
	{DECIM0_OUT, LS_SRC_DECIMATOR}, {SR3_4_OUT, LS_NONE1}, {NO_OUT, LS_NONE2},
	{NO_OUT, LS_NONE1}, {NO_OUT, LS_NONE2}, {NO_OUT, LS_NONE1},
	{DIGITAL_MIXER_OUT0, LS_MIXER_IN}, {DIGITAL_MIXER_OUT0, LS_MIXER_IN},
	{DIGITAL_MIXER_OUT0, LS_MIXER_IN}, {DIGITAL_MIXER_OUT0, LS_MIXER_IN},
	{DIGITAL_MIXER_OUT0, LS_MIXER_IN}, {DIGITAL_MIXER_OUT0, LS_MIXER_IN},
	{DIGITAL_MIXER_OUT0, LS_MIXER_IN}, {DIGITAL_MIXER_OUT0, LS_MIXER_IN},
	{DIGITAL_MIXER_OUT0, LS_MIXER_IN}, {DIGITAL_MIXER_OUT0, LS_MIXER_IN},
	{DIGITAL_MIXER_OUT0, LS_MIXER_IN}, {DIGITAL_MIXER_OUT0, LS_MIXER_IN},
	{GAINFUNC0_OUT, LS_MIXER_GAIN_FUNCTION}, {GAINFUNC1_OUT,
						  LS_MIXER_GAIN_FUNCTION},
	{GAINFUNC2_OUT, LS_MIXER_GAIN_FUNCTION}, {GAINFUNC3_OUT,
						  LS_MIXER_GAIN_FUNCTION},
	{GAINFUNC4_OUT, LS_MIXER_GAIN_FUNCTION}, {SOFTMODEMTX, LS_NONE1},
	{SPLITTER0_OUTL, LS_SRC_SPLITTER}, {SPLITTER1_OUTL, LS_SRC_SPLITTER},
	{SPLITTER2_OUTL, LS_SRC_SPLITTER}, {SPLITTER3_OUTL, LS_SRC_SPLITTER},
	{MERGER0_OUT, LS_SRC_MERGER}, {MERGER0_OUT, LS_SRC_MERGER},
	    {MERGER1_OUT, LS_SRC_MERGER},
	{MERGER1_OUT, LS_SRC_MERGER}, {MERGER2_OUT, LS_SRC_MERGER},
	    {MERGER2_OUT, LS_SRC_MERGER},
	{MERGER3_OUT, LS_SRC_MERGER}, {MERGER3_OUT, LS_SRC_MERGER}, {NO_OUT, LS_NONE2},
};

static unsigned char lbus_play_opl3[] = {
	DIGITAL_MIXER_IN0 + FM_MIXER, 0xff
};
static unsigned char lbus_play_modem[] = {
	DIGITAL_MIXER_IN0 + MODEM_MIXER, 0xff
};
static unsigned char lbus_play_i2s[] = {
	INTER0_IN + I2S_INTDEC, DIGITAL_MIXER_IN0 + I2S_MIXER, 0xff
};
static unsigned char lbus_play_out[] = {
	PDAC2ACLNK, 0xff
};
static unsigned char lbus_play_outhp[] = {
	HNDSPK2ACLNK, 0xff
};
static unsigned char lbus_play_noconv1[] = {
	DIGITAL_MIXER_IN0, 0xff
};
static unsigned char lbus_play_stereo1[] = {
	INTER0_IN, DIGITAL_MIXER_IN0, 0xff
};
static unsigned char lbus_play_mono1[] = {
	INTERM0_IN, DIGITAL_MIXER_IN0, 0xff
};
static unsigned char lbus_play_noconv2[] = {
	DIGITAL_MIXER_IN1, 0xff
};
static unsigned char lbus_play_stereo2[] = {
	INTER1_IN, DIGITAL_MIXER_IN1, 0xff
};
static unsigned char lbus_play_mono2[] = {
	INTERM1_IN, DIGITAL_MIXER_IN1, 0xff
};
static unsigned char lbus_play_noconv3[] = {
	DIGITAL_MIXER_IN2, 0xff
};
static unsigned char lbus_play_stereo3[] = {
	INTER2_IN, DIGITAL_MIXER_IN2, 0xff
};
static unsigned char lbus_play_mono3[] = {
	INTERM2_IN, DIGITAL_MIXER_IN2, 0xff
};
static unsigned char lbus_rec_noconv1[] = {
	LBUS2ARM_FIFO5, 0xff
};
static unsigned char lbus_rec_stereo1[] = {
	DECIM0_IN, LBUS2ARM_FIFO5, 0xff
};
static unsigned char lbus_rec_mono1[] = {
	DECIMM3_IN, LBUS2ARM_FIFO5, 0xff
};

static unsigned char play_ids[] = { 4, 1, 2, };
static unsigned char play_sources[] = {
	ARM2LBUS_FIFO4, ARM2LBUS_FIFO1, ARM2LBUS_FIFO2,
};
static struct _snd_lbuspath lbus_play_paths[] = {
	{
	 .noconv = lbus_play_noconv1,
	 .stereo = lbus_play_stereo1,
	 .mono = lbus_play_mono1,
	 },
	{
	 .noconv = lbus_play_noconv2,
	 .stereo = lbus_play_stereo2,
	 .mono = lbus_play_mono2,
	 },
	{
	 .noconv = lbus_play_noconv3,
	 .stereo = lbus_play_stereo3,
	 .mono = lbus_play_mono3,
	 },
};
static struct _snd_lbuspath lbus_rec_path = {
	.noconv = lbus_rec_noconv1,
	.stereo = lbus_rec_stereo1,
	.mono = lbus_rec_mono1,
};

static snd_device_ops_t ops = {
	.dev_free = snd_riptide_dev_free,
};

static snd_pcm_ops_t snd_riptide_playback_ops = {
	.open = snd_riptide_playback_open,
	.close = snd_riptide_playback_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = snd_riptide_hw_params,
	.hw_free = snd_riptide_hw_free,
	.prepare = snd_riptide_prepare,
	.page = snd_pcm_sgbuf_ops_page,
	.trigger = snd_riptide_trigger,
	.pointer = snd_riptide_pointer,
};
static snd_pcm_ops_t snd_riptide_capture_ops = {
	.open = snd_riptide_capture_open,
	.close = snd_riptide_capture_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = snd_riptide_hw_params,
	.hw_free = snd_riptide_hw_free,
	.prepare = snd_riptide_prepare,
	.page = snd_pcm_sgbuf_ops_page,
	.trigger = snd_riptide_trigger,
	.pointer = snd_riptide_pointer,
};

static snd_pcm_hardware_t snd_riptide_playback = {
	.info = (SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_MMAP_VALID),
	.formats =
	    SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S8
	    | SNDRV_PCM_FMTBIT_U16_LE,
	.rates = SNDRV_PCM_RATE_KNOT | SNDRV_PCM_RATE_8000_48000,
	.rate_min = 5500,
	.rate_max = 48000,
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = (64 * 1024),
	.period_bytes_min = PAGE_SIZE >> 1,
	.period_bytes_max = PAGE_SIZE << 8,
	.periods_min = 2,
	.periods_max = 64,
	.fifo_size = 0,
};
static snd_pcm_hardware_t snd_riptide_capture = {
	.info = (SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
		 SNDRV_PCM_INFO_BLOCK_TRANSFER |
		 SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_MMAP_VALID),
	.formats =
	    SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S8
	    | SNDRV_PCM_FMTBIT_U16_LE,
	.rates = SNDRV_PCM_RATE_KNOT | SNDRV_PCM_RATE_8000_48000,
	.rate_min = 5500,
	.rate_max = 48000,
	.channels_min = 1,
	.channels_max = 2,
	.buffer_bytes_max = (64 * 1024),
	.period_bytes_min = PAGE_SIZE >> 1,
	.period_bytes_max = PAGE_SIZE << 3,
	.periods_min = 2,
	.periods_max = 64,
	.fifo_size = 0,
};

static u32 atoh(unsigned char *in, unsigned int len)
{
	u32 sum = 0;
	unsigned int mult = 1;
	unsigned char c;

	while (len) {
		c = in[len - 1];
		if ((c >= '0') && (c <= '9'))
			sum += mult * (c - '0');
		else if ((c >= 'A') && (c <= 'F'))
			sum += mult * (c - ('A' - 10));
		else if ((c >= 'a') && (c <= 'f'))
			sum += mult * (c - ('a' - 10));
		mult *= 16;
		--len;
	}
	return sum;
}

static int senddata(cmdif_t * cif, unsigned char *in, u32 offset)
{
	u32 addr;
	u32 data;
	u32 i;
	unsigned char *p;

	i = atoh(&in[1], 2);
	addr = offset + atoh(&in[3], 4);
	if (SEND_SMEM(cif, 0, addr) != 0)
		return -EACCES;
	p = in + 9;
	while (i) {
		data = atoh(p, 8);
		if (SEND_WMEM(cif, 2,
			      ((data & 0x0f0f0f0f) << 4) | ((data & 0xf0f0f0f0) >> 4))
		    != 0)
			return -EACCES;
		i -= 4;
		p += 8;
	}
	return 0;
}

static int loadfirmware(cmdif_t * cif, unsigned char *img, unsigned int size)
{
	unsigned char *in;
	u32 laddr, saddr, t, val;
	int err = 0;

	laddr = saddr = 0;
	while (size > 0 && err == 0) {
		in = img;
		if (in[0] == ':') {
			t = atoh(&in[7], 2);
			switch (t) {
			case DATA_REC:
				err = senddata(cif, in, laddr + saddr);
				break;
			case EXT_SEG_ADDR_REC:
				saddr = atoh(&in[9], 4) << 4;
				break;
			case EXT_LIN_ADDR_REC:
				laddr = atoh(&in[9], 4) << 16;
				break;
			case EXT_GOTO_CMD_REC:
				val = atoh(&in[9], 8);
				if (SEND_GOTO(cif, val) != 0)
					err = -EACCES;
				break;
			case EXT_END_OF_FILE:
				size = 0;
				break;
			default:
				break;
			}
			while (size > 0) {
				size--;
				if (*img++ == '\n')
					break;
			}
		}
	}
	snd_printd("load firmware return %d\n", err);
	return err;
}

static void
alloclbuspath(cmdif_t * cif, unsigned char source,
	      unsigned char *path, unsigned char *mixer, unsigned char *s)
{
	while ((*path != 0xff)) {
		unsigned char sink, type;

		sink = *path & (~SPLIT_PATH);
		if (sink != E2SINK_MAX) {
			snd_printd("alloc path 0x%x->0x%x\n", source, sink);
			SEND_PSEL(cif, source, sink);
			source = lbusin2out[sink][0];
			type = lbusin2out[sink][1];
			if (type == LS_MIXER_IN) {
				if (mixer)
					*mixer = sink - DIGITAL_MIXER_IN0;
			}
			if (type == LS_SRC_DECIMATORM
			    || type == LS_SRC_DECIMATOR
			    || type == LS_SRC_INTERPOLATORM
			    || type == LS_SRC_INTERPOLATOR) {
				if (s) {
					if (s[0] != 0xff)
						s[1] = sink;
					else
						s[0] = sink;
				}
			}
		}
		if (*path++ & SPLIT_PATH) {
			unsigned char *npath = path;

			while (*npath != 0xff)
				npath++;
			alloclbuspath(cif, source + 1, ++npath, mixer, s);
		}
	}
}

static void
freelbuspath(cmdif_t * cif, unsigned char source, unsigned char *path)
{
	while (*path != 0xff) {
		unsigned char sink;

		sink = *path & (~SPLIT_PATH);
		if (sink != E2SINK_MAX) {
			snd_printd("free path 0x%x->0x%x\n", source, sink);
			SEND_PCLR(cif, source, sink);
			source = lbusin2out[sink][0];
		}
		if (*path++ & SPLIT_PATH) {
			unsigned char *npath = path;

			while (*npath != 0xff)
				npath++;
			freelbuspath(cif, source + 1, ++npath);
		}
	}
}

static int writearm(cmdif_t * cif, u32 addr, u32 data, u32 mask)
{
	cmdret_t rptr = { {(u32) 0, (u32) 0} };
	unsigned int i = MAX_WRITE_RETRY;
	int flag = 1;

	SEND_RMEM(cif, 0x02, addr, &rptr);
	rptr.retlongs[0] &= (~mask);

	while (--i) {
		SEND_SMEM(cif, 0x01, addr);
		SEND_WMEM(cif, 0x02, (rptr.retlongs[0] | data));
		SEND_RMEM(cif, 0x02, addr, &rptr);
		if ((rptr.retlongs[0] & data) == data) {
			flag = 0;
			break;
		} else
			rptr.retlongs[0] &= (~mask);
	}
	snd_printd("send arm 0x%x 0x%x 0x%x return %d\n", addr, data, mask, flag);
	return (flag);
}

static int sendcmd(cmdif_t * cif, u32 flags, u32 cmd, u32 parm, cmdret_t * ret)
{
	int i, j;
	int err = -EIO;
	unsigned int time = 0;
	unsigned long irqflags;
	riptideport_t *hwport = NULL;
	cmdport_t *cmdport = NULL;

	if (cif == NULL)
		return -EINVAL;
	hwport = cif->hwport;
	if (cif->errcnt > MAX_ERROR_COUNT) {
		if (cif->is_reset == TRUE) {
			snd_printk("Riptide: Too many failed cmds, reinitializing\n");
			if (riptide_reset(cif, NULL) == 0) {
				cif->errcnt = 0;
				return -EIO;
			}
		}
		return -EINVAL;
	}
	if (ret) {
		ret->retlongs[0] = 0;
		ret->retlongs[1] = 0;
	}
	i = 0;
	spin_lock_irqsave(&cif->lock, irqflags);
	while ((i++ < CMDIF_TIMEOUT) && (!IS_READY(cif->hwport)))
		udelay(10);
	if (i < CMDIF_TIMEOUT) {
		j = 0;
		while ((time < CMDIF_TIMEOUT) && (err != 0)) {
			cmdport = &(hwport->port[j % 2]);
			if (IS_CMDE(cmdport)) {
				if (flags & PARM)
					WRITE_PORT_ULONG(cmdport->data2, parm);
				WRITE_PORT_ULONG(cmdport->data1, cmd);
				if ((flags & RESP) && ret) {
					while ((!IS_DATF(cmdport))
					       && (time++ < CMDIF_TIMEOUT))
						udelay(10);
					if (time < CMDIF_TIMEOUT) {
						err = 0;
						ret->retlongs[0] =
						    READ_PORT_ULONG(cmdport->data1);
						ret->retlongs[1] =
						    READ_PORT_ULONG(cmdport->data2);
					} else
						err = -ENOSYS;
				} else
					err = 0;
			}
			udelay(20);
			time += 2;
			j++;
		}
		if (time == CMDIF_TIMEOUT)
			err = -ENODATA;
	} else
		err = -EBUSY;
	if (err == 0) {
		(cif->cmdcnt)++;
		cif->cmdtime += time;
		if (time > cif->cmdtimemax)
			cif->cmdtimemax = time;
		if (time < cif->cmdtimemin)
			cif->cmdtimemin = time;
	} else {
		(cif->errcnt)++;
		snd_printd("send cmd %d hw: 0x%x flag: 0x%x cmd: 0x%x parm: 0x%x ret: 0x%x 0x%x failed %d\n",
			   cif->cmdcnt,
			   (void *)&(cmdport->stat) - (void *)hwport, flags,
			   cmd, parm, ret ? ret->retlongs[0] : 0,
			   ret ? ret->retlongs[1] : 0, err);
	}
	spin_unlock_irqrestore(&cif->lock, irqflags);
	return err;
}

static int
setmixer(cmdif_t * cif, short num, unsigned short rval, unsigned short lval)
{
	cmdret_t rptr = { {(u32) 0, (u32) 0} };
	int i = 0;

	do {
		SEND_SDGV(cif, num, num, rval, lval);
		SEND_RDGV(cif, num, num, &rptr);
	}
	while ((rptr.retwords[0] != lval) && (rptr.retwords[1] != rval)
	       && (i++ < MAX_WRITE_RETRY));
	if (i == MAX_WRITE_RETRY) {
		snd_printd("sent mixer %d: 0x%d 0x%d failed\n", num, rval,
			   lval);
		return -EIO;
	}
	snd_printd("sent mixer %d: 0x%d 0x%d\n", num, rval, lval);
	return 0;
}

static int getpaths(cmdif_t * cif, unsigned char *o)
{
	unsigned char src[E2SINK_MAX];
	unsigned char sink[E2SINK_MAX];
	int i, j = 0;

	for (i = 0; i < E2SINK_MAX; i++) {
		getsourcesink(cif, i, i, &src[i], &sink[i]);
		if (sink[i] < E2SINK_MAX) {
			o[j++] = sink[i];
			o[j++] = i;
		}
	}
	return j;
}

static int
getsourcesink(cmdif_t * cif, unsigned char source, unsigned char sink,
	      unsigned char *a, unsigned char *b)
{
	cmdret_t rptr = { {(u32) 0, (u32) 0} };

	if ((SEND_RSSV(cif, source, sink, &rptr))
	    && (SEND_RSSV(cif, source, sink, &rptr)))
		return -EIO;
	*a = rptr.retbytes[0];
	*b = rptr.retbytes[1];
	snd_printd("getsourcesink 0x%x 0x%x\n", *a, *b);
	return 0;
}

static int
getsamplerate(cmdif_t * cif, unsigned char *intdec, unsigned int *rate)
{
	unsigned char *s;
	unsigned int p[2] = { 0, 0 };
	int i;
	cmdret_t rptr = { {(u32) 0, (u32) 0} };

	s = intdec;
	for (i = 0; i < 2; i++) {
		if (*s != 0xff) {
			if (SEND_RSRC(cif, *s, &rptr)
			    && SEND_RSRC(cif, *s, &rptr))
				return -EIO;
			p[i] += rptr.retwords[1];
			p[i] *= rptr.retwords[2];
			p[i] += rptr.retwords[3];
			p[i] /= 65536;
		}
		s++;
	}
	if (p[0]) {
		if (p[1] != p[0])
			snd_printd("rates differ %d %d\n", p[0], p[1]);
		*rate = (unsigned int)p[0];
	} else
		*rate = (unsigned int)p[1];
	snd_printd("getsampleformat %d %d %d\n", intdec[0], intdec[1], *rate);
	return 0;
}

static int
setsampleformat(cmdif_t * cif,
		unsigned char mixer, unsigned char id,
		unsigned char channels, unsigned char format)
{
	unsigned char w, ch, sig, order;

	snd_printd("setsampleformat mixer: %d id: %d channels: %d format: %d\n",
		   mixer, id, channels, format);
	switch (channels) {
	case 1:
		ch = 1;
		break;
	case 2:
		ch = 0;
		break;
	default:
		snd_printk("Riptide: wrong number of channels: %d\n", channels);
		return -EINVAL;
	}
	switch (format) {
	case SNDRV_PCM_FORMAT_S16_BE:
		w = 0;
		sig = 0;
		order = 1;
		break;
	case SNDRV_PCM_FORMAT_U16_BE:
		w = 0;
		sig = 1;
		order = 1;
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
		w = 0;
		sig = 0;
		order = 0;
		break;
	case SNDRV_PCM_FORMAT_U16_LE:
		w = 0;
		sig = 1;
		order = 0;
		break;
	case SNDRV_PCM_FORMAT_S8:
		w = 1;
		sig = 0;
		order = 0;
		break;
	case SNDRV_PCM_FORMAT_U8:
		w = 1;
		sig = 1;
		order = 0;
		break;
	default:
		snd_printk("Riptide: wrong format: 0x%x\n", format);
		return -EINVAL;
	}
	if (SEND_SETF(cif, mixer, w, ch, order, sig, id)
	    && SEND_SETF(cif, mixer, w, ch, order, sig, id)) {
		snd_printd("setsampleformat failed\n");
		return -EIO;
	}
	return 0;
}

static int
setsamplerate(cmdif_t * cif, unsigned char *intdec, unsigned int rate)
{
	u32 D, M, N;
	cmdret_t rptr = { {(u32) 0, (u32) 0} };
	int i;

	snd_printd("setsamplerate intdec: %d,%d rate: %d\n", intdec[0],
		   intdec[1], rate);
	D = 48000;
	M = ((rate == 48000) ? 47999 : rate) * 65536;
	N = M % D;
	M /= D;
	for (i = 0; i < 2; i++) {
		if (*intdec != 0xff) {
			do {
				SEND_SSRC(cif, *intdec, D, M, N);
				SEND_RSRC(cif, *intdec, &rptr);
			}
			while ((rptr.retwords[1] != D)
			       && (rptr.retwords[2] != M)
			       && (rptr.retwords[3] != N)
			       && (i++ < MAX_WRITE_RETRY));
			if (i == MAX_WRITE_RETRY) {
				snd_printd("sent samplerate %d: %d failed\n",
					   *intdec, rate);
				return -EIO;
			}
		}
		intdec++;
	}
	return 0;
}

static int
getmixer(cmdif_t * cif, short num, unsigned short *rval, unsigned short *lval)
{
	cmdret_t rptr = { {(u32) 0, (u32) 0} };

	if (SEND_RDGV(cif, num, num, &rptr) && SEND_RDGV(cif, num, num, &rptr))
		return -EIO;
	*rval = rptr.retwords[0];
	*lval = rptr.retwords[1];
	snd_printd("got mixer %d: 0x%d 0x%d\n", num, *rval, *lval);
	return 0;
}

static void riptide_handleirq(unsigned long dev_id)
{
	riptide_t *chip = (void *)dev_id;
	cmdif_t *cif = chip->cif;
	snd_pcm_substream_t *substream[PLAYBACK_SUBSTREAMS + 1];
	snd_pcm_runtime_t *runtime;
	pcmhw_t *data = NULL;
	unsigned int pos, period_bytes;
	sgd_t *c;
	int i, j;
	unsigned int flag;

	if (! cif)
		return;

	for (i = 0; i < PLAYBACK_SUBSTREAMS; i++)
		substream[i] = chip->playback_substream[i];
	substream[i] = chip->capture_substream;
	for (i = 0; i < PLAYBACK_SUBSTREAMS + 1; i++) {
		if (substream[i]
		    && (runtime = substream[i]->runtime)
		    && (data = runtime->private_data)
		    && (data->state != ST_STOP)) {
			pos = 0;
			for (j = 0; j < data->pages; j++) {
				c = &(((sgd_t *) (data->sgdlist.area))[j]);
				flag = c->dwStat_Ctl;
				if (flag & EOB_STATUS)
					pos += c->dwSegLen;
				if (flag & EOC_STATUS)
					pos += c->dwSegLen;
				if ((flag & EOS_STATUS)
				    && (data->state == ST_PLAY)) {
					data->state = ST_STOP;
					snd_printk("Riptide: DMA stopped unexpectedly\n");
				}
				c->dwStat_Ctl = flag & ~(EOS_STATUS | EOB_STATUS | EOC_STATUS);
			}
			data->pointer += pos;
			pos += data->oldpos;
			if (data->state != ST_STOP) {
				period_bytes = frames_to_bytes(runtime, runtime->period_size);
				snd_printd("interrupt 0x%x after 0x%lx of 0x%lx frames in period\n",
					   READ_AUDIO_STATUS(cif->hwport),
					   bytes_to_frames(runtime, pos),
					   runtime->period_size);
				j = 0;
				if (pos >= period_bytes) {
					j++;
					while (pos >= period_bytes)
						pos -= period_bytes;
				}
				data->oldpos = pos;
				if (j > 0)
					snd_pcm_period_elapsed(substream[i]);
			}
		}
	}
}

#ifdef CONFIG_PM
static void riptide_suspend(riptide_t * chip)
{
	snd_card_t *card;
	unsigned short pciw;

	if (chip) {
		card = chip->card;
		snd_printd("send pci suspend\n");
		chip->in_suspend = 1;
		pci_read_config_word(chip->pci, PCI_EXT_PWSCR, &pciw);
		pci_write_config_word(chip->pci, PCI_EXT_PWSCR,
				      (pciw & ~POWER_STATE_MASK) |
				      D3_POWER_STATE);
		pci_disable_device(chip->pci);
	}
}

static void riptide_resume(riptide_t * chip)
{
	snd_card_t *card;
	unsigned short pciw;

	if (chip) {
		card = chip->card;
		snd_printd("send pci resume\n");
		pci_enable_device(chip->pci);
		pci_set_master(chip->pci);
		pci_read_config_word(chip->pci, PCI_EXT_PWSCR, &pciw);
		pci_write_config_word(chip->pci, PCI_EXT_PWSCR,
				      (pciw & ~POWER_STATE_MASK) |
				      D0_POWER_STATE);
		snd_riptide_initialize(chip);

		if (chip->ac97) {
			snd_printd("send ac97 resume\n");
			snd_ac97_resume(chip->ac97);
		}
		chip->in_suspend = 0;
	}
}
#endif

static int riptide_reset(cmdif_t * cif, riptide_t * chip)
{
	int timeout;
	cmdret_t rptr = { {(u32) 0, (u32) 0} };
	const struct firmware *fw_entry;
	int err;

	if (!(cif))
		return -EINVAL;
	cif->cmdcnt = 0;
	cif->cmdtime = 0;
	cif->cmdtimemax = 0;
	cif->cmdtimemin = 0xffffffff;
	cif->errcnt = 0;
	cif->is_reset = FALSE;

	snd_printd("Resetting\n");
	WRITE_PORT_ULONG(cif->hwport->port[0].data1, 0);
	WRITE_PORT_ULONG(cif->hwport->port[0].data2, 0);
	WRITE_PORT_ULONG(cif->hwport->port[1].data1, 0);
	WRITE_PORT_ULONG(cif->hwport->port[1].data2, 0);
	SET_GRESET(cif->hwport);
	udelay(100);
	UNSET_GRESET(cif->hwport);
	udelay(100);

	snd_printd("Getting version\n");
	TIMEOUT_CMD(timeout, 100000, (IS_READY(cif->hwport))
		    && (!IS_GERR(cif->hwport)),);
	snd_printd("Audio status 0x%x\n", READ_AUDIO_STATUS(cif->hwport));
	SEND_GETV(cif, &rptr);
	snd_printd("Version before firmware upload: ASIC: %d CODEC %d AUXDSP %d PROG %d\n",
		   rptr.retwords[0], rptr.retwords[1], rptr.retwords[2],
		   rptr.retwords[3]);

	if ((chip != NULL) && (rptr.retwords[0] == 0) && (rptr.retwords[1] == 0)
	    && (rptr.retwords[2] == 0) && (rptr.retwords[3] == 0)) {
		snd_printd("Writing Firmware\n");
		if ((err = request_firmware(&fw_entry, "riptide.hex",
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
					    &chip->pci->dev
#else
					    "riptide"
#endif
					    )) != 0) {
			snd_printk("Riptide: Firmware not available %d\n", err);
			return -EIO;
		}

		err = loadfirmware(cif, fw_entry->data, fw_entry->size);
		release_firmware(fw_entry);

		if (err)
			snd_printk("Riptide: Could not load firmware %d\n", err);
	}

	TIMEOUT_CMD(timeout, 100000, (IS_READY(cif->hwport))
		    && (!IS_GERR(cif->hwport)),);
	if (timeout == 0) {
		snd_printd("audio status 0x%x %d %d\n",
			   READ_AUDIO_STATUS(cif->hwport),
			   IS_READY(cif->hwport), IS_GERR(cif->hwport));
		return -EIO;
	}
	SEND_GETV(cif, &rptr);
	TIMEOUT_CMD(timeout, 100, (rptr.retwords[0] != 0)
		    || (rptr.retwords[1] != 0) || (rptr.retwords[2] != 0)
		    || (rptr.retwords[3] != 0), SEND_GETV(cif, &rptr);
	    );
	snd_printk("Riptide: version after firmware upload: ASIC: %d CODEC %d AUXDSP %d PROG %d (delay %d)\n",
		   rptr.retwords[0], rptr.retwords[1], rptr.retwords[2],
		   rptr.retwords[3], timeout);
	if ((rptr.retwords[0] == 0) && (rptr.retwords[1] == 0)
	    && (rptr.retwords[2] == 0) && (rptr.retwords[3] == 0)) {
		snd_printk("Riptide: no firmware version?\n");
		return -EIO;
	}
	snd_printd("Resetting AC97\n");
	SEND_SACR(cif, 0, AC97_RESET);
	SEND_RACR(cif, AC97_RESET, &rptr);
	snd_printd("AC97: 0x%x 0x%x\n", rptr.retlongs[0], rptr.retlongs[1]);
	SEND_PLST(cif, 0);
	SEND_SLST(cif, 0);
	SEND_DLST(cif, 0);
	SEND_ALST(cif, 0);
	SEND_KDMA(cif);
	snd_printd("HP Power Amp?\n");
	writearm(cif, 0x301F8, 1, 1);
	writearm(cif, 0x301F4, 1, 1);
	snd_printd("Set sources/paths for modem\n");
	SEND_LSEL(cif, MODEM_CMD, 0, 0, MODEM_INTDEC, MODEM_MERGER,
		  MODEM_SPLITTER, MODEM_MIXER);
	setmixer(cif, MODEM_MIXER, 0x7fff, 0x7fff);
	alloclbuspath(cif, ARM2LBUS_FIFO13, lbus_play_modem, NULL, NULL);
	snd_printd("Set sources/paths for opl3\n");
	SEND_LSEL(cif, FM_CMD, 0, 0, FM_INTDEC, FM_MERGER, FM_SPLITTER,
		  FM_MIXER);
	setmixer(cif, FM_MIXER, 0x7fff, 0x7fff);
	writearm(cif, 0x30648 + FM_MIXER * 4, 0x01, 0x00000005);
	writearm(cif, 0x301A8, 0x02, 0x00000002);
	writearm(cif, 0x30264, 0x08, 0xffffffff);
	alloclbuspath(cif, OPL3_SAMPLE, lbus_play_opl3, NULL, NULL);
	snd_printd("Set sources/paths for i2s\n");
	SEND_SSRC(cif, I2S_INTDEC, 48000,
		  ((u32) I2S_RATE * 65536) / 48000,
		  ((u32) I2S_RATE * 65536) % 48000);
	SEND_LSEL(cif, I2S_CMD0, 0, 0, I2S_INTDEC, I2S_MERGER, I2S_SPLITTER,
		  I2S_MIXER);
	SEND_SI2S(cif, 1);
	alloclbuspath(cif, ARM2LBUS_FIFO0, lbus_play_i2s, NULL, NULL);
	alloclbuspath(cif, DIGITAL_MIXER_OUT0, lbus_play_out, NULL, NULL);
	alloclbuspath(cif, DIGITAL_MIXER_OUT0, lbus_play_outhp, NULL, NULL);
	snd_printd("Clear pending interrupts\n");
	SET_AIACK(cif->hwport);
	SET_AIE(cif->hwport);
	SET_AIACK(cif->hwport);
	cif->is_reset = TRUE;
	return 0;
}

static snd_pcm_uframes_t snd_riptide_pointer(snd_pcm_substream_t * substream)
{
	riptide_t *chip = snd_pcm_substream_chip(substream);
	snd_pcm_runtime_t *runtime = substream->runtime;
	pcmhw_t *data = get_pcmhwdev(substream);
	cmdif_t *cif = chip->cif;
	cmdret_t rptr = { {(u32) 0, (u32) 0} };
	snd_pcm_uframes_t ret;

	snd_printd("get pointer cif:0x%p id:%d\n", cif, data->id);
	SEND_GPOS(cif, 0, data->id, &rptr);
	if ((data->size != 0) && (runtime->period_size != 0)) {
		snd_printd("pointer stream %d position 0x%x(0x%x in buffer) bytes 0x%lx(0x%lx in period) frames\n",
			   data->id, rptr.retlongs[1], rptr.retlongs[1] % data->size,
			   bytes_to_frames(runtime, rptr.retlongs[1]),
			   bytes_to_frames(runtime,
					   rptr.retlongs[1]) % runtime->period_size);
		if (rptr.retlongs[1] > data->pointer)
			ret = bytes_to_frames(runtime, rptr.retlongs[1] % data->size);
		else
			ret = bytes_to_frames(runtime, data->pointer % data->size);
	} else {
		snd_printd("stream not started or strange parms (%d %ld)\n",
			   data->size, runtime->period_size);
		ret = bytes_to_frames(runtime, 0);
	}
	return ret;
}

static int snd_riptide_trigger(snd_pcm_substream_t * substream, int cmd)
{
	unsigned long irqflags;
	int i, j;
	riptide_t *chip = snd_pcm_substream_chip(substream);
	pcmhw_t *data = get_pcmhwdev(substream);
	cmdif_t *cif = chip->cif;
	cmdret_t rptr = { {(u32) 0, (u32) 0} };

	snd_printd("trigger id %d cmd 0x%x\n", data->id, cmd);
	spin_lock_irqsave(&chip->lock, irqflags);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		if (!(data->state & ST_PLAY)) {
			SEND_SSTR(cif, data->id, data->sgdlist.addr);
			SET_AIE(cif->hwport);
			data->state = ST_PLAY;
			if (data->mixer != 0xff)
				setmixer(cif, data->mixer, 0x7fff, 0x7fff);
			(chip->openstreams)++;
			data->oldpos = 0;
			data->pointer = 0;
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		if (data->mixer != 0xff)
			setmixer(cif, data->mixer, 0, 0);
		setmixer(cif, data->mixer, 0, 0);
		SEND_KSTR(cif, data->id);
		data->state = ST_STOP;
		(chip->openstreams)--;
		j = 0;
		do {
			i = rptr.retlongs[1];
			SEND_GPOS(cif, 0, data->id, &rptr);
			udelay(1);
		}
		while ((i != rptr.retlongs[1]) && (j++ < MAX_WRITE_RETRY));
		if (j >= MAX_WRITE_RETRY)
			snd_printk("Riptide: Could not stop stream!");
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (!(data->state & ST_PAUSE)) {
			SEND_PSTR(cif, data->id);
			data->state |= ST_PAUSE;
			(chip->openstreams)--;
		}
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (data->state & ST_PAUSE) {
			SEND_SSTR(cif, data->id, data->sgdlist.addr);
			data->state &= ~ST_PAUSE;
			(chip->openstreams)++;
		}
		break;
	default:
		spin_unlock_irqrestore(&chip->lock, irqflags);
		return -EINVAL;
	}
	spin_unlock_irqrestore(&chip->lock, irqflags);
	return 0;
}

static int snd_riptide_prepare(snd_pcm_substream_t * substream)
{
	riptide_t *chip = snd_pcm_substream_chip(substream);
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_pcm_sgbuf_t *sgbuf = snd_pcm_substream_sgbuf(substream);
	pcmhw_t *data = get_pcmhwdev(substream);
	cmdif_t *cif = chip->cif;
	unsigned char *lbuspath = NULL;
	unsigned int rate, channels;
	unsigned long irqflags;
	int err = -EINVAL;
	snd_pcm_format_t format;

	spin_lock_irqsave(&chip->lock, irqflags);
	if (cif && data) {
		snd_printd("prepare id %d ch: %d f:0x%x r:%d\n", data->id,
			   runtime->channels, runtime->format, runtime->rate);
		channels = runtime->channels;
		format = runtime->format;
		rate = runtime->rate;
		switch (channels) {
		case 1:
			if ((rate == 48000)
			    && (format == SNDRV_PCM_FORMAT_S16_LE))
				lbuspath = data->paths.noconv;
			else
				lbuspath = data->paths.mono;
			break;
		case 2:
			if ((rate == 48000)
			    && (format == SNDRV_PCM_FORMAT_S16_LE))
				lbuspath = data->paths.noconv;
			else
				lbuspath = data->paths.stereo;
			break;
		}
		snd_printd("use sgdlist at 0x%p and buffer at 0x%p\n",
			   data->sgdlist.area, sgbuf);
		if (data->sgdlist.area && sgbuf) {
			unsigned int i, j, size, pages, f, pt, period;
			sgd_t *c, *p = NULL;

			size = frames_to_bytes(runtime, runtime->buffer_size);
			period = frames_to_bytes(runtime, runtime->period_size);
			f = PAGE_SIZE;
			while (((size + (f >> 1) - 1) <= (f << 7))
			       && ((f << 1) > period))
				f = f >> 1;
			pages = (size + f - 1) / f;
			data->size = size;
			data->pages = pages;
			snd_printd("create sgd size: 0x%x pages %d of size 0x%x for period 0x%x\n",
				   size, pages, f, period);
			pt = 0;
			j = 0;
			for (i = 0; i < pages; i++) {
				c = &(((sgd_t *) (data->sgdlist.area))[i]);
				if (p)
					p->dwNextLink = data->sgdlist.addr +
						(i * sizeof(sgd_t));
				c->dwNextLink = data->sgdlist.addr;
				c->dwSegPtrPhys = sgbuf->table[j].addr + pt;
				pt = (pt + f) % PAGE_SIZE;
				if (pt == 0)
					j++;
				c->dwSegLen = f;
				c->dwStat_Ctl = IEOB_ENABLE | IEOS_ENABLE | IEOC_ENABLE;
				p = c;
				size -= f;
			}
			((sgd_t *) (data->sgdlist.area))[i].dwSegLen = size;
		}
		if (lbuspath && (lbuspath != data->lbuspath)) {
			if (data->lbuspath)
				freelbuspath(cif, data->source, data->lbuspath);
			alloclbuspath(cif, data->source, lbuspath,
				      &data->mixer, data->intdec);
			data->lbuspath = lbuspath;
			data->rate = 0;
		}
		if ((data->rate != rate) || (data->format != format)
		    || (data->channels != channels)) {
			data->rate = rate;
			data->format = format;
			data->channels = channels;
			if (setsampleformat(cif, data->mixer, data->id,
					    channels, format)
			    || setsamplerate(cif, data->intdec, rate))
				err = -EIO;
			else
				err = 0;
		}
		err = 0;
	}
	snd_printd("prepare returned %d\n", err);
	spin_unlock_irqrestore(&chip->lock, irqflags);
	return err;
}

static int
snd_riptide_hw_params(snd_pcm_substream_t * substream,
		      snd_pcm_hw_params_t * hw_params)
{
	riptide_t *chip = snd_pcm_substream_chip(substream);
	pcmhw_t *data = get_pcmhwdev(substream);
	struct snd_dma_buffer *sgdlist = &data->sgdlist;
	int err = -EINVAL;

	snd_printd("hw params id %d (sgdlist: 0x%p 0x%x %d)\n", data->id,
		   sgdlist->area, sgdlist->addr, sgdlist->bytes);
	if (sgdlist->area)
		snd_dma_free_pages(sgdlist);
	if ((err =
	     snd_dma_alloc_pages(SNDRV_DMA_TYPE_DEV,
				 snd_dma_pci_data(chip->pci),
				 sizeof(sgd_t) * (DESC_MAX_MASK + 1),
				 sgdlist)) < 0) {
		snd_printd("failed to alloc %d dma bytes\n",
			   sizeof(sgd_t) * (DESC_MAX_MASK + 1));
		return err;
	}
	if ((err = snd_pcm_lib_malloc_pages(substream,
					    params_buffer_bytes(hw_params))) <
	    0)
		snd_printd("failed to malloc substream pages (size %d)\n",
			   params_buffer_bytes(hw_params));
	return err;
}

static int snd_riptide_hw_free(snd_pcm_substream_t * substream)
{
	riptide_t *chip = snd_pcm_substream_chip(substream);
	pcmhw_t *data = get_pcmhwdev(substream);
	cmdif_t *cif = chip->cif;

	snd_printd("hw free id %d\n", data->id);
	if (cif && data) {
		if (data->lbuspath)
			freelbuspath(cif, data->source, data->lbuspath);
		data->lbuspath = NULL;
		data->source = 0xff;
		data->intdec[0] = 0xff;
		data->intdec[1] = 0xff;
	}
	if (data->sgdlist.area) {
		snd_dma_free_pages(&data->sgdlist);
		data->sgdlist.area = NULL;
	}
	return snd_pcm_lib_free_pages(substream);
}

static int snd_riptide_playback_open(snd_pcm_substream_t * substream)
{
	riptide_t *chip = snd_pcm_substream_chip(substream);
	snd_pcm_runtime_t *runtime = substream->runtime;
	pcmhw_t *data;
	int index = substream->number;

	snd_printd("hw open playback %d\n", index);
	chip->playback_substream[index] = substream;
	runtime->hw = snd_riptide_playback;
	data = kcalloc(1, sizeof(pcmhw_t), GFP_KERNEL);
	data->paths = lbus_play_paths[index];
	data->id = play_ids[index];
	data->source = play_sources[index];
	data->intdec[0] = 0xff;
	data->intdec[1] = 0xff;
	data->state = ST_STOP;
	runtime->private_data = data;
	return snd_pcm_hw_constraint_integer(runtime,
					     SNDRV_PCM_HW_PARAM_PERIODS);
}

static int snd_riptide_capture_open(snd_pcm_substream_t * substream)
{
	riptide_t *chip = snd_pcm_substream_chip(substream);
	snd_pcm_runtime_t *runtime = substream->runtime;
	pcmhw_t *data;

	snd_printd("hw open capture\n");
	chip->capture_substream = substream;
	runtime->hw = snd_riptide_capture;
	data = kcalloc(1, sizeof(pcmhw_t), GFP_KERNEL);
	data->paths = lbus_rec_path;
	data->id = PADC;
	data->source = ACLNK2PADC;
	data->intdec[0] = 0xff;
	data->intdec[1] = 0xff;
	data->state = ST_STOP;
	runtime->private_data = data;
	return snd_pcm_hw_constraint_integer(runtime,
					     SNDRV_PCM_HW_PARAM_PERIODS);
}

static int snd_riptide_playback_close(snd_pcm_substream_t * substream)
{
	riptide_t *chip = snd_pcm_substream_chip(substream);
	pcmhw_t *data = get_pcmhwdev(substream);
	int index = substream->number;

	snd_printd("hw close playback %d\n", index);
	kfree(data);
	substream->runtime->private_data = NULL;
	chip->playback_substream[index] = NULL;
	return 0;
}

static int snd_riptide_capture_close(snd_pcm_substream_t * substream)
{
	riptide_t *chip = snd_pcm_substream_chip(substream);
	pcmhw_t *data = get_pcmhwdev(substream);

	snd_printd("hw close capture %d\n", substream->number);
	kfree(data);
	substream->runtime->private_data = NULL;
	chip->capture_substream = NULL;
	return 0;
}

static void snd_riptide_pcm_free(snd_pcm_t * pcm)
{
	riptide_t *chip = pcm->private_data;

	snd_printd("free pcm\n");
	chip->pcm = NULL;
	snd_pcm_lib_preallocate_free_for_all(pcm);
}

static int __devinit
snd_riptide_pcm(riptide_t * chip, int device, snd_pcm_t ** rpcm)
{
	snd_pcm_t *pcm;
	int err;

	snd_printd("new pcm %d 0x%p\n", device, chip);
	if (rpcm)
		*rpcm = NULL;
	if ((err =
	     snd_pcm_new(chip->card, "RIPTIDE", device, PLAYBACK_SUBSTREAMS, 1,
			 &pcm)) < 0)
		return err;
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK,
			&snd_riptide_playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE,
			&snd_riptide_capture_ops);
	pcm->private_data = chip;
	pcm->private_free = snd_riptide_pcm_free;
	pcm->info_flags = 0;
	strcpy(pcm->name, "RIPTIDE");
	chip->pcm = pcm;
	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV_SG,
					      snd_dma_pci_data(chip->pci),
					      64 * 1024, 128 * 1024);
	if (rpcm)
		*rpcm = pcm;
	return 0;
}

static irqreturn_t
snd_riptide_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	riptide_t *chip = dev_id;
	cmdif_t *cif = chip->cif;

	if (cif) {
		(chip->received_irqs)++;
		if (IS_EOBIRQ(cif->hwport) || IS_EOSIRQ(cif->hwport)
		    || IS_EOCIRQ(cif->hwport)) {
			(chip->handled_irqs)++;
			tasklet_hi_schedule(&(chip->riptide_tq));
		}
		if (chip->rmidi && IS_MPUIRQ(cif->hwport)) {
			(chip->handled_irqs)++;
			snd_printd("send mpu irq\n");
			snd_mpu401_uart_interrupt(irq,
						  chip->rmidi->private_data,
						  regs);
		}
		SET_AIACK(cif->hwport);
	}
	return IRQ_HANDLED;
}

static void
snd_riptide_codec_write(ac97_t * ac97, unsigned short reg, unsigned short val)
{
	riptide_t *chip = ac97->private_data;
	cmdif_t *cif = chip->cif;
	cmdret_t rptr = { {(u32) 0, (u32) 0}
	};
	int i = 0;

	if (cif) {
		do {
			SEND_SACR(cif, val, reg);
			SEND_RACR(cif, reg, &rptr);
		}
		while ((rptr.retwords[1] != val) && (i++ < MAX_WRITE_RETRY));
		if (i == MAX_WRITE_RETRY) {
			snd_printd("Write AC97 reg 0x%x 0x%x failed\n", reg,
				   val);
			return;
		}
		snd_printd("Write AC97 reg 0x%x 0x%x\n", reg, val);
	}
}

static unsigned short snd_riptide_codec_read(ac97_t * ac97, unsigned short reg)
{
	riptide_t *chip = ac97->private_data;
	cmdif_t *cif = chip->cif;
	cmdret_t rptr = { {(u32) 0, (u32) 0}
	};

	if (cif) {
		if (SEND_RACR(cif, reg, &rptr) != 0)
			SEND_RACR(cif, reg, &rptr);
		snd_printd("Read AC97 reg 0x%x got 0x%x\n", reg,
			   rptr.retwords[1]);
		return rptr.retwords[1];
	}
	snd_printd("Read AC97 reg 0x%x failed\n", reg);
	return 0;
}

#ifdef CONFIG_PM
static int snd_riptide_suspend(snd_card_t * card, pm_message_t state)
{
	riptide_t *chip = card->pm_private_data;

	riptide_suspend(chip);
	return 0;
}
static int snd_riptide_resume(snd_card_t * card)
{
	riptide_t *chip = card->pm_private_data;

	riptide_resume(chip);
	return 0;
}

#endif

static int snd_riptide_initialize(riptide_t * chip)
{
	cmdif_t *cif = NULL;
	unsigned long device_id = 0;
	int err = 0;

	snd_printd("initialize 0x%p\n", chip);
	if (chip) {
		device_id = chip->device_id;
		if ((cif = kcalloc(1, sizeof(cmdif_t), GFP_KERNEL)) == NULL)
			return -ENOMEM;
		cif->hwport = (riptideport_t *) chip->port;
		cif->is_reset = FALSE;
		chip->cif = cif;
		spin_lock_init(&(cif->lock));
		err = riptide_reset(cif, chip);
		switch (device_id) {
		case 0x4310:
		case 0x4320:
		case 0x4330:
			snd_printd("Modem enable?\n");
			SEND_SETDPLL(cif);
			break;
		}
		snd_printd("Enabling MPU IRQs\n");
		if (chip->rmidi)
			SET_EMPUIRQ(cif->hwport);
		return err;
	}
	return -EINVAL;
}

static int snd_riptide_hw_close(riptide_t * chip)
{
	snd_printd("hw close\n");
	if (chip)
		return snd_riptide_free(chip);
	return 0;
}

static int snd_riptide_free(riptide_t * chip)
{
	cmdif_t *cif;

	snd_printd("riptide free\n");
	if (chip && (cif = chip->cif)) {
		snd_printd("riptide reset\n");
		SET_GRESET(cif->hwport);
		udelay(100);
		UNSET_GRESET(cif->hwport);
		kfree(chip->cif);
		chip->cif = NULL;
	}
	if (chip->res_port) {
		snd_printd("release port\n");
		release_resource(chip->res_port);
		kfree_nocheck(chip->res_port);
		chip->res_port = NULL;
	}
	if (chip->irq >= 0) {
		snd_printd("free irq\n");
		free_irq(chip->irq, (void *)chip);
		chip->irq = 0;
	}
	kfree(chip);
	return 0;
}

static int snd_riptide_dev_free(snd_device_t * device)
{
	riptide_t *chip = device->device_data;

	snd_printd("free device\n");
	return snd_riptide_free(chip);
}

static int __devinit
snd_riptide_create(snd_card_t * card, struct pci_dev *pci, riptide_t ** rchip)
{
	riptide_t *chip;
	riptideport_t *hwport;
	int err;

	*rchip = NULL;
	if ((err = pci_enable_device(pci)) < 0)
		return err;
	chip = kcalloc(1, sizeof(riptide_t), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;
	spin_lock_init(&(chip->lock));
	chip->card = card;
	chip->pci = pci;
	chip->irq = -1;
	chip->openstreams = 0;
	chip->port = pci_resource_start(pci, 0);
	chip->received_irqs = 0;
	chip->handled_irqs = 0;
	chip->cif = NULL;
	tasklet_init(&chip->riptide_tq, riptide_handleirq, (unsigned long)chip);
	snd_printd("grabbing region\n");
	if ((chip->res_port =
	     request_region(chip->port, 64, "RIPTIDE")) == NULL) {
		snd_printk("Riptide: unable to grab region 0x%lx-0x%lx\n",
			   chip->port, chip->port + 64 - 1);
		snd_riptide_free(chip);
		return -EBUSY;
	}
	hwport = (riptideport_t *) chip->port;
	UNSET_AIE(hwport);
	snd_printd("grabbing irq\n");
	if (request_irq(pci->irq, snd_riptide_interrupt, SA_INTERRUPT | SA_SHIRQ,
			"RIPTIDE", (void *)chip)) {
		snd_printk(KERN_ERR "Riptide: unable to grab IRQ %d\n", pci->irq);
		snd_riptide_free(chip);
		return -EBUSY;
	}
	chip->irq = pci->irq;
	chip->device_id = pci->device;
	pci_set_master(pci);
	snd_printd("initializing\n");
	if ((err = snd_riptide_initialize(chip)) < 0) {
		snd_riptide_hw_close(chip);
		return err;
	}
#ifdef CONFIG_PM
	snd_card_set_pm_callback(card, snd_riptide_suspend, snd_riptide_resume,
				 chip);
#endif

	if ((err = snd_device_new(card, SNDRV_DEV_LOWLEVEL, chip, &ops)) < 0) {
		snd_riptide_hw_close(chip);
		return err;
	}

	*rchip = chip;
	return 0;
}

static void
snd_riptide_proc_read(snd_info_entry_t * entry, snd_info_buffer_t * buffer)
{
	riptide_t *chip = entry->private_data;
	pcmhw_t *data;
	int i;
	cmdif_t *cif = NULL;
	cmdret_t rptr = { {(u32) 0, (u32) 0} };
	unsigned char p[256];
	unsigned short rval, lval;
	unsigned int rate;

	if (chip) {
		snd_iprintf(buffer, "%s\n\n", chip->card->longname);
		snd_iprintf(buffer,
			    "Device ID: 0x%x\nReceived IRQs: (%ld)%ld\nPorts:",
			    chip->device_id, chip->handled_irqs,
			    chip->received_irqs);
		for (i = 0; i < 64; i += 4)
			snd_iprintf(buffer, "%c%02x: %08x",
				    (i % 16) ? ' ' : '\n', i,
				    inl(chip->port + i));
		if ((cif = chip->cif)) {
			i = SEND_GETV(cif, &rptr);
			snd_iprintf
			    (buffer,
			     "\nVersion: %d ASIC: %d CODEC: %d AUXDSP: %d PROG: %d",
			     i, rptr.retwords[0], rptr.retwords[1],
			     rptr.retwords[2], rptr.retwords[3]);
			snd_iprintf(buffer, "\nDigital mixer:");
			for (i = 0; i < 12; i++) {
				getmixer(cif, i, &rval, &lval);
				snd_iprintf(buffer, "\n %d: %d %d", i, rval,
					    lval);
			}
			snd_iprintf(buffer,
				    "\nARM Commands num: %d failed: %d time: %d max: %d min: %d",
				    cif->cmdcnt, cif->errcnt,
				    cif->cmdtime, cif->cmdtimemax,
				    cif->cmdtimemin);
		}
		snd_iprintf(buffer, "\nOpen streams %d:\n", chip->openstreams);
		for (i = 0; i < PLAYBACK_SUBSTREAMS; i++) {
			if (chip->playback_substream[i]
			    && chip->playback_substream[i]->runtime
			    && (data =
				chip->playback_substream[i]->runtime->
				private_data)) {
				snd_iprintf(buffer,
					    "stream: %d mixer: %d source: %d (%d,%d)\n",
					    data->id, data->mixer, data->source,
					    data->intdec[0], data->intdec[1]);
				if (!(getsamplerate(cif, data->intdec, &rate)))
					snd_iprintf(buffer, "rate: %d\n", rate);
			}
		}
		if (chip->capture_substream
		    && chip->capture_substream->runtime
		    && (data = chip->capture_substream->runtime->private_data))
		{
			snd_iprintf(buffer,
				    "stream: %d mixer: %d source: %d (%d,%d)\n",
				    data->id, data->mixer,
				    data->source, data->intdec[0],
				    data->intdec[1]);
			if (!(getsamplerate(cif, data->intdec, &rate)))
				snd_iprintf(buffer, "rate: %d\n", rate);
		}
		snd_iprintf(buffer, "Paths:\n");
		i = getpaths(cif, p);
		while (i--) {
			snd_iprintf(buffer, "%x->%x ", p[i - 1], p[i]);
			i--;
		}
		snd_iprintf(buffer, "\n");
	}
}

static void __devinit snd_riptide_proc_init(riptide_t * chip)
{
	snd_info_entry_t *entry;

	if (!snd_card_proc_new(chip->card, "riptide", &entry))
		snd_info_set_text_ops(entry, chip, 4096, snd_riptide_proc_read);
}

static void snd_riptide_mixer_free_ac97(ac97_t * ac97)
{
	riptide_t *chip = ac97->private_data;
	chip->ac97 = NULL;
}

static void snd_riptide_mixer_free_ac97_bus(ac97_bus_t * ac97)
{
	riptide_t *chip = ac97->private_data;
	chip->ac97_bus = NULL;
}

static int __devinit snd_riptide_mixer(riptide_t * chip)
{
	ac97_bus_t *pbus;
	ac97_template_t ac97;
	int err = 0;
	static ac97_bus_ops_t ops = {
		.write = snd_riptide_codec_write,
		.read = snd_riptide_codec_read,
	};

	memset(&ac97, 0, sizeof(ac97));
	ac97.private_data = chip;
	ac97.private_free = snd_riptide_mixer_free_ac97;
	ac97.scaps = AC97_SCAP_SKIP_MODEM;

	if ((err = snd_ac97_bus(chip->card, 0, &ops, chip, &pbus)) < 0)
		return err;
	pbus->private_free = snd_riptide_mixer_free_ac97_bus;
	pbus->shared_type = AC97_SHARED_TYPE_NONE;	/* shared with modem driver ? */

	chip->ac97_bus = pbus;
	ac97.pci = chip->pci;
	if ((err = snd_ac97_mixer(pbus, &ac97, &chip->ac97)) < 0)
		return err;
	return err;
}

#ifdef SUPPORT_JOYSTICK
static int __devinit
snd_riptide_joystick_probe(struct pci_dev *pci, const struct pci_device_id *id)
{
	static int dev;

	if (dev >= SNDRV_CARDS)
		return -ENODEV;
	if (!enable[dev]) {
		dev++;
		return -ENOENT;
	}

	if (joystick_port[dev]) {
		riptide_gameport = kcalloc(1, sizeof(gameport_t), GFP_KERNEL);
		if (riptide_gameport) {
			if (!request_region(joystick_port[dev], 8, "Riptide gameport"))
				snd_printk(KERN_ERR "Riptide: cannot grab gameport 0x%x\n",
					   joystick_port[dev]);
			else {
				riptide_gameport_pci = pci;
				riptide_gameport->io = joystick_port[dev];
				gameport_register_port(riptide_gameport);
			}
		}
	}
	dev++;
	return 0;
}

static void __devexit snd_riptide_joystick_remove(struct pci_dev *pci)
{
	if (riptide_gameport) {
		if (riptide_gameport_pci == pci) {
			release_region(riptide_gameport->io, 8);
			riptide_gameport_pci = NULL;
			gameport_unregister_port(riptide_gameport);
		}
		kfree(riptide_gameport);
	}
}
#endif

static int __devinit
snd_card_riptide_probe(struct pci_dev *pci, const struct pci_device_id *pci_id)
{
	static int dev;
	snd_card_t *card;
	riptide_t *chip;
	unsigned short addr;
	int err = 0;

	if (dev >= SNDRV_CARDS)
		return -ENODEV;
	if (!enable[dev]) {
		dev++;
		return -ENOENT;
	}

	card = snd_card_new(index[dev], id[dev], THIS_MODULE, 0);
	if (card == NULL)
		return -ENOMEM;
	snd_printd("Creating riptide instance\n");
	if ((err = snd_riptide_create(card, pci, &chip)) < 0) {
		snd_printd("failed to create riptide\n");
		snd_card_free(card);
		return err;
	}
	snd_printd("Creating pcm instance\n");
	if ((err = snd_riptide_pcm(chip, 0, NULL)) < 0) {
		snd_printd("failed to create pcm\n");
		snd_card_free(card);
		return err;
	}
	snd_printd("Creating mixer instance\n");
	if ((err = snd_riptide_mixer(chip)) < 0) {
		snd_printd("failed to create mixer\n");
		snd_card_free(card);
		return err;
	}
	snd_printd("Enabling legacy devices instance\n");
	pci_write_config_word(chip->pci, PCI_EXT_Legacy_Mask, LEGACY_ENABLE_ALL
			      | ((opl3_port[dev]) ? LEGACY_ENABLE_FM : 0)
#ifdef SUPPORT_JOYSTICK
			      | ((joystick_port[dev]) ? LEGACY_ENABLE_GAMEPORT :
				 0)
#endif
			      | ((mpu_port[dev])
				 ? (LEGACY_ENABLE_MPU_INT | LEGACY_ENABLE_MPU) :
				 0)
			      | ((chip->irq << 4) & 0xF0));
	if ((addr = mpu_port[dev]) != 0) {
		pci_write_config_word(chip->pci, PCI_EXT_MPU_Base, addr);
		if ((err =
		     snd_mpu401_uart_new(card, 0, MPU401_HW_RIPTIDE,
					 addr, 0, chip->irq, 0,
					 &chip->rmidi)) < 0)
			snd_printk("Riptide: Can't Allocate MPU at 0x%x\n",
				   addr);
		else
			chip->mpuaddr = addr;
	}
	if ((addr = opl3_port[dev]) != 0) {
		pci_write_config_word(chip->pci, PCI_EXT_FM_Base, addr);
		if ((err =
		     snd_opl3_create(card, addr, addr + 2,
				     OPL3_HW_RIPTIDE, 0, &chip->opl3)) < 0)
			snd_printk("Riptide: Can't Allocate OPL3 at 0x%x\n",
				   addr);
		else {
			chip->opladdr = addr;
			if ((err =
			     snd_opl3_hwdep_new(chip->opl3, 0, 1, NULL)) < 0)
				snd_printk
				    ("Riptide: Can't Allocate OPL3-HWDEP\n");
		}
	}
#ifdef SUPPORT_JOYSTICK
	if ((addr = joystick_port[dev]) != 0) {
		pci_write_config_word(chip->pci, PCI_EXT_Game_Base, addr);
		chip->gameaddr = addr;
	}
#endif

	strcpy(card->driver, "RIPTIDE");
	strcpy(card->shortname, "Riptide");
#ifdef SUPPORT_JOYSTICK
	sprintf(card->longname,
		"%s at 0x%lx, irq %i mpu 0x%x opl3 0x%x gameport 0x%x",
		card->shortname, chip->port, chip->irq, chip->mpuaddr,
		chip->opladdr, chip->gameaddr);
#else
	sprintf(card->longname,
		"%s at 0x%lx, irq %i mpu 0x%x opl3 0x%x",
		card->shortname, chip->port, chip->irq, chip->mpuaddr,
		chip->opladdr);
#endif
	snd_riptide_proc_init(chip);
	if ((err = snd_card_register(card)) < 0) {
		snd_printd("failed to register card\n");
		snd_card_free(card);
		return err;
	}
	pci_set_drvdata(pci, card);
	dev++;
	return 0;
}

static void __devexit snd_card_riptide_remove(struct pci_dev *pci)
{
	snd_card_free(pci_get_drvdata(pci));
	pci_set_drvdata(pci, NULL);
}

static int __init alsa_card_riptide_init(void)
{
	int err;
	if ((err = pci_register_driver(&driver)) < 0)
		return err;
#if defined(SUPPORT_JOYSTICK)
	if (pci_register_driver(&joystick_driver) < 0) {
		have_joystick = 0;
		snd_printk(KERN_INFO "no joystick found\n");
	} else
		have_joystick = 1;
#endif
	return 0;
}

static void __exit alsa_card_riptide_exit(void)
{
	pci_unregister_driver(&driver);
#if defined(SUPPORT_JOYSTICK)
	if (have_joystick)
		pci_unregister_driver(&joystick_driver);
#endif
}

module_init(alsa_card_riptide_init);
module_exit(alsa_card_riptide_exit);

EXPORT_NO_SYMBOLS;
