/*
 *  Advanced Linux Sound Architecture - ALSA - Driver
 *
 *  The interface file between the ALSA driver & the user space
 *  Copyright (c) 1994-98 by Jaroslav Kysela <perex@suse.cz>
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
#define SND_SW_TYPE_USER		(~0)	/* user type */

typedef struct snd_switch_list_item {
	unsigned char name[32];
} snd_switch_list_item_t;

typedef struct snd_switch_list {
	int switches_size;		/* size of switches in array */
	int switches;			/* filled switches in array */
	int switches_over;		/* missing switches in array */
	snd_switch_list_item_t *pswitches; /* pointer to list item array */
} snd_switch_list_t;

typedef struct snd_switch {
	unsigned char name[32];	/* unique identification of switch (from driver) */
	unsigned int type;	/* look to SND_SW_TYPE_* */
	unsigned int low;	/* low range value */
	unsigned int high;	/* high range value */
	union {
		unsigned int enable: 1;		/* 0 = off, 1 = on */
		unsigned char data8[32];	/* 8-bit data */
		unsigned short data16[16];	/* 16-bit data */
		unsigned int data32[8];		/* 32-bit data */
		int item_number;		/* active list item number */
		char item[32];			/* list item, low=high -> item number */
	} value;
	unsigned char reserved[32];
} snd_switch_t;
 
/****************************************************************************
 *                                                                          *
 *        Section for driver control interface - /dev/snd/control?          *
 *                                                                          *
 ****************************************************************************/

#define SND_CTL_VERSION			SND_PROTOCOL_VERSION(2, 0, 0)

#define SND_CTL_SW_JOYSTICK		"Joystick"
#define SND_CTL_SW_JOYSTICK_ADDRESS	"Joystick Address"
#define SND_CTL_SW_JOYSTICK_SPEED	"Joystick Speed Compensation"

typedef struct snd_ctl_hw_info {
	unsigned int type;	/* type of card - look to SND_CARD_TYPE_XXXX */
	unsigned int hwdepdevs;	/* count of hardware dependent devices (0 to N) */
	unsigned int pcmdevs;	/* count of PCM devices (0 to N) */
	unsigned int mixerdevs;	/* count of MIXER devices (0 to N) */
	unsigned int mididevs;	/* count of raw MIDI devices (0 to N) */
	unsigned int timerdevs;	/* count of timer devices (0 to N) */
	char id[16];		/* ID of card (user selectable) */
	char abbreviation[16];	/* Abbreviation for soundcard */
	char name[32];		/* Short name of soundcard */
	char longname[80];	/* name + info text about soundcard */
	unsigned int switches;	/* count of switches */
	unsigned char reserved[128];	/* reserved for future */
} snd_ctl_hw_info_t;

#define SND_CTL_IOCTL_PVERSION		_IOR ('U', 0x00, int)
#define SND_CTL_IOCTL_HW_INFO		_IOR ('U', 0x01, snd_ctl_hw_info_t)
#define SND_CTL_IOCTL_SWITCH_LIST	_IOWR('U', 0x02, snd_switch_list_t)
#define SND_CTL_IOCTL_SWITCH_READ	_IOWR('U', 0x03, snd_switch_t)
#define SND_CTL_IOCTL_SWITCH_WRITE	_IOWR('U', 0x04, snd_switch_t)
#define SND_CTL_IOCTL_HWDEP_DEVICE	_IOWR('U', 0x08, int)
#define SND_CTL_IOCTL_HWDEP_INFO	_IOR ('U', 0x09, snd_hwdep_info_t)
#define SND_CTL_IOCTL_MIXER_DEVICE	_IOWR('U', 0x10, int)
#define SND_CTL_IOCTL_MIXER_INFO	_IOR ('U', 0x10, snd_mixer_info_t)
#define SND_CTL_IOCTL_MIXER_SWITCH_LIST	_IOWR('U', 0x11, snd_switch_list_t)
#define SND_CTL_IOCTL_MIXER_SWITCH_READ	_IOWR('U', 0x12, snd_switch_t)
#define SND_CTL_IOCTL_MIXER_SWITCH_WRITE _IOWR('U', 0x13, snd_switch_t)
#define SND_CTL_IOCTL_PCM_DEVICE	_IOWR('U', 0x20, int)
#define SND_CTL_IOCTL_PCM_INFO		_IOR ('U', 0x21, snd_pcm_info_t)
#define SND_CTL_IOCTL_PCM_PLAYBACK_INFO	_IOR ('U', 0x22, snd_pcm_playback_info_t)
#define SND_CTL_IOCTL_PCM_CAPTURE_INFO	_IOR ('U', 0x23, snd_pcm_capture_info_t)
#define SND_CTL_IOCTL_PCM_PSWITCH_LIST	_IOWR('U', 0x24, snd_switch_list_t)
#define SND_CTL_IOCTL_PCM_PSWITCH_READ  _IOWR('U', 0x25, snd_switch_t)
#define SND_CTL_IOCTL_PCM_PSWITCH_WRITE _IOWR('U', 0x26, snd_switch_t)
#define SND_CTL_IOCTL_PCM_RSWITCH_LIST	_IOWR('U', 0x27, snd_switch_list_t)
#define SND_CTL_IOCTL_PCM_RSWITCH_READ  _IOWR('U', 0x28, snd_switch_t)
#define SND_CTL_IOCTL_PCM_RSWITCH_WRITE _IOWR('U', 0x29, snd_switch_t)
#define SND_CTL_IOCTL_RAWMIDI_DEVICE	_IOWR('U', 0x30, int)
#define SND_CTL_IOCTL_RAWMIDI_INFO	_IOR ('U', 0x31, snd_rawmidi_info_t)
#define SND_CTL_IOCTL_RAWMIDI_OUTPUT_INFO _IOR('U', 0x32, snd_rawmidi_output_info_t)
#define SND_CTL_IOCTL_RAWMIDI_INPUT_INFO _IOR('U', 0x33, snd_rawmidi_input_info_t)
#define SND_CTL_IOCTL_RAWMIDI_OSWITCH_LIST _IOWR('U', 0x34, snd_switch_list_t)
#define SND_CTL_IOCTL_RAWMIDI_OSWITCH_READ _IOWR('U', 0x35, snd_switch_t)
#define SND_CTL_IOCTL_RAWMIDI_OSWITCH_WRITE _IOWR('U', 0x36, snd_switch_t)
#define SND_CTL_IOCTL_RAWMIDI_ISWITCH_LIST _IOWR('U', 0x37, snd_switch_list_t)
#define SND_CTL_IOCTL_RAWMIDI_ISWITCH_READ _IOWR('U', 0x38, snd_switch_t)
#define SND_CTL_IOCTL_RAWMIDI_ISWITCH_WRITE _IOWR('U', 0x39, snd_switch_t)

/*
 *  Read interface.
 */

#define SND_CTL_READ_REBUILD		0	/* rebuild the whole structure */
#define SND_CTL_READ_SWITCH_VALUE	1	/* the switch value was changed */
#define SND_CTL_READ_SWITCH_CHANGE	2	/* the switch was changed */
#define SND_CTL_READ_SWITCH_ADD		3	/* the switch was added */
#define SND_CTL_READ_SWITCH_REMOVE	4	/* the switch was removed */

#define SND_CTL_IFACE_CONTROL		0
#define SND_CTL_IFACE_MIXER		1
#define SND_CTL_IFACE_PCM_PLAYBACK	2
#define SND_CTL_IFACE_PCM_CAPTURE	3
#define SND_CTL_IFACE_RAWMIDI_OUTPUT	4
#define SND_CTL_IFACE_RAWMIDI_INPUT	5

typedef struct snd_ctl_read {
	unsigned int cmd;		/* command - SND_MIXER_READ_* */
	union {
		struct {
			int iface;	/* interface */
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
#define SND_HWDEP_TYPE_EMU8000		3
#define SND_HWDEP_TYPE_YSS225           4      /* Yamaha FX processor */
#define SND_HWDEP_TYPE_ICS2115          5      /* Wavetable synth */
/* --- */
#define SND_HWDEP_TYPE_LAST             5

typedef struct snd_hwdep_info {
	unsigned int type;	/* type of card - look to SND_CARD_TYPE_XXXX */
	unsigned char id[32];	/* ID of this hardware dependent device */
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

#define SND_MIXER_VERSION		SND_PROTOCOL_VERSION(2, 1, 0)

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

/* outputs */				/* max 24 chars */
#define SND_MIXER_OUT_MASTER		"Master"
#define SND_MIXER_OUT_MASTER_MONO	"Master Mono"
#define SND_MIXER_OUT_MASTER_DIGITAL	"Master Digital"
#define SND_MIXER_OUT_HEADPHONE		"Headphone"
#define SND_MIXER_OUT_PHONE		"Phone Output"
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
/* capture endpoint */
#define SND_MIXER_ETYPE_CAPTURE		2
/* playback startpoint */
#define SND_MIXER_ETYPE_PLAYBACK	3
/* ADC */
#define SND_MIXER_ETYPE_ADC		4
/* DAC */
#define SND_MIXER_ETYPE_DAC		5
/* simple on/off switch */
#define SND_MIXER_ETYPE_SWITCH1		100
/* simple on/off switch for each voices */
#define SND_MIXER_ETYPE_SWITCH2		101
/* simple voice route switch */
#define SND_MIXER_ETYPE_SWITCH3		102
/* simple volume control */
#define SND_MIXER_ETYPE_VOLUME1		200
/* simple volume control - PCM voices to DAC */
#define SND_MIXER_ETYPE_VOLUME2		201
/* simple accumulator */
#define SND_MIXER_ETYPE_ACCU1		300
/* simple accumulator with MONO output */
#define SND_MIXER_ETYPE_ACCU2		301
/* simple accumulator with programmable attenuation */
#define SND_MIXER_ETYPE_ACCU3		302
/* simple MUX */
#define SND_MIXER_ETYPE_MUX1		400
/* simple MUX with one control for each voices */
#define SND_MIXER_ETYPE_MUX2		401
/* simple tone control */
#define SND_MIXER_ETYPE_TONE_CONTROL1	500
/* equalizer */
#define SND_MIXER_ETYPE_EQUALIZER1	501
/* simple 3D effect */
#define SND_MIXER_ETYPE_3D_EFFECT1	600
/* predefined effect */
#define SND_MIXER_ETYPE_PRE_EFFECT1	610

/*
 *  voice definitions
 */

#define SND_MIXER_VOICE_UNUSED		0
#define SND_MIXER_VOICE_MONO		1
#define SND_MIXER_VOICE_LEFT		2
#define SND_MIXER_VOICE_RIGHT		3
#define SND_MIXER_VOICE_CENTER		4
#define SND_MIXER_VOICE_REAR_LEFT	5
#define SND_MIXER_VOICE_REAR_RIGHT	6

typedef struct {
	unsigned short voice: 15,
		       vindex: 1;
} snd_mixer_voice_t;

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
	unsigned int attrib;	/* some attributes about this device (SND_MIXER_ATTR_*) */
	unsigned char id[32];	/* ID of this mixer */
	unsigned char name[80];	/* name of this device */
	int elements;		/* count of elements */
	int groups;		/* count of element groups */
	int switches;		/* count of switches */
	char reserved[32];	/* reserved for future use */
} snd_mixer_info_t;

/*
 *  Element information.
 */

typedef struct snd_mixer_elements {
	int elements_size;	/* size in element identifiers */
	int elements;		/* count of filled element identifiers */
	int elements_over;	/* missing element identifiers */
	snd_mixer_eid_t *pelements; /* array */
} snd_mixer_elements_t;

/*
 *  Route information.
 */

typedef struct snd_mixer_routes {
	snd_mixer_eid_t eid;
	int routes_size;	/* size in element identifiers */
	int routes;		/* count of filled element identifiers */
	int routes_over;	/* missing element identifiers */
	snd_mixer_eid_t *proutes; /* array */
} snd_mixer_routes_t;

/*
 *  Group information.
 */

typedef struct snd_mixer_groups {
	int groups_size;	/* size in group identifiers */
	int groups;		/* count of filled group identifiers */
	int groups_over;	/* missing group identifiers */
	snd_mixer_gid_t *pgroups; /* array */
} snd_mixer_groups_t;

#define SND_MIXER_CHN_MONO		0
#define SND_MIXER_CHN_FRONT_LEFT	0
#define SND_MIXER_CHN_FRONT_RIGHT	1
#define SND_MIXER_CHN_FRONT_CENTER	2
#define SND_MIXER_CHN_REAR_LEFT		3
#define SND_MIXER_CHN_REAR_RIGHT	4
#define SND_MIXER_CHN_WOOFER		5

#define SND_MIXER_CHN_MASK_MONO		(1<<SND_MIXER_CHN_MONO)
#define SND_MIXER_CHN_MASK_FRONT_LEFT	(1<<SND_MIXER_CHN_FRONT_LEFT)
#define SND_MIXER_CHN_MASK_FRONT_RIGHT	(1<<SND_MIXER_CHN_FRONT_RIGHT)
#define SND_MIXER_CHN_MASK_FRONT_CENTER	(1<<SND_MIXER_CHN_FRONT_CENTER)
#define SND_MIXER_CHN_MASK_REAR_LEFT	(1<<SND_MIXER_CHN_REAR_LEFT)
#define SND_MIXER_CHN_MASK_REAR_RIGHT	(1<<SND_MIXER_CHN_REAR_RIGHT)
#define SND_MIXER_CHN_MASK_WOOFER	(1<<SND_MIXER_CHN_WOOFER)
#define SND_MIXER_CHN_MASK_STEREO	(SND_MIXER_CHN_MASK_FRONT_LEFT|SND_MIXER_CHN_MASK_FRONT_RIGHT)

#define SND_MIXER_GRPCAP_VOLUME		(1<<0)
#define SND_MIXER_GRPCAP_MUTE		(1<<1)
#define SND_MIXER_GRPCAP_JOINTLY_MUTE	(1<<2)
#define SND_MIXER_GRPCAP_RECORD		(1<<3)
#define SND_MIXER_GRPCAP_JOINTLY_RECORD	(1<<4)
#define SND_MIXER_GRPCAP_EXCL_RECORD	(1<<5)

typedef struct snd_mixer_group {
	snd_mixer_gid_t gid;
	int elements_size;		/* size in element identifiers */
	int elements;			/* count of filled element identifiers */
	int elements_over;		/* missing element identifiers */
	snd_mixer_eid_t *pelements;	/* array */
	unsigned int caps;		/* capabilities */
	unsigned int channels;		/* bitmap of active channels */	
	unsigned int mute;		/* bitmap of muted channels */
	unsigned int record;		/* bitmap of record channels */
	int record_group;		/* record group (for exclusive record source) */
	int min;			/* minimum value */
	int max;			/* maximum value */
	int front_left;			/* front left value */
	int front_right;		/* front right value */
	int front_center;		/* front center */
	int rear_left;			/* left rear */
	int rear_right;			/* right rear */
	int woofer;			/* woofer */
	char reserved[64];		/* for the future use */
} snd_mixer_group_t;

/*
 *  INPUT/OUTPUT - read only
 *
 *    The input element describes input voices.
 */

#define SND_MIXER_EIO_DIGITAL		(0<<1)

struct snd_mixer_element_io_info {
	unsigned int attrib;		/* SND_MIXER_EIO_* */
	int voices_size;		/* size in voice descriptors */
	int voices;			/* count of filled voice descriptors */
	int voices_over;		/* missing voice descriptors */
	snd_mixer_voice_t *pvoices;	/* array */
};

/*
 *  PCM CAPTURE/PLAYBACK - read only
 *
 *    The element controls an capture endpoint.
 */

struct snd_mixer_element_pcm_info {
	int devices_size;		/* size in device descriptors */
	int devices;			/* count of filled device descriptors */
	int devices_over;		/* missing device descriptors */
	int *pdevices;			/* PCM devices - array */
};

/*
 *  ADC/DAC - read only
 *
 *    The element controls an analog-digital/digital-analog converter.
 */

struct snd_mixer_element_converter_info {
	unsigned int resolution;	/* resolution in bits (usually 16) */
};

/*
 *  Simple on/off switch - read write
 *
 *    This switch turns on or off the voice route for a group of voices.
 */

struct snd_mixer_element_switch1 {
	int sw_size;			/* size of bitmap (in bits) */
	int sw;				/* count of filled bits */
	int sw_over;			/* missing bits */
	unsigned int *psw;		/* bitmap!!! */
};

/*
 *  Simple on/off switch for each voices - read write
 *
 *    This switch turns on or off the voice route for group of voices.
 */

struct snd_mixer_element_switch2 {
	unsigned int sw:1;
};

/*
 *  Simple voice route switch - read write
 *
 *    This switch determines route from input voices to output voices.
 */

#define SND_MIXER_SWITCH3_FULL_FEATURED			0
#define SND_MIXER_SWITCH3_ALWAYS_DESTINATION		1
#define SND_MIXER_SWITCH3_ONE_DESTINATION		2
#define SND_MIXER_SWITCH3_ALWAYS_ONE_DESTINATION	3

struct snd_mixer_element_switch3_info {
	unsigned int type;		/* SND_MIXER_SWITCH3_* */
	/* X = output / Y = input voice descriptors */
	int voices_size;		/* size in voice descriptors */
	int voices;			/* count of filled voice descriptors */
	int voices_over;		/* missing voice descriptors */
	snd_mixer_voice_t *pvoices;	/* array */
};

struct snd_mixer_element_switch3 {
	/* two dimensional matrix of voice route switch */
	int rsw_size;			/* size in voice route descriptors (must be voice_size * voice_size bits !!!) */
	int rsw;			/* count of filled voice route descriptors */
	int rsw_over;			/* missing voice descriptors */
	unsigned int *prsw;		/* array */
};

/*
 *  Volume (attenuation/gain) control - read write
 *
 *    The volume must be always linear!!!
 */

struct snd_mixer_element_volume1_range {
	int min, max;		/* linear volume */
	int min_dB, max_dB;	/* negative - attenuation, positive - amplification */
};

struct snd_mixer_element_volume1_info {
	int range_size;		/* size of range descriptors */
	int range;		/* count of filled range descriptors */
	int range_over;		/* missing range descriptors */
	struct snd_mixer_element_volume1_range *prange;	/* array */
};

struct snd_mixer_element_volume1 {
	int voices_size;	/* size of voice descriptors */	
	int voices;		/* count of filled voice descriptors */
	int voices_over;	/* missing voice descriptors */
	int *pvoices;		/* array of volumes */
};

/*
 *  Volume (balance) control - read write
 *
 *    The volume must be always linear!!!
 */

struct snd_mixer_element_volume2_range {
	int min, max;		/* linear volume */
	int min_dB, max_dB;	/* negative - attenuation, positive - amplification */
	snd_mixer_voice_t dvoice; /* destonation voice */
};

struct snd_mixer_element_volume2_info {
	/* source voices */
	int svoices_size;
	int svoices;
	int svoices_over;
	snd_mixer_voice_t *psvoices;
	/* destonation ranges */
	int range_size;		/* size of range descriptors */
	int range;		/* count of filled range descriptors */
	int range_over;		/* missing range descriptors */
	struct snd_mixer_element_volume2_range *prange;	/* array */
};

/* avoices means the array of voices which describes volume offsets for */
/* each outputs, the size of this array is info->svoices * info->range */

struct snd_mixer_element_volume2 {
	int avoices_size;	/* size of voice descriptors */	
	int avoices;		/* count of filled voice descriptors */
	int avoices_over;	/* missing voice descriptors */
	int *pavoices;		/* array of volumes */
};

/*
 *  Simple accumulator
 */

struct snd_mixer_element_accu1_info {
	int attenuation;		/* in dB */
};

/*
 *  Simple accumulator with the MONO output
 */

struct snd_mixer_element_accu2_info {
	int attenuation;		/* in dB */
};

/* 
 *  Simple accumulator with programmable attenuation
 */

struct snd_mixer_element_accu3_range {
	int min, max;		/* linear volume */
	int min_dB, max_dB;	/* negative - attenuation, positive - amplification */
};

struct snd_mixer_element_accu3_info {
	int range_size;		/* size of range descriptors */
	int range;		/* count of filled range descriptors */
	int range_over;		/* missing range descriptors */
	struct snd_mixer_element_accu3_range *prange;	/* array */
};

struct snd_mixer_element_accu3 {
	int voices_size;	/* size of voice descriptors */	
	int voices;		/* count of filled voice descriptors */
	int voices_over;	/* missing voice descriptors */
	int *pvoices;		/* array of volumes */
};

/*
 *  Simple MUX
 *
 *     This mux allows selection of some (or none - optional) input.
 *     Each voices have got the separate control.
 */

#define SND_MIXER_MUX1_NONE		(1<<0)

struct snd_mixer_element_mux1_info {
	unsigned int attrib;		/* SND_MIXER_MUX1_ */
};

struct snd_mixer_element_mux1 {
	int output_size;
	int output;
	int output_over;
	snd_mixer_eid_t *poutput;	/* input source on output */
};

/*
 *  Simple MUX
 *
 *     This mux allows selection of exactly one (or none - optional) input.
 */

#define SND_MIXER_MUX2_NONE		(1<<0)

struct snd_mixer_element_mux2_info {
	unsigned int attrib;		/* SND_MIXER_MUX1_ */
};

struct snd_mixer_element_mux2 {
	snd_mixer_eid_t output;		/* input source on output */
};

/*
 *  Simple tone control
 */

#define SND_MIXER_TC1_SW		(1<<0)
#define SND_MIXER_TC1_BASS		(1<<1)
#define SND_MIXER_TC1_TREBLE		(1<<2)

struct snd_mixer_element_tone_control1_info {
	unsigned int tc;		/* bitmap of SND_MIXER_TC_* */
	int min_bass, max_bass;		/* Bass */
	int min_bass_dB, max_bass_dB;	/* in decibels * 100 */
	int min_treble, max_treble;	/* Treble */
	int min_treble_dB, max_treble_dB; /* in decibels * 100 */
};

struct snd_mixer_element_tone_control1 {
	unsigned int tc;		/* bitmap of SND_MIXER_TC_* */
	unsigned int sw:1;		/* on/off switch */
	int bass;			/* Bass control */
	int treble;			/* Treble control */
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

struct snd_mixer_element_3d_effect1_info {
	unsigned int effect;		/* bitmap of SND_MIXER_EFF1_* */
	int min_wide, max_wide;		/* 3D wide */
	int min_volume, max_volume;	/* 3D volume */
	int min_center, max_center;	/* 3D center */
	int min_space, max_space;	/* 3D space */
	int min_depth, max_depth;	/* 3D depth */
	int min_delay, max_delay;	/* 3D delay */
	int min_feedback, max_feedback;	/* 3D feedback */
};

struct snd_mixer_element_3d_effect1 {
	unsigned int effect;		/* bitmap of SND_MIXER_EFF1_* */
	unsigned int sw:1,		/* on/off switch */
		     mono_sw:1;		/* on/off switch */
	int wide;			/* 3D wide */
	int volume;			/* 3D volume */
	int center;			/* 3D center */
	int space;			/* 3D space */
	int depth;			/* 3D depth */
	int delay;			/* 3D delay */
	int feedback;			/* 3D feedback */
};

/*
 *  Simple predefined effect
 */

struct snd_mixer_element_pre_effect1_info_item {
	char name[32];
};

struct snd_mixer_element_pre_effect1_info_parameter {
	char name[32];
	int min, max;			/* minimum and maximum value */
};

struct snd_mixer_element_pre_effect1_info {
	/* predefined programs */
	int items_size;
	int items;
	int items_over;
	struct snd_mixer_element_pre_effect1_info_item *pitems;
	/* user parameters */
	int parameters_size;
	int parameters;
	int parameters_over;
	struct snd_mixer_element_pre_effect1_info_parameter *pparameters;
};

struct snd_mixer_element_pre_effect1 {
	int item;			/* chose item index or -1 = user parameters */
	int parameters_size;
	int parameters;
	int parameters_over;
	int *pparameters;
};

/*
 *
 */

typedef struct snd_mixer_element_info {
	snd_mixer_eid_t eid;
	union {
		struct snd_mixer_element_io_info io;
		struct snd_mixer_element_pcm_info pcm;
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
		struct snd_mixer_element_3d_effect1 teffect1;
		struct snd_mixer_element_pre_effect1 peffect1;
		char reserve[120];
	} data;
} snd_mixer_element_t;

/* ioctl commands */
#define SND_MIXER_IOCTL_PVERSION	_IOR ('R', 0x00, int)
#define SND_MIXER_IOCTL_INFO		_IOWR('R', 0x01, snd_mixer_info_t)
#define SND_MIXER_IOCTL_ELEMENTS	_IOWR('R', 0x10, snd_mixer_elements_t)
#define SND_MIXER_IOCTL_ROUTES		_IOWR('R', 0x11, snd_mixer_routes_t)
#define SND_MIXER_IOCTL_GROUPS		_IOWR('R', 0x12, snd_mixer_groups_t)
#define SND_MIXER_IOCTL_GROUP_READ	_IOWR('R', 0x13, snd_mixer_group_t)
#define SND_MIXER_IOCTL_GROUP_WRITE	_IOWR('R', 0x14, snd_mixer_group_t)
#define SND_MIXER_IOCTL_ELEMENT_INFO	_IOWR('R', 0x20, snd_mixer_element_info_t)
#define SND_MIXER_IOCTL_ELEMENT_READ	_IOWR('R', 0x21, snd_mixer_element_t)
#define SND_MIXER_IOCTL_ELEMENT_WRITE	_IOWR('R', 0x22, snd_mixer_element_t)

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
		char channel_name[32];	/* channel name */
		unsigned char data8[60];
	} data;
} snd_mixer_read_t;

/*
 *  Obsolete interface compatible with Open Sound System API
 */

#ifdef __SND_OSS_COMPAT__

#define SND_MIXER_OSS_CAP_EXCL_INPUT	0x00000001	/* only one recording source at moment */

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

#define SND_MIXER_OSS_SET_RECSRC _IOWR( 'M', 255, int )
#define SND_MIXER_OSS_RECSRC	_IOR ( 'M', 255, int )
#define SND_MIXER_OSS_DEVMASK	_IOR ( 'M', 254, int )
#define SND_MIXER_OSS_RECMASK	_IOR ( 'M', 253, int )
#define SND_MIXER_OSS_CAPS	_IOR ( 'M', 252, int )
#define SND_MIXER_OSS_STEREODEVS _IOR ( 'M', 251, int )
#define SND_MIXER_OSS_INFO      _IOR ( 'M', 101, struct snd_oss_mixer_info )
#define SND_MIXER_OSS_OLD_INFO	_IOR ( 'M', 101, struct snd_oss_mixer_info_obsolete )
#define SND_OSS_GETVERSION	_IOR ( 'M', 118, int )

#endif				/* __SND_OSS_COMPAT__ */

/*****************************************************************************
 *                                                                           *
 *             Digital Audio (PCM) interface - /dev/snd/pcm??                *
 *                                                                           *
 *****************************************************************************/

#define SND_PCM_VERSION			SND_PROTOCOL_VERSION(1, 0, 2)

#define SND_PCM_SFMT_MU_LAW		0
#define SND_PCM_SFMT_A_LAW		1
#define SND_PCM_SFMT_IMA_ADPCM		2
#define SND_PCM_SFMT_U8			3
#define SND_PCM_SFMT_S16_LE		4
#define SND_PCM_SFMT_S16_BE		5
#define SND_PCM_SFMT_S8			6
#define SND_PCM_SFMT_U16_LE		7
#define SND_PCM_SFMT_U16_BE		8
#define SND_PCM_SFMT_MPEG		9
#define SND_PCM_SFMT_GSM		10
#define SND_PCM_SFMT_S24_LE		11
#define SND_PCM_SFMT_S24_BE		12
#define SND_PCM_SFMT_U24_LE		13
#define SND_PCM_SFMT_U24_BE		14
#define SND_PCM_SFMT_S32_LE		15
#define SND_PCM_SFMT_S32_BE		16
#define SND_PCM_SFMT_U32_LE		17
#define SND_PCM_SFMT_U32_BE		18
#define SND_PCM_SFMT_FLOAT		19	/* 4-byte float, need specification!! */
#define SND_PCM_SFMT_FLOAT64		20	/* 8-byte float, need specification!! */
#define SND_PCM_SFMT_SPECIAL		31

#define SND_PCM_FMT_QUERY		0
#define SND_PCM_FMT_MU_LAW		(1 << SND_PCM_SFMT_MU_LAW)
#define SND_PCM_FMT_A_LAW		(1 << SND_PCM_SFMT_A_LAW)
#define SND_PCM_FMT_IMA_ADPCM		(1 << SND_PCM_SFMT_IMA_ADPCM)
#define SND_PCM_FMT_U8			(1 << SND_PCM_SFMT_U8)
#define SND_PCM_FMT_S16_LE		(1 << SND_PCM_SFMT_S16_LE)
#define SND_PCM_FMT_S16_BE		(1 << SND_PCM_SFMT_S16_BE)
#define SND_PCM_FMT_S8			(1 << SND_PCM_SFMT_S8)
#define SND_PCM_FMT_U16_LE		(1 << SND_PCM_SFMT_U16_LE)
#define SND_PCM_FMT_U16_BE		(1 << SND_PCM_SFMT_U16_BE)
#define SND_PCM_FMT_MPEG		(1 << SND_PCM_SFMT_MPEG)
#define SND_PCM_FMT_GSM			(1 << SND_PCM_SFMT_GSM)
#define SND_PCM_FMT_S24_LE		(1 << SND_PCM_SFMT_S24_LE)
#define SND_PCM_FMT_S24_BE		(1 << SND_PCM_SFMT_S24_BE)
#define SND_PCM_FMT_U24_LE		(1 << SND_PCM_SFMT_U24_LE)
#define SND_PCM_FMT_U24_BE		(1 << SND_PCM_SFMT_U24_BE)
#define SND_PCM_FMT_S32_LE		(1 << SND_PCM_SFMT_S32_LE)
#define SND_PCM_FMT_S32_BE		(1 << SND_PCM_SFMT_S32_BE)
#define SND_PCM_FMT_U32_LE		(1 << SND_PCM_SFMT_U32_LE)
#define SND_PCM_FMT_U32_BE		(1 << SND_PCM_SFMT_U32_BE)
#define SND_PCM_FMT_FLOAT		(1 << SND_PCM_SFMT_FLOAT)
#define SND_PCM_FMT_FLOAT64		(1 << SND_PCM_SFMT_FLOAT64)
#define SND_PCM_FMT_SPECIAL		(1 << SND_PCM_SFMT_SPECIAL)

#define SND_PCM_INFO_CODEC		0x00000001
#define SND_PCM_INFO_DSP		SND_PCM_INFO_CODEC
#define SND_PCM_INFO_MMAP		0x00000002	/* for compatibility with OSS, this flag shouldn't be used with native applications */
#define SND_PCM_INFO_PLAYBACK		0x00000100
#define SND_PCM_INFO_CAPTURE		0x00000200
#define SND_PCM_INFO_DUPLEX		0x00000400
#define SND_PCM_INFO_DUPLEX_LIMIT	0x00000800	/* rate for playback & record channels must be same!!! */
#define SND_PCM_INFO_DUPLEX_MONO	0x00001000	/* in duplex mode - only mono (one channel) is supported */

#define SND_PCM_PINFO_BATCH		0x00000001	/* double buffering */
#define SND_PCM_PINFO_8BITONLY		0x00000002	/* hardware supports only 8-bit samples, but driver does conversions from 16-bit to 8-bit */
#define SND_PCM_PINFO_16BITONLY		0x00000004	/* hardware supports only 16-bit samples, but driver does conversions from 8-bit to 16-bit */

#define SND_PCM_RINFO_BATCH		0x00000001	/* double buffering */
#define SND_PCM_RINFO_8BITONLY		0x00000002	/* hardware supports only 8-bit samples, but driver does conversions from 16-bit to 8-bit */
#define SND_PCM_RINFO_16BITONLY		0x00000004	/* hardware supports only 16-bit samples, but driver does conversions from 8-bit to 16-bit */
#define SND_PCM_RINFO_OVERRANGE		0x00010000	/* hardware supports ADC overrange detection */

/*
 * Things to know:
 *   1) Real fragment size can be aligned by driver if hardware needs.
 *      Current fragment value can be taken from the status structure.
 *   2) If fragments_max in the playback_params structure is -N, the value
 *      means total fragments - N.
 */

typedef struct snd_pcm_info {
	unsigned int type;		/* soundcard type */
	unsigned int flags;		/* see to SND_PCM_INFO_XXXX */
	unsigned char id[32];		/* ID of this PCM device */
	unsigned char name[80];		/* name of this device */
	unsigned int playback;		/* playback subdevices - 1 */
	unsigned int capture;		/* capture subdevices - 1 */
	unsigned char reserved[56];	/* reserved for future... */
} snd_pcm_info_t;

typedef struct snd_pcm_playback_info {
	unsigned int flags;		/* see to SND_PCM_PINFO_XXXX */
	unsigned int formats;		/* supported formats */
	unsigned int min_rate;		/* min rate (in Hz) */
	unsigned int max_rate;		/* max rate (in Hz) */
	unsigned int min_channels;	/* min channels (probably always 1) */
	unsigned int max_channels;	/* max channels */
	unsigned int buffer_size;	/* playback buffer size */
	unsigned int min_fragment_size;	/* min fragment size in bytes */
	unsigned int max_fragment_size;	/* max fragment size in bytes */
	unsigned int fragment_align;	/* align fragment value */
	unsigned int hw_formats;	/* formats supported by hardware */
	unsigned int subdevice;		/* subdevice number */
	unsigned char reserved[56];	/* reserved for future... */
} snd_pcm_playback_info_t;

typedef struct snd_pcm_capture_info {
	unsigned int flags;		/* see to SND_PCM_RINFO_XXXX */
	unsigned int formats;		/* supported formats */
	unsigned int min_rate;		/* min rate (in Hz) */
	unsigned int max_rate;		/* max rate (in Hz) */
	unsigned int min_channels;	/* min channels (probably always 1) */
	unsigned int max_channels;	/* max channels */
	unsigned int buffer_size;	/* capture buffer size */
	unsigned int min_fragment_size;	/* min fragment size in bytes */
	unsigned int max_fragment_size;	/* max fragment size in bytes */
	unsigned int fragment_align;	/* align fragment value */
	unsigned int hw_formats;	/* formats supported by hardware */
	unsigned int subdevice;		/* subdevice number */
	unsigned char reserved[56];	/* reserved for future... */
} snd_pcm_capture_info_t;

typedef struct snd_pcm_format {
	unsigned int format;		/* SND_PCM_SFMT_XXXX */
	unsigned int rate;		/* rate in Hz */
	unsigned int channels;		/* channels (voices) */
	unsigned int special;		/* special description of format */
	unsigned char reserved[12];
} snd_pcm_format_t;

typedef struct snd_pcm_playback_params {
	int fragment_size;		/* requested size of fragment in bytes */
	int fragments_max;		/* maximum number of fragments in queue for wakeup */
	int fragments_room;		/* minimum number of fragments writeable for wakeup */
	unsigned char reserved[16];	/* must be filled with zero */
} snd_pcm_playback_params_t;

typedef struct snd_pcm_capture_params {
	int fragment_size;		/* requested size of fragment in bytes */
	int fragments_min;		/* minimum number of filled fragments for wakeup */
	unsigned char reserved[16];	/* must be filled with zero */
} snd_pcm_capture_params_t;

typedef struct snd_pcm_playback_status {
	unsigned int rate;	/* real used rate */
	int fragments;		/* allocated fragments */
	int fragment_size;	/* current fragment size in bytes */
	int count;		/* number of bytes writeable without blocking */
	int queue;		/* number of bytes in queue */
	int underrun;		/* count of underruns from last status */
	struct timeval time;	/* time the next write is going to play */
	struct timeval stime;	/* time when playback was started */
	int scount;		/* number of bytes processed from playback start (last underrun) */
	unsigned char reserved[16];
} snd_pcm_playback_status_t;

typedef struct snd_pcm_capture_status {
	unsigned int rate;	/* real used rate */
	int fragments;		/* allocated fragments */
	int fragment_size;	/* current fragment size in bytes */
	int count;		/* number of bytes readable without blocking */
	int free;		/* bytes in buffer still free */
	int overrun;		/* count of overruns from last status */
	struct timeval time;	/* time the next read was taken */
	struct timeval stime;	/* time when capture was started */
	int scount;		/* number of bytes processed from capture start */
	int overrange;		/* ADC overrange detection */
	unsigned char reserved[12];
} snd_pcm_capture_status_t;

#define SND_PCM_IOCTL_PVERSION		_IOR ('A', 0x00, int)
#define SND_PCM_IOCTL_INFO		_IOR ('A', 0x01, snd_pcm_info_t)
#define SND_PCM_IOCTL_PLAYBACK_INFO	_IOR ('A', 0x02, snd_pcm_playback_info_t)
#define SND_PCM_IOCTL_CAPTURE_INFO	_IOR ('A', 0x03, snd_pcm_capture_info_t)
#define SND_PCM_IOCTL_PLAYBACK_FORMAT	_IOWR('A', 0x10, snd_pcm_format_t)
#define SND_PCM_IOCTL_CAPTURE_FORMAT	_IOWR('A', 0x11, snd_pcm_format_t)
#define SND_PCM_IOCTL_PLAYBACK_PARAMS	_IOWR('A', 0x12, snd_pcm_playback_params_t)
#define SND_PCM_IOCTL_CAPTURE_PARAMS	_IOWR('A', 0x13, snd_pcm_capture_params_t)
#define SND_PCM_IOCTL_PLAYBACK_STATUS	_IOR ('A', 0x20, snd_pcm_playback_status_t)
#define SND_PCM_IOCTL_CAPTURE_STATUS	_IOR ('A', 0x21, snd_pcm_capture_status_t)
#define SND_PCM_IOCTL_DRAIN_PLAYBACK	_IO  ('A', 0x30)
#define SND_PCM_IOCTL_FLUSH_PLAYBACK	_IO  ('A', 0x31)
#define SND_PCM_IOCTL_FLUSH_CAPTURE	_IO  ('A', 0x32)
#define SND_PCM_IOCTL_PLAYBACK_PAUSE	_IOWR('A', 0x33, int)
#define SND_PCM_IOCTL_PLAYBACK_TIME	_IOWR('A', 0x40, int)
#define SND_PCM_IOCTL_CAPTURE_TIME	_IOWR('A', 0x41, int)

/*
 *  Loopback interface
 */

#define SND_PCM_LB_VERSION		SND_PROTOCOL_VERSION(1, 0, 0)

#define SND_PCM_LB_STREAM_MODE_RAW	0
#define SND_PCM_LB_STREAM_MODE_PACKET	1

#define SND_PCM_LB_TYPE_DATA		0	/* sample data */
#define SND_PCM_LB_TYPE_FORMAT		1	/* format change */

typedef struct snd_pcm_loopback_header {
	unsigned int size;		/* block size */
	unsigned int type;		/* block type (SND_PCM_LB_TYPE_*) */
} snd_pcm_loopback_header_t;

#define SND_PCM_LB_IOCTL_PVERSION	_IOR ( 'L', 0x00, int )
#define SND_PCM_LB_IOCTL_STREAM_MODE	_IOWR( 'L', 0x01, int )
#define SND_PCM_LB_IOCTL_FORMAT		_IOR ( 'L', 0x02, struct snd_pcm_format )

/*
 *  Obsolete interface compatible with Open Sound System API
 */

#ifdef __SND_OSS_COMPAT__

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

#define SND_RAWMIDI_VERSION		SND_PROTOCOL_VERSION(1, 0, 0)

#define SND_RAWMIDI_INFO_OUTPUT		0x00000001
#define SND_RAWMIDI_INFO_INPUT		0x00000002
#define SND_RAWMIDI_INFO_DUPLEX		0x00000004

typedef struct snd_rawmidi_info {
	unsigned int type;		/* soundcard type */
	unsigned int flags;		/* SND_RAWMIDI_INFO_XXXX */
	unsigned char id[32];		/* ID of this raw midi device */
	unsigned char name[80];		/* name of this raw midi device */
	unsigned char reserved[64];	/* reserved for future use */
} snd_rawmidi_info_t;

typedef struct snd_rawmidi_output_info {
	unsigned int switches;	/* count of switches */
	unsigned char reserved[64];
} snd_rawmidi_output_info_t;

typedef struct snd_rawmidi_input_info {
	unsigned int switches;	/* count of switches */
	unsigned char reserved[64];
} snd_rawmidi_input_info_t;

typedef struct snd_rawmidi_output_params {
	int size;		/* requested queue size in bytes */
	int max;		/* maximum number of bytes in queue for wakeup */
	int room;		/* minumum number of bytes writeable for wakeup */
	unsigned char reserved[16];	/* reserved for future use */
} snd_rawmidi_output_params_t;

typedef struct snd_rawmidi_input_params {
	int size;		/* requested queue size in bytes */
	int min;		/* minimum number of bytes fragments for wakeup */
	unsigned char reserved[16];	/* reserved for future use */
} snd_rawmidi_input_params_t;

typedef struct snd_rawmidi_output_status {
	int size;		/* real queue size */
	int count;		/* number of bytes writeable without blocking */
	int queue;		/* number of bytes in queue */
	unsigned char reserved[16];	/* reserved for future use */
} snd_rawmidi_output_status_t;

typedef struct snd_rawmidi_input_status {
	int size;		/* real queue size */
	int count;		/* number of bytes readable without blocking */
	int free;		/* bytes in buffer still free */
	int overrun;		/* count of overruns from last status (in bytes) */
	unsigned char reserved[16];	/* reserved for future use */
} snd_rawmidi_input_status_t;

#define SND_RAWMIDI_IOCTL_PVERSION	_IOR ('W', 0x00, int)
#define SND_RAWMIDI_IOCTL_INFO		_IOR ('W', 0x01, snd_rawmidi_info_t)
#define SND_RAWMIDI_IOCTL_OUTPUT_INFO	_IOR ('W', 0x02, snd_rawmidi_output_info_t)
#define SND_RAWMIDI_IOCTL_INPUT_INFO	_IOR ('W', 0x03, snd_rawmidi_input_info_t)
#define SND_RAWMIDI_IOCTL_OUTPUT_PARAMS	_IOWR('W', 0x10, snd_rawmidi_output_params_t)
#define SND_RAWMIDI_IOCTL_INPUT_PARAMS	_IOWR('W', 0x11, snd_rawmidi_input_params_t)
#define SND_RAWMIDI_IOCTL_OUTPUT_STATUS	_IOR ('W', 0x20, snd_rawmidi_output_status_t)
#define SND_RAWMIDI_IOCTL_INPUT_STATUS	_IOW ('W', 0x21, snd_rawmidi_input_status_t)
#define SND_RAWMIDI_IOCTL_DRAIN_OUTPUT	_IO  ('W', 0x30)
#define SND_RAWMIDI_IOCTL_FLUSH_OUTPUT  _IO  ('W', 0x31)
#define SND_RAWMIDI_IOCTL_FLUSH_INPUT	_IO  ('W', 0x32)

/*
 *  Timer section - /dev/snd/timer
 */

#define SND_TIMER_VERSION		SND_PROTOCOL_VERSION(1, 0, 0)

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
	char id[32];			/* timer identificator */
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

typedef struct snd_timer_status {
	unsigned long resolution;	/* current resolution */
	unsigned long lost;		/* counter of master tick lost */
	unsigned long overrun;		/* count of read queue overruns */
	int queue_size;			/* total queue size */
	int queue;			/* used queue size */
	char reserved[64];
} snd_timer_status_t;

#define SND_TIMER_IOCTL_PVERSION	_IOR ('T', 0x00, int)
#define SND_TIMER_IOCTL_GINFO		_IOW ('T', 0x01, snd_timer_general_info_t)
#define SND_TIMER_IOCTL_SELECT		_IOW ('T', 0x10, snd_timer_select_t)
#define SND_TIMER_IOCTL_INFO		_IOR ('T', 0x11, snd_timer_info_t)
#define SND_TIMER_IOCTL_PARAMS		_IOW ('T', 0x12, snd_timer_params_t)
#define SND_TIMER_IOCTL_STATUS		_IOW ('T', 0x13, snd_timer_status_t)
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

#endif				/* __ASOUND_H */
