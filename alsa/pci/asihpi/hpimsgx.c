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

Extended Message Function With Response Cacheing

(C) Copyright AudioScience Inc. 2002
*****************************************************************************/
#include "hpimsgx.h"
#include "hpidebug.h"

void hpi_message(const HPI_HSUBSYS * psubsys, HPI_MESSAGE * phm,
		 HPI_RESPONSE * phr);
#define HPI_Message hpi_message
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

u32 HPI_IndexesToHandle(const char cObject, const u16 wIndex1,
			const u16 wIndex2);
u32 HPI_IndexesToHandle3(const char cObject, const u16 wIndex1,
			 const u16 wIndex2, const u16 wIndex0);
u16 HPI_AdapterFindObject(const HPI_HSUBSYS * phSubSys, u16 wAdapterIndex,
			  u16 wObjectType, u16 wObjectIndex, u16 * pDspIndex);

typedef struct {
	u16 wSize;
	u16 wType;		/* HPI_MSG_MESSAGE, HPI_MSG_RESPONSE */
	u16 wObject;		/* HPI_OBJ_SUBSYS, _OBJ_ADAPTER, _OBJ_DEVICE, _OBJ_MIXER */
	u16 wFunction;		/* HPI_SUBSYS_xxx, HPI_ADAPTER_xxx */
	u16 wAdapterIndex;	/* the adapter index */
	u16 wDspIndex;		/* the dsp index on the adapter */
} HPI_MESSAGE_HEADER;

typedef struct {
	u16 wSize;
	u16 wType;		/* HPI_MSG_MESSAGE, HPI_MSG_RESPONSE */
	u16 wObject;		/* HPI_OBJ_SUBSYS, _OBJ_ADAPTER, _OBJ_DEVICE, _OBJ_MIXER */
	u16 wFunction;		/* HPI_SUBSYS_xxx, HPI_ADAPTER_xxx */
	u16 wError;		/* HPI_ERROR_xxx */
	u16 wSpecificError;	/* Adapter specific error */
} HPI_RESPONSE_HEADER;

typedef struct {
	HPI_RESPONSE_HEADER h;
	HPI_SUBSYS_RES s;
} HPI_SUBSYS_RESPONSE;

typedef struct {
	HPI_RESPONSE_HEADER h;
	HPI_ADAPTER_RES a;
} HPI_ADAPTER_RESPONSE;

typedef struct {
	HPI_RESPONSE_HEADER h;
	HPI_MIXER_RES m;
} HPI_MIXER_RESPONSE;

typedef struct {
	HPI_RESPONSE_HEADER h;
	HPI_STREAM_RES d;
} HPI_STREAM_RESPONSE;

//////////////////////////////////////////////////////////////////////////////////
// Globals
static HPI_ADAPTER_RESPONSE aRESP_HPI_ADAPTER_OPEN[HPI_MAX_ADAPTERS];
static HPI_STREAM_RESPONSE
    aRESP_HPI_OSTREAM_OPEN[HPI_MAX_ADAPTERS][HPI_MAX_STREAMS];
static HPI_STREAM_RESPONSE
    aRESP_HPI_ISTREAM_OPEN[HPI_MAX_ADAPTERS][HPI_MAX_STREAMS];
static HPI_MIXER_RESPONSE aRESP_HPI_MIXER_OPEN[HPI_MAX_ADAPTERS];
static HPI_SUBSYS_RESPONSE gRESP_HPI_SUBSYS_FIND_ADAPTERS;

typedef struct {
	int nOpenFlag;
	void *hOwner;
	u16 wDspIndex;
} ASI_OPEN_STATE;

// use these to keep track of opens from user mode apps/DLLs
static ASI_OPEN_STATE aOStreamUserOpen[HPI_MAX_ADAPTERS][HPI_MAX_STREAMS];
static ASI_OPEN_STATE aIStreamUserOpen[HPI_MAX_ADAPTERS][HPI_MAX_STREAMS];

void HPIMSGX_MessageExAdapterReset(u16 wAdapterIndex)
{
	int i;
	u16 wAdapter;

	if (wAdapterIndex == HPIMSGX_ALLADAPTERS) {
		memset(&gRESP_HPI_SUBSYS_FIND_ADAPTERS, 0, sizeof(gRESP_HPI_SUBSYS_FIND_ADAPTERS));	// zero out response

// reset all responses to contain errrors
		for (wAdapter = 0; wAdapter < HPI_MAX_ADAPTERS; wAdapter++) {
			aRESP_HPI_ADAPTER_OPEN[wAdapter].h.wError =
			    HPI_ERROR_BAD_ADAPTER;
			aRESP_HPI_MIXER_OPEN[wAdapter].h.wError =
			    HPI_ERROR_INVALID_OBJ;
			for (i = 0; i < HPI_MAX_STREAMS; i++) {
				aRESP_HPI_OSTREAM_OPEN[wAdapter][i].h.wError =
				    HPI_ERROR_INVALID_OBJ;
				aRESP_HPI_ISTREAM_OPEN[wAdapter][i].h.wError =
				    HPI_ERROR_INVALID_OBJ;
			}
		}
	} else if (wAdapterIndex < HPI_MAX_ADAPTERS) {
		aRESP_HPI_ADAPTER_OPEN[wAdapterIndex].h.wError =
		    HPI_ERROR_BAD_ADAPTER;
		aRESP_HPI_MIXER_OPEN[wAdapterIndex].h.wError =
		    HPI_ERROR_INVALID_OBJ;
		for (i = 0; i < HPI_MAX_STREAMS; i++) {
			aRESP_HPI_OSTREAM_OPEN[wAdapterIndex][i].h.wError =
			    HPI_ERROR_INVALID_OBJ;
			aRESP_HPI_ISTREAM_OPEN[wAdapterIndex][i].h.wError =
			    HPI_ERROR_INVALID_OBJ;
		}
		if (gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.
		    awAdapterList[wAdapterIndex]) {
			gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.
			    awAdapterList[wAdapterIndex] = 0;
			gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.wNumAdapters--;
		}
	}
}

u16 HPIMSGX_MessageExAdapterInit(const HPI_HSUBSYS * phSubSysHandle, HPI_MESSAGE * phm,	// HPI_SUBSYS_CREATE_ADAPTER structure with
// resource list or NULL=find all
				 HPI_RESPONSE * phr	// response from HPI_ADAPTER_GET_INFO
    )
{
	u16 wAdapter, wFoundAdapter = 0;
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	u16 isPnP;

	if (phm)		// If PnP OS finds an adapter and add it
	{
		isPnP = TRUE;

		if (gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.wNumAdapters >=
		    HPI_MAX_ADAPTERS)
			return HPI_ERROR_BAD_ADAPTER_NUMBER;

// Init response here so we can pass in previous adapter list
		HPI_InitResponse(&hr, phm->wObject, phm->wFunction,
				 HPI_ERROR_INVALID_OBJ);
		memcpy(hr.u.s.awAdapterList,
		       gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.awAdapterList,
		       sizeof(gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.awAdapterList));
		HPI_Message(phSubSysHandle, phm, &hr);	//** HPI_SUBSYS_CREATE_ADAPTER call
		if (hr.wError != 0) {
			return hr.wError;
		}

		wFoundAdapter = hr.u.s.wAdapterIndex;
	} else			// Else we need to find all adapters
	{
		isPnP = FALSE;

// call to HPI_SUBSYS_FIND_ADAPTERS
		HPI_InitMessage(&hm, HPI_OBJ_SUBSYSTEM,
				HPI_SUBSYS_FIND_ADAPTERS);
		HPI_Message(phSubSysHandle, &hm, &hr);
		memcpy(&gRESP_HPI_SUBSYS_FIND_ADAPTERS, &hr,
		       sizeof(gRESP_HPI_SUBSYS_FIND_ADAPTERS));

		if ((hr.wError) && (hr.u.s.wNumAdapters == 0))
			return hr.wError;

		if (gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.wNumAdapters >
		    HPI_MAX_ADAPTERS)
			gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.wNumAdapters =
			    HPI_MAX_ADAPTERS;
	}

// Open the adapter and streams
	for (wAdapter = (isPnP ? wFoundAdapter : 0);
	     wAdapter < HPI_MAX_ADAPTERS; wAdapter++) {
		u16 wNumInStreams, wNumOutStreams, i;

		if (gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.awAdapterList[wAdapter] ==
		    0 && !isPnP)
			continue;

// call to HPI_ADAPTER_OPEN
		HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_OPEN);
		hm.wAdapterIndex = wAdapter;
		HPI_Message(phSubSysHandle, &hm, &hr);
		wNumOutStreams = hr.u.a.wNumOStreams;
		wNumInStreams = hr.u.a.wNumIStreams;
		memcpy(&aRESP_HPI_ADAPTER_OPEN[wAdapter], &hr,
		       sizeof(aRESP_HPI_ADAPTER_OPEN[0]));
		if (hr.wError) {
			if (!isPnP)
				continue;
			else
				return hr.wError;
		}
// call to HPI_ADAPTER_GET_INFO
		HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_GET_INFO);
		hm.wAdapterIndex = wAdapter;
		HPI_Message(phSubSysHandle, &hm, &hr);
		wNumOutStreams = hr.u.a.wNumOStreams;
		wNumInStreams = hr.u.a.wNumIStreams;

		if (isPnP) {
			memcpy(phr, &hr, hr.wSize);
			gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.
			    awAdapterList[wAdapter] = hr.u.a.wAdapterType;
			gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.wNumAdapters++;
			if (gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.wNumAdapters >
			    HPI_MAX_ADAPTERS)
				gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.wNumAdapters =
				    HPI_MAX_ADAPTERS;
		}
// call to HPI_OSTREAM_OPEN
		for (i = 0; i < wNumOutStreams; i++) {
			HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_OPEN);
			hm.wAdapterIndex = wAdapter;
			hm.u.d.wOStreamIndex = i;
			HPI_Message(phSubSysHandle, &hm, &hr);
			memcpy(&aRESP_HPI_OSTREAM_OPEN[wAdapter][i], &hr,
			       sizeof(aRESP_HPI_OSTREAM_OPEN[0][0]));
			aOStreamUserOpen[wAdapter][i].nOpenFlag = 0;
			aOStreamUserOpen[wAdapter][i].hOwner = 0;
		}

// call to HPI_ISTREAM_OPEN
		for (i = 0; i < wNumInStreams; i++) {
			HPI_AdapterFindObject(phSubSysHandle, wAdapter,
					      HPI_OBJ_ISTREAM, i,
					      &aIStreamUserOpen[wAdapter][i].
					      wDspIndex);
			HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_OPEN);
			hm.wAdapterIndex = wAdapter;
			hm.u.d.wIStreamIndex = i;
			hm.wDspIndex = aIStreamUserOpen[wAdapter][i].wDspIndex;
			HPI_Message(phSubSysHandle, &hm, &hr);
			memcpy(&aRESP_HPI_ISTREAM_OPEN[wAdapter][i], &hr,
			       sizeof(aRESP_HPI_ISTREAM_OPEN[0][0]));
			aIStreamUserOpen[wAdapter][i].nOpenFlag = 0;
			aIStreamUserOpen[wAdapter][i].hOwner = 0;
		}

// call to HPI_MIXER_OPEN
		HPI_InitMessage(&hm, HPI_OBJ_MIXER, HPI_MIXER_OPEN);
		hm.wAdapterIndex = wAdapter;
		HPI_Message(phSubSysHandle, &hm, &hr);
		memcpy(&aRESP_HPI_MIXER_OPEN[wAdapter], &hr,
		       sizeof(aRESP_HPI_MIXER_OPEN[0]));

		if (isPnP)
			break;
	}
	return gRESP_HPI_SUBSYS_FIND_ADAPTERS.h.wError;
}

void HPIMSGX_MessageEx(const HPI_HSUBSYS * phSubSys,
		       HPI_MESSAGE * phm, HPI_RESPONSE * phr, void *hOwner)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

	memcpy(&hm, phm, sizeof(HPI_MESSAGE));
	memset(&hr, 0, sizeof(HPI_RESPONSE));

	HPI_DEBUG_MESSAGE(phm);

// test for adapter index that might produce a BSOD
	if (hm.wAdapterIndex >= HPI_MAX_ADAPTERS) {
// send dummy response back to user mode
		memcpy(phr, &aRESP_HPI_ADAPTER_OPEN[0],
		       sizeof(aRESP_HPI_ADAPTER_OPEN[0]));

// flag an error
		phr->wError = HPI_ERROR_BAD_ADAPTER_NUMBER;
	}
// test for stream index that might produce a BSOD
	else if ((((hm.wFunction == HPI_OSTREAM_OPEN) ||
		   (hm.wFunction == HPI_OSTREAM_CLOSE)) &&
		  (hm.u.d.wOStreamIndex >= HPI_MAX_STREAMS))
		 ||
		 (((hm.wFunction == HPI_ISTREAM_OPEN) ||
		   (hm.wFunction == HPI_ISTREAM_CLOSE)) &&
		  (hm.u.d.wIStreamIndex >= HPI_MAX_STREAMS))
	    ) {
// send dummy response back to user mode
		memcpy(phr, &aRESP_HPI_ADAPTER_OPEN[0],
		       sizeof(aRESP_HPI_ADAPTER_OPEN[0]));

// flag an error
		phr->wError = HPI_ERROR_INVALID_STREAM;
	} else {
		switch (hm.wFunction) {

		case HPI_SUBSYS_OPEN:
			hr.wSize = sizeof(HPI_SUBSYS_RESPONSE);
			hr.wType = HPI_TYPE_RESPONSE;
			hr.wObject = hm.wObject;
			hr.wFunction = hm.wFunction;
			hr.wError = 0;
			memcpy(phr, &hr, hr.wSize);
			break;
		case HPI_SUBSYS_CLOSE:
			hr.wSize = sizeof(HPI_SUBSYS_RESPONSE);
			hr.wType = HPI_TYPE_RESPONSE;
			hr.wObject = hm.wObject;
			hr.wFunction = hm.wFunction;
			hr.wError = 0;
			memcpy(phr, &hr, hr.wSize);
			HPIMSGX_MessageExAdapterCleanup(HPIMSGX_ALLADAPTERS,
							hOwner);
			break;
		case HPI_SUBSYS_FIND_ADAPTERS:
			memcpy(phr, &gRESP_HPI_SUBSYS_FIND_ADAPTERS,
			       sizeof(gRESP_HPI_SUBSYS_FIND_ADAPTERS));
			break;
		case HPI_ADAPTER_OPEN:
// send response back to user mode
			memcpy(phr, &aRESP_HPI_ADAPTER_OPEN[hm.wAdapterIndex],
			       sizeof(aRESP_HPI_ADAPTER_OPEN[0]));
			break;
		case HPI_MIXER_OPEN:
// send response back to user mode
			memcpy(phr, &aRESP_HPI_MIXER_OPEN[hm.wAdapterIndex],
			       sizeof(aRESP_HPI_MIXER_OPEN[0]));
			break;
		case HPI_OSTREAM_OPEN:
			{
// send response back to user mode
				memcpy(phr,
				       &aRESP_HPI_OSTREAM_OPEN[hm.
							       wAdapterIndex]
				       [hm.u.d.wOStreamIndex],
				       sizeof(aRESP_HPI_OSTREAM_OPEN[0][0]));
				if (aOStreamUserOpen[hm.wAdapterIndex]
				    [hm.u.d.wOStreamIndex].nOpenFlag) {
					phr->wError =
					    HPI_ERROR_OBJ_ALREADY_OPEN;

				} else {
// issue a reset
					hm.wFunction = HPI_OSTREAM_RESET;
					HPI_Message(phSubSys, &hm, &hr);
					if (hr.wError)
						phr->wError =
						    HPI_ERROR_INVALID_STREAM;
					else {
						aOStreamUserOpen[hm.
								 wAdapterIndex]
						    [hm.u.d.wOStreamIndex].
						    nOpenFlag = 1;
						aOStreamUserOpen[hm.
								 wAdapterIndex]
						    [hm.u.d.wOStreamIndex].
						    hOwner = hOwner;
					}
				}
			}
			break;
		case HPI_OSTREAM_CLOSE:
			{
				if (hOwner ==
				    aOStreamUserOpen[hm.wAdapterIndex][hm.u.d.
								       wOStreamIndex].
				    hOwner) {
// issue a reset
					hm.wFunction = HPI_OSTREAM_RESET;
					HPI_Message(phSubSys, &hm, &hr);
// handled in hpifunc.c::HPI_OutStreamClose
// hm.wFunction=HPI_OSTREAM_HOSTBUFFER_FREE;
//HPI_Message(phSubSys,&hm,&hr);
					if (hr.wError)
						phr->wError =
						    HPI_ERROR_INVALID_STREAM;
					else {
						hr.wSize =
						    sizeof(HPI_STREAM_RESPONSE);
						hr.wType = HPI_TYPE_RESPONSE;
						hr.wObject = hm.wObject;
						hr.wFunction = hm.wFunction;
						hr.wError = 0;
						aOStreamUserOpen[hm.
								 wAdapterIndex]
						    [hm.u.d.wOStreamIndex].
						    nOpenFlag = 0;
						aOStreamUserOpen[hm.
								 wAdapterIndex]
						    [hm.u.d.wOStreamIndex].
						    hOwner = 0;
						memcpy(phr, &hr, hr.wSize);
					}
				} else
					phr->wError = HPI_ERROR_OBJ_NOT_OPEN;

			}
			break;
		case HPI_ISTREAM_OPEN:
			{
// send response back to user mode
				memcpy(phr,
				       &aRESP_HPI_ISTREAM_OPEN[hm.
							       wAdapterIndex]
				       [hm.u.d.wIStreamIndex],
				       sizeof(aRESP_HPI_ISTREAM_OPEN[0][0]));

				if (aIStreamUserOpen[hm.wAdapterIndex]
				    [hm.u.d.wIStreamIndex].nOpenFlag)
					phr->wError =
					    HPI_ERROR_OBJ_ALREADY_OPEN;
				else {
// issue a reset
					hm.wFunction = HPI_ISTREAM_RESET;
					HPI_Message(phSubSys, &hm, &hr);
					if (hr.wError)
						phr->wError =
						    HPI_ERROR_INVALID_STREAM;
					else {
						aIStreamUserOpen[hm.
								 wAdapterIndex]
						    [hm.u.d.wIStreamIndex].
						    nOpenFlag = 1;
						aIStreamUserOpen[hm.
								 wAdapterIndex]
						    [hm.u.d.wIStreamIndex].
						    hOwner = hOwner;
					}
				}
			}
			break;
		case HPI_ISTREAM_CLOSE:
			{
				if (hOwner ==
				    aIStreamUserOpen[hm.wAdapterIndex][hm.u.d.
								       wIStreamIndex].
				    hOwner) {
// issue a reset
					hm.wFunction = HPI_ISTREAM_RESET;
					HPI_Message(phSubSys, &hm, &hr);
// handled in hpifunc.c::HPI_InStreamClose
// hm.wFunction=HPI_ISTREAM_HOSTBUFFER_FREE;
//HPI_Message(phSubSys,&hm,&hr);
					if (hr.wError)
						phr->wError =
						    HPI_ERROR_INVALID_STREAM;
					else {
						hr.wSize =
						    sizeof(HPI_STREAM_RESPONSE);
						hr.wType = HPI_TYPE_RESPONSE;
						hr.wObject = hm.wObject;
						hr.wFunction = hm.wFunction;
						hr.wError = 0;
						aIStreamUserOpen[hm.
								 wAdapterIndex]
						    [hm.u.d.wIStreamIndex].
						    nOpenFlag = 0;
						aIStreamUserOpen[hm.
								 wAdapterIndex]
						    [hm.u.d.wIStreamIndex].
						    hOwner = 0;
						memcpy(phr, &hr, hr.wSize);
					}
				}
			}
			break;
		case HPI_ADAPTER_CLOSE:
			{
				HPIMSGX_MessageExAdapterCleanup(hm.
								wAdapterIndex,
								hOwner);

				hr.wSize = sizeof(HPI_ADAPTER_RESPONSE);
				hr.wType = HPI_TYPE_RESPONSE;
				hr.wObject = hm.wObject;
				hr.wFunction = hm.wFunction;
				hr.wError = 0;
				memcpy(phr, &hr, hr.wSize);

			}
			break;
		case HPI_MIXER_CLOSE:
			{
				hr.wSize = sizeof(HPI_MIXER_RESPONSE);
				hr.wType = HPI_TYPE_RESPONSE;
				hr.wObject = hm.wObject;
				hr.wFunction = hm.wFunction;
				hr.wError = 0;
				memcpy(phr, &hr, hr.wSize);
			}
			break;
		default:
			{
// send the message to the HPI Manager
				HPI_Message(phSubSys, &hm, &hr);

// send response back to user mode
				if (hr.wSize > sizeof(HPI_RESPONSE)) {
					memcpy(phr, &hr, sizeof(hr));
					phr->wError = HPI_ERROR_CUSTOM;
				} else
					memcpy(phr, &hr, /* sizeof(hr) */
					       hr.wSize);
			}
			break;
		}		// switch ( wFunction )
	}			// else
}

void HPIMSGX_MessageExAdapterCleanup(u16 wAdapterIndex, void *hOwner)
{
	int i, wAdapter, wAdapterLimit;
	if (wAdapterIndex == HPIMSGX_ALLADAPTERS) {
		wAdapter = 0;
		wAdapterLimit = HPI_MAX_ADAPTERS;
	} else {
		wAdapter = wAdapterIndex;
		wAdapterLimit = wAdapter + 1;
	}

	for (; wAdapter < wAdapterLimit; wAdapter++) {
//      printk(KERN_INFO "Cleanup adapter #%d\n",wAdapter);

		for (i = 0; i < HPI_MAX_STREAMS; i++) {
			if (hOwner == aOStreamUserOpen[wAdapter][i].hOwner) {
				HPI_MESSAGE hm;
				HPI_RESPONSE hr;

				HPI_PRINT_VERBOSE
				    ("Close adapter %d ostream %d\n", wAdapter,
				     i);

				HPI_InitMessage(&hm, HPI_OBJ_OSTREAM,
						HPI_OSTREAM_HOSTBUFFER_FREE);
				hm.wAdapterIndex = (u16) wAdapter;
				hm.u.d.wOStreamIndex = (u16) i;
				hm.wDspIndex =
				    aOStreamUserOpen[wAdapter][i].wDspIndex;
				HPI_Message(0, &hm, &hr);

				aOStreamUserOpen[wAdapter][i].nOpenFlag = 0;
				aOStreamUserOpen[wAdapter][i].hOwner = 0;
			}
			if (hOwner == aIStreamUserOpen[wAdapter][i].hOwner) {
				HPI_MESSAGE hm;
				HPI_RESPONSE hr;

				HPI_PRINT_VERBOSE
				    ("Close adapter %d istream %d\n", wAdapter,
				     i);

				HPI_InitMessage(&hm, HPI_OBJ_ISTREAM,
						HPI_ISTREAM_HOSTBUFFER_FREE);
				hm.wAdapterIndex = (u16) wAdapter;
				hm.u.d.wIStreamIndex = (u16) i;
				hm.wDspIndex =
				    aIStreamUserOpen[wAdapter][i].wDspIndex;
				HPI_Message(0, &hm, &hr);

				aIStreamUserOpen[wAdapter][i].nOpenFlag = 0;
				aIStreamUserOpen[wAdapter][i].hOwner = 0;
			}
//      else printk(KERN_INFO "istream %d owner %p\n",i,aIStreamUserOpen[wAdapter][i].hOwner);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////
