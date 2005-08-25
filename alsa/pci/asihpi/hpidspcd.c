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
If DSPCODE_FILE is defined, code is read from a file

(Linux only:) If DSPCODE_FIRMWARE_LOADER is defined, code is read using
hotplug firmware loader from individual dsp code files

If neither of the above is defined, code is read from linked arrays.
DSPCODE_ARRAY is defined.

HPI_INCLUDE_**** must be defined
and the appropriate hzz?????.c or hex?????.c linked in

If USE_ZLIB is defined, hpizlib.c must also be linked

 */
/***********************************************************************/
#include <hpidspcd.h>
#include <hpidebug.h>

/**
 Header structure for binary dsp code file (see asidsp.doc)
 This structure must match that used in s2bin.c for generation of asidsp.bin
 */
typedef struct {
    HW32 size;
    HW8 type[4];
    HW32 adapter;
    HW32 version;
    HW32 crc;
} header_t;

/***********************************************************************/
#ifdef DSPCODE_FIRMWARE
#include "linux/pci.h"
/*-------------------------------------------------------------------*/
short HpiDspCode_Open(
    HW32 nAdapter,
    DSP_CODE * psDspCode )
{
    const struct firmware *psFirmware = psDspCode->psFirmware;
    header_t header;
    char fw_name[13];
	int err;

    sprintf(fw_name,"dsp%04lx.bin",nAdapter);
	HPI_PRINT_INFO("Requesting firmware for %s\n",fw_name);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
    if (0 != (err=request_firmware(&psFirmware, fw_name, psDspCode->psDev->slot_name )))
#else
		if (0 != (err=request_firmware(&psFirmware, fw_name, &psDspCode->psDev->dev)) )
#endif
    {
		HPI_PRINT_ERROR("%d, request_firmware failed for  %s\n",err,fw_name);
	goto error1;
    }
    if  (psFirmware->size < sizeof(header)) {
	HPI_PRINT_ERROR("Header size too small %s\n",fw_name);
	goto error2;
    }
    memcpy(&header,psFirmware->data,sizeof(header));
    if (header.adapter != nAdapter) {
	HPI_PRINT_ERROR("Adapter type incorrect %4lx != %4lx\n",header.adapter,nAdapter);
	goto error2;
    }
    if (header.size != psFirmware->size) {
	HPI_PRINT_ERROR("Code size wrong  %ld != %d\n",header.size,psFirmware->size);
	goto error2;
    }

    HPI_PRINT_DEBUG("Dsp code %s opened\n",fw_name);
    psDspCode->psFirmware = psFirmware;
    psDspCode->dwBlockLength  = header.size/sizeof(HW32);
    psDspCode->dwWordCount=sizeof(header)/sizeof(HW32); // start pointing to data
	psDspCode->dwVersion = header.version;
    return 0;

 error2:
    release_firmware(psFirmware);
 error1:
    // HPI_PRINT_ERROR("Firmware %s not available\n",fw_name);
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
       HPI_PRINT_DEBUG("Dsp code closed\n");
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
#endif // defined DSPCODE_FIRMWARE_LOADER

/********************************************************************/
/* Load dspcode from file asidsp.bin */
#ifdef DSPCODE_FILE

/* must match or be greater than the same constant in S2BIN.C */
#define BLOCK_LIMIT_DWORD 2048L
static HW32 HFAR aCodeBuffer[BLOCK_LIMIT_DWORD];


/*-------------------------------------------------------------------*/
#ifdef HPI_OS_LINUX
#ifndef DSPBINPATH
#define DSPBINPATH "/usr/lib/hotplug/firmware/"
#endif
#endif

#define XSTR(S) STR(S)
#define STR(S) #S

short HpiDspCode_Open(
    HW32 nAdapter,
    DSP_CODE * psDspCode )
{
    int i,nRead=(int)NO_FILE;
    header_t header;
    char	*pszFilepath;


#ifdef HPI_OS_LINUX
    char szFilepath[128];

    pszFilepath = &szFilepath[0];
    sprintf(szFilepath, XSTR(DSPBINPATH) "dsp%04lx.bin",nAdapter);
#else
    pszFilepath = HpiOs_GetDspCodePath();
#endif

    psDspCode->pDspCodeFile = HpiOs_fopen_rb(pszFilepath);
    if  (psDspCode->pDspCodeFile == NO_FILE)
        return ( HPI_ERROR_DSP_FILE_NOT_FOUND );

    // seek for first CODE header
    for (i=0; i<1000; i++)
    {
        HpiOs_fseek(psDspCode->pDspCodeFile,i,SEEK_SET);
        nRead=HpiOs_fread(&header, 1, sizeof(header), psDspCode->pDspCodeFile);
        if (nRead < sizeof(header))
        {
            HPI_PRINT_DEBUG("Ran out of data\n");
            break;
        }
        if (strncmp(header.type,"CODE",4)==0)
            break;
    }
    if ((nRead < sizeof(header)) || (i==1000))
    {
        HpiOs_fclose ( psDspCode->pDspCodeFile );
        HPI_PRINT_DEBUG("CODE header not found\n");
        return ( HPI_ERROR_DSP_FILE_NOT_FOUND );
    }

#ifndef HPI_OS_LINUX
    // walk CODE headers searching for correct one
    while ( (nRead == sizeof(header)) && (header.adapter != nAdapter) )
    {
//		printk("Skipping %04lx ",header.adapter);
        HpiOs_fseek(psDspCode->pDspCodeFile, (header.size-sizeof(header)), SEEK_CUR);
        nRead=HpiOs_fread(&header, 1, sizeof(header), psDspCode->pDspCodeFile);
    }
#endif

    if  ((nRead < sizeof(header)) || (header.adapter != nAdapter))
    {
        HpiOs_fclose ( psDspCode->pDspCodeFile );
        HPI_PRINT_DEBUG("Adapter type %04lx not found\n",nAdapter);
        return ( HPI_ERROR_DSP_FILE_NOT_FOUND );
    }

	psDspCode->nAdapter = nAdapter;
    psDspCode->dwBlockLength  = (header.size-sizeof(header))/4;
    psDspCode->dwWordCount=0;
	psDspCode->dwVersion = header.version;
    return 0;
}

/*-------------------------------------------------------------------*/
void HpiDspCode_Rewind(
    DSP_CODE * psDspCode )
{
	HpiDspCode_Close(psDspCode);
	HpiDspCode_Open(psDspCode->nAdapter,psDspCode);
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
#include <hpizlib.h>
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
#ifdef	HPI_INCLUDE_4400
extern HW32 HFAR * adwDspCode_4400Arrays[];
extern int nDspCode_4400ArrayCount;
#endif
#ifdef	HPI_INCLUDE_4500
extern HW32 HFAR * adwDspCode_4500Arrays[];
extern int nDspCode_4500ArrayCount;
#endif
#ifdef	HPI_INCLUDE_4600
extern HW32 HFAR * adwDspCode_4600Arrays[];
extern int nDspCode_4600ArrayCount;
#endif
#ifdef	HPI_INCLUDE_5000
extern HW32 HFAR * adwDspCode_hex5000Arrays[];
extern int nDspCode_hex5000ArrayCount;
#endif
#ifdef HPI_INCLUDE_8800
extern HW32 *adwDspCode_boot8800Arrays[];
extern int nDspCode_boot8800ArrayCount;
#endif
#ifdef HPI_INCLUDE_8600
extern HW32 *adwDspCode_boot8600Arrays[];
extern int nDspCode_boot8600ArrayCount;
#endif
// same code used by both adapter families
#ifdef HPI_INCLUDE_6400
extern HW32 HFAR * adwDspCode_hex6205Arrays[];	//c6205 bus-master pci i/f
extern int nDspCode_hex6205ArrayCount;
extern HW32 HFAR *adwDspCode_hex6413Arrays[];
extern int nDspCode_hex6413ArrayCount;
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
    DSP_CODE * psDspCode )
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
        return( HPI_ERROR_DSP_FILE_NOT_FOUND );
#endif

    case Load4300:		// ASI4300
#if	defined(HPI_INCLUDE_4300)
        psDspCode->apaCodeArrays = adwDspCode_4300Arrays;
        psDspCode->nDspCode_ArrayCount=nDspCode_4300ArrayCount;
        break;
#else
        return( HPI_ERROR_DSP_FILE_NOT_FOUND );
#endif

    case Load4400:		// ASI4400
#if	defined(HPI_INCLUDE_4400)
        psDspCode->apaCodeArrays = adwDspCode_4400Arrays;
        psDspCode->nDspCode_ArrayCount=nDspCode_4400ArrayCount;
        break;
#else
        return( HPI_ERROR_DSP_FILE_NOT_FOUND );
#endif

    case Load4600:		// ASI4600
#if	defined(HPI_INCLUDE_4600)
        psDspCode->apaCodeArrays = adwDspCode_4600Arrays;
        psDspCode->nDspCode_ArrayCount=nDspCode_4600ArrayCount;
        break;
#else
        return( HPI_ERROR_DSP_FILE_NOT_FOUND );
#endif
    case Load6200:
#if	defined(HPI_INCLUDE_6000)
        psDspCode->apaCodeArrays = adwDspCode_boot6200Arrays;
        psDspCode->nDspCode_ArrayCount=nDspCode_boot6200ArrayCount;
        break;
#else
        return( HPI_ERROR_DSP_FILE_NOT_FOUND );
#endif
	 case Load8800:
#if	defined(HPI_INCLUDE_8800)
		  psDspCode->apaCodeArrays = adwDspCode_boot8800Arrays;
		  psDspCode->nDspCode_ArrayCount=nDspCode_boot8800ArrayCount;
		  break;
#else
		  return( HPI_ERROR_DSP_FILE_NOT_FOUND );
#endif
	 case Load8600:
#if defined(HPI_INCLUDE_8600)
		  psDspCode->apaCodeArrays = adwDspCode_boot8600Arrays;
		  psDspCode->nDspCode_ArrayCount=nDspCode_boot8600ArrayCount;
		  break;
#else
		  return( HPI_ERROR_DSP_FILE_NOT_FOUND );
#endif
	 case Load5000:
#if defined(HPI_INCLUDE_5000)
		  psDspCode->apaCodeArrays = adwDspCode_hex5000Arrays;
		  psDspCode->nDspCode_ArrayCount=nDspCode_hex5000ArrayCount;
		  break;
#else
		  return( HPI_ERROR_DSP_FILE_NOT_FOUND );
#endif
	 case Load6205:		// TMS320C6205 PCI interface code.
#if (defined(HPI_INCLUDE_6400) + defined(HPI_INCLUDE_8700))
		  psDspCode->apaCodeArrays = adwDspCode_hex6205Arrays;
		  psDspCode->nDspCode_ArrayCount=nDspCode_hex6205ArrayCount;
		  break;
#else
		  return( HPI_ERROR_DSP_FILE_NOT_FOUND );
#endif
	 case Load6413:
#if defined(HPI_INCLUDE_6400)
		  psDspCode->apaCodeArrays = adwDspCode_hex6413Arrays;
		  psDspCode->nDspCode_ArrayCount=nDspCode_hex6413ArrayCount;
		  break;
#else
		  return( HPI_ERROR_DSP_FILE_NOT_FOUND );
#endif
	 case Load8713:
#if defined(HPI_INCLUDE_8700)
		  psDspCode->apaCodeArrays = adwDspCode_hex8713Arrays;
		  psDspCode->nDspCode_ArrayCount=nDspCode_hex8713ArrayCount;
		  break;
#else
		  return( HPI_ERROR_DSP_FILE_NOT_FOUND );
#endif
	 case Load4500:
	default:	// ASI4500
#ifdef	HPI_INCLUDE_4500
		psDspCode->apaCodeArrays = adwDspCode_4500Arrays;
		psDspCode->nDspCode_ArrayCount=nDspCode_4500ArrayCount;
		break;
#else
		return( HPI_ERROR_DSP_FILE_NOT_FOUND );
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
