/*
 *  Advanced Linux Sound Architecture - ALSA - Driver
 *  Copyright (c) 1994-2000 by Jaroslav Kysela <perex@suse.cz>,
 *                             Abramo Bagnara <abramo@alsa-project.org>
 *
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

#ifndef __ASOUND_H
#define __ASOUND_H

#if defined(LINUX) || defined(__LINUX__) || defined(__linux__)
#include <linux/ioctl.h>
#include <endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define SND_LITTLE_ENDIAN
#elif __BYTE_ORDER == __BIG_ENDIAN
#define SND_BIG_ENDIAN
#else
#error "Unsupported endian..."
#endif
#endif
#ifndef __KERNEL__
#include <sys/time.h>
#include <sys/types.h>
#include <asm/page.h>
#endif

/*
 *  protocol version
 */

#define SND_PROTOCOL_VERSION(major, minor, subminor) (((major)<<16)|((minor)<<8)|(subminor))
#define SND_PROTOCOL_MAJOR(version) (((version)>>16)&0xffff)
#define SND_PROTOCOL_MINOR(version) (((version)>>8)&0xff)
#define SND_PROTOCOL_MICRO(version) ((version)&0xff)
#define SND_PROTOCOL_INCOMPATIBLE(kversion, uversion) \
	(SND_PROTOCOL_MAJOR(kversion) != SND_PROTOCOL_MAJOR(uversion) || \
	 (SND_PROTOCOL_MAJOR(kversion) == SND_PROTOCOL_MAJOR(uversion) && \
	   SND_PROTOCOL_MINOR(kversion) != SND_PROTOCOL_MINOR(uversion)))

/*
 *  various limits
 */

#define SND_CARDS			8

/*
 *  Types of sound drivers...
 *  Note: Do not assign a new number to 100% clones...
 *  Note: The order should not be preserved but the assigment must be!!!
 */

#define SND_CARD_TYPE_GUS_CLASSIC	0x00000001	/* GUS Classic */
#define SND_CARD_TYPE_GUS_EXTREME	0x00000002	/* GUS Extreme */
#define SND_CARD_TYPE_GUS_ACE		0x00000003	/* GUS ACE */
#define SND_CARD_TYPE_GUS_MAX		0x00000004	/* GUS MAX */
#define SND_CARD_TYPE_AMD_INTERWAVE	0x00000005	/* GUS PnP - AMD InterWave */
#define SND_CARD_TYPE_SB_10		0x00000006	/* SoundBlaster v1.0 */
#define SND_CARD_TYPE_SB_20		0x00000007	/* SoundBlaster v2.0 */
#define SND_CARD_TYPE_SB_PRO		0x00000008	/* SoundBlaster Pro */
#define SND_CARD_TYPE_SB_16		0x00000009	/* SoundBlaster 16 */
#define SND_CARD_TYPE_SB_AWE		0x0000000a	/* SoundBlaster AWE */
#define SND_CARD_TYPE_ESS_ES1688	0x0000000b	/* ESS AudioDrive ESx688 */
#define SND_CARD_TYPE_OPL3_SA2		0x0000000c	/* Yamaha OPL3 SA2/SA3 */
#define SND_CARD_TYPE_MOZART		0x0000000d	/* OAK Mozart */
#define SND_CARD_TYPE_S3_SONICVIBES	0x0000000e	/* S3 SonicVibes */
#define SND_CARD_TYPE_ENS1370		0x0000000f	/* Ensoniq ES1370 */
#define SND_CARD_TYPE_ENS1371		0x00000010	/* Ensoniq ES1371 */
#define SND_CARD_TYPE_CS4232		0x00000011	/* CS4232/CS4232A */
#define SND_CARD_TYPE_CS4236		0x00000012	/* CS4235/CS4236B/CS4237B/CS4238B/CS4239 */
#define SND_CARD_TYPE_AMD_INTERWAVE_STB	0x00000013	/* AMD InterWave + TEA6330T */
#define SND_CARD_TYPE_ESS_ES1938	0x00000014	/* ESS Solo-1 ES1938 */
#define SND_CARD_TYPE_ESS_ES18XX	0x00000015	/* ESS AudioDrive ES18XX */
#define SND_CARD_TYPE_CS4231		0x00000016      /* CS4231 */
#define SND_CARD_TYPE_OPTI92X		0x00000017	/* OPTi 92x chipset */
#define SND_CARD_TYPE_SERIAL		0x00000018	/* Serial MIDI driver */
#define SND_CARD_TYPE_AD1848		0x00000019	/* Generic AD1848 driver */
#define SND_CARD_TYPE_TRID4DWAVEDX	0x0000001a	/* Trident 4DWave DX */
#define SND_CARD_TYPE_TRID4DWAVENX	0x0000001b	/* Trident 4DWave NX */
#define SND_CARD_TYPE_SGALAXY           0x0000001c      /* Aztech Sound Galaxy */
#define SND_CARD_TYPE_CS461X		0x0000001d	/* Sound Fusion CS4610/12/15 */
#define SND_CARD_TYPE_WAVEFRONT         0x0000001e      /* TB WaveFront generic */
#define SND_CARD_TYPE_TROPEZ            0x0000001f      /* TB Tropez */
#define SND_CARD_TYPE_TROPEZPLUS        0x00000020      /* TB Tropez+ */
#define SND_CARD_TYPE_MAUI              0x00000021      /* TB Maui */
#define SND_CARD_TYPE_CMI8330           0x00000022      /* C-Media CMI8330 */
#define SND_CARD_TYPE_DUMMY		0x00000023	/* dummy soundcard */
#define SND_CARD_TYPE_ALS100		0x00000024	/* Avance Logic ALS100 */
#define SND_CARD_TYPE_SHARE		0x00000025	/* share soundcard */
#define SND_CARD_TYPE_SI_7018		0x00000026	/* SiS 7018 */
#define SND_CARD_TYPE_OPTI93X		0x00000027	/* OPTi 93x chipset */
#define SND_CARD_TYPE_MTPAV		0x00000028	/* MOTU MidiTimePiece AV multiport MIDI */
#define SND_CARD_TYPE_VIRMIDI		0x00000029	/* Virtual MIDI */
#define SND_CARD_TYPE_EMU10K1		0x0000002a	/* EMU10K1 */
#define SND_CARD_TYPE_HAMMERFALL	0x0000002b	/* RME Digi9652  */
#define SND_CARD_TYPE_HAMMERFALL_LIGHT	0x0000002c	/* RME Digi9652, but no expansion card */
#define SND_CARD_TYPE_ICE1712		0x0000002d	/* ICE1712 */
#define SND_CARD_TYPE_CMI8338		0x0000002e	/* C-Media CMI8338 */
#define SND_CARD_TYPE_CMI8738		0x0000002f	/* C-Media CMI8738 */
#define SND_CARD_TYPE_AD1816A		0x00000030	/* ADI SoundPort AD1816A */
#define SND_CARD_TYPE_INTEL8X0		0x00000031	/* Intel 810/820/830/840/MX440 */
#define SND_CARD_TYPE_ESS_ESOLDM1	0x00000032	/* Maestro 1 */
#define SND_CARD_TYPE_ESS_ES1968	0x00000033	/* Maestro 2 */
#define SND_CARD_TYPE_ESS_ES1978	0x00000034	/* Maestro 2E */
#define SND_CARD_TYPE_DIGI96		0x00000035	/* RME Digi96 */
#define SND_CARD_TYPE_VIA82C686A	0x00000036	/* VIA 82C686A */
#define SND_CARD_TYPE_FM801		0x00000037	/* FM801 */
#define SND_CARD_TYPE_AZT2320		0x00000038	/* AZT2320 */
#define SND_CARD_TYPE_PRODIF_PLUS	0x00000039	/* Marian/Sek'D Prodif Plus */
#define SND_CARD_TYPE_YMFPCI		0x0000003a	/* YMF724/740/744/754 */
#define SND_CARD_TYPE_CS4281		0x0000003b	/* CS4281 */

#define SND_CARD_TYPE_LAST		0x0000003b

typedef struct timeval snd_timestamp_t;

/****************************************************************************
 *                                                                          *
 *        Section for driver control interface - /dev/snd/control?          *
 *                                                                          *
 ****************************************************************************/

#define SND_CTL_VERSION			SND_PROTOCOL_VERSION(2, 0, 0)

typedef struct snd_ctl_hw_info {
	unsigned int type;	/* type of card - look to SND_CARD_TYPE_XXXX */
	unsigned int hwdepdevs;	/* count of hardware dependent devices */
	unsigned int pcmdevs;	/* count of PCM devices */
	unsigned int mididevs;	/* count of raw MIDI devices */
	unsigned int timerdevs;	/* count of timer devices */
	unsigned int resdevs[4]; /* reserved for possible future devices */
	char id[16];		/* ID of card (user selectable) */
	char abbreviation[16];	/* Abbreviation for soundcard */
	char name[32];		/* Short name of soundcard */
	char longname[80];	/* name + info text about soundcard */
	char mixerid[16];	/* ID of mixer */
	char mixername[80];	/* mixer identification */
	unsigned char reserved[128];	/* reserved for future */
} snd_ctl_hw_info_t;

typedef enum {
	SND_CONTROL_TYPE_NONE = 0,		/* invalid */
	SND_CONTROL_TYPE_BOOLEAN,		/* boolean type */
	SND_CONTROL_TYPE_INTEGER,		/* integer type */
	SND_CONTROL_TYPE_ENUMERATED,		/* enumerated type */
	SND_CONTROL_TYPE_BYTES			/* byte array */
} snd_control_type_t;

typedef enum {
	SND_CONTROL_IFACE_CARD = 0,		/* global control */
	SND_CONTROL_IFACE_HWDEP,		/* hardware dependent device */
	SND_CONTROL_IFACE_MIXER,		/* virtual mixer device */
	SND_CONTROL_IFACE_PCM,			/* PCM device */
	SND_CONTROL_IFACE_RAWMIDI,		/* RawMidi device */
	SND_CONTROL_IFACE_TIMER,		/* timer device */
	SND_CONTROL_IFACE_SEQUENCER		/* sequencer client */
} snd_control_iface_t;

#define SND_CONTROL_ACCESS_READ		(1<<0)
#define SND_CONTROL_ACCESS_WRITE	(1<<1)
#define SND_CONTROL_ACCESS_READWRITE	(SND_CONTROL_ACCESS_READ|SND_CONTROL_ACCESS_WRITE)
#define SND_CONTROL_ACCESS_VOLATILE	(1<<2)	/* control value may be changed without notification */
#define SND_CONTROL_ACCESS_INACTIVE	(1<<8)	/* control does actually nothing, but may be updated */
#define SND_CONTROL_ACCESS_LOCK		(1<<9)	/* write lock */
#define SND_CONTROL_ACCESS_INDIRECT	(1<<31)	/* indirect access */

typedef struct snd_control_id {
	unsigned int numid;		/* numeric identifier, zero = invalid */
	snd_control_iface_t iface;	/* interface identifier */
	unsigned int device;		/* device/client number */
	unsigned int subdevice;		/* subdevice (substream) number */
        unsigned char name[44];		/* ASCII name of item */
	unsigned int index;		/* index of item */
} snd_control_id_t;

typedef struct snd_control_list {
	unsigned int controls_offset;	/* W: first control ID to get */
	unsigned int controls_request;	/* W: count of control IDs to get */
	unsigned int controls_count;	/* R: count of available (set) IDs */
	unsigned int controls;		/* R: count of all available controls */
	snd_control_id_t *pids;		/* W: IDs */
        char reserved[50];
} snd_control_list_t;

typedef struct snd_control_info {
	snd_control_id_t id;		/* W: control ID */
	snd_control_type_t type;	/* R: value type - SND_CONTROL_TYPE_* */
	unsigned int access;		/* R: value access (bitmask) - SND_CONTROL_ACCESS_* */
	unsigned int values_count;	/* count of values */
	union {
		struct {
			long min;		/* R: minimum value */
			long max;		/* R: maximum value */
			long step;		/* R: step (0 variable) */
		} integer;
		struct {
			unsigned int items;	/* R: number of items */
			unsigned int item;	/* W: item number */
			char name[64];		/* R: value name */
		} enumerated;
		unsigned char reserved[128];
	} value;
	unsigned char reserved[64];
} snd_control_info_t;

typedef struct snd_control {
	snd_control_id_t id;			/* W: control ID */
	int indirect: 1;			/* W: use indirect pointer (xxx_ptr member) */
        union {
		union {
			long value[128];
			long *value_ptr;
		} integer;
		union {
			unsigned int item[128];
			unsigned int *item_ptr;
		} enumerated;
		union {
			unsigned char data[512];
			unsigned char *data_ptr;
		} bytes;
        } value;                /* RO */
        unsigned char reserved[128];
} snd_control_t;

#define SND_CTL_IOCTL_PVERSION		_IOR ('U', 0x00, int)
#define SND_CTL_IOCTL_HW_INFO		_IOR ('U', 0x01, snd_ctl_hw_info_t)
#define SND_CTL_IOCTL_CONTROL_LIST	_IOWR('U', 0x10, snd_control_list_t)
#define SND_CTL_IOCTL_CONTROL_INFO	_IOWR('U', 0x11, snd_control_info_t)
#define SND_CTL_IOCTL_CONTROL_READ	_IOWR('U', 0x12, snd_control_t)
#define SND_CTL_IOCTL_CONTROL_WRITE	_IOWR('U', 0x13, snd_control_t)
#define SND_CTL_IOCTL_CONTROL_LOCK	_IOW ('U', 0x14, snd_control_id_t)
#define SND_CTL_IOCTL_CONTROL_UNLOCK	_IOW ('U', 0x15, snd_control_id_t)
#define SND_CTL_IOCTL_HWDEP_INFO	_IOR ('U', 0x20, snd_hwdep_info_t)
#define SND_CTL_IOCTL_PCM_INFO		_IOR ('U', 0x30, snd_pcm_info_t)
#define SND_CTL_IOCTL_PCM_PREFER_SUBDEVICE _IOW('U', 0x31, int)
#define SND_CTL_IOCTL_RAWMIDI_INFO	_IOR ('U', 0x40, snd_rawmidi_info_t)
#define SND_CTL_IOCTL_RAWMIDI_PREFER_SUBDEVICE _IOW('U', 0x41, int)

/*
 *  Read interface.
 */

typedef enum {
	SND_CTL_EVENT_REBUILD = 0,	/* rebuild everything */
	SND_CTL_EVENT_VALUE,		/* a control value was changed */
	SND_CTL_EVENT_CHANGE,		/* a control was changed */
	SND_CTL_EVENT_ADD,		/* a control was added */
	SND_CTL_EVENT_REMOVE,		/* a control was removed */
} snd_ctl_event_type_t;

typedef struct snd_ctl_event {
	snd_ctl_event_type_t type;	/* event type - SND_CTL_EVENT_* */
	union {
		snd_control_id_t id;
                unsigned char data8[60];
        } data;
} snd_ctl_event_t;

/*
 *  Mixer interface compatible with Open Sound System API
 */

#ifdef __SND_OSS_COMPAT__

#define SND_MIXER_OSS_CAP_EXCL_INPUT	0x00000001	/* only one capture source at moment */

#define SND_MIXER_OSS_DEVS	25
#define SND_MIXER_OSS_VOLUME	0
#define SND_MIXER_OSS_BASS	1
#define SND_MIXER_OSS_TREBLE	2
#define SND_MIXER_OSS_SYNTH	3
#define SND_MIXER_OSS_PCM	4
#define SND_MIXER_OSS_SPEAKER	5
#define SND_MIXER_OSS_LINE	6
#define SND_MIXER_OSS_MIC	7
#define SND_MIXER_OSS_CD	8
#define SND_MIXER_OSS_IMIX	9	/* recording monitor */
#define SND_MIXER_OSS_ALTPCM	10
#define SND_MIXER_OSS_RECLEV	11	/* recording level */
#define SND_MIXER_OSS_IGAIN	12	/* input gain */
#define SND_MIXER_OSS_OGAIN	13	/* output gain */
#define SND_MIXER_OSS_LINE1	14
#define SND_MIXER_OSS_LINE2	15
#define SND_MIXER_OSS_LINE3	16
#define SND_MIXER_OSS_DIGITAL1	17
#define SND_MIXER_OSS_DIGITAL2	18
#define SND_MIXER_OSS_DIGITAL3	19
#define SND_MIXER_OSS_PHONEIN	20
#define SND_MIXER_OSS_PHONEOUT	21
#define SND_MIXER_OSS_VIDEO	22
#define SND_MIXER_OSS_RADIO	23
#define SND_MIXER_OSS_MONITOR	24
#define SND_MIXER_OSS_UNKNOWN	(32+1)

struct snd_oss_mixer_info {
	char id[16];
	char name[32];
	int modify_counter;
	int fillers[10];
};

struct snd_oss_mixer_info_obsolete {
	char id[16];
	char name[32];
};

#define SND_MIXER_OSS_SET_RECSRC _IOWR('M', 255, int)
#define SND_MIXER_OSS_RECSRC	_IOR ('M', 255, int)
#define SND_MIXER_OSS_DEVMASK	_IOR ('M', 254, int)
#define SND_MIXER_OSS_RECMASK	_IOR ('M', 253, int)
#define SND_MIXER_OSS_CAPS	_IOR ('M', 252, int)
#define SND_MIXER_OSS_STEREODEVS _IOR ('M', 251, int)
#define SND_MIXER_OSS_INFO      _IOR ('M', 101, struct snd_oss_mixer_info)
#define SND_MIXER_OSS_OLD_INFO	_IOR ('M', 101, struct snd_oss_mixer_info_obsolete)
#define SND_OSS_GETVERSION	_IOR ('M', 118, int)

#endif				/* __SND_OSS_COMPAT__ */

/****************************************************************************
 *                                                                          *
 *      Section for driver hardware dependent interface - /dev/snd/hw?      *
 *                                                                          *
 ****************************************************************************/

#define SND_HWDEP_VERSION		SND_PROTOCOL_VERSION(1, 0, 0)

#define SND_HWDEP_TYPE_OPL2		0
#define SND_HWDEP_TYPE_OPL3		1
#define SND_HWDEP_TYPE_OPL4		2
#define SND_HWDEP_TYPE_SB16CSP		3	/* Creative Signal Processor */
#define SND_HWDEP_TYPE_EMU10K1		4	/* FX8010 processor in EMU10K1 chip */
#define SND_HWDEP_TYPE_YSS225		5	/* Yamaha FX processor */
#define SND_HWDEP_TYPE_ICS2115		6	/* Wavetable synth */
/* --- */
#define SND_HWDEP_TYPE_LAST		6

typedef struct snd_hwdep_info {
	int device;		/* WR: device number */
	unsigned int type;	/* type of card - look to SND_CARD_TYPE_XXXX */
	unsigned char id[64];	/* ID of this hardware dependent device (user selectable) */
	unsigned char name[80];	/* name of this hardware dependent device */
	unsigned int hw_type;	/* hardware depedent device type */
	unsigned char reserved[64];	/* reserved for future */
} snd_hwdep_info_t;

#define SND_HWDEP_IOCTL_PVERSION	_IOR ('H', 0x00, int)
#define SND_HWDEP_IOCTL_INFO		_IOR ('H', 0x01, snd_hwdep_info_t)

/*****************************************************************************
 *                                                                           *
 *             Digital Audio (PCM) interface - /dev/snd/pcm??                *
 *                                                                           *
 *****************************************************************************/

#define SND_PCM_VERSION			SND_PROTOCOL_VERSION(2, 0, 0)

#define SND_PCM_CLASS_GENERIC		0x0000	/* standard mono or stereo device */
#define SND_PCM_SCLASS_GENERIC_MIX	0x0001	/* mono or stereo subdevices are mixed together */

#define SND_PCM_CLASS_MULTI		0x0001	/* multichannel device */
#define SND_PCM_SCLASS_MULTI_MIX	0x0001	/* multichannel subdevices are mixed together */

#define SND_PCM_CLASS_MODEM		0x0010	/* software modem class */
#define SND_PCM_CLASS_DIGITIZER		0x0011	/* digitizer class */

#define SND_PCM_STREAM_PLAYBACK		0
#define SND_PCM_STREAM_CAPTURE		1

#define SND_PCM_SFMT_S8			0
#define SND_PCM_SFMT_U8			1
#define SND_PCM_SFMT_S16_LE		2
#define SND_PCM_SFMT_S16_BE		3
#define SND_PCM_SFMT_U16_LE		4
#define SND_PCM_SFMT_U16_BE		5
#define SND_PCM_SFMT_S24_LE		6	/* low three bytes */
#define SND_PCM_SFMT_S24_BE		7	/* low three bytes */
#define SND_PCM_SFMT_U24_LE		8	/* low three bytes */
#define SND_PCM_SFMT_U24_BE		9	/* low three bytes */
#define SND_PCM_SFMT_S32_LE		10
#define SND_PCM_SFMT_S32_BE		11
#define SND_PCM_SFMT_U32_LE		12
#define SND_PCM_SFMT_U32_BE		13
#define SND_PCM_SFMT_FLOAT_LE		14	/* 4-byte float, IEEE-754 32-bit */
#define SND_PCM_SFMT_FLOAT_BE		15	/* 4-byte float, IEEE-754 32-bit */
#define SND_PCM_SFMT_FLOAT64_LE		16	/* 8-byte float, IEEE-754 64-bit */
#define SND_PCM_SFMT_FLOAT64_BE		17	/* 8-byte float, IEEE-754 64-bit */
#define SND_PCM_SFMT_IEC958_SUBFRAME_LE	18	/* IEC-958 subframe, Little Endian */
#define SND_PCM_SFMT_IEC958_SUBFRAME_BE	19	/* IEC-958 subframe, Big Endian */
#define SND_PCM_SFMT_MU_LAW		20
#define SND_PCM_SFMT_A_LAW		21
#define SND_PCM_SFMT_IMA_ADPCM		22
#define SND_PCM_SFMT_MPEG		23
#define SND_PCM_SFMT_GSM		24
#define SND_PCM_SFMT_SPECIAL		31

#ifdef SND_LITTLE_ENDIAN
#define SND_PCM_SFMT_S16		SND_PCM_SFMT_S16_LE
#define SND_PCM_SFMT_U16		SND_PCM_SFMT_U16_LE
#define SND_PCM_SFMT_S24		SND_PCM_SFMT_S24_LE
#define SND_PCM_SFMT_U24		SND_PCM_SFMT_U24_LE
#define SND_PCM_SFMT_S32		SND_PCM_SFMT_S32_LE
#define SND_PCM_SFMT_U32		SND_PCM_SFMT_U32_LE
#define SND_PCM_SFMT_FLOAT		SND_PCM_SFMT_FLOAT_LE
#define SND_PCM_SFMT_FLOAT64		SND_PCM_SFMT_FLOAT64_LE
#define SND_PCM_SFMT_IEC958_SUBFRAME	SND_PCM_SFMT_IEC958_SUBFRAME_LE
#endif
#ifdef SND_BIG_ENDIAN
#define SND_PCM_SFMT_S16		SND_PCM_SFMT_S16_BE
#define SND_PCM_SFMT_U16		SND_PCM_SFMT_U16_BE
#define SND_PCM_SFMT_S24		SND_PCM_SFMT_S24_BE
#define SND_PCM_SFMT_U24		SND_PCM_SFMT_U24_BE
#define SND_PCM_SFMT_S32		SND_PCM_SFMT_S32_BE
#define SND_PCM_SFMT_U32		SND_PCM_SFMT_U32_BE
#define SND_PCM_SFMT_FLOAT		SND_PCM_SFMT_FLOAT_BE
#define SND_PCM_SFMT_FLOAT64		SND_PCM_SFMT_FLOAT64_BE
#define SND_PCM_SFMT_IEC958_SUBFRAME	SND_PCM_SFMT_IEC958_SUBFRAME_BE
#endif

#define SND_PCM_FMT_S8			(1 << SND_PCM_SFMT_S8)
#define SND_PCM_FMT_U8			(1 << SND_PCM_SFMT_U8)
#define SND_PCM_FMT_S16_LE		(1 << SND_PCM_SFMT_S16_LE)
#define SND_PCM_FMT_S16_BE		(1 << SND_PCM_SFMT_S16_BE)
#define SND_PCM_FMT_U16_LE		(1 << SND_PCM_SFMT_U16_LE)
#define SND_PCM_FMT_U16_BE		(1 << SND_PCM_SFMT_U16_BE)
#define SND_PCM_FMT_S24_LE		(1 << SND_PCM_SFMT_S24_LE)
#define SND_PCM_FMT_S24_BE		(1 << SND_PCM_SFMT_S24_BE)
#define SND_PCM_FMT_U24_LE		(1 << SND_PCM_SFMT_U24_LE)
#define SND_PCM_FMT_U24_BE		(1 << SND_PCM_SFMT_U24_BE)
#define SND_PCM_FMT_S32_LE		(1 << SND_PCM_SFMT_S32_LE)
#define SND_PCM_FMT_S32_BE		(1 << SND_PCM_SFMT_S32_BE)
#define SND_PCM_FMT_U32_LE		(1 << SND_PCM_SFMT_U32_LE)
#define SND_PCM_FMT_U32_BE		(1 << SND_PCM_SFMT_U32_BE)
#define SND_PCM_FMT_FLOAT_LE		(1 << SND_PCM_SFMT_FLOAT_LE)
#define SND_PCM_FMT_FLOAT_BE		(1 << SND_PCM_SFMT_FLOAT_BE)
#define SND_PCM_FMT_FLOAT64_LE		(1 << SND_PCM_SFMT_FLOAT64_LE)
#define SND_PCM_FMT_FLOAT64_BE		(1 << SND_PCM_SFMT_FLOAT64_BE)
#define SND_PCM_FMT_IEC958_SUBFRAME_LE	(1 << SND_PCM_SFMT_IEC958_SUBFRAME_LE)
#define SND_PCM_FMT_IEC958_SUBFRAME_BE	(1 << SND_PCM_SFMT_IEC958_SUBFRAME_BE)
#define SND_PCM_FMT_MU_LAW		(1 << SND_PCM_SFMT_MU_LAW)
#define SND_PCM_FMT_A_LAW		(1 << SND_PCM_SFMT_A_LAW)
#define SND_PCM_FMT_IMA_ADPCM		(1 << SND_PCM_SFMT_IMA_ADPCM)
#define SND_PCM_FMT_MPEG		(1 << SND_PCM_SFMT_MPEG)
#define SND_PCM_FMT_GSM			(1 << SND_PCM_SFMT_GSM)
#define SND_PCM_FMT_SPECIAL		(1 << SND_PCM_SFMT_SPECIAL)

#ifdef SND_LITTLE_ENDIAN
#define SND_PCM_FMT_S16			SND_PCM_FMT_S16_LE
#define SND_PCM_FMT_U16			SND_PCM_FMT_U16_LE
#define SND_PCM_FMT_S24			SND_PCM_FMT_S24_LE
#define SND_PCM_FMT_U24			SND_PCM_FMT_U24_LE
#define SND_PCM_FMT_S32			SND_PCM_FMT_S32_LE
#define SND_PCM_FMT_U32			SND_PCM_FMT_U32_LE
#define SND_PCM_FMT_FLOAT		SND_PCM_FMT_FLOAT_LE
#define SND_PCM_FMT_FLOAT64		SND_PCM_FMT_FLOAT64_LE
#define SND_PCM_FMT_IEC958_SUBFRAME	SND_PCM_FMT_IEC958_SUBFRAME_LE
#endif
#ifdef SND_BIG_ENDIAN
#define SND_PCM_FMT_S16			SND_PCM_FMT_S16_BE
#define SND_PCM_FMT_U16			SND_PCM_FMT_U16_BE
#define SND_PCM_FMT_S24			SND_PCM_FMT_S24_BE
#define SND_PCM_FMT_U24			SND_PCM_FMT_U24_BE
#define SND_PCM_FMT_S32			SND_PCM_FMT_S32_BE
#define SND_PCM_FMT_U32			SND_PCM_FMT_U32_BE
#define SND_PCM_FMT_FLOAT		SND_PCM_FMT_FLOAT_BE
#define SND_PCM_FMT_FLOAT64		SND_PCM_FMT_FLOAT64_BE
#define SND_PCM_FMT_IEC958_SUBFRAME	SND_PCM_FMT_IEC958_SUBFRAME_BE
#endif

#define SND_PCM_RATE_CONTINUOUS		(1<<0)		/* continuous range */
#define SND_PCM_RATE_KNOT		(1<<1)		/* supports more non-continuos rates */
#define SND_PCM_RATE_8000		(1<<2)		/* 8000Hz */
#define SND_PCM_RATE_11025		(1<<3)		/* 11025Hz */
#define SND_PCM_RATE_16000		(1<<4)		/* 16000Hz */
#define SND_PCM_RATE_22050		(1<<5)		/* 22050Hz */
#define SND_PCM_RATE_32000		(1<<6)		/* 32000Hz */
#define SND_PCM_RATE_44100		(1<<7)		/* 44100Hz */
#define SND_PCM_RATE_48000		(1<<8)		/* 48000Hz */
#define SND_PCM_RATE_88200		(1<<9)		/* 88200Hz */
#define SND_PCM_RATE_96000		(1<<10)		/* 96000Hz */
#define SND_PCM_RATE_176400		(1<<11)		/* 176400Hz */
#define SND_PCM_RATE_192000		(1<<12)		/* 192000Hz */

#define SND_PCM_RATE_8000_44100		(SND_PCM_RATE_8000|SND_PCM_RATE_11025|\
					 SND_PCM_RATE_16000|SND_PCM_RATE_22050|\
					 SND_PCM_RATE_32000|SND_PCM_RATE_44100)
#define SND_PCM_RATE_8000_48000		(SND_PCM_RATE_8000_44100|SND_PCM_RATE_48000)
#define SND_PCM_RATE_8000_96000		(SND_PCM_RATE_8000_48000|SND_PCM_RATE_88200|\
					 SND_PCM_RATE_96000)
#define SND_PCM_RATE_8000_192000	(SND_PCM_RATE_8000_96000|SND_PCM_RATE_176400|\
					 SND_PCM_RATE_192000)

#define SND_PCM_INFO_MMAP		0x00000001	/* hardware supports mmap */
#define SND_PCM_INFO_BATCH		0x00000010	/* double buffering */
#define SND_PCM_INFO_INTERLEAVED	0x00000100	/* channels are interleaved */
#define SND_PCM_INFO_NONINTERLEAVED	0x00000200	/* channels are not interleaved */
#define SND_PCM_INFO_COMPLEX		0x00000400	/* complex frame organization (mmap only) */
#define SND_PCM_INFO_BLOCK_TRANSFER	0x00010000	/* hardware transfer block of samples */
#define SND_PCM_INFO_OVERRANGE		0x00020000	/* hardware supports ADC (capture) overrange detection */
#define SND_PCM_INFO_MMAP_VALID		0x00040000	/* fragment data are valid during transfer */
#define SND_PCM_INFO_PAUSE		0x00080000	/* pause ioctl is supported */
#define SND_PCM_INFO_HALF_DUPLEX	0x00100000	/* only half duplex */
#define SND_PCM_INFO_JOINT_DUPLEX	0x00200000	/* playback and capture stream are somewhat correlated */
#define SND_PCM_INFO_SYNC_START		0x00400000	/* pcm support some kind of sync go */

#define SND_PCM_XFER_UNSPECIFIED	0	/* don't care access type */
#define SND_PCM_XFER_INTERLEAVED	1	/* read/write access type */
#define SND_PCM_XFER_NONINTERLEAVED	2	/* readv/writev access type */
#define SND_PCM_XFER_LAST		2

#define SND_PCM_START_DATA		0	/* start when some data are written (playback) or requested (capture) */
#define SND_PCM_START_EXPLICIT		1	/* start on the go command */
#define SND_PCM_START_LAST		1

#define SND_PCM_XRUN_FRAGMENT		0	/* Efficient xrun detection */
#define SND_PCM_XRUN_ASAP		1	/* Accurate xrun detection */
#define SND_PCM_XRUN_NONE		2	/* No xrun detection */
#define SND_PCM_XRUN_LAST		2

#define SND_PCM_READY_FRAGMENT		0	/* Efficient ready detection */
#define SND_PCM_READY_ASAP		1	/* Accurate ready detection */
#define SND_PCM_READY_LAST		1

#define SND_PCM_MMAP_UNSPECIFIED	0	/* don't care buffer type */
#define SND_PCM_MMAP_INTERLEAVED	1	/* simple interleaved buffer */
#define SND_PCM_MMAP_NONINTERLEAVED	2	/* simple noninterleaved buffer */
#define SND_PCM_MMAP_COMPLEX		3	/* complex buffer */
#define SND_PCM_MMAP_LAST		3

#define SND_PCM_PARAMS_SFMT		(1<<0)
#define SND_PCM_PARAMS_RATE		(1<<1)
#define SND_PCM_PARAMS_CHANNELS		(1<<2)
#define SND_PCM_PARAMS_START_MODE	(1<<3)
#define SND_PCM_PARAMS_READY_MODE	(1<<4)
#define SND_PCM_PARAMS_XFER_MODE	(1<<5)
#define SND_PCM_PARAMS_XRUN_MODE	(1<<6)
#define SND_PCM_PARAMS_BUFFER_SIZE	(1<<7)
#define SND_PCM_PARAMS_FRAGMENT_SIZE	(1<<8)
#define SND_PCM_PARAMS_MMAP_SHAPE	(1<<9)
#define SND_PCM_PARAMS_WHEN		(1<<10)

#define SND_PCM_PARAMS_FAIL_NONE		0
#define SND_PCM_PARAMS_FAIL_INVAL		1
#define SND_PCM_PARAMS_FAIL_INT_INCOMPAT	2
#define SND_PCM_PARAMS_FAIL_EXT_INCOMPAT	3

#define SND_PCM_WHEN_IDLE		0 /* Apply only if PCM is idle */
#define SND_PCM_WHEN_LAST		0

#define SND_PCM_STATE_OPEN		0	/* stream is open */
#define SND_PCM_STATE_SETUP		1	/* stream has a setup */
#define SND_PCM_STATE_PREPARED		2	/* stream is ready to start */
#define SND_PCM_STATE_RUNNING		3	/* stream is running */
#define SND_PCM_STATE_XRUN		4	/* stream reached an xrun */
#define SND_PCM_STATE_DRAINING		5	/* stream is draining */
#define SND_PCM_STATE_PAUSED		6	/* stream is paused */

#define SND_PCM_MMAP_OFFSET_DATA	0x00000000
#define SND_PCM_MMAP_OFFSET_STATUS	0x80000000
#define SND_PCM_MMAP_OFFSET_CONTROL	0x81000000

/* digital setup types */
#define SND_PCM_DIG_AES_IEC958		0

/* AES/IEC958 channel status bits */
#define SND_PCM_AES0_PROFESSIONAL	(1<<0)	/* 0 = consumer, 1 = professional */
#define SND_PCM_AES0_NONAUDIO		(1<<1)	/* 0 = audio, 1 = non-audio */
#define SND_PCM_AES0_PRO_EMPHASIS	(7<<2)	/* mask - emphasis */
#define SND_PCM_AES0_PRO_EMPHASIS_NOTID	(0<<2)	/* emphasis not indicated */
#define SND_PCM_AES0_PRO_EMPHASIS_NONE	(1<<2)	/* none emphasis */
#define SND_PCM_AES0_PRO_EMPHASIS_5015	(3<<2)	/* 50/15us emphasis */
#define SND_PCM_AES0_PRO_EMPHASIS_CCITT	(7<<2)	/* CCITT J.17 emphasis */
#define SND_PCM_AES0_PRO_FREQ_UNLOCKED	(1<<5)	/* source sample frequency: 0 = locked, 1 = unlocked */
#define SND_PCM_AES0_PRO_FS		(3<<6)	/* mask - sample frequency */
#define SND_PCM_AES0_PRO_FS_NOTID	(0<<6)	/* fs not indicated */
#define SND_PCM_AES0_PRO_FS_44100	(1<<6)	/* 44.1kHz */
#define SND_PCM_AES0_PRO_FS_48000	(2<<6)	/* 48kHz */
#define SND_PCM_AES0_PRO_FS_32000	(3<<6)	/* 32kHz */
#define SND_PCM_AES0_CON_NOT_COPYRIGHT	(1<<2)	/* 0 = copyright, 1 = not copyright */
#define SND_PCM_AES0_CON_EMPHASIS	(7<<3)	/* mask - emphasis */
#define SND_PCM_AES0_CON_EMPHASIS_NONE	(0<<3)	/* none emphasis */
#define SND_PCM_AES0_CON_EMPHASIS_5015	(1<<3)	/* 50/15us emphasis */
#define SND_PCM_AES0_CON_MODE		(3<<6)	/* mask - mode */
#define SND_PCM_AES1_PRO_MODE		(15<<0)	/* mask - channel mode */
#define SND_PCM_AES1_PRO_MODE_NOTID	(0<<0)	/* not indicated */
#define SND_PCM_AES1_PRO_MODE_STEREOPHONIC (2<<0) /* stereophonic - ch A is left */
#define SND_PCM_AES1_PRO_MODE_SINGLE	(4<<0)	/* single channel */
#define SND_PCM_AES1_PRO_MODE_TWO	(8<<0)	/* two channels */
#define SND_PCM_AES1_PRO_MODE_PRIMARY	(12<<0)	/* primary/secondary */
#define SND_PCM_AES1_PRO_MODE_BYTE3	(15<<0)	/* vector to byte 3 */
#define SND_PCM_AES1_PRO_USERBITS	(15<<4)	/* mask - user bits */
#define SND_PCM_AES1_PRO_USERBITS_NOTID	(0<<4)	/* not indicated */
#define SND_PCM_AES1_PRO_USERBITS_192	(8<<4)	/* 192-bit structure */
#define SND_PCM_AES1_PRO_USERBITS_UDEF	(12<<4)	/* user defined application */
#define SND_PCM_AES1_CON_CATEGORY	0x7f
#define SND_PCM_AES1_CON_GENERAL	0x00
#define SND_PCM_AES1_CON_EXPERIMENTAL	0x40
#define SND_PCM_AES1_CON_SOLIDMEM_MASK	0x0f
#define SND_PCM_AES1_CON_SOLIDMEM_ID	0x08
#define SND_PCM_AES1_CON_BROADCAST1_MASK 0x07
#define SND_PCM_AES1_CON_BROADCAST1_ID	0x04
#define SND_PCM_AES1_CON_DIGDIGCONV_MASK 0x07
#define SND_PCM_AES1_CON_DIGDIGCONV_ID	0x02
#define SND_PCM_AES1_CON_ADC_COPYRIGHT_MASK 0x1f
#define SND_PCM_AES1_CON_ADC_COPYRIGHT_ID 0x06
#define SND_PCM_AES1_CON_ADC_MASK	0x1f
#define SND_PCM_AES1_CON_ADC_ID		0x16
#define SND_PCM_AES1_CON_BROADCAST2_MASK 0x0f
#define SND_PCM_AES1_CON_BROADCAST2_ID	0x0e
#define SND_PCM_AES1_CON_LASEROPT_MASK	0x07
#define SND_PCM_AES1_CON_LASEROPT_ID	0x01
#define SND_PCM_AES1_CON_MUSICAL_MASK	0x07
#define SND_PCM_AES1_CON_MUSICAL_ID	0x05
#define SND_PCM_AES1_CON_MAGNETIC_MASK	0x07
#define SND_PCM_AES1_CON_MAGNETIC_ID	0x03
#define SND_PCM_AES1_CON_IEC908_CD	(SND_PCM_AES1_CON_LASEROPT_ID|0x00)
#define SND_PCM_AES1_CON_NON_IEC908_CD	(SND_PCM_AES1_CON_LASEROPT_ID|0x08)
#define SND_PCM_AES1_CON_PCM_CODER	(SND_PCM_AES1_CON_DIGDIGCONV_ID|0x00)
#define SND_PCM_AES1_CON_SAMPLER	(SND_PCM_AES1_CON_DIGDIGCONV_ID|0x20)
#define SND_PCM_AES1_CON_MIXER		(SND_PCM_AES1_CON_DIGDIGCONV_ID|0x10)
#define SND_PCM_AES1_CON_RATE_CONVERTER	(SND_PCM_AES1_CON_DIGDIGCONV_ID|0x18)
#define SND_PCM_AES1_CON_SYNTHESIZER	(SND_PCM_AES1_CON_MUSICAL_ID|0x00)
#define SND_PCM_AES1_CON_MICROPHONE	(SND_PCM_AES1_CON_MUSICAL_ID|0x08)
#define SND_PCM_AES1_CON_DAT		(SND_PCM_AES1_CON_MAGNETIC_ID|0x00)
#define SND_PCM_AES1_CON_VCR		(SND_PCM_AES1_CON_MAGNETIC_ID|0x08)
#define SND_PCM_AES1_CON_ORIGINAL	(1<<7)	/* this bits depends on the category code */
#define SND_PCM_AES2_PRO_SBITS		(7<<0)	/* mask - sample bits */
#define SND_PCM_AES2_PRO_SBITS_20	(2<<0)	/* 20-bit - coordination */
#define SND_PCM_AES2_PRO_SBITS_24	(4<<0)	/* 24-bit - main audio */
#define SND_PCM_AES2_PRO_SBITS_UDEF	(6<<0)	/* user defined application */
#define SND_PCM_AES2_PRO_WORDLEN	(7<<3)	/* mask - source word length */
#define SND_PCM_AES2_PRO_WORDLEN_NOTID	(0<<3)	/* not indicated */
#define SND_PCM_AES2_PRO_WORDLEN_22_18	(2<<3)	/* 22-bit or 18-bit */
#define SND_PCM_AES2_PRO_WORDLEN_23_19	(4<<3)	/* 23-bit or 19-bit */
#define SND_PCM_AES2_PRO_WORDLEN_24_20	(5<<3)	/* 24-bit or 20-bit */
#define SND_PCM_AES2_PRO_WORDLEN_20_16	(6<<3)	/* 20-bit or 16-bit */
#define SND_PCM_AES2_CON_SOURCE		(15<<0)	/* mask - source number */
#define SND_PCM_AES2_CON_SOURCE_UNSPEC	(0<<0)	/* unspecified */
#define SND_PCM_AES2_CON_CHANNEL	(15<<4)	/* mask - channel number */
#define SND_PCM_AES2_CON_CHANNEL_UNSPEC	(0<<4)	/* unspecified */
#define SND_PCM_AES3_CON_FS		(15<<0)	/* mask - sample frequency */
#define SND_PCM_AES3_CON_FS_44100	(0<<0)	/* 44.1kHz */
#define SND_PCM_AES3_CON_FS_48000	(2<<0)	/* 48kHz */
#define SND_PCM_AES3_CON_FS_32000	(3<<0)	/* 32kHz */
#define SND_PCM_AES3_CON_CLOCK		(3<<4)	/* mask - clock accuracy */
#define SND_PCM_AES3_CON_CLOCK_1000PPM	(0<<4)	/* 1000 ppm */
#define SND_PCM_AES3_CON_CLOCK_50PPM	(1<<4)	/* 50 ppm */
#define SND_PCM_AES3_CON_CLOCK_VARIABLE	(2<<4)	/* variable pitch */

typedef union snd_pcm_sync_id {
	char id[16];
	short id16[8];
	int id32[4];
} snd_pcm_sync_id_t;

typedef struct snd_pcm_digital {
	int type;			/* digital API type */
	unsigned int valid: 1;		/* set if the digital setup is valid */
	union {
		struct {
			unsigned char status[24];	/* AES/IEC958 channel status bits */
			unsigned char subcode[147];	/* AES/IEC958 subcode bits */
			unsigned char pad;		/* nothing */
			unsigned char dig_subframe[4];	/* AES/IEC958 subframe bits */
		} aes;
		char reserved[256];
	} dig;
	char reserved[16];		/* must be filled with zero */
} snd_pcm_digital_t;

typedef struct snd_pcm_info {
	int device;			/* device number */
	int stream;			/* stream number */
	int subdevice;			/* subdevice number */
        unsigned int type;              /* soundcard type */
	unsigned char id[64];		/* ID of this PCM device (user selectable) */
	unsigned char name[80];		/* name of this device */
	unsigned char subname[32];	/* subdevice name */
	unsigned short device_class;	/* SND_PCM_CLASS_* */
	unsigned short device_subclass;	/* SND_PCM_SCLASS_* */
	unsigned int subdevices_count;
	unsigned int subdevices_avail;
	snd_pcm_sync_id_t sync;		/* hardware synchronization ID */
	char reserved[64];		/* reserved for future... */
} snd_pcm_info_t;

typedef struct snd_pcm_channel_info {
	unsigned int channel;		/* channel number */
	char channel_name[32];		/* name of channel */
	int dig_group;			/* digital group */
	snd_pcm_digital_t dig_mask;	/* AES/EBU/IEC958 supported bits, zero = no AES/EBU/IEC958 */
	char reserved[64];		/* reserved for future... */
} snd_pcm_channel_info_t;

typedef struct snd_pcm_format {
	int sfmt;			/* SND_PCM_SFMT_XXXX */
	unsigned int rate;		/* rate in Hz */
	unsigned int channels;		/* channels */
	int special;			/* special (custom) description of format */
	char reserved[16];		/* must be filled with zero */
} snd_pcm_format_t;

typedef struct snd_pcm_params {
	snd_pcm_format_t format;	/* format */
	snd_pcm_digital_t digital;	/* digital setup */
	int start_mode;			/* start mode */
	int ready_mode;			/* ready detection mode */
	size_t avail_min;		/* min available frames for wakeup */
	int xfer_mode;			/* xfer mode */
	size_t xfer_min;		/* xfer min size */
	size_t xfer_align;		/* xfer size need to be a multiple */
	int xrun_mode;			/* xrun detection mode */
	int mmap_shape;			/* mmap buffer shape */
	size_t buffer_size;		/* requested buffer size in frames */
	size_t frag_size;		/* requested fragment size in frames */
	size_t boundary;		/* pointers wrap point */
	unsigned int time: 1;		/* timestamp switch */
	int when;			/* Params apply time/condition */
	snd_timestamp_t tstamp;		/* Timestamp */
	unsigned int fail_mask;		/* failure locations */
	int fail_reason;		/* failure reason */
	char reserved[64];		/* must be filled with zero */
} snd_pcm_params_t;

typedef struct {
	unsigned int req_mask;		/* Requests mask */
	snd_pcm_params_t req;		/* Requested params (only some fields 
					   are currently relevant) */
	unsigned int flags;		/* see to SND_PCM_INFO_XXXX */
	snd_pcm_digital_t dig_mask;	/* AES/EBU/IEC958 supported bits, zero = no AES/EBU/IEC958 */
	unsigned int formats;		/* supported formats */
	unsigned int rates;		/* hardware rates */
	unsigned int min_rate;		/* min rate (in Hz) */
	unsigned int max_rate;		/* max rate (in Hz) */
	unsigned int min_channels;	/* min channels */
	unsigned int max_channels;	/* max channels */
	size_t buffer_size;		/* max buffer size */
	size_t min_fragment_size;	/* min fragment size */
	size_t max_fragment_size;	/* max fragment size */
	size_t min_fragments;		/* min # of fragments */
	size_t max_fragments;		/* max # of fragments */
	size_t fragment_align;		/* align fragment value */
	/* NB: If a param is requested, the relating min and max fields are
	   loaded with the nearest valid value <= and >= the requested one.
	   NB: size fields are filled only if frame size is known
	*/
  
	char reserved[64];
} snd_pcm_params_info_t;

typedef struct snd_pcm_channel_params {
	unsigned int channel;
	snd_pcm_digital_t digital;	/* digital setup */
	int when;			/* Params apply time/condition */
	snd_timestamp_t tstamp;		/* Timestamp */
	unsigned int fail_mask;		/* failure locations */
	int fail_reason;		/* failure reason */
	char reserved[64];
} snd_pcm_channel_params_t;

typedef struct snd_pcm_setup {
	snd_pcm_format_t format;	/* real used format */
	snd_pcm_digital_t digital;	/* digital setup */
	int start_mode;			/* start mode */
	int ready_mode;			/* ready detection mode */
	size_t avail_min;		/* min available frames for wakeup */
	int xfer_mode;			/* xfer mode */
	size_t xfer_min;		/* xfer min size */
	size_t xfer_align;		/* xfer size need to be a multiple */
	int xrun_mode;			/* xrun detection mode */
	int mmap_shape;			/* mmap buffer shape */
	size_t buffer_size;		/* current buffer size in frames */
	size_t frag_size;		/* current fragment size in frames */
	size_t boundary;		/* position in frames wrap point */
	unsigned int time: 1;		/* timestamp switch */
	size_t frags;			/* allocated fragments */
	size_t mmap_bytes;		/* mmap data size in bytes*/
	unsigned int msbits;		/* used most significant bits */
	unsigned int rate_master;	/* Exact rate is rate_master / */
	unsigned int rate_divisor;	/* rate_divisor */
	size_t fifo_size;		/* chip FIFO size in frames */
	char reserved[64];		/* must be filled with zero */
} snd_pcm_setup_t;

typedef struct snd_pcm_channel_area {
	void *addr;			/* base address of channel samples */
	unsigned int first;		/* offset to first sample in bits */
	unsigned int step;		/* samples distance in bits */
} snd_pcm_channel_area_t;

typedef struct snd_pcm_channel_setup {
	unsigned int channel;
	snd_pcm_digital_t digital;	/* digital setup */
	snd_pcm_channel_area_t running_area;
	snd_pcm_channel_area_t stopped_area;
	char reserved[64];
} snd_pcm_channel_setup_t;

typedef struct snd_pcm_status {
	int state;		/* stream state - SND_PCM_STATE_XXXX */
	snd_timestamp_t trigger_time;	/* time when stream was started/stopped/paused */
	snd_timestamp_t tstamp;	/* Timestamp */
	ssize_t delay;		/* current delay in frames */
	size_t avail;		/* number of frames available */
	size_t avail_max;	/* max frames available on hw since last status */
	size_t overrange;	/* count of ADC (capture) overrange detections from last status */
	char reserved[64];	/* must be filled with zero */
} snd_pcm_status_t;

typedef struct {
	int state;		/* RO: status - SND_PCM_STATE_XXXX */
	int pad1;		/* Needed for 64 bit alignment */
	size_t hw_ptr;		/* RO: hw side ptr (0 ... boundary-1) 
				   updated only on request and at interrupt time */
	snd_timestamp_t tstamp;	/* Timestamp */
	char pad[PAGE_SIZE - (sizeof(int) * 2 + sizeof(size_t) +
			      sizeof(snd_timestamp_t))];		
} snd_pcm_mmap_status_t;

typedef struct {
	size_t appl_ptr;	/* RW: application side ptr (0...boundary-1) */
	size_t avail_min;	/* RW: min available frames for wakeup */
	char pad[PAGE_SIZE - sizeof(size_t) * 2];
} snd_pcm_mmap_control_t;

typedef struct {
	ssize_t result;
	void *buf;
	size_t frames;
} snd_xferi_t;

typedef struct {
	ssize_t result;
	void **bufs;
	size_t frames;
} snd_xfern_t;

#define SND_PCM_IOCTL_PVERSION		_IOR ('A', 0x00, int)
#define SND_PCM_IOCTL_INFO		_IOR ('A', 0x02, snd_pcm_info_t)
#define SND_PCM_IOCTL_PARAMS		_IOW ('A', 0x10, snd_pcm_params_t)
#define SND_PCM_IOCTL_PARAMS_INFO	_IOW ('A', 0x11, snd_pcm_params_info_t)
#define SND_PCM_IOCTL_SETUP		_IOR ('A', 0x20, snd_pcm_setup_t)
#define SND_PCM_IOCTL_STATUS		_IOR ('A', 0x21, snd_pcm_status_t)
#define SND_PCM_IOCTL_DELAY		_IOW ('A', 0x23, ssize_t)
#define SND_PCM_IOCTL_REWIND		_IOW ('A', 0x24, size_t)
#define SND_PCM_IOCTL_PREPARE		_IO  ('A', 0x30)
#define SND_PCM_IOCTL_START		_IO  ('A', 0x31)
#define SND_PCM_IOCTL_DROP		_IO  ('A', 0x32)
#define SND_PCM_IOCTL_DRAIN		_IO  ('A', 0x34)
#define SND_PCM_IOCTL_PAUSE		_IOW ('A', 0x35, int)
#define SND_PCM_IOCTL_CHANNEL_INFO	_IOR ('A', 0x40, snd_pcm_channel_info_t)
#define SND_PCM_IOCTL_CHANNEL_PARAMS	_IOW ('A', 0x41, snd_pcm_channel_params_t)
#define SND_PCM_IOCTL_CHANNEL_SETUP	_IOR ('A', 0x42, snd_pcm_channel_setup_t)
#define SND_PCM_IOCTL_WRITEI_FRAMES	_IOW ('A', 0x50, snd_xferi_t)
#define SND_PCM_IOCTL_READI_FRAMES	_IOR ('A', 0x51, snd_xferi_t)
#define SND_PCM_IOCTL_WRITEN_FRAMES	_IOW ('A', 0x52, snd_xfern_t)
#define SND_PCM_IOCTL_READN_FRAMES	_IOR ('A', 0x53, snd_xfern_t)
#define SND_PCM_IOCTL_LINK		_IOW ('A', 0x61, int)
#define SND_PCM_IOCTL_UNLINK		_IO  ('A', 0x62)

/*****************************************************************************
 *                                                                           *
 *                            MIDI v1.0 interface                            *
 *                                                                           *
 *****************************************************************************/

#define SND_MIDI_CHANNELS		16
#define SND_MIDI_GM_DRUM_CHANNEL	(10-1)

/*
 *  MIDI commands
 */

#define SND_MCMD_NOTE_OFF		0x80
#define SND_MCMD_NOTE_ON		0x90
#define SND_MCMD_NOTE_PRESSURE		0xa0
#define SND_MCMD_CONTROL		0xb0
#define SND_MCMD_PGM_CHANGE		0xc0
#define SND_MCMD_CHANNEL_PRESSURE	0xd0
#define SND_MCMD_BENDER			0xe0

#define SND_MCMD_COMMON_SYSEX		0xf0
#define SND_MCMD_COMMON_MTC_QUARTER	0xf1
#define SND_MCMD_COMMON_SONG_POS	0xf2
#define SND_MCMD_COMMON_SONG_SELECT	0xf3
#define SND_MCMD_COMMON_TUNE_REQUEST	0xf6
#define SND_MCMD_COMMON_SYSEX_END	0xf7
#define SND_MCMD_COMMON_CLOCK		0xf8
#define SND_MCMD_COMMON_START		0xfa
#define SND_MCMD_COMMON_CONTINUE	0xfb
#define SND_MCMD_COMMON_STOP		0xfc
#define SND_MCMD_COMMON_SENSING		0xfe
#define SND_MCMD_COMMON_RESET		0xff

/*
 *  MIDI controllers
 */

#define SND_MCTL_MSB_BANK		0x00
#define SND_MCTL_MSB_MODWHEEL         	0x01
#define SND_MCTL_MSB_BREATH           	0x02
#define SND_MCTL_MSB_FOOT             	0x04
#define SND_MCTL_MSB_PORTNAMENTO_TIME 	0x05
#define SND_MCTL_MSB_DATA_ENTRY		0x06
#define SND_MCTL_MSB_MAIN_VOLUME      	0x07
#define SND_MCTL_MSB_BALANCE          	0x08
#define SND_MCTL_MSB_PAN              	0x0a
#define SND_MCTL_MSB_EXPRESSION       	0x0b
#define SND_MCTL_MSB_EFFECT1		0x0c
#define SND_MCTL_MSB_EFFECT2		0x0d
#define SND_MCTL_MSB_GENERAL_PURPOSE1 	0x10
#define SND_MCTL_MSB_GENERAL_PURPOSE2 	0x11
#define SND_MCTL_MSB_GENERAL_PURPOSE3 	0x12
#define SND_MCTL_MSB_GENERAL_PURPOSE4 	0x13
#define SND_MCTL_LSB_BANK		0x20
#define SND_MCTL_LSB_MODWHEEL        	0x21
#define SND_MCTL_LSB_BREATH           	0x22
#define SND_MCTL_LSB_FOOT             	0x24
#define SND_MCTL_LSB_PORTNAMENTO_TIME 	0x25
#define SND_MCTL_LSB_DATA_ENTRY		0x26
#define SND_MCTL_LSB_MAIN_VOLUME      	0x27
#define SND_MCTL_LSB_BALANCE          	0x28
#define SND_MCTL_LSB_PAN              	0x2a
#define SND_MCTL_LSB_EXPRESSION       	0x2b
#define SND_MCTL_LSB_EFFECT1		0x2c
#define SND_MCTL_LSB_EFFECT2		0x2d
#define SND_MCTL_LSB_GENERAL_PURPOSE1 	0x30
#define SND_MCTL_LSB_GENERAL_PURPOSE2 	0x31
#define SND_MCTL_LSB_GENERAL_PURPOSE3 	0x32
#define SND_MCTL_LSB_GENERAL_PURPOSE4 	0x33
#define SND_MCTL_SUSTAIN              	0x40
#define SND_MCTL_PORTAMENTO           	0x41
#define SND_MCTL_SUSTENUTO            	0x42
#define SND_MCTL_SOFT_PEDAL           	0x43
#define SND_MCTL_LEGATO_FOOTSWITCH	0x44
#define SND_MCTL_HOLD2                	0x45
#define SND_MCTL_SC1_SOUND_VARIATION	0x46
#define SND_MCTL_SC2_TIMBRE		0x47
#define SND_MCTL_SC3_RELEASE_TIME	0x48
#define SND_MCTL_SC4_ATTACK_TIME	0x49
#define SND_MCTL_SC5_BRIGHTNESS		0x4a
#define SND_MCTL_SC6			0x4b
#define SND_MCTL_SC7			0x4c
#define SND_MCTL_SC8			0x4d
#define SND_MCTL_SC9			0x4e
#define SND_MCTL_SC10			0x4f
#define SND_MCTL_GENERAL_PURPOSE5     	0x50
#define SND_MCTL_GENERAL_PURPOSE6     	0x51
#define SND_MCTL_GENERAL_PURPOSE7     	0x52
#define SND_MCTL_GENERAL_PURPOSE8     	0x53
#define SND_MCTL_PORNAMENTO_CONTROL	0x54
#define SND_MCTL_E1_REVERB_DEPTH	0x5b
#define SND_MCTL_E2_TREMOLO_DEPTH	0x5c
#define SND_MCTL_E3_CHORUS_DEPTH	0x5d
#define SND_MCTL_E4_DETUNE_DEPTH	0x5e
#define SND_MCTL_E5_PHASER_DEPTH	0x5f
#define SND_MCTL_DATA_INCREMENT       	0x60
#define SND_MCTL_DATA_DECREMENT       	0x61
#define SND_MCTL_NONREG_PARM_NUM_LSB  	0x62
#define SND_MCTL_NONREG_PARM_NUM_MSB  	0x63
#define SND_MCTL_REGIST_PARM_NUM_LSB  	0x64
#define SND_MCTL_REGIST_PARM_NUM_MSB	0x65
#define SND_MCTL_ALL_SOUNDS_OFF		0x78
#define SND_MCTL_RESET_CONTROLLERS	0x79
#define SND_MCTL_LOCAL_CONTROL_SWITCH	0x7a
#define SND_MCTL_ALL_NOTES_OFF		0x7b
#define SND_MCTL_OMNI_OFF		0x7c
#define SND_MCTL_OMNI_ON		0x7d
#define SND_MCTL_MONO1			0x7e
#define SND_MCTL_MONO2			0x7f

/*
 *  Raw MIDI section - /dev/snd/midi??
 */

#define SND_RAWMIDI_VERSION		SND_PROTOCOL_VERSION(2, 0, 0)

#define SND_RAWMIDI_STREAM_OUTPUT	0
#define SND_RAWMIDI_STREAM_INPUT	1

#define SND_RAWMIDI_INFO_OUTPUT		0x00000001
#define SND_RAWMIDI_INFO_INPUT		0x00000002
#define SND_RAWMIDI_INFO_DUPLEX		0x00000004

typedef struct snd_rawmidi_info {
	int device;			/* WR: device number */
	int subdevice;			/* WR: subdevice number */
	unsigned int type;		/* soundcard type */
	unsigned int flags;		/* SND_RAWMIDI_INFO_XXXX */
	unsigned char id[64];		/* ID of this raw midi device (user selectable) */
	unsigned char name[80];		/* name of this raw midi device */
	unsigned int output_subdevices_count;
	unsigned int output_subdevices_avail;
	unsigned int input_subdevices_count;
	unsigned int input_subdevices_avail;
	unsigned char reserved[64];	/* reserved for future use */
} snd_rawmidi_info_t;

typedef struct snd_rawmidi_params {
	int when;		/* Params apply time/condition */
	snd_timestamp_t tstamp;	/* Timestamp */
	int stream;		/* Requested stream */
	size_t size;		/* I/O requested queue size in bytes */
	size_t min;		/* I minimum count of bytes in queue for wakeup */
	size_t max;		/* O maximum count of bytes in queue for wakeup */
	size_t room;		/* O minumum number of bytes writeable for wakeup */
	unsigned int fail_mask;	/* failure locations */
	int fail_reason;	/* failure reason */
	int no_active_sensing: 1; /* O do not send active sensing byte in close() */
	unsigned char reserved[16];	/* reserved for future use */
} snd_rawmidi_params_t;

typedef struct snd_rawmidi_setup {
	int stream;		/* Requested stream */
	size_t size;		/* I/O real queue queue size in bytes */
	size_t min;		/* I minimum count of bytes in queue for wakeup */
	size_t max;		/* O maximum count of bytes in queue for wakeup */
	size_t room;		/* O minumum number of bytes writeable for wakeup */
	int no_active_sensing: 1;	/* O do not send active sensing byte in close() */
	unsigned char reserved[16];	/* reserved for future use */
} snd_rawmidi_setup_t;

typedef struct snd_rawmidi_status {
	int stream;		/* Requested stream */
	snd_timestamp_t tstamp;	/* Timestamp */
	size_t count;		/* I/O number of bytes readable/writeable without blocking */
	size_t queue;		/* O number of bytes in queue */
	size_t pad;		/* O not used yet */
	size_t free;		/* I bytes in buffer still free */
	size_t overrun;		/* I count of overruns since last status (in bytes) */
	unsigned char reserved[16];	/* reserved for future use */
} snd_rawmidi_status_t;

#define SND_RAWMIDI_IOCTL_PVERSION	_IOR ('W', 0x00, int)
#define SND_RAWMIDI_IOCTL_INFO		_IOR ('W', 0x01, snd_rawmidi_info_t)
#define SND_RAWMIDI_IOCTL_STREAM_PARAMS _IOW ('W', 0x10, snd_rawmidi_params_t)
#define SND_RAWMIDI_IOCTL_STREAM_SETUP	_IOWR('W', 0x11, snd_rawmidi_setup_t)
#define SND_RAWMIDI_IOCTL_STREAM_STATUS _IOWR('W', 0x20, snd_rawmidi_status_t)
#define SND_RAWMIDI_IOCTL_STREAM_DROP	_IOW ('W', 0x30, int)
#define SND_RAWMIDI_IOCTL_STREAM_DRAIN	_IOW ('W', 0x31, int)

/*
 *  Timer section - /dev/snd/timer
 */

#define SND_TIMER_VERSION		SND_PROTOCOL_VERSION(2, 0, 0)

#define SND_TIMER_TYPE_GLOBAL		(0<<28)
#define SND_TIMER_TYPE_SOUNDCARD	(1<<28)
#define SND_TIMER_TYPE_PCM		(2<<28)
#define SND_TIMER_TYPE_MAX		(7<<28)

/* type */
#define SND_TIMER_TYPE(tmr)		(((tmr) >> 28) & 0x3f)
/* global number */
#define SND_TIMER_GLOBAL_MAX		0x000003ff
#define SND_TIMER_GLOBAL(tmr)		((tmr) & SND_TIMER_GLOBAL_MAX)
#define SND_TIMER_GLOBAL_SYSTEM		0	/* system timer number */
#define SND_TIMER_GLOBAL_RTC		1	/* RTC timer */
/* soundcard number */
#define SND_TIMER_SOUNDCARD_CARD_MAX	(SND_CARDS-1)
#define SND_TIMER_SOUNDCARD_CARD_SHIFT	22
#define SND_TIMER_SOUNDCARD_CARD(tmr)	(((tmr) >> SND_TIMER_SOUNDCARD_CARD_SHIFT) & SND_TIMER_SOUNDCARD_CARD_MAX)
#define SND_TIMER_SOUNDCARD_DEV_MAX	0x003fffff
#define SND_TIMER_SOUNDCARD_DEV(tmr)	((tmr) & SND_TIMER_SOUNDCARD_DEV_MAX)
#define SND_TIMER_SOUNDCARD(card,dev)	(SND_TIMER_TYPE_SOUNDCARD|(((card)&SND_TIMER_SOUNDCARD_CARD_MAX)<<SND_TIMER_SOUNDCARD_CARD_SHIFT)|((dev)&SND_TIMER_SOUNDCARD_DEV_MAX))
/* PCM slave timer numbers */
#if SND_CARDS > 64
#error "There is not enough space for the timer identifier."
#endif
#define SND_TIMER_PCM_CARD_MAX		(SND_CARDS-1)
#define SND_TIMER_PCM_CARD_SHIFT	22
#define SND_TIMER_PCM_CARD(tmr)		(((tmr) >> SND_TIMER_PCM_CARD_SHIFT) & SND_TIMER_PCM_CARD_MAX)
#define SND_TIMER_PCM_DEV_MAX		0x000003ff
#define SND_TIMER_PCM_DEV_SHIFT		12
#define SND_TIMER_PCM_DEV(tmr)		(((tmr) >> SND_TIMER_PCM_DEV_SHIFT) & SND_TIMER_PCM_DEV_MAX)
#define SND_TIMER_PCM_SUBDEV_MAX	0x00000fff
#define SND_TIMER_PCM_SUBDEV(tmr)	(tmr & SND_TIMER_PCM_SUBDEV_MAX)
#define SND_TIMER_PCM(card,dev,subdev)	(SND_TIMER_TYPE_PCM|(((card)&SND_TIMER_PCM_CARD_MAX)<<SND_TIMER_PCM_CARD_SHIFT)|(((dev)&SND_TIMER_PCM_DEV_MAX)<<SND_TIMER_PCM_DEV_SHIFT)|((subdev)&SND_TIMER_PCM_SUBDEV_MAX))

/* slave timer types */
#define SND_TIMER_STYPE_NONE		0
#define SND_TIMER_STYPE_APPLICATION	1
#define SND_TIMER_STYPE_SEQUENCER	2
#define SND_TIMER_STYPE_OSS_SEQUENCER	3

#define SND_TIMER_FLG_SLAVE		(1<<0)	/* cannot be controlled */

#define SND_TIMER_PSFLG_AUTO		(1<<0)	/* auto start */

typedef struct snd_timer_general_info {
	unsigned int count;		/* count of global timers */
	char reserved[64];
} snd_timer_general_info_t;

typedef struct snd_timer_select {
	unsigned int slave: 1;		/* timer is slave */
	union {
		int number;		/* timer number */
		struct {
			int type;	/* slave type - SND_TIMER_STYPE_ */
			int id;		/* slave identification */
		} slave;
	} data;
	char reserved[32];
} snd_timer_select_t;

typedef struct snd_timer_info {
	unsigned int flags;		/* timer flags - SND_MIXER_FLG_* */
	char id[64];			/* timer identificator (user selectable) */
	char name[80];			/* timer name */
	unsigned long ticks;		/* maximum ticks */
	unsigned long resolution;	/* average resolution */
	char reserved[64];
} snd_timer_info_t;

typedef struct snd_timer_params {
	int when;			/* Params apply time/condition */
	snd_timestamp_t tstamp;		/* Timestamp */
	unsigned int flags;		/* flags - SND_MIXER_PSFLG_* */
	unsigned long ticks;		/* requested resolution in ticks */
	int queue_size;			/* total size of queue (32-1024) */
	unsigned int fail_mask;		/* failure locations */
	int fail_reason;		/* failure reason */
	char reserved[64];
} snd_timer_params_t;

typedef struct snd_timer_setup {
	unsigned int flags;		/* flags - SND_MIXER_PSFLG_* */
	unsigned long ticks;		/* requested resolution in ticks */
	int queue_size;			/* total queue size */
	char reserved[64];
} snd_timer_setup_t;

typedef struct snd_timer_status {
	snd_timestamp_t tstamp;		/* Timestamp */
	unsigned long resolution;	/* current resolution */
	unsigned long lost;		/* counter of master tick lost */
	unsigned long overrun;		/* count of read queue overruns */
	int queue;			/* used queue size */
	char reserved[64];
} snd_timer_status_t;

#define SND_TIMER_IOCTL_PVERSION	_IOR ('T', 0x00, int)
#define SND_TIMER_IOCTL_GINFO		_IOW ('T', 0x01, snd_timer_general_info_t)
#define SND_TIMER_IOCTL_SELECT		_IOW ('T', 0x10, snd_timer_select_t)
#define SND_TIMER_IOCTL_INFO		_IOR ('T', 0x11, snd_timer_info_t)
#define SND_TIMER_IOCTL_PARAMS		_IOW ('T', 0x12, snd_timer_params_t)
#define SND_TIMER_IOCTL_SETUP		_IOR ('T', 0x13, snd_timer_setup_t)
#define SND_TIMER_IOCTL_STATUS		_IOW ('T', 0x14, snd_timer_status_t)
#define SND_TIMER_IOCTL_START		_IO  ('T', 0x20)
#define SND_TIMER_IOCTL_STOP		_IO  ('T', 0x21)
#define SND_TIMER_IOCTL_CONTINUE	_IO  ('T', 0x22)

typedef struct snd_timer_read {
	unsigned long resolution;
	unsigned long ticks;
} snd_timer_read_t;

/*
 *
 */

#define SND_IOCTL_READV		_IOW ('K', 0x00, snd_xferv_t)
#define SND_IOCTL_WRITEV	_IOW ('K', 0x01, snd_xferv_t)

#endif				/* __ASOUND_H */
