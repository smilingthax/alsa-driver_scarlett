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

Hardware Programming Interface (HPI) Manager
This module contains the main HPI entry point HPI_Message

******************************************************************************/
#include "hpi.h"
#include "hpios.h"

////////////////////////////////////////////////////////////////////////////
// Allow a wrapper called HPI_Message around this function
void hpi_message(const HPI_HSUBSYS * phSubSys, HPI_MESSAGE * phm,
		 HPI_RESPONSE * phr)
{

// All HPI messages come here 1st, before being sent to individual HPI
// modules
	(void)phSubSys;		//get rid of warning

/* Avoid garbage being used for wSize if not filled in correctly
check if wSize has been filled in at the end of this function.
Response size must be set by the generator of the response.
*/
	phr->wSize = 0;

// check for various global messages
	if (phm->wObject == HPI_OBJ_SUBSYSTEM) {
// subsys message get sent to more than one hpi, may not be filled in by any of them
// so need to create a default response here
// HPI_SUBSYS_CREATE_ADAPTER is a special case where the response from previous hpi
// calls is passed in to facilitate duplicate adapter index detection, so don't init
		if (phm->wObject == HPI_OBJ_SUBSYSTEM
		    && phm->wFunction == HPI_SUBSYS_CREATE_ADAPTER)
			phr->wSize =
			    HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_SUBSYS_RES);
		else
			HPI_InitResponse(phr, phm->wObject, phm->wFunction,
					 HPI_ERROR_INVALID_OBJ);

		switch (phm->wFunction) {
// subsys open - init the debug function
		case HPI_SUBSYS_OPEN:
			HPIOS_DEBUG_STRING(" HPI_SUBSYS_OPEN\n");
			break;	// continue to HPIs...

		case HPI_SUBSYS_FIND_ADAPTERS:
			phr->wError = 0;	// special case - find adapters DOES NOT overwrite wError.
			break;

		case HPI_SUBSYS_CLOSE:
			HPIOS_DEBUG_STRING(" HPI_SUBSYS_CLOSE\n");
			break;	// continue to HPIs...

// version message - if so then leave
		case HPI_SUBSYS_GET_VERSION:
			HPIOS_DEBUG_STRING(" HPI_SUBSYS_GET_VERSION\n");
			HPI_InitResponse(phr, HPI_OBJ_SUBSYSTEM,
					 HPI_SUBSYS_GET_VERSION, 0);
			phr->u.s.dwVersion = HPI_VER;
			return;

// generic read/write i/o ports
		case HPI_SUBSYS_WRITE_PORT_8:
			HOUT8((u16) phm->u.s.Resource.r.PortIO.dwAddress,
			      (u8) phm->u.s.Resource.r.PortIO.dwData);
			HPI_InitResponse(phr, HPI_OBJ_SUBSYSTEM,
					 HPI_SUBSYS_WRITE_PORT_8, 0);
			return;

		case HPI_SUBSYS_READ_PORT_8:
			HPI_InitResponse(phr, HPI_OBJ_SUBSYSTEM,
					 HPI_SUBSYS_READ_PORT_8, 0);
			phr->u.s.dwData =
			    (u32) HINP8((u16) phm->u.s.Resource.r.PortIO.
					dwAddress);
			return;

		default:
			break;
		}
	}

// call each HPI in turn
#ifdef HPI_INCLUDE_8400
	HPI_8400(phm, phr);	// A840X series boards - 4  record
//HPI_8411( phm, phr );             // A8411
#endif

#if ( defined ( HPI_INCLUDE_4100 ) + defined ( HPI_INCLUDE_4300 ) + defined ( HPI_INCLUDE_4400 ) + defined ( HPI_INCLUDE_4500 ) + defined ( HPI_INCLUDE_4600 ) )
	HPI_4000(phm, phr);
#endif

//#ifdef HPI_INCLUDE_6000
#if ( defined ( HPI_INCLUDE_6000 ) + defined ( HPI_INCLUDE_8800 ) + defined ( HPI_INCLUDE_8600 ) )
	HPI_6000(phm, phr);	// ASI6000, ASI8800 series boards - TI C6711 based
#endif

#if ( defined ( HPI_INCLUDE_5000 ) + defined ( HPI_INCLUDE_6400 ) + defined ( HPI_INCLUDE_8700 ) )
	HPI_6205(phm, phr);	// ASI5000/ASI8700, TI C6205 based
#endif

#if ( defined ( HPI_INCLUDE_USB ) )
	HPI_Usb(phm, phr);
#endif

	if (phr->wSize == 0) {	// This should not happen. If it does this is a coding error.
// However, still fill in enough of the response to return an error to help debugging.
		phr->wError = HPI_ERROR_PROCESSING_MESSAGE;
		phr->wSize = HPI_RESPONSE_FIXED_SIZE;
	}

}

////////////////////////////////////////////////////////////////////////////////
