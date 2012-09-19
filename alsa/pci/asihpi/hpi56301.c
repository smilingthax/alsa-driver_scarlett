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

Errata Notes
------------

ED46
Description (added 12/10/2001):

The following sequence gives erroneous results:
1) A different slave on the bus terminates a transaction (for
example, assertion of "stop" ).
2) Immediately afterwards (no more than one PCI clock), the chips
memory space control/status register at PCI address ADDR is read
in a single-word transaction. In this transaction, the chip drives to
the bus the data corresponding to the register at PCI address
ADDR+4, instead of the requested ADDR.

NOTE: ADDR is the PCI address of one of the following registers:
HCTR (ADDR=$10) , HSTR (ADDR=$14), or HCVR (ADDR=$18),
and not the data register.
Workaround:
The user should find a way to set/clear at least one bit in the control/status
registers to clearly differentiate between them. For example, you can set
HNMI in the HCVR, as this bit will always be 0 in the HSTR. If NMI
cannot be used, then HCVR{HV4,HV3,HV2} and
HSTR{HF5,HF4,HF3} can be set in any combinations that distinguish
between HCVR and HSTR data reads.
Pertains to:
DSP56301 Users Manual: Put this errata text as a note in the description
of the HCTR (p.
6-48), the HSTR (p. 6-57), and the HCVR (p. 6-59). These page numbers
are for Revision 3 of the manual.
DSP56305 Users Manual: Put this errata text as a note in the description
of the HCTR (p. 6-54), the HSTR (p. 6-68), and the HCVR (p. 6-72).
These page numbers are for Revision 1of the manual.

AudioScience workaround
-----------------------

1.      Save a copy of HCTR so it never needs to be read.

2.      Whenever we write HCVR set the CVR_NMI bit. See Dpi_SafeWriteHCVR().
Whenever we read HSTR we check that CVR_NMI from HCVR is not set. See Dpi_SafeReadHSTR().

3.      Whenever we read HCVR we make sure it matches the last value written (except possibly the CVR_INT bit).
See Dpi_SafeReadHCVR().

(C) Copyright AudioScience Inc. 1997-2006
************************************************************************/
// #define DEBUG
//#define USE_ASM_DATA
//#define USE_ASM_MSG
//#define TIMING_DEBUG  // writes to I/O port 0x182!

#include "hpi.h"
#include "hpidebug.h"
#include "hpicmn.h"
#include "hpidspcd.h"

#include "hpi56301.h"
#include "dpi56301.h"

#ifdef ASI_DRV_DEBUG
#include "asidrv.h"
#endif

#define ASIWDM_LogMessage(a)

#ifdef LOG_REGISTER_READS_AND_WRITES
#define RegLogLength (1024)

static char szHCTR[5] = "HCTR";
static char szHSTR[5] = "HSTR";
static char szHCVR[5] = "HCVR";
static char szHRXM[5] = "HRXM";
static char szHTXR[5] = "HTXR";
static char szUNKN[5] = "UNKN";

struct {
	char *pReg;
	u32 dwVal;
	int read;
	int error;
	int count;
} RegLog[RegLogLength] = {
0};
int RegLogIndex = 0;

int LogEntryMatchesLast(int NewIndex)
{
	int LastIndex;

	if (NewIndex == 0)
		LastIndex = RegLogLength - 1;
	else
		LastIndex = NewIndex - 1;

	if (RegLog[NewIndex].pReg != RegLog[LastIndex].pReg)
		return 0;
	if (RegLog[NewIndex].dwVal != RegLog[LastIndex].dwVal)
		return 0;
	if (RegLog[NewIndex].read != RegLog[LastIndex].read)
		return 0;
	if (RegLog[NewIndex].error != RegLog[LastIndex].error)
		return 0;

	RegLog[LastIndex].count++;
	return 1;
}

void LogRegisterRead(u32 offset, u32 value, int error)
{
	switch (offset) {
	case 0x10:
		RegLog[RegLogIndex].pReg = szHCTR;
		break;
	case 0x14:
		RegLog[RegLogIndex].pReg = szHSTR;
		break;
	case 0x18:
		RegLog[RegLogIndex].pReg = szHCVR;
		break;
	case 0x1C:
		RegLog[RegLogIndex].pReg = szHRXM;
		break;
	default:
		RegLog[RegLogIndex].pReg = szUNKN;
	}
	RegLog[RegLogIndex].dwVal = value;
	RegLog[RegLogIndex].read = 1;
	RegLog[RegLogIndex].error = error;
	RegLog[RegLogIndex].count = 1;

	if (!LogEntryMatchesLast(RegLogIndex)) {
		RegLogIndex++;
		if (RegLogIndex == RegLogLength)
			RegLogIndex = 0;
	}
}

void LogRegisterWrite(u32 offset, u32 value)
{
	switch (offset) {
	case 0x10:
		RegLog[RegLogIndex].pReg = szHCTR;
		break;
	case 0x14:
		RegLog[RegLogIndex].pReg = szHSTR;
		break;
	case 0x18:
		RegLog[RegLogIndex].pReg = szHCVR;
		break;
	case 0x1C:
		RegLog[RegLogIndex].pReg = szHTXR;
		break;
	default:
		RegLog[RegLogIndex].pReg = szUNKN;
	}
	RegLog[RegLogIndex].dwVal = value;
	RegLog[RegLogIndex].read = 0;
	RegLog[RegLogIndex].error = 0;
	RegLog[RegLogIndex].count = 1;

	if (!LogEntryMatchesLast(RegLogIndex)) {
		RegLogIndex++;
		if (RegLogIndex == RegLogLength)
			RegLogIndex = 0;
	}
}

void DumpRegisterLog(void)
{
	int DumpIndex = RegLogIndex;
	int Count;
	char buffer[128];

	ASIWDM_LogMessage("Starting HPI56301 register log.\r\n");

	for (Count = 0; Count < RegLogLength; Count++) {
		if (RegLog[DumpIndex].pReg != NULL) {
			if (RegLog[DumpIndex].count == 1)
				sprintf(buffer, "%s %s > %08.8X      %c\r\n",
					RegLog[DumpIndex].
					read ? "read " : "write",
					RegLog[DumpIndex].pReg,
					RegLog[DumpIndex].dwVal,
					RegLog[DumpIndex].error ? '!' : ' ');
			else
				sprintf(buffer, "%s %s > %08.8X [%d] %c\r\n",
					RegLog[DumpIndex].
					read ? "read " : "write",
					RegLog[DumpIndex].pReg,
					RegLog[DumpIndex].dwVal,
					RegLog[DumpIndex].count,
					RegLog[DumpIndex].error ? '!' : ' ');

			ASIWDM_WriteLogEntry(buffer);
		}
		DumpIndex++;
		if (DumpIndex == RegLogLength)
			DumpIndex = 0;
	}
}
#else				// #ifdef LOG_REGISTER_READS_AND_WRITES
#define LogRegisterRead(a,b,c)
#define LogRegisterWrite(a,b)
#define DumpRegisterLog()
#endif				// #ifdef LOG_REGISTER_READS_AND_WRITES

#ifdef ENABLE_MESSAGE_LOG
#include <stdarg.h>
#define MESSAGE_BUFFER_LENGTH   32768
#define MESSAGE_BUFFER_START_STRING_LENGTH      32

char MessageBuffer[MESSAGE_BUFFER_LENGTH] = "ASIHPIWN+HPI56301#msg#buf#start#";
int MessageBufferCurOffset = MESSAGE_BUFFER_START_STRING_LENGTH;

char MessageBufferEndString[14] = "#msg#buf#end#";

void MessageBuffer_Sprintf(const char *pFmt, ...)
{
	va_list args;
	char msg[80];
	int len, idx, n;

	va_start(args, pFmt);
	len = _vsnprintf(msg, 80, pFmt, args);
	va_end(args);

	for (idx = 0; idx < len; idx++) {
		MessageBuffer[MessageBufferCurOffset] = msg[idx];
		MessageBufferCurOffset++;
		if (MessageBufferCurOffset == MESSAGE_BUFFER_LENGTH)
			MessageBufferCurOffset =
			    MESSAGE_BUFFER_START_STRING_LENGTH;
	}
	n = MessageBufferCurOffset;
	for (idx = 0; idx < 13; idx++) {
		MessageBuffer[n] = MessageBufferEndString[idx];
		n++;
		if (n == MESSAGE_BUFFER_LENGTH)
			n = MESSAGE_BUFFER_START_STRING_LENGTH;
	}
}
#else
#define MessageBuffer_Sprintf(pfmt,...)
#endif

//////////////////////////////////////////////////////////////////////////
// DSP56301 Pci Boot code (multi-segment)
// The following code came from boot4000.asm, via makeboot.bat
///////////////////////////////////////////////////////////////////////
#ifndef __linux__
#pragma hdrstop			// allow headers above here to be precompiled
#endif
#include "boot4ka.h"

// DSP56301 PCI HOST registers (these are the offset from the base address)
// pMemBase is pointer to u32
#define REG56301_HCTR      (0x0010/sizeof(u32))	// HI32 control reg

#define HCTR_HTF0           0x0100
#define HCTR_HTF1           0x0200
#define HCTR_HRF0           0x0800
#define HCTR_HRF1           0x1000
#define HCTR_XFER_FORMAT    (HCTR_HTF0|HCTR_HRF0)
#define HCTR_XFER_FORMAT_MASK    (HCTR_HTF0|HCTR_HTF1|HCTR_HRF0|HCTR_HRF1)

#define REG56301_HSTR       (0x0014/sizeof(u32))	// HI32 status reg
#define HSTR_TRDY           0x0001	/* tx fifo empty */
#define HSTR_HTRQ           0x0002	/* txfifo not full */
#define HSTR_HRRQ           0x0004	/* rx fifo not empty */
#define HSTR_HF4            0x0010

#define REG56301_HCVR       (0x0018/sizeof(u32))	// HI32 command vector reg

#define REG56301_HTXR       (0x001C/sizeof(u32))	// Host Transmit data reg (FIFO)
#define REG56301_HRXS       (0x001C/sizeof(u32))	// Host Receive data reg (FIFO)

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

// u32 dwLast_HCVR=CVR_RESET|CVR_INT;

/**************************** LOCAL PROTOTYPES **************************/
static u32 Dpi_SafeReadHSTR(HPI_56301_INFO_OBJ * pio);
static u32 Dpi_SafeReadHCVR(HPI_56301_INFO_OBJ * pio);
static void Dpi_SafeWriteHCVR(HPI_56301_INFO_OBJ * pio, u32 dwHCVR);

static short Dpi_Command(HPI_56301_INFO_OBJ * pio, u32 Cmd);
static void Dpi_SetFlags(HPI_56301_INFO_OBJ * pio, short Flagval);
static short Dpi_WaitFlags(HPI_56301_INFO_OBJ * pio, short Flagval);
static short Dpi_GetFlags(HPI_56301_INFO_OBJ * pio);
static short DpiData_Read16(HPI_56301_INFO_OBJ * pio, u16 * pwData);
static short DpiData_Read32(HPI_56301_INFO_OBJ * pio, u32 * pdwData);
static short DpiData_Write32(HPI_56301_INFO_OBJ * pio, u32 * pdwData);
static short DpiData_WriteBlock16(HPI_56301_INFO_OBJ * pio, u16 * pwData,
				  u32 dwLength);
#ifdef WANT_UNUSED_FUNCTION_DECLARED
static short DpiData_ReadBlock32(u32 dwBase, u32 * pdwData, u32 dwLength);
static short DpiData_WriteBlock32(u32 dwBase, u32 * pdwData, u32 dwLength);
#endif
static short DpiData_Write24(HPI_56301_INFO_OBJ * pio, u32 * pdwData);

/**************************** EXPORTED FUNCTIONS ************************/
/************************************************************************/
// test out PCI i/f by writing a message block
// and then receiving it back again
#if 0
short Hpi56301_SelfTest(u32 pMemBase)
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
	dwData = pMemBase;
	i = dwData;
	dwData = i;

	return (0);
}
#endif
/************************************************************************/
// this function just checks that we have the correct adapter (in this case
// the 301 chip) at the indicated memory address

short Hpi56301_CheckAdapterPresent(HPI_56301_INFO_OBJ * pio)
{
	u32 dwData = 0;
//! Don't set TWSD
	HPIOS_MEMWRITE32(pio->pMemBase + REG56301_HCTR, 0xFFF7FFFFL);
	dwData = HPIOS_MEMREAD32(pio->pMemBase + REG56301_HCTR);
	if (dwData != 0x1DBFEL) {
		HPI_DEBUG_LOG1(DEBUG, "Read should be 1DBFE, but is %X\n",
			       dwData);
		return (1);	//error
	} else {
		u32 dwHSTR = 0, dwHCVR = 0;
		dwHSTR = HPIOS_MEMREAD32(pio->pMemBase + REG56301_HSTR);
		dwHCVR = HPIOS_MEMREAD32(pio->pMemBase + REG56301_HCVR);
		HPI_DEBUG_LOG3(VERBOSE,
			       "PCI registers, HCTR=%08X HSTR=%08X HCVR=%08X\n",
			       dwData, dwHSTR, dwHCVR);
		return (0);	//success
	}
}

/************************************************************************/
//short Hpi56301_BootLoadDsp( u32 pMemBase, u32  * apaDspCodeArrays[] ) old

short Hpi56301_BootLoadDsp(HPI_ADAPTER_OBJ * pao, HPI_56301_INFO_OBJ * pio,
			   u32 * pdwOsErrorCode)
{
	__iomem u32 *pMemBase = pio->pMemBase;
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

	Dpi_SetFlags(pio, HF_DSP_STATE_RESET);

#ifdef JTAGLOAD

//  Use this if not bootloading
	Dpi_Command(pio, CVR_RESTART + CVR_NMI);
	dwHSTR = 0;
	HPIOS_MEMWRITE32(pMemBase + REG56301_HCTR, dwHSTR);
	LogRegisterWrite(REG56301_HCTR, dwHSTR);
	HPIOS_DEBUG_STRING("reset\n");	//*************** debug
	return (0);
#endif

	HPIOS_DEBUG_STRING("Boot-");	//*************** debug
// If board isn't reset, try to reset it now
	i = 0;
	do {
		dwHSTR = HPIOS_MEMREAD32(pMemBase + REG56301_HSTR);
		bFlags = (short)(dwHSTR & HF_MASK);
		if (bFlags) {
			HPIOS_DEBUG_STRING("reset,");	//*************** debug
			Dpi_Command(pio, CVR_NMIRESET);
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
// Also set HS[2-0] to 111b to distinguish it from HSTR
	pio->dwHCTR = HCTR_XFER_FORMAT | 0x1C000;
	HPIOS_MEMWRITE32(pMemBase + REG56301_HCTR, pio->dwHCTR);
	LogRegisterWrite(REG56301_HCTR, pio->dwHCTR);

	HpiOs_DelayMicroSeconds(100);

	Dpi_SafeWriteHCVR(pio, 0);

// write length and starting address
	if (DpiData_Write24(pio, &dwDspCodeLength))
		return (DPI_ERROR_DOWNLOAD + 2);
	if (DpiData_Write24(pio, &dwDspCodeAddr))
		return (DPI_ERROR_DOWNLOAD + 3);

// and then actual code
// Skip array[2] which contains type
	for (i = 3; i < dwDspCodeLength + 3; i++) {
		dwData = (u32) adwDspCode_Boot4000a[(u16) i];
		if (DpiData_Write24(pio, &dwData))
			return (DPI_ERROR_DOWNLOAD + 4);
	}
// should we check somehow that bootloader is running ????
// use HF0-2 ??????

//////////////////////////////////////////////////////////////////////////////////
// SGT NOV-25-97 - get adapter ID from bootloader through HF3,4,5, wait until !=0
	HPIOS_DEBUG_STRING("Id,");	//*************** debug
	for (k = 10000; k != 0; k--) {
		dwHSTR = HPIOS_MEMREAD32(pMemBase + REG56301_HSTR);
		LogRegisterRead(REG56301_HSTR, dwHSTR, 0);
		if (dwHSTR & 0x0038)
			break;
	}
	if (k == 0)
		return (DPI_ERROR_DOWNLOAD + 5);	// 935

	wAdapterId = (unsigned short)(dwHSTR & 0x38) >> 3;

// assign DSP code based on adapter type returned
//if (wAdapterId==4) wAdapterId=6;     //************** assume 4500 for now ****************

// in DOS we load the DSP code from a file, so the DSP code
// structures are not used (and not linked in)
	switch (wAdapterId) {
	case 2:		// ASI4300
		nLoadFamily = Load4300;
		break;
	case 4:		// ASI4100
		nLoadFamily = Load4100;
		break;
	default:		// ASI4500
		nLoadFamily = Load4300;
		break;
	}

	HPI_DEBUG_LOG1(VERBOSE, "(Family = %x) \n", nLoadFamily);

	DspCode.psDev = pao->Pci.pOsData;
	nError = HpiDspCode_Open(nLoadFamily, &DspCode, pdwOsErrorCode);
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
		if (DpiData_Write24(pio, &dwDspCodeLength)) {
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

		if (DpiData_Write24(pio, &dwDspCodeAddr)) {
			nError = (DPI_ERROR_DOWNLOAD + 7);
			goto exit;
		}
		if (DpiData_Write24(pio, &dwDspCodeType)) {
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
			if (DpiData_Write24(pio, &dwData)) {
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

	if (Dpi_WaitFlags(pio, HF_PCI_HANDSHAKE)) {
		nError = (DPI_ERROR_DOWNLOAD + 10);	// Never got to "ready for mode change"
		goto exit;
	}
//EWB indicate finished with PCI reconfig
// DSP will now reset the PCI interface, PC mustn't access the
// PCI for a little while, so the timed delay is mandatory.
	Dpi_SetFlags(pio, HF_PCI_HANDSHAKE);
	HpiOs_DelayMicroSeconds(100000);

// Wait for transition to Idle - delay 100ms
// Full DRAM buffer init may take 50ms
	HPIOS_DEBUG_STRING("Wi,");	//*************** debug

//********************** steve test
//HpiOs_DelayMicroSeconds( 500000 ); /*SGT*/
//********************** steve test
#endif
//HpiOs_DelayMicroSeconds( 500000 );
//if (Dpi_WaitFlags(pMemBase,HF_DSP_STATE_IDLE))
//          return(DPI_ERROR_DOWNLOAD+11);  // Never got to idle

// SGT - use custom WaitFlags code that has an extra long timeout
	{
		u32 dwTimeout = 1000000;
		u16 wFlags = 0;
		do {
			wFlags = Dpi_GetFlags(pio);
		}
		while ((wFlags != HF_DSP_STATE_IDLE) && (--dwTimeout));
		if (!dwTimeout) {
			nError = (DPI_ERROR_DOWNLOAD + 11);	// Never got to idle
			goto exit;
		}
	}
	Dpi_SetFlags(pio, CMD_NONE);

	HPIOS_DEBUG_STRING("!\n");	//*************** debug

      exit:
	HpiDspCode_Close(&DspCode);
	return (nError);

}

/************************************************************************/
static u16 Hpi56301_Resync(HPI_56301_INFO_OBJ * pio)
{
	u16 wError;
//?  u32 dwData;
	u32 dwTimeout;

	Dpi_SetFlags(pio, CMD_NONE);
	Dpi_Command(pio, CVR_CMDRESET);
	wError = Dpi_WaitFlags(pio, HF_DSP_STATE_IDLE);
	dwTimeout = TIMEOUT;
	while (dwTimeout
	       && (HPIOS_MEMREAD32(pio->pMemBase + REG56301_HSTR) & HSTR_HRRQ))
	{
//?         dwData=HPIOS_MEMREAD32(pMemBase + REG56301_HRXS);
		dwTimeout--;
	}
	if (dwTimeout == 0)
		wError = HPI_ERROR_BAD_ADAPTER;
	return wError;

}

/************************************************************************/
static short Hpi56301_Send(HPI_56301_INFO_OBJ * pio,
			   u16 * pData, u32 dwLengthInWords, short wDataType)
{
//u32  * pdwData=(u32  *)pData;
	u16 *pwData = pData;
	u16 nErrorIndex = 0;
	u16 wError;
//u32 dwCount;
	u32 dwLength;
	u32 dwTotal;

	HPI_DEBUG_LOG3(VERBOSE, "Send, ptr=%p, len=%d, cmd=%d\n",
		       pData, dwLengthInWords, wDataType);
	HPI_DEBUG_DATA(pData, dwLengthInWords);

	wError = Dpi_WaitFlags(pio, HF_DSP_STATE_IDLE);
	if (wError) {
#ifdef ASI_DRV_DEBUG
//DBGPRINTF1( TEXT("!Dpi_WaitFlags %d!"),wError);
#endif
		nErrorIndex = 1;
		goto ErrorExit;	// error 911
	}
	if (!(Dpi_SafeReadHSTR(pio) & 1)) {
// XMIT FIFO NOT EMPTY
		DumpRegisterLog();
		HPI_DEBUG_LOG0(ERROR,
			       "Xmit FIFO not empty in Hpi56301_Send()\n");
		Hpi56301_Resync(pio);

		if (!(Dpi_SafeReadHSTR(pio) & 1) || wDataType != CMD_SEND_MSG) {
			nErrorIndex = 13;
			goto ErrorExit;
		}
	}
	HPI_DEBUG_LOG1(VERBOSE, "flags = %d\n", Dpi_GetFlags(pio));
	Dpi_SetFlags(pio, wDataType);
	HPI_DEBUG_LOG1(VERBOSE, "flags = %d\n", Dpi_GetFlags(pio));
	if (DpiData_Write32(pio, &dwLengthInWords)) {
		nErrorIndex = 2;
		goto ErrorExit;
	}			// error 912
	HPI_DEBUG_LOG1(VERBOSE, "flags = %d\n", Dpi_GetFlags(pio));
	if (Dpi_Command(pio, CVR_CMD)) {
		nErrorIndex = 3;
		goto ErrorExit;
	}			// error 913
	HPI_DEBUG_LOG1(VERBOSE, "flags = %d\n", Dpi_GetFlags(pio));
	if (Dpi_WaitFlags(pio, HF_DSP_STATE_CMD_OK)) {
		nErrorIndex = 4;
		goto ErrorExit;
	}			// error 914
// read back length before wrap
	if (DpiData_Read32(pio, &dwLength)) {
		HPI_DEBUG_LOG1(DEBUG, "DpiData_Read32 error line %d\n",
			       __LINE__);
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
	if (dwLength > dwLengthInWords) {
//ASIWDM_LogMessage("Returned length longer than requested in Hpi56301_Send().\r\n");
		DumpRegisterLog();
		dwLength = dwLengthInWords;
	}
	dwTotal = dwLength;
/* non-block way
for (dwCount=0; dwCount<dwTotal; dwCount++)
{  if (DpiData_Write32( dw56301Base,pdwData))
{nErrorIndex=6; goto ErrorExit;}
pdwData++;
}*/
// block way - SGT JUL-13-98
//  if(DpiData_WriteBlock32( dw56301Base, pdwData, dwTotal))
	if (DpiData_WriteBlock16(pio, pwData, dwTotal)) {
		nErrorIndex = 6;	// error 916
		goto ErrorExit;
	}

	if (dwLength < dwLengthInWords) {
		if (Dpi_Command(pio, CVR_DMASTOP)) {
			nErrorIndex = 7;
			goto ErrorExit;
		}		// error 917
// Wait for DMA to be reprogrammed for second block
		if (Dpi_WaitFlags(pio, HF_DSP_STATE_WRAP_READY)) {
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
		if (DpiData_WriteBlock16(pio, pwData, dwTotal)) {
			nErrorIndex = 9;	// error 919
			goto ErrorExit;
		}
	}
/////////////////////////////////////////////// SGT test transfer time, end
//outportb(0x378,0);

	if (Dpi_Command(pio, CVR_DMASTOP)) {
		nErrorIndex = 10;
		goto ErrorExit;
	}			// error 920

	if (wDataType == CMD_SEND_MSG) {
		if (Dpi_WaitFlags(pio, HF_DSP_STATE_RESP_READY)) {
			nErrorIndex = 11;
			goto ErrorExit;
		}		// error 921
	} else {
		if (Dpi_WaitFlags(pio, HF_DSP_STATE_IDLE)) {
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
	if (Hpi56301_Resync(pio))
		nErrorIndex += 1000;
	return (DPI_ERROR_SEND + nErrorIndex);
}

/************************************************************************/
static short Hpi56301_Get(HPI_56301_INFO_OBJ * pio,
			  u16 * pData,
			  u32 * pdwLengthInWords,
			  short wDataType, u32 dwMaxLengthInWords)
{
	u32 dwLength1, dwLength2;
//  u32  * pdwData = (u32  *) pData;
	u16 *pwData = pData;
	u16 nErrorIndex = 0;
	u32 dwCount;
	u32 dwBeforeWrap, dwAfterWrap;
	u32 dwJunk;
	u32 *pdwJunk = &dwJunk;

	HPI_DEBUG_LOG2(VERBOSE, "Get, ptr=%p, cmd=%d\n", pData, wDataType);
	MessageBuffer_Sprintf("Get, ptr =0x%lx, len = 0x%x, cmd = %d\n", pData,
			      dwMaxLengthInWords * 2, wDataType);

// Wait for indication that previous command has been processed
	if (wDataType == CMD_GET_RESP) {
		if (Dpi_WaitFlags(pio, HF_DSP_STATE_RESP_READY)) {
			nErrorIndex = 1;
			goto ErrorExit;
		}		// error 951
	} else {
		if (Dpi_WaitFlags(pio, HF_DSP_STATE_IDLE)) {
			nErrorIndex = 2;
			goto ErrorExit;
		}		// error 952
	}
// Tell DSP what to do next.
	Dpi_SetFlags(pio, wDataType);
	if (Dpi_Command(pio, CVR_CMD)) {
		nErrorIndex = 3;
		goto ErrorExit;
	}			// error 953

	if (Dpi_WaitFlags(pio, HF_DSP_STATE_CMD_OK)) {
		nErrorIndex = 4;
		goto ErrorExit;
	}			// error 954

// returned length in 16 bit words
	if (DpiData_Read32(pio, &dwLength1)) {
		HPI_DEBUG_LOG1(DEBUG, "DpiData_Read32 error line %d\n",
			       __LINE__);
		nErrorIndex = 5;
		goto ErrorExit;
	}			// error 955
// dwBeforeWrap = ((dwLength1+1)/2);
	dwBeforeWrap = dwLength1;
// length of second part of data, may be 0
	if (DpiData_Read32(pio, &dwLength2)) {
		HPI_DEBUG_LOG1(DEBUG, "DpiData_Read32 error line %d\n",
			       __LINE__);
		nErrorIndex = 5;
		goto ErrorExit;
	}			// error 955
	dwAfterWrap = dwLength2;
	*pdwLengthInWords = dwLength1 + dwLength2;	// AGE Feb 19 98

	if (dwBeforeWrap + dwAfterWrap > dwMaxLengthInWords) {
// XMIT FIFO NOT EMPTY
		DumpRegisterLog();
		if (dwBeforeWrap > dwMaxLengthInWords) {
			dwBeforeWrap = dwMaxLengthInWords;
			dwAfterWrap = 0;
		} else {
			dwAfterWrap = dwMaxLengthInWords - dwBeforeWrap;
		}
	}
// read the first part of data
	for (dwCount = 0; dwCount < dwBeforeWrap; dwCount++) {
		if (DpiData_Read16(pio, pwData)) {
			nErrorIndex = 6;
			goto ErrorExit;
		}
		pwData++;
	}			// error 956

// check for a bad value of dwAfterWrap
	if (dwAfterWrap && (wDataType == CMD_GET_RESP)) {
		HPI_DEBUG_LOG1(DEBUG,
			       "Unexpected data wrap for response, got %u\n",
			       dwAfterWrap);
		dwAfterWrap = 0;	// this may fix the error - it depends if the DSP is in error or not.
	}

	if (dwAfterWrap)	// Handshake, then read second block
	{
		if (Dpi_Command(pio, CVR_DMASTOP)) {
			HPI_DEBUG_LOG1(DEBUG, "Dpi_Command %d\n", __LINE__);
			nErrorIndex = 7;
			goto ErrorExit;
		}		// error 957
		if (Dpi_WaitFlags(pio, HF_DSP_STATE_WRAP_READY)) {
			HPI_DEBUG_LOG1(DEBUG, "Dpi_WaitFlags %d\n", __LINE__);
			nErrorIndex = 8;
			goto ErrorExit;
		}		// error 958
/* Clean the extra data out of the FIFO */
		if (DpiData_Read32(pio, pdwJunk)) {
			HPI_DEBUG_LOG1(DEBUG, "DpiData_Read32 error line %d\n",
				       __LINE__);
			nErrorIndex = 9;
			goto ErrorExit;
		}		// error 959
		if (DpiData_Read32(pio, pdwJunk)) {
			HPI_DEBUG_LOG1(DEBUG, "DpiData_Read32 error line %d\n",
				       __LINE__);
			nErrorIndex = 9;
			goto ErrorExit;
		}		// error 959
		if (DpiData_Read32(pio, pdwJunk)) {
			HPI_DEBUG_LOG1(DEBUG, "DpiData_Read32 error line %d\n",
				       __LINE__);
			nErrorIndex = 9;
			goto ErrorExit;
		}		// error 959
		for (dwCount = 0; dwCount < dwAfterWrap; dwCount++) {
			if (DpiData_Read16(pio, pwData)) {
				nErrorIndex = 10;
				goto ErrorExit;
			}
			pwData++;
		}		// error 960
	}

	if (Dpi_Command(pio, CVR_DMASTOP)) {
		nErrorIndex = 11;
		goto ErrorExit;
	}			// error 961
	if (Dpi_WaitFlags(pio, HF_DSP_STATE_IDLE)) {
		nErrorIndex = 12;
		goto ErrorExit;
	}			// error 962

/* Clean the extra data out of the FIFO */
	if (DpiData_Read32(pio, pdwJunk)) {
		HPI_DEBUG_LOG1(DEBUG, "DpiData_Read32 error line %d\n",
			       __LINE__);
		nErrorIndex = 13;
		goto ErrorExit;
	}			// error 963
	if (DpiData_Read32(pio, pdwJunk)) {
		HPI_DEBUG_LOG1(DEBUG, "DpiData_Read32 error line %d\n",
			       __LINE__);
		nErrorIndex = 14;
		goto ErrorExit;
	}			// error 964
	if (DpiData_Read32(pio, pdwJunk)) {
		HPI_DEBUG_LOG1(DEBUG, "DpiData_Read32 error line %d\n",
			       __LINE__);
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
	if (Hpi56301_Resync(pio))
		nErrorIndex += 1000;
	return (DPI_ERROR_GET + nErrorIndex);
}

/************************************************************************/
void Hpi56301_Message(HPI_56301_INFO_OBJ * pio, HPI_MESSAGE * phm,
		      HPI_RESPONSE * phr)
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

	nError = Hpi56301_Send(pio,
			       (u16 *) phm,
			       (phm->wSize / sizeof(u32)) * sizeof(u16),
			       CMD_SEND_MSG);
	if (nError) {
		phr->wError = nError;
		return;
	}

	HPIOS_DEBUG_STRING("B");	// *************** debug
	nError = Hpi56301_Get(pio,
			      (u16 *) phr,
			      &dwSize,
			      CMD_GET_RESP, sizeof(HPI_RESPONSE) / sizeof(u16));
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
		nError = Hpi56301_Send(pio,
				       (u16 *) phm->u.d.u.Data.pbData,
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
		nError = Hpi56301_Get(pio,
				      (u16 *) phm->u.d.u.Data.pbData,
				      &dwSize,
				      CMD_GET_DATA,
				      phm->u.d.u.Data.dwDataSize / sizeof(u16));
		if (nError) {
			phr->wError = nError;
			return;
		}

		if (phm->u.d.u.Data.dwDataSize != (dwSize * 2)) {	// mismatch in requested and received data size
			phr->wError = HPI_ERROR_INVALID_DATA_TRANSFER;
		}
	}

	HPIOS_DEBUG_STRING("G");	// *************** debug
	return;
}

/**************************** LOCAL FUNCTIONS ***************************/
/************************************************************************/
static u32 Dpi_SafeReadHSTR(HPI_56301_INFO_OBJ * pio)
{
	u32 dwHSTR;
	u32 dwTimeout = 5;

	do {
		dwTimeout--;
		dwHSTR = HPIOS_MEMREAD32(pio->pMemBase + REG56301_HSTR);
		if (dwHSTR & CVR_NMI) {
			LogRegisterRead(REG56301_HSTR, dwHSTR, 1);
			HPI_DEBUG_LOG1(DEBUG,
				       "Dpi_SafeReadHSTR() bad read.  Value:0x%08x\r\n",
				       dwHSTR);
		} else
			break;
	}
	while (dwTimeout);

	if (dwTimeout) {
		LogRegisterRead(REG56301_HSTR, dwHSTR, 0);
	} else {
		HPI_DEBUG_LOG1(ERROR,
			       "Dpi_SafeReadHSTR() failed.  Value:0x%08x\r\n",
			       dwHSTR);
	}
	return dwHSTR;
}

static u32 Dpi_SafeReadHCVR(HPI_56301_INFO_OBJ * pio)
{
	u32 dwHCVR;
	u32 dwTimeout = 5;

	do {
		dwTimeout--;
		dwHCVR = HPIOS_MEMREAD32(pio->pMemBase + REG56301_HCVR);
		if ((dwHCVR | CVR_INT) != pio->dwHCVR) {
			LogRegisterRead(REG56301_HCVR, dwHCVR, 1);
			HPI_DEBUG_LOG1(DEBUG,
				       "Dpi_SafeReadHCVR() bad read.  Value:0x%08x\r\n",
				       dwHCVR);
		} else
			break;
	}
	while (dwTimeout);

	if (dwTimeout) {
		LogRegisterRead(REG56301_HCVR, dwHCVR, 0);
	} else {
		HPI_DEBUG_LOG1(ERROR,
			       "Dpi_SafeReadHCVR() failed.  Value:0x%08x\r\n",
			       dwHCVR);
	}
	return dwHCVR;
}

static void Dpi_SafeWriteHCVR(HPI_56301_INFO_OBJ * pio, u32 dwHCVR)
{
	HPIOS_MEMWRITE32(pio->pMemBase + REG56301_HCVR, dwHCVR | CVR_NMI);
	LogRegisterWrite(REG56301_HCVR, dwHCVR | CVR_NMI);
	pio->dwHCVR = dwHCVR | CVR_NMI | CVR_INT;
}

static void Dpi_SetFlags(HPI_56301_INFO_OBJ * pio, short Flagval)
{
	pio->dwHCTR = (pio->dwHCTR & HF_CLEAR) | Flagval;

	HPIOS_MEMWRITE32(pio->pMemBase + REG56301_HCTR, pio->dwHCTR);
	LogRegisterWrite(REG56301_HCTR, pio->dwHCTR);
}

/************************************************************************
Wait for specified flag value to appear in the HSTR
Return 0 if it does, DPI_ERROR if we time out waiting
************************************************************************/

static short Dpi_WaitFlags(HPI_56301_INFO_OBJ * pio, short Flagval)
{
//      u32 dwHSTR;
	u16 wFlags;
	u32 dwTimeout = TIMEOUT;

// HPIOS_DEBUG_STRING("Wf,");

	do {
		wFlags = Dpi_GetFlags(pio);
		if (wFlags == Flagval)
			break;
	}
	while (--dwTimeout);
	HPI_DEBUG_LOG3(VERBOSE, "Expecting %d, got %d%s\r\n",
		       Flagval, wFlags, !dwTimeout ? " timeout" : "");
	if (!dwTimeout) {
		HPI_DEBUG_LOG2(DEBUG,
			       "Dpi_WaitFlags()  Expecting %d, got %d\r\n",
			       Flagval, wFlags);
		return (DPI_ERROR + wFlags);
	} else
		return (0);
}

/************************************************************************/
static short Dpi_GetFlags(HPI_56301_INFO_OBJ * pio)
{
	u32 dwHSTR;

	dwHSTR = Dpi_SafeReadHSTR(pio);
	return ((short)(dwHSTR & HF_MASK));
}

/************************************************************************/
// Wait for CVR_INT bits to be set to 0
static u32 Dpi_Command_Handshake(HPI_56301_INFO_OBJ * pio, u32 Cmd)
{
	u32 dwTimeout;
	u32 dwHCVR;
//      u32 dwHCTR;

////HPIOS_DEBUG_STRING("Dc,");

// make sure HCVR-HC is zero before writing to HCVR
	dwTimeout = TIMEOUT;
	do {
		dwTimeout--;
		dwHCVR = Dpi_SafeReadHCVR(pio);
	}
	while ((dwHCVR & CVR_INT) && dwTimeout);
	HPI_DEBUG_LOG2(VERBOSE, "(cmd = %d) attempts = %d\n",
		       Cmd, TIMEOUT - dwTimeout);

	return dwTimeout;
}

static short Dpi_Command(HPI_56301_INFO_OBJ * pio, u32 Cmd)
{
	if (!Dpi_Command_Handshake(pio, Cmd))
		return (DPI_ERROR);

	Dpi_SafeWriteHCVR(pio, Cmd | CVR_INT);

/* Wait for interrupt to be acknowledged */
//      if(!Dpi_Command_Handshake(pio,Cmd))
//              return (DPI_ERROR);

	return (0);
}

/************************************************************************/
static u32 DpiData_ReadHandshake(HPI_56301_INFO_OBJ * pio)
{
	u32 dwTimeout;
	u32 dwHSTR;
	dwTimeout = TIMEOUT;
	do {
		dwTimeout--;
		dwHSTR = Dpi_SafeReadHSTR(pio);
	}
	while ((!(dwHSTR & HSTR_HRRQ)) && dwTimeout);

	if (TIMEOUT - dwTimeout > 500) {
		HPI_DEBUG_LOG1(DEBUG, "DpiData_ReadHandshake() attempts = %d\n",
			       TIMEOUT - dwTimeout);
	}

	if (!dwTimeout) {
		HPI_DEBUG_LOG1(DEBUG, "DataRead - FIFO is staying empty"
			       " HSTR=%08X\n", dwHSTR);
	}
	return dwTimeout;
}

/************************************************************************/
// Reading the FIFO, protected against FIFO empty
// Returns DPI_ERROR if FIFO stays empty for TIMEOUT loops
static short DpiData_Read32(HPI_56301_INFO_OBJ * pio, u32 * pdwData)
{
	u32 dwD1 = 0, dwD2 = 0;

	if (!DpiData_ReadHandshake(pio))
		return (DPI_ERROR);

	dwD1 = HPIOS_MEMREAD32(pio->pMemBase + REG56301_HRXS);
	LogRegisterRead(REG56301_HRXS, dwD1, 0);

	if (!DpiData_ReadHandshake(pio))
		return (DPI_ERROR);

	dwD2 = HPIOS_MEMREAD32(pio->pMemBase + REG56301_HRXS);
	LogRegisterRead(REG56301_HRXS, dwD2, 0);

	*pdwData = (dwD2 << 16) | (dwD1 & 0xFFFF);
	return (0);
}

static short DpiData_Read16(HPI_56301_INFO_OBJ * pio, u16 * pwData)
{
	u32 dwD1 = 0;

	if (!DpiData_ReadHandshake(pio))
		return (DPI_ERROR);

	dwD1 = HPIOS_MEMREAD32(pio->pMemBase + REG56301_HRXS);
	LogRegisterRead(REG56301_HRXS, dwD1, 0);
	dwD1 &= 0xFFFF;
	*pwData = (u16) dwD1;
	return (0);
}

/************************************************************************/
static u32 DpiData_WriteHandshake(HPI_56301_INFO_OBJ * pio, u32 dwBitMask)
{
	u32 dwTimeout;
	u32 dwHSTR;

	dwTimeout = TIMEOUT;
	do {
		dwTimeout--;
		dwHSTR = Dpi_SafeReadHSTR(pio);
	}
	while ((!(dwHSTR & dwBitMask)) && dwTimeout);
	if (!dwTimeout) {
		HPI_DEBUG_LOG1(DEBUG, "DataWrite - FIFO is staying full"
			       " HSTR=%08X\n", dwHSTR);
	}
	return dwTimeout;
}

/************************************************************************/
// Writing the FIFO, protected against FIFO full
// Returns error if FIFO stays full
static short DpiData_Write32(HPI_56301_INFO_OBJ * pio, u32 * pdwData)
{
	u32 dwD1, dwD2;

	dwD1 = *pdwData & 0xFFFF;
	dwD2 = *pdwData >> 16;

	if (!DpiData_WriteHandshake(pio, HSTR_HTRQ))
		return (DPI_ERROR);

	HPIOS_MEMWRITE32(pio->pMemBase + REG56301_HTXR, dwD1);
	LogRegisterWrite(REG56301_HTXR, dwD1);
/*
HPI_DEBUG_LOG1(VERBOSE,"Wrote (lo) 0x%08lx, hstr = 0x%08lx," \
" attempts = %ld\n",
dwD1, dwHSTR, TIMEOUT - dwTimeout);
*/

	if (!DpiData_WriteHandshake(pio, HSTR_HTRQ))
		return (DPI_ERROR);

	HPIOS_MEMWRITE32(pio->pMemBase + REG56301_HTXR, dwD2);
	LogRegisterWrite(REG56301_HTXR, dwD2);
/*
HPI_DEBUG_LOG1(VERBOSE,"Wrote (hi) 0x%08lx, hstr = 0x%08lx," \
" attempts = %ld\n",
dwD2, dwHSTR, TIMEOUT - dwTimeout);
*/
	return (0);
}

/************************************************************************/
// Writing the FIFO, protected against FIFO full
// Returns error if FIFO stays full
static short DpiData_Write24(HPI_56301_INFO_OBJ * pio, u32 * pdwData)
{
	if (!DpiData_WriteHandshake(pio, HSTR_HTRQ))
		return (DPI_ERROR);

	HPIOS_MEMWRITE32(pio->pMemBase + REG56301_HTXR, *pdwData);
	LogRegisterWrite(REG56301_HTXR, *pdwData);
	return (0);
}

/************************************************************************/
// Writing the FIFO, protected against FIFO full
// Returns error if FIFO stays full
static short DpiData_Write16(HPI_56301_INFO_OBJ * pio, u16 * pwData)
{
	u32 wData;

	if (!DpiData_WriteHandshake(pio, HSTR_HTRQ))
		return (DPI_ERROR);

	wData = (u32) * pwData;
	HPIOS_MEMWRITE32(pio->pMemBase + REG56301_HTXR, wData);
	LogRegisterWrite(REG56301_HTXR, wData);
//  HPI_DEBUG_LOG1(VERBOSE,"Wrote 0x%08lx, hstr = 0x%08lx, attempts = %ld\n",
//                    wData, dwHSTR, TIMEOUT - dwTimeout);
	return (0);
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
		LogRegisterWrite(REG56301_HTXR, dwD0);
		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD1);
		LogRegisterWrite(REG56301_HTXR, dwD1);
		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD2);
		LogRegisterWrite(REG56301_HTXR, dwD2);
		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD3);
		LogRegisterWrite(REG56301_HTXR, dwD3);
		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD4);
		LogRegisterWrite(REG56301_HTXR, dwD4);
		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD5);
		LogRegisterWrite(REG56301_HTXR, dwD5);

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

static short DpiData_WriteBlock16(HPI_56301_INFO_OBJ * pio, u16 * pwData,
				  u32 dwLength)
{
	__iomem void *dwAddrFifoWrite = pio->pMemBase + REG56301_HTXR;
	u32 dwD0, dwD1, dwD2, dwD3, dwD4, dwD5;
	u32 dwLeft;

// HPI_DEBUG_LOG1(VERBOSE,"Length = %ld\n", dwLength);
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
		if (!DpiData_WriteHandshake(pio, HSTR_TRDY))
			return (DPI_ERROR);

// write 6 words to FIFO (only have data in bottom 16bits)
		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD0);
		LogRegisterWrite(REG56301_HTXR, dwD0);
#ifdef PARANOID_FLAG_CHECK
		if (!DpiData_WriteHandshake(pio, HSTR_HTRQ))
			return (DPI_ERROR);
#endif

		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD1);
		LogRegisterWrite(REG56301_HTXR, dwD1);
#ifdef PARANOID_FLAG_CHECK
		if (!DpiData_WriteHandshake(pio, HSTR_HTRQ))
			return (DPI_ERROR);
#endif

		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD2);
		LogRegisterWrite(REG56301_HTXR, dwD2);
#ifdef PARANOID_FLAG_CHECK
		if (!DpiData_WriteHandshake(pio, HSTR_HTRQ))
			return (DPI_ERROR);
#endif

		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD3);
		LogRegisterWrite(REG56301_HTXR, dwD3);
#ifdef PARANOID_FLAG_CHECK
		if (!DpiData_WriteHandshake(pio, HSTR_HTRQ))
			return (DPI_ERROR);
#endif

		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD4);
		LogRegisterWrite(REG56301_HTXR, dwD4);
#ifdef PARANOID_FLAG_CHECK
		if (!DpiData_WriteHandshake(pio, HSTR_HTRQ))
			return (DPI_ERROR);
#endif

		HPIOS_MEMWRITE32(dwAddrFifoWrite, dwD5);
		LogRegisterWrite(REG56301_HTXR, dwD5);

		dwLeft -= 6;	//just sent (6x16 bit words)
	}
// write remaining words

	while (dwLeft != 0) {
		if (DpiData_Write16(pio, pwData))
			return (DPI_ERROR);
		pwData++;
		dwLeft--;
	}

// wait for DSP to empty the fifo prior to issuing DMASTOP
	if (!DpiData_WriteHandshake(pio, HSTR_TRDY))
		return (DPI_ERROR);

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
