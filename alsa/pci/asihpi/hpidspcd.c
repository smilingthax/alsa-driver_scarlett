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

(Linux only:) If DSPCODE_FIRMWARE_LOADER is defined, code is read using
hotplug firmware loader from individual dsp code files

If neither of the above is defined, code is read from linked arrays.
DSPCODE_ARRAY is defined.

HPI_INCLUDE_**** must be defined
and the appropriate hzz?????.c or hex?????.c linked in

If USE_ZLIB is defined, hpizlib.c must also be linked

 */
/***********************************************************************/
#include "hpidspcd.h"
#include "hpidebug.h"

#ifndef HPI_KERNEL_MODE
#include <string.h>
#endif

/**
 Header structure for binary dsp code file (see asidsp.doc)
 This structure must match that used in s2bin.c for generation of asidsp.bin
 */

#ifndef DISABLE_PRAGMA_PACK1
#pragma pack(push,1)
#endif

typedef struct {
    HW32 size;
    char type[4];
    HW32 adapter;
    HW32 version;
    HW32 crc;
} header_t;

#ifndef DISABLE_PRAGMA_PACK1
#pragma pack(pop)
#endif

/***********************************************************************/
#ifdef DSPCODE_FIRMWARE
#include "linux/pci.h"
/*-------------------------------------------------------------------*/
short HpiDspCode_Open(
    HW32 nAdapter,
    DSP_CODE * psDspCode,
	HW32 * pdwOsErrorCode )
{
    const struct firmware *psFirmware = psDspCode->psFirmware;
    header_t header;
    char fw_name[20];
	int err;

    sprintf(fw_name,"asihpi/dsp%04x.bin",nAdapter);
	HPI_DEBUG_LOG1(INFO,"Requesting firmware for %s\n",fw_name);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
    if (0 != (err=request_firmware(&psFirmware, fw_name, psDspCode->psDev->slot_name )))
#else
		if (0 != (err=request_firmware(&psFirmware, fw_name, &psDspCode->psDev->dev)) )
#endif
    {
		HPI_DEBUG_LOG2(ERROR,"%d, request_firmware failed for  %s\n",err,fw_name);
	goto error1;
    }
    if  (psFirmware->size < sizeof(header)) {
		HPI_DEBUG_LOG1(ERROR,"Header size too small %s\n",fw_name);
	goto error2;
    }
    memcpy(&header,psFirmware->data,sizeof(header));
    if (header.adapter != nAdapter) {
		HPI_DEBUG_LOG2(ERROR,"Adapter type incorrect %4x != %4x\n",header.adapter,nAdapter);
	goto error2;
    }
    if (header.size != psFirmware->size) {
		HPI_DEBUG_LOG2(ERROR,"Code size wrong  %d != %ld\n",header.size,(unsigned long)psFirmware->size);
	goto error2;
    }

	HPI_DEBUG_LOG1(INFO,"Dsp code %s opened\n",fw_name);
    psDspCode->psFirmware = psFirmware;
    psDspCode->dwBlockLength  = header.size/sizeof(HW32);
    psDspCode->dwWordCount=sizeof(header)/sizeof(HW32); // start pointing to data
	psDspCode->dwVersion = header.version;
	psDspCode->dwCrc = header.crc;
    return 0;

 error2:
    release_firmware(psFirmware);
 error1:
    // HPI_DEBUG_LOG1(ERROR,"Firmware %s not available\n",fw_name);
    psDspCode->psFirmware = NULL;
    psDspCode->dwBlockLength  = 0;
    return ( HPI_ERROR_DSP_FILE_NOT_FOUND );
}

/*-------------------------------------------------------------------*/
void HpiDspCode_Close(
    DSP_CODE * psDspCode )
{
    if (psDspCode->psFirmware != NULL)
   {
       HPI_DEBUG_LOG0(DEBUG,"Dsp code closed\n");
       release_firmware(psDspCode->psFirmware);
       psDspCode->psFirmware = NULL;
    }
}

/*-------------------------------------------------------------------*/
void HpiDspCode_Rewind(
    DSP_CODE * psDspCode )
{
    psDspCode->dwWordCount=sizeof(header_t)/sizeof(HW32); // start pointing to data
}

/*-------------------------------------------------------------------*/
short HpiDspCode_ReadWord(
    DSP_CODE * psDspCode,
    HW32 * pdwWord )
{
    if (psDspCode->dwWordCount+1 > psDspCode->dwBlockLength)
        return(HPI_ERROR_DSP_FILE_FORMAT);

    *pdwWord = ((HW32 *)(psDspCode->psFirmware->data))[psDspCode->dwWordCount];
    psDspCode->dwWordCount++;
    return 0;
}

/*-------------------------------------------------------------------*/
short HpiDspCode_ReadBlock(
    size_t nWordsRequested,
    DSP_CODE * psDspCode,
    HW32 * * ppdwBlock)
{
    if (psDspCode->dwWordCount + nWordsRequested > psDspCode->dwBlockLength)
        return(HPI_ERROR_DSP_FILE_FORMAT);

    *ppdwBlock = ((HW32 *)(psDspCode->psFirmware->data))+psDspCode->dwWordCount;
    psDspCode->dwWordCount += nWordsRequested;
    return (0);
}
#endif //not  defined HPI_OS_LINUX

/********************************************************************/
/* Load dspcode from file asidsp.bin */
#ifdef DSPCODE_FILE

/* must match or be greater than the same constant in S2BIN.C */
#define BLOCK_LIMIT_DWORD 2048L
static HW32 HFAR aCodeBuffer[BLOCK_LIMIT_DWORD];


/*-------------------------------------------------------------------*/

short HpiDspCode_Open(
    HW32 nAdapter,
    DSP_CODE * psDspCode,
	HW32 * pdwOsErrorCode )
{
	HW16 wError;
    int i,nRead=(int)NO_FILE;
    header_t header;
    char	*pszFilepath;

    pszFilepath = HpiOs_GetDspCodePath(nAdapter);

    wError = HpiOs_fopen_rb(pszFilepath,&psDspCode->pDspCodeFile,pdwOsErrorCode);
    if  (wError != 0)
        return ( wError );

    // seek for first CODE header
    for (i=0; i<1000; i++)
    {
        HpiOs_fseek(psDspCode->pDspCodeFile,i,SEEK_SET);
        nRead=HpiOs_fread(&header, 1, sizeof(header), psDspCode->pDspCodeFile);
        if (nRead < sizeof(header))
        {
            HPI_DEBUG_LOG0(DEBUG,"Ran out of data\n");
            break;
        }
        if (strncmp(header.type,"CODE",4)==0)
            break;
    }
    if ((nRead < sizeof(header)) || (i==1000))
    {
        HpiOs_fclose ( psDspCode->pDspCodeFile );
        HPI_DEBUG_LOG0(DEBUG,"CODE header not found\n");
        return ( HPI_ERROR_DSP_FILE_NO_HEADER );
    }
    // walk CODE headers searching for correct one
    while ( (nRead == sizeof(header)) && (header.adapter != nAdapter) )
    {
		HPI_DEBUG_LOG1(VERBOSE,"Skipping %04lx ",header.adapter);
		if(header.size == 0)
		{
		    HpiOs_fclose ( psDspCode->pDspCodeFile );
	        HPI_DEBUG_LOG0(DEBUG,"NULL header found\n");
	        return ( HPI_ERROR_DSP_FILE_NULL_HEADER );
		}
        HpiOs_fseek(psDspCode->pDspCodeFile, (header.size-sizeof(header)), SEEK_CUR);
        nRead=HpiOs_fread(&header, 1, sizeof(header), psDspCode->pDspCodeFile);
    }
    if  ((nRead < sizeof(header)) || (header.adapter != nAdapter))
    {
        HpiOs_fclose ( psDspCode->pDspCodeFile );
        HPI_DEBUG_LOG1(ERROR,"Adapter type %04x not found\n",nAdapter);
        return ( HPI_ERROR_DSP_SECTION_NOT_FOUND );
    }

	psDspCode->nAdapter = nAdapter;
    psDspCode->dwBlockLength  = (header.size-sizeof(header))/4;
    psDspCode->dwWordCount=0;
	psDspCode->dwVersion = header.version;
	psDspCode->dwCrc = header.crc;
    return 0;
}

/*-------------------------------------------------------------------*/
void HpiDspCode_Rewind(
    DSP_CODE * psDspCode )
{
	HpiDspCode_Close(psDspCode);
	HpiDspCode_Open(psDspCode->nAdapter,psDspCode,NULL);
}

/*-------------------------------------------------------------------*/
void HpiDspCode_Close(
    DSP_CODE * psDspCode )
{
    if (psDspCode->pDspCodeFile != NO_FILE)
    {
        HpiOs_fclose ( psDspCode->pDspCodeFile );
        psDspCode->pDspCodeFile = NO_FILE;
    }
}


/*-------------------------------------------------------------------*/
short HpiDspCode_ReadWord(
    DSP_CODE * psDspCode,
    HW32 * pdwWord )
{
    size_t count;
    count=HpiOs_fread(pdwWord,sizeof(HW32),1,psDspCode->pDspCodeFile);
    if (count!=1)
        return(HPI_ERROR_DSP_FILE_FORMAT);
    // could also calculate checksum here?
    psDspCode->dwWordCount++;
    if (psDspCode->dwWordCount > psDspCode->dwBlockLength)
        return(HPI_ERROR_DSP_FILE_FORMAT);
    return 0;
}

/*-------------------------------------------------------------------*/
short HpiDspCode_ReadBlock(
    size_t nWordsRequested,
    DSP_CODE * psDspCode,
    HW32 * * ppdwBlock)
{
    size_t count;

    if ( nWordsRequested > BLOCK_LIMIT_DWORD)
        return(HPI_ERROR_DSP_FILE_FORMAT);

    count=HpiOs_fread(aCodeBuffer,sizeof(HW32),nWordsRequested,psDspCode->pDspCodeFile);
    if (count!=nWordsRequested)
        return(HPI_ERROR_DSP_FILE_FORMAT);

    psDspCode->dwWordCount += nWordsRequested;
    if (psDspCode->dwWordCount > psDspCode->dwBlockLength)
        return(HPI_ERROR_DSP_FILE_FORMAT);

    *ppdwBlock = aCodeBuffer;
    return (0);
}
#endif //defined DSPCODE_FILE


/***********************************************************************/
#ifdef DSPCODE_ARRAY

#ifdef USE_ZLIB
#include "hpizlib.h"
#endif

/* USE LINKED-IN ARRAYS OF CODE */
#ifdef	HPI_INCLUDE_4100
extern HW32 HFAR * adwDspCode_4100Arrays[];
extern int nDspCode_4100ArrayCount;
#endif
#ifdef	HPI_INCLUDE_4300
extern HW32 HFAR * adwDspCode_4300Arrays[];
extern int nDspCode_4300ArrayCount;
#endif
#ifdef	HPI_INCLUDE_5000
extern HW32 HFAR * adwDspCode_hex5000Arrays[];
extern int nDspCode_hex5000ArrayCount;
#endif
#ifdef HPI_INCLUDE_8800
extern HW32 *adwDspCode_boot8800Arrays[];
extern int nDspCode_boot8800ArrayCount;
#endif
#ifdef HPI_INCLUDE_6400
extern HW32 HFAR * adwDspCode_hex6205Arrays[];	//c6205 bus-master pci i/f
extern int nDspCode_hex6205ArrayCount;
extern HW32 HFAR *adwDspCode_hex6413Arrays[];
extern int nDspCode_hex6413ArrayCount;
#endif
#ifdef HPI_INCLUDE_6600		/* also does ASI6500 */
extern HW32 HFAR * adwDspCode_hex6205Arrays[];	//c6205 bus-master pci i/f
extern int nDspCode_hex6205ArrayCount;
extern HW32 HFAR *adwDspCode_hex6600Arrays[];
extern int nDspCode_hex6600ArrayCount;
#endif
#ifdef HPI_INCLUDE_8700
extern HW32 HFAR * adwDspCode_hex6205Arrays[];	//c6205 bus-master pci i/f
extern int nDspCode_hex6205ArrayCount;
extern HW32 HFAR * adwDspCode_hex8713Arrays[];
extern int nDspCode_hex8713ArrayCount;
#endif
#ifdef HPI_INCLUDE_6000
extern HW32 *adwDspCode_boot6200Arrays[];
extern int nDspCode_boot6200ArrayCount;
#endif

/*-------------------------------------------------------------------*/
short HpiDspCode_Open(
    HW32 nAdapter,
    DSP_CODE * psDspCode,
	HW32 * pdwOsErrorCode )
{
    psDspCode->dwOffset = 0;
    psDspCode->nArrayNum=0;
	switch((HW16)nAdapter)
    {

    case Load4100:		// ASI4100
#if defined(HPI_INCLUDE_4100)
        psDspCode->apaCodeArrays = adwDspCode_4100Arrays;
        psDspCode->nDspCode_ArrayCount=nDspCode_4100ArrayCount;
        break;
#else
        return( HPI_ERROR_DSP_SECTION_NOT_FOUND );
#endif

    case Load4300:		// ASI4300
#if	defined(HPI_INCLUDE_4300)
        psDspCode->apaCodeArrays = adwDspCode_4300Arrays;
        psDspCode->nDspCode_ArrayCount=nDspCode_4300ArrayCount;
        break;
#else
        return( HPI_ERROR_DSP_SECTION_NOT_FOUND );
#endif

    case Load6200:
#if	defined(HPI_INCLUDE_6000)
        psDspCode->apaCodeArrays = adwDspCode_boot6200Arrays;
        psDspCode->nDspCode_ArrayCount=nDspCode_boot6200ArrayCount;
        break;
#else
        return( HPI_ERROR_DSP_SECTION_NOT_FOUND );
#endif
	 case Load8800:
#if	defined(HPI_INCLUDE_8800)
		  psDspCode->apaCodeArrays = adwDspCode_boot8800Arrays;
		  psDspCode->nDspCode_ArrayCount=nDspCode_boot8800ArrayCount;
		  break;
#else
		  return( HPI_ERROR_DSP_SECTION_NOT_FOUND );
#endif
	 case Load5000:
#if defined(HPI_INCLUDE_5000)
		  psDspCode->apaCodeArrays = adwDspCode_hex5000Arrays;
		  psDspCode->nDspCode_ArrayCount=nDspCode_hex5000ArrayCount;
		  break;
#else
		  return( HPI_ERROR_DSP_SECTION_NOT_FOUND );
#endif
	 case Load6205:		// TMS320C6205 PCI interface code.
#if (defined(HPI_INCLUDE_6400) + defined(HPI_INCLUDE_6600) + defined(HPI_INCLUDE_8700)) /* also does ASI6500 */
		  psDspCode->apaCodeArrays = adwDspCode_hex6205Arrays;
		  psDspCode->nDspCode_ArrayCount=nDspCode_hex6205ArrayCount;
		  break;
#else
		  return( HPI_ERROR_DSP_SECTION_NOT_FOUND );
#endif
	 case Load6413:
#if defined(HPI_INCLUDE_6400)
		  psDspCode->apaCodeArrays = adwDspCode_hex6413Arrays;
		  psDspCode->nDspCode_ArrayCount=nDspCode_hex6413ArrayCount;
		  break;
#else
		  return( HPI_ERROR_DSP_SECTION_NOT_FOUND );
#endif
	 case Load6600:
#if defined(HPI_INCLUDE_6600) /* also does ASI6500 */
		  psDspCode->apaCodeArrays = adwDspCode_hex6600Arrays;
		  psDspCode->nDspCode_ArrayCount=nDspCode_hex6600ArrayCount;
		  break;
#else
		  return( HPI_ERROR_DSP_SECTION_NOT_FOUND );
#endif
	 case Load8713:
#if defined(HPI_INCLUDE_8700)
		  psDspCode->apaCodeArrays = adwDspCode_hex8713Arrays;
		  psDspCode->nDspCode_ArrayCount=nDspCode_hex8713ArrayCount;
		  break;
#else
		  return( HPI_ERROR_DSP_SECTION_NOT_FOUND );
#endif
    }
    return 0;
}

/*-------------------------------------------------------------------*/
void HpiDspCode_Rewind(DSP_CODE * psDspCode )
{
    psDspCode->dwOffset = 0;
    psDspCode->nArrayNum=0;
}

/*-------------------------------------------------------------------*/
void HpiDspCode_Close(DSP_CODE * psDspCode)
{
    /* nothing to do for linked array version */
}

/*-------------------------------------------------------------------*/
short HpiDspCode_ReadWord(
    DSP_CODE * psDspCode,
    HW32 * pdwWord )
{
    //EWB added check for running off the end of arrays
    if (psDspCode->nArrayNum < psDspCode->nDspCode_ArrayCount)
    {
		#ifdef USE_ZLIB
		#define HEX_OPEN_ARRAY psDspCode->adwDspCodeArray=HpiZlib_OpenArray(psDspCode->apaCodeArrays[psDspCode->nArrayNum])
		#else
		#define HEX_OPEN_ARRAY psDspCode->adwDspCodeArray=psDspCode->apaCodeArrays[psDspCode->nArrayNum]
		#endif
        if(psDspCode->dwOffset==0)
            HEX_OPEN_ARRAY;
        if (psDspCode->adwDspCodeArray != NULL)
        {
            *pdwWord=(psDspCode->adwDspCodeArray[ (HW16)psDspCode->dwOffset ] );
            psDspCode->dwOffset++;
            return (0);
        }
    }
    else
        psDspCode->adwDspCodeArray=NULL;

    return(HPI_ERROR_DSP_FILE_FORMAT);
}

/*-------------------------------------------------------------------*/
short HpiDspCode_ReadBlock(
    size_t nWordsRequested,
    DSP_CODE * psDspCode,
    HW32 * * ppdwBlock)
{
    *ppdwBlock = psDspCode->adwDspCodeArray+psDspCode->dwOffset;
    psDspCode->dwOffset += nWordsRequested;
    return (0);
}

#endif

/////////////////////////////////////////////////////////////////
