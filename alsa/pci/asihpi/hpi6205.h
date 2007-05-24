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

Host Interface module for an ASI6205 based
bus mastering PCI adapter.

Copyright AudioScience, Inc., 2003
******************************************************************************/

#ifndef _HPI6205_H_
#define _HPI6205_H_

#include "hpi.h"

/***********************************************************
Defines used for basic messaging
************************************************************/
#define H620_HIF_RESET          0
#define H620_HIF_IDLE           1
#define H620_HIF_GET_RESP       2
#define H620_HIF_DATA_DONE      3
#define H620_HIF_DATA_MASK      0x10
#define H620_HIF_SEND_DATA      0x14
#define H620_HIF_GET_DATA       0x15
#define H620_HIF_UNKNOWN                0xffff

///////////////////////////////////////////////////////////////////////////////
// HIF errors get listed here

#define HPI6205_ERROR_BASE                                      1000

#define HPI6205_ERROR_MEM_ALLOC                         1
#define HPI6205_ERROR_6205_NO_IRQ                       2
#define HPI6205_ERROR_6205_INIT_FAILED          3
//#define HPI6205_ERROR_MISSING_DSPCODE         4
#define HPI6205_ERROR_UNKNOWN_PCI_DEVICE        5
#define HPI6205_ERROR_6205_REG                          6
#define HPI6205_ERROR_6205_DSPPAGE                      7
#define HPI6205_ERROR_BAD_DSPINDEX                      8
#define HPI6205_ERROR_C6713_HPIC                        9
#define HPI6205_ERROR_C6713_HPIA                        10
#define HPI6205_ERROR_C6713_PLL                         11
#define HPI6205_ERROR_DSP_INTMEM            12
#define HPI6205_ERROR_DSP_EXTMEM                        13
#define HPI6205_ERROR_DSP_PLD               14
#define HPI6205_ERROR_MSG_RESP_IDLE_TIMEOUT 15
#define HPI6205_ERROR_MSG_RESP_TIMEOUT          16
#define HPI6205_ERROR_6205_EEPROM                       17
#define HPI6205_ERROR_DSP_EMIF                          18

/***********************************************************
Types used for mixer control caching
************************************************************/

#define H620_MAX_ISTREAMS 32
#define H620_MAX_OSTREAMS 32
#define HPI_NMIXER_CONTROLS 2048

/*********************************************************************
This is used for background buffer bus mastering stream buffers.
**********************************************************************/
typedef volatile struct {
	u32 dwSamplesProcessed;
	u32 dwAuxiliaryDataAvailable;
	u32 dwStreamState;
	u32 dwDSPIndex;		// DSP index in to the host bus master buffer.
	u32 dwHostIndex;	// Host index in to the host bus master buffer.
	u32 dwSizeInBytes;
} H620_HOSTBUFFER_STATUS;

/*********************************************************************
This is used for dynamic control cache allocation
**********************************************************************/
typedef volatile struct {
	u32 dwNumberOfControls;
	u32 dwPhysicalPCI32address;
	u32 dwSpare;
} H620_CONTROLCACHE;

/*********************************************************************
This is used for dynamic allocation of async event array
**********************************************************************/
typedef volatile struct {
	u32 dwPhysicalPCI32address;
	u32 dwSpare;
	tHPIFIFOBuffer b;
} H620_ASYNC_EVENT_BUFFER;

/***********************************************************
The Host located memory buffer that the 6205 will bus master
in and out of.
************************************************************/
#define HPI6205_SIZEOF_DATA (16*1024)
typedef volatile struct {
	u32 dwHostCmd;
	u32 dwDspAck;
	u32 dwTransferSizeInBytes;
	union {
		HPI_MESSAGE MessageBuffer;
		HPI_RESPONSE ResponseBuffer;
		u8 bData[HPI6205_SIZEOF_DATA];
	} u;
	H620_CONTROLCACHE aControlCache;
	H620_ASYNC_EVENT_BUFFER aAsyncBuffer;
	H620_HOSTBUFFER_STATUS aInStreamHostBufferStatus[H620_MAX_ISTREAMS];
	H620_HOSTBUFFER_STATUS aOutStreamHostBufferStatus[H620_MAX_OSTREAMS];
} tBusMasteringInterfaceBuffer;

#endif

///////////////////////////////////////////////////////
