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


 Hardware Programming Interface (HPI) for AudioScience ASI6200 series adapters.
 These PCI bus adapters are based on the TI C6711 DSP.

 Exported functions:
 void HPI_6000( HPI_MESSAGE *phm, HPI_RESPONSE *phr )

 #defines
 USE_ZLIB	enable use of Z compressed DSP code files
 HIDE_PCI_ASSERTS to show the PCI asserts
 PROFILE_DSP2 get profile data from DSP2 if present (instead of DSP 1)

(C) Copyright AudioScience Inc. 1998-2003
*******************************************************************************/

#include "hpi.h"
#include "hpidebug.h"
#include "hpi6000.h"
#include "hpidspcd.h"
#include "hpicmn.h"

#if defined(HPI_OS_DOS) || defined (HPI_OS_WIN16)
#include "hpipci.h"
#endif

////////////////////////////////////////////////////////////////////////////
// local defines

#define HIDE_PCI_ASSERTS
#define PROFILE_DSP2

// for PCI2040 i/f chip
// HPI CSR registers
// word offsets from CSR base
// use when io addresses defined as HW32 *

#define INTERRUPT_EVENT_SET 	0
#define INTERRUPT_EVENT_CLEAR 	1
#define INTERRUPT_MASK_SET 	2
#define INTERRUPT_MASK_CLEAR	3
#define HPI_ERROR_REPORT	4
#define HPI_RESET		5
#define HPI_DATA_WIDTH		6

// HPI registers, spaced 8K bytes = 2K words apart
#define DSP_SPACING		0x800

#define CONTROL			0x0000
#define ADDRESS			0x0200
#define DATA_AUTOINC		0x0400
#define DATA			0x0600

#define TIMEOUT 500000

typedef struct
{
    __iomem HW32 *	prHPIControl;
    __iomem HW32 *	prHPIAddress;
    __iomem HW32 *	prHPIData;
    __iomem HW32 *	prHPIDataAutoInc;
    char	cDspRev;  //A, B
    HW32	dwControlCacheAddressOnDSP;
    HW32	dwControlCacheLengthOnDSP;
	HPI_ADAPTER_OBJ *paParentAdapter;
}
DSP_OBJ;

typedef struct
{

	 __iomem HW32 *	dw2040_HPICSR;
	 __iomem HW32 *	dw2040_HPIDSP;

	 HW16	wNumDsp;
	 DSP_OBJ ado[4];


    HW32	dwMessageBufferAddressOnDSP;
    HW32	dwResponseBufferAddressOnDSP;
    HW32  dwPCI2040HPIErrorCount;

    HW16	wNumErrors;	//counts number of consecutive communications errors reported from DSP
    HW16	wDspCrashed; // when '1' DSP has crashed/died/OTL

    HW16 wHasControlCache;
    tHPIControlCacheSingle aControlCache[HPI_NMIXER_CONTROLS];
}
HPI_HW_OBJ;

#if defined(HPI_OS_WDM) && !defined(HPI_WDM_MONOLITHIC) && defined(DEBUG)
NTSTATUS ASIHPIW_LogBootError(char *pMsg);
#endif

////////////////////////////////////////////////////////////////////////////
static HW16 Hpi6000_DspBlockWrite32( HPI_ADAPTER_OBJ *pao, HW16 wDspIndex, HW32 dwHpiAddress, HW32 * dwSource, HW32 dwCount);
static HW16 Hpi6000_DspBlockRead32( HPI_ADAPTER_OBJ *pao, HW16 wDspIndex, HW32 dwHpiAddress, HW32 * dwDest, HW32 dwCount);

static short Hpi6000_AdapterBootLoadDsp( HPI_ADAPTER_OBJ *pao, HW32 *pdwOsErrorCode );
static short Hpi6000_Check_PCI2040_ErrorFlag(HPI_ADAPTER_OBJ *pao, HW16 nReadOrWrite);
#define H6READ 1
#define H6WRITE 0

static short Hpi6000_UpdateControlCache( HPI_ADAPTER_OBJ *pao, HPI_MESSAGE *phm );
static short Hpi6000_MessageResponseSequence(HPI_ADAPTER_OBJ *pao,HPI_MESSAGE *phm,HPI_RESPONSE *phr);

static void  HW_Message( HPI_ADAPTER_OBJ *pao, HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static short Hpi6000_WaitDspAck(HPI_ADAPTER_OBJ *pao,HW16 wDspIndex, HW32 dwAckValue);
static short Hpi6000_SendHostCommand(HPI_ADAPTER_OBJ *pao,HW16 wDspIndex, HW32 dwHostCmd);
static void Hpi6000_SendDspInterrupt(DSP_OBJ *pdo);
static short Hpi6000_SendData( HPI_ADAPTER_OBJ *pao,HPI_MESSAGE *phm,HPI_RESPONSE *phr);
static short Hpi6000_GetData( HPI_ADAPTER_OBJ *pao, HPI_MESSAGE *phm, HPI_RESPONSE *phr);

static void HpiWriteWord( DSP_OBJ *pdo, HW32 dwAddress, HW32 dwData);
static HW32 HpiReadWord( DSP_OBJ *pdo, HW32 dwAddress );
static void HpiWriteBlock( DSP_OBJ *pdo, HW32 dwAddress, HW32 *pdwData, HW32 dwLength);
static void HpiReadBlock( DSP_OBJ *pdo, HW32 dwAddress, HW32 *pdwData, HW32 dwLength );

////////////////////////////////////////////////////////////////////////////
// local prototypes
static void SubSysCreateAdapter(HPI_ADAPTERS_LIST *adaptersList, HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void SubSysDeleteAdapter(HPI_ADAPTERS_LIST *adaptersList, HPI_MESSAGE *phm, HPI_RESPONSE *phr);

static void AdapterGetAsserts(HPI_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);

static short CreateAdapterObj( HPI_ADAPTER_OBJ *pao, HW32 *pdwOsErrorCode);

////////////////////////////////////////////////////////////////////////////
// external globals
// extern HW16 gwHpiLastError;


////////////////////////////////////////////////////////////////////////////
// local globals
static HW32 gadwHpiSpecificError[4];

static HPI_ADAPTERS_LIST adapters;
static HW16 gwPciReadAsserts=0;		// used to count PCI2040 errors
static HW16 gwPciWriteAsserts=0;		// used to count PCI2040 errors

static void SubSysMessage( HPI_ADAPTERS_LIST *adaptersList, HPI_MESSAGE *phm, HPI_RESPONSE *phr ) {

	switch(phm->wFunction) {
		case HPI_SUBSYS_OPEN:
		case HPI_SUBSYS_CLOSE:
		case HPI_SUBSYS_DRIVER_UNLOAD:
			phr->wError=0;
			break;
		case HPI_SUBSYS_DRIVER_LOAD:
			WipeAdapterList( adaptersList );
			phr->wError=0;
			break;
		case HPI_SUBSYS_GET_INFO:
			SubSysGetAdapters( adaptersList, phr );
			break;
		case HPI_SUBSYS_FIND_ADAPTERS:
			phr->wError = HPI_ERROR_INVALID_FUNC;
			break;
		case HPI_SUBSYS_CREATE_ADAPTER:
			SubSysCreateAdapter( adaptersList, phm, phr );
			break;
		case HPI_SUBSYS_DELETE_ADAPTER:
			SubSysDeleteAdapter( adaptersList, phm, phr );
			break;
		default:
			phr->wError = HPI_ERROR_INVALID_FUNC;
			break;
	}
}

static void ControlMessage( HPI_ADAPTER_OBJ *pao, HPI_MESSAGE *phm, HPI_RESPONSE *phr ) {

	switch(phm->wFunction) {
		case HPI_CONTROL_GET_STATE:
			if (pao->wHasControlCache)
			{
				HW16 err;
				err = Hpi6000_UpdateControlCache( pao, phm );

				if (err) {
					phr->wError = err;
					break;
				}

				if (CheckControlCache( &((HPI_HW_OBJ*)pao->priv)->aControlCache[phm->u.c.wControlIndex], phm, phr ))
					break;
			}
			HW_Message( pao, phm, phr );
			break;
		case HPI_CONTROL_GET_INFO:
			HW_Message( pao, phm, phr);
			break;
		case HPI_CONTROL_SET_STATE:
			HW_Message( pao, phm, phr);
			SyncControlCache(&((HPI_HW_OBJ*)pao->priv)->aControlCache[phm->u.c.wControlIndex], phm, phr);
			break;
		default:
			phr->wError = HPI_ERROR_INVALID_FUNC;
			break;
	}
}

static void AdapterMessage( HPI_ADAPTER_OBJ *pao, HPI_MESSAGE *phm, HPI_RESPONSE *phr ) {

	switch(phm->wFunction) {
		case HPI_ADAPTER_GET_INFO:
			HW_Message( pao, phm, phr);
			break;
		case HPI_ADAPTER_GET_ASSERT:
			AdapterGetAsserts( pao, phm, phr );
			break;
		case HPI_ADAPTER_OPEN:
		case HPI_ADAPTER_CLOSE:
		case HPI_ADAPTER_TEST_ASSERT:
		case HPI_ADAPTER_SELFTEST:
		case HPI_ADAPTER_GET_MODE:
		case HPI_ADAPTER_SET_MODE:
		case HPI_ADAPTER_FIND_OBJECT:
		case HPI_ADAPTER_GET_PROPERTY:
		case HPI_ADAPTER_SET_PROPERTY:
		case HPI_ADAPTER_ENUM_PROPERTY:
			HW_Message( pao, phm, phr);
			break;
		default:
			phr->wError = HPI_ERROR_INVALID_FUNC;
			break;
	}
}

static void OStreamMessage( HPI_ADAPTER_OBJ *pao, HPI_MESSAGE *phm, HPI_RESPONSE *phr ) {

	switch(phm->wFunction) {
		case HPI_OSTREAM_HOSTBUFFER_ALLOC:
		case HPI_OSTREAM_HOSTBUFFER_FREE:
			// Don't let these messages go to the HW function because they're called
			// without allocating the spinlock.  For the HPI6000 adapters the HW would
			// return HPI_ERROR_INVALID_FUNC anyway.
			phr->wError = HPI_ERROR_INVALID_FUNC;
			break;
		default:
			HW_Message( pao, phm, phr);
           	return;
	}
}

static void IStreamMessage( HPI_ADAPTER_OBJ *pao, HPI_MESSAGE *phm, HPI_RESPONSE *phr ) {

	switch(phm->wFunction) {
		case HPI_ISTREAM_HOSTBUFFER_ALLOC:
		case HPI_ISTREAM_HOSTBUFFER_FREE:
			// Don't let these messages go to the HW function because they're called
			// without allocating the spinlock.  For the HPI6000 adapters the HW would
			// return HPI_ERROR_INVALID_FUNC anyway.
			phr->wError = HPI_ERROR_INVALID_FUNC;
			break;
		default:
			HW_Message( pao, phm, phr);
           	return;
	}
}

////////////////////////////////////////////////////////////////////////////
// HPI_6000()
// Entry point from HPIMAN
// All calls to the HPI start here

void HPI_6000( HPI_MESSAGE *phm, HPI_RESPONSE *phr )
{
	HPI_ADAPTER_OBJ *pao = NULL;

	// subsytem messages get executed by every HPI.
	// All other messages are ignored unless the adapter index matches
	// an adapter in the HPI
	HPI_DEBUG_LOG2(DEBUG,"O %d,F %d\n", phm->wObject, phm->wFunction);

	//phr->wError=0;      // SGT JUN-15-01 - so that modules can't overwrite errors from FindAdapter

	// if Dsp has crashed then do not try and communicated with it any more
	if( phm->wObject != HPI_OBJ_SUBSYSTEM ) {
		pao = FindAdapter( &adapters, phm->wAdapterIndex );
        	if ( !pao ) {
				HPI_DEBUG_LOG2(DEBUG," %d,%d refused, for another HPI?\n", phm->wObject, phm->wFunction);
			return; // message probably meant for another HPI module
		}

		if( pao->wDspCrashed ) {
			HPI_InitResponse(phr, phm->wObject, phm->wFunction, HPI_ERROR_DSP_HARDWARE );
			HPI_DEBUG_LOG2(DEBUG," %d,%d dsp crashed.\n", phm->wObject, phm->wFunction);
			return;
        	}
	}

	//Init default response ( sets the size field in the response structure )
	if( phm->wFunction != HPI_SUBSYS_CREATE_ADAPTER )
		HPI_InitResponse(phr, phm->wObject, phm->wFunction, HPI_ERROR_PROCESSING_MESSAGE );

	switch(phm->wType) {
		case HPI_TYPE_MESSAGE:
		switch(phm->wObject) {
			case HPI_OBJ_SUBSYSTEM:
				SubSysMessage( &adapters, phm, phr );
				break;

			case HPI_OBJ_ADAPTER:
				phr->wSize=HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_ADAPTER_RES);
				AdapterMessage( pao, phm, phr );
				break;

			case HPI_OBJ_CONTROL:
				ControlMessage( pao, phm, phr );
				break;

			case HPI_OBJ_OSTREAM:
				OStreamMessage( pao, phm, phr );
				break;

			case HPI_OBJ_ISTREAM:
				IStreamMessage( pao, phm, phr );
				break;

			default:
				HW_Message( pao, phm, phr );
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

// create an adapter object and initialise it based on resource information
// passed in in the message
// **** NOTE - you cannot use this function AND the FindAdapters function at the
// same time, the application must use only one of them to get the adapters ******

static void SubSysCreateAdapter(HPI_ADAPTERS_LIST *adaptersList, HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
    HPI_ADAPTER_OBJ ao;	// create temp adapter obj, because we don't know what index yet
	HW32 dwOsErrorCode;
    short nError=0;
    HW32 dwDspIndex=0;

    HPI_DEBUG_LOG0(VERBOSE, "SubSysCreateAdapter\n" );

    memset( &ao, 0, sizeof(HPI_ADAPTER_OBJ) );

    // this HPI only creates adapters for TI/PCI2040 based devices
	if (phm->u.s.Resource.wBusType != HPI_BUS_PCI)
		return;
	if ( phm->u.s.Resource.r.Pci->wVendorId != HPI_PCI_VENDOR_ID_TI )
		return;
	if ( phm->u.s.Resource.r.Pci->wDeviceId != HPI_ADAPTER_PCI2040 )
		return;

    ao.priv = HpiOs_MemAlloc( sizeof(HPI_HW_OBJ) );
    memset( ao.priv, 0, sizeof(HPI_HW_OBJ) );
    // create the adapter object based on the resource information
    //? memcpy(&ao.Pci,&phm->u.s.Resource.r.Pci,sizeof(ao.Pci));
	ao.Pci = *phm->u.s.Resource.r.Pci;

    nError = CreateAdapterObj( &ao, &dwOsErrorCode );
    if(nError)
    {
		phr->u.s.dwData = dwOsErrorCode;	// OS error, if any, is in dwOsErrorCode
        HpiOs_MemFree( ao.priv );
        phr->wError = nError;
        return;
    }

    // add to adapter list - but don't allow two adapters of same number!
    if( phr->u.s.awAdapterList[ ao.wIndex ] != 0)
    {
        HpiOs_MemFree( ao.priv );
        phr->wError = HPI_DUPLICATE_ADAPTER_NUMBER;
        return;
    }
    //? memcpy( &adaptersList->adapter[ ao.wIndex ], &ao, sizeof(HPI_ADAPTER_OBJ));
	adaptersList->adapter[ ao.wIndex ] = ao;

    // need to update paParentAdapter
    for(dwDspIndex=0; dwDspIndex<4; dwDspIndex++)
    {
		(*(HPI_HW_OBJ*)adaptersList->adapter[ ao.wIndex ].priv).ado[dwDspIndex].paParentAdapter = &adaptersList->adapter[ ao.wIndex ];
    }
    HpiOs_Dsplock_Init( &adaptersList->adapter[ ao.wIndex ] );

    adaptersList->gwNumAdapters++;         // inc the number of adapters known by this HPI
    phr->u.s.awAdapterList[ ao.wIndex ] = ao.wAdapterType;
    phr->u.s.wAdapterIndex = ao.wIndex;
    phr->u.s.wNumAdapters++;    // add the number of adapters recognised by this HPI to the system total
    phr->wError = 0;            // the function completed OK;
}

// delete an adapter - required by WDM driver


static void SubSysDeleteAdapter(HPI_ADAPTERS_LIST *adaptersList, HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
    HPI_ADAPTER_OBJ *pao=NULL;

    pao = FindAdapter( adaptersList, phm->wAdapterIndex );
    if(!pao)
        return; // message probably meant for another HPI module

#ifdef HPI_OS_DOS
	// disable PCI interface so other apps can't access 6701
	HpiPci_WriteConfig( &pao->Pci, HPIPCI_CCMR, 0 );
#endif

    adaptersList->gwNumAdapters--;
    HpiOs_MemFree( pao->priv );
    memset( pao, 0, sizeof(HPI_ADAPTER_OBJ) );

    phr->wError = 0;
}


// this routine is called from SubSysFindAdapter and SubSysCreateAdapter
static short CreateAdapterObj( HPI_ADAPTER_OBJ *pao, HW32 *pdwOsErrorCode )
{
    short	nBootError=0;
    HW32	dwDspIndex=0;
    HPI_HW_OBJ *phw = (HPI_HW_OBJ*)pao->priv;

    // init error reporting
    phw->wNumErrors = 0;
    pao->wDspCrashed = 0;

#ifdef HPI_OS_DOS ////////////////////////////// DOS ONLY, ASI6000 ONLY, NO ASI8800 SUPPORT!
        // for real mode DOS only - move the PCI addresses to 0xD0000-0xDFFFF (64K)
        // by changing the base address register in PCI config space
        // The PCI2040 has two blocks, BAR0=4K and BAR1=32K
        // Move the 4K block to 0xD0000 and the 32K block to 0xD8000
        HpiPci_WriteConfig( pao->Pci, HPIPCI_CBMA,     0xD0000L );
        HpiPci_WriteConfig( pao->Pci, HPIPCI_CBMA+0x4, 0xD8000L );
        if( HpiPci_FindDeviceEx( pao->Pci, nIndex, HPI_PCI_VENDOR_ID_TI, HPI_ADAPTER_PCI2040,HPI_PCI_VENDOR_ID_AUDIOSCIENCE ) )
        {
            return HPI_ERROR_DOS_MEMORY_ALLOC;
        }
        // get subsys dev id, to figure out if 6100 or 6200
        {
            HW32 dwSubDev=0;

            // fill in the same stuff as the PnP subsystem
            HpiPci_ReadConfig( &pao->Pci, HPIPCI_CSUB, &dwSubDev );
            pao->Pci.wVendorId = HPI_PCI_VENDOR_ID_TI;
            pao->Pci.wDeviceId = HPI_ADAPTER_PCI2040;
            pao->Pci.wSubSysVendorId = dwSubDev&0xffff;
            pao->Pci.wSubSysDeviceId = dwSubDev>>16;
        }
#endif

        // under WIN16, get the subsys device ID, if present
        // find out if we have a 8801 present
        // because it uses different DSP code than the 6200 series
#ifdef HPI_OS_WIN16
        {
            HW32 dwSubDev=0;

            // fill in the same stuff as the PnP subsystem
				HpiPci_ReadConfig( &pao->Pci, HPIPCI_CSUB, &dwSubDev );

            pao->Pci.wVendorId = HPI_PCI_VENDOR_ID_TI;
            pao->Pci.wDeviceId = HPI_ADAPTER_PCI2040;
            pao->Pci.wSubSysVendorId = dwSubDev&0xffff;
            pao->Pci.wSubSysDeviceId = dwSubDev>>16;

            switch(pao->Pci.wSubSysDeviceId)
            {
                //case 0x0000:	//prototypes.
            case 0x5100: // ASI5100 series revA
            case 0x5110: // ASI5100 series revB and higher
            case 0x6100:
            case 0x6200:
            case 0x8800:
                break;	// valid values of Subdevice Id
            default:
                return HPI6000_ERROR_UNHANDLED_SUBSYS_ID;
            }
        }
#endif

    // The PCI2040 has the following address map
    // BAR0 - 4K = HPI control and status registers on PCI2040  (HPI CSR)
    // BAR1 - 32K = HPI registers on DSP
    // convert the physical address in a 16bit protected mode address (Selector:Offset)
    phw->dw2040_HPICSR = pao->Pci.apMemBase[0];
    phw->dw2040_HPIDSP = pao->Pci.apMemBase[1];
HPI_DEBUG_LOG2(VERBOSE,"csr %p, dsp %p\n",phw->dw2040_HPICSR,phw->dw2040_HPIDSP);

    // set addresses for the 4 possible DSP HPI interfaces
    for(dwDspIndex=0; dwDspIndex<4; dwDspIndex++)
    {
	phw->ado[dwDspIndex].prHPIControl =
		phw->dw2040_HPIDSP + (CONTROL + DSP_SPACING*dwDspIndex);

        phw->ado[dwDspIndex].prHPIAddress =
		phw->dw2040_HPIDSP + (ADDRESS + DSP_SPACING*dwDspIndex);
	phw->ado[dwDspIndex].prHPIData =
		phw->dw2040_HPIDSP + (DATA    + DSP_SPACING*dwDspIndex);

	phw->ado[dwDspIndex].prHPIDataAutoInc =
		phw->dw2040_HPIDSP + (DATA_AUTOINC + DSP_SPACING*dwDspIndex);

HPI_DEBUG_LOG4(VERBOSE,"ctl %p, adr %p, dat %p, dat++ %p\n",phw->ado[dwDspIndex].prHPIControl,
phw->ado[dwDspIndex].prHPIAddress,phw->ado[dwDspIndex].prHPIData,phw->ado[dwDspIndex].prHPIDataAutoInc);

	phw->ado[dwDspIndex].paParentAdapter = pao;
    }

    phw->dwPCI2040HPIErrorCount = 0;		// set error count to 0
    pao->wHasControlCache = 0;


    // Set the default number of DSPs on this card
    // This is (conditionally) adjusted after bootloading of the first DSP in the bootload section.
    phw->wNumDsp = 1;

    if (0 != (nBootError = Hpi6000_AdapterBootLoadDsp(pao,pdwOsErrorCode))){
        return(nBootError); //error
    }
    HPI_DEBUG_LOG0(INFO,"Bootload DSP OK\n" );

    phw->dwMessageBufferAddressOnDSP=0L;
    phw->dwResponseBufferAddressOnDSP=0L;

    // get info about the adapter by asking the adapter
    // send a HPI_ADAPTER_GET_INFO message
    {
        HPI_MESSAGE     hM;
        HPI_RESPONSE    hR0;		// response from DSP 0
        HPI_RESPONSE    hR1;		// response from DSP 1
        HW16 wError=0;

        //HpiOs_Printf("GetInfo-"); //*************** debug
        HPI_DEBUG_LOG0(VERBOSE, "send ADAPTER_GET_INFO\n" );
        memset(&hM, 0, sizeof(HPI_MESSAGE));
        hM.wType = HPI_TYPE_MESSAGE;
        hM.wSize = sizeof(HPI_MESSAGE);
        hM.wObject = HPI_OBJ_ADAPTER;
        hM.wFunction =  HPI_ADAPTER_GET_INFO;
        hM.wAdapterIndex = 0;
	hM.wDspIndex = 0;
        memset(&hR0, 0, sizeof(HPI_RESPONSE));
        memset(&hR1, 0, sizeof(HPI_RESPONSE));
        hR0.wSize = sizeof(HPI_RESPONSE);
        hR1.wSize = sizeof(HPI_RESPONSE);

        wError = Hpi6000_MessageResponseSequence( pao, &hM,&hR0 );
        if(hR0.wError)
        {
            HPI_DEBUG_LOG1(DEBUG, "message error %d\n",hR0.wError );
            return(hR0.wError); //error
        }
        if(phw->wNumDsp==2)
        {
		hM.wDspIndex=1;
            wError = Hpi6000_MessageResponseSequence( pao, &hM,&hR1 );
            if(wError)
                return wError;
        }
        pao->wAdapterType = hR0.u.a.wAdapterType;
        pao->wIndex = hR0.u.a.wAdapterIndex;
    }

    memset(&phw->aControlCache[0],0,sizeof(tHPIControlCacheSingle)*HPI_NMIXER_CONTROLS);
   	// Read the control cache length to figure out if it is turned on.....
	if( HpiReadWord(&phw->ado[0],HPI_HIF_BASE + HPI_HIF_OFS_CONTROL_CACHE_SIZE_IN_BYTES))
        pao->wHasControlCache = 1;
    else
        pao->wHasControlCache = 0;




    HPI_DEBUG_LOG0(VERBOSE, "Get adapter info OK\n" );
    pao->wOpen=0;	// upon creation the adapter is closed
    return(0);  //sucess!
}

////////////////////////////////////////////////////////////////////////////
// ADAPTER

static void AdapterGetAsserts(HPI_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
#ifndef HIDE_PCI_ASSERTS
    // if we have PCI2040 asserts then collect them
    if((gwPciReadAsserts>0) || (gwPciWriteAsserts>0))
    {
        phr->u.a.dwSerialNumber = gwPciReadAsserts*100 + gwPciWriteAsserts;
        phr->u.a.wAdapterIndex=1; // assert count
        phr->u.a.wAdapterType= -1; // "dsp index"
        strcpy(phr->u.a.szAdapterAssert, "PCI2040 error");
        gwPciReadAsserts=0;
        gwPciWriteAsserts=0;
        phr->wError=0;
    }
    else
#endif

        HW_Message( pao, phm,phr);    //get DSP asserts
    return;
}


////////////////////////////////////////////////////////////////////////////
// LOW-LEVEL
///////////////////////////////////////////////////////////////////////////

static short Hpi6000_AdapterBootLoadDsp( HPI_ADAPTER_OBJ *pao, HW32 *pdwOsErrorCode)
{
    HPI_HW_OBJ *phw = (HPI_HW_OBJ*)pao->priv;
    short nError;
    HW32	dwTimeout;
    HW32	dwRead=0;
    HW32	i=0;
    HW32    dwData=0;
    HW32    j=0;
    HW32 	dwTestAddr=0x80000000;
    HW32 	dwTestData=0x00000001;
    HW32 	dw2040Reset =0;
    HW32 	dwDspIndex = 0;
    HW32 	dwEndian=0;
    HW32	dwAdapterInfo=0;
    HW32 volatile dwDelay=0;

    DSP_CODE DspCode;
    HW16 nBootLoadFamily=0;

    /* NOTE - don't use wAdapterType in this routine. It is not setup yet ! */

    switch(pao->Pci.wSubSysDeviceId)
    {
    case 0x5100:
    case 0x5110: // ASI5100 revB or higher with C6711D
    case 0x6100:
    case 0x6200:
        nBootLoadFamily = Load6200;
        break;
    case 0x8800:
        nBootLoadFamily = Load8800;
        break;
    default:
        return(HPI6000_ERROR_UNHANDLED_SUBSYS_ID);
    }

    {
        ///////////////////////////////////////////////////////////
        // reset all DSPs, indicate two DSPs are present
        // set RST3-=1 to disconnect HAD8 to set DSP in little endian mode
        dwEndian = 0;
        dw2040Reset = 0x0003000F;
        HPIOS_MEMWRITE32(phw->dw2040_HPICSR+HPI_RESET, dw2040Reset );

        // read back register to make sure PCI2040 chip is functioning
        // note that bits 4..15 are read-only and so should always return zero, even though we wrote 1 to them
        for(i=0; i<1000; i++)
            dwDelay = HPIOS_MEMREAD32(phw->dw2040_HPICSR+HPI_RESET );  //delay
        if(dwDelay != dw2040Reset)
        {
            gadwHpiSpecificError[0] = 0;
            gadwHpiSpecificError[1] = dw2040Reset;
            gadwHpiSpecificError[2] = dwDelay;
            HPI_DEBUG_LOG2(ERROR,"INIT_PCI2040 %x %x\n",dw2040Reset,dwDelay);
            return(HPI6000_ERROR_INIT_PCI2040);
        }


        HPIOS_MEMWRITE32(phw->dw2040_HPICSR + HPI_DATA_WIDTH, 0x00000003 );	// Indicate that DSP#0,1 is a C6X
        HPIOS_MEMWRITE32(phw->dw2040_HPICSR + INTERRUPT_MASK_SET, 0x60000000 );	// set Bit30 and 29 - which will prevent Target aborts from being issued upon HPI or GP error

        ////////////////////////////////////////////////////////////////////////////
        // determine what rev DSP we have by reading back HAD8.  Its pulled high for a revA DSP
        dw2040Reset = dw2040Reset & (~(dwEndian<<3)); 		// isolate DSP HAD8 line from PCI2040 so that Little endian can be set by pullup
        HPIOS_MEMWRITE32(phw->dw2040_HPICSR + HPI_RESET, dw2040Reset );

        //*************************************************************** sgt test  ** must delete
        /*
        dw2040Reset = dw2040Reset & (~0x00000008);      // set HAD8 back to PCI2040, now that DSP has been set to little endian mode
        HPIOS_MEMWRITE32(phw->dw2040_HPICSR + HPI_RESET, dw2040Reset );
        //delay to allow DSP to get going
        for(i=0; i<100; i++)
        dwDelay = HPIOS_MEMREAD32(phw->dw2040_HPICSR+HPI_RESET );  // *********** delay

        dwData = HPIOS_MEMREAD32(phw->ado[0].prHPIAddress);
        if(dwData & 0x0100)
        */
        //{
        //	phw->ado[0].cDspRev = 'A';	// revA      // **** SGT - remove revA DSP support jan-28-2002
        //	phw->ado[1].cDspRev = 'A';	// revA
        //}
        //else
        {
            phw->ado[0].cDspRev = 'B';	// revB
            phw->ado[1].cDspRev = 'B';	// revB
        }

        ////////////////////////////////////////////////////////////////////////////////
        // Take both DSPs out of reset while setting HAD8 to set the correct Endian
        dw2040Reset = dw2040Reset & (~0x00000001); 		// start DSP 0
        HPIOS_MEMWRITE32(phw->dw2040_HPICSR + HPI_RESET, dw2040Reset );
        dw2040Reset = dw2040Reset & (~0x00000002); 		// start DSP 1
        HPIOS_MEMWRITE32(phw->dw2040_HPICSR + HPI_RESET, dw2040Reset );

        dw2040Reset = dw2040Reset & (~0x00000008);      // set HAD8 back to PCI2040, now that DSP has been set to little endian mode
        HPIOS_MEMWRITE32(phw->dw2040_HPICSR + HPI_RESET, dw2040Reset );
        //delay to allow DSP to get going
        for(i=0; i<100; i++)
            dwDelay = HPIOS_MEMREAD32(phw->dw2040_HPICSR+HPI_RESET );  //*********** delay


        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // loop through all DSPs, downloading DSP code
        for(dwDspIndex=0; dwDspIndex<phw->wNumDsp; dwDspIndex++)
        {
            DSP_OBJ *pdo = &phw->ado[dwDspIndex];

            ///////////////////////////////////////////////////////////
            // configure DSP so that we download code into the SRAM
            HPIOS_MEMWRITE32(pdo->prHPIControl, 0x00010001 );	// set control reg for little endian, HWOB=1

            // test access to the HPI address register (HPIA)
            dwTestData=0x00000001;
            for(j=0; j<32; j++)
            {
                HPIOS_MEMWRITE32(pdo->prHPIAddress, dwTestData);
                dwData = HPIOS_MEMREAD32(pdo->prHPIAddress);
                if(dwData != dwTestData)
                {
                    gadwHpiSpecificError[0] = 0;
                    gadwHpiSpecificError[1] = dwTestData;
                    gadwHpiSpecificError[2] = dwData;
                    gadwHpiSpecificError[3] = dwDspIndex;
                    HPI_DEBUG_LOG3(ERROR,"INIT_DSPHPI %x %x %x\n",dwTestData,dwData,dwDspIndex);
                    return(HPI6000_ERROR_INIT_DSPHPI);       //error
                }
                dwTestData = dwTestData << 1;
            }

			#if 1
            // if C6713 the setup PLL to generate 225MHz from 25MHz.
			// TFE - Since the PLLDIV1 read is sometimes wrong, even on a C6713, we're going to do this unconditionally
            //if( HpiReadWord( pdo,0x01B7C118 ) == 0x8000 )	// PLLDIV1 should have a value of 8000 after reset
            {
                // ** C6713 datasheet says we cannot program PLL from HPI, and indeed if we try to set the
                // PLL multiply from the HPI, the PLL does not seem to lock, so we enable the PLL and use the default
                // multiply of x 7
                HpiWriteWord( pdo, 0x01B7C100, 0x0000 );  // bypass PLL
                for(i=0; i<100; i++) dwDelay = HPIOS_MEMREAD32(phw->dw2040_HPICSR+HPI_RESET );  //*********** delay

                //HpiWriteWord( pdo, 0x01B7C110, 0x1001 );  // PLL multiply=x9, 25x9=225MHz
                //** use default of  x7 **
                HpiWriteWord( pdo, 0x01B7C120, 0x8002 );  // EMIF = 225/3=75MHz
                HpiWriteWord( pdo, 0x01B7C11C, 0x8001 );  // peri = 225/2
                HpiWriteWord( pdo, 0x01B7C118, 0x8000 );  // cpu  = 225/1

                for(i=0; i<2000; i++) dwDelay = HPIOS_MEMREAD32(phw->dw2040_HPICSR+HPI_RESET );  //*********** ~200us delay
                HpiWriteWord( pdo, 0x01B7C100, 0x0001 );  // PLL not bypassed
                for(i=0; i<2000; i++) dwDelay = HPIOS_MEMREAD32(phw->dw2040_HPICSR+HPI_RESET );  //*********** ~200us delay
            }
            #endif

            // test r/w to internal DSP memory
            // C6711 has L2 cache mapped to 0x0 when reset
            //
            // ** revA - must use auto inc HPI access, because of silicon bug 1.3.2 **
            // ** revB - because of bug 3.0.1 last HPI read (before HPI address issued) must be non-autoin **
            //
            // test each bit in the 32bit word
            #if 1
            for(i=0; i<100; i++)
            {
                dwTestAddr=0x00000000;
                dwTestData=0x00000001;
                for(j=0; j<32; j++)
                {
                    HpiWriteWord( pdo, dwTestAddr+i, dwTestData);   //write the data to internal DSP mem
                    dwData = HpiReadWord( pdo, dwTestAddr+i);      //read the data back
                    if(dwData != dwTestData)
                    {
                        gadwHpiSpecificError[0] = dwTestAddr+i;
                        gadwHpiSpecificError[1] = dwTestData;
                        gadwHpiSpecificError[2] = dwData;
                        gadwHpiSpecificError[3] = dwDspIndex;
                        return(HPI6000_ERROR_INIT_DSPINTMEM);	// error
                    }
                    dwTestData = dwTestData << 1;
                }
            }
			#endif

            // memory map of ASI6200
            // 00000000-0000FFFF	16Kx32 internal program
            // 01800000-019FFFFF	Internal peripheral
            // 80000000-807FFFFF	CE0	2Mx32 SDRAM running @ 100MHz
            // 90000000-9000FFFF	CE1	Async peripherals:

            // EMIF config
            //------------
            // Global EMIF control
            // 0 -
            // 1 -
            // 2 -
            // 3 CLK2EN=1	CLKOUT2 enabled
            // 4 CLK1EN=0	CLKOUT1 disabled
            // 5 EKEN=1	<------------------------------ !! C6713 specific, enables ECLKOUT
            // 6 -
            // 7 NOHOLD=1	external HOLD disabled
            // 8 HOLDA=0    HOLDA output is low
            // 9 HOLD=0     HOLD input is low
            // 10 ARDY=1    ARDY input is high
            // 11 BUSREQ=0   BUSREQ output is low
            // 12,13 Reserved = 1
            HpiWriteWord( pdo, 0x01800000, 0x34A8 );  // global control (orig=C6711 only = 0x3488)

            // EMIF CE0 setup - 2Mx32 Sync DRAM
            // 31..28	Wr setup
            // 27..22   Wr strobe
            // 21..20   Wr hold
            // 19..16   Rd setup
            // 15..14   -
            // 13..8    Rd strobe
            // 7..4		MTYPE	0011		Sync DRAM 32bits
            // 3        Wr hold MSB
            // 2..0     Rd hold
            HpiWriteWord( pdo, 0x01800008, 0x00000030);  // CE0

            // EMIF SDRAM Extension
            // 31-21 	0
            // 20  		WR2RD = 0
            // 19-18 	WR2DEAC=1
            // 17  		WR2WR=0
            // 16-15 	R2WDQM=2
            // 14-12 	RD2WR=4
            // 11-10 	RD2DEAC=1
            // 9        RD2RD= 1
            // 8-7      THZP = 10b
            // 6-5      TWR  = 2-1 = 01b (tWR = 10ns )
            // 4        TRRD = 0b = 2 ECLK (tRRD = 14ns)
            // 3-1      TRAS = 5-1 = 100b (Tras=42ns = 5 ECLK)
            // 1 		CAS latency = 3 ECLK (for Micron 2M32-7 operating at 100Mhz)
            //
            // JAN-21-2002 - was 0x001BDF29
            //
            //HpiWriteWord( pdo, 0x01800020, 0x00054729 );
            HpiWriteWord( pdo, 0x01800020, 0x001BDF29 );   // need to use this else DSP code crashes?

            // EMIF SDRAM control - set up for a 2Mx32 SDRAM (512x32x4 bank)
            // 31       -       -
            // 30       SDBSZ   1   	4 bank
            // 29..28   SDRSZ   00      11 row address pins
            // 27..26   SDCSZ   01      8 column address pins
            // 25   	RFEN 	1       refersh enabled
            // 24   	INIT	1       init SDRAM
            // 23..20	TRCD    0001
            // 19..16	TRP     0001
            // 15..12	TRC     0110
            // 11..0	-       -
            //
            // JAN-21-2002 - was 0x47117000
            //
            //HpiWriteWord( pdo, 0x01800018, 0x45116000);	// EMIF SDRAM control
            HpiWriteWord( pdo, 0x01800018, 0x47117000);	//  need to use this else DSP code crashes?

            // EMIF SDRAM Refresh Timing
            HpiWriteWord( pdo, 0x0180001C,0x00000410);	// EMIF SDRAM timing  (orig = 0x410, emulator = 0x61a)

            // EMIF CE1 setup - Async peripherals
            // @100MHz bus speed, each cycle is 10ns,
            // 31..28	Wr setup  = 1
            // 27..22   Wr strobe = 3           30ns
            // 21..20   Wr hold = 1
            // 19..16   Rd setup =1
            // 15..14   Ta = 2
            // 13..8    Rd strobe = 3           30ns
            // 7..4		MTYPE	0010		Async 32bits
            // 3        Wr hold MSB =0
            // 2..0     Rd hold = 1
            {
                //HW32 dwCE1 = (0L<<28) | (3L<<22) | (1L<<20) | (1L<<16) | (2L<<14) | (3L<<8) | (2L<<4) | 0L;
                HW32 dwCE1 = (1L<<28) | (3L<<22) | (1L<<20) | (1L<<16) | (2L<<14) | (3L<<8) | (2L<<4) | 1L;
                HpiWriteWord( pdo, 0x01800004, dwCE1);  // CE1 = 0001 0000 1001 0001 1100 0010 0010 0000
            }

            // delay a little to allow SDRAM to "get going"
            //delay to allow DSP to get going


            for(i=0; i<1000; i++)
                dwDelay = HPIOS_MEMREAD32((*(HPI_HW_OBJ*)pao->priv).dw2040_HPICSR+HPI_RESET );

#if 1
            // test access to SDRAM
            {
                //short j=0;
                //HW32 dwTestAddr=0x80000000;
                //HW32 dwTestData=0x00000001;
                // test each bit in the 32bit word
                for(j=0; j<32; j++)
                {
                    HpiWriteWord( pdo, dwTestAddr, dwTestData);
                    dwData = HpiReadWord( pdo, dwTestAddr );
                    if(dwData != dwTestData)
                    {
                        gadwHpiSpecificError[0] = dwTestAddr;
                        gadwHpiSpecificError[1] = dwTestData;
                        gadwHpiSpecificError[2] = dwData;
                        gadwHpiSpecificError[3] = dwDspIndex;
                        return(HPI6000_ERROR_INIT_SDRAM1);     // error
                    }
                    dwTestData = dwTestData << 1;
                }
                // test every Nth address in the DRAM
#define DRAM_SIZE_WORDS 0x200000 //2Mx32
	#define DRAM_INC 1024
                dwTestAddr=0x80000000;
                dwTestData=0x0;
                for(i=0; i<DRAM_SIZE_WORDS; i=i+DRAM_INC)
                {
                    HpiWriteWord( pdo, dwTestAddr+i, dwTestData);
                    dwTestData++;
                }
                dwTestAddr=0x80000000;
                dwTestData=0x0;
                for(i=0; i<DRAM_SIZE_WORDS; i=i+DRAM_INC)
                {
                    dwData = HpiReadWord( pdo, dwTestAddr+i);
                    if(dwData != dwTestData)
                    {
                        gadwHpiSpecificError[0] = dwTestAddr+i;
                        gadwHpiSpecificError[1] = dwTestData;
                        gadwHpiSpecificError[2] = dwData;
                        gadwHpiSpecificError[3] = dwDspIndex;
                        return(HPI6000_ERROR_INIT_SDRAM2);     // error
                    }
                    dwTestData++;
                }

            }
#endif

            ///////////////////////////////////////////////////////////
            // write the DSP code down into the DSPs memory
            //HpiDspCode_Open(nBootLoadFamily,&DspCode,pdwOsErrorCode);
#if defined DSPCODE_FIRMWARE
	    DspCode.psDev = pao->Pci.pOsData;
#endif

            if ((nError=HpiDspCode_Open(nBootLoadFamily,&DspCode,pdwOsErrorCode))!= 0)
                return( nError);

            while (1)
            {
                HW32 dwLength;
                HW32 dwAddress;
                HW32 dwType;
                HW32 *pdwCode;

                if ((nError=HpiDspCode_ReadWord(&DspCode,&dwLength))!= 0)
                    break;
                if (dwLength == 0xFFFFFFFF)
                    break; // end of code

#ifdef DSPCODE_ARRAY
                // check for end of array with continuation to another one
                if (dwLength == 0xFFFFFFFEL )
                {
                    DspCode.nArrayNum++;
                    DspCode.dwOffset = 0;
                    if ((nError = HpiDspCode_ReadWord(&DspCode,&dwLength))!= 0)
                        break;
                }
#endif

                if ((nError=HpiDspCode_ReadWord(&DspCode,&dwAddress))!= 0)
                    break;
                if ((nError=HpiDspCode_ReadWord(&DspCode,&dwType))!= 0)
                    break;
                if ((nError=HpiDspCode_ReadBlock(dwLength,&DspCode,&pdwCode))!= 0)
                    break;
                if ((nError=Hpi6000_DspBlockWrite32( pao, (HW16)dwDspIndex, dwAddress, pdwCode, dwLength))
                        != 0)
                    break;
			}

			if (nError) {
				HpiDspCode_Close(&DspCode);
				return(nError);
			}

            ///////////////////////////////////////////////////////////
            // verify that code was written correctly
            // this time through, assume no errors in DSP code file/array

			HpiDspCode_Rewind(&DspCode);
            while (1)
            {
                HW32 dwLength;
                HW32 dwAddress;
                HW32 dwType;
                HW32 *pdwCode;

                HpiDspCode_ReadWord(&DspCode,&dwLength);
                if (dwLength == 0xFFFFFFFF)
                    break; // end of code

#ifdef DSPCODE_ARRAY
                // check for end of array with continuation to another one
                if (dwLength == 0xFFFFFFFEL )
                {
                    DspCode.nArrayNum++;
                    DspCode.dwOffset = 0;
                    HpiDspCode_ReadWord(&DspCode,&dwLength);
                }
#endif

                HpiDspCode_ReadWord(&DspCode,&dwAddress);
                HpiDspCode_ReadWord(&DspCode,&dwType);
                HpiDspCode_ReadBlock(dwLength,&DspCode,&pdwCode);

                for(i=0; i<dwLength; i++)
                {
                    dwData = HpiReadWord( pdo, dwAddress);
                    if(dwData != *pdwCode)
                    {
                        nError = HPI6000_ERROR_INIT_VERIFY;
                        gadwHpiSpecificError[0] = dwAddress;
                        gadwHpiSpecificError[1] = *pdwCode;
                        gadwHpiSpecificError[2] = dwData;
                        gadwHpiSpecificError[3] = dwDspIndex;
                        break;
                    }
                    pdwCode++;
                    dwAddress += 4;
                }
                if (nError)
                    break;
            }
            HpiDspCode_Close(&DspCode);
            if (nError)
                return (nError);

                        // zero out the hostmailbox (this assumes DSP consts are loaded using .cinit)
            {
                HW32 dwAddress=HPI_HIF_BASE + HPI_HIF_OFS_HOSTCMD;
                for(i=0; i<4; i++ )
                {
                    HpiWriteWord( pdo, dwAddress, 0);
                    dwAddress+=4;
                }
            }
            // write the DSP number into the hostmailbox structure before starting the DSP
            HpiWriteWord( pdo, HPI_HIF_BASE + HPI_HIF_OFS_DSP_NUMBER, dwDspIndex);

            // write the DSP adapter Info into the hostmailbox structure before starting the DSP
            if(dwDspIndex>0)
            {
                HpiWriteWord( pdo, HPI_HIF_BASE + HPI_HIF_OFS_ADAPTER_INFO, dwAdapterInfo);
            }

            // step 3. Start code by sending interrupt
            HPIOS_MEMWRITE32(pdo->prHPIControl, 0x00030003 );
            for(i=0; i<10000; i++)
                dwDelay = HPIOS_MEMREAD32(phw->dw2040_HPICSR+HPI_RESET );  // delay

            // wait for a non-zero value in hostcmd - indicating initialization is complete
            //
            dwTimeout = 2000000;   	// Init could take a while if DSP checks SDRAM memory
            // Was 200000. Increased to 2000000 for ASI8801 so we don't get 938 errors.
            while(dwTimeout)
            {
                // read the ack mailbox
                do
                {
                    dwRead = HpiReadWord( pdo, HPI_HIF_BASE + HPI_HIF_OFS_HOSTCMD);
                }
                while( --dwTimeout && Hpi6000_Check_PCI2040_ErrorFlag(pao, H6READ));
                if(dwRead)
                    break;
                //	The following is a workaround for bug #94: Bluescreen on install and subsequent
                //  boots on a DELL PowerEdge 600SC PC with 1.8GHz P4 and ServerWorks chipset.
                //	Without this delay the system locks up with a bluescreen (NOT GPF or pagefault).
                else
                    HpiOs_DelayMicroSeconds( 1000 );
            }
            if(dwTimeout==0)
            {
                gadwHpiSpecificError[0] = dwDspIndex;
                return(HPI6000_ERROR_INIT_NOACK);
            }

            // read the DSP adapter Info from the hostmailbox structure after starting the DSP
            if(dwDspIndex==0)
            {
                //HW32 dwTestData=0;
                HW32 dwMask=0;

                dwAdapterInfo = HpiReadWord( pdo, HPI_HIF_BASE + HPI_HIF_OFS_ADAPTER_INFO);
                if( (HPI_HIF_ADAPTER_INFO_EXTRACT_ADAPTER(dwAdapterInfo)&HPI_ADAPTER_FAMILY_MASK)==HPI_ADAPTER_ASI6200 )
                {
                    // we have a 2 DSP adapter
                    phw->wNumDsp = 2;		//?????????????????? do all 6200 cards have this many DSPs? - YES - SGT
                }
                // test that the PLD is programmed and we can read/write 24bits
#define PLD_BASE_ADDRESS 0x90000000L  //for ASI6100/6200/8800

                switch(nBootLoadFamily)
                {
                case Load6200:
                    dwMask = 0xFFFFFF00L; 	// ASI6100/6200 has 24bit path to FPGA
                    if( ( pao->Pci.wSubSysDeviceId & 0xFF00 ) == 0x5100)
                        dwMask = 0x00000000L;	// ASI5100 uses AX6 code, but has no PLD r/w register to test
                    break;
                case Load8800:
                    dwMask = 0xFFFF0000L;    // ASI8800 has 16bit path to FPGA
                    break;
                }
                dwTestData = 0xAAAAAA00L & dwMask;
                HpiWriteWord( pdo, PLD_BASE_ADDRESS+4L, dwTestData); 	// write to Debug register which is 24bits wide (D31-D8)
                dwRead = HpiReadWord( pdo, PLD_BASE_ADDRESS+4L) & dwMask;
                if(dwRead != dwTestData)
                {
                    gadwHpiSpecificError[0] = 0;
                    gadwHpiSpecificError[1] = dwTestData;
                    gadwHpiSpecificError[2] = dwRead;
                    return(HPI6000_ERROR_INIT_PLDTEST1);
                }
                dwTestData = 0x55555500L & dwMask;
                HpiWriteWord( pdo, PLD_BASE_ADDRESS+4L, dwTestData); 	// write to Debug register which is 24bits wide (D31-D8)
                dwRead = HpiReadWord( pdo, PLD_BASE_ADDRESS+4L) & dwMask;
                if(dwRead != dwTestData)
                {
                    gadwHpiSpecificError[0] = 0;
                    gadwHpiSpecificError[1] = dwTestData;
                    gadwHpiSpecificError[2] = dwRead;
                    return(HPI6000_ERROR_INIT_PLDTEST2);
                }
            }
        } // for wNumDSP
        /////////////////////////////////////////////////////////////

    }
	HPI_DEBUG_LOG0(INFO,"HPI6000 bootload dsp OK\n");
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////
#define PCI_TIMEOUT 100

static int HpiSetAddress( DSP_OBJ *pdo, HW32 dwAddress)
{
	HW32 dwTimeout = PCI_TIMEOUT;

	do {
		HPIOS_MEMWRITE32(pdo->prHPIAddress, dwAddress);
	}
    while( Hpi6000_Check_PCI2040_ErrorFlag(pdo->paParentAdapter, H6WRITE) &&  --dwTimeout);

	if(dwTimeout)
		return 0;
	return 1;
}

// write one word to the HPI port
static void HpiWriteWord( DSP_OBJ *pdo, HW32 dwAddress, HW32 dwData)
{
	if(HpiSetAddress(pdo,dwAddress))
		return;
    HPIOS_MEMWRITE32(pdo->prHPIData, dwData);
}

// read one word from the HPI port
static HW32 HpiReadWord( DSP_OBJ *pdo, HW32 dwAddress )
{
    HW32 dwData=0;

	if(HpiSetAddress(pdo,dwAddress))
		return 0; //? No way to return error

    // take care of errata in revB DSP (2.0.1)
    dwData = HPIOS_MEMREAD32(pdo->prHPIData);
    return(dwData);
}

// write a block of 32bit words to the DSP HPI port using auto-inc mode
static  void HpiWriteBlock( DSP_OBJ *pdo, HW32 dwAddress, HW32 *pdwData, HW32 dwLength)
{
    if(dwLength==0)
        return;

	if(HpiSetAddress(pdo,dwAddress))
		return;

#if ((defined HPIOS_MEMWRITEBLK32) && (defined HPI_OS_LINUX))
    {
        HW16 wLength = dwLength-1;
        HPIOS_MEMWRITEBLK32(pdwData , pdo->prHPIDataAutoInc, wLength );

        // take care of errata in revB DSP (2.0.1)
        HPIOS_MEMWRITE32(pdo->prHPIData,*(pdwData+dwLength-1));  // must end with non auto-inc
    }
#else
	{
		int i;
		for(i=0; i<dwLength-1; i++)
		HPIOS_MEMWRITE32(pdo->prHPIDataAutoInc, *pdwData++);
		// take care of errata in revB DSP (2.0.1)
		HPIOS_MEMWRITE32(pdo->prHPIData, *pdwData);  // take care of errata in revB DSP
	}
#endif
}

/** read a block of 32bit words from the DSP HPI port using auto-inc mode
 */
static void HpiReadBlock( DSP_OBJ *pdo, HW32 dwAddress, HW32 *pdwData, HW32 dwLength )
{
    if(dwLength==0)
        return;

	if(HpiSetAddress(pdo,dwAddress))
		return;

#if ((defined HPIOS_MEMREADBLK32) && (defined HPI_OS_LINUX))
    {
        HW16 wLength = dwLength-1;
        HPIOS_MEMREADBLK32(pdo->prHPIDataAutoInc, pdwData , wLength );

        // take care of errata in revB DSP (2.0.1)
        *(pdwData+dwLength-1) = HPIOS_MEMREAD32(pdo->prHPIData);  // must end with non auto-inc
    }
#else
    {
        int i;
        for(i=0; i<dwLength-1; i++)
            *pdwData++ = HPIOS_MEMREAD32(pdo->prHPIDataAutoInc);

        // take care of errata in revB DSP (2.0.1)
        *pdwData = HPIOS_MEMREAD32(pdo->prHPIData);  // must end with non auto-inc
    }
#endif
}


static HW16 Hpi6000_DspBlockWrite32( HPI_ADAPTER_OBJ *pao, HW16 wDspIndex, HW32 dwHpiAddress, HW32 * dwSource, HW32 dwCount)
{
    //#define USE_BLOCKS - can't use this at the moment because of revB bug 3.0.1
    DSP_OBJ *pdo = &(*(HPI_HW_OBJ*)pao->priv).ado[wDspIndex];
    HW32	dwTimeOut = PCI_TIMEOUT;
    int	nC6711BurstSize=128;
	HW32 	dwLocalHpiAddress=dwHpiAddress;
	int 	wLocalCount=dwCount;
	int 	wXferSize;
	HW32	*pdwData = dwSource;

	while(wLocalCount)
	{
		if(wLocalCount>nC6711BurstSize)
			wXferSize=nC6711BurstSize;
		else
			wXferSize=wLocalCount;

		dwTimeOut = PCI_TIMEOUT;
		do
		{
			HpiWriteBlock( pdo, dwLocalHpiAddress, pdwData, wXferSize);
		}
		while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6WRITE) && --dwTimeOut);
		if(!dwTimeOut)
			break;
		pdwData += wXferSize;
		dwLocalHpiAddress+=sizeof(HW32)*wXferSize;
		wLocalCount-=wXferSize;
	}

    if(dwTimeOut)
        return 0;  // no error
    else
        return 1;  // error
}

static HW16 Hpi6000_DspBlockRead32( HPI_ADAPTER_OBJ *pao, HW16 wDspIndex, HW32 dwHpiAddress, HW32 * dwDest, HW32 dwCount)
{
    DSP_OBJ *pdo = &(*(HPI_HW_OBJ*)pao->priv).ado[wDspIndex];
    HW32	dwTimeOut = PCI_TIMEOUT;
    int	nC6711BurstSize=16;
	HW32 dwLocalHpiAddress=dwHpiAddress;
	int wLocalCount=dwCount;
	int wXferSize;
	HW32 *pdwData = dwDest;
	HW32 dwLoopCount = 0;

    while(wLocalCount)
    {
        if(wLocalCount>nC6711BurstSize)
            wXferSize=nC6711BurstSize;
        else
            wXferSize=wLocalCount;

		dwTimeOut = PCI_TIMEOUT;
		do
		{
			HpiReadBlock( pdo, dwLocalHpiAddress, pdwData, wXferSize);
		}
		while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6READ) && --dwTimeOut);
		if(!dwTimeOut)
		{
			break;
		}

        pdwData += wXferSize;
        dwLocalHpiAddress+=sizeof(HW32)*wXferSize;
        wLocalCount-=wXferSize;
		dwLoopCount++;
    }

    if(dwTimeOut)
        return 0;  // no error
    else
        return 1;  // error
}

#ifdef HPI_OS_WDM
char verify_error_buffer[128];
#endif

/////////////////////////////////////////////////////////////////////////////////

static short Hpi6000_MessageResponseSequence(
    HPI_ADAPTER_OBJ *pao,
    HPI_MESSAGE *phm,
    HPI_RESPONSE *phr)
{
	 HW16 wDspIndex = phm->wDspIndex;
    HPI_HW_OBJ *phw = (HPI_HW_OBJ*)pao->priv;
    DSP_OBJ *pdo = &phw->ado[wDspIndex];
    HW32 dwTimeout;
    HW16 wAck;
    HW32 dwAddress;
    HW32 dwLength;
    HW32 *pData;

    // does the DSP we are referencing exist?
    if(wDspIndex >= phw->wNumDsp )
        return HPI6000_ERROR_MSG_INVALID_DSP_INDEX;

    wAck = Hpi6000_WaitDspAck(pao, wDspIndex, HPI_HIF_IDLE);
    if(wAck&HPI_HIF_ERROR_MASK)
    {
        phw->wNumErrors++;
        if(phw->wNumErrors == 10)
		{
            pao->wDspCrashed = 1;
		}
        return HPI6000_ERROR_MSG_RESP_IDLE_TIMEOUT;
    }
    phw->wNumErrors = 0;

    // send the message

    // get the address and size
    if(phw->dwMessageBufferAddressOnDSP==0)
    {
        dwTimeout = TIMEOUT;
        do
        {
            dwAddress = HpiReadWord( pdo, HPI_HIF_BASE + HPI_HIF_OFS_MSG_BUFFER_ADR);
            phw->dwMessageBufferAddressOnDSP=dwAddress;
        }
        while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6READ) && --dwTimeout);
        if(!dwTimeout)
            return HPI6000_ERROR_MSG_GET_ADR;
    }
    else
        dwAddress = phw->dwMessageBufferAddressOnDSP;

    //    dwLength = sizeof(HPI_MESSAGE);
    dwLength = phm->wSize;

    // send it
    pData=(HW32 *)phm;
    if( Hpi6000_DspBlockWrite32( pao, wDspIndex, dwAddress, pData, (HW16)dwLength/4) )
        return HPI6000_ERROR_MSG_RESP_BLOCKWRITE32;

    if( Hpi6000_SendHostCommand(pao, wDspIndex, HPI_HIF_GET_RESP) )
		return HPI6000_ERROR_MSG_RESP_GETRESPCMD;
    Hpi6000_SendDspInterrupt(pdo);				// ***** FOR DSP/BIOS version only

    wAck = Hpi6000_WaitDspAck(pao, wDspIndex, HPI_HIF_GET_RESP);
    if(wAck&HPI_HIF_ERROR_MASK)
        return HPI6000_ERROR_MSG_RESP_GET_RESP_ACK;

    // get the address and size
    if(phw->dwResponseBufferAddressOnDSP==0)
    {
        dwTimeout = TIMEOUT;
        do
        {
            dwAddress = HpiReadWord( pdo, HPI_HIF_BASE + HPI_HIF_OFS_RESP_BUFFER_ADR);
        }
        while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6READ) && --dwTimeout);
        phw->dwResponseBufferAddressOnDSP = dwAddress;
        if(!dwTimeout)
            return HPI6000_ERROR_RESP_GET_ADR;
    }
    else
        dwAddress = phw->dwResponseBufferAddressOnDSP;

    // read the length of the response back from the DSP
    dwTimeout = TIMEOUT;
    do
    {
        dwLength = HpiReadWord( pdo, HPI_HIF_BASE + HPI_HIF_OFS_LENGTH);
    }
    while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6READ) && --dwTimeout);
    if(!dwTimeout)
        dwLength = sizeof(HPI_RESPONSE);

    // get it
    pData=(HW32 *)phr;
    if( Hpi6000_DspBlockRead32( pao, wDspIndex, dwAddress, pData, (HW16)dwLength/4) )
        return HPI6000_ERROR_MSG_RESP_BLOCKREAD32;

    // set i/f back to idle
    if( Hpi6000_SendHostCommand(pao, wDspIndex,HPI_HIF_IDLE) )
		return HPI6000_ERROR_MSG_RESP_IDLECMD;
    Hpi6000_SendDspInterrupt(pdo);

    return 0; // no error
}

// have to set up the below defines to match stuff in the MAP file

#define MSG_ADDRESS (HPI_HIF_BASE+0x18)
#define MSG_LENGTH 11
#define RESP_ADDRESS (HPI_HIF_BASE+0x44)
#define RESP_LENGTH 16
#define QUEUE_START  (HPI_HIF_BASE+0x88)
#define QUEUE_SIZE 0x8000

static short Hpi6000_SendData_CheckAdr(HW32 dwAddress, HW32 dwLengthInDwords)
{
    //#define CHECKING   // comment this line in to enable checking
#ifdef CHECKING
    if(dwAddress < (HW32)MSG_ADDRESS)
        return 0;
    if(dwAddress > (HW32)(QUEUE_START+QUEUE_SIZE))
        return 0;
    if( (dwAddress + (dwLengthInDwords<<2)) > (HW32)(QUEUE_START+QUEUE_SIZE))
        return 0;
#endif

    return 1;
}

static short Hpi6000_SendData( HPI_ADAPTER_OBJ *pao,
                               HPI_MESSAGE *phm,
                               HPI_RESPONSE *phr)
{
	 HW16 wDspIndex = phm->wDspIndex;
    DSP_OBJ *pdo = &(*(HPI_HW_OBJ*)pao->priv).ado[wDspIndex];
    HW32				dwDataSent=0;
    HW16				wAck;
    HW32				dwLength,dwAddress;
    HW32 HUGE *pData =	(HW32 HUGE *)phm->u.d.u.Data.pbData;
    HW16				wTimeOut=8;

    while( (dwDataSent < (phm->u.d.u.Data.dwDataSize & ~3L)) && --wTimeOut )  // round dwDataSize down to nearest 4 bytes
    {
        wAck = Hpi6000_WaitDspAck(pao, wDspIndex,HPI_HIF_IDLE);
        if(wAck&HPI_HIF_ERROR_MASK)
            return HPI6000_ERROR_SEND_DATA_IDLE_TIMEOUT;

        if( Hpi6000_SendHostCommand(pao, wDspIndex,HPI_HIF_SEND_DATA) )
			return HPI6000_ERROR_SEND_DATA_CMD;

        Hpi6000_SendDspInterrupt(pdo);

        wAck = Hpi6000_WaitDspAck(pao, wDspIndex,HPI_HIF_SEND_DATA);

        if(wAck&HPI_HIF_ERROR_MASK)
            return HPI6000_ERROR_SEND_DATA_ACK;

        do
        {
            // get the address and size
            dwAddress = HpiReadWord( pdo, HPI_HIF_BASE + HPI_HIF_OFS_ADDRESS);
            dwLength = HpiReadWord( pdo, HPI_HIF_BASE + HPI_HIF_OFS_ADDRESS + 4);  // DSP returns number of DWORDS
        }
        while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6READ));

        if(!Hpi6000_SendData_CheckAdr(dwAddress, dwLength))
            return HPI6000_ERROR_SEND_DATA_ADR;

        // send the data
        // break data into 512 DWORD blocks (2K bytes) and send using block write
        // 2Kbytes is the max as this is the memory window given to the HPI data
        // register by the PCI2040

        {
            HW32 dwLen = dwLength;
            HW32 dwBlkLen = 512;
            while(dwLen)
            {
                if(dwLen<dwBlkLen)
                    dwBlkLen=dwLen;
                if( Hpi6000_DspBlockWrite32( pao, wDspIndex, dwAddress, pData, dwBlkLen) )
					return HPI6000_ERROR_SEND_DATA_WRITE;
                dwAddress += dwBlkLen*4;
                pData += dwBlkLen;
                dwLen -= dwBlkLen;
            }
        }

        if( Hpi6000_SendHostCommand(pao, wDspIndex,HPI_HIF_IDLE) )
			return HPI6000_ERROR_SEND_DATA_IDLECMD;

        Hpi6000_SendDspInterrupt(pdo);

        dwDataSent += dwLength*4;
    }
    if(!wTimeOut)
        return HPI6000_ERROR_SEND_DATA_TIMEOUT;
    return 0;
}

static short Hpi6000_GetData( HPI_ADAPTER_OBJ *pao, HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
	 HW16 wDspIndex = phm->wDspIndex;
    DSP_OBJ *pdo = &(*(HPI_HW_OBJ*)pao->priv).ado[wDspIndex];
    HW32	dwDataGot=0;
    HW16	wAck;
    HW32	dwLength,dwAddress;
    HW32  HUGE *pData =	(HW32 HUGE *)phm->u.d.u.Data.pbData;

    while( dwDataGot < (phm->u.d.u.Data.dwDataSize & ~3L) )  // round dwDataSize down to nearest 4 bytes
    {
        wAck = Hpi6000_WaitDspAck(pao, wDspIndex,HPI_HIF_IDLE);
        if(wAck&HPI_HIF_ERROR_MASK)
            return HPI6000_ERROR_GET_DATA_IDLE_TIMEOUT;

        if( Hpi6000_SendHostCommand(pao, wDspIndex,HPI_HIF_GET_DATA) )
			return HPI6000_ERROR_GET_DATA_CMD;
        Hpi6000_SendDspInterrupt(pdo);

        wAck = Hpi6000_WaitDspAck(pao, wDspIndex,HPI_HIF_GET_DATA);

        if(wAck&HPI_HIF_ERROR_MASK)
            return HPI6000_ERROR_GET_DATA_ACK;

        // get the address and size
        do
        {
            dwAddress = HpiReadWord( pdo, HPI_HIF_BASE + HPI_HIF_OFS_ADDRESS);
            dwLength = HpiReadWord( pdo, HPI_HIF_BASE + HPI_HIF_OFS_LENGTH);
        }
        while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6READ));

        // read the data
        {
            HW32 dwLen = dwLength;
            HW32 dwBlkLen = 512;
            while(dwLen)
            {
                if(dwLen<dwBlkLen)
                    dwBlkLen=dwLen;
                if( Hpi6000_DspBlockRead32( pao, wDspIndex, dwAddress, pData, dwBlkLen) )
					return HPI6000_ERROR_GET_DATA_READ;
                dwAddress += dwBlkLen*4;
                pData += dwBlkLen;
                dwLen -= dwBlkLen;
            }
        }

        if( Hpi6000_SendHostCommand(pao, wDspIndex,HPI_HIF_IDLE) )
			return HPI6000_ERROR_GET_DATA_IDLECMD;
        Hpi6000_SendDspInterrupt(pdo);

        dwDataGot += dwLength*4;
    }
    return 0;
}

static void Hpi6000_SendDspInterrupt(DSP_OBJ *pdo)
{
    HPIOS_MEMWRITE32(pdo->prHPIControl, 0x00030003 );	// DSPINT
}

static short Hpi6000_SendHostCommand(HPI_ADAPTER_OBJ *pao, HW16 wDspIndex, HW32 dwHostCmd)
{
    DSP_OBJ *pdo = &(*(HPI_HW_OBJ*)pao->priv).ado[wDspIndex];
    HW32 dwTimeout=TIMEOUT;
    //volatile HW32 dwAddr;     // SGT commented out because not used???
    //volatile HW32 dwControl;

    // set command
    do
    {
        HpiWriteWord( pdo, HPI_HIF_BASE + HPI_HIF_OFS_HOSTCMD, dwHostCmd);
		HpiSetAddress( pdo, HPI_HIF_BASE + HPI_HIF_OFS_HOSTCMD);    // flush the FIFO
    }
    while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6WRITE) && --dwTimeout);
    //dwAddr = HPIOS_MEMREAD32(pdo->prHPIAddress);
    //dwControl = HPIOS_MEMREAD32(pdo->prHPIControl);

    // reset the interrupt bit
    HPIOS_MEMWRITE32(pdo->prHPIControl, 0x00040004);


    if(dwTimeout)
        return 0;	// no error
    else
    {
        return 1;	// error
    }
}

#ifdef ENABLE_HPI6000_BACKDOOR_READ
// This is a backdoor method of reading the HPI error count on a particular
// PCI2040. This fn should be declared in say WHPI.C and called directly.
// It is a backdoor into this module and should only be used for test purposes.
long Hpi6000_BackDoor_Read_PCI2040_ErrorFlagCount(HW16 wAdapter)
{
    if( wAdapter < MAX_ADAPTERS)
        return gao60[wAdapter].dwPCI2040HPIErrorCount;
    else
        return 0;
}
#endif

// if the PCI2040 has recorded an HPI timeout, reset the error and return 1
static short Hpi6000_Check_PCI2040_ErrorFlag(HPI_ADAPTER_OBJ *pao, HW16 nReadOrWrite)
{
    HW32 dwHPIError;

    HPI_HW_OBJ *phw = (HPI_HW_OBJ*)pao->priv;

    dwHPIError = HPIOS_MEMREAD32(phw->dw2040_HPICSR+HPI_ERROR_REPORT);		// read the error bits from the PCI2040
    if(dwHPIError)
    {
        HPIOS_MEMWRITE32(phw->dw2040_HPICSR+HPI_ERROR_REPORT,0L);	// reset the error flag
        phw->dwPCI2040HPIErrorCount++;
        if(nReadOrWrite == 1)
            gwPciReadAsserts++;		// *********************************************************************** inc global
        else
            gwPciWriteAsserts++;
        return 1;
    }
    else
        return 0;
}

static short Hpi6000_WaitDspAck(HPI_ADAPTER_OBJ *pao, HW16 wDspIndex, HW32 dwAckValue)
{
    DSP_OBJ *pdo = &(*(HPI_HW_OBJ*)pao->priv).ado[wDspIndex];
    HW32 dwAck=0L;
    HW32 dwTimeout;
    HW32 dwHPIC=0L;

    // wait for host interrupt to signal ack is ready
    dwTimeout = TIMEOUT;
    while(--dwTimeout)
    {
        dwHPIC =	HPIOS_MEMREAD32(pdo->prHPIControl);
        if(dwHPIC & 0x04)	// 0x04 = HINT from DSP
            break;
    }
    if(dwTimeout==0)
        return HPI_HIF_ERROR_MASK;

    // wait for dwAckValue
    dwTimeout = TIMEOUT;
    while(--dwTimeout)
    {
        // read the ack mailbox
        dwAck = HpiReadWord( pdo, HPI_HIF_BASE + HPI_HIF_OFS_DSPACK);
        if(dwAck==dwAckValue)
            break;
        if( (dwAck&HPI_HIF_ERROR_MASK) & !Hpi6000_Check_PCI2040_ErrorFlag(pao, H6READ))
            break;
        //for(i=0;i<1000;i++)
        //	dwPause=i+1;
    }
    if(dwAck&HPI_HIF_ERROR_MASK)
        dwAck = HPI_HIF_ERROR_MASK;    // indicates bad read from DSP - typically 0xffffff is read for some reason

    if(dwTimeout==0)
        dwAck = HPI_HIF_ERROR_MASK;    //error!!
    return (short)dwAck;
}

static short Hpi6000_UpdateControlCache( HPI_ADAPTER_OBJ *pao, HPI_MESSAGE *phm ) {
    const HW16 wDspIndex = phm->wDspIndex; // ###### have to think about this
    HPI_HW_OBJ *phw = (HPI_HW_OBJ*)pao->priv;
    DSP_OBJ *pdo = &phw->ado[wDspIndex];
    HW32 dwTimeout;
    HW32 dwCacheDirtyFlag;
    HW16 err;
    HPIOS_LOCK_FLAGS(flags);

	HpiOs_Dsplock_Lock( pao, &flags );

    dwTimeout=TIMEOUT;
    do
    {
        dwCacheDirtyFlag = HpiReadWord((DSP_OBJ *)pdo,
                                       HPI_HIF_BASE + HPI_HIF_OFS_CONTROL_CACHE_IS_DIRTY);
    }
    while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6READ) && --dwTimeout);
	if(!dwTimeout) {
		err= HPI6000_ERROR_CONTROL_CACHE_PARAMS;
		goto unlock;
	}

    if(dwCacheDirtyFlag)
    {
        // read the cached controls
        HW32 dwAddress;
        HW32 dwLength;

        dwTimeout=TIMEOUT;
		if(pdo->dwControlCacheAddressOnDSP==0)
		{
			do
			{
				dwAddress = HpiReadWord((DSP_OBJ *)pdo,
										HPI_HIF_BASE + HPI_HIF_OFS_CONTROL_CACHE_ADDRESS);
				dwLength = HpiReadWord((DSP_OBJ *)pdo,
									   HPI_HIF_BASE + HPI_HIF_OFS_CONTROL_CACHE_SIZE_IN_BYTES);
			}
			while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6READ) && --dwTimeout);
			if(!dwTimeout) {
				err=HPI6000_ERROR_CONTROL_CACHE_ADDRLEN;
				goto unlock;
			}
			pdo->dwControlCacheAddressOnDSP = dwAddress;
			pdo->dwControlCacheLengthOnDSP = dwLength;
		}
		else
		{
			dwAddress = pdo->dwControlCacheAddressOnDSP;
			dwLength = pdo->dwControlCacheLengthOnDSP;
		}

        if(Hpi6000_DspBlockRead32( pao, wDspIndex, dwAddress, (HW32 *)&phw->aControlCache[0], dwLength/sizeof(HW32)))
		{
			err=HPI6000_ERROR_CONTROL_CACHE_READ;
			goto unlock;
		}
        do
        {
            HpiWriteWord((DSP_OBJ *)pdo,
                         HPI_HIF_BASE + HPI_HIF_OFS_CONTROL_CACHE_IS_DIRTY, 0);
			HpiSetAddress( pdo, HPI_HIF_BASE + HPI_HIF_OFS_HOSTCMD);    // flush the FIFO
        }
        while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6WRITE) && --dwTimeout);
        if(!dwTimeout) {
            err=HPI6000_ERROR_CONTROL_CACHE_FLUSH;
			goto unlock;
		}

    }
    err= 0;

unlock:
	 HpiOs_Dsplock_UnLock( pao, &flags );
	 return err;
}

static void  HW_Message( HPI_ADAPTER_OBJ *pao, HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
	HW16 nError=0;
	HPIOS_LOCK_FLAGS(flags);

	HpiOs_Dsplock_Lock( pao, &flags );

	nError = Hpi6000_MessageResponseSequence( pao, phm, phr );

	/* maybe an error response */
	if( nError ) {
		phr->wError = nError;		// something failed in the HPI/DSP interface
		phr->wSize = HPI_RESPONSE_FIXED_SIZE;  // just the header of the response is valid
		goto err;
	}

	if  (phr->wError != 0)          // something failed in the DSP
		goto err;

	switch (phm->wFunction) {
		case HPI_OSTREAM_WRITE:
		case HPI_ISTREAM_ANC_WRITE:
	        	nError = Hpi6000_SendData( pao, phm, phr);
			break;
		case HPI_ISTREAM_READ:
		case HPI_OSTREAM_ANC_READ:
			nError = Hpi6000_GetData( pao, phm, phr);
			break;
		case HPI_ADAPTER_GET_ASSERT:
			phr->u.a.wAdapterIndex=0; // dsp 0 default
			if (((HPI_HW_OBJ*)pao->priv)->wNumDsp == 2) {
				if (!phr->u.a.wAdapterType) {	// no assert from dsp 0, check dsp 1
				phm->wDspIndex=1;
				nError = Hpi6000_MessageResponseSequence( pao, phm, phr );
				phr->u.a.wAdapterIndex=1;
			}
		}
	}

	if( nError )
		phr->wError = nError;

err:
	 HpiOs_Dsplock_UnLock( pao, &flags );
	 return;
}

///////////////////////////////////////////////////////////////////////////
