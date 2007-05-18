/***********************************************************************/
/*!

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

\file
Functions for reading DSP code to load into DSP
USE_ZLIB is forced to be undefined.

If it is not defined, code is read from linked arrays.
HPI_INCLUDE_**** must be defined
and the appropriate hzz?????.c or hex?????.c linked in

If USE_ZLIB is defined, hpizlib.c must also be linked
*/
/***********************************************************************/
#ifndef _HPIDSPLD_H_
#define _HPIDSPLD_H_

//#include <stdio.h>
#include "hpi.h"

#ifndef DISABLE_PRAGMA_PACK1
#pragma pack(push,1)
#endif

/** Descriptor for dspcode from firmware loader */
struct DSP_CODE_FIRMWARE {
	const struct firmware *psFirmware;	// Firmware descriptor
	struct pci_dev *psDev;
	long int dwBlockLength;	//!< Expected number of words in the whole dsp code,INCL header
	long int dwWordCount;	//!< Number of words read so far
	u32 dwVersion;		//<! Version read from dsp code file
	u32 dwCrc;		//<! CRC read from dsp code file
};

/* DSP CODE IS LOADED FROM FILE DSPnnnn.BIN */
typedef struct DSP_CODE_FIRMWARE DSP_CODE;

#ifndef DISABLE_PRAGMA_PACK1
#pragma pack(pop)
#endif

enum BootLoadFamily {
	Load2200 = 0x2200,
	Load4100 = 0x4100,
	Load4300 = 0x4300,
	Load4400 = 0x4400,
	Load4500 = 0x4500,
	Load4600 = 0x4600,
	Load5000 = 0x5000,
	Load6200 = 0x6200,
	Load6413 = 0x6413,
	Load6600 = 0x6600,
	Load8600 = 0x8600,
	Load6205 = 0x6205,
	Load8713 = 0x8713,
	Load8800 = 0x8800,
	Load8900 = 0x8900
};

/*! Prepare *psDspCode to refer to the requuested adapter.
Searches the file, or selects the appropriate linked array

\return 0 for success, or error code if requested code is not available
*/
short HpiDspCode_Open(u32 nAdapter,	//!< Adapter family
		      DSP_CODE * psDspCode,	//!< Pointer to DSP code control structure
		      u32 * pdwOsErrorCode	//!< Pointer to dword to receive OS specific error code
    );

/*! Close the DSP code file */
void HpiDspCode_Close(DSP_CODE * psDspCode	//!< Pointer to DSP code control structure
    );

/*! Rewind to the beginning of the DSP code file (for verify) */
void HpiDspCode_Rewind(DSP_CODE * psDspCode	//!< Pointer to DSP code control structure
    );

/*! Read one word from the dsp code file
\return 0 for success, or error code if eof, or block length exceeded
*/
short HpiDspCode_ReadWord(DSP_CODE * psDspCode,	//!< Pointer to DSP code control structure
			  u32 * pdwWord	//!< Where to store the read word
    );

/*! Get a block of dsp code into an internal buffer, and provide a pointer to
that buffer. (If dsp code is already an array in memory, it is referenced, not copied.)
\return Error if requested number of words are not available
*/
short HpiDspCode_ReadBlock(size_t nWordsRequested,	//!< Number of words
			   DSP_CODE * psDspCode,	//!< Pointer to DSP code control structure
			   u32 * *ppdwBlock	//!< Pointer to store (Pointer to code buffer)
    );

#endif

////////////////////////////////////////////////////////////////
