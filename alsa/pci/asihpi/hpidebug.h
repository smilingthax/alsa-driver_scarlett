/*****************************************************************************

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

Debug macros.

*****************************************************************************/

#ifndef _HPIDEBUG_H
#define _HPIDEBUG_H

#include "hpi.h"
#include "hpios.h"

#ifdef __cplusplus
extern "C" {
#endif

// Default DBG character type definitions.
#ifndef DBG_TEXT
#define DBG_TEXT(s) s
#define DBG_CHAR char
#endif

/* Define debugging levels.  */
	enum { HPI_DEBUG_LEVEL_ERROR,	/* Always log errors */
		HPI_DEBUG_LEVEL_WARNING,
		HPI_DEBUG_LEVEL_NOTICE,
		HPI_DEBUG_LEVEL_INFO,
		HPI_DEBUG_LEVEL_DEBUG,
		HPI_DEBUG_LEVEL_VERBOSE	/* Same printk level as DEBUG */
	};

#define HPI_DEBUG_LEVEL_DEFAULT HPI_DEBUG_LEVEL_NOTICE

/* an OS can define an extra flag string that is appended to
the start of each message, eg see hpios_linux.h */

/* OSes without printf-like function must provide plain string output
and define HPIOS_DEBUG_PRINT to the name of this function.
The printf functionality then provided by hpidebug.c::hpi_debug_printf
*/
#if defined ( HPIOS_DEBUG_PRINT ) && ! defined ( HPIOS_DEBUG_PRINTF )
#define HPIOS_DEBUG_PRINTF hpi_debug_printf
	void hpi_debug_printf(DBG_CHAR * fmt, ...);
#endif

#if ! defined ( HPIOS_DEBUG_PRINT ) && ! defined ( HPIOS_DEBUG_PRINTF ) && defined ( HPI_DEBUG )
#undef HPI_DEBUG
#ifdef _MSC_VER
#pragma message ("Warning: Can't debug with this build because no debug print functions defined")
#endif
#endif

#ifdef SOURCEFILE_NAME
#define FILE_LINE  DBG_TEXT(SOURCEFILE_NAME) DBG_TEXT(":") DBG_TEXT(__stringify(__LINE__)) DBG_TEXT(" ")
#else
#define FILE_LINE  DBG_TEXT(__FILE__) DBG_TEXT(":") DBG_TEXT(__stringify(__LINE__)) DBG_TEXT(" ")
#endif

#define HPI_DEBUG_LOG(level, ...) \
do { \
if ( hpiDebugLevel >= HPI_DEBUG_LEVEL_##level ) { \
HPIOS_DEBUG_PRINTF( HPI_DEBUG_FLAG_##level  FILE_LINE  __VA_ARGS__); \
} \
} while (0)

#define HPI_DEBUG_LOG0 HPI_DEBUG_LOG
#define HPI_DEBUG_LOG1 HPI_DEBUG_LOG
#define HPI_DEBUG_LOG2 HPI_DEBUG_LOG
#define HPI_DEBUG_LOG3 HPI_DEBUG_LOG
#define HPI_DEBUG_LOG4 HPI_DEBUG_LOG

#define HPIOS_DEBUG_STRING(s) HPI_DEBUG_LOG0(VERBOSE,s "\n")

	void HPI_DebugInit(void);
	int HPI_DebugLevelSet(int level);
	int HPI_DebugLevelGet(void);
	extern int hpiDebugLevel;	// needed by Linux driver for dynamic debug level changes

	void hpi_debug_message(HPI_MESSAGE * phm, DBG_CHAR * szFileline);
#if 0
	void hpi_debug_response(HPI_RESPONSE * phr, DBG_CHAR * szFileline);
#endif

	extern void
	 hpi_debug_data(u16 * pdata, u32 len);

#define HPI_DEBUG_DATA(pdata,len)                                                                               \
do {                                                                                                                            \
if (hpiDebugLevel >= HPI_DEBUG_LEVEL_VERBOSE) hpi_debug_data(pdata,len); \
} while (0)

#define HPI_DEBUG_MESSAGE(phm)                                                                  \
do {                                                                                                            \
if (hpiDebugLevel >= HPI_DEBUG_LEVEL_DEBUG) {                   \
hpi_debug_message(phm,HPI_DEBUG_FLAG_DEBUG FILE_LINE DBG_TEXT("DEBUG ") );\
}                                                                                                               \
} while (0)

#define HPI_DEBUG_RESPONSE(phr)                                                                 \
do {                                                                                                            \
if ((hpiDebugLevel >= HPI_DEBUG_LEVEL_DEBUG) && (phr->wError))  \
HPI_DEBUG_LOG1(ERROR,DBG_TEXT("HPI Response - error# %d\n"), phr->wError); \
else if (hpiDebugLevel >= HPI_DEBUG_LEVEL_VERBOSE) \
HPI_DEBUG_LOG0(VERBOSE,DBG_TEXT("HPI Response OK\n")); \
} while (0)

/* Be careful with these macros.  Ensure that they are used within a block.
Otherwise the second if might have an else after it... */

/* These strings should be generated using a macro which defines
the corresponding symbol values.  */
#define HPI_OBJ_STRINGS         \
{                               \
DBG_TEXT("HPI_OBJ_SUBSYSTEM"),        \
DBG_TEXT("HPI_OBJ_ADAPTER"),          \
DBG_TEXT("HPI_OBJ_OSTREAM"),          \
DBG_TEXT("HPI_OBJ_ISTREAM"),          \
DBG_TEXT("HPI_OBJ_MIXER"),            \
DBG_TEXT("HPI_OBJ_NODE"),                     \
DBG_TEXT("HPI_OBJ_CONTROL"),          \
DBG_TEXT("HPI_OBJ_NVMEMORY"),         \
DBG_TEXT("HPI_OBJ_DIGITALIO"),        \
DBG_TEXT("HPI_OBJ_WATCHDOG"),         \
DBG_TEXT("HPI_OBJ_CLOCK"),            \
DBG_TEXT("HPI_OBJ_PROFILE"),          \
DBG_TEXT("HPI_OBJ_CONTROLEX")         \
};

#define HPI_SUBSYS_STRINGS      \
{                               \
DBG_TEXT("HPI_SUBSYS_OPEN"),          \
DBG_TEXT("HPI_SUBSYS_GET_VERSION"),   \
DBG_TEXT("HPI_SUBSYS_GET_INFO"),      \
DBG_TEXT("HPI_SUBSYS_FIND_ADAPTERS"), \
DBG_TEXT("HPI_SUBSYS_CREATE_ADAPTER"),        \
DBG_TEXT("HPI_SUBSYS_CLOSE"),         \
DBG_TEXT("HPI_SUBSYS_DELETE_ADAPTER"), \
DBG_TEXT("HPI_SUBSYS_DRIVER_LOAD"), \
DBG_TEXT("HPI_SUBSYS_DRIVER_UNLOAD"), \
DBG_TEXT("HPI_SUBSYS_READ_PORT_8"),   \
DBG_TEXT("HPI_SUBSYS_WRITE_PORT_8"),  \
DBG_TEXT("HPI_SUBSYS_GET_NUM_ADAPTERS"),      \
DBG_TEXT("HPI_SUBSYS_GET_ADAPTER")    \
};

#define HPI_ADAPTER_STRINGS     \
{                               \
DBG_TEXT("HPI_ADAPTER_OPEN"),         \
DBG_TEXT("HPI_ADAPTER_CLOSE"),                \
DBG_TEXT("HPI_ADAPTER_GET_INFO"),     \
DBG_TEXT("HPI_ADAPTER_GET_ASSERT"),   \
DBG_TEXT("HPI_ADAPTER_TEST_ASSERT"),    \
DBG_TEXT("HPI_ADAPTER_SET_MODE"),       \
DBG_TEXT("HPI_ADAPTER_GET_MODE"),       \
DBG_TEXT("HPI_ADAPTER_ENABLE_CAPABILITY"),\
DBG_TEXT("HPI_ADAPTER_SELFTEST"),        \
DBG_TEXT("HPI_ADAPTER_FIND_OBJECT"),     \
DBG_TEXT("HPI_ADAPTER_QUERY_FLASH"),     \
DBG_TEXT("HPI_ADAPTER_START_FLASH"),     \
DBG_TEXT("HPI_ADAPTER_PROGRAM_FLASH"),   \
DBG_TEXT("HPI_ADAPTER_SET_PROPERTY"),    \
DBG_TEXT("HPI_ADAPTER_GET_PROPERTY"),    \
DBG_TEXT("HPI_ADAPTER_ENUM_PROPERTY")    \
};

#define HPI_OSTREAM_STRINGS     \
{                               \
DBG_TEXT("HPI_OSTREAM_OPEN"),         \
DBG_TEXT("HPI_OSTREAM_CLOSE"),                \
DBG_TEXT("HPI_OSTREAM_WRITE"),                \
DBG_TEXT("HPI_OSTREAM_START"),                \
DBG_TEXT("HPI_OSTREAM_STOP"),         \
DBG_TEXT("HPI_OSTREAM_RESET"),                \
DBG_TEXT("HPI_OSTREAM_GET_INFO"),     \
DBG_TEXT("HPI_OSTREAM_QUERY_FORMAT"), \
DBG_TEXT("HPI_OSTREAM_DATA"),         \
DBG_TEXT("HPI_OSTREAM_SET_VELOCITY"), \
DBG_TEXT("HPI_OSTREAM_SET_PUNCHINOUT"), \
DBG_TEXT("HPI_OSTREAM_SINEGEN"),        \
DBG_TEXT("HPI_OSTREAM_ANC_RESET"),      \
DBG_TEXT("HPI_OSTREAM_ANC_GET_INFO"),   \
DBG_TEXT("HPI_OSTREAM_ANC_READ"),       \
DBG_TEXT("HPI_OSTREAM_SET_TIMESCALE"),                \
DBG_TEXT("HPI_OSTREAM_SET_FORMAT"), \
DBG_TEXT("HPI_OSTREAM_HOSTBUFFER_ALLOC"), \
DBG_TEXT("HPI_OSTREAM_HOSTBUFFER_FREE"), \
DBG_TEXT("HPI_OSTREAM_GROUP_ADD"),\
DBG_TEXT("HPI_OSTREAM_GROUP_GETMAP"), \
DBG_TEXT("HPI_OSTREAM_GROUP_RESET"), \
};

#define HPI_ISTREAM_STRINGS     \
{                               \
DBG_TEXT("HPI_ISTREAM_OPEN"),         \
DBG_TEXT("HPI_ISTREAM_CLOSE"),                \
DBG_TEXT("HPI_ISTREAM_SET_FORMAT"),   \
DBG_TEXT("HPI_ISTREAM_READ"),         \
DBG_TEXT("HPI_ISTREAM_START"),                \
DBG_TEXT("HPI_ISTREAM_STOP"),         \
DBG_TEXT("HPI_ISTREAM_RESET"),                \
DBG_TEXT("HPI_ISTREAM_GET_INFO"),     \
DBG_TEXT("HPI_ISTREAM_QUERY_FORMAT"), \
DBG_TEXT("HPI_ISTREAM_ANC_RESET"),      \
DBG_TEXT("HPI_ISTREAM_ANC_GET_INFO"),   \
DBG_TEXT("HPI_ISTREAM_ANC_WRITE"),   \
DBG_TEXT("HPI_ISTREAM_HOSTBUFFER_ALLOC"),     \
DBG_TEXT("HPI_ISTREAM_HOSTBUFFER_FREE"), \
DBG_TEXT("HPI_ISTREAM_GROUP_ADD"), \
DBG_TEXT("HPI_ISTREAM_GROUP_GETMAP"), \
DBG_TEXT("HPI_ISTREAM_GROUP_RESET"), \
};

#define HPI_MIXER_STRINGS       \
{                               \
DBG_TEXT("HPI_MIXER_OPEN"),           \
DBG_TEXT("HPI_MIXER_CLOSE"),          \
DBG_TEXT("HPI_MIXER_GET_INFO"),               \
DBG_TEXT("HPI_MIXER_GET_NODE_INFO"),  \
DBG_TEXT("HPI_MIXER_GET_CONTROL"),    \
DBG_TEXT("HPI_MIXER_SET_CONNECTION"), \
DBG_TEXT("HPI_MIXER_GET_CONNECTIONS"),        \
DBG_TEXT("HPI_MIXER_GET_CONTROL_BY_INDEX"),   \
DBG_TEXT("HPI_MIXER_GET_CONTROL_ARRAY_BY_INDEX"),     \
DBG_TEXT("HPI_MIXER_GET_CONTROL_MULTIPLE_VALUES"),    \
DBG_TEXT("HPI_MIXER_STORE"),  \
};

#define HPI_CONTROL_STRINGS     \
{                               \
DBG_TEXT("HPI_CONTROL_GET_INFO"),     \
DBG_TEXT("HPI_CONTROL_GET_STATE"),    \
DBG_TEXT("HPI_CONTROL_SET_STATE")     \
};

#define HPI_NVMEMORY_STRINGS    \
{                               \
DBG_TEXT("HPI_NVMEMORY_OPEN"),                \
DBG_TEXT("HPI_NVMEMORY_READ_BYTE"),   \
DBG_TEXT("HPI_NVMEMORY_WRITE_BYTE")   \
};

#define HPI_DIGITALIO_STRINGS   \
{                               \
DBG_TEXT("HPI_GPIO_OPEN"),            \
DBG_TEXT("HPI_GPIO_READ_BIT"),        \
DBG_TEXT("HPI_GPIO_WRITE_BIT"),       \
DBG_TEXT("HPI_GPIO_READ_ALL")\
};

#define HPI_WATCHDOG_STRINGS    \
{                               \
DBG_TEXT("HPI_WATCHDOG_OPEN"),                \
DBG_TEXT("HPI_WATCHDOG_SET_TIME"),    \
DBG_TEXT("HPI_WATCHDOG_PING")         \
};

#define HPI_CLOCK_STRINGS       \
{                               \
DBG_TEXT("HPI_CLOCK_OPEN"),           \
DBG_TEXT("HPI_CLOCK_SET_TIME"),               \
DBG_TEXT("HPI_CLOCK_GET_TIME")                \
};

#define HPI_PROFILE_STRINGS     \
{                               \
DBG_TEXT("HPI_PROFILE_OPEN_ALL"),     \
DBG_TEXT("HPI_PROFILE_START_ALL"),    \
DBG_TEXT("HPI_PROFILE_STOP_ALL"),     \
DBG_TEXT("HPI_PROFILE_GET"),          \
DBG_TEXT("HPI_PROFILE_GET_IDLECOUNT"),  \
DBG_TEXT("HPI_PROFILE_GET_NAME"),       \
DBG_TEXT("HPI_PROFILE_GET_UTILIZATION") \
};

#define HPI_CONTROL_TYPE_STRINGS \
{ \
DBG_TEXT("no control (0)"), \
DBG_TEXT("HPI_CONTROL_CONNECTION"), \
DBG_TEXT("HPI_CONTROL_VOLUME"), \
DBG_TEXT("HPI_CONTROL_METER"), \
DBG_TEXT("HPI_CONTROL_MUTE"), \
DBG_TEXT("HPI_CONTROL_MULTIPLEXER"), \
DBG_TEXT("HPI_CONTROL_AESEBU_TRANSMITTER"), \
DBG_TEXT("HPI_CONTROL_AESEBU_RECEIVER"), \
DBG_TEXT("HPI_CONTROL_LEVEL"), \
DBG_TEXT("HPI_CONTROL_TUNER"), \
DBG_TEXT("HPI_CONTROL_ONOFFSWITCH"), \
DBG_TEXT("HPI_CONTROL_VOX"), \
DBG_TEXT("HPI_CONTROL_AES18_TRANSMITTER"), \
DBG_TEXT("HPI_CONTROL_AES18_RECEIVER"), \
DBG_TEXT("HPI_CONTROL_AES18_BLOCKGENERATOR"), \
DBG_TEXT("HPI_CONTROL_CHANNEL_MODE"), \
DBG_TEXT("HPI_CONTROL_BITSTREAM"), \
DBG_TEXT("HPI_CONTROL_SAMPLECLOCK"), \
DBG_TEXT("HPI_CONTROL_MICROPHONE"), \
DBG_TEXT("HPI_CONTROL_PARAMETRIC_EQ"), \
DBG_TEXT("HPI_CONTROL_COMPANDER"), \
DBG_TEXT("HPI_CONTROL_COBRANET"), \
DBG_TEXT("HPI_CONTROL_TONE_DETECT"), \
DBG_TEXT("HPI_CONTROL_SILENCE_DETECT") \
}
#define NUM_CONTROL_STRINGS 24
#if ( NUM_CONTROL_STRINGS != ( HPI_CONTROL_LAST_INDEX + 1 ) )
#error DBG_TEXT("Control type strings don't match #defines")
#endif

#define HPI_SOURCENODE_STRINGS \
{ \
DBG_TEXT("no source"), \
DBG_TEXT("HPI_SOURCENODE_OSTREAM"), \
DBG_TEXT("HPI_SOURCENODE_LINEIN"), \
DBG_TEXT("HPI_SOURCENODE_AESEBU_IN"), \
DBG_TEXT("HPI_SOURCENODE_TUNER"), \
DBG_TEXT("HPI_SOURCENODE_RF"), \
DBG_TEXT("HPI_SOURCENODE_CLOCK_SOURCE"), \
DBG_TEXT("HPI_SOURCENODE_RAW_BITSTREAM"), \
DBG_TEXT("HPI_SOURCENODE_MICROPHONE"), \
DBG_TEXT("HPI_SOURCENODE_COBRANET") \
}
#define NUM_SOURCENODE_STRINGS 10
#if ( NUM_SOURCENODE_STRINGS != ( HPI_SOURCENODE_LAST_INDEX - HPI_SOURCENODE_BASE + 1 ) )
#error DBG_TEXT("Sourcenode strings don't match #defines")
#endif

#define HPI_DESTNODE_STRINGS \
{ \
DBG_TEXT("no destination"), \
DBG_TEXT("HPI_DESTNODE_ISTREAM"), \
DBG_TEXT("HPI_DESTNODE_LINEOUT"), \
DBG_TEXT("HPI_DESTNODE_AESEBU_OUT"), \
DBG_TEXT("HPI_DESTNODE_RF"), \
DBG_TEXT("HPI_DESTNODE_SPEAKER"), \
DBG_TEXT("HPI_DESTNODE_COBRANET") \
}
#define NUM_DESTNODE_STRINGS 7
#if ( NUM_DESTNODE_STRINGS != ( HPI_DESTNODE_LAST_INDEX - HPI_DESTNODE_BASE + 1 ) )
#error "Destnode strings don't match #defines"
#endif

#define HPI_CONTROL_CHANNEL_MODE_STRINGS \
{ \
DBG_TEXT("XXX HPI_CHANNEL_MODE_ERROR XXX"), \
DBG_TEXT("HPI_CHANNEL_MODE_NORMAL"), \
DBG_TEXT("HPI_CHANNEL_MODE_SWAP"), \
DBG_TEXT("HPI_CHANNEL_MODE_LEFT_ONLY"), \
DBG_TEXT("HPI_CHANNEL_MODE_RIGHT_ONLY") \
}

////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
#endif				/* _HPIDEBUG_H  */
