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
#include "hpidspcd.h"
#include "hpidebug.h"

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
#include <linux/pci.h>
/*-------------------------------------------------------------------*/
short HpiDspCode_Open(
    HW32 nAdapter,
    DSP_CODE * psDspCode )
{
    const struct firmware *psFirmware = psDspCode->psFirmware;
    header_t header;
    char fw_name[13];
	int err;

    sprintf(fw_name,"dsp%04x.bin",nAdapter);
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

