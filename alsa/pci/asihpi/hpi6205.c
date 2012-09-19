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


 Hardware Programming Interface (HPI) for AudioScience ASI5000
 series adapters.
 These PCI bus adapters are based on a TI C6205 PCI bus mastering DSP

 Exported functions:
 void HPI_6205( HPI_MESSAGE *phm, HPI_RESPONSE *phr )

 #defines
 HPI6205_EE_PCI_CONFIG_FAILED
	 NO EEPROM BOOT MODE - development only
	 SUBSYS ID = 0x00000000
	 HSR bit C6205_HSR_EEREAD ste to 0
 TEST_8700_ON_5000
	 Load 87xx code instead of 5000 code onto ASI5000 card.

(C) Copyright AudioScience Inc. 1998-2003
*******************************************************************************/

#include "hpi.h"
#include "hpios.h"  // for debug
#include "hpipci.h"
#include "hpi6205.h"

#include "hpidspcd.h"


////////////////////////////////////////////////////////////////////////////
// local defines

#define H620_MAX_ISTREAMS 32
#define H620_MAX_OSTREAMS 32

// for C6205 PCI i/f
// Host Status Register (HSR) bitfields
#define C6205_HSR_INTSRC 	0x01
#define C6205_HSR_INTAVAL 	0x02
#define C6205_HSR_INTAM 	0x04
#define C6205_HSR_CFGERR 	0x08
#define C6205_HSR_EEREAD 	0x10
// Host-to-DSP Control Register (HDCR) bitfields
#define C6205_HDCR_WARMRESET 	0x01
#define C6205_HDCR_DSPINT	 	0x02
#define C6205_HDCR_PCIBOOT	 	0x04
// DSP Page Register (DSPP) bitfields (defines 4 Mbyte page that BAR0 points to).
#define C6205_DSPP_MAP1 	0x400

// BAR0 maps to prefetchable 4 Mbyte memory block set by DSPP.
// BAR1 maps to non-prefetchable 8 Mbyte memory block of DSP memory mapped registers (starting at 0x01800000).
// 0x01800000 is hardcoded in the PCI i/f, so that only the offset from this needs to be added to the BAR1
// base address set in the PCI config reg
#define C6205_BAR1_PCI_IO_OFFSET (0x027FFF0L)
#define C6205_BAR1_HSR (C6205_BAR1_PCI_IO_OFFSET)
#define C6205_BAR1_HDCR (C6205_BAR1_PCI_IO_OFFSET+4)
#define C6205_BAR1_DSPP (C6205_BAR1_PCI_IO_OFFSET+8)

#define C6205_BAR0_TIMER1_CTL (0x01980000L)	// used to control LED (revA) and reset C6713 (revB)

// For first 6713 in CE1 space, using DA17,16,2
#define HPICL_ADDR		0x01400000L
#define HPICH_ADDR     	0x01400004L
#define HPIAL_ADDR     	0x01410000L
#define HPIAH_ADDR     	0x01410004L
#define HPIDIL_ADDR    	0x01420000L
#define HPIDIH_ADDR    	0x01420004L
#define HPIDL_ADDR     	0x01430000L
#define HPIDH_ADDR     	0x01430004L

#define C6713_EMIF_GCTL   		0x01800000
#define C6713_EMIF_CE1			0x01800004
#define C6713_EMIF_CE0        	0x01800008
#define C6713_EMIF_CE2        	0x01800010
#define C6713_EMIF_CE3        	0x01800014
#define C6713_EMIF_SDRAMCTL   	0x01800018
#define C6713_EMIF_SDRAMTIMING  0x0180001C
#define C6713_EMIF_SDRAMEXT   	0x01800020



typedef struct
{
	 HPI_PCI Pci;        // PCI info - bus#,dev#,address etc
	 HW16    wAdapterType;   // ASI6701 etc
	 HW16    wIndex;     //
	 HW16    wOpen;      // =1 when adapter open
	 HW16    wMixerOpen;

	 HW16    wDspCrashed;
	 HW16    wHasControlCache;

	 // PCI registers
	 HW32	dwHSR;
	 HW32	dwHDCR;
	 HW32	dwDSPP;


	 //	 tHPI6000ControlCacheSingle aControlCache[HPI_NMIXER_CONTROLS];
	HW32 dwDspPage;

	HpiOs_LockedMem_Handle hLockedMem;
	tBusMasteringInterfaceBuffer *pInterfaceBuffer;

	HW16 flagOStreamJustReset[H620_MAX_OSTREAMS];
	HpiOs_LockedMem_Handle InStreamHostBuffers[H620_MAX_ISTREAMS];
	HpiOs_LockedMem_Handle OutStreamHostBuffers[H620_MAX_OSTREAMS];
	HW32 InStreamHostBufferSize[H620_MAX_ISTREAMS];
	HW32 OutStreamHostBufferSize[H620_MAX_OSTREAMS];
}
H620_ADAPTER_OBJ;


////////////////////////////////////////////////////////////////////////////
// local prototypes

#define Hpi6205_Abstract_MEMWRITE32(a,b,c)	HPIOS_MEMWRITE32((b),(c))
#define Hpi6205_Abstract_MEMREAD32(a,b)		HPIOS_MEMREAD32((b))


//HW16 Hpi6205_DspBlockWrite32( H620_ADAPTER_OBJ *pao, HW16 wDspIndex, HW32 dwHpiAddress, HW32 dwSource, HW32 dwCount);
//HW16 Hpi6205_DspBlockRead32( H620_ADAPTER_OBJ *pao, HW16 wDspIndex, HW32 dwHpiAddress, HW32 dwDest, HW32 dwCount);

static short Hpi6205_AdapterBootLoadDsp( H620_ADAPTER_OBJ *pao );
static short Hpi6205_MessageResponseSequence(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm,HPI_RESPONSE *phr);
static void  Hpi6205_Message( H620_ADAPTER_OBJ *pao, HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static HW16  Hpi6205_Error( int nDspIndex, int nError );

#define DPI_ERROR           900 /* non-specific error */
#define DPI_ERROR_SEND      910
#define DPI_ERROR_GET       950  //EWB more space for subcodes
#define DPI_ERROR_DOWNLOAD  930

#define TIMEOUT 1000000L

static void H620_SubSysOpen(void);
static void H620_SubSysClose(void);
static void H620_SubSysGetAdapters(HPI_RESPONSE *phr);
static void H620_SubSysCreateAdapter(HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H620_SubSysDeleteAdapter(HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H620_SubSysFindAdapters(HPI_RESPONSE *phr);

static void H620_AdapterOpen(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H620_AdapterClose(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H620_AdapterGetInfo(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H620_AdapterGetAsserts(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);

static H620_ADAPTER_OBJ* H620_FindAdapter( HW16 wAdapterIndex );
static short H620_CreateAdapterObj( H620_ADAPTER_OBJ *pao);

static void H620_OutStreamHostBufferAllocate(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H620_OutStreamHostBufferFree(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H620_OutStreamWrite(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H620_OutStreamGetInfo(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H620_OutStreamStart(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);	// for debug
static void H620_OutStreamOpen(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H620_OutStreamReset(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);

static void H620_InStreamHostBufferAllocate(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H620_InStreamHostBufferFree(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H620_InStreamRead(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H620_InStreamGetInfo(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H620_InStreamStart(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);	// for debug


static HW32 BootLoader_ReadMem32(H620_ADAPTER_OBJ *pao, int nDSPIndex, HW32 dwAddress);
static HW16 BootLoader_WriteMem32(H620_ADAPTER_OBJ *pao, int nDSPIndex, HW32 dwAddress, HW32 dwData);
static HW16 BootLoader_ConfigEMIF(H620_ADAPTER_OBJ *pao, int nDSPIndex);
static HW16 BootLoader_TestMemory(H620_ADAPTER_OBJ *pao, int nDSPIndex, HW32 dwAddress, HW32 dwLength);
static HW16 BootLoader_TestInternalMemory(H620_ADAPTER_OBJ *pao, int nDSPIndex);
static HW16 BootLoader_TestExternalMemory(H620_ADAPTER_OBJ *pao, int nDSPIndex);
//static HW16 BootLoader_BlockWrite32( H620_ADAPTER_OBJ *pao, HW16 wDspIndex, HW32 dwDspDestinationAddress, HW32 dwSourceAddress, HW32 dwCount);
static HW16 BootLoader_TestPld(H620_ADAPTER_OBJ *pao, int nDSPIndex);

////////////////////////////////////////////////////////////////////////////
// external globals
extern HW16 gwHpiLastError;
extern HW32 gadwHpiSpecificError[8];

////////////////////////////////////////////////////////////////////////////
// local globals
#define H620_MAX_ADAPTERS HPI_MAX_ADAPTERS
static H620_ADAPTER_OBJ gao60[H620_MAX_ADAPTERS];

static HW16 gwNum60Adapters;       // total number of adapters created in this HPI
static HW16 gwTotalOStreams;      // total number of devices created in this HPI

////////////////////////////////////////////////////////////////////////////
// HPI_6205()
// Entry point from HPIMAN
// All calls to the HPI start here

void HPI_6205( HPI_MESSAGE *phm, HPI_RESPONSE *phr )
{
	 H620_ADAPTER_OBJ *pao;

    // subsytem messages get executed by every HPI.
	 // All other messages are ignored unless the adapter index matches
	 // an adapter in the HPI
	HPI_PRINT_VERBOSE("start\n ");
	if (phm->wObject==HPI_OBJ_SUBSYSTEM)
	{
		HPI_PRINT_VERBOSE("subsys message\n");
	    pao=NULL;
	}
	 else
	{
		pao = H620_FindAdapter( phm->wAdapterIndex );
        if(!pao)
		 {
		return; // message probably meant for another HPI module
		}
	 }

    //phr->wError=0;      // SGT JUN-15-01 - so that modules can't overwrite errors from FindAdapter

	 // if Dsp has crashed then do not try and communicated with it any more
	if(phm->wObject!=HPI_OBJ_SUBSYSTEM)
		if(pao->wDspCrashed)
        {
			HPI_InitResponse(phr, phm->wObject, phm->wFunction, HPI_ERROR_DSP_HARDWARE );
            return;
        }
	HPI_PRINT_VERBOSE("start of switch\n");
    switch(phm->wType)
    {
    case HPI_TYPE_MESSAGE:
        switch(phm->wObject)
		{
		  case HPI_OBJ_SUBSYSTEM:
				switch(phm->wFunction)
			{
			case HPI_SUBSYS_OPEN:
				H620_SubSysOpen();
                phr->wError=0;
					 return;    // note that error is cleared
            case HPI_SUBSYS_CLOSE:
					 H620_SubSysClose();
				phr->wError=0;
				return;    // note that error is cleared
				case HPI_SUBSYS_GET_INFO:
				H620_SubSysGetAdapters(phr);
                return;
            case HPI_SUBSYS_FIND_ADAPTERS:
				H620_SubSysFindAdapters(phr);
                return;
            case HPI_SUBSYS_CREATE_ADAPTER:
                H620_SubSysCreateAdapter(phm, phr);
					 return;
				case HPI_SUBSYS_DELETE_ADAPTER:
                H620_SubSysDeleteAdapter(phm, phr);
                return;
			default:
					 break;
            }
			phr->wError = HPI_ERROR_INVALID_FUNC;
				break;

		  case HPI_OBJ_ADAPTER:
				phr->wSize=HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_ADAPTER_RES);
			switch(phm->wFunction)
			{
			case HPI_ADAPTER_OPEN:
				H620_AdapterOpen(pao,phm,phr);
				return;
			case HPI_ADAPTER_CLOSE:
				H620_AdapterClose(pao,phm,phr);
				return;
			case HPI_ADAPTER_GET_INFO:
				H620_AdapterGetInfo(pao,phm,phr);
				return;
			case HPI_ADAPTER_GET_ASSERT:
				H620_AdapterGetAsserts(pao,phm, phr);
				return;
			case HPI_ADAPTER_TEST_ASSERT:
			case HPI_ADAPTER_SELFTEST:
			case HPI_ADAPTER_GET_MODE:
				case HPI_ADAPTER_SET_MODE:
			case HPI_ADAPTER_FIND_OBJECT:
					 Hpi6205_Message( pao, phm,phr);
				return;
			default:
				break;
			}
			phr->wError = HPI_ERROR_INVALID_FUNC;
			break;

		  case HPI_OBJ_OSTREAM:
			switch(phm->wFunction)
			{
			case HPI_OSTREAM_WRITE:
				H620_OutStreamWrite(pao,phm,phr);
				return;
			case HPI_OSTREAM_GET_INFO:
				H620_OutStreamGetInfo(pao,phm,phr);
				return;
			case HPI_OSTREAM_HOSTBUFFER_ALLOC:
				H620_OutStreamHostBufferAllocate(pao,phm,phr);
				return;
			case HPI_OSTREAM_HOSTBUFFER_FREE:
				H620_OutStreamHostBufferFree(pao,phm,phr);
				return;
			case HPI_OSTREAM_START:
				H620_OutStreamStart(pao,phm,phr);
				return;
			case HPI_OSTREAM_OPEN:
				H620_OutStreamOpen(pao,phm,phr);
				return;
			case HPI_OSTREAM_RESET:
				H620_OutStreamReset(pao,phm,phr);
				return;
			default:
				Hpi6205_Message( pao, phm,phr);
				break;
			}
			break;
		  case HPI_OBJ_ISTREAM:
			switch(phm->wFunction)
			{
			case HPI_ISTREAM_READ:
				H620_InStreamRead(pao,phm,phr);
				return;
			case HPI_ISTREAM_GET_INFO:
				H620_InStreamGetInfo(pao,phm,phr);
				return;
			case HPI_ISTREAM_HOSTBUFFER_ALLOC:
				H620_InStreamHostBufferAllocate(pao,phm,phr);
				return;
			case HPI_ISTREAM_HOSTBUFFER_FREE:
				H620_InStreamHostBufferFree(pao,phm,phr);
				return;
			case HPI_ISTREAM_START:
				H620_InStreamStart(pao,phm,phr);
				return;
			default:
				Hpi6205_Message( pao, phm,phr);
				break;
			}
			break;
		default:
				Hpi6205_Message( pao, phm,phr);
			break;
		}
		break;
	default:
		phr->wError = HPI_ERROR_INVALID_TYPE;
		break;
	}
}

////////////////////////////////////////////////////////////////////////////
// SUBSYSTEM

// This message gets processed by all HPIs
// and is used to initialise the objects within each HPI
void H620_SubSysOpen()
{
	HPI_PRINT_INFO( " HPI6205_SubSysOpen\n" );

	gwNum60Adapters = 0;
	gwTotalOStreams = 0;
	memset( gao60, 0, sizeof(H620_ADAPTER_OBJ)*H620_MAX_ADAPTERS );
}

void H620_SubSysClose()
{
	HPI_PRINT_INFO( " HPI6205_SubSysClose\n" );

    gwNum60Adapters = 0;
    gwTotalOStreams = 0;
    memset( gao60, 0, sizeof(H620_ADAPTER_OBJ)*H620_MAX_ADAPTERS );
}


void H620_SubSysGetAdapters(HPI_RESPONSE *phr)
{
    // fill in the response adapter array with the position
    // identified by the adapter number/index of the adapters in
	 // this HPI
    // i.e. if we have an A120 with it's jumper set to
	 // Adapter Number 2 then put an Adapter type A120 in the
    // array in position 1
    // NOTE: AdapterNumber is 1..N, Index is 0..N-1

	// input:  NONE
    // output: wNumAdapters
    //         awAdapter[]
	 //

	short i;
	 H620_ADAPTER_OBJ *pao=0;

	HPI_PRINT_VERBOSE( " HPI6205_SubSysGetAdapters\n" );

    // for each adapter, place it's type in the position of the array
	// corresponding to it's adapter number
    for(i=0; i<gwNum60Adapters; i++)
    {
        pao = &gao60[i];
        if( phr->u.s.awAdapterList[ pao->wIndex ] != 0)
        {
			phr->wError = HPI_DUPLICATE_ADAPTER_NUMBER;
				return;
		  }
		  phr->u.s.awAdapterList[ pao->wIndex ] = pao->wAdapterType;
    }

	// add the number of adapters recognised by this HPI to the system total
	phr->u.s.wNumAdapters += gwNum60Adapters;
    phr->wError = 0;        // the function completed OK;
}



void H620_SubSysFindAdapters(HPI_RESPONSE *phr)
{
	// Go out and try to find all adapters that belong to this
	 // HPI, and initialise them
	 //
	 // For PCI bus adapters, use the PCI BIOS (or other OS functionality)
	 // to find all the adapters that this HPI knows about.
	 //
	 // When an adapter is found, put it in the response adapter array
	 // with the position identified by the adapter number/index of the
	 // adapters in this HPI
	 // i.e. if we have an ASI50xx with it's jumper set to
	 // Adapter Number 2 then put an Adapter type ASI50xx in the
	 // array in position 1
	 // NOTE: AdapterNumber is 1..N, Index is 0..N-1
	 //
	 //

    short nIndex=0;
    short nError=0;
    //    	HW32 dwSubDev=0;

	 HPI_PRINT_VERBOSE( "HPI6205_SubSysFindAdapters\n" );

	// Cycle through all the PCI bus slots looking for this adapters
	// vendor and device ID
	//
	gwNum60Adapters = 0;
	for(nIndex=0; nIndex<H620_MAX_ADAPTERS; nIndex++)
	{
		H620_ADAPTER_OBJ Adap;
		// Need to use temporary adapter object because we don't
		// know what index to assign it yet. Fixes bug in NT kernel driver.

		memset( &Adap, 0, sizeof(H620_ADAPTER_OBJ) );

		if( HpiPci_FindDeviceEx( &Adap.Pci, nIndex, HPI_PCI_VENDOR_ID_TI, HPI_ADAPTER_DSP6205,HPI_PCI_VENDOR_ID_AUDIOSCIENCE ) )
		{
			if( HpiPci_FindDeviceEx( &Adap.Pci, nIndex, HPI_PCI_VENDOR_ID_TI, HPI_ADAPTER_DSP6205,0 ) )	// no EEPROM = "early pci vga device"
				break; // no adapter found!
			else
			{
				// eeprom was not read so have to fill in fields manually - assume ASI8700
				Adap.Pci.wSubSysVendorId = HPI_PCI_VENDOR_ID_AUDIOSCIENCE;
				Adap.Pci.wSubSysDeviceId = 0x8700;
			}
		}

		// do we have a valid PCI device for this HPI module?
		switch(Adap.Pci.wSubSysDeviceId)
		{
                //case 0x0000:	//prototypes.
				case 0x5000:
				case 0x6400:
				case 0x8700:
					break;	// valid values of Subdevice Id
				default:
					phr->wError = 920;
					return;  // Error - did not find a valid subsys device ID
		}

		// create the adapter object based on the resource information inside Adap.PCI
		nError = H620_CreateAdapterObj( &Adap);                // adapter obj
		if(nError) {
		    phr->wError = nError;
			 HPI_PRINT_ERROR("%d from CreateAdapterObj\n",nError);
		    continue;
            //return;
		}

        // add to adapter list - but don't allow two adapters of same number!
        if( phr->u.s.awAdapterList[ Adap.wIndex ] != 0) {
            phr->wError = HPI_DUPLICATE_ADAPTER_NUMBER;
	    HPI_PRINT_ERROR("Existing ASI%4x index %d, can't add this ASI%4x\n",
			    phr->u.s.awAdapterList[Adap.wIndex],
			    Adap.wIndex,Adap.wAdapterType);
		continue;
	    /*
	    // Try to find an empty slot
	    int i;
	    for (i=HPI_MAX_ADAPTERS-1; i>=0; i--) {
		if (phr->u.s.awAdapterList[i] ==0) {
		    Adap.wIndex=i;
		    break;
		}
	    }
	    if (i<0) // highly unlikely, no free slots.
	      continue;
	    */
	}
        phr->u.s.awAdapterList[ Adap.wIndex ] = Adap.wAdapterType;

        gao60[ Adap.wIndex ] = Adap;
        memcpy( &gao60[ Adap.wIndex ], &Adap, sizeof(H620_ADAPTER_OBJ));
		  gwNum60Adapters++; // inc the number of adapters known by this HPI
	 }


	// add the number of adapters recognised by this HPI to the system total
    phr->u.s.wNumAdapters += gwNum60Adapters;
}

// Create an adapter object and initialise it based on resource information
// passed in in the message
// **** NOTE - you cannot use this function AND the FindAdapters function at the
// same time, the application must use only one of them to get the adapters ******

void H620_SubSysCreateAdapter(HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
    H620_ADAPTER_OBJ ao;	// create temp adapter obj, because we don't know what index yet
	short nError=0;

	HPI_PRINT_INFO( " HPI6205_SubSysCreateAdapter\n" );

	memset( &ao, 0, sizeof(H620_ADAPTER_OBJ) );

	// this HPI only creates adapters for TI/PCI devices
    if((phm->u.s.Resource.wBusType != HPI_BUS_PCI)
				|| ( phm->u.s.Resource.r.Pci.wVendorId != HPI_PCI_VENDOR_ID_TI )
				|| ( phm->u.s.Resource.r.Pci.wDeviceId != HPI_ADAPTER_DSP6205 ))
		  return;

	// create the adapter object based on the resource information
	memcpy(&ao.Pci,&phm->u.s.Resource.r.Pci,sizeof(ao.Pci));

	 nError = H620_CreateAdapterObj( &ao );
	 if(nError)
	 {
		  phr->wError = nError;
		  return;
	 }

	 // add to adapter list - but don't allow two adapters of same number!
	 if( phr->u.s.awAdapterList[ ao.wIndex ] != 0)
	 {
		phr->wError = HPI_DUPLICATE_ADAPTER_NUMBER;
		  return;
	}
	 memcpy( &gao60[ ao.wIndex ], &ao, sizeof(H620_ADAPTER_OBJ));

	 gwNum60Adapters++;         // inc the number of adapters known by this HPI
	phr->u.s.awAdapterList[ ao.wIndex ] = ao.wAdapterType;
	 phr->u.s.wAdapterIndex = ao.wIndex;
	 phr->u.s.wNumAdapters++;    // add the number of adapters recognised by this HPI to the system total
	 phr->wError = 0;            // the function completed OK;
}

// delete an adapter - required by WDM driver


void H620_SubSysDeleteAdapter(HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
	H620_ADAPTER_OBJ *pao=NULL;
	int i;

	 pao = H620_FindAdapter( phm->wAdapterIndex );
	 if(!pao)
		  return; // message probably meant for another HPI module

	if(pao->pInterfaceBuffer)
		HpiOs_LockedMem_Free( pao->hLockedMem );
		pao->pInterfaceBuffer = 0;

	for(i=0;i<H620_MAX_ISTREAMS;i++)
		if( pao->InStreamHostBuffers[i] )
			HpiOs_LockedMem_Free( pao->InStreamHostBuffers[i] );
			pao->InStreamHostBuffers[i] = 0;
			pao->InStreamHostBufferSize[i] = 0;

	for(i=0;i<H620_MAX_OSTREAMS;i++)
		if( pao->OutStreamHostBuffers[i] )
			HpiOs_LockedMem_Free( pao->OutStreamHostBuffers[i] );
			pao->OutStreamHostBuffers[i] = 0;
			pao->OutStreamHostBufferSize[i] = 0;

	for(i=0; i<HPI_MAX_ADAPTER_MEM_SPACES; i++) {
		if ( pao->Pci.dwMemBase[i] ) {
			HPI_PRINT_DEBUG("unmapping pci memory (%lx)\n",pao->Pci.dwMemBase[i]);
			iounmap( (void *)pao->Pci.dwMemBase[i] );
			pao->Pci.dwMemBase[i] = 0;
		}
	}

	gwNum60Adapters--;
	memset( pao, 0, sizeof(H620_ADAPTER_OBJ) );

	phr->wError = 0;
}

// this routine is called from SubSysFindAdapter and SubSysCreateAdapter
short H620_CreateAdapterObj( H620_ADAPTER_OBJ *pao )
{
	 short	nBootError=0;
	tBusMasteringInterfaceBuffer *interface;
	volatile HW32 dwTemp1;
	HW32 dwTimeOut = TIMEOUT;
	int i;

	 // init error reporting
	 pao->wDspCrashed = 0;

	for(i=0;i<H620_MAX_OSTREAMS;i++)
		pao->flagOStreamJustReset[i]=1;

	// The C6205 has the following address map
	// BAR0 - 4Mbyte window into DSP memory
	// BAR1 - 8Mbyte window into DSP registers
	pao->dwHSR = pao->Pci.dwMemBase[1] + C6205_BAR1_HSR;
	pao->dwHDCR = pao->Pci.dwMemBase[1] + C6205_BAR1_HDCR;
	pao->dwDSPP = pao->Pci.dwMemBase[1] + C6205_BAR1_DSPP;

	pao->wHasControlCache = 0;

	if( HpiOs_LockedMem_Alloc(	&pao->hLockedMem,
								sizeof(tBusMasteringInterfaceBuffer),
								(void *)pao->Pci.pOsData
							   ) )
		pao->pInterfaceBuffer = 0;
	else
		if(HpiOs_LockedMem_GetVirtAddr( pao->hLockedMem, (void *)&pao->pInterfaceBuffer ))
			pao->pInterfaceBuffer = 0;

	HPI_PRINT_DEBUG("Interface buffer address 0x%lx\n",(HW32)pao->pInterfaceBuffer);
	if(pao->pInterfaceBuffer) {
		memset((void *)pao->pInterfaceBuffer,0,sizeof(tBusMasteringInterfaceBuffer));
		pao->pInterfaceBuffer->dwDspAck=-1;
	}


	if (0 != (nBootError = Hpi6205_AdapterBootLoadDsp(pao)))
	{
		if(pao->pInterfaceBuffer)
			HpiOs_LockedMem_Free( pao->hLockedMem );
			pao->pInterfaceBuffer = 0;
		return(nBootError); //error
	}
	HPI_PRINT_DEBUG( "Load DSP code OK\n" );

	// allow boot load even if mem alloc wont work
	if (!pao->pInterfaceBuffer)
		return( Hpi6205_Error(0, HPI6205_ERROR_MEM_ALLOC));

	interface=pao->pInterfaceBuffer;

	// wait for first interrupt indicating the DSP init is done
	dwTimeOut = TIMEOUT;
	dwTemp1=0;
	while(((dwTemp1 & C6205_HSR_INTSRC) == 0) && --dwTimeOut)
	{
		dwTemp1 = Hpi6205_Abstract_MEMREAD32(pao,pao->dwHSR);
	}
	if(dwTemp1 & C6205_HSR_INTSRC)
	{
		HPI_PRINT_DEBUG("Interrupt confirming DSP code running OK\n");
	}
	else
	{
		HPI_PRINT_ERROR("Timed out waiting for interrupt confirming DSP code running\n");
		return(Hpi6205_Error( 0, HPI6205_ERROR_6205_NO_IRQ));
	}

	// reset the interrupt
	Hpi6205_Abstract_MEMWRITE32(pao,pao->dwHSR,C6205_HSR_INTSRC);

	// make sure the DSP has started ok
	dwTimeOut = 100;
	while( (interface->dwDspAck != H620_HIF_RESET) && dwTimeOut )
	{
		dwTimeOut--;
		HpiOs_DelayMicroSeconds(10000);
	}

	if(dwTimeOut==0) {
		HPI_PRINT_ERROR("Timed out waiting ack \n");
	    return(Hpi6205_Error( 0, HPI6205_ERROR_6205_INIT_FAILED));
	}
	// set interface to idle
	interface->dwHostCmd = H620_HIF_IDLE;
	// interrupt the DSP again
	dwTemp1 = Hpi6205_Abstract_MEMREAD32(pao,pao->dwHDCR );  //read the control register
	dwTemp1 |= (HW32)C6205_HDCR_DSPINT;
	Hpi6205_Abstract_MEMWRITE32(pao,pao->dwHDCR,dwTemp1);
	dwTemp1 &= ~(HW32)C6205_HDCR_DSPINT;
	Hpi6205_Abstract_MEMWRITE32(pao,pao->dwHDCR,dwTemp1);

	// get info about the adapter by asking the adapter
	// send a HPI_ADAPTER_GET_INFO message
	{
		HPI_MESSAGE     hM;
		HPI_RESPONSE    hR;
		HW16 wError=0;

		//HpiOs_Printf("GetInfo-"); //*************** debug
		HPI_PRINT_DEBUG( "HPI6205.C - send ADAPTER_GET_INFO\n" );
		memset(&hM, 0, sizeof(HPI_MESSAGE));
		hM.wType = HPI_TYPE_MESSAGE;
		hM.wSize = sizeof(HPI_MESSAGE);
		hM.wObject = HPI_OBJ_ADAPTER;
		hM.wFunction =  HPI_ADAPTER_GET_INFO;
		hM.wAdapterIndex = 0;
		memset(&hR, 0, sizeof(HPI_RESPONSE));
		hR.wSize = sizeof(HPI_RESPONSE);

		wError = Hpi6205_MessageResponseSequence( pao,&hM,&hR );
		if(wError)
		{
		    HPI_PRINT_ERROR( "message transport error %d\n",wError );
			return(wError); //error
		}
		if(hR.wError)
		{
			return(hR.wError); //error
		}
		pao->wAdapterType = hR.u.a.wAdapterType;
		pao->wIndex = hR.u.a.wAdapterIndex;
	 }
	if(
		( (pao->wAdapterType&0xf000)==0x5000) ||
	   ( (pao->wAdapterType&0xf000)==0x8000) ||
	   ( (pao->wAdapterType&0xff00)==0x6400)
	   )
		 pao->wHasControlCache=1;

	HPI_PRINT_VERBOSE( "Get adapter info OK\n" );
	pao->wOpen=0;	// upon creation the adapter is closed

	HPI_PRINT_INFO( "Bootload DSP OK\n" );
	return(0);  //sucess!
}

////////////////////////////////////////////////////////////////////////////
// ADAPTER


void H620_AdapterOpen(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
	// input:  wAdapterIndex
    // output: none
	 HPI_PRINT_VERBOSE( "HPI6205_AdapterOpen\n" );

    // can't open adapter if already open
    if(pao->wOpen)
    {
		phr->wError = HPI_ERROR_OBJ_ALREADY_OPEN;
		return;
	}
	 pao->wOpen = 1;         // adapter is now open
	Hpi6205_Message( pao, phm,phr);
}

void H620_AdapterClose(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
	// input:  wAdapterIndex
    // output: none
	HPI_PRINT_VERBOSE( " HPI6205_AdapterClose\n" );

	Hpi6205_Message( pao, phm,phr);

	pao->wOpen = 0;         // adapter is now closed
}

void H620_AdapterGetInfo(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
    Hpi6205_Message( pao, phm,phr);
}

void H620_AdapterGetAsserts(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
	Hpi6205_Message( pao, phm,phr);    //get DSP asserts
	return;
}

//////////////////////////////////////////////////////////////////////
//						OutStream Host buffer functions
//////////////////////////////////////////////////////////////////////

static void H620_OutStreamHostBufferAllocate(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
	HW32 dwError;
	HW32 dwSizeToAllocate;
	tBusMasteringInterfaceBuffer *interface=pao->pInterfaceBuffer;

	HPI_InitResponse(phr, phm->wObject, phm->wFunction, 0 );
	dwSizeToAllocate = sizeof(H620_HOSTBUFFER_STATUS) + phm->u.d.u.Data.dwDataSize;

	if(pao->OutStreamHostBufferSize[phm->u.d.wOStreamIndex]!=dwSizeToAllocate)
	{
		if(pao->OutStreamHostBuffers[phm->u.d.wOStreamIndex])
			HpiOs_LockedMem_Free( pao->OutStreamHostBuffers[phm->u.d.wOStreamIndex] );

		dwError = HpiOs_LockedMem_Alloc(
						&pao->OutStreamHostBuffers[phm->u.d.wOStreamIndex],
						phm->u.d.u.Data.dwDataSize,
						(void *)pao->Pci.pOsData
						);

		if(dwError)
		{
			phr->wSize = HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_STREAM_RES);
			phr->wError = HPI_ERROR_INVALID_DATASIZE;
			pao->OutStreamHostBufferSize[phm->u.d.wOStreamIndex] = 0;
		}
		else
		{
			H620_HOSTBUFFER_STATUS *status;
			HW32 dwPhysicalPCIaddress;
			HW16 wError;

			pao->OutStreamHostBufferSize[phm->u.d.wOStreamIndex] = dwSizeToAllocate;

			status = &interface->aOutStreamHostBufferStatus[phm->u.d.wOStreamIndex];
			status->dwSamplesProcessed = 0;
			status->dwStreamState = HPI_STATE_STOPPED;
			status->dwDSPIndex = 0;
			status->dwHostIndex = 0;
			status->dwSizeInBytes = phm->u.d.u.Data.dwDataSize;
			wError = HpiOs_LockedMem_GetPhysAddr(pao->OutStreamHostBuffers[phm->u.d.wOStreamIndex], &dwPhysicalPCIaddress );
			if(!wError)
			{
				phm->u.d.u.Data.dwpbData = dwPhysicalPCIaddress;
				Hpi6205_Message( pao, phm,phr);
				if(phr->wError)
				{
					// free the buffer
					HpiOs_LockedMem_Free( pao->OutStreamHostBuffers[phm->u.d.wOStreamIndex] );
					pao->OutStreamHostBufferSize[phm->u.d.wOStreamIndex] = 0;
					pao->OutStreamHostBuffers[phm->u.d.wOStreamIndex] = 0;
				}
			}
		}
	}
}
static void H620_OutStreamHostBufferFree(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
	Hpi6205_Message( pao, phm,phr); // Tell adapter to stop using the host buffer.
	if( pao->OutStreamHostBuffers[phm->u.d.wOStreamIndex] )
	{
		HpiOs_LockedMem_Free( pao->OutStreamHostBuffers[phm->u.d.wOStreamIndex] );
		pao->OutStreamHostBufferSize[phm->u.d.wOStreamIndex] = 0;
	}
	pao->OutStreamHostBuffers[phm->u.d.wOStreamIndex] = 0;
}
static long H620_OutStreamGetSpaceAvailable(H620_HOSTBUFFER_STATUS *status)
{
	long nDiff;

	// When dwDSPindex==dwHostIndex the buffer is empty
	// Need to add code to the DSP to make sure that the buffer is never fulled
	// to the point that dwDSPindex==dwHostIndex.
	nDiff =  (long)(status->dwDSPIndex) - (long)(status->dwHostIndex) - 4;	// - 4 bytes at end so we don't overfill
	if(nDiff<0)
		nDiff += status->dwSizeInBytes;
	return nDiff;
}
static void H620_OutStreamWrite(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
	tBusMasteringInterfaceBuffer *interface=pao->pInterfaceBuffer;
	H620_HOSTBUFFER_STATUS *status;
	long dwSpaceAvailable;
	HW8 * pBBMData;
	long lFirstWrite;
	long lSecondWrite;
	HW8 *pAppData = (HW8 *)phm->u.d.u.Data.dwpbData;
	if( !pao->OutStreamHostBuffers[phm->u.d.wOStreamIndex])
	{
		Hpi6205_Message( pao, phm, phr);
		return;
	}
	HPI_InitResponse(phr, phm->wObject, phm->wFunction, 0 );

	if(HpiOs_LockedMem_GetVirtAddr( pao->OutStreamHostBuffers[phm->u.d.wOStreamIndex], (void *)&pBBMData ))
	{
		phr->wError = HPI_ERROR_INVALID_OPERATION;
		return;
	}

	// check whether we need to send the format to the DSP
	if(pao->flagOStreamJustReset[phm->u.d.wOStreamIndex])
	{
		pao->flagOStreamJustReset[phm->u.d.wOStreamIndex]=0;
		phm->wFunction = HPI_OSTREAM_SET_FORMAT;
		Hpi6205_Message( pao, phm, phr);		// send the format to the DSP
		if(phr->wError)
			return;
	}

	status = &interface->aOutStreamHostBufferStatus[phm->u.d.wOStreamIndex];
	dwSpaceAvailable = H620_OutStreamGetSpaceAvailable(status);
	if(dwSpaceAvailable < (long)phm->u.d.u.Data.dwDataSize)
	{
		phr->wError = HPI_ERROR_INVALID_DATASIZE;
		return;
	}

	lFirstWrite = status->dwSizeInBytes - status->dwHostIndex;
	if(lFirstWrite > (long)phm->u.d.u.Data.dwDataSize)
		lFirstWrite = (long)phm->u.d.u.Data.dwDataSize;
	lSecondWrite = (long)phm->u.d.u.Data.dwDataSize - lFirstWrite;

	{
		HW32 dwHostIndex = status->dwHostIndex;

		memcpy(&pBBMData[dwHostIndex], &pAppData[0], lFirstWrite);
		dwHostIndex += (HW32)lFirstWrite;
		if(dwHostIndex >= status->dwSizeInBytes)
			dwHostIndex -= status->dwSizeInBytes;
		if(lSecondWrite)
		{
			memcpy(&pBBMData[dwHostIndex], &pAppData[lFirstWrite], lSecondWrite);
			dwHostIndex += (HW32)lSecondWrite;
			if(dwHostIndex >= status->dwSizeInBytes)
				dwHostIndex -= status->dwSizeInBytes;
		}
		status->dwHostIndex = dwHostIndex;
	}
}
static void H620_OutStreamGetInfo(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
	tBusMasteringInterfaceBuffer *interface=pao->pInterfaceBuffer;
	H620_HOSTBUFFER_STATUS *status;

	if( !pao->OutStreamHostBuffers[phm->u.d.wOStreamIndex])
	{
		Hpi6205_Message( pao, phm,phr);
		return;
	}

	HPI_InitResponse(phr, phm->wObject, phm->wFunction, 0 );

	status = &interface->aOutStreamHostBufferStatus[phm->u.d.wOStreamIndex];

	phr->u.d.wState = (HW16)status->dwStreamState;
	phr->u.d.dwSamplesTransfered = status->dwSamplesProcessed;
	phr->u.d.dwBufferSize = status->dwSizeInBytes;
	phr->u.d.dwDataAvailable = status->dwSizeInBytes - H620_OutStreamGetSpaceAvailable(status);
	phr->u.d.dwAuxilaryDataAvailable = status->dwAuxilaryDataAvailable;
}
static void H620_OutStreamStart(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
	Hpi6205_Message( pao, phm,phr);
}
static void H620_OutStreamReset(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
	pao->flagOStreamJustReset[phm->u.d.wOStreamIndex] = 1;
	Hpi6205_Message( pao, phm,phr);
}
static void H620_OutStreamOpen(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
	H620_OutStreamReset(pao,phm,phr);
}


//////////////////////////////////////////////////////////////////////
//						InStream Host buffer functions
//////////////////////////////////////////////////////////////////////

static void H620_InStreamHostBufferAllocate(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
	HW32 dwError;
	HW32 dwSizeToAllocate;
	tBusMasteringInterfaceBuffer *interface=pao->pInterfaceBuffer;

	HPI_InitResponse(phr, phm->wObject, phm->wFunction, 0 );

	dwSizeToAllocate = sizeof(H620_HOSTBUFFER_STATUS) + phm->u.d.u.Data.dwDataSize;

	if(pao->InStreamHostBufferSize[phm->u.d.wIStreamIndex]!=dwSizeToAllocate)
	{
		if(pao->InStreamHostBuffers[phm->u.d.wIStreamIndex])
			HpiOs_LockedMem_Free( pao->InStreamHostBuffers[phm->u.d.wIStreamIndex] );

		dwError = HpiOs_LockedMem_Alloc(
						&pao->InStreamHostBuffers[phm->u.d.wIStreamIndex],
						phm->u.d.u.Data.dwDataSize,
						(void *)pao->Pci.pOsData
						);

		if(dwError)
		{
			phr->wError = HPI_ERROR_INVALID_DATASIZE;
			pao->InStreamHostBufferSize[phm->u.d.wIStreamIndex] = 0;
		}
		else
		{
			H620_HOSTBUFFER_STATUS *status;
			HW32 dwPhysicalPCIaddress;
			HW16 wError;

			pao->InStreamHostBufferSize[phm->u.d.wIStreamIndex] = dwSizeToAllocate;

			// Why doesn't this work ?? - causes strange behaviour under Win16
			//memset(buffer,0,sizeof(H620_HOSTBUFFER_STATUS) + phm->u.d.u.Data.dwDataSize);
			status = &interface->aInStreamHostBufferStatus[phm->u.d.wIStreamIndex];
			status->dwSamplesProcessed = 0;
			status->dwStreamState = HPI_STATE_STOPPED;
			status->dwDSPIndex = 0;
			status->dwHostIndex = 0;
			status->dwSizeInBytes = phm->u.d.u.Data.dwDataSize;
			wError = HpiOs_LockedMem_GetPhysAddr(pao->InStreamHostBuffers[phm->u.d.wIStreamIndex], &dwPhysicalPCIaddress );
			if(!wError)
			{
				phm->u.d.u.Data.dwpbData = dwPhysicalPCIaddress;
				Hpi6205_Message( pao, phm,phr);
				if(phr->wError)
				{
					// free the buffer
					HpiOs_LockedMem_Free( pao->InStreamHostBuffers[phm->u.d.wIStreamIndex] );
					pao->InStreamHostBufferSize[phm->u.d.wIStreamIndex] = 0;
					pao->InStreamHostBuffers[phm->u.d.wIStreamIndex] = 0;
				}
			}
		}
	}
}

static void H620_InStreamHostBufferFree(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
	Hpi6205_Message( pao, phm,phr);
	if( pao->InStreamHostBuffers[phm->u.d.wIStreamIndex] )
	{
		HpiOs_LockedMem_Free( pao->InStreamHostBuffers[phm->u.d.wIStreamIndex] );
		pao->InStreamHostBufferSize[phm->u.d.wIStreamIndex] = 0;
	}

	pao->InStreamHostBuffers[phm->u.d.wIStreamIndex] = 0;
}
short nValue=0;
static void H620_InStreamStart(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
/*
	if( pao->InStreamHostBuffers[phm->u.d.wIStreamIndex] )	// preset the buffer values
	{
		int i;
		short *pData;
		H620_HOSTBUFFER_STATUS *buffer;

		if(HpiOs_LockedMem_GetVirtAddr( pao->InStreamHostBuffers[phm->u.d.wIStreamIndex], (void *)&buffer ))
		{
			phr->wError = HPI_ERROR_INVALID_OPERATION;
			return;
		}
		pData = ((short *)buffer) + sizeof(H620_HOSTBUFFER_STATUS)/sizeof(short);
		for(i=0;i<buffer->dwSizeInBytes/2/sizeof(short);i++)
		{
			*pData++ = nValue;
			*pData++ = nValue;
			nValue = (nValue+1) & 0x7fff;
		}
	}
*/
	Hpi6205_Message( pao, phm,phr);
}

static long H620_InStreamGetBytesAvailable(	H620_HOSTBUFFER_STATUS *status)
{
	long nDiff;

	// When dwDSPindex==dwHostIndex the buffer is empty
	// Need to add code to the DSP to make sure that the buffer is never fulled
	// to the point that dwDSPindex==dwHostIndex.
	nDiff =  (long)(status->dwDSPIndex) - (long)(status->dwHostIndex);
	if(nDiff<0)
		nDiff += status->dwSizeInBytes;
	return nDiff;
}

static void H620_InStreamRead(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
	tBusMasteringInterfaceBuffer *interface=pao->pInterfaceBuffer;
	H620_HOSTBUFFER_STATUS *status;
	long dwDataAvailable;
	HW8 * pBBMData;
	long lFirstRead;
	long lSecondRead;
	HW8 *pAppData = (HW8 *)phm->u.d.u.Data.dwpbData;
	/* DEBUG
	int i;
	long *pTest;
	*/
	if( !pao->InStreamHostBuffers[phm->u.d.wIStreamIndex])
	{
		Hpi6205_Message( pao, phm,phr);
		return;
	}
	HPI_InitResponse(phr, phm->wObject, phm->wFunction, 0 );

	if(HpiOs_LockedMem_GetVirtAddr( pao->InStreamHostBuffers[phm->u.d.wIStreamIndex], (void *)&pBBMData ))
	{
		phr->wError = HPI_ERROR_INVALID_OPERATION;
		return;
	}

	status = &interface->aInStreamHostBufferStatus[phm->u.d.wIStreamIndex];
	dwDataAvailable= H620_InStreamGetBytesAvailable(status);
	if(dwDataAvailable<(long)phm->u.d.u.Data.dwDataSize)
	{
		phr->wError = HPI_ERROR_INVALID_DATASIZE;
		return;
	}

	lFirstRead = status->dwSizeInBytes - status->dwHostIndex;
	if(lFirstRead > (long)phm->u.d.u.Data.dwDataSize)
		lFirstRead = (long)phm->u.d.u.Data.dwDataSize;
	lSecondRead = (long)phm->u.d.u.Data.dwDataSize - lFirstRead;

	{  // avoid having status->dwHostIndex invalid, even momentarily
		HW32 dwHostIndex =  status->dwHostIndex;

		memcpy(&pAppData[0], &pBBMData[dwHostIndex],lFirstRead);
		dwHostIndex += (HW32)lFirstRead;
		if(dwHostIndex >= status->dwSizeInBytes)
			dwHostIndex -= status->dwSizeInBytes;
		if(lSecondRead)
		{
			memcpy(&pAppData[lFirstRead], &pBBMData[dwHostIndex],lSecondRead);
			dwHostIndex += (HW32)lSecondRead;
			if(dwHostIndex >= status->dwSizeInBytes)
				dwHostIndex -= status->dwSizeInBytes;
		}
		status->dwHostIndex = dwHostIndex;
	}

	/* DBEUG */
	/*
	pTest = (long *)phm->u.d.u.Data.dwpbData;
	for(i=0;i<phm->u.d.u.Data.dwDataSize/sizeof(long);i++)
	{
		if(pTest[i])
			pTest[i]--;
	}
	pTest = (long *)((char *)buffer) + sizeof(H620_HOSTBUFFER_STATUS);
	for(i=0;i<buffer->dwSizeInBytes/sizeof(long);i++)
	{
		if(pTest[i])
			pTest[i]--;
	}
	*/
}

static void H620_InStreamGetInfo(H620_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
	tBusMasteringInterfaceBuffer *interface=pao->pInterfaceBuffer;
	H620_HOSTBUFFER_STATUS *status;
	if( !pao->InStreamHostBuffers[phm->u.d.wIStreamIndex])
	{
		Hpi6205_Message( pao, phm,phr);
		return;
	}

	status = &interface->aInStreamHostBufferStatus[phm->u.d.wIStreamIndex];

	HPI_InitResponse(phr, phm->wObject, phm->wFunction, 0 );

	phr->u.d.wState = (HW16)status->dwStreamState;
	phr->u.d.dwSamplesTransfered = status->dwSamplesProcessed;
	phr->u.d.dwBufferSize = status->dwSizeInBytes;
	phr->u.d.dwDataAvailable = H620_InStreamGetBytesAvailable(status);
	phr->u.d.dwAuxilaryDataAvailable = status->dwAuxilaryDataAvailable;
}

////////////////////////////////////////////////////////////////////////////
// LOW-LEVEL
///////////////////////////////////////////////////////////////////////////

void H620_AdapterIndex( HPI_RESOURCE * res, short * wAdapterIndex )
{
	int idx;

	HPIPCI_MATCH_RESOURCE( idx, H620_MAX_ADAPTERS, *wAdapterIndex, gao60, *res );
	HPI_PRINT_VERBOSE("H620_AdapterIndex %d\n",*wAdapterIndex);
}

H620_ADAPTER_OBJ* H620_FindAdapter( HW16 wAdapterIndex )
{
	H620_ADAPTER_OBJ *pao=NULL;

	HPI_PRINT_VERBOSE("H620_FindAdapter %d ",wAdapterIndex);
    pao = &gao60[wAdapterIndex];
	if(pao->wAdapterType != 0)
	{
		HPI_PRINT_VERBOSE("found\n");
		return(pao);
	}
	else
	{
		HPI_PRINT_VERBOSE("not found\n");
		 return(NULL);
	}
}

////////////////////////////////////////////////////////////////////////////

short Hpi6205_AdapterCheckPresent( H620_ADAPTER_OBJ *pao )
{
	 return 0;
}

#define H620_MAX_FILES_TO_LOAD 3

short Hpi6205_AdapterBootLoadDsp( H620_ADAPTER_OBJ *pao)
{
	DSP_CODE DspCode;
	HW16 anBootLoadFamily[H620_MAX_FILES_TO_LOAD];
	volatile HW32 dwTemp;
	int nDsp=0, i=0;
	HW16 wError=0;

	// by default there is no DSP code to load
	for(i=0;i<H620_MAX_FILES_TO_LOAD;i++)
		anBootLoadFamily[i]=0;

	switch(pao->Pci.wSubSysDeviceId)
	{
		case 0x5000:
			anBootLoadFamily[0] = Load5000;	// base 6205 code
			break;
		case 0x6400:
			anBootLoadFamily[0] = Load6205;	// base 6205 code
			anBootLoadFamily[1] = Load6413;	// 6713 code
			break;
		case 0x8700:
			anBootLoadFamily[0] = Load6205;	// base 6205 code
			anBootLoadFamily[1] = Load8713;	// 6713 code
			break;
		default:
			return(Hpi6205_Error(0,HPI6205_ERROR_UNKNOWN_PCI_DEVICE));
	}

	//////////////////////////////////////////////////
	// check we can read the 6205 registers ok

	// reset DSP by writing a 1 to the WARMRESET bit
	dwTemp = C6205_HDCR_WARMRESET;
	Hpi6205_Abstract_MEMWRITE32(pao,pao->dwHDCR, dwTemp );
	HpiOs_DelayMicroSeconds(1000);
//	for(i=0;i<1000; i++) dwTemp = Hpi6205_Abstract_MEMREAD32(pao,pao->dwHDCR );  //delay

	// check that PCI i/f was configured by EEPROM
	dwTemp = Hpi6205_Abstract_MEMREAD32(pao,pao->dwHSR );  //read the HSR register.
	if((dwTemp & (C6205_HSR_CFGERR|C6205_HSR_EEREAD)) != C6205_HSR_EEREAD)
		return Hpi6205_Error(0, HPI6205_ERROR_6205_EEPROM);
	dwTemp |= 0x04;
	Hpi6205_Abstract_MEMWRITE32(pao,pao->dwHSR, dwTemp );	// disable PINTA interrupt


	// check control register reports PCI boot mode
	dwTemp = Hpi6205_Abstract_MEMREAD32(pao,pao->dwHDCR );  //read the control register.
	if( !(dwTemp&C6205_HDCR_PCIBOOT) )
		return Hpi6205_Error(0, HPI6205_ERROR_6205_REG);

	// try writing a couple of numbers to the DSP page register and reading them back.
	dwTemp = 1 ;
	Hpi6205_Abstract_MEMWRITE32(pao,pao->dwDSPP, dwTemp );
	if( (dwTemp | C6205_DSPP_MAP1)  != Hpi6205_Abstract_MEMREAD32(pao,pao->dwDSPP ) )
		return Hpi6205_Error(0, HPI6205_ERROR_6205_DSPPAGE);
	dwTemp = 2;
	Hpi6205_Abstract_MEMWRITE32(pao,pao->dwDSPP, dwTemp );
	if((dwTemp | C6205_DSPP_MAP1) != Hpi6205_Abstract_MEMREAD32(pao,pao->dwDSPP ) )
		return Hpi6205_Error(0, HPI6205_ERROR_6205_DSPPAGE);
	dwTemp = 3;
	Hpi6205_Abstract_MEMWRITE32(pao,pao->dwDSPP, dwTemp );
	if((dwTemp | C6205_DSPP_MAP1) != Hpi6205_Abstract_MEMREAD32(pao,pao->dwDSPP) )
		return Hpi6205_Error(0, HPI6205_ERROR_6205_DSPPAGE);
	// reset DSP page to the correct number
	dwTemp = 0;
	Hpi6205_Abstract_MEMWRITE32(pao,pao->dwDSPP, dwTemp );
	if( (dwTemp | C6205_DSPP_MAP1) != Hpi6205_Abstract_MEMREAD32(pao,pao->dwDSPP) )
		return Hpi6205_Error(0, HPI6205_ERROR_6205_DSPPAGE);
	pao->dwDspPage = 0;

	/* release 6713 from reset before 6205 is bootloaded. This ensures that the EMIF
		is inactive, and the 6713 HPI gets the correct bootmode etc
	*/
	if(anBootLoadFamily[1]!=0) {
		// DSP 1 is a C6713
		BootLoader_WriteMem32( pao, 0, (0x018C0024L),0x00002202);	// CLKX0 <- '1' release the C6205 bootmode pulldowns
		HpiOs_DelayMicroSeconds(100);
		BootLoader_WriteMem32( pao, 0, C6205_BAR0_TIMER1_CTL,0);	// Reset the 6713 #1 - revB

		// dummy read every 4 words for 6205 advisory 1.4.4
		BootLoader_ReadMem32( pao, 0, 0);

		HpiOs_DelayMicroSeconds(100);
		BootLoader_WriteMem32( pao, 0, C6205_BAR0_TIMER1_CTL,4);	// Release C6713 from reset - revB
		HpiOs_DelayMicroSeconds(100);
	}


	for(nDsp=0;nDsp<H620_MAX_FILES_TO_LOAD;nDsp++)
	{

		if(anBootLoadFamily[nDsp]==0)	// is there a DSP to load
			continue;

		// for each DSP

		// configure EMIF
		wError=BootLoader_ConfigEMIF(pao, nDsp);
		if (wError)
			return(wError);

		// check internal memory
		wError=BootLoader_TestInternalMemory(pao, nDsp);
		if (wError)
			return(wError);

		// test SDRAM
		wError=BootLoader_TestExternalMemory(pao, nDsp);
		if (wError)
			return(wError);

		// test for PLD located on DSPs EMIF bus
		wError=BootLoader_TestPld(pao, nDsp);
		if (wError)
			return(wError);

		///////////////////////////////////////////////////////////
		// write the DSP code down into the DSPs memory
	    DspCode.psDev = pao->Pci.pOsData;
		if ((wError=HpiDspCode_Open(anBootLoadFamily[nDsp],&DspCode))!= 0)
			return( wError );
		while (1)
		{
			HW32 dwLength;
			HW32 dwAddress;
			HW32 dwType;
			HW32 *pdwCode;

			if ((wError=HpiDspCode_ReadWord(&DspCode,&dwLength))!= 0)
				break;
			if (dwLength == 0xFFFFFFFF)
				break; // end of code

			if ((wError=HpiDspCode_ReadWord(&DspCode,&dwAddress))!= 0)
				break;
			if ((wError=HpiDspCode_ReadWord(&DspCode,&dwType))!= 0)
				break;
			if ((wError=HpiDspCode_ReadBlock(dwLength,&DspCode,&pdwCode))!= 0)
				break;
			//if ((wError=BootLoader_BlockWrite32( pao, nDsp, dwAddress, (HW32)pdwCode, dwLength))
			//			!= 0)
			//	break;
			for(i=0; i<(int)dwLength; i++)
			{
				wError = BootLoader_WriteMem32( pao, nDsp, dwAddress, *pdwCode);
				if(wError)
						break;
				// dummy read every 4 words for 6205 advisory 1.4.4
				if(i%4==0)
						BootLoader_ReadMem32( pao, nDsp, dwAddress);
				pdwCode++;
				dwAddress += 4;
			}

		}
		if (wError) {
		    HpiDspCode_Close(&DspCode);
		    return(wError);
		}
		/////////////////////////////
		// verify code
		HpiDspCode_Rewind(&DspCode);
		while (1)
		{
			HW32 dwLength=0;
			HW32 dwAddress=0;
			HW32 dwType=0;
			HW32 *pdwCode=NULL;
			HW32 dwData=0;

			HpiDspCode_ReadWord(&DspCode,&dwLength);
			if (dwLength == 0xFFFFFFFF)
					break; // end of code

			HpiDspCode_ReadWord(&DspCode,&dwAddress);
			HpiDspCode_ReadWord(&DspCode,&dwType);
			HpiDspCode_ReadBlock(dwLength,&DspCode,&pdwCode);

			for(i=0; i<(int)dwLength; i++)
			{
				dwData = BootLoader_ReadMem32( pao, nDsp, dwAddress);      //read the data back
				if(dwData != *pdwCode)
				{
						wError = 0;
						break;
				}
				pdwCode++;
				dwAddress += 4;
			}
			if (wError)
				break;
		}
		HpiDspCode_Close(&DspCode);
		if (wError)
			return (wError);
	}

	// After bootloading all DSPs, start DSP0 running
	// The DSP0 code will handle starting and synchronizing with its slaves
	if (pao->pInterfaceBuffer)
	{
		// we need to tell the card the physical PCI address
		HW32 dwPhysicalPCIaddress;
		HW16 wError;
		tBusMasteringInterfaceBuffer *interface=pao->pInterfaceBuffer;
		HW32 dwHostMailboxAddressOnDsp;
		HW32 dwPhysicalPCIaddressVerify=0;
		int nTimeOut=10;
		// set ack so we know when DSP is ready to go (dwDspAck will be changed to H620_HIF_RESET)
		interface->dwDspAck = H620_HIF_UNKNOWN;

		wError = HpiOs_LockedMem_GetPhysAddr( pao->hLockedMem, &dwPhysicalPCIaddress );

		// locate the host mailbox on the DSP.
		dwHostMailboxAddressOnDsp = 0x80000000;
		while( (dwPhysicalPCIaddress != dwPhysicalPCIaddressVerify) && nTimeOut-- )
		{
			wError = BootLoader_WriteMem32( pao, 0, dwHostMailboxAddressOnDsp, dwPhysicalPCIaddress);
			dwPhysicalPCIaddressVerify = BootLoader_ReadMem32( pao, 0, dwHostMailboxAddressOnDsp);
		}
	}
	HPI_PRINT_DEBUG("Starting DSPs running\n");
	// enable interrupts
	dwTemp = Hpi6205_Abstract_MEMREAD32(pao,pao->dwHSR );  //read the control register
	dwTemp &= ~(HW32)C6205_HSR_INTAM;
	Hpi6205_Abstract_MEMWRITE32(pao,pao->dwHSR,dwTemp);

	// start code running...
	// need to remove from here because actual implementation will depend on DSP index.
	dwTemp = Hpi6205_Abstract_MEMREAD32(pao,pao->dwHDCR );  //read the control register
	dwTemp |= (HW32)C6205_HDCR_DSPINT;
	Hpi6205_Abstract_MEMWRITE32(pao,pao->dwHDCR,dwTemp);

	// give the DSP 10ms to start up
	HpiOs_DelayMicroSeconds(10000);
	return wError;

}

//////////////////////////////////////////////////////////////////////
//						Bootloader utility functions
//////////////////////////////////////////////////////////////////////


static HW32 BootLoader_ReadMem32(H620_ADAPTER_OBJ *pao, int nDSPIndex, HW32 dwAddress)
{
	HW32 dwData=0;

	if(nDSPIndex==0)
	{
		// DSP 0 is always C6205
		// DSP 0 is always C6205
		if((dwAddress >= 0x01800000) & (dwAddress < 0x02000000 ))
		{
			// BAR1 register access
			dwData = Hpi6205_Abstract_MEMREAD32(pao, pao->Pci.dwMemBase[1] + (dwAddress&0x007fffff));
		}
		else
		{
			HW32 dw4MPage = dwAddress >> 22L;
			if(dw4MPage != pao->dwDspPage)
			{
				pao->dwDspPage = dw4MPage;
				Hpi6205_Abstract_MEMWRITE32(pao,pao->dwDSPP, pao->dwDspPage );
			}
			dwAddress &= 0x3fffff;	// address within 4M page
			// BAR0 memory access
			dwData = Hpi6205_Abstract_MEMREAD32(pao, pao->Pci.dwMemBase[0] + dwAddress);
		}
	}
	else if (nDSPIndex==1)
	{	// DSP 1 is a C6713
		HW32 dwLsb;
		BootLoader_WriteMem32(pao, 0, HPIAL_ADDR, dwAddress);
		BootLoader_WriteMem32(pao, 0, HPIAH_ADDR, dwAddress>>16);
		dwLsb=BootLoader_ReadMem32(pao, 0, HPIDL_ADDR);
		dwData=BootLoader_ReadMem32(pao, 0, HPIDH_ADDR);
		dwData = (dwData<<16) | (dwLsb & 0xFFFF);
	}
	else if (nDSPIndex==2)
	{
		// DSP 1 is a C6713

	}
	return dwData;
}
static HW16 BootLoader_WriteMem32(H620_ADAPTER_OBJ *pao, int nDSPIndex, HW32 dwAddress, HW32 dwData)
{
	HW16 nError=0;
	//	HW32 dwVerifyData=0;

	if(nDSPIndex==0)
	{
		// DSP 0 is always C6205
		if((dwAddress >= 0x01800000) & (dwAddress < 0x02000000 ))
		{
			// BAR1 - DSP  register access using Non-prefetchable PCI access
			Hpi6205_Abstract_MEMWRITE32(pao,pao->Pci.dwMemBase[1]+(dwAddress&0x007fffff),dwData);
		}
		else // BAR0 access - all of DSP memory using pre-fetchable PCI access
		{
			HW32 dw4MPage = dwAddress >> 22L;
			if(dw4MPage != pao->dwDspPage)
			{
				pao->dwDspPage = dw4MPage;
				Hpi6205_Abstract_MEMWRITE32(pao,pao->dwDSPP, pao->dwDspPage  );
			}
			dwAddress &= 0x3fffff;	// address within 4M page
			Hpi6205_Abstract_MEMWRITE32(pao,pao->Pci.dwMemBase[0]+dwAddress,dwData);

			if (0) {
				//			if ((dwAddress !=HPIDL_ADDR) && (dwAddress !=HPIDH_ADDR )) {
				//!EWB verifying writes to 6713 HPID causes problems!!! temporarily disable
				// create a new function that doesn't do it specially?
				HW32 dwVerifyData = Hpi6205_Abstract_MEMREAD32(pao, pao->Pci.dwMemBase[0] + dwAddress);
				if(dwVerifyData != dwData)
				{
					nError = HPI_ERROR_DSP_HARDWARE;
					return nError;
				}
			}
		}
	}
	else if (nDSPIndex==1)
	{	// DSP 1 is a C6713
		BootLoader_WriteMem32(pao, 0, HPIAL_ADDR, dwAddress);
		BootLoader_WriteMem32(pao, 0, HPIAH_ADDR, dwAddress>>16);

		// dummy read every 4 words for 6205 advisory 1.4.4
		BootLoader_ReadMem32( pao, 0, 0);

		BootLoader_WriteMem32(pao, 0, HPIDL_ADDR, dwData);
		BootLoader_WriteMem32(pao, 0, HPIDH_ADDR, dwData>>16);

		// dummy read every 4 words for 6205 advisory 1.4.4
		BootLoader_ReadMem32( pao, 0, 0);
	}
	else if (nDSPIndex==2)
	{
		// DSP 1 is a C6713

	}
	else
		nError = Hpi6205_Error(nDSPIndex, HPI6205_ERROR_BAD_DSPINDEX);
	return nError;
}
/*
HW16 BootLoader_BlockWrite32( H620_ADAPTER_OBJ *pao, HW16 wDspIndex, HW32 dwDspDestinationAddress, HW32 dwSourceAddress, HW32 dwCount)
{
	HW32 dwIndex;
	HW16 wError;
	HW32 *pdwData=(HW32 *)dwSourceAddress;

	for(dwIndex=0; dwIndex<dwCount; dwIndex++)
	{
		wError = BootLoader_WriteMem32(pao, wDspIndex, dwDspDestinationAddress+dwIndex*4, pdwData[dwIndex]);
		if(wError)
			break;
	}
	return wError;
}
*/
static HW16 BootLoader_ConfigEMIF(H620_ADAPTER_OBJ *pao, int nDSPIndex)
{
	HW16 nError=0;

	if(nDSPIndex==0)
	{
		HW32 dwSetting;

		// DSP 0 is always C6205

		// Set the EMIF
		// memory map of C6205
		// 00000000-0000FFFF	16Kx32 internal program
		// 00400000-00BFFFFF	CE0	2Mx32 SDRAM running @ 100MHz

		// EMIF config
		//------------
		// Global EMIF control
		BootLoader_WriteMem32( pao, nDSPIndex, 0x01800000, 0x3779 );
#define WS_OFS 28
#define WST_OFS 22
#define WH_OFS 20
#define RS_OFS 16
#define RST_OFS 8
#define MTYPE_OFS 4
#define RH_OFS 0

		// EMIF CE0 setup - 2Mx32 Sync DRAM on ASI5000 cards only
		dwSetting = 0x00000030;
		BootLoader_WriteMem32( pao, nDSPIndex, 0x01800008, dwSetting);  // CE0
		if(dwSetting!=BootLoader_ReadMem32(pao, nDSPIndex, 0x01800008) )
			return Hpi6205_Error( nDSPIndex , HPI6205_ERROR_DSP_EMIF);

		// EMIF CE1 setup - 32 bit async. This is 6713 #1 HPI, which occupies D15..0. 6713 starts at 27MHz, so need
		// plenty of wait states. See dsn8701.rtf, and 6713 errata.
		dwSetting =
			(1L<<WS_OFS)  |
			(63L<<WST_OFS) |
			 (1L<<WH_OFS)  |
			 (1L<<RS_OFS)  |
			(63L<<RST_OFS) |  // should be 71, but 63 is max possible
			 (1L<<RH_OFS)  |
			 (2L<<MTYPE_OFS);
		BootLoader_WriteMem32( pao, nDSPIndex, 0x01800004, dwSetting); // CE1
		if(dwSetting!=BootLoader_ReadMem32(pao, nDSPIndex, 0x01800004) )
			return Hpi6205_Error( nDSPIndex , HPI6205_ERROR_DSP_EMIF);

		// EMIF CE2 setup - 32 bit async. This is 6713 #2 HPI, which occupies D15..0. 6713 starts at 27MHz, so need
		// plenty of wait states
		dwSetting =
			 (1L<<WS_OFS)  |
			(28L<<WST_OFS) |
			 (1L<<WH_OFS)  |
			 (1L<<RS_OFS)  |
			(63L<<RST_OFS) |
			 (1L<<RH_OFS)  |
			 (2L<<MTYPE_OFS);
		BootLoader_WriteMem32( pao, nDSPIndex, 0x01800010, dwSetting);  // CE2
		if(dwSetting!=BootLoader_ReadMem32(pao, nDSPIndex, 0x01800010) )
			return Hpi6205_Error( nDSPIndex , HPI6205_ERROR_DSP_EMIF);

		// EMIF CE3 setup - 32 bit async. This is the PLD on the ASI5000 cards only
		dwSetting =
			 (1L<<WS_OFS)  |
			(10L<<WST_OFS) |
			 (1L<<WH_OFS)  |
			 (1L<<RS_OFS)  |
			(10L<<RST_OFS) |
			 (1L<<RH_OFS)  |
			 (2L<<MTYPE_OFS);
		BootLoader_WriteMem32( pao, nDSPIndex, 0x01800014, dwSetting); // CE3
		if(dwSetting!=BootLoader_ReadMem32(pao, nDSPIndex, 0x01800014) )
			return Hpi6205_Error( nDSPIndex , HPI6205_ERROR_DSP_EMIF);

		// EMIF SDRAM control - set up for a 2Mx32 SDRAM (512x32x4 bank)
		BootLoader_WriteMem32( pao, nDSPIndex, 0x01800018, 0x07117000);	//  need to use this else DSP code crashes?

		// EMIF SDRAM Refresh Timing
		BootLoader_WriteMem32( pao, nDSPIndex, 0x0180001C,0x00000410);	// EMIF SDRAM timing  (orig = 0x410, emulator = 0x61a)

	}
	else if (nDSPIndex==1)
	{
		// test access to the C6713s HPI registers
		HW32 dwWriteData=0, dwReadData=0, i=0;

		// HPIC - Set up HPIC for little endian, by setiing HPIC:HWOB=1
		dwWriteData = 1;
		BootLoader_WriteMem32( pao, 0, HPICL_ADDR, dwWriteData);
		BootLoader_WriteMem32( pao, 0, HPICH_ADDR, dwWriteData);
		dwReadData= 0xFFF7& BootLoader_ReadMem32( pao, 0, HPICL_ADDR);	// C67 HPI is on lower 16bits of 32bit EMIF
		if(dwWriteData != dwReadData)
		{
			nError = Hpi6205_Error(nDSPIndex, HPI6205_ERROR_C6713_HPIC);
            gadwHpiSpecificError[0] = HPICL_ADDR;
			gadwHpiSpecificError[1] = dwWriteData;
            gadwHpiSpecificError[2] = dwReadData;
			return nError;
		}

		// HPIA - walking ones test
		dwWriteData = 1;
		for(i=0; i<32; i++)
		{
			BootLoader_WriteMem32( pao, 0, HPIAL_ADDR, dwWriteData);
			BootLoader_WriteMem32( pao, 0, HPIAH_ADDR, (dwWriteData >> 16));
			dwReadData= 0xFFFF & BootLoader_ReadMem32( pao, 0, HPIAL_ADDR);
			dwReadData= dwReadData | ((0xFFFF & BootLoader_ReadMem32( pao, 0, HPIAH_ADDR))<<16);
			if(dwReadData != dwWriteData)
			{
				nError = Hpi6205_Error(nDSPIndex, HPI6205_ERROR_C6713_HPIA);
            	gadwHpiSpecificError[0] = HPIAH_ADDR;
				gadwHpiSpecificError[1] = dwWriteData;
            	gadwHpiSpecificError[2] = dwReadData;
				return nError;
			}
			dwWriteData = dwWriteData << 1;
		}

		// setup C67x PLL
		// ** C6713 datasheet says we cannot program PLL from HPI, and indeed if we try to set the
		// PLL multiply from the HPI, the PLL does not seem to lock, so we enable the PLL and use the default
		// multiply of x 7, which for a 27MHz clock gives a DSP speed of 189MHz
		BootLoader_WriteMem32( pao,nDSPIndex, 0x01B7C100, 0x0000 );  // bypass PLL
		HpiOs_DelayMicroSeconds(1000);
		BootLoader_WriteMem32( pao,nDSPIndex, 0x01B7C120, 0x8002 );  // EMIF = 189/3=63MHz
		BootLoader_WriteMem32( pao,nDSPIndex, 0x01B7C11C, 0x8001 );  // peri = 189/2
		BootLoader_WriteMem32( pao,nDSPIndex, 0x01B7C118, 0x8000 );  // cpu  = 189/1
		HpiOs_DelayMicroSeconds(1000);
		// ** SGT test to take GPO3 high when we start the PLL and low when the delay is completed
		BootLoader_WriteMem32( pao, 0, (0x018C0024L),0x00002A0A);	// FSX0 <- '1' (GPO3)
		BootLoader_WriteMem32( pao,nDSPIndex, 0x01B7C100, 0x0001 );  // PLL not bypassed
		HpiOs_DelayMicroSeconds(1000);
		BootLoader_WriteMem32( pao, 0, (0x018C0024L),0x00002A02);	// FSX0 <- '0' (GPO3)

		// 6205 EMIF CE1 resetup - 32 bit async. Now 6713 #1 is running at 189MHz can reduce waitstates
		BootLoader_WriteMem32( pao, 0, 0x01800004,  // CE1
			 1L<<WS_OFS  |
			 8L<<WST_OFS |
			 1L<<WH_OFS  |
			 1L<<RS_OFS  |
			12L<<RST_OFS |
			 1L<<RH_OFS  |
			 2L<<MTYPE_OFS);

		HpiOs_DelayMicroSeconds(1000);

		// check that we can read one of the PLL registers
		if( (BootLoader_ReadMem32( pao,nDSPIndex,0x01B7C100 ) & 0xF) != 0x0001 )	// PLL should not be bypassed!
		{
			nError = Hpi6205_Error( nDSPIndex , HPI6205_ERROR_C6713_PLL);
			return nError;
		}

		// setup C67x EMIF
		BootLoader_WriteMem32( pao,nDSPIndex, C6713_EMIF_GCTL,       0x000034A8 );  // global control (orig=C6711 only = 0x3488)
		BootLoader_WriteMem32( pao,nDSPIndex, C6713_EMIF_CE0,        0x00000030);  // CE0
		BootLoader_WriteMem32( pao,nDSPIndex, C6713_EMIF_SDRAMEXT,   0x001BDF29 );   // need to use this else DSP code crashes?
		BootLoader_WriteMem32( pao,nDSPIndex, C6713_EMIF_SDRAMCTL,   0x47117000);	//  need to use this else DSP code crashes?
		BootLoader_WriteMem32( pao,nDSPIndex, C6713_EMIF_SDRAMTIMING,0x00000410);	// EMIF SDRAM timing  (orig = 0x410, emulator = 0x61a)

		HpiOs_DelayMicroSeconds(1000);
	}
	else if (nDSPIndex==2)
	{
		// DSP 2 is a C6713

	}
	else
		nError = Hpi6205_Error( nDSPIndex , HPI6205_ERROR_BAD_DSPINDEX);
	return nError;
}

static HW16 BootLoader_TestMemory(H620_ADAPTER_OBJ *pao, int nDSPIndex, HW32 dwStartAddress, HW32 dwLength)
{
	HW32 i=0, j=0;
	HW32 dwTestAddr=0;
	HW32 dwTestData=0, dwData=0;

	// test each bit in the 32bit word, dwLength specifies number of 32bit words to test
	for(i=0; i<dwLength; i++)
	{
		dwTestAddr=dwStartAddress+(HW32)i*4;
		dwTestData=0x00000001;
		for(j=0; j<32; j++)
		{
			BootLoader_WriteMem32( pao, nDSPIndex, dwTestAddr, dwTestData);   //write the data to internal DSP mem
			dwData = BootLoader_ReadMem32( pao, nDSPIndex, dwTestAddr);      //read the data back
			if(dwData != dwTestData)
			{
				gadwHpiSpecificError[0] = dwTestAddr;
				gadwHpiSpecificError[1] = dwTestData;
				gadwHpiSpecificError[2] = dwData;
				gadwHpiSpecificError[3] = nDSPIndex;
                HPI_PRINT_VERBOSE("Memtest error details  %08lx %08lx %08lx %i\n", dwTestAddr,dwTestData,dwData,nDSPIndex);
				return(1);	// error
			}
			dwTestData = dwTestData << 1;
		} // for(j)
		BootLoader_WriteMem32( pao, nDSPIndex, dwTestAddr, 0x0);   // leave location as zero
	} // for(i)
	return(0);	//success!
}

static HW16 BootLoader_TestInternalMemory(H620_ADAPTER_OBJ *pao, int nDSPIndex)
{
	int nError=0;

	nError = BootLoader_TestMemory( pao, nDSPIndex, 0x00000000, 1000); // test 1st 1000 program RAM locations

	// only DSP0 (C6205) has internal data memory
	if(nDSPIndex==0)
	{
		if(!nError)
			nError = BootLoader_TestMemory( pao, nDSPIndex, 0x80000000, 1000); // test 1st 1000 data RAM locations
	}
	if(nError)
		return( Hpi6205_Error( nDSPIndex, HPI6205_ERROR_DSP_INTMEM));
	else
		return 0;
}
static HW16 BootLoader_TestExternalMemory(H620_ADAPTER_OBJ *pao, int nDSPIndex)
{
	HW32 dwDRAMStartAddress;
	HW32 dwDRAMSize;
	HW32 dwDRAMinc;

	if(nDSPIndex==0)
	{
		// only test for SDRAM if an ASI5000 card
		if(pao->Pci.wSubSysDeviceId == 0x5000)
		{
			// DSP 0 is always C6205
			dwDRAMStartAddress=0x00400000;
			dwDRAMSize=0x200000;
			dwDRAMinc=1024;
		}
		else
			return(0);
	}
	else if (nDSPIndex==1)
	{
		// DSP 1 is a C6713
		dwDRAMStartAddress=0x80000000;
		dwDRAMSize=0x200000;
		dwDRAMinc=1024;
	}
	else if (nDSPIndex==2)
	{
		// DSP 2 is a C6713
		dwDRAMStartAddress=0x80000000;
		dwDRAMSize=0x200000;
		dwDRAMinc=1024;
	}
	else
		return  Hpi6205_Error(nDSPIndex, HPI6205_ERROR_BAD_DSPINDEX);

#if 1
	// use more comprehensive test, but doesn't cover whole dram
   return BootLoader_TestMemory( pao, nDSPIndex, dwDRAMStartAddress, dwDRAMinc); // test 1st 1000 program RAM locations
#else
	// test every Nth address in the DRAM
   {
	HW32 dwTestData;
	HW32 i;
	dwTestData=0x0;
	for(i=0; i<dwDRAMSize; i=i+dwDRAMinc)
	{
		BootLoader_WriteMem32( pao, nDSPIndex, dwDRAMStartAddress+i, dwTestData);
		dwTestData++;
	}
	dwTestData=0x0;
	for(i=0; i<dwDRAMSize; i=i+dwDRAMinc)
	{
		HW32 dwData = BootLoader_ReadMem32( pao, nDSPIndex, dwDRAMStartAddress+i);
		if(dwData != dwTestData)
		{
			gadwHpiSpecificError[0] = dwDRAMStartAddress+i;
			gadwHpiSpecificError[1] = dwTestData;
			gadwHpiSpecificError[2] = dwData;
			gadwHpiSpecificError[3] = nDSPIndex;
			return( Hpi6205_Error(nDSPIndex, HPI6205_ERROR_DSP_EXTMEM));
		}
		dwTestData++;
	}
   }
	return 0; 	//success!
#endif
}


HW16 BootLoader_TestPld(H620_ADAPTER_OBJ *pao, int nDSPIndex)
{
	HW32 dwData=0;
	// only test for PLD on ASI5000 card
	if(nDSPIndex==0)
	{
		if(pao->Pci.wSubSysDeviceId == 0x5000)
		{
			// PLD is located at CE3=0x03000000
			dwData = BootLoader_ReadMem32( pao, nDSPIndex, 0x03000008);
			if((dwData & 0xF) != 0x5)
				return( Hpi6205_Error(nDSPIndex, HPI6205_ERROR_DSP_PLD) );
			dwData = BootLoader_ReadMem32( pao, nDSPIndex, 0x0300000C);
			if((dwData & 0xF) != 0xA)
				return( Hpi6205_Error(nDSPIndex, HPI6205_ERROR_DSP_PLD) );
			// 5000 - just for fun, turn off the LED attached to the PLD
			//BootLoader_WriteMem32(pao,0,pao->Pci.dwMemBase[0]+C6205_BAR0_TIMER1_CTL,4);	// SYSRES- = 1
			//BootLoader_WriteMem32( pao, nDSPIndex, 0x03000014, 0x02);                       // LED off
			//BootLoader_WriteMem32(pao,0,pao->Pci.dwMemBase[0]+C6205_BAR0_TIMER1_CTL,0);	// SYSRES- = 0 , LED back on
		}
		//BootLoader_WriteMem32( pao, nDSPIndex, 0x018C0024, 0x00003501);                       // 8705 - LED on
	}
	else if (nDSPIndex==1)
	{
		// DSP 1 is a C6713
		if(pao->Pci.wSubSysDeviceId == 0x8700)
		{
			// PLD is located at CE1=0x90000000
			dwData = BootLoader_ReadMem32( pao, nDSPIndex, 0x90000010);
			if((dwData & 0xFF) != 0xAA)
				return( Hpi6205_Error(nDSPIndex, HPI6205_ERROR_DSP_PLD) );
			BootLoader_WriteMem32( pao, nDSPIndex, 0x90000000, 0x02);                       // 8713 - LED on
		}
	}
	return(0);
}


static short Hpi6205_TransferData( H620_ADAPTER_OBJ *pao,
				   HW8 *pData,
				   HW32 dwDataSize,
				   int nOperation)		// H620_HIF_SEND_DATA or H620_HIF_GET_DATA
{
  HW32				dwDataTransfered=0;
  //	HW8 *pData =	(HW8 *)phm->u.d.u.Data.dwpbData;
	//	HW16				wTimeOut=8;
	HW16				wError=0;
	HW32				dwTimeOut,dwTemp1,dwTemp2;
	tBusMasteringInterfaceBuffer *interface=pao->pInterfaceBuffer;

	dwDataSize &= ~3L;  // round dwDataSize down to nearest 4 bytes

	// make sure state is IDLE
	dwTimeOut = TIMEOUT;
	dwTemp2=0;
	while( (interface->dwDspAck != H620_HIF_IDLE) && dwTimeOut--	)
	{
		HpiOs_DelayMicroSeconds(1);
	}


	if (interface->dwDspAck != H620_HIF_IDLE)
		return HPI_ERROR_DSP_HARDWARE;

	interface->dwHostCmd = nOperation;

    while(dwDataTransfered < dwDataSize)
    {
		HW32 nThisCopy = dwDataSize - dwDataTransfered;

		if(nThisCopy>HPI6205_SIZEOF_DATA)
			nThisCopy = HPI6205_SIZEOF_DATA;

		if(nOperation==H620_HIF_SEND_DATA)
			memcpy((void *)&interface->u.bData[0],&pData[dwDataTransfered],nThisCopy);

		interface->dwTransferSizeInBytes = nThisCopy;

		// interrupt the DSP
		dwTemp1 = Hpi6205_Abstract_MEMREAD32(pao,pao->dwHDCR );  //read the control register
		dwTemp1 |= (HW32)C6205_HDCR_DSPINT;
		Hpi6205_Abstract_MEMWRITE32(pao,pao->dwHDCR,dwTemp1);
		dwTemp1 &= ~(HW32)C6205_HDCR_DSPINT;
		Hpi6205_Abstract_MEMWRITE32(pao,pao->dwHDCR,dwTemp1);


		// spin waiting on the result
		dwTimeOut = TIMEOUT;
		dwTemp2=0;
		while( (dwTemp2 == 0) && dwTimeOut--)
		{
		    // give 16k bus mastering transfer time to happen
		    //(16k / 132Mbytes/s = 122usec)
		    HpiOs_DelayMicroSeconds(20);
		    dwTemp2 = 	Hpi6205_Abstract_MEMREAD32(pao,pao->dwHSR);
		    dwTemp2 &=  C6205_HSR_INTSRC;
		}
		HPI_PRINT_DEBUG("Spun %ld times for data xfer of %ld\n", TIMEOUT-dwTimeOut, nThisCopy);
		if(dwTemp2 ==  C6205_HSR_INTSRC)
		{
			HPI_PRINT_VERBOSE("HPI6205.C - Interrupt from HIF <data> module OK\n");
			/*
			if(interface->dwDspAck != nOperation) {
			    HPI_PRINT_DEBUG("interface->dwDspAck=%d, expected %d \n",
					    interface->dwDspAck,nOperation);
			}
			*/
		}
// need to handle this differently...
		else {

			wError=HPI_ERROR_DSP_HARDWARE;
		}

		// reset the interrupt from the DSP
		Hpi6205_Abstract_MEMWRITE32(pao,pao->dwHSR,C6205_HSR_INTSRC);

		if(nOperation==H620_HIF_GET_DATA)
			memcpy(&pData[dwDataTransfered],(void *)&interface->u.bData[0],nThisCopy);

		  dwDataTransfered += nThisCopy;
    }
    if(interface->dwDspAck != nOperation /*H620_HIF_DATA_DONE */) {
		HPI_PRINT_DEBUG("interface->dwDspAck=%ld, expected %d\n",
						interface->dwDspAck,nOperation);
		//			wError=HPI_ERROR_DSP_HARDWARE;
    }

	// set interface back to idle
	interface->dwHostCmd = H620_HIF_IDLE;
	// interrupt the DSP again
	dwTemp1 = Hpi6205_Abstract_MEMREAD32(pao,pao->dwHDCR );  //read the control register
	dwTemp1 |= (HW32)C6205_HDCR_DSPINT;
	Hpi6205_Abstract_MEMWRITE32(pao,pao->dwHDCR,dwTemp1);
	dwTemp1 &= ~(HW32)C6205_HDCR_DSPINT;
	Hpi6205_Abstract_MEMWRITE32(pao,pao->dwHDCR,dwTemp1);

	return wError;
}

static unsigned int messageCount=0;
static short Hpi6205_MessageResponseSequence(
	H620_ADAPTER_OBJ *pao,
    HPI_MESSAGE *phm,
    HPI_RESPONSE *phr)
{
    HW32 dwTemp1,dwTemp2,dwTimeOut,dwTimeOut2;
	tBusMasteringInterfaceBuffer *interface=pao->pInterfaceBuffer;
	HW16 wError=0;

	messageCount++;
	/* Assume buffer of type tBusMasteringInterfaceBuffer is allocated "noncacheable" */

	// make sure state is IDLE
	dwTimeOut = TIMEOUT;
	dwTemp2=0;
	while( (interface->dwDspAck != H620_HIF_IDLE) && --dwTimeOut	)
	{
		HpiOs_DelayMicroSeconds(1);
	}
	if(dwTimeOut==0) {
	    HPI_PRINT_DEBUG("Timeout waiting for idle\n");
		return( Hpi6205_Error(0, HPI6205_ERROR_MSG_RESP_IDLE_TIMEOUT) );
	}

	// copy the message in to place
	memcpy((void *)&interface->u.MessageBuffer,phm,sizeof(HPI_MESSAGE));

	// signal we want a response
	interface->dwHostCmd = H620_HIF_GET_RESP;

	// interrupt the DSP
	dwTemp1 = Hpi6205_Abstract_MEMREAD32(pao,pao->dwHDCR );  //read the control register
	dwTemp1 |= (HW32)C6205_HDCR_DSPINT;
	HpiOs_DelayMicroSeconds(1);
	Hpi6205_Abstract_MEMWRITE32(pao,pao->dwHDCR,dwTemp1);
	dwTemp1 &= ~(HW32)C6205_HDCR_DSPINT;
	Hpi6205_Abstract_MEMWRITE32(pao,pao->dwHDCR,dwTemp1);

	// spin waiting on state change (start of msg process)
	dwTimeOut2 = TIMEOUT;
	dwTemp2=0;
	while((interface->dwDspAck != H620_HIF_GET_RESP) && --dwTimeOut2)
	{
		dwTemp2 = Hpi6205_Abstract_MEMREAD32(pao,pao->dwHSR);
		dwTemp2 &=  C6205_HSR_INTSRC;
	}
	if(dwTimeOut2 ==0) {
	    HPI_PRINT_DEBUG("(%u)Timed out waiting for GET_RESP state [%lx]\n",messageCount,interface->dwDspAck);
	} else {
	    HPI_PRINT_VERBOSE("(%u)Transition to GET_RESP after %lu\n",messageCount,TIMEOUT-dwTimeOut2);
	}


	// spin waiting on HIF interrupt flag (end of msg process)
	dwTimeOut = TIMEOUT;
	dwTemp2=0;
	while( (dwTemp2 == 0) && --dwTimeOut)
	{
		dwTemp2 = 	Hpi6205_Abstract_MEMREAD32(pao,pao->dwHSR);
		dwTemp2 &=  C6205_HSR_INTSRC;
		// HpiOs_DelayMicroSeconds(5);
	}
	if(dwTemp2 ==  C6205_HSR_INTSRC)
	{
	    if ((interface->dwDspAck != H620_HIF_GET_RESP)) {
		HPI_PRINT_DEBUG("(%u)interface->dwDspAck(0x%lx) != H620_HIF_GET_RESP, t=%lu\n",messageCount,interface->dwDspAck,TIMEOUT-dwTimeOut);
	    } else {
		HPI_PRINT_VERBOSE("(%u)Int with GET_RESP after %lu\n",messageCount,TIMEOUT-dwTimeOut);
	    }


	}

	// reset the interrupt from the DSP
	Hpi6205_Abstract_MEMWRITE32(pao,pao->dwHSR,C6205_HSR_INTSRC);

	// read the result
	if (dwTimeOut != 0)
		memcpy(phr,(void *)&interface->u.ResponseBuffer,sizeof(HPI_RESPONSE));

	// set interface back to idle
	interface->dwHostCmd = H620_HIF_IDLE;
	// interrupt the DSP again
	dwTemp1 = Hpi6205_Abstract_MEMREAD32(pao,pao->dwHDCR );  //read the control register
	dwTemp1 |= (HW32)C6205_HDCR_DSPINT;
	Hpi6205_Abstract_MEMWRITE32(pao,pao->dwHDCR,dwTemp1);
	dwTemp1 &= ~(HW32)C6205_HDCR_DSPINT;
	Hpi6205_Abstract_MEMWRITE32(pao,pao->dwHDCR,dwTemp1);

// EWB move timeoutcheck to after IDLE command, maybe recover?
	if ((dwTimeOut==0) || (dwTimeOut2 ==0)) {
	    HPI_PRINT_DEBUG("Something timed out!\n");
		return Hpi6205_Error( 0, HPI6205_ERROR_MSG_RESP_TIMEOUT );
	}

	return wError;
}

static short Hpi6205_ControlMessage( H620_ADAPTER_OBJ *pao, HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
	HW16 	nError=0;
	HW32	dwControlType;
	tBusMasteringInterfaceBuffer *interface=pao->pInterfaceBuffer;

	// if the control type in the cache is non-zero then we have cached control information to process
	dwControlType = interface->ControlCache[phm->u.c.wControlIndex].ControlType;
	phr->wSize = 	HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_CONTROL_RES);
	phr->wError = 0;
	switch( dwControlType )
	{
		case HPI_CONTROL_METER:
			if (phm->u.c.wAttribute == HPI_METER_PEAK)
			{
				phr->u.c.anLogValue[0] = interface->ControlCache[phm->u.c.wControlIndex].u.p.anLogPeak[0];
				phr->u.c.anLogValue[1] = interface->ControlCache[phm->u.c.wControlIndex].u.p.anLogPeak[1];
			}
			else if (phm->u.c.wAttribute == HPI_METER_RMS)
			{
				phr->u.c.anLogValue[0] = interface->ControlCache[phm->u.c.wControlIndex].u.p.anLogRMS[0];
				phr->u.c.anLogValue[1] = interface->ControlCache[phm->u.c.wControlIndex].u.p.anLogRMS[1];
			}
			else
				dwControlType = 0;	// signal that message was not cached
			break;
		case HPI_CONTROL_VOLUME:
			if (phm->u.c.wAttribute == HPI_VOLUME_GAIN)
			{
				phr->u.c.anLogValue[0] = interface->ControlCache[phm->u.c.wControlIndex].u.v.anLog[0];
				phr->u.c.anLogValue[1] = interface->ControlCache[phm->u.c.wControlIndex].u.v.anLog[1];
			}
			else
				dwControlType = 0;	// signal that message was not cached
			break;
		case HPI_CONTROL_MULTIPLEXER:
			if (phm->u.c.wAttribute == HPI_MULTIPLEXER_SOURCE)
			{
				phr->u.c.dwParam1 = interface->ControlCache[phm->u.c.wControlIndex].u.x.wSourceNodeType;
				phr->u.c.dwParam2 = interface->ControlCache[phm->u.c.wControlIndex].u.x.wSourceNodeIndex;
			}
			else
				dwControlType = 0;	// signal that message was not cached
			break;
		case HPI_CONTROL_LEVEL:
			if (phm->u.c.wAttribute == HPI_CONTROL_LEVEL)
			{
				phr->u.c.anLogValue[0] = interface->ControlCache[phm->u.c.wControlIndex].u.l.anLog[0];
				phr->u.c.anLogValue[1] = interface->ControlCache[phm->u.c.wControlIndex].u.l.anLog[1];
			}
			else
				dwControlType = 0;	// signal that message was not cached
			break;
		case HPI_CONTROL_TUNER:
			if (phm->u.c.wAttribute == HPI_TUNER_FREQ)
				phr->u.c.dwParam1 = interface->ControlCache[phm->u.c.wControlIndex].u.t.dwFreqInkHz;
			else if (phm->u.c.wAttribute == HPI_TUNER_BAND)
				phr->u.c.dwParam1 = interface->ControlCache[phm->u.c.wControlIndex].u.t.wBand;
			else if ((phm->u.c.wAttribute == HPI_TUNER_LEVEL) && ( phm->u.c.dwParam1==HPI_TUNER_LEVEL_AVERAGE))
				phr->u.c.dwParam1 = interface->ControlCache[phm->u.c.wControlIndex].u.t.wLevel;
			else
				dwControlType = 0;	// signal that message was not cached
			break;
		default:
				dwControlType = 0;	// signal that message was not cached
			break;
	}
	if(dwControlType==0)	// value of 0 indicates that control is not cached.
		nError = Hpi6205_MessageResponseSequence( pao, phm,phr );

	return nError;
}

void  Hpi6205_Message( H620_ADAPTER_OBJ *pao, HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{

	HW16	nError;

	if(pao->wHasControlCache && (phm->wFunction==HPI_CONTROL_GET_STATE))
		nError = Hpi6205_ControlMessage( pao, phm, phr);
	else
		nError = Hpi6205_MessageResponseSequence( pao, phm,phr );

	 // maybe an error response
	 if( nError )
	 {
		  phr->wError = nError;		// something failed in the HPI/DSP interface
		  phr->wSize = HPI_RESPONSE_FIXED_SIZE;  // just the header of the response is valid
		  return;
	 }
	 if  (phr->wError != 0)          // something failed in the DSP
		  return;

	 switch (phm->wFunction) {
	 case HPI_OSTREAM_WRITE:
	 case HPI_ISTREAM_ANC_WRITE:
	   // nError = Hpi6205_TransferData( pao, phm, phr,H620_HIF_SEND_DATA);
	   nError = Hpi6205_TransferData( pao,
									  (void *)phm->u.d.u.Data.dwpbData ,phm->u.d.u.Data.dwDataSize,
									  H620_HIF_SEND_DATA);
	   break;

	 case HPI_ISTREAM_READ:
	 case HPI_OSTREAM_ANC_READ:
	   nError = Hpi6205_TransferData( pao,
									  (void *) phm->u.d.u.Data.dwpbData ,phm->u.d.u.Data.dwDataSize,
									  H620_HIF_GET_DATA);
	   break;

	 case HPI_CONTROL_SET_STATE:
	 	if (phm->wObject==HPI_OBJ_CONTROLEX && phm->u.cx.wAttribute==HPI_COBRANET_SET_DATA)
	        nError = Hpi6205_TransferData( pao,
										   (void *)phm->u.cx.u.cobranet_bigdata.dwpbData ,phm->u.cx.u.cobranet_bigdata.dwByteCount,
									  H620_HIF_SEND_DATA);
        break;

	 case HPI_CONTROL_GET_STATE:
	 	if (phm->wObject==HPI_OBJ_CONTROLEX && phm->u.cx.wAttribute==HPI_COBRANET_GET_DATA)
	        nError = Hpi6205_TransferData( pao,
										   (void *)phm->u.cx.u.cobranet_bigdata.dwpbData ,phr->u.cx.u.cobranet_data.dwByteCount,
									  H620_HIF_GET_DATA);
	    break;
	 }
	 phr->wError = nError;
	 return;
}

HW16 Hpi6205_Error( int nDspIndex, int nError )
{
	return( (HW16)(HPI6205_ERROR_BASE + nDspIndex*100 + nError) );
}


///////////////////////////////////////////////////////////////////////////
