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

HPI internal definitions

(C) Copyright AudioScience Inc. 1996-2008
******************************************************************************/

#ifndef _HPI_INTERNAL_H_
#define _HPI_INTERNAL_H_

#include "hpi.h"

/** maximum number of memory regions mapped to an adapter */
#define HPI_MAX_ADAPTER_MEM_SPACES (2)

#include "hpios.h"

#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/******************************************* CONTROL ATTRIBUTES ****/
/* (in order of control type ID */

	/* This allows for 255 control types, 256 unique attributes each */
#define HPI_CTL_ATTR(ctl, ai) (HPI_CONTROL_##ctl * 0x100 + ai)

/* Get the sub-index of the attribute for a control type */
#define HPI_CTL_ATTR_INDEX(i) (i&0xff)

/* Original 0-based non-unique attributes, might become unique later */
#define HPI_CTL_ATTR0(ctl, ai) (ai)

/* Generic control attributes.  If a control uses any of these attributes
   its other attributes must also be defined using HPI_CTL_ATTR()
*/

/** Enable a control.
0=disable, 1=enable
\note generic to all mixer plugins?
*/
#define HPI_GENERIC_ENABLE HPI_CTL_ATTR(GENERIC, 1)

/** Enable event generation for a control.
0=disable, 1=enable
\note generic to all controls that can generate events
*/
#define HPI_GENERIC_EVENT_ENABLE HPI_CTL_ATTR(GENERIC, 2)

/* Volume Control attributes */
#define HPI_VOLUME_GAIN                 HPI_CTL_ATTR0(VOLUME, 1)
#define HPI_VOLUME_AUTOFADE             HPI_CTL_ATTR0(VOLUME, 2)

/** For HPI_ControlQuery() to get the number of channels of a volume control*/
#define HPI_VOLUME_NUM_CHANNELS         HPI_CTL_ATTR0(VOLUME, 6)
#define HPI_VOLUME_RANGE                HPI_CTL_ATTR0(VOLUME, 10)

/** Level Control attributes */
#define HPI_LEVEL_GAIN                  HPI_CTL_ATTR0(LEVEL, 1)
#define HPI_LEVEL_RANGE                 HPI_CTL_ATTR0(LEVEL, 10)

/* Meter Control attributes */
/** return RMS signal level */
#define HPI_METER_RMS                   HPI_CTL_ATTR0(METER, 1)
/** return peak signal level */
#define HPI_METER_PEAK                  HPI_CTL_ATTR0(METER, 2)
/** ballistics for ALL rms meters on adapter */
#define HPI_METER_RMS_BALLISTICS        HPI_CTL_ATTR0(METER, 3)
/** ballistics for ALL peak meters on adapter */
#define HPI_METER_PEAK_BALLISTICS       HPI_CTL_ATTR0(METER, 4)

/** For HPI_ControlQuery() to get the number of channels of a meter control*/
#define HPI_METER_NUM_CHANNELS          HPI_CTL_ATTR0(METER, 5)

/* Multiplexer control attributes */
#define HPI_MULTIPLEXER_SOURCE          HPI_CTL_ATTR0(MULTIPLEXER, 1)
#define HPI_MULTIPLEXER_QUERYSOURCE     HPI_CTL_ATTR0(MULTIPLEXER, 2)

/** AES/EBU transmitter control attributes */
/** AESEBU or SPDIF */
#define HPI_AESEBUTX_FORMAT             HPI_CTL_ATTR0(AESEBUTX, 1)
#define HPI_AESEBUTX_SAMPLERATE         HPI_CTL_ATTR0(AESEBUTX, 3)
#define HPI_AESEBUTX_CHANNELSTATUS      HPI_CTL_ATTR0(AESEBUTX, 4)
#define HPI_AESEBUTX_USERDATA           HPI_CTL_ATTR0(AESEBUTX, 5)

/** AES/EBU receiver control attributes */
#define HPI_AESEBURX_FORMAT             HPI_CTL_ATTR0(AESEBURX, 1)
#define HPI_AESEBURX_ERRORSTATUS        HPI_CTL_ATTR0(AESEBURX, 2)
#define HPI_AESEBURX_SAMPLERATE         HPI_CTL_ATTR0(AESEBURX, 3)
#define HPI_AESEBURX_CHANNELSTATUS      HPI_CTL_ATTR0(AESEBURX, 4)
#define HPI_AESEBURX_USERDATA           HPI_CTL_ATTR0(AESEBURX, 5)

/** \defgroup tuner_defs Tuners
\{
*/
/** \defgroup tuner_attrs Tuner control attributes
\{
*/
#define HPI_TUNER_BAND                  HPI_CTL_ATTR0(TUNER, 1)
#define HPI_TUNER_FREQ                  HPI_CTL_ATTR0(TUNER, 2)
#define HPI_TUNER_LEVEL                 HPI_CTL_ATTR0(TUNER, 3)
#define HPI_TUNER_AUDIOMUTE             HPI_CTL_ATTR0(TUNER, 4)
/* use TUNER_STATUS instead */
#define HPI_TUNER_VIDEO_STATUS          HPI_CTL_ATTR0(TUNER, 5)
#define HPI_TUNER_GAIN                  HPI_CTL_ATTR0(TUNER, 6)
#define HPI_TUNER_STATUS                HPI_CTL_ATTR0(TUNER, 7)
#define HPI_TUNER_MODE                  HPI_CTL_ATTR0(TUNER, 8)
/** RDS data. */
#define HPI_TUNER_RDS                   HPI_CTL_ATTR0(TUNER, 9)
/** Audio pre-emphasis. */
#define HPI_TUNER_DEEMPHASIS            HPI_CTL_ATTR(TUNER, 10)
/** HD Radio tuner program control. */
#define HPI_TUNER_PROGRAM               HPI_CTL_ATTR(TUNER, 11)
/** HD Radio tuner digital signal quality. */
#define HPI_TUNER_HDRADIO_SIGNAL_QUALITY        HPI_CTL_ATTR(TUNER, 12)
/** HD Radio SDK firmware version. */
#define HPI_TUNER_HDRADIO_SDK_VERSION   HPI_CTL_ATTR(TUNER, 13)
/** HD Radio DSP firmware version. */
#define HPI_TUNER_HDRADIO_DSP_VERSION   HPI_CTL_ATTR(TUNER, 14)

/** \} */

/** \defgroup pads_attrs Tuner PADs control attributes
\{
*/
/** The text string containing the station/channel combination. */
#define HPI_PAD_CHANNEL_NAME            HPI_CTL_ATTR(PAD, 1)
/** The text string containing the artist. */
#define HPI_PAD_ARTIST                  HPI_CTL_ATTR(PAD, 2)
/** The text string containing the title. */
#define HPI_PAD_TITLE                   HPI_CTL_ATTR(PAD, 3)
/** The text string containing the comment. */
#define HPI_PAD_COMMENT                 HPI_CTL_ATTR(PAD, 4)
/** The integer containing the PTY code. */
#define HPI_PAD_PROGRAM_TYPE            HPI_CTL_ATTR(PAD, 5)
/** The integer containing the program identification. */
#define HPI_PAD_PROGRAM_ID              HPI_CTL_ATTR(PAD, 6)
/** The integer containing whether traffic information is supported.
Contains either 1 or 0. */
#define HPI_PAD_TA_SUPPORT              HPI_CTL_ATTR(PAD, 7)
/** The integer containing whether traffic announcement is in progress.
Contains either 1 or 0. */
#define HPI_PAD_TA_ACTIVE               HPI_CTL_ATTR(PAD, 8)
/** \} */

/* VOX control attributes */
#define HPI_VOX_THRESHOLD               HPI_CTL_ATTR0(VOX, 1)

/*?? channel mode used hpi_multiplexer_source attribute == 1 */
#define HPI_CHANNEL_MODE_MODE HPI_CTL_ATTR0(CHANNEL_MODE, 1)

/** \defgroup channel_modes Channel Modes
Used for HPI_ChannelModeSet/Get()
\{
*/
/** Left channel out = left channel in, Right channel out = right channel in. */
#define HPI_CHANNEL_MODE_NORMAL                 1
/** Left channel out = right channel in, Right channel out = left channel in. */
#define HPI_CHANNEL_MODE_SWAP                   2
/** Left channel out = left channel in, Right channel out = left channel in. */
#define HPI_CHANNEL_MODE_LEFT_TO_STEREO         3
/** Left channel out = right channel in, Right channel out = right channel in.*/
#define HPI_CHANNEL_MODE_RIGHT_TO_STEREO        4
/** Left channel out = (left channel in + right channel in)/2,
    Right channel out = mute. */
#define HPI_CHANNEL_MODE_STEREO_TO_LEFT         5
/** Left channel out = mute,
    Right channel out = (right channel in + left channel in)/2. */
#define HPI_CHANNEL_MODE_STEREO_TO_RIGHT        6
#define HPI_CHANNEL_MODE_LAST                   6
/** \} */

/* Bitstream control set attributes */
#define HPI_BITSTREAM_DATA_POLARITY     HPI_CTL_ATTR0(BITSTREAM, 1)
#define HPI_BITSTREAM_CLOCK_EDGE        HPI_CTL_ATTR0(BITSTREAM, 2)
#define HPI_BITSTREAM_CLOCK_SOURCE      HPI_CTL_ATTR0(BITSTREAM, 3)

#define HPI_POLARITY_POSITIVE           0
#define HPI_POLARITY_NEGATIVE           1

/* Bitstream control get attributes */
#define HPI_BITSTREAM_ACTIVITY          1

/* SampleClock control attributes */
#define HPI_SAMPLECLOCK_SOURCE                  HPI_CTL_ATTR0(SAMPLECLOCK, 1)
#define HPI_SAMPLECLOCK_SAMPLERATE              HPI_CTL_ATTR0(SAMPLECLOCK, 2)
#define HPI_SAMPLECLOCK_SOURCE_INDEX            HPI_CTL_ATTR0(SAMPLECLOCK, 3)
#define HPI_SAMPLECLOCK_LOCAL_SAMPLERATE        HPI_CTL_ATTR0(SAMPLECLOCK, 4)
#define HPI_SAMPLECLOCK_AUTO                    HPI_CTL_ATTR0(SAMPLECLOCK, 5)

/* Microphone control attributes */
#define HPI_MICROPHONE_PHANTOM_POWER HPI_CTL_ATTR0(MICROPHONE, 1)

/** Equalizer control attributes
*/
/** Used to get number of filters in an EQ. (Can't set) */
#define HPI_EQUALIZER_NUM_FILTERS HPI_CTL_ATTR0(EQUALIZER, 1)
/** Set/get the filter by type, freq, Q, gain */
#define HPI_EQUALIZER_FILTER HPI_CTL_ATTR0(EQUALIZER, 2)
/** Get the biquad coefficients */
#define HPI_EQUALIZER_COEFFICIENTS HPI_CTL_ATTR0(EQUALIZER, 3)

#define HPI_COMPANDER_PARAMS HPI_CTL_ATTR(COMPANDER, 1)

/* Cobranet control attributes.
   MUST be distinct from all other control attributes.
   This is so that host side processing can easily identify a Cobranet control
   and apply additional host side operations (like copying data) as required.
*/
#define HPI_COBRANET_SET         HPI_CTL_ATTR(COBRANET, 1)
#define HPI_COBRANET_GET         HPI_CTL_ATTR(COBRANET, 2)
#define HPI_COBRANET_SET_DATA    HPI_CTL_ATTR(COBRANET, 3)
#define HPI_COBRANET_GET_DATA    HPI_CTL_ATTR(COBRANET, 4)
#define HPI_COBRANET_GET_STATUS  HPI_CTL_ATTR(COBRANET, 5)
#define HPI_COBRANET_SEND_PACKET HPI_CTL_ATTR(COBRANET, 6)
#define HPI_COBRANET_GET_PACKET  HPI_CTL_ATTR(COBRANET, 7)

/*------------------------------------------------------------
 Cobranet Chip Bridge - copied from HMI.H
------------------------------------------------------------*/
#define  HPI_COBRANET_HMI_cobraBridge           0x20000
#define  HPI_COBRANET_HMI_cobraBridgeTxPktBuf \
	(HPI_COBRANET_HMI_cobraBridge + 0x1000)
#define  HPI_COBRANET_HMI_cobraBridgeRxPktBuf \
	(HPI_COBRANET_HMI_cobraBridge + 0x2000)
#define  HPI_COBRANET_HMI_cobraIfTable1         0x110000
#define  HPI_COBRANET_HMI_cobraIfPhyAddress \
	(HPI_COBRANET_HMI_cobraIfTable1 + 0xd)
#define  HPI_COBRANET_HMI_cobraProtocolIP       0x72000
#define  HPI_COBRANET_HMI_cobraIpMonCurrentIP \
	(HPI_COBRANET_HMI_cobraProtocolIP + 0x0)
#define  HPI_COBRANET_HMI_cobraIpMonStaticIP \
	(HPI_COBRANET_HMI_cobraProtocolIP + 0x2)
#define  HPI_COBRANET_HMI_cobraSys              0x100000
#define  HPI_COBRANET_HMI_cobraSysDesc \
		(HPI_COBRANET_HMI_cobraSys + 0x0)
#define  HPI_COBRANET_HMI_cobraSysObjectID \
	(HPI_COBRANET_HMI_cobraSys + 0x100)
#define  HPI_COBRANET_HMI_cobraSysContact \
	(HPI_COBRANET_HMI_cobraSys + 0x200)
#define  HPI_COBRANET_HMI_cobraSysName \
		(HPI_COBRANET_HMI_cobraSys + 0x300)
#define  HPI_COBRANET_HMI_cobraSysLocation \
	(HPI_COBRANET_HMI_cobraSys + 0x400)

/*------------------------------------------------------------
 Cobranet Chip Status bits
------------------------------------------------------------*/
#define HPI_COBRANET_HMI_STATUS_RXPACKET 2
#define HPI_COBRANET_HMI_STATUS_TXPACKET 3

/*------------------------------------------------------------
 Ethernet header size
------------------------------------------------------------*/
#define HPI_ETHERNET_HEADER_SIZE (16)

/* These defines are used to fill in protocol information for an Ethernet packet
    sent using HMI on CS18102 */
/** ID supplied by Cirrius for ASI packets. */
#define HPI_ETHERNET_PACKET_ID                  0x85
/** Simple packet - no special routing required */
#define HPI_ETHERNET_PACKET_V1                  0x01
/** This packet must make its way to the host across the HPI interface */
#define HPI_ETHERNET_PACKET_HOSTED_VIA_HMI      0x20
/** This packet must make its way to the host across the HPI interface */
#define HPI_ETHERNET_PACKET_HOSTED_VIA_HMI_V1   0x21
/** This packet must make its way to the host across the HPI interface */
#define HPI_ETHERNET_PACKET_HOSTED_VIA_HPI      0x40
/** This packet must make its way to the host across the HPI interface */
#define HPI_ETHERNET_PACKET_HOSTED_VIA_HPI_V1   0x41

#define HPI_ETHERNET_UDP_PORT (44600)	/*!< UDP messaging port */

/** Base network time out is set to 100 milli-seconds. */
#define HPI_ETHERNET_TIMEOUT_MS      (100)

/** \defgroup tonedet_attr Tonedetector attributes
\{
Used by HPI_ToneDetector_Set() and HPI_ToneDetector_Get()
*/

/** Set the threshold level of a tonedetector,
Threshold is a -ve number in units of dB/100,
*/
#define HPI_TONEDETECTOR_THRESHOLD HPI_CTL_ATTR(TONEDETECTOR, 1)

/** Get the current state of tonedetection
The result is a bitmap of detected tones.  pairs of bits represent the left
and right channels, with left channel in LSB.
The lowest frequency detector state is in the LSB
*/
#define HPI_TONEDETECTOR_STATE HPI_CTL_ATTR(TONEDETECTOR, 2)

/** Get the frequency of a tonedetector band.
*/
#define HPI_TONEDETECTOR_FREQUENCY HPI_CTL_ATTR(TONEDETECTOR, 3)

/**\}*/

/** \defgroup silencedet_attr SilenceDetector attributes
\{
*/

/** Get the current state of tonedetection
The result is a bitmap with 1s for silent channels. Left channel is in LSB
*/
#define HPI_SILENCEDETECTOR_STATE \
  HPI_CTL_ATTR(SILENCEDETECTOR, 2)

/** Set the threshold level of a SilenceDetector,
Threshold is a -ve number in units of dB/100,
*/
#define HPI_SILENCEDETECTOR_THRESHOLD \
  HPI_CTL_ATTR(SILENCEDETECTOR, 1)

/** get/set the silence time before the detector triggers
*/
#define HPI_SILENCEDETECTOR_DELAY \
       HPI_CTL_ATTR(SILENCEDETECTOR, 3)

/**\}*/

/* Locked memory buffer alloc/free phases */
/** use one message to allocate or free physical memory */
#define HPI_BUFFER_CMD_EXTERNAL                 0
/** alloc physical memory */
#define HPI_BUFFER_CMD_INTERNAL_ALLOC           1
/** send physical memory address to adapter */
#define HPI_BUFFER_CMD_INTERNAL_GRANTADAPTER    2
/** notify adapter to stop using physical buffer */
#define HPI_BUFFER_CMD_INTERNAL_REVOKEADAPTER   3
/** free physical buffer */
#define HPI_BUFFER_CMD_INTERNAL_FREE            4

/******************************************* CONTROLX ATTRIBUTES ****/
/* NOTE: All controlx attributes must be unique, unlike control attributes */

/*****************************************************************************/
/*****************************************************************************/
/********               HPI LOW LEVEL MESSAGES                  *******/
/*****************************************************************************/
/*****************************************************************************/
/** Pnp ids */
/** "ASI"  - actual is "ASX" - need to change */
#define HPI_ID_ISAPNP_AUDIOSCIENCE      0x0669
/** PCI vendor ID that AudioScience uses */
#define HPI_PCI_VENDOR_ID_AUDIOSCIENCE  0x175C
/** PCI vendor ID that the DSP56301 has */
#define HPI_PCI_VENDOR_ID_MOTOROLA      0x1057
/** PCI vendor ID that TI uses */
#define HPI_PCI_VENDOR_ID_TI            0x104C

#define HPI_USB_VENDOR_ID_AUDIOSCIENCE  0x1257
#define HPI_USB_W2K_TAG                 0x57495341	/* "ASIW"       */
#define HPI_USB_LINUX_TAG               0x4C495341	/* "ASIL"       */

/******************************************* message types */
#define HPI_TYPE_MESSAGE                        1
#define HPI_TYPE_RESPONSE                       2
#define HPI_TYPE_DATA                           3
#define HPI_TYPE_SSX2BYPASS_MESSAGE             4

/******************************************* object types */
#define HPI_OBJ_SUBSYSTEM                       1
#define HPI_OBJ_ADAPTER                         2
#define HPI_OBJ_OSTREAM                         3
#define HPI_OBJ_ISTREAM                         4
#define HPI_OBJ_MIXER                           5
#define HPI_OBJ_NODE                            6
#define HPI_OBJ_CONTROL                         7
#define HPI_OBJ_NVMEMORY                        8
#define HPI_OBJ_GPIO                            9
#define HPI_OBJ_WATCHDOG                        10
#define HPI_OBJ_CLOCK                           11
#define HPI_OBJ_PROFILE                         12
#define HPI_OBJ_CONTROLEX                       13
#define HPI_OBJ_ASYNCEVENT                      14

#define HPI_OBJ_MAXINDEX                        14

/******************************************* methods/functions */

#define HPI_OBJ_FUNCTION_SPACING        0x100
#define HPI_MAKE_INDEX(obj, index) (obj * HPI_OBJ_FUNCTION_SPACING + index)
#define HPI_EXTRACT_INDEX(fn) (fn & 0xff)

/* SUB-SYSTEM */
#define HPI_SUBSYS_OPEN                 HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM, 1)
#define HPI_SUBSYS_GET_VERSION          HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM, 2)
#define HPI_SUBSYS_GET_INFO             HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM, 3)
#define HPI_SUBSYS_FIND_ADAPTERS        HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM, 4)
#define HPI_SUBSYS_CREATE_ADAPTER       HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM, 5)
#define HPI_SUBSYS_CLOSE                HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM, 6)
#define HPI_SUBSYS_DELETE_ADAPTER       HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM, 7)
#define HPI_SUBSYS_DRIVER_LOAD          HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM, 8)
#define HPI_SUBSYS_DRIVER_UNLOAD        HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM, 9)
 /*SGT*/
#define HPI_SUBSYS_READ_PORT_8          HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM, 10)
#define HPI_SUBSYS_WRITE_PORT_8         HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM, 11)
#define HPI_SUBSYS_GET_NUM_ADAPTERS     HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM, 12)
#define HPI_SUBSYS_GET_ADAPTER          HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM, 13)
#define HPI_SUBSYS_SET_NETWORK_INTERFACE HPI_MAKE_INDEX(HPI_OBJ_SUBSYSTEM, 14)
/* ADAPTER */
#define HPI_ADAPTER_OPEN                HPI_MAKE_INDEX(HPI_OBJ_ADAPTER, 1)
#define HPI_ADAPTER_CLOSE               HPI_MAKE_INDEX(HPI_OBJ_ADAPTER, 2)
#define HPI_ADAPTER_GET_INFO            HPI_MAKE_INDEX(HPI_OBJ_ADAPTER, 3)
#define HPI_ADAPTER_GET_ASSERT          HPI_MAKE_INDEX(HPI_OBJ_ADAPTER, 4)
#define HPI_ADAPTER_TEST_ASSERT         HPI_MAKE_INDEX(HPI_OBJ_ADAPTER, 5)
#define HPI_ADAPTER_SET_MODE            HPI_MAKE_INDEX(HPI_OBJ_ADAPTER, 6)
#define HPI_ADAPTER_GET_MODE            HPI_MAKE_INDEX(HPI_OBJ_ADAPTER, 7)
#define HPI_ADAPTER_ENABLE_CAPABILITY   HPI_MAKE_INDEX(HPI_OBJ_ADAPTER, 8)
#define HPI_ADAPTER_SELFTEST            HPI_MAKE_INDEX(HPI_OBJ_ADAPTER, 9)
#define HPI_ADAPTER_FIND_OBJECT         HPI_MAKE_INDEX(HPI_OBJ_ADAPTER, 10)
#define HPI_ADAPTER_QUERY_FLASH         HPI_MAKE_INDEX(HPI_OBJ_ADAPTER, 11)
#define HPI_ADAPTER_START_FLASH         HPI_MAKE_INDEX(HPI_OBJ_ADAPTER, 12)
#define HPI_ADAPTER_PROGRAM_FLASH       HPI_MAKE_INDEX(HPI_OBJ_ADAPTER, 13)
#define HPI_ADAPTER_SET_PROPERTY        HPI_MAKE_INDEX(HPI_OBJ_ADAPTER, 14)
#define HPI_ADAPTER_GET_PROPERTY        HPI_MAKE_INDEX(HPI_OBJ_ADAPTER, 15)
#define HPI_ADAPTER_ENUM_PROPERTY       HPI_MAKE_INDEX(HPI_OBJ_ADAPTER, 16)
#define HPI_ADAPTER_MODULE_INFO         HPI_MAKE_INDEX(HPI_OBJ_ADAPTER, 17)
#define HPI_ADAPTER_DEBUG_READ          HPI_MAKE_INDEX(HPI_OBJ_ADAPTER, 18)
#define HPI_ADAPTER_FUNCTION_COUNT 18
/* OUTPUT STREAM */
#define HPI_OSTREAM_OPEN                HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 1)
#define HPI_OSTREAM_CLOSE               HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 2)
#define HPI_OSTREAM_WRITE               HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 3)
#define HPI_OSTREAM_START               HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 4)
#define HPI_OSTREAM_STOP                HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 5)
#define HPI_OSTREAM_RESET               HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 6)
#define HPI_OSTREAM_GET_INFO            HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 7)
#define HPI_OSTREAM_QUERY_FORMAT        HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 8)
#define HPI_OSTREAM_DATA                HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 9)
#define HPI_OSTREAM_SET_VELOCITY        HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 10)
#define HPI_OSTREAM_SET_PUNCHINOUT      HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 11)
#define HPI_OSTREAM_SINEGEN             HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 12)
#define HPI_OSTREAM_ANC_RESET           HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 13)
#define HPI_OSTREAM_ANC_GET_INFO        HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 14)
#define HPI_OSTREAM_ANC_READ            HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 15)
#define HPI_OSTREAM_SET_TIMESCALE       HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 16)
#define HPI_OSTREAM_SET_FORMAT          HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 17)
#define HPI_OSTREAM_HOSTBUFFER_ALLOC    HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 18)
#define HPI_OSTREAM_HOSTBUFFER_FREE     HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 19)
#define HPI_OSTREAM_GROUP_ADD           HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 20)
#define HPI_OSTREAM_GROUP_GETMAP        HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 21)
#define HPI_OSTREAM_GROUP_RESET         HPI_MAKE_INDEX(HPI_OBJ_OSTREAM, 22)
#define HPI_OSTREAM_FUNCTION_COUNT              (22)
/* INPUT STREAM */
#define HPI_ISTREAM_OPEN                HPI_MAKE_INDEX(HPI_OBJ_ISTREAM, 1)
#define HPI_ISTREAM_CLOSE               HPI_MAKE_INDEX(HPI_OBJ_ISTREAM, 2)
#define HPI_ISTREAM_SET_FORMAT          HPI_MAKE_INDEX(HPI_OBJ_ISTREAM, 3)
#define HPI_ISTREAM_READ                HPI_MAKE_INDEX(HPI_OBJ_ISTREAM, 4)
#define HPI_ISTREAM_START               HPI_MAKE_INDEX(HPI_OBJ_ISTREAM, 5)
#define HPI_ISTREAM_STOP                HPI_MAKE_INDEX(HPI_OBJ_ISTREAM, 6)
#define HPI_ISTREAM_RESET               HPI_MAKE_INDEX(HPI_OBJ_ISTREAM, 7)
#define HPI_ISTREAM_GET_INFO            HPI_MAKE_INDEX(HPI_OBJ_ISTREAM, 8)
#define HPI_ISTREAM_QUERY_FORMAT        HPI_MAKE_INDEX(HPI_OBJ_ISTREAM, 9)
#define HPI_ISTREAM_ANC_RESET           HPI_MAKE_INDEX(HPI_OBJ_ISTREAM, 10)
#define HPI_ISTREAM_ANC_GET_INFO        HPI_MAKE_INDEX(HPI_OBJ_ISTREAM, 11)
#define HPI_ISTREAM_ANC_WRITE           HPI_MAKE_INDEX(HPI_OBJ_ISTREAM, 12)
#define HPI_ISTREAM_HOSTBUFFER_ALLOC    HPI_MAKE_INDEX(HPI_OBJ_ISTREAM, 13)
#define HPI_ISTREAM_HOSTBUFFER_FREE     HPI_MAKE_INDEX(HPI_OBJ_ISTREAM, 14)
#define HPI_ISTREAM_GROUP_ADD           HPI_MAKE_INDEX(HPI_OBJ_ISTREAM, 15)
#define HPI_ISTREAM_GROUP_GETMAP        HPI_MAKE_INDEX(HPI_OBJ_ISTREAM, 16)
#define HPI_ISTREAM_GROUP_RESET         HPI_MAKE_INDEX(HPI_OBJ_ISTREAM, 17)
#define HPI_ISTREAM_FUNCTION_COUNT              (17)
/* MIXER */
/* NOTE:
   GET_NODE_INFO, SET_CONNECTION, GET_CONNECTIONS are not currently used */
#define HPI_MIXER_OPEN                  HPI_MAKE_INDEX(HPI_OBJ_MIXER, 1)
#define HPI_MIXER_CLOSE                 HPI_MAKE_INDEX(HPI_OBJ_MIXER, 2)
#define HPI_MIXER_GET_INFO              HPI_MAKE_INDEX(HPI_OBJ_MIXER, 3)
#define HPI_MIXER_GET_NODE_INFO         HPI_MAKE_INDEX(HPI_OBJ_MIXER, 4)
#define HPI_MIXER_GET_CONTROL           HPI_MAKE_INDEX(HPI_OBJ_MIXER, 5)
#define HPI_MIXER_SET_CONNECTION        HPI_MAKE_INDEX(HPI_OBJ_MIXER, 6)
#define HPI_MIXER_GET_CONNECTIONS       HPI_MAKE_INDEX(HPI_OBJ_MIXER, 7)
#define HPI_MIXER_GET_CONTROL_BY_INDEX  HPI_MAKE_INDEX(HPI_OBJ_MIXER, 8)
#define HPI_MIXER_GET_CONTROL_ARRAY_BY_INDEX  HPI_MAKE_INDEX(HPI_OBJ_MIXER, 9)
#define HPI_MIXER_GET_CONTROL_MULTIPLE_VALUES HPI_MAKE_INDEX(HPI_OBJ_MIXER, 10)
#define HPI_MIXER_STORE                 HPI_MAKE_INDEX(HPI_OBJ_MIXER, 11)
#define HPI_MIXER_FUNCTION_COUNT        11
/* MIXER CONTROLS */
#define HPI_CONTROL_GET_INFO            HPI_MAKE_INDEX(HPI_OBJ_CONTROL, 1)
#define HPI_CONTROL_GET_STATE           HPI_MAKE_INDEX(HPI_OBJ_CONTROL, 2)
#define HPI_CONTROL_SET_STATE           HPI_MAKE_INDEX(HPI_OBJ_CONTROL, 3)
#define HPI_CONTROL_FUNCTION_COUNT 3
/* NONVOL MEMORY */
#define HPI_NVMEMORY_OPEN               HPI_MAKE_INDEX(HPI_OBJ_NVMEMORY, 1)
#define HPI_NVMEMORY_READ_BYTE          HPI_MAKE_INDEX(HPI_OBJ_NVMEMORY, 2)
#define HPI_NVMEMORY_WRITE_BYTE         HPI_MAKE_INDEX(HPI_OBJ_NVMEMORY, 3)
#define HPI_NVMEMORY_FUNCTION_COUNT 3
/* GPIO */
#define HPI_GPIO_OPEN                   HPI_MAKE_INDEX(HPI_OBJ_GPIO, 1)
#define HPI_GPIO_READ_BIT               HPI_MAKE_INDEX(HPI_OBJ_GPIO, 2)
#define HPI_GPIO_WRITE_BIT              HPI_MAKE_INDEX(HPI_OBJ_GPIO, 3)
#define HPI_GPIO_READ_ALL               HPI_MAKE_INDEX(HPI_OBJ_GPIO, 4)
#define HPI_GPIO_WRITE_STATUS           HPI_MAKE_INDEX(HPI_OBJ_GPIO, 5)
#define HPI_GPIO_FUNCTION_COUNT 5
/* ASYNC EVENT */
#define HPI_ASYNCEVENT_OPEN             HPI_MAKE_INDEX(HPI_OBJ_ASYNCEVENT, 1)
#define HPI_ASYNCEVENT_CLOSE            HPI_MAKE_INDEX(HPI_OBJ_ASYNCEVENT, 2)
#define HPI_ASYNCEVENT_WAIT             HPI_MAKE_INDEX(HPI_OBJ_ASYNCEVENT, 3)
#define HPI_ASYNCEVENT_GETCOUNT         HPI_MAKE_INDEX(HPI_OBJ_ASYNCEVENT, 4)
#define HPI_ASYNCEVENT_GET              HPI_MAKE_INDEX(HPI_OBJ_ASYNCEVENT, 5)
#define HPI_ASYNCEVENT_SENDEVENTS       HPI_MAKE_INDEX(HPI_OBJ_ASYNCEVENT, 6)
#define HPI_ASYNCEVENT_FUNCTION_COUNT 6
/* WATCH-DOG */
#define HPI_WATCHDOG_OPEN               HPI_MAKE_INDEX(HPI_OBJ_WATCHDOG, 1)
#define HPI_WATCHDOG_SET_TIME           HPI_MAKE_INDEX(HPI_OBJ_WATCHDOG, 2)
#define HPI_WATCHDOG_PING               HPI_MAKE_INDEX(HPI_OBJ_WATCHDOG, 3)
/* CLOCK */
#define HPI_CLOCK_OPEN                  HPI_MAKE_INDEX(HPI_OBJ_CLOCK, 1)
#define HPI_CLOCK_SET_TIME              HPI_MAKE_INDEX(HPI_OBJ_CLOCK, 2)
#define HPI_CLOCK_GET_TIME              HPI_MAKE_INDEX(HPI_OBJ_CLOCK, 3)
/* PROFILE */
#define HPI_PROFILE_OPEN_ALL            HPI_MAKE_INDEX(HPI_OBJ_PROFILE, 1)
#define HPI_PROFILE_START_ALL           HPI_MAKE_INDEX(HPI_OBJ_PROFILE, 2)
#define HPI_PROFILE_STOP_ALL            HPI_MAKE_INDEX(HPI_OBJ_PROFILE, 3)
#define HPI_PROFILE_GET                 HPI_MAKE_INDEX(HPI_OBJ_PROFILE, 4)
#define HPI_PROFILE_GET_IDLECOUNT       HPI_MAKE_INDEX(HPI_OBJ_PROFILE, 5)
#define HPI_PROFILE_GET_NAME            HPI_MAKE_INDEX(HPI_OBJ_PROFILE, 6)
#define HPI_PROFILE_GET_UTILIZATION     HPI_MAKE_INDEX(HPI_OBJ_PROFILE, 7)
#define HPI_PROFILE_FUNCTION_COUNT 7
/* ////////////////////////////////////////////////////////////////////// */
/* PRIVATE ATTRIBUTES */
/* ////////////////////////////////////////////////////////////////////// */
/* STRUCTURES */
#ifndef DISABLE_PRAGMA_PACK1
#pragma pack(push, 1)
#endif
/** PCI bus resource */
	struct hpi_pci {
	u32 __iomem *apMemBase[HPI_MAX_ADAPTER_MEM_SPACES];
	struct pci_dev *pOsData;

#ifndef HPI64BIT		/* keep structure size constant */
	u32 dwPadding[HPI_MAX_ADAPTER_MEM_SPACES + 1];
#endif
	u16 wVendorId;
	u16 wDeviceId;
	u16 wSubSysVendorId;
	u16 wSubSysDeviceId;
	u16 wBusNumber;
	u16 wDeviceNumber;
	u32 wInterrupt;
};

struct hpi_resource {
	union {
		struct hpi_pci *Pci;
		char *net_if;
	} r;
#ifndef HPI64BIT		/* keep structure size constant */
	u32 dwPadTo64;
#endif
	u16 wBusType;		/* HPI_BUS_PNPISA, _PCI, _USB etc */
	u16 wPadding;

};

/** Format info used inside struct hpi_message
    Not the same as public API struct hpi_format */
struct hpi_msg_format {
	u32 dwSampleRate;
				/**< 11025, 32000, 44100 ... */
	u32 dwBitRate;	       /**< for MPEG */
	u32 dwAttributes;
				/**< Stereo/JointStereo/Mono */
	u16 wChannels;	       /**< 1,2..., (or ancillary mode or idle bit */
	u16 wFormat; /**< HPI_FORMAT_PCM16, _MPEG etc. see \ref formats. */
};

/**  Buffer+format structure.
	 Must be kept 7 * 32 bits to match public struct hpi_datastruct */
struct hpi_msg_data {
	struct hpi_msg_format Format;
	u8 *pbData;
#ifndef HPI64BIT
	u32 dwPadding;
#endif
	u32 dwDataSize;
};

/** struct hpi_datastructure used up to 3.04 driver */
struct hpi_data_legacy32 {
	struct hpi_format Format;
	u32 pbData;
	u32 dwDataSize;
};

#ifdef HPI64BIT
/* Compatibility version of struct hpi_data*/
struct hpi_data_compat32 {
	struct hpi_msg_format Format;
	u32 pbData;
	u32 dwPadding;
	u32 dwDataSize;
};
#endif

struct hpi_buffer {
  /** placehoder for backward compatability (see dwBufferSize) */
	struct hpi_msg_format reserved;
	u32 dwCommand;	  /**< HPI_BUFFER_CMD_xxx*/
	u32 dwPciAddress; /**< PCI physical address of buffer for DSP DMA */
	u32 dwBufferSize; /**< must line up with dwDataSize of HPI_DATA*/
};

struct hpi_streamid {
	u16 wObjectType;
		    /**< Type of object, HPI_OBJ_OSTREAM or HPI_OBJ_ISTREAM. */
	u16 wStreamIndex; /**< OStream or IStream index. */
};

struct hpi_punchinout {
	u32 dwPunchInSample;
	u32 dwPunchOutSample;
};

struct hpi_subsys_msg {
	struct hpi_resource Resource;
};

struct hpi_subsys_res {
	u32 dwVersion;
	u32 dwData;		/* used to return extended version */
	u16 wNumAdapters;	/* number of adapters */
	u16 wAdapterIndex;
	u16 awAdapterList[HPI_MAX_ADAPTERS];
};

struct hpi_adapter_msg {
	u32 dwAdapterMode;	/* adapter mode */
	u16 wAssertId;		/* assert number for "test assert" call
				   wObjectIndex for find object call
				   wQueryOrSet for HPI_AdapterSetModeEx() */
	u16 wObjectType;	/* for adapter find object call */
};

union hpi_adapterx_msg {
	struct hpi_adapter_msg adapter;
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
	struct {
		u16 index;
	} module_info;
	struct {
		u32 dwDspAddress;
		u32 dwCountBytes;
	} debug_read;
};

struct hpi_adapter_res {
	u32 dwSerialNumber;
	u16 wAdapterType;
	u16 wAdapterIndex;	/* Is this needed? also used for wDspIndex */
	u16 wNumIStreams;
	u16 wNumOStreams;
	u16 wNumMixers;
	u16 wVersion;
	u8 szAdapterAssert[HPI_STRING_LEN];
};

union hpi_adapterx_res {
	struct hpi_adapter_res adapter;
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
};

struct hpi_stream_msg {
	union {
		struct hpi_msg_data Data;
		struct hpi_data_legacy32 Data32;
		u16 wVelocity;
		struct hpi_punchinout Pio;
		u32 dwTimeScale;
		struct hpi_buffer Buffer;
		struct hpi_streamid Stream;
	} u;
	u16 wStreamIndex;
	u16 wIStreamIndex;
};

struct hpi_stream_res {
	union {
		struct {
			/* size of hardware buffer */
			u32 dwBufferSize;
			/* OutStream - data to play,
			   InStream - data recorded */
			u32 dwDataAvailable;
			/* OutStream - samples played,
			   InStream - samples recorded */
			u32 dwSamplesTransferred;
			/* Adapter - OutStream - data to play,
			   InStream - data recorded */
			u32 dwAuxiliaryDataAvailable;
			u16 wState;	/* HPI_STATE_PLAYING, _STATE_STOPPED */
			u16 wPadding;
		} stream_info;
		struct {
			u32 dwBufferSize;
			u32 dwDataAvailable;
			u32 dwSamplesTransfered;
			u16 wState;
			u16 wOStreamIndex;
			u16 wIStreamIndex;
			u16 wPadding;
			u32 dwAuxiliaryDataAvailable;
		} legacy_stream_info;
		struct {
			/* bitmap of grouped OutStreams */
			u32 dwOutStreamGroupMap;
			/* bitmap of grouped InStreams */
			u32 dwInStreamGroupMap;
		} group_info;
	} u;
};

struct hpi_mixer_msg {
	u16 wControlIndex;
	u16 wControlType;	/* = HPI_CONTROL_METER _VOLUME etc */
	u16 wPadding1;		/* Maintain alignment of subsequent fields */
	u16 wNodeType1;		/* = HPI_SOURCENODE_LINEIN etc */
	u16 wNodeIndex1;	/* = 0..N */
	u16 wNodeType2;
	u16 wNodeIndex2;
	u16 wPadding2;		/* round to 4 bytes */
};

struct hpi_mixer_res {
	u16 wSrcNodeType;	/* = HPI_SOURCENODE_LINEIN etc */
	u16 wSrcNodeIndex;	/* = 0..N */
	u16 wDstNodeType;
	u16 wDstNodeIndex;
	/* Also controlType for MixerGetControlByIndex */
	u16 wControlIndex;
	/* may indicate which DSP the control is located on */
	u16 wDspIndex;
};

union hpi_mixerx_msg {
	struct {
		u16 wStartingIndex;
		u16 wFlags;
		u32 dwLengthInBytes;	/* length in bytes of pData */
		u32 pData;	/* pointer to a data array */
	} gcabi;
	struct {
		u16 wCommand;
		u16 wIndex;
	} store;		/* for HPI_MIXER_STORE message */
};

union hpi_mixerx_res {
	struct {
		u32 dwBytesReturned;	/* size of items returned */
		u32 pData;	/* pointer to data array */
		u16 wMoreToDo;	/* indicates if there is more to do */
	} gcabi;
};

struct hpi_control_msg {
	u32 dwParam1;		/* generic parameter 1 */
	u32 dwParam2;		/* generic parameter 2 */
	short anLogValue[HPI_MAX_CHANNELS];
	u16 wAttribute;		/* control attribute or property */
	u16 wControlIndex;
};

struct hpi_control_union_msg {
	union {
		struct {
			u32 dwParam1;	/* generic parameter 1 */
			u32 dwParam2;	/* generic parameter 2 */
			short anLogValue[HPI_MAX_CHANNELS];
		} old;
		union {
			u32 dwFrequency;
			u32 dwGain;
			u32 dwBand;
			u32 dwDeemphasis;
			u32 dwProgram;
			struct {
				u32 dwMode;
				u32 dwValue;
			} mode;
		} tuner;
	} u;
	u16 wAttribute;		/* control attribute or property */
	u16 wControlIndex;
};

struct hpi_control_res {
	/* Could make union. dwParam, anLogValue never used in same response */
	u32 dwParam1;
	u32 dwParam2;
	short anLogValue[HPI_MAX_CHANNELS];
};

union hpi_control_union_res {
	struct {
		u32 dwParam1;
		u32 dwParam2;
		short anLogValue[HPI_MAX_CHANNELS];
	} old;
	union {
		u32 dwBand;
		u32 dwFrequency;
		u32 dwGain;
		u32 dwLevel;
		u32 dwDeemphasis;
		struct {
			u32 dwData[2];
			u32 dwBLER;
		} rds;
	} tuner;
	struct {
		char szData[8];
		u32 dwRemainingChars;
	} chars8;
};

/* HPI_CONTROLX_STRUCTURES */

/* Message */

/** Used for all HMI variables where max length <= 8 bytes
*/
struct hpi_controlx_msg_cobranet_data {
	u32 dwHmiAddress;
	u32 dwByteCount;
	u32 dwData[2];
};

/** Used for string data, and for packet bridge
*/
struct hpi_controlx_msg_cobranet_bigdata {
	u32 dwHmiAddress;
	u32 dwByteCount;
	u8 *pbData;
#ifndef HPI64BIT
	u32 dwPadding;
#endif
};

/** Used for PADS control reading of string fields.
*/
struct hpi_controlx_msg_pad_data {
	u32 dwField;
	u32 dwByteCount;
	u8 *pbData;
#ifndef HPI64BIT
	u32 dwPadding;
#endif
};

/** Used for generic data
*/

struct hpi_controlx_msg_generic {
	u32 dwParam1;
	u32 dwParam2;
};

struct hpi_controlx_msg {
	union {
		struct hpi_controlx_msg_cobranet_data cobranet_data;
		struct hpi_controlx_msg_cobranet_bigdata cobranet_bigdata;
		struct hpi_controlx_msg_generic generic;
		struct hpi_controlx_msg_pad_data pad_data;
		/* nothing extra to send for status read */
	} u;
	u16 wControlIndex;
	u16 wAttribute;		/* control attribute or property */
};

/* Response */
/**
*/
struct hpi_controlx_res_cobranet_data {
	u32 dwByteCount;
	u32 dwData[2];
};

struct hpi_controlx_res_cobranet_bigdata {
	u32 dwByteCount;
};

struct hpi_controlx_res_cobranet_status {
	u32 dwStatus;
	u32 dwReadableSize;
	u32 dwWriteableSize;
};

struct hpi_controlx_res_generic {
	u32 dwParam1;
	u32 dwParam2;
};

struct hpi_controlx_res {
	union {
		struct hpi_controlx_res_cobranet_bigdata cobranet_bigdata;
		struct hpi_controlx_res_cobranet_data cobranet_data;
		struct hpi_controlx_res_cobranet_status cobranet_status;
		struct hpi_controlx_res_generic generic;
	} u;
};

struct hpi_nvmemory_msg {
	u16 wIndex;
	u16 wData;
};

struct hpi_nvmemory_res {
	u16 wSizeInBytes;
	u16 wData;
};

struct hpi_gpio_msg {
	u16 wBitIndex;
	u16 wBitData;
};

struct hpi_gpio_res {
	u16 wNumberInputBits;
	u16 wNumberOutputBits;
	u16 wBitData[4];
};

struct hpi_async_msg {
	u32 dwEvents;
	u16 wMaximumEvents;
	u16 wPadding;
};

struct hpi_async_res {
	union {
		struct {
			u16 wCount;
		} count;
		struct {
			u32 dwEvents;
			u16 wNumberReturned;
			u16 wPadding;
		} get;
		struct hpi_async_event event;
	} u;
};

struct hpi_watchdog_msg {
	u32 dwTimeMs;
};

struct hpi_watchdog_res {
	u32 dwTimeMs;
};

struct hpi_clock_msg {
	u16 wHours;
	u16 wMinutes;
	u16 wSeconds;
	u16 wMilliSeconds;
};

struct hpi_clock_res {
	u16 wSizeInBytes;
	u16 wHours;
	u16 wMinutes;
	u16 wSeconds;
	u16 wMilliSeconds;
	u16 wPadding;
};

struct hpi_profile_msg {
	u16 wIndex;
	u16 wPadding;
};

struct hpi_profile_res_open {
	u16 wMaxProfiles;
};

struct hpi_profile_res_time {
	u32 dwMicroSeconds;
	u32 dwCallCount;
	u32 dwMaxMicroSeconds;
	u32 dwMinMicroSeconds;
	u16 wSeconds;
};

struct hpi_profile_res_name {
/* u8 messes up response size for 56301 DSP */
	u16 szName[16];
};

struct hpi_profile_res {
	union {
		struct hpi_profile_res_open o;
		struct hpi_profile_res_time t;
		struct hpi_profile_res_name n;
	} u;
};

struct hpi_message_header {
	u16 wSize;
	u16 wType;		/* HPI_MSG_MESSAGE, HPI_MSG_RESPONSE */
	u16 wObject;		/* HPI_OBJ_* */
	u16 wFunction;		/* HPI_SUBSYS_xxx, HPI_ADAPTER_xxx */
	u16 wAdapterIndex;	/* the adapter index */
	u16 wDspIndex;		/* the dsp index on the adapter */
};

struct hpi_message {
	/* following fields must match HPI_MESSAGE_HEADER */
	u16 wSize;
	u16 wType;		/* HPI_TYPE_MESSAGE, HPI_TYPE_RESPONSE */
	u16 wObject;		/* HPI_OBJ_* */
	u16 wFunction;		/* HPI_SUBSYS_xxx, HPI_ADAPTER_xxx */
	u16 wAdapterIndex;	/* the adapter index */
	u16 wDspIndex;		/* the dsp index on the adapter */
	union {
		struct hpi_subsys_msg s;
		struct hpi_adapter_msg a;
		union hpi_adapterx_msg ax;
		struct hpi_stream_msg d;
		struct hpi_mixer_msg m;
		union hpi_mixerx_msg mx;	/* extended mixer; */
		struct hpi_control_msg c;	/* mixer control; */
		/* identical to struct hpi_control_msg,
		   but field naming is improved */
		struct hpi_control_union_msg cu;
		struct hpi_controlx_msg cx;	/* extended mixer control; */
		struct hpi_nvmemory_msg n;
		struct hpi_gpio_msg l;	/* digital i/o */
		struct hpi_watchdog_msg w;
		struct hpi_clock_msg t;	/* dsp time */
		struct hpi_profile_msg p;
		struct hpi_async_msg as;
	} u;
};

#define HPI_MESSAGE_SIZE_BY_OBJECT { \
	sizeof(struct hpi_message_header) ,   /* Default, no object type 0 */ \
	sizeof(struct hpi_message_header) + sizeof(struct hpi_subsys_msg),\
	sizeof(struct hpi_message_header) + sizeof(union hpi_adapterx_msg),\
	sizeof(struct hpi_message_header) + sizeof(struct hpi_stream_msg),\
	sizeof(struct hpi_message_header) + sizeof(struct hpi_stream_msg),\
	sizeof(struct hpi_message_header) + sizeof(struct hpi_mixer_msg),\
	sizeof(struct hpi_message_header) ,   /* no node message */ \
	sizeof(struct hpi_message_header) + sizeof(struct hpi_control_msg),\
	sizeof(struct hpi_message_header) + sizeof(struct hpi_nvmemory_msg),\
	sizeof(struct hpi_message_header) + sizeof(struct hpi_gpio_msg),\
	sizeof(struct hpi_message_header) + sizeof(struct hpi_watchdog_msg),\
	sizeof(struct hpi_message_header) + sizeof(struct hpi_clock_msg),\
	sizeof(struct hpi_message_header) + sizeof(struct hpi_profile_msg),\
	sizeof(struct hpi_message_header) + sizeof(struct hpi_controlx_msg),\
	sizeof(struct hpi_message_header) + sizeof(struct hpi_async_msg) \
}

struct hpi_response_header {
	u16 wSize;
	u16 wType;		/* HPI_MSG_MESSAGE, HPI_MSG_RESPONSE */
	u16 wObject;		/* HPI_OBJ_* */
	u16 wFunction;		/* HPI_SUBSYS_xxx, HPI_ADAPTER_xxx */
	u16 wError;		/* HPI_ERROR_xxx */
	u16 wSpecificError;	/* Adapter specific error */
};

struct hpi_response {
/* following fields must match HPI_RESPONSE_HEADER */
	u16 wSize;
	u16 wType;		/* HPI_MSG_MESSAGE, HPI_MSG_RESPONSE */
	u16 wObject;		/* HPI_OBJ_* */
	u16 wFunction;		/* HPI_SUBSYS_xxx, HPI_ADAPTER_xxx */
	u16 wError;		/* HPI_ERROR_xxx */
	u16 wSpecificError;	/* Adapter specific error */
	union {
		struct hpi_subsys_res s;
		struct hpi_adapter_res a;
		union hpi_adapterx_res ax;
		struct hpi_stream_res d;
		struct hpi_mixer_res m;
		union hpi_mixerx_res mx;	/* extended mixer; */
		struct hpi_control_res c;	/* mixer control; */
		/* identical to hpi_control_res, but field naming is improved */
		union hpi_control_union_res cu;
		struct hpi_controlx_res cx;	/* extended mixer control; */
		struct hpi_nvmemory_res n;
		struct hpi_gpio_res l;	/* digital i/o */
		struct hpi_watchdog_res w;
		struct hpi_clock_res t;	/* dsp time */
		struct hpi_profile_res p;
		struct hpi_async_res as;
		u8 bytes[52];
	} u;
};

#define HPI_RESPONSE_SIZE_BY_OBJECT { \
	sizeof(struct hpi_response_header) ,/* Default, no object type 0 */ \
	sizeof(struct hpi_response_header) + sizeof(struct hpi_subsys_res),\
	sizeof(struct hpi_response_header) + sizeof(union  hpi_adapterx_res),\
	sizeof(struct hpi_response_header) + sizeof(struct hpi_stream_res),\
	sizeof(struct hpi_response_header) + sizeof(struct hpi_stream_res),\
	sizeof(struct hpi_response_header) + sizeof(struct hpi_mixer_res),\
	sizeof(struct hpi_response_header) , /* no node response */ \
	sizeof(struct hpi_response_header) + sizeof(struct hpi_control_res),\
	sizeof(struct hpi_response_header) + sizeof(struct hpi_nvmemory_res),\
	sizeof(struct hpi_response_header) + sizeof(struct hpi_gpio_res),\
	sizeof(struct hpi_response_header) + sizeof(struct hpi_watchdog_res),\
	sizeof(struct hpi_response_header) + sizeof(struct hpi_clock_res),\
	sizeof(struct hpi_response_header) + sizeof(struct hpi_profile_res),\
	sizeof(struct hpi_response_header) + sizeof(struct hpi_controlx_res),\
	sizeof(struct hpi_response_header) + sizeof(struct hpi_async_res) \
}

/*////////////////////////////////////////////////////////////////////////// */
/* declarations for compact control calls  */
struct hpi_control_defn {
	u8 wType;
	u8 wChannels;
	u8 wSrcNodeType;
	u8 wSrcNodeIndex;
	u8 wDestNodeType;
	u8 wDestNodeIndex;
};

/*////////////////////////////////////////////////////////////////////////// */
/* declarations for control caching (internal to HPI<->DSP interaction)      */

/** A compact representation of (part of) a controls state.
Used for efficient transfer of the control state
between DSP and host or across a network
*/
struct hpi_control_cache_info {
	/** one of HPI_CONTROL_* */
	u8 ControlType;
	/** The total size of cached information in 32-bit words. */
	u8 nSizeIn32bitWords;
	/** The original index of the control on the DSP */
	u16 ControlIndex;
};

struct hpi_control_cache_single {
	struct hpi_control_cache_info i;
	union {
		struct {	/* volume */
			u16 anLog[2];
		} v;
		struct {	/* peak meter */
			u16 anLogPeak[2];
			u16 anLogRMS[2];
		} p;
		struct {	/* channel mode */
			u16 wMode;
		} m;
		struct {	/* multiplexer */
			u16 wSourceNodeType;
			u16 wSourceNodeIndex;
		} x;
		struct {	/* level/trim */
			u16 anLog[2];
		} l;
		struct {	/* tuner - partial caching.
				   Some attributes go to the DSP. */
			u32 dwFreqInkHz;
			u16 wBand;
			u16 wLevel;
		} t;
		struct {	/* AESEBU Rx status */
			u32 dwErrorStatus;
			u32 dwSource;
		} aes3rx;
		struct {	/* AESEBU Tx */
			u32 dwFormat;
		} aes3tx;
		struct {	/* tone detector */
			u16 wState;
		} tone;
		struct {	/* silence detector */
			u32 dwState;
			u32 dwCount;
		} silence;
		struct {	/* sample clock */
			u16 wSource;
			u16 wSourceIndex;
			u32 dwSampleRate;
		} clk;
		struct {	/* generic control */
			u32 dw1;
			u32 dw2;
		} g;
	} u;
};

struct hpi_control_cache_pad {
	struct hpi_control_cache_info i;
	u32 dwFieldValidFlags;
	u8 cChannel[8];
	u8 cArtist[40];
	u8 cTitle[40];
	u8 cComment[200];
	u32 dwPTY;
	u32 dwPI;
	u32 dwTrafficSupported;
	u32 dwTrafficAnouncement;
};

/*/////////////////////////////////////////////////////////////////////////// */
/* declarations for 2^N sized FIFO buffer (internal to HPI<->DSP interaction) */
struct hpi_fifo_buffer {
	u32 dwSize;
	u32 dwDSPIndex;
	u32 dwHostIndex;
};

#ifndef DISABLE_PRAGMA_PACK1
#pragma pack(pop)
#endif

/* skip host side function declarations for DSP
   compile and documentation extraction */

char HPI_HandleObject(
	const u32 dwHandle
);

void HPI_HandleToIndexes(
	const u32 dwHandle,
	u16 *pwAdapterIndex,
	u16 *pwObjectIndex,
	u16 *pwDspIndex
);

u32 HPI_IndexesToHandle(
	const char cObject,
	const u16 wAdapterIndex,
	const u16 wObjectIndex,
	const u16 wDspIndex
);

/*////////////////////////////////////////////////////////////////////////// */

/* main HPI entry point */
HPI_HandlerFunc HPI_Message;

/* UDP message */
void HPI_MessageUDP(
	struct hpi_message *phm,
	struct hpi_response *phr,
	unsigned int nTimeout
);

/* used in PnP OS/driver */
u16 HPI_SubSysCreateAdapter(
	struct hpi_hsubsys *phSubSys,
	struct hpi_resource *pResource,
	u16 *pwAdapterIndex
);

u16 HPI_SubSysDeleteAdapter(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex
);

void HPI_FormatToMsg(
	struct hpi_msg_format *pMF,
	struct hpi_format *pF
);
void HPI_StreamResponseToLegacy(
	struct hpi_stream_res *pSR
);

/*////////////////////////////////////////////////////////////////////////// */
/* declarations for individual HPI entry points */
HPI_HandlerFunc HPI_6000;
HPI_HandlerFunc HPI_6205;
HPI_HandlerFunc HPI_COMMON;

#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#endif				/* _HPI_INTERNAL_H_ */
