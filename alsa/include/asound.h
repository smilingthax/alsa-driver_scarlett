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
#define SND_CARD_TYPE_MPU401_UART	0x0000003c	/* MPU-401 UART */
#define SND_CARD_TYPE_ALS4000		0x0000003d	/* Avance Logic ALS4000 */
#define SND_CARD_TYPE_ALLEGRO_1		0x0000003f	/* ESS Allegro-1 */
#define SND_CARD_TYPE_ALLEGRO		0x00000040	/* ESS Allegro */
#define SND_CARD_TYPE_MAESTRO3		0x00000041	/* ESS Maestro3 */
#define SND_CARD_TYPE_AWACS		0x00000042	/* PMac AWACS */

#define SND_CARD_TYPE_LAST		0x0000003d

typedef struct timeval snd_timestamp_t;

/****************************************************************************
 *                                                                          *
 *        Section for driver control interface - /dev/snd/control?          *
 *                                                                          *
 ****************************************************************************/

#define SND_CTL_VERSION			SND_PROTOCOL_VERSION(2, 0, 0)

typedef struct _snd_ctl_hw_info {
	unsigned int type;	/* type of card - look to SND_CARD_TYPE_XXXX */
	char id[16];		/* ID of card (user selectable) */
	char abbreviation[16];	/* Abbreviation for soundcard */
	char name[32];		/* Short name of soundcard */
	char longname[80];	/* name + info text about soundcard */
	char mixerid[16];	/* ID of mixer */
	char mixername[80];	/* mixer identification */
	char reserved[128];	/* reserved for future */
} snd_ctl_hw_info_t;

typedef enum _snd_control_type {
	SND_CONTROL_TYPE_NONE = 0,		/* invalid */
	SND_CONTROL_TYPE_BOOLEAN,		/* boolean type */
	SND_CONTROL_TYPE_INTEGER,		/* integer type */
	SND_CONTROL_TYPE_ENUMERATED,		/* enumerated type */
	SND_CONTROL_TYPE_BYTES			/* byte array */
} snd_control_type_t;

typedef enum _snd_control_iface {
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

typedef struct _snd_control_id {
	unsigned int numid;		/* numeric identifier, zero = invalid */
	snd_control_iface_t iface;	/* interface identifier */
	unsigned int device;		/* device/client number */
	unsigned int subdevice;		/* subdevice (substream) number */
        unsigned char name[44];		/* ASCII name of item */
	unsigned int index;		/* index of item */
} snd_control_id_t;

typedef struct _snd_control_list {
	unsigned int controls_offset;	/* W: first control ID to get */
	unsigned int controls_request;	/* W: count of control IDs to get */
	unsigned int controls_count;	/* R: count of available (set) IDs */
	unsigned int controls;		/* R: count of all available controls */
	snd_control_id_t *pids;		/* W: IDs */
        char reserved[50];
} snd_control_list_t;

typedef struct _snd_control_info {
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
		char reserved[128];
	} value;
	char reserved[64];
} snd_control_info_t;

typedef struct _snd_control_t {
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
        char reserved[128];
} snd_control_t;

#define SND_CTL_IOCTL_PVERSION		_IOR ('U', 0x00, int)
#define SND_CTL_IOCTL_HW_INFO		_IOR ('U', 0x01, snd_ctl_hw_info_t)
#define SND_CTL_IOCTL_CONTROL_LIST	_IOWR('U', 0x10, snd_control_list_t)
#define SND_CTL_IOCTL_CONTROL_INFO	_IOWR('U', 0x11, snd_control_info_t)
#define SND_CTL_IOCTL_CONTROL_READ	_IOWR('U', 0x12, snd_control_t)
#define SND_CTL_IOCTL_CONTROL_WRITE	_IOWR('U', 0x13, snd_control_t)
#define SND_CTL_IOCTL_CONTROL_LOCK	_IOW ('U', 0x14, snd_control_id_t)
#define SND_CTL_IOCTL_CONTROL_UNLOCK	_IOW ('U', 0x15, snd_control_id_t)
#define SND_CTL_IOCTL_HWDEP_NEXT_DEVICE	_IOWR('U', 0x20, int)
#define SND_CTL_IOCTL_HWDEP_INFO	_IOR ('U', 0x21, snd_hwdep_info_t)
#define SND_CTL_IOCTL_PCM_NEXT_DEVICE	_IOR ('U', 0x30, int)
#define SND_CTL_IOCTL_PCM_INFO		_IOWR('U', 0x31, snd_pcm_info_t)
#define SND_CTL_IOCTL_PCM_PREFER_SUBDEVICE _IOW('U', 0x32, int)
#define SND_CTL_IOCTL_RAWMIDI_NEXT_DEVICE _IOWR('U', 0x40, int)
#define SND_CTL_IOCTL_RAWMIDI_INFO	_IOWR('U', 0x41, snd_rawmidi_info_t)
#define SND_CTL_IOCTL_RAWMIDI_PREFER_SUBDEVICE _IOW('U', 0x42, int)

/*
 *  Read interface.
 */

typedef enum _snd_ctl_event_type {
	SND_CTL_EVENT_REBUILD = 0,	/* rebuild everything */
	SND_CTL_EVENT_VALUE,		/* a control value was changed */
	SND_CTL_EVENT_CHANGE,		/* a control was changed */
	SND_CTL_EVENT_ADD,		/* a control was added */
	SND_CTL_EVENT_REMOVE,		/* a control was removed */
} snd_ctl_event_type_t;

typedef struct _snd_ctl_event {
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

typedef struct _snd_hwdep_info {
	int device;		/* WR: device number */
	unsigned int type;	/* type of card - look to SND_CARD_TYPE_XXXX */
	unsigned char id[64];	/* ID of this hardware dependent device (user selectable) */
	unsigned char name[80];	/* name of this hardware dependent device */
	unsigned int hw_type;	/* hardware depedent device type */
	char reserved[64];	/* reserved for future */
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

#define SND_PCM_ACCESS_MMAP_INTERLEAVED		0 /* interleaved mmap */
#define SND_PCM_ACCESS_MMAP_NONINTERLEAVED	1 /* noninterleaved mmap */
#define SND_PCM_ACCESS_MMAP_COMPLEX		2 /* complex mmap */
#define SND_PCM_ACCESS_RW_INTERLEAVED		3 /* readi/writei */
#define SND_PCM_ACCESS_RW_NONINTERLEAVED	4 /* readn/writen */
#define SND_PCM_ACCESS_LAST			4

#define SND_PCM_ACCBIT_MMAP_INTERLEAVED	(1 << SND_PCM_ACCESS_MMAP_INTERLEAVED)
#define SND_PCM_ACCBIT_MMAP_NONINTERLEAVED (1 << SND_PCM_ACCESS_MMAP_NONINTERLEAVED)
#define SND_PCM_ACCBIT_MMAP_COMPLEX	(1 << SND_PCM_ACCESS_MMAP_COMPLEX)
#define SND_PCM_ACCBIT_RW_INTERLEAVED	(1 << SND_PCM_ACCESS_RW_INTERLEAVED)
#define SND_PCM_ACCBIT_RW_NONINTERLEAVED (1 << SND_PCM_ACCESS_RW_NONINTERLEAVED)
#define SND_PCM_ACCBIT_MMAP	(SND_PCM_ACCBIT_MMAP_INTERLEAVED | \
				 SND_PCM_ACCBIT_MMAP_NONINTERLEAVED | \
				 SND_PCM_ACCBIT_MMAP_COMPLEX)

#define SND_PCM_FORMAT_S8		0
#define SND_PCM_FORMAT_U8		1
#define SND_PCM_FORMAT_S16_LE		2
#define SND_PCM_FORMAT_S16_BE		3
#define SND_PCM_FORMAT_U16_LE		4
#define SND_PCM_FORMAT_U16_BE		5
#define SND_PCM_FORMAT_S24_LE		6	/* low three bytes */
#define SND_PCM_FORMAT_S24_BE		7	/* low three bytes */
#define SND_PCM_FORMAT_U24_LE		8	/* low three bytes */
#define SND_PCM_FORMAT_U24_BE		9	/* low three bytes */
#define SND_PCM_FORMAT_S32_LE		10
#define SND_PCM_FORMAT_S32_BE		11
#define SND_PCM_FORMAT_U32_LE		12
#define SND_PCM_FORMAT_U32_BE		13
#define SND_PCM_FORMAT_FLOAT_LE		14	/* 4-byte float, IEEE-754 32-bit */
#define SND_PCM_FORMAT_FLOAT_BE		15	/* 4-byte float, IEEE-754 32-bit */
#define SND_PCM_FORMAT_FLOAT64_LE	16	/* 8-byte float, IEEE-754 64-bit */
#define SND_PCM_FORMAT_FLOAT64_BE	17	/* 8-byte float, IEEE-754 64-bit */
#define SND_PCM_FORMAT_IEC958_SUBFRAME_LE 18	/* IEC-958 subframe, Little Endian */
#define SND_PCM_FORMAT_IEC958_SUBFRAME_BE 19	/* IEC-958 subframe, Big Endian */
#define SND_PCM_FORMAT_MU_LAW		20
#define SND_PCM_FORMAT_A_LAW		21
#define SND_PCM_FORMAT_IMA_ADPCM	22
#define SND_PCM_FORMAT_MPEG		23
#define SND_PCM_FORMAT_GSM		24
#define SND_PCM_FORMAT_SPECIAL		31
#define SND_PCM_FORMAT_LAST		31

#ifdef SND_LITTLE_ENDIAN
#define SND_PCM_FORMAT_S16		SND_PCM_FORMAT_S16_LE
#define SND_PCM_FORMAT_U16		SND_PCM_FORMAT_U16_LE
#define SND_PCM_FORMAT_S24		SND_PCM_FORMAT_S24_LE
#define SND_PCM_FORMAT_U24		SND_PCM_FORMAT_U24_LE
#define SND_PCM_FORMAT_S32		SND_PCM_FORMAT_S32_LE
#define SND_PCM_FORMAT_U32		SND_PCM_FORMAT_U32_LE
#define SND_PCM_FORMAT_FLOAT		SND_PCM_FORMAT_FLOAT_LE
#define SND_PCM_FORMAT_FLOAT64		SND_PCM_FORMAT_FLOAT64_LE
#define SND_PCM_FORMAT_IEC958_SUBFRAME	SND_PCM_FORMAT_IEC958_SUBFRAME_LE
#endif
#ifdef SND_BIG_ENDIAN
#define SND_PCM_FORMAT_S16		SND_PCM_FORMAT_S16_BE
#define SND_PCM_FORMAT_U16		SND_PCM_FORMAT_U16_BE
#define SND_PCM_FORMAT_S24		SND_PCM_FORMAT_S24_BE
#define SND_PCM_FORMAT_U24		SND_PCM_FORMAT_U24_BE
#define SND_PCM_FORMAT_S32		SND_PCM_FORMAT_S32_BE
#define SND_PCM_FORMAT_U32		SND_PCM_FORMAT_U32_BE
#define SND_PCM_FORMAT_FLOAT		SND_PCM_FORMAT_FLOAT_BE
#define SND_PCM_FORMAT_FLOAT64		SND_PCM_FORMAT_FLOAT64_BE
#define SND_PCM_FORMAT_IEC958_SUBFRAME	SND_PCM_FORMAT_IEC958_SUBFRAME_BE
#endif

#define SND_PCM_FMTBIT_S8		(1 << SND_PCM_FORMAT_S8)
#define SND_PCM_FMTBIT_U8		(1 << SND_PCM_FORMAT_U8)
#define SND_PCM_FMTBIT_S16_LE		(1 << SND_PCM_FORMAT_S16_LE)
#define SND_PCM_FMTBIT_S16_BE		(1 << SND_PCM_FORMAT_S16_BE)
#define SND_PCM_FMTBIT_U16_LE		(1 << SND_PCM_FORMAT_U16_LE)
#define SND_PCM_FMTBIT_U16_BE		(1 << SND_PCM_FORMAT_U16_BE)
#define SND_PCM_FMTBIT_S24_LE		(1 << SND_PCM_FORMAT_S24_LE)
#define SND_PCM_FMTBIT_S24_BE		(1 << SND_PCM_FORMAT_S24_BE)
#define SND_PCM_FMTBIT_U24_LE		(1 << SND_PCM_FORMAT_U24_LE)
#define SND_PCM_FMTBIT_U24_BE		(1 << SND_PCM_FORMAT_U24_BE)
#define SND_PCM_FMTBIT_S32_LE		(1 << SND_PCM_FORMAT_S32_LE)
#define SND_PCM_FMTBIT_S32_BE		(1 << SND_PCM_FORMAT_S32_BE)
#define SND_PCM_FMTBIT_U32_LE		(1 << SND_PCM_FORMAT_U32_LE)
#define SND_PCM_FMTBIT_U32_BE		(1 << SND_PCM_FORMAT_U32_BE)
#define SND_PCM_FMTBIT_FLOAT_LE		(1 << SND_PCM_FORMAT_FLOAT_LE)
#define SND_PCM_FMTBIT_FLOAT_BE		(1 << SND_PCM_FORMAT_FLOAT_BE)
#define SND_PCM_FMTBIT_FLOAT64_LE	(1 << SND_PCM_FORMAT_FLOAT64_LE)
#define SND_PCM_FMTBIT_FLOAT64_BE	(1 << SND_PCM_FORMAT_FLOAT64_BE)
#define SND_PCM_FMTBIT_IEC958_SUBFRAME_LE (1 << SND_PCM_FORMAT_IEC958_SUBFRAME_LE)
#define SND_PCM_FMTBIT_IEC958_SUBFRAME_BE (1 << SND_PCM_FORMAT_IEC958_SUBFRAME_BE)
#define SND_PCM_FMTBIT_MU_LAW		(1 << SND_PCM_FORMAT_MU_LAW)
#define SND_PCM_FMTBIT_A_LAW		(1 << SND_PCM_FORMAT_A_LAW)
#define SND_PCM_FMTBIT_IMA_ADPCM	(1 << SND_PCM_FORMAT_IMA_ADPCM)
#define SND_PCM_FMTBIT_MPEG		(1 << SND_PCM_FORMAT_MPEG)
#define SND_PCM_FMTBIT_GSM		(1 << SND_PCM_FORMAT_GSM)
#define SND_PCM_FMTBIT_SPECIAL		(1 << SND_PCM_FORMAT_SPECIAL)

#ifdef SND_LITTLE_ENDIAN
#define SND_PCM_FMTBIT_S16		SND_PCM_FMTBIT_S16_LE
#define SND_PCM_FMTBIT_U16		SND_PCM_FMTBIT_U16_LE
#define SND_PCM_FMTBIT_S24		SND_PCM_FMTBIT_S24_LE
#define SND_PCM_FMTBIT_U24		SND_PCM_FMTBIT_U24_LE
#define SND_PCM_FMTBIT_S32		SND_PCM_FMTBIT_S32_LE
#define SND_PCM_FMTBIT_U32		SND_PCM_FMTBIT_U32_LE
#define SND_PCM_FMTBIT_FLOAT		SND_PCM_FMTBIT_FLOAT_LE
#define SND_PCM_FMTBIT_FLOAT64		SND_PCM_FMTBIT_FLOAT64_LE
#define SND_PCM_FMTBIT_IEC958_SUBFRAME	SND_PCM_FMTBIT_IEC958_SUBFRAME_LE
#endif
#ifdef SND_BIG_ENDIAN
#define SND_PCM_FMTBIT_S16		SND_PCM_FMTBIT_S16_BE
#define SND_PCM_FMTBIT_U16		SND_PCM_FMTBIT_U16_BE
#define SND_PCM_FMTBIT_S24		SND_PCM_FMTBIT_S24_BE
#define SND_PCM_FMTBIT_U24		SND_PCM_FMTBIT_U24_BE
#define SND_PCM_FMTBIT_S32		SND_PCM_FMTBIT_S32_BE
#define SND_PCM_FMTBIT_U32		SND_PCM_FMTBIT_U32_BE
#define SND_PCM_FMTBIT_FLOAT		SND_PCM_FMTBIT_FLOAT_BE
#define SND_PCM_FMTBIT_FLOAT64		SND_PCM_FMTBIT_FLOAT64_BE
#define SND_PCM_FMTBIT_IEC958_SUBFRAME	SND_PCM_FMTBIT_IEC958_SUBFRAME_BE
#endif

#define SND_PCM_SUBFORMAT_STD		0
#define SND_PCM_SUBFORMAT_LAST		0

#define SND_PCM_SUBFMTBIT_STD		(1<<SND_PCM_SUBFORMAT_STD)

#define SND_PCM_INFO_MMAP		0x00000001	/* hardware supports mmap */
#define SND_PCM_INFO_MMAP_VALID		0x00000002	/* fragment data are valid during transfer */
#define SND_PCM_INFO_DOUBLE		0x00000004	/* Double buffering needed for PCM start/stop */
#define SND_PCM_INFO_BATCH		0x00000010	/* double buffering */
#define SND_PCM_INFO_INTERLEAVED	0x00000100	/* channels are interleaved */
#define SND_PCM_INFO_NONINTERLEAVED	0x00000200	/* channels are not interleaved */
#define SND_PCM_INFO_COMPLEX		0x00000400	/* complex frame organization (mmap only) */
#define SND_PCM_INFO_BLOCK_TRANSFER	0x00010000	/* hardware transfer block of samples */
#define SND_PCM_INFO_OVERRANGE		0x00020000	/* hardware supports ADC (capture) overrange detection */
#define SND_PCM_INFO_PAUSE		0x00080000	/* pause ioctl is supported */
#define SND_PCM_INFO_HALF_DUPLEX	0x00100000	/* only half duplex */
#define SND_PCM_INFO_JOINT_DUPLEX	0x00200000	/* playback and capture stream are somewhat correlated */
#define SND_PCM_INFO_SYNC_START		0x00400000	/* pcm support some kind of sync go */

#define SND_PCM_START_DATA		0	/* start when some data are written (playback) or requested (capture) */
#define SND_PCM_START_EXPLICIT		1	/* start on the go command */
#define SND_PCM_START_LAST		1

#define SND_PCM_READY_FRAGMENT		0	/* Efficient ready detection */
#define SND_PCM_READY_ASAP		1	/* Accurate ready detection */
#define SND_PCM_READY_LAST		1

#define SND_PCM_XRUN_FRAGMENT		0	/* Efficient xrun detection */
#define SND_PCM_XRUN_ASAP		1	/* Accurate xrun detection */
#define SND_PCM_XRUN_NONE		2	/* No xrun detection */
#define SND_PCM_XRUN_LAST		2

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
#define SND_PCM_DIG_NONE		(-1)
#define SND_PCM_DIG_AES_IEC958C		0	/* consumer mode */
#define SND_PCM_DIG_AES_IEC958P		1	/* professional mode */
#define SND_PCM_DIGBIT_AES_IEC958C	(1<<0)	/* consumer mode */
#define SND_PCM_DIGBIT_AES_IEC958P	(1<<1)	/* professional mode */

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

typedef union _snd_pcm_sync_id {
	char id[16];
	short id16[8];
	int id32[4];
} snd_pcm_sync_id_t;

typedef struct _snd_pcm_info {
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

typedef union _snd_pcm_digital {
	struct {
		unsigned char status[24];	/* AES/IEC958 channel status bits */
		unsigned char subcode[147];	/* AES/IEC958 subcode bits */
		unsigned char pad;		/* nothing */
		unsigned char dig_subframe[4];	/* AES/IEC958 subframe bits */
	} aes;
	char reserved[256];
} snd_pcm_digital_t;

#define SND_PCM_DIG_PARBIT_GROUP	(1<<0)
#define SND_PCM_DIG_PARBIT_TYPE		(1<<1)
#define SND_PCM_DIG_PARBIT_VALUE	(1<<2)	/* data integrity error */

typedef struct _snd_pcm_dig_info {
	int group;			/* W : channels group */
	unsigned int type_mask;		/* R : supported digital types */
	int type;			/* W : digital type (none - don't handle mask/val) */
	snd_pcm_digital_t mask;		/* R : supported bits */
	snd_pcm_digital_t val;		/* R : actual value */
	unsigned int fail_mask;		/* R : failure locations */
	char reserved[64];
} snd_pcm_dig_info_t;

typedef struct _snd_pcm_dig_params {
	int group;			/* W : channels group */
	int type;			/* W : digital type */
	snd_pcm_digital_t val;		/* W : values */
	unsigned int fail_mask;		/* R : failure locations */
	char reserved[64];
} snd_pcm_dig_params_t;

#define SND_PCM_HW_PARAM_ACCESS		0
#define SND_PCM_HW_PARAM_FORMAT		1
#define SND_PCM_HW_PARAM_SUBFORMAT	2
#define SND_PCM_HW_PARAM_CHANNELS	3
#define SND_PCM_HW_PARAM_RATE		4
#define SND_PCM_HW_PARAM_FRAGMENT_SIZE	5
#define SND_PCM_HW_PARAM_FRAGMENTS	6
#define SND_PCM_HW_PARAM_BUFFER_SIZE	7
#define SND_PCM_HW_PARAM_LAST		7

#define SND_PCM_HW_PARBIT_ACCESS	(1<<SND_PCM_HW_PARAM_ACCESS)
#define SND_PCM_HW_PARBIT_FORMAT	(1<<SND_PCM_HW_PARAM_FORMAT)
#define SND_PCM_HW_PARBIT_SUBFORMAT	(1<<SND_PCM_HW_PARAM_SUBFORMAT)
#define SND_PCM_HW_PARBIT_CHANNELS	(1<<SND_PCM_HW_PARAM_CHANNELS)
#define SND_PCM_HW_PARBIT_RATE		(1<<SND_PCM_HW_PARAM_RATE)
#define SND_PCM_HW_PARBIT_FRAGMENT_SIZE (1<<SND_PCM_HW_PARAM_FRAGMENT_SIZE)
#define SND_PCM_HW_PARBIT_FRAGMENTS	(1<<SND_PCM_HW_PARAM_FRAGMENTS)
#define SND_PCM_HW_PARBIT_BUFFER_SIZE	(1<<SND_PCM_HW_PARAM_BUFFER_SIZE)

typedef struct _snd_pcm_hw_info {
	unsigned int access_mask;	/* RW: access mask */
	unsigned int format_mask;	/* RW: format mask */
	unsigned int subformat_mask;	/* RW: subformat mask */
	unsigned int channels_min;	/* RW: min channels */
	unsigned int channels_max;	/* RW: max channels */
	unsigned int rate_min;		/* RW: min rate */
	unsigned int rate_max;		/* RW: max rate */
	size_t fragment_size_min;	/* RW: min fragment size */
	size_t fragment_size_max;	/* RW: max fragment size */
	unsigned int fragments_min;	/* RW: min fragments */
	unsigned int fragments_max;	/* RW: max fragments */
	size_t buffer_size_min;		/* RW: min buffer size */
	size_t buffer_size_max;		/* RW: max buffer size */
	/* The following fields are filled only when applicable to 
	   all params combinations */
	unsigned int info;		/* R: Info for returned setup */
	unsigned int msbits;		/* R: used most significant bits */
	unsigned int rate_num;		/* R: rate numerator */
	unsigned int rate_den;		/* R: rate denominator */
	size_t fifo_size;		/* R: chip FIFO size in frames */
	unsigned int dig_groups;	/* R: number of channel groups for digital setup */
	char reserved[64];
} snd_pcm_hw_info_t;

typedef struct _snd_pcm_hw_params {
	unsigned int access;		/* W: access mode */
	unsigned int format;		/* W: SND_PCM_FORMAT_* */
	unsigned int subformat;		/* W: subformat */
	unsigned int channels;		/* W: channels */
	unsigned int rate;		/* W: rate in Hz */
	size_t fragment_size;		/* W: fragment size */
	unsigned int fragments;		/* W: fragments */
	unsigned int fail_mask;		/* R: failure locations */
	char reserved[64];
} snd_pcm_hw_params_t;

#define SND_PCM_SW_PARAM_START_MODE	0
#define SND_PCM_SW_PARAM_READY_MODE	1
#define SND_PCM_SW_PARAM_AVAIL_MIN	2
#define SND_PCM_SW_PARAM_XFER_MIN	3
#define SND_PCM_SW_PARAM_XFER_ALIGN	4
#define SND_PCM_SW_PARAM_XRUN_MODE	5
#define SND_PCM_SW_PARAM_TIME		6
#define SND_PCM_SW_PARAM_LAST		6

#define SND_PCM_SW_PARBIT_START_MODE	(1<<SND_PCM_SW_PARAM_START_MODE)
#define SND_PCM_SW_PARBIT_READY_MODE	(1<<SND_PCM_SW_PARAM_READY_MODE)
#define SND_PCM_SW_PARBIT_AVAIL_MIN	(1<<SND_PCM_SW_PARAM_AVAIL_MIN)
#define SND_PCM_SW_PARBIT_XFER_MIN	(1<<SND_PCM_SW_PARAM_XFER_MIN)
#define SND_PCM_SW_PARBIT_XFER_ALIGN	(1<<SND_PCM_SW_PARAM_XFER_ALIGN)
#define SND_PCM_SW_PARBIT_XRUN_MODE	(1<<SND_PCM_SW_PARAM_XRUN_MODE)
#define SND_PCM_SW_PARBIT_TIME		(1<<SND_PCM_SW_PARAM_TIME)

typedef struct _snd_pcm_sw_params {
	unsigned int start_mode;	/* RW: start mode */
	unsigned int ready_mode;	/* RW: ready detection mode */
	unsigned int xrun_mode;		/* RW: xrun detection mode */
	size_t avail_min;		/* RW: min avail frames for wakeup */
	size_t xfer_min;		/* RW: xfer min size */
	size_t xfer_align;		/* RW: xfer size need to be a multiple */
	unsigned int time: 1;		/* RW: mmap timestamp switch */
	size_t boundary;		/* R : pointers wrap point */
	unsigned int fail_mask;		/* R : failure locations */
	char reserved[64];
} snd_pcm_sw_params_t;

typedef struct _snd_pcm_hw_channel_info {
	unsigned int channel;
	off_t offset;			/* mmap offset */
	unsigned int first;		/* offset to first sample in bits */
	unsigned int step;		/* samples distance in bits */
} snd_pcm_hw_channel_info_t;

typedef struct _snd_pcm_status {
	int state;		/* stream state - SND_PCM_STATE_XXXX */
	snd_timestamp_t trigger_time;	/* time when stream was started/stopped/paused */
	snd_timestamp_t tstamp;	/* Timestamp */
	ssize_t delay;		/* current delay in frames */
	size_t avail;		/* number of frames available */
	size_t avail_max;	/* max frames available on hw since last status */
	size_t overrange;	/* count of ADC (capture) overrange detections from last status */
	char reserved[64];	/* must be filled with zero */
} snd_pcm_status_t;

typedef struct _snd_pcm_mmap_status {
	int state;		/* RO: state - SND_PCM_STATE_XXXX */
	int pad1;		/* Needed for 64 bit alignment */
	size_t hw_ptr;		/* RO: hw side ptr (0 ... boundary-1) 
				   updated only on request and at interrupt time */
	snd_timestamp_t tstamp;	/* Timestamp */
} snd_pcm_mmap_status_t;

typedef struct _snd_pcm_mmap_control {
	size_t appl_ptr;	/* RW: application side ptr (0...boundary-1) */
	size_t avail_min;	/* RW: min available frames for wakeup */
} snd_pcm_mmap_control_t;

typedef struct _snd_xferi {
	ssize_t result;
	void *buf;
	size_t frames;
} snd_xferi_t;

typedef struct _snd_xfern_t {
	ssize_t result;
	void **bufs;
	size_t frames;
} snd_xfern_t;

#define SND_PCM_IOCTL_PVERSION		_IOR ('A', 0x00, int)
#define SND_PCM_IOCTL_INFO		_IOR ('A', 0x01, snd_pcm_info_t)
#define SND_PCM_IOCTL_HW_INFO		_IOWR('A', 0x10, snd_pcm_hw_info_t)
#define SND_PCM_IOCTL_HW_PARAMS		_IOWR('A', 0x11, snd_pcm_hw_params_t)
#define SND_PCM_IOCTL_SW_PARAMS		_IOWR('A', 0x12, snd_pcm_sw_params_t)
#define SND_PCM_IOCTL_DIG_INFO		_IOWR('A', 0x13, snd_pcm_dig_info_t)
#define SND_PCM_IOCTL_DIG_PARAMS	_IOWR('A', 0x14, snd_pcm_dig_params_t)
#define SND_PCM_IOCTL_STATUS		_IOR ('A', 0x20, snd_pcm_status_t)
#define SND_PCM_IOCTL_DELAY		_IOR ('A', 0x21, ssize_t)
#define SND_PCM_IOCTL_CHANNEL_INFO	_IOR ('A', 0x32, snd_pcm_hw_channel_info_t)
#define SND_PCM_IOCTL_PREPARE		_IO  ('A', 0x40)
#define SND_PCM_IOCTL_RESET		_IO  ('A', 0x41)
#define SND_PCM_IOCTL_START		_IO  ('A', 0x42)
#define SND_PCM_IOCTL_DROP		_IO  ('A', 0x43)
#define SND_PCM_IOCTL_DRAIN		_IO  ('A', 0x44)
#define SND_PCM_IOCTL_PAUSE		_IOW ('A', 0x45, int)
#define SND_PCM_IOCTL_REWIND		_IOW ('A', 0x46, size_t)
#define SND_PCM_IOCTL_WRITEI_FRAMES	_IOW ('A', 0x50, snd_xferi_t)
#define SND_PCM_IOCTL_READI_FRAMES	_IOR ('A', 0x51, snd_xferi_t)
#define SND_PCM_IOCTL_WRITEN_FRAMES	_IOW ('A', 0x52, snd_xfern_t)
#define SND_PCM_IOCTL_READN_FRAMES	_IOR ('A', 0x53, snd_xfern_t)
#define SND_PCM_IOCTL_LINK		_IOW ('A', 0x60, int)
#define SND_PCM_IOCTL_UNLINK		_IO  ('A', 0x61)

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

typedef struct _snd_rawmidi_info {
	int device;			/* WR: device number */
	int subdevice;			/* RO/WR (control): subdevice number */
	unsigned int type;		/* soundcard type */
	unsigned int flags;		/* SND_RAWMIDI_INFO_XXXX */
	unsigned char id[64];		/* ID of this raw midi device (user selectable) */
	unsigned char name[80];		/* name of this raw midi device */
	unsigned char subname[32];	/* name of active or selected subdevice */
	unsigned int output_subdevices_count;
	unsigned int output_subdevices_avail;
	unsigned int input_subdevices_count;
	unsigned int input_subdevices_avail;
	char reserved[64];	/* reserved for future use */
} snd_rawmidi_info_t;

#define SND_RAWMIDI_PARBIT_STREAM	(1<<0)
#define SND_RAWMIDI_PARBIT_BUFFER_SIZE	(1<<1)
#define SND_RAWMIDI_PARBIT_AVAIL_MIN	(1<<2)

typedef struct _snd_rawmidi_params {
	int stream;		/* Requested stream */
	size_t buffer_size;	/* requested queue size in bytes */
	size_t avail_min;	/* minimum avail bytes for wakeup */
	unsigned int fail_mask;	/* failure locations */
	unsigned int no_active_sensing: 1; /* do not send active sensing byte in close() */
	char reserved[16];	/* reserved for future use */
} snd_rawmidi_params_t;

typedef struct _snd_rawmidi_status {
	int stream;		/* Requested stream */
	snd_timestamp_t tstamp;	/* Timestamp */
	size_t avail;		/* available bytes */
	size_t xruns;		/* I count of overruns since last status (in bytes) */
	char reserved[16];	/* reserved for future use */
} snd_rawmidi_status_t;

#define SND_RAWMIDI_IOCTL_PVERSION	_IOR ('W', 0x00, int)
#define SND_RAWMIDI_IOCTL_INFO		_IOR ('W', 0x01, snd_rawmidi_info_t)
#define SND_RAWMIDI_IOCTL_PARAMS	_IOWR('W', 0x10, snd_rawmidi_params_t)
#define SND_RAWMIDI_IOCTL_STATUS	_IOWR('W', 0x20, snd_rawmidi_status_t)
#define SND_RAWMIDI_IOCTL_DROP		_IOW ('W', 0x30, int)
#define SND_RAWMIDI_IOCTL_DRAIN		_IOW ('W', 0x31, int)

/*
 *  Timer section - /dev/snd/timer
 */

#define SND_TIMER_VERSION		SND_PROTOCOL_VERSION(2, 0, 0)

typedef enum _snd_timer_type {
	SND_TIMER_TYPE_NONE = -1,
	SND_TIMER_TYPE_SLAVE = 0,
	SND_TIMER_TYPE_GLOBAL,
	SND_TIMER_TYPE_CARD,
	SND_TIMER_TYPE_PCM
} snd_timer_type_t;

/* slave timer types */
typedef enum _snd_timer_slave_type {
	SND_TIMER_STYPE_NONE = 0,
	SND_TIMER_STYPE_APPLICATION,
	SND_TIMER_STYPE_SEQUENCER,		/* alias */
	SND_TIMER_STYPE_OSS_SEQUENCER		/* alias */
} snd_timer_slave_type_t;

/* global timers (device member) */
typedef enum _snd_timer_global {
	SND_TIMER_GLOBAL_SYSTEM = 0,
	SND_TIMER_GLOBAL_RTC
} snd_timer_global_t;

typedef struct _snd_timer_id {
	snd_timer_type_t type;		/* timer type - SND_TIMER_TYPE_* */
	snd_timer_slave_type_t stype;	/* slave type - SND_TIMER_STYPE_* */
	int card;
	int device;
	int subdevice;
} snd_timer_id_t;

typedef struct _snd_timer_select {
	snd_timer_id_t id;		/* bind to timer ID */
	char reserved[32];
} snd_timer_select_t;

#define SND_TIMER_FLG_SLAVE		(1<<0)	/* cannot be controlled */

typedef struct _snd_timer_info {
	unsigned int flags;		/* timer flags - SND_TIMER_FLG_* */
	char id[64];			/* timer identificator */
	char name[80];			/* timer name */
	unsigned long ticks;		/* maximum ticks */
	unsigned long resolution;	/* average resolution */
	char reserved[64];
} snd_timer_info_t;

#define SND_TIMER_PARBIT_FLAGS		(1<<0)
#define SND_TIMER_PARBIT_TICKS		(1<<1)
#define SND_TIMER_PARBIT_QUEUE_SIZE	(1<<2)

#define SND_TIMER_PSFLG_AUTO		(1<<0)	/* supports auto start */

typedef struct _snd_timer_params {
	unsigned int flags;		/* flags - SND_MIXER_PSFLG_* */
	unsigned long ticks;		/* requested resolution in ticks */
	int queue_size;			/* total size of queue (32-1024) */
	unsigned int fail_mask;		/* failure locations */
	char reserved[64];
} snd_timer_params_t;

typedef struct _snd_timer_status {
	snd_timestamp_t tstamp;		/* Timestamp */
	unsigned long resolution;	/* current resolution */
	unsigned long lost;		/* counter of master tick lost */
	unsigned long overrun;		/* count of read queue overruns */
	int queue;			/* used queue size */
	char reserved[64];
} snd_timer_status_t;

#define SND_TIMER_IOCTL_PVERSION	_IOR ('T', 0x00, int)
#define SND_TIMER_IOCTL_NEXT_DEVICE	_IOWR('T', 0x01, snd_timer_id_t)
#define SND_TIMER_IOCTL_SELECT		_IOW ('T', 0x10, snd_timer_select_t)
#define SND_TIMER_IOCTL_INFO		_IOR ('T', 0x11, snd_timer_info_t)
#define SND_TIMER_IOCTL_PARAMS		_IOW ('T', 0x12, snd_timer_params_t)
#define SND_TIMER_IOCTL_STATUS		_IOW ('T', 0x14, snd_timer_status_t)
#define SND_TIMER_IOCTL_START		_IO  ('T', 0x20)
#define SND_TIMER_IOCTL_STOP		_IO  ('T', 0x21)
#define SND_TIMER_IOCTL_CONTINUE	_IO  ('T', 0x22)

typedef struct _snd_timer_read {
	unsigned long resolution;
	unsigned long ticks;
} snd_timer_read_t;

/*
 *
 */

#define SND_IOCTL_READV		_IOW ('K', 0x00, snd_xferv_t)
#define SND_IOCTL_WRITEV	_IOW ('K', 0x01, snd_xferv_t)

#endif				/* __ASOUND_H */
