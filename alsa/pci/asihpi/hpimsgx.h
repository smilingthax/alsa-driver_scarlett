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

HPI Extended Message Handler Functions

(C) Copyright AudioScience Inc. 1997-2003
******************************************************************************/

#ifndef _HPIMSGX_H_
#define _HPIMSGX_H_

#include "hpi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HPIMSGX_ALLADAPTERS     (0xFFFF)

	void HPIMSGX_MessageExAdapterReset(u16 wAdapterIndex);

	u16 HPIMSGX_MessageExAdapterInit(const HPI_HSUBSYS * phSubSysHandle, HPI_MESSAGE * phm,	// HPI_SUBSYS_CREATE_ADAPTER structure with
// resource list or NULL=find all
					 HPI_RESPONSE * phr	// response from HPI_ADAPTER_GET_INFO
	    );

	void HPIMSGX_MessageEx(const HPI_HSUBSYS * phSubSys,
			       HPI_MESSAGE * phm,
			       HPI_RESPONSE * phr, void *hOwner);

	void HPIMSGX_MessageExAdapterCleanup(u16 wAdapterIndex, void *hOwner);

#ifdef __cplusplus
}
#endif
#endif				/* _HPIMSGX_H_ */
///////////////////////////////////////////////////////////////////////////
