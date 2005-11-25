/************************************************************************

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

Hardware Programming Interface (HPI) for AudioScience A4500 series
adapters.  These PCI bus adapters are based on the Motorola DSP56301
DSP with on-chip PCI I/F.

#defines:
USE_ZLIB use zlib compressed dsp code files. Expects to be linked with hzz4?00?.c
USE_ASIDSP load dsp code from external file "asidsp.bin".  Hex files no longer
need to be linked in

(C) Copyright AudioScience Inc. 1997-2003
************************************************************************/

// #define DEBUG
//#define USE_ASM_DATA
//#define USE_ASM_MSG
//#define TIMING_DEBUG  // writes to I/O port 0x182!
#include "hpidspcd.h"

#include "hpi.h"
#include "hpidebug.h"
#include "hpios.h"
#include "hpipci.h"
#include "hpi56301.h"
#include <dpi56301.h>

#ifdef ASI_DRV_DEBUG
#include "asidrv.h"
#endif

//////////////////////////////////////////////////////////////////////////
// DSP56301 Pci Boot code (multi-segment)
// The following code came from boot4000.asm, via makeboot.bat
///////////////////////////////////////////////////////////////////////
#ifndef __linux__
#pragma hdrstop			// allow headers above here to be precompiled
#endif
#include <boot4ka.h>

// DSP56301 PCI HOST registers (these are the offset from the base address)
#define REG56301_HCTR       0x0010	// HI32 control reg

#define HCTR_HTF0           0x0100
#define HCTR_HTF1           0x0200
#define HCTR_HRF0           0x0800
#define HCTR_HRF1           0x1000

#define REG56301_HSTR       0x0014	// HI32 status reg
#define HSTR_TRDY           0x0001	/* tx fifo empty */
#define HSTR_HTRQ           0x0002	/* txfifo not full */
#define HSTR_HRRQ           0x0004	/* rx fifo not empty */
#define HSTR_HF4            0x0010

#define REG56301_HCVR       0x0018	// HI32 command vector reg

#define REG56301_HTXR       0x001C	// Host Transmit data reg (FIFO)
#define REG56301_HRXS       0x001C	// Host Receive data reg (FIFO)

#define TIMEOUT             100000	// # of times to retry operations

// printfs do weird thing under Win16
//#ifdef HPI_OS_WIN16
//#define //HPIOS_DEBUG_STRING(a)
//#endif

#define DPI_DATA_SIZE   16
typedef struct {
	u16 wSize;
	u16 wStreamIndex;
	u32 wData[DPI_DATA_SIZE];
} DPI_DATA_MESSAGE;

/**************************** LOCAL PROTOTYPES **************************/
static short Dpi_Command(u32 dw56301Base, u32 Cmd);
static void Dpi_SetFlags(u32 dw56301Base, short Flagval);
static short Dpi_WaitFlags(u32 dw56301Base, short Flagval);
static short Dpi_GetFlags(u32 dw56301Base);
static short DpiData_Read16(u32 dwBase, u16 * pwData);
static short DpiData_Read32(u32 dwBase, u32 * pdwData);
static short DpiData_Write32(u32 dwBase, u32 * pdwData);
static short DpiData_WriteBlock16(u32 dwBase, u16 * pwData, u32 dwLength);
#ifdef WANT_UNUSED_FUNCTION_DECLARED
static short DpiData_ReadBlock32(u32 dwBase, u32 * pdwData, u32 dwLength);
static short DpiData_WriteBlock32(u32 dwBase, u32 * pdwData, u32 dwLength);
#endif
static short DpiData_Write24(u32 dwBase, u32 * pdwData);

/**************************** EXPORTED FUNCTIONS ************************/
/************************************************************************/
// test out PCI i/f by writing a message block
// and then receiving it back again

short Hpi56301_SelfTest(u32 dwMemBase)
{

	u32 i = 0;
	u32 dwData;
//    HPI_MESSAGE hm;
//    HPI_RESPONSE hr;
//    HPI_MESSAGE *phm=&hm;
//    HPI_RESPONSE *phr=&hr;

//#ifdef DEBUG
////HPIOS_DEBUG_STRING("message size %i\n",sizeof(hm));
////HPIOS_DEBUG_STRING("response size %i\n",sizeof(hr));
//#endif

// Put some self test code here at some stage!
// Some silly code to get rid of compiler warnings until we complete
// this function
	dwData = dwMemBase;
	i = dwData;
	dwData = i;

	return (0);
}

/************************************************************************/
// this function just checks that we have the correct adapter (in this case
// the 301 chip) at the indicated memory address

short Hpi56301_CheckAdapterPresent(u32 dwMemBase)
{
	u32 dwData = 0;
//! Don't set TWSD
	HPIOS_MEMWRITE32(dwMemBase + REG56301_HCTR, 0xFFF7FFFFL);
	dwData = HPIOS_MEMREAD32(dwMemBase + REG56301_HCTR);
	if (dwData != 0x1DBFEL) {
		HPI_PRINT_ERROR("Read should be 1DBFE, but is %X\n", dwData);
		return (1);	//error
	} else {
		u32 dwHSTR = 0, dwHCVR = 0;
		dwHSTR = HPIOS_MEMREAD32(dwMemBase + REG56301_HSTR);
		dwHCVR = HPIOS_MEMREAD32(dwMemBase + REG56301_HCVR);
		HPI_PRINT_DEBUG
		    ("PCI registers, HCTR=%08X HSTR=%08X HCVR=%08X\n", dwData,
		     dwHSTR, dwHCVR);
		return (0);	//success
	}
}

/************************************************************************/
//short Hpi56301_BootLoadDsp( u32 dwMemBase, u32  * apaDspCodeArrays[] ) old

short Hpi56301_BootLoadDsp(H400_ADAPTER_OBJ * pao)
{
	u32 dwMemBase = pao->Pci.dwMemBase[0];
	DSP_CODE DspCode;
	short nError = 0;

	u32 dwDspCodeLength = adwDspCode_Boot4000a[0];
	u32 dwDspCodeAddr = adwDspCode_Boot4000a[1];
	u32 dwDspCodeType = 0;
//u32  * adwDspCodeArray;
//   short nArrayNum;
	short bFlags;
	u32 dwHSTR;
	u32 dwData;
//    u32 dwOffset=0;
	u32 i = 0;
	u32 dwCount;
	u16 wAdapterId = 0;
	short k = 0;
	enum BootLoadFamily nLoadFamily = Load4500;

// Number of words per dot during download 102 = 1K/10
#define DOTRATE 102

	Dpi_SetFlags(dwMemBase, HF_DSP_STATE_RESET);

#ifdef JTAGLOAD

//  Use this if not bootloading
	Dpi_Command(dwMemBase, CVR_RESTART + CVR_NMI);
	dwHSTR = 0;
	HPIOS_MEMWRITE32(dwMemBase + REG56301_HCTR, dwHSTR);
	HPIOS_DEBUG_STRING("reset\n");	//*************** debug
	return (0);
#endif

	HPIOS_DEBUG_STRING("Boot-");	//*************** debug
// If board isn't reset, try to reset it now
	i = 0;
	do {
		dwHSTR = HPIOS_MEMREAD32(dwMemBase + REG56301_HSTR);
		bFlags = (short)(dwHSTR & HF_MASK);
		if (bFlags) {
			HPIOS_DEBUG_STRING("reset,");	//*************** debug
			Dpi_Command(dwMemBase, CVR_NMIRESET);
			HpiOs_DelayMicroSeconds(100);
		}
	}
	while ((bFlags != 0) && (++i < 5));

	if (bFlags) {
		HPIOS_DEBUG_STRING("ERROR: Bootload - Cannot soft reset DSP\n");
		return (DPI_ERROR_DOWNLOAD + 1);	// expect them to be 0 after reset
	}
///////////////////////////////////////////////////////////////////////////////
// SGT 3-29-97 download boot-loader

	HPIOS_DEBUG_STRING("Dbl,");	//*************** debug

// Set up the transmit and receive FIFO for 24bit words in lower portion of 32 bit word
	HPIOS_MEMWRITE32(dwMemBase + REG56301_HCTR, HCTR_HTF0 | HCTR_HRF0);
	HpiOs_DelayMicroSeconds(100);

// write length and starting address
	if (DpiData_Write24(dwMemBase, &dwDspCodeLength))
		return (DPI_ERROR_DOWNLOAD + 2);
	if (DpiData_Write24(dwMemBase, &dwDspCodeAddr))
		return (DPI_ERROR_DOWNLOAD + 3);

// and then actual code
// Skip array[2] which contains type
	for (i = 3; i < dwDspCodeLength + 3; i++) {
		dwData = (u32) adwDspCode_Boot4000a[(u16) i];
		if (DpiData_Write24(dwMemBase, &dwData))
			return (DPI_ERROR_DOWNLOAD + 4);
	}
// should we check somehow that bootloader is running ????
// use HF0-2 ??????

//////////////////////////////////////////////////////////////////////////////////
// SGT NOV-25-97 - get adapter ID from bootloader through HF3,4,5, wait until !=0
	HPIOS_DEBUG_STRING("Id,");	//*************** debug
	for (k = 10000; k != 0; k--) {
		dwHSTR = HPIOS_MEMREAD32(dwMemBase + REG56301_HSTR);
		if (dwHSTR & 0x0038)
			break;
	}
	if (k == 0)
		return (DPI_ERROR_DOWNLOAD + 5);

	wAdapterId = (unsigned short)(dwHSTR & 0x38) >> 3;

// assign DSP code based on adapter type returned
//if (wAdapterId==4) wAdapterId=6;     //************** assume 4500 for now ****************

// in DOS we load the DSP code from a file, so the DSP code
// structures are not used (and not linked in)
	switch (wAdapterId) {
	case 2:		// ASI4300
		nLoadFamily = Load4300;
		break;
	case 3:		// ASI4400
		nLoadFamily = Load4400;
		break;
	case 4:		// ASI4100
		nLoadFamily = Load4100;
		break;
	case 6:		// ASI4600
		nLoadFamily = Load4600;
		break;
	default:		// ASI4500
		nLoadFamily = Load4500;
		break;
	}

	HPI_PRINT_VERBOSE("(Family = %x) \n", nLoadFamily);

	DspCode.psDev = pao->Pci.pOsData;
	nError = HpiDspCode_Open(nLoadFamily, &DspCode);
	if (nError)
		goto exit;

///////////////////////////////////////////////////////////////////////////////
// SGT 3-29-97 download multi-segment, multi-array/file DSP code (P,X,Y)
	HPIOS_DEBUG_STRING("Dms,");	// *************** debug
	dwCount = 0;
	while (1) {
// write length, starting address and segment type (P,X or Y)
		if ((nError =
		     HpiDspCode_ReadWord(&DspCode, &dwDspCodeLength)) != 0)
			goto exit;
#ifdef DSPCODE_ARRAY
// check for end of array with continuation to another one
		if (dwDspCodeLength == 0xFFFFFFFEL) {
			DspCode.nArrayNum++;
			DspCode.dwOffset = 0;
			if ((nError =
			     HpiDspCode_ReadWord(&DspCode,
						 &dwDspCodeLength)) != 0)
				goto exit;
		}
#endif
		if (DpiData_Write24(dwMemBase, &dwDspCodeLength)) {
			nError = (DPI_ERROR_DOWNLOAD + 6);
			goto exit;
		}

		if (dwDspCodeLength == 0xFFFFFFFFL)
			break;	//end of code, still wrote to DSP to signal end of load

		if ((nError =
		     HpiDspCode_ReadWord(&DspCode, &dwDspCodeAddr)) != 0)
			goto exit;
		if ((nError =
		     HpiDspCode_ReadWord(&DspCode, &dwDspCodeType)) != 0)
			goto exit;

		if (DpiData_Write24(dwMemBase, &dwDspCodeAddr)) {
			nError = (DPI_ERROR_DOWNLOAD + 7);
			goto exit;
		}
		if (DpiData_Write24(dwMemBase, &dwDspCodeType)) {
			nError = (DPI_ERROR_DOWNLOAD + 8);
			goto exit;
		}
		dwCount += 3;

// and then actual code segment
		for (i = 0; i < dwDspCodeLength; i++) {
//if ((i % DOTRATE)==0) //HPIOS_DEBUG_STRING(".");
			if ((nError =
			     HpiDspCode_ReadWord(&DspCode, &dwData)) != 0)
				goto exit;
			if (DpiData_Write24(dwMemBase, &dwData)) {
				nError = (DPI_ERROR_DOWNLOAD + 9);
				goto exit;
			}
			dwCount++;
		}
//HPIOS_DEBUG_STRING(".");
	}
// ======== END OF MAIN DOWNLOAD ========

//////////////////////////////////////////////////////////////////////////
// EWB Wait for DSP to be ready to reset PCI
//HPIOS_DEBUG_STRING("Wr,");  // *************** debug
//#define HANDSHAKE_PCI_RESET
#ifdef HANDSHAKE_PCI_RESET

	if (Dpi_WaitFlags(dwMemBase, HF_PCI_HANDSHAKE)) {
		nError = (DPI_ERROR_DOWNLOAD + 10);	// Never got to "ready for mode change"
		goto exit;
	}
//EWB indicate finished with PCI reconfig
// DSP will now reset the PCI interface, PC mustn't access the
// PCI for a little while, so the timed delay is mandatory.
	Dpi_SetFlags(dwMemBase, HF_PCI_HANDSHAKE);
	HpiOs_DelayMicroSeconds(100000);

// Wait for transition to Idle - delay 100ms
// Full DRAM buffer init may take 50ms
	HPIOS_DEBUG_STRING("Wi,");	//*************** debug

//********************** steve test
//HpiOs_DelayMicroSeconds( 500000 ); /*SGT*/
//********************** steve test
#endif
//HpiOs_DelayMicroSeconds( 500000 );
//if (Dpi_WaitFlags(dwMemBase,HF_DSP_STATE_IDLE))
//          return(DPI_ERROR_DOWNLOAD+11);  // Never got to idle

// SGT - use custom WaitFlags code that has an extra long timeout
	{
		u32 dwTimeout = 1000000;
		u16 wFlags = 0;
		do {
			wFlags = Dpi_GetFlags(dwMemBase);
		}
		while ((wFlags != HF_DSP_STATE_IDLE) && (--dwTimeout));
		if (!dwTimeout) {
			nError = (DPI_ERROR_DOWNLOAD + 11);	// Never got to idle
			goto exit;
		}
	}
	Dpi_SetFlags(dwMemBase, CMD_NONE);

	HPIOS_DEBUG_STRING("!\n");	//*************** debug

      exit:
	HpiDspCode_Close(&DspCode);
	return (nError);

}

/************************************************************************/
static u16 Hpi56301_Resync(u32 dwBase)
{
	u16 wError;
//?  u32 dwData;
	u32 dwTimeout;

	Dpi_SetFlags(dwBase, CMD_NONE);
	Dpi_Command(dwBase, CVR_CMDRESET);
	wError = Dpi_WaitFlags(dwBase, HF_DSP_STATE_IDLE);
	dwTimeout = TIMEOUT;
	while (dwTimeout
	       && (HPIOS_MEMREAD32(dwBase + REG56301_HSTR) & HSTR_HRRQ)) {
//?         dwData=HPIOS_MEMREAD32(dwBase + REG56301_HRXS);
		dwTimeout--;
	}
	if (dwTimeout == 0)
		wError = HPI_ERROR_BAD_ADAPTER;
	return wError;

}

/************************************************************************/
static short Hpi56301_Send(u32 dw56301Base,
			   u16 * pData, u32 dwLengthInWords, short wDataType)
{
//u32  * pdwData=(u32  *)pData;
	u16 *pwData = pData;
	u16 nErrorIndex = 0;
	u16 wError;
//u32 dwCount;
	u32 dwLength;
	u32 dwTotal;

	HPI_PRINT_VERBOSE("Send, ptr = 0x%x, len = %d, cmd = %d\n",
			  (u32) pData, dwLengthInWords, wDataType);
	HPI_DEBUG_DATA(pData, dwLengthInWords);

	wError = Dpi_WaitFlags(dw56301Base, HF_DSP_STATE_IDLE);
	if (wError) {
#ifdef ASI_DRV_DEBUG
//DBGPRINTF1( TEXT("!Dpi_WaitFlags %d!"),wError);
#endif
		nErrorIndex = 1;
		goto ErrorExit;	// error 911
	}
	HPI_PRINT_VERBOSE("flags = %d\n", Dpi_GetFlags(dw56301Base));
	Dpi_SetFlags(dw56301Base, wDataType);
	HPI_PRINT_VERBOSE("flags = %d\n", Dpi_GetFlags(dw56301Base));
	if (DpiData_Write32(dw56301Base, &dwLengthInWords)) {
		nErrorIndex = 2;
		goto ErrorExit;
	}			// error 912
	HPI_PRINT_VERBOSE("flags = %d\n", Dpi_GetFlags(dw56301Base));
	if (Dpi_Command(dw56301Base, CVR_CMD)) {
		nErrorIndex = 3;
		goto ErrorExit;
	}			// error 913
	HPI_PRINT_VERBOSE("flags = %d\n", Dpi_GetFlags(dw56301Base));
	if (Dpi_WaitFlags(dw56301Base, HF_DSP_STATE_CMD_OK)) {
		nErrorIndex = 4;
		goto ErrorExit;
	}			// error 914
// read back length before wrap
	if (DpiData_Read32(dw56301Base, &dwLength)) {
		HPI_PRINT_ERROR("DpiData_Read32 error line %d\n", __LINE__);
		nErrorIndex = 5;
		goto ErrorExit;
	}			// error 915
/* round up limit.
Buffer must have extra word if odd length is requested.
I.e buffers must be allocated in 32bit multiples
*/
/////////////////////////////////////////////// SGT test transfer time, start
//outportb(0x378,0);
//  dwTotal = ((dwLength+1)/2);
	dwTotal = dwLength;
/* non-block way
for (dwCount=0; dwCount<dwTotal; dwCount++)
{  if (DpiData_Write32( dw56301Base,pdwData))
{nErrorIndex=6; goto ErrorExit;}
pdwData++;
}*/
// block way - SGT JUL-13-98
//  if(DpiData_WriteBlock32( dw56301Base, pdwData, dwTotal))
	if (DpiData_WriteBlock16(dw56301Base, pwData, dwTotal)) {
		nErrorIndex = 6;	// error 916
		goto ErrorExit;
	}

	if (dwLength < dwLengthInWords) {
		if (Dpi_Command(dw56301Base, CVR_DMASTOP)) {
			nErrorIndex = 7;
			goto ErrorExit;
		}		// error 917
// Wait for DMA to be reprogrammed for second block
		if (Dpi_WaitFlags(dw56301Base, HF_DSP_STATE_WRAP_READY)) {
			nErrorIndex = 8;
			goto ErrorExit;
		}		// error 918
//              pdwData=(u32  *)(pData+ dwLength);
		pwData = (pData + dwLength);
/* round up limit.  Buffer must have extra word if odd length is requested.
Buffers must be allocated in 32bit multiples
Should this code try to use aligned dwords where possible?
i.e if pData starts odd, do word xfer,loop dword xfer, word xfer */

// remaining words after wrap
		dwTotal = (dwLengthInWords - dwLength);
/* non-block way
for (dwCount=0; dwCount<dwTotal; dwCount++)
{   if (DpiData_Write32( dw56301Base, pdwData ))
{nErrorIndex=9; goto ErrorExit;           }
pdwData++;
}*/
//              if(DpiData_WriteBlock32( dw56301Base, pdwData, dwTotal))
		if (DpiData_WriteBlock16(dw56301Base, pwData, dwTotal)) {
			nErrorIndex = 9;	// error 919
			goto ErrorExit;
		}
	}
/////////////////////////////////////////////// SGT test transfer time, end
//outportb(0x378,0);

	if (Dpi_Command(dw56301Base, CVR_DMASTOP)) {
		nErrorIndex = 10;
		goto ErrorExit;
	}			// error 920

	if (wDataType == CMD_SEND_MSG) {
		if (Dpi_WaitFlags(dw56301Base, HF_DSP_STATE_RESP_READY)) {
			nErrorIndex = 11;
			goto ErrorExit;
		}		// error 921
	} else {
		if (Dpi_WaitFlags(dw56301Base, HF_DSP_STATE_IDLE)) {
			nErrorIndex = 12;
			goto ErrorExit;
		}		// error 922
	}
	return (0);

      ErrorExit:
/*
* When there is an error, we automatically try to resync.
* If the error persists, bump the error count up by 1000.
* Fatal errors will now return 19xx (decimal), and errors that "should"
* be recoverable will now return 9xx.
*/
	if (Hpi56301_Resync(dw56301Base))
		nErrorIndex += 1000;
	return (DPI_ERROR_SEND + nErrorIndex);
}

/************************************************************************/
static short Hpi56301_Get(u32 dw56301Base,
			  u16 * pData, u32 * pdwLengthInWords, short wDataType)
{
	u32 dwLength1, dwLength2;
//  u32  * pdwData = (u32  *) pData;
	u16 *pwData = pData;
	u16 nErrorIndex = 0;
	u32 dwCount;
	u32 dwBeforeWrap, dwAfterWrap;
	u32 dwJunk;
	u32 *pdwJunk = &dwJunk;

	HPI_PRINT_VERBOSE("Get, ptr = 0x%x, cmd = %d\n", (u32) pData,
			  wDataType);

// Wait for indication that previous command has been processed
	if (wDataType == CMD_GET_RESP) {
		if (Dpi_WaitFlags(dw56301Base, HF_DSP_STATE_RESP_READY)) {
			nErrorIndex = 1;
			goto ErrorExit;
		}		// error 951
	} else {
		if (Dpi_WaitFlags(dw56301Base, HF_DSP_STATE_IDLE)) {
			nErrorIndex = 2;
			goto ErrorExit;
		}		// error 952
	}
// Tell DSP what to do next.
	Dpi_SetFlags(dw56301Base, wDataType);
	if (Dpi_Command(dw56301Base, CVR_CMD)) {
		nErrorIndex = 3;
		goto ErrorExit;
	}			// error 953

	if (Dpi_WaitFlags(dw56301Base, HF_DSP_STATE_CMD_OK)) {
		nErrorIndex = 4;
		goto ErrorExit;
	}			// error 954

// returned length in 16 bit words
	if (DpiData_Read32(dw56301Base, &dwLength1)) {
		HPI_PRINT_ERROR("DpiData_Read32 error line %d\n", __LINE__);
		nErrorIndex = 5;
		goto ErrorExit;
	}			// error 955
// dwBeforeWrap = ((dwLength1+1)/2);
	dwBeforeWrap = dwLength1;
// length of second part of data, may be 0
	if (DpiData_Read32(dw56301Base, &dwLength2)) {
		HPI_PRINT_ERROR("DpiData_Read32 error line %d\n", __LINE__);
		nErrorIndex = 5;
		goto ErrorExit;
	}			// error 955
	dwAfterWrap = dwLength2;
	*pdwLengthInWords = dwLength1 + dwLength2;	// AGE Feb 19 98

// read the first part of data
	for (dwCount = 0; dwCount < dwBeforeWrap; dwCount++) {
		if (DpiData_Read16(dw56301Base, pwData)) {
			nErrorIndex = 6;
			goto ErrorExit;
		}
		pwData++;
	}			// error 956

// check for a bad value of dwAfterWrap
	if (dwAfterWrap && (wDataType == CMD_GET_RESP)) {
		HPI_PRINT_ERROR("Unexpected data wrap for response, got %u\n",
				dwAfterWrap);
		dwAfterWrap = 0;	// this may fix the error - it depends if the DSP is in error or not.
	}

	if (dwAfterWrap)	// Handshake, then read second block
	{
		if (Dpi_Command(dw56301Base, CVR_DMASTOP)) {
			HPI_PRINT_ERROR("Dpi_Command %d\n", __LINE__);
			nErrorIndex = 7;
			goto ErrorExit;
		}		// error 957
		if (Dpi_WaitFlags(dw56301Base, HF_DSP_STATE_WRAP_READY)) {
			HPI_PRINT_ERROR("Dpi_WaitFlags %d\n", __LINE__);
			nErrorIndex = 8;
			goto ErrorExit;
		}		// error 958
/* Clean the extra data out of the FIFO */
		if (DpiData_Read32(dw56301Base, pdwJunk)) {
			HPI_PRINT_ERROR("DpiData_Read32 error line %d\n",
					__LINE__);
			nErrorIndex = 9;
			goto ErrorExit;
		}		// error 959
		if (DpiData_Read32(dw56301Base, pdwJunk)) {
			HPI_PRINT_ERROR("DpiData_Read32 error line %d\n",
					__LINE__);
			nErrorIndex = 9;
			goto ErrorExit;
		}		// error 959
		if (DpiData_Read32(dw56301Base, pdwJunk)) {
			HPI_PRINT_ERROR("DpiData_Read32 error line %d\n",
					__LINE__);
			nErrorIndex = 9;
			goto ErrorExit;
		}		// error 959
		for (dwCount = 0; dwCount < dwAfterWrap; dwCount++) {
			if (DpiData_Read16(dw56301Base, pwData)) {
				nErrorIndex = 10;
				goto ErrorExit;
			}
			pwData++;
		}		// error 960
	}

	if (Dpi_Command(dw56301Base, CVR_DMASTOP)) {
		nErrorIndex = 11;
		goto ErrorExit;
	}			// error 961
	if (Dpi_WaitFlags(dw56301Base, HF_DSP_STATE_IDLE)) {
		nErrorIndex = 12;
		goto ErrorExit;
	}			// error 962

/* Clean the extra data out of the FIFO */
	if (DpiData_Read32(dw56301Base, pdwJunk)) {
		HPI_PRINT_ERROR("DpiData_Read32 error line %d\n", __LINE__);
		nErrorIndex = 13;
		goto ErrorExit;
	}			// error 963
	if (DpiData_Read32(dw56301Base, pdwJunk)) {
		HPI_PRINT_ERROR("DpiData_Read32 error line %d\n", __LINE__);
		nErrorIndex = 14;
		goto ErrorExit;
	}			// error 964
	if (DpiData_Read32(dw56301Base, pdwJunk)) {
		HPI_PRINT_ERROR("DpiData_Read32 error line %d\n", __LINE__);
		nErrorIndex = 15;
		goto ErrorExit;
	}			// error 965
	HPI_DEBUG_DATA(pData, *pdwLengthInWords);
	return (0);

      ErrorExit:
/*
* When there is an error, we automatically try to resync.
* If the error persists, bump the error count up by 1000.
* Fatal errors will now return 19xx (decimal), and errors that "should"
* be recoverable will now return 9xx.
*/
	if (Hpi56301_Resync(dw56301Base))
		nErrorIndex += 1000;
	return (DPI_ERROR_GET + nErrorIndex);
}

/************************************************************************/
void Hpi56301_Message(u32 dwMemBase, HPI_MESSAGE * phm, HPI_RESPONSE * phr)
{
	short nError;
	u32 dwSize;

	HPIOS_DEBUG_STRING("A");	// *************** debug

// Need initialise in case hardware fails to return a response
	HPI_InitResponse(phr, 0, 0, HPI_ERROR_INVALID_RESPONSE);

	if ((phm->wObject == 0) || (phm->wFunction == 0)) {
		phr->wError = DPI_ERROR;
		return;
	}

/* error is for data transport. If no data transport error, there
still may be an error in the message, so dont assign directly */

	nError = Hpi56301_Send(dwMemBase,
			       (u16 *) phm,
			       (phm->wSize / sizeof(u32)) * sizeof(u16),
			       CMD_SEND_MSG);
	if (nError) {
		phr->wError = nError;
		return;
	}

	HPIOS_DEBUG_STRING("B");	// *************** debug
	nError = Hpi56301_Get(dwMemBase, (u16 *) phr, &dwSize, CMD_GET_RESP);
	if (nError) {
		phr->wError = nError;
		return;
	}

/* maybe an error response to WRITE or READ message */
	if (phr->wError != 0) {
		return;
	}

	if (phm->wFunction == HPI_OSTREAM_WRITE) {
		HPIOS_DEBUG_STRING("C");	// *************** debug
// note - this transfers the entire packet in one go!
		nError = Hpi56301_Send(dwMemBase,
				       (u16 *) phm->u.d.u.Data.dwpbData,
				       (phm->u.d.u.Data.dwDataSize /
					sizeof(u16)), CMD_SEND_DATA);
		if (nError) {
			phr->wError = nError;
			return;
		}
	}

	if (phm->wFunction == HPI_ISTREAM_READ) {
		HPIOS_DEBUG_STRING("D");	// *************** debug
// note - this transfers the entire packet in one go!
		nError = Hpi56301_Get(dwMemBase,
				      (u16 *) phm->u.d.u.Data.dwpbData,
				      &dwSize, CMD_GET_DATA);
		if (nError) {
			phr->wError = nError;
			return;
		}

		if (phm->u.d.u.Data.dwDataSize != (dwSize * 2)) {	// mismatch in requested and received data size
			phr->wError = HPI_ERROR_INVALID_DATA_TRANSFER;
		}
	}

	if ((phm->wObject == HPI_OBJ_CONTROLEX)
	    && (phm->u.cx.wAttribute == HPI_AES18_MESSAGE)) {

		if (phm->wFunction == HPI_CONTROL_SET_STATE) {
			HPIOS_DEBUG_STRING("E");	// *************** debug
// note - this transfers the entire packet in one go!
			nError = Hpi56301_Send(dwMemBase,
					       (u16 *) phm->u.cx.u.
					       aes18tx_send_message.dwpbMessage,
					       (phm->u.cx.u.
						aes18tx_send_message.
						wMessageLength / sizeof(u16)),
					       CMD_SEND_DATA);
			if (nError) {
				phr->wError = nError;
				return;
			}
		}

		if (phm->wFunction == HPI_CONTROL_GET_STATE) {
			HPIOS_DEBUG_STRING("F");	// *************** debug
// note - this transfers the entire packet in one go!
			nError = Hpi56301_Get(dwMemBase,
					      (u16 *) phm->u.cx.u.
					      aes18rx_get_message.dwpbMessage,
					      &dwSize, CMD_GET_DATA);
			if (nError) {
				phr->wError = nError;
				return;
			}

/*
if (phm->u.d.Data.dwDataSize != (dwSize *2))
{   // mismatch in requested and received data size
phr->wError=HPI_ERROR_INVALID_DATA_TRANSFER;
}
*/
		}
	}

	HPIOS_DEBUG_STRING("G");	// *************** debug
	return;
}

/**************************** LOCAL FUNCTIONS ***************************/
/************************************************************************/
void Dpi_SetFlags(u32 dw56301Base, short Flagval)
{
	u32 dwHCTR;

	dwHCTR = HPIOS_MEMREAD32(dw56301Base + REG56301_HCTR);
	dwHCTR = (dwHCTR & HF_CLEAR) | Flagval;
	HPIOS_MEMWRITE32(dw56301Base + REG56301_HCTR, dwHCTR);
}

/************************************************************************
Wait for specified flag value to appear in the HSTR
Return 0 if it does, DPI_ERROR if we time out waiting
************************************************************************/

short Dpi_WaitFlags(u32 dw56301Base, short Flagval)
{
	u16 wFlags;
	u32 dwTimeout = TIMEOUT;

// HPIOS_DEBUG_STRING("Wf,");

	do {
		wFlags = Dpi_GetFlags(dw56301Base);
	}
	while ((wFlags != Flagval) && (--dwTimeout));
	HPI_PRINT_VERBOSE("Expecting %d, got %d%s\n",
			  Flagval, wFlags, !dwTimeout ? " timeout" : "");
	if (!dwTimeout) {
		HPI_PRINT_ERROR("Dpi_WaitFlags()  Expecting %d, got %d\n",
				Flagval, wFlags);
		return (DPI_ERROR + wFlags);
	} else
		return (0);
}

/************************************************************************/
short Dpi_GetFlags(u32 dw56301Base)
{
	u32 dwHSTR1, dwHSTR2;

// HPIOS_DEBUG_STRING("Gf,");
	do {
		dwHSTR1 = HPIOS_MEMREAD32(dw56301Base + REG56301_HSTR);
		dwHSTR2 = HPIOS_MEMREAD32(dw56301Base + REG56301_HSTR);
	}
	while (dwHSTR1 != dwHSTR2);
	return ((short)(dwHSTR1 & HF_MASK));
}

/************************************************************************/
short Dpi_Command(u32 dw56301Base, u32 Cmd)
{

	u32 dwTimeout;
	u32 dwHCVR;

////HPIOS_DEBUG_STRING("Dc,");

// make sure HCVR-HC is zero before writing to HCVR
	dwTimeout = TIMEOUT;
	do {
		dwTimeout--;
		dwHCVR = HPIOS_MEMREAD32(dw56301Base + REG56301_HCVR);
	}
	while ((dwHCVR & CVR_INT) && dwTimeout);
	HPI_PRINT_VERBOSE("(cmd = %d) attempts = %d\n",
			  Cmd, TIMEOUT - dwTimeout);
	if (!dwTimeout)
		return (DPI_ERROR);

	HPIOS_MEMWRITE32(dw56301Base + REG56301_HCVR, Cmd + CVR_INT);
/* Wait for interrupt to be acknowledged */
	dwTimeout = TIMEOUT;
	do {
		dwTimeout--;
		dwHCVR = HPIOS_MEMREAD32(dw56301Base + REG56301_HCVR);
	}
	while ((dwHCVR & CVR_INT) && dwTimeout);
	HPI_PRINT_VERBOSE("(cmd = %d) attempts = %d\n",
			  Cmd, TIMEOUT - dwTimeout);
	if (!dwTimeout)
		return (DPI_ERROR);
	else
		return (0);
}

/************************************************************************/
// Reading the FIFO, protected against FIFO empty
// Returns DPI_ERROR if FIFO stays empty for TIMEOUT loops
short DpiData_Read32(u32 dwBase, u32 * pdwData)
{
	u32 dwTimeout;
	u32 dwHSTR;
	u32 dwD1 = 0, dwD2 = 0;

	dwTimeout = TIMEOUT;
	do {
		dwTimeout--;
		dwHSTR = HPIOS_MEMREAD32(dwBase + REG56301_HSTR);
	}
	while ((!(dwHSTR & HSTR_HRRQ)) && dwTimeout);
	if (TIMEOUT - dwTimeout > 500) {
		HPI_PRINT_ERROR("DpiData_Read32() attempts = %d\n",
				TIMEOUT - dwTimeout);
	}

	if (!dwTimeout) {
		HPI_PRINT_ERROR("DataRead - FIFO is staying empty"
				" HSTR=%08X\n",
				HPIOS_MEMREAD32(dwBase + REG56301_HSTR));
		return (DPI_ERROR);
	} else {
		dwD1 = HPIOS_MEMREAD32(dwBase + REG56301_HRXS);
	}

	dwTimeout = TIMEOUT;
	do {
		dwTimeout--;
		dwHSTR = HPIOS_MEMREAD32(dwBase + REG56301_HSTR);
	}
	while ((!(dwHSTR & HSTR_HRRQ)) && dwTimeout);
	if (!dwTimeout) {
		HPI_PRINT_ERROR("DataRead - FIFO is staying empty"
				" HSTR=%08X\n",
				HPIOS_MEMREAD32(dwBase + REG56301_HSTR));
		return (DPI_ERROR);
	} else {
		dwD2 = HPIOS_MEMREAD32(dwBase + REG56301_HRXS);
	}
	*pdwData = (dwD2 << 16) | (dwD1 & 0xFFFF);
	return (0);
}

short DpiData_Read16(u32 dwBase, u16 * pwData)
{
	u32 dwTimeout;
	u32 dwHSTR;
	u32 dwD1 = 0;

	dwTimeout = TIMEOUT;
	do {
		dwTimeout--;
		dwHSTR = HPIOS_MEMREAD32(dwBase + REG56301_HSTR);
	}
	while ((!(dwHSTR & HSTR_HRRQ)) && dwTimeout);

	if (!dwTimeout) {
		HPI_PRINT_ERROR("DataRead - FIFO is staying empty"
				" HSTR=%08X\n",
				HPIOS_MEMREAD32(dwBase + REG56301_HSTR));
		return (DPI_ERROR);
	}

	dwD1 = HPIOS_MEMREAD32(dwBase + REG56301_HRXS);
//    HPI_PRINT_VERBOSE("Read 0x%08lx, hstr = 0x%08lx, attempts = %ld\n",
//                dwD1, dwHSTR, TIMEOUT - dwTimeout);
	dwD1 &= 0xFFFF;
	*pwData = (u16) dwD1;
	return (0);
}

/************************************************************************/
// Writing the FIFO, protected against FIFO full
// Returns error if FIFO stays full
short DpiData_Write32(u32 dwBase, u32 * pdwData)
{
	u32 dwTimeout;
	u32 dwD1, dwD2;
	u32 dwHSTR;

	dwD1 = *pdwData & 0xFFFF;
	dwD2 = *pdwData >> 16;

	dwTimeout = TIMEOUT;
	do {
		dwTimeout--;
		dwHSTR = HPIOS_MEMREAD32(dwBase + REG56301_HSTR);
	}
	while ((!(dwHSTR & HSTR_HTRQ)) && dwTimeout);
	if (!dwTimeout) {
		HPI_PRINT_ERROR("DataWrite - FIFO is staying full"
				" HSTR=%08X\n",
				HPIOS_MEMREAD32(dwBase + REG56301_HSTR));
		return (DPI_ERROR);
	} else {
		HPIOS_MEMWRITE32(dwBase + REG56301_HTXR, dwD1);
/*
HPI_PRINT_VERBOSE("Wrote (lo) 0x%08lx, hstr = 0x%08lx," \
" attempts = %ld\n",
dwD1, dwHSTR, TIMEOUT - dwTimeout);
*/
	}
	dwTimeout = TIMEOUT;
	do {
		dwTimeout--;
		dwHSTR = HPIOS_MEMREAD32(dwBase + REG56301_HSTR);
	}
	while ((!(dwHSTR & HSTR_HTRQ)) && dwTimeout);
	if (!dwTimeout) {
		HPI_PRINT_ERROR("DataWrite - FIFO is staying full"
				" HSTR=%08X\n",
				HPIOS_MEMREAD32(dwBase + REG56301_HSTR));
		return (DPI_ERROR);
	} else {
		HPIOS_MEMWRITE32(dwBase + REG56301_HTXR, dwD2);
/*
HPI_PRINT_VERBOSE("Wrote (hi) 0x%08lx, hstr = 0x%08lx," \
" attempts = %ld\n",
dwD2, dwHSTR, TIMEOUT - dwTimeout);
*/
		return (0);
	}
}

/************************************************************************/
// Writing the FIFO, protected against FIFO full
// Returns error if FIFO stays full
short DpiData_Write24(u32 dwBase, u32 * pdwData)
{
	u32 dwTimeout = TIMEOUT;

	do
		dwTimeout--;
	while ((!(HPIOS_MEMREAD32(dwBase + REG56301_HSTR) & HSTR_HTRQ))
	       && dwTimeout);
	if (!dwTimeout) {
		HPI_PRINT_ERROR("DataWrite - FIFO is staying full"
				" HSTR=%08x\n",
				HPIOS_MEMREAD32(dwBase + REG56301_HSTR));
		return (DPI_ERROR);
	} else {
		HPIOS_MEMWRITE32(dwBase + REG56301_HTXR, *pdwData);
		return (0);
	}
}

/************************************************************************/
// Writing the FIFO, protected against FIFO full
// Returns error if FIFO stays full
short DpiData_Write16(u32 dwBase, u16 * pwData)
{
	u32 dwTimeout = TIMEOUT;
	u32 dwHSTR;

	do {
		dwTimeout--;
		dwHSTR = HPIOS_MEMREAD32(dwBase + REG56301_HSTR);
	}
	while ((!(dwHSTR & HSTR_HTRQ)) && dwTimeout);
	if (!dwTimeout) {
		HPI_PRINT_ERROR("DataWrite - FIFO is staying full"
				" HSTR=%08X\n",
				HPIOS_MEMREAD32(dwBase + REG56301_HSTR));
		return (DPI_ERROR);
	} else {
		u32 wData = *pwData;
		HPIOS_MEMWRITE32(dwBase + REG56301_HTXR, wData);
//      HPI_PRINT_VERBOSE("Wrote 0x%08lx, hstr = 0x%08lx, attempts = %ld\n",
//                        wData, dwHSTR, TIMEOUT - dwTimeout);
		return (0);
	}
}

#ifdef WANT_UNUSED_FUNTION_DEFINED
/************************************************************************/
// Reading a block from the FIFO, protected against FIFO empty
// Returns error if FIFO stays empty
short DpiData_ReadBlock32(u32 dwBase, u32 * pdwData, u32 dwLength)
{
	u32 dwTimeout = TIMEOUT;
	u32 dwAddrFifoStatus = dwBase + REG56301_HSTR;
	u32 dwAddrFifoRead = dwBase + REG56301_HRXS;
	u32 i;

	for (i = 0; i < dwLength; i++) {
		do
			dwTimeout--;
		while ((!(HPIOS_MEMREAD32(dwAddrFifoStatus) & HSTR_HRRQ))
		       && dwTimeout);

		if (!dwTimeout)
			return (DPI_ERROR);
		else {
			*pdwData++ = HPIOS_MEMREAD32(dwAddrFifoRead);
		}
	}
	return (0);
}

/************************************************************************/
// Writing a block to the FIFO as 24bit words (only 16 used),
// protected against FIFO full. Returns error if FIFO stays full
short DpiData_WriteBlock32(u32 dwBase, u32 * pdwData, u32 dwLength)
{
	u32 dwTimeout = TIMEOUT;
	u32 dwAddrFifoStatus = dwBase + REG56301_HSTR;
	u32 dwAddrFifoWrite = dwBase + REG56301_HTXR;
	u32 dwD0, dwD1, dwD2, dwD3, dwD4, dwD5;
	u32 dwLeft;

	dwLeft = dwLength;
	while (dwLeft >= 3) {
// assemble 6 words to transmit
		dwD0 = *pdwData & 0xFFFF;
		dwD1 = *pdwData++ >> 16;
		dwD2 = *pdwData & 0xFFFF;
		dwD3 = *pdwData++ >> 16;
		dwD4 = *pdwData & 0xFFFF;
		dwD5 = *pdwData++ >> 16;

// wait for transmit FIFO to be empty (TRDY==1)
		dwTimeout = TIMEOUT;
		do
			dwTimeout--;
		while ((!(HPIOS_MEMREAD32(dwAddrFifoStatus) & HSTR_TRDY))
		       && dwTimeout);
		if (!dwTimeout)
			return (DPI_ERROR);

// write 6 words to FIFO (only have data in bottom 16bits)
		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD0);
		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD1);
		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD2);
		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD3);
		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD4);
		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD5);

		dwLeft -= 3;	//just sent 3x32bit words (6x16 bit words)
	}
// write remaining words

	while (dwLeft != 0) {
		if (DpiData_Write32(dwBase, pdwData))
			return (DPI_ERROR);
		pdwData++;
		dwLeft--;
	}
	return (0);
}
#endif

short DpiData_WriteBlock16(u32 dwBase, u16 * pwData, u32 dwLength)
{
	u32 dwTimeout = TIMEOUT;
	u32 dwAddrFifoStatus = dwBase + REG56301_HSTR;
	u32 dwAddrFifoWrite = dwBase + REG56301_HTXR;
	u32 dwD0, dwD1, dwD2, dwD3, dwD4, dwD5;
	u32 dwLeft;

// HPI_PRINT_DEBUG("Length = %ld\n", dwLength);
	dwLeft = dwLength;
	while (dwLeft >= 6) {
// assemble 6 words to transmit
		dwD0 = *pwData++ & 0xFFFF;
		dwD1 = *pwData++ & 0xFFFF;
		dwD2 = *pwData++ & 0xFFFF;
		dwD3 = *pwData++ & 0xFFFF;
		dwD4 = *pwData++ & 0xFFFF;
		dwD5 = *pwData++ & 0xFFFF;

// wait for transmit FIFO to be empty (TRDY==1)
		dwTimeout = TIMEOUT;
		do
			dwTimeout--;
		while ((!(HPIOS_MEMREAD32(dwAddrFifoStatus) & HSTR_TRDY))
		       && dwTimeout);
		if (!dwTimeout)
			return (DPI_ERROR);

// write 6 words to FIFO (only have data in bottom 16bits)
		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD0);
#ifdef PARANOID_FLAG_CHECK

		if (!(HPIOS_MEMREAD32(dwAddrFifoStatus) & HSTR_HTRQ))
			return (DPI_ERROR);
#endif

		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD1);
#ifdef PARANOID_FLAG_CHECK

		if (!(HPIOS_MEMREAD32(dwAddrFifoStatus) & HSTR_HTRQ))
			return (DPI_ERROR);
#endif

		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD2);
#ifdef PARANOID_FLAG_CHECK

		if (!(HPIOS_MEMREAD32(dwAddrFifoStatus) & HSTR_HTRQ))
			return (DPI_ERROR);
#endif

		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD3);
#ifdef PARANOID_FLAG_CHECK

		if (!(HPIOS_MEMREAD32(dwAddrFifoStatus) & HSTR_HTRQ))
			return (DPI_ERROR);
#endif

		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD4);
#ifdef PARANOID_FLAG_CHECK

		if (!(HPIOS_MEMREAD32(dwAddrFifoStatus) & HSTR_HTRQ))
			return (DPI_ERROR);
#endif

		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD5);

		dwLeft -= 6;	//just sent (6x16 bit words)
	}
// write remaining words

	while (dwLeft != 0) {
		if (DpiData_Write16(dwBase, pwData))
			return (DPI_ERROR);
		pwData++;
		dwLeft--;
	}
	return (0);
}

/************************************************************************/
// Writing a block to the FIFO, protected against FIFO full
// Returns error if FIFO stays full
/* old version
short DpiData_WriteBlock32(u32 dwBase, u32 *pdwData, u32 dwLength)
{
u32 dwTimeout = TIMEOUT;
u32 dwAddrFifoStatus = dwBase+REG56301_HSTR;
u32 dwAddrFifoWrite = dwBase + REG56301_HTXR;
u32 i;

for(i=0; i<dwLength; i++)
{
do dwTimeout--;
while ((!(HPIOS_MEMREAD32(dwAddrFifoStatus) & HSTR_HTRQ)) && dwTimeout);

if (!dwTimeout)
return (DPI_ERROR);
else
{
HPIOS_MEMWRITE32( dwAddrFifoWrite, *pdwData++);
}
}
return(0);
}
*/

/************************************************************************
// Writing a block to the FIFO, protected against FIFO full
// Returns error if FIFO stays full
// new version using MOVS gives ~10MB/s transfer rate
short DpiData_WriteBlock32(u32 dwBase, u32 *pdwData, u32 dwLength)
{
u32 dwTimeout = TIMEOUT;
u32 dwAddrFifoStatus = dwBase+REG56301_HSTR;
u32 dwAddrFifoWrite = dwBase + REG56301_HTXR;
u32 i;

#define BLK_LEN 2

for(i=0; i<dwLength/BLK_LEN; i++)
{
do dwTimeout--;
while ((!(HPIOS_MEMREAD32(dwAddrFifoStatus) & HSTR_HTRQ)) && dwTimeout);

if (!dwTimeout)
return (DPI_ERROR);
else
{
HPIOS_MEMWRITEBLK32(pdwData,dwAddrFifoWrite,BLK_LEN);
pdwData+=BLK_LEN;
}
}
return(0);
}
************************************************************************/
