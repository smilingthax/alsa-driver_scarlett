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

Hardware Programming Interface (HPI) for AudioScience ASI4500 and 4100
series adapters.  These PCI bus adapters are based on the Motorola DSP56301
DSP with on-chip PCI I/F.

Exported functions:
void HPI_4000( HPI_MESSAGE *phm, HPI_RESPONSE *phr )
******************************************************************************/
#include "hpi.h"
#include "hpidebug.h"		// for debug
#include "hpicmn.h"
#include "hpi56301.h"
#include "hpipci.h"

////////////////////////////////////////////////////////////////////////////
// local defines

typedef struct {
	HPI_56301_INFO_OBJ hpi56301info;
} HPI_HW_OBJ;

////////////////////////////////////////////////////////////////////////////
// local prototypes
static void SubSysCreateAdapter(HPI_ADAPTERS_LIST * adaptersList,
				HPI_MESSAGE * phm, HPI_RESPONSE * phr);
static void SubSysDeleteAdapter(HPI_ADAPTERS_LIST * adaptersList,
				HPI_MESSAGE * phm, HPI_RESPONSE * phr);
static inline void HW_Message(HPI_ADAPTER_OBJ * pao, HPI_MESSAGE * phm,
			      HPI_RESPONSE * phr);
static short CreateAdapterObj(HPI_ADAPTER_OBJ * pao, u32 * pdwOsErrorCode);

////////////////////////////////////////////////////////////////////////////
// local globals

static HPI_ADAPTERS_LIST adapters;

static inline void HW_Message(HPI_ADAPTER_OBJ * pao, HPI_MESSAGE * phm,
			      HPI_RESPONSE * phr)
{
	HPIOS_LOCK_FLAGS(flags);

	HpiOs_Dsplock_Lock(pao, &flags);
	Hpi56301_Message(&((HPI_HW_OBJ *) pao->priv)->hpi56301info, phm, phr);
	HpiOs_Dsplock_UnLock(pao, &flags);
}

static void SubSysMessage(HPI_ADAPTERS_LIST * adaptersList, HPI_MESSAGE * phm,
			  HPI_RESPONSE * phr)
{

	switch (phm->wFunction) {
	case HPI_SUBSYS_OPEN:
	case HPI_SUBSYS_CLOSE:
	case HPI_SUBSYS_DRIVER_UNLOAD:
		phr->wError = 0;
		break;
	case HPI_SUBSYS_DRIVER_LOAD:
		WipeAdapterList(adaptersList);
		phr->wError = 0;
		break;
	case HPI_SUBSYS_GET_INFO:
		SubSysGetAdapters(adaptersList, phr);
		break;
	case HPI_SUBSYS_FIND_ADAPTERS:
		phr->wError = HPI_ERROR_INVALID_FUNC;
		break;
	case HPI_SUBSYS_CREATE_ADAPTER:
		SubSysCreateAdapter(adaptersList, phm, phr);
		break;
	case HPI_SUBSYS_DELETE_ADAPTER:
		SubSysDeleteAdapter(adaptersList, phm, phr);
		break;
	default:
		phr->wError = HPI_ERROR_INVALID_FUNC;
		break;
	}
}

static void AdapterMessage(HPI_ADAPTER_OBJ * pao, HPI_MESSAGE * phm,
			   HPI_RESPONSE * phr)
{

	switch (phm->wFunction) {
	case HPI_ADAPTER_GET_INFO:
		HW_Message(pao, phm, phr);
		break;
	case HPI_ADAPTER_GET_ASSERT:
		HW_Message(pao, phm, phr);
		phr->u.a.wAdapterType = 0;
		break;
	case HPI_ADAPTER_OPEN:
	case HPI_ADAPTER_CLOSE:
	case HPI_ADAPTER_TEST_ASSERT:
	case HPI_ADAPTER_SELFTEST:
	case HPI_ADAPTER_GET_MODE:
	case HPI_ADAPTER_SET_MODE:
		HW_Message(pao, phm, phr);
		break;
	case HPI_ADAPTER_FIND_OBJECT:
		HPI_InitResponse(phr, HPI_OBJ_ADAPTER, HPI_ADAPTER_FIND_OBJECT,
				 0);
		phr->u.a.wAdapterIndex = 0;	// really DSP index in this context
		break;
	default:
		phr->wError = HPI_ERROR_INVALID_FUNC;
		break;
	}
}

static void MixerMessage(HPI_ADAPTER_OBJ * pao, HPI_MESSAGE * phm,
			 HPI_RESPONSE * phr)
{

	switch (phm->wFunction) {
	case HPI_MIXER_OPEN:
// experiment with delay to allow settling of D/As on adapter
// before enabling mixer and so outputs
		HpiOs_DelayMicroSeconds(500000L);	//500ms
		HW_Message(pao, phm, phr);
		return;
	default:
		HW_Message(pao, phm, phr);
		return;
	}
}

static void OStreamMessage(HPI_ADAPTER_OBJ * pao, HPI_MESSAGE * phm,
			   HPI_RESPONSE * phr)
{

	switch (phm->wFunction) {
	case HPI_OSTREAM_HOSTBUFFER_ALLOC:
	case HPI_OSTREAM_HOSTBUFFER_FREE:
// Don't let these messages go to the HW function because they're called
// without allocating the spinlock.  For the HPI4000 adapters the HW would
// return HPI_ERROR_INVALID_FUNC anyway.
		phr->wError = HPI_ERROR_INVALID_FUNC;
		break;
	default:
		HW_Message(pao, phm, phr);
		return;
	}
}

static void IStreamMessage(HPI_ADAPTER_OBJ * pao, HPI_MESSAGE * phm,
			   HPI_RESPONSE * phr)
{

	switch (phm->wFunction) {
	case HPI_ISTREAM_HOSTBUFFER_ALLOC:
	case HPI_ISTREAM_HOSTBUFFER_FREE:
// Don't let these messages go to the HW function because they're called
// without allocating the spinlock.  For the HPI4000 adapters the HW would
// return HPI_ERROR_INVALID_FUNC anyway.
		phr->wError = HPI_ERROR_INVALID_FUNC;
		break;
	default:
		HW_Message(pao, phm, phr);
		return;
	}
}

////////////////////////////////////////////////////////////////////////////
// HPI_4000()
// Entry point from HPIMAN
// All calls to the HPI start here

void HPI_4000(HPI_MESSAGE * phm, HPI_RESPONSE * phr)
{
	HPI_ADAPTER_OBJ *pao = NULL;

// subsytem messages get executed by every HPI.
// All other messages are ignored unless the adapter index matches
// an adapter in the HPI
	HPI_DEBUG_LOG2(DEBUG, "O %d,F %d\n", phm->wObject, phm->wFunction);

//phr->wError=0;      // SGT JUN-15-01 - so that modules can't overwrite errors from FindAdapter

// if Dsp has crashed then do not try and communicated with it any more
	if (phm->wObject != HPI_OBJ_SUBSYSTEM) {
		pao = FindAdapter(&adapters, phm->wAdapterIndex);
		if (!pao) {
			HPI_DEBUG_LOG2(DEBUG,
				       " %d,%d refused, for another HPI?\n",
				       phm->wObject, phm->wFunction);
			return;	// message probably meant for another HPI module
		}

		if (pao->wDspCrashed) {
			HPI_InitResponse(phr, phm->wObject, phm->wFunction,
					 HPI_ERROR_DSP_HARDWARE);
			HPI_DEBUG_LOG2(DEBUG, " %d,%d dsp crashed.\n",
				       phm->wObject, phm->wFunction);
			return;
		}
	}
//Init default response ( sets the size field in the response structure )
	if (phm->wFunction != HPI_SUBSYS_CREATE_ADAPTER)
		HPI_InitResponse(phr, phm->wObject, phm->wFunction,
				 HPI_ERROR_PROCESSING_MESSAGE);

	HPI_DEBUG_LOG0(VERBOSE, "start of switch\n");
	switch (phm->wType) {
	case HPI_TYPE_MESSAGE:
		switch (phm->wObject) {
		case HPI_OBJ_SUBSYSTEM:
			SubSysMessage(&adapters, phm, phr);
			break;

		case HPI_OBJ_ADAPTER:
			phr->wSize =
			    HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_ADAPTER_RES);
			AdapterMessage(pao, phm, phr);
			break;

		case HPI_OBJ_MIXER:
			MixerMessage(pao, phm, phr);
			break;

		case HPI_OBJ_OSTREAM:
			OStreamMessage(pao, phm, phr);
			break;

		case HPI_OBJ_ISTREAM:
			IStreamMessage(pao, phm, phr);
			break;

		default:
			HW_Message(pao, phm, phr);
			break;
		}
		break;

	default:
		phr->wError = HPI_ERROR_INVALID_TYPE;
		break;
	}
}

////////////////////////////////////////////////////////////////////////////
// SUBSYSTEM

// create an adapter object and initialise it based on resource information
// passed in in the message
// **** NOTE - you cannot use this function AND the FindAdapters function at the
// same time, the application must use only one of them to get the adapters ******

static void SubSysCreateAdapter(HPI_ADAPTERS_LIST * adaptersList,
				HPI_MESSAGE * phm, HPI_RESPONSE * phr)
{
	HPI_ADAPTER_OBJ ao;	// create temp adapter obj, because we don't know what index yet
	u32 dwOsErrorCode;
	short nError = 0;

	HPI_DEBUG_LOG0(VERBOSE, "SubSysCreateAdapter\n");

	memset(&ao, 0, sizeof(HPI_ADAPTER_OBJ));

	if (phm->u.s.Resource.wBusType != HPI_BUS_PCI)
		return;
	if (phm->u.s.Resource.r.Pci->wVendorId != HPI_PCI_VENDOR_ID_MOTOROLA)
		return;

	ao.priv = HpiOs_MemAlloc(sizeof(HPI_HW_OBJ));
	memset(ao.priv, 0, sizeof(HPI_HW_OBJ));
// create the adapter object based on the resource information
//? memcpy(&ao.Pci,&phm->u.s.Resource.r.Pci,sizeof(ao.Pci));
	ao.Pci = *phm->u.s.Resource.r.Pci;

	((HPI_HW_OBJ *) ao.priv)->hpi56301info.pMemBase = ao.Pci.apMemBase[0];

	nError = CreateAdapterObj(&ao, &dwOsErrorCode);
	if (nError) {
		phr->u.s.dwData = dwOsErrorCode;	// OS error, if any, is in dwOsErrorCode
		HpiOs_MemFree(ao.priv);
		phr->wError = nError;
		return;
	}
// add to adapter list - but don't allow two adapters of same number!
	if (phr->u.s.awAdapterList[ao.wIndex] != 0) {
		HpiOs_MemFree(ao.priv);
		phr->wError = HPI_DUPLICATE_ADAPTER_NUMBER;
		return;
	}
//? memcpy( &adaptersList->adapter[ ao.wIndex ], &ao, sizeof(HPI_ADAPTER_OBJ));
	adaptersList->adapter[ao.wIndex] = ao;

	HpiOs_Dsplock_Init(&adaptersList->adapter[ao.wIndex]);

	adaptersList->gwNumAdapters++;	// inc the number of adapters known by this HPI
	phr->u.s.awAdapterList[ao.wIndex] = ao.wAdapterType;
	phr->u.s.wAdapterIndex = ao.wIndex;
	phr->u.s.wNumAdapters++;	// add the number of adapters recognised by this HPI to the system total
	phr->wError = 0;	// the function completed OK;
}

static void SubSysDeleteAdapter(HPI_ADAPTERS_LIST * adaptersList,
				HPI_MESSAGE * phm, HPI_RESPONSE * phr)
{
	HPI_ADAPTER_OBJ *pao = NULL;

	pao = FindAdapter(adaptersList, phm->wAdapterIndex);
	if (!pao)
		return;		// message probably meant for another HPI module

	adaptersList->gwNumAdapters--;
	HpiOs_MemFree(pao->priv);
	memset(pao, 0, sizeof(HPI_ADAPTER_OBJ));

	phr->wError = 0;
}

// this routine is called from SubSysFindAdapter and SubSysCreateAdapter

static short CreateAdapterObj(HPI_ADAPTER_OBJ * pao, u32 * pdwOsErrorCode)
{
	short nBootError = 0;

	pao->wOpen = 0;

// turn on the adapters memory address decoding (in PCI config space)
// also enable parity error responses and bus mastering
	HpiPci_WriteConfig(&pao->Pci, HPIPCI_CCMR,
			   HPIPCI_CCMR_MSE | HPIPCI_CCMR_PERR | HPIPCI_CCMR_BM);

// Is it really a 301 chip?
	if (Hpi56301_CheckAdapterPresent
	    (&((HPI_HW_OBJ *) pao->priv)->hpi56301info)) {
//phr->wSpecificError = 1;
		return (HPI_ERROR_BAD_ADAPTER);	//error
	}
	HPI_DEBUG_LOG0(VERBOSE, "CreateAdapterObj - Adapter present OK\n");

	if (0 !=
	    (nBootError =
	     Hpi56301_BootLoadDsp(pao,
				  &((HPI_HW_OBJ *) pao->priv)->hpi56301info,
				  pdwOsErrorCode))) {
//phr->wSpecificError = 2;
		return (nBootError);	//error
	}
	HPI_DEBUG_LOG0(INFO, "Bootload DSP OK\n");
#if 0
/* Hpi56301_SelfTest doesnt do anything */
	if (Hpi56301_SelfTest(&((HPI_HW_OBJ *) pao->priv)->hpi56301info)) {
//phr->wSpecificError = 3;
		return (HPI_ERROR_DSP_SELFTEST);	//error
	}
#endif

// check that the code got into the adapter Ok and is running by
// getting back a checksum and comparing it to a local checksum
// ** TODO **

// get info about the adapter by asking the adapter
// send a HPI_ADAPTER_GET_INFO message

	{
		HPI_MESSAGE hM;
		HPI_RESPONSE hR;

		HPIOS_DEBUG_STRING
		    ("CreateAdapterObj - Send ADAPTER_GET_INFO\n");
		memset(&hM, 0, sizeof(HPI_MESSAGE));
		hM.wType = HPI_TYPE_MESSAGE;
		hM.wSize = sizeof(HPI_MESSAGE);
		hM.wObject = HPI_OBJ_ADAPTER;
		hM.wFunction = HPI_ADAPTER_GET_INFO;
		hM.wAdapterIndex = 0;
		memset(&hR, 0, sizeof(HPI_RESPONSE));
		hR.wSize = sizeof(HPI_RESPONSE);

		Hpi56301_Message(&((HPI_HW_OBJ *) pao->priv)->hpi56301info, &hM,
				 &hR);

		if (hR.wError) {
			HPIOS_DEBUG_STRING("HPI4000.C - message error\n");
			return (hR.wError);	//error
		}

		pao->wAdapterType = hR.u.a.wAdapterType;
		pao->wIndex = hR.u.a.wAdapterIndex;
	}
	HPIOS_DEBUG_STRING("CreateAdapterObj - Get adapter info OK\n");
	return (0);		//success!
}

///////////////////////////////////////////////////////////////////////////
