/************************************************************************
Copyright (C) 1997-2003 AudioScience, Inc. All rights reserved.

This software is provided 'as-is', without any express or implied warranty.
In no event will AudioScience Inc. be held liable for any damages arising
from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This copyright notice and list of conditions may not be altered or removed 
   from any source distribution.

AudioScience, Inc. <support@audioscience.com>

( This license is GPL compatible see http://www.gnu.org/licenses/license-list.html#GPLCompatibleLicenses )

Debug macro translation.

************************************************************************/

#include "hpi.h"
#include "hpios.h"
#include "hpidebug.h"

/* Debug level; 0 quiet; 1 informative, 2 debug, 3 verbose debug.  */
int hpiDebugLevel = HPI_DEBUG_LEVEL_DEFAULT;

void HPI_DebugInit(void)
{
	#ifdef SGT_SERIAL_LOGGING
	HpiDebug_SerialLog_Init(); //SGT
	#endif
	HPIOS_DEBUG_PRINTF("Debug Start\n");
}

int HPI_DebugLevelSet(int level)
{
  int old_level;

  old_level = hpiDebugLevel;
  hpiDebugLevel = level;
  return old_level;
}

int HPI_DebugLevelGet(void)
{
	return(hpiDebugLevel);
}


#ifdef HPIOS_DEBUG_PRINT
/* implies OS has no printf-like function */
#include <stdarg.h>

void
hpi_debug_printf(char *fmt, ...)
{
    va_list arglist;
    char buffer[256];
    va_start (arglist, fmt);

    // the following may work only for Windows kernel?
    #ifdef HPI_OS_WDM
    _vsnprintf(buffer,80,fmt,arglist);
    #endif

    if (buffer[0])
		HPIOS_DEBUG_PRINT(buffer);
    va_end (arglist);
}
#endif

typedef struct {
	void *array;
	int numElements;
} treenode_t;

#define make_treenode_from_array( nodename, array ) \
static void *tmp_strarray_##nodename[] = array; \
static treenode_t nodename = { \
	&tmp_strarray_##nodename, \
	ARRAY_SIZE(tmp_strarray_##nodename) \
};

#define get_treenode_elem( node_ptr, idx, type )  ( &( *( (type *)(node_ptr)->array )[idx] ) )


make_treenode_from_array( hpi_subsys_strings, HPI_SUBSYS_STRINGS )
make_treenode_from_array( hpi_adapter_strings, HPI_ADAPTER_STRINGS )
make_treenode_from_array( hpi_istream_strings, HPI_ISTREAM_STRINGS )
make_treenode_from_array( hpi_ostream_strings, HPI_OSTREAM_STRINGS )
make_treenode_from_array( hpi_mixer_strings, HPI_MIXER_STRINGS )
make_treenode_from_array( hpi_node_strings, {"NODE is invalid object"} )
make_treenode_from_array( hpi_control_strings, HPI_CONTROL_STRINGS )
make_treenode_from_array( hpi_nvmemory_strings, HPI_OBJ_STRINGS )
make_treenode_from_array( hpi_digitalio_strings, HPI_DIGITALIO_STRINGS )
make_treenode_from_array( hpi_watchdog_strings, HPI_WATCHDOG_STRINGS )
make_treenode_from_array( hpi_clock_strings, HPI_CLOCK_STRINGS )
make_treenode_from_array( hpi_profile_strings, HPI_PROFILE_STRINGS )

#define HPI_FUNCTION_STRINGS \
{ \
  &hpi_subsys_strings,\
  &hpi_adapter_strings,\
  &hpi_ostream_strings,\
  &hpi_istream_strings,\
  &hpi_mixer_strings,\
  &hpi_node_strings,\
  &hpi_control_strings,\
  &hpi_nvmemory_strings,\
  &hpi_digitalio_strings,\
  &hpi_watchdog_strings,\
  &hpi_clock_strings,\
  &hpi_profile_strings,\
  &hpi_control_strings \
};

make_treenode_from_array( hpi_function_strings, HPI_FUNCTION_STRINGS )

#if 0
//Not used anywhere???

make_treenode_from_array( hpi_obj_strings, HPI_OBJ_STRINGS )

static char *
hpi_object_string(unsigned int object)
{
  if (object == 0 || object ==  HPI_OBJ_NODE
      || object > hpi_obj_strings.numElements )
    return "Invalid object";

    return get_treenode_elem( &hpi_obj_strings, object - 1, char* );
}
#endif

static char *
hpi_function_string(unsigned int function)
{
  int object;
  treenode_t *tmp;

  object = function / HPI_OBJ_FUNCTION_SPACING;
  function = function - object * HPI_OBJ_FUNCTION_SPACING;

  if (object == 0 || object ==  HPI_OBJ_NODE
      || object > hpi_function_strings.numElements )
    return "Invalid object";

  tmp = get_treenode_elem( &hpi_function_strings, object - 1, treenode_t* );

  if ( function == 0 || function > tmp->numElements )
    return "Invalid function";

  return get_treenode_elem( tmp, function - 1, char* );
}


void
hpi_debug_message(HPI_MESSAGE *phm)
{
	if (phm) {
		if ((phm->wObject <= HPI_OBJ_MAXINDEX ) && phm->wObject) {

			switch (phm->wObject) {
			case HPI_OBJ_OSTREAM:
			case HPI_OBJ_ISTREAM:
			case HPI_OBJ_CONTROLEX:
			case HPI_OBJ_CONTROL:
				HPIOS_DEBUG_PRINTF("Adapter #%d %s #%d \n",
					phm->wAdapterIndex,
					hpi_function_string(phm->wFunction),
					phm->u.c.wControlIndex); /* controlex, stream index at same offset */
				break;
			default:
				HPIOS_DEBUG_PRINTF("Adapter #%d %s \n",
					phm->wAdapterIndex,
					hpi_function_string(phm->wFunction));
				break;
			}
		} else {
			HPIOS_DEBUG_PRINTF("Adap=%d, Invalid Obj=%d, Func=%d\n",
				phm->wAdapterIndex,phm->wObject, phm->wFunction);
		}
	} else
		HPIOS_DEBUG_PRINTF("NULL message pointer to hpi_debug_message!\n");
}

#if 0
void
hpi_debug_response(HPI_RESPONSE *phr)
{
    if (phr->wError)
        HPIOS_DEBUG_PRINTF("Error %d\n", phr->wError);
    else {
		HPIOS_DEBUG_PRINTF("OK\n");
    }
}
#endif

void
hpi_debug_data(HW16 HUGE *pdata, HW32 len)
{
    int i;
    int j;
    int k;
    int lines;
    int cols = 8;

    lines = (len + cols - 1) / cols;
    if (lines > 8)
	lines = 8;

    for (i = 0, j = 0; j < lines; j++)
    {
    HPIOS_DEBUG_PRINTF(HPI_DEBUG_FLAG_VERBOSE "%p:", (pdata + i));

	for (k = 0; k < cols && i < len; i++, k++)
	{
	  HPIOS_DEBUG_PRINTF("%s%04x", k == 0 ? "" : " ", pdata[i]);
	}
	HPIOS_DEBUG_PRINTF("\n");
    }
}

#ifdef SGT_SERIAL_LOGGING
///////////////////////////////////////////////////////////////////////////////
// SGT
void HpiDebug_SerialLog_Init( void )
{
	#define COM1_PORT_ADDRESS 0x3F8
	#define MAX_STRING 40

 	int ch=0, i=0;
 	char sz[MAX_STRING];

 	HOUT8(COM1_PORT_ADDRESS + 1 , 0);   // Turn off interrupts - Port1
 	HOUT8(COM1_PORT_ADDRESS + 3 , 0x80);  // SET DLAB ON */
 	HOUT8(COM1_PORT_ADDRESS + 0 , 0x01);  // Set Baud rate - Divisor Latch Low Byte  = 115,200BPS
 	HOUT8(COM1_PORT_ADDRESS + 1 , 0x00);  // Set Baud rate - Divisor Latch High Byte
 	HOUT8(COM1_PORT_ADDRESS + 3 , 0x03);  // 8 Bits, No Parity, 1 Stop Bit
 	HOUT8(COM1_PORT_ADDRESS + 2 , 0xC7);  // FIFO Control Register
 	HOUT8(COM1_PORT_ADDRESS + 4 , 0x0B);  // Turn on DTR, RTS, and OUT2

	HOUT8(COM1_PORT_ADDRESS, '\r');  // Send Char to Serial Port
	HOUT8(COM1_PORT_ADDRESS, '\n');  // Send Char to Serial Port
	HOUT8(COM1_PORT_ADDRESS, '\n');  // Send Char to Serial Port
	HOUT8(COM1_PORT_ADDRESS, '*');  // Send Char to Serial Port
	HOUT8(COM1_PORT_ADDRESS, '\r');  // Send Char to Serial Port
	HOUT8(COM1_PORT_ADDRESS, '\n');  // Send Char to Serial Port
}

void HpiDebug_SerialLog_SendBuffer(char* pszBuffer)
{
	int i=0;
	unsigned char bLSR=0;

	/*
	for(i=0; i<80; i++)
	{
		if(pszBuffer[i]==0)
			break;
		do
		{
			bLSR = HINP8(COM1_PORT_ADDRESS+5);
		}while((bLSR & 0x20)!= 0x20);	// wait till bit5 is high

		HOUT8(COM1_PORT_ADDRESS+0, pszBuffer[i]);
	}
	// seems like the terminal s/w needs to see a \r\n to get a full LF/CR
	if(i>0)
		if(pszBuffer[i-1] == '\n')
			HOUT8(COM1_PORT_ADDRESS+0,'\r');
	*/
	do
	{
		bLSR = HINP8(COM1_PORT_ADDRESS+5);
	}while((bLSR & 0x20)!= 0x20);	// wait till bit5 is high - 16char FIFO buffer is empty

	for(i=0; i<14; i++)
	{
		if((pszBuffer[i]==0) || (pszBuffer[i]=='\n'))
			break;
		HOUT8(COM1_PORT_ADDRESS+0, pszBuffer[i]);
	}
	// seems like the terminal s/w needs to see a \r\n to get a full LF/CR
	if((pszBuffer[i]=='\n') || (i==14))
	{
		HOUT8(COM1_PORT_ADDRESS+0,'\r');
		HOUT8(COM1_PORT_ADDRESS+0,'\n');
	}
}
#endif

