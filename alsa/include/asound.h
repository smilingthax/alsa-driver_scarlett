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
#include <netinet/in.h>
#else
#include <linux/in.h>
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
 *  hardware independent conversion
 */

#define snd_htoi_32(val) htonl(val)
#define snd_htoi_16(val) htons(val)
#define snd_itoh_32(val) ntohl(val)
#define snd_itoh_16(val) ntohs(val)

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
#define SND_CARD_TYPE_TRID4DWAVEDX	0x0000001A	/* Trident 4DWave DX */
#define SND_CARD_TYPE_TRID4DWAVENX	0x0000001B	/* Trident 4DWave NX */
#define SND_CARD_TYPE_SGALAXY           0x0000001C      /* Aztech Sound Galaxy */
#define SND_CARD_TYPE_CS461X		0x0000001D	/* Sound Fusion CS4610/12/15 */
#define SND_CARD_TYPE_WAVEFRONT         0x0000001E      /* TB WaveFront generic */
#define SND_CARD_TYPE_TROPEZ            0x0000001F      /* TB Tropez */
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

#define SND_CARD_TYPE_LAST		0x0000003a

/*
 *  Universal switch interface
 */

#define SND_SW_TYPE_NONE		0	/* invalid */
#define SND_SW_TYPE_BOOLEAN		1	/* 0 or 1 (enable) */
#define SND_SW_TYPE_BYTE		2	/* 0 to 255 (low to high) */
#define SND_SW_TYPE_WORD		3	/* 0 to 65535 (low to high) */
#define SND_SW_TYPE_DWORD		4	/* 0 to 4294967296 (low to high) */
#define SND_SW_TYPE_LIST		5	/* list type */
#define SND_SW_TYPE_LIST_ITEM		6	/* list item */
#define SND_SW_TYPE_LAST		6	/* last known */
#define SND_SW_TYPE_USER_READ_ONLY	0xfffffffe /* user type - read only */
#define SND_SW_TYPE_USER		0xffffffff /* user type */

#define SND_SW_SUBTYPE_NONE		0	/* ignored */
#define SND_SW_SUBTYPE_HEXA		1	/* hexadecimal value */

#define SND_CTL_IFACE_CONTROL		0
#define SND_CTL_IFACE_MIXER		1
#define SND_CTL_IFACE_PCM		2
#define SND_CTL_IFACE_RAWMIDI		3

typedef struct snd_switch_list_item {
	unsigned char name[32];
} snd_switch_list_item_t;

typedef struct snd_switch_list {
	int iface;			/* WR: switch interface number */
	int device;			/* WR: device number */
	int stream;			/* WR: stream number */
	int switches_size;		/* WR: size of switches in array */
	int switches;			/* RO: filled switches in array */
	int switches_over;		/* RO: missing switches in array */
	snd_switch_list_item_t *pswitches; /* WR: pointer to list item array */
	char reserved[12];
} snd_switch_list_t;

typedef struct snd_switch {
	int iface;		/* WR: interface number */
	int device;		/* WR: device number */
	int stream;		/* WR: stream number */
	unsigned char name[32];	/* WR: unique identification of switch (from driver) */
	unsigned int type;	/* RW: look to SND_SW_TYPE_* */
	unsigned int subtype;	/* RW: look to SND_SW_SUBTYPE_* */
	unsigned int low;	/* RO: low range value */
	unsigned int high;	/* RO: high range value */
	union {
		unsigned int enable: 1;		/* 0 = off, 1 = on */
		unsigned char data8[32];	/* 8-bit data */
		unsigned short data16[16];	/* 16-bit data */
		unsigned int data32[8];		/* 32-bit data */
		unsigned int item_number;	/* active list item number */
		char item[32];			/* list item, low -> item number */
	} value;		/* RO */
	unsigned char reserved[32];
} snd_switch_t;
 
/****************************************************************************
 *                                                                          *
 *        Section for driver control interface - /dev/snd/control?          *
 *                                                                          *
 ****************************************************************************/

#define SND_CTL_VERSION			SND_PROTOCOL_VERSION(1, 0, 0)

#define SND_CTL_SW_JOYSTICK		"Joystick"
#define SND_CTL_SW_JOYSTICK_ADDRESS	"Joystick Address"
#define SND_CTL_SW_JOYSTICK_SPEED	"Joystick Speed Compensation"

typedef struct snd_ctl_hw_info {
	unsigned int type;	/* type of card - look to SND_CARD_TYPE_XXXX */
	unsigned int hwdepdevs;	/* count of hardware dependent devices (0 to A) */
	unsigned int pcmdevs;	/* count of PCM devices */
	unsigned int mixerdevs;	/* count of MIXER devices */
	unsigned int mididevs;	/* count of raw MIDI devices */
	unsigned int timerdevs;	/* count of timer devices */
	char id[16];		/* ID of card (user selectable) */
	char abbreviation[16];	/* Abbreviation for soundcard */
	char name[32];		/* Short name of soundcard */
	char longname[80];	/* name + info text about soundcard */
	unsigned char reserved[128];	/* reserved for future */
} snd_ctl_hw_info_t;

#define SND_CTL_IOCTL_PVERSION		_IOR ('U', 0x00, int)
#define SND_CTL_IOCTL_HW_INFO		_IOR ('U', 0x01, snd_ctl_hw_info_t)
#define SND_CTL_IOCTL_SWITCH_LIST	_IOWR('U', 0x02, snd_switch_list_t)
#define SND_CTL_IOCTL_SWITCH_READ	_IOWR('U', 0x03, snd_switch_t)
#define SND_CTL_IOCTL_SWITCH_WRITE	_IOWR('U', 0x04, snd_switch_t)
#define SND_CTL_IOCTL_HWDEP_DEVICE	_IOW ('U', 0x08, int)
#define SND_CTL_IOCTL_HWDEP_INFO	_IOR ('U', 0x09, snd_hwdep_info_t)
#define SND_CTL_IOCTL_MIXER_DEVICE	_IOW ('U', 0x10, int)
#define SND_CTL_IOCTL_MIXER_INFO	_IOR ('U', 0x10, snd_mixer_info_t)
#define SND_CTL_IOCTL_PCM_DEVICE	_IOW ('U', 0x20, int)
#define SND_CTL_IOCTL_PCM_STREAM	_IOW ('U', 0x21, int)
#define SND_CTL_IOCTL_PCM_SUBDEVICE	_IOW ('U', 0x22, int)
#define SND_CTL_IOCTL_PCM_PREFER_SUBDEVICE _IOW('U', 0x23, int)
#define SND_CTL_IOCTL_PCM_INFO		_IOR ('U', 0x24, snd_pcm_info_t)
#define SND_CTL_IOCTL_PCM_STREAM_INFO	_IOR ('U', 0x25, snd_pcm_stream_info_t)
#define SND_CTL_IOCTL_RAWMIDI_DEVICE	_IOW ('U', 0x30, int)
#define SND_CTL_IOCTL_RAWMIDI_STREAM	_IOW ('U', 0x31, int)
#define SND_CTL_IOCTL_RAWMIDI_INFO	_IOR ('U', 0x32, snd_rawmidi_info_t)

/*
 *  Read interface.
 */

#define SND_CTL_READ_REBUILD		0	/* rebuild the whole structure */
#define SND_CTL_READ_SWITCH_VALUE	1	/* the switch value was changed */
#define SND_CTL_READ_SWITCH_CHANGE	2	/* the switch was changed */
#define SND_CTL_READ_SWITCH_ADD		3	/* the switch was added */
#define SND_CTL_READ_SWITCH_REMOVE	4	/* the switch was removed */

typedef struct snd_ctl_read {
	unsigned int cmd;		/* command - SND_MIXER_READ_* */
	union {
		struct {
			int iface;	/* interface */
			int device;	/* device */
			int stream;	/* stream */
			snd_switch_list_item_t switem; /* switch item */
		} sw;
		unsigned char data8[60];
	} data;
} snd_ctl_read_t;

/****************************************************************************
 *                                                                          *
 *        Section for driver control interface - /dev/snd/control?          *
 *                                                                          *
 ****************************************************************************/

#define SND_HWDEP_VERSION		SND_PROTOCOL_VERSION(1, 0, 0)

#define SND_HWDEP_TYPE_OPL2		0
#define SND_HWDEP_TYPE_OPL3		1
#define SND_HWDEP_TYPE_OPL4		2
#define SND_HWDEP_TYPE_SB16CSP		3	/* Creative Signal Processor */
#define SND_HWDEP_TYPE_EMU8000		4
#define SND_HWDEP_TYPE_YSS225		5	/* Yamaha FX processor */
#define SND_HWDEP_TYPE_ICS2115		6	/* Wavetable synth */
/* --- */
#define SND_HWDEP_TYPE_LAST		6

typedef struct snd_hwdep_info {
	unsigned int type;	/* type of card - look to SND_CARD_TYPE_XXXX */
	unsigned char id[64];	/* ID of this hardware dependent device (user selectable) */
	unsigned char name[80];	/* name of this hardware dependent device */
	unsigned int hw_type;	/* hardware depedent device type */
	unsigned char reserved[64];	/* reserved for future */
} snd_hwdep_info_t;

#define SND_HWDEP_IOCTL_PVERSION	_IOR ('H', 0x00, int)
#define SND_HWDEP_IOCTL_INFO		_IOR ('H', 0x01, snd_hwdep_info_t)

/****************************************************************************
 *                                                                          *
 *                  MIXER interface - /dev/snd/mixer??                      *
 *                                                                          *
 ****************************************************************************/

#define SND_MIXER_VERSION		SND_PROTOCOL_VERSION(2, 0, 0)

/* inputs */				/* max 24 chars */
#define SND_MIXER_IN_SYNTHESIZER	"Synth"
#define SND_MIXER_IN_PCM		"PCM"
#define SND_MIXER_IN_DAC		"DAC"
#define SND_MIXER_IN_FM			"FM"
#define SND_MIXER_IN_DSP		"DSP Input"
#define SND_MIXER_IN_LINE		"Line"
#define SND_MIXER_IN_MIC		"MIC"
#define SND_MIXER_IN_CD			"CD"
#define SND_MIXER_IN_VIDEO		"Video"
#define SND_MIXER_IN_RADIO		"Radio"
#define SND_MIXER_IN_PHONE		"Phone"
#define SND_MIXER_IN_MONO		"Mono"
#define SND_MIXER_IN_SPEAKER		"PC Speaker"
#define SND_MIXER_IN_AUX		"Aux"
#define SND_MIXER_IN_CENTER		"Center Input"
#define SND_MIXER_IN_WOOFER		"Woofer Input"
#define SND_MIXER_IN_SURROUND		"Surround Input"

/* outputs */				/* max 24 chars */
#define SND_MIXER_OUT_MASTER		"Master"
#define SND_MIXER_OUT_MASTER_MONO	"Master Mono"
#define SND_MIXER_OUT_MASTER_DIGITAL	"Master Digital"
#define SND_MIXER_OUT_HEADPHONE		"Headphone"
#define SND_MIXER_OUT_PHONE		"Phone Output"
#define SND_MIXER_OUT_CENTER		"Center"
#define SND_MIXER_OUT_WOOFER		"Woofer"
#define SND_MIXER_OUT_SURROUND		"Surround"
#define SND_MIXER_OUT_DSP		"DSP Output"

/* groups */				/* max 24 chars */
#define SND_MIXER_GRP_BASS		"Bass"
#define SND_MIXER_GRP_TREBLE		"Treble"
#define SND_MIXER_GRP_EQUALIZER		"Equalizer"
#define SND_MIXER_GRP_FADER		"Fader"
#define SND_MIXER_GRP_EFFECT		"Effect"
#define SND_MIXER_GRP_EFFECT_3D		"3D Effect"
#define SND_MIXER_GRP_MIC_GAIN		"Mic Gain"
#define SND_MIXER_GRP_IGAIN		"Input Gain"
#define SND_MIXER_GRP_OGAIN		"Output Gain"
#define SND_MIXER_GRP_ANALOG_LOOPBACK	"Analog Loopback"
#define SND_MIXER_GRP_DIGITAL_LOOPBACK	"Digital Loopback"

/* others */
#define SND_MIXER_ELEMENT_TONE_CONTROL	"Tone Control"
#define SND_MIXER_ELEMENT_INPUT_MUX	"Input MUX"
#define SND_MIXER_ELEMENT_DIGITAL_ACCU	"Digital Accumulator"
#define SND_MIXER_ELEMENT_OUTPUT_ACCU	"Output Accumulator"
#define SND_MIXER_ELEMENT_INPUT_ACCU	"Input Accumulator"
#define SND_MIXER_ELEMENT_MONO_OUT_ACCU	"Mono-Out Accumulator"
#define SND_MIXER_ELEMENT_MONO_IN_ACCU	"Mono-In Accumulator"
#define SND_MIXER_ELEMENT_DAC		"Digital-Analog Converter"
#define SND_MIXER_ELEMENT_ADC		"Analog-Digital Converter"
#define SND_MIXER_ELEMENT_CAPTURE	"Capture"
#define SND_MIXER_ELEMENT_PLAYBACK	"Playback"

/* switches */
#define SND_MIXER_SW_IEC958_OUTPUT	"IEC-958 (S/PDIF) Output"
#define SND_MIXER_SW_IEC958_INPUT	"IEC-958 (S/PDIF) Input"
#define SND_MIXER_SW_SIM_STEREO		"Simulated Stereo Enhancement"
#define SND_MIXER_SW_LOUDNESS		"Loudness (bass boost)"

/*
 *  element types
 */

/* input */
#define SND_MIXER_ETYPE_INPUT		0
/* output */
#define SND_MIXER_ETYPE_OUTPUT		1
/* capture stream endpoint */
#define SND_MIXER_ETYPE_CAPTURE1	2
/* capture substream endpoint */
#define SND_MIXER_ETYPE_CAPTURE2	3
/* playback stream startpoint */
#define SND_MIXER_ETYPE_PLAYBACK1	4
/* playback substream startpoint */
#define SND_MIXER_ETYPE_PLAYBACK2	5
/* ADC */
#define SND_MIXER_ETYPE_ADC		6
/* DAC */
#define SND_MIXER_ETYPE_DAC		7
/* capture substream/channel endpoint */
#define SND_MIXER_ETYPE_CAPTURE3	8
/* playback substream/channel endpoint */
#define SND_MIXER_ETYPE_PLAYBACK3	9
/* simple on/off switch (channels separated) */
#define SND_MIXER_ETYPE_SWITCH1		100
/* simple on/off switch (all channels together) */
#define SND_MIXER_ETYPE_SWITCH2		101
/* simple channel route switch */
#define SND_MIXER_ETYPE_SWITCH3		102
/* simple volume control */
#define SND_MIXER_ETYPE_VOLUME1		200
/* simple volume control - PCM channels to DAC */
#define SND_MIXER_ETYPE_VOLUME2		201
/* simple accumulator */
#define SND_MIXER_ETYPE_ACCU1		300
/* simple accumulator with MONO output */
#define SND_MIXER_ETYPE_ACCU2		301
/* simple accumulator with programmable attenuation */
#define SND_MIXER_ETYPE_ACCU3		302
/* simple MUX (channels separated) */
#define SND_MIXER_ETYPE_MUX1		400
/* simple MUX (all channels together) */
#define SND_MIXER_ETYPE_MUX2		401
/* simple tone control */
#define SND_MIXER_ETYPE_TONE_CONTROL1	500
/* equalizer */
#define SND_MIXER_ETYPE_EQUALIZER1	501
/* simple pan control */
#define SND_MIXER_ETYPE_PAN_CONTROL1	502
/* simple 3D effect */
#define SND_MIXER_ETYPE_3D_EFFECT1	600
/* predefined effect */
#define SND_MIXER_ETYPE_PRE_EFFECT1	610

/*
 *  channel definitions
 */

#define SND_MIXER_CHANNEL_UNKNOWN		0
#define SND_MIXER_CHANNEL_MONO		1
#define SND_MIXER_CHANNEL_LEFT		2
#define SND_MIXER_CHANNEL_RIGHT		3
#define SND_MIXER_CHANNEL_CENTER		4
#define SND_MIXER_CHANNEL_REAR_LEFT	5
#define SND_MIXER_CHANNEL_REAR_RIGHT	6
#define SND_MIXER_CHANNEL_WOOFER		7

#define SND_MIXER_CHANNEL_MASK_MONO	(1 << (SND_MIXER_CHANNEL_MONO-1))
#define SND_MIXER_CHANNEL_MASK_LEFT	(1 << (SND_MIXER_CHANNEL_LEFT-1))
#define SND_MIXER_CHANNEL_MASK_RIGHT	(1 << (SND_MIXER_CHANNEL_RIGHT-1))
#define SND_MIXER_CHANNEL_MASK_CENTER	(1 << (SND_MIXER_CHANNEL_CENTER-1))
#define SND_MIXER_CHANNEL_MASK_REAR_LEFT	(1 << (SND_MIXER_CHANNEL_REAR_LEFT-1))
#define SND_MIXER_CHANNEL_MASK_REAR_RIGHT	(1 << (SND_MIXER_CHANNEL_REAR_RIGHT-1))
#define SND_MIXER_CHANNEL_MASK_WOOFER	(1 << (SND_MIXER_CHANNEL_WOOFER-1))

#define SND_MIXER_CHANNEL_MASK_FOUR	(SND_MIXER_CHANNEL_MASK_LEFT|\
					 SND_MIXER_CHANNEL_MASK_RIGHT|\
					 SND_MIXER_CHANNEL_MASK_REAR_LEFT|\
					 SND_MIXER_CHANNEL_MASK_REAR_RIGHT)
#define SND_MIXER_CHANNEL_MASK_SIX	(SND_MIXER_CHANNEL_MASK_LEFT|\
					 SND_MIXER_CHANNEL_MASK_RIGHT|\
					 SND_MIXER_CHANNEL_MASK_CENTER|\
					 SND_MIXER_CHANNEL_MASK_REAR_LEFT|\
					 SND_MIXER_CHANNEL_MASK_REAR_RIGHT|\
					 SND_MIXER_CHANNEL_MASK_WOOFER)

typedef struct {
	unsigned short position;	/* SND_MIXER_CHANNEL_* */
} snd_mixer_channel_t;

typedef struct {
	unsigned char name[24];
	int index;			/* 0..N */
	int type;			/* element type - SND_MIXER_ETYPE_ */
} snd_mixer_eid_t;

typedef struct {
	unsigned char name[24];
	int index;			/* 0..N */
	int reserved;
} snd_mixer_gid_t;

/*
 *  General information.
 */

typedef struct snd_mixer_info {
	unsigned int type;	/* type of soundcard - SND_CARD_TYPE_XXXX */
	unsigned int attrib;	/* attributes, not used */
	unsigned char id[64];	/* ID of this mixer */
	unsigned char name[80];	/* name of this device */
	int elements;		/* count of elements */
	int groups;		/* count of element groups */
	char reserved[32];	/* reserved for future use */
} snd_mixer_info_t;

/*
 *  Element information.
 */

typedef struct snd_mixer_elements {
	int elements_size;	/* WR: size in element identifiers */
	int elements;		/* WR: count of filled element identifiers */
	int elements_over;	/* RO: missing element identifiers */
	snd_mixer_eid_t *pelements; /* WR: array */
} snd_mixer_elements_t;

/*
 *  Route information.
 */

typedef struct snd_mixer_route {
	snd_mixer_eid_t src_eid; /* WR: source element identification */
	snd_mixer_eid_t dst_eid; /* WR: destination element identification */
	int src_channels;		/* RO: source channels */
	int dst_channels;		/* RO: destination channels */
	unsigned int *channel_wires; /* WR: source * destination bitmap matrix */
} snd_mixer_route_t;

typedef struct snd_mixer_routes {
	snd_mixer_eid_t eid;	/* WR: element identifier */
	int routes_size;	/* WR: size in element identifiers */
	int routes;		/* RO: count of filled element identifiers */
	int routes_over;	/* RO: missing element identifiers */
	snd_mixer_eid_t *proutes; /* WR: array */
} snd_mixer_routes_t;

/*
 *  Group information.
 */

typedef struct snd_mixer_groups {
	int groups_size;	/* WR: size in group identifiers */
	int groups;		/* RO: count of filled group identifiers */
	int groups_over;	/* RO: missing group identifiers */
	snd_mixer_gid_t *pgroups; /* WR: array */
} snd_mixer_groups_t;

typedef enum {
	SND_MIXER_CHN_FRONT_LEFT,
	SND_MIXER_CHN_FRONT_RIGHT,
	SND_MIXER_CHN_FRONT_CENTER,
	SND_MIXER_CHN_REAR_LEFT,
	SND_MIXER_CHN_REAR_RIGHT,
	SND_MIXER_CHN_WOOFER,
	SND_MIXER_CHN_LAST = 31,
	SND_MIXER_CHN_MONO = SND_MIXER_CHN_FRONT_LEFT
} snd_mixer_channel_id_t;

#define SND_MIXER_CHN_MASK_MONO		(1<<SND_MIXER_CHN_MONO)
#define SND_MIXER_CHN_MASK_FRONT_LEFT	(1<<SND_MIXER_CHN_FRONT_LEFT)
#define SND_MIXER_CHN_MASK_FRONT_RIGHT	(1<<SND_MIXER_CHN_FRONT_RIGHT)
#define SND_MIXER_CHN_MASK_FRONT_CENTER	(1<<SND_MIXER_CHN_FRONT_CENTER)
#define SND_MIXER_CHN_MASK_REAR_LEFT	(1<<SND_MIXER_CHN_REAR_LEFT)
#define SND_MIXER_CHN_MASK_REAR_RIGHT	(1<<SND_MIXER_CHN_REAR_RIGHT)
#define SND_MIXER_CHN_MASK_WOOFER	(1<<SND_MIXER_CHN_WOOFER)
#define SND_MIXER_CHN_MASK_STEREO	(SND_MIXER_CHN_MASK_FRONT_LEFT|SND_MIXER_CHN_MASK_FRONT_RIGHT)

#define SND_MIXER_GRPCAP_VOLUME		(1<<0)
#define SND_MIXER_GRPCAP_JOINTLY_VOLUME	(1<<1)
#define SND_MIXER_GRPCAP_MUTE		(1<<2)
#define SND_MIXER_GRPCAP_JOINTLY_MUTE	(1<<3)
#define SND_MIXER_GRPCAP_CAPTURE	(1<<4)
#define SND_MIXER_GRPCAP_JOINTLY_CAPTURE (1<<5)
#define SND_MIXER_GRPCAP_EXCL_CAPTURE	(1<<6)

typedef struct snd_mixer_group {
	snd_mixer_gid_t gid;		/* WR: group identification */
	int elements_size;		/* WR: size in element identifiers */
	int elements;			/* RO: count of filled element identifiers */
	int elements_over;		/* RO: missing element identifiers */
	snd_mixer_eid_t *pelements;	/* WR: array */
	unsigned int caps;		/* RO: capabilities */
	unsigned int channels;		/* RO: bitmap of active channels */	
	unsigned int mute;		/* RW: bitmap of muted channels */
	unsigned int capture;		/* RW: bitmap of capture channels */
	int capture_group;		/* RO: capture group (for exclusive capture source) */
	int min;			/* RO: minimum value */
	int max;			/* RO: maximum value */
	union {
		struct {
			int front_left;		/* front left value */
			int front_right;	/* front right value */
			int front_center;	/* front center */
			int rear_left;		/* left rear */
			int rear_right;		/* right rear */
			int woofer;		/* woofer */
		} names;
		int values[32];
	} volume;			/* RW */
} snd_mixer_group_t;

/*
 *  INPUT/OUTPUT - read only
 *
 *    The input element describes input channels.
 */

#define SND_MIXER_EIO_DIGITAL		(0<<1)

struct snd_mixer_element_io_info {
	unsigned int attrib;		/* RO: SND_MIXER_EIO_* */
	int channels_size;		/* WR: size in channel descriptors */
	int channels;			/* RO: count of filled channel descriptors */
	int channels_over;		/* RO: missing channel descriptors */
	snd_mixer_channel_t *pchannels;	/* WR: array */
};

/*
 *  PCM CAPTURE/PLAYBACK - read only
 *
 *    The element controls a playback or capture endpoint.
 */

struct snd_mixer_element_pcm1_info {
	int devices_size;		/* WR: size in device descriptors */
	int devices;			/* RO: count of filled device descriptors */
	int devices_over;		/* RO: missing device descriptors */
	int *pdevices;			/* WR: PCM devices - array */
};

struct snd_mixer_element_pcm2_info {
	int device;			/* RO: device index */
	int subdevice;			/* RO: subdevice index */
};

struct snd_mixer_element_pcm3_info {
	int device;			/* RO: device index */
	int subdevice;			/* RO: subdevice index */
	int channel;			/* RO: channel index */
};

/*
 *  ADC/DAC - read only
 *
 *    The element controls an analog-digital/digital-analog converter.
 */

struct snd_mixer_element_converter_info {
	unsigned int resolution;	/* RO: resolution in bits (usually 16) */
};

/*
 *  Simple on/off switch - read write
 *
 *    This switch turns on or off the channel route for a group of channels.
 */

struct snd_mixer_element_switch1 {
	int sw_size;			/* WR: size of bitmap (in bits) */
	int sw;				/* RO: count of filled bits */
	int sw_over;			/* RO: missing bits */
	unsigned int *psw;		/* WR: bitmap!!! */
};

/*
 *  Simple on/off switch for each channels - read write
 *
 *    This switch turns on or off the channel route for group of channels.
 */

struct snd_mixer_element_switch2 {
	unsigned int sw:1;		/* RW */
};

/*
 *  Simple channel route switch - read write
 *
 *    This switch determines route from input channels to output channels.
 */

#define SND_MIXER_SWITCH3_FULL_FEATURED			0
#define SND_MIXER_SWITCH3_ALWAYS_DESTINATION		1
#define SND_MIXER_SWITCH3_ONE_DESTINATION		2
#define SND_MIXER_SWITCH3_ALWAYS_ONE_DESTINATION	3

struct snd_mixer_element_switch3_info {
	unsigned int type;		/* RO: SND_MIXER_SWITCH3_* */
};

struct snd_mixer_element_switch3 {
	/* two dimensional matrix of channel route switch */
	int rsw_size;			/* WR: size in channel route descriptors (must be input_channels * output_channels bits !!!) */
	int rsw;			/* RO: count of filled channel route descriptors */
	int rsw_over;			/* RO: missing channel descriptors */
	unsigned int *prsw;		/* WR: array */
};

/*
 *  Volume (attenuation/gain) control - read write
 *
 *    The volume must be always linear!!!
 */

struct snd_mixer_element_volume1_range {
	int min, max;		/* RO: linear volume */
	int min_dB, max_dB;	/* RO: negative - attenuation, positive - amplification */
};

struct snd_mixer_element_volume1_info {
	int range_size;		/* WR: size of range descriptors */
	int range;		/* RO: count of filled range descriptors */
	int range_over;		/* RO: missing range descriptors */
	struct snd_mixer_element_volume1_range *prange;	/* WR: array */
};

struct snd_mixer_element_volume1 {
	int channels_size;	/* WR: size of channel descriptors */	
	int channels;		/* RO: count of filled channel descriptors */
	int channels_over;	/* RO: missing channel descriptors */
	int *pchannels;		/* WR: array of volumes */
};

/*
 *  Volume (balance) control - read write
 *
 *    The volume must be always linear!!!
 */

struct snd_mixer_element_volume2_range {
	int min, max;		/* RO: linear volume */
	int min_dB, max_dB;	/* RO: negative - attenuation, positive - amplification */
};

struct snd_mixer_element_volume2_info {
	/* source channels */
	int schannels_size;	/* WR: size of source channels */
	int schannels;		/* RO: count of filled channel descriptors */
	int schannels_over;	/* RO: missing channel descriptors */
	snd_mixer_channel_t *pschannels; /* WR: array of channels */
	/* destination ranges */
	int range_size;		/* WR: size of range descriptors */
	int range;		/* RO: count of filled range descriptors */
	int range_over;		/* RO: missing range descriptors */
	struct snd_mixer_element_volume2_range *prange;	/* WR: array */
};

/* achannels means the array of channels which describes volume offsets for */
/* each outputs, the size of this array is info->schannels * info->range */

struct snd_mixer_element_volume2 {
	int achannels_size;	/* WR: size of channel descriptors */	
	int achannels;		/* RO: count of filled channel descriptors */
	int achannels_over;	/* RO: missing channel descriptors */
	int *pachannels;		/* WR: array of volumes */
};

/*
 *  Simple accumulator
 */

struct snd_mixer_element_accu1_info {
	int attenuation;	/* RO: in dB */
};

/*
 *  Simple accumulator with the MONO output
 */

struct snd_mixer_element_accu2_info {
	int attenuation;	/* RO: in dB */
};

/* 
 *  Simple accumulator with programmable attenuation
 */

struct snd_mixer_element_accu3_range {
	int min, max;		/* RO: linear volume */
	int min_dB, max_dB;	/* RO: negative - attenuation, positive - amplification */
};

struct snd_mixer_element_accu3_info {
	int range_size;		/* WR: size of range descriptors */
	int range;		/* RO: count of filled range descriptors */
	int range_over;		/* RO: missing range descriptors */
	struct snd_mixer_element_accu3_range *prange;	/* WR: array */
};

struct snd_mixer_element_accu3 {
	int channels_size;	/* WR: size of channel descriptors */	
	int channels;		/* RO: count of filled channel descriptors */
	int channels_over;	/* RO: missing channel descriptors */
	int *pchannels;		/* WR: array of volumes */
};

/*
 *  Simple MUX
 *
 *     This mux allows selection of exactly one (or none - optional) source
 *     per each channel.
 */

#define SND_MIXER_MUX1_NONE		(1<<0)

struct snd_mixer_element_mux1_info {
	unsigned int attrib;	/* RO: SND_MIXER_MUX1_ */
};

struct snd_mixer_element_mux1 {
	int sel_size;		/* WR: size of mixer elements */
	int sel;		/* RO: count of mixer elements */
	int sel_over;		/* RO: missing channel elements */
	snd_mixer_eid_t *psel;	/* WR: selected source on element output */
};

/*
 *  Simple MUX
 *
 *     This mux allows selection of exactly one (or none - optional) source.
 */

#define SND_MIXER_MUX2_NONE		(1<<0)

struct snd_mixer_element_mux2_info {
	unsigned int attrib;	/* RO: SND_MIXER_MUX1_ */
};

struct snd_mixer_element_mux2 {
	snd_mixer_eid_t sel;	/* RO: selected input source on element output */
};

/*
 *  Simple tone control
 */

#define SND_MIXER_TC1_SW		(1<<0)
#define SND_MIXER_TC1_BASS		(1<<1)
#define SND_MIXER_TC1_TREBLE		(1<<2)

struct snd_mixer_element_tone_control1_info {
	unsigned int tc;		/* RO: bitmap of SND_MIXER_TC_* */
	int min_bass, max_bass;		/* RO: Bass */
	int min_bass_dB, max_bass_dB;	/* RO: in decibels * 100 */
	int min_treble, max_treble;	/* RO: Treble */
	int min_treble_dB, max_treble_dB; /* RO: in decibels * 100 */
};

struct snd_mixer_element_tone_control1 {
	unsigned int tc;		/* WR: bitmap of SND_MIXER_TC_* */
	unsigned int sw:1;		/* WR: on/off switch */
	int bass;			/* WR: Bass control */
	int treble;			/* WR: Treble control */
};

/*
 *  Simple pan control
 */

#define SND_MIXER_PAN_LEFT_RIGHT        0
#define SND_MIXER_PAN_FRONT_REAR        1
#define SND_MIXER_PAN_BOTTOM_UP         2

struct snd_mixer_element_pan_control1_range {
	int pan_type;		/* RO: SND_MIXER_PAN_XXXX */
	int min, max;		/* RO: min & max range */
	int min_dB, max_dB;	/* RO: min dB & max dB range */
};

struct snd_mixer_element_pan_control1_info {
	int range_size;		/* WR: size of range descriptors */
	int range;		/* RO: count of filled range descriptors */
	int range_over;		/* RO: missing range descriptors */
	struct snd_mixer_element_pan_control1_range *prange; /* WR: array */
};

struct snd_mixer_element_pan_control1 {
	int pan_size;		/* WR: size of pan descriptors */
	int pan;		/* RO: count of filled pan descriptors */
	int pan_over;		/* RO: missing pan descriptors */
	int *ppan;		/* WR: array of pan values */
};

/*
 *  Equalizer
 */

/* TODO */

/*
 *  Simple 3D Effect
 */

#define SND_MIXER_EFF1_SW		(1<<0)
#define SND_MIXER_EFF1_MONO_SW		(1<<1)
#define SND_MIXER_EFF1_WIDE		(1<<2)
#define SND_MIXER_EFF1_VOLUME		(1<<3)
#define SND_MIXER_EFF1_CENTER		(1<<4)
#define SND_MIXER_EFF1_SPACE		(1<<5)
#define SND_MIXER_EFF1_DEPTH		(1<<6)
#define SND_MIXER_EFF1_DELAY		(1<<7)
#define SND_MIXER_EFF1_FEEDBACK		(1<<8)

#define SND_MIXER_EFF1_DEPTH_REAR	(1<<9)

struct snd_mixer_element_3d_effect1_info {
	unsigned int effect;		/* RO: bitmap of SND_MIXER_EFF1_* */
	int min_wide, max_wide;		/* RO: 3D wide */
	int min_volume, max_volume;	/* RO: 3D volume */
	int min_center, max_center;	/* RO: 3D center */
	int min_space, max_space;	/* RO: 3D space */
	int min_depth, max_depth;	/* RO: 3D depth */
	int min_delay, max_delay;	/* RO: 3D delay */
	int min_feedback, max_feedback;	/* RO: 3D feedback */
	int min_depth_rear, max_depth_rear;	/* RO: 3D depth rear */
};

struct snd_mixer_element_3d_effect1 {
	unsigned int effect;		/* RW: bitmap of SND_MIXER_EFF1_* */
	unsigned int sw:1,		/* RW: on/off switch */
		     mono_sw:1;		/* RW: on/off switch */
	int wide;			/* RW: 3D wide */
	int volume;			/* RW: 3D volume */
	int center;			/* RW: 3D center */
	int space;			/* RW: 3D space */
	int depth;			/* RW: 3D depth */
	int delay;			/* RW: 3D delay */
	int feedback;			/* RW: 3D feedback */
	int depth_rear;			/* RW: 3D depth rear */
};

/*
 *  Simple predefined effect
 */

struct snd_mixer_element_pre_effect1_info_item {
	char name[32];
};

struct snd_mixer_element_pre_effect1_info_parameter {
	char name[32];			/* RO: parameter name */
	int min, max;			/* RO: minimum and maximum value */
};

struct snd_mixer_element_pre_effect1_info {
	/* predefined programs */
	int items_size;			/* WR: size of programs */
	int items;			/* RO: count of filled programs */
	int items_over;			/* RO: missing programs */
	struct snd_mixer_element_pre_effect1_info_item *pitems; /* WR: array */
	/* user parameters */
	int parameters_size;		/* WR: size of parameters */
	int parameters;			/* RO: count of filled parameters */
	int parameters_over;		/* RO: missing parameters */
	struct snd_mixer_element_pre_effect1_info_parameter *pparameters; /* WR: array */
};

struct snd_mixer_element_pre_effect1 {
	int item;			/* WR: chose item index or -1 = user parameters */
	int parameters_size;		/* WR: size of parameters */
	int parameters;			/* RO: count of filled parameters */
	int parameters_over;		/* RO: missing parameters */
	int *pparameters;		/* WR: array */
};

/*
 *
 */

typedef struct snd_mixer_element_info {
	snd_mixer_eid_t eid;		/* WR: element identification */
	int input_channels;		/* RO: input channels to element */
	int output_channels;		/* RO: output channels from element */
	union {
		struct snd_mixer_element_io_info io;
		struct snd_mixer_element_pcm1_info pcm1;
		struct snd_mixer_element_pcm2_info pcm2;
		struct snd_mixer_element_pcm3_info pcm3;
		struct snd_mixer_element_converter_info converter;
		struct snd_mixer_element_switch3_info switch3;
		struct snd_mixer_element_volume1_info volume1;
		struct snd_mixer_element_volume2_info volume2;
		struct snd_mixer_element_accu1_info accu1;
		struct snd_mixer_element_accu2_info accu2;
		struct snd_mixer_element_accu3_info accu3;
		struct snd_mixer_element_mux1_info mux1;
		struct snd_mixer_element_mux2_info mux2;
		struct snd_mixer_element_tone_control1_info tc1;
		struct snd_mixer_element_pan_control1_info pc1;
		struct snd_mixer_element_3d_effect1_info teffect1;
		struct snd_mixer_element_pre_effect1_info peffect1;
		char reserve[120];
	} data;
} snd_mixer_element_info_t;

typedef struct snd_mixer_element {
	snd_mixer_eid_t eid;
	union {
		struct snd_mixer_element_switch1 switch1;
		struct snd_mixer_element_switch2 switch2;
		struct snd_mixer_element_switch3 switch3;
		struct snd_mixer_element_mux1 mux1;
		struct snd_mixer_element_mux2 mux2;
		struct snd_mixer_element_accu3 accu3;
		struct snd_mixer_element_volume1 volume1;
		struct snd_mixer_element_volume2 volume2;
		struct snd_mixer_element_tone_control1 tc1;
		struct snd_mixer_element_pan_control1 pc1;
		struct snd_mixer_element_3d_effect1 teffect1;
		struct snd_mixer_element_pre_effect1 peffect1;
		char reserve[120];
	} data;
} snd_mixer_element_t;

typedef struct snd_mixer_filter {
	unsigned int read_cmds[8];
} snd_mixer_filter_t;

/* ioctl commands */
#define SND_MIXER_IOCTL_PVERSION	_IOR ('R', 0x00, int)
#define SND_MIXER_IOCTL_INFO		_IOR ('R', 0x01, snd_mixer_info_t)
#define SND_MIXER_IOCTL_ELEMENTS	_IOWR('R', 0x10, snd_mixer_elements_t)
#define SND_MIXER_IOCTL_ROUTES		_IOWR('R', 0x11, snd_mixer_routes_t)
#define SND_MIXER_IOCTL_ROUTE		_IOWR('R', 0x12, snd_mixer_route_t)
#define SND_MIXER_IOCTL_GROUPS		_IOWR('R', 0x13, snd_mixer_groups_t)
#define SND_MIXER_IOCTL_GROUP_READ	_IOWR('R', 0x14, snd_mixer_group_t)
#define SND_MIXER_IOCTL_GROUP_WRITE	_IOWR('R', 0x15, snd_mixer_group_t)
#define SND_MIXER_IOCTL_ELEMENT_INFO	_IOWR('R', 0x20, snd_mixer_element_info_t)
#define SND_MIXER_IOCTL_ELEMENT_READ	_IOWR('R', 0x21, snd_mixer_element_t)
#define SND_MIXER_IOCTL_ELEMENT_WRITE	_IOWR('R', 0x22, snd_mixer_element_t)
#define SND_MIXER_IOCTL_GET_FILTER	_IOR ('R', 0x30, snd_mixer_filter_t)
#define SND_MIXER_IOCTL_PUT_FILTER	_IOW ('R', 0x30, snd_mixer_filter_t)

/*
 *  Read interface.
 */

#define SND_MIXER_READ_REBUILD		0	/* rebuild the mixer structure */
#define SND_MIXER_READ_ELEMENT_VALUE	1	/* the element value was changed */
#define SND_MIXER_READ_ELEMENT_CHANGE	2	/* the element was changed */
#define SND_MIXER_READ_ELEMENT_ROUTE	3	/* the element route was changed */
#define SND_MIXER_READ_ELEMENT_ADD	4	/* the element was added */
#define SND_MIXER_READ_ELEMENT_REMOVE	5	/* the element was removed */
#define SND_MIXER_READ_GROUP_VALUE	6	/* the group value was changed */
#define SND_MIXER_READ_GROUP_CHANGE	7	/* the group was changed */
#define SND_MIXER_READ_GROUP_ADD	8	/* the group was added */
#define SND_MIXER_READ_GROUP_REMOVE	9	/* the group was removed */

typedef struct snd_mixer_read {
	unsigned int cmd;		/* command - SND_MIXER_READ_* */
	union {
		snd_mixer_eid_t eid;	/* element identification */
		snd_mixer_gid_t gid;	/* group identification */
		unsigned char data8[60];
	} data;
} snd_mixer_read_t;

/*
 *  Interface compatible with Open Sound System API
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

#define SND_PCM_STREAM_PLAYBACK	0
#define SND_PCM_STREAM_CAPTURE		1

#define SND_PCM_MODE_UNKNOWN		(-1)
#define SND_PCM_MODE_FRAME		0
#define SND_PCM_MODE_FRAGMENT		1

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

#define SND_PCM_INFO_PLAYBACK		0x00000001
#define SND_PCM_INFO_CAPTURE		0x00000002
#define SND_PCM_INFO_DUPLEX		0x00000100
#define SND_PCM_INFO_DUPLEX_RATE	0x00000200	/* rate for playback & capture streams must be same!!! */
#define SND_PCM_INFO_DUPLEX_MONO	0x00000400	/* in duplex mode - only mono (one channel) is supported */

#define SND_PCM_STREAM_INFO_MMAP		0x00000001	/* hardware supports mmap */
#define SND_PCM_STREAM_INFO_FRAME		0x00000002	/* hardware supports frame mode */
#define SND_PCM_STREAM_INFO_FRAGMENT		0x00000004	/* hardware supports fragment mode */
#define SND_PCM_STREAM_INFO_BATCH		0x00000010	/* double buffering */
#define SND_PCM_STREAM_INFO_INTERLEAVE		0x00000100	/* channels are interleaved */
#define SND_PCM_STREAM_INFO_NONINTERLEAVE	0x00000200	/* channels are not interleaved */
#define SND_PCM_STREAM_INFO_BLOCK_TRANSFER	0x00010000	/* hardware transfer block of samples */
#define SND_PCM_STREAM_INFO_OVERRANGE		0x00020000	/* hardware supports ADC (capture) overrange detection */
#define SND_PCM_STREAM_INFO_MMAP_VALID		0x00040000	/* fragment data are valid during transfer */
#define SND_PCM_STREAM_INFO_PAUSE		0x00080000	/* pause ioctl is supported */

#define SND_PCM_START_DATA		0	/* start when some data are written (playback) or requested (capture) */
#define SND_PCM_START_FULL		1	/* start when whole queue is filled (playback) */
#define SND_PCM_START_GO		2	/* start on the go command */

#define SND_PCM_XRUN_FLUSH		0	/* stop on xrun */
#define SND_PCM_XRUN_DRAIN		1	/* erase and stop on xrun (capture) */

#define SND_PCM_FILL_NONE		0	/* don't fill the buffer with silent samples */
#define SND_PCM_FILL_SILENCE_WHOLE	1	/* fill the whole buffer with silence */
#define SND_PCM_FILL_SILENCE		2	/* fill the partial buffer with silence */

#define SND_PCM_STATE_NOTREADY		0	/* stream is not ready */
#define SND_PCM_STATE_READY		1	/* stream is ready for prepare call */
#define SND_PCM_STATE_PREPARED		2	/* stream is ready to go */
#define SND_PCM_STATE_RUNNING		3	/* stream is running */
#define SND_PCM_STATE_XRUN		4
#define SND_PCM_STATE_UNDERRUN		SND_PCM_STATE_XRUN	/* stream reached an underrun and it is not ready */
#define SND_PCM_STATE_OVERRUN		SND_PCM_STATE_XRUN	/* stream reached an overrun and it is not ready */
#define SND_PCM_STATE_PAUSED		5	/* stream is paused */

#define SND_PCM_MMAP_OFFSET_CONTROL	0x00000000
#define SND_PCM_MMAP_OFFSET_DATA	0x80000000

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

typedef union snd_pcm_sync {
	char id[16];
	short id16[8];
	int id32[4];
} snd_pcm_sync_t;

typedef struct snd_pcm_digital {
	int type;			/* digital API type */
	int valid: 1;			/* set if the digital setup is valid */
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
	unsigned int type;		/* soundcard type */
	unsigned int flags;		/* see to SND_PCM_INFO_* */
	unsigned char id[64];		/* ID of this PCM device (user selectable) */
	unsigned char name[80];		/* name of this device */
	int playback;			/* playback subdevices - 1 */
	int capture;			/* capture subdevices - 1 */
	unsigned short pcm_class;	/* SND_PCM_CLASS_* */
	unsigned short pcm_subclass;	/* SND_PCM_SCLASS_* */
	char reserved[60];		/* reserved for future... */
} snd_pcm_info_t;

typedef struct snd_pcm_stream_info {
	int subdevice;			/* subdevice number */
	char subname[32];		/* subdevice name */
	int stream;			/* stream number */
	snd_pcm_sync_t sync;		/* hardware synchronization ID */
	unsigned int flags;		/* see to SND_PCM_STREAM_INFO_XXXX */
	unsigned int formats;		/* supported formats */
	unsigned int rates;		/* hardware rates */
	unsigned int min_rate;		/* min rate (in Hz) */
	unsigned int max_rate;		/* max rate (in Hz) */
	unsigned int min_channels;	/* min channels */
	unsigned int max_channels;	/* max channels */
	size_t buffer_size;		/* max buffer size in bytes */
	size_t min_fragment_size;	/* min fragment size in bytes */
	size_t max_fragment_size;	/* max fragment size in bytes */
	size_t min_fragments;		/* min # of fragments */
	size_t max_fragments;		/* max # of fragments */
	size_t fragment_align;		/* align fragment value */
	size_t fifo_size;		/* stream FIFO size in bytes */
	size_t transfer_block_size;	/* bus transfer block size in bytes */
	snd_pcm_digital_t dig_mask;	/* AES/EBU/IEC958 supported bits, zero = no AES/EBU/IEC958 */
	size_t mmap_size;		/* mmap data size */
	int mixer_device;		/* mixer device */
	snd_mixer_eid_t mixer_eid;	/* mixer element identification */
	char reserved[64];		/* reserved for future... */
} snd_pcm_stream_info_t;

typedef struct snd_pcm_channel_info {
	unsigned int channel;		/* channel number */
	char channel_name[32];		/* name of channel */
	int mixer_device;		/* mixer device */
	snd_mixer_eid_t mixer_eid;	/* mixer element identification */
	int dig_group;			/* digital group */
	snd_pcm_digital_t dig_mask;	/* AES/EBU/IEC958 supported bits, zero = no AES/EBU/IEC958 */
	char reserved[64];		/* reserved for future... */
} snd_pcm_channel_info_t;

typedef struct snd_pcm_format {
	int interleave: 1;		/* data are interleaved */
	int format;			/* SND_PCM_SFMT_XXXX */
	unsigned int rate;		/* rate in Hz */
	unsigned int channels;		/* channels */
	int special;			/* special (custom) description of format */
	char reserved[16];		/* must be filled with zero */
} snd_pcm_format_t;

typedef struct snd_pcm_stream_params {
	int stream;			/* stream information (IGNORED in kernel space) */
	int mode;			/* transfer mode */
	snd_pcm_format_t format;	/* playback format */
	snd_pcm_digital_t digital;	/* digital setup */
	int start_mode;			/* start mode - SND_PCM_START_XXXX */
	int xrun_mode;			/* underrun/overrun mode - SND_PCM_XRUN_XXXX */
	int time: 1,			/* timestamp (gettimeofday) switch */
	    ust_time: 1;		/* UST time switch */
	snd_pcm_sync_t sync;		/* sync group */
	size_t buffer_size;		/* requested buffer size */
	size_t frag_size;		/* requested size of fragment in bytes */
	size_t bytes_min;		/* min available bytes for wakeup */
	size_t bytes_align;		/* transferred size need to be a multiple */
	size_t bytes_xrun_max;		/* maximum size of underrun/overrun before unconditional stop */
	int fill_mode;			/* fill mode - SND_PCM_FILL_XXXX */
	size_t bytes_fill_max;		/* maximum silence fill in bytes */
	size_t byte_boundary;		/* position in bytes wrap point */
	char reserved[64];		/* must be filled with zero */
} snd_pcm_stream_params_t;

typedef struct snd_pcm_channel_params {
	unsigned int channel;
	snd_pcm_digital_t digital;	/* digital setup */
	char reserved[64];
} snd_pcm_channel_params_t;

typedef struct snd_pcm_stream_setup {
	int stream;			/* stream information */
	int mode;			/* transfer mode */
	snd_pcm_format_t format;	/* real used format */
	snd_pcm_digital_t digital;	/* digital setup */
	int start_mode;			/* start mode - SND_PCM_START_XXXX */
	int xrun_mode;			/* underrun/overrun mode - SND_PCM_XRUN_XXXX */
	int time: 1,			/* timestamp (gettimeofday) switch */
	    ust_time: 1;		/* UST time switch */
	snd_pcm_sync_t sync;		/* sync group */
	size_t buffer_size;		/* current buffer size in bytes */
	size_t frag_size;		/* current fragment size in bytes */
	size_t bytes_min;		/* min available bytes for wakeup */
	size_t bytes_align;		/* transferred size need to be a multiple */
	size_t bytes_xrun_max;		/* max size of underrun/overrun before unconditional stop */
	int fill_mode;			/* fill mode - SND_PCM_FILL_XXXX */
	size_t bytes_fill_max;		/* maximum silence fill in bytes */
	size_t byte_boundary;		/* position in bytes wrap point */
	size_t frags;			/* allocated fragments */
	unsigned int msbits_per_sample;		/* used most significant bits per sample */
	char reserved[64];		/* must be filled with zero */
} snd_pcm_stream_setup_t;

typedef struct snd_pcm_channel_area {
	void *addr;			/* base address of channel samples */
	unsigned int first;		/* offset to first sample in bits */
	unsigned int step;		/* samples distance in bits */
} snd_pcm_channel_area_t;

typedef struct snd_pcm_channel_setup {
	unsigned int channel;
	snd_pcm_digital_t digital;	/* digital setup */
	snd_pcm_channel_area_t area;
	char reserved[64];
} snd_pcm_channel_setup_t;

typedef struct snd_pcm_stream_status {
	int stream;		/* stream information */
	int state;		/* stream state - SND_PCM_STATE_XXXX */
	struct timeval stime;	/* time when playback/capture was started */
	long long ust_stime;	/* UST time when playback/capture was started */
	size_t byte_io;		/* current I/O position in bytes */
	size_t byte_data;	/* current data position */
	size_t bytes_avail;	/* number of bytes available for application */
	size_t bytes_avail_max;	/* max bytes available since last status */
	unsigned int xruns;	/* count of underruns/overruns from last status */
	unsigned int overrange;	/* count of ADC (capture) overrange detections from last status */
	char reserved[64];	/* must be filled with zero */
} snd_pcm_stream_status_t;

typedef struct {
	volatile int state;	/* RO: status - SND_PCM_STATE_XXXX */
	size_t byte_io;		/* RO: I/O position (0 ... byte_boundary-1) updated only on status query and at interrupt time */
	size_t byte_data;	/* RW: application position (0 ... byte_boundary-1) checked only on status query and at interrupt time */
	int reserved[59];	/* reserved */
} snd_pcm_mmap_control_t;

#define SND_PCM_IOCTL_PVERSION		_IOR ('A', 0x00, int)
#define SND_PCM_IOCTL_INFO		_IOR ('A', 0x01, snd_pcm_info_t)
#define SND_PCM_IOCTL_STREAM_INFO	_IOR ('A', 0x02, snd_pcm_stream_info_t)
#define SND_PCM_IOCTL_STREAM_PARAMS	_IOW ('A', 0x10, snd_pcm_stream_params_t)
#define SND_PCM_IOCTL_STREAM_SETUP	_IOR ('A', 0x20, snd_pcm_stream_setup_t)
#define SND_PCM_IOCTL_STREAM_STATUS	_IOR ('A', 0x21, snd_pcm_stream_status_t)
#define SND_PCM_IOCTL_STREAM_BYTE_IO	_IO  ('A', 0x22)
#define SND_PCM_IOCTL_STREAM_PREPARE	_IO  ('A', 0x30)
#define SND_PCM_IOCTL_STREAM_GO		_IO  ('A', 0x31)
#define SND_PCM_IOCTL_STREAM_FLUSH	_IO  ('A', 0x32)
#define SND_PCM_IOCTL_SYNC_GO		_IOW ('A', 0x33, snd_pcm_sync_t)
#define SND_PCM_IOCTL_STREAM_DRAIN	_IO  ('A', 0x34)
#define SND_PCM_IOCTL_STREAM_PAUSE	_IOW ('A', 0x35, int)
#define SND_PCM_IOCTL_CHANNEL_INFO	_IOR ('A', 0x40, snd_pcm_channel_info_t)
#define SND_PCM_IOCTL_CHANNEL_PARAMS	_IOW ('A', 0x41, snd_pcm_channel_params_t)
#define SND_PCM_IOCTL_CHANNEL_SETUP	_IOR ('A', 0x42, snd_pcm_channel_setup_t)

/*
 *  Interface compatible with Open Sound System API
 */

#ifdef __SND_OSS_COMPAT__

#define SND_PCM_AFMT_QUERY		0
#define SND_PCM_AFMT_MU_LAW		(1<<0)
#define SND_PCM_AFMT_A_LAW		(1<<1)
#define SND_PCM_AFMT_IMA_ADPCM		(1<<2)
#define SND_PCM_AFMT_U8			(1<<3)
#define SND_PCM_AFMT_S16_LE		(1<<4)
#define SND_PCM_AFMT_S16_BE		(1<<5)
#define SND_PCM_AFMT_S8			(1<<6)
#define SND_PCM_AFMT_U16_LE		(1<<7)
#define SND_PCM_AFMT_U16_BE		(1<<8)
#define SND_PCM_AFMT_MPEG		(1<<9)

#define SND_PCM_ENABLE_CAPTURE		0x00000001
#define SND_PCM_ENABLE_PLAYBACK		0x00000002

#define SND_PCM_CAP_REVISION		0x000000ff
#define SND_PCM_CAP_DUPLEX		0x00000100
#define SND_PCM_CAP_REALTIME		0x00000200
#define SND_PCM_CAP_BATCH		0x00000400
#define SND_PCM_CAP_COPROC		0x00000800
#define SND_PCM_CAP_TRIGGER		0x00001000
#define SND_PCM_CAP_MMAP		0x00002000

#define SND_PCM_AFP_NORMAL		0
#define SND_PCM_AFP_NETWORK		1
#define SND_PCM_AFP_CPUINTENS		2

struct snd_pcm_buffer_info {
	int fragments;		/* # of available fragments (partially used ones not counted) */
	int fragstotal;		/* Total # of fragments allocated */
	int fragsize;		/* Size of a fragment in bytes */
	int bytes;		/* Available space in bytes (includes partially used fragments) */
};

struct snd_pcm_count_info {
	int bytes;		/* Total # of bytes processed */
	int blocks;		/* # of fragment transitions since last time */
	int ptr;		/* Current DMA pointer value */
};

struct snd_pcm_buffer_description {
	unsigned char *buffer;
	int size;
};

#define SND_PCM_IOCTL_OSS_RESET		_IO  ('P', 0)
#define SND_PCM_IOCTL_OSS_SYNC		_IO  ('P', 1)
#define SND_PCM_IOCTL_OSS_RATE		_IOWR('P', 2, int)
#define SND_PCM_IOCTL_OSS_GETRATE	_IOR ('P', 2, int)
#define SND_PCM_IOCTL_OSS_STEREO	_IOWR('P', 3, int)
#define SND_PCM_IOCTL_OSS_GETBLKSIZE	_IOWR('P', 4, int)
#define SND_PCM_IOCTL_OSS_FORMAT	_IOWR('P', 5, int)
#define SND_PCM_IOCTL_OSS_GETFORMAT	_IOR ('P', 5, int)
#define SND_PCM_IOCTL_OSS_CHANNELS	_IOWR('P', 6, int)
#define SND_PCM_IOCTL_OSS_GETCHANNELS	_IOR ('P', 6, int)
#define SND_PCM_IOCTL_OSS_FILTER	_IOWR('P', 7, int)
#define SND_PCM_IOCTL_OSS_GETFILTER	_IOR ('P', 7, int)
#define SND_PCM_IOCTL_OSS_POST		_IO  ('P', 8 )
#define SND_PCM_IOCTL_OSS_SUBDIVIDE	_IOWR('P', 9, int)
#define SND_PCM_IOCTL_OSS_SETFRAGMENT	_IOWR('P', 10, int)
#define SND_PCM_IOCTL_OSS_GETFORMATS	_IOR ('P', 11, int)
#define SND_PCM_IOCTL_OSS_GETPBKSPACE	_IOR ('P', 12, struct snd_pcm_buffer_info)
#define SND_PCM_IOCTL_OSS_GETRECSPACE	_IOR ('P', 13, struct snd_pcm_buffer_info)
#define SND_PCM_IOCTL_OSS_NONBLOCK	_IO  ('P', 14)
#define SND_PCM_IOCTL_OSS_GETCAPS	_IOR ('P', 15, int)
#define SND_PCM_IOCTL_OSS_GETTRIGGER	_IOR ('P', 16, int)
#define SND_PCM_IOCTL_OSS_SETTRIGGER	_IOW ('P', 16, int)
#define SND_PCM_IOCTL_OSS_GETRECPTR	_IOR ('P', 17, struct snd_pcm_count_info)
#define SND_PCM_IOCTL_OSS_GETPBKPTR	_IOR ('P', 18, struct snd_pcm_count_info)
#define SND_PCM_IOCTL_OSS_MAPRECBUFFER	_IOR ('P', 19, struct snd_pcm_buffer_description)
#define SND_PCM_IOCTL_OSS_MAPPBKBUFFER	_IOR ('P', 20, struct snd_pcm_buffer_description)
#define SND_PCM_IOCTL_OSS_SYNCRO	_IO  ('P', 21)
#define SND_PCM_IOCTL_OSS_DUPLEX	_IO  ('P', 22)
#define SND_PCM_IOCTL_OSS_GETODELAY	_IOR ('P', 23, int)
#define SND_PCM_IOCTL_OSS_PROFILE	_IOW ('P', 23, int)

#endif				/* __SND_OSS_COMPAT__ */

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
	unsigned int type;		/* soundcard type */
	unsigned int flags;		/* SND_RAWMIDI_INFO_XXXX */
	unsigned char id[64];		/* ID of this raw midi device (user selectable) */
	unsigned char name[80];		/* name of this raw midi device */
	unsigned char reserved[64];	/* reserved for future use */
} snd_rawmidi_info_t;

typedef struct snd_rawmidi_params {
	int stream;		/* Requested stream */
	size_t size;		/* I/O requested queue size in bytes */
	size_t min;		/* I minimum count of bytes in queue for wakeup */
	size_t max;		/* O maximum count of bytes in queue for wakeup */
	size_t room;		/* O minumum number of bytes writeable for wakeup */
	unsigned char reserved[16];	/* reserved for future use */
} snd_rawmidi_params_t;

typedef struct snd_rawmidi_setup {
	int stream;		/* Requested stream */
	size_t size;		/* I/O real queue queue size in bytes */
	size_t min;		/* I minimum count of bytes in queue for wakeup */
	size_t max;		/* O maximum count of bytes in queue for wakeup */
	size_t room;		/* O minumum number of bytes writeable for wakeup */
	unsigned char reserved[16];	/* reserved for future use */
} snd_rawmidi_setup_t;

typedef struct snd_rawmidi_status {
	int stream;		/* Requested stream */
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
#define SND_RAWMIDI_IOCTL_STREAM_DRAIN	_IOW ('W', 0x30, int)
#define SND_RAWMIDI_IOCTL_STREAM_FLUSH _IOW ('W', 0x31, int)

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
	int count;			/* count of global timers */
	char reserved[64];
} snd_timer_general_info_t;

typedef struct snd_timer_select {
	int slave: 1;			/* timer is slave */
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
	unsigned int flags;		/* flags - SND_MIXER_PSFLG_* */
	unsigned long ticks;		/* requested resolution in ticks */
	int queue_size;			/* total size of queue (32-1024) */
	char reserved[64];
} snd_timer_params_t;

typedef struct snd_timer_setup {
	unsigned int flags;		/* flags - SND_MIXER_PSFLG_* */
	unsigned long ticks;		/* requested resolution in ticks */
	int queue_size;			/* total queue size */
	char reserved[64];
} snd_timer_setup_t;

typedef struct snd_timer_status {
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

typedef struct {
	const struct iovec *vector;
	unsigned long count;
} snd_v_args_t;

#define SND_IOCTL_READV		_IOW ('K', 0x00, snd_v_args_t)
#define SND_IOCTL_WRITEV	_IOW ('K', 0x01, snd_v_args_t)

#endif				/* __ASOUND_H */
