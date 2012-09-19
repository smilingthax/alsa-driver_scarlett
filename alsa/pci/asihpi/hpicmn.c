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

Common functions used by hpixxxx.c modules

(C) Copyright AudioScience Inc. 1998-2003
*******************************************************************************/

#include "hpi.h"
#include "hpidebug.h"
#include "hpicmn.h"
#include "hpicheck.h"

/**
* FindAdapter returns a pointer to the HPI_ADAPTER_OBJ with index wAdapterIndex
* in an HPI_ADAPTERS_LIST structure.
*
**/
HPI_ADAPTER_OBJ *FindAdapter(HPI_ADAPTERS_LIST * adaptersList,
			     u16 wAdapterIndex)
{
	HPI_ADAPTER_OBJ *pao = NULL;

	if (wAdapterIndex >= HPI_MAX_ADAPTERS) {
		HPI_DEBUG_LOG1(VERBOSE, "FindAdapter invalid index %d ",
			       wAdapterIndex);
		return NULL;
	}

	pao = &adaptersList->adapter[wAdapterIndex];
	if (pao->wAdapterType != 0) {
		HPI_DEBUG_LOG1(VERBOSE, "Found adapter index %d\n",
			       wAdapterIndex);
		return (pao);
	} else {
		HPI_DEBUG_LOG1(VERBOSE, "No adapter index %d\n", wAdapterIndex);
		return (NULL);
	}
}

/**
*
* wipe an HPI_ADAPTERS_LIST structure.
*
**/
void WipeAdapterList(HPI_ADAPTERS_LIST * adaptersList)
{
	memset(adaptersList, 0, sizeof(HPI_ADAPTERS_LIST));
}

/**
*
* SubSysGetAdapters fills awAdapterList in an HPI_RESPONSE structure with all
* adapters in the given HPI_ADAPTERS_LIST.
*
**/
void SubSysGetAdapters(HPI_ADAPTERS_LIST * adaptersList, HPI_RESPONSE * phr)
{
// fill in the response adapter array with the position
// identified by the adapter number/index of the adapters in
// this HPI
// i.e. if we have an A120 with it's jumper set to
// Adapter Number 2 then put an Adapter type A120 in the
// array in position 1
// NOTE: AdapterNumber is 1..N, Index is 0..N-1

// input:  NONE
// output: wNumAdapters
//         awAdapter[]
//

	short i;
	HPI_ADAPTER_OBJ *pao = NULL;

	HPI_DEBUG_LOG0(VERBOSE, "SubSysGetAdapters\n");

// for each adapter, place it's type in the position of the array
// corresponding to it's adapter number
	for (i = 0; i < adaptersList->gwNumAdapters; i++) {
		pao = &adaptersList->adapter[i];
		if (phr->u.s.awAdapterList[pao->wIndex] != 0) {
			phr->wError = HPI_DUPLICATE_ADAPTER_NUMBER;
			return;
		}
		phr->u.s.awAdapterList[pao->wIndex] = pao->wAdapterType;
	}

// add the number of adapters recognised by this HPI to the system total
	phr->u.s.wNumAdapters += adaptersList->gwNumAdapters;
	phr->wError = 0;	// the function completed OK;
}

/**
*
* CheckControlCache checks if a given tHPIControlCacheSingle control value is in the cache and fills the HPI_RESPONSE
* accordingly in case of a cache hit. It returns nonzero if a cache hit occurred, zero otherwise.
*
**/
short CheckControlCache(volatile tHPIControlCacheSingle * pC, HPI_MESSAGE * phm,
			HPI_RESPONSE * phr)
{
	short found = 1;
// if the control type in the cache is non-zero then we have cached control information to process
	phr->wSize = HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_CONTROL_RES);
	phr->wError = 0;
	switch (pC->ControlType) {
	case HPI_CONTROL_METER:
		if (phm->u.c.wAttribute == HPI_METER_PEAK) {
			phr->u.c.anLogValue[0] = pC->u.p.anLogPeak[0];
			phr->u.c.anLogValue[1] = pC->u.p.anLogPeak[1];
		} else if (phm->u.c.wAttribute == HPI_METER_RMS) {
			phr->u.c.anLogValue[0] = pC->u.p.anLogRMS[0];
			phr->u.c.anLogValue[1] = pC->u.p.anLogRMS[1];
		} else
			found = 0;	// signal that message was not cached
		break;
	case HPI_CONTROL_VOLUME:
		if (phm->u.c.wAttribute == HPI_VOLUME_GAIN) {
			phr->u.c.anLogValue[0] = pC->u.v.anLog[0];
			phr->u.c.anLogValue[1] = pC->u.v.anLog[1];
		} else
			found = 0;	// signal that message was not cached
		break;
	case HPI_CONTROL_MULTIPLEXER:
		if (phm->u.c.wAttribute == HPI_MULTIPLEXER_SOURCE) {
			phr->u.c.dwParam1 = pC->u.x.wSourceNodeType;
			phr->u.c.dwParam2 = pC->u.x.wSourceNodeIndex;
		} else
			found = 0;	// signal that message was not cached
		break;
	case HPI_CONTROL_LEVEL:
		if (phm->u.c.wAttribute == HPI_LEVEL_GAIN) {
			phr->u.c.anLogValue[0] = pC->u.l.anLog[0];
			phr->u.c.anLogValue[1] = pC->u.l.anLog[1];
		} else
			found = 0;	// signal that message was not cached
		break;
	case HPI_CONTROL_TUNER:
		if (phm->u.c.wAttribute == HPI_TUNER_FREQ)
			phr->u.c.dwParam1 = pC->u.t.dwFreqInkHz;
		else if (phm->u.c.wAttribute == HPI_TUNER_BAND)
			phr->u.c.dwParam1 = pC->u.t.wBand;
		else if ((phm->u.c.wAttribute == HPI_TUNER_LEVEL) &&
			 (phm->u.c.dwParam1 == HPI_TUNER_LEVEL_AVERAGE))
			phr->u.c.dwParam1 = pC->u.t.wLevel;
		else
			found = 0;	// signal that message was not cached
		break;
	case HPI_CONTROL_AESEBU_RECEIVER:
		if (phm->u.c.wAttribute == HPI_AESEBU_ERRORSTATUS)
			phr->u.c.dwParam1 = pC->u.aes3rx.dwErrorStatus;
		else if (phm->u.c.wAttribute == HPI_AESEBU_SOURCE)
			phr->u.c.dwParam1 = pC->u.aes3rx.dwSource;
		else
			found = 0;	// signal that message was not cached
		break;
	case HPI_CONTROL_TONEDETECTOR:
		if (phm->u.c.wAttribute == HPI_TONEDETECTOR_STATE)
			phr->u.c.dwParam1 = pC->u.tone.wState;
		else
			found = 0;	// signal that message was not cached
		break;
	case HPI_CONTROL_SILENCEDETECTOR:
		if (phm->u.c.wAttribute == HPI_SILENCEDETECTOR_STATE) {
			phr->u.c.dwParam1 = pC->u.silence.dwState;
			phr->u.c.dwParam2 = pC->u.silence.dwCount;
		} else
			found = 0;	// signal that message was not cached
		break;
	default:
		found = 0;	// signal that message was not cached
		break;
	}
	return found;
}

///////////////////////////////////////////////////////////////////////////
