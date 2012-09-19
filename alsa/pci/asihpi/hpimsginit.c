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

 Hardware Programming Interface (HPI) Utility functions.

 (C) Copyright AudioScience Inc. 2007
*******************************************************************************/

#include "hpi_internal.h"
#include "hpimsginit.h"

/* The actual message size for each object type */
static u16 aMsgSize[HPI_OBJ_MAXINDEX + 1] = HPI_MESSAGE_SIZE_BY_OBJECT;
/* The actual response size for each object type */
static u16 aResSize[HPI_OBJ_MAXINDEX + 1] = HPI_RESPONSE_SIZE_BY_OBJECT;
/* Flag to enable alternate message type for SSX2 bypass. */
static u16 gwSSX2Bypass;

/** \internal
  * Used by ASIO driver to disable SSX2 for a single process
  * \param phSubSys Pointer to HPI subsystem handle.
  * \param wBypass New bypass setting 0 = off, nonzero = on
  * \return Previous bypass setting.
  */
u16 HPI_SubSysSsx2Bypass(
	const struct hpi_hsubsys *phSubSys,
	u16 wBypass
)
{
	u16 oldValue = gwSSX2Bypass;

	gwSSX2Bypass = wBypass;

	return oldValue;
}

/** \internal
  * initialize the HPI message structure
  */
static void HPI_InitMessage(
	struct hpi_message *phm,
	u16 wObject,
	u16 wFunction
)
{
	memset(phm, 0, sizeof(*phm));
	if ((wObject > 0) && (wObject <= HPI_OBJ_MAXINDEX))
		phm->wSize = aMsgSize[wObject];
	else
		phm->wSize = sizeof(*phm);

	if (gwSSX2Bypass)
		phm->wType = HPI_TYPE_SSX2BYPASS_MESSAGE;
	else
		phm->wType = HPI_TYPE_MESSAGE;
	phm->wObject = wObject;
	phm->wFunction = wFunction;
	phm->version = 0;
	/* Expect adapter index to be set by caller */
}

/** \internal
  * initialize the HPI response structure
  */
void HPI_InitResponse(
	struct hpi_response *phr,
	u16 wObject,
	u16 wFunction,
	u16 wError
)
{
	memset(phr, 0, sizeof(*phr));
	phr->wType = HPI_TYPE_RESPONSE;
	if ((wObject > 0) && (wObject <= HPI_OBJ_MAXINDEX))
		phr->wSize = aResSize[wObject];
	else
		phr->wSize = sizeof(*phr);
	phr->wObject = wObject;
	phr->wFunction = wFunction;
	phr->wError = wError;
	phr->wSpecificError = 0;
	phr->version = 0;
}

void HPI_InitMessageResponse(
	struct hpi_message *phm,
	struct hpi_response *phr,
	u16 wObject,
	u16 wFunction
)
{
	HPI_InitMessage(phm, wObject, wFunction);
	/* default error return if the response is
	   not filled in by the callee */
	HPI_InitResponse(phr, wObject, wFunction,
		HPI_ERROR_PROCESSING_MESSAGE);
}

static void HPI_InitMessageV1(
	struct hpi_message_header *phm,
	u16 wSize,
	u16 wObject,
	u16 wFunction
)
{
	memset(phm, 0, sizeof(*phm));
	if ((wObject > 0) && (wObject <= HPI_OBJ_MAXINDEX)) {
		phm->wSize = wSize;
		phm->wType = HPI_TYPE_MESSAGE;
		phm->wObject = wObject;
		phm->wFunction = wFunction;
		phm->version = 1;
		/* Expect adapter index to be set by caller */
	}
}

void HPI_InitResponseV1(
	struct hpi_response_header *phr,
	u16 wSize,
	u16 wObject,
	u16 wFunction
)
{
	memset(phr, 0, sizeof(*phr));
	phr->wSize = wSize;
	phr->version = 1;
	phr->wType = HPI_TYPE_RESPONSE;
	phr->wError = HPI_ERROR_PROCESSING_MESSAGE;
}

void HPI_InitMessageResponseV1(
	struct hpi_message_header *phm,
	u16 wMsgSize,
	struct hpi_response_header *phr,
	u16 wResSize,
	u16 wObject,
	u16 wFunction
)
{
	HPI_InitMessageV1(phm, wMsgSize, wObject, wFunction);
	HPI_InitResponseV1(phr, wResSize, wObject, wFunction);
}
