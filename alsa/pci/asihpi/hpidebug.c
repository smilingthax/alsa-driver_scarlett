/************************************************************************

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

#ifdef HPIOS_DEBUG_PRINT
/* implies OS has no printf-like function */
#include <stdarg.h>

void hpi_debug_printf(char *fmt, ...)
{
	va_list arglist;
	char buffer[256];

	va_start(arglist, fmt);
	vswprintf(buffer, fmt, arglist);
	if (buffer[0])
		HPIOS_DEBUG_PRINT(buffer);
	va_end(ap);
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

make_treenode_from_array(hpi_subsys_strings, HPI_SUBSYS_STRINGS)
    make_treenode_from_array(hpi_adapter_strings, HPI_ADAPTER_STRINGS)
    make_treenode_from_array(hpi_istream_strings, HPI_ISTREAM_STRINGS)
    make_treenode_from_array(hpi_ostream_strings, HPI_OSTREAM_STRINGS)
    make_treenode_from_array(hpi_mixer_strings, HPI_MIXER_STRINGS)
    make_treenode_from_array(hpi_node_strings,
			     {
			     "NODE is invalid object"})

    make_treenode_from_array(hpi_control_strings, HPI_CONTROL_STRINGS)
    make_treenode_from_array(hpi_nvmemory_strings, HPI_OBJ_STRINGS)
    make_treenode_from_array(hpi_digitalio_strings, HPI_DIGITALIO_STRINGS)
    make_treenode_from_array(hpi_watchdog_strings, HPI_WATCHDOG_STRINGS)
    make_treenode_from_array(hpi_clock_strings, HPI_CLOCK_STRINGS)
    make_treenode_from_array(hpi_profile_strings, HPI_PROFILE_STRINGS)
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
    make_treenode_from_array(hpi_function_strings, HPI_FUNCTION_STRINGS)
#if 0
//Not used anywhere???
    make_treenode_from_array(hpi_obj_strings, HPI_OBJ_STRINGS)

static char *hpi_object_string(unsigned int object)
{
	if (object == 0 || object == HPI_OBJ_NODE
	    || object > hpi_obj_strings.numElements)
		return "Invalid object";

	return get_treenode_elem(&hpi_obj_strings, object - 1, char *);
}
#endif

static char *hpi_function_string(unsigned int function)
{
	int object;
	treenode_t *tmp;

	object = function / HPI_OBJ_FUNCTION_SPACING;
	function = function - object * HPI_OBJ_FUNCTION_SPACING;

	if (object == 0 || object == HPI_OBJ_NODE
	    || object > hpi_function_strings.numElements)
		return "Invalid object";

	tmp =
	    get_treenode_elem(&hpi_function_strings, object - 1, treenode_t *);

	if (function == 0 || function > tmp->numElements)
		return "Invalid function";

	return get_treenode_elem(tmp, function - 1, char *);
}

void hpi_debug_message(HPI_MESSAGE * phm)
{
	if (phm) {
		if ((phm->wObject <= HPI_OBJ_MAXINDEX) && phm->wObject) {
			HPIOS_DEBUG_PRINTF("Adapter #%d %s",
					   phm->wAdapterIndex,
					   hpi_function_string(phm->wFunction));
			switch (phm->wObject) {
			case HPI_OBJ_OSTREAM:
				HPIOS_DEBUG_PRINTF("#%d ",
						   phm->u.d.wOStreamIndex);
				break;
			case HPI_OBJ_ISTREAM:
				HPIOS_DEBUG_PRINTF("#%d ",
						   phm->u.d.wIStreamIndex);
				break;
			}
			HPIOS_DEBUG_PRINTF("\n");
		} else {
			HPIOS_DEBUG_PRINTF("Adap=%d, Invalid Obj=%d, Func=%d\n",
					   phm->wAdapterIndex, phm->wObject,
					   phm->wFunction);
		}
	} else
		HPIOS_DEBUG_PRINTF
		    ("NULL message pointer to hpi_debug_message!\n");
}

#if 0
void hpi_debug_response(HPI_RESPONSE * phr)
{
	if (phr->wError)
		HPIOS_DEBUG_PRINTF("Error %d\n", phr->wError);
	else {
		HPIOS_DEBUG_PRINTF("OK\n");
	}
}
#endif

void hpi_debug_data(u16 * pdata, u32 len)
{
	int i;
	int j;
	int k;
	int lines;
	int cols = 8;

	lines = (len + cols - 1) / cols;
	if (lines > 8)
		lines = 8;

	for (i = 0, j = 0; j < lines; j++) {
		HPIOS_DEBUG_PRINTF(HPI_DEBUG_FLAG_VERBOSE "%08x:",
				   (u32) (pdata + i));

		for (k = 0; k < cols && i < len; i++, k++) {
			HPIOS_DEBUG_PRINTF("%s%04x", k == 0 ? "" : " ",
					   pdata[i]);
		}
		HPIOS_DEBUG_PRINTF("\n");
	}
}
