/******************************************************************************

    AudioScience HPI driver
    Copyright (C) 1997-2003  AudioScience Inc. <support@audioscience.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of version 2 of the GNU General Public License as
    published by the Free Software Foundation;

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
/** \file hpi.h

 AudioScience Hardware Programming Interface (HPI)
 public API definition.

 The HPI is a low-level hardware abstraction layer to all
 AudioScience digital audio adapters
*/
/*
 You must define one operating systems that the HPI is to be compiled under
 HPI_OS_WIN32_USER        32bit Windows
 HPI_OS_DSP_C6000         DSP TI C6000 environment
 HPI_OS_WDM                       Windows WDM kernel driver
 HPI_OS_LINUX             Linux kernel driver

(C) Copyright AudioScience Inc. 1998-2010
******************************************************************************/
#ifndef _HPI_H_
#define _HPI_H_
/* HPI Version
If HPI_VER_MINOR is odd then its a development release not intended for the
public. If HPI_VER_MINOR is even then is a release version
i.e 3.05.02 is a development version
*/
#define HPI_VERSION_CONSTRUCTOR(maj, min, rel) \
	((maj << 16) + (min << 8) + rel)

#define HPI_VER_MAJOR(v) ((int)(v >> 16))
#define HPI_VER_MINOR(v) ((int)((v >> 8) & 0xFF))
#define HPI_VER_RELEASE(v) ((int)(v & 0xFF))

/* Use single digits for versions less that 10 to avoid octal. */
#define HPI_VER HPI_VERSION_CONSTRUCTOR(4L, 3, 10)

/* Library version as documented in hpi-api-versions.txt */
#define HPI_LIB_VER  HPI_VERSION_CONSTRUCTOR(3, 5, 0)

#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

#include <linux/types.h>
#define HPI_EXCLUDE_DEPRECATED

/******************************************************************************/
/******************************************************************************/
/********       HPI API DEFINITIONS                                       *****/
/******************************************************************************/
/******************************************************************************/
/*******************************************/
/**  Audio format types
\ingroup stream
*/
enum HPI_FORMATS {
/** Used internally on adapter. */
	HPI_FORMAT_MIXER_NATIVE = 0,
/** 8-bit unsigned PCM. Windows equivalent is WAVE_FORMAT_PCM. */
	HPI_FORMAT_PCM8_UNSIGNED = 1,
/** 16-bit signed PCM. Windows equivalent is WAVE_FORMAT_PCM. */
	HPI_FORMAT_PCM16_SIGNED = 2,
/** MPEG-1 Layer-1. */
	HPI_FORMAT_MPEG_L1 = 3,
/** MPEG-1 Layer-2.

Windows equivalent is WAVE_FORMAT_MPEG.

The following table shows what combinations of mode and bitrate are possible:

<table border=1 cellspacing=0 cellpadding=5>
<tr>
<td><p><b>Bitrate (kbs)</b></p>
<td><p><b>Mono</b></p>
<td><p><b>Stereo,<br>Joint Stereo or<br>Dual Channel</b></p>

<tr><td>32<td>X<td>_
<tr><td>40<td>_<td>_
<tr><td>48<td>X<td>_
<tr><td>56<td>X<td>_
<tr><td>64<td>X<td>X
<tr><td>80<td>X<td>_
<tr><td>96<td>X<td>X
<tr><td>112<td>X<td>X
<tr><td>128<td>X<td>X
<tr><td>160<td>X<td>X
<tr><td>192<td>X<td>X
<tr><td>224<td>_<td>X
<tr><td>256<td>-<td>X
<tr><td>320<td>-<td>X
<tr><td>384<td>_<td>X
</table>
*/
	HPI_FORMAT_MPEG_L2 = 4,
/** MPEG-1 Layer-3.
Windows equivalent is WAVE_FORMAT_MPEG.

The following table shows what combinations of mode and bitrate are possible:

<table border=1 cellspacing=0 cellpadding=5>
<tr>
<td><p><b>Bitrate (kbs)</b></p>
<td><p><b>Mono<br>Stereo @ 8,<br>11.025 and<br>12kHz*</b></p>
<td><p><b>Mono<br>Stereo @ 16,<br>22.050 and<br>24kHz*</b></p>
<td><p><b>Mono<br>Stereo @ 32,<br>44.1 and<br>48kHz</b></p>

<tr><td>16<td>X<td>X<td>_
<tr><td>24<td>X<td>X<td>_
<tr><td>32<td>X<td>X<td>X
<tr><td>40<td>X<td>X<td>X
<tr><td>48<td>X<td>X<td>X
<tr><td>56<td>X<td>X<td>X
<tr><td>64<td>X<td>X<td>X
<tr><td>80<td>_<td>X<td>X
<tr><td>96<td>_<td>X<td>X
<tr><td>112<td>_<td>X<td>X
<tr><td>128<td>_<td>X<td>X
<tr><td>144<td>_<td>X<td>_
<tr><td>160<td>_<td>X<td>X
<tr><td>192<td>_<td>_<td>X
<tr><td>224<td>_<td>_<td>X
<tr><td>256<td>-<td>_<td>X
<tr><td>320<td>-<td>_<td>X
</table>
\b * Available on the ASI6000 series only
*/
	HPI_FORMAT_MPEG_L3 = 5,
/** Dolby AC-2. */
	HPI_FORMAT_DOLBY_AC2 = 6,
/** Dolbt AC-3. */
	HPI_FORMAT_DOLBY_AC3 = 7,
/** 16-bit PCM big-endian. */
	HPI_FORMAT_PCM16_BIGENDIAN = 8,
/** TAGIT-1 algorithm - hits. */
	HPI_FORMAT_AA_TAGIT1_HITS = 9,
/** TAGIT-1 algorithm - inserts. */
	HPI_FORMAT_AA_TAGIT1_INSERTS = 10,
/** 32-bit signed PCM. Windows equivalent is WAVE_FORMAT_PCM.
Each sample is a 32bit word. The most significant 24 bits contain a 24-bit
sample and the least significant 8 bits are set to 0.
*/
	HPI_FORMAT_PCM32_SIGNED = 11,
/** Raw bitstream - unknown format. */
	HPI_FORMAT_RAW_BITSTREAM = 12,
/** TAGIT-1 algorithm hits - extended. */
	HPI_FORMAT_AA_TAGIT1_HITS_EX1 = 13,
/** 32-bit PCM as an IEEE float. Windows equivalent is WAVE_FORMAT_IEEE_FLOAT.
Each sample is a 32bit word in IEEE754 floating point format.
The range is +1.0 to -1.0, which corresponds to digital fullscale.
*/
	HPI_FORMAT_PCM32_FLOAT = 14,
/** 24-bit PCM signed. Windows equivalent is WAVE_FORMAT_PCM. */
	HPI_FORMAT_PCM24_SIGNED = 15,
/** OEM format 1 - private. */
	HPI_FORMAT_OEM1 = 16,
/** OEM format 2 - private. */
	HPI_FORMAT_OEM2 = 17,
/** Undefined format. */
	HPI_FORMAT_UNDEFINED = 0xffff
};

/******************************************* bus types */
enum HPI_BUSES {
	HPI_BUS_ISAPNP = 1,
	HPI_BUS_PCI = 2,
	HPI_BUS_USB = 3
};
/******************************************* in/out Stream states */
/*******************************************/
/** Stream States
\ingroup stream
*/
enum HPI_STREAM_STATES {
	/** State stopped - stream is stopped. */
	HPI_STATE_STOPPED = 1,
	/** State playing - stream is playing audio. */
	HPI_STATE_PLAYING = 2,
	/** State recording - stream is recording. */
	HPI_STATE_RECORDING = 3,
	/** State drained - playing stream ran out of data to play. */
	HPI_STATE_DRAINED = 4,
	/** State generate sine - to be implemented. */
	HPI_STATE_SINEGEN = 5,
	/** State wait - used for inter-card sync to mean waiting for all
		cards to be ready. */
	HPI_STATE_WAIT = 6
};
/******************************************* mixer source node types */
/** Source node types
\ingroup mixer
*/
enum HPI_SOURCENODES {
	/** This define can be used instead of 0 to indicate
	that there is no valid source node. A control that
	exists on a destination node can be searched for using a source
	node value of either 0, or HPI_SOURCENODE_NONE */
	HPI_SOURCENODE_NONE = 100,
	/** \deprecated Use HPI_SOURCENODE_NONE instead. */
	HPI_SOURCENODE_BASE = 100,
	/** Out Stream (Play) node. */
	HPI_SOURCENODE_OSTREAM = 101,
	/** Line in node - could be analog, AES/EBU or network. */
	HPI_SOURCENODE_LINEIN = 102,
	HPI_SOURCENODE_AESEBU_IN = 103,	     /**< AES/EBU input node. */
	HPI_SOURCENODE_TUNER = 104,	     /**< Tuner node. */
	HPI_SOURCENODE_RF = 105,	     /**< RF input node. */
	HPI_SOURCENODE_CLOCK_SOURCE = 106,   /**< Clock source node. */
	HPI_SOURCENODE_RAW_BITSTREAM = 107,  /**< Raw bitstream node. */
	HPI_SOURCENODE_MICROPHONE = 108,     /**< Microphone node. */
	/** Cobranet input node -
	    Audio samples come from the Cobranet network and into the device. */
	HPI_SOURCENODE_COBRANET = 109,
	HPI_SOURCENODE_ANALOG = 110,	     /**< Analog input node. */
	HPI_SOURCENODE_ADAPTER = 111,	     /**< Adapter node. */
	/* !!!Update this  AND hpidebug.h if you add a new sourcenode type!!! */
	HPI_SOURCENODE_LAST_INDEX = 111	     /**< largest ID */
		/* AX6 max sourcenode types = 15 */
};

/******************************************* mixer dest node types */
/** Destination node types
\ingroup mixer
*/
enum HPI_DESTNODES {
	/** This define can be used instead of 0 to indicate
	that there is no valid destination node. A control that
	exists on a source node can be searched for using a destination
	node value of either 0, or HPI_DESTNODE_NONE */
	HPI_DESTNODE_NONE = 200,
	/** \deprecated Use HPI_DESTNODE_NONE instead. */
	HPI_DESTNODE_BASE = 200,
	/** In Stream (Record) node. */
	HPI_DESTNODE_ISTREAM = 201,
	HPI_DESTNODE_LINEOUT = 202,	    /**< Line Out node. */
	HPI_DESTNODE_AESEBU_OUT = 203,	     /**< AES/EBU output node. */
	HPI_DESTNODE_RF = 204,		     /**< RF output node. */
	HPI_DESTNODE_SPEAKER = 205,	     /**< Speaker output node. */
	/** Cobranet output node -
	    Audio samples from the device are sent out on the Cobranet network.*/
	HPI_DESTNODE_COBRANET = 206,
	HPI_DESTNODE_ANALOG = 207,	     /**< Analog output node. */

	/* !!!Update this AND hpidebug.h if you add a new destnode type!!! */
	HPI_DESTNODE_LAST_INDEX = 207	     /**< largest ID */
		/* AX6 max destnode types = 15 */
};

/*******************************************/
/** Mixer control types
\ingroup mixer
*/
enum HPI_CONTROLS {
	HPI_CONTROL_GENERIC = 0,	/**< Generic control. */
	HPI_CONTROL_CONNECTION = 1, /**< A connection between nodes. */
	HPI_CONTROL_VOLUME = 2,	      /**< Volume control - works in dBFs. */
	HPI_CONTROL_METER = 3,	/**< Peak meter control. */
	HPI_CONTROL_MUTE = 4,	/*Mute control - not used at present. */
	HPI_CONTROL_MULTIPLEXER = 5,	/**< Multiplexer control. */

	HPI_CONTROL_AESEBU_TRANSMITTER = 6,	/**< AES/EBU transmitter control. */
	HPI_CONTROL_AESEBUTX = HPI_CONTROL_AESEBU_TRANSMITTER,

	HPI_CONTROL_AESEBU_RECEIVER = 7, /**< AES/EBU receiver control. */
	HPI_CONTROL_AESEBURX = HPI_CONTROL_AESEBU_RECEIVER,

	HPI_CONTROL_LEVEL = 8, /**< Level/trim control - works in dBu. */
	HPI_CONTROL_TUNER = 9,	/**< Tuner control. */
/*      HPI_CONTROL_ONOFFSWITCH =       10 */
	HPI_CONTROL_VOX = 11,	/**< Vox control. */
/*      HPI_CONTROL_AES18_TRANSMITTER = 12 */
/*      HPI_CONTROL_AES18_RECEIVER = 13 */
/*      HPI_CONTROL_AES18_BLOCKGENERATOR  = 14 */
	HPI_CONTROL_CHANNEL_MODE = 15,	/**< Channel mode control. */

	HPI_CONTROL_BITSTREAM = 16,	/**< Bitstream control. */
	HPI_CONTROL_SAMPLECLOCK = 17,	/**< Sample clock control. */
	HPI_CONTROL_MICROPHONE = 18,	/**< Microphone control. */
	HPI_CONTROL_PARAMETRIC_EQ = 19,	/**< Parametric EQ control. */
	HPI_CONTROL_EQUALIZER = HPI_CONTROL_PARAMETRIC_EQ,

	HPI_CONTROL_COMPANDER = 20,	/**< Compander control. */
	HPI_CONTROL_COBRANET = 21,	/**< Cobranet control. */
	HPI_CONTROL_TONEDETECTOR = 22,	/**< Tone detector control. */
	HPI_CONTROL_SILENCEDETECTOR = 23,	/**< Silence detector control. */
	HPI_CONTROL_PAD = 24,	/**< Tuner PAD control. */
	HPI_CONTROL_SRC = 25,	/**< Samplerate converter control. */
	HPI_CONTROL_UNIVERSAL = 26,	/**< Universal control. */

/*  !!! Update this AND hpidebug.h if you add a new control type!!!*/
	HPI_CONTROL_LAST_INDEX = 26 /**<Highest control type ID */
/* WARNING types 256 or greater impact bit packing in all AX6 DSP code */
};

/* Shorthand names that match attribute names */

/******************************************* ADAPTER ATTRIBUTES ****/

/** Adapter properties
These are used in HPI_AdapterSetProperty() and HPI_AdapterGetProperty()
\ingroup adapter
*/
enum HPI_ADAPTER_PROPERTIES {
/** \internal Used in dwProperty field of HPI_AdapterSetProperty() and
HPI_AdapterGetProperty(). This errata applies to all ASI6000 cards with both
analog and digital outputs. The CS4224 A/D+D/A has a one sample delay between
left and right channels on both its input (ADC) and output (DAC).
More details are available in Cirrus Logic errata ER284B2.
PDF available from www.cirrus.com, released by Cirrus in 2001.
*/
	HPI_ADAPTER_PROPERTY_ERRATA_1 = 1,

/** Adapter grouping property
Indicates whether the adapter supports the grouping API (for ASIO and SSX2)
*/
	HPI_ADAPTER_PROPERTY_GROUPING = 2,

/** Driver SSX2 property
Tells the kernel driver to turn on SSX2 stream mapping.
This feature is not used by the DSP. In fact the call is completely processed
by the driver and is not passed on to the DSP at all.
*/
	HPI_ADAPTER_PROPERTY_ENABLE_SSX2 = 3,

/** Adapter SSX2 property
Indicates the state of the adapter's SSX2 setting. This setting is stored in
non-volatile memory on the adapter. A typical call sequence would be to use
HPI_ADAPTER_PROPERTY_SSX2_SETTING to set SSX2 on the adapter and then to reload
the driver. The driver would query HPI_ADAPTER_PROPERTY_SSX2_SETTING during startup
and if SSX2 is set, it would then call HPI_ADAPTER_PROPERTY_ENABLE_SSX2 to enable
SSX2 stream mapping within the kernel level of the driver.
*/
	HPI_ADAPTER_PROPERTY_SSX2_SETTING = 4,

/** Base number for readonly properties */
	HPI_ADAPTER_PROPERTY_READONLYBASE = 256,

/** Readonly adapter latency property.
This property returns in the input and output latency in samples.
Property 1 is the estimated input latency
in samples, while Property 2 is that output latency in  samples.
*/
	HPI_ADAPTER_PROPERTY_LATENCY = 256,

/** Readonly adapter granularity property.
The granulariy is the smallest size chunk of stereo samples that is processed by
the adapter.
This property returns the record granularity in samples in Property 1.
Property 2 returns the play granularity.
*/
	HPI_ADAPTER_PROPERTY_GRANULARITY = 257,

/** Readonly adapter number of current channels property.
Property 1 is the number of record channels per record device.
Property 2 is the number of play channels per playback device.*/
	HPI_ADAPTER_PROPERTY_CURCHANNELS = 258,

/** Readonly adapter software version.
The SOFTWARE_VERSION property returns the version of the software running
on the adapter as Major.Minor.Release.
Property 1 contains Major in bits 15..8 and Minor in bits 7..0.
Property 2 contains Release in bits 7..0. */
	HPI_ADAPTER_PROPERTY_SOFTWARE_VERSION = 259,

/** Readonly adapter MAC address MSBs.
The MAC_ADDRESS_MSB property returns
the most significant 32 bits of the MAC address.
Property 1 contains bits 47..32 of the MAC address.
Property 2 contains bits 31..16 of the MAC address. */
	HPI_ADAPTER_PROPERTY_MAC_ADDRESS_MSB = 260,

/** Readonly adapter MAC address LSBs
The MAC_ADDRESS_LSB property returns
the least significant 16 bits of the MAC address.
Property 1 contains bits 15..0 of the MAC address. */
	HPI_ADAPTER_PROPERTY_MAC_ADDRESS_LSB = 261,

/** Readonly extended adapter type number
The EXTENDED_ADAPTER_TYPE property returns the 4 digits of an extended
adapter type, i.e ASI8920-0022, 0022 is the extended type.
The digits are returned as ASCII characters rather than the hex digits that
are returned for the main type
Property 1 returns the 1st two (left most) digits, i.e "00"
in the example above, the upper byte being the left most digit.
Property 2 returns the 2nd two digits, i.e "22" in the example above*/
	HPI_ADAPTER_PROPERTY_EXTENDED_ADAPTER_TYPE = 262,

/** Readonly debug log buffer information */
	HPI_ADAPTER_PROPERTY_LOGTABLEN = 263,
	HPI_ADAPTER_PROPERTY_LOGTABBEG = 264,

/** Readonly adapter IP address
For 192.168.1.101
Property 1 returns the 1st two (left most) digits, i.e 192*256 + 168
in the example above, the upper byte being the left most digit.
Property 2 returns the 2nd two digits, i.e 1*256 + 101 in the example above, */
	HPI_ADAPTER_PROPERTY_IP_ADDRESS = 265,

/** Readonly adapter buffer processed count. Returns a buffer processed count
that is incremented every time all buffers for all streams are updated. This
is useful for checking completion of all stream operations across the adapter
when using grouped streams.
*/
	HPI_ADAPTER_PROPERTY_BUFFER_UPDATE_COUNT = 266,

/** Readonly mixer and stream intervals

These intervals are  measured in mixer frames.
To convert to time, divide  by the adapter samplerate.

The mixer interval is the number of frames processed in one mixer iteration.
The stream update interval is the interval at which streams check for and
process data, and BBM host buffer counters are updated.

Property 1 is the mixer interval in mixer frames.
Property 2 is the stream update interval in mixer frames.
*/
	HPI_ADAPTER_PROPERTY_INTERVAL = 267,
/** Adapter capabilities 1
Property 1 - adapter can do multichannel (SSX1)
Property 2 - adapter can do stream grouping (supports SSX2)
*/
	HPI_ADAPTER_PROPERTY_CAPS1 = 268,
/** Adapter capabilities 2
Property 1 - adapter can do samplerate conversion (MRX)
Property 2 - adapter can do timestretch (TSX)
*/
	HPI_ADAPTER_PROPERTY_CAPS2 = 269
};

/** Adapter mode commands

Used in wQueryOrSet field of HPI_AdapterSetModeEx().
\ingroup adapter
*/
enum HPI_ADAPTER_MODE_CMDS {
	HPI_ADAPTER_MODE_SET = 0,
	HPI_ADAPTER_MODE_QUERY = 1
};

/** Adapter Modes
	These are used by HPI_AdapterSetModeEx()

\warning - more than 16 possible modes breaks
a bitmask in the Windows WAVE DLL
\ingroup adapter
*/
enum HPI_ADAPTER_MODES {
/** 4 outstream mode.
- ASI6114: 1 instream
- ASI6044: 4 instreams
- ASI6012: 1 instream
- ASI6102: no instreams
- ASI6022, ASI6122: 2 instreams
- ASI5111, ASI5101: 2 instreams
- ASI652x, ASI662x: 2 instreams
- ASI654x, ASI664x: 4 instreams
*/
	HPI_ADAPTER_MODE_4OSTREAM = 1,

/** 6 outstream mode.
- ASI6012: 1 instream,
- ASI6022, ASI6122: 2 instreams
- ASI652x, ASI662x: 4 instreams
*/
	HPI_ADAPTER_MODE_6OSTREAM = 2,

/** 8 outstream mode.
- ASI6114: 8 instreams
- ASI6118: 8 instreams
- ASI6585: 8 instreams
*/
	HPI_ADAPTER_MODE_8OSTREAM = 3,

/** 16 outstream mode.
- ASI6416 16 instreams
- ASI6518, ASI6618 16 instreams
- ASI6118 16 mono out and in streams
*/
	HPI_ADAPTER_MODE_16OSTREAM = 4,

/** one outstream mode.
- ASI5111 1 outstream, 1 instream
*/
	HPI_ADAPTER_MODE_1OSTREAM = 5,

/** ASI504X mode 1. 12 outstream, 4 instream 0 to 48kHz sample rates
	(see ASI504X datasheet for more info).
*/
	HPI_ADAPTER_MODE_1 = 6,

/** ASI504X mode 2. 4 outstreams, 4 instreams at 0 to 192kHz sample rates
	(see ASI504X datasheet for more info).
*/
	HPI_ADAPTER_MODE_2 = 7,

/** ASI504X mode 3. 4 outstreams, 4 instreams at 0 to 192kHz sample rates
	(see ASI504X datasheet for more info).
*/
	HPI_ADAPTER_MODE_3 = 8,

/** ASI504X multichannel mode.
	2 outstreams -> 4 line outs = 1 to 8 channel streams),
	4 lineins -> 1 instream (1 to 8 channel streams) at 0-48kHz.
	For more info see the SSX Specification.
*/
	HPI_ADAPTER_MODE_MULTICHANNEL = 9,

/** 12 outstream mode.
- ASI6514, ASI6614: 2 instreams
- ASI6540,ASI6544: 8 instreams
- ASI6640,ASI6644: 8 instreams
*/
	HPI_ADAPTER_MODE_12OSTREAM = 10,

/** 9 outstream mode.
- ASI6044: 8 instreams
*/
	HPI_ADAPTER_MODE_9OSTREAM = 11,

/** mono mode.
- ASI6416: 16 outstreams/instreams
- ASI5402: 2 outstreams/instreams
*/
	HPI_ADAPTER_MODE_MONO = 12
};

/* Note, adapters can have more than one capability -
encoding as bitfield is recommended. */
#define HPI_CAPABILITY_NONE             (0)
#define HPI_CAPABILITY_MPEG_LAYER3      (1)

/* Set this equal to maximum capability index,
Must not be greater than 32 - see axnvdef.h */
#define HPI_CAPABILITY_MAX                      1
/* #define HPI_CAPABILITY_AAC              2 */

/******************************************* STREAM ATTRIBUTES ****/

/** MPEG Ancillary Data modes

The mode for the ancillary data insertion or extraction to operate in.
\ingroup stream
*/
enum HPI_MPEG_ANC_MODES {
	/** the MPEG frames have energy information stored in them (5 bytes per stereo frame, 3 per mono) */
	HPI_MPEG_ANC_HASENERGY = 0,
	/** the entire ancillary data field is taken up by data from the Anc data buffer
	On encode, the encoder will insert the energy bytes before filling the remainder
	of the ancillary data space with data from the ancillary data buffer.
	*/
	HPI_MPEG_ANC_RAW = 1
};

/** Ancillary Data Alignment
\ingroup instream
*/
enum HPI_ISTREAM_MPEG_ANC_ALIGNS {
	/** data is packed against the end of data, then padded to the end of frame */
	HPI_MPEG_ANC_ALIGN_LEFT = 0,
	/** data is packed against the end of the frame */
	HPI_MPEG_ANC_ALIGN_RIGHT = 1
};

/** MPEG modes
MPEG modes - can be used optionally for HPI_FormatCreate()
parameter dwAttributes.

Using any mode setting other than HPI_MPEG_MODE_DEFAULT
with single channel format will return an error.
\ingroup stream
*/
enum HPI_MPEG_MODES {
/** Causes the MPEG-1 Layer II bitstream to be recorded
in single_channel mode when the number of channels is 1 and in stereo when the
number of channels is 2. */
	HPI_MPEG_MODE_DEFAULT = 0,
	/** Standard stereo without joint-stereo compression */
	HPI_MPEG_MODE_STEREO = 1,
	/** Joint stereo  */
	HPI_MPEG_MODE_JOINTSTEREO = 2,
	/** Left and Right channels are completely independent */
	HPI_MPEG_MODE_DUALCHANNEL = 3
};
/******************************************* MIXER ATTRIBUTES ****/

/* \defgroup mixer_flags Mixer flags for HPI_MIXER_GET_CONTROL_MULTIPLE_VALUES
{
*/
#define HPI_MIXER_GET_CONTROL_MULTIPLE_CHANGED  (0)
#define HPI_MIXER_GET_CONTROL_MULTIPLE_RESET    (1)
/*}*/

/** Commands used by HPI_MixerStore()
\ingroup mixer
*/
enum HPI_MIXER_STORE_COMMAND {
/** Save all mixer control settings. */
	HPI_MIXER_STORE_SAVE = 1,
/** Restore all controls from saved. */
	HPI_MIXER_STORE_RESTORE = 2,
/** Delete saved control settings. */
	HPI_MIXER_STORE_DELETE = 3,
/** Enable auto storage of some control settings. */
	HPI_MIXER_STORE_ENABLE = 4,
/** Disable auto storage of some control settings. */
	HPI_MIXER_STORE_DISABLE = 5,
/** Save the attributes of a single control. */
	HPI_MIXER_STORE_SAVE_SINGLE = 6
};

/************************************* CONTROL ATTRIBUTE VALUES ****/
/** Used by mixer plugin enable functions

E.g. HPI_ParametricEQ_SetState()
\ingroup mixer
*/
enum HPI_SWITCH_STATES {
	HPI_SWITCH_OFF = 0,	/**< Turn the mixer plugin on. */
	HPI_SWITCH_ON = 1	/**< Turn the mixer plugin off. */
};

/* Volume control special gain values */
/** volumes units are 100ths of a dB
\ingroup volume
*/
#define HPI_UNITS_PER_dB                100
/** turns volume control OFF or MUTE
\ingroup volume
*/
#define HPI_GAIN_OFF                    (-100 * HPI_UNITS_PER_dB)

/** value returned for no signal
\ingroup meter
*/
#define HPI_METER_MINIMUM               (-150 * HPI_UNITS_PER_dB)

/** autofade profiles
\ingroup volume
*/
enum HPI_VOLUME_AUTOFADES {
/** log fade - dB attenuation changes linearly over time */
	HPI_VOLUME_AUTOFADE_LOG = 2,
/** linear fade - amplitude changes linearly */
	HPI_VOLUME_AUTOFADE_LINEAR = 3
};

/** The physical encoding format of the AESEBU I/O.

Used in HPI_AESEBU_Transmitter_SetFormat(), HPI_AESEBU_Receiver_SetFormat()
along with related Get and Query functions
\ingroup aestx
*/
enum HPI_AESEBU_FORMATS {
/** AES/EBU physical format - AES/EBU balanced "professional"  */
	HPI_AESEBU_FORMAT_AESEBU = 1,
/** AES/EBU physical format - S/PDIF unbalanced "consumer"      */
	HPI_AESEBU_FORMAT_SPDIF = 2
};

/** AES/EBU error status bits

Returned by HPI_AESEBU_Receiver_GetErrorStatus()
\ingroup aesrx
*/
enum HPI_AESEBU_ERRORS {
/**  bit0: 1 when PLL is not locked */
	HPI_AESEBU_ERROR_NOT_LOCKED = 0x01,
/**  bit1: 1 when signal quality is poor */
	HPI_AESEBU_ERROR_POOR_QUALITY = 0x02,
/** bit2: 1 when there is a parity error */
	HPI_AESEBU_ERROR_PARITY_ERROR = 0x04,
/**  bit3: 1 when there is a bi-phase coding violation */
	HPI_AESEBU_ERROR_BIPHASE_VIOLATION = 0x08,
/**  bit4: 1 when the validity bit is high */
	HPI_AESEBU_ERROR_VALIDITY = 0x10,
/**  bit5: 1 when the CRC error bit is high */
	HPI_AESEBU_ERROR_CRC = 0x20
};

/** \addtogroup pad
\{
*/
/** The text string containing the station/channel combination. */
#define HPI_PAD_CHANNEL_NAME_LEN        16
/** The text string containing the artist. */
#define HPI_PAD_ARTIST_LEN              64
/** The text string containing the title. */
#define HPI_PAD_TITLE_LEN               64
/** The text string containing the comment. */
#define HPI_PAD_COMMENT_LEN             256
/** The PTY when the tuner has not recieved any PTY. */
#define HPI_PAD_PROGRAM_TYPE_INVALID    0xffff
/** \} */

/** Data types for PTY string translation.
\ingroup rds
*/
enum eHPI_RDS_type {
	HPI_RDS_DATATYPE_RDS = 0,	/**< RDS bitstream.*/
	HPI_RDS_DATATYPE_RBDS = 1	/**< RBDS bitstream.*/
};

/** Tuner bands

Used for HPI_Tuner_SetBand(),HPI_Tuner_GetBand()
\ingroup tuner
*/
enum HPI_TUNER_BAND {
	HPI_TUNER_BAND_AM = 1,	 /**< AM band */
	HPI_TUNER_BAND_FM = 2,	 /**< FM band (mono) */
	HPI_TUNER_BAND_TV_NTSC_M = 3,	 /**< NTSC-M TV band*/
	HPI_TUNER_BAND_TV = 3,	/* use TV_NTSC_M */
	HPI_TUNER_BAND_FM_STEREO = 4,	 /**< FM band (stereo) */
	HPI_TUNER_BAND_AUX = 5,	 /**< Auxiliary input */
	HPI_TUNER_BAND_TV_PAL_BG = 6,	 /**< PAL-B/G TV band*/
	HPI_TUNER_BAND_TV_PAL_I = 7,	 /**< PAL-I TV band*/
	HPI_TUNER_BAND_TV_PAL_DK = 8,	 /**< PAL-D/K TV band*/
	HPI_TUNER_BAND_TV_SECAM_L = 9,	 /**< SECAM-L TV band*/
	HPI_TUNER_BAND_LAST = 9	/**< The index of the last tuner band. */
};

/** Tuner mode attributes

Used by HPI_Tuner_SetMode(), HPI_Tuner_GetMode()
\ingroup tuner

*/
enum HPI_TUNER_MODES {
	HPI_TUNER_MODE_RSS = 1,	/**< Tuner mode attribute RSS */
	HPI_TUNER_MODE_RDS = 2	/**< Tuner mode attribute RBDS/RDS */
};

/** Tuner mode attribute values

Used by HPI_Tuner_SetMode(), HPI_Tuner_GetMode()
\ingroup tuner
*/
enum HPI_TUNER_MODE_VALUES {
/* RSS attribute values */
	HPI_TUNER_MODE_RSS_DISABLE = 0,	/**< Tuner mode attribute RSS disable */
	HPI_TUNER_MODE_RSS_ENABLE = 1,	/**< Tuner mode attribute RSS enable */

/* RDS mode attributes */
	HPI_TUNER_MODE_RDS_DISABLE = 0,	/**< Tuner mode attribute RDS - disabled */
	HPI_TUNER_MODE_RDS_RDS = 1,  /**< Tuner mode attribute RDS - RDS mode */
	HPI_TUNER_MODE_RDS_RBDS = 2 /**< Tuner mode attribute RDS - RBDS mode */
};

/** Tuner Level settings
\ingroup tuner
*/
enum HPI_TUNER_LEVEL {
	HPI_TUNER_LEVEL_AVERAGE = 0,
	HPI_TUNER_LEVEL_RAW = 1
};

/** Tuner Status Bits

These bitfield values are returned by a call to HPI_Tuner_GetStatus().
Multiple fields are returned from a single call.
\ingroup tuner
*/
enum HPI_TUNER_STATUS_BITS {
	HPI_TUNER_VIDEO_COLOR_PRESENT = 0x0001,	/**< Video color is present. */
	HPI_TUNER_VIDEO_IS_60HZ = 0x0020,	/**< 60 Hz video detected. */
	HPI_TUNER_VIDEO_HORZ_SYNC_MISSING = 0x0040,	/**< Video HSYNC is missing. */
	HPI_TUNER_VIDEO_STATUS_VALID = 0x0100,	/**< Video status is valid. */
	HPI_TUNER_PLL_LOCKED = 0x1000,		/**< The tuner's PLL is locked. */
	HPI_TUNER_FM_STEREO = 0x2000,		/**< Tuner reports back FM stereo. */
	HPI_TUNER_DIGITAL = 0x0200,		/**< Tuner reports digital programming. */
	HPI_TUNER_MULTIPROGRAM = 0x0400		/**< Tuner reports multiple programs. */
};

/** Channel Modes
Used for HPI_ChannelModeSet/Get()
\ingroup channelmode
*/
enum HPI_CHANNEL_MODES {
/** Left channel out = left channel in, Right channel out = right channel in. */
	HPI_CHANNEL_MODE_NORMAL = 1,
/** Left channel out = right channel in, Right channel out = left channel in. */
	HPI_CHANNEL_MODE_SWAP = 2,
/** Left channel out = left channel in, Right channel out = left channel in. */
	HPI_CHANNEL_MODE_LEFT_TO_STEREO = 3,
/** Left channel out = right channel in, Right channel out = right channel in.*/
	HPI_CHANNEL_MODE_RIGHT_TO_STEREO = 4,
/** Left channel out = (left channel in + right channel in)/2,
    Right channel out = mute. */
	HPI_CHANNEL_MODE_STEREO_TO_LEFT = 5,
/** Left channel out = mute,
    Right channel out = (right channel in + left channel in)/2. */
	HPI_CHANNEL_MODE_STEREO_TO_RIGHT = 6,
	HPI_CHANNEL_MODE_LAST = 6
};

/** SampleClock source values
\ingroup sampleclock
*/
enum HPI_SAMPLECLOCK_SOURCES {
/** The sampleclock output is derived from its local samplerate generator.
    The local samplerate may be set using HPI_SampleClock_SetLocalRate(). */
	HPI_SAMPLECLOCK_SOURCE_LOCAL = 1,
/** \deprecated Use HPI_SAMPLECLOCK_SOURCE_LOCAL instead */
	HPI_SAMPLECLOCK_SOURCE_ADAPTER = 1,
/** The adapter is clocked from a dedicated AES/EBU SampleClock input.*/
	HPI_SAMPLECLOCK_SOURCE_AESEBU_SYNC = 2,
/** From external wordclock connector */
	HPI_SAMPLECLOCK_SOURCE_WORD = 3,
/** Board-to-board header */
	HPI_SAMPLECLOCK_SOURCE_WORD_HEADER = 4,
/** FUTURE - SMPTE clock. */
	HPI_SAMPLECLOCK_SOURCE_SMPTE = 5,
/** One of the aesebu inputs */
	HPI_SAMPLECLOCK_SOURCE_AESEBU_INPUT = 6,
/** \deprecated The first aesebu input with a valid signal
Superseded by separate Auto enable flag
*/
	HPI_SAMPLECLOCK_SOURCE_AESEBU_AUTO = 7,
/** From a network interface e.g. Cobranet or Livewire at either 48 or 96kHz */
	HPI_SAMPLECLOCK_SOURCE_NETWORK = 8,
/** From previous adjacent module (ASI2416 only)*/
	HPI_SAMPLECLOCK_SOURCE_PREV_MODULE = 10,
/*! Update this if you add a new clock source.*/
	HPI_SAMPLECLOCK_SOURCE_LAST = 10
};

/** Equalizer filter types. Used by HPI_ParametricEQ_SetBand()
\ingroup parmeq
*/
enum HPI_FILTER_TYPE {
	HPI_FILTER_TYPE_BYPASS = 0,	/**< Filter is turned off */

	HPI_FILTER_TYPE_LOWSHELF = 1,	/**< EQ low shelf */
	HPI_FILTER_TYPE_HIGHSHELF = 2,	/**< EQ high shelf */
	HPI_FILTER_TYPE_EQ_BAND = 3,	/**< EQ gain */

	HPI_FILTER_TYPE_LOWPASS = 4,	/**< Standard low pass */
	HPI_FILTER_TYPE_HIGHPASS = 5,	/**< Standard high pass */
	HPI_FILTER_TYPE_BANDPASS = 6,	/**< Standard band pass */
	HPI_FILTER_TYPE_BANDSTOP = 7	/**< Standard band stop/notch */
};

/** Async Event sources
\ingroup async
*/
enum ASYNC_EVENT_SOURCES {
	HPI_ASYNC_EVENT_GPIO = 1,	/**< GPIO event. */
	HPI_ASYNC_EVENT_SILENCE = 2,	/**< Silence event detected. */
	HPI_ASYNC_EVENT_TONE = 3	/**< tone event detected. */
};
/*******************************************/
/** HPI Error codes

Almost all HPI functions return an error code
A return value of zero means there was no error.
Otherwise one of these error codes is returned.
Error codes can be converted to a descriptive string using HPI_GetErrorText()

\note When a new error code is added HPI_GetErrorText() MUST be updated.
\note Codes 1-100 are reserved for driver use
\ingroup utility
*/
enum HPI_ERROR_CODES {
	/** Message type does not exist. */
	HPI_ERROR_INVALID_TYPE = 100,
	/** Object type does not exist. */
	HPI_ERROR_INVALID_OBJ = 101,
	/** Function does not exist. */
	HPI_ERROR_INVALID_FUNC = 102,
	/** The specified object (adapter/Stream) does not exist. */
	HPI_ERROR_INVALID_OBJ_INDEX = 103,
	/** Trying to access an object that has not been opened yet. */
	HPI_ERROR_OBJ_NOT_OPEN = 104,
	/** Trying to open an already open object. */
	HPI_ERROR_OBJ_ALREADY_OPEN = 105,
	/** PCI, ISA resource not valid. */
	HPI_ERROR_INVALID_RESOURCE = 106,
	/** GetInfo call from SubSysFindAdapters failed. */
	HPI_ERROR_SUBSYSFINDADAPTERS_GETINFO = 107,
	/** Default response was never updated with actual error code. */
	HPI_ERROR_INVALID_RESPONSE = 108,
	/** wSize field of response was not updated,
	indicating that the message was not processed. */
	HPI_ERROR_PROCESSING_MESSAGE = 109,
	/** The network did not respond in a timely manner. */
	HPI_ERROR_NETWORK_TIMEOUT = 110,
	/** An HPI handle is invalid (uninitialised?). */
	HPI_ERROR_INVALID_HANDLE = 111,
	/** A function or attribute has not been implemented yet. */
	HPI_ERROR_UNIMPLEMENTED = 112,
	/** There are too many clients attempting to access a network resource. */
	HPI_ERROR_NETWORK_TOO_MANY_CLIENTS = 113,
	/** Response buffer passed to HPI_Message was smaller than returned response */
	HPI_ERROR_RESPONSE_BUFFER_TOO_SMALL = 114,

	/** Too many adapters.*/
	HPI_ERROR_TOO_MANY_ADAPTERS = 200,
	/** Bad adpater. */
	HPI_ERROR_BAD_ADAPTER = 201,
	/** Adapter number out of range or not set properly. */
	HPI_ERROR_BAD_ADAPTER_NUMBER = 202,
	/** 2 adapters with the same adapter number. */
	HPI_DUPLICATE_ADAPTER_NUMBER = 203,
	/** DSP code failed to bootload. */
	HPI_ERROR_DSP_BOOTLOAD = 204,
	/** Adapter failed DSP code self test. */
	HPI_ERROR_DSP_SELFTEST = 205,
	/** Couldn't find or open the DSP code file. */
	HPI_ERROR_DSP_FILE_NOT_FOUND = 206,
	/** Internal DSP hardware error. */
	HPI_ERROR_DSP_HARDWARE = 207,
	/** Could not allocate memory in DOS. */
	HPI_ERROR_DOS_MEMORY_ALLOC = 208,
	/** Could not allocate memory */
	HPI_ERROR_MEMORY_ALLOC = 208,
	/** Failed to correctly load/config PLD .*/
	HPI_ERROR_PLD_LOAD = 209,
	/** Unexpected end of file, block length too big etc. */
	HPI_ERROR_DSP_FILE_FORMAT = 210,

	/** Found but could not open DSP code file. */
	HPI_ERROR_DSP_FILE_ACCESS_DENIED = 211,
	/** First DSP code section header not found in DSP file. */
	HPI_ERROR_DSP_FILE_NO_HEADER = 212,
	/** File read operation on DSP code file failed. */
	HPI_ERROR_DSP_FILE_READ_ERROR = 213,
	/** DSP code for adapter family not found. */
	HPI_ERROR_DSP_SECTION_NOT_FOUND = 214,
	/** Other OS specific error opening DSP file. */
	HPI_ERROR_DSP_FILE_OTHER_ERROR = 215,
	/** Sharing violation opening DSP code file. */
	HPI_ERROR_DSP_FILE_SHARING_VIOLATION = 216,
	/** DSP code section header had size == 0. */
	HPI_ERROR_DSP_FILE_NULL_HEADER = 217,

	/** Base number for flash errors. */
	HPI_ERROR_FLASH = 220,

	/** Flash has bad checksum */
	HPI_ERROR_BAD_CHECKSUM = (HPI_ERROR_FLASH + 1),
	HPI_ERROR_BAD_SEQUENCE = (HPI_ERROR_FLASH + 2),
	HPI_ERROR_FLASH_ERASE = (HPI_ERROR_FLASH + 3),
	HPI_ERROR_FLASH_PROGRAM = (HPI_ERROR_FLASH + 4),
	HPI_ERROR_FLASH_VERIFY = (HPI_ERROR_FLASH + 5),
	HPI_ERROR_FLASH_TYPE = (HPI_ERROR_FLASH + 6),
	HPI_ERROR_FLASH_START = (HPI_ERROR_FLASH + 7),

	/** Reserved for OEMs. */
	HPI_ERROR_RESERVED_1 = 290,

	/** Stream does not exist. */
	HPI_ERROR_INVALID_STREAM = 300,
	/** Invalid compression format. */
	HPI_ERROR_INVALID_FORMAT = 301,
	/** Invalid format samplerate */
	HPI_ERROR_INVALID_SAMPLERATE = 302,
	/** Invalid format number of channels. */
	HPI_ERROR_INVALID_CHANNELS = 303,
	/** Invalid format bitrate. */
	HPI_ERROR_INVALID_BITRATE = 304,
	/** Invalid datasize used for stream read/write. */
	HPI_ERROR_INVALID_DATASIZE = 305,
	/** Stream buffer is full during stream write. */
	HPI_ERROR_BUFFER_FULL = 306,
	/** Stream buffer is empty during stream read. */
	HPI_ERROR_BUFFER_EMPTY = 307,
	/** Invalid datasize used for stream read/write. */
	HPI_ERROR_INVALID_DATA_TRANSFER = 308,
	/** Packet ordering error for stream read/write. */
	HPI_ERROR_INVALID_PACKET_ORDER = 309,

	/** Object can't do requested operation in its current
	state, eg set format, change rec mux state while recording.*/
	HPI_ERROR_INVALID_OPERATION = 310,

	/** Where an SRG is shared amongst streams, an incompatible samplerate is one
	that is different to any currently playing or recording stream. */
	HPI_ERROR_INCOMPATIBLE_SAMPLERATE = 311,
	/** Adapter mode is illegal.*/
	HPI_ERROR_BAD_ADAPTER_MODE = 312,

	/** There have been too many attempts to set the adapter's
	capabilities (using bad keys), the card should be returned
	to ASI if further capabilities updates are required */
	HPI_ERROR_TOO_MANY_CAPABILITY_CHANGE_ATTEMPTS = 313,
	/** Streams on different adapters cannot be grouped. */
	HPI_ERROR_NO_INTERADAPTER_GROUPS = 314,
	/** Streams on different DSPs cannot be grouped. */
	HPI_ERROR_NO_INTERDSP_GROUPS = 315,

	/** Invalid mixer node for this adapter. */
	HPI_ERROR_INVALID_NODE = 400,
	/** Invalid control. */
	HPI_ERROR_INVALID_CONTROL = 401,
	/** Invalid control value was passed. */
	HPI_ERROR_INVALID_CONTROL_VALUE = 402,
	/** Control attribute not supported by this control. */
	HPI_ERROR_INVALID_CONTROL_ATTRIBUTE = 403,
	/** Control is disabled. */
	HPI_ERROR_CONTROL_DISABLED = 404,
	/** I2C transaction failed due to a missing ACK. */
	HPI_ERROR_CONTROL_I2C_MISSING_ACK = 405,
	/** Control attribute is valid, but not supported by this hardware. */
	HPI_ERROR_UNSUPPORTED_CONTROL_ATTRIBUTE = 406,
	/** Control is busy, or coming out of
	reset and cannot be accessed at this time. */
	HPI_ERROR_CONTROL_NOT_READY = 407,

	/** Non volatile memory */
	HPI_ERROR_NVMEM_BUSY = 450,
	HPI_ERROR_NVMEM_FULL = 451,
	HPI_ERROR_NVMEM_FAIL = 452,

	/** I2C */
	HPI_ERROR_I2C_MISSING_ACK = HPI_ERROR_CONTROL_I2C_MISSING_ACK,
	HPI_ERROR_I2C_BAD_ADR = 460,

	/** Entity errors */
	HPI_ERROR_ENTITY_TYPE_MISMATCH = 470,
	HPI_ERROR_ENTITY_ITEM_COUNT = 471,
	HPI_ERROR_ENTITY_TYPE_INVALID = 472,
	HPI_ERROR_ENTITY_ROLE_INVALID = 473,

	/* AES18 specific errors were 500..507 */

	/** custom error to use for debugging */
	HPI_ERROR_CUSTOM = 600,

	/** hpioct32.c can't obtain mutex */
	HPI_ERROR_MUTEX_TIMEOUT = 700,

	/** errors from HPI backends have values >= this */
	HPI_ERROR_BACKEND_BASE = 900,

	/** indicates a cached u16 value is invalid. */
	HPI_ERROR_ILLEGAL_CACHE_VALUE = 0xffff
};

/** \defgroup maximums HPI maximum values
\{
*/
/** Maximum number of adapters per HPI sub-system
   WARNING: modifying this value changes the response structure size.*/
#define HPI_MAX_ADAPTERS                20
/** Maximum number of in or out streams per adapter */
#define HPI_MAX_STREAMS                 16
#define HPI_MAX_CHANNELS                2	/* per stream */
#define HPI_MAX_NODES                   8	/* per mixer ? */
#define HPI_MAX_CONTROLS                4	/* per node ? */
/** maximum number of ancillary bytes per MPEG frame */
#define HPI_MAX_ANC_BYTES_PER_FRAME     (64)
#define HPI_STRING_LEN                  16

/** Velocity units */
#define HPI_OSTREAM_VELOCITY_UNITS      4096
/** OutStream timescale units */
#define HPI_OSTREAM_TIMESCALE_UNITS     10000
/** OutStream timescale passthrough - turns timescaling on in passthough mode */
#define HPI_OSTREAM_TIMESCALE_PASSTHROUGH       99999

/**\}*/

/* ////////////////////////////////////////////////////////////////////// */
/* STRUCTURES */
#ifndef DISABLE_PRAGMA_PACK1
#pragma pack(push, 1)
#endif

/** Structure containing sample format information.
    See also HPI_FormatCreate().
  */
struct hpi_format {
	u32 dwSampleRate;
				/**< 11025, 32000, 44100 ... */
	u32 dwBitRate;	       /**< for MPEG */
	u32 dwAttributes;
				/**< Stereo/JointStereo/Mono */
	u16 wModeLegacy;
				/**< Legacy ancillary mode or idle bit  */
	u16 wUnused;	       /**< Unused */
	u16 wChannels; /**< 1,2..., (or ancillary mode or idle bit */
	u16 wFormat;   /**< HPI_FORMAT_PCM16, _MPEG etc. See #HPI_FORMATS. */
};

struct hpi_anc_frame {
	u32 dwValidBitsInThisFrame;
	u8 bData[HPI_MAX_ANC_BYTES_PER_FRAME];
};

/** An object for containing a single async event.
*/
struct hpi_async_event {
	u16 wEventType;	/**< Type of event. \sa async_event  */
	u16 wSequence;	/**< Sequence number, allows lost event detection */
	u32 dwState;	/**< New state */
	u32 hObject;	/**< Handle to the object returning the event. */
	union {
		struct {
			u16 wIndex; /**< GPIO bit index. */
		} gpio;
		struct {
			u16 wNodeIndex;	/**< What node is the control on ? */
			u16 wNodeType;	/**< What type of node is the control on ? */
		} control;
	} u;
};

/*/////////////////////////////////////////////////////////////////////////// */
/* Public HPI Entity related definitions                                     */

struct hpi_entity;

enum e_entity_type {
	entity_type_null,
	entity_type_sequence,	/* sequence of potentially heterogeneous TLV entities */

	entity_type_reference,	/* refers to a TLV entity or NULL */

	entity_type_int,	/* 32 bit */
	entity_type_float,	/* ieee754 binary 32 bit encoding */
	entity_type_double,

	entity_type_cstring,
	entity_type_octet,
	entity_type_ip4_address,
	entity_type_ip6_address,
	entity_type_mac_address,

	LAST_ENTITY_TYPE
};

enum e_entity_role {
	entity_role_null,
	entity_role_value,
	entity_role_classname,

	entity_role_units,
	entity_role_flags,
	entity_role_range,

	entity_role_mapping,
	entity_role_enum,

	entity_role_instance_of,
	entity_role_depends_on,
	entity_role_member_of_group,
	entity_role_value_constraint,
	entity_role_parameter_port,

	entity_role_block,
	entity_role_node_group,
	entity_role_audio_port,
	entity_role_clock_port,
	LAST_ENTITY_ROLE
};

/* skip host side function declarations for
   DSP compile and documentation extraction */

struct hpi_hsubsys {
	int not_really_used;
};

#ifndef DISABLE_PRAGMA_PACK1
#pragma pack(pop)
#endif

/*////////////////////////////////////////////////////////////////////////// */
/* HPI FUNCTIONS */

/*/////////////////////////// */
/* DATA and FORMAT and STREAM */

u16 HPI_StreamEstimateBufferSize(
	struct hpi_format *pF,
	u32 dwHostPollingRateInMilliSeconds,
	u32 *dwRecommendedBufferSize
);

/*/////////// */
/* SUB SYSTEM */
struct hpi_hsubsys *HPI_SubSysCreate(
	void
);

void HPI_SubSysFree(
	const struct hpi_hsubsys *phSubSys
);

u16 HPI_SubSysGetVersion(
	const struct hpi_hsubsys *phSubSys,
	u32 *pdwVersion
);

u16 HPI_SubSysGetVersionEx(
	const struct hpi_hsubsys *phSubSys,
	u32 *pdwVersionEx
);

u16 HPI_SubSysGetInfo(
	const struct hpi_hsubsys *phSubSys,
	u32 *pdwVersion,
	u16 *pwNumAdapters,
	u16 awAdapterList[],
	u16 wListLength
);

u16 HPI_SubSysFindAdapters(
	const struct hpi_hsubsys *phSubSys,
	u16 *pwNumAdapters,
	u16 awAdapterList[],
	u16 wListLength
);

u16 HPI_SubSysGetNumAdapters(
	const struct hpi_hsubsys *phSubSys,
	int *pnNumAdapters
);

u16 HPI_SubSysGetAdapter(
	const struct hpi_hsubsys *phSubSys,
	int nIterator,
	u32 *pdwAdapterIndex,
	u16 *pwAdapterType
);

u16 HPI_SubSysSsx2Bypass(
	const struct hpi_hsubsys *phSubSys,
	u16 wBypass
);

u16 HPI_SubSysSetHostNetworkInterface(
	const struct hpi_hsubsys *phSubSys,
	const char *szInterface
);

/*///////// */
/* ADAPTER */

u16 HPI_AdapterOpen(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex
);

u16 HPI_AdapterClose(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex
);

u16 HPI_AdapterGetInfo(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 *pwNumOutStreams,
	u16 *pwNumInStreams,
	u16 *pwVersion,
	u32 *pdwSerialNumber,
	u16 *pwAdapterType
);

u16 HPI_AdapterGetModuleByIndex(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wModuleIndex,
	u16 *pwNumOutputs,
	u16 *pwNumInputs,
	u16 *pwVersion,
	u32 *pdwSerialNumber,
	u16 *pwModuleType,
	u32 *phModule
);

u16 HPI_AdapterSetMode(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 dwAdapterMode
);
u16 HPI_AdapterSetModeEx(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 dwAdapterMode,
	u16 wQueryOrSet
);

u16 HPI_AdapterGetMode(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 *pdwAdapterMode
);

u16 HPI_AdapterGetAssert(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 *wAssertPresent,
	char *pszAssert,
	u16 *pwLineNumber
);

u16 HPI_AdapterGetAssertEx(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 *wAssertPresent,
	char *pszAssert,
	u32 *pdwLineNumber,
	u16 *pwAssertOnDsp
);

u16 HPI_AdapterTestAssert(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wAssertId
);

u16 HPI_AdapterEnableCapability(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wCapability,
	u32 dwKey
);

u16 HPI_AdapterSelfTest(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex
);

u16 HPI_AdapterDebugRead(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 dwDspAddress,
	char *pBytes,
	int *dwCountBytes
);

u16 HPI_AdapterSetProperty(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wProperty,
	u16 wParamter1,
	u16 wParamter2
);

u16 HPI_AdapterGetProperty(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wProperty,
	u16 *pwParamter1,
	u16 *pwParamter2
);

u16 HPI_AdapterEnumerateProperty(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wIndex,
	u16 wWhatToEnumerate,
	u16 wPropertyIndex,
	u32 *pdwSetting
);

/*////////////// */
/* NonVol Memory */
u16 HPI_NvMemoryOpen(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 *phNvMemory,
	u16 *pwSizeInBytes
);

u16 HPI_NvMemoryReadByte(
	const struct hpi_hsubsys *phSubSys,
	u32 hNvMemory,
	u16 wIndex,
	u16 *pwData
);

u16 HPI_NvMemoryWriteByte(
	const struct hpi_hsubsys *phSubSys,
	u32 hNvMemory,
	u16 wIndex,
	u16 wData
);

/*////////////// */
/* Digital I/O */
u16 HPI_GpioOpen(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 *phGpio,
	u16 *pwNumberInputBits,
	u16 *pwNumberOutputBits
);

u16 HPI_GpioReadBit(
	const struct hpi_hsubsys *phSubSys,
	u32 hGpio,
	u16 wBitIndex,
	u16 *pwBitData
);

u16 HPI_GpioReadAllBits(
	const struct hpi_hsubsys *phSubSys,
	u32 hGpio,
	u16 awAllBitData[4]
);

u16 HPI_GpioWriteBit(
	const struct hpi_hsubsys *phSubSys,
	u32 hGpio,
	u16 wBitIndex,
	u16 wBitData
);

u16 HPI_GpioWriteStatus(
	const struct hpi_hsubsys *phSubSys,
	u32 hGpio,
	u16 awAllBitData[4]
);

/**********************/
/* Async Event Object */
/**********************/
u16 HPI_AsyncEventOpen(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 *phAsync
);

u16 HPI_AsyncEventClose(
	const struct hpi_hsubsys *phSubSys,
	u32 hAsync
);

u16 HPI_AsyncEventWait(
	const struct hpi_hsubsys *phSubSys,
	u32 hAsync,
	u16 wMaximumEvents,
	struct hpi_async_event *pEvents,
	u16 *pwNumberReturned
);

u16 HPI_AsyncEventGetCount(
	const struct hpi_hsubsys *phSubSys,
	u32 hAsync,
	u16 *pwCount
);

u16 HPI_AsyncEventGet(
	const struct hpi_hsubsys *phSubSys,
	u32 hAsync,
	u16 wMaximumEvents,
	struct hpi_async_event *pEvents,
	u16 *pwNumberReturned
);

/*/////////// */
/* WATCH-DOG  */
u16 HPI_WatchdogOpen(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 *phWatchdog
);

u16 HPI_WatchdogSetTime(
	const struct hpi_hsubsys *phSubSys,
	u32 hWatchdog,
	u32 dwTimeMillisec
);

u16 HPI_WatchdogPing(
	const struct hpi_hsubsys *phSubSys,
	u32 hWatchdog
);

/**************/
/* OUT STREAM */
/**************/
u16 HPI_OutStreamOpen(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wOutStreamIndex,
	u32 *phOutStream
);

u16 HPI_OutStreamClose(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream
);

u16 HPI_OutStreamGetInfoEx(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	u16 *pwState,
	u32 *pdwBufferSize,
	u32 *pdwDataToPlay,
	u32 *pdwSamplesPlayed,
	u32 *pdwAuxiliaryDataToPlay
);

u16 HPI_OutStreamWriteBuf(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	const u8 *pbWriteBuf,
	u32 dwBytesToWrite,
	const struct hpi_format *pFormat
);

u16 HPI_OutStreamStart(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream
);

u16 HPI_OutStreamWaitStart(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream
);

u16 HPI_OutStreamStop(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream
);

u16 HPI_OutStreamSinegen(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream
);

u16 HPI_OutStreamReset(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream
);

u16 HPI_OutStreamQueryFormat(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	struct hpi_format *pFormat
);

u16 HPI_OutStreamSetFormat(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	struct hpi_format *pFormat
);

u16 HPI_OutStreamSetPunchInOut(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	u32 dwPunchInSample,
	u32 dwPunchOutSample
);

u16 HPI_OutStreamSetVelocity(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	short nVelocity
);

u16 HPI_OutStreamAncillaryReset(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	u16 wMode
);

u16 HPI_OutStreamAncillaryGetInfo(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	u32 *pdwFramesAvailable
);

u16 HPI_OutStreamAncillaryRead(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	struct hpi_anc_frame *pAncFrameBuffer,
	u32 dwAncFrameBufferSizeInBytes,
	u32 dwNumberOfAncillaryFramesToRead
);

u16 HPI_OutStreamSetTimeScale(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	u32 dwTimeScaleX10000
);

u16 HPI_OutStreamHostBufferAllocate(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	u32 dwSizeInBytes
);

u16 HPI_OutStreamHostBufferFree(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream
);

u16 HPI_OutStreamGroupAdd(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	u32 hStream
);

u16 HPI_OutStreamGroupGetMap(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	u32 *pdwOutStreamMap,
	u32 *pdwInStreamMap
);

u16 HPI_OutStreamGroupReset(
	const struct hpi_hsubsys *phSubSys,
	u32 hOutStream
);

/*////////// */
/* IN_STREAM */
u16 HPI_InStreamOpen(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wInStreamIndex,
	u32 *phInStream
);

u16 HPI_InStreamClose(
	const struct hpi_hsubsys *phSubSys,
	u32 hInStream
);

u16 HPI_InStreamQueryFormat(
	const struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	const struct hpi_format *pFormat
);

u16 HPI_InStreamSetFormat(
	const struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	const struct hpi_format *pFormat
);

u16 HPI_InStreamReadBuf(
	const struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	u8 *pbReadBuf,
	u32 dwBytesToRead
);

u16 HPI_InStreamStart(
	const struct hpi_hsubsys *phSubSys,
	u32 hInStream
);

u16 HPI_InStreamWaitStart(
	const struct hpi_hsubsys *phSubSys,
	u32 hInStream
);

u16 HPI_InStreamStop(
	const struct hpi_hsubsys *phSubSys,
	u32 hInStream
);

u16 HPI_InStreamReset(
	const struct hpi_hsubsys *phSubSys,
	u32 hInStream
);

u16 HPI_InStreamGetInfoEx(
	const struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	u16 *pwState,
	u32 *pdwBufferSize,
	u32 *pdwDataRecorded,
	u32 *pdwSamplesRecorded,
	u32 *pdwAuxiliaryDataRecorded
);

u16 HPI_InStreamAncillaryReset(
	const struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	u16 wBytesPerFrame,
	u16 wMode,
	u16 wAlignment,
	u16 wIdleBit
);

u16 HPI_InStreamAncillaryGetInfo(
	const struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	u32 *pdwFrameSpace
);

u16 HPI_InStreamAncillaryWrite(
	const struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	const struct hpi_anc_frame *pAncFrameBuffer,
	u32 dwAncFrameBufferSizeInBytes,
	u32 dwNumberOfAncillaryFramesToWrite
);

u16 HPI_InStreamHostBufferAllocate(
	const struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	u32 dwSizeInBytes
);

u16 HPI_InStreamHostBufferFree(
	const struct hpi_hsubsys *phSubSys,
	u32 hInStream
);

u16 HPI_InStreamGroupAdd(
	const struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	u32 hStream
);

u16 HPI_InStreamGroupGetMap(
	const struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	u32 *pdwOutStreamMap,
	u32 *pdwInStreamMap
);

u16 HPI_InStreamGroupReset(
	const struct hpi_hsubsys *phSubSys,
	u32 hInStream
);

/*********/
/* MIXER */
/*********/
u16 HPI_MixerOpen(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 *phMixer
);

u16 HPI_MixerClose(
	const struct hpi_hsubsys *phSubSys,
	u32 hMixer
);

u16 HPI_MixerGetControl(
	const struct hpi_hsubsys *phSubSys,
	u32 hMixer,
	u16 wSrcNodeType,
	u16 wSrcNodeTypeIndex,
	u16 wDstNodeType,
	u16 wDstNodeTypeIndex,
	u16 wControlType,
	u32 *phControl
);

u16 HPI_MixerGetControlByIndex(
	const struct hpi_hsubsys *phSubSys,
	u32 hMixer,
	u16 wControlIndex,
	u16 *pwSrcNodeType,
	u16 *pwSrcNodeIndex,
	u16 *pwDstNodeType,
	u16 *pwDstNodeIndex,
	u16 *pwControlType,
	u32 *phControl
);

u16 HPI_MixerStore(
	const struct hpi_hsubsys *phSubSys,
	u32 hMixer,
	enum HPI_MIXER_STORE_COMMAND Command,
	u16 wIndex
);
/*************************/
/* mixer CONTROLS                */
/*************************/
/*************************/
/* volume control                */
/*************************/
u16 HPI_VolumeSetGain(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short anGain0_01dB[HPI_MAX_CHANNELS]
);

u16 HPI_VolumeGetGain(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short anGain0_01dB_out[HPI_MAX_CHANNELS]
);

#define HPI_VolumeGetRange HPI_VolumeQueryRange
u16 HPI_VolumeQueryRange(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short *nMinGain_01dB,
	short *nMaxGain_01dB,
	short *nStepGain_01dB
);

u16 HPI_Volume_QueryChannels(
	const struct hpi_hsubsys *phSubSys,
	const u32 hVolume,
	u32 *pChannels
);

u16 HPI_VolumeAutoFade(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short anStopGain0_01dB[HPI_MAX_CHANNELS],
	u32 wDurationMs
);

u16 HPI_VolumeAutoFadeProfile(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short anStopGain0_01dB[HPI_MAX_CHANNELS],
	u32 dwDurationMs,
	u16 dwProfile
);

/*************************/
/* level control         */
/*************************/
u16 HPI_LevelQueryRange(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short *nMinGain_01dB,
	short *nMaxGain_01dB,
	short *nStepGain_01dB
);

u16 HPI_LevelSetGain(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short anGain0_01dB[HPI_MAX_CHANNELS]
);

u16 HPI_LevelGetGain(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short anGain0_01dB_out[HPI_MAX_CHANNELS]
);

/*************************/
/* meter control                 */
/*************************/
u16 HPI_Meter_QueryChannels(
	const struct hpi_hsubsys *phSubSys,
	const u32 hMeter,
	u32 *pChannels
);

u16 HPI_MeterGetPeak(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short anPeak0_01dB_out[HPI_MAX_CHANNELS]
);

u16 HPI_MeterGetRms(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short anPeak0_01dB_out[HPI_MAX_CHANNELS]
);

u16 HPI_MeterSetPeakBallistics(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 nAttack,
	u16 nDecay
);

u16 HPI_MeterSetRmsBallistics(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 nAttack,
	u16 nDecay
);

u16 HPI_MeterGetPeakBallistics(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *nAttack,
	u16 *nDecay
);

u16 HPI_MeterGetRmsBallistics(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *nAttack,
	u16 *nDecay
);

/*************************/
/* channel mode control  */
/*************************/
u16 HPI_ChannelMode_QueryMode(
	const struct hpi_hsubsys *phSubSys,
	const u32 hMode,
	const u32 dwIndex,
	u16 *pwMode
);

u16 HPI_ChannelModeSet(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wMode
);

u16 HPI_ChannelModeGet(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *wMode
);

/*************************/
/* Tuner control                 */
/*************************/
u16 HPI_Tuner_QueryBand(
	const struct hpi_hsubsys *phSubSys,
	const u32 hTuner,
	const u32 dwIndex,
	u16 *pwBand
);

u16 HPI_Tuner_SetBand(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wBand
);

u16 HPI_Tuner_GetBand(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwBand
);

u16 HPI_Tuner_QueryFrequency(
	const struct hpi_hsubsys *phSubSys,
	const u32 hTuner,
	const u32 dwIndex,
	const u16 band,
	u32 *pdwFreq
);

u16 HPI_Tuner_SetFrequency(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 wFreqInkHz
);

u16 HPI_Tuner_GetFrequency(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pwFreqInkHz
);

u16 HPI_Tuner_GetRFLevel(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short *pwLevel
);

u16 HPI_Tuner_GetRawRFLevel(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short *pwLevel
);

u16 HPI_Tuner_QueryGain(
	const struct hpi_hsubsys *phSubSys,
	const u32 hTuner,
	const u32 dwIndex,
	u16 *pwGain
);

u16 HPI_Tuner_SetGain(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short nGain
);

u16 HPI_Tuner_GetGain(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short *pnGain
);

u16 HPI_Tuner_GetStatus(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwStatusMask,
	u16 *pwStatus
);

u16 HPI_Tuner_SetMode(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 nMode,
	u32 nValue
);

u16 HPI_Tuner_GetMode(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 nMode,
	u32 *pnValue
);

u16 HPI_Tuner_GetRDS(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	char *pRdsData
);

u16 HPI_Tuner_QueryDeemphasis(
	const struct hpi_hsubsys *phSubSys,
	const u32 hTuner,
	const u32 dwIndex,
	const u16 band,
	u32 *pdwDeemphasis
);

u16 HPI_Tuner_SetDeemphasis(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwDeemphasis
);
u16 HPI_Tuner_GetDeemphasis(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwDeemphasis
);

u16 HPI_Tuner_QueryProgram(
	const struct hpi_hsubsys *phSubSys,
	const u32 hTuner,
	u32 *pbitmapProgram
);

u16 HPI_Tuner_SetProgram(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwProgram
);

u16 HPI_Tuner_GetProgram(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwProgram
);

u16 HPI_Tuner_GetHdRadioDspVersion(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	char *pszDspVersion,
	const u32 dwStringSize
);

u16 HPI_Tuner_GetHdRadioSdkVersion(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	char *pszSdkVersion,
	const u32 dwStringSize
);

u16 HPI_Tuner_GetHdRadioSignalQuality(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwQuality
);

/****************************/
/* PADs control             */
/****************************/

u16 HPI_PAD_GetChannelName(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	char *pszString,
	const u32 dwStringLength
);

u16 HPI_PAD_GetArtist(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	char *pszString,
	const u32 dwStringLength
);

u16 HPI_PAD_GetTitle(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	char *pszString,
	const u32 dwStringLength
);

u16 HPI_PAD_GetComment(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	char *pszString,
	const u32 dwStringLength
);

u16 HPI_PAD_GetProgramType(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwPTY
);

u16 HPI_PAD_GetRdsPI(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwPI
);

u16 HPI_PAD_GetProgramTypeString(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	const u32 dwDataType,
	const u32 nPTY,
	char *pszString,
	const u32 dwStringLength
);

/****************************/
/* AES/EBU Receiver control */
/****************************/
u16 HPI_AESEBU_Receiver_QueryFormat(
	const struct hpi_hsubsys *phSubSys,
	const u32 hAesRx,
	const u32 dwIndex,
	u16 *pwFormat
);

u16 HPI_AESEBU_Receiver_SetFormat(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wSource
);

u16 HPI_AESEBU_Receiver_GetFormat(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwSource
);

u16 HPI_AESEBU_Receiver_GetSampleRate(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwSampleRate
);

u16 HPI_AESEBU_Receiver_GetUserData(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wIndex,
	u16 *pwData
);

u16 HPI_AESEBU_Receiver_GetChannelStatus(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wIndex,
	u16 *pwData
);

u16 HPI_AESEBU_Receiver_GetErrorStatus(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwErrorData
);

/*******************************/
/* AES/EBU Transmitter control */
/*******************************/
u16 HPI_AESEBU_Transmitter_SetSampleRate(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwSampleRate
);

u16 HPI_AESEBU_Transmitter_SetUserData(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wIndex,
	u16 wData
);

u16 HPI_AESEBU_Transmitter_SetChannelStatus(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wIndex,
	u16 wData
);

u16 HPI_AESEBU_Transmitter_GetChannelStatus(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wIndex,
	u16 *pwData
);

u16 HPI_AESEBU_Transmitter_QueryFormat(
	const struct hpi_hsubsys *phSubSys,
	const u32 hAesTx,
	const u32 dwIndex,
	u16 *pwFormat
);

u16 HPI_AESEBU_Transmitter_SetFormat(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wOutputFormat
);

u16 HPI_AESEBU_Transmitter_GetFormat(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwOutputFormat
);

/***********************/
/* multiplexer control */
/***********************/
u16 HPI_Multiplexer_SetSource(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wSourceNodeType,
	u16 wSourceNodeIndex
);

u16 HPI_Multiplexer_GetSource(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *wSourceNodeType,
	u16 *wSourceNodeIndex
);

u16 HPI_Multiplexer_QuerySource(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 nIndex,
	u16 *wSourceNodeType,
	u16 *wSourceNodeIndex
);

/***************/
/* VOX control */
/***************/
u16 HPI_VoxSetThreshold(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short anGain0_01dB
);

u16 HPI_VoxGetThreshold(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short *anGain0_01dB
);

/*********************/
/* Bitstream control */
/*********************/
u16 HPI_Bitstream_SetClockEdge(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wEdgeType
);

u16 HPI_Bitstream_SetDataPolarity(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wPolarity
);

u16 HPI_Bitstream_GetActivity(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwClkActivity,
	u16 *pwDataActivity
);

/***********************/
/* SampleClock control */
/***********************/

u16 HPI_SampleClock_QuerySource(
	const struct hpi_hsubsys *phSubSys,
	const u32 hClock,
	const u32 dwIndex,
	u16 *pwSource
);

u16 HPI_SampleClock_SetSource(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wSource
);

u16 HPI_SampleClock_GetSource(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwSource
);

u16 HPI_SampleClock_QuerySourceIndex(
	const struct hpi_hsubsys *phSubSys,
	const u32 hClock,
	const u32 dwIndex,
	const u32 dwSource,
	u16 *pwSourceIndex
);

u16 HPI_SampleClock_SetSourceIndex(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wSourceIndex
);

u16 HPI_SampleClock_GetSourceIndex(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwSourceIndex
);

u16 HPI_SampleClock_GetSampleRate(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwSampleRate
);

u16 HPI_SampleClock_QueryLocalRate(
	const struct hpi_hsubsys *phSubSys,
	const u32 hClock,
	const u32 dwIndex,
	u32 *pdwSource
);

u16 HPI_SampleClock_SetLocalRate(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwSampleRate
);

u16 HPI_SampleClock_GetLocalRate(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwSampleRate
);

u16 HPI_SampleClock_SetAuto(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwAuto
);

u16 HPI_SampleClock_GetAuto(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwAuto
);

u16 HPI_SampleClock_SetLocalRateLock(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwLock
);

u16 HPI_SampleClock_GetLocalRateLock(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwLock
);

/***********************/
/* Microphone control */
/***********************/
u16 HPI_Microphone_SetPhantomPower(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wOnOff
);

u16 HPI_Microphone_GetPhantomPower(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwOnOff
);

/*******************************
  Parametric Equalizer control
*******************************/
u16 HPI_ParametricEQ_GetInfo(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwNumberOfBands,
	u16 *pwEnabled
);

u16 HPI_ParametricEQ_SetState(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wOnOff
);

u16 HPI_ParametricEQ_SetBand(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wIndex,
	u16 nType,
	u32 dwFrequencyHz,
	short nQ100,
	short nGain0_01dB
);

u16 HPI_ParametricEQ_GetBand(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wIndex,
	u16 *pnType,
	u32 *pdwFrequencyHz,
	short *pnQ100,
	short *pnGain0_01dB
);

u16 HPI_ParametricEQ_GetCoeffs(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wIndex,
	short coeffs[5]
);

/*******************************
  Compressor Expander control
*******************************/

u16 HPI_Compander_Set(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wAttack,
	u16 wDecay,
	short wRatio100,
	short nThreshold0_01dB,
	short nMakeupGain0_01dB
);

u16 HPI_Compander_Get(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwAttack,
	u16 *pwDecay,
	short *pwRatio100,
	short *pnThreshold0_01dB,
	short *pnMakeupGain0_01dB
);

/*******************************
  Cobranet HMI control
*******************************/
u16 HPI_Cobranet_HmiWrite(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwHmiAddress,
	u32 dwByteCount,
	u8 *pbData
);

u16 HPI_Cobranet_HmiRead(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwHmiAddress,
	u32 dwMaxByteCount,
	u32 *pdwByteCount,
	u8 *pbData
);

u16 HPI_Cobranet_HmiGetStatus(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwStatus,
	u32 *pdwReadableSize,
	u32 *pdwWriteableSize
);

/*Read the current IP address
*/
u16 HPI_Cobranet_GetIPaddress(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwIPaddress
);
/* Write the current IP address
*/
u16 HPI_Cobranet_SetIPaddress(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwIPaddress
);
/* Read the static IP address
*/
u16 HPI_Cobranet_GetStaticIPaddress(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwIPaddress
);
/* Write the static IP address
*/
u16 HPI_Cobranet_SetStaticIPaddress(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwIPaddress
);
/* Read the MAC address
*/
u16 HPI_Cobranet_GetMACaddress(
	const struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwMAC_MSBs,
	u32 *pdwMAC_LSBs
);
/*******************************
  Tone Detector control
*******************************/
u16 HPI_ToneDetector_GetState(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	u32 *State
);

u16 HPI_ToneDetector_SetEnable(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	u32 Enable
);

u16 HPI_ToneDetector_GetEnable(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	u32 *Enable
);

u16 HPI_ToneDetector_SetEventEnable(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	u32 EventEnable
);

u16 HPI_ToneDetector_GetEventEnable(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	u32 *EventEnable
);

u16 HPI_ToneDetector_SetThreshold(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	int Threshold
);

u16 HPI_ToneDetector_GetThreshold(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	int *Threshold
);

u16 HPI_ToneDetector_GetFrequency(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	u32 nIndex,
	u32 *dwFrequency
);

/*******************************
  Silence Detector control
*******************************/
u16 HPI_SilenceDetector_GetState(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	u32 *State
);

u16 HPI_SilenceDetector_SetEnable(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	u32 Enable
);

u16 HPI_SilenceDetector_GetEnable(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	u32 *Enable
);

u16 HPI_SilenceDetector_SetEventEnable(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	u32 EventEnable
);

u16 HPI_SilenceDetector_GetEventEnable(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	u32 *EventEnable
);

u16 HPI_SilenceDetector_SetDelay(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	u32 Delay
);

u16 HPI_SilenceDetector_GetDelay(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	u32 *Delay
);

u16 HPI_SilenceDetector_SetThreshold(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	int Threshold
);

u16 HPI_SilenceDetector_GetThreshold(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	int *Threshold
);

/*******************************
  Universal control
*******************************/
u16 HPI_Entity_FindNext(
	struct hpi_entity *container_entity,
	enum e_entity_type type,
	enum e_entity_role role,
	int recursive_flag,
	struct hpi_entity **current_match
);

u16 HPI_Entity_CopyValueFrom(
	struct hpi_entity *entity,
	enum e_entity_type type,
	size_t item_count,
	void *value_dst_p
);

u16 HPI_Entity_Unpack(
	struct hpi_entity *entity,
	enum e_entity_type *type,
	size_t *items,
	enum e_entity_role *role,
	void **value
);

u16 HPI_Entity_AllocAndPack(
	const enum e_entity_type type,
	const size_t item_count,
	const enum e_entity_role role,
	void *value,
	struct hpi_entity **entity
);

void HPI_Entity_Free(
	struct hpi_entity *entity
);

u16 HPI_Universal_Info(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	struct hpi_entity **info
);

u16 HPI_Universal_Get(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	struct hpi_entity **value
);

u16 HPI_Universal_Set(
	const struct hpi_hsubsys *phSubSys,
	u32 hC,
	struct hpi_entity *value
);

/*/////////// */
/* DSP CLOCK  */
/*/////////// */
u16 HPI_ClockOpen(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 *phDspClock
);

u16 HPI_ClockSetTime(
	const struct hpi_hsubsys *phSubSys,
	u32 hClock,
	u16 wHour,
	u16 wMinute,
	u16 wSecond,
	u16 wMilliSecond
);

u16 HPI_ClockGetTime(
	const struct hpi_hsubsys *phSubSys,
	u32 hClock,
	u16 *pwHour,
	u16 *pwMinute,
	u16 *pwSecond,
	u16 *pwMilliSecond
);

/*/////////// */
/* PROFILE        */
/*/////////// */
u16 HPI_ProfileOpenAll(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wProfileIndex,
	u32 *phProfile,
	u16 *pwMaxProfiles
);

u16 HPI_ProfileGet(
	const struct hpi_hsubsys *phSubSys,
	u32 hProfile,
	u16 wIndex,
	u16 *pwSeconds,
	u32 *pdwMicroSeconds,
	u32 *pdwCallCount,
	u32 *pdwMaxMicroSeconds,
	u32 *pdwMinMicroSeconds
);

u16 HPI_ProfileStartAll(
	const struct hpi_hsubsys *phSubSys,
	u32 hProfile
);

u16 HPI_ProfileStopAll(
	const struct hpi_hsubsys *phSubSys,
	u32 hProfile
);

u16 HPI_ProfileGetName(
	const struct hpi_hsubsys *phSubSys,
	u32 hProfile,
	u16 wIndex,
	char *szProfileName,
	u16 nProfileNameLength
);

u16 HPI_ProfileGetUtilization(
	const struct hpi_hsubsys *phSubSys,
	u32 hProfile,
	u32 *pdwUtilization
);

/*//////////////////// */
/* UTILITY functions */

u16 HPI_FormatCreate(
	struct hpi_format *pFormat,
	u16 wChannels,
	u16 wFormat,
	u32 dwSampleRate,
	u32 dwBitRate,
	u32 dwAttributes
);

/* Until it's verified, this function is for Windows OSs only */

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#endif	 /*_H_HPI_ */
/*
///////////////////////////////////////////////////////////////////////////////
// See CVS for history.  Last complete set in rev 1.146
////////////////////////////////////////////////////////////////////////////////
*/
