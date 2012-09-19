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

HPI Operating System function declarations

(C) Copyright AudioScience Inc. 1997-2003
******************************************************************************/
#ifndef _HPIOS_H_
#define _HPIOS_H_

#include "hpi.h"

/////////////////////////// MACROS ///////////////////////////////////////
#ifdef HPI_DEBUG
#define HPIOS_DEBUG_INIT()      HpiOs_DebugInit()
#define HPIOS_DEBUG_STRING(s)   HpiOs_DebugString(s)
#define HPIOS_DEBUG_DWORD(d)    HpiOs_DebugDword(d)
#else
#define HPIOS_DEBUG_INIT()
#define HPIOS_DEBUG_STRING(s)
#define HPIOS_DEBUG_DWORD(d)
#endif

#define _test

#if defined ( _test )
/*  was stripped by preprocessing scripts */
#else

#ifndef __GNUC__
#define  huge
#else
#undef
#define
#endif

#endif

#include "hpios_linux.h"

/////////////////////////// PROTOTYPES ///////////////////////////////////

// monochrome screen debug functions
void HpiOs_DebugInit(void);
void HpiOs_DebugString(char *pszString);	// print a string to the monochrome display
void HpiOs_DebugDword(u32 dwDword);

// memory allocation
u16 HpiOs_AllocLockedMemory(u32 dwSize, void **ppvLinear, u32 * pPhysical);
u16 HpiOs_FreeLockedMemory(void *pvLinear);

// physical memory allocation
#ifndef NO_HPIOS_LOCKEDMEM_OPS
u16 HpiOs_LockedMem_Alloc(HpiOs_LockedMem_Handle * pLockedMemHandle, u32 dwSize,
			  void *pOsReference);
u16 HpiOs_LockedMem_Free(HpiOs_LockedMem_Handle LockedMemHandle);
u16 HpiOs_LockedMem_GetPhysAddr(HpiOs_LockedMem_Handle LockedMemHandle,
				u32 * pPhysicalAddr);
u16 HpiOs_LockedMem_GetVirtAddr(HpiOs_LockedMem_Handle LockedMemHandle,
				void **ppvVirtualAddr);
#endif

// memory read/write
u32 HpiOs_MemRead32(u32 dwAddress);
void HpiOs_MemWrite32(u32 dwAddress, u32 dwData);

// port I/O
void HpiOs_OutBuf8(u16 wDataPort, void *pbBuffer, u16 wLength);

// timing/delay
void HpiOs_DelayMicroSeconds(u32 dwNumMicroSec);

#ifndef NO_HPIOS_FILE_OPS
#ifndef HpiOs_fopen_rb		// functions not implemented as macros in OS.h files
HpiOs_FILE HpiOs_fopen_rb(const char *filename);
int HpiOs_fseek(HpiOs_FILE stream, long offset, int origin);
int HpiOs_fread(void *buffer, size_t size, size_t count, HpiOs_FILE stream);
int HpiOs_fclose(HpiOs_FILE stream);
#endif

char *HpiOs_GetDspCodePath(void);
#endif
#endif				//_HPIOS_H_

///////////////////////////////////////////////////////////////////////////
