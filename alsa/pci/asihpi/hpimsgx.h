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
extern "C"
{
#endif

#define HPIMSGX_ALLADAPTERS	(0xFFFF)

    void HPI_MessageEx(
        HPI_MESSAGE *phm,
        HPI_RESPONSE *phr,
        void * hOwner
    );

#ifdef __cplusplus
}
#endif

#endif /* _HPIMSGX_H_ */

///////////////////////////////////////////////////////////////////////////
