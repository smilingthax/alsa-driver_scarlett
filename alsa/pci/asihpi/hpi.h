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

Hardware Programming Interface (HPI) header
The HPI is a low-level hardware abstraction layer to all
AudioScience digital audio adapters

You must define one operating systems that the HPI is to be compiled under
HPI_OS_DOS           16bit real-mode DOS
HPI_OS_WIN16         16bit Windows
HPI_OS_WIN32_USER    32bit Windows
HPI_OS_WINNT_KERN    WinNT kernel driver
HPI_OS_WIN95_KERN    Win95 VXD kernel driver
HPI_OS_DSP_563XX     DSP563XX environment
HPI_OS_DSP_C6000     DSP TI C6000 environment
HPI_OS_WDM           Windows WDM kernel driver
HPI_OS_LINUX         Linux kernel driver

(C) Copyright AudioScience Inc. 1998-2003
******************************************************************************/
#ifndef _HPI_H_
#define _HPI_H_

/* HPI Version
If HPI_VER_MINOR is odd then its a development release not intended for the public
If HPI_VER_MINOR is even then is a release version
i.e 3.05.02 is a development version
*/
#define HPI_VERSION_CONSTRUCTOR(maj,min,rel)  (((maj) <<16 ) + ((min) << 8) + (rel))

#define HPI_VER_MAJOR(v) ((v)>>16)
#define HPI_VER_MINOR(v) (((v)>>8) & 0xFF )
#define HPI_VER_RELEASE(v) ((v) & 0xFF )

#define HPI_VER HPI_VERSION_CONSTRUCTOR( 3, 05, 07 )

/** Define HPI_WITHOUT_HPI_DATA to remove public definition and use of HPI_DATA struct */
/* #define HPI_WITHOUT_HPI_DATA */

#define HPI_SUPPORT_ONOFFSWITCH
#define HPI_SUPPORT_AESEBUTXSETCLKSRC

#ifdef __cplusplus
extern "C" {
#endif

#define HPI_MAX_ADAPTER_MEM_SPACES (2) /**< maximum number of memory regions mapped to an adapter */

#include "hpios.h"

/* A few convenience macros */
#ifndef DEPRECATED
#define DEPRECATED
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(A) (sizeof(A)/sizeof((A)[0]))
#endif

#ifndef __stringify
#define __stringify_1(x) #x
#define __stringify(x) __stringify_1(x)
#endif

#define HPI_UNUSED(param) (void)param

/******************************************************************************/
/******************************************************************************/
/********                     HPI API DEFINITIONS                     *********/
/******************************************************************************/
/******************************************************************************/

/* //////////////////////////////////////////////////////////////////////// */
/* BASIC TYPES */
/* u8, u16, u32 MUST BE DEFINED IN HPIOS_xxx.H */

/* ////////////////////////////////////////////////////////////////////// */
/** \addtogroup hpi_defines HPI constant definitions
\{
*/

/*******************************************/
/** \defgroup adapter_ids Adapter types/product ids
\{
*/
#define HPI_ADAPTER_ASI1101             0x1101		/**< ASI1101 - 1 Stream MPEG playback */
#define HPI_ADAPTER_ASI1201             0x1201		/**< ASI1201 - 2 Stream MPEG playback */

#define HPI_ADAPTER_EVM6701             0x1002		/**< TI's C6701 EVM has this ID */
#define HPI_ADAPTER_DSP56301            0x1801		/**< DSP56301 rev A has this ID */
#define HPI_ADAPTER_PCI2040             0xAC60		/**< TI's PCI2040 PCI I/F chip has this ID */
#define HPI_ADAPTER_DSP6205             0xA106		/**< TI's C6205 PCI interface has this ID */

#define HPI_ADAPTER_FAMILY_MASK         0xff00		/**< First 2 hex digits define the adapter family */

#define HPI_ADAPTER_ASI4030             0x4030		/**< ASI4030 = PSI30 = OEM 3 Stereo playback */

#define HPI_ADAPTER_ASI2214             0x2214		/**< ASI2214 - USB 2.0 1xanalog in, 4 x analog out, 1 x AES in/out */

#define HPI_ADAPTER_ASI2416             0x2416		/**< ASI2416 - CobraNet peripheral */

#define HPI_ADAPTER_ASI4111             0x4111		/**< 2 play-1out, 1 rec PCM, MPEG*/
#define HPI_ADAPTER_ASI4113             0x4113		/**< 4 play-3out, 1 rec PCM, MPEG*/
#define HPI_ADAPTER_ASI4215             0x4215		/**< 4 play-5out, 1 rec, PCM, MPEG*/

#define HPI_ADAPTER_FAMILY_ASI4300              0x4300
#define HPI_ADAPTER_ASI4311             0x4311		/**< 4 play-1out, 1 rec-1in, PCM, MPEG*/
#define HPI_ADAPTER_ASI4312             0x4312		/**< 4 play-2out, 1 rec-1in, PCM, MPEG*/
#define HPI_ADAPTER_ASI4332             0x4332		/**< 4 play-2out, 1 rec-3in, PCM, MPEG*/
#define HPI_ADAPTER_ASI4334             0x4334		/**< 4 play-4out, 1 rec-3in, PCM, MPEG*/
#define HPI_ADAPTER_ASI4335             0x4335		/**< 4 play-4out, 1 rec-3in, PCM, MPEG, 8-relay, 16-opto*/
#define HPI_ADAPTER_ASI4336             0x4336		/**< 4 play-4out, 1 rec-3in, PCM, MPEG, 8-relay, 16-opto, RS422*/
#define HPI_ADAPTER_ASI4342             0x4342		/**< (ASI4312 with MP3) 4 play-2out, 1 rec-1in, PCM, MPEG-L2, MP3 */
#define HPI_ADAPTER_ASI4344             0x4344		/**< (ASI4334 with MP3)4 play-4out, 1 rec-3in, PCM, MPEG-L2, MP3 */
#define HPI_ADAPTER_ASI4346             0x4346		/**< (ASI4336 with MP3)4 play-4out, 1 rec-3in, PCM, MPEG-L2, MP3, 8-relay, 16-opto, RS422*/

#define HPI_ADAPTER_ASI4401             0x4401		/**< OEM 2 play, PCM mono/stereo, 44.1kHz*/

#define HPI_ADAPTER_ASI4501             0x4501		/**< OEM 4 play, 1 rec PCM, MPEG*/
#define HPI_ADAPTER_ASI4502             0x4502		/**< OEM 1 play, 1 rec PCM, MPEG*/
#define HPI_ADAPTER_ASI4503             0x4503		/**< OEM 4 play PCM, MPEG*/

#define HPI_ADAPTER_ASI4601             0x4601		/**< OEM 4 play PCM, MPEG & 1 record with AES-18 */
#define HPI_ADAPTER_ASI4701             0x4701		/**< OEM 24 mono play PCM with 512MB RAM */

#define HPI_ADAPTER_FAMILY_ASI5000              0x5000
#define HPI_ADAPTER_ASI5001             0x5001		/**< ASI5001 OEM, PCM only, 4 in, 1 out analog */
#define HPI_ADAPTER_ASI5002             0x5002		/**< ASI5002 OEM, PCM only, 4 in, 1 out analog and digital */
#define HPI_ADAPTER_ASI5020             0x5020		/**< ASI5020 PCM only, 2 analog only in/out */
#define HPI_ADAPTER_ASI5044             0x5044		/**< ASI5044 PCM only, 4 analog and digital in/out */
#define HPI_ADAPTER_ASI5041             0x5041		/**< ASI5041 PCM only, 4 digital only in/out */
#define HPI_ADAPTER_ASI5042             0x5042		/**< ASI5042 PCM only, 4 analog only in/out */

#define HPI_ADAPTER_FAMILY_ASI5100              0x5100
#define HPI_ADAPTER_ASI5101             0x5101		/**< ASI5101 OEM is ASI5111 with no mic. */
#define HPI_ADAPTER_ASI5111             0x5111		/**< ASI5111 PCM only */

#define HPI_ADAPTER_ASI6101             0x6101		/**< ASI6101 prototype */

#define HPI_ADAPTER_FAMILY_ASI6000              0x6000
#define HPI_ADAPTER_ASI6000             0x6000		/**< ASI6000 - generic 1 DSP adapter, exact config undefined */
#define HPI_ADAPTER_ASI6012             0x6012		/**< ASI6012 - 1 in, 2 out analog only */
#define HPI_ADAPTER_ASI6022             0x6022		/**< ASI6022 - 2 in, 2 out analog only */
#define HPI_ADAPTER_ASI6044             0x6044		/**< ASI6044 - 4 in/out analog only */

#define HPI_ADAPTER_FAMILY_ASI6100              0x6100
#define HPI_ADAPTER_ASI6111             0x6111		/**< ASI6111 - 1 in/out, analog and AES3  */
#define HPI_ADAPTER_ASI6102             0x6102		/**< ASI6102 - 2out,analog and AES3  */
#define HPI_ADAPTER_ASI6113             0x6113		/**< 300MHz version of ASI6114 for testing*/
#define HPI_ADAPTER_ASI6122             0x6122		/**< ASI6122 - 2 in/out, analog and AES3  */
#define HPI_ADAPTER_ASI6114             0x6114		/**< ASI6114 - 4os,1is,4out,1in,analog and AES3  */
#define HPI_ADAPTER_ASI6118             0x6118		/**< ASI6118 - 8os,1is,8out,1in analog+AES3 */

#define HPI_ADAPTER_FAMILY_ASI6200              0x6200
#define HPI_ADAPTER_ASI6201             0x6201		/**< ASI6201 - OEM  */
#define HPI_ADAPTER_ASI6244             0x6244		/**< ASI6244 - 4os,4is,4out,4in,analog and AES3 */
#define HPI_ADAPTER_ASI6246             0x6246		/**< ASI6246 - 6os,2is,6out,4in,analog and AES3 */
#define HPI_ADAPTER_ASI6200             0x6200		/**< ASI6200 - generic 2 DSP adapter, exact config undefined */
#define HPI_ADAPTER_ASI6100             0x6100		/**< ASI6100 - generic 1 DSP adapter, exact config undefined */

#define HPI_ADAPTER_FAMILY_ASI6400              0x6400
#define HPI_ADAPTER_ASI6408             0x6408		/**< ASI6408 - cobranet PCI 8 mono in/out */
#define HPI_ADAPTER_ASI6416             0x6416		/**< ASI6416 - cobranet PCI 16 mono in/out */

#define HPI_ADAPTER_FAMILY_ASI6500              0x6500			/**< ASI6500 PCI sound cards */
#define HPI_ADAPTER_ASI6514             0x6514			/**< ASI6514 - ASI6114 replacement, 12os,2is,4out,1in,analog and AES3  */
#define HPI_ADAPTER_ASI6520             0x6520			/**< ASI6520 - 6os,4is,2out,2in,analog only  */
#define HPI_ADAPTER_ASI6522             0x6522			/**< ASI6522 - 6os,4is,2out,2in,analog and AES3  */
#define HPI_ADAPTER_ASI6540             0x6540			/**< ASI6540 - 12os,8is,4out,4in,analog only  */
#define HPI_ADAPTER_ASI6544             0x6544			/**< ASI6544 - 12os,8is,4out,4in,analog and AES3  */
#define HPI_ADAPTER_ASI6585             0x6585			/**< ASI6585  - 8in, 8out, Livewire */

#define HPI_ADAPTER_FAMILY_ASI6600              0x6600			/**< ASI6600 PCI Express sound cards */
#define HPI_ADAPTER_ASI6614             0x6614			/**< ASI6614 - ASI6114 replacement, 12os,2is,4out,1in,analog and AES3  */
#define HPI_ADAPTER_ASI6620             0x6620			/**< ASI6620 - 6os,4is,2out,2in,analog only  */
#define HPI_ADAPTER_ASI6622             0x6622			/**< ASI6622 - 6os,4is,2out,2in,analog and AES3  */
#define HPI_ADAPTER_ASI6640             0x6640			/**< ASI6640 - 12os,8is,4out,4in,analog only  */
#define HPI_ADAPTER_ASI6644             0x6644			/**< ASI6644 - 12os,8is,4out,4in,analog and AES3  */
#define HPI_ADAPTER_ASI6648             0x6648			/**< ASI6648 - 16os,8is,8out,4in,analog and AES3  */

#define HPI_ADAPTER_ASI8401             0x8401		/**< OEM 4 record */
#define HPI_ADAPTER_ASI8411             0x8411		/**< OEM RF switcher */

#define HPI_ADAPTER_ASI8601             0x8601		/**< OEM 8 record */

#define HPI_ADAPTER_FAMILY_ASI8700              0x8700
#define HPI_ADAPTER_ASI8701             0x8701		/**< OEM 8 record 2 AM/FM 8 FM/TV , AM has 10kHz b/w*/
#define HPI_ADAPTER_ASI8702             0x8702		/**< 8 AM/FM record */
#define HPI_ADAPTER_ASI8703             0x8703		/**< 8 TV/FM record */
#define HPI_ADAPTER_ASI8704             0x8704		/**< standard product 2 AM/FM 8 FM/TV */
#define HPI_ADAPTER_ASI8705             0x8705		/**< 4 TV/FM, 4 AM/FM record */
#define HPI_ADAPTER_ASI8706             0x8706		/**< OEM 8 record 2 AM/FM 8 FM/TV */
#define HPI_ADAPTER_ASI8707             0x8707		/**< 8 record AM/FM - 4 ext antenna jacks */
#define HPI_ADAPTER_ASI8708             0x8708		/**< 8 record AM/FM - 6 ext antenna jacks */
#define HPI_ADAPTER_ASI8709             0x8709		/**< 8 record - no tuners */
#define HPI_ADAPTER_ASI8710             0x8710		/**< 8 record AM/FM - 1 ext antenna jacks*/
#define HPI_ADAPTER_ASI8711             0x8711		/**< 8 record AM/FM - 2 ext antenna jacks*/

#define HPI_ADAPTER_ASI8712             0x8712		/**< 4 record AM/FM */
#define HPI_ADAPTER_ASI8713             0x8713		/**< 4 record NTSC-TV/FM */

#define HPI_ADAPTER_ASI8722             0x8722		/**< 8 record 6xAM/FM+2xNTSC */
#define HPI_ADAPTER_ASI8723             0x8723		/**< 8 record NTSC */
#define HPI_ADAPTER_ASI8724             0x8724		/**< 4 record NTSC */
#define HPI_ADAPTER_ASI8725             0x8725		/**< 4 record 4xAM/FM+4xNTSC */

#define HPI_ADAPTER_ASI8732             0x8732		/**< 8 record 6xAM/FM+2xPAL */
#define HPI_ADAPTER_ASI8733             0x8733		/**< 8 record PAL */
#define HPI_ADAPTER_ASI8734             0x8734		/**< 4 record PAL */
#define HPI_ADAPTER_ASI8735             0x8735		/**< 4 record 4xAM/FM+4xPAL */

#define HPI_ADAPTER_ASI8801             0x8801		/**< OEM 8 record */

#define HPI_ADAPTER_ILLEGAL             0xFFFF		/**< Used in DLL to indicate device not present */

/**\}*/
/*******************************************/
/** \defgroup formats Audio format types
\{
*/
#define HPI_FORMAT_MIXER_NATIVE         0	// used internally on adapter
#define HPI_FORMAT_PCM8_UNSIGNED        1
#define HPI_FORMAT_PCM16_SIGNED         2
#define HPI_FORMAT_MPEG_L1              3
#define HPI_FORMAT_MPEG_L2              4
#define HPI_FORMAT_MPEG_L3              5
#define HPI_FORMAT_DOLBY_AC2            6
#define HPI_FORMAT_DOLBY_AC3            7
#define HPI_FORMAT_PCM16_BIGENDIAN      8
#define HPI_FORMAT_AA_TAGIT1_HITS       9
#define HPI_FORMAT_AA_TAGIT1_INSERTS    10
#define HPI_FORMAT_PCM32_SIGNED         11
#define HPI_FORMAT_RAW_BITSTREAM        12
#define HPI_FORMAT_AA_TAGIT1_HITS_EX1   13
#define HPI_FORMAT_PCM32_FLOAT                  14
#define HPI_FORMAT_PCM24_SIGNED         15
#define HPI_FORMAT_OEM1                         16
#define HPI_FORMAT_OEM2                         17
#define HPI_FORMAT_UNDEFINED            (0xffff)
/**\}*/
/******************************************* bus types */
#define HPI_BUS_ISAPNP                  1
#define HPI_BUS_PCI                     2
#define HPI_BUS_USB                     3

/******************************************* in/out Stream states */
#define HPI_STATE_STOPPED               1
#define HPI_STATE_PLAYING               2
#define HPI_STATE_RECORDING             3
#define HPI_STATE_DRAINED               4
#define HPI_STATE_SINEGEN               5

/******************************************* mixer source node types */
#define HPI_SOURCENODE_BASE             100
#define HPI_SOURCENODE_OSTREAM          101
#define HPI_SOURCENODE_LINEIN           102
#define HPI_SOURCENODE_AESEBU_IN        103
#define HPI_SOURCENODE_TUNER            104	/* AGE 8/4/97 */
#define HPI_SOURCENODE_RF               105
#define HPI_SOURCENODE_CLOCK_SOURCE     106
#define HPI_SOURCENODE_RAW_BITSTREAM    107
#define HPI_SOURCENODE_MICROPHONE       108
#define HPI_SOURCENODE_COBRANET                 109
/*! Update this if you add a new sourcenode type, AND hpidebug.h */
#define HPI_SOURCENODE_LAST_INDEX         109
/* AX4 max sourcenode type = 15 */
/* AX6 max sourcenode type = 15 */

/******************************************* mixer dest node types */
#define HPI_DESTNODE_BASE               200
#define HPI_DESTNODE_ISTREAM            201
#define HPI_DESTNODE_LINEOUT            202
#define HPI_DESTNODE_AESEBU_OUT         203
#define HPI_DESTNODE_RF                 204
#define HPI_DESTNODE_SPEAKER            205
#define HPI_DESTNODE_COBRANET                   206
/*! Update this if you add a new destnode type. , AND hpidebug.h  */
#define HPI_DESTNODE_LAST_INDEX         206
/* AX4 max destnode type = 7 */
/* AX6 max destnode type = 15 */

/******************************************* mixer control types */
#define HPI_CONTROL_GENERIC             0
#define HPI_CONTROL_CONNECTION          1
#define HPI_CONTROL_VOLUME              2	/* works in dBFs */
#define HPI_CONTROL_METER               3
#define HPI_CONTROL_MUTE                4
#define HPI_CONTROL_MULTIPLEXER         5
#define HPI_CONTROL_AESEBU_TRANSMITTER  6
#define HPI_CONTROL_AESEBU_RECEIVER     7
#define HPI_CONTROL_LEVEL               8	/* works in dBu */
#define HPI_CONTROL_TUNER               9
#define HPI_CONTROL_ONOFFSWITCH         10
#define HPI_CONTROL_VOX                 11
#define HPI_CONTROL_AES18_TRANSMITTER   12
#define HPI_CONTROL_AES18_RECEIVER      13
#define HPI_CONTROL_AES18_BLOCKGENERATOR   14
#define HPI_CONTROL_CHANNEL_MODE                15
/* WARNING types 16 or greater impact bit packing in AX4100 and AX4500 DSP code */
#define HPI_CONTROL_BITSTREAM                   16
#define HPI_CONTROL_SAMPLECLOCK                 17
#define HPI_CONTROL_MICROPHONE                  18
#define HPI_CONTROL_PARAMETRIC_EQ               19
#define HPI_CONTROL_COMPANDER                   20
#define HPI_CONTROL_COBRANET                    21
#define HPI_CONTROL_TONEDETECTOR                22
#define HPI_CONTROL_SILENCEDETECTOR     23

/*! Update this if you add a new control type. , AND hpidebug.h */
#define HPI_CONTROL_LAST_INDEX                  23

/* WARNING types 32 or greater impact bit packing in all AX4 DSP code */
/* WARNING types 256 or greater impact bit packing in all AX6 DSP code */

/******************************************* ADAPTER ATTRIBUTES ****/

/** \defgroup adapter_properties Adapter properties used in HPI_AdapterSetProperty() API.
\{
*/
/*! Used in dwProperty field of HPI_AdapterSetProperty() and HPI_AdapterGetProperty(). This errata applies to all
ASI6000 cards with both analog and digital outputs. The CS4224 A/D+D/A has a one sample
delay between left and right channels on both its input (ADC) and output (DAC). More details are available in
Cirrus Logic errata ER284B2.PDF available from http://www.cirrus.com, released by Cirrus in 2001.
*/
#define HPI_ADAPTER_PROPERTY_ERRATA_1   (1)

/** Adapter grouping property
Indicates whether the adapter supports the grouping API (for ASIO and SSX2)
*/
#define HPI_ADAPTER_PROPERTY_GROUPING           (2)

/** Adapter SSX2 property
Indicates whether the adapter supports SSX2 multichannel streams
*/
#define HPI_ADAPTER_PROPERTY_ENABLE_SSX2        (3)

#define HPI_ADAPTER_PROPERTY_READONLYBASE  (256)	/**< Base number for readonly properties */
/** Readonly adapter latency property.
This property returns in the input and output latency in samples.Property 1 is the estimated input latency
in samples, while Property 2 is that output latency in  samples.
*/
#define HPI_ADAPTER_PROPERTY_LATENCY                    (HPI_ADAPTER_PROPERTY_READONLYBASE+0)
/** Readonly adapter granularity property.
The granulariy is the smallest size chunk of stereo samples that is processed by
the adapter. This property returns the record granularity in samples in Property 1. Property 2 returns the
play granularity.
*/
#define HPI_ADAPTER_PROPERTY_GRANULARITY        (HPI_ADAPTER_PROPERTY_READONLYBASE+1)
/** Readonly adapter number of current channels property.
The CURCHANNELs property returns is the number of record and playback channels per device. Property 1 is the number
of record channels per record device, while Property 2 is the number of play channels per playback device.*/
#define HPI_ADAPTER_PROPERTY_CURCHANNELS        (HPI_ADAPTER_PROPERTY_READONLYBASE+2)

/**\}*/

/** \defgroup adapter_modes Adapter modes used in HPI_AdapterSetMode API
\warning - more than 16 possible modes breaks a bitmask in the Windows WAVE DLL
\{
*/
#define HPI_ADAPTER_MODE_SET    (0)	/**< used in wQueryOrSet field of HPI_AdapterSetModeEx() */
#define HPI_ADAPTER_MODE_QUERY (1)
#define HPI_ADAPTER_MODE_4OSTREAM (1)
#define HPI_ADAPTER_MODE_6OSTREAM (2)
#define HPI_ADAPTER_MODE_8OSTREAM (3)
#define HPI_ADAPTER_MODE_16OSTREAM (4)
#define HPI_ADAPTER_MODE_1OSTREAM (5)
#define HPI_ADAPTER_MODE_1 (6)
#define HPI_ADAPTER_MODE_2 (7)
#define HPI_ADAPTER_MODE_3 (8)
#define HPI_ADAPTER_MODE_MULTICHANNEL (9)
#define HPI_ADAPTER_MODE_12OSTREAM (10)
#define HPI_ADAPTER_MODE_9OSTREAM (11)
/**\}*/

/* Note, adapters can have more than one capability - encoding as bitfield is recommended. */
#define HPI_CAPABILITY_NONE  (0)
#define HPI_CAPABILITY_MPEG_LAYER3  (1)
/* Set this equal to maximum capability index, Must not be greater than 32 - see axnvdef.h */
#define HPI_CAPABILITY_MAX          1
/* #define HPI_CAPABILITY_AAC          2 */

/******************************************* STREAM ATTRIBUTES ****/

/* Ancillary Data modes */
#define HPI_MPEG_ANC_HASENERGY  (0)
#define HPI_MPEG_ANC_RAW                (1)
#define HPI_MPEG_ANC_ALIGN_LEFT (0)
#define HPI_MPEG_ANC_ALIGN_RIGHT (1)

/** \defgroup mpegmodes MPEG modes
\{
MPEG modes - can be used optionally for HPI_FormatCreate() parameter dwAttributes.

The operation of the below modes varies acCording to the number of channels. Using HPI_MPEG_MODE_DEFAULT
causes the MPEG-1 Layer II bitstream to be recorded in single_channel mode when the number
of channels is 1 and in stereo when the number of channels is 2. Using any mode setting other
than HPI_MPEG_MODE_DEFAULT when the number of channels is set to 1 will return an error.
*/
#define HPI_MPEG_MODE_DEFAULT   (0)
#define HPI_MPEG_MODE_STEREO    (1)
#define HPI_MPEG_MODE_JOINTSTEREO       (2)
#define HPI_MPEG_MODE_DUALCHANNEL       (3)
/** \} */

/* Locked memory buffer alloc/free phases */
#define HPI_BUFFER_CMD_EXTERNAL                                 0	/**< use one message to allocate or free physical memory */
#define HPI_BUFFER_CMD_INTERNAL_ALLOC                   1	/**< alloc physical memory */
#define HPI_BUFFER_CMD_INTERNAL_GRANTADAPTER    2	/**< send physical memory address to adapter */
#define HPI_BUFFER_CMD_INTERNAL_REVOKEADAPTER   3	/**< notify adapter to stop using physical buffer */
#define HPI_BUFFER_CMD_INTERNAL_FREE                    4	/**< free physical buffer */

/******************************************* MIXER ATTRIBUTES ****/

/** \defgroup mixer_flags Mixer flags used in processing function HPI_MIXER_GET_CONTROL_MULTIPLE_VALUES
\{
*/
#define HPI_MIXER_GET_CONTROL_MULTIPLE_CHANGED (0)
#define HPI_MIXER_GET_CONTROL_MULTIPLE_RESET (1)
/**\}*/

/** Commands used by HPI_MixerStore()
*/
	enum HPI_MIXER_STORE_COMMAND {
		HPI_MIXER_STORE_SAVE = 1,
				    /**< save all mixer control settings */
		HPI_MIXER_STORE_RESTORE = 2,
				    /**< restore all controls from saved */
		HPI_MIXER_STORE_DELETE = 3,
				    /**< delete saved control settings*/
		HPI_MIXER_STORE_ENABLE = 4,
				    /**< enable auto storage of some control settings */
		HPI_MIXER_STORE_DISABLE = 5,
				    /**< disable auto storage of some control settings */
		HPI_MIXER_STORE_SAVE_SINGLE = 6
				    /**< save the attributes of a single control */
	};

/******************************************* CONTROL ATTRIBUTES ****/
/* (in order of control type ID as above */

/* This allows for 255 control types, 255 unique attributes each */
/*#define HPI_CONTROL_SPACING (0x100)*/
#define HPI_MAKE_ATTRIBUTE(obj,index) (obj*0x100+index)

/* Generic control attributes.  If a control uses any of these attributes
its other attributes must also be defined using HPI_MAKE_ATTRIBUTE()
*/

/** Enable a control.
0=disable, 1=enable
\note generic to all mixer plugins?
*/
/*#define HPI_CONTROL_ENABLE     HPI_MAKE_ATTRIBUTE(HPI_CONTROL_GENERIC,1)*/
#define HPI_GENERIC_ENABLE     HPI_MAKE_ATTRIBUTE(HPI_CONTROL_GENERIC,1)

/** Enable event generation for a control.
0=disable, 1=enable
\note generic to all controls that can generate events
*/
/*#define HPI_EVENT_ENABLE       HPI_MAKE_ATTRIBUTE(HPI_CONTROL_GENERIC,2)*/
#define HPI_GENERIC_EVENT_ENABLE       HPI_MAKE_ATTRIBUTE(HPI_CONTROL_GENERIC,2)

/* Volume Control attributes */
#define HPI_VOLUME_GAIN             1
#define HPI_VOLUME_RANGE            10	/* make this very different from the other HPI_VOLUME_ defines below */

/* Volume Control attributes */
#define HPI_LEVEL_GAIN             1

#define HPI_VOLUME_AUTOFADE_LOG     2  /**< log fade - dB attenuation changes linearly over time */
#define HPI_VOLUME_AUTOFADE         HPI_VOLUME_AUTOFADE_LOG
#define HPI_VOLUME_AUTOFADE_LINEAR  3  /**< linear fade - amplitude changes linearly */
#define HPI_VOLUME_AUTOFADE_1       4  /**< Not implemented yet -may be special profiles */
#define HPI_VOLUME_AUTOFADE_2       5

/* Volume control special gain values */
#define HPI_UNITS_PER_dB                        (100)		/**< volumes units are 100ths of a dB */
#define HPI_GAIN_OFF                (-100*HPI_UNITS_PER_dB)  /**< turns volume control OFF or MUTE */
#define HPI_METER_MINIMUM           (-150*HPI_UNITS_PER_dB)  /**< value returned for no signal */

/* Meter Control attributes */
#define HPI_METER_RMS               1	    /**< return RMS signal level */
#define HPI_METER_PEAK              2	    /**< return peak signal level */
#define HPI_METER_RMS_BALLISTICS    3	    /**< ballistics for ALL rms meters on adapter */
#define HPI_METER_PEAK_BALLISTICS   4	    /**< ballistics for ALL peak meters on adapter */

/* Multiplexer control attributes */
#define HPI_MULTIPLEXER_SOURCE      1
#define HPI_MULTIPLEXER_QUERYSOURCE 2

/** AES/EBU control attributes */
#define HPI_AESEBU_SOURCE           1
#define HPI_AESEBU_ERRORSTATUS      2
#define HPI_AESEBU_SAMPLERATE       3
#define HPI_AESEBU_CHANNELSTATUS    4
#define HPI_AESEBU_USERDATA         5
#define HPI_AESEBU_CLOCKSOURCE      6	/*SGT nov-4-98 */

/** AES/EBU transmitter clock sources */
#define HPI_AESEBU_CLOCKSOURCE_ADAPTER          1
#define HPI_AESEBU_CLOCKSOURCE_AESEBU_SYNC      2

/** AES/EBU sources */
/** Receiver */
#define HPI_AESEBU_SOURCE_AESEBU    1
#define HPI_AESEBU_SOURCE_SPDIF     2

/** AES/EBU error status bits */
#define HPI_AESEBU_ERROR_NOT_LOCKED     0x01	/**<  bit0: 1 when PLL is not locked */
#define HPI_AESEBU_ERROR_POOR_QUALITY   0x02	/**<  bit1: 1 when signal quality is poor */
#define HPI_AESEBU_ERROR_PARITY_ERROR   0x04	/**< bit2: 1 when there is a parity error */
#define HPI_AESEBU_ERROR_BIPHASE_VIOLATION  0x08    /**<  bit3: 1 when there is a bi-phase coding violation */
#define HPI_AESEBU_ERROR_VALIDITY       0x10	/**<  bit4: 1 when the validity bit is high */

/** \defgroup tuner_defs Tuners
\{
*/
/** \defgroup tuner_attrs Tuner control attributes
\{
*/
#define HPI_TUNER_BAND                  1
#define HPI_TUNER_FREQ                  2
#define HPI_TUNER_LEVEL                 3
#define HPI_TUNER_AUDIOMUTE             4
#define HPI_TUNER_VIDEO_STATUS          5	/* AGE 11/10/97 <------------ use TUNER_STATUS instead */
#define HPI_TUNER_GAIN                      6	/* AGE 09/03/03 */
#define HPI_TUNER_STATUS                        7	/* SGT JUN-08-04 */
#define HPI_TUNER_MODE                  8
/** \} */

/** \defgroup tuner_bands Tuner bands

Used for HPI_Tuner_SetBand(),HPI_Tuner_GetBand()
\{
*/
#define HPI_TUNER_BAND_AM               1    /**< AM band */
#define HPI_TUNER_BAND_FM               2    /**< FM band (mono) */
#define HPI_TUNER_BAND_TV               3    /**< TV band = NTSC-M*/
#define HPI_TUNER_BAND_FM_STEREO        4    /**< FM band (stereo) */
#define HPI_TUNER_BAND_AUX              5    /**< Auxiliary input */
#define HPI_TUNER_BAND_TV_NTSC_M        HPI_TUNER_BAND_TV    /**< NTSC-M TV band*/
#define HPI_TUNER_BAND_TV_PAL_BG        6    /**< PAL-B/G TV band*/
#define HPI_TUNER_BAND_TV_PAL_I         7    /**< PAL-I TV band*/
#define HPI_TUNER_BAND_TV_PAL_DK        8    /**< PAL-D/K TV band*/
#define HPI_TUNER_BAND_TV_SECAM_L       9    /**< SECAM-L TV band*/
#define HPI_TUNER_BAND_LAST                             9 /**< The index of the last tuner band. */
/** \} */

/** Tuner mode attributes */
#define HPI_TUNER_MODE_RSS              1

/** RSS attribute values */
#define HPI_TUNER_MODE_RSS_DISABLE      0
#define HPI_TUNER_MODE_RSS_ENABLE       1

/** Tuner Level settings */
#define HPI_TUNER_LEVEL_AVERAGE         0
#define HPI_TUNER_LEVEL_RAW                     1

/** Tuner video status */
#define HPI_TUNER_VIDEO_STATUS_VALID                0x100	/* AGE 11/10/97 */
#define HPI_TUNER_VIDEO_COLOR_PRESENT               0x1	/* AGE 11/10/97 */
#define HPI_TUNER_VIDEO_IS_60HZ                     0x20	/* AGE 11/10/97 */
#define HPI_TUNER_VIDEO_HORZ_SYNC_MISSING           0x40	/* AGE 11/10/97 */
#define HPI_TUNER_PLL_LOCKED                                            0x1000
#define HPI_TUNER_FM_STEREO                                                     0x2000
/** \} */

#ifdef HPI_SUPPORT_ONOFFSWITCH
/* switch control attributes */
#define HPI_ONOFFSWITCH_STATE       1	/* AGE 1/15/98 */

#define HPI_SWITCH_OFF              0
#define HPI_SWITCH_ON               1
#endif

/* VOX control attributes */
#define HPI_VOX_THRESHOLD           1	/* AGE 9/10/98 */

/** Channel mode settings */
#define HPI_CHANNEL_MODE_NORMAL                 1	/* AGE 8/6/99 */
#define HPI_CHANNEL_MODE_SWAP                   2
#define HPI_CHANNEL_MODE_LEFT_TO_STEREO         3
#define HPI_CHANNEL_MODE_RIGHT_TO_STEREO        4
#define HPI_CHANNEL_MODE_STEREO_TO_LEFT         5
#define HPI_CHANNEL_MODE_STEREO_TO_RIGHT        6
#define HPI_CHANNEL_MODE_LAST                           6

/* Bitstream control set attributes */
#define HPI_BITSTREAM_DATA_POLARITY     1
#define HPI_BITSTREAM_CLOCK_EDGE        2
#define HPI_BITSTREAM_CLOCK_SOURCE      3

#define HPI_POLARITY_POSITIVE           0
#define HPI_POLARITY_NEGATIVE           1

/*
Currently fixed at EXTERNAL
#define HPI_SOURCE_EXTERNAL             0
#define HPI_SOURCE_INTERNAL             1
*/

/* Bitstream control get attributes */
#define HPI_BITSTREAM_ACTIVITY       1

/* SampleClock control attributes */
#define HPI_SAMPLECLOCK_SOURCE                  1
#define HPI_SAMPLECLOCK_SAMPLERATE              2
#define HPI_SAMPLECLOCK_SOURCE_INDEX    3
#define HPI_SAMPLECLOCK_LOCAL_SAMPLERATE        4

/** \defgroup sampleclock_source SampleClock source values
\{
*/
#define HPI_SAMPLECLOCK_SOURCE_ADAPTER          1 /**< on card samplerate generator, card is master */
#define HPI_SAMPLECLOCK_SOURCE_AESEBU_SYNC      2 /**< the dedicated sync input  */
/**
\deprecated HPI_SAMPLECLOCK_SOURCE_AESEBU_SYNC should be used instead
*/
#define HPI_SAMPLECLOCK_SOURCE_AESEBU HPI_SAMPLECLOCK_SOURCE_AESEBU_SYNC
#define HPI_SAMPLECLOCK_SOURCE_WORD                     3	/**< from external connector */
#define HPI_SAMPLECLOCK_SOURCE_WORD_HEADER      4	/**< board-to-board header */
#define HPI_SAMPLECLOCK_SOURCE_SMPTE            5	/**< not currently implemented */
#define HPI_SAMPLECLOCK_SOURCE_AESEBU_INPUT     6	/**< one of the aesebu inputs */
#define HPI_SAMPLECLOCK_SOURCE_AESEBU_AUTO      7	/**< the first aesebu input with a valid signal */
#define HPI_SAMPLECLOCK_SOURCE_COBRANET         8	/**< from Cobranet interface */
#define HPI_SAMPLECLOCK_SOURCE_LOCAL        9	/**< local PLL on module */
#define HPI_SAMPLECLOCK_SOURCE_PREV_MODULE 10	/**< from previous adjacent module */
/*! Update this if you add a new clock source2.*/
#define HPI_SAMPLECLOCK_SOURCE_LAST                     10
/** \} */

/* Microphone control attributes */
#define HPI_MICROPHONE_PHANTOM_POWER            1

/** Equalizer control attributes
*/
#define HPI_EQUALIZER_NUM_FILTERS       1	/*!< Used to get number of filters in an EQ. (Can't set) */
#define HPI_EQUALIZER_FILTER            2	/*!< Set/get the filter by type, freq, Q, gain */
#define HPI_EQUALIZER_COEFFICIENTS      3	/*!< Get the biquad coefficients */

/** \defgroup eq_filter_types Equalizer filter types
\{
Equalizer filter types, used by HPI_ParametricEQ_SetBand() */
	enum HPI_FILTER_TYPE {
		HPI_FILTER_TYPE_BYPASS = 0,	/*!< Filter is turned off */

		HPI_FILTER_TYPE_LOWSHELF = 1,	/*!< EQ low shelf */
		HPI_FILTER_TYPE_HIGHSHELF = 2,	/*!< EQ high shelf */
		HPI_FILTER_TYPE_EQ_BAND = 3,	/*!< EQ gain */

		HPI_FILTER_TYPE_LOWPASS = 4,	/*!< Standard low pass */
		HPI_FILTER_TYPE_HIGHPASS = 5,	/*!< Standard high pass */
		HPI_FILTER_TYPE_BANDPASS = 6,	/*!< Standard band pass */
		HPI_FILTER_TYPE_BANDSTOP = 7	/*!< Standard band stop/notch */
	};
/**\}*/

/* Cobranet control attributes. MUST be distinct from all other control attributes.
This is so that host side processing can easily identify a Cobranet control and
apply additional host side operations (like copying data) as required.
*/
#define HPI_COBRANET_SET         HPI_MAKE_ATTRIBUTE(HPI_CONTROL_COBRANET,1)
#define HPI_COBRANET_GET         HPI_MAKE_ATTRIBUTE(HPI_CONTROL_COBRANET,2)
#define HPI_COBRANET_SET_DATA    HPI_MAKE_ATTRIBUTE(HPI_CONTROL_COBRANET,3)
#define HPI_COBRANET_GET_DATA    HPI_MAKE_ATTRIBUTE(HPI_CONTROL_COBRANET,4)
#define HPI_COBRANET_GET_STATUS  HPI_MAKE_ATTRIBUTE(HPI_CONTROL_COBRANET,5)
#define HPI_COBRANET_SEND_PACKET HPI_MAKE_ATTRIBUTE(HPI_CONTROL_COBRANET,6)
#define HPI_COBRANET_GET_PACKET  HPI_MAKE_ATTRIBUTE(HPI_CONTROL_COBRANET,7)
#define HPI_COBRANET_MODE        HPI_MAKE_ATTRIBUTE(HPI_CONTROL_COBRANET,8)

/*------------------------------------------------------------
Cobranet mode options
------------------------------------------------------------*/
#define HPI_COBRANET_MODE_NETWORK       0	/**< device is networked with other CobaNet devices [default]. */
#define HPI_COBRANET_MODE_TETHERED      1	/**< device is directly connected to an ASI2416 */
#define HPI_COBRANET_MODE_QUERY     0	/**< stipulates a query to the mode control */
#define HPI_COBRANET_MODE_SET       1	/**< stipulates a set command to the mode control */
#define HPI_COBRANET_MODE_INVALID       0xff   /**< invalid CobraNet mode */

/*------------------------------------------------------------
Cobranet Chip Bridge - copied from HMI.H
------------------------------------------------------------*/
#define  HPI_COBRANET_HMI_cobraBridge                    0x20000
#define  HPI_COBRANET_HMI_cobraBridgeTxPktBuf            ( HPI_COBRANET_HMI_cobraBridge + 0x1000 )
#define  HPI_COBRANET_HMI_cobraBridgeRxPktBuf            ( HPI_COBRANET_HMI_cobraBridge + 0x2000 )
#define  HPI_COBRANET_HMI_cobraIfTable1                  0x110000
#define  HPI_COBRANET_HMI_cobraIfPhyAddress              ( HPI_COBRANET_HMI_cobraIfTable1 + 0xd )
#define  HPI_COBRANET_HMI_cobraProtocolIP                0x72000
#define  HPI_COBRANET_HMI_cobraIpMonCurrentIP            ( HPI_COBRANET_HMI_cobraProtocolIP + 0x0 )

/*------------------------------------------------------------
Cobranet Chip Status bits
------------------------------------------------------------*/
#define HPI_COBRANET_HMI_STATUS_RXPACKET 2
#define HPI_COBRANET_HMI_STATUS_TXPACKET 3

/*------------------------------------------------------------
Ethernet header size
------------------------------------------------------------*/
#define HPI_ETHERNET_HEADER_SIZE (16)

/* These defines are used to fill in protocol information for an Ethernet packet sent using HMI on CS18102 */
#define HPI_ETHERNET_PACKET_ID 0x85	/*!< ID supplied by Cirrius for ASI packets. */
#define HPI_ETHERNET_PACKET_V1 0x01	/*!< Simple packet - no special routing required */
#define HPI_ETHERNET_PACKET_HOSTED_VIA_HMI 0x20	/*!< This packet must make its way to the host across the HPI interface */
#define HPI_ETHERNET_PACKET_HOSTED_VIA_HMI_V1 0x21	/*!< This packet must make its way to the host across the HPI interface */
#define HPI_ETHERNET_PACKET_HOSTED_VIA_HPI 0x40	/*!< This packet must make its way to the host across the HPI interface */
#define HPI_ETHERNET_PACKET_HOSTED_VIA_HPI_V1 0x41	/*!< This packet must make its way to the host across the HPI interface */

#define HPI_ETHERNET_UDP_PORT (5151)	/*!< UDP messaging port */
#define HPI_ETHERNET_UDP_PORT_BROADCAST_RESPONSE (5152)	/*!< UDP message port for returning broadcast responses */

/** \defgroup tonedet_attr Tonedetector attributes
\{
Used by HPI_ToneDetector_Set() and HPI_ToneDetector_Get()
*/

/** Set the threshold level of a tonedetector,
Threshold is a -ve number in units of dB/100,
*/
#define HPI_TONEDETECTOR_THRESHOLD HPI_MAKE_ATTRIBUTE(HPI_CONTROL_TONEDETECTOR,1)
/** Get the current state of tonedetection
The result is a bitmap of detected tones.  pairs of bits represent the left and right
channels, with left channel in LSB.  The lowest frequency detector state is in the LSB
*/
#define HPI_TONEDETECTOR_STATE HPI_MAKE_ATTRIBUTE(HPI_CONTROL_TONEDETECTOR,2)

/** Get the frequency of a tonedetector band.
*/
#define HPI_TONEDETECTOR_FREQUENCY HPI_MAKE_ATTRIBUTE(HPI_CONTROL_TONEDETECTOR,3)

/**\}*/

/** \defgroup silencedet_attr SilenceDetector attributes
\{
*/

/** Get the current state of tonedetection
The result is a bitmap with 1s for silent channels. Left channel is in LSB
*/
#define HPI_SILENCEDETECTOR_STATE HPI_MAKE_ATTRIBUTE(HPI_CONTROL_SILENCEDETECTOR,2)

/** Set the threshold level of a SilenceDetector,
Threshold is a -ve number in units of dB/100,
*/
#define HPI_SILENCEDETECTOR_THRESHOLD HPI_MAKE_ATTRIBUTE(HPI_CONTROL_SILENCEDETECTOR,1)

/** get/set the silence time before the detector triggers
*/
#define HPI_SILENCEDETECTOR_DELAY HPI_MAKE_ATTRIBUTE(HPI_CONTROL_SILENCEDETECTOR,3)

/**\}*/

/******************************************* CONTROLX ATTRIBUTES ****/
/* NOTE: All controlx attributes must be unique, unlike control attributes */
/******************************************* ASYNC ATTRIBUTES ****/
/** \defgroup async_event Async Event sources
\{
*/
#define HPI_ASYNC_EVENT_GPIO    1		/**< GPIO event. */
#define HPI_ASYNC_EVENT_SILENCE 2		/**< Silence event detected. */
#define HPI_ASYNC_EVENT_TONE    3	    /**< tone event detected. */
/** \} */

/*******************************************/
/** \defgroup errorcodes Error codes
errors (codes 1-100 reserved for driver use)

\note WHEN A NEW ERROR CODE IS ADDED HPIFUNC.C::HPI_GetErrorText() SHOULD BE UPDATED
\{
*/
#define HPI_ERROR_INVALID_TYPE          100 /**< message type does not exist */
#define HPI_ERROR_INVALID_OBJ           101 /**< object type does not exist */
#define HPI_ERROR_INVALID_FUNC          102 /**< function does not exist */
#define HPI_ERROR_INVALID_OBJ_INDEX     103 /**< the specified object (adapter/Stream) does not exist */
#define HPI_ERROR_OBJ_NOT_OPEN          104 /**< trying to access an object that has not been opened yet */
#define HPI_ERROR_OBJ_ALREADY_OPEN      105 /**< trying to open an already open object */
#define HPI_ERROR_INVALID_RESOURCE      106 /**< PCI, ISA resource not valid */
#define HPI_ERROR_SUBSYSFINDADAPTERS_GETINFO      107 /**< GetInfo call from SubSysFindAdapters failed. */
#define HPI_ERROR_INVALID_RESPONSE      108 /**< Default response was never updated with actual error code */
#define HPI_ERROR_PROCESSING_MESSAGE    109 /**< wSize field of response was not updated, indicating that the msg was not processed */
#define HPI_ERROR_NETWORK_TIMEOUT       110 /**< The network did not respond in a timely manner */
#define HPI_ERROR_INVALID_HANDLE        111 /**< An HPI handle is invalid (uninitialised?) */
#define HPI_ERROR_UNIMPLEMENTED         112 /**< A function or attribute has not been implemented yet */

#define HPI_ERROR_TOO_MANY_ADAPTERS     200
#define HPI_ERROR_BAD_ADAPTER           201
#define HPI_ERROR_BAD_ADAPTER_NUMBER    202    /**< adapter number out of range or not set properly */
#define HPI_DUPLICATE_ADAPTER_NUMBER    203    /**< 2 adapters with the same adapter number */
#define HPI_ERROR_DSP_BOOTLOAD          204    /**< dsp code failed to bootload */
#define HPI_ERROR_DSP_SELFTEST          205    /**< adapter falied DSP code self test */
#define HPI_ERROR_DSP_FILE_NOT_FOUND    206    /**< couldn't find or open the DSP code file */
#define HPI_ERROR_DSP_HARDWARE          207    /**< internal DSP hardware error */
#define HPI_ERROR_DOS_MEMORY_ALLOC      208    /**< could not allocate memory in DOS */
#define HPI_ERROR_PLD_LOAD              209    /**< failed to correctly load/config PLD */
#define HPI_ERROR_DSP_FILE_FORMAT               210    /**< Unexpected end of file, block length too big etc */

#define HPI_ERROR_DSP_FILE_ACCESS_DENIED 211	/**< Found but could not open DSP code file */
#define HPI_ERROR_DSP_FILE_NO_HEADER    212    /**< First DSP code section header not found in DSP file*/
#define HPI_ERROR_DSP_FILE_READ_ERROR   213    /**< File read operation on DSP code file failed*/
#define HPI_ERROR_DSP_SECTION_NOT_FOUND 214    /**< DSP code for adapter family not found */
#define HPI_ERROR_DSP_FILE_OTHER_ERROR  215    /**< Other OS specific error opening DSP file */
#define HPI_ERROR_DSP_FILE_SHARING_VIOLATION 216	/**< Sharing violation opening DSP code file */
#define HPI_ERROR_DSP_FILE_NULL_HEADER  217    /**< DSP code section header had size == 0*/

#define HPI_ERROR_FLASH 220						/**< Base number for flash errors */

#define HPI_ERROR_BAD_CHECKSUM (HPI_ERROR_FLASH+1)
#define HPI_ERROR_BAD_SEQUENCE (HPI_ERROR_FLASH+2)
#define HPI_ERROR_FLASH_ERASE (HPI_ERROR_FLASH+3)
#define HPI_ERROR_FLASH_PROGRAM (HPI_ERROR_FLASH+4)
#define HPI_ERROR_FLASH_VERIFY (HPI_ERROR_FLASH+5)
#define HPI_ERROR_FLASH_TYPE (HPI_ERROR_FLASH+6)
#define HPI_ERROR_FLASH_START (HPI_ERROR_FLASH+7)

#define HPI_ERROR_RESERVED_1            290    /**< Reserved for OEMs */

#define HPI_ERROR_INVALID_STREAM        300 /**< stream does not exist */
#define HPI_ERROR_INVALID_FORMAT        301 /**< invalid compression format */
#define HPI_ERROR_INVALID_SAMPLERATE    302 /**< invalid format samplerate */
#define HPI_ERROR_INVALID_CHANNELS      303 /**< invalid format number of channels */
#define HPI_ERROR_INVALID_BITRATE       304 /**< invalid format bitrate */
#define HPI_ERROR_INVALID_DATASIZE      305 /**< invalid datasize used for stream read/write */
#define HPI_ERROR_BUFFER_FULL           306 /**< stream buffer is full during stream write*/
#define HPI_ERROR_BUFFER_EMPTY          307 /**< stream buffer is empty during stream read*/
#define HPI_ERROR_INVALID_DATA_TRANSFER 308 /**< invalid datasize used for stream read/write */
#define HPI_ERROR_INVALID_OPERATION     310 /**< object can't do requested operation in its
current state, e.g. set format, change rec mux
state while recording
*/
#define HPI_ERROR_INCOMPATIBLE_SAMPLERATE 311 /**< Where an SRG is shared amongst streams, an
incompatible samplerate is one that is different to
any currently playing or recording stream
*/
#define HPI_ERROR_BAD_ADAPTER_MODE      312
#define HPI_ERROR_TOO_MANY_CAPABILITY_CHANGE_ATTEMPTS 313 /**< There have been to attempts to set the adapter's
capabilities (using bad keys). The card should be returned
to ASI if further capabilities updates are required */
#define HPI_ERROR_NO_INTERADAPTER_GROUPS 314 /**< Streams on different adapters cannot be grouped*/
#define HPI_ERROR_NO_INTERDSP_GROUPS 315 /**< Streams on different DSPs cannot be grouped*/

/** mixer controls */
#define HPI_ERROR_INVALID_NODE          400
#define HPI_ERROR_INVALID_CONTROL       401
#define HPI_ERROR_INVALID_CONTROL_VALUE  402
#define HPI_ERROR_INVALID_CONTROL_ATTRIBUTE 403
#define HPI_ERROR_CONTROL_DISABLED      404
#define HPI_ERROR_CONTROL_I2C_MISSING_ACK 405

/**Non volatie memory */
#define HPI_ERROR_NVMEM_BUSY                    450
#define HPI_ERROR_NVMEM_FULL                    451
#define HPI_ERROR_NVMEM_FAIL                    452

#define HPI_ERROR_CUSTOM                600 /**< custom error to use for debugging AGE 6/22/99 */

#define HPI_ERROR_MUTEX_TIMEOUT         700 /**< hpioct32.c can't obtain mutex */

#define HPI_ERROR_ILLEGAL_CACHE_VALUE   0xffff /**< indicates a cached u16 value is invalid. */
/**\}*/

/* maximums */
#define HPI_MAX_ADAPTERS        20     /**< Maximum number of adapters per HPI sub-system WARNING: modifying this value changes the response structure size.*/
#define HPI_MAX_STREAMS         16	 /**< Maximum number of in or out streams per adapter */
#define HPI_MAX_CHANNELS        2	/* per stream */
#define HPI_MAX_NODES           8	/* per mixer ? */
#define HPI_MAX_CONTROLS        4	/* per node ? */
#define HPI_MAX_ANC_BYTES_PER_FRAME             (64)	/**< maximum number of ancillary bytes per MPEG frame */
#define HPI_STRING_LEN          16

/* units */
#define HPI_OSTREAM_VELOCITY_UNITS        4096	/* AGE 6/4/99 */
#define HPI_OSTREAM_TIMESCALE_UNITS       (10000)

/**
end group hpi_defines
\}

*/
/* ////////////////////////////////////////////////////////////////////// */
/* STRUCTURES */
#ifndef DISABLE_PRAGMA_PACK1
#pragma pack(push,1)
#endif

#ifndef SWIG
	typedef union {
		struct {
			unsigned int hwassembly:3;
				/**< Assembly variant 0..7 */
			unsigned int hwrev:4;
				/**< encode A-P as 0-15    */
			unsigned int swminor:6;
				/**< sw minor version 0-63 */
			unsigned int swmajor:3;
				/**< sw major version 0-7  */
		} s;
		u16 w;
		u32 dw;
	} HPI_VERSION;
#endif

	typedef struct sHPI_FORMAT {
		u32 dwSampleRate;
		       /**< 11025, 32000, 44100 ... */
		u32 dwBitRate;
		       /**< for MPEG */
		u32 dwAttributes;
		       /**< Stereo/JointStereo/Mono */
		u16 wModeLegacy;
			     /**< Legacy ancillary mode or idle bit  */
		u16 wUnused;
			 /**< Unused */
		u16 wChannels;
		       /**< 1,2..., (or ancillary mode or idle bit */
		u16 wFormat;
		       /**< HPI_FORMAT_PCM16, _AC2, _MPEG ... */
	} HPI_FORMAT;

#ifndef HPI_WITHOUT_HPI_DATA
/**
Don't access fields in HPI_DATA.  Preferably switch  to
using HPI_OutStreamWriteBuf(), HPI_InStreamReadBuf().
Less desirably, always use HPI_DataCreate() to fill in the fields of
HPI_DATA.

\deprecated HPI_DATA will eventually disappear from this API
*/
	typedef struct sHPI_DATA {
		u32 opaque_fixed_size[7];
	} HPI_DATA;
#endif

	typedef struct {
		u32 dwValidBitsInThisFrame;
		u8 bData[HPI_MAX_ANC_BYTES_PER_FRAME];
	} HPI_ANC_FRAME;

	typedef struct {
		u32 dwPunchInSample;
		u32 dwPunchOutSample;
	} HPI_PUNCHINOUT;

	typedef struct {
		u16 wObjectType;
		       /**< Type of object, HPI_OBJ_OSTREAM or HPI_OBJ_ISTREAM. */
		u16 wStreamIndex;
		   /**< OStream or IStream index. */
	} HPI_STREAMID;

/** An object for containing a single async event.
*/
	typedef struct {
		u16 wEventType;	 /**< Type of event. See HPI_ASYNC_EVENT_GPIO (etc.) defines. */
		u16 wSequence;	 /**< Sequence number of event, allows lost event detection */
		u32 dwState;	     /**< New state */
		u32 hObject;	 /**< Handle to the object returning the event. */
		union {
			struct {
				u16 wIndex;
		       /**< GPIO bit index. */
			} gpio;
			struct {
				u16 wNodeIndex;
			       /**< What node is the control on ? */
				u16 wNodeType;
			       /**< What type of node is the control on ? */
			} control;
		} u;
	} HPI_ASYNC_EVENT;

	typedef unsigned char HPI_ETHERNET_MAC_ADR[6];
						/**< Used for sending ethernet packets VIA HMI interface */

/*////////////////////////////////////////////////////////////////////////// */
/* HPI FUNCTIONS */
	typedef u16 HPI_ERR;
	typedef u16 HPI_BOOL;
	typedef u32 HPI_HANDLE;

/* handles that reference various objects */
	typedef u16 HPI_HADAPTER;

	typedef HPI_HANDLE HPI_HOSTREAM;
	typedef HPI_HANDLE HPI_HISTREAM;
	typedef HPI_HANDLE HPI_HSTREAM;	/* either InStream or OutStream */
	typedef HPI_HANDLE HPI_HMIXER;
	typedef HPI_HANDLE HPI_HCONTROL;
	typedef HPI_HANDLE HPI_HNVMEMORY;
	typedef HPI_HANDLE HPI_HGPIO;
	typedef HPI_HANDLE HPI_HWATCHDOG;
	typedef HPI_HANDLE HPI_HCLOCK;
	typedef HPI_HANDLE HPI_HPROFILE;
	typedef HPI_HANDLE HPI_HASYNC;

#ifndef HPI_ON_DSP
/* skip host side function declarations for DSP compile and documentation extraction */

	typedef struct {
		int not_really_used;
	} HPI_HSUBSYS, *PHPI_HSUBSYS;

#ifndef DISABLE_PRAGMA_PACK1
#pragma pack(pop)
#endif

/*/////////////////////////// */
/* DATA and FORMAT and STREAM */

	u16 HPI_StreamEstimateBufferSize(HPI_FORMAT * pF,
					 u32 dwHostPollingRateInMilliSeconds,
					 u32 * dwRecommendedBufferSize);

/*/////////// */
/* SUB SYSTEM */
	PHPI_HSUBSYS HPI_SubSysCreate(void);

	void HPI_SubSysFree(HPI_HSUBSYS * phSubSys);

	u16 HPI_SubSysGetVersion(HPI_HSUBSYS * phSubSys, u32 * pdwVersion);

	u16 HPI_SubSysGetVersionEx(HPI_HSUBSYS * phSubSys, u32 * pdwVersionEx);

	u16 HPI_SubSysGetInfo(HPI_HSUBSYS * phSubSys,
			      u32 * pdwVersion,
			      u16 * pwNumAdapters,
			      u16 awAdapterList[], u16 wListLength);

/* SGT added 3-2-97 */
	u16 HPI_SubSysFindAdapters(HPI_HSUBSYS * phSubSys,
				   u16 * pwNumAdapters,
				   u16 awAdapterList[], u16 wListLength);

/*///////// */
/* ADAPTER */

	u16 HPI_AdapterOpen(HPI_HSUBSYS * phSubSys, u16 wAdapterIndex);

	u16 HPI_AdapterClose(HPI_HSUBSYS * phSubSys, u16 wAdapterIndex);

	u16 HPI_AdapterGetInfo(HPI_HSUBSYS * phSubSys,
			       u16 wAdapterIndex,
			       u16 * pwNumOutStreams,
			       u16 * pwNumInStreams,
			       u16 * pwVersion,
			       u32 * pdwSerialNumber, u16 * pwAdapterType);

	u16 HPI_AdapterSetMode(HPI_HSUBSYS * phSubSys,
			       u16 wAdapterIndex, u32 dwAdapterMode);
	u16 HPI_AdapterSetModeEx(HPI_HSUBSYS * phSubSys,
				 u16 wAdapterIndex,
				 u32 dwAdapterMode, u16 wQueryOrSet);

	u16 HPI_AdapterGetMode(HPI_HSUBSYS * phSubSys,
			       u16 wAdapterIndex, u32 * pdwAdapterMode);

	u16 HPI_AdapterGetAssert(HPI_HSUBSYS * phSubSys,
				 u16 wAdapterIndex,
				 u16 * wAssertPresent,
				 char *pszAssert, u16 * pwLineNumber);

	u16 HPI_AdapterGetAssertEx(HPI_HSUBSYS * phSubSys,
				   u16 wAdapterIndex,
				   u16 * wAssertPresent,
				   char *pszAssert,
				   u32 * pdwLineNumber, u16 * pwAssertOnDsp);

	u16 HPI_AdapterTestAssert(HPI_HSUBSYS * phSubSys,
				  u16 wAdapterIndex, u16 wAssertId);

	u16 HPI_AdapterEnableCapability(HPI_HSUBSYS * phSubSys,
					u16 wAdapterIndex,
					u16 wCapability, u32 dwKey);

	u16 HPI_AdapterSelfTest(HPI_HSUBSYS * phSubSys, u16 wAdapterIndex);

	u16 HPI_AdapterSetProperty(HPI_HSUBSYS * phSubSys,
				   u16 wAdapterIndex,
				   u16 wProperty,
				   u16 wParamter1, u16 wParamter2);

	u16 HPI_AdapterGetProperty(HPI_HSUBSYS * phSubSys,
				   u16 wAdapterIndex,
				   u16 wProperty,
				   u16 * pwParamter1, u16 * pwParamter2);

	u16 HPI_AdapterFindObject(const HPI_HSUBSYS * phSubSys,
				  u16 wAdapterIndex,
				  u16 wObjectType,
				  u16 wObjectIndex, u16 * pDspIndex);

	u16 HPI_AdapterEnumerateProperty(HPI_HSUBSYS * phSubSys,
					 u16 wAdapterIndex,
					 u16 wIndex,
					 u16 wWhatToEnumerate,
					 u16 wPropertyIndex, u32 * pdwSetting);

/*////////////// */
/* NonVol Memory */
	u16 HPI_NvMemoryOpen(HPI_HSUBSYS * phSubSys,
			     u16 wAdapterIndex,
			     HPI_HNVMEMORY * phNvMemory, u16 * pwSizeInBytes);

	u16 HPI_NvMemoryReadByte(HPI_HSUBSYS * phSubSys,
				 HPI_HNVMEMORY hNvMemory,
				 u16 wIndex, u16 * pwData);

	u16 HPI_NvMemoryWriteByte(HPI_HSUBSYS * phSubSys,
				  HPI_HNVMEMORY hNvMemory,
				  u16 wIndex, u16 wData);

/*////////////// */
/* Digital I/O */
	u16 HPI_GpioOpen(HPI_HSUBSYS * phSubSys,
			 u16 wAdapterIndex,
			 HPI_HGPIO * phGpio,
			 u16 * pwNumberInputBits, u16 * pwNumberOutputBits);

	u16 HPI_GpioReadBit(HPI_HSUBSYS * phSubSys,
			    HPI_HGPIO hGpio, u16 wBitIndex, u16 * pwBitData);

	u16 HPI_GpioReadAllBits(HPI_HSUBSYS * phSubSys,
				HPI_HGPIO hGpio, u16 * pwBitData);

	u16 HPI_GpioWriteBit(HPI_HSUBSYS * phSubSys,
			     HPI_HGPIO hGpio, u16 wBitIndex, u16 wBitData);

/*/////////////////// */
/* Async Event Object */
	u16 HPI_AsyncEventOpen(HPI_HSUBSYS * phSubSys,
			       u16 wAdapterIndex, HPI_HASYNC * phAsync);

	u16 HPI_AsyncEventClose(HPI_HSUBSYS * phSubSys, HPI_HASYNC hAsync);

	u16 HPI_AsyncEventWait(HPI_HSUBSYS * phSubSys,
			       HPI_HASYNC hAsync,
			       u16 wMaximumEvents,
			       HPI_ASYNC_EVENT * pEvents,
			       u16 * pwNumberReturned);

	u16 HPI_AsyncEventGetCount(HPI_HSUBSYS * phSubSys,
				   HPI_HASYNC hAsync, u16 * pwCount);

	u16 HPI_AsyncEventGet(HPI_HSUBSYS * phSubSys,
			      HPI_HASYNC hAsync,
			      u16 wMaximumEvents,
			      HPI_ASYNC_EVENT * pEvents,
			      u16 * pwNumberReturned);

/*/////////// */
/* WATCH-DOG  */
	u16 HPI_WatchdogOpen(HPI_HSUBSYS * phSubSys,
			     u16 wAdapterIndex, HPI_HWATCHDOG * phWatchdog);

	u16 HPI_WatchdogSetTime(HPI_HSUBSYS * phSubSys,
				HPI_HWATCHDOG hWatchdog, u32 dwTimeMillisec);

	u16 HPI_WatchdogPing(HPI_HSUBSYS * phSubSys, HPI_HWATCHDOG hWatchdog);

/*/////////// */
/* OUT STREAM */
	u16 HPI_OutStreamOpen(HPI_HSUBSYS * phSubSys,
			      u16 wAdapterIndex,
			      u16 wOutStreamIndex,
			      HPI_HOSTREAM * phOutStreamHandle);

	u16 HPI_OutStreamClose(HPI_HSUBSYS * phSubSys,
			       HPI_HOSTREAM hOutStreamHandle);

	DEPRECATED u16 HPI_OutStreamGetInfo(HPI_HSUBSYS * phSubSys,
					    HPI_HOSTREAM hOutStreamHandle,
					    u16 * pwState,
					    u32 * pdwBufferSize,
					    u32 * pdwDataToPlay);

	u16 HPI_OutStreamGetInfoEx(HPI_HSUBSYS * phSubSys,
				   HPI_HOSTREAM hOutStreamHandle,
				   u16 * pwState,
				   u32 * pdwBufferSize,
				   u32 * pdwDataToPlay,
				   u32 * pdwSamplesPlayed,
				   u32 * pdwAuxiliaryDataToPlay);

	u16 HPI_OutStreamWriteBuf(HPI_HSUBSYS * phSubSys,
				  HPI_HOSTREAM hOutStream,
				  u8 * pbWriteBuf,
				  u32 dwBytesToWrite, HPI_FORMAT * pFormat);

#ifndef HPI_WITHOUT_HPI_DATA
	DEPRECATED u16 HPI_OutStreamWrite(HPI_HSUBSYS * phSubSys,
					  HPI_HOSTREAM hOutStreamHandle,
					  HPI_DATA * pData);
#endif

	u16 HPI_OutStreamStart(HPI_HSUBSYS * phSubSys,
			       HPI_HOSTREAM hOutStreamHandle);

	u16 HPI_OutStreamStop(HPI_HSUBSYS * phSubSys,
			      HPI_HOSTREAM hOutStreamHandle);

	u16 HPI_OutStreamSinegen(HPI_HSUBSYS * phSubSys,
				 HPI_HOSTREAM hOutStream);

	u16 HPI_OutStreamReset(HPI_HSUBSYS * phSubSys,
			       HPI_HOSTREAM OutStreamHandle);

	u16 HPI_OutStreamQueryFormat(HPI_HSUBSYS * phSubSys,
				     HPI_HOSTREAM OutStreamHandle,
				     HPI_FORMAT * pFormat);

	u16 HPI_OutStreamSetPunchInOut(HPI_HSUBSYS * phSubSys,
				       HPI_HOSTREAM hOutStreamHandle,
				       u32 dwPunchInSample,
				       u32 dwPunchOutSample);

	u16 HPI_OutStreamSetVelocity(HPI_HSUBSYS * phSubSys,
				     HPI_HOSTREAM hOutStream, short nVelocity);

	u16 HPI_OutStreamAncillaryReset(HPI_HSUBSYS * phSubSys, HPI_HOSTREAM OutStreamHandle, u16 wMode);	/* wMode is HPI_MPEG_ANC_XXX */

	u16 HPI_OutStreamAncillaryGetInfo(HPI_HSUBSYS * phSubSys,
					  HPI_HOSTREAM hOutStreamHandle,
					  u32 * pdwFramesAvailable);

	u16 HPI_OutStreamAncillaryRead(HPI_HSUBSYS * phSubSys,
				       HPI_HOSTREAM hOutStreamHandle,
				       HPI_ANC_FRAME * pAncFrameBuffer,
				       u32 dwAncFrameBufferSizeInBytes,
				       u32 dwNumberOfAncillaryFramesToRead);

	u16 HPI_OutStreamSetTimeScale(HPI_HSUBSYS * phSubSys,
				      HPI_HOSTREAM hOutStreamHandle,
				      u32 dwTimeScaleX10000);

	u16 HPI_OutStreamHostBufferAllocate(HPI_HSUBSYS * phSubSys,
					    HPI_HOSTREAM hOutStreamHandle,
					    u32 dwSizeInBytes);

	u16 HPI_OutStreamHostBufferFree(HPI_HSUBSYS * phSubSys,
					HPI_HOSTREAM hOutStreamHandle);

	u16 HPI_OutStreamGroupAdd(HPI_HSUBSYS * phSubSys,
				  HPI_HOSTREAM hOutStreamHandle,
				  HPI_HSTREAM hStreamHandle);

	u16 HPI_OutStreamGroupGetMap(HPI_HSUBSYS * phSubSys,
				     HPI_HOSTREAM hOutStreamHandle,
				     u32 * pdwOutStreamMap,
				     u32 * pdwInStreamMap);

	u16 HPI_OutStreamGroupReset(HPI_HSUBSYS * phSubSys,
				    HPI_HOSTREAM hOutStreamHandle);

/*////////// */
/* IN_STREAM */
	u16 HPI_InStreamOpen(HPI_HSUBSYS * phSubSys,
			     u16 wAdapterIndex,
			     u16 wInStreamIndex, HPI_HISTREAM * phInStream);

	u16 HPI_InStreamClose(HPI_HSUBSYS * phSubSys, HPI_HISTREAM hInStream);

	u16 HPI_InStreamQueryFormat(HPI_HSUBSYS * phSubSys,
				    HPI_HISTREAM hInStream,
				    HPI_FORMAT * pFormat);

	u16 HPI_InStreamSetFormat(HPI_HSUBSYS * phSubSys,
				  HPI_HISTREAM hInStream, HPI_FORMAT * pFormat);

	u16 HPI_InStreamReadBuf(HPI_HSUBSYS * phSubSys,
				HPI_HISTREAM hInStream,
				u8 * pbReadBuf, u32 dwBytesToRead);

#ifndef HPI_WITHOUT_HPI_DATA
	DEPRECATED u16 HPI_InStreamRead(HPI_HSUBSYS * phSubSys,
					HPI_HISTREAM hInStream,
					HPI_DATA * pData);
#endif

	u16 HPI_InStreamStart(HPI_HSUBSYS * phSubSys, HPI_HISTREAM hInStream);

	u16 HPI_InStreamStop(HPI_HSUBSYS * phSubSys, HPI_HISTREAM hInStream);

	u16 HPI_InStreamReset(HPI_HSUBSYS * phSubSys, HPI_HISTREAM hInStream);

	DEPRECATED u16 HPI_InStreamGetInfo(HPI_HSUBSYS * phSubSys,
					   HPI_HISTREAM hInStream,
					   u16 * pwState,
					   u32 * pdwBufferSize,
					   u32 * pdwDataRecorded);

	u16 HPI_InStreamGetInfoEx(HPI_HSUBSYS * phSubSys,
				  HPI_HISTREAM hInStream,
				  u16 * pwState,
				  u32 * pdwBufferSize,
				  u32 * pdwDataRecorded,
				  u32 * pdwSamplesRecorded,
				  u32 * pdwAuxiliaryDataRecorded);

	u16 HPI_InStreamAncillaryReset(HPI_HSUBSYS * phSubSys, HPI_HISTREAM InStreamHandle, u16 wBytesPerFrame, u16 wMode,	/* = HPI_MPEG_ANC_XXX */
				       u16 wAlignment, u16 wIdleBit);

	u16 HPI_InStreamAncillaryGetInfo(HPI_HSUBSYS * phSubSys,
					 HPI_HISTREAM hInStreamHandle,
					 u32 * pdwFrameSpace);

	u16 HPI_InStreamAncillaryWrite(HPI_HSUBSYS * phSubSys,
				       HPI_HISTREAM hInStream,
				       HPI_ANC_FRAME * pAncFrameBuffer,
				       u32 dwAncFrameBufferSizeInBytes,
				       u32 dwNumberOfAncillaryFramesToWrite);

	u16 HPI_InStreamHostBufferAllocate(HPI_HSUBSYS * phSubSys,
					   HPI_HISTREAM hInStream,
					   u32 dwSizeInBytes);

	u16 HPI_InStreamHostBufferFree(HPI_HSUBSYS * phSubSys,
				       HPI_HISTREAM hInStream);

	u16 HPI_InStreamGroupAdd(HPI_HSUBSYS * phSubSys,
				 HPI_HISTREAM hInStreamHandle,
				 HPI_HSTREAM hStreamHandle);

	u16 HPI_InStreamGroupGetMap(HPI_HSUBSYS * phSubSys,
				    HPI_HISTREAM hInStreamHandle,
				    u32 * pdwOutStreamMap,
				    u32 * pdwInStreamMap);

	u16 HPI_InStreamGroupReset(HPI_HSUBSYS * phSubSys,
				   HPI_HISTREAM hInStreamHandle);

/*////// */
/* MIXER */
	u16 HPI_MixerOpen(HPI_HSUBSYS * phSubSys,
			  u16 wAdapterIndex, HPI_HMIXER * phMixerHandle);

	u16 HPI_MixerClose(HPI_HSUBSYS * phSubSys, HPI_HMIXER hMixerHandle);

	u16 HPI_MixerGetControl(HPI_HSUBSYS * phSubSys,
				HPI_HMIXER hMixerHandle,
				u16 wSrcNodeType,
				u16 wSrcNodeTypeIndex,
				u16 wDstNodeType,
				u16 wDstNodeTypeIndex,
				u16 wControlType,
				HPI_HCONTROL * phControlHandle);

	u16 HPI_MixerGetControlByIndex(HPI_HSUBSYS * phSubSys,
				       HPI_HMIXER hMixerHandle,
				       u16 wControlIndex,
				       u16 * pwSrcNodeType,
				       u16 * pwSrcNodeIndex,
				       u16 * pwDstNodeType,
				       u16 * pwDstNodeIndex,
				       u16 * pwControlType,
				       HPI_HCONTROL * phControlHandle);

	u16 HPI_MixerStore(HPI_HSUBSYS * phSubSys,
			   HPI_HMIXER hMixerHandle,
			   enum HPI_MIXER_STORE_COMMAND Command, u16 wIndex);
/*************************/
/* mixer CONTROLS        */
/*************************/

/* Generic query of available control settings */
	u16 HPI_ControlQuery(const HPI_HSUBSYS * phSubSys,
			     const HPI_HCONTROL hControlHandle,
			     const u16 wAttrib,
			     const u32 dwIndex,
			     const u32 dwParam, u32 * pdwSetting);

#ifndef SWIG
/* Generic setting of control attribute value */
	u16 HPI_ControlParamSet(const HPI_HSUBSYS * phSubSys,
				const HPI_HCONTROL hControlHandle,
				const u16 wAttrib,
				const u32 dwParam1, const u32 dwParam2);

/* generic getting of control attribute value.
Null pointers allowed for return values
*/
	u16 HPI_ControlParamGet(const HPI_HSUBSYS * phSubSys,
				const HPI_HCONTROL hControlHandle,
				const u16 wAttrib,
				u32 dwParam1,
				u32 dwParam2, u32 * pdwParam1, u32 * pdwParam2);
#endif

#ifdef HPI_GENERATE_FUNCTIONS

#define fnHPI_Control_Get(object,name,attribute, type) \
u16  HPI_##object##_Get##name( \
HPI_HSUBSYS *phSubSys, HPI_HCONTROL hControl, type * dwAttrValue \
);\
/** Get the #name attribute of # object */ \
u16  HPI_##object##_Get##name( \
HPI_HSUBSYS *phSubSys, HPI_HCONTROL hControl, type * dwAttrValue \
)\
{ return HPI_ControlParamGet(phSubSys, hControl, attribute, 0,0,(u32 *)dwAttrValue, NULL); } \

#define fnHPI_Control_Set(object,name,attribute, type) \
u16  HPI_##object##_Set##name( \
HPI_HSUBSYS *phSubSys, HPI_HCONTROL hControl,type dwAttrValue \
);\
u16  HPI_##object##_Set##name( \
HPI_HSUBSYS *phSubSys, HPI_HCONTROL hControl,type dwAttrValue \
);\
u16  HPI_##object##_Set##name( \
HPI_HSUBSYS *phSubSys, HPI_HCONTROL hControl,type dwAttrValue \
)\
{ return HPI_ControlParamSet(phSubSys, hControl, attribute, (u32)dwAttrValue, 0); } \

#define fnHPI_Control_SetGet(object,name,attribute, type) \
fnHPI_Control_Set(object,name,attribute, type) \
fnHPI_Control_Get(object,name,attribute, type) \


#else

#define fnHPI_Control_Get(object,name,attribute, type) \
u16  HPI_##object##_Get##name( \
HPI_HSUBSYS *phSubSys, HPI_HCONTROL hControl, type * dwAttrValue \
);\

#define fnHPI_Control_SetGet(object,name,attribute, type) \
fnHPI_Control_Get(object,name,attribute, type) \
u16  HPI_##object##_Set##name( \
HPI_HSUBSYS *phSubSys, HPI_HCONTROL hControl,type dwAttrValue \
);\

#endif

/*************************/
/* volume control        */
/*************************/
	u16 HPI_VolumeSetGain(HPI_HSUBSYS * phSubSys,
			      HPI_HCONTROL hControlHandle,
			      short anGain0_01dB[HPI_MAX_CHANNELS]
	    );

	u16 HPI_VolumeGetGain(HPI_HSUBSYS * phSubSys,
			      HPI_HCONTROL hControlHandle,
			      short anGain0_01dB_out[HPI_MAX_CHANNELS]
	    );

#define HPI_VolumeGetRange HPI_VolumeQueryRange
	u16 HPI_VolumeQueryRange(HPI_HSUBSYS * phSubSys,
				 HPI_HCONTROL hControlHandle,
				 short *nMinGain_01dB,
				 short *nMaxGain_01dB, short *nStepGain_01dB);

	u16 HPI_VolumeAutoFade(HPI_HSUBSYS * phSubSys,
			       HPI_HCONTROL hControlHandle,
			       short anStopGain0_01dB[HPI_MAX_CHANNELS],
			       u32 wDurationMs);

	u16 HPI_VolumeAutoFadeProfile(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, short anStopGain0_01dB[HPI_MAX_CHANNELS], u32 dwDurationMs, u16 dwProfile	/* HPI_VOLUME_AUTOFADE_??? */
	    );

/*************************/
/* level control         */
/*************************/
	u16 HPI_LevelSetGain(HPI_HSUBSYS * phSubSys,
			     HPI_HCONTROL hControlHandle,
			     short anGain0_01dB[HPI_MAX_CHANNELS]
	    );

	u16 HPI_LevelGetGain(HPI_HSUBSYS * phSubSys,
			     HPI_HCONTROL hControlHandle,
			     short anGain0_01dB_out[HPI_MAX_CHANNELS]
	    );

/*************************/
/* meter control         */
/*************************/
	u16 HPI_MeterGetPeak(HPI_HSUBSYS * phSubSys,
			     HPI_HCONTROL hControlHandle,
			     short anPeak0_01dB_out[HPI_MAX_CHANNELS]
	    );

	u16 HPI_MeterGetRms(HPI_HSUBSYS * phSubSys,
			    HPI_HCONTROL hControlHandle,
			    short anPeak0_01dB_out[HPI_MAX_CHANNELS]
	    );

	u16 HPI_MeterSetPeakBallistics(HPI_HSUBSYS * phSubSys,
				       HPI_HCONTROL hControlHandle,
				       unsigned short nAttack,
				       unsigned short nDecay);

	u16 HPI_MeterSetRmsBallistics(HPI_HSUBSYS * phSubSys,
				      HPI_HCONTROL hControlHandle,
				      unsigned short nAttack,
				      unsigned short nDecay);

	u16 HPI_MeterGetPeakBallistics(HPI_HSUBSYS * phSubSys,
				       HPI_HCONTROL hControlHandle,
				       unsigned short *nAttack,
				       unsigned short *nDecay);

	u16 HPI_MeterGetRmsBallistics(HPI_HSUBSYS * phSubSys,
				      HPI_HCONTROL hControlHandle,
				      unsigned short *nAttack,
				      unsigned short *nDecay);

/*************************/
/* channel mode control  */
/*************************/
	u16 HPI_ChannelModeSet(HPI_HSUBSYS * phSubSys,
			       HPI_HCONTROL hControlHandle, u16 wMode);

	u16 HPI_ChannelModeGet(HPI_HSUBSYS * phSubSys,
			       HPI_HCONTROL hControlHandle, u16 * wMode);

/*************************/
/* Tuner control         */
/*************************/
	u16 HPI_Tuner_SetBand(HPI_HSUBSYS * phSubSys,
			      HPI_HCONTROL hControlHandle, u16 wBand);

	u16 HPI_Tuner_GetBand(HPI_HSUBSYS * phSubSys,
			      HPI_HCONTROL hControlHandle, u16 * pwBand);

	u16 HPI_Tuner_SetFrequency(HPI_HSUBSYS * phSubSys,
				   HPI_HCONTROL hControlHandle, u32 wFreqInkHz);

	u16 HPI_Tuner_GetFrequency(HPI_HSUBSYS * phSubSys,
				   HPI_HCONTROL hControlHandle,
				   u32 * pwFreqInkHz);

	u16 HPI_Tuner_GetRFLevel(HPI_HSUBSYS * phSubSys,
				 HPI_HCONTROL hControlHandle, short *pwLevel);

	u16 HPI_Tuner_GetRawRFLevel(HPI_HSUBSYS * phSubSys,
				    HPI_HCONTROL hControlHandle,
				    short *pwLevel);

	u16 HPI_Tuner_SetGain(HPI_HSUBSYS * phSubSys,
			      HPI_HCONTROL hControlHandle, short nGain);

	u16 HPI_Tuner_GetGain(HPI_HSUBSYS * phSubSys,
			      HPI_HCONTROL hControlHandle, short *pnGain);

	DEPRECATED u16 HPI_Tuner_GetVideoStatus(	/* AGE 11/10/97 */
						       HPI_HSUBSYS * phSubSys,
						       HPI_HCONTROL
						       hControlHandle,
						       u16 * pwVideoStatus);

/* SGT proposed */
	u16 HPI_Tuner_GetStatus(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 * pwStatusMask,	// tells you which bits are valid
				u16 * pwStatus	// the actual bits
	    );

	u16 HPI_Tuner_SetMode(HPI_HSUBSYS * phSubSys,
			      HPI_HCONTROL hControlHandle,
			      u32 nMode, u32 nValue);

	u16 HPI_Tuner_GetMode(HPI_HSUBSYS * phSubSys,
			      HPI_HCONTROL hControlHandle,
			      u32 nMode, u32 * pnValue);

/****************************/
/* AES/EBU Receiver control */
/****************************/
	u16 HPI_AESEBU_Receiver_SetSource(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wSource	/* HPI_AESEBU_SOURCE_AESEBU, HPI_AESEBU_SOURCE_SPDIF */
	    );

	u16 HPI_AESEBU_Receiver_GetSource(	/* TFE apr-1-04 */
						 HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 * pwSource	/* HPI_AESEBU_SOURCE_AESEBU, HPI_AESEBU_SOURCE_SPDIF */
	    );

	u16 HPI_AESEBU_Receiver_GetSampleRate(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u32 * pdwSampleRate	/* 0,32000,44100 or 48000 returned */
	    );

	u16 HPI_AESEBU_Receiver_GetUserData(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wIndex,	/* ranges from 0..23 */
					    u16 * pwData	/* returned user data */
	    );

	u16 HPI_AESEBU_Receiver_GetChannelStatus(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wIndex,	/* ranges from 0..23 */
						 u16 * pwData	/* returned channel status data */
	    );

	u16 HPI_AESEBU_Receiver_GetErrorStatus(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 * pwErrorData	/* returned error data */
	    );

/*******************************/
/* AES/EBU Transmitter control */
/*******************************/
	u16 HPI_AESEBU_Transmitter_SetSampleRate(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u32 dwSampleRate	/* 32000,44100 or 48000 */
	    );

	u16 HPI_AESEBU_Transmitter_SetUserData(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wIndex,	/* ranges from 0..23 */
					       u16 wData	/* user data to set */
	    );

	u16 HPI_AESEBU_Transmitter_SetChannelStatus(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wIndex,	/* ranges from 0..23 */
						    u16 wData	/* channel status data to write */
	    );

	u16 HPI_AESEBU_Transmitter_GetChannelStatus(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wIndex,	// ranges from 0..23
						    u16 * pwData	// channel status data to write
	    );

#ifdef HPI_SUPPORT_AESEBUTXSETCLKSRC
	u16 HPI_AESEBU_Transmitter_SetClockSource(	/* SGT nov-4-98 */
							 HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wClockSource	/* SYNC, ADAPTER */
	    );

	u16 HPI_AESEBU_Transmitter_GetClockSource(	/* TFE apr-1-04 */
							 HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 * pwClockSource	/* SYNC, ADAPTER */
	    );
#endif

	u16 HPI_AESEBU_Transmitter_SetFormat(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wOutputFormat	/* HPI_AESEBU_SOURCE_AESEBU, _SPDIF */
	    );

	u16 HPI_AESEBU_Transmitter_GetFormat(	/* TFE apr-1-04 */
						    HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 * pwOutputFormat	/* HPI_AESEBU_SOURCE_AESEBU, _SPDIF */
	    );

/* multiplexer control */
	u16 HPI_Multiplexer_SetSource(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wSourceNodeType,	/* source node */
				      u16 wSourceNodeIndex	/* source index */
	    );
	u16 HPI_Multiplexer_GetSource(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 * wSourceNodeType,	/* returned source node */
				      u16 * wSourceNodeIndex	/* returned source index */
	    );

	u16 HPI_Multiplexer_QuerySource(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 nIndex,	/* index number to query (0..N) */
					u16 * wSourceNodeType,	/* returned source node */
					u16 * wSourceNodeIndex	/* returned source index */
	    );

/*************************/
/* on/off switch control */
/*************************/
	u16 HPI_OnOffSwitch_SetState(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wState	/* 1=on, 0=off */
	    );

	u16 HPI_OnOffSwitch_GetState(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 * wState	/* 1=on, 0=off */
	    );

/***************/
/* VOX control */
/***************/
	u16 HPI_VoxSetThreshold(HPI_HSUBSYS * phSubSys,
				HPI_HCONTROL hControlHandle,
				short anGain0_01dB);

	u16 HPI_VoxGetThreshold(HPI_HSUBSYS * phSubSys,
				HPI_HCONTROL hControlHandle,
				short *anGain0_01dB);

/*********************/
/* Bitstream control */
/*********************/
	u16 HPI_Bitstream_SetClockEdge(HPI_HSUBSYS * phSubSys,
				       HPI_HCONTROL hControlHandle,
				       u16 wEdgeType);

	u16 HPI_Bitstream_SetDataPolarity(HPI_HSUBSYS * phSubSys,
					  HPI_HCONTROL hControlHandle,
					  u16 wPolarity);

	u16 HPI_Bitstream_GetActivity(HPI_HSUBSYS * phSubSys,
				      HPI_HCONTROL hControlHandle,
				      u16 * pwClkActivity,
				      u16 * pwDataActivity);

/***********************/
/* SampleClock control */
/***********************/
	u16 HPI_SampleClock_SetSource(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wSource	/* HPI_SAMPLECLOCK_SOURCE_ADAPTER, _AESEBU etc */
	    );

	u16 HPI_SampleClock_GetSource(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 * pwSource	/* HPI_SAMPLECLOCK_SOURCE_ADAPTER, _AESEBU etc */
	    );

	u16 HPI_SampleClock_SetSourceIndex(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wSourceIndex	// index of the source to use
	    );

	u16 HPI_SampleClock_GetSourceIndex(HPI_HSUBSYS * phSubSys,
					   HPI_HCONTROL hControlHandle,
					   u16 * pwSourceIndex);

	u16 HPI_SampleClock_SetSampleRate(HPI_HSUBSYS * phSubSys,
					  HPI_HCONTROL hControlHandle,
					  u32 dwSampleRate);

	u16 HPI_SampleClock_GetSampleRate(HPI_HSUBSYS * phSubSys,
					  HPI_HCONTROL hControlHandle,
					  u32 * pdwSampleRate);

/***********************/
/* Microphone control */
/***********************/
	u16 HPI_Microphone_SetPhantomPower(HPI_HSUBSYS * phSubSys,
					   HPI_HCONTROL hControlHandle,
					   u16 wOnOff);

	u16 HPI_Microphone_GetPhantomPower(HPI_HSUBSYS * phSubSys,
					   HPI_HCONTROL hControlHandle,
					   u16 * pwOnOff);

/*******************************
Parametric Equalizer control
*******************************/
	u16 HPI_ParametricEQ_GetInfo(HPI_HSUBSYS * phSubSys,
				     HPI_HCONTROL hControlHandle,
				     u16 * pwNumberOfBands, u16 * pwEnabled);

	u16 HPI_ParametricEQ_SetState(HPI_HSUBSYS * phSubSys,
				      HPI_HCONTROL hControlHandle, u16 wOnOff);

	u16 HPI_ParametricEQ_SetBand(HPI_HSUBSYS * phSubSys,
				     HPI_HCONTROL hControlHandle,
				     u16 wIndex,
				     u16 nType,
				     u32 dwFrequencyHz,
				     short nQ100, short nGain0_01dB);

	u16 HPI_ParametricEQ_GetBand(HPI_HSUBSYS * phSubSys,
				     HPI_HCONTROL hControlHandle,
				     u16 wIndex,
				     u16 * pnType,
				     u32 * pdwFrequencyHz,
				     short *pnQ100, short *pnGain0_01dB);

	u16 HPI_ParametricEQ_GetCoeffs(HPI_HSUBSYS * phSubSys,
				       HPI_HCONTROL hControlHandle,
				       u16 wIndex, short coeffs[5]
	    );

/*******************************
Compressor Expander control
*******************************/

	u16 HPI_Compander_Set(HPI_HSUBSYS * phSubSys,
			      HPI_HCONTROL hControlHandle,
			      u16 wAttack,
			      u16 wDecay,
			      short wRatio100,
			      short nThreshold0_01dB, short nMakeupGain0_01dB);

/*! Get the settings of a compressor expander
*/
	u16 HPI_Compander_Get(HPI_HSUBSYS * phSubSys,
			      HPI_HCONTROL hControlHandle,
			      u16 * pwAttack,
			      u16 * pwDecay,
			      short *pwRatio100,
			      short *pnThreshold0_01dB,
			      short *pnMakeupGain0_01dB);

/*******************************
Cobranet HMI control
*******************************/

/*! Write data to a cobranet HMI variable
*/
	u16 HPI_Cobranet_HmiWrite(HPI_HSUBSYS * phSubSys,
				  HPI_HCONTROL hControlHandle,
				  u32 dwHmiAddress,
				  u32 dwByteCount, u8 * pbData);

/*! Read data from acobranet HMI variable
*/
	u16 HPI_Cobranet_HmiRead(HPI_HSUBSYS * phSubSys,
				 HPI_HCONTROL hControlHandle,
				 u32 dwHmiAddress,
				 u32 dwMaxByteCount,
				 u32 * pdwByteCount, u8 * pbData);

/*! Read the raw cobranet HMI status
*/
	u16 HPI_Cobranet_HmiGetStatus(HPI_HSUBSYS * phSubSys,
				      HPI_HCONTROL hControlHandle,
				      u32 * pdwStatus,
				      u32 * pdwReadableSize,
				      u32 * pdwWriteableSize);

/*! Set the CobraNet mode. Used for switching tethered mode on and off.
*/
	u16 HPI_Cobranet_SetMode(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u32 dwMode, u32 dwSetOrQuery	/* either, HPI_COBRANET_MODE_QUERY or HPI_COBRANET_MODE_SET */
	    );

/*! Get the CobraNet mode.
*/
	u16 HPI_Cobranet_GetMode(HPI_HSUBSYS * phSubSys,
				 HPI_HCONTROL hControlHandle, u32 * pdwMode);

/*! Read the IP address
*/
	u16 HPI_Cobranet_GetIPaddress(HPI_HSUBSYS * phSubSys,
				      HPI_HCONTROL hControlHandle,
				      u32 * pdwIPaddress);
/*! Read the MAC address
*/
	u16 HPI_Cobranet_GetMACaddress(HPI_HSUBSYS * phSubSys,
				       HPI_HCONTROL hControlHandle,
				       u32 * pdwMAC_MSBs, u32 * pdwMAC_LSBs);
/*******************************
Tone Detector control
*******************************/

	 fnHPI_Control_Get(ToneDetector, State, HPI_TONEDETECTOR_STATE, u32)
	 fnHPI_Control_SetGet(ToneDetector, Enable, HPI_GENERIC_ENABLE, u32)
	 fnHPI_Control_SetGet(ToneDetector, EventEnable,
			      HPI_GENERIC_EVENT_ENABLE, u32)
	 fnHPI_Control_SetGet(ToneDetector, Threshold,
			      HPI_TONEDETECTOR_THRESHOLD, int)

	u16 HPI_ToneDetector_GetFrequency(HPI_HSUBSYS * phSubSys,
					  HPI_HCONTROL hControl,
					  u32 nIndex, u32 * dwFrequency);

/*******************************
Silence Detector control
*******************************/
	 fnHPI_Control_Get(SilenceDetector, State, HPI_SILENCEDETECTOR_STATE,
			   u32)

	 fnHPI_Control_SetGet(SilenceDetector, Enable, HPI_GENERIC_ENABLE, u32)
	 fnHPI_Control_SetGet(SilenceDetector, EventEnable,
			      HPI_GENERIC_EVENT_ENABLE, u32)
	 fnHPI_Control_SetGet(SilenceDetector, Delay, HPI_SILENCEDETECTOR_DELAY,
			      u32)
	 fnHPI_Control_SetGet(SilenceDetector, Threshold,
			      HPI_SILENCEDETECTOR_THRESHOLD, int)

/*/////////// */
/* DSP CLOCK  */
/*/////////// */
	u16 HPI_ClockOpen(HPI_HSUBSYS * phSubSys,
			  u16 wAdapterIndex, HPI_HCLOCK * phDspClock);

	u16 HPI_ClockSetTime(HPI_HSUBSYS * phSubSys,
			     HPI_HCLOCK hClock,
			     u16 wHour,
			     u16 wMinute, u16 wSecond, u16 wMilliSecond);

	u16 HPI_ClockGetTime(HPI_HSUBSYS * phSubSys,
			     HPI_HCLOCK hClock,
			     u16 * pwHour,
			     u16 * pwMinute,
			     u16 * pwSecond, u16 * pwMilliSecond);

/*/////////// */
/* PROFILE    */
/*/////////// */
	u16 HPI_ProfileOpenAll(HPI_HSUBSYS * phSubSys, u16 wAdapterIndex, u16 wProfileIndex,	/*!< corresponds to DSP index */
			       HPI_HPROFILE * phProfile, u16 * pwMaxProfiles);

	u16 HPI_ProfileGet(HPI_HSUBSYS * phSubSys,
			   HPI_HPROFILE hProfile,
			   u16 wIndex,
			   u16 * pwSeconds,
			   u32 * pdwMicroSeconds,
			   u32 * pdwCallCount,
			   u32 * pdwMaxMicroSeconds, u32 * pdwMinMicroSeconds);

	DEPRECATED u16 HPI_ProfileGetIdleCount(HPI_HSUBSYS * phSubSys,
					       HPI_HPROFILE hProfile,
					       u16 wIndex,
					       u32 * pdwCycleCount,
					       u32 * pdwCount);

	u16 HPI_ProfileStartAll(HPI_HSUBSYS * phSubSys, HPI_HPROFILE hProfile);

	u16 HPI_ProfileStopAll(HPI_HSUBSYS * phSubSys, HPI_HPROFILE hProfile);

	u16 HPI_ProfileGetName(HPI_HSUBSYS * phSubSys,
			       HPI_HPROFILE hProfile,
			       u16 wIndex,
			       char *szProfileName, u16 nProfileNameLength);

	u16 HPI_ProfileGetUtilization(HPI_HSUBSYS * phSubSys,
				      HPI_HPROFILE hProfile,
				      u32 * pdwUtilization);

/*//////////////////// */
/* UTILITY functions */

	void HPI_GetErrorText(u16 wError, char *pszErrorText);

	u16 HPI_FormatCreate(HPI_FORMAT * pFormat,
			     u16 wChannels,
			     u16 wFormat,
			     u32 dwSampleRate, u32 dwBitRate, u32 dwAttributes);

#ifndef HPI_WITHOUT_HPI_DATA
	DEPRECATED u16 HPI_DataCreate(HPI_DATA * pData,
				      HPI_FORMAT * pFormat,
				      u8 * pbData, u32 dwDataSize);
#endif

/* Until it's verified, this function is for Windows OSs only */
#if defined ( HPI_OS_WIN16 ) || defined ( HPI_OS_WIN32_USER ) || defined ( INCLUDE_WINDOWS_ON_LINUX )

#include <asimmdef.h>

	u16 HPI_WaveFormatToHpiFormat(const PWAVEFORMATEX lpFormatEx,
				      HPI_FORMAT * pHpiFormat);

	u16 HPI_HpiFormatToWaveFormat(const HPI_FORMAT * pHpiFormat,
				      PWAVEFORMATEX lpFormatEx);

#endif				/* defined(HPI_OS_WIN16) || defined(HPI_OS_WIN32_USER) */

#endif				/* ndef HPI_ON_DSP  */
/******************************************************************************/
/******************************************************************************/
/********                     HPI LOW LEVEL MESSAGES                  *********/
/******************************************************************************/
/******************************************************************************/
#ifndef HPI_EXCLUDE_IMPLEMENTATION
/** Pnp ids */
#define HPI_ID_ISAPNP_AUDIOSCIENCE      0x0669	/*"ASI"  - actual is "ASX" - need to change */
#define HPI_PCI_VENDOR_ID_AUDIOSCIENCE  0x175C	 /**< PCI vendor ID that AudioScience uses */
#define HPI_PCI_VENDOR_ID_MOTOROLA      0x1057	 /**< PCI vendor ID that the DSP56301 has */
#define HPI_PCI_VENDOR_ID_TI            0x104C	 /**< PCI vendor ID that TI uses */

#define HPI_USB_VENDOR_ID_AUDIOSCIENCE  0x1257
#define HPI_USB_W2K_TAG         0x57495341	/* "ASIW"       */
#define HPI_USB_LINUX_TAG       0x4C495341	/* "ASIL"       */

/******************************************* message types */
#define HPI_TYPE_MESSAGE                1
#define HPI_TYPE_RESPONSE               2
#define HPI_TYPE_DATA                   3

/******************************************* object types */
#define HPI_OBJ_SUBSYSTEM               1
#define HPI_OBJ_ADAPTER                 2
#define HPI_OBJ_OSTREAM                 3
#define HPI_OBJ_ISTREAM                 4
#define HPI_OBJ_MIXER                   5
#define HPI_OBJ_NODE                    6
#define HPI_OBJ_CONTROL                 7
#define HPI_OBJ_NVMEMORY                8
#define HPI_OBJ_GPIO                    9
#define HPI_OBJ_WATCHDOG                10
#define HPI_OBJ_CLOCK                   11
#define HPI_OBJ_PROFILE                 12
#define HPI_OBJ_CONTROLEX               13
#define HPI_OBJ_ASYNCEVENT              14

#define HPI_OBJ_MAXINDEX                14

/******************************************* methods/functions */

#define HPI_OBJ_FUNCTION_SPACING (0x100)
#define HPI_MAKE_INDEX(obj,index) (obj*HPI_OBJ_FUNCTION_SPACING+index)
#define HPI_EXTRACT_INDEX(fn) (fn & 0xff)

/* SUB-SYSTEM */
#define HPI_SUBSYS_OPEN                 HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM,1)
#define HPI_SUBSYS_GET_VERSION          HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM,2)
#define HPI_SUBSYS_GET_INFO             HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM,3)
#define HPI_SUBSYS_FIND_ADAPTERS        HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM,4)	/* SGT feb-3-97 */
#define HPI_SUBSYS_CREATE_ADAPTER       HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM,5)	/* SGT feb-3-97 - not used any more */
#define HPI_SUBSYS_CLOSE                HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM,6)
#define HPI_SUBSYS_DELETE_ADAPTER       HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM,7)
#define HPI_SUBSYS_DRIVER_LOAD          HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM,8)
#define HPI_SUBSYS_DRIVER_UNLOAD        HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM,9)
	 /*SGT*/
#define HPI_SUBSYS_READ_PORT_8          HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM,10)
#define HPI_SUBSYS_WRITE_PORT_8         HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM,11)
	 /*SGT*/
/* ADAPTER */
#define HPI_ADAPTER_OPEN                HPI_MAKE_INDEX(HPI_OBJ_ADAPTER,1)
#define HPI_ADAPTER_CLOSE               HPI_MAKE_INDEX(HPI_OBJ_ADAPTER,2)
#define HPI_ADAPTER_GET_INFO            HPI_MAKE_INDEX(HPI_OBJ_ADAPTER,3)
#define HPI_ADAPTER_GET_ASSERT          HPI_MAKE_INDEX(HPI_OBJ_ADAPTER,4)	/* AGE SEP-12-97 */
#define HPI_ADAPTER_TEST_ASSERT         HPI_MAKE_INDEX(HPI_OBJ_ADAPTER,5)	/* AGE SEP-12-97 */
#define HPI_ADAPTER_SET_MODE            HPI_MAKE_INDEX(HPI_OBJ_ADAPTER,6)
#define HPI_ADAPTER_GET_MODE            HPI_MAKE_INDEX(HPI_OBJ_ADAPTER,7)
#define HPI_ADAPTER_ENABLE_CAPABILITY   HPI_MAKE_INDEX(HPI_OBJ_ADAPTER,8)
#define HPI_ADAPTER_SELFTEST            HPI_MAKE_INDEX(HPI_OBJ_ADAPTER,9)
#define HPI_ADAPTER_FIND_OBJECT         HPI_MAKE_INDEX(HPI_OBJ_ADAPTER,10)
#define HPI_ADAPTER_QUERY_FLASH       HPI_MAKE_INDEX(HPI_OBJ_ADAPTER,11)
#define HPI_ADAPTER_START_FLASH       HPI_MAKE_INDEX(HPI_OBJ_ADAPTER,12)
#define HPI_ADAPTER_PROGRAM_FLASH       HPI_MAKE_INDEX(HPI_OBJ_ADAPTER,13)
#define HPI_ADAPTER_SET_PROPERTY        HPI_MAKE_INDEX(HPI_OBJ_ADAPTER,14)
#define HPI_ADAPTER_GET_PROPERTY        HPI_MAKE_INDEX(HPI_OBJ_ADAPTER,15)
#define HPI_ADAPTER_ENUM_PROPERTY       HPI_MAKE_INDEX(HPI_OBJ_ADAPTER,16)
/* OUTPUT STREAM */
#define HPI_OSTREAM_OPEN                HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,1)
#define HPI_OSTREAM_CLOSE               HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,2)
#define HPI_OSTREAM_WRITE               HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,3)
#define HPI_OSTREAM_START               HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,4)
#define HPI_OSTREAM_STOP                HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,5)
#define HPI_OSTREAM_RESET               HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,6)
#define HPI_OSTREAM_GET_INFO            HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,7)
#define HPI_OSTREAM_QUERY_FORMAT        HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,8)
#define HPI_OSTREAM_DATA                HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,9)
#define HPI_OSTREAM_SET_VELOCITY        HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,10)
#define HPI_OSTREAM_SET_PUNCHINOUT      HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,11)
#define HPI_OSTREAM_SINEGEN             HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,12)
#define HPI_OSTREAM_ANC_RESET           HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,13)	/* MP2 ancillary data reset */
#define HPI_OSTREAM_ANC_GET_INFO        HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,14)	/* MP2 ancillary data get info */
#define HPI_OSTREAM_ANC_READ            HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,15)	/* MP2 ancillary data read */
#define HPI_OSTREAM_SET_TIMESCALE       HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,16)
#define HPI_OSTREAM_SET_FORMAT          HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,17)
#define HPI_OSTREAM_HOSTBUFFER_ALLOC    HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,18)
#define HPI_OSTREAM_HOSTBUFFER_FREE     HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,19)
#define HPI_OSTREAM_GROUP_ADD                   HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,20)
#define HPI_OSTREAM_GROUP_GETMAP                HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,21)
#define HPI_OSTREAM_GROUP_RESET                 HPI_MAKE_INDEX(HPI_OBJ_OSTREAM,22)
#define HPI_OSTREAM_FUNCTION_COUNT              (22)
/* INPUT STREAM */
#define HPI_ISTREAM_OPEN                HPI_MAKE_INDEX(HPI_OBJ_ISTREAM,1)
#define HPI_ISTREAM_CLOSE               HPI_MAKE_INDEX(HPI_OBJ_ISTREAM,2)
#define HPI_ISTREAM_SET_FORMAT          HPI_MAKE_INDEX(HPI_OBJ_ISTREAM,3)	/* SGT mar-19-97 */
#define HPI_ISTREAM_READ                HPI_MAKE_INDEX(HPI_OBJ_ISTREAM,4)	/* SGT mar-19-97 */
#define HPI_ISTREAM_START               HPI_MAKE_INDEX(HPI_OBJ_ISTREAM,5)	/* SGT mar-19-97 */
#define HPI_ISTREAM_STOP                HPI_MAKE_INDEX(HPI_OBJ_ISTREAM,6)
#define HPI_ISTREAM_RESET               HPI_MAKE_INDEX(HPI_OBJ_ISTREAM,7)	/* SGT mar-19-97 */
#define HPI_ISTREAM_GET_INFO            HPI_MAKE_INDEX(HPI_OBJ_ISTREAM,8)
#define HPI_ISTREAM_QUERY_FORMAT        HPI_MAKE_INDEX(HPI_OBJ_ISTREAM,9)
#define HPI_ISTREAM_ANC_RESET           HPI_MAKE_INDEX(HPI_OBJ_ISTREAM,10)	/* MP2 ancillary data reset */
#define HPI_ISTREAM_ANC_GET_INFO        HPI_MAKE_INDEX(HPI_OBJ_ISTREAM,11)
#define HPI_ISTREAM_ANC_WRITE           HPI_MAKE_INDEX(HPI_OBJ_ISTREAM,12)
#define HPI_ISTREAM_HOSTBUFFER_ALLOC    HPI_MAKE_INDEX(HPI_OBJ_ISTREAM,13)
#define HPI_ISTREAM_HOSTBUFFER_FREE     HPI_MAKE_INDEX(HPI_OBJ_ISTREAM,14)
#define HPI_ISTREAM_GROUP_ADD                   HPI_MAKE_INDEX(HPI_OBJ_ISTREAM,15)
#define HPI_ISTREAM_GROUP_GETMAP                HPI_MAKE_INDEX(HPI_OBJ_ISTREAM,16)
#define HPI_ISTREAM_GROUP_RESET                 HPI_MAKE_INDEX(HPI_OBJ_ISTREAM,17)
#define HPI_ISTREAM_FUNCTION_COUNT              (17)
/* MIXER */
/* NOTE: (EWB 2003-05-13)
GET_INFO, GET_NODE_INFO, SET_CONNECTION, GET_CONNECTIONS are not currently used */
#define HPI_MIXER_OPEN                  HPI_MAKE_INDEX(HPI_OBJ_MIXER,1)
#define HPI_MIXER_CLOSE                 HPI_MAKE_INDEX(HPI_OBJ_MIXER,2)
#define HPI_MIXER_GET_INFO              HPI_MAKE_INDEX(HPI_OBJ_MIXER,3)	/* gets list of source and dest node objects */
#define HPI_MIXER_GET_NODE_INFO         HPI_MAKE_INDEX(HPI_OBJ_MIXER,4)	/* gets info on a particular node */
#define HPI_MIXER_GET_CONTROL           HPI_MAKE_INDEX(HPI_OBJ_MIXER,5)	/* gets specified control type on given connection */
#define HPI_MIXER_SET_CONNECTION        HPI_MAKE_INDEX(HPI_OBJ_MIXER,6)	/* between a destination and source */
#define HPI_MIXER_GET_CONNECTIONS       HPI_MAKE_INDEX(HPI_OBJ_MIXER,7)	/* for a given destination */
#define HPI_MIXER_GET_CONTROL_BY_INDEX  HPI_MAKE_INDEX(HPI_OBJ_MIXER,8)	/* get a control index */
#define HPI_MIXER_GET_CONTROL_ARRAY_BY_INDEX  HPI_MAKE_INDEX(HPI_OBJ_MIXER,9)	/* get a control array index (internal call for the moment) */
#define HPI_MIXER_GET_CONTROL_MULTIPLE_VALUES HPI_MAKE_INDEX(HPI_OBJ_MIXER,10)	/* get an array of control values (internal call for the moment) */
#define HPI_MIXER_STORE                 HPI_MAKE_INDEX(HPI_OBJ_MIXER,11)	/* Access the mixer control store */
#define HPI_MIXER_FUNCTION_COUNT        11
/* MIXER CONTROLS */
#define HPI_CONTROL_GET_INFO            HPI_MAKE_INDEX(HPI_OBJ_CONTROL,1)	/* used by HPI_ControlQuery() */
#define HPI_CONTROL_GET_STATE           HPI_MAKE_INDEX(HPI_OBJ_CONTROL,2)
#define HPI_CONTROL_SET_STATE           HPI_MAKE_INDEX(HPI_OBJ_CONTROL,3)
/* NONVOL MEMORY */
#define HPI_NVMEMORY_OPEN               HPI_MAKE_INDEX(HPI_OBJ_NVMEMORY,1)
#define HPI_NVMEMORY_READ_BYTE          HPI_MAKE_INDEX(HPI_OBJ_NVMEMORY,2)
#define HPI_NVMEMORY_WRITE_BYTE         HPI_MAKE_INDEX(HPI_OBJ_NVMEMORY,3)
/* GPIO */
#define HPI_GPIO_OPEN                   HPI_MAKE_INDEX(HPI_OBJ_GPIO,1)
#define HPI_GPIO_READ_BIT               HPI_MAKE_INDEX(HPI_OBJ_GPIO,2)
#define HPI_GPIO_WRITE_BIT              HPI_MAKE_INDEX(HPI_OBJ_GPIO,3)
#define HPI_GPIO_READ_ALL               HPI_MAKE_INDEX(HPI_OBJ_GPIO,4)
/* ASYNC EVENT */
#define HPI_ASYNCEVENT_OPEN             HPI_MAKE_INDEX(HPI_OBJ_ASYNCEVENT,1)
#define HPI_ASYNCEVENT_CLOSE            HPI_MAKE_INDEX(HPI_OBJ_ASYNCEVENT,2)
#define HPI_ASYNCEVENT_WAIT             HPI_MAKE_INDEX(HPI_OBJ_ASYNCEVENT,3)
#define HPI_ASYNCEVENT_GETCOUNT         HPI_MAKE_INDEX(HPI_OBJ_ASYNCEVENT,4)
#define HPI_ASYNCEVENT_GET              HPI_MAKE_INDEX(HPI_OBJ_ASYNCEVENT,5)
#define HPI_ASYNCEVENT_SENDEVENTS       HPI_MAKE_INDEX(HPI_OBJ_ASYNCEVENT,6)
/* WATCH-DOG */
#define HPI_WATCHDOG_OPEN               HPI_MAKE_INDEX(HPI_OBJ_WATCHDOG,1)
#define HPI_WATCHDOG_SET_TIME           HPI_MAKE_INDEX(HPI_OBJ_WATCHDOG,2)
#define HPI_WATCHDOG_PING               HPI_MAKE_INDEX(HPI_OBJ_WATCHDOG,3)
/* CLOCK */
#define HPI_CLOCK_OPEN                  HPI_MAKE_INDEX(HPI_OBJ_CLOCK,1)
#define HPI_CLOCK_SET_TIME              HPI_MAKE_INDEX(HPI_OBJ_CLOCK,2)
#define HPI_CLOCK_GET_TIME              HPI_MAKE_INDEX(HPI_OBJ_CLOCK,3)
/* PROFILE */
#define HPI_PROFILE_OPEN_ALL            HPI_MAKE_INDEX(HPI_OBJ_PROFILE,1)
#define HPI_PROFILE_START_ALL           HPI_MAKE_INDEX(HPI_OBJ_PROFILE,2)
#define HPI_PROFILE_STOP_ALL            HPI_MAKE_INDEX(HPI_OBJ_PROFILE,3)
#define HPI_PROFILE_GET                 HPI_MAKE_INDEX(HPI_OBJ_PROFILE,4)
#define HPI_PROFILE_GET_IDLECOUNT       HPI_MAKE_INDEX(HPI_OBJ_PROFILE,5)
#define HPI_PROFILE_GET_NAME            HPI_MAKE_INDEX(HPI_OBJ_PROFILE,6)
#define HPI_PROFILE_GET_UTILIZATION     HPI_MAKE_INDEX(HPI_OBJ_PROFILE,7)
/* ////////////////////////////////////////////////////////////////////// */
/* STRUCTURES */
#ifndef DISABLE_PRAGMA_PACK1
#pragma pack(push,1)
#endif
/** PCI bus resource */
	    typedef struct sHPI_PCI {
		u32 __iomem *apMemBase[HPI_MAX_ADAPTER_MEM_SPACES];
		struct pci_dev *pOsData;
#ifndef HPI_64BIT		/* keep structure size constant */
		u32 dwPadding[HPI_MAX_ADAPTER_MEM_SPACES + 1];
#endif
		u16 wVendorId;
		u16 wDeviceId;
		u16 wSubSysVendorId;
		u16 wSubSysDeviceId;
		u16 wBusNumber;
		u16 wDeviceNumber;
		u32 wInterrupt;
	} HPI_PCI;

	typedef struct {
		union {
			HPI_PCI *Pci;
#ifndef HPI_64BIT		/* keep structure size constant */
			u32 dwPadTo64;
#endif
		} r;
		u16 wBusType;	/* HPI_BUS_PNPISA, _PCI, _USB etc */
		u16 wPadding;

	} HPI_RESOURCE;

/** Format info used inside HPI_MESSAGE. Not the same as public API HPI_FORMAT */
	typedef struct sHPI_MSG_FORMAT {
		u32 dwSampleRate;
		       /**< 11025, 32000, 44100 ... */
		u32 dwBitRate;
		       /**< for MPEG */
		u32 dwAttributes;
		       /**< Stereo/JointStereo/Mono */
		u16 wChannels;
		       /**< 1,2..., (or ancillary mode or idle bit */
		u16 wFormat;
		       /**< HPI_FORMAT_PCM16, _AC2, _MPEG ... */
	} HPI_MSG_FORMAT;

/**  Buffer+format structure.
Must be kept 7 * 32 bits to match public HPI_DATA struct */
	typedef struct sHPI_MSG_DATA {
		HPI_MSG_FORMAT Format;
		u8 *pbData;
#ifndef HPI_64BIT
		u32 dwPadding;
#endif
		u32 dwDataSize;
	} HPI_MSG_DATA;

#ifndef HPI_64BIT
/** HPI_DATA structure used up to 3.04 driver */
	typedef struct {
		HPI_FORMAT Format;
		u8 *pbData;
		u32 dwDataSize;
	} HPI_DATA_LEGACY32;
#endif

#ifdef HPI_64BIT
/* Compatibility version of HPI_DATA */
	typedef struct {
		HPI_MSG_FORMAT Format;
		u32 pbData;
		u32 dwPadding;
		u32 dwDataSize;
	} HPI_DATA_COMPAT32;
#endif

	typedef struct {
		HPI_MSG_FORMAT reserved;
				/**< placehoder for backward compatability (see dwBufferSize) */
		u32 dwCommand; /**< HPI_BUFFER_CMD_xxx*/
		u32 dwPciAddress;
			       /**< PCI physical address of buffer for DSP DMA */
		u32 dwBufferSize;
			       /**< must line up with dwDataSize of HPI_DATA*/
	} HPI_BUFFER;

	typedef struct {
		HPI_RESOURCE Resource;
	} HPI_SUBSYS_MSG;

	typedef struct {
		u32 dwVersion;
		u32 dwData;	/* used to return extended version */
		u16 wNumAdapters;	/* number of adapters */
		u16 wAdapterIndex;
		u16 awAdapterList[HPI_MAX_ADAPTERS];	/* array of adapters */
	} HPI_SUBSYS_RES;

	typedef struct {
		u32 dwAdapterMode;	/* adapter mode */
		u16 wAssertId;	/* assert number for "test assert" call
				   also wObjectIndex for find object call
				   also wQueryOrSet for HPI_AdapterSetModeEx() */
		u16 wObjectType;	/* for adapter find object call */
	} HPI_ADAPTER_MSG;

	typedef union {
		HPI_ADAPTER_MSG adapter;
		struct {
			u32 dwOffset;
		} query_flash;
		struct {
			u32 dwOffset;
			u32 dwLength;
			u32 dwKey;
		} start_flash;
		struct {
			u32 dwChecksum;
			u16 wSequence;
			u16 wLength;
		} program_flash;
		struct {
			u16 wProperty;
			u16 wParameter1;
			u16 wParameter2;
		} property_set;
		struct {
			u16 wIndex;
			u16 wWhat;
			u16 wPropertyIndex;
		} property_enum;
	} HPI_ADAPTERX_MSG;

	typedef struct {
		u32 dwSerialNumber;
		u16 wAdapterType;
		u16 wAdapterIndex;	/* Is this needed? also used for wDspIndex */
		u16 wNumIStreams;
		u16 wNumOStreams;
		u16 wNumMixers;
		u16 wVersion;
		u8 szAdapterAssert[STR_SIZE(HPI_STRING_LEN)];
	} HPI_ADAPTER_RES;

	typedef union {
		HPI_ADAPTER_RES adapter;
		struct {
			u32 dwChecksum;
			u32 dwLength;
			u32 dwVersion;
		} query_flash;
		struct {
			u16 wSequence;
		} program_flash;
		struct {
			u16 wParameter1;
			u16 wParameter2;
		} property_get;
	} HPI_ADAPTERX_RES;

	typedef struct {
		union {
			HPI_MSG_DATA Data;
			HPI_DATA_LEGACY32 Data32;
			u16 wVelocity;
			HPI_PUNCHINOUT Pio;
			u32 dwTimeScale;
			HPI_BUFFER Buffer;
			HPI_STREAMID Stream;
		} u;
		u16 wStreamIndex;
		u16 wIStreamIndex;
	} HPI_STREAM_MSG;

	typedef struct {
		union {
			struct {
				u32 dwBufferSize;	/* size of hardware buffer */
				u32 dwDataAvailable;	/* OutStream - data to play, InStream - data recorded */
				u32 dwSamplesTransferred;	/* OutStream - samples played, InStream - samples recorded */
				u32 dwAuxiliaryDataAvailable;	/* Adapter - OutStream - data to play, InStream - data recorded */
				u16 wState;	/* HPI_STATE_PLAYING, _STATE_STOPPED */
				u16 wPadding;
			} stream_info;
			struct {
				u32 dwOutStreamGroupMap;	/* bitmap of grouped OutStreams */
				u32 dwInStreamGroupMap;	/* bitmap of grouped InStreams */
			} group_info;
		} u;
	} HPI_STREAM_RES;

	typedef struct {
		u16 wControlIndex;
		u16 wControlType;	/* = HPI_CONTROL_METER _VOLUME etc */
		u16 wPadding1;	/* Maintain alignment of subsequent fields */
		u16 wNodeType1;	/* = HPI_SOURCENODE_LINEIN etc */
		u16 wNodeIndex1;	/* = 0..N */
		u16 wNodeType2;
		u16 wNodeIndex2;
		u16 wPadding2;	/* round to 4 bytes */
	} HPI_MIXER_MSG;

	typedef struct {
		u16 wSrcNodeType;	/* = HPI_SOURCENODE_LINEIN etc */
		u16 wSrcNodeIndex;	/* = 0..N */
		u16 wDstNodeType;
		u16 wDstNodeIndex;
		u16 wControlIndex;	/* Also returns controlType for HPI_MixerGetControlByIndex */
		u16 wDspIndex;	/* may indicate which DSP the control is located on */
	} HPI_MIXER_RES;

	typedef union HPI_MIXERX_MSG {
		struct {
			u16 wStartingIndex;
			u16 wFlags;
			u32 dwLengthInBytes;	/* length in bytes of pData */
			u32 pData;	/* pointer to a data array */
		} gcabi;
		struct {
			u16 wCommand;
			u16 wIndex;
		} store;	/* for HPI_MIXER_STORE message */
	} HPI_MIXERX_MSG;

	typedef union {
		struct {
			u32 dwBytesReturned;	/* number of items returned */
			u32 pData;	/* pointer to data array */
			u16 wMoreToDo;	/* indicates if there is more to do */
		} gcabi;
	} HPI_MIXERX_RES;

	typedef struct {
		u32 dwParam1;	/* generic parameter 1 */
		u32 dwParam2;	/* generic parameter 2 */
		short anLogValue[HPI_MAX_CHANNELS];
		u16 wAttribute;	/* control attribute or property */
		u16 wControlIndex;
	} HPI_CONTROL_MSG;

	typedef struct {
/* Could make union.  dwParam, anLogValue never used in same response */
		u32 dwParam1;
		u32 dwParam2;
		short anLogValue[HPI_MAX_CHANNELS];
		/*          short   anLogValue2[HPI_MAX_CHANNELS]; *//* to return RMS and Peak in same call */
	} HPI_CONTROL_RES;

/* HPI_CONTROLX_STRUCTURES */

/* Message */

/** Used for all HMI variables where max length <= 8 bytes
*/
	typedef struct {
		u32 dwHmiAddress;
		u32 dwByteCount;
		u32 dwData[2];
	} HPI_CONTROLX_MSG_COBRANET_DATA;

/** Used for string data, and for packet bridge
*/
	typedef struct {
		u32 dwHmiAddress;
		u32 dwByteCount;
		u8 *pbData;
#ifndef HPI_64BIT
		u32 dwPadding;
#endif
	} HPI_CONTROLX_MSG_COBRANET_BIGDATA;

/** Used for generic data
*/
	typedef struct {
		u32 dwParam1;
		u32 dwParam2;
	} HPI_CONTROLX_MSG_GENERIC;

	typedef struct {
		union {
			HPI_CONTROLX_MSG_COBRANET_DATA cobranet_data;
			HPI_CONTROLX_MSG_COBRANET_BIGDATA cobranet_bigdata;
			HPI_CONTROLX_MSG_GENERIC generic;
/* nothing extra to send for status read */
		} u;
		u16 wControlIndex;
		u16 wAttribute;	/* control attribute or property */
	} HPI_CONTROLX_MSG;

/* Response */

/**
*/
	typedef struct {
		u32 dwByteCount;
		u32 dwData[2];
	} HPI_CONTROLX_RES_COBRANET_DATA;

	typedef struct {
		u32 dwByteCount;
	} HPI_CONTROLX_RES_COBRANET_BIGDATA;

	typedef struct {
		u32 dwStatus;
		u32 dwReadableSize;
		u32 dwWriteableSize;
	} HPI_CONTROLX_RES_COBRANET_STATUS;

	typedef struct {
		u32 dwParam1;
		u32 dwParam2;
	} HPI_CONTROLX_RES_GENERIC;

	typedef struct {
		union {
			HPI_CONTROLX_RES_COBRANET_BIGDATA cobranet_bigdata;
			HPI_CONTROLX_RES_COBRANET_DATA cobranet_data;
			HPI_CONTROLX_RES_COBRANET_STATUS cobranet_status;
			HPI_CONTROLX_RES_GENERIC generic;
		} u;
	} HPI_CONTROLX_RES;

	typedef struct {
		u16 wIndex;
		u16 wData;
	} HPI_NVMEMORY_MSG;

	typedef struct {
		u16 wSizeInBytes;
		u16 wData;
	} HPI_NVMEMORY_RES;

	typedef struct {
		u16 wBitIndex;
		u16 wBitData;
	} HPI_GPIO_MSG;

	typedef struct {
		u16 wNumberInputBits;
		u16 wNumberOutputBits;
		u16 wBitData;
		u16 wPadding;
	} HPI_GPIO_RES;

	typedef struct {
		u32 dwEvents;
		u16 wMaximumEvents;
		u16 wPadding;
	} HPI_ASYNC_MSG;

	typedef struct {
		union {
			struct {
				u16 wCount;
			} count;
			struct {
				u32 dwEvents;
				u16 wNumberReturned;
				u16 wPadding;
			} get;
			HPI_ASYNC_EVENT event;
		} u;
	} HPI_ASYNC_RES;

	typedef struct {
		u32 dwTimeMs;
	} HPI_WATCHDOG_MSG;

	typedef struct {
		u32 dwTimeMs;
	} HPI_WATCHDOG_RES;

	typedef struct {
		u16 wHours;
		u16 wMinutes;
		u16 wSeconds;
		u16 wMilliSeconds;
	} HPI_CLOCK_MSG;

	typedef struct {
		u16 wSizeInBytes;
		u16 wHours;
		u16 wMinutes;
		u16 wSeconds;
		u16 wMilliSeconds;
		u16 wPadding;
	} HPI_CLOCK_RES;

	typedef struct {
		u16 wIndex;
		u16 wPadding;
	} HPI_PROFILE_MSG;

	typedef struct {
		u16 wMaxProfiles;
	} HPI_PROFILE_RES_OPEN;

	typedef struct {
		u32 dwMicroSeconds;
		u32 dwCallCount;
		u32 dwMaxMicroSeconds;
		u32 dwMinMicroSeconds;
		u16 wSeconds;
	} HPI_PROFILE_RES_TIME;

	typedef struct {
		u16 szName[16];	/* u8 messes up response size for 56301 DSP */
	} HPI_PROFILE_RES_NAME;

	typedef struct {
		union {
			HPI_PROFILE_RES_OPEN o;
			HPI_PROFILE_RES_TIME t;
			HPI_PROFILE_RES_NAME n;
		} u;
	} HPI_PROFILE_RES;

/* the size of the part of the message outside the union.  MUST update if more elements are added */
	typedef struct {
		u16 wSize;
		u16 wType;	/* HPI_MSG_MESSAGE, HPI_MSG_RESPONSE */
		u16 wObject;	/* HPI_OBJ_SUBSYS, _OBJ_ADAPTER, _OBJ_DEVICE, _OBJ_MIXER */
		u16 wFunction;	/* HPI_SUBSYS_xxx, HPI_ADAPTER_xxx */
		u16 wAdapterIndex;	/* the adapter index */
		u16 wDspIndex;	/* the dsp index on the adapter */
	} HPI_MESSAGE_HEADER;

#define HPI_MESSAGE_FIXED_SIZE (sizeof(HPI_MESSAGE_HEADER))

	typedef struct sHPI_MESSAGE {
/* following fields must match HPI_MESSAGE_HEADER */
		u16 wSize;
		u16 wType;	/* HPI_TYPE_MESSAGE, HPI_TYPE_RESPONSE */
		u16 wObject;	/* HPI_OBJ_SUBSYS, _OBJ_ADAPTER, _OBJ_DEVICE, _OBJ_MIXER */
		u16 wFunction;	/* HPI_SUBSYS_xxx, HPI_ADAPTER_xxx */
		u16 wAdapterIndex;	/* the adapter index */
		u16 wDspIndex;	/* the dsp index on the adapter */
		union {
			HPI_SUBSYS_MSG s;	/* Subsys; */
			HPI_ADAPTER_MSG a;	/* Adapter; */
			HPI_ADAPTERX_MSG ax;	/* Adapter; */
			HPI_STREAM_MSG d;	/* Stream; */
			HPI_MIXER_MSG m;	/* Mixer; */
			HPI_MIXERX_MSG mx;	/* extended Mixer; */
			HPI_CONTROL_MSG c;	/* mixer Control; */
			HPI_CONTROLX_MSG cx;	/* extended mixer Control; */
			HPI_NVMEMORY_MSG n;	/* non-vol memory; */
			HPI_GPIO_MSG l;	/* digital i/o */
			HPI_WATCHDOG_MSG w;	/* watch-dog */
			HPI_CLOCK_MSG t;	/* dsp time */
			HPI_PROFILE_MSG p;	/* profile */
			HPI_ASYNC_MSG as;	/* async event */
#ifdef HPI_MESSAGE_FORCE_SIZE
			u8 _x[HPI_MESSAGE_FORCE_SIZE - HPI_MESSAGE_FIXED_SIZE];
#endif
		} u;
	} HPI_MESSAGE;

#define HPI_MESSAGE_SIZE_BY_OBJECT { \
HPI_MESSAGE_FIXED_SIZE ,   /* Default, no object type 0 */ \
HPI_MESSAGE_FIXED_SIZE + sizeof(HPI_SUBSYS_MSG),\
HPI_MESSAGE_FIXED_SIZE + sizeof(HPI_ADAPTERX_MSG),\
HPI_MESSAGE_FIXED_SIZE + sizeof(HPI_STREAM_MSG),\
HPI_MESSAGE_FIXED_SIZE + sizeof(HPI_STREAM_MSG),\
HPI_MESSAGE_FIXED_SIZE + sizeof(HPI_MIXER_MSG),\
HPI_MESSAGE_FIXED_SIZE ,   /* No NODE message */ \
HPI_MESSAGE_FIXED_SIZE + sizeof(HPI_CONTROL_MSG),\
HPI_MESSAGE_FIXED_SIZE + sizeof(HPI_NVMEMORY_MSG),\
HPI_MESSAGE_FIXED_SIZE + sizeof(HPI_GPIO_MSG),\
HPI_MESSAGE_FIXED_SIZE + sizeof(HPI_WATCHDOG_MSG),\
HPI_MESSAGE_FIXED_SIZE + sizeof(HPI_CLOCK_MSG),\
HPI_MESSAGE_FIXED_SIZE + sizeof(HPI_PROFILE_MSG),\
HPI_MESSAGE_FIXED_SIZE + sizeof(HPI_CONTROLX_MSG),\
HPI_MESSAGE_FIXED_SIZE + sizeof(HPI_ASYNC_MSG) \
}

/* the size of the part of the response outside the union.  MUST update if more elements are added */

	typedef struct sHPI_RESPONSE_HEADER {
		u16 wSize;
		u16 wType;	/* HPI_MSG_MESSAGE, HPI_MSG_RESPONSE */
		u16 wObject;	/* HPI_OBJ_SUBSYS, _OBJ_ADAPTER, _OBJ_DEVICE, _OBJ_MIXER */
		u16 wFunction;	/* HPI_SUBSYS_xxx, HPI_ADAPTER_xxx */
		u16 wError;	/* HPI_ERROR_xxx */
		u16 wSpecificError;	/* Adapter specific error */
	} HPI_RESPONSE_HEADER;
#define HPI_RESPONSE_FIXED_SIZE (sizeof(HPI_RESPONSE_HEADER))

	typedef struct sHPI_RESPONSE {	/* following fields must match HPI_RESPONSE_HEADER */
		u16 wSize;
		u16 wType;	/* HPI_MSG_MESSAGE, HPI_MSG_RESPONSE */
		u16 wObject;	/* HPI_OBJ_SUBSYS, _OBJ_ADAPTER, _OBJ_DEVICE, _OBJ_MIXER */
		u16 wFunction;	/* HPI_SUBSYS_xxx, HPI_ADAPTER_xxx */
		u16 wError;	/* HPI_ERROR_xxx */
		u16 wSpecificError;	/* Adapter specific error */
		union {
			HPI_SUBSYS_RES s;	/* SubSys; */
			HPI_ADAPTER_RES a;	/* Adapter; */
			HPI_ADAPTERX_RES ax;	/* Adapter; */
			HPI_STREAM_RES d;	/* stream; */
			HPI_MIXER_RES m;	/* Mixer; */
			HPI_MIXERX_RES mx;	/* extended Mixer; */
			HPI_CONTROL_RES c;	/* mixer Control; */
			HPI_CONTROLX_RES cx;	/* extended mixer Control; */
			HPI_NVMEMORY_RES n;	/* non-vol memory; */
			HPI_GPIO_RES l;	/* digital i/o */
			HPI_WATCHDOG_RES w;	/* watch-dog */
			HPI_CLOCK_RES t;	/* dsp time */
			HPI_PROFILE_RES p;	/* profile */
			HPI_ASYNC_RES as;	/* async event */
		} u;
	} HPI_RESPONSE;

#define HPI_RESPONSE_SIZE_BY_OBJECT { \
HPI_RESPONSE_FIXED_SIZE ,   /* Default, no object type 0 */ \
HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_SUBSYS_RES),\
HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_ADAPTERX_RES),\
HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_STREAM_RES),\
HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_STREAM_RES),\
HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_MIXER_RES),\
HPI_RESPONSE_FIXED_SIZE , /* No NODE RESPONSE */ \
HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_CONTROL_RES),\
HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_NVMEMORY_RES),\
HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_GPIO_RES),\
HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_WATCHDOG_RES),\
HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_CLOCK_RES),\
HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_PROFILE_RES),\
HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_CONTROLX_RES),\
HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_ASYNC_RES) \
}

/*////////////////////////////////////////////////////////////////////////// */
/* declarations for compact control calls                                    */
	typedef struct HPI_CONTROL_DEFN {
		u16 wType;
		u8 wSrcNodeType;
		u8 wSrcNodeIndex;
		u8 wDestNodeType;
		u8 wDestNodeIndex;
	} HPI_CONTROL_DEFN;

/*////////////////////////////////////////////////////////////////////////// */
/* declarations for control caching (internal to HPI<->DSP interaction)      */
	typedef struct {
		u16 anLog[2];
	} tHPIControlCachedVolume;
	typedef struct {
		u16 anLogPeak[2];
		u16 anLogRMS[2];
	} tHPIControlCachedPeak;
	typedef struct {
		u16 wMode;
	} tHPIControlCachedChannelMode;
	typedef struct {
		u32 dwErrorStatus;
		u32 dwSource;
	} tHPIControlCachedAES3RxStatus;
	typedef struct {
		u32 dwFormat;
	} tHPIControlCachedAES3Tx;
	typedef struct {
		u16 wSourceNodeType;
		u16 wSourceNodeIndex;
	} tHPIControlCachedMultiplexer;
	typedef struct {
		u16 anLog[2];
	} tHPIControlCachedLevel;
	typedef struct {	/* only partial caching - some attributes have to go to the DSP. */
		u32 dwFreqInkHz;
		u16 wBand;
		u16 wLevel;
	} tHPIControlCachedTuner;
	typedef struct {
		u16 wState;
	} tHPIControlCachedToneDetector;
	typedef struct {
		u32 dwState;
		u32 dwCount;
	} tHPIControlCachedSilenceDetector;
	typedef struct {
		u16 wSource;
		u16 wSourceIndex;
		u32 dwSampleRate;
	} tHPIControlCachedSampleClock;
	typedef struct {
		u32 dw1;
		u32 dw2;
	} tHPIControlCachedGeneric;

/** A compact representation of (part of) a controls state.
Used for efficient transfer of the control state between DSP and host or across a network
*/
	typedef struct {
		u16 ControlType;
		      /**< one of HPI_CONTROL_* */
		u16 ControlIndex;
		      /**< The original index of the control on the DSP */
		union {
			tHPIControlCachedVolume v;
			tHPIControlCachedPeak p;
			tHPIControlCachedChannelMode m;
			tHPIControlCachedMultiplexer x;
			tHPIControlCachedLevel l;
			tHPIControlCachedTuner t;
			tHPIControlCachedAES3RxStatus aes3rx;
			tHPIControlCachedAES3Tx aes3tx;
			tHPIControlCachedToneDetector tone;
			tHPIControlCachedSilenceDetector silence;
			tHPIControlCachedSampleClock clk;
			tHPIControlCachedGeneric g;
		} u;
	} tHPIControlCacheSingle;
/*//////////////////////////////////////////////////////////////////////////////// */
/* declarations for 2^N sized FIFO buffer (internal to HPI<->DSP interaction)      */
	typedef struct {
		u32 dwSize;
		u32 dwDSPIndex;
		u32 dwHostIndex;
	} tHPIFIFOBuffer;

#ifndef DISABLE_PRAGMA_PACK1
#pragma pack(pop)
#endif

#ifndef HPI_ON_DSP
/* skip host side function declarations for DSP compile and documentation extraction */
	void HPI_InitMessage(HPI_MESSAGE * phm, u16 wObject, u16 wFunction);
	void HPI_InitResponse(HPI_RESPONSE * phr, u16 wObject, u16 wFunction,
			      u16 wError);

/*////////////////////////////////////////////////////////////////////////// */
/* main HPI entry point */
	HPI_HandlerFunc HPI_Message;

#ifndef HPI_KERNEL_MODE		//----------------- not Win95,WinNT,WDM kernel
	u16 HPI_DriverOpen(void);
	void HPI_DriverClose(void);
#endif

/* used in PnP OS/driver */
	u16 HPI_SubSysCreateAdapter(HPI_HSUBSYS * phSubSys,
				    HPI_RESOURCE * pResource,
				    u16 * pwAdapterIndex);

	u16 HPI_SubSysDeleteAdapter(HPI_HSUBSYS * phSubSys, u16 wAdapterIndex);

	void HPI_FormatToMsg(HPI_MSG_FORMAT * pMF, HPI_FORMAT * pF);

/*////////////////////////////////////////////////////////////////////////// */
/* declarations for individual HPI entry points */
	HPI_HandlerFunc HPI_4000;
	HPI_HandlerFunc HPI_6000;
	HPI_HandlerFunc HPI_6205;

#endif				/* ndef HPI_ON_DSP */

#endif				/* ndef HPI_EXCLUDE_IMPLEMENTATION */

#ifdef __cplusplus
}
#endif
#endif	 /*_H_HPI_ */
/*
///////////////////////////////////////////////////////////////////////////////
// See CVS for history.  Last complete set in rev 1.146
////////////////////////////////////////////////////////////////////////////////
*/
