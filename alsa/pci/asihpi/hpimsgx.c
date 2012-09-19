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
#define SOURCEFILE_NAME "hpimsgx.c"
#include "hpi.h"
#include "hpimsgx.h"
#include "hpidebug.h"

// defining PCI_ANY_ID for this module only!
#define PCI_ANY_ID 0xffff
static HPI_PCI_DEVICE_ID pci_dev_tbl[] = {
#include "hpipcida.h"
};

#define HPI_PreMessageSsx2(phm,phr,hOwner) (0)
#define HPI_PostMessageSsx2(phm,phr,hOwner)

static HPIOS_SPINLOCK msgxLock;

typedef HPI_HandlerFunc *HPI_ENTRY_POINT;

static HPI_ENTRY_POINT hpi_entry_points[HPI_MAX_ADAPTERS];

static inline HPI_ENTRY_POINT HPI_LookupEntryPointFunction(HPI_PCI * PciInfo)
{

	int i;

	for (i = 0; pci_dev_tbl[i].wVendorId != 0; i++) {
		if (pci_dev_tbl[i].wVendorId != PCI_ANY_ID
		    && pci_dev_tbl[i].wVendorId != PciInfo->wVendorId)
			continue;
		if (pci_dev_tbl[i].wDeviceId != PCI_ANY_ID
		    && pci_dev_tbl[i].wDeviceId != PciInfo->wDeviceId)
			continue;
		if (pci_dev_tbl[i].wSubSysVendorId != PCI_ANY_ID
		    && pci_dev_tbl[i].wSubSysVendorId !=
		    PciInfo->wSubSysVendorId)
			continue;
		if (pci_dev_tbl[i].wSubSysDeviceId != PCI_ANY_ID
		    && pci_dev_tbl[i].wSubSysDeviceId !=
		    PciInfo->wSubSysDeviceId)
			continue;

		HPI_DEBUG_LOG2(DEBUG, " %x,%p\n", i, pci_dev_tbl[i].drvData);
		return pci_dev_tbl[i].drvData;
	}

	return NULL;
}

static inline void HW_EntryPoint(HPI_MESSAGE * phm, HPI_RESPONSE * phr)
{

	HPI_ENTRY_POINT ep;

	if (phm->wAdapterIndex < HPI_MAX_ADAPTERS) {
		ep = hpi_entry_points[phm->wAdapterIndex];
		if (ep) {
			HPI_DEBUG_MESSAGE(phm);
			ep(phm, phr);
			HPI_DEBUG_RESPONSE(phr);
			return;
		}
	}
	HPI_InitResponse(phr, phm->wObject, phm->wFunction,
			 HPI_ERROR_PROCESSING_MESSAGE);
}

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

u32 HPI_IndexesToHandle(const char cObject, const u16 wIndex1,
			const u16 wIndex2);
u32 HPI_IndexesToHandle3(const char cObject, const u16 wIndex1,
			 const u16 wIndex2, const u16 wIndex0);

static void AdapterOpen(HPI_MESSAGE * phm, HPI_RESPONSE * phr);
static void AdapterClose(HPI_MESSAGE * phm, HPI_RESPONSE * phr);

static void MixerOpen(HPI_MESSAGE * phm, HPI_RESPONSE * phr);
static void MixerClose(HPI_MESSAGE * phm, HPI_RESPONSE * phr);

static void OutStreamOpen(HPI_MESSAGE * phm, HPI_RESPONSE * phr, void *hOwner);
static void OutStreamClose(HPI_MESSAGE * phm, HPI_RESPONSE * phr, void *hOwner);
static void InStreamOpen(HPI_MESSAGE * phm, HPI_RESPONSE * phr, void *hOwner);
static void InStreamClose(HPI_MESSAGE * phm, HPI_RESPONSE * phr, void *hOwner);

static void HPIMSGX_Reset(u16 wAdapterIndex);
static u16 HPIMSGX_Init(HPI_MESSAGE * phm, HPI_RESPONSE * phr);
static void HPIMSGX_Cleanup(u16 wAdapterIndex, void *hOwner);

#ifndef DISABLE_PRAGMA_PACK1
#pragma pack(push,1)
#endif

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

typedef struct {
	u16 wType;
	u16 wNumInStreams;
	u16 wNumOutStreams;
} ADAPTER_INFO;

#ifndef DISABLE_PRAGMA_PACK1
#pragma pack(pop)
#endif

//////////////////////////////////////////////////////////////////////////////////
// Globals
static HPI_ADAPTER_RESPONSE aRESP_HPI_ADAPTER_OPEN[HPI_MAX_ADAPTERS];
static HPI_STREAM_RESPONSE
    aRESP_HPI_OSTREAM_OPEN[HPI_MAX_ADAPTERS][HPI_MAX_STREAMS];
static HPI_STREAM_RESPONSE
    aRESP_HPI_ISTREAM_OPEN[HPI_MAX_ADAPTERS][HPI_MAX_STREAMS];
static HPI_MIXER_RESPONSE aRESP_HPI_MIXER_OPEN[HPI_MAX_ADAPTERS];
static HPI_SUBSYS_RESPONSE gRESP_HPI_SUBSYS_FIND_ADAPTERS;
static ADAPTER_INFO aADAPTER_INFO[HPI_MAX_ADAPTERS];

typedef struct {
	int nOpenFlag;
	void *hOwner;
	u16 wDspIndex;
} ASI_OPEN_STATE;

// use these to keep track of opens from user mode apps/DLLs
static ASI_OPEN_STATE aOStreamUserOpen[HPI_MAX_ADAPTERS][HPI_MAX_STREAMS];
static ASI_OPEN_STATE aIStreamUserOpen[HPI_MAX_ADAPTERS][HPI_MAX_STREAMS];

static void SubSysMessage(HPI_MESSAGE * phm, HPI_RESPONSE * phr, void *hOwner)
{

	if (phm->wAdapterIndex >= HPI_MAX_ADAPTERS
	    && phm->wAdapterIndex != HPIMSGX_ALLADAPTERS) {
		HPI_InitResponse(phr, HPI_OBJ_ADAPTER, HPI_SUBSYS_OPEN,
				 HPI_ERROR_BAD_ADAPTER_NUMBER);
		return;
	}

	switch (phm->wFunction) {
	case HPI_SUBSYS_GET_VERSION:
		HPI_InitResponse(phr, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_GET_VERSION,
				 0);
		phr->u.s.dwVersion = HPI_VER >> 8;	// return major.minor
		phr->u.s.dwData = HPI_VER;	// return major.minor.release
		break;
	case HPI_SUBSYS_OPEN:
//do not propagate the message down the chain
		HPI_InitResponse(phr, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_OPEN, 0);
		break;
	case HPI_SUBSYS_CLOSE:
//do not propagate the message down the chain
		HPI_InitResponse(phr, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_CLOSE, 0);
		HPIMSGX_Cleanup(HPIMSGX_ALLADAPTERS, hOwner);
		break;
	case HPI_SUBSYS_DRIVER_LOAD:
// Initialize this module's internal state
		HpiOs_Msgxlock_Init(&msgxLock);
		memset(&hpi_entry_points, 0, sizeof(hpi_entry_points));
#ifndef NO_HPIOS_LOCKEDMEM_OPS
		HpiOs_LockedMem_Init();
#endif
// Init subsys_findadapters response to no-adapters
		HPIMSGX_Reset(HPIMSGX_ALLADAPTERS);
// loop over all available HPIs passing the relevant message
// NOTE: we do it blindly because we don't know which cards are available at this stage
		{
			int i;
			HPI_ENTRY_POINT entry_point_func, prev_entry_point_func;
			prev_entry_point_func = NULL;
			for (i = 0; pci_dev_tbl[i].wVendorId != 0; i++) {
				entry_point_func =
				    (HPI_ENTRY_POINT) pci_dev_tbl[i].drvData;
// devices in pci_dev_tbl MUST be grouped by HPI entry point
				if (entry_point_func != prev_entry_point_func)
					entry_point_func(phm, phr);
				prev_entry_point_func = entry_point_func;
			}
		}
		HPI_InitResponse(phr, HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_DRIVER_LOAD,
				 0);
		break;
	case HPI_SUBSYS_DRIVER_UNLOAD:
// loop over all available HPIs passing the relevant message
// NOTE: we do it blindly because we don't know which cards are available at this stage
		{
			int i;
			HPI_ENTRY_POINT entry_point_func, prev_entry_point_func;
			prev_entry_point_func = NULL;
			for (i = 0; pci_dev_tbl[i].wVendorId != 0; i++) {
				entry_point_func =
				    (HPI_ENTRY_POINT) pci_dev_tbl[i].drvData;
// devices in pci_dev_tbl MUST be grouped by HPI entry point
				if (entry_point_func != prev_entry_point_func)
					entry_point_func(phm, phr);
				prev_entry_point_func = entry_point_func;
			}
		}
		HPIMSGX_Cleanup(HPIMSGX_ALLADAPTERS, hOwner);
#ifndef NO_HPIOS_LOCKEDMEM_OPS
		HpiOs_LockedMem_FreeAll();
#endif
		HPI_InitResponse(phr, HPI_OBJ_SUBSYSTEM,
				 HPI_SUBSYS_DRIVER_UNLOAD, 0);
		return;
	case HPI_SUBSYS_FIND_ADAPTERS:
		memcpy(phr, &gRESP_HPI_SUBSYS_FIND_ADAPTERS,
		       sizeof(gRESP_HPI_SUBSYS_FIND_ADAPTERS));
		break;
	case HPI_SUBSYS_CREATE_ADAPTER:
		HPIMSGX_Init(phm, phr);
		break;
	case HPI_SUBSYS_DELETE_ADAPTER:
		HPIMSGX_Cleanup(phm->wAdapterIndex, hOwner);
		{
			HPI_MESSAGE hm;
			HPI_RESPONSE hr;
// call to HPI_ADAPTER_CLOSE
			HPI_InitMessage(&hm, HPI_OBJ_ADAPTER,
					HPI_ADAPTER_CLOSE);
			hm.wAdapterIndex = phm->wAdapterIndex;
			HW_EntryPoint(&hm, &hr);
		}
		HW_EntryPoint(phm, phr);
		gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.awAdapterList[phm->
							       wAdapterIndex] =
		    0;
		hpi_entry_points[phm->wAdapterIndex] = NULL;
		break;
	default:
		HW_EntryPoint(phm, phr);
		break;
	}
}

static void AdapterMessage(HPI_MESSAGE * phm, HPI_RESPONSE * phr, void *hOwner)
{

	if (phm->wAdapterIndex >= HPI_MAX_ADAPTERS) {
		HPI_InitResponse(phr, HPI_OBJ_ADAPTER, HPI_ADAPTER_OPEN,
				 HPI_ERROR_BAD_ADAPTER_NUMBER);
		return;
	}

	switch (phm->wFunction) {
	case HPI_ADAPTER_OPEN:
		AdapterOpen(phm, phr);
		break;
	case HPI_ADAPTER_CLOSE:
		AdapterClose(phm, phr);
		break;
	default:
		HW_EntryPoint(phm, phr);
		break;
	}
}

static void MixerMessage(HPI_MESSAGE * phm, HPI_RESPONSE * phr)
{

	if (phm->wAdapterIndex >= HPI_MAX_ADAPTERS) {
		HPI_InitResponse(phr, HPI_OBJ_MIXER, HPI_MIXER_OPEN,
				 HPI_ERROR_BAD_ADAPTER_NUMBER);
		return;
	}

	switch (phm->wFunction) {
	case HPI_MIXER_OPEN:
		MixerOpen(phm, phr);
		break;
	case HPI_MIXER_CLOSE:
		MixerClose(phm, phr);
		break;
	default:
		HW_EntryPoint(phm, phr);
		break;
	}
}

static void OStreamMessage(HPI_MESSAGE * phm, HPI_RESPONSE * phr, void *hOwner)
{

	if (phm->wAdapterIndex >= HPI_MAX_ADAPTERS) {
		HPI_InitResponse(phr, HPI_OBJ_OSTREAM, HPI_OSTREAM_OPEN,
				 HPI_ERROR_BAD_ADAPTER_NUMBER);
		return;
	}

	if (phm->u.d.wStreamIndex >=
	    aADAPTER_INFO[phm->wAdapterIndex].wNumOutStreams) {
		HPI_InitResponse(phr, HPI_OBJ_OSTREAM, HPI_OSTREAM_OPEN,
				 HPI_ERROR_INVALID_OBJ_INDEX);
		return;
	}

	switch (phm->wFunction) {
	case HPI_OSTREAM_OPEN:
		OutStreamOpen(phm, phr, hOwner);
		break;
	case HPI_OSTREAM_CLOSE:
		OutStreamClose(phm, phr, hOwner);
		break;
	default:
		HW_EntryPoint(phm, phr);
		break;
	}
}

static void IStreamMessage(HPI_MESSAGE * phm, HPI_RESPONSE * phr, void *hOwner)
{

	if (phm->wAdapterIndex >= HPI_MAX_ADAPTERS) {
		HPI_InitResponse(phr, HPI_OBJ_ISTREAM, HPI_ISTREAM_OPEN,
				 HPI_ERROR_BAD_ADAPTER_NUMBER);
		return;
	}
	if (phm->u.d.wStreamIndex >=
	    aADAPTER_INFO[phm->wAdapterIndex].wNumInStreams) {
		HPI_InitResponse(phr, HPI_OBJ_ISTREAM, HPI_ISTREAM_OPEN,
				 HPI_ERROR_INVALID_OBJ_INDEX);
		return;
	}

	switch (phm->wFunction) {
	case HPI_ISTREAM_OPEN:
		InStreamOpen(phm, phr, hOwner);
		break;
	case HPI_ISTREAM_CLOSE:
		InStreamClose(phm, phr, hOwner);
		break;
	default:
		HW_EntryPoint(phm, phr);
		break;
	}
}

// NOTE: HPI_Message must be defined in the driver as a wrapper for HPI_MessageEx so that functions in hpifunc.c
// compile.
void HPI_MessageEx(HPI_MESSAGE * phm, HPI_RESPONSE * phr, void *hOwner)
{

	HPI_DEBUG_MESSAGE(phm);

	if (!HPI_PreMessageSsx2(phm, phr, hOwner)) {
		switch (phm->wType) {
		case HPI_TYPE_MESSAGE:
			switch (phm->wObject) {
			case HPI_OBJ_SUBSYSTEM:
				SubSysMessage(phm, phr, hOwner);
				break;

			case HPI_OBJ_ADAPTER:
				AdapterMessage(phm, phr, hOwner);
				break;

			case HPI_OBJ_MIXER:
				MixerMessage(phm, phr);
				break;

			case HPI_OBJ_OSTREAM:
				OStreamMessage(phm, phr, hOwner);
				break;

			case HPI_OBJ_ISTREAM:
				IStreamMessage(phm, phr, hOwner);
				break;

			default:
				HW_EntryPoint(phm, phr);
				break;
			}
			break;

		default:
			HPI_InitResponse(phr, phm->wObject, phm->wFunction,
					 HPI_ERROR_INVALID_TYPE);
			break;
		}
	}
	HPI_PostMessageSsx2(phm, phr, hOwner);

	HPI_DEBUG_RESPONSE(phr);
}

static void AdapterOpen(HPI_MESSAGE * phm, HPI_RESPONSE * phr)
{
	HPI_DEBUG_LOG0(VERBOSE, "AdapterOpen\n");
	memcpy(phr, &aRESP_HPI_ADAPTER_OPEN[phm->wAdapterIndex],
	       sizeof(aRESP_HPI_ADAPTER_OPEN[0]));
}

static void AdapterClose(HPI_MESSAGE * phm, HPI_RESPONSE * phr)
{
	HPI_DEBUG_LOG0(VERBOSE, "AdapterClose\n");
	HPI_InitResponse(phr, HPI_OBJ_ADAPTER, HPI_ADAPTER_CLOSE, 0);
}

static void MixerOpen(HPI_MESSAGE * phm, HPI_RESPONSE * phr)
{
	memcpy(phr, &aRESP_HPI_MIXER_OPEN[phm->wAdapterIndex],
	       sizeof(aRESP_HPI_MIXER_OPEN[0]));
}

static void MixerClose(HPI_MESSAGE * phm, HPI_RESPONSE * phr)
{
	HPI_InitResponse(phr, HPI_OBJ_MIXER, HPI_MIXER_CLOSE, 0);
}

static void InStreamOpen(HPI_MESSAGE * phm, HPI_RESPONSE * phr, void *hOwner)
{

	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

	HPIOS_LOCK_FLAGS(flags);

	HPI_InitResponse(phr, HPI_OBJ_ISTREAM, HPI_ISTREAM_OPEN, 0);

	HpiOs_Msgxlock_Lock(&msgxLock, &flags);

	if (aIStreamUserOpen[phm->wAdapterIndex][phm->u.d.wStreamIndex].
	    nOpenFlag) {
		phr->wError = HPI_ERROR_OBJ_ALREADY_OPEN;
	} else
	    if (aRESP_HPI_ISTREAM_OPEN[phm->wAdapterIndex]
		[phm->u.d.wStreamIndex].h.wError) {
		memcpy(phr,
		       &aRESP_HPI_ISTREAM_OPEN[phm->wAdapterIndex][phm->u.d.
								   wStreamIndex],
		       sizeof(aRESP_HPI_ISTREAM_OPEN[0][0]));
	} else {
		aIStreamUserOpen[phm->wAdapterIndex][phm->u.d.wStreamIndex].
		    nOpenFlag = 1;
		HpiOs_Msgxlock_UnLock(&msgxLock, &flags);

// issue a reset
		memcpy(&hm, phm, sizeof(HPI_MESSAGE));
		hm.wFunction = HPI_ISTREAM_RESET;
		HW_EntryPoint(&hm, &hr);

		HpiOs_Msgxlock_Lock(&msgxLock, &flags);
		if (hr.wError) {
			aIStreamUserOpen[phm->wAdapterIndex][phm->u.d.
							     wStreamIndex].
			    nOpenFlag = 0;
			phr->wError = hr.wError;
		} else {
			aIStreamUserOpen[phm->wAdapterIndex][phm->u.d.
							     wStreamIndex].
			    nOpenFlag = 1;
			aIStreamUserOpen[phm->wAdapterIndex][phm->u.d.
							     wStreamIndex].
			    hOwner = hOwner;
			memcpy(phr,
			       &aRESP_HPI_ISTREAM_OPEN[phm->wAdapterIndex][phm->
									   u.d.
									   wStreamIndex],
			       sizeof(aRESP_HPI_ISTREAM_OPEN[0][0]));
		}
	}
	HpiOs_Msgxlock_UnLock(&msgxLock, &flags);
}

static void InStreamClose(HPI_MESSAGE * phm, HPI_RESPONSE * phr, void *hOwner)
{

	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

	HPIOS_LOCK_FLAGS(flags);

	HPI_InitResponse(phr, HPI_OBJ_ISTREAM, HPI_ISTREAM_CLOSE, 0);

	HpiOs_Msgxlock_Lock(&msgxLock, &flags);
	if (hOwner ==
	    aIStreamUserOpen[phm->wAdapterIndex][phm->u.d.wStreamIndex].
	    hOwner) {
/* HPI_DEBUG_LOG3(INFO,"closing adapter %d instream %d owned by %p\n", 
phm->wAdapterIndex, phm->u.d.wStreamIndex, hOwner); */
		aIStreamUserOpen[phm->wAdapterIndex][phm->u.d.wStreamIndex].
		    hOwner = NULL;
		HpiOs_Msgxlock_UnLock(&msgxLock, &flags);
// issue a reset
		memcpy(&hm, phm, sizeof(HPI_MESSAGE));
		hm.wFunction = HPI_ISTREAM_RESET;
		HW_EntryPoint(&hm, &hr);
		HpiOs_Msgxlock_Lock(&msgxLock, &flags);
		if (hr.wError) {
			aIStreamUserOpen[phm->wAdapterIndex][phm->u.d.
							     wStreamIndex].
			    hOwner = hOwner;
			phr->wError = hr.wError;
		} else {
			aIStreamUserOpen[phm->wAdapterIndex][phm->u.d.
							     wStreamIndex].
			    nOpenFlag = 0;
			aIStreamUserOpen[phm->wAdapterIndex][phm->u.d.
							     wStreamIndex].
			    hOwner = NULL;
		}
	} else {
		HPI_DEBUG_LOG4(WARNING,
			       "%p trying to close %d instream %d owned by %p\n",
			       hOwner, phm->wAdapterIndex,
			       phm->u.d.wStreamIndex,
			       aIStreamUserOpen[phm->wAdapterIndex][phm->u.d.
								    wStreamIndex].
			       hOwner);
		phr->wError = HPI_ERROR_OBJ_NOT_OPEN;
	}
	HpiOs_Msgxlock_UnLock(&msgxLock, &flags);
}

static void OutStreamOpen(HPI_MESSAGE * phm, HPI_RESPONSE * phr, void *hOwner)
{

	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

	HPIOS_LOCK_FLAGS(flags);

	HPI_InitResponse(phr, HPI_OBJ_OSTREAM, HPI_OSTREAM_OPEN, 0);

	HpiOs_Msgxlock_Lock(&msgxLock, &flags);
	if (aOStreamUserOpen[phm->wAdapterIndex][phm->u.d.wStreamIndex].
	    nOpenFlag) {
		phr->wError = HPI_ERROR_OBJ_ALREADY_OPEN;
	} else
	    if (aRESP_HPI_OSTREAM_OPEN[phm->wAdapterIndex]
		[phm->u.d.wStreamIndex].h.wError) {
		memcpy(phr,
		       &aRESP_HPI_OSTREAM_OPEN[phm->wAdapterIndex][phm->u.d.
								   wStreamIndex],
		       sizeof(aRESP_HPI_OSTREAM_OPEN[0][0]));
	} else {
		aOStreamUserOpen[phm->wAdapterIndex][phm->u.d.wStreamIndex].
		    nOpenFlag = 1;
		HpiOs_Msgxlock_UnLock(&msgxLock, &flags);

// issue a reset
		memcpy(&hm, phm, sizeof(HPI_MESSAGE));
		hm.wFunction = HPI_OSTREAM_RESET;
		HW_EntryPoint(&hm, &hr);

		HpiOs_Msgxlock_Lock(&msgxLock, &flags);
		if (hr.wError) {
			aOStreamUserOpen[phm->wAdapterIndex][phm->u.d.
							     wStreamIndex].
			    nOpenFlag = 0;
			phr->wError = hr.wError;
		} else {
			aOStreamUserOpen[phm->wAdapterIndex][phm->u.d.
							     wStreamIndex].
			    nOpenFlag = 1;
			aOStreamUserOpen[phm->wAdapterIndex][phm->u.d.
							     wStreamIndex].
			    hOwner = hOwner;
			memcpy(phr,
			       &aRESP_HPI_OSTREAM_OPEN[phm->wAdapterIndex][phm->
									   u.d.
									   wStreamIndex],
			       sizeof(aRESP_HPI_OSTREAM_OPEN[0][0]));
		}
	}
	HpiOs_Msgxlock_UnLock(&msgxLock, &flags);
}

static void OutStreamClose(HPI_MESSAGE * phm, HPI_RESPONSE * phr, void *hOwner)
{

	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

	HPIOS_LOCK_FLAGS(flags);

	HPI_InitResponse(phr, HPI_OBJ_OSTREAM, HPI_OSTREAM_CLOSE, 0);

	HpiOs_Msgxlock_Lock(&msgxLock, &flags);
	if (hOwner ==
	    aOStreamUserOpen[phm->wAdapterIndex][phm->u.d.wStreamIndex].
	    hOwner) {
/* HPI_DEBUG_LOG3(INFO,"closing adapter %d outstream %d owned by %p\n", 
phm->wAdapterIndex, phm->u.d.wStreamIndex, hOwner); */
		aOStreamUserOpen[phm->wAdapterIndex][phm->u.d.wStreamIndex].
		    hOwner = NULL;
		HpiOs_Msgxlock_UnLock(&msgxLock, &flags);
// issue a reset
		memcpy(&hm, phm, sizeof(HPI_MESSAGE));
		hm.wFunction = HPI_OSTREAM_RESET;
		HW_EntryPoint(&hm, &hr);
		HpiOs_Msgxlock_Lock(&msgxLock, &flags);
		if (hr.wError) {
			aOStreamUserOpen[phm->wAdapterIndex][phm->u.d.
							     wStreamIndex].
			    hOwner = hOwner;
			phr->wError = hr.wError;
		} else {
			aOStreamUserOpen[phm->wAdapterIndex][phm->u.d.
							     wStreamIndex].
			    nOpenFlag = 0;
			aOStreamUserOpen[phm->wAdapterIndex][phm->u.d.
							     wStreamIndex].
			    hOwner = NULL;
		}
	} else {
		HPI_DEBUG_LOG4(WARNING,
			       "%p trying to close %d outstream %d owned by %p\n",
			       hOwner, phm->wAdapterIndex,
			       phm->u.d.wStreamIndex,
			       aOStreamUserOpen[phm->wAdapterIndex][phm->u.d.
								    wStreamIndex].
			       hOwner);
		phr->wError = HPI_ERROR_OBJ_NOT_OPEN;
	}
	HpiOs_Msgxlock_UnLock(&msgxLock, &flags);
}

static u16 AdapterPrepare(u16 wAdapter)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;

// Open the adapter and streams
	u16 i;

// call to HPI_ADAPTER_OPEN
	HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_OPEN);
	hm.wAdapterIndex = wAdapter;
	HW_EntryPoint(&hm, &hr);
	memcpy(&aRESP_HPI_ADAPTER_OPEN[wAdapter], &hr,
	       sizeof(aRESP_HPI_ADAPTER_OPEN[0]));
	if (hr.wError) {
		return hr.wError;
	}
// call to HPI_ADAPTER_GET_INFO
	HPI_InitMessage(&hm, HPI_OBJ_ADAPTER, HPI_ADAPTER_GET_INFO);
	hm.wAdapterIndex = wAdapter;
	HW_EntryPoint(&hm, &hr);
	if (hr.wError) {
		return hr.wError;
	}

	aADAPTER_INFO[wAdapter].wNumOutStreams = hr.u.a.wNumOStreams;
	aADAPTER_INFO[wAdapter].wNumInStreams = hr.u.a.wNumIStreams;
	aADAPTER_INFO[wAdapter].wType = hr.u.a.wAdapterType;

	gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.awAdapterList[wAdapter] =
	    hr.u.a.wAdapterType;
	gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.wNumAdapters++;
	if (gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.wNumAdapters > HPI_MAX_ADAPTERS)
		gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.wNumAdapters =
		    HPI_MAX_ADAPTERS;

// call to HPI_OSTREAM_OPEN
	for (i = 0; i < aADAPTER_INFO[wAdapter].wNumOutStreams; i++) {
		HPI_InitMessage(&hm, HPI_OBJ_OSTREAM, HPI_OSTREAM_OPEN);
		hm.wAdapterIndex = wAdapter;
		hm.u.d.wStreamIndex = i;
		HW_EntryPoint(&hm, &hr);
		memcpy(&aRESP_HPI_OSTREAM_OPEN[wAdapter][i], &hr,
		       sizeof(aRESP_HPI_OSTREAM_OPEN[0][0]));
		aOStreamUserOpen[wAdapter][i].nOpenFlag = 0;
		aOStreamUserOpen[wAdapter][i].hOwner = NULL;
	}

// call to HPI_ISTREAM_OPEN
	for (i = 0; i < aADAPTER_INFO[wAdapter].wNumInStreams; i++) {
		HPI_AdapterFindObject(NULL, wAdapter, HPI_OBJ_ISTREAM, i,
				      &aIStreamUserOpen[wAdapter][i].wDspIndex);
		HPI_InitMessage(&hm, HPI_OBJ_ISTREAM, HPI_ISTREAM_OPEN);
		hm.wAdapterIndex = wAdapter;
		hm.u.d.wStreamIndex = i;
		hm.wDspIndex = aIStreamUserOpen[wAdapter][i].wDspIndex;
		HW_EntryPoint(&hm, &hr);
		memcpy(&aRESP_HPI_ISTREAM_OPEN[wAdapter][i], &hr,
		       sizeof(aRESP_HPI_ISTREAM_OPEN[0][0]));
		aIStreamUserOpen[wAdapter][i].nOpenFlag = 0;
		aIStreamUserOpen[wAdapter][i].hOwner = NULL;
	}

// call to HPI_MIXER_OPEN
	HPI_InitMessage(&hm, HPI_OBJ_MIXER, HPI_MIXER_OPEN);
	hm.wAdapterIndex = wAdapter;
	HW_EntryPoint(&hm, &hr);
	memcpy(&aRESP_HPI_MIXER_OPEN[wAdapter], &hr,
	       sizeof(aRESP_HPI_MIXER_OPEN[0]));

	return gRESP_HPI_SUBSYS_FIND_ADAPTERS.h.wError;
}

static void HPIMSGX_Reset(u16 wAdapterIndex)
{
	int i;
	u16 wAdapter;
	HPI_RESPONSE hr;

	if (wAdapterIndex == HPIMSGX_ALLADAPTERS) {

// reset all responses to contain errors
		HPI_InitResponse(&hr, HPI_OBJ_SUBSYSTEM,
				 HPI_SUBSYS_FIND_ADAPTERS, 0);
		memcpy(&gRESP_HPI_SUBSYS_FIND_ADAPTERS, &hr,
		       sizeof(&gRESP_HPI_SUBSYS_FIND_ADAPTERS));

		for (wAdapter = 0; wAdapter < HPI_MAX_ADAPTERS; wAdapter++) {
			HPI_InitResponse(&hr, HPI_OBJ_ADAPTER, HPI_ADAPTER_OPEN,
					 HPI_ERROR_BAD_ADAPTER);
			memcpy(&aRESP_HPI_ADAPTER_OPEN[wAdapter], &hr,
			       sizeof(aRESP_HPI_ADAPTER_OPEN[wAdapter]));

			HPI_InitResponse(&hr, HPI_OBJ_MIXER, HPI_MIXER_OPEN,
					 HPI_ERROR_INVALID_OBJ);
			memcpy(&aRESP_HPI_MIXER_OPEN[wAdapter], &hr,
			       sizeof(aRESP_HPI_MIXER_OPEN[wAdapter]));

			for (i = 0; i < HPI_MAX_STREAMS; i++) {
				HPI_InitResponse(&hr, HPI_OBJ_OSTREAM,
						 HPI_OSTREAM_OPEN,
						 HPI_ERROR_INVALID_OBJ);
				memcpy(&aRESP_HPI_OSTREAM_OPEN[wAdapter][i],
				       &hr,
				       sizeof(aRESP_HPI_OSTREAM_OPEN[wAdapter]
					      [i]));
				HPI_InitResponse(&hr, HPI_OBJ_ISTREAM,
						 HPI_ISTREAM_OPEN,
						 HPI_ERROR_INVALID_OBJ);
				memcpy(&aRESP_HPI_ISTREAM_OPEN[wAdapter][i],
				       &hr,
				       sizeof(aRESP_HPI_ISTREAM_OPEN[wAdapter]
					      [i]));
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

static u16 HPIMSGX_Init(HPI_MESSAGE * phm,	// HPI_SUBSYS_CREATE_ADAPTER structure with
// resource list or NULL=find all
			HPI_RESPONSE * phr	// response from HPI_ADAPTER_GET_INFO
    )
{
	HPI_ENTRY_POINT entry_point_func;
	HPI_RESPONSE hr;

	if (gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.wNumAdapters >= HPI_MAX_ADAPTERS)
		return HPI_ERROR_BAD_ADAPTER_NUMBER;

// Init response here so we can pass in previous adapter list
	HPI_InitResponse(&hr, phm->wObject, phm->wFunction,
			 HPI_ERROR_INVALID_OBJ);
	memcpy(hr.u.s.awAdapterList,
	       gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.awAdapterList,
	       sizeof(gRESP_HPI_SUBSYS_FIND_ADAPTERS.s.awAdapterList));

	entry_point_func =
	    HPI_LookupEntryPointFunction(phm->u.s.Resource.r.Pci);

	if (entry_point_func) {
		HPI_DEBUG_MESSAGE(phm);
		entry_point_func(phm, &hr);
	} else {
		phr->wError = HPI_ERROR_PROCESSING_MESSAGE;
		return phr->wError;
	}
// if the adapter was created succesfully save the mapping for future use
	if (hr.wError == 0) {
		hpi_entry_points[hr.u.s.wAdapterIndex] = entry_point_func;
// prepare adapter (pre-open streams etc.)
		HPI_DEBUG_LOG0(DEBUG,
			       "HPI_SUBSYS_CREATE_ADAPTER successful, preparing adapter\n");
		AdapterPrepare(hr.u.s.wAdapterIndex);
	}
	memcpy(phr, &hr, hr.wSize);
	return phr->wError;
}

static void HPIMSGX_Cleanup(u16 wAdapterIndex, void *hOwner)
{
	int i, wAdapter, wAdapterLimit;

	if (!hOwner)
		return;

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

				HPI_DEBUG_LOG2(DEBUG,
					       "Close adapter %d ostream %d\n",
					       wAdapter, i);

				HPI_InitMessage(&hm, HPI_OBJ_OSTREAM,
						HPI_OSTREAM_RESET);
				hm.wAdapterIndex = (u16) wAdapter;
				hm.u.d.wStreamIndex = (u16) i;
// hm.wDspIndex = Always 0 for Ostream
				HW_EntryPoint(&hm, &hr);

				hm.wFunction = HPI_OSTREAM_HOSTBUFFER_FREE;
				HW_EntryPoint(&hm, &hr);

				hm.wFunction = HPI_OSTREAM_GROUP_RESET;
				HW_EntryPoint(&hm, &hr);

				aOStreamUserOpen[wAdapter][i].nOpenFlag = 0;
				aOStreamUserOpen[wAdapter][i].hOwner = NULL;
			}
			if (hOwner == aIStreamUserOpen[wAdapter][i].hOwner) {
				HPI_MESSAGE hm;
				HPI_RESPONSE hr;

				HPI_DEBUG_LOG2(DEBUG,
					       "Close adapter %d istream %d\n",
					       wAdapter, i);

				HPI_InitMessage(&hm, HPI_OBJ_ISTREAM,
						HPI_ISTREAM_RESET);
				hm.wAdapterIndex = (u16) wAdapter;
				hm.u.d.wStreamIndex = (u16) i;
				hm.wDspIndex =
				    aIStreamUserOpen[wAdapter][i].wDspIndex;
				HW_EntryPoint(&hm, &hr);

				hm.wFunction = HPI_ISTREAM_HOSTBUFFER_FREE;
				HW_EntryPoint(&hm, &hr);

				hm.wFunction = HPI_ISTREAM_GROUP_RESET;
				HW_EntryPoint(&hm, &hr);

				aIStreamUserOpen[wAdapter][i].nOpenFlag = 0;
				aIStreamUserOpen[wAdapter][i].hOwner = NULL;
			}
//    else printk(KERN_INFO "adapter %d, istream %d owner %p\n",wAdapter,i,aIStreamUserOpen[wAdapter][i].hOwner);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////
