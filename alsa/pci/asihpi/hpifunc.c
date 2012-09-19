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

#include "hpi.h"
#include "hpicheck.h"

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

/* Note: These assignments must be in this order to avoid corrupting fields */
void HPI_StreamResponseToLegacy(HPI_STREAM_RES * pSR)
{
	pSR->u.legacy_stream_info.dwAuxiliaryDataAvailable =
	    pSR->u.stream_info.dwAuxiliaryDataAvailable;
	pSR->u.legacy_stream_info.wState = pSR->u.stream_info.wState;
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
/**\defgroup subsys Subsystem
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

	{
		HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_OPEN);
		HPI_Message(&hm, &hr);

		if (hr.wError == 0)
			return (&ghSubSys);

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

}

/** HPI subsystem get version.
* Returns the HPI subsystem major and minor versions that were embedded into the HPI module at compile time. On a
* Windows machine this version is embedded in the kernel driver .sys file.
*
* \param phSubSys Pointer to HPI subsystem handle.
* \param pdwVersion 32 bit word containing version of HPI. Upper 24bits is major
* version number and lower 8 bits is minor version number, i.e., 0x00000304 is
* version 3.04.
* \return_hpierr
*/
u16 HPI_SubSysGetVersion(HPI_HSUBSYS * phSubSys, u32 * pdwVersion)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

	HPI_UNUSED(phSubSys);

	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_GET_VERSION);
	HPI_Message(&hm, &hr);
	*pdwVersion = hr.u.s.dwVersion;
	return (hr.wError);
}

/** Extended HPI_SubSysGetVersion() that returns Major, Minor and Build versions.
* Returns extended HPI subsystem version that was embedded into the HPI module at compile time. On a
* Windows machine this version is embedded in the kernel driver .sys file.
*
* \param phSubSys Pointer to HPI subsystem handle.
* \param pdwVersionEx 32 bit word containing version of HPI.\n
*    B23..16 = Major version\n
*    B15..8 = Minor version\n
*    B7..0 = Build version\n
*    i.e. 0x00030402 is version 3.04.02
*
* \return_hpierr
*/
u16 HPI_SubSysGetVersionEx(HPI_HSUBSYS * phSubSys, u32 * pdwVersionEx)
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
* \return_hpierr
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

/* Internal functions follow. */
/** \internal
*/

/** \internal
* Used by a PnP OS to create an adapter.
* \param phSubSys Pointer to HPI subsystem handle.
* \param pResource Pointer to the resources used by this adapter.
* \param pwAdapterIndex Returned index of the adapter that was just created.
* \return_hpierr
*/
u16 HPI_SubSysCreateAdapter(HPI_HSUBSYS * phSubSys,
			    HPI_RESOURCE * pResource, u16 * pwAdapterIndex)
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

/** \internal
* Used by a PnP OS to delete an adapter.
* \param phSubSys Pointer to HPI subsystem handle.
* \param wAdapterIndex Index of the adapter to delete.
* \return_hpierr
*/
u16 HPI_SubSysDeleteAdapter(HPI_HSUBSYS * phSubSys, u16 wAdapterIndex)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_DELETE_ADAPTER);
	hm.wAdapterIndex = wAdapterIndex;
	HPI_Message(&hm, &hr);
	return (hr.wError);
}

/** Return the total number of adapters including networked adapters.
* \return_hpierr
*/
u16 HPI_SubSysGetNumAdapters(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			     int *pnNumAdapters	///< total number of adapters including local and networked.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_GET_NUM_ADAPTERS);
	HPI_Message(&hm, &hr);
	*pnNumAdapters = (int)hr.u.s.wNumAdapters;
	return (hr.wError);
}

/** Extended version of HPI_SubSysFindAdapters() that iterates through all adapters present, returning adapter index and type for each one.
* \return_hpierr
*/
u16 HPI_SubSysGetAdapter(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			 int nIterator,	///< Adapter iterator {0 .. total number of adapters - 1}.
			 u32 * pdwAdapterIndex,	///< Index of adapter.
			 u16 * pwAdapterType	///< Type of adapter.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_GET_ADAPTER);
	hm.wAdapterIndex = (u16) nIterator;
	HPI_Message(&hm, &hr);
	*pdwAdapterIndex = (int)hr.u.s.wAdapterIndex;
	*pwAdapterType = hr.u.s.awAdapterList[0];
	return (hr.wError);
}

///\}
///////////////////////////////////////////////////////////////////////////
/** \defgroup adapter Adapter

@{
*/

/** Opens an adapter for use.
* The adapter is specified by wAdapterIndex which corresponds to the adapter
* index on the adapter hardware (typically set using jumpers or switch).
* \param phSubSys Pointer to HPI subsystem handle.
* \param wAdapterIndex Index of the adapter to be opened. For PCI adapters this
* ranges from 0-15. Network adapter indicies start at 1000.
*
* \return_hpierr
*/
u16 HPI_AdapterOpen(HPI_HSUBSYS * phSubSys, u16 wAdapterIndex)
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
* \param phSubSys Pointer to HPI subsystem handle.
* \param wAdapterIndex Index of the adapter to be opened. For PCI adapters this
* ranges from 0-15. Network adapter indicies start at 1000.
* \return_hpierr
*/
u16 HPI_AdapterClose(HPI_HSUBSYS * phSubSys, u16 wAdapterIndex)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_CLOSE);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/**     \internal
* Given an object type HPI_OBJ_*, index, and adapter index,
*     determine if the object exists, and if so the dsp index of the object.
* The DSP index defines which DSP on the adapter has the specified object.
*     Implementation is non-trivial only for multi-DSP adapters.
* \param phSubSys Pointer to HPI subsystem handle.
* \param wAdapterIndex Index of adapter to search.
* \param wObjectType Type of object HPI_OBJ_*.
* \param wObjectIndex Index of object
* \param pDspIndex Return the index of the DSP containing the object.
*
* \return_hpierr
*/
u16 HPI_AdapterFindObject(const HPI_HSUBSYS * phSubSys,
			  u16 wAdapterIndex,
			  u16 wObjectType, u16 wObjectIndex, u16 * pDspIndex)
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
* \return_hpierr
*  \retval HPI_ERROR_BAD_ADAPTER_MODE if an unsupported mode is set
*/

#if 0
Modes are already defined and documented in HPI.H,
    so this section is redundant ? ? ? See dwAdapterMode parameter
    documentation. * Currently defined modes are : <table border =
    1 cellspacing =
    0 > <tr > <td >< b > Mode < /b >< /td > <td >< b > Description < /b >< /td >
    </tr > <tr > <td >< small > HPI_ADAPTER_MODE_4OSTREAM < /small >< /td >
    <td >< small > ASI4401,
    4 stereo / mono ostreams->1 stereo line out. < br > ASI4215,
    4 ostreams->4 line outs. < br > ASI6114,
    4 ostreams->4 line outs. < /small >< /td > </tr > <tr >< td >< small >
    HPI_ADAPTER_MODE_6OSTREAM < /small >< /td > <td >< small > ASI4401,
    6 mono ostreams->2 mono line outs. < /small >< /td >< /tr >
    <tr >< td >< small > HPI_ADAPTER_MODE_8OSTREAM < /small >< /td >
    <td >< small > ASI6114, 8 ostreams->4 line outs. < br > ASI6118,
    8 ostreams->8 line outs. < /small >< /td >< /tr > <tr >< td >< small >
    HPI_ADAPTER_MODE_9OSTREAM < /small >< /td > <td >< small > ASI6044,
    9 ostreams->4 lineouts. < /small >< /td >< /tr > <tr >< td >< small >
    HPI_ADAPTER_MODE_12OSTREAM < /small >< /td > <td >< small > ASI504X,
    12 ostreams->4 lineouts. < /small >< /td >< /tr > <tr >< td >< small >
    HPI_ADAPTER_MODE_16OSTREAM < /small >< /td > <td >< small > ASI6118,
    16 ostreams->8 line outs. < /small >< /td >< /tr > <tr >< td >< small >
    HPI_ADAPTER_MULTICHANNEL < /small >< /td > <td >< small > ASI504X,
    2 ostreams->4 line outs(1 to 8 channel streams),
    4 lineins->1 instream(1 to 8 channel streams) at 0 -
    48 kHz.For more info see the SSX Specification. < /small >< /td >< /tr >
    <tr >< td >< small > HPI_ADAPTER_MODE1 < /small >< /td > <td >< small >
    ASI504X, 12 ostream,
    4 instream 0 to 48 kHz sample rates(see ASI504X datasheet for more info)
	. < /small >< /td >< /tr > <tr >< td >< small > HPI_ADAPTER_MODE2 <
	    /small >< /td > <td >< small > ASI504X, 4 ostream,
	    4 instream 0 to 192 kHz sample rates(see ASI504X datasheet for more
						 info)
		. < /small >< /td >< /tr > <tr >< td >< small >
		    HPI_ADAPTER_MODE3 < /small >< /td > <td >< small > ASI504X,
		    4 ostream,
		    4 instream 0 to 192 kHz sample rates(see ASI504X datasheet
							 for more info)
			. < /small >< /td >< /tr > </table >
#endif
			    u16 HPI_AdapterSetMode(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
						   u16 wAdapterIndex,	///< Index of adapter to set mode on.
						   u32 dwAdapterMode	///< One of the \ref adapter_modes
			    )
{
return HPI_AdapterSetModeEx(phSubSys, wAdapterIndex, dwAdapterMode,
			    HPI_ADAPTER_MODE_SET);
}

/** Adapter set mode extended. This updated version of HPI_AdapterSetMode() allows
* querying supported modes.<br>
* When wQueryOrSet=HPI_ADAPTER_MODE_QUERY doesn't set the mode, but the return value reflects
* whether the mode is valid for the adapter.<br>
*  When wQueryOrSet=HPI_ADAPTER_MODE_SET, the mode of the adapter is changed if valid.<br>
* The adapter must be restarted for the mode to take affect. Under Windows this means that
* the computer must be rebooted.
* \return \return_hpierr
* \retval 0 the adapter supports the given mode - no error
* \retval HPI_ERROR_BAD_ADAPTER_MODE if an unsupported mode is set or queried
* \retval HPI_ERROR_XXX code if an error occurs.
*/
u16 HPI_AdapterSetModeEx(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			 u16 wAdapterIndex,	///< Index of adapter to set mode on.
			 u32 dwAdapterMode,	///< One of the \ref adapter_modes
			 u16 wQueryOrSet	///< Controls whether the mode is being set or queried.
    )
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

/** Read the current adapter mode setting.
* \return_hpierr
*/
u16 HPI_AdapterGetMode(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
		       u16 wAdapterIndex,	///< Index of adapter to get mode from.
		       u32 * pdwAdapterMode	///< The returned adapter mode. Will be one of - \ref adapter_modes.
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
* \param phSubSys Pointer to HPI subsystem handle.
* \param wAdapterIndex Index of adapter to read adapter information from.
* \param pwNumOutStreams Number of output streams (play) on the adapter.
* \param pwNumInStreams Number of input streams (record) on the adapter.
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
* \param pdwSerialNumber Adapter serial number.  Starts at 1 and goes up.
* \param pwAdapterType Adapter ID code, defined in HPI.H.  Examples are HPI_ADAPTER_ASI6114 (0x6114).
* \return_hpierr
*/
u16 HPI_AdapterGetInfo(HPI_HSUBSYS * phSubSys,
		       u16 wAdapterIndex,
		       u16 * pwNumOutStreams,
		       u16 * pwNumInStreams,
		       u16 * pwVersion,
		       u32 * pdwSerialNumber, u16 * pwAdapterType)
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
* \return_hpierr
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

/** \internal Extended Get Assert
* The extened assert interface adds 32 bit 'line number' and dsp
* index to standard assert API.
* \todo Review whether this is actually implemented anywhere ?
* \param HPI_HSUBSYS *phSubSys, HPI subsystem handle.
* \param wAdapterIndex,    Adapter to query.
* \param *wAssertPresent, The number of asserts waiting including this one.
* \param *pszAssert,      Assert message, traditionally file name.
* \param *pdwLineNumber,  Assert number, traditionally line number in file.
* \param *pwAssertOnDsp   The index of the DSP that generated the assert.
* \return_hpierr
*/
u16 HPI_AdapterGetAssertEx(HPI_HSUBSYS * phSubSys,
			   u16 wAdapterIndex,
			   u16 * wAssertPresent,
			   char *pszAssert,
			   u32 * pdwLineNumber, u16 * pwAssertOnDsp)
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
* \return_hpierr
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

/** \internal Turn on a particular adapter capability.
*
* \return_hpierr
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

/** \internal Unimplemented */
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

/** Set an adapter property to a value.
* \return_hpierr
*/
u16 HPI_AdapterSetProperty(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			   u16 wAdapterIndex,	///< Adapter index.
			   u16 wProperty,	///< Which of the \ref adapter_properties to set
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

/** Gets the value of an adapter property.
* \return_hpierr
*/
u16 HPI_AdapterGetProperty(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			   u16 wAdapterIndex,	///< Adapter index.
			   u16 wProperty,	///< One of \ref adapter_properties to get.
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

/** \internal Enumerates adapter properties. To be implemented sometime in the future.
* This function allows an application to determine what property a particular
* adapter supports. Furthermore
* the settings for a particular propery can also be established.
* \param      *phSubSys HPI subsystem handle.
* \param      wAdapterIndex Adapter index.
* \param      wIndex Adapter property # to return.
* \param      wWhatToEnumerate Either HPI_ADAPTER_PROPERTY_ENUMERATE_PROPERTIES
*         or HPI_ADAPTER_PROPERTY_ENUMERATE_SETTINGS
* \param      wPropertyIndex Property index.
* \param      *pdwSetting  Returned adapter property, or property setting,
*         depending on wWhatToEnumerate.
* \return_hpierr
*/
u16 HPI_AdapterEnumerateProperty(HPI_HSUBSYS * phSubSys,
				 u16 wAdapterIndex,
				 u16 wIndex,
				 u16 wWhatToEnumerate,
				 u16 wPropertyIndex, u32 * pdwSetting)
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
* \return_hpierr
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
	*dwRecommendedBufferSize = roundup_pow_of_two(((dwSize + 4095L) & ~4095L));	// round up to nearest 4 K bounday, then round up to power of 2
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

The dwDataToPlay count returned from HPI_OutStreamGetInfoEx() measures the amount of encoded data (MPEG, PCM, etc.) in the stream's
buffer.  When there is less than a whole frame of encoded data left, it cannot be decoded
for mixing and output. It is possible that HPI_OutStreamGetInfoEx() indicates that there is still a small amount
of data to play, but the stream enters the DRAINED state, and the amount of data to play
never gets to zero.

The size of a frame varies depending on the audio format.
Compressed formats such as MPEG require whole frames of data in order to decode audio.  The
size of the input frame depends on the samplerate and bitrate
(e.g. MPEG frame_bytes = bitrate/8 * 1152/samplerate).

AudioScience's implementation of PCM decoding also requires a minimum amount of input data.
ASI4xxx adapters require 4608 bytes, whereas ASI6xxx adapters require 1536 bytes for stereo,
half this for mono.

Conservative conditions to detect end of play:<br>
- Have done the final OutStreamWrite<br>
- Stream state is HPI_STATE_DRAINED<br>
- dwDataToPlay < 4608

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
* \return_hpierr
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
* \return_hpierr
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

/** Get information about attributes and state of output stream.
* This is similar to HPI_OutStreamGetInfo() but returns extended information
* including the size of the streams buffer in pdwBufferSize.
* It also returns whether the stream is currently playing (the state) and the amount
* of audio data left to play.
*
* \return_hpierr
*/
u16 HPI_OutStreamGetInfoEx(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			   HPI_HOSTREAM hOutStream,	///< Handle to opened OutStream.
			   u16 * pwState,	///< State of stream = HPI_STATE_STOPPED,HPI_STATE_PLAYING or HPI_STATE_DRAINED.
			   u32 * pdwBufferSize,	///< Size (in bytes) of stream data buffer.
			   u32 * pdwDataToPlay,	///< Amount of data (in bytes) left in the buffer to play.
			   u32 * pdwSamplesPlayed,
				  /**< The SamplesPlayed parameter returns the number of samples
played since the last HPI_OutStreamReset command was issued.  It reflects
the number of stereo samples for a stereo stream and the number of mono
samples for a mono stream. This means that if a 44.1kHz stereo and mono
stream were both playing they would both return SamplesPlayed=44100 after 1 second.
*/
			   u32 * pdwAuxiliaryDataToPlay
				       /**< AuxiliaryDataToPlay is only relevant when BBM is active
(see HPI_OutStreamHostBufferAllocate).  In this case DataToPlay refers to
the host side buffer state while AuxiliaryDataToPlay refers to the data
remaining in the card's own buffers.
*/
    )
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
* dwBytesToWrite bytes are copied from  *pbData array to the output stream
* hardware.  On return the memory used to hold that data may be reused.
*
* A different format will only be accepted in the first write
* after the stream is opened or reset.
*
* The size of the data block that may be written is limited to half the size
* of the streams internal data buffer (specified by dwBufferSize returned by
* HPI_OutStreamGetInfo()).
*
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

wHE = HPI_OutStreamWriteBuf( phSubSys, hOutStream, &aData, dwBytes, &hpiFormat);
\endcode
* \return_hpierr
* \retval 0 The data was written to the stream
* \retval HPI_INVALID_DATASIZE tried to write more data than buffer space available - no data was written
*/
u16 HPI_OutStreamWriteBuf(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			  HPI_HOSTREAM hOutStream,	///< Handle to opened OutStream.
			  u8 * pbData,	///< Pointer to buffer containing data to be written to the playback buffer.
			  u32 dwBytesToWrite,	///< Number of bytes to write, must be <= space available.
			  HPI_FORMAT * pFormat	///< Format of the data (compression,channels,samplerate)
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

/** Starts an output stream playing audio data.
* Data is taken from the circular buffer on the adapter hardware that has already been
* partially filled by HPI_OutStreamWrite commands.  Audio is played from the current
* position in the buffer (which may be reset using HPI_OutStreamReset()).
* \return_hpierr
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
* \return_hpierr
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

/** \internal
* Generate a sinewave output on the specified stream.
* \note This function is unimplemented.
*/
u16 HPI_OutStreamSinegen(HPI_HSUBSYS * phSubSys, HPI_HOSTREAM hOutStream)
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
* \return_hpierr
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
* \return_hpierr
* \retval HPI_ERROR_INVALID_FORMAT if the format is not supported.
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
* nVelocity is set by
\code
nVelocity = (u16)(fVelocity * HPI_VELOCITY_UNITS);
\endcode
* where fVelocity is a floating point number in the range of -4.0 to +4.0.
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
* Forward scrubbing does not have any limitations whatsoever, apart from the maximum speed,
* as specified by HPI_OutStreamSetVelocity().
*
* Reverse scrubbing operates under the following constraints:<br>
* 1) The user may not scrub on audio prior to the HPI_OutStreamStart( ) data point.<br>
* 2) The user may not reverse scrub further than -10 seconds from the forward most scrub position.<br>
* If either of these constraints is broken, the stream state will return HPI_STATE_DRAINED
* and audio playback will cease. The user can then forward scrub again after this error
* condition.
* \return_hpierr
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

/** \internal
* Set punch in and out points in a buffer.
* \note Currently unimplemented.
*/
u16 HPI_OutStreamSetPunchInOut(HPI_HSUBSYS * phSubSys,
			       HPI_HOSTREAM hOutStream,
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
* or HPI_OutStreamReset() and before the first HPI_OutStreamWrite() call.
*
* \param wMode The mode for the ancillary data extraction to operate in. Valid settings
* are HPI_MPEG_ANC_RAW and HPI_MPEG_ANC_HASENERGY. The "RAW" mode indicates that the
* entire ancillary data field is taken up by data from the Anc data buffer. The "HASENERGY"
* option tells the decoder that the MPEG frames have energy information stored in them
* (5 bytes per stereo frame, 3 per mono).
*
* \return_hpierr
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
* \return_hpierr
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
* order:
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
* \return_hpierr
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

/** Sets the playback timescale with pitch and content preservation.
* Range is 0.8-1.2 (
to 120%) of original time.<br>
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
* \return_hpierr
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
* Once the buffer is allocated, OutStream data will be transferred from it in
* the background (rather than the application having to wait for the transfer).
*
* This function is provided so that the application can match the size of its
* transfers to the size of the buffer.
*
* After a call to HPI_OutStreamHostBufferAllocate(), HPI_OutStreamGetInfoEx()
* returns the size and data available in host buffer rather than the buffers
* on the adapter. However, while there is space in the adapter buffers, data
* will be quickly transferred to the adapter, providing additional buffering
* against delays in sending more audio data.
*
* \note There is a minimum buffer size that will work with a given audio
* format and polling rate. An appropriate size for the buffer can be calculated
* using HPI_StreamEstimateBufferSize().
*
* If an error occurs or the adapter doesn't support host buffering then no
* buffer is allocated. Stream transfers still take place using foreground
* transfers (all drivers pre 2004). Performance will be relatively worse.
*
* \return_hpierr
* \retval HPI_ERROR_INVALID_DATASIZE memory can't be allocated
*(retrying the call with a smaller size may succeed)
* \retval HPI_ERROR_INVALID_FUNC the adapter doesn't support busmastering
*/
u16 HPI_OutStreamHostBufferAllocate(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
				    HPI_HOSTREAM hOutStream,	///< Handle to OutStream.
				    u32 dwSizeInBytes	///< Size in bytes of bus mastering buffer to allocate.
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
* \return_hpierr
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
* streams at once, enabling multi-channel recording and playback.
*
* When using the "Group" functions all streams that are to be grouped
* together should be opened. One of the streams should be selected to
* be the master stream and the other streams should be added to it's group.
* Both in streams and out streams can be added to the same group.
* Once a group has been formed, HPI_OutStreamStart() called on the master
* will cause all streams to start at once.
*
* \note This function is only supported on on ASI6000 and ASI5000 adapters.
* \return_hpierr
*/
u16 HPI_OutStreamGroupAdd(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			  HPI_HOSTREAM hOutStreamHandle,	///< Handle to OutStream.
			  HPI_HSTREAM hStreamHandle	///< Handle to either an InStream or an OutStream.
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
* \note This function is only supported on on ASI6000 and ASI5000 adapters.
* \return_hpierr
*/
u16 HPI_OutStreamGroupGetMap(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			     HPI_HOSTREAM hOutStreamHandle,	///< Handle to OutStream.
			     u32 * pdwOutStreamMap,
				   /**< Bitmapped representation of OutStreams grouped with this output
stream. b0 represents OutStream 0, b1 OutStream 1, b2 OutStream 2 etc.
*/
			     u32 * pdwInStreamMap
				   /**< Bitmapped representation of InStreams grouped with this output stream.
b0 represents InStream 0, b1 InStream 1, b2 InStream 2 etc.
*/
    )
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
* \return_hpierr
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

The following figure describes the states an InStream uses.

\image html instream_states.png
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
* \return_hpierr
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

/** Closes an input stream. Deallocates allocated host buffer.
*
* \return_hpierr
*/
u16 HPI_InStreamClose(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
		      HPI_HISTREAM hInStream	///< Handle to the InStream to close.
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

/** Queries an input stream to see whether it supports a certain audio format, described in pFormat.
*
* \return_hpierr
* \retval \ref HPI_ERROR_INVALID_FORMAT if not supported.
*/

u16 HPI_InStreamQueryFormat(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			    HPI_HISTREAM hInStream,	///< InStream handle.
			    HPI_FORMAT * pFormat	///< Pointer to an HPI_FORMAT structure containing info about the audio format to query.
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

/** Sets the recording format for an input stream.
*
* The format to set is described by pFormat.
*
* \return_hpierr
* \retval \ref HPI_ERROR_INVALID_FORMAT if not supported.
*
* For example, to set an InStream to stereo, MPEG Layer II @ 256kbps, 44.1kHz sample rate:
\code
wHE = HPI_FormatCreate(
&hpiFormat,
2,                 // stereo channel
HPI_FORMAT_MPEG_L2,// compression format
44100,             // sample rate
256000,            // bits/sec (only used for MPEG)
0                  // currently no attributes
);

wHE = HPI_InStreamSetFormat(
phSubSys,
hInStream,
&hpiFormat
);
\endcode
\n
<b>MP3 Variable Bitrate</b>\n\n
On adapters that support MP3 encoding,  a quality factor between 0 and 100 controls variable
bitrate encoding. The quality factor replaces the bitrate in the dwBitrate parameter of
HPI_FormatCreate(). Setting the "bitrate" to 100 or less automatically activates variable bitrate
encoding.
\code
wHE = HPI_FormatCreate(
&hpiFormat,
2,                     // stereo channel
HPI_FORMAT_MPEG_L2,   // compression format
44100,                // sample rate
75,                   // VBR quality setting
0                     // currently no attributes
);

\endcode
\n
<b>InStream interaction with Bitstream</b>\n\n
Where an instream HPI_DESTNODE_ISTREAM is connected to a bitstream input HPI_SOURCENODE_RAW_BITSTREAM,
the bitstream input provides an unformatted stream of data bits. How this data is treated is affected by
the format chosen when HPI_InStreamSetFormat() is called.  There are two valid choices:\n
\n
(1) \ref HPI_FORMAT_RAW_BITSTREAM\n
The raw bits are copied into the stream without synchronization. The value returned by
HPI_InStreamGetInfoEx() into *pdwSamplesRecorded is the number of 32 bit words of data recorded.\n
\n
(2) \ref HPI_FORMAT_MPEG_L2
After the InStream has been reset the incoming bitstream is scanned for the MPEG2 Layer 2 sync pattern
(0xFFFE or 0xFFFC), and input (recording) of the bitstream data is inhibited until this pattern is found.
The first word of recorded or monitored data will be this sync pattern, and following data will be
word-aligned to it.\n
\n
This synchronization process only takes place once after stream reset.  There is a small chance that
the sync pattern appears elsewhere in the data and the mpeg sync in recorded data won't be byte aligned.
\n\n
The value returned by HPI_InStreamGetInfoEx() into *pdwSamplesRecorded is calculated from the number of
bits recorded using the  values for dwBitRate and dwSampleRate set by HPI_InStreamSetFormat().\n
\code
samples = bits recorded * dwSampleRate / dwBitRate
\endcode
If the samplerate is 44.1kHz, then the samplecount will not be exact.
*/
u16 HPI_InStreamSetFormat(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			  HPI_HISTREAM hInStream,	///< InStream handle.
			  HPI_FORMAT * pFormat	///< Pointer to a format structure.
    )
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
* Reads dwBytesToRead bytes of audio data from the specified InStream into a
* memory buffer pointed to by pbData
*
* The amount of data requested may be any size up to the amount
* of data available in the hardware buffer specified by dwDataAvailable
* returned by HPI_InStreamGetInfoEx().
*
* \image html instream_fifo.png
* \par Diagram
*
* \return_hpierr
* \retval HPI_ERROR_INVALID_DATASIZE if trying to read more data than available.
*
*/
u16 HPI_InStreamReadBuf(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			HPI_HISTREAM hInStream,	///< InStream handle.
			u8 * pbData,	///< Pointer to buffer for read data.
			u32 dwBytesToRead	///< Number of bytes to read, must be <= number available.
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

/** Starts an input stream recording audio data.
* Audio data is written into the adapters hardware buffer using the currently selected audio format
*(channels, sample rate, compression format etc.).  Data is then read from the buffer using HPI_InStreamRead().
*
* \return_hpierr
*/

u16 HPI_InStreamStart(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
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
* Stops an input stream recording audio data.  The audio data buffers is not cleared,
* so a subsequent InStreamStart will resume recording at the
* position in the buffer where the record had been stopped.
*
* \return_hpierr
*/

u16 HPI_InStreamStop(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
		     HPI_HISTREAM hInStream	///< InStream handle.
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

/** Clears the audio data buffer of an input stream.
* If the stream was recording at the time, it will be stopped.
*
* \return_hpierr
*/

u16 HPI_InStreamReset(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
		      HPI_HISTREAM hInStream	///< InStream handle.
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

/** Returns extended information about the input stream.
*
* This includes the size of the streams hardware buffer returned in pdwBufferSize.  Also includes whether the
* stream is currently recording (the state), the amount of audio data currently contained in the buffer
* and how many samples have been recorded.
*
*\param phSubSys HPI subsystem handle.
*\param hInStream InStream handle.
*\param pwState State of stream = \ref HPI_STATE_STOPPED or \ref HPI_STATE_RECORDING.
*\param pdwBufferSize Sixe (in bytes) of stream data buffer.
*\param pdwSamplesRecorded The SamplesRecorded parameter returns the number of samples recorded since the
*last HPI_InStreamReset() command was issued.  It reflects the number of stereo samples for a stereo stream and the
*number of mono samples for a mono stream.  This means that if a 44.1kHz stereo and mono stream were both recording
*they would both return SamplesRecorded=44100 after 1 second.
*\param pdwDataRecorded DataRecorded returns the amount of data available to be read back in the next
* HPI_InStreamRead() call. When BBM is active this is the data in the host buffer, otherwise it is the amount in the on-card buffer.
*\param pdwAuxiliaryDataRecorded AuxiliaryDataRecorded is only valid when BBM is being used (see HPI_InStreamHostBufferAllocate()).
*In BBM mode it returns the amount of data left in the on-card buffers.  This can be used to determine if a record overrun has occurred
*(both BBM and card buffers full), or at the end of recording to ensure that all recorded data has been read.
*
* \return_hpierr
*/

u16 HPI_InStreamGetInfoEx(HPI_HSUBSYS * phSubSys,
			  HPI_HISTREAM hInStream,
			  u16 * pwState,
			  u32 * pdwBufferSize,
			  u32 * pdwDataRecorded,
			  u32 * pdwSamplesRecorded,
			  u32 * pdwAuxiliaryDataRecorded)
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

/** Initializes the MPEG Layer II / III Ancillary data channel
*
* Initializes the MPEG Layer II / III Ancillary data channel to support the embedding of wBytesPerFrame bytes into the MPEG bitstream.
*
*
* \param phSubSys HPI subsystem handle.
* \param hInStream Handle for InStream.
* \param wBytesPerFrame This variable specifies the rate at which data is inserted into the Ancillary data channel.
* Note that when (wMode== HPI_MPEG_ANC_HASENERGY, see below) an additional 5 bytes per frame are
* automatically allocated for energy information.
* \param wMode The mode for the ancillary data extraction to operate in. Valid settings are HPI_MPEG_ANC_RAW and
* HPI_MPEG_ANC_HASENERGY. The RAW mode indicates that the entire ancillary data field is taken up by data from the
* Anc data buffer. The HASENERGY option tells the encoder that the MPEG frames have energy information stored in them
* (5 bytes per stereo frame, 3 per mono). The encoder will insert the energy bytes before filling the remainder of the
* ancillary data space with data from the ancillary data buffer.
* \param wAlignment HPI_MPEG_ANC_ALIGN_LEFT  the wBytesPerFrame data immediately follow the audio data.
* Spare space is left at the end of the frame.
* HPI_MPEG_ANC_ALIGN_RIGHT  the wBytesPerFrame data is packed against the end of the frame. Spare space is left at the start of the frame.
* HPI_MPEG_ANC_ALIGN_FILL  all ancillary data space in the frame is used. wBytesPerFrame or more data is written per frame.
* There is no spare space.
* This parameter is ignored for MP3 encoding, effectively it is fixed at HPI_MPEG_ANC_ALIGN_FILL  (See Note 2)
* \param wIdleBit This field tells the encoder what to set all the bits of the ancillary data field to in the case where there is no data
* waiting to be inserted.
* Valid values are 0 or 1.  This parameter is ignored for MP3 encoding, if no data is available, no data will be inserted  (See Note 2)
*
* \return_hpierr
*
* \image html instream_anc_reset.png
*
* See the below table for the relationship between wBytesPerFrame and the datarate on the ancillary data channel.
* For stereo recording, ancillary data is organized as follows:
<table border=1 cellspacing=0 cellpadding=5>
<tr>
<td><p><b>Sample rate</b></p></td>
<td><p><b>Frame rate</b></p></td>
<td><p><b>Energy rate</b></p></td>
<td><p><b>Bitrate per byte on ancillary data</b></p></td>
</tr>
<tr>
<td>48 kHz</td>
<td>41.66 frames/sec</td>
<td>1333.33 bits/sec</td>
<td>1333.33 bits/sec</td>
</tr>
<tr>
<td>44.1 kHz</td>
<td>38.28 frames/sec</td>
<td>1224.96 bits/sec</td>
<td>306.25 bits/sec</td>
</tr>
<tr>
<td>32 kHz</td>
<td>27.77 frames/sec</td>
<td>888.89 bits/sec</td>
<td>222.22 bits/sec</td>
</tr>
</table>

Ancillary data is embedded at the end of each MPEG frame as follows:\n
\verbatim
Key:
e = ancillary energy bits (correct number of bits not shown)
d = ancillary data bits
a = audio bits
f = fill bits
x = don't care bits\n
HPI_MPEG_ANC_ALIGN_LEFT (fill is at end for "raw" case)
wMode = HPI_MPEG_ANC_RAW
a,a,a,d,d,d,d,d,d,�,f,f,f <eof>
wMode = HPI_MPEG_ANC_HASENERGY
a,a,a,d,d,d,d,..,f,f,f,e,e,e,e <eof>

HPI_MPEG_ANC_ALIGN_RIGHT (fill is at front)
wMode = HPI_MPEG_ANC_RAW
a,a,a,f,f,f,d,d,d,d,d,d<eof>
wMode = HPI_MPEG_ANC_HASENERGY
a,a,a,f,f,f,d,d,d,d,..,e,e,e,e <eof>

HPI_MPEG_ANC_ALIGN_FILL (all available ancillary data spots are used)
wMode = HPI_MPEG_ANC_RAW
a,a,a,d,d,d,d,d,d,d,d,d<eof>
wMode = HPI_MPEG_ANC_HASENERGY
a,a,a,d,d,d,d,d,d,d,..,e,e,e,e <eof>
\endverbatim

* <b>Note1 (Calling order):</b>\n
* This call must be made before an HPI_InStreamSetFormat() call.\n\n
* <b>Note2 (MP3):</b>\n
* Embedded energy information in an MPEG1 layer III (MP3) stream is quite difficult to
* utilize because its position is not constant within the MPEG frame.
* With MP2, the ancillary data field always appears at the end of the MPEG frame.
* The parameters wIdleBit and wAlignment are ignored for MP3 due to the inherently more
* efficient data packing scheme used.\n\n
* <b>Note3 (Default settings):</b>\n
* A side effect of HPI_InStreamOpen() is that the following default ancillary data settings
* are made:\n
* Ancillary Bytes Per Frame = 0 \n
* Ancillary Mode = HPI_MPEG_ANC_HASENERGY \n
* Ancillary Alignment = HPI_MPEG_ANC_ALIGN_RIGHT \n
*/
u16 HPI_InStreamAncillaryReset(HPI_HSUBSYS * phSubSys,
			       HPI_HISTREAM hInStream,
			       u16 wBytesPerFrame,
			       u16 wMode, u16 wAlignment, u16 wIdleBit)
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

/** Returns information about the ancillary data stream.
*/

u16 HPI_InStreamAncillaryGetInfo(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
				 HPI_HISTREAM hInStream,	///< Handle to the InStream.
				 u32 * pdwFrameSpace)	///< Maximum number of ancillary data frames that can be written to the anc frame buffer.
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

/** Writes frames to the stream's ancillary data buffer.
*
* Writes dwNumberOfAncDataFramesToWrite frames from pAncFrameBuffer to the streams ancillary data buffer.
* The first bit of ancillary information that follows the valid audio data is bit 7 of bData[0].
* The first 8 bits of ancillary information following valid audio data are from bData[0].
* In the case where there are 6 bytes total of ancillary information (48 bits) the last byte
* inserted in the frame is bData[5].
* \return_hpierr
*/

u16 HPI_InStreamAncillaryWrite(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			       HPI_HISTREAM hInStream,	///< Handle to the InStream.
			       HPI_ANC_FRAME * pAncFrameBuffer,	///< A pointer to a buffer where the ancillary data frames to write should be placed.
			       u32 dwAncFrameBufferSizeInBytes,	///< The size of the Ancillary data buffer in bytes (used for a sanity check).
			       u32 dwNumberOfAncillaryFramesToWrite	///< How many frames to write.
    )
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

/** Allocates a buffer on the host PC for bus mastering transfers.
* Assuming no error:
* Allocates a buffer on the host PC for bus mastering transfers.
* Once the buffer is allocated, InStream data will be transferred to it in the
* background. (rather than the application having to wait for the transfer)
* This function is provided so that the application can match the size of its
* transfers to the size of the buffer.
*
* From now on, HPI_InStreamGetInfoEx() will return the size and data available
* in host buffer rather than the buffers on the adapter.
* However, if the host buffer is allowed to fill up, the adapter buffers will
* then begin to fill up. In other words you still have the benefit of the
* larger adapter buffers
*
* \note There is a minimum buffer size that will work with a given audio
* format and polling rate. An appropriate size for the buffer can be
* calculated using HPI_StreamEstimateHostBufferSize()
*
* \return_hpierr
* \retval HPI_ERROR_INVALID_DATASIZE memory can't be allocated
*(retrying the call with a smaller size may succeed)
* \retval HPI_ERROR_MEMORY_ALLOC virtual address of the allocated buffer can't be found.
* \retval HPI_ERROR_INVALID_FUNC the adapter doesn't support busmastering
*/
u16 HPI_InStreamHostBufferAllocate(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
				   HPI_HISTREAM hInStream,	///< Handle of the InStream.
				   u32 dwSizeInBytes)	///< Size of bus mastering buffer to allocate.
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

/** Free any buffers allocated by HPI_InStreamHostBufferAllocate.
*
* \return_hpierr
*     \retval HPI_ERROR_INVALID_FUNC if the function is not implemented
*/
u16 HPI_InStreamHostBufferFree(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			       HPI_HISTREAM hInStream)	///< Handle of the InStream.
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

/** This function adds a stream to a group of streams. Stream groups are
* used to synchronise starting and stopping of multiple streams at once.
* The application of this is to support recording (or playing) multiple
* streams at once, enabling multi-channel recording and playback.
*
* When using the "Group" functions all streams that are to be grouped
* together should be opened. One of the streams should be selected to
* be the master stream and the other streams should be added to it's group.
* Both in streams and out streams can be added to the same group.
* Once a group has been formed, HPI_InStreamStart() called on the master
* will cause all streams to start at once.
*
* \note This function is only supported on on ASI6000 and ASI5000 adapters.
* \return_hpierr
*/
u16 HPI_InStreamGroupAdd(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle
			 HPI_HISTREAM hInStreamHandle,	///< Handle to InStream.
			 HPI_HSTREAM hStreamHandle	///< Handle to either an InStream or an OutStream.
    )
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

/** This function returns information about the streams that form a group.
* Given an out stream handle, it returns a bit mapped representation of which
* streams belong to the group.
* \note This function is only supported on on ASI6000 and ASI5000 adapters.
* \return_hpierr
*/
u16 HPI_InStreamGroupGetMap(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			    HPI_HISTREAM hInStreamHandle,	///< Handle to InStream.
			    u32 * pdwOutStreamMap,
			       /**< Bitmapped representation of OutStreams grouped with this output
stream. b0 represents OutStream 0, b1 OutStream 1, b2 OutStream 2 etc.
*/
			    u32 * pdwInStreamMap
			       /**< Bitmapped representation of InStreams grouped with this output stream.
b0 represents InStream 0, b1 InStream 1, b2 InStream 2 etc.
*/
    )
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

/** Resets stream grouping information for a given InStream.
* \note This function is only supported on on ASI6000 and ASI5000 adapters.
* \return_hpierr
*/
u16 HPI_InStreamGroupReset(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			   HPI_HISTREAM hInStreamHandle	///< Handle to InStream.
    )
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

/** Opens and initializes an adapters mixer.
*
* An Adapter index is passed in and a Mixer handle is passed back.  The handle is used for all future calls to that Mixer.
*
* \return_hpierr
*/
u16 HPI_MixerOpen(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
		  u16 wAdapterIndex,	///< Index of adapter to be opened.  Ranges from 0 to 15 and corresponds to the Adapter Index set on the adapter hardware.
		  HPI_HMIXER * phMixerHandle	///< Returned handle to the Mixer.
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

/** Closes a mixer.
* \return_hpierr
*/

u16 HPI_MixerClose(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
		   HPI_HMIXER hMixerHandle	///< Handle to the adapter mixer to close.
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

/** Gets a mixer control.
*
* The control maybe located in one of three places:\n
* -# On a connection between a source node and a destination node.\n
* You specify both source and destination nodes (via type and type index).
* -# On a source node.\n
* You specify the source node and leave the destination node type and index as 0.
* -# On a destination node.\n
* You specify the destination node and leave the source node type and index as 0.
*
* \note Not all adapters have controls at all nodes, or between all nodes.  Consult the Mixer Map for your particular
*adapter to find out the location and type of controls in its mixer.
*
* Say you have an audio adapter with a mixer that has the following layout:
*
* \image html mixer_get_control.png
*
* You can see that the mixer has two meter controls, located on each of the Outstream source nodes, two volume controls, located between
* the OutStream sources and the LineOut destination nodes and one level control located on the LineOut destination node.
*
* You would use the following code to obtain a handle to the volume control that lies on the connection between OutStream#1 and LineOut#0:
\sample
wHE = HPI_MixerGetControl(
&hSubSys,
hMixer,
HPI_SOURCENODE_OSTREAM,
1,
HPI_DESTNODE_LINEOUT,
0,
HPI_CONTROL_VOLUME,
&hVolControl
);
\endsample
*
* You would use the following code to obtain a handle to the level control that lies on the LineOut#0 node:
\sample
wHE = HPI_MixerGetControl(
&hSubSys,
hMixer,
0,
0,
HPI_DESTNODE_LINEOUT,
0,
HPI_CONTROL_LEVEL,
&hLevControl
);
\endsample
*
*\return_hpierr
*/

u16 HPI_MixerGetControl(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			HPI_HMIXER hMixerHandle,	///< Mixer handle.
			u16 wSrcNodeType,	///< Source node type i.e. \ref HPI_SOURCENODE_OSTREAM.
			u16 wSrcNodeTypeIndex,	///< Index of particular source node type i.e. the 2nd \ref HPI_SOURCENODE_OSTREAM would be specified as index=1.
			u16 wDstNodeType,	///< Destination node type i.e. \ref HPI_DESTNODE_LINEOUT.
			u16 wDstNodeTypeIndex,	///< Index of particular source node type i.e. the 3rd \ref HPI_DESTNODE_LINEOUT would be specified as index=2
			u16 wControlType,	///< Type of mixer control i.e. \ref HPI_CONTROL_METER,  VOLUME, METER or LEVEL. See additional HPI_CONTROL_xxxx types defined in section \ref control_types of HPI.H
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

/** Get the location and type of a mixer control by index
*
* This function is primarily intended to be used to enumerate all the controls in a mixer.
* To enumerate all the mixer controls of an adapter, iterate wControlIndex
* from 0 until the function returns \ref HPI_ERROR_INVALID_OBJ_INDEX.
* The iteration should not be terminated if error \ref HPI_ERROR_CONTROL_DISABLED is returned.
* This indicates that a control that normally exists is disabled for some reason (possibly hardware
* failure).  The application should not access such controls, but may for instance display a
* grayed-out GUI control.
*
* A control may exist between two nodes, or on a single node in which case
* either source or destination node type is zero.
*
*\return_hpierr
*\retval HPI_ERROR_INVALID_OBJ_INDEX when wControlIndex > number of mixer controls
*/
u16 HPI_MixerGetControlByIndex(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			       HPI_HMIXER hMixerHandle,	///< Mixer handle.
			       u16 wControlIndex,	///< Control Index to query.
			       u16 * pwSrcNodeType,	///< Returned source node type for control at index wControlIndex.
			       u16 * pwSrcNodeIndex,	///< Returned source node index for control at index wControlIndex.
			       u16 * pwDstNodeType,	///< Returned destination node type for control at index wControlIndex.
			       u16 * pwDstNodeIndex,	///< Returned destination node index for control at index wControlIndex.
			       u16 * pwControlType,	///< Returned control type for control at index wControlIndex.
			       HPI_HCONTROL * phControlHandle	///< Returned control handle for control at index wControlIndex.
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
*
* Valid commands are members of \ref HPI_MIXER_STORE_COMMAND
*
*\return_hpierr
*\retval HPI_ERROR_INVALID_OBJ_INDEX when wControlIndex > number of mixer controls
*/
u16 HPI_MixerStore(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
		   HPI_HMIXER hMixerHandle,	///< Mixer handle.
		   enum HPI_MIXER_STORE_COMMAND command,	///< Command to execute.
		   u16 wIndex	///< Optional index.
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
/** \internal
* General function for setting up to 2 u32 return values from a ControlSet call.
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

#if 0
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
#endif
/** \internal
* General function for getting up to 2 u32 return values from a ControlGet call.
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

#if 0
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
#endif
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
*
* This is done without disturbing the current setting of the control.
* Do this by iterating dwIndex from 0 until the function returns HPI_ERROR_INVALID_OBJ_INDEX.
*
* For example, to determine which bands are supported by a particular tuner, do the following:
\code
for (dwIndex=0; dwIndex<10; dwIndex++) {
wErr= HPI_ControlQuery(phSS,hC, HPI_TUNER_BAND, dwIndex, 0 , AvailableBands[dwIndex]);
if (wErr !=0) break;
}
numBands=dwIndex;
\endcode
*
* For attributes that have a range, 3 values will be returned for indices 0 to 2: minimum, maximum and step.
* The supplementary parameter dwParam is used where the possible settings for one attribute depend on the setting of another attribute, e.g. a tuners frequency range depends on which band is selected.
* For example, to determine the frequency range of the AM band, do the following:

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
u16 HPI_ControlQuery(const HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle
		     const HPI_HCONTROL hControlHandle,	///< Control to query
		     const u16 wAttrib,	///< An attribute of the control
		     const u32 dwIndex,	///< Index for possible attribute values
		     const u32 dwParam,	///< Supplementary parameter
		     u32 * pdwSetting	///< One of N possible settings for the control attribute, specified by dwIndex=0..N-1(and possibly depending on the value of the supplementary parameter)
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

The AESEBU receiver  receives audio from a standard digital audio interface (AESEBU or SPDIF).
As well as receiving the audio, status and user bits are extracted from the digital bitstream.

\image html aesebu_receiver.png

\{
*/

/** Sets the physical format of the digital audio input to either the balanced, professional AES/EBU input
* or the unbalanced, consumer S/PDIF input.  Note that not all audio adpaters will have both kinds of inputs.
* \return_hpierr
*/
u16 HPI_AESEBU_Receiver_SetFormat(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle
				  HPI_HCONTROL hControlHandle,	///<Handle to control of type HPI_CONTROL_AESEBU_RECEIVER
				  u16 wFormat	///< \ref HPI_AESEBU_FORMAT_AESEBU or \ref HPI_AESEBU_FORMAT_SPDIF
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle, HPI_AESEBU_FORMAT,
				   wFormat, 0);
}

/** Gets the physical format of the digital audio input : either the balanced, professional AES/EBU input
* or the unbalanced, consumer S/PDIF input.
* \note Not all audio adapters will have both kinds of inputs.
* \return_hpierr
*/
u16 HPI_AESEBU_Receiver_GetFormat(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
				  HPI_HCONTROL hControlHandle,	///<Handle to control of type HPI_CONTROL_AESEBU_RECEIVER.
				  u16 * pwFormat	///< current format, either \ref HPI_AESEBU_FORMAT_SPDIF or\ref HPI_AESEBU_FORMAT_AESEBU.
    )
{
	u16 wErr;
	u32 dwParam;

	wErr =
	    HPI_ControlParam1Get(phSubSys, hControlHandle, HPI_AESEBU_FORMAT,
				 &dwParam);
	if (!wErr && pwFormat)
		*pwFormat = (u16) dwParam;

	return wErr;
}

/** Returns the sample rate of the incoming AES/EBU digital audio stream in *pdwSampleRate.
* This information is obtained from the channel status bits in the digital audio bitstream.
* \return_hpierr
* \retval HPI_ERROR_INVALID_OPERATION if PLL unlocked.
*/
u16 HPI_AESEBU_Receiver_GetSampleRate(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle
				      HPI_HCONTROL hControlHandle,	///<Handle to control of type HPI_CONTROL_AESEBU_RECEIVER
				      u32 * pdwSampleRate	///< samplerate 0, 32000,44100 or 48000 (or x2, x4) returned
    )
{
	return HPI_ControlParam1Get(phSubSys, hControlHandle,
				    HPI_AESEBU_SAMPLERATE, pdwSampleRate);
}

/** Get one of 4 userdata bytes from the AES/EBU stream.
* \return_hpierr
* \note Not all audio adapters will have both kinds of inputs.
*/
u16 HPI_AESEBU_Receiver_GetUserData(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
				    HPI_HCONTROL hControlHandle,	///<Handle to control of type HPI_CONTROL_AESEBU_RECEIVER.
				    u16 wIndex,	///< byte index ranges from 0..3.
				    u16 * pwData	///< returned user data.
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

/** Get one of 24 channel status bytes from the AES/EBU stream.
* \return_hpierr
* \note Not all audio adapters will have both kinds of inputs.
*/
u16 HPI_AESEBU_Receiver_GetChannelStatus(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
					 HPI_HCONTROL hControlHandle,	///<Handle to control of type HPI_CONTROL_AESEBU_RECEIVER.
					 u16 wIndex,	///< byte index ranges from 0..23.
					 u16 * pwData	///< returned channel status data.
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

/** Get error status from the AES/EBU stream
* \return_hpierr
* \note Not all audio adapters will have both kinds of inputs.
*/

u16 HPI_AESEBU_Receiver_GetErrorStatus(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
				       HPI_HCONTROL hControlHandle,	///<Handle to control of type HPI_CONTROL_AESEBU_RECEIVER.
				       u16 * pwErrorData	///< returned error status bitfields defined by \ref aesebu_errors.
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

The AESEBU transmitter transmits audio via a standard digital audio interface (AESEBU or SPDIF).

\image html aesebu_transmitter.png

\{
*/
/** Set the AES/EBU transmitters sample rate.
* This is only valid if the source is the analog mixer
* if the source is an outstream, then the samplerate will
* be that of the outstream.
* \return_hpierr
* \note Not all audio adapters will have both kinds of inputs.
*/
u16 HPI_AESEBU_Transmitter_SetSampleRate(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
					 HPI_HCONTROL hControlHandle,	///<Handle to control of type HPI_CONTROL_AESEBU_TRANSMITTER.
					 u32 dwSampleRate	///< 32000, 44100 or 48000.
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_AESEBU_SAMPLERATE, dwSampleRate, 0);
}

/** Set one of 4 userdata bytes in the AES/EBU stream.
* \return_hpierr
*/
u16 HPI_AESEBU_Transmitter_SetUserData(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle
				       HPI_HCONTROL hControlHandle,	///<Handle to control of type HPI_CONTROL_AESEBU_TRANSMITTER
				       u16 wIndex,	///< byte index ranges from 0..3
				       u16 wData	///< user data to set (? only byte values 0..255 allowed?)
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_AESEBU_USERDATA, wIndex, wData);
}

/** Set one of 24 channel status bytes in the AES/EBU stream.
* \return_hpierr
*/
u16 HPI_AESEBU_Transmitter_SetChannelStatus(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
					    HPI_HCONTROL hControlHandle,	///<Handle to control of type HPI_CONTROL_AESEBU_TRANSMITTER.
					    u16 wIndex,	///< byte index ranges from 0..23.
					    u16 wData	///< channel status data to write (? only byte values 0..255 allowed?).
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_AESEBU_CHANNELSTATUS, wIndex, wData);
}

/** Get a byte of channel status in the AES/EBU stream.
* \warning Currently disabled pending debug of DSP code.
* \return_hpierr
* \retval Always HPI_ERROR_INVALID_OPERATION.
*/
u16 HPI_AESEBU_Transmitter_GetChannelStatus(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
					    HPI_HCONTROL hControlHandle,	///<Handle to control of type HPI_CONTROL_AESEBU_TRANSMITTER.
					    u16 wIndex,	///< byte index ranges from 0..23.
					    u16 * pwData	///< read channel status data.
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

/** Set the output electrical format for the AESEBU transmitter.
*
* Some adapters only support one of the two formats (usually AESEBU).
* \return_hpierr
*/
u16 HPI_AESEBU_Transmitter_SetFormat(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
				     HPI_HCONTROL hControlHandle,	///<Handle to control of type HPI_CONTROL_AESEBU_TRANSMITTER.
				     u16 wOutputFormat	///< formats either \ref HPI_AESEBU_FORMAT_SPDIF or \ref HPI_AESEBU_FORMAT_AESEBU.
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle, HPI_AESEBU_FORMAT,
				   wOutputFormat, 0);
}

/** Get the current output electrical format for the AESEBU transmitter.
* \return_hpierr
*/
u16 HPI_AESEBU_Transmitter_GetFormat(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
				     HPI_HCONTROL hControlHandle,	///<Handle to control of type HPI_CONTROL_AESEBU_TRANSMITTER.
				     u16 * pwOutputFormat	///< Current format either \ref HPI_AESEBU_FORMAT_AESEBU or \ref HPI_AESEBU_FORMAT_SPDIF.
    )
{
	u16 wErr;
	u32 dwParam;

	wErr =
	    HPI_ControlParam1Get(phSubSys, hControlHandle, HPI_AESEBU_FORMAT,
				 &dwParam);
	if (!wErr && pwOutputFormat)
		*pwOutputFormat = (u16) dwParam;

	return wErr;
}

/*\}*/
/////////////////////////////////////////////////////////////////////////
/**\defgroup bitstream Bitstream control
Control synchronous bitstream I/O.  Only valid on ASI4346 adapters.
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

/**
* Returns 2 indicative measurements of the incoming data stream.
*
* The clock input is deemed inactive if no data bytes are received within a certain
* number of calls to a polling routine.  The time this takes varies according to the
* number of active streams.  If there is clock activity, the data activity indicator
* is a sample of 16 bits from the incoming data.  If this is persistently 0 or 0xFFFF,
* this may indicate that the data input is inactive.
*
* \return_hpierr
*/

u16 HPI_Bitstream_GetActivity(HPI_HSUBSYS * phSubSys,	///<Subsystem handle
			      HPI_HCONTROL hControlHandle,	///<Handle to bitstream control
			      u16 * pwClkActivity,	///< 1==clock is active, 0==clock is inactive
			      u16 * pwDataActivity	///< 1 word sampled from the incoming raw data
    )
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
A Channel Mode allows you to swap the left and right channels,
mix the left and right channels into the left or right channel only, or
send the left or right channels to both left and right.

@{
*/

/** Set the channel mode
\return_hpierr
*/
u16 HPI_ChannelModeSet(HPI_HSUBSYS * phSubSys,	///<Subsystem handle
		       HPI_HCONTROL hControlHandle,	///<Handle of a Channel Mode control
		       u16 wMode	///< One of the supported HPI_CHANNEL_MODE_XXX \ref channel_modes.
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_MULTIPLEXER_SOURCE, wMode, 0);
}

/** Get the current channel mode
\return_hpierr
*/
u16 HPI_ChannelModeGet(HPI_HSUBSYS * phSubSys,	///<Subsystem handle
		       HPI_HCONTROL hControlHandle,	///<Handle of a Channel Mode control
		       u16 * wMode	///< One of the supported HPI_CHANNEL_MODE_XXX \ref channel_modes.
    )
{
	u32 dwMode = 0;
	u16 wError =
	    HPI_ControlParam1Get(phSubSys, hControlHandle,
				 HPI_MULTIPLEXER_SOURCE, &dwMode);
	if (wMode)
		*wMode = (u16) dwMode;
	return (wError);
}

	  /** @} */// group channelmode

////////////////////////////////////////////////////////////////////////////////
/**\defgroup cobranet Cobranet control
A cobranet adapter has one cobranet control for each cobranet interface (usually only one).
The cobranet control is located on (HPI_SOURCENODE_COBRANET,0,HPI_DESTNODE_COBRANET,0)
The cobranet control allows reading and writing of the cobranet HMI variables
(See Cirrus cobranet documentation @ www.cobranet.info for details)

@{
*/

/** Write to an HMI variable.
\return_hpierr
\retval HPI_ERROR_INVALID_CONTROL if type is not cobranet
\retval HPI_ERROR_INVALID_OPERATION if HMI variable is not writeable
\retval HPI_ERROR_INVALID_DATASIZE if requested size is greater than the HMI variable size
*/

u16 HPI_Cobranet_HmiWrite(HPI_HSUBSYS * phSubSys,	///<Subsystem handle
			  HPI_HCONTROL hControlHandle,	///<Handle of a Cobranet control
			  u32 dwHmiAddress,	///< dwHmiAddress HMI address
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
\return_hpierr
\retval HPI_ERROR_INVALID_CONTROL if type is not cobranet

The amount of data returned will be the minimum of the input dwMaxByteCount and the actual
size of the variable reported by the HMI
*/

u16 HPI_Cobranet_HmiRead(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
			 HPI_HCONTROL hControlHandle,	///< Handle of a cobranet control
			 u32 dwHmiAddress,	///< HMI address
			 u32 dwMaxByteCount,	///< maximum number of bytes to return (<= buffer size of pbData)
			 u32 * pdwByteCount,	///< actual number of bytes returned
			 u8 * pbData	///< data read from HMI variable
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
* \return_hpierr
*/
u16 HPI_Cobranet_HmiGetStatus(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
			      HPI_HCONTROL hControlHandle,	///< Handle of a cobranet control
			      u32 * pdwStatus,	///< the raw status word from the HMI
			      u32 * pdwReadableSize,	///< the reported readable size from the last variable access
			      u32 * pdwWriteableSize	///< the reported writeable size from the last variable access
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

/** Get the CobraNet node's current IP address.
* Allows the user to get the current IP address of the CobraNet node.
* \return_hpierr 0 on success, or one of the \ref errorcodes
* \retval HPI_ERROR_INVALID_CONTROL if type is not cobranet
*/
u16 HPI_Cobranet_GetIPaddress(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
			      HPI_HCONTROL hControlHandle,	///< Handle of a cobranet control
			      u32 * pdwIPaddress	///< the current IP address
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

	if (wError)
		*pdwIPaddress = 0;

	return wError;

}

/** Set the CobraNet node's current IP address.
* Allows the user to set the current IP address of the CobraNet node.
* \return_hpierr
* \retval HPI_ERROR_INVALID_CONTROL if type is not cobranet
*/
u16 HPI_Cobranet_SetIPaddress(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
			      HPI_HCONTROL hControlHandle,	///< Handle of a cobranet control
			      u32 dwIPaddress	///< the new current IP address
    )
{
	u32 dwIP;
	u16 wError;

// byte swap the IP address
	dwIP =
	    ((dwIPaddress & 0xff000000) >> 8) |
	    ((dwIPaddress & 0x00ff0000) << 8) |
	    ((dwIPaddress & 0x0000ff00) >> 8) |
	    ((dwIPaddress & 0x000000ff) << 8);

	wError = HPI_Cobranet_HmiWrite(phSubSys, hControlHandle,
				       HPI_COBRANET_HMI_cobraIpMonCurrentIP,
				       4, (u8 *) & dwIP);

	return wError;

}

/** Get the CobraNet node's static IP address.
\return_hpierr
\retval HPI_ERROR_INVALID_CONTROL if type is not cobranet

Allows the user to get the static IP address of the CobraNet node.
*/
u16 HPI_Cobranet_GetStaticIPaddress(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
				    HPI_HCONTROL hControlHandle,	///< Handle of a cobranet control
				    u32 * pdwIPaddress	///< the static IP address
    )
{
	u32 dwByteCount;
	u32 dwIP;
	u16 wError;
	wError = HPI_Cobranet_HmiRead(phSubSys, hControlHandle,
				      HPI_COBRANET_HMI_cobraIpMonStaticIP,
				      4, &dwByteCount, (u8 *) & dwIP);
// byte swap the IP address
	*pdwIPaddress =
	    ((dwIP & 0xff000000) >> 8) |
	    ((dwIP & 0x00ff0000) << 8) |
	    ((dwIP & 0x0000ff00) >> 8) | ((dwIP & 0x000000ff) << 8);

	if (wError)
		*pdwIPaddress = 0;

	return wError;

}

/** Set the CobraNet node's static IP address.
\return_hpierr
\retval HPI_ERROR_INVALID_CONTROL if type is not cobranet

Allows the user to set the static IP address of the CobraNet node.
*/
u16 HPI_Cobranet_SetStaticIPaddress(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
				    HPI_HCONTROL hControlHandle,	///< Handle of a cobranet control
				    u32 dwIPaddress	///< the new static IP address
    )
{
	u32 dwIP;
	u16 wError;

// byte swap the IP address
	dwIP =
	    ((dwIPaddress & 0xff000000) >> 8) |
	    ((dwIPaddress & 0x00ff0000) << 8) |
	    ((dwIPaddress & 0x0000ff00) >> 8) |
	    ((dwIPaddress & 0x000000ff) << 8);

	wError = HPI_Cobranet_HmiWrite(phSubSys, hControlHandle,
				       HPI_COBRANET_HMI_cobraIpMonStaticIP,
				       4, (u8 *) & dwIP);

	return wError;

}

/** Get the CobraNet node's MAC address.
\return_hpierr
\retval HPI_ERROR_INVALID_CONTROL if type is not cobranet

Allows the user to get the MAC address of the CobraNet node.
*/
u16 HPI_Cobranet_GetMACaddress(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
			       HPI_HCONTROL hControlHandle,	///< Handle of a cobranet control
			       u32 * pdwMAC_MSBs,	///< the first 4 bytes of the MAC address.
			       u32 * pdwMAC_LSBs	///< the last 2 bytes of the MAC address returned in the upper 2 bytes.
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

	if (wError) {
		*pdwMAC_MSBs = 0;
		*pdwMAC_LSBs = 0;
	}

	return wError;
}

	  /** @} */// group cobranet

/////////////////////////////////////////////////////////////////////////////////
/**\defgroup compand  Compressor Expander control
\{
The compander multiplies its input signal by a factor that is dependent on the
amplitude of that signal.
*/
/** Set up a compressor expander
\return_hpierr
*/
u16 HPI_Compander_Set(HPI_HSUBSYS * phSubSys,	///<HPI subsystem handle
		      HPI_HCONTROL hControlHandle,	///< Compander control handle
		      u16 wAttack,	///< attack time in milliseconds
		      u16 wDecay,	///< decay time in milliseconds
		      short wRatio100,	///< gain ratio * 100
		      short nThreshold0_01dB,	///< threshold in 100ths of a dB
		      short nMakeupGain0_01dB	///< makeup gain in 100ths of a dB
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
\return_hpierr
*/
u16 HPI_Compander_Get(HPI_HSUBSYS * phSubSys,	///<HPI subsystem handle
		      HPI_HCONTROL hControlHandle,	///<Equalizer control handle
		      u16 * pwAttack,	///<attack time in milliseconds
		      u16 * pwDecay,	///<decay time in milliseconds
		      short *pwRatio100,	///<gain ratio * 100
		      short *pnThreshold0_01dB,	///<threshold in 100ths of a dB
		      short *pnMakeupGain0_01dB	///<makeup gain in 100ths of a dB
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

/** Sets the gain of a level control.
* The level, or trim as it is sometimes called, sets the level of an analog input
* or output of a HPI_CONTROL_LEVEL control. The gains will typically range between 0 and +24dBu.
* \note The gain is stereo.
* \return_hpierr
*/
u16 HPI_LevelSetGain(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
		     HPI_HCONTROL hControlHandle,	///< Handle to control of type HPI_CONTROL_LEVEL.
		     short anGain0_01dB[HPI_MAX_CHANNELS]
						   /**< Array containing the level control gain.
The gain is in units of 0.01 dBu, where 0dBu = 0.775VRMS. For example a gain
of 1400 is 14dBu.  Index 0 is the left channel and index 1 is the right
channel.
*/
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

/** Gets the gain of a level control.
* Gets the level of an analog input or output of a HPI_CONTROL_LEVEL control. The gains
* will typically range between 0 and +24dBu.
* \note The gain is stereo.
* \return_hpierr
*/

u16 HPI_LevelGetGain(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
		     HPI_HCONTROL hControlHandle,	///< Handle to control of type HPI_CONTROL_LEVEL.
		     short anGain0_01dB[HPI_MAX_CHANNELS]
						   /**< Array containing the level control gain.
The gain is in units of 0.01 dBu, where 0dBu = 0.775VRMS. For example a gain
of 1400 is 14dBu.  Index 0 is the left channel and index 1 is the right
channel.
*/
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
* \return_hpierr
*/
u16 HPI_MeterGetPeak(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle
		     HPI_HCONTROL hControlHandle,	///< meter control handle
		     short anPeakdB[HPI_MAX_CHANNELS]	///< meter peaks in millibels
    )
{
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

		memcpy(anPeakdB, hr.u.c.anLogValue,
		       sizeof(short) * HPI_MAX_CHANNELS);
	} else
		for (i = 0; i < HPI_MAX_CHANNELS; i++)
			anPeakdB[i] = HPI_METER_MINIMUM;
	return (hr.wError);
}

/** Get the meter RMS reading in 100ths of a dB
* \return_hpierr
*/
u16 HPI_MeterGetRms(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle
		    HPI_HCONTROL hControlHandle,	///< meter control handle
		    short anRmsdB[HPI_MAX_CHANNELS]	///< meter RMS values in millibels
    )
{
	short i = 0;

	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_METER_RMS;

	HPI_Message(&hm, &hr);

	if (!hr.wError) {
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

\return_hpierr
*/
u16 HPI_MeterSetRmsBallistics(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle
			      HPI_HCONTROL hControlHandle,	///< Handle to control of type HPI_CONTROL_METER
			      unsigned short nAttack,	///< Attack timeconstant in milliseconds
			      unsigned short nDecay	///< Decay timeconstant in milliseconds
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_METER_RMS_BALLISTICS, nAttack, nDecay);
}

/** Get the ballistics settings of the RMS part of a meter.
\return_hpierr
*/
u16 HPI_MeterGetRmsBallistics(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle
			      HPI_HCONTROL hControlHandle,	///< Handle to control of type HPI_CONTROL_METER
			      unsigned short *pnAttack,	///< Attack timeconstant in milliseconds
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
\return_hpierr
*/
u16 HPI_MeterSetPeakBallistics(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle
			       HPI_HCONTROL hControlHandle,	///< Handle to control of type HPI_CONTROL_METER
			       unsigned short nAttack,	///< Attack timeconstant in milliseconds
			       unsigned short nDecay	///< Decay timeconstant in milliseconds
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_METER_PEAK_BALLISTICS, nAttack, nDecay);
}

/** Get the ballistics settings of the Peak part of a meter.
\return_hpierr
*/
u16 HPI_MeterGetPeakBallistics(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle
			       HPI_HCONTROL hControlHandle,	///< Handle to control of type HPI_CONTROL_METER
			       unsigned short *pnAttack,	///< Attack timeconstant in milliseconds
			       unsigned short *pnDecay	///< Decay timeconstant in milliseconds
    )
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

/** \} */

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
A Microphone control of type HPI_CONTROL_MICROPHONE is always located on a source node of type
HPI_SOURCE_NODE_MICROPHONE.
This node type receives an audio signal from a microphone. If the microphone has adjustable
gain, then a VOLUME control will also be present on the node. Currently the Microphone
control is only used to turn on/off the microphone's phantom power.
\{
*/

/** Sets the microphone phantom power on or off.
\return_hpierr
*/
u16 HPI_Microphone_SetPhantomPower(HPI_HSUBSYS * phSubSys,	///< subsystem handle
				   HPI_HCONTROL hControlHandle,	///< Control handle to type HPI_CONTROL_MICROPHONE
				   u16 wOnOff	///< Should be set to 1 to turn on the microphone phantom power and 0 to turn it off.
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_MICROPHONE_PHANTOM_POWER, (u32) wOnOff,
				   0);
}

/** Gets the current microphone phantom power setting.
\return_hpierr
*/
u16 HPI_Microphone_GetPhantomPower(HPI_HSUBSYS * phSubSys,	///< subsystem handle
				   HPI_HCONTROL hControlHandle,	///< Control handle to type HPI_CONTROL_MICROPHONE
				   u16 * pwOnOff	///< Returns 1 if the microphone phantom power is on, 0 if off.
    )
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

/** Set the signal source that the multiplexer will send to the destination
\return_hpierr
*/
u16 HPI_Multiplexer_SetSource(HPI_HSUBSYS * phSubSys,	///< subsystem handle
			      HPI_HCONTROL hControlHandle,	///< Control handle to type HPI_CONTROL_MULTIPLEXER
			      u16 wSourceNodeType,	///< source node type - one of HPI_SOURCENODE_XXX \ref source_nodes
			      u16 wSourceNodeIndex	///< a 0 based index
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_MULTIPLEXER_SOURCE, wSourceNodeType,
				   wSourceNodeIndex);
}

/** Get the signal source that the multiplexer is currently connected to
\return_hpierr
*/
u16 HPI_Multiplexer_GetSource(HPI_HSUBSYS * phSubSys,	///< subsystem handle
			      HPI_HCONTROL hControlHandle,	///< Control handle to type HPI_CONTROL_MULTIPLEXER
			      u16 * wSourceNodeType,	///< source node type - one of HPI_SOURCENODE_XXX \ref source_nodes
			      u16 * wSourceNodeIndex	///< a 0 based index
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

/** Establish valid source node settings for this multiplexer.
* Call with wIndex starting at zero, incrementing until a non-zero return
* value indicates that the current wIndex is invalid, so querying should be terminated.
* \return_hpierr
*/
u16 HPI_Multiplexer_QuerySource(HPI_HSUBSYS * phSubSys,	///< subsystem handle
				HPI_HCONTROL hControlHandle,	///< Control handle to type HPI_CONTROL_MULTIPLEXER
				u16 wIndex,	///< a 0 based index
				u16 * wSourceNodeType,	///< source node type - one of HPI_SOURCENODE_XXX \ref source_nodes
				u16 * wSourceNodeIndex	///< a 0 based index
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

/////////////////////////////////////////////////////////////////////////////////
/**\addtogroup parmeq  Parametric Equalizer control

The parametric equalizer control consists of a series of filters that are applied
successively to the signal. The number of filters available is obtained by calling
HPI_ParametricEQ_GetInfo(), then the characteristics of each filter are configured
using HPI_ParametricEQ_SetBand().

The equalizer as a whole can be turned on and off using HPI_ParametricEQ_SetState().
Filters can still be set up when the equalizer is switched off.

Equalizers are typically located on a LineIn input node or an OutStream node.

Obtain a control handle to an equalizer like this:

\code
wHE = HPI_MixerGetControl(
phSubSys,
hMixer,
HPI_SOURCENODE_LINEIN, 0,
0,0,  // No destination node
HPI_CONTROL_PARAMETRIC_EQ,
&hControl
);
\endcode
\{
*/

/**     Find out the number of available bands of a parametric equalizer, and whether it is enabled or not.
* \return_hpierr
*/
u16 HPI_ParametricEQ_GetInfo(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			     HPI_HCONTROL hControlHandle,	///< Equalizer control handle.
			     u16 * pwNumberOfBands,	///< Returned number of bands available.
			     u16 * pwOnOff	///< Returned enabled status. 1 indicates enabled and 0 indicates disabled.
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

/**     Turn a parametric equalizer on or off.
*\return_hpierr
*/
u16 HPI_ParametricEQ_SetState(HPI_HSUBSYS * phSubSys,	///<  Pointer to HPI subsystem handle.
			      HPI_HCONTROL hControlHandle,	///<  Equalizer control handle
			      u16 wOnOff	///<  State setting, either \ref HPI_SWITCH_ON or \ref HPI_SWITCH_OFF.
    )
{
	return HPI_ControlParamSet(phSubSys,
				   hControlHandle,
				   HPI_EQUALIZER_NUM_FILTERS, wOnOff, 0);
}

/** Get the settings of one of the filters in a parametric equalizer.
See HPI_ParametricEQ_SetBand() for details of parameter interpretation.
\return_hpierr
*/
u16 HPI_ParametricEQ_GetBand(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			     HPI_HCONTROL hControlHandle,	///< Equalizer control handle
			     u16 wIndex,	///< Index of band to Get.
			     u16 * pnType,	///< Returned band type.
			     u32 * pdwFrequencyHz,	///< Returned band frequency.
			     short *pnQ100,	///< Returned filter Q * 100.
			     short *pnGain0_01dB	///< Returned filter gain in 100ths of a dB.
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

/** Set up one of the filters in a parametric equalizer.
Set the parameters for one equalizer filter.
The overall equalizer response will be a product of all its filter responses.

\return_hpierr

\param phSubSys Pointer to HPI subsystem handle.
\param hControlHandle Equalizer control handle.
\param wIndex Index of band to set.
\param nType The kind of filter that the band will implement has many different options.
In the following descriptions, low and high frequencies mean relative to dwBandFrequency.
"Elsewhere" means at frequencies different from dwBandFrequency, how different depends on
Q (look at the following figures)
- HPI_FILTER_TYPE_BYPASS\n
The filter is bypassed (turned off). Other parameters are ignored
- HPI_FILTER_TYPE_LOWPASS\n
Has unity gain at low frequencies, and attenuation tending towards infinite at high frequencies
nGain0_01dB parameter is ignored.
See "lp" in the following diagrams.
- HPI_FILTER_TYPE_HIGHPASS\n
Has unity gain at high frequencies, and attenuation tending towards infinite at low frequencies
nGain0_01dB parameter is ignored.
(Not illustrated, basically the opposite of lowpass)
- HPI_FILTER_TYPE_BANDPASS\n
Has unity gain at dwFrequencyHz and tends towards infinite attenuation elsewhere
nGain0_01dB parameter is ignored.
See "bp" in the following diagrams.
- HPI_FILTER_TYPE_BANDSTOP\n
Maximum attenuation at dwFrequencyHz, tends towards unity gain elsewhere.
nGain0_01dB parameter is ignored.
See "bs" in the following diagrams.
- HPI_FILTER_TYPE_LOWSHELF\n
Has gain of nGain0_01dB at low frequencies and unity gain at high frequencies.
See "ls" in the following diagrams.
- HPI_FILTER_TYPE_HIGHSHELF\n
Has gain of nGain0_01dB at high frequencies and unity gain at low frequencies.
See "hs" in the following diagrams.
- HPI_FILTER_TYPE_EQ_BAND \n
Has gain of nGain0_01dB at dwFrequencyHz and unity gain elsewhere.
See "eq" in the following diagrams.

\param dwFrequencyHz is the defining frequency of the filter.  It is the center frequency of types
HPI_FILTER_TYPE_ BANDPASS, HPI_FILTER_TYPE_BANDSTOP, HPI_FILTER_TYPE_EQ_BAND.
It is the -3dB frequency of HPI_FILTER_TYPE_LOWPASS, HPI_FILTER_TYPE_HIGHPASS when Q=1 or resonant
frequency when Q>1 and it is the half gain frequency of HPI_FILTER_TYPE_LOWSHELF, HPI_FILTER_TYPE_HIGHSHELF.
The maximum allowable value is half the current adapter samplerate i.e. Fs/2.   When the adapter samplerate
is changed, the equalizer filters will be recalculated.  If this results in the band frequency being greater
than Fs/2, then the filter will be turned off.

\param nQ100 controls filter sharpness. To allow the use of an integer parameter, Filter Q = dwQ100/100.\n
In the following figure, gain is 20dB (10x) (nGain0_01dB=2000) and sampling frequency is normalized to
1Hz (10^0) and nFrequency is 0.1 x sampling frequency. Q=[0.2 0.5 1 2 4 8].\n
Q can also be thought of as affecting bandwidth or shelf slope of some of these filters.\n
Bandwidth is measured in octaves (between -3 dB frequencies for BPF and notch or between midpoint (dBgain/2)
gain frequencies for peaking EQ).\n
The relationship between bandwidth and Q is:
\code
1/Q = 2*sinh[ln(2)/2*bandwidth*omega/sin(omega)]  (digital filter using BLT)
or      1/Q = 2*sinh[ln(2)/2*bandwidth])           (analog filter prototype)
Where omega = 2*pi*frequency/sampleRate
\endcode
Shelf slope S, a "shelf slope" parameter (for shelving EQ only).  When S = 1, the shelf slope is as steep
as it can be and remain monotonically increasing or decreasing gain with frequency.  The shelf slope, in
dB/octave, remains proportional to S for all other values.\n
The relationship between shelf slope and Q is 1/Q = sqrt[(A + 1/A)*(1/S - 1) + 2]\n
where A  = 10^(dBgain/40)\n
Effect of Q on EQ filters \image html EQ_effect_of_Q.png

\param nGain0_01dB The gain is expressed in milliBels (100ths of a decibel).
Allowable range is -1000 to +1000 mB. Usable range will likely be less than this.
This parameter is only applicable to the equalizer filter types HPI_FILTER_TYPE_LOWSHELF,
HPI_FILTER_TYPE_HIGHSHELF and HPI_FILTER_TYPE_EQ_BAND.Other filters always have unity gain in the passband.\n
In the following figure, Q=1.0 and sampling frequency is normalized to 1Hz (10^0) and nFrequency is 0.1 x sampling frequency.
dBgain=[-20 -10 0 10 20]\n
For example, to produce the upper (red) curve in the "Filtertype_eq_band" graph:
\code
wHE= HPI_ParametricEQ_SetBand(
phSubsys,
hMicrophoneControl,
HPI_FILTER_TYPE_EQ_BAND,
4410    // 4.41khz
100                     // Q=1
20*100, // 20dB
);
\endcode
Effect of gain on EQ filters \image html EQ_effect_of_gain.png
*/
u16 HPI_ParametricEQ_SetBand(HPI_HSUBSYS * phSubSys,
			     HPI_HCONTROL hControlHandle,
			     u16 wIndex,
			     u16 nType,
			     u32 dwFrequencyHz, short nQ100, short nGain0_01dB)
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

/** Retrieve the calculated IIR filter coefficients (scaled by 1000 into integers).
\return_hpierr
*/
u16 HPI_ParametricEQ_GetCoeffs(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			       HPI_HCONTROL hControlHandle,	///< Equalizer control handle.
			       u16 wIndex,	///< Index of band to Get.
			       short coeffs[5]	///< Returned IIR filter coefficients * 1000 a1,a2,b0,b1,b2 (a0==0).
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
The SampleClock control is used to control the clock source for the adapter.
The SampleClock control is always attached to a node of type HPI_SOURCENODE_CLOCK_SOURCE. To query supported
sample rates, see HPI_ControlQuery().

@{
*/

/** Sets the clock source for the sample clock.
\return_hpierr
*/
u16 HPI_SampleClock_SetSource(HPI_HSUBSYS * phSubSys,	///<Subsys handle
			      HPI_HCONTROL hControlHandle,	///<Handle to control of type HPI_CONTROL_SAMPLECLOCK
			      u16 wSource	///<Sample clock source - one of HPI_SAMPLECLOCK_SOURCE_XXX \ref sampleclock_source.
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_SAMPLECLOCK_SOURCE, wSource, 0);
}

/** Sets the adapter clock source for the samplerate generators to one of the AESUBU inputs.
Note, to use this function the source must already be set to HPI_SAMPLECLOCK_SOURCE_AESEBU_INPUT
\return_hpierr
*/
u16 HPI_SampleClock_SetSourceIndex(HPI_HSUBSYS * phSubSys,	///<Subsys handle
				   HPI_HCONTROL hControlHandle,	///<Handle to control of type HPI_CONTROL_SAMPLECLOCK
				   u16 wSourceIndex	///<Index of the source to use
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_SAMPLECLOCK_SOURCE_INDEX, wSourceIndex,
				   0);
}

/** Gets the current sample clock source.
\return_hpierr
*/
u16 HPI_SampleClock_GetSource(HPI_HSUBSYS * phSubSys,	///<Subsys handle
			      HPI_HCONTROL hControlHandle,	///<Handle to control of type HPI_CONTROL_SAMPLECLOCK
			      u16 * pwSource	///<Sample clock source - one of HPI_SAMPLECLOCK_SOURCE_XXX \ref sampleclock_source.
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

/** Gets the AES/EBU input used to source the adapter clock.
Note, to use this function the source must already be set to HPI_SAMPLECLOCK_SOURCE_AESEBU_INPUT
\return_hpierr
*/
u16 HPI_SampleClock_GetSourceIndex(HPI_HSUBSYS * phSubSys,	///<Subsys handle
				   HPI_HCONTROL hControlHandle,	///<Handle to control of type HPI_CONTROL_SAMPLECLOCK
				   u16 * pwSourceIndex	///<Index of the current source
    )
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

/** Sets the adapter samplerate when the SampleClock source is HPI_SAMPLECLOCK_SOURCE_ADAPTER or HPI_SAMPLECLOCK_SOURCE_LOCAL
\return_hpierr
*/
u16 HPI_SampleClock_SetSampleRate(HPI_HSUBSYS * phSubSys,	///<Subsys handle
				  HPI_HCONTROL hControlHandle,	///<Handle to control of type HPI_CONTROL_SAMPLECLOCK
				  u32 dwSampleRate	///<Sample rate to set. Valid values depend on adapter type.
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle,
				   HPI_SAMPLECLOCK_SAMPLERATE, dwSampleRate, 0);
}

/** Gets the current adapter samplerate
\return_hpierr
*/
u16 HPI_SampleClock_GetSampleRate(HPI_HSUBSYS * phSubSys,	///<Subsys handle
				  HPI_HCONTROL hControlHandle,	///<Handle to control of type HPI_CONTROL_SAMPLECLOCK
				  u32 * pdwSampleRate	///<Current sample rate
    )
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

	  /** @} */// group sampleclock

/////////////////////////////////////////////////////////////////////////////////
/**\defgroup tonedetector Tone Detector control
\{
The tone detector monitors its inputs for the presence of any of a number of tones.

Currently 25Hz and 35Hz tones can be detected independently on left and right channels.
Tones that exceed the threshold set by HPI_ToneDetector_SetThreshold() are detected.
The result of the detection is reflected in the controls state, and optionally by sending an
async event with the new state.
*/

/**  Enumerate the detection frequencies of the tone detector control
* \param phSubSys HPI subsystem handle.
* \param hControl Handle to tone detector control.
* \param nIndex iterate this from zero to get detector frequencies in *dwFreqency
* \param *dwFrequency  detection frequency of tone detector band number nIndex
* \return_hpierr
* \retval #HPI_ERROR_INVALID_CONTROL_VALUE if nIndex >= number of frequencies supported
*/
u16 HPI_ToneDetector_GetFrequency(HPI_HSUBSYS * phSubSys,
				  HPI_HCONTROL hControl,
				  u32 nIndex, u32 * dwFrequency)
{
	return HPI_ControlParamGet(phSubSys, hControl,
				   HPI_TONEDETECTOR_FREQUENCY, nIndex, 0,
				   dwFrequency, NULL);
}

/**  Get tone detector state.
* \param phSubSys HPI subsystem handle.
* \param hControl Handle to tone detector control.
* \param *State Tonedetector state reflected in the bits of *State.
*            Upper 16 bits is right channel, lower 16 bits is left channel.
*            LSB represents lowest frequency detector (25Hz)
* \return_hpierr
*/
u16 HPI_ToneDetector_GetState(HPI_HSUBSYS * phSubSys,
			      HPI_HCONTROL hControl, u32 * State)
{
	return HPI_ControlParamGet(phSubSys, hControl, HPI_TONEDETECTOR_STATE,
				   0, 0, (u32 *) State, NULL);
}

/** Enable (or disable) a ToneDetector control
* \param phSubSys HPI subsystem handle.
* \param hControl Handle to tone detector control.
* \param Enable 1=enable, 0=disable
*/
u16 HPI_ToneDetector_SetEnable(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControl,
			       u32 Enable)
{
	return HPI_ControlParamSet(phSubSys, hControl, HPI_GENERIC_ENABLE,
				   (u32) Enable, 0);
}

/** Get the Enable state of a ToneDetector control
* \param phSubSys HPI subsystem handle.
* \param hControl Handle to tone detector control.
* \param *Enable 1=enable, 0=disable
*/
u16 HPI_ToneDetector_GetEnable(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControl,
			       u32 * Enable)
{
	return HPI_ControlParamGet(phSubSys, hControl, HPI_GENERIC_ENABLE, 0, 0,
				   (u32 *) Enable, NULL);
}

/** Enable ToneDetector control event generation
* \param phSubSys HPI subsystem handle.
* \param hControl Handle to tone detector control.
* \param EventEnable 1=enable, 0=disable
*/
u16 HPI_ToneDetector_SetEventEnable(HPI_HSUBSYS * phSubSys,
				    HPI_HCONTROL hControl, u32 EventEnable)
{
	return HPI_ControlParamSet(phSubSys, hControl, HPI_GENERIC_EVENT_ENABLE,
				   (u32) EventEnable, 0);
}

/** Get the event generation enable state of a ToneDetector control
* \param phSubSys HPI subsystem handle.
* \param hControl Handle to tone detector control.
* \param EventEnable 1=enable, 0=disable
*/
u16 HPI_ToneDetector_GetEventEnable(HPI_HSUBSYS * phSubSys,
				    HPI_HCONTROL hControl, u32 * EventEnable)
{
	return HPI_ControlParamGet(phSubSys, hControl, HPI_GENERIC_EVENT_ENABLE,
				   0, 0, (u32 *) EventEnable, NULL);
}

/** Set the Threshold of a ToneDetector control.
* \param phSubSys HPI subsystem handle.
* \param hControl Handle to tone detector control.
* \param Threshold in millibels wrt full scale.  E.g. -2000 -> -20dBFS threshold.
* Tones with level above this threshold are detected.
* \return_hpierr
*/
u16 HPI_ToneDetector_SetThreshold(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControl,
				  int Threshold)
{
	return HPI_ControlParamSet(phSubSys, hControl,
				   HPI_TONEDETECTOR_THRESHOLD, (u32) Threshold,
				   0);
}

/** Get the Threshold of a ToneDetector control
* \param phSubSys HPI subsystem handle.
* \param hControl Handle to tone detector control.
* \param *Threshold current threshold, \sa HPI_ToneDetector_SetThreshold()
*/
u16 HPI_ToneDetector_GetThreshold(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControl,
				  int *Threshold)
{
	return HPI_ControlParamGet(phSubSys, hControl,
				   HPI_TONEDETECTOR_THRESHOLD, 0, 0,
				   (u32 *) Threshold, NULL);
}

///\}

///////////////////////////////////////////////////////////////////////////////
/** \defgroup silence Silence Detector Controls
*
* The silence detector control monitors its input for silence exceeding a set duration.
* Silence is defined as signal below a specified threshold set by HPI_SilenceDetector_SetThreshold()
* The duration is specified by  HPI_SilenceDetector_SetDelay()
* silence-detected state is reset immediately the signal exceeds the threshold (no delay)
\{
*/

/** Get the State of a SilenceDetector control
* \param phSubSys subsystem handle \param hControl silence detector handle
* \param *State The state is a bitmap corresponding to the channels
*               being monitored (LSB=left channel, LSB+1=right channel)
* \return_hpierr
*/
u16 HPI_SilenceDetector_GetState(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControl,
				 u32 * State)
{
	return HPI_ControlParamGet(phSubSys, hControl, HPI_GENERIC_ENABLE, 0, 0,
				   (u32 *) State, NULL);
}

/**  Enable a SilenceDetector control
* \param phSubSys subsystem handle \param hControl silence detector handle
* \param *Enable 1=enable, 0=disable
* \return_hpierr
*/
u16 HPI_SilenceDetector_SetEnable(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControl,
				  u32 Enable)
{
	return HPI_ControlParamSet(phSubSys, hControl, HPI_GENERIC_ENABLE,
				   (u32) Enable, 0);
}

/** Get the Enable setting of a SilenceDetector control
* \param phSubSys subsystem handle \param hControl silence detector handle
* \param Enable 1=enable, 0=disable
* \return_hpierr
*/
u16 HPI_SilenceDetector_GetEnable(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControl,
				  u32 * Enable)
{
	return HPI_ControlParamGet(phSubSys, hControl, HPI_GENERIC_ENABLE, 0, 0,
				   (u32 *) Enable, NULL);
}

/** Set the event generation by a SilenceDetector control
* \param phSubSys subsystem handle \param hControl silence detector handle
* \param EventEnable 1=enable, 0=disable
* \return_hpierr
*/
u16 HPI_SilenceDetector_SetEventEnable(HPI_HSUBSYS * phSubSys,
				       HPI_HCONTROL hControl, u32 EventEnable)
{
	return HPI_ControlParamSet(phSubSys, hControl, HPI_GENERIC_EVENT_ENABLE,
				   (u32) EventEnable, 0);
}

/** Get the event generation enable setting of a SilenceDetector control
* \param phSubSys subsystem handle \param hControl silence detector handle
* \param *EventEnable 1=enable, 0=disable
* \return_hpierr
*/
u16 HPI_SilenceDetector_GetEventEnable(HPI_HSUBSYS * phSubSys,
				       HPI_HCONTROL hControl, u32 * EventEnable)
{
	return HPI_ControlParamGet(phSubSys, hControl, HPI_GENERIC_EVENT_ENABLE,
				   0, 0, (u32 *) EventEnable, NULL);
}

/** Set the Delay of a SilenceDetector control
* \param phSubSys subsystem handle \param hControl silence detector handle
* \param Delay  Trigger delay in milliseconds, max 60000 (60 seconds)
* \return_hpierr
*/
u16 HPI_SilenceDetector_SetDelay(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControl,
				 u32 Delay)
{
	return HPI_ControlParamSet(phSubSys, hControl,
				   HPI_SILENCEDETECTOR_DELAY, (u32) Delay, 0);
}

/** Get the trigger delay of a SilenceDetector control
* \param phSubSys subsystem handle \param hControl silence detector handle
* \param *Delay see HPI_SilenceDetector_SetDelay()
* \return_hpierr
*/
u16 HPI_SilenceDetector_GetDelay(HPI_HSUBSYS * phSubSys, HPI_HCONTROL hControl,
				 u32 * Delay)
{
	return HPI_ControlParamGet(phSubSys, hControl,
				   HPI_SILENCEDETECTOR_DELAY, 0, 0,
				   (u32 *) Delay, NULL);
}

/** Set the Threshold of a SilenceDetector control
* \param phSubSys subsystem handle \param hControl silence detector handle
* \param Threshold in millibels wrt full scale.  E.g. -4000 -> -40dBFS threshold.
* \return_hpierr
*/
u16 HPI_SilenceDetector_SetThreshold(HPI_HSUBSYS * phSubSys,
				     HPI_HCONTROL hControl, int Threshold)
{
	return HPI_ControlParamSet(phSubSys, hControl,
				   HPI_SILENCEDETECTOR_THRESHOLD,
				   (u32) Threshold, 0);
}

/** Get the Threshold of a SilenceDetector control
* \param phSubSys subsystem handle \param hControl silence detector handle
* \param Threshold see HPI_SilenceDetector_SetThreshold()
* \return_hpierr
*/
u16 HPI_SilenceDetector_GetThreshold(HPI_HSUBSYS * phSubSys,
				     HPI_HCONTROL hControl, int *Threshold)
{
	return HPI_ControlParamGet(phSubSys, hControl,
				   HPI_SILENCEDETECTOR_THRESHOLD, 0, 0,
				   (u32 *) Threshold, NULL);
}

/**\}*/

///////////////////////////////////////////////////////////////////////////////
/** \defgroup tuner Tuner Controls

The tuner control sets the band and frequency of a tuner, and measures the RF level.

\image html tuner.png

\{
*/

/** Set the band that the tuner recieves.
*
* Not all tuners support all bands, e.g. AM+FM or TV+FM.
*
* \note That with the exception of #HPI_TUNER_BAND_AUX, the tuner frequency must subsequently
* be set using HPI_Tuner_SetFrequency().
* \note Plase see \sa HPI_ControlQuery() for details on determining the bands supported by a particular tuner.
* \return_hpierr
*/
u16 HPI_Tuner_SetBand(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
		      HPI_HCONTROL hControlHandle,	///< Handle to tuner control.
		      u16 wBand	///< One of the supported HPI_TUNER_BAND_XXX \ref tuner_bands.
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle, HPI_TUNER_BAND,
				   wBand, 0);
}

/** Set the RF attenuator gain of the tuner front end.
* \return_hpierr
*/
u16 HPI_Tuner_SetGain(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
		      HPI_HCONTROL hControlHandle,	///< Handle to tuner control.
		      short nGain	///< Valid values depend on the adapter type. For the ASI8700: 0dB or -20 x HPI_UNITS_PER_dB.
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle, HPI_TUNER_GAIN,
				   nGain, 0);
}

/** Set the tuner frequency.
*
* \note See HPI_ControlQuery() to determine how to find the frequency supported by a particular tuner band.
* \return_hpierr
*/
u16 HPI_Tuner_SetFrequency(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			   HPI_HCONTROL hControlHandle,	///< HPI subsystem handle.
			   u32 wFreqInkHz	///< Tuner frequncy in kHz. Valid values depend on the tuner band setting.
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle, HPI_TUNER_FREQ,
				   wFreqInkHz, 0);
}

/** Get the current tuner band.
* \return_hpierr
*/
u16 HPI_Tuner_GetBand(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
		      HPI_HCONTROL hControlHandle,	///< Handle to tuner control.
		      u16 * pwBand	///< Current tuner band - one of \ref tuner_bands.
    )
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

/** Get the current tuner gain
* \return_hpierr
*/
u16 HPI_Tuner_GetGain(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
		      HPI_HCONTROL hControlHandle,	///< Handle to tuner control.
		      short *pnGain	///< Current tuner gain in milliBels
    )
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

/** Get the current tuner frequency.
* \return_hpierr
*/
u16 HPI_Tuner_GetFrequency(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			   HPI_HCONTROL hControlHandle,	///< Handle to tuner control.
			   u32 * pwFreqInkHz	///< Returned tuner frequency in kHz.
    )
{
	return HPI_ControlParam1Get(phSubSys, hControlHandle, HPI_TUNER_FREQ,
				    pwFreqInkHz);
}

/** Get the RF level of a tuner input in millibel microvolts.
* Divide the return value by HPI_UNITS_PER_dB to get the level in dBuV.
* This function only applies to certain bands on certain tuners.

<table>
<tr><td>Tuner Type </td>         <td>Raw RF Level values</td>    <td>Comments</td></tr>
<tr><td>MT4039 (TV/FM)</td>      <td>1..4</td>                   <td>Only present in FM mode</td></tr>
<tr><td>MT1384 (AM</FM)</td>     <td>0..255</td>                 <td>.</td></tr>
<tr><td>Si4703 (FM)</td>      <td>Not supported</td>             <td>.</td></tr>
</table>

* \return_hpierr
*/
u16 HPI_Tuner_GetRFLevel(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			 HPI_HCONTROL hControlHandle,	///< Handle to tuner control.
			 short *pwLevel	///< Return level. The units are mBuV  (mB micro volts). Range is +/- 100 dBuV
    )
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

/** Get the RF raw level of a tuner. This is a "raw" value and it will depend
* on the type of tuner being accessed. This function only applies to certain bands on certain tuners.
* \b ASI87xx - Supports this function.<br>
* <b> ASI89xx with ASI1711 tuner </b> - Does not support this function. It will return #HPI_ERROR_INVALID_CONTROL_ATTRIBUTE.
* \return_hpierr
*/
u16 HPI_Tuner_GetRawRFLevel(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
			    HPI_HCONTROL hControlHandle,	///< Handle to tuner control.
			    short *pwLevel	///< The units of this depend on the tuner type. This is the raw level reading that the tuner returns.
    )
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

/** Get the status of various Boolean attributes of a tuner control.
* The pwStatusMask returns which bits in wStatus are valid, as not all
* tuners support all the status attributes.
* \param phSubSys HPI subsystem handle.
* \param hControlHandle Handle to tuner control.
* \param *pwStatusMask A returned bitfield indicating which of the bits in pwStatus
* contain valid status information. Valid bits are:<br>
* #HPI_TUNER_VIDEO_COLOR_PRESENT <br>
* #HPI_TUNER_VIDEO_HORZ_SYNC_MISSING <br>
* #HPI_TUNER_VIDEO_IS_60HZ <br>
* #HPI_TUNER_PLL_LOCKED <br>
* #HPI_TUNER_FM_STEREO
* \param *pwStatus Status bitfield containing the following bits:<br>
* #HPI_TUNER_VIDEO_COLOR_PRESENT <br>
* #HPI_TUNER_VIDEO_HORZ_SYNC_MISSING <br>
* #HPI_TUNER_VIDEO_IS_60HZ <br>
* #HPI_TUNER_PLL_LOCKED <br>
* #HPI_TUNER_FM_STEREO
* \return_hpierr
*/
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

/** This function turns off the RSS (FM FR level reading) capability for the specified tuner.
* This only applies to certain bands on certain tuners.
* \b ASI87xx - Supports this function.<br>
* <b> ASI89xx with ASI1711 tuner </b> - Does not support this function. It will return #HPI_ERROR_INVALID_CONTROL_ATTRIBUTE.
* \return_hpierr
*/
u16 HPI_Tuner_SetMode(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
		      HPI_HCONTROL hControlHandle,	///< Handle to tuner control.
		      u32 nMode,	///< Currently only supports HPI_TUNER_MODE_RSS.
		      u32 nValue	///< Should be set to either HPI_TUNER_MODE_RSS_DISABLE or HPI_TUNER_MODE_RSS_ENABLE.
    )
{
	return HPI_ControlParamSet(phSubSys, hControlHandle, HPI_TUNER_MODE,
				   nMode, nValue);
}

/** Get the current tuner mode. Currently supoprts checking whether RSS is enabled or disabled.
* There are some dependancies across adapters for this function.<br>
* \b ASI87xx - Supports this function.<br>
* <b> ASI89xx with ASI1711 tuner </b> - RSS is always enabled. This function will return #HPI_ERROR_INVALID_CONTROL_ATTRIBUTE.
*
* \return_hpierr
*/
u16 HPI_Tuner_GetMode(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
		      HPI_HCONTROL hControlHandle,	///< Handle to tuner control.
		      u32 nMode,	///< Currently only supports #HPI_TUNER_MODE_RSS.
		      u32 * pnValue	///< Returned value is either #HPI_TUNER_MODE_RSS_DISABLE or #HPI_TUNER_MODE_RSS_ENABLE.
    )
{
	return HPI_ControlParamGet(phSubSys, hControlHandle, HPI_TUNER_MODE,
				   nMode, 0, pnValue, NULL);
}

/** Get tuner RDS data. Returns RDS data if there is any.
* \b ASI87xx - Does not support this function.<br>
* <b> ASI89xx with ASI1711 tuner </b> - Does support this function.
* \return_hpierr
* \retval HPI_ERROR_BUFFER_EMPTY if the RDS buffer is now empty.
*/
u16 HPI_Tuner_GetRDS(HPI_HSUBSYS * phSubSys,	///< HPI subsystem handle.
		     HPI_HCONTROL hControlHandle,	///< Handle to tuner control.
		     char *pData	///< pointer to 12 element array for returned RDS data
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_TUNER_RDS;
	HPI_Message(&hm, &hr);
	if (pData) {
		*(u32 *) & pData[0] = hr.u.cu.tuner.rds.dwData[0];
		*(u32 *) & pData[4] = hr.u.cu.tuner.rds.dwData[1];
		*(u32 *) & pData[8] = hr.u.cu.tuner.rds.dwBLER;
	}
	return (hr.wError);
}

/**\}*/
/////////////////////////////////////////////////////////////////////////
/** \defgroup volume Volume Control
Volume controls are usually situated on a "connection" between a source node and a destination node.
They can also be present on source nodes, such as Ostream or LineIn. They control the gain/attenuation
of the signal passing through them.

For example if you have a -10dB (relative to digital full-scale) signal passing through a volume control
with a gain set to -6dB, then the output signal would be -16dB.

The units of gain parameters are milliBels (mB), equivalent to 1/1000 of a Bel, or 1/100 of a decibel.
For example a gain of -14.00dB is represented as -1400.

Gain parameters are stereo, stored in a 2 element array, element 0 is the left channel and element 1 is
the right channel.

A gain value of HPI_GAIN_OFF will set the gain to its maximum attenuation or mute if the adapter supports it.

While most volume controls are attenuation only, some volume controls support gain as well.
This is adapter and control dependant.  Use the HPI_VolumeGetRange() function to determine if the control
supports gain.

\{
*/
/** Set the gain of a volume control.

Setting the gain of a volume control has the side effect that any autofades currently underway
are terminated. The volume would become that of the current Set command.

\return_hpierr
*/
u16 HPI_VolumeSetGain(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
		      HPI_HCONTROL hControlHandle,	///< Handle to volume control.
		      short anLogGain[HPI_MAX_CHANNELS]	///< Gain in 100ths of a dB.
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

/** Gets the current gain of a volume control.
* \return_hpierr
*/
u16 HPI_VolumeGetGain(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
		      HPI_HCONTROL hControlHandle,	///< Handle to volume control.
		      short anLogGain[HPI_MAX_CHANNELS]	///< Gain in 100ths of a dB. If an autofade is in progess, it will be reflected here.
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

/** Query the range of a volume control. Gets the max,min and step of the specified volume control.
* \return_hpierr
*/
u16 HPI_VolumeQueryRange(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			 HPI_HCONTROL hControlHandle,	///< Handle to volume control.
			 short *nMinGain_01dB,	///< Minimum gain setting in 100ths of a dB.
			 short *nMaxGain_01dB,	///< Maximum gain setting in 100ths of a dB.
			 short *nStepGain_01dB	///< Step size in 100ths of a dB.
    )
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

/** Starts an automatic ramp of the volume control from the current gain setting to
*     the specified setting over the specified duration (in milliseconds).
* The gain starts at the current gain value and fades up/down to anStopGain0_01dB[]
* over the specified duration.
*
*     The fade profile can be either log or linear.
*
* When wProfile==HPI_VOLUME_AUTOFADE_LOG the gain in dB changes linearly over time.
*
* When wProfile==HPI_VOLUME_AUTOFADE_LINEAR the gain multiplier changes linearly over
* time. For example half way through the fade time of a fade from 0dB (100%) to -100dB
* (approx 0%) the gain will be -6dB (50%).
*
* \image html volume_fade_profile.png
*
* \return_hpierr
*
*/

u16 HPI_VolumeAutoFadeProfile(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			      HPI_HCONTROL hControlHandle,	///< Handle to volume control.
			      short anStopGain0_01dB[HPI_MAX_CHANNELS],	///< End point of the fade in 0.01ths of a dB.
			      u32 dwDurationMs,
			      /**< Duration of fade in milliseconds. Minimum duration is 20 ms.
Maximum duration is 100 seconds or 100 000 ms. Durations outside this
range will be converted to the nearest limit.
*/
			      u16 wProfile
				      /**< The profile, or shape of the autofade curve.
Allowed values are #HPI_VOLUME_AUTOFADE_LOG or #HPI_VOLUME_AUTOFADE_LINEAR.
*/
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

/** \deprecated See HPI_VolumeAutoFadeProfile().
*/
u16 HPI_VolumeAutoFade(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
		       HPI_HCONTROL hControlHandle,	///< Handle to a volume control.
		       short anStopGain0_01dB[HPI_MAX_CHANNELS],	///< The endpoint of the fade.
		       u32 dwDurationMs	///< Duration of fade in milliseconds.
    )
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
* Sets the threshold of a VOX control.  Note the threshold is in units of 0.01dB.
* The threshold will typically range between 0 and -100dB.
* On startup the VOX control is set to -100 dB.
* \return_hpierr
*/
u16 HPI_VoxSetThreshold(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			HPI_HCONTROL hControlHandle,	///< Handle to a VOX control.
			short anGain0_01dB	///< Trigger level in 100ths of a dB. For example a gain of -1400 is -14.00dB.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_CONTROL, HPI_CONTROL_SET_STATE);
	HPI_HANDLETOINDEXES(hControlHandle, &hm.wAdapterIndex,
			    &hm.u.c.wControlIndex);
	hm.u.c.wAttribute = HPI_VOX_THRESHOLD;

	hm.u.c.anLogValue[0] = anGain0_01dB;	// only use the first index (LEFT)

	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/**
* Gets the current threshold of a VOX control. Note the threshold is in units of 0.01dB.
* The threshold will typically range between 0 and -100dB.
* On startup the VOX control is set to -100 dB.
* \return_hpierr
*/
u16 HPI_VoxGetThreshold(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			HPI_HCONTROL hControlHandle,	///< Handle to a VOX control.
			short *anGain0_01dB	///< Returned trigger level in 100ths of a dB. For example a gain of -1400 is -14.00dB.
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

	*anGain0_01dB = hr.u.c.anLogValue[0];	// only use the first index (LEFT)

	return (hr.wError);
}

/** @}*/
	  /** @} */// group mixer

////////////////////////////////////////////////////////////////////////////
/**\defgroup gpio GPIO
The GPIO object on an adapter reperesents a number of input bits
that may be individually sensed and a number of digital output bits that
may be individually set.

There is at most one GPIO object per adapter.

On an adapter such as an ASI4346, the bit outputs control relay closurers.
HPI_GpioWriteBit() can be used to set the state of each of the relays.
Similarly, the inputs on the ASI4346 are mapped 1 to 1 to opto isolated inputs.
\{
*/

/**  Opens the GPIO on a particular adapter for reading and writing.
It returns a handle to the GPIO object (hGpio) and the number of input
and output bits (*pwNumberInputBits,*pwNumberOutputBits).
If the adapter does not have any GPIO functionality, the function will return an error.
* \return_hpierr
*/
u16 HPI_GpioOpen(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
		 u16 wAdapterIndex,	///< Get gpio handle for this adapter
		 HPI_HGPIO * phGpio,	///< Handle to GPIO object
		 u16 * pwNumberInputBits,	///< Number of GPIO inputs
		 u16 * pwNumberOutputBits	///< Number of GPIO outputs
    )
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

/** read a particular bit from an adapters digital input port
* \return_hpierr
*/
u16 HPI_GpioReadBit(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
		    HPI_HGPIO hGpio,	///< Handle to GPIO object
		    u16 wBitIndex,	///< An index which addresses one of the input bits
		    u16 * pwBitData	///< The state of the input.  A "1" means the input has been set.
    )
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

/** read all bits from an adapters GPIO input port (upto 16)
* \return_hpierr
*/
u16 HPI_GpioReadAllBits(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
			HPI_HGPIO hGpio,	///< Handle to GPIO object
			u16 * pwBitData	///< The input states.  Bit 0 refers to the 1st GPIO input
    )
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

/** write a particular bit to an adapters digital output port
* \return_hpierr
*/
u16 HPI_GpioWriteBit(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
		     HPI_HGPIO hGpio,	///< Handle to GPIO object
		     u16 wBitIndex,	///< An index which addresses one of the input bits
		     u16 wBitData	///< The state to set the output to.  A "1" turns the output on.
    )
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
\return_hpierr
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
\return_hpierr
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
\return_hpierr
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
\return_hpierr
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
\return_hpierr
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
/**
Opens the non-volatile memory on a particular adapter for reading and writing.
It takes as input the handle to the subsytem (phSubSys) and the adapter index
(wAdapterIndex) and returns a handle to the non-volatile memory (hNvMemory)
and the size of the memory in bytes (wSizeInBytes). If the adapter does not
have any non-volatile memory, the function will return an error.
* \return_hpierr
*/

u16 HPI_NvMemoryOpen(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
		     u16 wAdapterIndex,	///< Get nvmemory handle on this adapter
		     HPI_HNVMEMORY * phNvMemory,	///< Handle to an HPI_NVMEMORY object
		     u16 * pwSizeInBytes	///< size of the nv memory in bytes
    )
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

/**
Reads a byte from an adapters non-volatile memory. The input is a handle to
the non-volatile memory (hNvMemory - returned from HPI_NvMemoryOpen() )
and an index which addresses one of the bytes in the memory (wIndex).
The index may range from 0 to SizeInBytes-1 (returned by HPI_NvMemoryOpen() ).
The byte is returned in *pwData. ). An error return of HPI_ERROR_NVMEM_BUSY
indicates that an attempt to access the NvMem was made before the previous
operation has completed. The call should be re-tried.
* \return_hpierr
*/

u16 HPI_NvMemoryReadByte(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
			 HPI_HNVMEMORY hNvMemory,	///< Handle to an HPI_NVMEMORY object
			 u16 wIndex,	///< An Index that may range from 0 to SizeInBytes-1 (returned by HPI_NvMemoryOpen() )
			 u16 * pwData	///< Returned data byte
    )
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

/**
Writes a byte to an adapters non-volatile memory. The input is a handle to
the non-volatile memory ( hNvMemory - returned from HPI_NvMemoryOpen() ),
an index which addresses one of the bytes in the memory (wIndex) and the
data to write (wData). The index may range from 0 to SizeInBytes-1 (returned
by HPI_NvMemoryOpen() ). An error return of HPI_ERROR_NVMEM_BUSY indicates
that an attempt to access the NvMem was made before the previous operation
has completed. The call should be re-tried.
\return_hpierr
*/
u16 HPI_NvMemoryWriteByte(HPI_HSUBSYS * phSubSys,	///< Subsystem handle
			  HPI_HNVMEMORY hNvMemory,	///< Handle to an HPI_NVMEMORY object
			  u16 wIndex,	///< An Index that may range from 0 to SizeInBytes-1 (returned by HPI_NvMemoryOpen() )
			  u16 wData	///> Byte of data to write
    )
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
Comments in AXPROF.H describe the DSP side of the profiling operation. In general
this set of functions is intended for AudioScience internel use.
@{
*/
/** Open all the profiles on a particular adapter.

If the adapter does not have profiling enabled, the function will return an error.

The complete profile set of all profiles can be thought of as any array of execution timings/profiles.
Each indexed profile corresponds to the execution of a particular segment of DSP code.

Note that HPI_ProfileStartAll() must be called after HPI_ProfileOpenAll() to start the profiling operation on the DSP.

\return_hpierr
*/
u16 HPI_ProfileOpenAll(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
		       u16 wAdapterIndex,	///< Adapter index.
		       u16 wProfileIndex,	///< Corresponds to DSP index.
		       HPI_HPROFILE * phProfile,	///< Returned profile handle.
		       u16 * pwMaxProfiles	///< Returned maximum number of profile bins supported.
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

/** Reads a single profile from the DSP's profile store.
The input is a handle to the profiles (hProfiles - returned from HPI_ProfileOpenAll() )
and an index that addresses one of the profiles (wIndex).  The index may range from 0 to
wMaxProfiles  (returned by HPI_ProfileOpenAll() ). The return parameters describe the execution of
the profiled section of DSP code.

\return_hpierr
*/
u16 HPI_ProfileGet(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
		   HPI_HPROFILE hProfile,	///< Handle of profile object.
		   u16 wIndex,	///< Index of the profile to retrieve.
		   u16 * pwSeconds,	///< Returned number of seconds spent executing the profiled code.
		   u32 * pdwMicroSeconds,	///< Returned fractional seconds spent executing the profiled code (measured in \f$\mu s\f$, range 0-999,999).
		   u32 * pdwCallCount,	///< Returned number of times the profiled code was executed.
		   u32 * pdwMaxMicroSeconds,	///< Returned maximum \f$\mu s\f$ spent executing the profiled code (range 0-8,388,608, or  8 s).
		   u32 * pdwMinMicroSeconds	///< Returned minimum \f$\mu s\f$ spent executing the profiled code (range 0-8,388,608, or  8 s).
    )
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

/** Get the DSP utilization in 1/100 of a percent.
\return_hpierr
*/
u16 HPI_ProfileGetUtilization(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			      HPI_HPROFILE hProfile,	///<  Handle of profile object.
			      u32 * pdwUtilization	///< Returned DSP utilization in 100ths of a percent.
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

/** Get the name of a profile.
A typical adapter can support multiple "named" profiles simultaneously.
This function allows an application (or GUI) to read the names of the
profiles from the DSP so as to correctly label the returned timing information.
\return_hpierr
*/
u16 HPI_ProfileGetName(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
		       HPI_HPROFILE hProfile,	///< Handle of profile object.
		       u16 wIndex,	///< Index of the profile to retrieve the name of.
		       char *szName,	///< Pointer to a string that will have the name returned in it.
		       u16 nNameLength	///< Length of the szName string.
    )
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

/** Start profiling running.
This starts profile counters and timers running. It is up to the user
to periodically call HPI_ProfileGet() to retrieve timing information.
\return_hpierr
*/
u16 HPI_ProfileStartAll(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
			HPI_HPROFILE hProfile	///<  Handle of profile object.
    )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_UNUSED(phSubSys);
	HPI_InitMessage(&hm, HPI_OBJ_PROFILE, HPI_PROFILE_START_ALL);
	HPI_HANDLETOINDEXES(hProfile, &hm.wAdapterIndex, &hm.wDspIndex);
	HPI_Message(&hm, &hr);

	return (hr.wError);
}

/** Stop profiling.
When profiling is stopped counters are no longer updated.
\return_hpierr
*/
u16 HPI_ProfileStopAll(HPI_HSUBSYS * phSubSys,	///< Pointer to HPI subsystem handle.
		       HPI_HPROFILE hProfile	///<  Handle of profile object.
    )
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
// Hide this section from Doxygen since it is not implemented yet.
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

/** Convert one of the \ref errorcodes into a string
* @param wError the error code
* @param pszErrorText pointer to callers buffer. Must be at least 200 bytes!
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
	case HPI_ERROR_MEMORY_ALLOC:	//208
		strcat(pszErrorText, "could not allocate memory");
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

	case HPI_ERROR_MUTEX_TIMEOUT:
		strcat(pszErrorText, "Mutex timeout");

	default:
		strcat(pszErrorText, "Unknown Error");
	}
}

/** Initialize an audio format structure, given various defining parameters
* \return_hpierr
*/
u16 HPI_FormatCreate(HPI_FORMAT * pFormat,	///< Pointer to the format structure to be initialized.
		     u16 wChannels,	///< From 1 to 8.
		     u16 wFormat,	///< One of the \ref formats
		     u32 dwSampleRate,	///< Sample rate in Hz. Samplerate must be between 8000 and 200000.
		     u32 dwBitRate,	///< Bits per second, must be supplied for MPEG formats, it is calculated for PCM formats.
		     u32 dwAttributes	///< Format dependent attributes.  E.g. \ref mpegmodes
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

// The actual message size for each object type
static u16 aMsgSize[HPI_OBJ_MAXINDEX + 1] = HPI_MESSAGE_SIZE_BY_OBJECT;
// The actual response size for each object type
static u16 aResSize[HPI_OBJ_MAXINDEX + 1] = HPI_RESPONSE_SIZE_BY_OBJECT;

// initialize the HPI message structure
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

// initialize the HPI response structure
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

/** Encode 3 indices and an object type into a 32 bit handle.
\internal
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
