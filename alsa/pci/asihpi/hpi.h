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

#define HPI_VER     0x000297L

/* ///////////////////////////////////////////////////////////////////////// */
/* OS DEPENDENT STUFF */
/* use compiler ID to indentify OS */
#ifdef __DSP563C__
#define HPI_OS_DSP_563XX
#endif

#ifdef _C56
#define HPI_OS_DSP_563XX
#endif

#if defined ( _TMS320C6X )
#define HPI_OS_DSP_C6000
#endif

/* //////////////////////////////////////////////////////// */
/* Win32 User mode (AGE 8/29/97 moved here for C++ Builder) */

#ifdef __cplusplus
extern "C" {
#endif
/* ///////////////////////////////////////////////////////////////////////// */
/* OS DEPENDENT STUFF */

#ifndef __GNUC__
#pragma pack(1)			/* tell compilers to use byte alignment for items in a structure */
#endif

/* ////////////////////////////////////////////////////// */
/*16bit realmode DOS - Borland C/C++ 3.1 or MSVC*/

/* /////////////////////////////////////////////////// */
/* 16bit Windows - Borland C/C++ 4.X or Microsoft Visual C 1.52*/

/* //////////////////////////////////////////////////// */
/* kernel mode in WinNT - Microsoft Visual C/C++ 4.X */

/* ///////////////////////////////////////////////////// */
/* Win95 kernal mode(VXD)  - Microsoft Visual C/C++ 4.X */

/* /////////////////////////////////////////////// */
/* Motorola 563XX DSP environment - ?? C compiler */

/* /////////////////////////////////////////////// */
/*     TI C6000 DSP environment - ?? C compiler */

/* /////////////////////////////////////////////// */
/* Windows WDM kernel driver - Visual C 4.XX/5.XX */
/* /////////////////////////////////////////////// */
/* Linux 2.2 kernel driver - GNU C compiler.  */
#ifdef linux
#define HPI_OS_LINUX
#define HPI_OS_DEFINED

#if __GNUC__ >= 3
#define DEPRECATED __attribute__((deprecated))
#endif

#ifdef __KERNEL__
#define HPI_KERNEL_MODE

#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif
#include <linux/kernel.h>
#include <linux/string.h>
#endif

#define STR_SIZE(a) (a)
#define __pack__ __attribute__ ((packed))
#endif

/* /////////////////////////////////////////////// */
/* No OS defined!                                  */

#ifndef DEPRECATED
#define DEPRECATED
#endif

/******************************************************************************/
/******************************************************************************/
/********                     HPI API DEFINITIONS                     *********/
/******************************************************************************/
/******************************************************************************/

/* //////////////////////////////////////////////////////////////////////// */
/* BASIC TYPES */

#define u32_test

#if defined ( u32_test )
/* H_W_32 has been redefined to u32 which is already defined by the linux kernel */
#else
/* unsigned word types */
	typedef unsigned char u8;
			       /**< 8 bit word = byte */
	typedef unsigned short u16;
	typedef unsigned int u32;
			       /**< 32 bit word (can't use long here!) */

#endif

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

#define HPI_ADAPTER_ASI5001             0x5001		/**< ASI5001 OEM, PCM only, 4 in, 1 out analog */
#define HPI_ADAPTER_ASI5002             0x5002		/**< ASI5002 OEM, PCM only, 4 in, 1 out analog and digital */
#define HPI_ADAPTER_ASI5044             0x5044		/**< ASI5044 PCM only, 4 analog and digital in/out */
#define HPI_ADAPTER_ASI5041             0x5041		/**< ASI5041 PCM only, 4 digital only in/out */
#define HPI_ADAPTER_ASI5042             0x5042		/**< ASI5042 PCM only, 4 analog only in/out */

#define HPI_ADAPTER_ASI5101             0x5101		/**< ASI5101 OEM is ASI5111 with no mic. */
#define HPI_ADAPTER_ASI5111             0x5111		/**< ASI5111 PCM only */

#define HPI_ADAPTER_ASI6101             0x6101		/**< ASI6101 prototype */

#define HPI_ADAPTER_ASI6000             0x6000		/**< ASI6000 - generic 1 DSP adapter, exact config undefined */
#define HPI_ADAPTER_ASI6012             0x6012			/**< ASI6012 - 1 in, 2 out analog only */
#define HPI_ADAPTER_ASI6022             0x6022			/**< ASI6022 - 2 in, 2 out analog only */
#define HPI_ADAPTER_ASI6044             0x6044			/**< ASI6044 - 4 in/out analog only */
#define HPI_ADAPTER_ASI6111             0x6111		/**< ASI6111 - 1 in/out, analog and AES3  */
#define HPI_ADAPTER_ASI6102             0x6102		/**< ASI6102 - 2out,analog and AES3  */
#define HPI_ADAPTER_ASI6113             0x6113		/**< 300MHz version of ASI6114 for testing*/
#define HPI_ADAPTER_ASI6122             0x6122		/**< ASI6122 - 2 in/out, analog and AES3  */
#define HPI_ADAPTER_ASI6114             0x6114		/**< ASI6114 - 4os,1is,4out,1in,analog and AES3  */
#define HPI_ADAPTER_ASI6118             0x6118		/**< ASI6118 - 8os,1is,8out,1in analog+AES3 */
#define HPI_ADAPTER_ASI6201             0x6201		/**< ASI6201 - OEM  */
#define HPI_ADAPTER_ASI6244             0x6244		/**< ASI6244 - 4os,4is,4out,4in,analog and AES3 */
#define HPI_ADAPTER_ASI6246             0x6246		/**< ASI6246 - 6os,2is,6out,4in,analog and AES3 */
#define HPI_ADAPTER_ASI6200             0x6200		/**< ASI6200 - generic 2 DSP adapter, exact config undefined */
#define HPI_ADAPTER_ASI6100             0x6100		/**< ASI6100 - generic 1 DSP adapter, exact config undefined */

#define HPI_ADAPTER_ASI6408             0x6408		/**< ASI6408 - cobranet PCI 8 mono in/out */
#define HPI_ADAPTER_ASI6416             0x6416		/**< ASI6416 - cobranet PCI 16 mono in/out */

#define HPI_ADAPTER_ASI8401             0x8401		/**< OEM 4 record */
#define HPI_ADAPTER_ASI8411             0x8411		/**< OEM RF switcher */

#define HPI_ADAPTER_ASI8601             0x8601		/**< OEM 8 record */

#define HPI_ADAPTER_ASI8701             0x8701		/**< OEM 8 record 2 AM/FM 8 FM/TV , AM has 10kHz b/w*/
#define HPI_ADAPTER_ASI8702             0x8702		/**< 8 AM/FM record */
#define HPI_ADAPTER_ASI8703             0x8703		/**< 8 TV/FM record */
#define HPI_ADAPTER_ASI8704             0x8704		/**< standard product 2 AM/FM 8 FM/TV */
#define HPI_ADAPTER_ASI8705             0x8705		/**< 4 TV/FM, 4 AM/FM record */
#define HPI_ADAPTER_ASI8706             0x8706		/**< OEM 8 record 2 AM/FM 8 FM/TV */
#define HPI_ADAPTER_ASI8707             0x8707		/**< 8 record AM/FM - 4 ext antenna jacks */
#define HPI_ADAPTER_ASI8708             0x8708		/**< 8 record AM/FM - 6 ext antenna jacks */

#define HPI_ADAPTER_ASI8712             0x8712		/**< 4 record AM/FM */
#define HPI_ADAPTER_ASI8713             0x8713		/**< 4 record TV/FM */

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

/*! Update this if you add a new control type. , AND hpidebug.h */
#define HPI_CONTROL_LAST_INDEX                  21

/* WARNING types 32 or greater impact bit packing in all AX4 DSP code */
/* WARNING types 256 or greater impact bit packing in all AX6 DSP code */

/******************************************* CONTROL ATTRIBUTES */
/* This allows for 255 control types, 255 unique attributes each */
#define HPI_CONTROL_SPACING (0x100)
#define HPI_MAKE_ATTRIBUTE(obj,index) (obj*HPI_CONTROL_SPACING+index)

/* Volume Control attributes */
#define HPI_VOLUME_GAIN             1
#define HPI_VOLUME_RANGE            10	/* make this very different from the other HPI_VOLUME_ defines below */

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

/* Multiplexer control attributes */
#define HPI_ONOFFSWITCH_STATE       1	/* AGE 1/15/98 */

#define HPI_SWITCH_OFF              0
#define HPI_SWITCH_ON               1

/* VOX control attributes */
#define HPI_VOX_THRESHOLD           1	/* AGE 9/10/98 */

/** \defgroup adapter_properties Adapter properties used in HPI_AdapterSetProperty() API.
\{
*/
/*! Used in dwProperty field of HPI_AdapterSetProperty() and HPI_AdapterGetProperty(). This errata applies to all
ASI6000 cards with both analog and digital outputs. The CS4224 A/D+D/A has a one sample
delay between left and right channels on both its input (ADC) and output (DAC). More details are available in
Cirrus Logic errata ER284B2.PDF available from http://www.cirrus.com, released by Cirrus in 2001.
*/
#define HPI_ADAPTER_PROPERTY_ERRATA_1   (1)
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

/** \defgroup mixer_flags Mixer flags used in processing function HPI_MIXER_GET_CONTROL_MULTIPLE_VALUES
\{
*/
#define HPI_MIXER_GET_CONTROL_MULTIPLE_CHANGED (0)
#define HPI_MIXER_GET_CONTROL_MULTIPLE_RESET (1)
/**\}*/

/* Note, adapters can have more than one capability - encoding as bitfield is recommended. */
#define HPI_CAPABILITY_NONE  (0)
#define HPI_CAPABILITY_MPEG_LAYER3  (1)
/* Set this equal to maximum capability index, Must not be greater than 32 - see axnvdef.h */
#define HPI_CAPABILITY_MAX          1
/* #define HPI_CAPABILITY_AAC          2 */

/** Channel mode settings */
#define HPI_CHANNEL_MODE_NORMAL                 1	/* AGE 8/6/99 */
#define HPI_CHANNEL_MODE_SWAP                   2
#define HPI_CHANNEL_MODE_LEFT_TO_STEREO         3
#define HPI_CHANNEL_MODE_RIGHT_TO_STEREO        4
#define HPI_CHANNEL_MODE_STEREO_TO_LEFT         5
#define HPI_CHANNEL_MODE_STEREO_TO_RIGHT        6
#define HPI_CHANNEL_MODE_LAST                           6

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
#define HPI_TUNER_BAND_TV               3    /**< TV band*/
#define HPI_TUNER_BAND_FM_STEREO   4	/**< FM band (stereo) */
#define HPI_TUNER_BAND_AUX             5    /**< Auxiliary input */
#define HPI_TUNER_BAND_LAST                             5 /**< The index of the last tuner band. */
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

#define HPI_AES18_MODE_MASTER           0
#define HPI_AES18_MODE_SLAVE            1
#define HPI_AES18_CHANNEL_MODE_INDEPENDENT            (2)   /**< Left and Right channels operate independently */
#define HPI_AES18_CHANNEL_MODE_JOINT            (1) /**< Messages use alternating bits on the the left and right channels */

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

/** \defgroup async_event Async Event sources
\{
*/
#define HPI_ASYNC_EVENT_GPIO (1)		/**< GPIO event. */
#define HPI_ASYNC_EVENT_SILENCE (2)		/**< Silence event detected. */
#define HPI_ASYNC_EVENT_TONE_30HZ (3)	/**< 30 Hz tone event detected. */
/** \} */

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
/*! Update this if you add a new clock source2.*/
#define HPI_SAMPLECLOCK_SOURCE_LAST                     8
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
#define HPI_ETHERNET_PACKET_V1 0x01	/*!< Simple packet - no special routing required */
#define HPI_ETHERNET_PACKET_HOSTED_VIA_HMI 0x20	/*!< This packet must make its way to the host across the HPI interface */
#define HPI_ETHERNET_PACKET_HOSTED_VIA_HMI_V1 0x21	/*!< This packet must make its way to the host across the HPI interface */
#define HPI_ETHERNET_PACKET_HOSTED_VIA_HPI 0x40	/*!< This packet must make its way to the host across the HPI interface */
#define HPI_ETHERNET_PACKET_HOSTED_VIA_HPI_V1 0x41	/*!< This packet must make its way to the host across the HPI interface */

/* Ancillary Data modes */
#define HPI_MPEG_ANC_HASENERGY  (0)
#define HPI_MPEG_ANC_RAW                (1)
#define HPI_MPEG_ANC_ALIGN_LEFT (0)
#define HPI_MPEG_ANC_ALIGN_RIGHT (1)

/** \defgroup mpegmodes MPEG modes
\{
MPEG modes - can be used optionally for HPI_FormatCreate() parameter dwAttributes.

The operation of the below modes varies acording to the number of channels. Using HPI_MPEG_MODE_DEFAULT
causes the MPEG-1 Layer II bitstream to be recorded in single_channel mode when the number
of channels is 1 and in stereo when the number of channels is 2. Using any mode setting other
than HPI_MPEG_MODE_DEFAULT when the number of channels is set to 1 will return an error.
*/
#define HPI_MPEG_MODE_DEFAULT   (0)
#define HPI_MPEG_MODE_STEREO    (1)
#define HPI_MPEG_MODE_JOINTSTEREO       (2)
#define HPI_MPEG_MODE_DUALCHANNEL       (3)
/** \} */

/******************************************* CONTROLX ATTRIBUTES */
/* NOTE: All controlx attributes must be unique, unlike control attributes */
/* AES18 attributes */
#define HPI_AES18_CONFIG                101
#define HPI_AES18_INTERNAL_BUFFER_SIZE  102
#define HPI_AES18_INTERNAL_BUFFER_STATE 103
#define HPI_AES18_MESSAGE               104
#define HPI_AES18_ADDRESS               105

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
#define HPI_ERROR_OBJ_NOT_OPEN          104
#define HPI_ERROR_OBJ_ALREADY_OPEN      105
#define HPI_ERROR_INVALID_RESOURCE      106 /**< PCI, ISA resource not valid */
#define HPI_ERROR_SUBSYSFINDADAPTERS_GETINFO      107 /**< GetInfo call from SubSysFindAdapters failed. */
#define HPI_ERROR_INVALID_RESPONSE      108 /**< Default response was never updated with actual error code */
#define HPI_ERROR_PROCESSING_MESSAGE    109 /**< wSize field of response was not updated, indicating that the msg was not processed */
#define HPI_ERROR_NETWORK_TIMEOUT       110 /**< The network did not respond in a timely manner */

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

/** AES18 specific errors */
#define HPI_ERROR_AES18BG_BLOCKSPERSEC  500
#define HPI_ERROR_AES18BG_PRIORITYMASK  501
#define HPI_ERROR_AES18BG_MODE          502
#define HPI_ERROR_AES18_INVALID_PRIORITY    503
#define HPI_ERROR_AES18_INVALID_ADDRESS 504
#define HPI_ERROR_AES18_INVALID_REPETITION  505
#define HPI_ERROR_AES18BG_CHANNEL_MODE      506
#define HPI_ERROR_AES18_INVALID_CHANNEL  507

#define HPI_ERROR_CUSTOM                600 /**< custom error to use for debugging AGE 6/22/99 */

#define HPI_ERROR_MUTEX_TIMEOUT         700 /**< hpioct32.c can't obtain mutex */
/**\}*/

/* maximums */
#define HPI_MAX_ADAPTERS        20     /**< Maximum number of adapters per HPI sub-system WARNING: modifying this value changes the response structure size.*/
#define HPI_MAX_ADAPTER_MEM_SPACES (2)	/**< maximum number of memory regions mapped to an adapter */
#define HPI_MAX_STREAMS         16	 /**< Maximum number of in or out streams per adapter */
#define HPI_MAX_CHANNELS        2	/* per stream */
#define HPI_MAX_NODES           8	/* per mixer ? */
#define HPI_MAX_CONTROLS        4	/* per node ? */
#define HPI_MAX_ANC_BYTES_PER_FRAME             (64)	/**< maximum number of ancillary bytes per MPEG frame */
#define HPI_STRING_LEN          16
#define HPI_AES18_MAX_CHANNELS  2
#define HPI_AES18_MAX_PRIORITIES 4
#define HPI_AES18_MAX_PRIORITYMASK      0x000F
#define HPI_AES18BG_MAX_BLOCKSPERSEC    100	/**< AES18-1996, 100 blocks/sec */
#define HPI_AES18BG_MIN_BLOCKSPERSEC    2
#define HPI_AES18_MAX_ADDRESS           65536	/**< 16 bits address */
#define HPI_AES18_MAX_REPETITION        5	/**< AES18-1996, pg 5 */

/* units */
#define HPI_OSTREAM_VELOCITY_UNITS        4096	/* AGE 6/4/99 */
#define HPI_OSTREAM_TIMESCALE_UNITS       (10000)

/* defaults */
#define HPI_AES18_DEFAULT_PRIORITY_ENB  0x000F
/*
See pg 16, AES18-1996
@ 40 ms
44.1 kHz = 1764 bits/block
48 kHz = 1920 bits/block
*/
#define HPI_AES18_DEFAULT_BLOCK_LENGTH  1764	// Bits.  Corresponds to 40ms at 44.1kHz
#define HPI_AES18_DEFAULT_OP_MODE       HPI_AES18_MODE_MASTER

/** Pnp ids */
#define HPI_ID_ISAPNP_AUDIOSCIENCE      0x0669	/*"ASI"  - actual is "ASX" - need to change */
#define HPI_PCI_VENDOR_ID_AUDIOSCIENCE  0x175C	 /**< PCI vendor ID that AudioScience uses */
#define HPI_PCI_VENDOR_ID_MOTOROLA      0x1057	 /**< PCI vendor ID that the DSP56301 has */
#define HPI_PCI_VENDOR_ID_TI            0x104C	 /**< PCI vendor ID that TI uses */

#define HPI_USB_VENDOR_ID_AUDIOSCIENCE  0x1257
#define HPI_USB_W2K_TAG         0x57495341	/* "ASIW"       */
#define HPI_USB_LINUX_TAG       0x4C495341	/* "ASIL"       */

/**
end group hpi_defines
\}

*/
/* ////////////////////////////////////////////////////////////////////// */
/* STRUCTURES */

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

	typedef struct {
		u32 dwSampleRate;
		       /**< 11025, 32000, 44100 ... */
		u32 dwBitRate;
		       /**< for MPEG */
		u32 dwAttributes;
		       /**< special stuff like CRC/Copyright on/off */
		u16 wMode;
		       /**< Stereo/JointStereo/Mono etc */
		u16 wSize;
		       /**< length of this structure */
		u16 wChannels;
		       /**< 1,2... */
		u16 wFormat;
		       /**< HPI_FORMAT_PCM16, _AC2, _MPEG ... */
	} HPI_FORMAT;

	typedef struct {
		HPI_FORMAT Format;
		u32 dwpbData; /**< actually a pointer to BYTE*/
		u32 dwDataSize;
		/*u16            wSize; AGE 3/23/99 removed *//* length of this structure */
	} HPI_DATA;

	typedef struct {
		u32 dwValidBitsInThisFrame;
		u8 bData[HPI_MAX_ANC_BYTES_PER_FRAME];
	} HPI_ANC_FRAME;

	typedef struct {
		u32 dwPunchInSample;
		u32 dwPunchOutSample;
	} HPI_PUNCHINOUT;

/** An object for containing a single async event.
*/
	typedef struct {
		u16 wEventType;	 /**< Type of event. See HPI_ASYNC_EVENT_GPIO (etc.) defines. */
		u16 wState;	     /**< New state, either 0 or 1. */
		u32 hHandle;	 /**< Handle to the object returning the event. */
		union {
			struct {
				u16 wIndex;
		       /**< GPIO bit index. */
			} gpio;
			struct {
				u16 wNodeIndex;
			       /**< What node is the control on ? */
			} control;
		} u;
	} HPI_ASYNC_EVENT;

	typedef unsigned char HPI_ETHERNET_MAC_ADR[6];
						/**< Used for sending ethernet packets VIA HMI interface */

/** PCI bus resource */
	typedef struct {
		u16 wVendorId;
		u16 wDeviceId;
		u16 wSubSysVendorId;
		u16 wSubSysDeviceId;
		u16 wBusNumber;
		u16 wDeviceNumber;
		u32 dwMemBase[HPI_MAX_ADAPTER_MEM_SPACES];
		u32 wInterrupt;
		struct pci_dev *pOsData;
	} HPI_PCI;

/* USB bus type resource */
	typedef struct {
		u16 wVendorId;
		u16 wDeviceId;
		void *pReserved;	/* OS specific USB info */
	} HPI_USB;

/* PortIO */
	typedef struct {
		u32 dwAddress;
		u32 dwData;
	} HPI_PORTIO;

	typedef struct {
		union {
			HPI_PCI Pci;
			HPI_USB Usb;
			HPI_PORTIO PortIO;

		} r;
		u16 wBusType;	/* HPI_BUS_PNPISA, _PCI, _USB etc */
		u16 wPadding;

	} HPI_RESOURCE;

/*////////////////////////////////////////////////////////////////////////// */
/* HPI FUNCTIONS */

/* handles that reference various objects */
	typedef u32 HPI_HADAPTER;
	typedef u32 HPI_HOSTREAM;
	typedef u32 HPI_HISTREAM;
	typedef u32 HPI_HMIXER;
	typedef u32 HPI_HCONTROL;
	typedef u32 HPI_HNVMEMORY;
	typedef u32 HPI_HGPIO;
	typedef u32 HPI_HWATCHDOG;
	typedef u32 HPI_HCLOCK;
	typedef u32 HPI_HPROFILE;
	typedef u32 HPI_HASYNC;

/* skip host side function declarations for DSP compile and documentation extraction */

/* handles to various objects */
	typedef struct {

		short nOs;	/* Operating System = Win95,WinNT */
		u32 dwIoctlCode;
		short nHandle;
	} HPI_HSUBSYS;

/*/////////////////////////// */
/* DATA and FORMAT and STREAM */

	u16 HPI_FormatCreate(HPI_FORMAT * pFormat,
			     u16 wChannels,
			     u16 wFormat,
			     u32 dwSampleRate, u32 dwBitrate, u32 dwAttributes);

	u16 HPI_DataCreate(HPI_DATA * pData,
			   HPI_FORMAT * pFormat, u8 * pbData, u32 dwDataSize);

	u16 HPI_StreamEstimateBufferSize(HPI_FORMAT * pF,
					 u32 dwHostPollingRateInMilliSeconds,
					 u32 * dwRecommendedBufferSize);

/*/////////// */
/* SUB SYSTEM */
	HPI_HSUBSYS *HPI_SubSysCreate(void);

	void HPI_SubSysFree(HPI_HSUBSYS * phSubSysHandle);

	u16 HPI_SubSysGetVersion(HPI_HSUBSYS * phSubSysHandle,
				 u32 * pdwVersion);

	u16 HPI_SubSysGetInfo(HPI_HSUBSYS * phSubSysHandle,
			      u32 * pdwVersion,
			      u16 * pwNumAdapters,
			      u16 awAdapterList[], u16 wListLength);

/* used in PnP OS/driver */
	u16 HPI_SubSysCreateAdapter(HPI_HSUBSYS * phSubSysHandle,
				    HPI_RESOURCE * pResource,
				    u16 * pwAdapterIndex);
	u16 HPI_SubSysDeleteAdapter(HPI_HSUBSYS * phSubSys, u16 wAdapterIndex);

/* SGT added 3-2-97 */
	u16 HPI_SubSysFindAdapters(HPI_HSUBSYS * phSubSysHandle,
				   u16 * pwNumAdapters,
				   u16 awAdapterList[], u16 wListLength);

	u16 HPI_SubSysReadPort8(HPI_HSUBSYS * phSubSysHandle,
				u16 wAddress, u16 * pwData);

	u16 HPI_SubSysWritePort8(HPI_HSUBSYS * phSubSysHandle,
				 u16 wAddress, u16 wData);

/*///////// */
/* ADAPTER */

	u16 HPI_AdapterOpen(HPI_HSUBSYS * phSubSysHandle, u16 wAdapterIndex);

	u16 HPI_AdapterClose(HPI_HSUBSYS * phSubSysHandle, u16 wAdapterIndex);

	u16 HPI_AdapterGetInfo(HPI_HSUBSYS * phSubSys,
			       u16 wAdapterIndex,
			       u16 * pwNumOutStreams,
			       u16 * pwNumInStreams,
			       u16 * pwVersion,
			       u32 * pdwSerialNumber, u16 * pwAdapterType);

	u16 HPI_AdapterSetMode(HPI_HSUBSYS * phSubSysHandle,
			       u16 wAdapterIndex, u32 dwAdapterMode);
	u16 HPI_AdapterSetModeEx(HPI_HSUBSYS * phSubSysHandle,
				 u16 wAdapterIndex,
				 u32 dwAdapterMode, u16 wQueryOrSet);

	u16 HPI_AdapterGetMode(HPI_HSUBSYS * phSubSysHandle,
			       u16 wAdapterIndex, u32 * pdwAdapterMode);

	u16 HPI_AdapterGetAssert(HPI_HSUBSYS * phSubSysHandle,
				 u16 wAdapterIndex,
				 u16 * wAssertPresent,
				 char *pszAssert, u16 * pwLineNumber);

	u16 HPI_AdapterGetAssertEx(HPI_HSUBSYS * phSubSysHandle,
				   u16 wAdapterIndex,
				   u16 * wAssertPresent,
				   char *pszAssert,
				   u32 * pdwLineNumber, u16 * pwAssertOnDsp);

	u16 HPI_AdapterTestAssert(HPI_HSUBSYS * phSubSysHandle,
				  u16 wAdapterIndex, u16 wAssertId);

	u16 HPI_AdapterEnableCapability(HPI_HSUBSYS * phSubSysHandle,
					u16 wAdapterIndex,
					u16 wCapability, u32 dwKey);

	u16 HPI_AdapterSelfTest(HPI_HSUBSYS * phSubSysHandle,
				u16 wAdapterIndex);

	u16 HPI_AdapterSetProperty(HPI_HSUBSYS * phSubSysHandle,
				   u16 wAdapterIndex,
				   u16 wProperty,
				   u16 wParamter1, u16 wParamter2);

	u16 HPI_AdapterGetProperty(HPI_HSUBSYS * phSubSysHandle,
				   u16 wAdapterIndex,
				   u16 wProperty,
				   u16 * pwParamter1, u16 * pwParamter2);

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
	u16 HPI_OutStreamOpen(HPI_HSUBSYS * phSubSysHandle,
			      u16 wAdapterIndex,
			      u16 wOutStreamIndex,
			      HPI_HOSTREAM * phOutStreamHandle);

	u16 HPI_OutStreamClose(HPI_HSUBSYS * phSubSysHandle,
			       HPI_HOSTREAM hOutStreamHandle);

	DEPRECATED u16 HPI_OutStreamGetInfo(HPI_HSUBSYS * phSubSysHandle,
					    HPI_HOSTREAM hOutStreamHandle,
					    u16 * pwState,
					    u32 * pdwBufferSize,
					    u32 * pdwDataToPlay);

	u16 HPI_OutStreamGetInfoEx(HPI_HSUBSYS * phSubSysHandle,
				   HPI_HOSTREAM hOutStreamHandle,
				   u16 * pwState,
				   u32 * pdwBufferSize,
				   u32 * pdwDataToPlay,
				   u32 * pdwSamplesPlayed,
				   u32 * pdwAuxiliaryDataToPlay);

	u16 HPI_OutStreamWrite(HPI_HSUBSYS * phSubSysHandle,
			       HPI_HOSTREAM hOutStreamHandle, HPI_DATA * pData);

	u16 HPI_OutStreamStart(HPI_HSUBSYS * phSubSysHandle,
			       HPI_HOSTREAM hOutStreamHandle);

	u16 HPI_OutStreamStop(HPI_HSUBSYS * phSubSysHandle,
			      HPI_HOSTREAM hOutStreamHandle);

	u16 HPI_OutStreamSinegen(HPI_HSUBSYS * phSubSys,
				 HPI_HOSTREAM hOutStream);

	u16 HPI_OutStreamReset(HPI_HSUBSYS * phSubSysHandle,
			       HPI_HOSTREAM OutStreamHandle);

	u16 HPI_OutStreamQueryFormat(HPI_HSUBSYS * phSubSysHandle,
				     HPI_HOSTREAM OutStreamHandle,
				     HPI_FORMAT * pFormat);

	u16 HPI_OutStreamSetPunchInOut(HPI_HSUBSYS * phSubSysHandle,
				       HPI_HOSTREAM hOutStreamHandle,
				       u32 dwPunchInSample,
				       u32 dwPunchOutSample);

	u16 HPI_OutStreamSetVelocity(HPI_HSUBSYS * phSubSys,
				     HPI_HOSTREAM hOutStream, short nVelocity);

	u16 HPI_OutStreamAncillaryReset(HPI_HSUBSYS * phSubSysHandle, HPI_HOSTREAM OutStreamHandle, u16 wMode);	/* wMode is HPI_MPEG_ANC_XXX */

	u16 HPI_OutStreamAncillaryGetInfo(HPI_HSUBSYS * phSubSysHandle,
					  HPI_HOSTREAM hOutStreamHandle,
					  u32 * pdwFramesAvailable);

	u16 HPI_OutStreamAncillaryRead(HPI_HSUBSYS * phSubSysHandle,
				       HPI_HOSTREAM hOutStreamHandle,
				       HPI_ANC_FRAME * pAncFrameBuffer,
				       u32 dwAncFrameBufferSizeInBytes,
				       u32 dwNumberOfAncillaryFramesToRead);

	u16 HPI_OutStreamSetTimeScale(HPI_HSUBSYS * phSubSysHandle,
				      HPI_HOSTREAM hOutStreamHandle,
				      u32 dwTimeScaleX10000);

	u16 HPI_OutStreamHostBufferAllocate(HPI_HSUBSYS * phSubSys,
					    HPI_HOSTREAM hOutStreamHandle,
					    u32 dwSizeInBytes);

	u16 HPI_OutStreamHostBufferFree(HPI_HSUBSYS * phSubSys,
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

	u16 HPI_InStreamRead(HPI_HSUBSYS * phSubSys,
			     HPI_HISTREAM hInStream, HPI_DATA * pData);

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
				  u32 * pdwAuxilaryDataRecorded);

	u16 HPI_InStreamAncillaryReset(HPI_HSUBSYS * phSubSysHandle, HPI_HISTREAM InStreamHandle, u16 wBytesPerFrame, u16 wMode,	/* = HPI_MPEG_ANC_XXX */
				       u16 wAlignment, u16 wIdleBit);

	u16 HPI_InStreamAncillaryGetInfo(HPI_HSUBSYS * phSubSysHandle,
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

/*////// */
/* MIXER */
	u16 HPI_MixerOpen(HPI_HSUBSYS * phSubSysHandle,
			  u16 wAdapterIndex, HPI_HMIXER * phMixerHandle);

	u16 HPI_MixerClose(HPI_HSUBSYS * phSubSysHandle,
			   HPI_HMIXER hMixerHandle);

	u16 HPI_MixerGetControl(HPI_HSUBSYS * phSubSysHandle,
				HPI_HMIXER hMixerHandle,
				u16 wSrcNodeType,
				u16 wSrcNodeTypeIndex,
				u16 wDstNodeType,
				u16 wDstNodeTypeIndex,
				u16 wControlType,
				HPI_HCONTROL * phControlHandle);

	u16 HPI_MixerGetControlByIndex(HPI_HSUBSYS * phSubSysHandle,
				       HPI_HMIXER hMixerHandle,
				       u16 wControlIndex,
				       u16 * pwSrcNodeType,
				       u16 * pwSrcNodeIndex,
				       u16 * pwDstNodeType,
				       u16 * pwDstNodeIndex,
				       u16 * pwControlType,
				       HPI_HCONTROL * phControlHandle);

/*************************/
/* mixer CONTROLS        */
/*************************/

/* Generic query of available control settings */
	u16 HPI_ControlQuery(const HPI_HSUBSYS * phSubSysHandle,
			     const HPI_HCONTROL hControlHandle,
			     const u16 wAttrib,
			     const u32 dwIndex,
			     const u32 dwParam, u32 * pdwSetting);

/* Generic setting of control attribute value */
	u16 HPI_ControlParamSet(const HPI_HSUBSYS * phSubSysHandle,
				const HPI_HCONTROL hControlHandle,
				const u16 wAttrib,
				const u32 dwParam1, const u32 dwParam2);

/* generic getting of control attribute value.
Null pointers allowed for return values
*/
	u16 HPI_ControlParam2Get(const HPI_HSUBSYS * phSubSysHandle,
				 const HPI_HCONTROL hControlHandle,
				 const u16 wAttrib,
				 u32 * pdwParam1, u32 * pdwParam2);
/*************************/
/* volume control        */
/*************************/
	u16 HPI_VolumeSetGain(HPI_HSUBSYS * phSubSysHandle,
			      HPI_HCONTROL hControlHandle,
			      short anGain0_01dB[HPI_MAX_CHANNELS]
	    );

	u16 HPI_VolumeGetGain(HPI_HSUBSYS * phSubSysHandle,
			      HPI_HCONTROL hControlHandle,
			      short anGain0_01dB[HPI_MAX_CHANNELS]
	    );

#define HPI_VolumeGetRange HPI_VolumeQueryRange
	u16 HPI_VolumeQueryRange(HPI_HSUBSYS * phSubSysHandle,
				 HPI_HCONTROL hControlHandle,
				 short *nMinGain_01dB,
				 short *nMaxGain_01dB, short *nStepGain_01dB);

	u16 HPI_VolumeAutoFade(HPI_HSUBSYS * phSubSysHandle,
			       HPI_HCONTROL hControlHandle,
			       short anStopGain0_01dB[HPI_MAX_CHANNELS],
			       u32 wDurationMs);

	u16 HPI_VolumeAutoFadeProfile(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, short anStopGain0_01dB[HPI_MAX_CHANNELS], u32 dwDurationMs, u16 dwProfile	/* HPI_VOLUME_AUTOFADE_??? */
	    );

/*************************/
/* level control         */
/*************************/
	u16 HPI_LevelSetGain(HPI_HSUBSYS * phSubSysHandle,
			     HPI_HCONTROL hControlHandle,
			     short anGain0_01dB[HPI_MAX_CHANNELS]
	    );

	u16 HPI_LevelGetGain(HPI_HSUBSYS * phSubSysHandle,
			     HPI_HCONTROL hControlHandle,
			     short anGain0_01dB[HPI_MAX_CHANNELS]
	    );

/*************************/
/* meter control         */
/*************************/
	u16 HPI_MeterGetPeak(HPI_HSUBSYS * phSubSysHandle,
			     HPI_HCONTROL hControlHandle,
			     short anPeak0_01dB[HPI_MAX_CHANNELS]
	    );

	u16 HPI_MeterGetRms(HPI_HSUBSYS * phSubSysHandle,
			    HPI_HCONTROL hControlHandle,
			    short anPeak0_01dB[HPI_MAX_CHANNELS]
	    );

	u16 HPI_MeterSetPeakBallistics(HPI_HSUBSYS * phSubSysHandle,
				       HPI_HCONTROL hControlHandle,
				       unsigned short nAttack,
				       unsigned short nDecay);

	u16 HPI_MeterSetRmsBallistics(HPI_HSUBSYS * phSubSysHandle,
				      HPI_HCONTROL hControlHandle,
				      unsigned short nAttack,
				      unsigned short nDecay);

	u16 HPI_MeterGetPeakBallistics(HPI_HSUBSYS * phSubSysHandle,
				       HPI_HCONTROL hControlHandle,
				       unsigned short *nAttack,
				       unsigned short *nDecay);

	u16 HPI_MeterGetRmsBallistics(HPI_HSUBSYS * phSubSysHandle,
				      HPI_HCONTROL hControlHandle,
				      unsigned short *nAttack,
				      unsigned short *nDecay);

/*************************/
/* channel mode control  */
/*************************/
	u16 HPI_ChannelModeSet(HPI_HSUBSYS * phSubSysHandle,
			       HPI_HCONTROL hControlHandle, u16 wMode);

	u16 HPI_ChannelModeGet(HPI_HSUBSYS * phSubSysHandle,
			       HPI_HCONTROL hControlHandle, u16 * wMode);

/*************************/
/* Tuner control         */
/*************************/
	u16 HPI_Tuner_SetBand(HPI_HSUBSYS * phSubSysHandle,
			      HPI_HCONTROL hControlHandle, u16 wBand);

	u16 HPI_Tuner_GetBand(HPI_HSUBSYS * phSubSysHandle,
			      HPI_HCONTROL hControlHandle, u16 * pwBand);

	u16 HPI_Tuner_SetFrequency(HPI_HSUBSYS * phSubSysHandle,
				   HPI_HCONTROL hControlHandle, u32 wFreqInkHz);

	u16 HPI_Tuner_GetFrequency(HPI_HSUBSYS * phSubSysHandle,
				   HPI_HCONTROL hControlHandle,
				   u32 * pwFreqInkHz);

	u16 HPI_Tuner_GetRFLevel(HPI_HSUBSYS * phSubSysHandle,
				 HPI_HCONTROL hControlHandle, short *pwLevel);

	u16 HPI_Tuner_GetRawRFLevel(HPI_HSUBSYS * phSubSysHandle,
				    HPI_HCONTROL hControlHandle,
				    short *pwLevel);

	u16 HPI_Tuner_SetGain(HPI_HSUBSYS * phSubSysHandle,
			      HPI_HCONTROL hControlHandle, short nGain);

	u16 HPI_Tuner_GetGain(HPI_HSUBSYS * phSubSysHandle,
			      HPI_HCONTROL hControlHandle, short *pnGain);

	DEPRECATED u16 HPI_Tuner_GetVideoStatus(	/* AGE 11/10/97 */
						       HPI_HSUBSYS *
						       phSubSysHandle,
						       HPI_HCONTROL
						       hControlHandle,
						       u16 * pwVideoStatus);

/* SGT proposed */
	u16 HPI_Tuner_GetStatus(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 * pwStatusMask,	// tells you which bits are valid
				u16 * pwStatus	// the actual bits
	    );

	u16 HPI_Tuner_SetMode(HPI_HSUBSYS * phSubSysHandle,
			      HPI_HCONTROL hControlHandle,
			      u32 nMode, u32 nValue);

	u16 HPI_Tuner_GetMode(HPI_HSUBSYS * phSubSysHandle,
			      HPI_HCONTROL hControlHandle,
			      u32 nMode, u32 * pnValue);

/****************************/
/* AES/EBU Receiver control */
/****************************/
	u16 HPI_AESEBU_Receiver_SetSource(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 wSource	/* HPI_AESEBU_SOURCE_AESEBU, HPI_AESEBU_SOURCE_SPDIF */
	    );

	u16 HPI_AESEBU_Receiver_GetSource(	/* TFE apr-1-04 */
						 HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 * pwSource	/* HPI_AESEBU_SOURCE_AESEBU, HPI_AESEBU_SOURCE_SPDIF */
	    );

	u16 HPI_AESEBU_Receiver_GetSampleRate(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u32 * pdwSampleRate	/* 0,32000,44100 or 48000 returned */
	    );

	u16 HPI_AESEBU_Receiver_GetUserData(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 wIndex,	/* ranges from 0..23 */
					    u16 * pwData	/* returned user data */
	    );

	u16 HPI_AESEBU_Receiver_GetChannelStatus(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 wIndex,	/* ranges from 0..23 */
						 u16 * pwData	/* returned channel status data */
	    );

	u16 HPI_AESEBU_Receiver_GetErrorStatus(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 * pwErrorData	/* returned error data */
	    );

/*******************************/
/* AES/EBU Transmitter control */
/*******************************/
	u16 HPI_AESEBU_Transmitter_SetSampleRate(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u32 dwSampleRate	/* 32000,44100 or 48000 */
	    );

	u16 HPI_AESEBU_Transmitter_SetUserData(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 wIndex,	/* ranges from 0..23 */
					       u16 wData	/* user data to set */
	    );

	u16 HPI_AESEBU_Transmitter_SetChannelStatus(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 wIndex,	/* ranges from 0..23 */
						    u16 wData	/* channel status data to write */
	    );

	u16 HPI_AESEBU_Transmitter_GetChannelStatus(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 wIndex,	// ranges from 0..23
						    u16 * pwData	// channel status data to write
	    );

	u16 HPI_AESEBU_Transmitter_SetClockSource(	/* SGT nov-4-98 */
							 HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 wClockSource	/* SYNC, ADAPTER */
	    );

	u16 HPI_AESEBU_Transmitter_GetClockSource(	/* TFE apr-1-04 */
							 HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 * pwClockSource	/* SYNC, ADAPTER */
	    );

	u16 HPI_AESEBU_Transmitter_SetFormat(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 wOutputFormat	/* HPI_AESEBU_SOURCE_AESEBU, _SPDIF */
	    );

	u16 HPI_AESEBU_Transmitter_GetFormat(	/* TFE apr-1-04 */
						    HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 * pwOutputFormat	/* HPI_AESEBU_SOURCE_AESEBU, _SPDIF */
	    );

/* multiplexer control */
	u16 HPI_Multiplexer_SetSource(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 wSourceNodeType,	/* source node */
				      u16 wSourceNodeIndex	/* source index */
	    );
	u16 HPI_Multiplexer_GetSource(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 * wSourceNodeType,	/* returned source node */
				      u16 * wSourceNodeIndex	/* returned source index */
	    );

	u16 HPI_Multiplexer_QuerySource(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 nIndex,	/* index number to query (0..N) */
					u16 * wSourceNodeType,	/* returned source node */
					u16 * wSourceNodeIndex	/* returned source index */
	    );

/*************************/
/* on/off switch control */
/*************************/
	u16 HPI_OnOffSwitch_SetState(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 wState	/* 1=on, 0=off */
	    );

	u16 HPI_OnOffSwitch_GetState(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 * wState	/* 1=on, 0=off */
	    );

/***************/
/* VOX control */
/***************/
	u16 HPI_VoxSetThreshold(HPI_HSUBSYS * phSubSysHandle,
				HPI_HCONTROL hControlHandle,
				short anGain0_01dB);

	u16 HPI_VoxGetThreshold(HPI_HSUBSYS * phSubSysHandle,
				HPI_HCONTROL hControlHandle,
				short *anGain0_01dB);

/*********************/
/* Bitstream control */
/*********************/
	u16 HPI_Bitstream_SetClockEdge(HPI_HSUBSYS * phSubSysHandle,
				       HPI_HCONTROL hControlHandle,
				       u16 wEdgeType);

	u16 HPI_Bitstream_SetDataPolarity(HPI_HSUBSYS * phSubSysHandle,
					  HPI_HCONTROL hControlHandle,
					  u16 wPolarity);

	u16 HPI_Bitstream_GetActivity(HPI_HSUBSYS * phSubSysHandle,
				      HPI_HCONTROL hControlHandle,
				      u16 * pwClkActivity,
				      u16 * pwDataActivity);

/***********************/
/* SampleClock control */
/***********************/
	u16 HPI_SampleClock_SetSource(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 wSource	/* HPI_SAMPLECLOCK_SOURCE_ADAPTER, _AESEBU etc */
	    );

	u16 HPI_SampleClock_GetSource(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 * pwSource	/* HPI_SAMPLECLOCK_SOURCE_ADAPTER, _AESEBU etc */
	    );

	u16 HPI_SampleClock_SetSourceIndex(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 wSourceIndex	// index of the source to use
	    );

	u16 HPI_SampleClock_GetSourceIndex(HPI_HSUBSYS * phSubSysHandle,
					   HPI_HCONTROL hControlHandle,
					   u16 * pwSourceIndex);

	u16 HPI_SampleClock_SetSampleRate(HPI_HSUBSYS * phSubSysHandle,
					  HPI_HCONTROL hControlHandle,
					  u32 dwSampleRate);

	u16 HPI_SampleClock_GetSampleRate(HPI_HSUBSYS * phSubSysHandle,
					  HPI_HCONTROL hControlHandle,
					  u32 * pdwSampleRate);

/***********************/
/* Microphone control */
/***********************/
	u16 HPI_Microphone_SetPhantomPower(HPI_HSUBSYS * phSubSysHandle,
					   HPI_HCONTROL hControlHandle,
					   u16 wOnOff);

	u16 HPI_Microphone_GetPhantomPower(HPI_HSUBSYS * phSubSysHandle,
					   HPI_HCONTROL hControlHandle,
					   u16 * pwOnOff);

/*******************************
Parametric Equalizer control
*******************************/
	u16 HPI_ParametricEQ_GetInfo(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u16 * pwNumberOfBands, u16 * pwEnabled	//!< OUT: enabled status
	    );

/*!
Turn a parametric equalizer on or off

/return HPI_ERROR_*
*/
	u16 HPI_ParametricEQ_SetState(HPI_HSUBSYS * phSubSysHandle,	//!< IN: HPI subsystem handle
				      HPI_HCONTROL hControlHandle,	//!< IN: Equalizer control handle
				      u16 wOnOff	//!< IN: 1=on, 0=off
	    );

/*! Set up one of the filters in a parametric equalizer
/return HPI_ERROR_*
*/
	u16 HPI_ParametricEQ_SetBand(HPI_HSUBSYS * phSubSysHandle,	//!< IN: HPI subsystem handle
				     HPI_HCONTROL hControlHandle,	//!< IN: Equalizer control handle
				     u16 wIndex,	//!< IN: index of band to set
				     u16 nType,	//!< IN: band type
				     u32 dwFrequencyHz,	//!< IN: band frequency
				     short nQ100,	//!< IN: filter Q * 100
				     short nGain0_01dB	//!< IN: filter gain in 100ths of a dB
	    );

/*! Get the settings of one of the filters in a parametric equalizer
/return HPI_ERROR_*
*/
	u16 HPI_ParametricEQ_GetBand(HPI_HSUBSYS * phSubSysHandle,	//!< IN: HPI subsystem handle
				     HPI_HCONTROL hControlHandle,	//!< IN: Equalizer control handle
				     u16 wIndex,	//!< IN: index of band to Get
				     u16 * pnType,	//!< OUT: band type
				     u32 * pdwFrequencyHz,	//!< OUT: band frequency
				     short *pnQ100,	//!< OUT: filter Q * 100
				     short *pnGain0_01dB	//!< OUT: filter gain in 100ths of a dB
	    );

/*******************************
Compressor Expander control
*******************************/

/*! Set up a compressor expander
*/
	u16 HPI_Compander_Set(HPI_HSUBSYS * phSubSysHandle,	//!< IN: HPI subsystem handle
			      HPI_HCONTROL hControlHandle,	//!< IN: Equalizer control handle
			      u16 wAttack,	//!< IN: attack time in milliseconds
			      u16 wDecay,	//!< IN: decay time in milliseconds
			      short wRatio100,	//!< IN: gain ratio * 100
			      short nThreshold0_01dB,	//!< IN: threshold in 100ths of a dB
			      short nMakeupGain0_01dB	//!< IN: makeup gain in 100ths of a dB
	    );

/*! Get the settings of a compressor expander
*/
	u16 HPI_Compander_Get(HPI_HSUBSYS * phSubSysHandle,	//!< IN: HPI subsystem handle
			      HPI_HCONTROL hControlHandle,	//!< IN: Equalizer control handle
			      u16 * pwAttack,	//!< OUT: attack time in milliseconds
			      u16 * pwDecay,	//!< OUT: decay time in milliseconds
			      short *pwRatio100,	//!< OUT: gain ratio * 100
			      short *pnThreshold0_01dB,	//!< OUT: threshold in 100ths of a dB
			      short *pnMakeupGain0_01dB	//!< OUT: makeup gain in 100ths of a dB
	    );

/*******************************
Cobranet HMI control
*******************************/

/*! Write data to a cobranet HMI variable
*/
	u16 HPI_Cobranet_HmiWrite(HPI_HSUBSYS * phSubSysHandle,
				  HPI_HCONTROL hControlHandle,
				  u32 dwHmiAddress,
				  u32 dwByteCount, u8 * pbData);

/*! Read data from acobranet HMI variable
*/
	u16 HPI_Cobranet_HmiRead(HPI_HSUBSYS * phSubSysHandle,
				 HPI_HCONTROL hControlHandle,
				 u32 dwHmiAddress,
				 u32 dwMaxByteCount,
				 u32 * pdwByteCount, u8 * pbData);

/*! Read the raw cobranet HMI status
*/
	u16 HPI_Cobranet_HmiGetStatus(HPI_HSUBSYS * phSubSysHandle,
				      HPI_HCONTROL hControlHandle,
				      u32 * pdwStatus,
				      u32 * pdwReadableSize,
				      u32 * pdwWriteableSize);

/*! Set the CobraNet mode. Used for switching tethered mode on and off.
*/
	u16 HPI_Cobranet_SetMode(HPI_HSUBSYS * phSubSysHandle, HPI_HCONTROL hControlHandle, u32 dwMode, u32 dwSetOrQuery	/* either, HPI_COBRANET_MODE_QUERY or HPI_COBRANET_MODE_SET */
	    );

/*! Get the CobraNet mode.
*/
	u16 HPI_Cobranet_GetMode(HPI_HSUBSYS * phSubSysHandle,
				 HPI_HCONTROL hControlHandle, u32 * pdwMode);

/*! Read the IP address
*/
	u16 HPI_Cobranet_GetIPaddress(HPI_HSUBSYS * phSubSysHandle,
				      HPI_HCONTROL hControlHandle,
				      u32 * pdwIPaddress);
/*! Read the MAC address
*/
	u16 HPI_Cobranet_GetMACaddress(HPI_HSUBSYS * phSubSysHandle,
				       HPI_HCONTROL hControlHandle,
				       u32 * pdwMAC_MSBs, u32 * pdwMAC_LSBs);

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
			       u16 wIndex, u8 * szName, u16 nNameLength);

	u16 HPI_ProfileGetUtilization(HPI_HSUBSYS * phSubSys,
				      HPI_HPROFILE hProfile,
				      u32 * pdwUtilization);

/* /////////////////////// */
/* AES18 functions */

/* !!! Test code only !!! */
	void HPI_Aes18Init(void);

/* Block Generator */
	u16 HPI_AES18BGSetConfiguration(HPI_HSUBSYS * phSubSys,
					HPI_HCONTROL hControlHandle,
					u16
					wBlocksPerSec[HPI_AES18_MAX_CHANNELS],
					u16
					wPriorityEnableMask
					[HPI_AES18_MAX_CHANNELS],
					u16
					wOperatingMode[HPI_AES18_MAX_CHANNELS],
					u16 wChannelMode);

/* Transmitter */
	u16 HPI_AES18TxGetInternalBufferState(HPI_HSUBSYS * phSubSys,
					      HPI_HCONTROL hControlHandle,
					      u16
					      awInternalBufferBusy
					      [HPI_AES18_MAX_CHANNELS]
					      [HPI_AES18_MAX_PRIORITIES]
	    );

	u16 HPI_AES18TxGetInternalBufferSize(HPI_HSUBSYS * phSubSys,
					     HPI_HCONTROL hControlHandle,
					     u16
					     awBytesPerBuffer
					     [HPI_AES18_MAX_PRIORITIES]
	    );

	u16 HPI_AES18TxSendMessage(HPI_HSUBSYS * phSubSys,
				   HPI_HCONTROL hControlHandle,
				   u16 wChannel,
				   u32 dwpbMessage,
				   u16 wMessageLength,
				   u16 wDestinationAddress,
				   u16 wPriorityIndex, u16 wRepetitionIndex);

/* Receiver */
	u16 HPI_AES18RxSetAddress(HPI_HSUBSYS * phSubSys,
				  HPI_HCONTROL hControlHandle,
				  u16 awDecoderAddress[HPI_AES18_MAX_CHANNELS]
	    );

	u16 HPI_AES18RxGetInternalBufferSize(HPI_HSUBSYS * phSubSys,
					     HPI_HCONTROL hControlHandle,
					     u16
					     awBytesPerBuffer
					     [HPI_AES18_MAX_PRIORITIES]
	    );

	u16 HPI_AES18RxGetInternalBufferState(HPI_HSUBSYS * phSubSys,
					      HPI_HCONTROL hControlHandle,
					      u16
					      awFrameError
					      [HPI_AES18_MAX_CHANNELS],
					      u16
					      awMessageWaiting
					      [HPI_AES18_MAX_CHANNELS]
					      [HPI_AES18_MAX_PRIORITIES],
					      u16
					      awInternalBufferOverFlow
					      [HPI_AES18_MAX_CHANNELS]
					      [HPI_AES18_MAX_PRIORITIES],
					      u16
					      awMissedMessage
					      [HPI_AES18_MAX_CHANNELS]
					      [HPI_AES18_MAX_PRIORITIES]
	    );

	u16 HPI_AES18RxGetMessage(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wChannel, u16 wPriority, u16 wQueueSize,	/* in bytes */
				  u32 dwpbMessage,	/* Actually a pointer to bytes */
				  u16 * pwMessageLength	/* in bytes */
	    );

/*//////////////////// */
/* UTILITY functions */

/* obsolete - use HPI_GetLastErrorDetail */
	void HPI_GetErrorText(u16 wError, char *pszErrorText);

	void HPI_GetLastErrorDetail(u16 wError,
				    char *pszErrorText, u32 ** padwError);

	u16 HPI_FormatCreate(HPI_FORMAT * pFormat,
			     u16 wChannels,
			     u16 wFormat,
			     u32 dwSampleRate, u32 dwBitRate, u32 dwAttributes);

	u16 HPI_DataCreate(HPI_DATA * pData,
			   HPI_FORMAT * pFormat, u8 * pbData, u32 dwDataSize);

	u16 HPI_ResourceCreateIsaPnp(HPI_RESOURCE * pResource,
				     u16 wIsaPnpVendorId,
				     u16 wIsaPnpDeviceId,
				     u16 wIoPortBase, u16 wInterrupt);

/* Until it's verified, this function is for Windows OSs only */
#if defined ( HPI_OS_WIN16 ) || defined ( HPI_OS_WIN32_USER ) || defined ( INCLUDE_WINDOWS_ON_LINUX )

/*?EWB these functions dont belong here?
put them in hpihelper.c/h
*/

#if defined ( INCLUDE_WINDOWS_ON_LINUX )
#include <windows.h>
#include <mmsystem.h>
#endif

#include <mmreg.h>

	u16 HPI_WaveFormatToHpiFormat(const WAVEFORMATEX * lpFormatEx,
				      HPI_FORMAT * pHpiFormat);

	u16 HPI_HpiFormatToWaveFormat(const HPI_FORMAT * pHpiFormat,
				      WAVEFORMATEX * lpFormatEx);

#endif				/* defined(HPI_OS_WIN16) || defined(HPI_OS_WIN32_USER) */

/******************************************************************************/
/******************************************************************************/
/********                     HPI LOW LEVEL MESSAGES                  *********/
/******************************************************************************/
/******************************************************************************/

#ifndef HPI_EXCLUDE_IMPLEMENTATION
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

/* SUB-SYSTEM */
#define HPI_SUBSYS_OPEN                 HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM,1)
#define HPI_SUBSYS_GET_VERSION          HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM,2)
#define HPI_SUBSYS_GET_INFO             HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM,3)
#define HPI_SUBSYS_FIND_ADAPTERS        HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM,4)	/* SGT feb-3-97 */
#define HPI_SUBSYS_CREATE_ADAPTER       HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM,5)	/* SGT feb-3-97 - not used any more */
#define HPI_SUBSYS_CLOSE                HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM,6)
#define HPI_SUBSYS_DELETE_ADAPTER       HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM,7)
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
#define HPI_ADAPTER_QUERY_FLASH         HPI_MAKE_INDEX(HPI_OBJ_ADAPTER,11)
#define HPI_ADAPTER_START_FLASH         HPI_MAKE_INDEX(HPI_OBJ_ADAPTER,12)
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
#define HPI_MIXER_GET_CONTROL_MULTIPLE_VALUES  HPI_MAKE_INDEX(HPI_OBJ_MIXER,10)	/* get an arrya of control values (internal call for the moment) */
#define HPI_MIXER_FUNCTION_COUNT        10
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
	    typedef struct {
/*SGT-for port r/w*/
/*u32    dwAddress;*/
/*u32    dwData;*/
		HPI_RESOURCE Resource;
	} HPI_SUBSYS_MSG;

	typedef struct {
		u32 dwVersion;
/*SGT-for port r/w*/
		u32 dwData;
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
			HPI_DATA Data;
			u16 wVelocity;
			HPI_PUNCHINOUT Pio;
			u32 dwTimeScale;
		} u;
		u16 wOStreamIndex;
		u16 wIStreamIndex;
	} HPI_STREAM_MSG;

	typedef struct {
		u32 dwBufferSize;	/* size of hardware buffer */
		u32 dwDataAvailable;	/* OutStream - data to play, InStream - data recorded */
		u32 dwSamplesTransfered;	/* OutStream - samples played, InStream - samples recorded */
		u16 wState;	/* HPI_STATE_PLAYING, _STATE_STOPPED */
		u16 wOStreamIndex;
		u16 wIStreamIndex;
		u16 wPadding;
		u32 dwAuxilaryDataAvailable;	/* Adapter - OutStream - data to play, InStream - data recorded */
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
	typedef struct {
		u16 wBlocksPerSec[HPI_AES18_MAX_CHANNELS];
		u16 wPriorityMask[HPI_AES18_MAX_CHANNELS];
		u16 wOperatingMode[HPI_AES18_MAX_CHANNELS];
		u16 wChannelMode;
		u16 wPadding;
	} HPI_CONTROLX_MSG_AES18BG;

	typedef struct {
		u16 wAddress[HPI_AES18_MAX_CHANNELS];
	} HPI_CONTROLX_MSG_AES18RX_ADDRESS;

	typedef struct {
		u32 dwpbMessage;
		u16 wChannel;
		u16 wMessageLength;
		u16 wDestinationAddress;
		u16 wPriorityIndex;
		u16 wRepetitionIndex;
		u16 wPadding;
	} HPI_CONTROLX_MSG_AES18TX_SEND_MESSAGE;

	typedef struct {
		u32 dwpbMessage;	/* actually a pointer to byte */
		u16 wChannel;
		u16 wPriority;
		u16 wQueueSize;
		u16 wPadding;
	} HPI_CONTROLX_MSG_AES18RX_GET_MESSAGE;

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
		u32 dwpbData;
	} HPI_CONTROLX_MSG_COBRANET_BIGDATA;

/** Used for generic data
*/
	typedef struct {
		u32 dwParam1;
		u32 dwParam2;
	} HPI_CONTROLX_MSG_GENERIC;

	typedef struct {
		union {
			HPI_CONTROLX_MSG_AES18BG aes18bg;
			HPI_CONTROLX_MSG_AES18RX_ADDRESS aes18rx_address;
			HPI_CONTROLX_MSG_AES18TX_SEND_MESSAGE
			    aes18tx_send_message;
			HPI_CONTROLX_MSG_AES18RX_GET_MESSAGE
			    aes18rx_get_message;
			HPI_CONTROLX_MSG_COBRANET_DATA cobranet_data;
			HPI_CONTROLX_MSG_COBRANET_BIGDATA cobranet_bigdata;
			HPI_CONTROLX_MSG_GENERIC generic;
/* nothing extra to send for status read */
		} u;
		u16 wControlIndex;
		u16 wAttribute;	/* control attribute or property */
	} HPI_CONTROLX_MSG;

/* Response */

	typedef struct {
		u16 awFrameErrorPacked;
		u16 awMessageWaitingPacked;
		u16 awInternalBufferOverFlowPacked;
		u16 awMissedMessagePacked;
	} HPI_CONTROLX_RES_AES18RX_BUFFER_STATE;

	typedef struct {
		u16 awBytesPerBuffer[HPI_AES18_MAX_PRIORITIES];
	} HPI_CONTROLX_RES_AES18RX_BUFFER_SIZE;

	typedef struct {
		u16 wReturnedMessageSize;
	} HPI_CONTROLX_RES_AES18RX_GET_MESSAGE;

	typedef struct {
		u16 wInternalBufferBusyPacked;
	} HPI_CONTROLX_RES_AES18TX_BUFFER_STATE;

	typedef struct {
		u16 awBytesPerBuffer[HPI_AES18_MAX_PRIORITIES];
	} HPI_CONTROLX_RES_AES18TX_BUFFER_SIZE;

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
			HPI_CONTROLX_RES_AES18RX_BUFFER_STATE
			    aes18rx_internal_buffer_state;
			HPI_CONTROLX_RES_AES18RX_BUFFER_SIZE
			    aes18rx_internal_buffer_size;
			HPI_CONTROLX_RES_AES18RX_GET_MESSAGE
			    aes18rx_get_message;
			HPI_CONTROLX_RES_AES18TX_BUFFER_STATE
			    aes18tx_internal_buffer_state;
			HPI_CONTROLX_RES_AES18TX_BUFFER_SIZE
			    aes18tx_internal_buffer_size;
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
#define HPI_MESSAGE_FIXED_SIZE (6 * sizeof(u16))

	typedef struct {
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

/* Test message size */

/* the size of the part of the response outside the union.  MUST update if more elements are added */
#define HPI_RESPONSE_FIXED_SIZE (6 * sizeof(u16))
	typedef struct {
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

/* Test response size */

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
		u16 anLog[2];
	} tHPIControlCachedMode;
	typedef struct {
		u32 dwErrorStatus;
		u32 dwSource;
	} tHPIControlCachedAES3RxStatus;
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
		u32 dw1;
		u32 dw2;
	} tHPIControlCachedGeneric;

	typedef struct {
		u32 ControlType;
		union {
			tHPIControlCachedVolume v;
			tHPIControlCachedPeak p;
			tHPIControlCachedMode m;
			tHPIControlCachedMultiplexer x;
			tHPIControlCachedLevel l;
			tHPIControlCachedTuner t;
			tHPIControlCachedAES3RxStatus aes3rx;
			tHPIControlCachedGeneric g;
		} u;
	} tHPIControlCacheSingle;

	typedef struct {
		u32 dwControlIndex;
		tHPIControlCacheSingle c;
	} tHPIControlCacheValue;

#endif				// ifndef HPI_EXCLUDE_IMPLEMENTATION

/* skip host side function declarations for DSP compile and documentation extraction */

	void HPI_InitMessage(HPI_MESSAGE * phm, u16 wObject, u16 wFunction);
	void HPI_InitResponse(HPI_RESPONSE * phr, u16 wObject, u16 wFunction,
			      u16 wError);

/*////////////////////////////////////////////////////////////////////////// */
/* HPI IOCTL definitions */
#define HPI_IOCTL_WINNT      CTL_CODE(50000,0xA00,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define HPI_IOCTL_WIN95      0x101

/*////////////////////////////////////////////////////////////////////////// */
/* main HPI entry point */
	u16 HPI_DriverOpen(HPI_HSUBSYS * phSubSys);
	void HPI_Message(const HPI_HSUBSYS * phSubSys, HPI_MESSAGE * phm,
			 HPI_RESPONSE * phr);
	void HPI_DriverClose(HPI_HSUBSYS * phSubSys);

/*////////////////////////////////////////////////////////////////////////// */
/* declarations for individual HPI entry points */
	void HPI_4000(HPI_MESSAGE * phm, HPI_RESPONSE * phr);	/* ASI450X and ASI400 MPEG Play/Record */
	void HPI_4500(HPI_MESSAGE * phm, HPI_RESPONSE * phr);	/* ASI450X MPEG Play/Record */
	void HPI_8400(HPI_MESSAGE * phm, HPI_RESPONSE * phr);	/* ASI840X OEM */
	void HPI_8411(HPI_MESSAGE * phm, HPI_RESPONSE * phr);	/* ASI8411 OEM */
	void HPI_6000(HPI_MESSAGE * phm, HPI_RESPONSE * phr);	/* ASI6xxx and ASI8801 */
	void HPI_Usb(HPI_MESSAGE * phm, HPI_RESPONSE * phr);
	void HPI_6205(HPI_MESSAGE * phm, HPI_RESPONSE * phr);	/* ASI5000, TI C6205 based */

/* added so that some Win32 programs can restore 4byte packing if they desire */
#ifdef HPI_RESTORE_PACK4
#pragma pack(4)
#endif

/* added so that some Win32 programs can restore 8byte packing if they desire */
#ifdef HPI_RESTORE_PACK8
#pragma pack(8)
#endif

#ifdef __cplusplus
}
#endif
#endif	 /*_H_HPI_ */
/*
///////////////////////////////////////////////////////////////////////////////
// See CVS for history.  Last complete set in rev 1.146
////////////////////////////////////////////////////////////////////////////////
*/
