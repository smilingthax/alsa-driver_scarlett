/******************************************************************************
Copyright (C) 1997-2003 AudioScience, Inc. All rights reserved.

This software is provided 'as-is', without any express or implied warranty.
In no event will AudioScience Inc. be held liable for any damages arising
from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This copyright notice and list of conditions may not be altered or removed 
   from any source distribution.

AudioScience, Inc. <support@audioscience.com>

( This license is GPL compatible see http://www.gnu.org/licenses/license-list.html#GPLCompatibleLicenses )

HPI Operating System function declarations

(C) Copyright AudioScience Inc. 1997-2003
******************************************************************************/
#ifndef _HPIOS_H_
#define _HPIOS_H_

#include "hpi.h"


/////////////////////////// MACROS ///////////////////////////////////////
#ifdef HPI_DEBUG
	#define HPIOS_DEBUG_INIT()	HpiOs_DebugInit()
	#define HPIOS_DEBUG_STRING(s)	HpiOs_DebugString(s)
	#define HPIOS_DEBUG_DWORD(d)	HpiOs_DebugDword(d)
#else
	#define HPIOS_DEBUG_INIT()
	#define HPIOS_DEBUG_STRING(s)
	#define HPIOS_DEBUG_DWORD(d)
#endif

#include "hpios_linux.h"


/////////////////////////// PROTOTYPES ///////////////////////////////////

// monochrome screen debug functions
void HpiOs_DebugInit(void);
void HpiOs_DebugString( char *pszString );	// print a string to the monochrome display
void HpiOs_DebugDword( HW32 dwDword );

// memory allocation
HW16 HpiOs_AllocLockedMemory( HW32 dwSize, void **ppvLinear, HW32 *pPhysical );
HW16 HpiOs_FreeLockedMemory( void *pvLinear );

// physical memory allocation
HW16 HpiOs_LockedMem_Alloc( HpiOs_LockedMem_Handle *pLockedMemHandle, HW32 dwSize, void *pOsReference );
HW16 HpiOs_LockedMem_Free( HpiOs_LockedMem_Handle LockedMemHandle );
HW16 HpiOs_LockedMem_GetPhysAddr( HpiOs_LockedMem_Handle LockedMemHandle, HW32 *pPhysicalAddr );
HW16 HpiOs_LockedMem_GetVirtAddr( HpiOs_LockedMem_Handle LockedMemHandle, void **ppvVirtualAddr );

// memory read/write
HW32 HpiOs_MemRead32( HW32 dwAddress );
void HpiOs_MemWrite32(HW32 dwAddress, HW32 dwData );

// port I/O
void HpiOs_OutBuf8( HW16 wDataPort, void *pbBuffer, HW16 wLength );

// timing/delay
void HpiOs_DelayMicroSeconds( HW32 dwNumMicroSec );

char *HpiOs_GetDspCodePath(void);

#endif //_HPIOS_H_

///////////////////////////////////////////////////////////////////////////
