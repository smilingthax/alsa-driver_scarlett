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

Hardware Programming Interface (HPI) functions

(C) Copyright AudioScience Inc. 1996,1997, 1998
*******************************************************************************/

#define HPI_GENERATE_FUNCTIONS 1
#include "hpi.h"
#include "hpicheck.h"

#ifndef HPI_KERNEL_MODE
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>		// for log10 in meter get level
#endif

// local prototypes

#define HPI_UNUSED(param) (void)param

void HPI_InitMessage(HPI_MESSAGE * phm, u16 wObject, u16 wFunction);
HPI_HANDLE HPI_IndexesToHandle3(const char cObject,	///< HPI_OBJ_* the type of object
				const u16 wAdapterIndex,	///< The Adapter index
				const u16 wObjectIndex,	///< The stream or control index, if used
				const u16 wDspIndex	///< The index of the DSP which implements the object
    );

HPI_HANDLE HPI_IndexesToHandle(const char cObject,	///< HPI_OBJ_* the type of object
			       const u16 wAdapterIndex,	///< The Adapter index
			       const u16 wObjectIndex	///< The stream or control index, if used
    );

void HPI_HandleToIndexes3(const HPI_HANDLE dwHandle, u16 * pwIndex1,
			  u16 * pwIndex2, u16 * pwIndex0);
char HPI_HandleObject(const HPI_HANDLE dwHandle);

#define HPI_HANDLETOINDEXES(h,i1,i2) if (h==0) return HPI_ERROR_INVALID_OBJ; else HPI_HandleToIndexes3(h,i1,i2,NULL)
#define HPI_HANDLETOINDEXES3(h,i1,i2,i0) if (h==0) return HPI_ERROR_INVALID_OBJ; else HPI_HandleToIndexes3(h,i1,i2,i0)

/* Note: Can't use memcpy or typecast because some fields have different offsets */
void HPI_FormatToMsg(HPI_MSG_FORMAT * pMF, HPI_FORMAT * pF)
{
	pMF->dwSampleRate = pF->dwSampleRate;
	pMF->dwBitRate = pF->dwBitRate;
	pMF->dwAttributes = pF->dwAttributes;
	pMF->wChannels = pF->wChannels;
	pMF->wFormat = pF->wFormat;
}

static void HPI_MsgToFormat(HPI_FORMAT * pF, HPI_MSG_FORMAT * pMF)
{
	pF->dwSampleRate = pMF->dwSampleRate;
	pF->dwBitRate = pMF->dwBitRate;
	pF->dwAttributes = pMF->dwAttributes;
	pF->wChannels = pMF->wChannels;
	pF->wFormat = pMF->wFormat;
	pF->wModeLegacy = 0;
	pF->wUnused = 0;
}

///////////////////////////////////////////////////////////////////////////
/**\defgroup subsys HPI Subsystem
\{
*/
static HPI_HSUBSYS ghSubSys;	// not really used

/** HPI Subsystem create.
* Creates, opens and initializes the audio subsystem. Must be called before other
* HPI functions are called.
* \return Pointer to HPI subsystem handle. Returns NULL on error.
*/
HPI_HSUBSYS *HPI_SubSysCreate(void)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

	memset(&ghSubSys, 0, sizeof(HPI_HSUBSYS));
#ifndef HPI_KERNEL_MODE		//----------------- not Win95,WinNT,WDM kernel

	if (HPI_DriverOpen())
#endif

	{
		HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_OPEN);
		HPI_Message(&hm, &hr);

		if (hr.wError == 0)
			return (&ghSubSys);
#ifndef HPI_KERNEL_MODE		//----------------- not Win95,WinNT,WDM kernel

		else
			HPI_DriverClose();
#endif

	}
	return (NULL);
}

/** HPI Subsystem free.
* Closes the HPI subsystem, freeing any resources allocated.  Must be the last HPI function called.
*/
void HPI_SubSysFree(HPI_HSUBSYS * phSubSys	///< Pointer to HPI subsystem handle.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

// tell HPI to shutdown
	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_CLOSE);
	HPI_Message(&hm, &hr);

#ifndef HPI_KERNEL_MODE		// not Win95,WinNT,WDM kernel

	HPI_DriverClose();
#endif
}

/** HPI subsystem get version.
* Returns the HPI subsystem major and minor versions that were embedded into the HPI module at compile time. On a
* Windows machine this version is embedded in the kernel driver .sys file.
* 
* \param pdwVersion 32 bit word containing version of HPI. Upper 24bits is major 
* version number and lower 8 bits is minor version number, i.e., 0x00000304 is 
* version 3.04.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_SubSysGetVersion(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			 u32 * pdwVersion)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_GET_VERSION);
	HPI_Message(&hm, &hr);
	*pdwVersion = hr.u.s.dwVersion;
	return (hr.wError);
}

/** HPI subsystem extended get version that returns Major, Minor and Build versions.
* Returns extended HPI subsystem version that was embedded into the HPI module at compile time. On a
* Windows machine this version is embedded in the kernel driver .sys file.
* 
* \param pdwVersionEx 32 bit word containing version of HPI.\n
*     B23..16 = Major version\n
*     B15..8 = Minor version\n
*     B7..0 = Build version\n
*     i.e. 0x00030402 is version 3.04.02
*
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_SubSysGetVersionEx(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			   u32 * pdwVersionEx)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_GET_VERSION);
	HPI_Message(&hm, &hr);
	*pdwVersionEx = hr.u.s.dwData;
	return (hr.wError);
}

/** \deprecated Use a combination of HPI_SubSysGetVersionEx() and HPI_SubSysFindAdapters() instead.
*/
u16 HPI_SubSysGetInfo(HPI_HSUBSYS * phSubSys,
		      u32 * pdwVersion,
		      u16 * pwNumAdapters, u16 awAdapterList[], u16 wListLength)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_GET_INFO);

	HPI_Message(&hm, &hr);

	*pdwVersion = hr.u.s.dwVersion;
	if (wListLength > HPI_MAX_ADAPTERS)
		memcpy(awAdapterList, &hr.u.s.awAdapterList, HPI_MAX_ADAPTERS);
	else
		memcpy(awAdapterList, &hr.u.s.awAdapterList, wListLength);
	*pwNumAdapters = hr.u.s.wNumAdapters;
	return (hr.wError);
}

/** Find all adapters that the HPI subsystem knows about.
*
* The type and adapter number of each adapter is returned in an array of u16 pointed to by 
* awAdapterList.  Each position in the array identifies an adapter with an adapter index of 
* the corresponding array index.  The value of the array indicates the adapter type.
* A value of zero indicates that no adapter exists for that adapter number.  
*
* For example if awAdapterList had a 6114 in position 0, a 0 in position 1 and a 6514 in 
* position 2, that would indicate an 6114 adapter set to adapter number 1 and a 6514 adapter 
* set to adapter number 3 in the system. 
* Note that the Adapter number (as set on the card/adapter) will be one more than the array index.
<table border=1 cellspacing=0 cellpadding=5>
<tr>
<td width=200><p><b>Index:</b></p></td>
<td width=80 align=center><p><b>0</b></p></td>
<td width=80 align=center><p><b>1</b></p></td>
<td width=80 align=center><p><b>2</b></p></td>
<td width=80 align=center><p><b>3</b></p></td>
</tr>
<tr>
<td width=200><p>awAdapterList</p></td>
<td width=80 align=center><p>0x6244</p></td>
<td width=80 align=center><p>0</p></td>
<td width=80 align=center><p>0x4346</p></td>
<td width=80 align=center><p>0</p></td>
</tr>
</table>
* 
* \return
* The return value is the last error that occurred when initializing all the adapters.  As there 
* is only one error return, when there are multiple adapters some of the adapters may have 
* initialized correctly and still be usable. This is indicated by wNumAdapters > 0.
* It is up to the application whether to continue or fail, but the user should be notified of the error in any case.
* Some possible return values are: \n
* HPI_DUPLICATE_ADAPTER_NUMBER - two adapters have the same jumper setting. Only the first one will be accessible.\n
* HPI_ERROR_DSP_BOOTLOAD - something went wrong with starting the adapter DSP.\n
* HPI_ERROR_DSP_FILE_NOT_FOUND - probably an old driver and a new card, or asidsp.bin is missing.\n
* Other errors in the 900 range are adapter specific.
*/
u16 HPI_SubSysFindAdapters(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			   u16 * pwNumAdapters,	///< Returned number of initialised adapters in the audio sub-system.
			   u16 awAdapterList[],	///< Array of adapter types (see description).
			   u16 wListLength	///< Size (in elements) of *pawAdapterList array.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_FIND_ADAPTERS);

	HPI_Message(&hm, &hr);

	if (wListLength > HPI_MAX_ADAPTERS) {
		memcpy(awAdapterList, &hr.u.s.awAdapterList,
		       HPI_MAX_ADAPTERS * sizeof(u16));
		memset(&awAdapterList[HPI_MAX_ADAPTERS], 0,
		       (wListLength - HPI_MAX_ADAPTERS) * sizeof(u16));
	} else
		memcpy(awAdapterList, &hr.u.s.awAdapterList,
		       wListLength * sizeof(u16));
	*pwNumAdapters = hr.u.s.wNumAdapters;

	return (hr.wError);
}

#ifndef HPI_EXCLUDE_IMPLEMENTATION
/* Internal functions follow. */
/** \internal
*/

/** Used by a PnP OS to create an adapter.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_SubSysCreateAdapter(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			    HPI_RESOURCE * pResource,	///< Pointer to the resources used by this adapter.
			    u16 * pwAdapterIndex	///< Returned index of the adapter that was just created.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_CREATE_ADAPTER);
	memcpy(&hm.u.s.Resource, pResource, sizeof(HPI_RESOURCE));

	HPI_Message(&hm, &hr);

	*pwAdapterIndex = hr.u.s.wAdapterIndex;
	return (hr.wError);
}

/** Used by a PnP OS to delete an adapter.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_SubSysDeleteAdapter(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			    u16 wAdapterIndex	///< Index of the adapter to delete.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_DELETE_ADAPTER);
	hm.wAdapterIndex = wAdapterIndex;
	HPI_Message(&hm, &hr);
	return (hr.wError);
}
#endif

///\}
///////////////////////////////////////////////////////////////////////////
/** \defgroup adapter Adapter

@{
*/

/** Opens an adapter for use.
* The adapter is specified by wAdapterIndex which corresponds to the adapter 
* index on the adapter hardware (set using jumpers or switch).
*
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_AdapterOpen(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
		    u16 wAdapterIndex	///< Index of adapter to be opened. Ranges from 0 to 15 and corresponds to the Adapter Index set on the adapter hardware.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_OPEN);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	return (hr.wError);

}

/** Closes the adapter associated with the wAdapterIndex.
*
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_AdapterClose(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
		     u16 wAdapterIndex	///< Index of adapter to be closed. Ranges from 0 to 15 and corresponds to the Adapter Index set on the adapter hardware.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_CLOSE);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/**     Given an object type HPI_OBJ_*, index, and adapter index,
*     determine if the object exists, and if so the dsp index of the object.
* The DSP index defines which DSP on the adapter has the specified object.
*     Implementation is non-trivial only for multi-DSP adapters.
*
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_AdapterFindObject(const HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			  u16 wAdapterIndex,	///< Index of adapter to search.
			  u16 wObjectType,	///< Type of object HPI_OBJ_*.
			  u16 wObjectIndex,	///< Index of object
			  u16 * pDspIndex	///< Return the index of the DSP containing the object.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

// contruct the HPI message from the function parameters
	HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_FIND_OBJECT);
	hm.wAdapterIndex = wAdapterIndex;
	hm.u.a.wAssertId = wObjectIndex;
	hm.u.a.wObjectType = wObjectType;

	HPI_Message(&hm, &hr);	// Find out where the object is
/* HPIs that serve only a single DSP can process this message on the PC, returning
0 in wDspIndex */

	if (hr.wError == 0)
		*pDspIndex = hr.u.a.wAdapterIndex;
	else if (hr.wError == HPI_ERROR_INVALID_FUNC) {
// for backwards compatibility
		*pDspIndex = 0;
		hr.wError = 0;
	}

	return hr.wError;
}

/** Sets the operating mode of an adapter. The adapter must be restarted for the mode 
* to take affect. Under Windows this means that the computer must be rebooted.
*
* Currently defined modes are:
<table border=1 cellspacing=0>
<tr>
<td><b>Mode</b></td>
<td><b>Description</b></td>
</tr>

<tr>
<td><small>HPI_ADAPTER_MODE_4OSTREAM</small></td>
<td><small>
ASI4401, 4 stereo/mono ostreams -> 1 stereo line out.<br>
ASI4215, 4 ostreams -> 4 line outs.<br>
ASI6114, 4 ostreams -> 4 line outs.
</small></td>
</tr>

<tr><td><small>HPI_ADAPTER_MODE_6OSTREAM</small></td>
<td><small>ASI4401, 6 mono ostreams -> 2 mono line outs.</small></td></tr>

<tr><td><small>HPI_ADAPTER_MODE_8OSTREAM</small></td>
<td><small>
ASI6114, 8 ostreams -> 4 line outs.<br>
ASI6118, 8 ostreams -> 8 line outs.
</small></td></tr>

<tr><td><small>HPI_ADAPTER_MODE_9OSTREAM</small></td>
<td><small>ASI6044, 9 ostreams -> 4 lineouts.</small></td></tr>

<tr><td><small>HPI_ADAPTER_MODE_12OSTREAM</small></td>
<td><small>ASI504X, 12 ostreams -> 4 lineouts.</small></td></tr>

<tr><td><small>HPI_ADAPTER_MODE_16OSTREAM</small></td>
<td><small>ASI6118, 16 ostreams -> 8 line outs.</small></td></tr>

<tr><td><small>HPI_ADAPTER_MULTICHANNEL</small></td>
<td><small>ASI504X, 2 ostreams -> 4 line outs (1 to 8 channel streams), 4 lineins -> 1 instream (1 to 8 channel streams) at 0-48kHz. For more info see the SSX Specification.</small></td></tr>

<tr><td><small>HPI_ADAPTER_MODE1</small></td>
<td><small>ASI504X, 12 ostream, 4 instream 0 to 48kHz sample rates (see ASI504X datasheet for more info).</small></td></tr>

<tr><td><small>HPI_ADAPTER_MODE2</small></td>
<td><small>ASI504X, 4 ostream, 4 instream 0 to 192kHz sample rates (see ASI504X datasheet for more info).</small></td></tr>

<tr><td><small>HPI_ADAPTER_MODE3</small></td>
<td><small>ASI504X, 4 ostream, 4 instream 0 to 192kHz sample rates (see ASI504X datasheet for more info).</small></td></tr>

</table>
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_AdapterSetMode(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
		       u16 wAdapterIndex,	///< Index of adapter to set mode on.
		       u32 dwAdapterMode	///< The adapter mode. See list of modes in the function description.
    )
{
	return HPI_AdapterSetModeEx(phSubSys, wAdapterIndex, dwAdapterMode,
				    HPI_ADAPTER_MODE_SET);
}

/** Adapter set mode extended. This updated version of adapter set mode supports.
*     querying supported modes.  A return value of 0 indicates success. 
* \param wQueryOrSet Controls whether the mode is being set or queried. Valid values are
* HPI_ADAPTER_MODE_SET or HPI_ADAPTER_MODE_QUERY. When a mode is being queried the function
* returns 0 if the mode is supported, otherwise HPI_ERROR_BAD_ADAPTER_MODE is returned.
* When wSetOrQuery is set to HPI_ADAPTER_MODE_SET the mode of the adapter is changed.
* The adapter must be restarted for the mode to take affect. Under Windows this means that 
* the computer must be rebooted.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs. HPI_ERROR_BAD_ADAPTER_MODE
* is returned if an unsupported mode is requested.
*/
u16 HPI_AdapterSetModeEx(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			 u16 wAdapterIndex,	///< Index of adapter to set mode on.
			 u32 dwAdapterMode,	///< The adapter mode. See list of modes in the function HPI_AdapterSetMode().
			 u16 wQueryOrSet)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_SET_MODE);
	hm.wAdapterIndex = wAdapterIndex;
	hm.u.a.dwAdapterMode = dwAdapterMode;
	hm.u.a.wAssertId = wQueryOrSet;
	HPI_Message(&hm, &hr);
	return (hr.wError);
}

/** Read the current mode setting.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_AdapterGetMode(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
		       u16 wAdapterIndex,	///< Index of adapter to get mode from.
		       u32 * pdwAdapterMode	///< The returned adapter mode. Will be one of HPI_ADAPTER_MODE_XXXX.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_GET_MODE);
	hm.wAdapterIndex = wAdapterIndex;
	HPI_Message(&hm, &hr);
	if (pdwAdapterMode)
		*pdwAdapterMode = hr.u.a.dwSerialNumber;
	return (hr.wError);
}

/** Obtains information about the specified adapter, including the number of output streams 
* and number of input streams, version, serial number and it's type. The adapter is assumed to have one mixer.
* \param pwVersion Adapter hardware and DSP software version information.
* The 16bit word contains the following information:<br>
<table border=1 cellspacing=0 cellpadding=5>
<tr>
<td><p><b>Bits</b></p></td>
<td><p><b>Description</b></p></td>
<td><p><b>Range</b></p></td>
</tr>
<tr>
<td><p>b15-13</p></td>
<td><p>DSP software major version</p></td>
<td><p>0..7</p></td>
</tr>
<tr>
<td><p>b12-b7</p></td>
<td><p>DSP software minor version</p></td>
<td><p>0..63</p></td>
</tr>
<tr>
<td><p>b6-b3</p></td>
<td><p>Adapter PCB revision</p></td>
<td><p>A..P represented as 0..15</p></td>
</tr>
<tr>
<td><p>b2-b0</p></td>
<td><p>Adapter assembly revision</p></td>
<td><p>0..7</p></td>
</tr>
</table>
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_AdapterGetInfo(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
		       u16 wAdapterIndex,	///< Index of adapter to read mode from.
		       u16 * pwNumOutStreams,	///< Number of output streams (play) on the adapter.
		       u16 * pwNumInStreams,	///< Number of input streams (record) on the adapter.
//u8   szAdapterName[],
//u16 wStringLen
		       u16 * pwVersion, u32 * pdwSerialNumber,	///< Adapter serial number.  Starts at 1 and goes up.
		       u16 * pwAdapterType	///< Adapter ID code, defined in HPI.H.  Examples are HPI_ADAPTER_ASI6114 (0x6114).
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_GET_INFO);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	*pwAdapterType = hr.u.a.wAdapterType;
	*pwNumOutStreams = hr.u.a.wNumOStreams;
	*pwNumInStreams = hr.u.a.wNumIStreams;
	*pwVersion = hr.u.a.wVersion;
	*pdwSerialNumber = hr.u.a.dwSerialNumber;
//strncpy( szAdapterName, hr.u.a.szAdapterName, wStringLen );
	return (hr.wError);
}

/*

OR

Gets various attributes of an adapter, such as serial number

u16 HPI_AdapterGetAttribute(
HPI_HSUBSYS *phSubSys,
u16    wAdapterIndex,      // 0..N-1
u16    wAttribute,         // HPI_ATTRIBUTE_SERIALNUMBER
u32    *pdwValue
)
{
}
*/

/** Returns DSP generated assert messages.
* Most ASI adapters have a small buffer that can collect up to 16 asserts
* that are conditionally generated as the DSP code is running. This API
* provides a mechanism for the host to read any asserts pending in the
* queue.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_AdapterGetAssert(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			 u16 wAdapterIndex,	///< Adpater index to read assert from.
			 u16 * wAssertPresent,	///< Set to 1 if an assert was returned, otherwise it returns 0.
			 char *pszAssert,	///< char buffer to contain the returned assert string. String should be declared as<br>char szAssert[STR_SIZE(HPI_STRING_LEN)].
			 u16 * pwLineNumber	///< The line number that caused the assert.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_GET_ASSERT);
	hm.wAdapterIndex = wAdapterIndex;
	HPI_Message(&hm, &hr);

// no assert
	*wAssertPresent = 0;

	if (!hr.wError) {
		*pwLineNumber = (u16) hr.u.a.dwSerialNumber;	// send line number back in the dwSerialNumber field
		if (*pwLineNumber) {
// we have an assert
			int i;
			char *Src = (char *)hr.u.a.szAdapterAssert;
			char *Dst = pszAssert;

			*wAssertPresent = 1;

// simple version of strncpy (saves linking to a library)
			for (i = 0; i < STR_SIZE(HPI_STRING_LEN); i++) {
				char c;
				c = *Src++;
				*Dst++ = c;
				if (c == 0)
					break;
			}

		}
	}
	return (hr.wError);
}

/** Extended Get Assert
* The extened assert interface adds 32 bit 'line number' and dsp 
* index to standard assert API.
* \todo Review whether this is actually implemented anywhere ?
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_AdapterGetAssertEx(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			   u16 wAdapterIndex,	///<  Adapter to query.
			   u16 * wAssertPresent,	///< OUT* The number of asserts waiting including this one.
			   char *pszAssert,	///< OUT* Assert message, traditionally file name.
			   u32 * pdwLineNumber,	///< OUT* Assert number, traditionally line number in file.
			   u16 * pwAssertOnDsp	///< OUT* The index of the DSP that generated the assert.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_GET_ASSERT);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	*wAssertPresent = 0;

	if (!hr.wError) {
		*pdwLineNumber = hr.u.a.dwSerialNumber;	// send line number back in the dwSerialNumber field
		*wAssertPresent = hr.u.a.wAdapterType;	// count of asserts BEFORE getting this one
		*pwAssertOnDsp = hr.u.a.wAdapterIndex;	// DSP where error occurred

		if (!*wAssertPresent && *pdwLineNumber)
			*wAssertPresent = 1;	// for backward compatibility. See also hpi4000.c HPI_4000()

		if (*wAssertPresent) {
// we have an assert
			int i;
			char *Src = (char *)hr.u.a.szAdapterAssert;
			char *Dst = pszAssert;

// simple version of strncpy (saves linking to a library)
			for (i = 0; i < STR_SIZE(HPI_STRING_LEN); i++) {
				char c;
				c = *Src++;
				*Dst++ = c;
				if (c == 0)
					break;
			}

		} else {
			*pszAssert = 0;
		}
	}
	return (hr.wError);
}

/** This function tests that asserts are working correctly on the selected adapter.
* The message processing code on the target adapter generates an assert when this
* function is called and that assert can then be read back using the HPI_AdapterGetAssert()
* function.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_AdapterTestAssert(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			  u16 wAdapterIndex,	///< Index of adapter to generate the test assert.
			  u16 wAssertId	///< An assert id number that is returned as the line number in HPI_AdapterGetAssert().
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_TEST_ASSERT);
	hm.wAdapterIndex = wAdapterIndex;
	hm.u.a.wAssertId = wAssertId;

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/** Turn on a particular adapter capability.
*
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_AdapterEnableCapability(HPI_HSUBSYS * phSubSys,
				u16 wAdapterIndex, u16 wCapability, u32 dwKey)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_ENABLE_CAPABILITY);
	hm.wAdapterIndex = wAdapterIndex;
	hm.u.a.wAssertId = wCapability;
	hm.u.a.dwAdapterMode = dwKey;

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

u16 HPI_AdapterSelfTest(HPI_HSUBSYS * phSubSys, u16 wAdapterIndex)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_SELFTEST);
	hm.wAdapterIndex = wAdapterIndex;
	HPI_Message(&hm, &hr);
	return (hr.wError);
}

/*! Sets the adapter property specified but the dwProperty field. As of driver version 2.95
the only property supported is HPI_ADAPTER_PROPERTY_ERRATA_1 on an ASI6xxx adapter.

HPI_ADAPTER_PROPERTY_ERRATA_1 supports correcting the CS4224 single sample delay on each Line Out
independently. The sample delay for each stereo line out is bitmapped into dwParamter1, i.e. bit0
applies to Line Out 0 and bit1 applies to Line Out 1 etc. dwParamter2 is unused for this property
and should be set to 0.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_AdapterSetProperty(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			   u16 wAdapterIndex,	///< Adapter index.
			   u16 wProperty,	///< Adapter property to set. One of HPI_ADAPTER_PROPERTY_*.
			   u16 wParameter1,	///< Adapter property parameter 1.
			   u16 wParameter2	///< Adapter property parameter 2.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_SET_PROPERTY);
	hm.wAdapterIndex = wAdapterIndex;
	hm.u.ax.property_set.wProperty = wProperty;
	hm.u.ax.property_set.wParameter1 = wParameter1;
	hm.u.ax.property_set.wParameter2 = wParameter2;

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/*! Gets the adapter property specified but the dwProperty field. As of driver version 2.95
the only property supported is HPI_ADAPTER_PROPERTY_ERRATA_1 on an ASI6xxx adapter.

HPI_ADAPTER_PROPERTY_ERRATA_1 supports correcting the CS4224 single sample delay on each Line Out
independently. The sample delay for each stereo line out is bitmapped into the returned pdwParamter1,
i.e. bit0 applies to Line Out 0 and bit1 applies to Line Out 1 etc. pdwParamter2 is unused for this
property and should be set to 0.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_AdapterGetProperty(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			   u16 wAdapterIndex,	///< Adapter index.
			   u16 wProperty,	///< Adapter property to set. One of HPI_ADAPTER_PROPERTY_*.
			   u16 * pwParameter1,	///< Returned adapter property parameter 1.
			   u16 * pwParameter2	///< Returned adapter property parameter 2.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_GET_PROPERTY);
	hm.wAdapterIndex = wAdapterIndex;
	hm.u.ax.property_set.wProperty = wProperty;

	HPI_Message(&hm, &hr);
	if (!hr.wError) {
		if (pwParameter1)
			*pwParameter1 = hr.u.ax.property_get.wParameter1;
		if (pwParameter2)
			*pwParameter2 = hr.u.ax.property_get.wParameter2;
	}

	return (hr.wError);
}

/*! Enumerates adapter properties. To be implemented sometime in the future.

This function allows an application to determine what property a particular adapter supports. Furthermore
the settings for a particular propery can also be established.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_AdapterEnumerateProperty(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
				 u16 wAdapterIndex,	///< Adapter index.
				 u16 wIndex,	///< Adapter property # to return.
				 u16 wWhatToEnumerate,	///< Either HPI_ADAPTER_PROPERTY_ENUMERATE_PROPERTIES or HPI_ADAPTER_PROPERTY_ENUMERATE_SETTINGS
				 u16 wPropertyIndex,	///< Property index.
				 u32 * pdwSetting	///< Returned adapter property, or property setting, depending on wWhatToEnumerate.
    )
{
	return 0;
}

/** @} */

/** \defgroup stream Streams
Perform audio I/O and format conversion
@{
*/

/** Given a format and rate that the buffer is processed, return the correct buffer
* size to support ping-pong buffering of audio.
*
* Calculate the minimum buffer size for a stream given the audio format that the stream 
* will be set to use and the rate at which the host polls the stream state and reads or 
* writes data. The buffer size returned by this function should be used as the minimum 
* value passed to HPI_OutStreamHostBufferAllocate() or HPI_InStreamHostBufferAllocate().
* If different formats and samplerates will be used on the stream, buffer size should be 
* calculated for the highest datarate format, or the buffer should be freed and allocated 
* again for each format change.
*
* Enabling background bus mastering (BBM) places some additional constraints on your application.
* In order to allow the BBM to catch up if the host app has got behind, it must be possible 
* to transfer data faster than it is being acquired.  This means that the buffer needs to 
* be bigger than minimum.  Recommended size is at least 2x minimum.  A buffer size of 
* Nx4096 makes the best use of memory.
*
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_StreamEstimateBufferSize(HPI_FORMAT * pFormat,	///< The format of the stream.
				 u32 dwHostPollingRateInMilliSeconds,	///< The polling rate of the host tread that fills or empties the buffer.
				 u32 * dwRecommendedBufferSize)	///< The recommended buffer size in milliseconds.
{
// compute bytes per second
	u32 dwBytesPerSecond;
	u32 dwSize;
	u16 wChannels;
	HPI_FORMAT *pF = pFormat;

	wChannels = pF->wChannels;

	switch (pF->wFormat) {
	case HPI_FORMAT_PCM16_BIGENDIAN:
	case HPI_FORMAT_PCM16_SIGNED:
		dwBytesPerSecond = pF->dwSampleRate * 2L * wChannels;
		break;
	case HPI_FORMAT_PCM24_SIGNED:
		dwBytesPerSecond = pF->dwSampleRate * 3L * wChannels;
		break;
	case HPI_FORMAT_PCM32_SIGNED:
	case HPI_FORMAT_PCM32_FLOAT:
		dwBytesPerSecond = pF->dwSampleRate * 4L * wChannels;
		break;
	case HPI_FORMAT_PCM8_UNSIGNED:
		dwBytesPerSecond = pF->dwSampleRate * 1L * wChannels;
		break;
	case HPI_FORMAT_MPEG_L1:
	case HPI_FORMAT_MPEG_L2:
	case HPI_FORMAT_MPEG_L3:
		dwBytesPerSecond = pF->dwBitRate / 8L;
		break;
	case HPI_FORMAT_DOLBY_AC2:
// Dolby AC-2 is around 256 kBytes/second
		dwBytesPerSecond = 256000L / 8L;
		break;
	default:
		return HPI_ERROR_INVALID_FORMAT;
	}
	dwSize =
	    (dwBytesPerSecond * dwHostPollingRateInMilliSeconds * 2) / 1000L;
	*dwRecommendedBufferSize = (dwSize + 4095L) & ~4095L;	// round up to nearest 4 K bounday.
	return 0;
}

////////////////////////////////////////////////////////////////////////////
/** \defgroup outstream Output Stream

The following section describes the states a stream uses.

\image html outstream_states.png

The state HPI_STATE_DRAINED indicates that the stream has run out of decoded data.

This can happen for two reasons:<br>
Intentionally, when the end of the currently playing audio is reached.<br>
A transient error condition when the adapter DSP was unable to decode audio fast enough.

The dwDataToPlay count returned from HPI_OutStreamGetInfoEx() measures the amount of encoded data (MPEG, PCM etc) in the stream's 
buffer.  When there is less than a whole frame of encoded data left, it cannot be decoded 
for mixing and output. It is possible that HPI_OutStreamGetInfoEx() indicates that there is still a small amount 
of data to play, but the stream enters the DRAINED state, and the amount of data to play
and never gets to zero.

The size of a frame varies depends on the audio format.
Compressed formats such as MPEG require whole frames of data in order to decode audio.  The 
size of the input frame depends on the samplerate and bitrate 
(e.g. MPEG frame_bytes = bitrate/8 * 1152/samplerate).  

AudioScience's implementation of PCM decoding also requires a minimum amount of input data. 
ASI4xxx adapters require 4608 bytes, whereas ASI6xxx adapters require 1536 bytes for stereo, 
half this for mono.

Conservative conditions to detect end of play<br>
Have done the final OutStreamWrite<br>
Stream state is HPI_STATE_DRAINED<br>
dwDataToPlay < 4608

Input data requirements for different algorithms.

<table border=1 cellspacing=0 cellpadding=5>
<tr>
<td><p><b>Bytes</b></p></td>
<td><p><b>PCM-16</b></p></td>
<td><p><b>MP1</b></p></td>
<td><p><b>MP2</b></p></td>
<td><p><b>MP3</b></p></td>
<td><p><b>AC-2</b></p></td>
</tr>
<tr>
<td>AX</td>
<td>4608</td>
<td><3456</td>
<td><3456</td>
<td>-</td>
<td>380</td>
</tr>
<tr>
<td>AX4</td>
<td>4608</td>
<td><3456</td>
<td><3456</td>
<td>3456</td>
<td>-</td>
</tr>
<tr>
<td>AX6</td>
<td>1536</td>
<td><=700</td>
<td><=700</td>
<td>1100</td>
<td>380</td>
</tr>
</table>

@{
*/

/** Open and initializes an output stream.  An Adapter index and OutStream index are 
* passed in and an OutStream handle is passed returned.  The handle is used for all 
* future calls to that OutStream. An output stream may only be open by only one 
* application at a time. 
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_OutStreamOpen(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
		      u16 wAdapterIndex,	///< Index of adapter to be opened. Ranges from 0 to 15 and corresponds to the Adapter Index set on the adapter hardware.
		      u16 wOutStreamIndex,	///< Index of the OutStream to be opened. Ranges from 0 to wNumOutStreams-1 (returned by HPI_AdapterGetInfo()).
		      HPI_HOSTREAM * phOutStream	///< Returned Handle to the OutStream.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_OPEN);
	hm.wAdapterIndex = wAdapterIndex;
	hm.u.d.wStreamIndex = wOutStreamIndex;

	HPI_Message(&hm, &hr);

	if (hr.wError == 0)
		*phOutStream =
		    HPI_IndexesToHandle(HPI_OBJ_OSTREAM, wAdapterIndex,
					wOutStreamIndex);
	else
		*phOutStream = 0;
	return (hr.wError);
}

/** Closes an output stream and deallocates host buffers if they are being used.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_OutStreamClose(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
		       HPI_HOSTREAM hOutStream	///< Handle of the OutStream to close.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_HOSTBUFFER_FREE);
	HPI_HANDLETOINDEXES(hOutStream, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);
	HPI_Message(&hm, &hr);

	hm.wFunction = HPI_OSTREAM_GROUP_RESET;
	HPI_Message(&hm, &hr);

	hm.wFunction = HPI_OSTREAM_CLOSE;
	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/**
\deprecated This function has been superseded by HPI_OutStreamGetInfoEx()
*/
u16 HPI_OutStreamGetInfo(HPI_HSUBSYS * phSubSys,
			 HPI_HOSTREAM hOutStream,
			 u16 * pwState,
			 u32 * pdwBufferSize, u32 * pdwDataToPlay)
{
	return (HPI_OutStreamGetInfoEx(phSubSys,
				       hOutStream,
				       pwState,
				       pdwBufferSize, pdwDataToPlay, NULL, NULL)
	    );
}

/** Get information about attributes and state of output stream.
* This is similar to HPI_OutStreamGetInfo but returns extended information 
* including the size of the streams buffer in pdwBufferSize. 
* It also returns whether the stream is currently playing (the state) and the amount 
* of audio data left to play.
*
* \param pdwSamplesPlayed The SamplesPlayed parameter returns the number of samples 
* played since the last HPI_OutStreamReset command was issued.  It reflects the number 
* of stereo samples for a stereo stream and the number of mono samples for a mono stream. 
* This means that if a 44.1kHz stereo and mono stream were both playing they would both 
* return SamplesPlayed=44100 after 1 second.
*
* \param pdwAuxiliaryDataToPlay AuxiliaryDataToPlay is only relevant when BBM is active 
* (see HPI_OutStreamHostBufferAllocate).  In this case DataToPlay refers to the host 
* side buffer state while AuxiliaryDataToPlay refers to the data remaining in the card's 
* own buffers.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_OutStreamGetInfoEx(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			   HPI_HOSTREAM hOutStream,	///< Handle to opened OutStream.
			   u16 * pwState,	///< State of stream = HPI_STATE_STOPPED,HPI_STATE_PLAYING or HPI_STATE_DRAINED.
			   u32 * pdwBufferSize,	///< Size (in bytes) of stream data buffer.
			   u32 * pdwDataToPlay,	///< Amount of data (in bytes) left in the buffer to play.
			   u32 * pdwSamplesPlayed, u32 * pdwAuxiliaryDataToPlay)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_GET_INFO);
	HPI_HANDLETOINDEXES(hOutStream, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);

	HPI_Message(&hm, &hr);

// only send back data if valid pointers supplied!!
	if (pwState)
		*pwState = hr.u.d.u.stream_info.wState;
	if (pdwBufferSize)
		*pdwBufferSize = hr.u.d.u.stream_info.dwBufferSize;
	if (pdwDataToPlay)
		*pdwDataToPlay = hr.u.d.u.stream_info.dwDataAvailable;
	if (pdwSamplesPlayed)
		*pdwSamplesPlayed = hr.u.d.u.stream_info.dwSamplesTransferred;
	if (pdwAuxiliaryDataToPlay)
		*pdwAuxiliaryDataToPlay =
		    hr.u.d.u.stream_info.dwAuxiliaryDataAvailable;
	return (hr.wError);
}

/** Writes a block of audio data to the specified output stream.
* The data to write is specified by pData. pData also contains the format of the data 
* (channels, compression, sampleRate).  The HPI_OutStreamWriteBuf() call copies the data in pbData array to the 
* output stream hardware.  Once the data has been written to the stream. i.e. after the HPI_OutStreamWriteBuf() call, 
* the memory used to hold that data may be reused.  
*
* \deprecated Use HPI_OutStreamWriteBuf() instead. This function may disappear from a 
* future version.
*
* A different format will only be accepted in the first write after the stream is opened 
* or reset.
*
* The size of the data block that may be written is limited to half the size of the 
* streams internal data buffer (specified by dwBufferSize returned by HPI_OutStreamGetInfo()).
*
* \image html outstream_buf.png
\code
wHE = HPI_FormatCreate(
&hpiFormat,
2,                 // stereo channel
HPI_FORMAT_MPEG_L2,// MPEG Layer II
44100L,            //sample rate
128000L,           //128k bits/sec
0                  // no attributes
);

wHE = HPI_DataCreate( &Data, &hpiFormat, gabBuffer, BLOCK_SIZE );
wHE = HPI_OutStreamWriteBuf( phSubSys, hOutStream, &aData, dwBytes, &hpiFormat);
\endcode
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_OutStreamWriteBuf(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			  HPI_HOSTREAM hOutStream,	///< Handle to opened OutStream.
			  u8 * pbData,	///< Pointer to buffer containing data to be written to the playback buffer.
			  u32 dwBytesToWrite,	///< Number of bytes to write, must be <= space available.
			  HPI_FORMAT * pFormat	///< Format structure describing the format of the data.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_WRITE);
	HPI_HANDLETOINDEXES(hOutStream, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);
	hm.u.d.u.Data.pbData = pbData;
	hm.u.d.u.Data.dwDataSize = dwBytesToWrite;

	HPI_FormatToMsg(&hm.u.d.u.Data.Format, pFormat);

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

#ifndef HPI_WITHOUT_HPI_DATA
/** Writes a block of audio data to the specified output stream.
* The data to write is specified by pData. pData also contains the format of the data 
* (channels, compression, sampleRate).  The HPI_OutStreamWrite() call copies the data in pData to the 
* output stream hardware.  Once the data has been written to the stream, the memory used to 
* hold that data may be reused.  
*
* \deprecated Use HPI_OutStreamWriteBuf() instead. This function may disappear from a 
* future version.
*
* A different format will only be accepted in the first write after the stream is opened 
* or reset.
*
* The size of the data block that may be written is limited to half the size of the 
* streams internal data buffer (specified by dwBufferSize returned by HPI_OutStreamGetInfo()).
*
* \image html outstream_buf.png
\code
wHE = HPI_FormatCreate(
&hpiFormat,
2,                 // stereo channel
HPI_FORMAT_MPEG_L2,// MPEG Layer II
44100L,            //sample rate
128000L,           //128k bits/sec
0                  // no attributes
);

wHE = HPI_DataCreate( &Data, &hpiFormat, gabBuffer, BLOCK_SIZE );
wHE = HPI_OutStreamWrite( phSubSys, hOutStream, &Data);
\endcode
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_OutStreamWrite(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
		       HPI_HOSTREAM hOutStream,	///< Handle to opened OutStream.
		       HPI_DATA * pData	///< Pointer to an HPI_DATA structure containing a description of the audio data to be written to the OutStream and played.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_WRITE);
	HPI_HANDLETOINDEXES(hOutStream, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);
	memcpy(&hm.u.d.u.Data, pData, sizeof(hm.u.d.u.Data));

	HPI_Message(&hm, &hr);

	return (hr.wError);
}
#endif

/** Starts an output stream playing audio data.  
* Data is taken from the circular buffer on the adapter hardware that has already been 
* partially filled by HPI_OutStreamWrite commands.  Audio is played from the current 
* position in the buffer (which may be reset using HPI_OutStreamReset).
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_OutStreamStart(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
		       HPI_HOSTREAM hOutStream	///< Handle to OutStream.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_START);
	HPI_HANDLETOINDEXES(hOutStream, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/** Stops an output stream playing audio data.
* The audio data buffer is not cleared, so a subsequent OutStreamStart will resume playing 
* at the position in the buffer where the playback had been stopped.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_OutStreamStop(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
		      HPI_HOSTREAM hOutStream	///< Handle to OutStream.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_STOP);
	HPI_HANDLETOINDEXES(hOutStream, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/** Generate a sinewave output on the specified stream.
* \note This function is unimplemented.
*/
u16 HPI_OutStreamSinegen(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			 HPI_HOSTREAM hOutStream	///< Handle to OutStream.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_SINEGEN);
	HPI_HANDLETOINDEXES(hOutStream, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/** Clears the audio data buffer of an output stream.  
* If the stream was playing at the time, it will be stopped.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_OutStreamReset(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
		       HPI_HOSTREAM hOutStream	///< Handle to OutStream.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_RESET);
	HPI_HANDLETOINDEXES(hOutStream, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/** Queries an OutStream to see whether it supports a certain audio format, 
* described in pFormat.  The result, returned in the error code, is 0 if 
* supported and HPI_ERROR_INVALID_FORMAT if not supported.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_OutStreamQueryFormat(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			     HPI_HOSTREAM hOutStream,	///< Handle to OutStream.
			     HPI_FORMAT * pFormat	///< Format structure containing the format to query.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_QUERY_FORMAT);
	HPI_HANDLETOINDEXES(hOutStream, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);

	HPI_FormatToMsg(&hm.u.d.u.Data.Format, pFormat);

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/** Sets the playback velocity for scrubbing. Velocity range is +/- 4.0. 
* nVelocity in set by
\code
nVelocity = (u16)(fVelocity * HPI_VELOCITY_UNITS);
\endcode
* where fVelocity in a floating point number in the range of -4.0 to +4.0.
* This call puts the stream in "scrub" mode. The first call to HPI_OutStreamSetVelocity() 
* should be made while the stream is reset so that scrubbing can be performed after 
* starting playback.
*
*\note <b>This functionality is only available on the ASI4300 series adapters.</b>
*
* <b>Call sequence</b>
*
* A typical playback call sequence without scrubbing is:
\verbatim
Write
Write
Start
Write
.....

Stop
\endverbatim
A typical playback sequence with scrubbing is:
\verbatim
Write
Write
SetVelocity
Start
Write
SetVelocity
.....
\endverbatim
*
* Stop - automatically turns of velocity/scrub mode.
*
* <b>Data flow</b>
*
* The scrubbing approach taken here is to decode audio to a "scrub buffer" that contains 
* many seconds of PCM that can be traversed in at a variable rate.
* \image html outstream_scrub.png
* Forward scrubbing does not have any limitations what-so-ever, apart from the maximum speed, 
* as specified by HPI_OutStreamSetVelocity(). 
*
* Reverse scrubbing operates under the following constraints:<br>
* 1) The user may not scrub on audio prior to the HPI_OutStreamStart( ) data point.<br>
* 2) The user may not reverse scrub further than -10 seconds from the forward most scrub position.<br>
* If either of these constraints is broken, the stream state will return HPI_STATE_DRAINED 
* and audio playback will cease. The user can then forward scrub again after this error 
* condition.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_OutStreamSetVelocity(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			     HPI_HOSTREAM hOutStream,	///< Handle to OutStream.
			     short nVelocity	///< The velocity in units HPI_VELOCITY_UNITS.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_SET_VELOCITY);
	HPI_HANDLETOINDEXES(hOutStream, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);
	hm.u.d.u.wVelocity = nVelocity;

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/** Set punch in and out points in a buffer.
* \note Currently unimplemented.
*/
u16 HPI_OutStreamSetPunchInOut(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			       HPI_HOSTREAM hOutStream,	///< Handle to OutStream.
			       u32 dwPunchInSample, u32 dwPunchOutSample)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_SET_PUNCHINOUT);
	HPI_HANDLETOINDEXES(hOutStream, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);

	hm.u.d.u.Pio.dwPunchInSample = dwPunchInSample;
	hm.u.d.u.Pio.dwPunchOutSample = dwPunchOutSample;

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/** Resets MPEG ancillary data extraction.
* Initializes the MPEG Layer II / III Ancillary data channel to support the extraction 
* of wBytesPerFrame bytes from the MPEG bitstream. 
*
* \note This call must be made after HPI_OutStreamOpen()
* or HPI_OutStreamReset and before the first HPI_OutStreamWrite() call.
*
* \param wMode The mode for the ancillary data extraction to operate in. Valid settings 
* are HPI_MPEG_ANC_RAW and HPI_MPEG_ANC_HASENERGY. The "RAW" mode indicates that the 
* entire ancillary data field is taken up by data from the Anc data buffer. The "HASENERGY" 
* option tells the decoder that the MPEG frames have energy information stored in them 
* (5 bytes per stereo frame, 3 per mono). 
*
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_OutStreamAncillaryReset(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
				HPI_HOSTREAM hOutStream,	///< Handle to OutStream.
				u16 wMode)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_ANC_RESET);
	HPI_HANDLETOINDEXES(hOutStream, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);
	hm.u.d.u.Data.Format.wChannels = wMode;
	HPI_Message(&hm, &hr);
	return (hr.wError);
}

/** Returns information about the Ancillary stream. 
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_OutStreamAncillaryGetInfo(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
				  HPI_HOSTREAM hOutStream,	///< Handle to OutStream.
				  u32 * pdwFramesAvailable	///< Number of HPI_ANC_FRAMEs in the hardware buffer available for reading.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_ANC_GET_INFO);
	HPI_HANDLETOINDEXES(hOutStream, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);
	HPI_Message(&hm, &hr);
	if (hr.wError == 0) {
		if (pdwFramesAvailable)
			*pdwFramesAvailable =
			    hr.u.d.u.stream_info.dwDataAvailable /
			    sizeof(HPI_ANC_FRAME);
	}
	return (hr.wError);
}

/** Reads frames of ancillary data from a stream's ancillary data buffer to pdwBuffer. 
* Note that in the situation where energy level information is embedded in the ancillary 
* data stream along with ancillary data, only the ancillary data will be returned in the 
* ancillary data buffer.
*
* Bytes are filled in the bData[] array of the HPI_ANC_FRAME structures in the following 
* order.
* 
* The first bit of ancillary information that follows the valid audio data is placed in 
* bit 7 of  bData[0]. The first 8 bits of ancillary information following valid audio 
* data are all placed in bData[0]. In the case where there are 6 bytes total of ancillary 
* information (48 bits) the last byte filled is bData[5].
* 
* \note If OutStreamAncillaryReset() was called with mode=HPI_MPEG_ANC_RAW, the ancillary 
* data in its entirety is placed in the AncFrameBuffer, so if the file was recorded with 
* energy information in the ancillary data field, the energy information will be included 
* in the extracted ancillary data.
* \note If OutStreamAncillaryReset() was called with mode=HPI_MPEG_ANC_HASENERGY, the 
* ancillary data minus the energy information is placed in the AncFrameBuffer.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_OutStreamAncillaryRead(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			       HPI_HOSTREAM hOutStream,	///< Handle to OutStream.
			       HPI_ANC_FRAME * pAncFrameBuffer,	///< A pointer to a buffer where the read Ancillary data frames should be placed.
			       u32 dwAncFrameBufferSizeInBytes,	///< The size of the Ancillary data buffer in bytes (used for a sanity check).
			       u32 dwNumberOfAncillaryFramesToRead	///< How many frames to read.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_ANC_READ);
	HPI_HANDLETOINDEXES(hOutStream, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);
	hm.u.d.u.Data.pbData = (u8 *) pAncFrameBuffer;
	hm.u.d.u.Data.dwDataSize =
	    dwNumberOfAncillaryFramesToRead * sizeof(HPI_ANC_FRAME);
	if (hm.u.d.u.Data.dwDataSize <= dwAncFrameBufferSizeInBytes)
		HPI_Message(&hm, &hr);
	else
		hr.wError = HPI_ERROR_INVALID_DATA_TRANSFER;
	return (hr.wError);
}

/** Sets the playback time scale with pitch and content preservation. 
* Range is 0.8-1.2 (80% to 120%) of original time.<br>
* dwTimeScale in set by:<br>
\code
dwTimeScale = (u16)(fTimeScale * HPI_OSTREAM_TIMESCALE_UNITS);
\endcode
* where fTimeScale in a floating point number in the range of 0.8 < fTimeScale  < 1.2.
* The actual granularity of this setting is 1 / 2048  or approximately 0.05% 
* (approx 5 HPI_OSTREAM_TIMESCALE_UNITS).
*
* The first call to HPI_OutStreamSetTimeScale should be made while the stream is reset 
* so that time scaling can be performed after starting playback.  Subsequent calls to 
* HPI_OutStreamSetTimeScale can be made when the stream is playing to modify the 
* timescale factor.
*
* \note This functionality is only available on ASI6000 series adapters.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_OutStreamSetTimeScale(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			      HPI_HOSTREAM hOutStream,	///< Handle to OutStream.
			      u32 dwTimeScale	///< The time scale in units of HPI_OSTREAM_TIMESCALE_UNITS.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_SET_TIMESCALE);
	HPI_HANDLETOINDEXES(hOutStream, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);

	hm.u.d.u.dwTimeScale = dwTimeScale;

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/** Allocates a buffer inside the driver for bus mastering transfers.
* Once the buffer is allocated, OutStream data will be transferred from it in the 
* background (rather than the application having to wait for the transfer).
*
* This function is provided so that the application can match the size of its transfers 
* to the size of the buffer.
*
* After a call to HPI_OutStreamHostBufferAllocate(), HPI_OutStreamGetInfoEx()  returns 
* the size and data available in host buffer rather than the buffers on the adapter. 
* However, while there is space in the adapter buffers, data will be quickly transferred 
* to the adapter, providing additional buffering against delays in sending more audio data.
*
* \note There is a minimum buffer size that will work with a given audio format and 
* polling rate. An appropriate size for the buffer can be calculated using HPI_StreamEstimateHostBufferSize().
*
* If an error occurs or the adapter doesn't support host buffering then no buffer is 
* allocated. Stream transfers still take place using foreground transfers (all drivers pre 2004).
* Performance will be relatively worse.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*     Special cases include HPI_ERROR_INVALID_DATASIZE if memory can't be 
* allocated (retrying the call with a smaller size may succeed) and HPI_ERROR_INVALID_OPERATION
* if virtual address of the allocated buffer can't be found.
*/
u16 HPI_OutStreamHostBufferAllocate(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
				    HPI_HOSTREAM hOutStream,	///< Handle to OutStream.
				    u32 dwSizeInBytes	///< Size of bus mastering buffer to allocate.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_HOSTBUFFER_ALLOC);
	HPI_HANDLETOINDEXES(hOutStream, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);
	hm.u.d.u.Data.dwDataSize = dwSizeInBytes;
	HPI_Message(&hm, &hr);
	return (hr.wError);
}

/** Free any buffers allocated by HPI_OutStreamHostBufferAllocate().
* \return 0 upon success\n
* HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_OutStreamHostBufferFree(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
				HPI_HOSTREAM hOutStream	///< Handle to OutStream.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_HOSTBUFFER_FREE);
	HPI_HANDLETOINDEXES(hOutStream, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);
	HPI_Message(&hm, &hr);
	return (hr.wError);
}

/** This function adds a stream to a group of streams. Stream groups are
* used to synchronise starting and stopping of multiple streams at once.
* The application of this is to support playing (or recording) multiple
* streams at once so as to support multi-channel recording and playback.
*
* When using the "Group" functions all streams that are to be grouped
* together should be opened. One of the streams should be selected to
* be the master stream and the other streams should be added to it's group.
* Both in streams and out streams can be added to the same group.
* Once a group has been formed, HPI_OutStreamStart() called on the master
* will cause all streams to start at once.
*
* \note This function is only supported on on ASI6000 and ASI5000 adapters.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_OutStreamGroupAdd(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			  HPI_HOSTREAM hOutStreamHandle,	///< Handle to OutStream.
			  HPI_HSTREAM hStreamHandle	///< Handle to either an in stream or an out stream.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	u16 wAdapter;
	u16 wDspIndex;
	char cObjType;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_GROUP_ADD);
	hr.wError = 0;
	HPI_HANDLETOINDEXES(hOutStreamHandle, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);
	cObjType = HPI_HandleObject(hStreamHandle);
	switch (cObjType) {
	case HPI_OBJ_OSTREAM:
		hm.u.d.u.Stream.wObjectType = HPI_OBJ_OSTREAM;
		HPI_HANDLETOINDEXES(hStreamHandle, &wAdapter,
				    &hm.u.d.u.Stream.wStreamIndex);
		break;
	case HPI_OBJ_ISTREAM:
		hm.u.d.u.Stream.wObjectType = HPI_OBJ_ISTREAM;
		HPI_HANDLETOINDEXES3(hStreamHandle, &wAdapter,
				     &hm.u.d.u.Stream.wStreamIndex, &wDspIndex);
		if (wDspIndex != 0)
			return HPI_ERROR_NO_INTERDSP_GROUPS;
		break;
	default:
		hr.wError = HPI_ERROR_INVALID_STREAM;
	}
	if (wAdapter != hm.wAdapterIndex)
		hr.wError = HPI_ERROR_NO_INTERADAPTER_GROUPS;
	if (hr.wError == 0)
		HPI_Message(&hm, &hr);
	return (hr.wError);
}

/** This function returns information about the streams that form a group.
* Given an out stream handle, it returns a bit mapped representation of which
* streams belong to the group.
* \param pdwOutStreamMap Bitmapped representation of out streams grouped with this 
* output stream. b0 represents outstream 0, b1 outstream 1, b2 outstream 2 etc.
* \param pdwInStreamMap Bitmapped representation of in streams grouped with this 
* output stream. b0 represents instream 0, b1 instream 1, b2 instream 2 etc.
* \note This function is only supported on on ASI6000 and ASI5000 adapters.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_OutStreamGroupGetMap(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			     HPI_HOSTREAM hOutStreamHandle,	///< Handle to OutStream.
			     u32 * pdwOutStreamMap, u32 * pdwInStreamMap)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_GROUP_GETMAP);
	HPI_HANDLETOINDEXES(hOutStreamHandle, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);
	HPI_Message(&hm, &hr);

	if (pdwOutStreamMap)
		*pdwOutStreamMap = hr.u.d.u.group_info.dwOutStreamGroupMap;
	if (pdwInStreamMap)
		*pdwInStreamMap = hr.u.d.u.group_info.dwInStreamGroupMap;

	return (hr.wError);
}

/** Resets stream grouping information for a given out stream.
* \note This function is only supported on on ASI6000 and ASI5000 adapters.
* \return 0 upon success, HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_OutStreamGroupReset(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			    HPI_HOSTREAM hOutStreamHandle	///< Handle to OutStream.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_GROUP_RESET);
	HPI_HANDLETOINDEXES(hOutStreamHandle, &hm.wAdapterIndex,
			    &hm.u.d.wStreamIndex);
	HPI_Message(&hm, &hr);
	return (hr.wError);
}

	  /** @} */// outstream
///////////////////////////////////////////////////////////////////////////
/** \defgroup instream Input Stream
@{
*/
/** Open and initializes an input stream.
* 
* An Adapter index and InStream index are passed in
* and an InStream handle is passed back.  The handle is used for all future calls to
* that InStream.  A particular input stream may only be open by one application at a time.
*
* \note A side effect of HPI_InStreamOpen() is that the following default ancillary data settings are made:
*     - Ancillary Bytes Per Frame = 0
*     - Ancillary Mode = HPI_MPEG_ANC_HASENERGY
*     - Ancillary Alignment = HPI_MPEG_ANC_ALIGN_RIGHT
*
* \return Error 0 upon success\n
* HPI_ERROR_XXX code if an error occurs
*/

u16 HPI_InStreamOpen(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
		     u16 wAdapterIndex,	///< Index of adapter to be accessed.  Ranges from 0 to 15 and corresponds to the Adapter Index set on the adapter hardware.
		     u16 wInStreamIndex,	///< Index of the InStream to be opened.  Ranges from 0 to wNumInStreams-1 (returned by HPI_AdapterGetInfo())
		     HPI_HISTREAM * phInStream	///< Returned handle to the opened InStream.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	u16 wDspIndex;
	HPI_UNUSED(phSubSys);

// only need to make this call for objects that can be distributed
	hr.wError = HPI_AdapterFindObject(phSubSys, wAdapterIndex,
					  HPI_OBJ_ISTREAM, wInStreamIndex,
					  &wDspIndex);

	if (hr.wError == 0) {
		HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_OPEN);
		hm.wDspIndex = wDspIndex;	// overloaded member
		hm.wAdapterIndex = wAdapterIndex;
		hm.u.d.wStreamIndex = wInStreamIndex;

		HPI_Message(&hm, &hr);

// construct a global (to the audio subsystem) handle from the adapter,DSP
// and stream index
		if (hr.wError == 0)
			*phInStream =
			    HPI_IndexesToHandle3(HPI_OBJ_ISTREAM, wAdapterIndex,
						 wInStreamIndex, wDspIndex);
		else
			*phInStream = 0;
	} else {
		*phInStream = 0;
	}
	return (hr.wError);
}

/**
* Closes an input stream. Deallocates allocated host buffer.
*
* \return Error 0 upon success\n
* HPI_ERROR_XXX code if an error occurs
*/
u16 HPI_InStreamClose(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle
		      HPI_HISTREAM hInStream	///< Handle to the InStream to close
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_HOSTBUFFER_FREE);
	HPI_HANDLETOINDEXES3(hInStream, &hm.wAdapterIndex, &hm.u.d.wStreamIndex,
			     &hm.wDspIndex);
	HPI_Message(&hm, &hr);

	hm.wFunction = HPI_ISTREAM_GROUP_RESET;
	HPI_Message(&hm, &hr);

	hm.wFunction = HPI_ISTREAM_CLOSE;
	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/**
* Queries an input stream to see whether it supports a certain audio format, described in pFormat.
* 
* \return Error 0 if supported\n
* HPI_ERROR_INVALID_FORMAT if not supported.
*/

u16 HPI_InStreamQueryFormat(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle
			    HPI_HISTREAM hInStream,	///< Handle to the InStream to close
			    HPI_FORMAT * pFormat	///< Not listed in 3.7.12 of Spchpi.doc
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

// contruct the HPI message from the function parameters
	HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_QUERY_FORMAT);
	HPI_HANDLETOINDEXES3(hInStream, &hm.wAdapterIndex, &hm.u.d.wStreamIndex,
			     &hm.wDspIndex);
	HPI_FormatToMsg(&hm.u.d.u.Data.Format, pFormat);

	HPI_Message(&hm, &hr);	// send the message to all the HPIs

	return (hr.wError);
}

/**
* Sets the recording format for an input stream.
*
* The format to set is described by pFormat.
*
* \return Error code 0 if supported\n
*HPI_ERROR_INVALID_FORMAT if not supported.
*
* \note Code example in Spchpi.doc.  Need to figure out how to format.
* Also need to add 3.7.13.1.
* Also need to add 3.7.14.
*/

u16 HPI_InStreamSetFormat(HPI_HSUBSYS * phSubSys,
			  HPI_HISTREAM hInStream, HPI_FORMAT * pFormat)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

// contruct the HPI message from the function parameters
	HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_SET_FORMAT);
	HPI_HANDLETOINDEXES3(hInStream, &hm.wAdapterIndex, &hm.u.d.wStreamIndex,
			     &hm.wDspIndex);
	HPI_FormatToMsg(&hm.u.d.u.Data.Format, pFormat);

	HPI_Message(&hm, &hr);	// send the message to all the HPIs

	return (hr.wError);
}

/** Read data from an InStream into a buffer
*/
/**
* Reads a block of audio data from the specified InStream. The memory data buffer to read into is specified by pData. 
* The HPI will copy the data in the InStream hardware buffer to memory buffer pointed to by pData->pbData.
*
* The size of the data block that may be read may be any size up to the amount of data available in the hardware buffer
* specified by dwDataAvailable returned by HPI_InStreamGetInfo() ).
* \par Diagram
* Need to add diagram
*
* \param *phSubSys Pointer to HPI subsystem handle.
* \param hInStream Handle to the InStream to close
* \param pbData Pointer to an HPI_DATA structure containing a description of the audio data to be read from the InStream. 
* \param dwBytesToRead Number of bytes to read, must be <= number available
*
* \return Error 0 upon success\n
*      HPI_ERROR_XXX code if an error occurs
*
*/
u16 HPI_InStreamReadBuf(HPI_HSUBSYS * phSubSys,	///< subsystem handle
			HPI_HISTREAM hInStream,	///< InStream handle
			u8 * pbData,	///< Pointer to buffer for read data
			u32 dwBytesToRead	///< Number of bytes to read, must be <= number available
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

// contruct the HPI message from the function parameters
	HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_READ);
	HPI_HANDLETOINDEXES3(hInStream, &hm.wAdapterIndex, &hm.u.d.wStreamIndex,
			     &hm.wDspIndex);
	hm.u.d.u.Data.dwDataSize = dwBytesToRead;
	hm.u.d.u.Data.pbData = pbData;

	HPI_Message(&hm, &hr);	// send the message to all the HPIs

	return (hr.wError);
}

#ifndef HPI_WITHOUT_HPI_DATA
/** Read data to a buffer described by HPI_DATA

Note that the format part of HPI_DATA is ignored.

\deprecated Use HPI_InStreamReadBuf() instead. This function may disappear from a future version.
*/

/**
* Reads a block of audio data from the specified InStream. The memory data buffer to read into is specified by pData.
* The HPI will copy the data in the InStream hardware buffer to memory buffer pointed to by pData->pbData. 
*
* The size of the data block that may be read may be any size up to the amount of data available in the hardware
* buffer (specified by dwDataAvailable returned by HPI_InStreamGetInfo()).
*
* \par Diagram
* Need to add diagram
*
* \return Error        0 upon success\n
*      HPI_ERROR_XXX code if an error occurs
*/
u16 HPI_InStreamRead(HPI_HSUBSYS * phSubSys,	///< subsystem handle
		     HPI_HISTREAM hInStream,	///< InStream handle
		     HPI_DATA * pData	///< Pointer to buffer for read data
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

// contruct the HPI message from the function parameters
	HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_READ);
	HPI_HANDLETOINDEXES3(hInStream, &hm.wAdapterIndex, &hm.u.d.wStreamIndex,
			     &hm.wDspIndex);
	memcpy(&hm.u.d.u.Data, pData, sizeof(hm.u.d.u.Data));

	HPI_Message(&hm, &hr);	// send the message to all the HPIs

	return (hr.wError);
}
#endif

/**
* Starts an input stream recording audio data.  Audio data is written into the adapters hardware buffer using the currently selected audio format
*(channels, sample rate, compression format etc.).  Data is then read from the buffer using HPI_InStreamRead().
*
* \return Error 0 upon success\n
* HPI_ERROR_XXX code if an error occurs
*/

u16 HPI_InStreamStart(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle
		      HPI_HISTREAM hInStream	///< InStream handle
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

// contruct the HPI message from the function parameters
	HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_START);
	HPI_HANDLETOINDEXES3(hInStream, &hm.wAdapterIndex, &hm.u.d.wStreamIndex,
			     &hm.wDspIndex);

	HPI_Message(&hm, &hr);	// send the message to all the HPIs

	return (hr.wError);
}

/**
* Stops an input stream recording audio data.  The audio data buffers is not cleaared, so a subsequent InStreamStart will resume recording at the
* position in the buffer where the record had been stopped.
* Second paragraph
*
* \return Error 0 upon success\n
* HPI_ERROR_XXX code if an error occurs
*/

u16 HPI_InStreamStop(HPI_HSUBSYS * phSubSys,	///< subsystem handle
		     HPI_HISTREAM hInStream	///< InStream handle
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

// contruct the HPI message from the function parameters
	HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_STOP);
	HPI_HANDLETOINDEXES3(hInStream, &hm.wAdapterIndex, &hm.u.d.wStreamIndex,
			     &hm.wDspIndex);

	HPI_Message(&hm, &hr);	// send the message to all the HPIs

	return (hr.wError);
}

/**
* Clears the audio data buffer of an input stream.  If the stream was recording at the time, it will be stopped.
*
* \return  Error 0 upon success\n
* HPI_ERROR_XXX code if an error occurs
*/

u16 HPI_InStreamReset(HPI_HSUBSYS * phSubSys,	///< subsystem handle
		      HPI_HISTREAM hInStream	///< InStream handle
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

// contruct the HPI message from the function parameters
	HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_RESET);
	HPI_HANDLETOINDEXES3(hInStream, &hm.wAdapterIndex, &hm.u.d.wStreamIndex,
			     &hm.wDspIndex);

	HPI_Message(&hm, &hr);	// send the message to all the HPIs

	return (hr.wError);
}

/**
\deprecated This function has been superseded by HPI_InStreamGetInfoEx()
*/

/**
\deprecated This function has been superseded by HPI_InStreamGetInfoEx()
*
* Returns information about the input stream.
*
*This includes the size of the stream’s hardware buffer returned in pdwBufferSize. 
* Also includes whether the stream is currently recording (the state) and the amount of audio data currently contained in the buffer.
*/

u16 HPI_InStreamGetInfo(HPI_HSUBSYS * phSubSys,	///< subsystem handle
			HPI_HOSTREAM hOutStream,	///< InStream handle
			u16 * pwState,	///< State of stream = HPI_STATE_STOPPED or HPI_STATE_RECORDING
			u32 * pdwBufferSize,	///< Size (in bytes) of stream data buffer
			u32 * pdwDataRecorded	///< Amount of data (in bytes) contained in the hardware buffer.
    )
{
	return (HPI_InStreamGetInfoEx(phSubSys,
				      hOutStream,
				      pwState,
				      pdwBufferSize,
				      pdwDataRecorded, NULL, NULL)
	    );
}

/** Returns extended information about the input stream.
*
* This includes the size of the stream’s hardware buffer returned in pdwBufferSize.  Also includes whether the
* stream is currently recording (the state), the amount of audio data currently contained in the buffer and how many samples have been recorded.
*
* \param pdwSamplesRecorded The SamplesRecorded parameter returns the number of samples recorded since the
* last HPI_InStreamReset command was issued.  It reflects the number of stereo samples for a stereo stream and the
* number of mono samples for a mono stream.  This means that if a 44.1kHz stereo and mono stream were both recording
* they would both return SamplesRecorded=44100 after 1 second.
*
* \param pdwDataRecorded DataRecorded returns the amount of data available to be read back in the next
* HPI_InStreamRead call. When BBM is active this is the data in the host buffer, otherwise it is the amount in the on-card buffer.
*
* \param pdwAuxiliaryDataRecorded AuxiliaryDataRecorded is only valid when BBM is being used (see HPI_InStreamHostBufferAllocate).
In BBM mode it returns the amount of data left in the on-card buffers.  This can be used to determine if a record overrun has occurred
*  (both BBM and card buffers full), or at the end of recording to ensure that all recorded data has been read.

*/

u16 HPI_InStreamGetInfoEx(HPI_HSUBSYS * phSubSys,	///< subsystem handle
			  HPI_HISTREAM hInStream,	///< InStream handle
			  u16 * pwState,	///< State of stream = HPI_STATE_STOPPED or HPI_STATE_RECORDING
			  u32 * pdwBufferSize,	///< Sixe (in bytes) of stream data buffer
			  u32 * pdwDataRecorded,	///< Amount of data (in bytes) contained in the hardware buffer.
			  u32 * pdwSamplesRecorded,	///< Number of samples recorded since an HPI_InStreamReset was last issued
			  u32 * pdwAuxiliaryDataRecorded	///< Amount of data in on-card buffers when BBM is active
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_GET_INFO);
	HPI_HANDLETOINDEXES3(hInStream, &hm.wAdapterIndex, &hm.u.d.wStreamIndex,
			     &hm.wDspIndex);

	HPI_Message(&hm, &hr);

// only send back data if valid pointers supplied!!
	if (pwState)
		*pwState = hr.u.d.u.stream_info.wState;
	if (pdwBufferSize)
		*pdwBufferSize = hr.u.d.u.stream_info.dwBufferSize;
	if (pdwDataRecorded)
		*pdwDataRecorded = hr.u.d.u.stream_info.dwDataAvailable;
	if (pdwSamplesRecorded)
		*pdwSamplesRecorded = hr.u.d.u.stream_info.dwSamplesTransferred;
	if (pdwAuxiliaryDataRecorded)
		*pdwAuxiliaryDataRecorded =
		    hr.u.d.u.stream_info.dwAuxiliaryDataAvailable;
	return (hr.wError);
}

/**
* Initializes the MPEG Layer II / III Ancillary data channel to support the embedding of wBytesPerFrame bytes into the MPEG bitstream.
*
* See the below table for the relationship between wBytesPerFrame and the datarate on the ancillary data channel.
*
* \note Need to add diagram, ancillary data table, and Notes1-3.
*
* \param wBytesPerFrame This variable specifies the rate at which data is inserted into the Ancillary data channel.
* Note that when (wMode== HPI_MPEG_ANC_HASENERGY, see below) an additional 5 bytes per frame are
* automatically allocated for energy information.
* \param wMode The mode for the ancillary data extraction to operate in. Valid settings are HPI_MPEG_ANC_RAW and
* HPI_MPEG_ANC_HASENERGY. The “RAW” mode indicates that the entire ancillary data field is taken up by data from the
* Anc data buffer. The “HASENERGY” option tells the encoder that the MPEG frames have energy information stored in them
* (5 bytes per stereo frame, 3 per mono). The encoder will insert the energy bytes before filling the remainder of the
* ancillary data space with data from the ancillary data buffer.
* \param wAlignment HPI_MPEG_ANC_ALIGN_LEFT – the wBytesPerFrame data immediately follow the audio data.
* Spare space is left at the end of the frame.
* HPI_MPEG_ANC_ALIGN_RIGHT – the wBytesPerFrame data is packed against the end of the frame. Spare space is left at the start of the frame.
* HPI_MPEG_ANC_ALIGN_FILL – all ancillary data space in the frame is used. wBytesPerFrame or more data is written per frame. There is no spare space.
*This parameter is ignored for MP3 encoding, effectively it is fixed at HPI_MPEG_ANC_ALIGN_FILL  (See Note 2)
* \param wIdleBit This field tells the encoder what to set all the bits of the ancillary data field to in the case where there is no data waiting to be inserted.
* Valid values are 0 or 1.  This parameter is ignored for MP3 encoding, if no data is available, no data will be inserted  (See Note 2)
* 
* \return Error 0 upon success\n
* HPI_ERROR_XXX code if an error occurs
*/

u16 HPI_InStreamAncillaryReset(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle
			       HPI_HISTREAM hInStream,	///< Handle for InStream
			       u16 wBytesPerFrame, u16 wMode,	// = HPI_MPEG_ANC_XXX
			       u16 wAlignment, u16 wIdleBit)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_ANC_RESET);
	HPI_HANDLETOINDEXES3(hInStream, &hm.wAdapterIndex, &hm.u.d.wStreamIndex,
			     &hm.wDspIndex);
/*
* Format structure packing
* dwAttributes = wBytesPerFrame
* wFormat = wMode
* wMode = wIdleBit
*/
	hm.u.d.u.Data.Format.dwAttributes = wBytesPerFrame;
	hm.u.d.u.Data.Format.wFormat = (wMode << 8) | (wAlignment & 0xff);
	hm.u.d.u.Data.Format.wChannels = wIdleBit;
	HPI_Message(&hm, &hr);
	return (hr.wError);
}

/**
* Returns information about the ancillary data stream. 
*/

u16 HPI_InStreamAncillaryGetInfo(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle
				 HPI_HISTREAM hInStream,	///< Handle to the InStream
				 u32 * pdwFrameSpace)	///< Maximum number of ancillary data frames that can be written to the anc frame buffer
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_ANC_GET_INFO);
	HPI_HANDLETOINDEXES3(hInStream, &hm.wAdapterIndex, &hm.u.d.wStreamIndex,
			     &hm.wDspIndex);
	HPI_Message(&hm, &hr);
	if (pdwFrameSpace)
		*pdwFrameSpace =
		    (hr.u.d.u.stream_info.dwBufferSize -
		     hr.u.d.u.stream_info.dwDataAvailable) /
		    sizeof(HPI_ANC_FRAME);
	return (hr.wError);
}

/**
* Writes dwNumberOfAncDataFramesToWrite frames from pAncFrameBuffer to the stream’s ancillary data buffer.
*
*The first bit of ancillary information that follows the valid audio data is bit 7 of  bData[0]. The first 8 bits of ancillary information following
* valid audio data are from bData[0]. In the case where there are 6 bytes total of ancillary information (48 bits) the last byte inserted in the frame is bData[5].
*/

u16 HPI_InStreamAncillaryWrite(HPI_HSUBSYS * phSubSys,
			       HPI_HISTREAM hInStream,
			       HPI_ANC_FRAME * pAncFrameBuffer,
			       u32 dwAncFrameBufferSizeInBytes,
			       u32 dwNumberOfAncillaryFramesToWrite)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_ANC_WRITE);
	HPI_HANDLETOINDEXES3(hInStream, &hm.wAdapterIndex, &hm.u.d.wStreamIndex,
			     &hm.wDspIndex);
	hm.u.d.u.Data.pbData = (u8 *) pAncFrameBuffer;
	hm.u.d.u.Data.dwDataSize =
	    dwNumberOfAncillaryFramesToWrite * sizeof(HPI_ANC_FRAME);
	if (hm.u.d.u.Data.dwDataSize <= dwAncFrameBufferSizeInBytes)
		HPI_Message(&hm, &hr);
	else
		hr.wError = HPI_ERROR_INVALID_DATA_TRANSFER;
	return (hr.wError);
}

/**
* Assuming no error:
* Allocates a buffer on the host PC for bus mastering transfers.  Once the buffer is allocated, InStream data will be transferred
* to it in the background. (rather than the application having to wait for the transfer)
* This function is provided so that the application can match the size of its transfers to the size of the buffer.
*
* From now on, HPI_InStreamGetInfoEx() will return the size and data available in host buffer rather than the buffers on the adapter.
* However, if the host buffer is allowed to fill up, the adapter buffers will then begin to fill up. In other words you still have the benefit
* of the larger adapter buffers
* \note There is a minimum buffer size that will work with a given audio format and polling rate. An appropriate size
* for the buffer can be calculated using HPI_StreamEstimateHostBufferSize()
*
* \return Error 0 upon success\n
* HPI_ERROR_INVALID_DATASIZE if memory can’t be allocated (retrying the call with a smaller size may succeed)\n
* HPI_ERROR_INVALID_OPERATION if virtual address of buffer can’t be found.
*/
u16 HPI_InStreamHostBufferAllocate(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle
				   HPI_HISTREAM hInStream,	///< Handle of the InStream
				   u32 dwSizeInBytes)	///< Size of bus mastering buffer to allocate
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_HOSTBUFFER_ALLOC);
	HPI_HANDLETOINDEXES3(hInStream, &hm.wAdapterIndex, &hm.u.d.wStreamIndex,
			     &hm.wDspIndex);
	hm.u.d.u.Data.dwDataSize = dwSizeInBytes;
	HPI_Message(&hm, &hr);
	return (hr.wError);
}

/**
* Free any buffers allocated by HPI_InStreamHostBufferAllocate
*
* \return Error 0 upon success\n
*      HPI_ERROR_INVALID_FUNC if the function is not implemented
*/
u16 HPI_InStreamHostBufferFree(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle
			       HPI_HISTREAM hInStream)	///< Handle of the InStream
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_HOSTBUFFER_FREE);
	HPI_HANDLETOINDEXES3(hInStream, &hm.wAdapterIndex, &hm.u.d.wStreamIndex,
			     &hm.wDspIndex);
	HPI_Message(&hm, &hr);
	return (hr.wError);
}

u16 HPI_InStreamGroupAdd(HPI_HSUBSYS * phSubSys,
			 HPI_HISTREAM hInStreamHandle,
			 HPI_HSTREAM hStreamHandle)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	u16 wAdapter;
	u16 wDspIndex;
	char cObjType;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_GROUP_ADD);
	hr.wError = 0;
	HPI_HANDLETOINDEXES3(hInStreamHandle, &hm.wAdapterIndex,
			     &hm.u.d.wStreamIndex, &hm.wDspIndex);
	cObjType = HPI_HandleObject(hStreamHandle);
	switch (cObjType) {
	case HPI_OBJ_OSTREAM:
		hm.u.d.u.Stream.wObjectType = HPI_OBJ_OSTREAM;
		HPI_HANDLETOINDEXES(hStreamHandle, &wAdapter,
				    &hm.u.d.u.Stream.wStreamIndex);
		break;
	case HPI_OBJ_ISTREAM:
		hm.u.d.u.Stream.wObjectType = HPI_OBJ_ISTREAM;
		HPI_HANDLETOINDEXES3(hStreamHandle, &wAdapter,
				     &hm.u.d.u.Stream.wStreamIndex, &wDspIndex);
		if (wDspIndex != hm.wDspIndex)
			return HPI_ERROR_NO_INTERDSP_GROUPS;
		break;
	default:
		hr.wError = HPI_ERROR_INVALID_STREAM;
	}
	if (wAdapter != hm.wAdapterIndex)
		hr.wError = HPI_ERROR_NO_INTERADAPTER_GROUPS;
	if (hr.wError == 0)
		HPI_Message(&hm, &hr);
	return (hr.wError);
}

u16 HPI_InStreamGroupGetMap(HPI_HSUBSYS * phSubSys,
			    HPI_HISTREAM hInStreamHandle,
			    u32 * pdwOutStreamMap, u32 * pdwInStreamMap)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_HOSTBUFFER_FREE);
	HPI_HANDLETOINDEXES3(hInStreamHandle, &hm.wAdapterIndex,
			     &hm.u.d.wStreamIndex, &hm.wDspIndex);
	HPI_Message(&hm, &hr);

	if (pdwOutStreamMap)
		*pdwOutStreamMap = hr.u.d.u.group_info.dwOutStreamGroupMap;
	if (pdwInStreamMap)
		*pdwInStreamMap = hr.u.d.u.group_info.dwInStreamGroupMap;

	return (hr.wError);
}

u16 HPI_InStreamGroupReset(HPI_HSUBSYS * phSubSys, HPI_HISTREAM hInStreamHandle)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_GROUP_RESET);
	HPI_HANDLETOINDEXES3(hInStreamHandle, &hm.wAdapterIndex,
			     &hm.u.d.wStreamIndex, &hm.wDspIndex);
	HPI_Message(&hm, &hr);
	return (hr.wError);
}

	  /** @} */// Instream
	  /** @} */// Streams

///////////////////////////////////////////////////////////////////////////
/** \defgroup mixer Mixer and Controls

@{
*/

/**
*Open and initializes an adapters mixer. 
*
* An Adapter index is passed in and a Mixer handle is passed back.  The handle is used for all future calls to that Mixer.
* A particular mixer may only be open by one application at one time.
*
* \return 0 upon success\n
*HPI_ERROR_XXX code if an error occurs
*/

u16 HPI_MixerOpen(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle
		  u16 wAdapterIndex,	///< Index of adapter to be opened.  Ranges from 0 to 15 and corresponds to the Adapter Index set on the adapter hardware.
		  HPI_HMIXER * phMixerHandle	///< Handle to the Mixer
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_MIXER, HPI_MIXER_OPEN);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	if (hr.wError == 0)
		*phMixerHandle =
		    HPI_IndexesToHandle(HPI_OBJ_MIXER, wAdapterIndex, 0);
	else
		*phMixerHandle = 0;
	return (hr.wError);
}

/**
* Closes a mixer.
*
* \return 0 upon success\n
* HPI_ERROR_XXX code if an error occurs
*/

u16 HPI_MixerClose(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle
		   HPI_HMIXER hMixerHandle	///< Handle to the adapter mixer to close
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_MIXER, HPI_MIXER_CLOSE);
	HPI_HANDLETOINDEXES(hMixerHandle, &hm.wAdapterIndex, NULL);
	HPI_Message(&hm, &hr);
	return (hr.wError);
}

/**
* Gets a mixer control.
* The control maybe located in one of three places:\n
* 1 - On a connection between a source node and a destination node.
* You specify both source and destination nodes (via type and type index).
* 2 - On a source node.
*You specify the source node and leave the destination node type and index as 0.
*3 - On a destination node.
*You specify the destination node and leave the source node type and index as 0.
*
* \note Not all adapters have controls at all nodes, or between all nodes.  Consult the Mixer Map for your particular
*adapter to find out the location and type of controls in its mixer.
* \example Say you have an audio adapter with a mixer that has the following layout: 
* (Need to add diagram.)
* You can see that the mixer has two meter controls, located on each of the Outstream source nodes, two volume controls, located between
* the OutStream sources and the LineOut destination nodes and one level control located on the LineOut destination node.
*
* You would use the following code to obtain a handle to the volume control that lies on the connection between OutStream#1 and LineOut#0:
* (Need to add code block.)
*
* You would use the following code to obtain a handle to the level control that lies on the LineOut#0 node:
* (Need to add code block.)
*
*\return 0 upon success\n
* HPI_ERROR_XXX code if an error occurs
*/

u16 HPI_MixerGetControl(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle
			HPI_HMIXER hMixerHandle,	///< Mixer handle
			u16 wSrcNodeType,	///< Source node type i.e. HPI_SOURCENODE_OSTREAM
			u16 wSrcNodeTypeIndex,	///< Index of particular source node type i.e. the 2nd HPI_SOURCENODE_OSTREAM would be specified as index=1
			u16 wDstNodeType,	///< Destination node type i.e. HPI_DESTNODE_LINEOUT
			u16 wDstNodeTypeIndex,	///< Index of particular source node type i.e. the 3rd HPI_DESTNODE_LINEOUT would be specified as index=2
			u16 wControlType,	// HPI_CONTROL_METER, _VOLUME etc///< Type of mixer control i.e. HPI_CONTROL_METER,  VOLUME, METER or LEVEL. See additional HPI_CONTROL_xxxx types defined in HPI.H
			HPI_HCONTROL * phControlHandle	///< Handle to the particular control
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_MIXER, HPI_MIXER_GET_CONTROL);
	HPI_HANDLETOINDEXES(hMixerHandle, &hm.wAdapterIndex, NULL);
	hm.u.m.wNodeType1 = wSrcNodeType;
	hm.u.m.wNodeIndex1 = wSrcNodeTypeIndex;
	hm.u.m.wNodeType2 = wDstNodeType;
	hm.u.m.wNodeIndex2 = wDstNodeTypeIndex;
	hm.u.m.wControlType = wControlType;

	HPI_Message(&hm, &hr);

// each control in an adapter/mixer has a unique index.
	if (hr.wError == 0)
		*phControlHandle =
		    HPI_IndexesToHandle(HPI_OBJ_CONTROL, hm.wAdapterIndex,
					hr.u.m.wControlIndex);
	else
		*phControlHandle = 0;
	return (hr.wError);
}

/** Get the location and type of a mixer control by index.
To enumerate all the mixer controls of an adapter, iterate wControlIndex
from 0 until the function returns HPI_ERROR_INVALID_OBJ_INDEX

A control may exist between two nodes, or on a single node in which case
either source or destination node type is zero.

\return HPI_ERROR_*
\retval HPI_ERROR_INVALID_OBJ_INDEX when wControlIndex > number of mixer controls
*/
u16 HPI_MixerGetControlByIndex(HPI_HSUBSYS * phSubSys,	///<  HPI subsystem handle
			       HPI_HMIXER hMixerHandle,	///<  Mixer Handle
			       u16 wControlIndex,	///<  Control Index to query
			       u16 * pwSrcNodeType,	///< [out] Source node type
			       u16 * pwSrcNodeIndex,	///< [out] Source ndoe index
			       u16 * pwDstNodeType,	///< [out] Destination node type
			       u16 * pwDstNodeIndex,	///< [out] Destination node index
			       u16 * pwControlType,	///< [out] Control Type
			       HPI_HCONTROL * phControlHandle	///< [out] Control handle
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_MIXER, HPI_MIXER_GET_CONTROL_BY_INDEX);
	HPI_HANDLETOINDEXES(hMixerHandle, &hm.wAdapterIndex, NULL);
	hm.u.m.wControlIndex = wControlIndex;
	HPI_Message(&hm, &hr);

	if (pwSrcNodeType) {	/* return all or none of info fields */
		*pwSrcNodeType = hr.u.m.wSrcNodeType + HPI_SOURCENODE_BASE;
		*pwSrcNodeIndex = hr.u.m.wSrcNodeIndex;
		*pwDstNodeType = hr.u.m.wDstNodeType + HPI_DESTNODE_BASE;
		*pwDstNodeIndex = hr.u.m.wDstNodeIndex;
		*pwControlType = hr.u.m.wControlIndex;	/* Actually Type */
	}
// each control in an adapter/mixer has a unique index.
	if (phControlHandle) {
		if (hr.wError == 0)
			*phControlHandle =
			    HPI_IndexesToHandle(HPI_OBJ_CONTROL,
						hm.wAdapterIndex,
						wControlIndex);
		else
			*phControlHandle = 0;
	}
	return (hr.wError);
}

/** Execute a command on the Mixer Control store
Valid commands are members of HPI_MIXER_STORE_COMMAND

\return HPI_ERROR_*
\retval HPI_ERROR_INVALID_OBJ_INDEX when wControlIndex > number of mixer controls
*/
u16 HPI_MixerStore(HPI_HSUBSYS * phSubSys,	///<  HPI subsystem handle
		   HPI_HMIXER hMixerHandle,	///<  Mixer Handle
		   enum HPI_MIXER_STORE_COMMAND command,	///< Command to execute
		   u16 wIndex	///< Optional index
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_MIXER, HPI_MIXER_STORE);
	HPI_HANDLETOINDEXES(hMixerHandle, &hm.wAdapterIndex, NULL);
	hm.u.mx.store.wCommand = command;
	hm.u.mx.store.wIndex = wIndex;
	HPI_Message(&hm, &hr);
	return (hr.wError);
}

/////////////////////////////////////////////////////////////////////////
// MIXER CONTROLS
/** General function for setting common control parameters
Still need specific code to set analog values
*/
u16 HPI_ControlParamSet(const HPI_HSUBSYS * phSubSys,
			const HPI_HCONTROL hControlHandle,
			const u16 wAttrib,
			const u32 dwParam1, const u32 dwParam2)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_SET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = wAttrib;
	hm.u.c.dwParam1 = dwParam1;
	hm.u.c.dwParam2 = dwParam2;
	HPI_Message(&hm, &hr);
	return (hr.wError);
}
static u16 HPI_ControlExParamSet(const HPI_HSUBSYS * phSubSys,
				 const HPI_HCONTROL hControlHandle,
				 const u16 wAttrib,
				 const u32 dwParam1, const u32 dwParam2)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROLEX, HPI_CONTROL_SET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.cx.wControlIndex);
	hm.u.cx.wAttribute = wAttrib;
	hm.u.cx.u.generic.dwParam1 = dwParam1;
	hm.u.cx.u.generic.dwParam2 = dwParam2;
	HPI_Message(&hm, &hr);
	return (hr.wError);
}

/** General function for getting up to 2 u32 return values
*/
u16 HPI_ControlParamGet(const HPI_HSUBSYS * phSubSys,
			const HPI_HCONTROL hControlHandle,
			const u16 wAttrib,
			u32 dwParam1,
			u32 dwParam2, u32 * pdwParam1, u32 * pdwParam2)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = wAttrib;
	hm.u.c.dwParam1 = dwParam1;
	hm.u.c.dwParam2 = dwParam2;
	HPI_Message(&hm, &hr);
	if (pdwParam1)
		*pdwParam1 = hr.u.c.dwParam1;
	if (pdwParam2)
		*pdwParam2 = hr.u.c.dwParam2;

	return (hr.wError);
}
static u16 HPI_ControlExParamGet(const HPI_HSUBSYS * phSubSys,
				 const HPI_HCONTROL hControlHandle,
				 const u16 wAttrib,
				 u32 dwParam1,
				 u32 dwParam2, u32 * pdwParam1, u32 * pdwParam2)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROLEX, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.cx.wControlIndex);
	hm.u.cx.wAttribute = wAttrib;
	hm.u.cx.u.generic.dwParam1 = dwParam1;
	hm.u.cx.u.generic.dwParam2 = dwParam2;
	HPI_Message(&hm, &hr);
	if (pdwParam1)
		*pdwParam1 = hr.u.cx.u.generic.dwParam1;
	if (pdwParam2)
		*pdwParam2 = hr.u.cx.u.generic.dwParam2;

	return (hr.wError);
}

#if 1
#define HPI_ControlParam1Get(s,h,a,p1) HPI_ControlParamGet(s,h,a,0,0,p1,NULL)
#define HPI_ControlParam2Get(s,h,a,p1,p2) HPI_ControlParamGet(s,h,a,0,0,p1,p2)
#define HPI_ControlExParam1Get(s,h,a,p1) HPI_ControlExParamGet(s,h,a,0,0,p1,NULL)
#define HPI_ControlExParam2Get(s,h,a,p1,p2) HPI_ControlExParamGet(s,h,a,0,0,p1,p2)
#else
u16 HPI_ControlParam2Get(const HPI_HSUBSYS * phSubSys,
			 const HPI_HCONTROL hControlHandle,
			 const u16 wAttrib, u32 * pdwParam1, u32 * pdwParam2)
{
	return HPI_ControlParamGet(phSubSys, hControlHandle, wAttrib, 0, 0,
				   pdwParam1, pdwParam2);
}

u16 HPI_ControlParam1Get(const HPI_HSUBSYS * phSubSys,
			 const HPI_HCONTROL hControlHandle,
			 const u16 wAttrib, u32 * pdwParam1)
{
	return HPI_ControlParamGet(phSubSys, hControlHandle, wAttrib, 0, 0,
				   pdwParam1, 0L);
}
#endif

/** Get the possible settings of an attribute of a mixer control given its index.

This is done without disturbing the current setting of the control.
Do this by iterating dwIndex from 0 until the function returns HPI_ERROR_INVALID_OBJ_INDEX.

For example, to determine which bands are supported by a particular tuner, do the following:
\code
for (dwIndex=0; dwIndex<10; dwIndex++) {
wErr= HPI_ControlQuery(phSS,hC, HPI_TUNER_BAND, dwIndex, 0 , AvailableBands[dwIndex]);
if (wErr !=0) break;
}
numBands=dwIndex;
\endcode

For attributes that have a range, 3 values will be returned for indices 0 to 2: minimum, maximum and step.
The supplementary parameter dwParam is used where the possible settings for one attribute depend on the setting of another attribute, e.g. a tuners frequency range depends on which band is selected.
For example, to determine the frequency range of the AM band, do the following:

\code
wErr= HPI_ControlQuery(phSS,hC, HPI_TUNER_FREQ, 0, HPI_TUNER_BAND_AM , pdwMinFreq);
wErr= HPI_ControlQuery(phSS,hC, HPI_TUNER_FREQ, 1, HPI_TUNER_BAND_AM , pdwMaxFreq);
wErr= HPI_ControlQuery(phSS,hC, HPI_TUNER_FREQ, 2, HPI_TUNER_BAND_AM , pdwFreqStep);
\endcode

<table>
<caption>Supported attributes</caption>
<tr><td>Associated set function</td>         <td>wAttrib           </td>              <td>dwParam</td>            <td>*dwSetting</td></tr>
<tr><td>HPI_Tuner_SetBand()</td>             <td>HPI_TUNER_BAND</td>              <td>0</td>                  <td>HPI_TUNER_BAND_*</td></tr>
<tr><td>HPI_Tuner_SetFrequency()</td>        <td>HPI_TUNER_FREQ</td>              <td>HPI_TUNER_BAND_*</td>   <td>Range in kHz</td></tr>
<tr><td>HPI_Tuner_SetGain()</td>             <td>HPI_TUNER_GAIN</td>              <td>0</td>                  <td>Range in milliBels</td></tr>
<tr><td>HPI_SampleClock_SetSource()</td>     <td>HPI_SAMPLECLOCK_SOURCE</td>      <td>0</td>                  <td>HPI_SAMPLECLOCK_SOURCE_*</td></tr>
<tr><td>HPI_SampleClock_SetSourceIndex()</td><td>HPI_SAMPLECLOCK_SOURCE_INDEX</td><td>HPI_SAMPLECLOCK_SOURCE_*</td><td>a source index</td></tr>
<tr><td>HPI_SampleClock_SetSampleRate() </td><td>HPI_SAMPLECLOCK_SAMPLERATE</td>  <td>0</td>                  <td> List or range of frequencies in Hz</td></tr>
</table>
*/
u16 HPI_ControlQuery(const HPI_HSUBSYS * phSubSys, const HPI_HCONTROL hControlHandle, const u16 wAttrib,	///< A control attribute
		     const u32 dwIndex,	///< Index for possible attribute values
		     const u32 dwParam,	///< Supplementary parameter
		     u32 * pdwSetting	///< [out] One of N possible settings for the control attribute, specified by dwIndex=0..N-1(and possibly depending on the value of the supplementary parameter)
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_INFO);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);

	hm.u.c.wAttribute = wAttrib;
	hm.u.c.dwParam1 = dwIndex;
	hm.u.c.dwParam2 = dwParam;

	HPI_Message(&hm, &hr);
	if (pdwSetting)
		*pdwSetting = hr.u.c.dwParam1;

	return (hr.wError);
}

/////////////////////////////////////////////////////////////////////////
/** \defgroup aesrx AES/EBU Digital audio receiver controls
\{
*/

/** set whether to input from the professional AES/EBU input
or the consumer S/PDIF input
*/
u16 HPI_AESEBU_Receiver_SetSource(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wSource	//  HPI_AESEBU_SOURCE_AESEBU
//  HPI_AESEBU_SOURCE_SPDIF
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle, HPI_AESEBU_SOURCE,
				   wSource, 0);
}

u16 HPI_AESEBU_Receiver_GetSource(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 * pwSource	//  HPI_AESEBU_SOURCE_AESEBU
//  HPI_AESEBU_SOURCE_SPDIF
    )
{
	u16 wErr;
	u32 dwParam;

	wErr =
	    HPI_ControlParam1Get(phSubSys, hControlHandle, HPI_AESEBU_SOURCE,
				 &dwParam);
	if (!wErr && pwSource)
		*pwSource = (u16) dwParam;

	return wErr;
}

/* get the sample rate of the current AES/EBU input
* Returns HPI_ERROR_INVALID_OPERATION if PLL unlocked.
*/
u16 HPI_AESEBU_Receiver_GetSampleRate(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u32 * pdwSampleRate	//0, 32000,44100 or 48000 returned
    )
{
	return HPI_ControlParam1Get(phSubSys, hControlHandle,
				    HPI_AESEBU_SAMPLERATE, pdwSampleRate);
}

/// get a byte of user data from the AES/EBU stream
u16 HPI_AESEBU_Receiver_GetUserData(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wIndex,	// ranges from 0..3
				    u16 * pwData	// returned user data
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_AESEBU_USERDATA;
	hm.u.c.dwParam1 = wIndex;

	HPI_Message(&hm, &hr);

	if (pwData)
		*pwData = (u16) hr.u.c.dwParam2;
	return (hr.wError);
}

///get a byte of channel status from the AES/EBU stream
u16 HPI_AESEBU_Receiver_GetChannelStatus(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wIndex,	// ranges from 0..23
					 u16 * pwData	// returned channel status data
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_AESEBU_CHANNELSTATUS;
	hm.u.c.dwParam1 = wIndex;

	HPI_Message(&hm, &hr);

	if (pwData)
		*pwData = (u16) hr.u.c.dwParam2;
	return (hr.wError);
}

/** get error status from the AES/EBU stream

*pwErrorData:    bit0: 1 when PLL is not locked
bit1: 1 when signal quality is poor
bit2: 1 when there is a parity error
bit3: 1 when there is a bi-phase coding violation
bit4: 1 whne the validity bit is high
*/

u16 HPI_AESEBU_Receiver_GetErrorStatus(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 * pwErrorData	///< returned error data
    )
{
	u32 dwErrorData = 0;
	u16 wError = 0;

	wError =
	    HPI_ControlParam1Get(phSubSys, hControlHandle,
				 HPI_AESEBU_ERRORSTATUS, &dwErrorData);
	if (pwErrorData)
		*pwErrorData = (u16) dwErrorData;
	return (wError);
}

/*\}*/
///////////////////////////////////////////////////////////
/**\defgroup aestx AES/EBU Digital audio transmitter control
\{
*/
/** set the AES/EBU transmitters sample rate
this is only valid if the source is the analog mixer
if the source is an outstream, then the samplerate will
be that of the outstream.
*/
u16 HPI_AESEBU_Transmitter_SetSampleRate(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u32 dwSampleRate	//32000,44100 or 48000
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_AESEBU_SAMPLERATE, dwSampleRate, 0);
}

u16 HPI_AESEBU_Transmitter_SetUserData(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wIndex,	// ranges from 0..3
				       u16 wData	// user data to set
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_AESEBU_USERDATA, wIndex, wData);
}

/// set a byte of channel status in the AES/EBU stream
u16 HPI_AESEBU_Transmitter_SetChannelStatus(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wIndex,	// ranges from 0..23
					    u16 wData	// channel status data to write
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_AESEBU_CHANNELSTATUS, wIndex, wData);
}

/** get a byte of channel status in the AES/EBU stream
\warning Currently disabled pending debug of DSP code
*/
u16 HPI_AESEBU_Transmitter_GetChannelStatus(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wIndex,	// ranges from 0..23
					    u16 * pwData	// channel status data to write
    )
{
#if 0
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_AESEBU_CHANNELSTATUS;
	hm.u.c.dwParam1 = wIndex;

	HPI_Message(&hm, &hr);

	if (!hr.wError && pwData)
		*pwData = (u16) hr.u.c.dwParam2;

	return hr.wError;
#else
	return HPI_ERROR_INVALID_OPERATION;
#endif
}

#ifdef HPI_SUPPORT_AESEBUTXSETCLKSRC
/** sets the AES3 Transmitter clock source to be say the adapter or external sync
*/
u16 HPI_AESEBU_Transmitter_SetClockSource(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wClockSource	/* SYNC, ADAPTER */
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_AESEBU_CLOCKSOURCE, wClockSource, 0);
}

u16 HPI_AESEBU_Transmitter_GetClockSource(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 * pwClockSource	/* SYNC, ADAPTER */
    )
{
	u16 wErr;
	u32 dwParam;

	wErr =
	    HPI_ControlParam1Get(phSubSys, hControlHandle,
				 HPI_AESEBU_CLOCKSOURCE, &dwParam);
	if (!wErr && pwClockSource)
		*pwClockSource = (u16) dwParam;

	return wErr;
}
#elif defined HPIDLL_EXPORTS
u16 HPI_AESEBU_Transmitter_SetClockSource(HPI_HSUBSYS * phSubSys,
					  HPI_HCONTROL hControlHandle,
					  u16 wClockSource)
{
return HPI_ERROR_INVALID_FUNC};

u16 HPI_AESEBU_Transmitter_GetClockSource(HPI_HSUBSYS * phSubSys,
					  HPI_HCONTROL hControlHandle,
					  u16 * pwClockSource)
{
return HPI_ERROR_INVALID_FUNC};
#endif

u16 HPI_AESEBU_Transmitter_SetFormat(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wOutputFormat	/* HPI_AESEBU_SOURCE_AESEBU, _SPDIF */
    )
{
// we use the HPI_AESEBU_SOURCE attribute, because thats used on the receiver side - _FORMAT would be a better descriptor
	return HPI_ControlParamSet(phSubSys, hControlHandle, HPI_AESEBU_SOURCE,
				   wOutputFormat, 0);
}

u16 HPI_AESEBU_Transmitter_GetFormat(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 * pwOutputFormat	/* HPI_AESEBU_SOURCE_AESEBU, _SPDIF */
    )
{
	u16 wErr;
	u32 dwParam;

// we use the HPI_AESEBU_SOURCE attribute, because thats used on the receiver side - _FORMAT would be a better descriptor
	wErr =
	    HPI_ControlParam1Get(phSubSys, hControlHandle, HPI_AESEBU_SOURCE,
				 &dwParam);
	if (!wErr && pwOutputFormat)
		*pwOutputFormat = (u16) dwParam;

	return wErr;
}

/*\}*/
/////////////////////////////////////////////////////////////////////////
/**\defgroup bitstream Bitstream control
Control synchronous bitstream I/O
\{
*/
u16 HPI_Bitstream_SetClockEdge(HPI_HSUBSYS * phSubSys,
			       HPI_HCONTROL hControlHandle, u16 wEdgeType)
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_BITSTREAM_CLOCK_EDGE, wEdgeType, 0);
}

u16 HPI_Bitstream_SetDataPolarity(HPI_HSUBSYS * phSubSys,
				  HPI_HCONTROL hControlHandle, u16 wPolarity)
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_BITSTREAM_DATA_POLARITY, wPolarity, 0);
}

u16 HPI_Bitstream_GetActivity(HPI_HSUBSYS * phSubSys,
			      HPI_HCONTROL hControlHandle,
			      u16 * pwClkActivity, u16 * pwDataActivity)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_BITSTREAM_ACTIVITY;
	HPI_Message(&hm, &hr);
	if (pwClkActivity)
		*pwClkActivity = (u16) hr.u.c.dwParam1;
	if (pwDataActivity)
		*pwDataActivity = (u16) hr.u.c.dwParam2;
	return (hr.wError);
}

///\}

/////////////////////////////////////////////////////////////////////////
/**\defgroup channelmode Channel Mode control
\{
*/
u16 HPI_ChannelModeSet(HPI_HSUBSYS * phSubSys,
		       HPI_HCONTROL hControlHandle, u16 wMode)
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_MULTIPLEXER_SOURCE, wMode, 0);
}

u16 HPI_ChannelModeGet(HPI_HSUBSYS * phSubSys,
		       HPI_HCONTROL hControlHandle, u16 * wMode)
{
	u32 dwMode = 0;
	u16 wError =
	    HPI_ControlParam1Get(phSubSys, hControlHandle,
				 HPI_MULTIPLEXER_SOURCE, &dwMode);
	if (wMode)
		*wMode = (u16) dwMode;
	return (wError);
}

/**\}*/
////////////////////////////////////////////////////////////////////////////////
/**\defgroup cobranet Cobranet control
A cobranet adapter has one cobranet control for each cobranet interface (usually only one).
The cobranet control is located on (HPI_SOURCENODE_COBRANET,0,HPI_DESTNODE_COBRANET,0)
The cobranet control allows reading and writing of the cobranet HMI variables
(See Cirrus cobranet documentation for details)

@{
*/

/** Write to an HMI variable.
\return 0=success or HPI_ERROR_*
\retval HPI_ERROR_INVALID_CONTROL if type is not cobranet
\retval HPI_ERROR_INVALID_OPERATION if HMI variable is not writeable
\retval HPI_ERROR_INVALID_DATASIZE if requested size is greater than the HMI variable size
*/

u16 HPI_Cobranet_HmiWrite(HPI_HSUBSYS * phSubSys,	///<Subsystem handle
			  HPI_HCONTROL hControlHandle,	///<Handle of a cobranet control
			  u32 dwHmiAddress,	/// dwHmiAddress HMI address
			  u32 dwByteCount,	///<Number of bytes to send to the control
			  u8 * pbData	///<pointer to data to send
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROLEX, HPI_CONTROL_SET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.cx.wControlIndex);

	hm.u.cx.u.cobranet_data.dwByteCount = dwByteCount;
	hm.u.cx.u.cobranet_data.dwHmiAddress = dwHmiAddress;

	if (dwByteCount <= 8) {
		memcpy(hm.u.cx.u.cobranet_data.dwData, pbData, dwByteCount);
		hm.u.cx.wAttribute = HPI_COBRANET_SET;
	} else {
		hm.u.cx.u.cobranet_bigdata.pbData = pbData;
		hm.u.cx.wAttribute = HPI_COBRANET_SET_DATA;
	}

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/** Read from an HMI variable.
\return 0==success or HPI_ERROR_*
\retval HPI_ERROR_INVALID_CONTROL if type is not cobranet

The amount of data returned will be the minimum of the input dwMaxByteCount and the actual
size of the variable reported by the HMI
*/

u16 HPI_Cobranet_HmiRead(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
			 HPI_HCONTROL hControlHandle,	///< Handle of a cobranet control
			 u32 dwHmiAddress,	///<HMI address
			 u32 dwMaxByteCount,	///<maximum number of bytes to return (<= buffer size of pbData)
			 u32 * pdwByteCount,	///<[out]actual number of bytes returned
			 u8 * pbData	///<[out] data read from HMI variable
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROLEX, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.cx.wControlIndex);

	hm.u.cx.u.cobranet_data.dwByteCount = dwMaxByteCount;
	hm.u.cx.u.cobranet_data.dwHmiAddress = dwHmiAddress;

	if (dwMaxByteCount <= 8) {
		hm.u.cx.wAttribute = HPI_COBRANET_GET;
	} else {
		hm.u.cx.u.cobranet_bigdata.pbData = pbData;
		hm.u.cx.wAttribute = HPI_COBRANET_GET_DATA;
	}

	HPI_Message(&hm, &hr);
	if (!hr.wError && pbData) {

		*pdwByteCount = hr.u.cx.u.cobranet_data.dwByteCount;

		if (*pdwByteCount < dwMaxByteCount)
			dwMaxByteCount = *pdwByteCount;

		if (hm.u.cx.wAttribute == HPI_COBRANET_GET) {
			memcpy(pbData, hr.u.cx.u.cobranet_data.dwData,
			       dwMaxByteCount);
		} else {
/* data was already copied in hpi */
		}

	}
	return (hr.wError);
}

/** Get the status of the last cobranet operation
*/
u16 HPI_Cobranet_HmiGetStatus(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
			      HPI_HCONTROL hControlHandle,	///< Handle of a cobranet control
			      u32 * pdwStatus,	///<[out]the raw status word from the HMI
			      u32 * pdwReadableSize,	///<[out] the reported readable size from the last variable access
			      u32 * pdwWriteableSize	///<[out] the reported writeable size from the last variable access
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROLEX, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.cx.wControlIndex);

	hm.u.cx.wAttribute = HPI_COBRANET_GET_STATUS;

	HPI_Message(&hm, &hr);
	if (!hr.wError) {
		if (pdwStatus)
			*pdwStatus = hr.u.cx.u.cobranet_status.dwStatus;
		if (pdwReadableSize)
			*pdwReadableSize =
			    hr.u.cx.u.cobranet_status.dwReadableSize;
		if (pdwWriteableSize)
			*pdwWriteableSize =
			    hr.u.cx.u.cobranet_status.dwWriteableSize;
	}
	return (hr.wError);
}

/** Set the CobraNet mode.
\return 0==success or HPI_ERROR_*
\retval HPI_ERROR_INVALID_CONTROL if type is not cobranet

Allows the user to set the mode of CobraNet operation. This changes the way that a ASI6416 and ASI2416
work together. A reload of the DSP code is required for the new setting to take effect. In Windows
this requires a reboot of the computer. In Linux this requires that driver be unloaded and reloaded.
*/
u16 HPI_Cobranet_SetMode(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
			 HPI_HCONTROL hControlHandle,	///< Handle of a CobraNet control
			 u32 dwMode,	///< Mode should be one of HPI_COBRANET_MODE_NETWORK, HPI_COBRANET_MODE_TETHERED
			 u32 dwSetOrQuery	///< Use HPI_COBRANET_MODE_QUERY to query or HPI_COBRANET_MODE_SET to set
    )
{
	return HPI_ControlExParamSet(phSubSys, hControlHandle,
				     HPI_COBRANET_MODE, dwMode, dwSetOrQuery);
}

/** Get the CobraNet mode.
\return 0==success or HPI_ERROR_*
\retval HPI_ERROR_INVALID_CONTROL if type is not cobranet

Allows the user to get the mode of CobraNet operation. This changes the way that a ASI6416 and ASI2416
work together.
*/
u16 HPI_Cobranet_GetMode(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
			 HPI_HCONTROL hControlHandle,	///< Handle of a CobraNet control
			 u32 * pdwMode	///< Returns HPI_COBRANET_MODE_NETWORK or HPI_COBRANET_MODE_TETHERED
    )
{
	u16 nError = 0;
	u32 dwMode = 0;
	nError =
	    HPI_ControlExParam1Get(phSubSys, hControlHandle, HPI_COBRANET_MODE,
				   &dwMode);
	if (pdwMode)
		*pdwMode = (u16) dwMode;
	return (nError);
}

/** Get the CobraNet node's IP address.
\return 0==success or HPI_ERROR_*
\retval HPI_ERROR_INVALID_CONTROL if type is not cobranet

Allows the user to get the IP address of the CobraNet node.
*/
u16 HPI_Cobranet_GetIPaddress(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
			      HPI_HCONTROL hControlHandle,	///< Handle of a cobranet control
			      u32 * pdwIPaddress	///< OUT: the IP address
    )
{
	u32 dwByteCount;
	u32 dwIP;
	u16 wError;
	wError = HPI_Cobranet_HmiRead(phSubSys, hControlHandle,
				      HPI_COBRANET_HMI_cobraIpMonCurrentIP,
				      4, &dwByteCount, (u8 *) & dwIP);
// byte swap the IP address
	*pdwIPaddress =
	    ((dwIP & 0xff000000) >> 8) |
	    ((dwIP & 0x00ff0000) << 8) |
	    ((dwIP & 0x0000ff00) >> 8) | ((dwIP & 0x000000ff) << 8);

	return wError;

}

/** Get the CobraNet node's MAC address.
\return 0==success or HPI_ERROR_*
\retval HPI_ERROR_INVALID_CONTROL if type is not cobranet

Allows the user to get the MAC address of the CobraNet node.
*/
u16 HPI_Cobranet_GetMACaddress(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
			       HPI_HCONTROL hControlHandle,	///< Handle of a cobranet control
			       u32 * pdwMAC_MSBs,	///< OUT: the first 4 bytes of the MAC address.
			       u32 * pdwMAC_LSBs	///< OUT: the last 2 bytes of the MAC address returned in the upper 2 bytes.
    )
{
	u32 dwByteCount;
	u16 wError;
	u32 dwMAC;
	wError = HPI_Cobranet_HmiRead(phSubSys, hControlHandle,
				      HPI_COBRANET_HMI_cobraIfPhyAddress,
				      4, &dwByteCount, (u8 *) & dwMAC);
	*pdwMAC_MSBs =
	    ((dwMAC & 0xff000000) >> 8) |
	    ((dwMAC & 0x00ff0000) << 8) |
	    ((dwMAC & 0x0000ff00) >> 8) | ((dwMAC & 0x000000ff) << 8);
	wError += HPI_Cobranet_HmiRead(phSubSys, hControlHandle,
				       HPI_COBRANET_HMI_cobraIfPhyAddress + 1,
				       4, &dwByteCount, (u8 *) & dwMAC);
	*pdwMAC_LSBs =
	    ((dwMAC & 0xff000000) >> 8) |
	    ((dwMAC & 0x00ff0000) << 8) |
	    ((dwMAC & 0x0000ff00) >> 8) | ((dwMAC & 0x000000ff) << 8);
	return wError;

}

	  /** @} */// group cobranet
/////////////////////////////////////////////////////////////////////////////////
/**\defgroup compand  Compressor Expander control
\{
The compander multiplies its input signal by a factor that is dependent on the
amplitude of that signal.
*/
/*! Set up a compressor expander
\return HPI_ERROR_*
*/
u16 HPI_Compander_Set(HPI_HSUBSYS * phSubSys,	//!<HPI subsystem handle
		      HPI_HCONTROL hControlHandle,	//!<Equalizer control handle
		      u16 wAttack,	//!<attack time in milliseconds
		      u16 wDecay,	//!<decay time in milliseconds
		      short wRatio100,	//!<gain ratio * 100
		      short nThreshold0_01dB,	//!<threshold in 100ths of a dB
		      short nMakeupGain0_01dB	//!<makeup gain in 100ths of a dB
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_SET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);

	hm.u.c.dwParam1 = wAttack + ((u32) wRatio100 << 16);
	hm.u.c.dwParam2 = (wDecay & 0xFFFFL);
	hm.u.c.anLogValue[0] = nThreshold0_01dB;
	hm.u.c.anLogValue[1] = nMakeupGain0_01dB;
	hm.u.c.wAttribute = 0;

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/*! Get the settings of a compressor expander
\return HPI_ERROR_*
*/
u16 HPI_Compander_Get(HPI_HSUBSYS * phSubSys,	//!<HPI subsystem handle
		      HPI_HCONTROL hControlHandle,	//!<Equalizer control handle
		      u16 * pwAttack,	//!<[out] attack time in milliseconds
		      u16 * pwDecay,	//!<[out] decay time in milliseconds
		      short *pwRatio100,	//!<[out] gain ratio * 100
		      short *pnThreshold0_01dB,	//!<[out] threshold in 100ths of a dB
		      short *pnMakeupGain0_01dB	//!<[out] makeup gain in 100ths of a dB
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = 0;

	HPI_Message(&hm, &hr);

	if (pwAttack)
		*pwAttack = (short)(hr.u.c.dwParam1 & 0xFFFF);
	if (pwDecay)
		*pwDecay = (short)(hr.u.c.dwParam2 & 0xFFFF);
	if (pwRatio100)
		*pwRatio100 = (short)(hr.u.c.dwParam1 >> 16);

	if (pnThreshold0_01dB)
		*pnThreshold0_01dB = hr.u.c.anLogValue[0];
	if (pnMakeupGain0_01dB)
		*pnMakeupGain0_01dB = hr.u.c.anLogValue[1];

	return (hr.wError);
}

///\}
/////////////////////////////////////////////////////////////////////////
/** \defgroup level Level Control
The level control sets the gain of the line (analog) inputs and outputs
The level is in units of 0.01dBu (0dBu = 0.775 VRMS)
\{
*/

u16 HPI_LevelSetGain(HPI_HSUBSYS * phSubSys,
		     HPI_HCONTROL hControlHandle,
		     short anGain0_01dB[HPI_MAX_CHANNELS]
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_SET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	memcpy(hm.u.c.anLogValue, anGain0_01dB,
	       sizeof(short) * HPI_MAX_CHANNELS);
	hm.u.c.wAttribute = HPI_LEVEL_GAIN;

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

u16 HPI_LevelGetGain(HPI_HSUBSYS * phSubSys,
		     HPI_HCONTROL hControlHandle,
		     short anGain0_01dB[HPI_MAX_CHANNELS]
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_LEVEL_GAIN;

	HPI_Message(&hm, &hr);

	memcpy(anGain0_01dB, hr.u.c.anLogValue,
	       sizeof(short) * HPI_MAX_CHANNELS);
	return (hr.wError);
}

///\}
/////////////////////////////////////////////////////////////////////////
/** \defgroup meter  Meter Control
The meter control returns information about the level of the audio signal
at the position of the control.

Depending on the adapter, Peak and RMS readings are available.
On some adapters meter ballistics may be set (on older adapters the meter gives an instantaneous reading)

Readings are in units of 0.01dB

\todo Implement linear to log conversion in fixed point math so that it can be included with kernel code.

@{
*/

/** Get the meter peak reading
*/
u16 HPI_MeterGetPeak(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle
		     HPI_HCONTROL hControlHandle,	///< meter control handle
		     short anPeakdB[HPI_MAX_CHANNELS]	///< [out] meter peaks
    )
{
#ifndef HPI_KERNEL_MODE
	short nLinear = 0;
#endif
	short i = 0;

	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_METER_PEAK;

	HPI_Message(&hm, &hr);

	if (!hr.wError) {
// Method to return meter values from DSP
// If return value is -ve (-1 to -32767) then treat as Log(peak)
// range of log values will be -1 to -20000 (-0.01 to -200.00 dB)
// 0 will never be returned for log (-1 = -0.01dB is max)

#ifndef HPI_KERNEL_MODE
/// \bug Kernel mode cant do floating point log.  +ve linear value is returned instead.

// convert 0..32767 level to 0.01dB (20log10), 0 is -100.00dB
		for (i = 0; i < HPI_MAX_CHANNELS; i++) {
			nLinear = hr.u.c.anLogValue[i];
// don't have to touch the LogValue when it is -ve since it is already a log value
			if (nLinear >= 0) {
				if (nLinear == 0)
					hr.u.c.anLogValue[i] =
					    HPI_METER_MINIMUM;
				else
					hr.u.c.anLogValue[i] = (short)((float)(20 * log10((float)nLinear / 32767.0)) * 100.0);	// units are 0.01dB
			}
		}
#endif

		memcpy(anPeakdB, hr.u.c.anLogValue,
		       sizeof(short) * HPI_MAX_CHANNELS);
	} else
		for (i = 0; i < HPI_MAX_CHANNELS; i++)
			anPeakdB[i] = HPI_METER_MINIMUM;
	return (hr.wError);
}

/** Get the meter RMS reading in 100ths of a dB
*/
u16 HPI_MeterGetRms(HPI_HSUBSYS * phSubSys,
		    HPI_HCONTROL hControlHandle, short anRmsdB[HPI_MAX_CHANNELS]
    )
{
#ifndef HPI_KERNEL_MODE
	short nLinear = 0;
#endif
	short i = 0;

	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_METER_RMS;

	HPI_Message(&hm, &hr);

	if (!hr.wError) {
#ifndef HPI_KERNEL_MODE
/// \bug Kernel mode cant do floating point log.  +ve linear value is returned instead.
// convert 0..32767 level to 0.01dB (20log10), 0 is -100.00dB
		for (i = 0; i < HPI_MAX_CHANNELS; i++) {
			nLinear = hr.u.c.anLogValue[i];
// don't have to touch the LogValue when it is -ve since it is already a log value
			if (nLinear >= 0) {
				if (nLinear == 0)
					hr.u.c.anLogValue[i] =
					    HPI_METER_MINIMUM;
				else
					hr.u.c.anLogValue[i] = (short)((float)(20 * log10((float)nLinear / 32767.0)) * 100.0);	// units are 0.01dB
			}
		}
#endif
		memcpy(anRmsdB, hr.u.c.anLogValue,
		       sizeof(short) * HPI_MAX_CHANNELS);
	} else
		for (i = 0; i < HPI_MAX_CHANNELS; i++)
			anRmsdB[i] = HPI_METER_MINIMUM;	// return -100dB in case function is not supported.

	return (hr.wError);
}

/** Set the ballistics of the RMS part of a meter.

The attack and decay values represent the time constants of the equivalent single pole low pass filter used to create the ballistics.
With a time constant of T, if the meter is stable at full scale and the input is suddenly removed, the meter will decay.

Setting nAttack to 0 gives the meter instantaneous rise time.
Setting nDecay to a value smaller than a few times your  meter polling interval is not advised.
The meter will appear to read something approaching the instantaneous value at the time of polling
rather than the maximum peak since the previous reading.
*/
u16 HPI_MeterSetRmsBallistics(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, unsigned short nAttack,	///< Attack timeconstant in milliseconds
			      unsigned short nDecay	///< Decay timeconstant in milliseconds
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_METER_RMS_BALLISTICS, nAttack, nDecay);
}

/** Get the ballistics settings of the RMS part of a meter.
*/
u16 HPI_MeterGetRmsBallistics(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, unsigned short *pnAttack,	///< Attack timeconstant in milliseconds
			      unsigned short *pnDecay	///< Decay timeconstant in milliseconds
    )
{
	u32 dwAttack;
	u32 dwDecay;
	u16 nError;

	nError =
	    HPI_ControlParam2Get(phSubSys, hControlHandle,
				 HPI_METER_RMS_BALLISTICS, &dwAttack, &dwDecay);

	if (pnAttack)
		*pnAttack = (unsigned short)dwAttack;
	if (pnDecay)
		*pnDecay = (unsigned short)dwDecay;

	return nError;
}

/** Set the ballistics of the Peak part of a meter.
*/
u16 HPI_MeterSetPeakBallistics(HPI_HSUBSYS * phSubSys,
			       HPI_HCONTROL hControlHandle,
			       unsigned short nAttack, unsigned short nDecay)
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_METER_PEAK_BALLISTICS, nAttack, nDecay);
}

/** Get the ballistics settings of the Peak part of a meter.
*/
u16 HPI_MeterGetPeakBallistics(HPI_HSUBSYS * phSubSys,
			       HPI_HCONTROL hControlHandle,
			       unsigned short *pnAttack,
			       unsigned short *pnDecay)
{
	u32 dwAttack;
	u32 dwDecay;
	u16 nError;

	nError =
	    HPI_ControlParam2Get(phSubSys, hControlHandle,
				 HPI_METER_PEAK_BALLISTICS, &dwAttack,
				 &dwDecay);

	if (pnAttack)
		*pnAttack = (short)dwAttack;
	if (pnDecay)
		*pnDecay = (short)dwDecay;

	return nError;
}

/** @} */

/* future function to convert linear to log using integers

long lLogC[16] = {
5184138,
-95824768,
-196833680,
-297842592,
-398851488,
-499860384,
-600869312,
-701878208,
-802887104,
-903896000,
-1004904896,
-1105913856,
-1206922752,
-1307931648,
-1408940544,
30};
double __fastcall TFLogDemo::LinearToLogQuadApproxInt(short nPeak)
{
short   nBeforeFirstBit;
short   nR;
short nSample;
long    lAcc;

nBeforeFirstBit = 0;
nSample=nPeak;
while(!(nSample&0x8000) )
{
nSample <<= 1;
nBeforeFirstBit++;
}
nSample &= ~0x8000;
nR = nBeforeFirstBit;
lAcc = (long)nSample * -1047L;  // Q15 * Q9 = Q24
lAcc = lAcc + 135336768L;
lAcc = lAcc >> 15;              // Q24 > Q9
lAcc = lAcc * (long)nSample;    // Q9 * Q15 = Q24
lAcc = lAcc + lLogC[nR];        // lAcc has result in Q24
// lAcc = (lAcc>>8)*100;        // result is 100ths of dB in Q16 format
// dB100 = (short)(lAcc>>16L);
return (double)lAcc / pow(2.0,24.0);
}
*/
/////////////////////////////////////////////////////////////////////////////////
/**\defgroup mic Microphone control
\{
*/
u16 HPI_Microphone_SetPhantomPower(HPI_HSUBSYS * phSubSys,
				   HPI_HCONTROL hControlHandle, u16 wOnOff)
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_MICROPHONE_PHANTOM_POWER, (u32) wOnOff,
				   0);
}

u16 HPI_Microphone_GetPhantomPower(HPI_HSUBSYS * phSubSys,
				   HPI_HCONTROL hControlHandle, u16 * pwOnOff)
{
	u16 nError = 0;
	u32 dwOnOff = 0;
	nError =
	    HPI_ControlParam1Get(phSubSys, hControlHandle,
				 HPI_MICROPHONE_PHANTOM_POWER, &dwOnOff);
	if (pwOnOff)
		*pwOnOff = (u16) dwOnOff;
	return (nError);
}

///\}
/////////////////////////////////////////////////////////////////////////
/** \defgroup mux Multiplexer control
This control allows one of many sources to be connected to a destination
Typically used on the instream (record) side to select a record input
or on the linein to select analog or digital input.
\{
*/

u16 HPI_Multiplexer_SetSource(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wSourceNodeType,	// any source node
			      u16 wSourceNodeIndex	// any source node
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_MULTIPLEXER_SOURCE, wSourceNodeType,
				   wSourceNodeIndex);
}

u16 HPI_Multiplexer_GetSource(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 * wSourceNodeType,	/* any source node */
			      u16 * wSourceNodeIndex	/* any source node  */
    )
{
	u32 dwNode, dwIndex;
	u16 wError =
	    HPI_ControlParam2Get(phSubSys, hControlHandle,
				 HPI_MULTIPLEXER_SOURCE, &dwNode, &dwIndex);
	if (wSourceNodeType)
		*wSourceNodeType = (u16) dwNode;
	if (wSourceNodeIndex)
		*wSourceNodeIndex = (u16) dwIndex;
	return wError;
}

u16 HPI_Multiplexer_QuerySource(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wIndex, u16 * wSourceNodeType,	/* any source node */
				u16 * wSourceNodeIndex	/* any source node  */
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_MULTIPLEXER_QUERYSOURCE;
	hm.u.c.dwParam1 = wIndex;

	HPI_Message(&hm, &hr);

	if (wSourceNodeType)
		*wSourceNodeType = (u16) hr.u.c.dwParam1;
	if (wSourceNodeIndex)
		*wSourceNodeIndex = (u16) hr.u.c.dwParam2;
	return (hr.wError);
}

/**\}*/
#ifdef HPI_SUPPORT_ONOFFSWITCH
/////////////////////////////////////////////////////////////////////////
/** \defgroup onoff On/off switch control
This control allows make/break connections to be supported.
\{
*/

u16 HPI_OnOffSwitch_SetState(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wState	/* 1=on, 0=off */
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_ONOFFSWITCH_STATE, wState, 0);
}

u16 HPI_OnOffSwitch_GetState(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 * wState	/* 1=on, 0=off */
    )
{
	u32 dwState = 0;
	u16 wError =
	    HPI_ControlParam1Get(phSubSys, hControlHandle,
				 HPI_ONOFFSWITCH_STATE, &dwState);
	if (wState)
		*wState = (u16) dwState;
	return (wError);
}

/**\}*/
#elif defined HPIDLL_EXPORTS
u16 HPI_OnOffSwitch_SetState(HPI_HSUBSYS * phSubSys,
			     HPI_HCONTROL hControlHandle, u16 wState)
{
return HPI_ERROR_INVALID_FUNC};

u16 HPI_OnOffSwitch_GetState(HPI_HSUBSYS * phSubSys,
			     HPI_HCONTROL hControlHandle, u16 * wState)
{
return HPI_ERROR_INVALID_FUNC};
#endif
/////////////////////////////////////////////////////////////////////////////////
/**\addtogroup parmeq  Parametric Equalizer control
\{
*/

/*!
Find out the number of available bands of a parametric equalizer,
and whether it is enabled or not

\return HPI_ERROR_*
*/
u16 HPI_ParametricEQ_GetInfo(HPI_HSUBSYS * phSubSys,	//!<  HPI subsystem handle
			     HPI_HCONTROL hControlHandle,	//!<  Equalizer control handle
			     u16 * pwNumberOfBands,	//!< [out] number of bands available
			     u16 * pwOnOff	//!< [out] enabled status
    )
{
	u32 dwNOB = 0;
	u32 dwOO = 0;
	u16 nError = 0;

	nError =
	    HPI_ControlParam2Get(phSubSys, hControlHandle,
				 HPI_EQUALIZER_NUM_FILTERS, &dwOO, &dwNOB);
	if (pwNumberOfBands)
		*pwNumberOfBands = (u16) dwNOB;
	if (pwOnOff)
		*pwOnOff = (u16) dwOO;
	return nError;
}

/*!
Turn a parametric equalizer on or off

\return HPI_ERROR_*
*/
u16 HPI_ParametricEQ_SetState(HPI_HSUBSYS * phSubSys,	//!<  HPI subsystem handle
			      HPI_HCONTROL hControlHandle,	//!<  Equalizer control handle
			      u16 wOnOff	//!<  1=on, 0=off
    )
{
	return HPI_ControlParamSet(phSubSys,
				   hControlHandle,
				   HPI_EQUALIZER_NUM_FILTERS, wOnOff, 0);
}

/*! Set up one of the filters in a parametric equalizer
\return HPI_ERROR_*
*/
u16 HPI_ParametricEQ_SetBand(HPI_HSUBSYS * phSubSys,	//!<  HPI subsystem handle
			     HPI_HCONTROL hControlHandle,	//!<  Equalizer control handle
			     u16 wIndex,	//!<  index of band to set
			     u16 nType,	//!<  band type, One of the \ref eq_filter_types
			     u32 dwFrequencyHz,	//!<  band frequency
			     short nQ100,	//!<  filter Q * 100
			     short nGain0_01dB	//!<  filter gain in 100ths of a dB
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_SET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);

	hm.u.c.dwParam1 = dwFrequencyHz;
	hm.u.c.dwParam2 = (wIndex & 0xFFFFL) + ((u32) nType << 16);
	hm.u.c.anLogValue[0] = nGain0_01dB;
	hm.u.c.anLogValue[1] = nQ100;
	hm.u.c.wAttribute = HPI_EQUALIZER_FILTER;

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/*! Get the settings of one of the filters in a parametric equalizer
\return HPI_ERROR_*
*/
u16 HPI_ParametricEQ_GetBand(HPI_HSUBSYS * phSubSys,	//!<  HPI subsystem handle
			     HPI_HCONTROL hControlHandle,	//!<  Equalizer control handle
			     u16 wIndex,	//!<  index of band to Get
			     u16 * pnType,	//!< [out] band type
			     u32 * pdwFrequencyHz,	//!< [out] band frequency
			     short *pnQ100,	//!< [out] filter Q * 100
			     short *pnGain0_01dB	//!< [out] filter gain in 100ths of a dB
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_EQUALIZER_FILTER;
	hm.u.c.dwParam2 = wIndex;

	HPI_Message(&hm, &hr);

	if (pdwFrequencyHz)
		*pdwFrequencyHz = hr.u.c.dwParam1;
	if (pnType)
		*pnType = (u16) (hr.u.c.dwParam2 >> 16);
	if (pnQ100)
		*pnQ100 = hr.u.c.anLogValue[1];
	if (pnGain0_01dB)
		*pnGain0_01dB = hr.u.c.anLogValue[0];

	return (hr.wError);
}

/** Retrieve the calculated filter coefficients (scaled by 1000 into integers)
*/
u16 HPI_ParametricEQ_GetCoeffs(HPI_HSUBSYS * phSubSys,	//!<  HPI subsystem handle
			       HPI_HCONTROL hControlHandle,	//!<  Equalizer control handle
			       u16 wIndex,	//!<  index of band to Get
			       short coeffs[5]	//!< [out] filter coefficients * 1000 a1,a2,b0,b1,b2 (a0==0)
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_EQUALIZER_COEFFICIENTS;
	hm.u.c.dwParam2 = wIndex;

	HPI_Message(&hm, &hr);

	coeffs[0] = (short)hr.u.c.anLogValue[0];
	coeffs[1] = (short)hr.u.c.anLogValue[1];
	coeffs[2] = (short)hr.u.c.dwParam1;
	coeffs[3] = (short)(hr.u.c.dwParam1 >> 16);
	coeffs[4] = (short)hr.u.c.dwParam2;

	return (hr.wError);
}

///\}
/////////////////////////////////////////////////////////////////////////////////
/**\defgroup sampleclock SampleClock control
\{
*/
u16 HPI_SampleClock_SetSource(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wSource	// HPI_SAMPLECLOCK_SOURCE_ADAPTER, _AESEBU etc
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_SAMPLECLOCK_SOURCE, wSource, 0);
}

u16 HPI_SampleClock_SetSourceIndex(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wSourceIndex	// index of the source to use
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_SAMPLECLOCK_SOURCE_INDEX, wSourceIndex,
				   0);
}

u16 HPI_SampleClock_GetSource(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 * pwSource	// HPI_SAMPLECLOCK_SOURCE_ADAPTER, _AESEBU etc
    )
{
	u16 wError = 0;
	u32 dwSource = 0;
	wError =
	    HPI_ControlParam1Get(phSubSys, hControlHandle,
				 HPI_SAMPLECLOCK_SOURCE, &dwSource);
	if (!wError)
		if (pwSource)
			*pwSource = (u16) dwSource;
	return (wError);
}

u16 HPI_SampleClock_GetSourceIndex(HPI_HSUBSYS * phSubSys,
				   HPI_HCONTROL hControlHandle,
				   u16 * pwSourceIndex)
{
	u16 wError = 0;
	u32 dwSourceIndex = 0;
	wError =
	    HPI_ControlParam1Get(phSubSys, hControlHandle,
				 HPI_SAMPLECLOCK_SOURCE_INDEX, &dwSourceIndex);
	if (!wError)
		if (pwSourceIndex)
			*pwSourceIndex = (u16) dwSourceIndex;
	return (wError);
}

u16 HPI_SampleClock_SetSampleRate(HPI_HSUBSYS * phSubSys,
				  HPI_HCONTROL hControlHandle, u32 dwSampleRate)
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_SAMPLECLOCK_SAMPLERATE, dwSampleRate, 0);
}

u16 HPI_SampleClock_GetSampleRate(HPI_HSUBSYS * phSubSys,
				  HPI_HCONTROL hControlHandle,
				  u32 * pdwSampleRate)
{
	u16 wError = 0;
	u32 dwSampleRate = 0;
	wError =
	    HPI_ControlParam1Get(phSubSys, hControlHandle,
				 HPI_SAMPLECLOCK_SAMPLERATE, &dwSampleRate);
	if (!wError)
		if (pdwSampleRate)
			*pdwSampleRate = dwSampleRate;
	return (wError);
}

///\}

/////////////////////////////////////////////////////////////////////////////////
/**\defgroup tonedetector Tone Detector control
\{
The tone detector monitors its inputs for the presence of any of a number of tones.

Currently 25Hz and 35Hz tones can be detected independently on left and right channels.
The result of the detection is reflected in the controls state, and optionally by sending an
async event with the new sate.
*/

u16 HPI_ToneDetector_GetFrequency(HPI_HSUBSYS * phSubSys,
				  HPI_HCONTROL hControl,
				  u32 nIndex, u32 * dwFrequency)
{
	return HPI_ControlParamGet(phSubSys, hControl,
				   HPI_TONEDETECTOR_FREQUENCY, nIndex, 0,
				   dwFrequency, NULL);
}

///\}

///////////////////////////////////////////////////////////////////////////////
/** \defgroup tuner Tuner Controls

The tuner control sets the band and frequency of a tuner, and measures the RF level
\{
*/

/** Select the tuner band.

Not all tuners support all bands. (Currently either AM+FM or TV+FM).

Note that with the exception of HPI_TUNER_BAND_AUX,
the tuner frequency must subsequently be set using HPI_Tuner_SetFrequency().
\return 0 or HPI_ERROR_*
\retval HPI_ERROR_INVALID_CONTROL_VALUE if tuner does not support the requested band

\sa HPI_ControlQuery() for details on determining the bands supported by a particular tuner.
*/
u16 HPI_Tuner_SetBand(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wBand
		       /**< one of the \ref tuner_bands */
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle, HPI_TUNER_BAND,
				   wBand, 0);
}

/** Set the RF gain of the tuner front end.
*/
u16 HPI_Tuner_SetGain(HPI_HSUBSYS * phSubSys,
		      HPI_HCONTROL hControlHandle, short nGain)
{
	return HPI_ControlParamSet(phSubSys, hControlHandle, HPI_TUNER_GAIN,
				   nGain, 0);
}

u16 HPI_Tuner_SetFrequency(HPI_HSUBSYS * phSubSys,
			   HPI_HCONTROL hControlHandle, u32 wFreqInkHz)
{
	return HPI_ControlParamSet(phSubSys, hControlHandle, HPI_TUNER_FREQ,
				   wFreqInkHz, 0);
}

u16 HPI_Tuner_GetBand(HPI_HSUBSYS * phSubSys,
		      HPI_HCONTROL hControlHandle, u16 * pwBand)
{
	u32 dwBand = 0;
	u16 nError = 0;

	nError =
	    HPI_ControlParam1Get(phSubSys, hControlHandle, HPI_TUNER_BAND,
				 &dwBand);
	if (pwBand)
		*pwBand = (u16) dwBand;
	return nError;
}

u16 HPI_Tuner_GetGain(HPI_HSUBSYS * phSubSys,
		      HPI_HCONTROL hControlHandle, short *pnGain)
{
	u32 dwGain = 0;
	u16 nError = 0;

	nError =
	    HPI_ControlParam1Get(phSubSys, hControlHandle, HPI_TUNER_GAIN,
				 &dwGain);
	if (pnGain)
		*pnGain = (u16) dwGain;
	return nError;
}

u16 HPI_Tuner_GetFrequency(HPI_HSUBSYS * phSubSys,
			   HPI_HCONTROL hControlHandle, u32 * pwFreqInkHz)
{
	return HPI_ControlParam1Get(phSubSys, hControlHandle, HPI_TUNER_FREQ,
				    pwFreqInkHz);
}

u16 HPI_Tuner_GetRFLevel(HPI_HSUBSYS * phSubSys,
			 HPI_HCONTROL hControlHandle, short *pwLevel)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_TUNER_LEVEL;
	hm.u.c.dwParam1 = HPI_TUNER_LEVEL_AVERAGE;
	HPI_Message(&hm, &hr);
	if (pwLevel)
		*pwLevel = (short)hr.u.c.dwParam1;
	return (hr.wError);
}

u16 HPI_Tuner_GetRawRFLevel(HPI_HSUBSYS * phSubSys,
			    HPI_HCONTROL hControlHandle, short *pwLevel)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_TUNER_LEVEL;
	hm.u.c.dwParam1 = HPI_TUNER_LEVEL_RAW;
	HPI_Message(&hm, &hr);
	if (pwLevel)
		*pwLevel = (short)hr.u.c.dwParam1;
	return (hr.wError);
}

/**
\deprecated This function has been superceded by HPI_Tuner_GetStatus()
*/
u16 HPI_Tuner_GetVideoStatus(HPI_HSUBSYS * phSubSys,
			     HPI_HCONTROL hControlHandle, u16 * pwStatus)
{
	u32 dwStatus = 0;
	u16 nError = 0;

	nError =
	    HPI_ControlParam1Get(phSubSys, hControlHandle,
				 HPI_TUNER_VIDEO_STATUS, &dwStatus);
	if (pwStatus)
		*pwStatus = (u16) dwStatus;
	return nError;
}

u16 HPI_Tuner_GetStatus(HPI_HSUBSYS * phSubSys,
			HPI_HCONTROL hControlHandle,
			u16 * pwStatusMask, u16 * pwStatus)
{
	u32 dwStatus = 0;
	u16 nError = 0;

// wStatusmask is in high 16bit word, wStatus is in low 16bit word
	nError =
	    HPI_ControlParam1Get(phSubSys, hControlHandle, HPI_TUNER_STATUS,
				 &dwStatus);
	if (pwStatus) {
		if (!nError) {
			*pwStatusMask = (u16) (dwStatus >> 16);
			*pwStatus = (u16) (dwStatus & 0xFFFF);
		} else {
			*pwStatusMask = 0;
			*pwStatus = 0;
		}
	}
	return nError;
}

u16 HPI_Tuner_SetMode(HPI_HSUBSYS * phSubSys,
		      HPI_HCONTROL hControlHandle, u32 nMode, u32 nValue)
{
	return HPI_ControlParamSet(phSubSys, hControlHandle, HPI_TUNER_MODE,
				   nMode, nValue);
}

u16 HPI_Tuner_GetMode(HPI_HSUBSYS * phSubSys,
		      HPI_HCONTROL hControlHandle, u32 nMode, u32 * pnValue)
{
	return HPI_ControlParamGet(phSubSys, hControlHandle, HPI_TUNER_MODE,
				   nMode, 0, pnValue, NULL);
}

/**\}*/
/////////////////////////////////////////////////////////////////////////
/** \defgroup volume Volume Control
\{
*/
u16 HPI_VolumeSetGain(HPI_HSUBSYS * phSubSys,
		      HPI_HCONTROL hControlHandle,
		      short anLogGain[HPI_MAX_CHANNELS]
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_SET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	memcpy(hm.u.c.anLogValue, anLogGain, sizeof(short) * HPI_MAX_CHANNELS);
	hm.u.c.wAttribute = HPI_VOLUME_GAIN;

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

u16 HPI_VolumeGetGain(HPI_HSUBSYS * phSubSys,
		      HPI_HCONTROL hControlHandle,
		      short anLogGain[HPI_MAX_CHANNELS]
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_VOLUME_GAIN;

	HPI_Message(&hm, &hr);

	memcpy(anLogGain, hr.u.c.anLogValue, sizeof(short) * HPI_MAX_CHANNELS);
	return (hr.wError);
}

u16 HPI_VolumeQueryRange(HPI_HSUBSYS * phSubSys,
			 HPI_HCONTROL hControlHandle,
			 short *nMinGain_01dB,
			 short *nMaxGain_01dB, short *nStepGain_01dB)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_VOLUME_RANGE;

	HPI_Message(&hm, &hr);
	if (hr.wError) {
		hr.u.c.anLogValue[0] = hr.u.c.anLogValue[1] = 0;
		hr.u.c.dwParam1 = (u32) 0;
	}
	if (nMinGain_01dB)
		*nMinGain_01dB = hr.u.c.anLogValue[0];
	if (nMaxGain_01dB)
		*nMaxGain_01dB = hr.u.c.anLogValue[1];
	if (nStepGain_01dB)
		*nStepGain_01dB = (short)hr.u.c.dwParam1;
	return (hr.wError);
}

/* starts an automatic ramp of the volume control from the current gain setting to
the specified setting over the specified duration (in milliseconds )
The profile can be either log or linear
*/

u16 HPI_VolumeAutoFadeProfile(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, short anStopGain0_01dB[HPI_MAX_CHANNELS], u32 dwDurationMs, u16 wProfile	/* HPI_VOLUME_AUTOFADE_??? */
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_SET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	memcpy(hm.u.c.anLogValue, anStopGain0_01dB,
	       sizeof(short) * HPI_MAX_CHANNELS);

	if (wProfile < HPI_VOLUME_AUTOFADE)
		wProfile = HPI_VOLUME_AUTOFADE;

	hm.u.c.wAttribute = wProfile;
	hm.u.c.dwParam1 = dwDurationMs;

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

u16 HPI_VolumeAutoFade(HPI_HSUBSYS * phSubSys,
		       HPI_HCONTROL hControlHandle,
		       short anStopGain0_01dB[HPI_MAX_CHANNELS],
		       u32 dwDurationMs)
{
	return HPI_VolumeAutoFadeProfile(phSubSys,
					 hControlHandle,
					 anStopGain0_01dB,
					 dwDurationMs, HPI_VOLUME_AUTOFADE);
}

/** \} */
/////////////////////////////////////////////////////////////////////////
/**  \defgroup Vox Vox control
VOX controls are always situated on a destination node of type HPI_DESTNODE_ISTREAM.
The VOX control specifies the signal level required for recording to begin.
Until the signal exceeds the VOX level no samples are recorded to the record stream.
After the VOX level has been exceeded, recording continues until the stream is stopped or reset.

A trigger level of -100 dB indicates that recording begins with any level audio signal.
A single threshold level is used for both left and right channels.
If either channel exceeds the threshold, recording will proceed.

@{
*/

/**
Sets the threshold of a VOX control.  Note the threshold is in units of 0.01dB.
The threshold will typically range between 0 and -100dB.
On startup the VOX control is set to -100 dB.
*/

u16 HPI_VoxSetThreshold(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
			HPI_HCONTROL hControlHandle,	///< Handle of a vox control
			short anGain0_01dB	///<  Trigger level in 100ths of a dB
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_SET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_VOX_THRESHOLD;

#ifndef HPI_KERNEL_MODE
/// \bug Requires floating point operations, not usable in kernel mode. Kernel mode uses raw linear value instead.

// convert db range 0..-100 dB , 0.01dB (20log10) to 0..32767 (-96 dB = 0 )
	{
/*
fDB = 20 * log10( fLinear/32767 )

fLinear = 10^(fDB/20) * 32767

Want to avoid using pow() routine, because math libraries in the past
have not handled it well.

Re-arrange to us log()/exp() (natural log) routines.

Re-write 10^(fDB/20) using natural logs.

fLinear = exp( log(10.0) * fDB/20.0 ) * 32767.0;

*/
		float fDB = (float)anGain0_01dB / 100.0f;	// units are 0.01dB
		float fLinear = 0.0;
		fLinear = (float)(exp(log(10.0) * fDB / 20.0) * 32767.0);
		hm.u.c.anLogValue[0] = (short)(fLinear);	// only use the first index (LEFT)
	}
#else

	hm.u.c.anLogValue[0] = anGain0_01dB;	// only use the first index (LEFT)
#endif

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/** Get the current threshold setting of a vox control
*/
u16 HPI_VoxGetThreshold(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
			HPI_HCONTROL hControlHandle,	///< Handle of a vox contrl
			short *anGain0_01dB	///<  [out] Trigger level in 100ths of a dB
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_VOX_THRESHOLD;

	HPI_Message(&hm, &hr);

#ifndef HPI_KERNEL_MODE
/// \bug Requires floating point operations, not usable in kernel mode

// convert db range 0..-100 dB , 0.01dB (20log10) to 0..32767 (-96 dB = 0 )
	{
/*
fDB = 20 * log10( fLinear/32767 )

fLinear = 10^(fDB/20) * 32767

Want to avoid using pow() routine, because math libraries in the past
have not handled it well.

Re-arrange to us log()/exp() (natural log) routines.

Re-write 10^(fDB/20) using natural logs.

fLinear = exp( log(10.0) * fDB/20.0 ) * 32767.0;

*/
		float fDB, fLinear;

		if (hr.u.c.anLogValue[0] == 0) {
			fDB = -100.0;
		} else {
			fLinear = (float)hr.u.c.anLogValue[0];
			fDB =
			    (float)(log(fLinear / 32767.0) * 20.0 / log(10.0));
		}

		*anGain0_01dB = (short)(fDB * 100.0);	// only use the first index (LEFT)
	}
#else

	*anGain0_01dB = hr.u.c.anLogValue[0];	// only use the first index (LEFT)
#endif

	return (hr.wError);
}

/** @}*/
	  /** @} */// group mixer
////////////////////////////////////////////////////////////////////////////
/**\defgroup clock Clock
Realtime clock
\note Not currently implemented anywhere???
Only one clock per adapter
\{
*/
/// open the clock and get a handle
u16 HPI_ClockOpen(HPI_HSUBSYS * phSubSys,
		  u16 wAdapterIndex, HPI_HCLOCK * phClock)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_CLOCK, HPI_CLOCK_OPEN);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

#ifdef TRY_NEW_HANDLE_STRUCT
	if (hr.wError == 0) {
		phClock->adapterIndex = wAdapterIndex;
		phClock->dspIndex = 0;
		phClock->objIndex = 0;
		phClock->valid = 1;
	} else
		phClock->valid = 0;
#else
	if (hr.wError == 0)
		*phClock = HPI_IndexesToHandle(HPI_OBJ_CLOCK, wAdapterIndex, 0);	// only 1 clock obj per adapter
	else
		*phClock = 0;
#endif

	return (hr.wError);
}

/// set the time of the DSP clock object.
u16 HPI_ClockSetTime(HPI_HSUBSYS * phSubSys,
		     HPI_HCLOCK hClock,
		     u16 wHour, u16 wMinute, u16 wSecond, u16 wMilliSeconds)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_CLOCK, HPI_CLOCK_SET_TIME);
#ifdef TRY_NEW_HANDLE_STRUCT
	if (!hClock.valid)
		return HPI_ERROR_OBJ_NOT_OPEN;
	hm.wAdapterIndex = hClock.adapterIndex;
#else
	HPI_HANDLETOINDEXES(hClock, &hm.wAdapterIndex, NULL);
#endif
	hm.u.t.wMilliSeconds = wMilliSeconds;
	hm.u.t.wSeconds = wSecond;
	hm.u.t.wMinutes = wMinute;
	hm.u.t.wHours = wHour;
	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/// set the time of the DSP clock object.
u16 HPI_ClockGetTime(HPI_HSUBSYS * phSubSys,
		     HPI_HCLOCK hClock,
		     u16 * pwHour,
		     u16 * pwMinute, u16 * pwSecond, u16 * pwMilliSecond)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_CLOCK, HPI_CLOCK_GET_TIME);
#ifdef TRY_NEW_HANDLE_STRUCT
	if (!hClock.valid)
		return HPI_ERROR_OBJ_NOT_OPEN;
	hm.wAdapterIndex = hClock.adapterIndex;
#else
	HPI_HANDLETOINDEXES(hClock, &hm.wAdapterIndex, NULL);
#endif
	HPI_Message(&hm, &hr);
	if (pwMilliSecond)
		*pwMilliSecond = hr.u.t.wMilliSeconds;
	if (pwSecond)
		*pwSecond = hr.u.t.wSeconds;
	if (pwMinute)
		*pwMinute = hr.u.t.wMinutes;
	if (pwHour)
		*pwHour = hr.u.t.wHours;

	return (hr.wError);
}

///\}
////////////////////////////////////////////////////////////////////////////
/**\defgroup gpio Digital I/O
The digital I/O object on an adapter reperesents a number of input bits
that may be individually sensed and a number of digital output bits that
may be individually set.

There is at most one gpio object per adapter.
\{
*/

u16 HPI_GpioOpen(HPI_HSUBSYS * phSubSys,
		 u16 wAdapterIndex,
		 HPI_HGPIO * phGpio,
		 u16 * pwNumberInputBits, u16 * pwNumberOutputBits)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_GPIO, HPI_GPIO_OPEN);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	if (hr.wError == 0) {
		*phGpio = HPI_IndexesToHandle(HPI_OBJ_GPIO, wAdapterIndex, 0);	// only 1 digital i/o obj per adapter
		if (pwNumberInputBits)
			*pwNumberInputBits = hr.u.l.wNumberInputBits;
		if (pwNumberOutputBits)
			*pwNumberOutputBits = hr.u.l.wNumberOutputBits;
	} else
		*phGpio = 0;
	return (hr.wError);
}

/// read a particular bit from an adapters digital input port
u16 HPI_GpioReadBit(HPI_HSUBSYS * phSubSys,
		    HPI_HGPIO hGpio, u16 wBitIndex, u16 * pwBitData)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_GPIO, HPI_GPIO_READ_BIT);
	HPI_HANDLETOINDEXES(hGpio, &hm.wAdapterIndex, NULL);	// only one dig i/o obj per adapter
	hm.u.l.wBitIndex = wBitIndex;

	HPI_Message(&hm, &hr);

	*pwBitData = hr.u.l.wBitData;
	return (hr.wError);
}

/// read all bits from an adapters digital input port (upto 16)
u16 HPI_GpioReadAllBits(HPI_HSUBSYS * phSubSys,
			HPI_HGPIO hGpio, u16 * pwBitData)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_GPIO, HPI_GPIO_READ_ALL);
	HPI_HANDLETOINDEXES(hGpio, &hm.wAdapterIndex, NULL);	// only one dig i/o obj per adapter

	HPI_Message(&hm, &hr);

	*pwBitData = hr.u.l.wBitData;
	return (hr.wError);
}

/// write a particular bit to an adapters digital output port
u16 HPI_GpioWriteBit(HPI_HSUBSYS * phSubSys,
		     HPI_HGPIO hGpio, u16 wBitIndex, u16 wBitData)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_GPIO, HPI_GPIO_WRITE_BIT);
	HPI_HANDLETOINDEXES(hGpio, &hm.wAdapterIndex, NULL);	// only one dig i/o obj per adapter
	hm.u.l.wBitIndex = wBitIndex;
	hm.u.l.wBitData = wBitData;

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

///\}

////////////////////////////////////////////////////////////////////////////
/**\defgroup async Asynchronous Event Handling Functions
The asynchronous event module is designed to report events that occur on an adapter to an
\em interested application.

An Async object can be used to reciev notifications for both GPIO and other signal detection events.
A typical coding sequence would look something like:

\code

// The below code represents the bits that would go in an application.

...
// application startup section
HPI_AsyncEventOpen(pSS,0,&hAsync);
CreateThread(ThreadAsync);
...

...
// application shutdown
HPI_AsyncEventOpen(pSS,hAsync);
...

#define MAX_EVENTS 10
void ThreadAsync()
{
int ExitSignalled=0;
u16 wError;
tHPIAsyncEvent e[MAX_EVENTS]

while(!ExitSignalled)
{
wError = u16 HPI_AsyncEventWait( pSS, hAsync,
MAX_EVENTS,
&e[0],
&wEvents);

if(wError==HPI_ASYNC_TERMINATE_WAIT)
ExitSignalled=1;
else
{
if(!wError)
for(i=0;i<wEvents;i++)
CallCodeToProcessEvent(e[i]);
}

}
}

\endcode

\{
*/

/** Open an ASync object.
Opens a GPIO object and returns a handle to the same.
\return 0==success or HPI_ERROR_*
*/
u16 HPI_AsyncEventOpen(HPI_HSUBSYS * phSubSys,	///< Subsystem handle.
		       u16 wAdapterIndex,	///< The adapter index to open the Async object.
		       HPI_HASYNC * phAsync	///< Returned handle of an ASync object.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ASYNCEVENT, HPI_ASYNCEVENT_OPEN);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	if (hr.wError == 0) {
		*phAsync = HPI_IndexesToHandle(HPI_OBJ_ASYNCEVENT, wAdapterIndex, 0);	// only 1 nv-memory obj per adapter
	} else
		*phAsync = 0;
	return (hr.wError);

}

/** Closes an ASync object.
\return 0==success or HPI_ERROR_*
*/
u16 HPI_AsyncEventClose(HPI_HSUBSYS * phSubSys,	///< Subsystem handle.
			HPI_HASYNC hAsync	///< Handle of the Async object to close.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ASYNCEVENT, HPI_ASYNCEVENT_OPEN);
	HPI_HANDLETOINDEXES(hAsync, &hm.wAdapterIndex, NULL);

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/** Waits for a asynchronous events.
This call waits for any async event. The calling thread is suspended until an ASync event
is detected. After the async event is detected the call completes and returns information
about the event(s) that occured.
\return 0==success or HPI_ERROR_*
*/
u16 HPI_AsyncEventWait(HPI_HSUBSYS * phSubSys,	///< Subsystem handle.
		       HPI_HASYNC hAsync,	///< Handle of an Async object.
		       u16 wMaximumEvents,	///< Maximum number of events matches size of array passed in pEvents.
		       HPI_ASYNC_EVENT * pEvents,	///< Events are returned here.
		       u16 * pwNumberReturned	///< Number events returned.
    )
{
	return (0);
}

/** Returns the number of asynchronous events waiting.
\return 0==success or HPI_ERROR_*
*/
u16 HPI_AsyncEventGetCount(HPI_HSUBSYS * phSubSys,	///< Subsystem handle.
			   HPI_HASYNC hAsync,	///< Handle of an Async object.
			   u16 * pwCount	/// Returned number of events waiting.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ASYNCEVENT, HPI_ASYNCEVENT_GETCOUNT);
	HPI_HANDLETOINDEXES(hAsync, &hm.wAdapterIndex, NULL);

	HPI_Message(&hm, &hr);

	if (hr.wError == 0)
		if (pwCount)
			*pwCount = hr.u.as.u.count.wCount;

	return (hr.wError);
}

/** Returns single or many asynchronous events.
This call will read any waiting events from the asynchronous event queue and return a description of the event. It is
non-blocking.
\return 0==success or HPI_ERROR_*
*/
u16 HPI_AsyncEventGet(HPI_HSUBSYS * phSubSys,	///< Subsystem handle.
		      HPI_HASYNC hAsync,	///< Handle of an Async object.
		      u16 wMaximumEvents,	///< Maximum number of events matches size of array passed in pEvents.
		      HPI_ASYNC_EVENT * pEvents,	///< Events are returned here.
		      u16 * pwNumberReturned	///< Number events returned.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ASYNCEVENT, HPI_ASYNCEVENT_GET);
	HPI_HANDLETOINDEXES(hAsync, &hm.wAdapterIndex, NULL);

	HPI_Message(&hm, &hr);
	if (!hr.wError) {
		memcpy(pEvents, &hr.u.as.u.event, sizeof(HPI_ASYNC_EVENT));
		*pwNumberReturned = 1;
	}

	return (hr.wError);
}

///\}

////////////////////////////////////////////////////////////////////////////
/** \defgroup nvmem Nonvolatile memory

Some adapters contain non-volatile memory containing a number of bytes
The number of data words is adapter dependant and can be obtained
from the *pwSizeInBytes parameter returned from the Open function

There can be at most one nvmemory object per adapter.

@{
*/
u16 HPI_NvMemoryOpen(HPI_HSUBSYS * phSubSys,
		     u16 wAdapterIndex,
		     HPI_HNVMEMORY * phNvMemory, u16 * pwSizeInBytes)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_NVMEMORY, HPI_NVMEMORY_OPEN);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	if (hr.wError == 0) {
		*phNvMemory = HPI_IndexesToHandle(HPI_OBJ_NVMEMORY, wAdapterIndex, 0);	// only 1 nv-memory obj per adapter
		if (pwSizeInBytes)
			*pwSizeInBytes = hr.u.n.wSizeInBytes;
	} else
		*phNvMemory = 0;
	return (hr.wError);
}

u16 HPI_NvMemoryReadByte(HPI_HSUBSYS * phSubSys,
			 HPI_HNVMEMORY hNvMemory, u16 wIndex, u16 * pwData)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_NVMEMORY, HPI_NVMEMORY_READ_BYTE);
	HPI_HANDLETOINDEXES(hNvMemory, &hm.wAdapterIndex, NULL);	// only one NvMem obj per adapter
	hm.u.n.wIndex = wIndex;

	HPI_Message(&hm, &hr);

	*pwData = hr.u.n.wData;
	return (hr.wError);
}

u16 HPI_NvMemoryWriteByte(HPI_HSUBSYS * phSubSys,
			  HPI_HNVMEMORY hNvMemory, u16 wIndex, u16 wData)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_NVMEMORY, HPI_NVMEMORY_WRITE_BYTE);
	HPI_HANDLETOINDEXES(hNvMemory, &hm.wAdapterIndex, NULL);	// only one NvMem obj per adapter
	hm.u.n.wIndex = wIndex;
	hm.u.n.wData = wData;

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/** @}  */
////////////////////////////////////////////////////////////////////////////
/** \defgroup profile Profile
The Profile object supports profiling of the DSP code.
It should be used as a development tool for measuring DSP code operation.
Comments in AXPROF.H describe the DSP side of the profiling operation
@{
*/
/** open all the profiles on a particular adapter.

If the adapter does not have profiling enabled, the function will return an error.

The complete profile set of all profiles can be thought of as any array of execution timings/profiles.
Each indexed profile corresponds to the execution of a particular segment of DSP code.

Note that HPI_ProfileStartAll() must be called after HPI_ProfileOpenAll() to start the profiling operation on the DSP.
*/
u16 HPI_ProfileOpenAll(HPI_HSUBSYS * phSubSys,	///<[in] HPI subsystem handle
		       u16 wAdapterIndex,	///<[in] adapter index
		       u16 wProfileIndex,	///< corresponds to DSP index
		       HPI_HPROFILE * phProfile,	///<[out] profile handle
		       u16 * pwMaxProfiles	///<[out] maximum number of profile bins supported
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_PROFILE, HPI_PROFILE_OPEN_ALL);
	hm.wAdapterIndex = wAdapterIndex;
	hm.wDspIndex = wProfileIndex;
	HPI_Message(&hm, &hr);

	*pwMaxProfiles = hr.u.p.u.o.wMaxProfiles;
	if (hr.wError == 0)
		*phProfile =
		    HPI_IndexesToHandle(HPI_OBJ_PROFILE, wAdapterIndex,
					wProfileIndex);
	else
		*phProfile = 0;
	return (hr.wError);
}

u16 HPI_ProfileGet(HPI_HSUBSYS * phSubSys,
		   HPI_HPROFILE hProfile,
		   u16 wIndex,
		   u16 * pwSeconds,
		   u32 * pdwMicroSeconds,
		   u32 * pdwCallCount,
		   u32 * pdwMaxMicroSeconds, u32 * pdwMinMicroSeconds)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_PROFILE, HPI_PROFILE_GET);
	HPI_HANDLETOINDEXES(hProfile, &hm.wAdapterIndex, &hm.wDspIndex);
	hm.u.p.wIndex = wIndex;
	HPI_Message(&hm, &hr);
	if (pwSeconds)
		*pwSeconds = hr.u.p.u.t.wSeconds;
	if (pdwMicroSeconds)
		*pdwMicroSeconds = hr.u.p.u.t.dwMicroSeconds;
	if (pdwCallCount)
		*pdwCallCount = hr.u.p.u.t.dwCallCount;
	if (pdwMaxMicroSeconds)
		*pdwMaxMicroSeconds = hr.u.p.u.t.dwMaxMicroSeconds;
	if (pdwMinMicroSeconds)
		*pdwMinMicroSeconds = hr.u.p.u.t.dwMinMicroSeconds;
	return (hr.wError);
}

/**
\deprecated this function is no longer supported. Please use HPI_ProfileGetUtilization() instead.
*/
u16 HPI_ProfileGetIdleCount(HPI_HSUBSYS * phSubSys,
			    HPI_HPROFILE hProfile,
			    u16 wIndex, u32 * pdwIdleCycles, u32 * pdwCount)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_PROFILE, HPI_PROFILE_GET_IDLECOUNT);
	HPI_HANDLETOINDEXES(hProfile, &hm.wAdapterIndex, &hm.wDspIndex);
	hm.u.p.wIndex = wIndex;
	HPI_Message(&hm, &hr);
	if (hr.wError) {
		if (pdwIdleCycles)
			*pdwIdleCycles = 0;
		if (pdwCount)
			*pdwCount = 0;
	} else {
		if (pdwIdleCycles)
			*pdwIdleCycles = hr.u.p.u.t.dwMicroSeconds;
		if (pdwCount)
			*pdwCount = hr.u.p.u.t.dwCallCount;
	}
	return (hr.wError);
}

/** Get the DSP utilization in 1/100 of a percent
*/
u16 HPI_ProfileGetUtilization(HPI_HSUBSYS * phSubSys,	///<[in] HPI subsystem handle
			      HPI_HPROFILE hProfile,	///<[in] handle of profile object
			      u32 * pdwUtilization	///<[out] DSP utilization in 100ths of a percent
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_PROFILE, HPI_PROFILE_GET_UTILIZATION);
	HPI_HANDLETOINDEXES(hProfile, &hm.wAdapterIndex, &hm.wDspIndex);
	HPI_Message(&hm, &hr);
	if (hr.wError) {
		if (pdwUtilization)
			*pdwUtilization = 0;
	} else {
		if (pdwUtilization)
			*pdwUtilization = hr.u.p.u.t.dwCallCount;
	}
	return (hr.wError);
}

u16 HPI_ProfileGetName(HPI_HSUBSYS * phSubSys,
		       HPI_HPROFILE hProfile,
		       u16 wIndex, char *szName, u16 nNameLength)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_PROFILE, HPI_PROFILE_GET_NAME);
	HPI_HANDLETOINDEXES(hProfile, &hm.wAdapterIndex, &hm.wDspIndex);
	hm.u.p.wIndex = wIndex;
	HPI_Message(&hm, &hr);
	if (hr.wError) {
		if (szName)
			strcpy(szName, "??");
	} else {
		if (szName)
			memcpy(szName, (char *)hr.u.p.u.n.szName, nNameLength);
	}
	return (hr.wError);
}

u16 HPI_ProfileStartAll(HPI_HSUBSYS * phSubSys, HPI_HPROFILE hProfile)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_PROFILE, HPI_PROFILE_START_ALL);
	HPI_HANDLETOINDEXES(hProfile, &hm.wAdapterIndex, &hm.wDspIndex);
	HPI_Message(&hm, &hr);

	return (hr.wError);
}

u16 HPI_ProfileStopAll(HPI_HSUBSYS * phSubSys, HPI_HPROFILE hProfile)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_PROFILE, HPI_PROFILE_STOP_ALL);
	HPI_HANDLETOINDEXES(hProfile, &hm.wAdapterIndex, &hm.wDspIndex);
	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/** @} */
////////////////////////////////////////////////////////////////////////////
/**\defgroup watchdog  Watchdog timer
Only one watch-dog per adapter.
\note Not implemented on any adapters as of 2005-06-01
\{
*/
/// Open a watch dog object and return a handle
u16 HPI_WatchdogOpen(HPI_HSUBSYS * phSubSys,
		     u16 wAdapterIndex, HPI_HWATCHDOG * phWatchdog)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_WATCHDOG, HPI_WATCHDOG_OPEN);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	if (hr.wError == 0)
		*phWatchdog = HPI_IndexesToHandle(HPI_OBJ_WATCHDOG, wAdapterIndex, 0);	// only 1 watchdog obj per adapter
	else
		*phWatchdog = 0;
	return (hr.wError);
}

/** set the time of the watch dog object.
Time=0 will disable the watchdog
*/
u16 HPI_WatchdogSetTime(HPI_HSUBSYS * phSubSys,
			HPI_HWATCHDOG hWatchdog, u32 dwTimeMillisec)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_WATCHDOG, HPI_WATCHDOG_SET_TIME);
	HPI_HANDLETOINDEXES(hWatchdog, &hm.wAdapterIndex, NULL);	// only one watchdog obj per adapter
	hm.u.w.dwTimeMs = dwTimeMillisec;

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/// reset watchdog timer
u16 HPI_WatchdogPing(HPI_HSUBSYS * phSubSys, HPI_HWATCHDOG hWatchdog)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_WATCHDOG, HPI_WATCHDOG_PING);
	HPI_HANDLETOINDEXES(hWatchdog, &hm.wAdapterIndex, NULL);	// only one watchdog obj per adapter

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

///\}
#ifdef HPI_OS_DELETE

/////////////////////////////////////////////////////////////////////////////////
/**\defgroup aes18 AES18 data transfer over AES/EBU link
Currently only relevant to ASI4601 adapters.
\{
*/

#ifdef HAS_AES18_ON_HOST

//****** test vars for dummy code
u8 awTest[200];
u16 wLength;
//**********

/* !!! Test only !!! */
void HPI_Aes18Init(void)
{
	Aes18_Init();
}

#endif

u16 HPI_AES18BGSetConfiguration(HPI_HSUBSYS * phSubSys,
				HPI_HCONTROL hControlHandle,
				u16 wBlocksPerSec[HPI_AES18_MAX_CHANNELS],
				u16 wPriorityEnableMask[HPI_AES18_MAX_CHANNELS],
				u16 wOperatingMode[HPI_AES18_MAX_CHANNELS],
				u16 wChannelMode)
{
	u16 i;
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_CONTROLX_MSG_AES18BG *pAes18BGSetConfiguration = &hm.u.cx.u.aes18bg;

	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_CONTROLEX, HPI_CONTROL_SET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.cx.wControlIndex);
	hm.u.cx.wAttribute = HPI_AES18_CONFIG;

// Input bounds check
	for (i = 0; i < HPI_AES18_MAX_CHANNELS; i++) {
		switch (wBlocksPerSec[i]) {
		case 2:	// these are all ok
		case 5:
		case 24:
		case 25:
		case 33:
		case 100:
			break;
		default:
			return (HPI_ERROR_AES18BG_BLOCKSPERSEC);
		}

		if (wPriorityEnableMask[i] > HPI_AES18_MAX_PRIORITYMASK)
			return (HPI_ERROR_AES18BG_PRIORITYMASK);
		switch (wOperatingMode[i]) {
		case HPI_AES18_MODE_MASTER:
		case HPI_AES18_MODE_SLAVE:
			break;
		default:
			return (HPI_ERROR_AES18BG_MODE);
		}
		switch (wChannelMode) {
		case HPI_AES18_CHANNEL_MODE_JOINT:
		case HPI_AES18_CHANNEL_MODE_INDEPENDENT:
			break;
		default:
			return (HPI_ERROR_AES18BG_CHANNEL_MODE);
		}
	}

	for (i = 0; i < HPI_AES18_MAX_CHANNELS; i++) {
		pAes18BGSetConfiguration->wBlocksPerSec[i] = wBlocksPerSec[i];
		pAes18BGSetConfiguration->wPriorityMask[i] =
		    wPriorityEnableMask[i];
		pAes18BGSetConfiguration->wOperatingMode[i] = wOperatingMode[i];
	}
	pAes18BGSetConfiguration->wChannelMode = wChannelMode;

#ifndef HAS_AES18_ON_HOST

	HPI_Message(&hm, &hr);
#else

	AES18_Message(HPI_CONTROL_AES18_BLOCKGENERATOR, &hm, &hr);
#endif

	return (hr.wError);
}

u16 HPI_AES18RxSetAddress(HPI_HSUBSYS * phSubSys,
			  HPI_HCONTROL hControlHandle,
			  u16 awDecoderAddress[HPI_AES18_MAX_CHANNELS]
    )
{
	u16 i;
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_CONTROLX_MSG_AES18RX_ADDRESS *pAes18RxSetAddress =
	    &hm.u.cx.u.aes18rx_address;

	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_CONTROLEX, HPI_CONTROL_SET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.cx.wControlIndex);
	hm.u.cx.wAttribute = HPI_AES18_ADDRESS;

// Check input bounds
/* HPI_AES18_MAX_ADDRESS = 0x10000, so following check is not required
for (i=0;i<HPI_AES18_MAX_CHANNELS;i++)
{
if(awDecoderAddress[i] >= HPI_AES18_MAX_ADDRESS)
return(HPI_ERROR_AES18_INVALID_ADDRESS);
}
*/

	for (i = 0; i < HPI_AES18_MAX_CHANNELS; i++) {
		pAes18RxSetAddress->wAddress[i] = awDecoderAddress[i];
	}
#ifndef HAS_AES18_ON_HOST
	HPI_Message(&hm, &hr);
#else

	AES18_Message(HPI_CONTROL_AES18_RECEIVER, &hm, &hr);
#endif

	return (hr.wError);
}

/* Receiver */

u16 HPI_AES18RxGetInternalBufferState(HPI_HSUBSYS * phSubSys,
				      HPI_HCONTROL hControlHandle,
				      u16 awFrameError[HPI_AES18_MAX_CHANNELS],
				      u16
				      awMessageWaiting[HPI_AES18_MAX_CHANNELS]
				      [HPI_AES18_MAX_PRIORITIES],
				      u16
				      awInternalBufferOverFlow
				      [HPI_AES18_MAX_CHANNELS]
				      [HPI_AES18_MAX_PRIORITIES],
				      u16
				      awMissedMessage[HPI_AES18_MAX_CHANNELS]
				      [HPI_AES18_MAX_PRIORITIES]
    )
{
	u16 wChannelCount, wPriorityCount, wBitShift;
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_CONTROLX_RES_AES18RX_BUFFER_STATE *pAes18RxGetIBState =
	    &hr.u.cx.u.aes18rx_internal_buffer_state;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_CONTROLEX, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.cx.wControlIndex);
	hm.u.cx.wAttribute = HPI_AES18_INTERNAL_BUFFER_STATE;

#ifndef HAS_AES18_ON_HOST

	HPI_Message(&hm, &hr);
#else

	AES18_Message(HPI_CONTROL_AES18_RECEIVER, &hm, &hr);
#endif

/* Unpack the status bits from the response */
	if (awFrameError) {
		for (wChannelCount = 0; wChannelCount < HPI_AES18_MAX_CHANNELS;
		     wChannelCount++) {
			awFrameError[wChannelCount] =
			    (pAes18RxGetIBState->
			     awFrameErrorPacked >> wChannelCount) & 1;
		}
	}

/* NOTE!!! Also need to check awInternalBufferOverFlow and awMissedMessage */
	wBitShift = 0;
	if (awMessageWaiting) {
		for (wChannelCount = 0; wChannelCount < HPI_AES18_MAX_CHANNELS;
		     wChannelCount++) {
			for (wPriorityCount = 0;
			     wPriorityCount < HPI_AES18_MAX_PRIORITIES;
			     wPriorityCount++) {
				awMessageWaiting[wChannelCount][wPriorityCount]
				    =
				    (pAes18RxGetIBState->
				     awMessageWaitingPacked >> wBitShift) & 1;

				awInternalBufferOverFlow[wChannelCount]
				    [wPriorityCount] =
				    (pAes18RxGetIBState->
				     awInternalBufferOverFlowPacked >>
				     wBitShift) & 1;

				awMissedMessage[wChannelCount][wPriorityCount] =
				    (pAes18RxGetIBState->
				     awMissedMessagePacked >> (wBitShift++)) &
				    1;
			}
		}
	}

	return (hr.wError);

}

u16 HPI_AES18RxGetInternalBufferSize(HPI_HSUBSYS * phSubSys,
				     HPI_HCONTROL hControlHandle,
				     u16
				     awBytesPerBuffer[HPI_AES18_MAX_PRIORITIES]
    )
{
	u16 wPriorityCount;
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_CONTROLX_RES_AES18RX_BUFFER_SIZE *pAes18RxGetIBSize =
	    &hr.u.cx.u.aes18rx_internal_buffer_size;

	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_CONTROLEX, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.cx.wControlIndex);
	hm.u.cx.wAttribute = HPI_AES18_INTERNAL_BUFFER_SIZE;

#ifndef HAS_AES18_ON_HOST

	HPI_Message(&hm, &hr);
#else

	AES18_Message(HPI_CONTROL_AES18_RECEIVER, &hm, &hr);
#endif

	if (awBytesPerBuffer) {
		for (wPriorityCount = 0;
		     wPriorityCount < HPI_AES18_MAX_PRIORITIES;
		     wPriorityCount++) {
			awBytesPerBuffer[wPriorityCount] =
			    pAes18RxGetIBSize->awBytesPerBuffer[wPriorityCount];
		}
	}
	return (hr.wError);
}

u16 HPI_AES18RxGetMessage(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wChannel, u16 wPriority, u16 wQueueSize,	// In bytes
			  u8 * pbMessage, u16 * pwMessageLength	// in bytes
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_CONTROLX_RES_AES18RX_GET_MESSAGE *pAes18ResRxGetMessage =
	    &hr.u.cx.u.aes18rx_get_message;
	HPI_CONTROLX_MSG_AES18RX_GET_MESSAGE *pAes18MsgRxGetMessage =
	    &hm.u.cx.u.aes18rx_get_message;

	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_CONTROLEX, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.cx.wControlIndex);
	hm.u.cx.wAttribute = HPI_AES18_MESSAGE;

// Input bounds check
	if (wChannel >= HPI_AES18_MAX_CHANNELS)
		return (HPI_ERROR_INVALID_CHANNELS);

	if (wPriority > HPI_AES18_MAX_PRIORITIES)
		return (HPI_ERROR_AES18_INVALID_PRIORITY);

	pAes18MsgRxGetMessage->wChannel = wChannel;
	pAes18MsgRxGetMessage->wPriority = wPriority;
	pAes18MsgRxGetMessage->wQueueSize = wQueueSize;
	pAes18MsgRxGetMessage->dwpbMessage = (u32) pbMessage;

#ifndef HAS_AES18_ON_HOST

	HPI_Message(&hm, &hr);
#else

	AES18_Message(HPI_CONTROL_AES18_RECEIVER, &hm, &hr);
#endif

	*pwMessageLength = pAes18ResRxGetMessage->wReturnedMessageSize;
	return (hr.wError);
}

// Transmitter

u16 HPI_AES18TxSendMessage(HPI_HSUBSYS * phSubSys,
			   HPI_HCONTROL hControlHandle,
			   u16 wChannel,
			   u8 * pbMessage,
			   u16 wMessageLength,
			   u16 wDestinationAddress,
			   u16 wPriorityIndex, u16 wRepetitionIndex)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_CONTROLX_MSG_AES18TX_SEND_MESSAGE *pAes18SendMsg =
	    &hm.u.cx.u.aes18tx_send_message;

	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_CONTROLEX, HPI_CONTROL_SET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.cx.wControlIndex);
	hm.u.cx.wAttribute = HPI_AES18_MESSAGE;

// Input bounds check

	if (wChannel >= HPI_AES18_MAX_CHANNELS)
		return (HPI_ERROR_INVALID_CHANNELS);

	if (wPriorityIndex > HPI_AES18_MAX_PRIORITIES)
		return (HPI_ERROR_AES18_INVALID_PRIORITY);

/* HPI_AES18_MAX_ADDRESS = 0x10000, so following test is not required
if(wDestinationAddress > HPI_AES18_MAX_ADDRESS)
return(HPI_ERROR_AES18_INVALID_ADDRESS);
*/

	if (wRepetitionIndex > HPI_AES18_MAX_REPETITION)
		return (HPI_ERROR_AES18_INVALID_REPETITION);

	pAes18SendMsg->wChannel = wChannel;
	pAes18SendMsg->dwpbMessage = (u32) pbMessage;
	pAes18SendMsg->wMessageLength = wMessageLength;
	pAes18SendMsg->wDestinationAddress = wDestinationAddress;
	pAes18SendMsg->wPriorityIndex = wPriorityIndex;
	pAes18SendMsg->wRepetitionIndex = wRepetitionIndex;

#ifndef HAS_AES18_ON_HOST
	HPI_Message(&hm, &hr);
#else

	AES18_Message(HPI_CONTROL_AES18_TRANSMITTER, &hm, &hr);
#endif

	return (hr.wError);
}

u16 HPI_AES18TxGetInternalBufferState(HPI_HSUBSYS * phSubSys,
				      HPI_HCONTROL hControlHandle,
				      u16
				      awInternalBufferBusy
				      [HPI_AES18_MAX_CHANNELS]
				      [HPI_AES18_MAX_PRIORITIES]
    )
{
	u16 wChannelCount, wPriorityCount, wBitShift, wPackedBits;
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_CONTROLX_RES_AES18TX_BUFFER_STATE *pAes18TxGetIBState =
	    &hr.u.cx.u.aes18tx_internal_buffer_state;
	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_CONTROLEX, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.cx.wControlIndex);
	hm.u.cx.wAttribute = HPI_AES18_INTERNAL_BUFFER_STATE;

	wBitShift = 0;

#ifndef HAS_AES18_ON_HOST

	HPI_Message(&hm, &hr);
#else

	AES18_Message(HPI_CONTROL_AES18_TRANSMITTER, &hm, &hr);
#endif

	wPackedBits = pAes18TxGetIBState->wInternalBufferBusyPacked;

/* Unpack the status bits from the response */
	if (awInternalBufferBusy) {
		for (wChannelCount = 0; wChannelCount < HPI_AES18_MAX_CHANNELS;
		     wChannelCount++) {
			for (wPriorityCount = 0;
			     wPriorityCount < HPI_AES18_MAX_PRIORITIES;
			     wPriorityCount++) {
				awInternalBufferBusy[wChannelCount]
				    [wPriorityCount] =
				    (wPackedBits >> (wBitShift++)) & 1;
			}
		}
	}
	return (hr.wError);

};

u16 HPI_AES18TxGetInternalBufferSize(HPI_HSUBSYS * phSubSys,
				     HPI_HCONTROL hControlHandle,
				     u16
				     awBytesPerBuffer[HPI_AES18_MAX_PRIORITIES]
    )
{
	u16 wPriorityCount;
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_CONTROLX_RES_AES18TX_BUFFER_SIZE *pAes18TxGetIBSize =
	    &hr.u.cx.u.aes18tx_internal_buffer_size;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_CONTROLEX, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.cx.wControlIndex);
	hm.u.cx.wAttribute = HPI_AES18_INTERNAL_BUFFER_SIZE;

#ifndef HAS_AES18_ON_HOST
	HPI_Message(&hm, &hr);
#else
	AES18_Message(HPI_CONTROL_AES18_TRANSMITTER, &hm, &hr);
#endif

	if (awBytesPerBuffer) {
		for (wPriorityCount = 0;
		     wPriorityCount < HPI_AES18_MAX_PRIORITIES;
		     wPriorityCount++) {
			awBytesPerBuffer[wPriorityCount] =
			    pAes18TxGetIBSize->awBytesPerBuffer[wPriorityCount];
		}
	}
	return (hr.wError);
}

#ifdef HAS_AES18_ON_HOST
/* prototype the functions in aes18.c */
int Aes18BgCtrl_Set(int nIndex, HPI_CONTROL_MSG * pCm, HPI_CONTROL_RES * pCr);
int Aes18RxCtrl_Set(int nIndex, HPI_CONTROL_MSG * pCm, HPI_CONTROL_RES * pCr);
int Aes18RxCtrl_Get(int nIndex, HPI_CONTROL_MSG * pCm, HPI_CONTROL_RES * pCr);
int Aes18TxCtrl_Set(int nIndex, HPI_CONTROL_MSG * pCm, HPI_CONTROL_RES * pCr);
int Aes18TxCtrl_Get(int nIndex, HPI_CONTROL_MSG * pCm, HPI_CONTROL_RES * pCr);

// Copied from ax4\axctrl.c DSP code.
// this function takes the control type as a parameter because when running
// on the pc, we dont get a correct control index

void AES18_Message(int wControlType, HPI_MESSAGE * pHm, HPI_RESPONSE * pHr)
{
	HPI_CONTROL_MSG *pCm = &pHm->u.c;
	HPI_CONTROL_RES *pCr = &pHr->u.c;
	int nIndex = 0;

	pHr->wError = HPI_ERROR_INVALID_CONTROL;
//  if( pCm->wControlIndex > nNumControls )
//      return;

	switch (pHm->wFunction) {
	case HPI_CONTROL_GET_STATE:
		switch (wControlType) {
		case HPI_CONTROL_AES18_TRANSMITTER:
			Aes18TxCtrl_Get(nIndex, pCm, pCr);
			break;
		case HPI_CONTROL_AES18_RECEIVER:
			Aes18RxCtrl_Get(nIndex, pCm, pCr);
			break;
		case HPI_CONTROL_AES18_BLOCKGENERATOR:
			break;
		}
		break;
	case HPI_CONTROL_SET_STATE:
		switch (wControlType) {
		case HPI_CONTROL_AES18_TRANSMITTER:
			Aes18TxCtrl_Set(nIndex, pCm, pCr);
			break;
		case HPI_CONTROL_AES18_RECEIVER:
			Aes18RxCtrl_Set(nIndex, pCm, pCr);
			break;
		case HPI_CONTROL_AES18_BLOCKGENERATOR:
			Aes18BgCtrl_Set(nIndex, pCm, pCr);
			break;
		}
		break;
	default:
		return;
	}

}
#endif				/* HAS_AES18_ON_HOST */
///\}
#elif defined HPIDLL_EXPORTS

#define HPI_AES18_MAX_CHANNELS  2
#define HPI_AES18_MAX_PRIORITIES 4

/* stub functions for dll compatibility */
u16 HPI_AES18BGSetConfiguration(HPI_HSUBSYS * phSubSys,
				HPI_HCONTROL hControlHandle,
				u16 wBlocksPerSec[HPI_AES18_MAX_CHANNELS],
				u16 wPriorityEnableMask[HPI_AES18_MAX_CHANNELS],
				u16 wOperatingMode[HPI_AES18_MAX_CHANNELS],
				u16 wChannelMode)
{
	return HPI_ERROR_INVALID_FUNC;
}

u16 HPI_AES18RxSetAddress(HPI_HSUBSYS * phSubSys,
			  HPI_HCONTROL hControlHandle,
			  u16 awDecoderAddress[HPI_AES18_MAX_CHANNELS]
    )
{
	return HPI_ERROR_INVALID_FUNC;
}

u16 HPI_AES18RxGetInternalBufferState(HPI_HSUBSYS * phSubSys,
				      HPI_HCONTROL hControlHandle,
				      u16 awFrameError[HPI_AES18_MAX_CHANNELS],
				      u16
				      awMessageWaiting[HPI_AES18_MAX_CHANNELS]
				      [HPI_AES18_MAX_PRIORITIES],
				      u16
				      awInternalBufferOverFlow
				      [HPI_AES18_MAX_CHANNELS]
				      [HPI_AES18_MAX_PRIORITIES],
				      u16
				      awMissedMessage[HPI_AES18_MAX_CHANNELS]
				      [HPI_AES18_MAX_PRIORITIES]
    )
{
	return HPI_ERROR_INVALID_FUNC;
}

u16 HPI_AES18RxGetInternalBufferSize(HPI_HSUBSYS * phSubSys,
				     HPI_HCONTROL hControlHandle,
				     u16
				     awBytesPerBuffer[HPI_AES18_MAX_PRIORITIES]
    )
{
	return HPI_ERROR_INVALID_FUNC;
}

u16 HPI_AES18RxGetMessage(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControlHandle, u16 wChannel, u16 wPriority, u16 wQueueSize,	// In bytes
			  u8 * pbMessage, u16 * pwMessageLength	// in bytes
    )
{
	return HPI_ERROR_INVALID_FUNC;
}

u16 HPI_AES18TxSendMessage(HPI_HSUBSYS * phSubSys,
			   HPI_HCONTROL hControlHandle,
			   u16 wChannel,
			   u8 * pbMessage,
			   u16 wMessageLength,
			   u16 wDestinationAddress,
			   u16 wPriorityIndex, u16 wRepetitionIndex)
{
	return HPI_ERROR_INVALID_FUNC;
}

u16 HPI_AES18TxGetInternalBufferState(HPI_HSUBSYS * phSubSys,
				      HPI_HCONTROL hControlHandle,
				      u16
				      awInternalBufferBusy
				      [HPI_AES18_MAX_CHANNELS]
				      [HPI_AES18_MAX_PRIORITIES]
    )
{
	return HPI_ERROR_INVALID_FUNC;
}

u16 HPI_AES18TxGetInternalBufferSize(HPI_HSUBSYS * phSubSys,
				     HPI_HCONTROL hControlHandle,
				     u16
				     awBytesPerBuffer[HPI_AES18_MAX_PRIORITIES]
    )
{
	return HPI_ERROR_INVALID_FUNC;
}

#endif				/* elif defined HPIDLL_EXPORTS */
/////////////////////////////////////////////////////////////////////////
/** \defgroup utility Utility Functions
@{
*/

#if 0
/// global to store specifc errors for use by HPI_GetLastErrorDetail
u32 gadwHpiSpecificError[4] = { 0, 0, 0, 0 };

/// Get HW specific errors
void HPI_GetLastErrorDetail(u16 wError, char *pszErrorText,
			    u32 ** padwSpecificError)
{
/* Need to get this via HPI message interface */
	*padwSpecificError = &gadwHpiSpecificError[0];	// send back pointer to array of errors
	HPI_GetErrorText(wError, pszErrorText);
}
#endif

/**
*/

void HPI_GetErrorText(u16 wError, char *pszErrorText)
{
	strcpy(pszErrorText, " ");

	sprintf(pszErrorText, "#%3d - ", wError);

	switch (wError) {
	case 0:
		strcat(pszErrorText, "No Error");
		break;
	case HPI_ERROR_INVALID_TYPE:	//100 // message type does not exist
		strcat(pszErrorText, "Invalid message type");
		break;
	case HPI_ERROR_INVALID_OBJ:	//101 // object type does not exist
		strcat(pszErrorText, "Invalid object type");
		break;
	case HPI_ERROR_INVALID_FUNC:	//102 // function does not exist
		strcat(pszErrorText, "Invalid function");
		break;
	case HPI_ERROR_INVALID_OBJ_INDEX:	//103 // the specified object (adapter/Stream) does not exist
		strcat(pszErrorText,
		       "Invalid object (adapter/stream/mixer/control) index");
		break;
	case HPI_ERROR_OBJ_NOT_OPEN:	//104
		strcat(pszErrorText, "Object not open");
		break;
	case HPI_ERROR_OBJ_ALREADY_OPEN:	//105
		strcat(pszErrorText, "Object already in use (opened)");
		break;
	case HPI_ERROR_INVALID_RESOURCE:	//106 // PCI, ISA resource not valid
		strcat(pszErrorText, "Invalid bus/port resource");
		break;
	case HPI_ERROR_SUBSYSFINDADAPTERS_GETINFO:	//107
		strcat(pszErrorText,
		       "GetInfo call from SubSysFindAdapters failed");
		break;
	case HPI_ERROR_INVALID_RESPONSE:	// 108
		strcat(pszErrorText, "Invalid HPI Response");
		break;
	case HPI_ERROR_PROCESSING_MESSAGE:	// 109
		strcat(pszErrorText, "wSize field of response was not updated");
		break;
	case HPI_ERROR_NETWORK_TIMEOUT:	//110
		strcat(pszErrorText, "Network timeout waiting for response");
		break;
	case HPI_ERROR_INVALID_HANDLE:	//111
		strcat(pszErrorText, "Invalid HPI handle (uninitialised?)");
		break;
	case HPI_ERROR_UNIMPLEMENTED:	//1112
		strcat(pszErrorText, "Functionality not yet implemented");
		break;

	case HPI_ERROR_TOO_MANY_ADAPTERS:	//200
		strcat(pszErrorText, "Too many adapters");
		break;
	case HPI_ERROR_BAD_ADAPTER:	//201
		strcat(pszErrorText, "Bad adapter");
		break;
	case HPI_ERROR_BAD_ADAPTER_NUMBER:	//202    // adapter number out of range or not set properly
		strcat(pszErrorText, "Invalid adapter index");
		break;
	case HPI_DUPLICATE_ADAPTER_NUMBER:	//203    // 2 adapters with the same adapter number
		strcat(pszErrorText, "Duplicate adapter index");
		break;
	case HPI_ERROR_DSP_BOOTLOAD:	//204
		strcat(pszErrorText, "DSP failed bootload");
		break;
	case HPI_ERROR_DSP_SELFTEST:	//205
		strcat(pszErrorText, "DSP failed selftest");
		break;
	case HPI_ERROR_DSP_FILE_NOT_FOUND:	//206
		strcat(pszErrorText, "Failed to find/open DSP code file");
		break;
	case HPI_ERROR_DSP_HARDWARE:	//207
		strcat(pszErrorText, "internal DSP hardware error");
		break;
	case HPI_ERROR_DOS_MEMORY_ALLOC:	//208
		strcat(pszErrorText, "could not allocate memory in DOS");
		break;
	case HPI_ERROR_PLD_LOAD:	//209
		strcat(pszErrorText, "PLD could not be configured");
		break;
	case HPI_ERROR_DSP_FILE_FORMAT:	//210
		strcat(pszErrorText,
		       "Invalid DSP code file format (corrupt file?)");
		break;

	case HPI_ERROR_DSP_FILE_ACCESS_DENIED:	//211
		strcat(pszErrorText, "Found but could not open DSP code file");
		break;
	case HPI_ERROR_DSP_FILE_NO_HEADER:	//212
		strcat(pszErrorText,
		       "First DSP code section header not found in DSP file");
		break;
	case HPI_ERROR_DSP_FILE_READ_ERROR:	//213
		strcat(pszErrorText,
		       "File read operation on DSP code file failed");
		break;
	case HPI_ERROR_DSP_SECTION_NOT_FOUND:	//214
		strcat(pszErrorText, "DSP code for adapter family not found");
		break;
	case HPI_ERROR_DSP_FILE_OTHER_ERROR:	//215
		strcat(pszErrorText,
		       "Other OS specific error opening DSP file");
		break;
	case HPI_ERROR_DSP_FILE_SHARING_VIOLATION:	//216
		strcat(pszErrorText, "Sharing violation opening DSP code file");
		break;

	case HPI_ERROR_BAD_CHECKSUM:	//221
		strcat(pszErrorText,
		       "Flash - could not determine a valid checksum");
		break;
	case HPI_ERROR_BAD_SEQUENCE:	//222
		strcat(pszErrorText,
		       "Flash - bad packet sequence number during flash programming");
		break;
	case HPI_ERROR_FLASH_ERASE:	//223
		strcat(pszErrorText, "Flash - erase failed");
		break;
	case HPI_ERROR_FLASH_PROGRAM:	//224
		strcat(pszErrorText, "Flash - programming failed");
		break;
	case HPI_ERROR_FLASH_VERIFY:	//225
		strcat(pszErrorText, "Flash - verification failed");
		break;
	case HPI_ERROR_FLASH_TYPE:	//226
		strcat(pszErrorText, "Flash - wrong type of flash on hardware");
		break;
	case HPI_ERROR_FLASH_START:	//227
		strcat(pszErrorText,
		       "Flash - command to start programming sequence failed");
		break;

	case HPI_ERROR_INVALID_STREAM:	//300
		strcat(pszErrorText, "Invalid stream");
		break;
	case HPI_ERROR_INVALID_FORMAT:	//301
		strcat(pszErrorText, "Invalid Format");
		break;
	case HPI_ERROR_INVALID_SAMPLERATE:	//  302
		strcat(pszErrorText, "Invalid format sample rate");
		break;
	case HPI_ERROR_INVALID_CHANNELS:	//  303
		strcat(pszErrorText, "Invalid format number of channels");
		break;
	case HPI_ERROR_INVALID_BITRATE:	//  304
		strcat(pszErrorText, "Invalid format bitrate");
		break;
	case HPI_ERROR_INVALID_DATASIZE:	// 305
		strcat(pszErrorText,
		       "Invalid datasize used for stream read/write");
		break;
	case HPI_ERROR_BUFFER_FULL:	// 306
		strcat(pszErrorText,
		       "Stream buffer is full during stream write");
		break;
	case HPI_ERROR_BUFFER_EMPTY:	// 307
		strcat(pszErrorText,
		       "stream buffer is empty during stream read");
		break;
	case HPI_ERROR_INVALID_DATA_TRANSFER:	// 308
		strcat(pszErrorText,
		       "invalid datasize used for stream read/write");
		break;
	case HPI_ERROR_INVALID_OPERATION:	// 310
		strcat(pszErrorText,
		       "Invalid operation - object can't do requested operation in its current state");
		break;
	case HPI_ERROR_INCOMPATIBLE_SAMPLERATE:	// 311
		strcat(pszErrorText, "Cannot change to requested samplerate");
		break;
	case HPI_ERROR_BAD_ADAPTER_MODE:	// 312
		strcat(pszErrorText, "Invalid adapter mode");
		break;
	case HPI_ERROR_TOO_MANY_CAPABILITY_CHANGE_ATTEMPTS:	// 313
		strcat(pszErrorText,
		       "There have been too many attempts to set the adapter's "
		       "capabilities (using bad keys). The card should be returned "
		       "to ASI if further capabilities updates are required");
		break;
	case HPI_ERROR_NO_INTERADAPTER_GROUPS:	// 314
		strcat(pszErrorText,
		       "Streams on different adapters cannot be grouped.");
		break;
	case HPI_ERROR_NO_INTERDSP_GROUPS:	// 315
		strcat(pszErrorText,
		       "Streams on different DSPs cannot be grouped.");
		break;

	case HPI_ERROR_INVALID_NODE:	//400
		strcat(pszErrorText, "Invalid mixer node");
		break;
	case HPI_ERROR_INVALID_CONTROL:	//401
		strcat(pszErrorText, "Invalid mixer control");
		break;
	case HPI_ERROR_INVALID_CONTROL_VALUE:	// 402
		strcat(pszErrorText, "Invalid mixer control value");
		break;
	case HPI_ERROR_INVALID_CONTROL_ATTRIBUTE:	// 403
		strcat(pszErrorText, "Invalid mixer control attribute");
		break;
	case HPI_ERROR_CONTROL_DISABLED:	// 404
		strcat(pszErrorText, "Mixer control disabled");
		break;

	case HPI_ERROR_NVMEM_BUSY:	// 450
		strcat(pszErrorText, "Non-volatile memory is busy");
		break;

	default:
		strcat(pszErrorText, "Unknown Error");
	}
}

/** Initialize an audio format structure, given various defining parameters

*/
u16 HPI_FormatCreate(HPI_FORMAT * pFormat,	///< [out] Format structure to be initialized
		     u16 wChannels,	///< From 1 to 8
		     u16 wFormat,	///< One of the \ref formats
		     u32 dwSampleRate,	///< Hz. Samplerate must be between 8000 and 200000
		     u32 dwBitRate,	///< bits per second, must be supplied for MPEG formats, it is calculated for PCM formats
		     u32 dwAttributes	///< format dependent attributes.  E.g. \ref mpegmodes
    )
{
	u16 wError = 0;
	HPI_MSG_FORMAT Format;

// can be mono or stereo
	if (wChannels < 1)
		wChannels = 1;
	if (wChannels > 8)
		wChannels = 8;

	Format.wChannels = wChannels;

// make sure we have a valid audio format
	switch (wFormat) {
	case HPI_FORMAT_PCM16_SIGNED:
	case HPI_FORMAT_PCM24_SIGNED:
	case HPI_FORMAT_PCM32_SIGNED:
	case HPI_FORMAT_PCM32_FLOAT:
	case HPI_FORMAT_PCM16_BIGENDIAN:
	case HPI_FORMAT_PCM8_UNSIGNED:
	case HPI_FORMAT_MPEG_L1:
	case HPI_FORMAT_MPEG_L2:
	case HPI_FORMAT_MPEG_L3:
	case HPI_FORMAT_DOLBY_AC2:
	case HPI_FORMAT_AA_TAGIT1_HITS:
	case HPI_FORMAT_AA_TAGIT1_INSERTS:
	case HPI_FORMAT_RAW_BITSTREAM:
	case HPI_FORMAT_AA_TAGIT1_HITS_EX1:
	case HPI_FORMAT_OEM1:
	case HPI_FORMAT_OEM2:
		break;
	default:
		wError = HPI_ERROR_INVALID_FORMAT;
		return (wError);
	}
	Format.wFormat = wFormat;

//sample rate can be between 8kHz and 200kHz
	if (dwSampleRate < 8000L) {
		wError = HPI_ERROR_INCOMPATIBLE_SAMPLERATE;
		dwSampleRate = 8000L;
	}
	if (dwSampleRate > 200000L) {
		wError = HPI_ERROR_INCOMPATIBLE_SAMPLERATE;
		dwSampleRate = 200000L;
	}
	Format.dwSampleRate = dwSampleRate;

// for some formats (MPEG) we accept a bitrate
// for some (PCM) we calculate the bit rate
	switch (wFormat) {
	case HPI_FORMAT_MPEG_L1:
	case HPI_FORMAT_MPEG_L2:
	case HPI_FORMAT_MPEG_L3:
		Format.dwBitRate = dwBitRate;	// should validate!!!!!!!
		break;
	case HPI_FORMAT_PCM16_SIGNED:
	case HPI_FORMAT_PCM16_BIGENDIAN:
		Format.dwBitRate = (u32) wChannels *dwSampleRate * 2;
		break;
	case HPI_FORMAT_PCM32_SIGNED:
	case HPI_FORMAT_PCM32_FLOAT:
		Format.dwBitRate = (u32) wChannels *dwSampleRate * 4;
		break;
	case HPI_FORMAT_PCM8_UNSIGNED:
		Format.dwBitRate = (u32) wChannels *dwSampleRate;
		break;
	default:
		Format.dwBitRate = 0;
	}

// Set the dwAttributes field.
// The attributes are format dependent.
	switch (wFormat) {
	case HPI_FORMAT_MPEG_L2:
		if ((wChannels == 1) && (dwAttributes != HPI_MPEG_MODE_DEFAULT)) {
			dwAttributes = HPI_MPEG_MODE_DEFAULT;	// correct the error anyway !
			wError = HPI_ERROR_INVALID_FORMAT;
		} else if (dwAttributes > HPI_MPEG_MODE_DUALCHANNEL) {
			dwAttributes = HPI_MPEG_MODE_DEFAULT;	// correct the error anyway !
			wError = HPI_ERROR_INVALID_FORMAT;
		}
		Format.dwAttributes = dwAttributes;
		break;
	default:
		Format.dwAttributes = dwAttributes;
	}

	HPI_MsgToFormat(pFormat, &Format);
	return (wError);
}

#if defined ( HPI_OS_WIN16 ) || defined ( HPI_OS_WIN32_USER ) || defined ( INCLUDE_WINDOWS_ON_LINUX )
static unsigned char PcmSubformatGUID[16] =
    { 1, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9B, 0x71 };

u16 HPI_WaveFormatToHpiFormat(const PWAVEFORMATEX lpFormatEx,
			      HPI_FORMAT * pHpiFormat)
{
	u16 wError = 0;

	switch (lpFormatEx->wFormatTag) {
#if defined ( WAVE_FORMAT_EXTENSIBLE ) && defined ( _WAVEFORMATEXTENSIBLE_ )
	case WAVE_FORMAT_EXTENSIBLE:
// Make sure the subformat is PCM
		if (memcmp
		    (&((PWAVEFORMATEXTENSIBLE) lpFormatEx)->SubFormat,
		     PcmSubformatGUID, 16) != 0) {
			wError = HPI_ERROR_INVALID_FORMAT;
			break;
		}
// else fallthrough
#endif
	case WAVE_FORMAT_PCM:
//DBGPRINTF0(DEBUG_MASK_WOD_CUSTOM,TEXT("PCM"));
		if (lpFormatEx->wBitsPerSample == 16)
			pHpiFormat->wFormat = HPI_FORMAT_PCM16_SIGNED;
		else if (lpFormatEx->wBitsPerSample == 8)
			pHpiFormat->wFormat = HPI_FORMAT_PCM8_UNSIGNED;
		else if (lpFormatEx->wBitsPerSample == 24)
			pHpiFormat->wFormat = HPI_FORMAT_PCM24_SIGNED;
		else if (lpFormatEx->wBitsPerSample == 32)
			pHpiFormat->wFormat = HPI_FORMAT_PCM32_SIGNED;
		else {
			pHpiFormat->wFormat = 0;
			wError = HPI_ERROR_INVALID_FORMAT;
		}
		break;
	case WAVE_FORMAT_IEEE_FLOAT:
		if (lpFormatEx->wBitsPerSample == 32)
			pHpiFormat->wFormat = HPI_FORMAT_PCM32_FLOAT;
		else {
			pHpiFormat->wFormat = 0;
			wError = HPI_ERROR_INVALID_FORMAT;
		}
		break;

	case WAVE_FORMAT_DOLBY_AC2:
//DBGPRINTF0(DEBUG_MASK_WOD_CUSTOM,TEXT("AC2"));
		pHpiFormat->wFormat = HPI_FORMAT_DOLBY_AC2;
		break;

	case WAVE_FORMAT_MPEG:
		switch (((MPEG1WAVEFORMAT *) lpFormatEx)->fwHeadLayer) {
		case ACM_MPEG_LAYER1:
			pHpiFormat->wFormat = HPI_FORMAT_MPEG_L1;
			break;
		case ACM_MPEG_LAYER2:
			pHpiFormat->wFormat = HPI_FORMAT_MPEG_L2;
			break;
		case ACM_MPEG_LAYER3:
			pHpiFormat->wFormat = HPI_FORMAT_MPEG_L3;
			break;
		default:
			pHpiFormat->wFormat = HPI_FORMAT_MPEG_L2;
			break;	// really should be error
		}
		pHpiFormat->dwBitRate =
		    ((MPEG1WAVEFORMAT *) lpFormatEx)->dwHeadBitrate;
		if (pHpiFormat->dwBitRate == 0)
			pHpiFormat->dwBitRate = 256000L;	// must have a default
		break;
	case WAVE_FORMAT_MPEGLAYER3:
		pHpiFormat->wFormat = HPI_FORMAT_MPEG_L3;
		pHpiFormat->dwBitRate =
		    ((MPEGLAYER3WAVEFORMAT *) lpFormatEx)->wfx.nAvgBytesPerSec *
		    8;
		if (pHpiFormat->dwBitRate == 0)
			pHpiFormat->dwBitRate = 256000L;	// must have a default
		break;

	default:
		wError = HPI_ERROR_INVALID_FORMAT;
	}
	pHpiFormat->wChannels = lpFormatEx->nChannels;
	pHpiFormat->dwSampleRate = lpFormatEx->nSamplesPerSec;
	pHpiFormat->dwAttributes = 0;
	pHpiFormat->wModeLegacy = 0;
	pHpiFormat->wUnused = 0;

	return (wError);
}

u16 HPI_HpiFormatToWaveFormat(const HPI_FORMAT * pHpiFormat,
			      PWAVEFORMATEX lpFormatEx)
{
	u16 wError = 0;

	lpFormatEx->cbSize = 0;

	lpFormatEx->nChannels = pHpiFormat->wChannels;
	lpFormatEx->nSamplesPerSec = pHpiFormat->dwSampleRate;

	switch (pHpiFormat->wFormat) {
	case HPI_FORMAT_PCM8_UNSIGNED:
		lpFormatEx->nAvgBytesPerSec =
		    pHpiFormat->wChannels * pHpiFormat->dwSampleRate;
		lpFormatEx->nBlockAlign = pHpiFormat->wChannels;
		lpFormatEx->wBitsPerSample = 8;
		lpFormatEx->wFormatTag = WAVE_FORMAT_PCM;
		break;
	case HPI_FORMAT_PCM16_SIGNED:
		lpFormatEx->nAvgBytesPerSec =
		    2 * pHpiFormat->wChannels * pHpiFormat->dwSampleRate;
		lpFormatEx->nBlockAlign = 2 * pHpiFormat->wChannels;
		lpFormatEx->wBitsPerSample = 16;
		lpFormatEx->wFormatTag = WAVE_FORMAT_PCM;
		break;
	case HPI_FORMAT_PCM24_SIGNED:
		lpFormatEx->nAvgBytesPerSec =
		    3 * pHpiFormat->wChannels * pHpiFormat->dwSampleRate;
		lpFormatEx->nBlockAlign = 3 * pHpiFormat->wChannels;
		lpFormatEx->wBitsPerSample = 24;
		lpFormatEx->wFormatTag = WAVE_FORMAT_PCM;
		break;

	case HPI_FORMAT_PCM32_SIGNED:
		lpFormatEx->nAvgBytesPerSec =
		    4 * pHpiFormat->wChannels * pHpiFormat->dwSampleRate;
		lpFormatEx->nBlockAlign = 4 * pHpiFormat->wChannels;
		lpFormatEx->wBitsPerSample = 32;
		lpFormatEx->wFormatTag = WAVE_FORMAT_PCM;
		break;

	case HPI_FORMAT_PCM32_FLOAT:
		lpFormatEx->nAvgBytesPerSec =
		    4 * pHpiFormat->wChannels * pHpiFormat->dwSampleRate;
		lpFormatEx->nBlockAlign = 4 * pHpiFormat->wChannels;
		lpFormatEx->wBitsPerSample = 32;
		lpFormatEx->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
		break;

	case HPI_FORMAT_AA_TAGIT1_HITS:
		lpFormatEx->nAvgBytesPerSec =
		    2 * pHpiFormat->wChannels * pHpiFormat->dwSampleRate;
		lpFormatEx->nBlockAlign = 2 * pHpiFormat->wChannels;
		lpFormatEx->wBitsPerSample = 16;
		lpFormatEx->wFormatTag = WAVE_FORMAT_DEVELOPMENT;
		break;

//this is setup as MPEG layer 2
	case HPI_FORMAT_MPEG_L2:
	case HPI_FORMAT_MPEG_L3:
		lpFormatEx->wFormatTag = WAVE_FORMAT_MPEG;
		lpFormatEx->wBitsPerSample = 0;
// this is for 44.1kHz, stereo, 256kbs
		lpFormatEx->nBlockAlign
		    =
		    (unsigned short)((144 * pHpiFormat->dwBitRate) /
				     pHpiFormat->dwSampleRate);
		lpFormatEx->nAvgBytesPerSec = pHpiFormat->dwBitRate / 8;
		lpFormatEx->cbSize = 22;
		if (pHpiFormat->wFormat == HPI_FORMAT_MPEG_L2)
			((MPEG1WAVEFORMAT *) lpFormatEx)->fwHeadLayer =
			    ACM_MPEG_LAYER2;
		else
			((MPEG1WAVEFORMAT *) lpFormatEx)->fwHeadLayer =
			    ACM_MPEG_LAYER3;
//this makes the desired rather than actual
		((MPEG1WAVEFORMAT *) lpFormatEx)->dwHeadBitrate =
		    pHpiFormat->dwBitRate;
		if (pHpiFormat->wChannels == 2)
			((MPEG1WAVEFORMAT *) lpFormatEx)->fwHeadMode =
			    ACM_MPEG_JOINTSTEREO;
		else
			((MPEG1WAVEFORMAT *) lpFormatEx)->fwHeadMode =
			    ACM_MPEG_SINGLECHANNEL;

		((MPEG1WAVEFORMAT *) lpFormatEx)->fwHeadModeExt = 0;	//0x000F;   //??
		((MPEG1WAVEFORMAT *) lpFormatEx)->wHeadEmphasis = 0;
		((MPEG1WAVEFORMAT *) lpFormatEx)->fwHeadFlags = 0;	//ACM_MPEG_ID_MPEG1;
		((MPEG1WAVEFORMAT *) lpFormatEx)->dwPTSLow = 0;
		((MPEG1WAVEFORMAT *) lpFormatEx)->dwPTSHigh = 0;
		break;

	default:
		wError = HPI_ERROR_INVALID_FORMAT;
	}

	return (wError);
}

#endif				/* defined(HPI_OS_WIN16) || defined(HPI_OS_WIN32_USER) */

#ifndef HPI_WITHOUT_HPI_DATA
/** Initialize a HPI_DATA structure
\deprecated HPI_DATA struct may be removed from a future version.
To avoid using HPI_DATA switch to using HPI_OutStreamWriteBuf() and HPI_InStreamReadBuf()
*/
u16 HPI_DataCreate(HPI_DATA * pData,	///<[inout] Structure to be initialised
		   HPI_FORMAT * pFormat,	///<[in] format of the data
		   u8 * pbData,	///<[in] pointer to data buffer
		   u32 dwDataSize	///<[in] amount of data in buffer
    )
{
	HPI_MSG_DATA *pMD = (HPI_MSG_DATA *) pData;

	HPI_FormatToMsg(&pMD->Format, pFormat);

	pMD->pbData = pbData;
	pMD->dwDataSize = dwDataSize;
	return (0);
}
#endif

/// The actual message size for each object type
static u16 aMsgSize[HPI_OBJ_MAXINDEX + 1] = HPI_MESSAGE_SIZE_BY_OBJECT;
/// The actual response size for each object type
static u16 aResSize[HPI_OBJ_MAXINDEX + 1] = HPI_RESPONSE_SIZE_BY_OBJECT;

/// initialize the HPI message structure
void HPI_InitMessage(HPI_MESSAGE * phm, u16 wObject, u16 wFunction)
{
	memset(phm, 0, sizeof(HPI_MESSAGE));
	if ((wObject > 0) && (wObject <= HPI_OBJ_MAXINDEX)) {
		phm->wType = HPI_TYPE_MESSAGE;
		phm->wSize = aMsgSize[wObject];
		phm->wObject = wObject;
		phm->wFunction = wFunction;
		phm->wDspIndex = 0;
// Expect adapter index to be set by caller
	} else {
		phm->wType = 0;
		phm->wSize = 0;
	}
}

/// initialize the HPI response structure
void HPI_InitResponse(HPI_RESPONSE * phr, u16 wObject, u16 wFunction,
		      u16 wError)
{
	memset(phr, 0, sizeof(HPI_RESPONSE));
	phr->wType = HPI_TYPE_RESPONSE;
	phr->wSize = aResSize[wObject];
	phr->wObject = wObject;
	phr->wFunction = wFunction;
	phr->wError = wError;
	phr->wSpecificError = 0;
}

	  /** @} */// group utility
////////////////////////////////////////////////////////////////////////
// local functions

/** handle struct for internal hpifunc use.  Total bit count must be <= 32 */
struct sHANDLE {
	unsigned int objIndex:12;	///< Up to 4096 objects
	unsigned int objType:4;	///< HPI_OBJ_*
	unsigned int adapterIndex:14;	///< up to 16K
	unsigned int dspIndex:1;	///< for asi62xx
	unsigned int readOnly:1;	///< future readonly flag
};

/** allow conversion from handle to u32 */
typedef union {
	struct sHANDLE h;
	u32 w;
} tHANDLE;

/**
Encode 3 indices and an object type into a 32 bit handle.

\return Object handle
\sa HPI_HandleToIndexes3()
*/
HPI_HANDLE HPI_IndexesToHandle3(const char cObject,	///< HPI_OBJ_* - the type code of object
				const u16 wAdapterIndex,	///< The Adapter index
				const u16 wObjectIndex,	///< The stream or control index, if used
				const u16 wDspIndex	///< The index of the DSP which implements the object, usually 0
    )
{
	tHANDLE handle;

	handle.h.adapterIndex = wAdapterIndex;
	handle.h.dspIndex = wDspIndex;
	handle.h.readOnly = 0;
	handle.h.objType = cObject;
	handle.h.objIndex = wObjectIndex;
	return handle.w;
}

HPI_HANDLE HPI_IndexesToHandle(const char cObject,	///< HPI_OBJ_* - the type code of object
			       const u16 wAdapterIndex,	///< The Adapter index
			       const u16 wObjectIndex)	///< The stream or control index, if used
{
	return HPI_IndexesToHandle3(cObject, wAdapterIndex, wObjectIndex, 0);
}

/**
Extract up to 3 indices from an object handle, if non-NULL pointers are supplied

/sa HPI_IndexesToHandle3(), HPI_IndexesToHandle()
*/

void HPI_HandleToIndexes3(const HPI_HANDLE dwHandle,	///< The handle to decode
			  u16 * pwAdapterIndex,	///< Where to store the Adapter index
			  u16 * pwObjectIndex,	///< Where to store the stream or control index, if used
			  u16 * pwDspIndex	///< Where to store the index of the DSP which implements the object
    )
{
	tHANDLE handle;
	handle.w = dwHandle;

	if (pwDspIndex)
		*pwDspIndex = (u16) handle.h.dspIndex;
	if (pwAdapterIndex)
		*pwAdapterIndex = (u16) handle.h.adapterIndex;
	if (pwObjectIndex)
		*pwObjectIndex = (u16) handle.h.objIndex;
}

char HPI_HandleObject(const HPI_HANDLE dwHandle)
{
	tHANDLE handle;
	handle.w = dwHandle;
	return (char)handle.h.objType;
}

///////////////////////////////////////////////////////////////////////////
