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

\file hpicmn.c

 Common functions used by hpixxxx.c modules

(C) Copyright AudioScience Inc. 1998-2003
*******************************************************************************/
#define SOURCEFILE_NAME "hpicmn.c"

#include "hpi_internal.h"
#include "hpidebug.h"
#include "hpicmn.h"

struct hpi_adapters_list {
	struct hpios_spinlock aListLock;
	struct hpi_adapter_obj adapter[HPI_MAX_ADAPTERS];
	u16 gwNumAdapters;
};

static struct hpi_adapters_list adapters;

/**
* Given an HPI Message that was sent out and a response that was received,
* validate that the response has the correct fields filled in,
* i.e ObjectType, Function etc
**/
u16 HpiValidateResponse(
	struct hpi_message *phm,
	struct hpi_response *phr
)
{
	u16 wError = 0;

	if ((phr->wType != HPI_TYPE_RESPONSE)
		|| (phr->wObject != phm->wObject)
		|| (phr->wFunction != phm->wFunction))
		wError = HPI_ERROR_INVALID_RESPONSE;

	return wError;
}

u16 HpiAddAdapter(
	struct hpi_adapter_obj *pao
)
{
	u16 retval = 0;
	/*HPI_ASSERT(pao->wAdapterType); */

	HpiOs_Alistlock_Lock(&adapters);

	if (pao->wIndex >= HPI_MAX_ADAPTERS) {
		retval = HPI_ERROR_BAD_ADAPTER_NUMBER;
		goto unlock;
	}

	if (adapters.adapter[pao->wIndex].wAdapterType) {
		{
			retval = HPI_DUPLICATE_ADAPTER_NUMBER;
			goto unlock;
		}
	}
	adapters.adapter[pao->wIndex] = *pao;
	HpiOs_Dsplock_Init(&adapters.adapter[pao->wIndex]);
	adapters.gwNumAdapters++;

unlock:
	HpiOs_Alistlock_UnLock(&adapters);
	return retval;
}

void HpiDeleteAdapter(
	struct hpi_adapter_obj *pao
)
{
	memset(pao, 0, sizeof(struct hpi_adapter_obj));

	HpiOs_Alistlock_Lock(&adapters);
	adapters.gwNumAdapters--;	/* dec the number of adapters */
	HpiOs_Alistlock_UnLock(&adapters);
}

/**
* FindAdapter returns a pointer to the struct hpi_adapter_obj with
* index wAdapterIndex in an HPI_ADAPTERS_LIST structure.
*
*/
struct hpi_adapter_obj *HpiFindAdapter(
	u16 wAdapterIndex
)
{
	struct hpi_adapter_obj *pao = NULL;

	if (wAdapterIndex >= HPI_MAX_ADAPTERS) {
		HPI_DEBUG_LOG(VERBOSE,
			"FindAdapter invalid index %d ", wAdapterIndex);
		return NULL;
	}

	pao = &adapters.adapter[wAdapterIndex];
	if (pao->wAdapterType != 0) {
		/*
		   HPI_DEBUG_LOG(VERBOSE, "Found adapter index %d\n",
		   wAdapterIndex);
		 */
		return (pao);
	} else {
		/*
		   HPI_DEBUG_LOG(VERBOSE, "No adapter index %d\n",
		   wAdapterIndex);
		 */
		return (NULL);
	}
}

/**
*
* wipe an HPI_ADAPTERS_LIST structure.
*
**/
static void WipeAdapterList(
	void
)
{
	memset(&adapters, 0, sizeof(adapters));
}

/**
* SubSysGetAdapters fills awAdapterList in an struct hpi_response structure
* with all adapters in the given HPI_ADAPTERS_LIST.
*
*/
static void SubSysGetAdapters(
	struct hpi_response *phr
)
{
	/* fill in the response adapter array with the position */
	/* identified by the adapter number/index of the adapters in */
	/* this HPI */
	/* i.e. if we have an A120 with it's jumper set to */
	/* Adapter Number 2 then put an Adapter type A120 in the */
	/* array in position 1 */
	/* NOTE: AdapterNumber is 1..N, Index is 0..N-1 */

	/* input:  NONE */
	/* output: wNumAdapters */
	/*                 awAdapter[] */
	/* */

	short i;
	struct hpi_adapter_obj *pao = NULL;

	HPI_DEBUG_LOG(VERBOSE, "SubSysGetAdapters\n");

	/* for each adapter, place it's type in the position of the array */
	/* corresponding to it's adapter number */
	for (i = 0; i < adapters.gwNumAdapters; i++) {
		pao = &adapters.adapter[i];
		if (phr->u.s.awAdapterList[pao->wIndex] != 0) {
			phr->wError = HPI_DUPLICATE_ADAPTER_NUMBER;
			phr->wSpecificError = pao->wIndex;
			return;
		}
		phr->u.s.awAdapterList[pao->wIndex] = pao->wAdapterType;
	}

	phr->u.s.wNumAdapters = adapters.gwNumAdapters;
	phr->wError = 0;	/* the function completed OK; */
}

static unsigned int ControlCacheAllocCheck(
	struct hpi_control_cache *pC
)
{
	unsigned int i;
	int nCached = 0;
	if (!pC)
		return 0;
	if ((!pC->dwInit) &&
		(pC->pCache != NULL) &&
		(pC->dwControlCount) && (pC->dwCacheSizeInBytes)
		) {
		u32 *pMasterCache;
		pC->dwInit = 1;

		pMasterCache = (u32 *)pC->pCache;
		HPI_DEBUG_LOG(VERBOSE, "check %d controls\n",
			pC->dwControlCount);
		for (i = 0; i < pC->dwControlCount; i++) {
			struct hpi_control_cache_info *info =
				(struct hpi_control_cache_info *)pMasterCache;

			if (info->ControlType) {
				pC->pInfo[i] = info;
				nCached++;
			} else
				pC->pInfo[i] = NULL;

			if (info->nSizeIn32bitWords)
				pMasterCache += info->nSizeIn32bitWords;
			else
				pMasterCache +=
					sizeof(struct
					hpi_control_cache_single) /
					sizeof(u32);

			HPI_DEBUG_LOG(VERBOSE,
				"nCached %d, pinfo %p index %d type %d\n",
				nCached, pC->pInfo[i], info->ControlIndex,
				info->ControlType);
		}
		/*
		   We didn't find anything to cache, so try again later !
		 */
		if (!nCached)
			pC->dwInit = 0;
	}
	return pC->dwInit;
}

/** Find a control.
*/
static short FindControl(
	struct hpi_message *phm,
	struct hpi_control_cache *pCache,
	struct hpi_control_cache_info **pI,
	u16 *pwControlIndex
)
{
	*pwControlIndex = phm->wObjIndex;

	if (!ControlCacheAllocCheck(pCache)) {
		HPI_DEBUG_LOG(VERBOSE,
			"ControlCacheAllocCheck() failed. adap%d ci%d\n",
			phm->wAdapterIndex, *pwControlIndex);
		return 0;
	}

	*pI = pCache->pInfo[*pwControlIndex];
	if (!*pI) {
		HPI_DEBUG_LOG(VERBOSE,
			"Uncached Adap %d, Control %d\n",
			phm->wAdapterIndex, *pwControlIndex);
		return 0;
	} else {
		HPI_DEBUG_LOG(VERBOSE,
			"FindControl() Type %d\n", (*pI)->ControlType);
	}
	return 1;
}

/** Used by the kernel driver to figure out if a buffer needs mapping.
 */
short HpiCheckBufferMapping(
	struct hpi_control_cache *pCache,
	struct hpi_message *phm,
	void **p,
	unsigned int *pN
)
{
	*pN = 0;
	*p = NULL;
	if ((phm->wFunction == HPI_CONTROL_GET_STATE) &&
		(phm->wObject == HPI_OBJ_CONTROLEX)
		) {
		u16 wControlIndex;
		struct hpi_control_cache_info *pI;

		if (!FindControl(phm, pCache, &pI, &wControlIndex))
			return 0;
	}
	return 0;
}

/* allow unified treatment of several string fields within struct */
#define HPICMN_PAD_OFS_AND_SIZE(m)  {\
	offsetof(struct hpi_control_cache_pad, m), \
	sizeof(((struct hpi_control_cache_pad *)(NULL))->m) }

struct pad_ofs_size {
	unsigned int offset;
	unsigned int field_size;
};

static struct pad_ofs_size aPadDesc[] = {
	HPICMN_PAD_OFS_AND_SIZE(cChannel),	/* HPI_PAD_CHANNEL_NAME */
	HPICMN_PAD_OFS_AND_SIZE(cArtist),	/* HPI_PAD_ARTIST */
	HPICMN_PAD_OFS_AND_SIZE(cTitle),	/* HPI_PAD_TITLE */
	HPICMN_PAD_OFS_AND_SIZE(cComment),	/* HPI_PAD_COMMENT */
};

/** CheckControlCache checks the cache and fills the struct hpi_response
 * accordingly. It returns one if a cache hit occurred, zero otherwise.
 */
short HpiCheckControlCache(
	struct hpi_control_cache *pCache,
	struct hpi_message *phm,
	struct hpi_response *phr
)
{
	short found = 1;
	u16 wControlIndex;
	struct hpi_control_cache_info *pI;
	struct hpi_control_cache_single *pC;
	struct hpi_control_cache_pad *pPad;

	if (!FindControl(phm, pCache, &pI, &wControlIndex))
		return 0;

	phr->wError = 0;

	/* pC is the default cached control strucure. May be cast to
	   something else in the following switch statement.
	 */
	pC = (struct hpi_control_cache_single *)pI;
	pPad = (struct hpi_control_cache_pad *)pI;

	switch (pI->ControlType) {

	case HPI_CONTROL_METER:
		if (phm->u.c.wAttribute == HPI_METER_PEAK) {
			phr->u.c.anLogValue[0] = pC->u.p.anLogPeak[0];
			phr->u.c.anLogValue[1] = pC->u.p.anLogPeak[1];
		} else if (phm->u.c.wAttribute == HPI_METER_RMS) {
			phr->u.c.anLogValue[0] = pC->u.p.anLogRMS[0];
			phr->u.c.anLogValue[1] = pC->u.p.anLogRMS[1];
		} else
			found = 0;
		break;
	case HPI_CONTROL_VOLUME:
		if (phm->u.c.wAttribute == HPI_VOLUME_GAIN) {
			phr->u.c.anLogValue[0] = pC->u.v.anLog[0];
			phr->u.c.anLogValue[1] = pC->u.v.anLog[1];
		} else
			found = 0;
		break;
	case HPI_CONTROL_MULTIPLEXER:
		if (phm->u.c.wAttribute == HPI_MULTIPLEXER_SOURCE) {
			phr->u.c.dwParam1 = pC->u.x.wSourceNodeType;
			phr->u.c.dwParam2 = pC->u.x.wSourceNodeIndex;
		} else {
			found = 0;
		}
		break;
	case HPI_CONTROL_CHANNEL_MODE:
		if (phm->u.c.wAttribute == HPI_CHANNEL_MODE_MODE)
			phr->u.c.dwParam1 = pC->u.m.wMode;
		else
			found = 0;
		break;
	case HPI_CONTROL_LEVEL:
		if (phm->u.c.wAttribute == HPI_LEVEL_GAIN) {
			phr->u.c.anLogValue[0] = pC->u.l.anLog[0];
			phr->u.c.anLogValue[1] = pC->u.l.anLog[1];
		} else
			found = 0;
		break;
	case HPI_CONTROL_TUNER:
		{
			struct hpi_control_cache_single *pCT =
				(struct hpi_control_cache_single *)pI;
			if (phm->u.c.wAttribute == HPI_TUNER_FREQ)
				phr->u.c.dwParam1 = pCT->u.t.dwFreqInkHz;
			else if (phm->u.c.wAttribute == HPI_TUNER_BAND)
				phr->u.c.dwParam1 = pCT->u.t.wBand;
			else if ((phm->u.c.wAttribute == HPI_TUNER_LEVEL) &&
				(phm->u.c.dwParam1 ==
					HPI_TUNER_LEVEL_AVERAGE))
				phr->u.c.dwParam1 = pCT->u.t.wLevel;
			else
				found = 0;
		}
		break;
	case HPI_CONTROL_AESEBU_RECEIVER:
		if (phm->u.c.wAttribute == HPI_AESEBURX_ERRORSTATUS)
			phr->u.c.dwParam1 = pC->u.aes3rx.dwErrorStatus;
		else if (phm->u.c.wAttribute == HPI_AESEBURX_FORMAT)
			phr->u.c.dwParam1 = pC->u.aes3rx.dwSource;
		else
			found = 0;
		break;
	case HPI_CONTROL_AESEBU_TRANSMITTER:
		if (phm->u.c.wAttribute == HPI_AESEBUTX_FORMAT)
			phr->u.c.dwParam1 = pC->u.aes3tx.dwFormat;
		else
			found = 0;
		break;
	case HPI_CONTROL_TONEDETECTOR:
		if (phm->u.c.wAttribute == HPI_TONEDETECTOR_STATE)
			phr->u.c.dwParam1 = pC->u.tone.wState;
		else
			found = 0;
		break;
	case HPI_CONTROL_SILENCEDETECTOR:
		if (phm->u.c.wAttribute == HPI_SILENCEDETECTOR_STATE) {
			phr->u.c.dwParam1 = pC->u.silence.dwState;
			phr->u.c.dwParam2 = pC->u.silence.dwCount;
		} else
			found = 0;
		break;
	case HPI_CONTROL_SAMPLECLOCK:
		if (phm->u.c.wAttribute == HPI_SAMPLECLOCK_SOURCE)
			phr->u.c.dwParam1 = pC->u.clk.wSource;
		else if (phm->u.c.wAttribute == HPI_SAMPLECLOCK_SOURCE_INDEX) {
			if (pC->u.clk.wSourceIndex ==
				HPI_ERROR_ILLEGAL_CACHE_VALUE) {
				phr->u.c.dwParam1 = 0;
				phr->wError = HPI_ERROR_INVALID_OPERATION;
			} else
				phr->u.c.dwParam1 = pC->u.clk.wSourceIndex;
		} else if (phm->u.c.wAttribute == HPI_SAMPLECLOCK_SAMPLERATE)
			phr->u.c.dwParam1 = pC->u.clk.dwSampleRate;
		else
			found = 0;
		break;
	case HPI_CONTROL_PAD:

		if (!(pPad->dwFieldValidFlags &
				(1 << HPI_CTL_ATTR_INDEX(phm->u.c.
						wAttribute)))) {
			phr->wError = HPI_ERROR_INVALID_CONTROL_ATTRIBUTE;
			break;
		}

		if (phm->u.c.wAttribute == HPI_PAD_PROGRAM_ID)
			phr->u.c.dwParam1 = pPad->dwPI;
		else if (phm->u.c.wAttribute == HPI_PAD_PROGRAM_TYPE)
			phr->u.c.dwParam1 = pPad->dwPTY;
		else {
			unsigned int index =
				HPI_CTL_ATTR_INDEX(phm->u.c.wAttribute) - 1;
			unsigned int offset = phm->u.c.dwParam1;
			unsigned int pad_string_len, field_size;
			char *pad_string;
			unsigned int tocopy;

			HPI_DEBUG_LOG(VERBOSE,
				"PADS HPI_PADS_ %d\n", phm->u.c.wAttribute);

			if (index > ARRAY_SIZE(aPadDesc) - 1) {
				phr->wError =
					HPI_ERROR_INVALID_CONTROL_ATTRIBUTE;
				break;
			}

			pad_string = ((char *)pPad) + aPadDesc[index].offset;
			field_size = aPadDesc[index].field_size;
			/* Ensure null terminator */
			pad_string[field_size - 1] = 0;

			pad_string_len = strlen(pad_string) + 1;

			if (offset > pad_string_len) {
				phr->wError = HPI_ERROR_INVALID_CONTROL_VALUE;
				break;
			}

			tocopy = pad_string_len - offset;
			if (tocopy > sizeof(phr->u.cu.chars8.szData))
				tocopy = sizeof(phr->u.cu.chars8.szData);

			HPI_DEBUG_LOG(VERBOSE,
				"PADS memcpy(%d), offset %d \n", tocopy,
				offset);
			memcpy(phr->u.cu.chars8.szData, &pad_string[offset],
				tocopy);

			phr->u.cu.chars8.dwRemainingChars =
				pad_string_len - offset - tocopy;
		}
		break;
	default:
		found = 0;
		break;
	}

	if (found)
		HPI_DEBUG_LOG(VERBOSE,
			"Cached Adap %d, Ctl %d, Type %d, Attr %d\n",
			phm->wAdapterIndex, pI->ControlIndex,
			pI->ControlType, phm->u.c.wAttribute);
	else
		HPI_DEBUG_LOG(VERBOSE,
			"Uncached Adap %d, Ctl %d, Ctl type %d\n",
			phm->wAdapterIndex, pI->ControlIndex,
			pI->ControlType);

	if (found)
		phr->wSize =
			sizeof(struct hpi_response_header) +
			sizeof(struct hpi_control_res);

	return found;
}

/** Updates the cache with Set values.

Only update if no error.
Volume and Level return the limited values in the response, so use these
Multiplexer does so use sent values
*/
void HpiSyncControlCache(
	struct hpi_control_cache *pCache,
	struct hpi_message *phm,
	struct hpi_response *phr
)
{
	u16 wControlIndex;
	struct hpi_control_cache_single *pC;
	struct hpi_control_cache_info *pI;

	if (!FindControl(phm, pCache, &pI, &wControlIndex))
		return;

	/* pC is the default cached control strucure.
	   May be cast to something else in the following switch statement.
	 */
	pC = (struct hpi_control_cache_single *)pI;

	switch (pI->ControlType) {
	case HPI_CONTROL_VOLUME:
		if (phm->u.c.wAttribute == HPI_VOLUME_GAIN) {
			pC->u.v.anLog[0] = phr->u.c.anLogValue[0];
			pC->u.v.anLog[1] = phr->u.c.anLogValue[1];
		}
		break;
	case HPI_CONTROL_MULTIPLEXER:
		/* mux does not return its setting on Set command. */
		if (phr->wError)
			return;
		if (phm->u.c.wAttribute == HPI_MULTIPLEXER_SOURCE) {
			pC->u.x.wSourceNodeType = (u16)phm->u.c.dwParam1;
			pC->u.x.wSourceNodeIndex = (u16)phm->u.c.dwParam2;
		}
		break;
	case HPI_CONTROL_CHANNEL_MODE:
		/* mode does not return its setting on Set command. */
		if (phr->wError)
			return;
		if (phm->u.c.wAttribute == HPI_CHANNEL_MODE_MODE)
			pC->u.m.wMode = (u16)phm->u.c.dwParam1;
		break;
	case HPI_CONTROL_LEVEL:
		if (phm->u.c.wAttribute == HPI_LEVEL_GAIN) {
			pC->u.v.anLog[0] = phr->u.c.anLogValue[0];
			pC->u.v.anLog[1] = phr->u.c.anLogValue[1];
		}
		break;
	case HPI_CONTROL_AESEBU_TRANSMITTER:
		if (phr->wError)
			return;
		if (phm->u.c.wAttribute == HPI_AESEBUTX_FORMAT)
			pC->u.aes3tx.dwFormat = phm->u.c.dwParam1;
		break;
	case HPI_CONTROL_AESEBU_RECEIVER:
		if (phr->wError)
			return;
		if (phm->u.c.wAttribute == HPI_AESEBURX_FORMAT)
			pC->u.aes3rx.dwSource = phm->u.c.dwParam1;
		break;
	case HPI_CONTROL_SAMPLECLOCK:
		if (phr->wError)
			return;
		if (phm->u.c.wAttribute == HPI_SAMPLECLOCK_SOURCE)
			pC->u.clk.wSource = (u16)phm->u.c.dwParam1;
		else if (phm->u.c.wAttribute == HPI_SAMPLECLOCK_SOURCE_INDEX) {
			pC->u.clk.wSourceIndex = (u16)phm->u.c.dwParam1;
		} else if (phm->u.c.wAttribute == HPI_SAMPLECLOCK_SAMPLERATE)
			pC->u.clk.dwSampleRate = phm->u.c.dwParam1;
		break;
	default:
		break;
	}
}

struct hpi_control_cache *HpiAllocControlCache(
	const u32 dwNumberOfControls,
	const u32 dwSizeInBytes,
	struct hpi_control_cache_info *pDSPControlBuffer
)
{
	struct hpi_control_cache *pCache =
		kmalloc(sizeof(*pCache), GFP_KERNEL);
	pCache->dwCacheSizeInBytes = dwSizeInBytes;
	pCache->dwControlCount = dwNumberOfControls;
	pCache->pCache = (struct hpi_control_cache_single *)pDSPControlBuffer;
	pCache->dwInit = 0;
	pCache->pInfo =
		kmalloc(sizeof(*pCache->pInfo) * pCache->dwControlCount,
		GFP_KERNEL);
	return pCache;
}

void HpiFreeControlCache(
	struct hpi_control_cache *pCache
)
{
	if ((pCache->dwInit) && (pCache->pInfo)) {
		kfree(pCache->pInfo);
		pCache->pInfo = NULL;
		pCache->dwInit = 0;
		kfree(pCache);
	}
}

static void SubSysMessage(
	struct hpi_message *phm,
	struct hpi_response *phr
)
{

	switch (phm->wFunction) {
	case HPI_SUBSYS_OPEN:
	case HPI_SUBSYS_CLOSE:
	case HPI_SUBSYS_DRIVER_UNLOAD:
		phr->wError = 0;
		break;
	case HPI_SUBSYS_DRIVER_LOAD:
		WipeAdapterList();
		HpiOs_Alistlock_Init(&adapters);
		phr->wError = 0;
		break;
	case HPI_SUBSYS_GET_INFO:
		SubSysGetAdapters(phr);
		break;
	case HPI_SUBSYS_CREATE_ADAPTER:
	case HPI_SUBSYS_DELETE_ADAPTER:
		phr->wError = 0;
		break;
	default:
		phr->wError = HPI_ERROR_INVALID_FUNC;
		break;
	}
}

void HPI_COMMON(
	struct hpi_message *phm,
	struct hpi_response *phr
)
{
	switch (phm->wType) {
	case HPI_TYPE_MESSAGE:
		switch (phm->wObject) {
		case HPI_OBJ_SUBSYSTEM:
			SubSysMessage(phm, phr);
			break;
		}
		break;

	default:
		phr->wError = HPI_ERROR_INVALID_TYPE;
		break;
	}
}
