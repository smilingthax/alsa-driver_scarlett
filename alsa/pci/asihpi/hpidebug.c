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

int HPI_DebugLevelSet(int level)
{
  int old_level;

  old_level = hpiDebugLevel;
  hpiDebugLevel = level;
  return old_level;
}


#ifdef HPI_DEBUG_STRING_REQD
#include <stdarg.h>

void
hpi_debug_string(char *fmt, ...)
{
    va_list arglist;
    char buffer[256];

    va_start (arglist, fmt);
    vswprintf(buffer,fmt,arglist);
    if (buffer[0])
		HPIOS_DEBUG_STRING(buffer);
    va_end (ap);
}
#endif

static char *hpi_obj_strings[] = HPI_OBJ_STRINGS;

static char *hpi_subsys_strings[] = HPI_SUBSYS_STRINGS;

static char *hpi_adapter_strings[] = HPI_ADAPTER_STRINGS;

static char *hpi_ostream_strings[] = HPI_OSTREAM_STRINGS;

static char *hpi_istream_strings[] = HPI_ISTREAM_STRINGS;

static char *hpi_mixer_strings[] = HPI_MIXER_STRINGS;

static char *hpi_node_strings[] = {"NODE is invalid object"};

static char *hpi_control_strings[] = HPI_CONTROL_STRINGS;

static char *hpi_nvmemory_strings[] = HPI_NVMEMORY_STRINGS;

static char *hpi_digitalio_strings[] = HPI_DIGITALIO_STRINGS;

static char *hpi_watchdog_strings[] = HPI_WATCHDOG_STRINGS;

static char *hpi_clock_strings[] = HPI_CLOCK_STRINGS;

static char *hpi_profile_strings[] = HPI_PROFILE_STRINGS;

static char **hpi_function_strings[] =
{
  hpi_subsys_strings,
  hpi_adapter_strings,
  hpi_ostream_strings,
  hpi_istream_strings,
  hpi_mixer_strings,
  hpi_node_strings,
  hpi_control_strings,
  hpi_nvmemory_strings,
  hpi_digitalio_strings,
  hpi_watchdog_strings,
  hpi_clock_strings,
  hpi_profile_strings,
  hpi_control_strings  // controlX object
};


static char *
hpi_object_string(unsigned int object)
{
  if (object == 0 || object ==  HPI_OBJ_NODE
      || object > sizeof(hpi_obj_strings) / sizeof(hpi_obj_strings[0]))
    return "Invalid object";
  else
    return hpi_obj_strings[object - 1];
}


static char *
hpi_function_string(unsigned int function)
{
  int object;

  object = function / HPI_OBJ_FUNCTION_SPACING;
  function = function - object * HPI_OBJ_FUNCTION_SPACING;
  if (object == 0 || object ==  HPI_OBJ_NODE
      || object > sizeof(hpi_obj_strings) / sizeof(hpi_obj_strings[0]))
    return "Invalid object";
  if ((function >  HPI_OSTREAM_SET_TIMESCALE) || // max function number
      ( hpi_function_strings[object - 1][function - 1][0] != 'H'))
      return "Invalid function";

  /* Should check that have valid function number... */
  return hpi_function_strings[object - 1][function - 1];
}


void
hpi_debug_message(HPI_MESSAGE *phm)
{
    if (phm) {
	if ((phm->wObject <= HPI_OBJ_MAXINDEX ) && phm->wObject)
	HPI_DEBUG_STRING("Adap=%d, Obj = %s (%d), Func = %s (%d)\n",
			 phm->wAdapterIndex,
			 hpi_object_string(phm->wObject), phm->wObject,
			 hpi_function_string(phm->wFunction), phm->wFunction);
	else
	    HPI_DEBUG_STRING("Adap=%d, Invalid Obj = (%d), Func = (%d)\n",
			     phm->wAdapterIndex,phm->wObject, phm->wFunction);
    } else 
	HPI_DEBUG_STRING("NULL message pointer to hpi_debug_message!\n");
}


void
hpi_debug_response(HPI_RESPONSE *phr)
{
    if (phr->wError)
        HPI_DEBUG_STRING("Error (%d)\n", phr->wError);
    else {
	HPI_PRINT_VERBOSE("OK\n");
    }
}


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
	HPI_DEBUG_STRING("%08lx:", (HW32)(pdata + i));

	for (k = 0; k < cols && i < len; i++, k++)
	{
	  HPI_DEBUG_STRING("%s%04x", k == 0 ? "" : " ", pdata[i]);
	}
	HPI_DEBUG_STRING("\n");
    }
}

