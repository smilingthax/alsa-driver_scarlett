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
 */

typedef enum _snd_card_type {
	SND_CARD_TYPE_GUS_CLASSIC,	/* GUS Classic */
	SND_CARD_TYPE_GUS_EXTREME,	/* GUS Extreme */
	SND_CARD_TYPE_GUS_ACE,		/* GUS ACE */
	SND_CARD_TYPE_GUS_MAX,		/* GUS MAX */
	SND_CARD_TYPE_AMD_INTERWAVE,	/* GUS PnP - AMD InterWave */
	SND_CARD_TYPE_SB_10,		/* SoundBlaster v1.0 */
	SND_CARD_TYPE_SB_20,		/* SoundBlaster v2.0 */
	SND_CARD_TYPE_SB_PRO,		/* SoundBlaster Pro */
	SND_CARD_TYPE_SB_16,		/* SoundBlaster 16 */
	SND_CARD_TYPE_SB_AWE,		/* SoundBlaster AWE */
	SND_CARD_TYPE_ESS_ES1688,	/* ESS AudioDrive ESx688 */
	SND_CARD_TYPE_OPL3_SA2,		/* Yamaha OPL3 SA2/SA3 */
	SND_CARD_TYPE_MOZART,		/* OAK Mozart */
	SND_CARD_TYPE_S3_SONICVIBES,	/* S3 SonicVibes */
	SND_CARD_TYPE_ENS1370,		/* Ensoniq ES1370 */
	SND_CARD_TYPE_ENS1371,		/* Ensoniq ES1371 */
	SND_CARD_TYPE_CS4232,		/* CS4232/CS4232A */
	SND_CARD_TYPE_CS4236,		/* CS4235/CS4236B/CS4237B/CS4238B/CS4239 */
	SND_CARD_TYPE_AMD_INTERWAVE_STB,/* AMD InterWave + TEA6330T */
	SND_CARD_TYPE_ESS_ES1938,	/* ESS Solo-1 ES1938 */
	SND_CARD_TYPE_ESS_ES18XX,	/* ESS AudioDrive ES18XX */
	SND_CARD_TYPE_CS4231,		/* CS4231 */
	SND_CARD_TYPE_OPTI92X,		/* OPTi 92x chipset */
	SND_CARD_TYPE_SERIAL,		/* Serial MIDI driver */
	SND_CARD_TYPE_AD1848,		/* Generic AD1848 driver */
	SND_CARD_TYPE_TRID4DWAVEDX,	/* Trident 4DWave DX */
	SND_CARD_TYPE_TRID4DWAVENX,	/* Trident 4DWave NX */
	SND_CARD_TYPE_SGALAXY,		/* Aztech Sound Galaxy */
	SND_CARD_TYPE_CS46XX,		/* Sound Fusion CS4610/12/15 */
	SND_CARD_TYPE_WAVEFRONT,	/* TB WaveFront generic */
	SND_CARD_TYPE_TROPEZ,		/* TB Tropez */
	SND_CARD_TYPE_TROPEZPLUS,	/* TB Tropez+ */
	SND_CARD_TYPE_MAUI,		/* TB Maui */
	SND_CARD_TYPE_CMI8330,		/* C-Media CMI8330 */
	SND_CARD_TYPE_DUMMY,		/* dummy soundcard */
	SND_CARD_TYPE_ALS100,		/* Avance Logic ALS100 */
	SND_CARD_TYPE_SHARE,		/* share soundcard */
	SND_CARD_TYPE_SI_7018,		/* SiS 7018 */
	SND_CARD_TYPE_OPTI93X,		/* OPTi 93x chipset */
	SND_CARD_TYPE_MTPAV,		/* MOTU MidiTimePiece AV multiport MIDI */
	SND_CARD_TYPE_VIRMIDI,		/* Virtual MIDI */
	SND_CARD_TYPE_EMU10K1,		/* EMU10K1 */
	SND_CARD_TYPE_HAMMERFALL,	/* RME Digi9652	 */
	SND_CARD_TYPE_HAMMERFALL_LIGHT,	/* RME Digi9652, but no expansion card */
	SND_CARD_TYPE_ICE1712,		/* ICE1712 */
	SND_CARD_TYPE_CMI8338,		/* C-Media CMI8338 */
	SND_CARD_TYPE_CMI8738,		/* C-Media CMI8738 */
	SND_CARD_TYPE_AD1816A,		/* ADI SoundPort AD1816A */
	SND_CARD_TYPE_INTEL8X0,		/* Intel 810/820/830/840/MX440 */
	SND_CARD_TYPE_ESS_ESOLDM1,	/* Maestro 1 */
	SND_CARD_TYPE_ESS_ES1968,	/* Maestro 2 */
	SND_CARD_TYPE_ESS_ES1978,	/* Maestro 2E */
	SND_CARD_TYPE_DIGI96,		/* RME Digi96 */
	SND_CARD_TYPE_VIA82C686A,	/* VIA 82C686A */
	SND_CARD_TYPE_FM801,		/* FM801 */
	SND_CARD_TYPE_AZT2320,		/* AZT2320 */
	SND_CARD_TYPE_PRODIF_PLUS,	/* Marian/Sek'D Prodif Plus */
	SND_CARD_TYPE_YMFPCI,		/* YMF724/740/744/754 */
	SND_CARD_TYPE_CS4281,		/* CS4281 */
	SND_CARD_TYPE_MPU401_UART,	/* MPU-401 UART */
	SND_CARD_TYPE_ALS4000,		/* Avance Logic ALS4000 */
	SND_CARD_TYPE_ALLEGRO_1,	/* ESS Allegro-1 */
	SND_CARD_TYPE_ALLEGRO,		/* ESS Allegro */
	SND_CARD_TYPE_MAESTRO3,		/* ESS Maestro3 */
	SND_CARD_TYPE_AWACS,		/* PMac AWACS */
	SND_CARD_TYPE_NM256AV,		/* NM256AV */
	SND_CARD_TYPE_NM256ZX,		/* NM256ZX */

	/* Don't forget to change the following: */
	SND_CARD_TYPE_LAST = SND_CARD_TYPE_NM256ZX,
} snd_card_type_t;

typedef struct timeval snd_timestamp_t;

/****************************************************************************
 *                                                                          *
 *        Digital audio interface					    *
 *                                                                          *
 ****************************************************************************/

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

typedef struct _snd_aes_iec958 {
	unsigned char status[24];	/* AES/IEC958 channel status bits */
	unsigned char subcode[147];	/* AES/IEC958 subcode bits */
	unsigned char pad;		/* nothing */
	unsigned char dig_subframe[4];	/* AES/IEC958 subframe bits */
} snd_aes_iec958_t;

typedef union _snd_digital_audio {
	snd_aes_iec958_t aes;
	unsigned char reserved[256];
} snd_digital_audio_t;

/****************************************************************************
 *                                                                          *
 *      Section for driver hardware dependent interface - /dev/snd/hw?      *
 *                                                                          *
 ****************************************************************************/

#define SND_HWDEP_VERSION		SND_PROTOCOL_VERSION(1, 0, 0)

typedef enum _snd_hwdep_type {
	SND_HWDEP_TYPE_OPL2,
	SND_HWDEP_TYPE_OPL3,
	SND_HWDEP_TYPE_OPL4,
	SND_HWDEP_TYPE_SB16CSP,	/* Creative Signal Processor */
	SND_HWDEP_TYPE_EMU10K1,	/* FX8010 processor in EMU10K1 chip */
	SND_HWDEP_TYPE_YSS225,	/* Yamaha FX processor */
	SND_HWDEP_TYPE_ICS2115,	/* Wavetable synth */

	/* Don't forget to change the following: */
	SND_HWDEP_TYPE_LAST = SND_HWDEP_TYPE_ICS2115,
} snd_hwdep_type_t;

typedef struct _snd_hwdep_info {
	int device;		/* WR: device number */
	snd_card_type_t type;	/* type of card - look to SND_CARD_TYPE_XXXX */
	unsigned char id[64];	/* ID of this hardware dependent device (user selectable) */
	unsigned char name[80];	/* name of this hardware dependent device */
	snd_hwdep_type_t hw_type;	/* hardware depedent device type */
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

typedef enum _snd_pcm_class {
	SND_PCM_CLASS_GENERIC,		/* standard mono or stereo device */
	SND_PCM_CLASS_MULTI,		/* multichannel device */
	SND_PCM_CLASS_MODEM,		/* software modem class */
	SND_PCM_CLASS_DIGITIZER,	/* digitizer class */
} snd_pcm_class_t;

typedef enum _snd_pcm_subclass {
	SND_PCM_SUBCLASS_GENERIC_MIX = 0x0001,	/* mono or stereo subdevices are mixed together */
	SND_PCM_SUBCLASS_MULTI_MIX = 0x0001,	/* multichannel subdevices are mixed together */
} snd_pcm_subclass_t;

typedef enum _snd_pcm_stream {
	SND_PCM_STREAM_PLAYBACK,
	SND_PCM_STREAM_CAPTURE,
	SND_PCM_STREAM_LAST,
} snd_pcm_stream_t;

typedef enum _snd_pcm_access {
	SND_PCM_ACCESS_MMAP_INTERLEAVED, /* interleaved mmap */
	SND_PCM_ACCESS_MMAP_NONINTERLEAVED, /* noninterleaved mmap */
	SND_PCM_ACCESS_MMAP_COMPLEX, /* complex mmap */
	SND_PCM_ACCESS_RW_INTERLEAVED, /* readi/writei */
	SND_PCM_ACCESS_RW_NONINTERLEAVED, /* readn/writen */
	SND_PCM_ACCESS_LAST = SND_PCM_ACCESS_RW_NONINTERLEAVED,
} snd_pcm_access_type_t;

typedef enum _snd_pcm_format {
	SND_PCM_FORMAT_S8,
	SND_PCM_FORMAT_U8,
	SND_PCM_FORMAT_S16_LE,
	SND_PCM_FORMAT_S16_BE,
	SND_PCM_FORMAT_U16_LE,
	SND_PCM_FORMAT_U16_BE,
	SND_PCM_FORMAT_S24_LE,	/* low three bytes */
	SND_PCM_FORMAT_S24_BE,	/* low three bytes */
	SND_PCM_FORMAT_U24_LE,	/* low three bytes */
	SND_PCM_FORMAT_U24_BE,	/* low three bytes */
	SND_PCM_FORMAT_S32_LE,
	SND_PCM_FORMAT_S32_BE,
	SND_PCM_FORMAT_U32_LE,
	SND_PCM_FORMAT_U32_BE,
	SND_PCM_FORMAT_FLOAT_LE,	/* 4-byte float, IEEE-754 32-bit */
	SND_PCM_FORMAT_FLOAT_BE,	/* 4-byte float, IEEE-754 32-bit */
	SND_PCM_FORMAT_FLOAT64_LE,	/* 8-byte float, IEEE-754 64-bit */
	SND_PCM_FORMAT_FLOAT64_BE,	/* 8-byte float, IEEE-754 64-bit */
	SND_PCM_FORMAT_IEC958_SUBFRAME_LE,	/* IEC-958 subframe, Little Endian */
	SND_PCM_FORMAT_IEC958_SUBFRAME_BE,	/* IEC-958 subframe, Big Endian */
	SND_PCM_FORMAT_MU_LAW,
	SND_PCM_FORMAT_A_LAW,
	SND_PCM_FORMAT_IMA_ADPCM,
	SND_PCM_FORMAT_MPEG,
	SND_PCM_FORMAT_GSM,
	SND_PCM_FORMAT_SPECIAL = 31,
	SND_PCM_FORMAT_LAST = 31,

#ifdef SND_LITTLE_ENDIAN
	SND_PCM_FORMAT_S16 = SND_PCM_FORMAT_S16_LE,
	SND_PCM_FORMAT_U16 = SND_PCM_FORMAT_U16_LE,
	SND_PCM_FORMAT_S24 = SND_PCM_FORMAT_S24_LE,
	SND_PCM_FORMAT_U24 = SND_PCM_FORMAT_U24_LE,
	SND_PCM_FORMAT_S32 = SND_PCM_FORMAT_S32_LE,
	SND_PCM_FORMAT_U32 = SND_PCM_FORMAT_U32_LE,
	SND_PCM_FORMAT_FLOAT = SND_PCM_FORMAT_FLOAT_LE,
	SND_PCM_FORMAT_FLOAT64 = SND_PCM_FORMAT_FLOAT64_LE,
	SND_PCM_FORMAT_IEC958_SUBFRAME = SND_PCM_FORMAT_IEC958_SUBFRAME_LE,
#endif
#ifdef SND_BIG_ENDIAN
	SND_PCM_FORMAT_S16 = SND_PCM_FORMAT_S16_BE,
	SND_PCM_FORMAT_U16 = SND_PCM_FORMAT_U16_BE,
	SND_PCM_FORMAT_S24 = SND_PCM_FORMAT_S24_BE,
	SND_PCM_FORMAT_U24 = SND_PCM_FORMAT_U24_BE,
	SND_PCM_FORMAT_S32 = SND_PCM_FORMAT_S32_BE,
	SND_PCM_FORMAT_U32 = SND_PCM_FORMAT_U32_BE,
	SND_PCM_FORMAT_FLOAT = SND_PCM_FORMAT_FLOAT_BE,
	SND_PCM_FORMAT_FLOAT64 = SND_PCM_FORMAT_FLOAT64_BE,
	SND_PCM_FORMAT_IEC958_SUBFRAME = SND_PCM_FORMAT_IEC958_SUBFRAME_BE,
#endif
} snd_pcm_format_t;

typedef enum _snd_pcm_subformat {
	SND_PCM_SUBFORMAT_STD,
	SND_PCM_SUBFORMAT_LAST = SND_PCM_SUBFORMAT_STD,
} snd_pcm_subformat_t;

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

typedef enum _snd_pcm_state {
	SND_PCM_STATE_OPEN,	/* stream is open */
	SND_PCM_STATE_SETUP,	/* stream has a setup */
	SND_PCM_STATE_PREPARED,	/* stream is ready to start */
	SND_PCM_STATE_RUNNING,	/* stream is running */
	SND_PCM_STATE_XRUN,	/* stream reached an xrun */
	SND_PCM_STATE_DRAINING,	/* stream is draining */
	SND_PCM_STATE_PAUSED,	/* stream is paused */
	SND_PCM_STATE_LAST = SND_PCM_STATE_PAUSED,
} snd_pcm_state_t;

typedef enum _snd_pcm_mmap_offset {
	SND_PCM_MMAP_OFFSET_DATA = 0x00000000,
	SND_PCM_MMAP_OFFSET_STATUS = 0x80000000,
	SND_PCM_MMAP_OFFSET_CONTROL = 0x81000000,
} snd_pcm_mmap_offset_t;

typedef union _snd_pcm_sync_id {
	unsigned char id[16];
	unsigned short id16[8];
	unsigned int id32[4];
} snd_pcm_sync_id_t;

typedef struct _snd_pcm_info {
	int device;			/* device number */
	snd_pcm_stream_t stream;	/* stream number */
	int subdevice;			/* subdevice number */
        snd_card_type_t type;           /* soundcard type */
	unsigned char id[64];		/* ID of this PCM device (user selectable) */
	unsigned char name[80];		/* name of this device */
	unsigned char subname[32];	/* subdevice name */
	snd_pcm_class_t device_class;	/* SND_PCM_CLASS_* */
	snd_pcm_subclass_t device_subclass;	/* SND_PCM_SUBCLASS_* */
	int subdevices_count;
	int subdevices_avail;
	snd_pcm_sync_id_t sync;		/* hardware synchronization ID */
	unsigned char reserved[64];	/* reserved for future... */
} snd_pcm_info_t;

typedef enum _snd_pcm_hw_param {
	SND_PCM_HW_PARAM_ACCESS,
	SND_PCM_HW_PARAM_FIRST_MASK = SND_PCM_HW_PARAM_ACCESS,
	SND_PCM_HW_PARAM_FORMAT,
	SND_PCM_HW_PARAM_SUBFORMAT,
	SND_PCM_HW_PARAM_LAST_MASK = SND_PCM_HW_PARAM_SUBFORMAT,

	SND_PCM_HW_PARAM_CHANNELS,
	SND_PCM_HW_PARAM_FIRST_INTERVAL = SND_PCM_HW_PARAM_CHANNELS,
	SND_PCM_HW_PARAM_RATE,
	SND_PCM_HW_PARAM_FRAGMENT_LENGTH,
	SND_PCM_HW_PARAM_FRAGMENT_SIZE,
	SND_PCM_HW_PARAM_FRAGMENTS,
	SND_PCM_HW_PARAM_BUFFER_LENGTH,
	SND_PCM_HW_PARAM_BUFFER_SIZE,
	SND_PCM_HW_PARAM_SAMPLE_BITS,
	SND_PCM_HW_PARAM_FRAME_BITS,
	SND_PCM_HW_PARAM_FRAGMENT_BYTES,
	SND_PCM_HW_PARAM_BUFFER_BYTES,
	SND_PCM_HW_PARAM_LAST_INTERVAL = SND_PCM_HW_PARAM_BUFFER_BYTES,
	SND_PCM_HW_PARAM_LAST = SND_PCM_HW_PARAM_LAST_INTERVAL,
} snd_pcm_hw_param_t;

#define SND_PCM_HW_PARAMS_RUNTIME		(1<<0)

typedef struct _interval {
	unsigned int min, max;
	unsigned int openmin:1,
		     openmax:1,
		     real:1,
		     empty:1;
} interval_t;

typedef struct _snd_pcm_hw_params {
	unsigned int flags;
	unsigned int masks[SND_PCM_HW_PARAM_LAST_MASK - 
			   SND_PCM_HW_PARAM_FIRST_MASK + 1];
	interval_t intervals[SND_PCM_HW_PARAM_LAST_INTERVAL -
			     SND_PCM_HW_PARAM_FIRST_INTERVAL + 1];
	unsigned int appl_cmask;
	unsigned int hw_cmask;
	unsigned int info;		/* R: Info flags for returned setup */
	unsigned int msbits;		/* R: used most significant bits */
	unsigned int rate_num;		/* R: rate numerator */
	unsigned int rate_den;		/* R: rate denominator */
	size_t fifo_size;		/* R: chip FIFO size in frames */
	unsigned char reserved[64];
} snd_pcm_hw_params_t;

typedef enum _snd_pcm_start {
	SND_PCM_START_DATA,	/* start when some data are written (playback) or requested (capture) */
	SND_PCM_START_EXPLICIT,	/* start on the go command */
	SND_PCM_START_LAST = SND_PCM_START_EXPLICIT,
} snd_pcm_start_t;

typedef enum _snd_pcm_ready {
	SND_PCM_READY_FRAGMENT,	/* Efficient ready detection */
	SND_PCM_READY_ASAP,	/* Accurate ready detection */
	SND_PCM_READY_LAST = SND_PCM_READY_ASAP,
} snd_pcm_ready_t;

typedef enum _snd_pcm_xrun {
	SND_PCM_XRUN_FRAGMENT,	/* Efficient xrun detection */
	SND_PCM_XRUN_ASAP,	/* Accurate xrun detection */
	SND_PCM_XRUN_NONE,	/* No xrun detection */
	SND_PCM_XRUN_LAST = SND_PCM_XRUN_NONE,
} snd_pcm_xrun_t;

typedef enum _snd_pcm_silence {
	SND_PCM_SILENCE_FRAGMENT,	/* Silencing happens only at interrupt time */
	SND_PCM_SILENCE_ASAP,	/* Silencing can Use also timers */
	SND_PCM_SILENCE_LAST = SND_PCM_SILENCE_ASAP,
} snd_pcm_silence_t;

typedef enum _snd_pcm_tstamp {
	SND_PCM_TSTAMP_NONE,
	SND_PCM_TSTAMP_MMAP,
	SND_PCM_TSTAMP_LAST = SND_PCM_TSTAMP_MMAP,
} snd_pcm_tstamp_t;

typedef struct _snd_pcm_sw_params {
	snd_pcm_start_t start_mode;	/* start mode */
	snd_pcm_ready_t ready_mode;	/* ready detection mode */
	snd_pcm_xrun_t xrun_mode;		/* xrun detection mode */
	snd_pcm_silence_t silence_mode;	/* silence filling mode */
	snd_pcm_tstamp_t tstamp_mode;	/* timestamp mode */
	size_t avail_min;		/* min avail frames for wakeup */
	size_t xfer_align;		/* xfer size need to be a multiple */
	size_t silence_threshold;	/* min distance to noise for silence filling */
	size_t silence_size;		/* silence block size */
	size_t boundary;		/* pointers wrap point */
	unsigned char reserved[64];
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
	unsigned char reserved[64]; /* must be filled with zero */
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

typedef struct _snd_xfern {
	ssize_t result;
	void **bufs;
	size_t frames;
} snd_xfern_t;

enum {
	SND_PCM_IOCTL_PVERSION = _IOR('A', 0x00, int),
	SND_PCM_IOCTL_INFO = _IOR('A', 0x01, snd_pcm_info_t),
	SND_PCM_IOCTL_HW_REFINE = _IOWR('A', 0x10, snd_pcm_hw_params_t),
	SND_PCM_IOCTL_HW_PARAMS = _IOWR('A', 0x11, snd_pcm_hw_params_t),
	SND_PCM_IOCTL_SW_PARAMS = _IOWR('A', 0x12, snd_pcm_sw_params_t),
	SND_PCM_IOCTL_STATUS = _IOR('A', 0x20, snd_pcm_status_t),
	SND_PCM_IOCTL_DELAY = _IOR('A', 0x21, ssize_t),
	SND_PCM_IOCTL_CHANNEL_INFO = _IOR('A', 0x32, snd_pcm_hw_channel_info_t),
	SND_PCM_IOCTL_PREPARE = _IO('A', 0x40),
	SND_PCM_IOCTL_RESET = _IO('A', 0x41),
	SND_PCM_IOCTL_START = _IO('A', 0x42),
	SND_PCM_IOCTL_DROP = _IO('A', 0x43),
	SND_PCM_IOCTL_DRAIN = _IO('A', 0x44),
	SND_PCM_IOCTL_PAUSE = _IOW('A', 0x45, int),
	SND_PCM_IOCTL_REWIND = _IOW('A', 0x46, size_t),
	SND_PCM_IOCTL_WRITEI_FRAMES = _IOW('A', 0x50, snd_xferi_t),
	SND_PCM_IOCTL_READI_FRAMES = _IOR('A', 0x51, snd_xferi_t),
	SND_PCM_IOCTL_WRITEN_FRAMES = _IOW('A', 0x52, snd_xfern_t),
	SND_PCM_IOCTL_READN_FRAMES = _IOR('A', 0x53, snd_xfern_t),
	SND_PCM_IOCTL_LINK = _IOW('A', 0x60, int),
	SND_PCM_IOCTL_UNLINK = _IO('A', 0x61),
};

/* Trick to make alsa-lib/acinclude.m4 happy */
#define SND_PCM_IOCTL_REWIND SND_PCM_IOCTL_REWIND

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

typedef enum _snd_mcmd {
	SND_MCMD_NOTE_OFF = 0x80,
	SND_MCMD_NOTE_ON = 0x90,
	SND_MCMD_NOTE_PRESSURE = 0xa0,
	SND_MCMD_CONTROL = 0xb0,
	SND_MCMD_PGM_CHANGE = 0xc0,
	SND_MCMD_CHANNEL_PRESSURE = 0xd0,
	SND_MCMD_BENDER = 0xe0,

	SND_MCMD_COMMON_SYSEX = 0xf0,
	SND_MCMD_COMMON_MTC_QUARTER = 0xf1,
	SND_MCMD_COMMON_SONG_POS = 0xf2,
	SND_MCMD_COMMON_SONG_SELECT = 0xf3,
	SND_MCMD_COMMON_TUNE_REQUEST = 0xf6,
	SND_MCMD_COMMON_SYSEX_END = 0xf7,
	SND_MCMD_COMMON_CLOCK = 0xf8,
	SND_MCMD_COMMON_START = 0xfa,
	SND_MCMD_COMMON_CONTINUE = 0xfb,
	SND_MCMD_COMMON_STOP = 0xfc,
	SND_MCMD_COMMON_SENSING = 0xfe,
	SND_MCMD_COMMON_RESET = 0xff,
} snd_mcmd_t;

/*
 *  MIDI controllers
 */

typedef enum _snd_mctl {
	SND_MCTL_MSB_BANK = 0x00,
	SND_MCTL_MSB_MODWHEEL = 0x01,
	SND_MCTL_MSB_BREATH = 0x02,
	SND_MCTL_MSB_FOOT = 0x04,
	SND_MCTL_MSB_PORTNAMENTO_TIME = 0x05,
	SND_MCTL_MSB_DATA_ENTRY = 0x06,
	SND_MCTL_MSB_MAIN_VOLUME = 0x07,
	SND_MCTL_MSB_BALANCE = 0x08,
	SND_MCTL_MSB_PAN = 0x0a,
	SND_MCTL_MSB_EXPRESSION = 0x0b,
	SND_MCTL_MSB_EFFECT1 = 0x0c,
	SND_MCTL_MSB_EFFECT2 = 0x0d,
	SND_MCTL_MSB_GENERAL_PURPOSE1 = 0x10,
	SND_MCTL_MSB_GENERAL_PURPOSE2 = 0x11,
	SND_MCTL_MSB_GENERAL_PURPOSE3 = 0x12,
	SND_MCTL_MSB_GENERAL_PURPOSE4 = 0x13,
	SND_MCTL_LSB_BANK = 0x20,
	SND_MCTL_LSB_MODWHEEL = 0x21,
	SND_MCTL_LSB_BREATH = 0x22,
	SND_MCTL_LSB_FOOT = 0x24,
	SND_MCTL_LSB_PORTNAMENTO_TIME = 0x25,
	SND_MCTL_LSB_DATA_ENTRY = 0x26,
	SND_MCTL_LSB_MAIN_VOLUME = 0x27,
	SND_MCTL_LSB_BALANCE = 0x28,
	SND_MCTL_LSB_PAN = 0x2a,
	SND_MCTL_LSB_EXPRESSION = 0x2b,
	SND_MCTL_LSB_EFFECT1 = 0x2c,
	SND_MCTL_LSB_EFFECT2 = 0x2d,
	SND_MCTL_LSB_GENERAL_PURPOSE1 = 0x30,
	SND_MCTL_LSB_GENERAL_PURPOSE2 = 0x31,
	SND_MCTL_LSB_GENERAL_PURPOSE3 = 0x32,
	SND_MCTL_LSB_GENERAL_PURPOSE4 = 0x33,
	SND_MCTL_SUSTAIN = 0x40,
	SND_MCTL_PORTAMENTO = 0x41,
	SND_MCTL_SUSTENUTO = 0x42,
	SND_MCTL_SOFT_PEDAL = 0x43,
	SND_MCTL_LEGATO_FOOTSWITCH = 0x44,
	SND_MCTL_HOLD2 = 0x45,
	SND_MCTL_SC1_SOUND_VARIATION = 0x46,
	SND_MCTL_SC2_TIMBRE = 0x47,
	SND_MCTL_SC3_RELEASE_TIME = 0x48,
	SND_MCTL_SC4_ATTACK_TIME = 0x49,
	SND_MCTL_SC5_BRIGHTNESS = 0x4a,
	SND_MCTL_SC6 = 0x4b,
	SND_MCTL_SC7 = 0x4c,
	SND_MCTL_SC8 = 0x4d,
	SND_MCTL_SC9 = 0x4e,
	SND_MCTL_SC10 = 0x4f,
	SND_MCTL_GENERAL_PURPOSE5 = 0x50,
	SND_MCTL_GENERAL_PURPOSE6 = 0x51,
	SND_MCTL_GENERAL_PURPOSE7 = 0x52,
	SND_MCTL_GENERAL_PURPOSE8 = 0x53,
	SND_MCTL_PORNAMENTO_CONTROL = 0x54,
	SND_MCTL_E1_REVERB_DEPTH = 0x5b,
	SND_MCTL_E2_TREMOLO_DEPTH = 0x5c,
	SND_MCTL_E3_CHORUS_DEPTH = 0x5d,
	SND_MCTL_E4_DETUNE_DEPTH = 0x5e,
	SND_MCTL_E5_PHASER_DEPTH = 0x5f,
	SND_MCTL_DATA_INCREMENT = 0x60,
	SND_MCTL_DATA_DECREMENT = 0x61,
	SND_MCTL_NONREG_PARM_NUM_LSB = 0x62,
	SND_MCTL_NONREG_PARM_NUM_MSB = 0x63,
	SND_MCTL_REGIST_PARM_NUM_LSB = 0x64,
	SND_MCTL_REGIST_PARM_NUM_MSB = 0x65,
	SND_MCTL_ALL_SOUNDS_OFF = 0x78,
	SND_MCTL_RESET_CONTROLLERS = 0x79,
	SND_MCTL_LOCAL_CONTROL_SWITCH = 0x7a,
	SND_MCTL_ALL_NOTES_OFF = 0x7b,
	SND_MCTL_OMNI_OFF = 0x7c,
	SND_MCTL_OMNI_ON = 0x7d,
	SND_MCTL_MONO1 = 0x7e,
	SND_MCTL_MONO2 = 0x7f,
} snd_mctl_t;

/*
 *  Raw MIDI section - /dev/snd/midi??
 */

#define SND_RAWMIDI_VERSION		SND_PROTOCOL_VERSION(2, 0, 0)

typedef enum _snd_rawmidi_stream {
	SND_RAWMIDI_STREAM_OUTPUT,
	SND_RAWMIDI_STREAM_INPUT,
} snd_rawmidi_stream_t;

#define SND_RAWMIDI_INFO_OUTPUT		0x00000001
#define SND_RAWMIDI_INFO_INPUT		0x00000002
#define SND_RAWMIDI_INFO_DUPLEX		0x00000004

typedef struct _snd_rawmidi_info {
	int device;			/* WR: device number */
	int subdevice;			/* RO/WR (control): subdevice number */
	snd_card_type_t type;		/* soundcard type */
	unsigned int flags;		/* SND_RAWMIDI_INFO_XXXX */
	unsigned char id[64];		/* ID of this raw midi device (user selectable) */
	unsigned char name[80];		/* name of this raw midi device */
	unsigned char subname[32];	/* name of active or selected subdevice */
	int output_subdevices_count;
	int output_subdevices_avail;
	int input_subdevices_count;
	int input_subdevices_avail;
	unsigned char reserved[64];	/* reserved for future use */
} snd_rawmidi_info_t;

#define SND_RAWMIDI_PARBIT_STREAM	(1<<0)
#define SND_RAWMIDI_PARBIT_BUFFER_SIZE	(1<<1)
#define SND_RAWMIDI_PARBIT_AVAIL_MIN	(1<<2)

typedef struct _snd_rawmidi_params {
	snd_rawmidi_stream_t stream;		/* Requested stream */
	size_t buffer_size;	/* requested queue size in bytes */
	size_t avail_min;	/* minimum avail bytes for wakeup */
	unsigned int fail_mask;	/* failure locations */
	unsigned int no_active_sensing: 1; /* do not send active sensing byte in close() */
	unsigned char reserved[16]; /* reserved for future use */
} snd_rawmidi_params_t;

typedef struct _snd_rawmidi_status {
	snd_rawmidi_stream_t stream;		/* Requested stream */
	snd_timestamp_t tstamp;	/* Timestamp */
	size_t avail;		/* available bytes */
	size_t xruns;		/* count of overruns since last status (in bytes) */
	unsigned char reserved[16]; /* reserved for future use */
} snd_rawmidi_status_t;

enum {
	SND_RAWMIDI_IOCTL_PVERSION = _IOR('W', 0x00, int),
	SND_RAWMIDI_IOCTL_INFO = _IOR('W', 0x01, snd_rawmidi_info_t),
	SND_RAWMIDI_IOCTL_PARAMS = _IOWR('W', 0x10, snd_rawmidi_params_t),
	SND_RAWMIDI_IOCTL_STATUS = _IOWR('W', 0x20, snd_rawmidi_status_t),
	SND_RAWMIDI_IOCTL_DROP = _IOW('W', 0x30, int),
	SND_RAWMIDI_IOCTL_DRAIN = _IOW('W', 0x31, int),
};

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
	SND_TIMER_STYPE_NONE,
	SND_TIMER_STYPE_APPLICATION,
	SND_TIMER_STYPE_SEQUENCER,		/* alias */
	SND_TIMER_STYPE_OSS_SEQUENCER		/* alias */
} snd_timer_slave_type_t;

/* global timers (device member) */
typedef enum _snd_timer_global {
	SND_TIMER_GLOBAL_SYSTEM,
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
	unsigned char reserved[32];
} snd_timer_select_t;

#define SND_TIMER_FLG_SLAVE		(1<<0)	/* cannot be controlled */

typedef struct _snd_timer_info {
	unsigned int flags;		/* timer flags - SND_TIMER_FLG_* */
	unsigned char id[64];		/* timer identificator */
	unsigned char name[80];		/* timer name */
	unsigned long ticks;		/* maximum ticks */
	unsigned long resolution;	/* average resolution */
	unsigned char reserved[64];
} snd_timer_info_t;

#define SND_TIMER_PARBIT_FLAGS		(1<<0)
#define SND_TIMER_PARBIT_TICKS		(1<<1)
#define SND_TIMER_PARBIT_QUEUE_SIZE	(1<<2)

#define SND_TIMER_PSFLG_AUTO		(1<<0)	/* supports auto start */

typedef struct _snd_timer_params {
	unsigned int flags;		/* flags - SND_MIXER_PSFLG_* */
	size_t ticks;			/* requested resolution in ticks */
	size_t queue_size;		/* total size of queue (32-1024) */
	unsigned int fail_mask;		/* failure locations */
	unsigned char reserved[64];
} snd_timer_params_t;

typedef struct _snd_timer_status {
	snd_timestamp_t tstamp;		/* Timestamp */
	size_t resolution;		/* current resolution */
	size_t lost;			/* counter of master tick lost */
	size_t overrun;			/* count of read queue overruns */
	size_t queue;			/* used queue size */
	unsigned char reserved[64];
} snd_timer_status_t;

enum {
	SND_TIMER_IOCTL_PVERSION = _IOR('T', 0x00, int),
	SND_TIMER_IOCTL_NEXT_DEVICE = _IOWR('T', 0x01, snd_timer_id_t),
	SND_TIMER_IOCTL_SELECT = _IOW('T', 0x10, snd_timer_select_t),
	SND_TIMER_IOCTL_INFO = _IOR('T', 0x11, snd_timer_info_t),
	SND_TIMER_IOCTL_PARAMS = _IOW('T', 0x12, snd_timer_params_t),
	SND_TIMER_IOCTL_STATUS = _IOW('T', 0x14, snd_timer_status_t),
	SND_TIMER_IOCTL_START = _IO('T', 0x20),
	SND_TIMER_IOCTL_STOP = _IO('T', 0x21),
	SND_TIMER_IOCTL_CONTINUE = _IO('T', 0x22),
};

typedef struct _snd_timer_read {
	size_t resolution;
	size_t ticks;
} snd_timer_read_t;

/****************************************************************************
 *                                                                          *
 *        Section for driver control interface - /dev/snd/control?          *
 *                                                                          *
 ****************************************************************************/

#define SND_CTL_VERSION			SND_PROTOCOL_VERSION(2, 0, 0)

typedef struct _snd_ctl_hw_info {
	snd_card_type_t type;		/* type of card - SND_CARD_TYPE_XXXX */
	unsigned char id[16];		/* ID of card (user selectable) */
	unsigned char abbreviation[16];	/* Abbreviation for soundcard */
	unsigned char name[32];		/* Short name of soundcard */
	unsigned char longname[80];	/* name + info text about soundcard */
	unsigned char mixerid[16];	/* ID of mixer */
	unsigned char mixername[80];	/* mixer identification */
	unsigned char reserved[128];	/* reserved for future */
} snd_ctl_hw_info_t;

typedef enum _snd_control_type {
	SND_CONTROL_TYPE_NONE,			/* invalid */
	SND_CONTROL_TYPE_BOOLEAN,		/* boolean type */
	SND_CONTROL_TYPE_INTEGER,		/* integer type */
	SND_CONTROL_TYPE_ENUMERATED,		/* enumerated type */
	SND_CONTROL_TYPE_BYTES,			/* byte array */
	SND_CONTROL_TYPE_IEC958 = 0x1000,	/* IEC958 (S/PDIF) setup */
} snd_control_type_t;

typedef enum _snd_control_iface {
	SND_CONTROL_IFACE_CARD,			/* global control */
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
#define SND_CONTROL_ACCESS_VOLATILE	(1<<2)	/* control value may be changed without a notification */
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
	unsigned char reserved[50];
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
		unsigned char reserved[128];
	} value;
	unsigned char reserved[64];
} snd_control_info_t;

typedef struct _snd_control {
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
		snd_digital_audio_t diga;
        } value;                /* RO */
        unsigned char reserved[128];
} snd_control_t;

enum {
	SND_CTL_IOCTL_PVERSION = _IOR('U', 0x00, int),
	SND_CTL_IOCTL_HW_INFO = _IOR('U', 0x01, snd_ctl_hw_info_t),
	SND_CTL_IOCTL_CONTROL_LIST = _IOWR('U', 0x10, snd_control_list_t),
	SND_CTL_IOCTL_CONTROL_INFO = _IOWR('U', 0x11, snd_control_info_t),
	SND_CTL_IOCTL_CONTROL_READ = _IOWR('U', 0x12, snd_control_t),
	SND_CTL_IOCTL_CONTROL_WRITE = _IOWR('U', 0x13, snd_control_t),
	SND_CTL_IOCTL_CONTROL_LOCK = _IOW('U', 0x14, snd_control_id_t),
	SND_CTL_IOCTL_CONTROL_UNLOCK = _IOW('U', 0x15, snd_control_id_t),
	SND_CTL_IOCTL_HWDEP_NEXT_DEVICE = _IOWR('U', 0x20, int),
	SND_CTL_IOCTL_HWDEP_INFO = _IOR('U', 0x21, snd_hwdep_info_t),
	SND_CTL_IOCTL_PCM_NEXT_DEVICE = _IOR('U', 0x30, int),
	SND_CTL_IOCTL_PCM_INFO = _IOWR('U', 0x31, snd_pcm_info_t),
	SND_CTL_IOCTL_PCM_PREFER_SUBDEVICE = _IOW('U', 0x32, int),
	SND_CTL_IOCTL_RAWMIDI_NEXT_DEVICE = _IOWR('U', 0x40, int),
	SND_CTL_IOCTL_RAWMIDI_INFO = _IOWR('U', 0x41, snd_rawmidi_info_t),
	SND_CTL_IOCTL_RAWMIDI_PREFER_SUBDEVICE = _IOW('U', 0x42, int),
};

/*
 *  Read interface.
 */

typedef enum _snd_ctl_event_type {
	SND_CTL_EVENT_REBUILD,		/* rebuild everything */
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
 *
 */

typedef struct {
	const struct iovec *vector;
	unsigned long count;
} snd_xferv_t;

enum {
	SND_IOCTL_READV = _IOW('K', 0x00, snd_xferv_t),
	SND_IOCTL_WRITEV = _IOW('K', 0x01, snd_xferv_t),
};

#endif				/* __ASOUND_H */
