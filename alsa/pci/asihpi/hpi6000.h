/*****************************************************************************

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


Public declarations for DSP Proramming Interface to TI C6701

(C) Copyright AudioScience Inc. 1998-2003
******************************************************************************/

#ifndef _HPI6000_H_
#define _HPI6000_H_

#define HPI_NMIXER_CONTROLS 200 //(NMIXER_VOLUME_CONTROLS + NMIXER_LEVEL_CONTROLS + NMIXER_PEAK_METERS + NMIXER_AES3 + 4)

/*
 * Control caching is always supported in the HPI code.
 * The DSP should make sure that dwControlCacheSizeInBytes is initialized to 0
 * during boot to make it in-active.
 */
typedef struct {
	HW32 dwHostCmd;
	HW32 dwDspAck;
	HW32 dwAddress;
	HW32 dwLength;
	HW32 dwMessageBufferAddress;
	HW32 dwResponseBufferAddress;
	HW32 dwDspNumber;
	HW32 dwAdapterInfo;
	HW32 dwControlCacheIsDirty;
	HW32 dwControlCacheAddress;
	HW32 dwControlCacheSizeInBytes;
} HPI_HIF_6000;

#define HPI_HIF_BASE (0x00000200)	/* start of C67xx internal RAM */
#define HPI_HIF_OFS_HOSTCMD (0)		/* byte offsets */
#define HPI_HIF_OFS_DSPACK (4)
#define HPI_HIF_OFS_ADDRESS (8)
#define HPI_HIF_OFS_LENGTH (12)
#define HPI_HIF_OFS_MSG_BUFFER_ADR (16)
#define HPI_HIF_OFS_RESP_BUFFER_ADR (20)
#define HPI_HIF_OFS_DSP_NUMBER (24)
#define HPI_HIF_OFS_ADAPTER_INFO (28)
#define HPI_HIF_OFS_CONTROL_CACHE_IS_DIRTY (32)
#define HPI_HIF_OFS_CONTROL_CACHE_ADDRESS (36)
#define HPI_HIF_OFS_CONTROL_CACHE_SIZE_IN_BYTES (40)

#define HPI_HIF_PACK_ADAPTER_INFO(adapter,versionMajor,versionMinor) ((adapter<<16)|(versionMajor<<8) | versionMinor)
#define HPI_HIF_ADAPTER_INFO_EXTRACT_ADAPTER(adapterinfo) ((adapterinfo>>16)&0xffff)
#define HPI_HIF_ADAPTER_INFO_EXTRACT_HWVERSION_MAJOR(adapterinfo) ((adapterinfo>> 8)&0xff)
#define HPI_HIF_ADAPTER_INFO_EXTRACT_HWVERSION_MINOR(adapterinfo) (adapterinfo&0xff)

#define HPI_HIF_IDLE		0
#define HPI_HIF_SEND_MSG	1
#define HPI_HIF_GET_RESP	2
#define HPI_HIF_DATA_MASK	0x10
#define HPI_HIF_SEND_DATA	0x13
#define HPI_HIF_GET_DATA	0x14
#define HPI_HIF_SEND_DONE	5
#define HPI_HIF_RESET		9

#define HPI_HIF_ERROR_MASK	0x4000

///////////////////////////////////////////////////////////////////////////////
// HIF errors get listed here

#define HPI6000_ERROR_MSG_RESP_IDLE_TIMEOUT		901
#define HPI6000_ERROR_MSG_RESP_SEND_MSG_ACK 	902
#define HPI6000_ERROR_MSG_RESP_GET_RESP_ACK		903
#define HPI6000_ERROR_MSG_GET_ADR				904
#define HPI6000_ERROR_RESP_GET_ADR 				905
#define HPI6000_ERROR_MSG_RESP_BLOCKWRITE32  	906
#define HPI6000_ERROR_MSG_RESP_BLOCKREAD32   	907
#define HPI6000_ERROR_MSG_INVALID_DSP_INDEX   	908
#define HPI6000_ERROR_CONTROL_CACHE_PARAMS		909

#define HPI6000_ERROR_SEND_DATA_IDLE_TIMEOUT 	911
#define HPI6000_ERROR_SEND_DATA_ACK 			912
#define HPI6000_ERROR_SEND_DATA_ADR 			913
#define HPI6000_ERROR_SEND_DATA_TIMEOUT			914
#define HPI6000_ERROR_SEND_DATA_CMD				915
#define HPI6000_ERROR_SEND_DATA_WRITE			916
#define HPI6000_ERROR_SEND_DATA_IDLECMD			917
#define HPI6000_ERROR_SEND_DATA_VERIFY			918

#define HPI6000_ERROR_GET_DATA_IDLE_TIMEOUT 	921
#define HPI6000_ERROR_GET_DATA_ACK 				922
#define HPI6000_ERROR_GET_DATA_CMD				923
#define HPI6000_ERROR_GET_DATA_READ				924
#define HPI6000_ERROR_GET_DATA_IDLECMD			925

#define HPI6000_ERROR_CONTROL_CACHE_ADDRLEN		951
#define HPI6000_ERROR_CONTROL_CACHE_READ		952
#define HPI6000_ERROR_CONTROL_CACHE_FLUSH		953

#define HPI6000_ERROR_MSG_RESP_GETRESPCMD		961
#define HPI6000_ERROR_MSG_RESP_IDLECMD			962
#define HPI6000_ERROR_MSG_RESP_BLOCKVERIFY32   	963


// adapter init errors
#define HPI6000_ERROR_UNHANDLED_SUBSYS_ID       930

// 931 - can't access PCI2040
#define HPI6000_ERROR_INIT_PCI2040              931
// 932 - can't access DSP HPI i/f
#define HPI6000_ERROR_INIT_DSPHPI               932
// 933 - can't access internal DSP memory
#define HPI6000_ERROR_INIT_DSPINTMEM            933
// 934 - can't access SDRAM - test#1
#define HPI6000_ERROR_INIT_SDRAM1               934
// 935 - can't access SDRAM - test#2
#define HPI6000_ERROR_INIT_SDRAM2               935

#define HPI6000_ERROR_INIT_VERIFY               938

#define HPI6000_ERROR_INIT_NOACK                939

#define HPI6000_ERROR_INIT_PLDTEST1             941
#define HPI6000_ERROR_INIT_PLDTEST2             942

#endif // _HPI6000_H_

/////////////////////////////////////////////////////////////////
