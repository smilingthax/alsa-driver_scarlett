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


  Hardware Programming Interface (HPI) for AudioScience ASI4500 and 4100
  series adapters.  These PCI bus adapters are based on the Motorola DSP56301
  DSP with on-chip PCI I/F.

  Exported functions:
  void HPI_4000( HPI_MESSAGE *phm, HPI_RESPONSE *phr )
******************************************************************************/

#include <hpi.h>
#include <hpios.h>  // for debug
#include <hpidebug.h>  // for debug
#include <hpipci.h>
#include <hpi56301.h>

////////////////////////////////////////////////////////////////////////////
// local defines


////////////////////////////////////////////////////////////////////////////
// local prototypes
static void H400_SubSysOpen(void);
static void H400_SubSysClose(void);
static void H400_SubSysGetAdapters(HPI_RESPONSE *phr);
static void H400_SubSysCreateAdapter(HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H400_SubSysDeleteAdapter(HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H400_SubSysFindAdapters(HPI_RESPONSE *phr);

static void H400_AdapterOpen(H400_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H400_AdapterClose(H400_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);

static H400_ADAPTER_OBJ* H400_FindAdapter( HW16 wAdapterIndex );
//static short H400_CreateAdapterObj( H400_ADAPTER_OBJ *pao, HW16 wAdapterType, HW32 dwMemoryBase, HW16 dwInterrupt);
static short H400_CreateAdapterObj( H400_ADAPTER_OBJ *pao, HW16 wAdapterType);

////////////////////////////////////////////////////////////////////////////
// local globals
#define H400_MAX_ADAPTERS HPI_MAX_ADAPTERS
static H400_ADAPTER_OBJ gao45[H400_MAX_ADAPTERS];

static HW16 gwNum450Adapters;       // total number of adapters created in this HPI
static HW16 gwTotalOStreams;      // total number of devices created in this HPI

////////////////////////////////////////////////////////////////////////////
// HPI_4000()
// Entry point from HPIMAN
// All calls to the HPI start here

void HPI_4000(HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
    H400_ADAPTER_OBJ *pao;

    // subsytem messages get executed by every HPI.
    // All other messages are ignored unless the adapter index matches
    // an adapter in the HPI
    if (phm->wObject==HPI_OBJ_SUBSYSTEM)
        pao=NULL;
    else
    {
        pao = H400_FindAdapter( phm->wAdapterIndex );
        if(!pao)
            return; // message probably meant for another HPI module
    }

    //phr->wError=0;      // SGT JUN-15-01 - - so that modules can't overwrite errors from FindAdapter



    switch(phm->wType)
    {
    case HPI_TYPE_MESSAGE:
        switch(phm->wObject)
        {
        case HPI_OBJ_SUBSYSTEM:
            switch(phm->wFunction)
            {
            case HPI_SUBSYS_OPEN:
                H400_SubSysOpen();
                phr->wError=0;
                return;     // note that error is cleared
            case HPI_SUBSYS_CLOSE:
                H400_SubSysClose();
                phr->wError=0;
                return;     // note that error is cleared
            case HPI_SUBSYS_GET_INFO:
                H400_SubSysGetAdapters(phr);
                return;
            case HPI_SUBSYS_FIND_ADAPTERS:
                H400_SubSysFindAdapters(phr);
                return;
            case HPI_SUBSYS_CREATE_ADAPTER:
                H400_SubSysCreateAdapter(phm, phr);
                return;
            case HPI_SUBSYS_DELETE_ADAPTER:
                H400_SubSysDeleteAdapter(phm, phr);
                return;
            default:
                break;
            }
            break;

        case HPI_OBJ_ADAPTER:
            switch(phm->wFunction)
            {
            case HPI_ADAPTER_OPEN:
                H400_AdapterOpen(pao,phm,phr);
                return;
            case HPI_ADAPTER_CLOSE:
                H400_AdapterClose(pao,phm,phr);
                return;
            case HPI_ADAPTER_FIND_OBJECT:
                HPI_InitResponse(phr, HPI_OBJ_ADAPTER, HPI_ADAPTER_FIND_OBJECT,0);
                phr->u.a.wAdapterIndex=0; // really DSP index in this context
					 return;
				case HPI_ADAPTER_GET_ASSERT:
					 /* make sure extended assert count is 0  - avoid touching AX,AX4 DSP code
						 See also hpifunc.c HPI_AdapterGetAssertEx */
					 Hpi56301_Message( pao->Pci.dwMemBase[0], phm,phr);
					 phr->u.a.wAdapterType=0;
					 return;
				default:	// let DSP handle all default cases
					 Hpi56301_Message( pao->Pci.dwMemBase[0], phm,phr);
					 return;
				};
				break;

        case HPI_OBJ_MIXER:
            switch(phm->wFunction)
            {
            case HPI_MIXER_OPEN:
                // experiment with delay to allow settling of D/As on adapter
                // before enabling mixer and so outputs
                HpiOs_DelayMicroSeconds( 500000L );      //500ms
					 Hpi56301_Message( pao->Pci.dwMemBase[0], phm,phr);
                return;
            default:
					 Hpi56301_Message( pao->Pci.dwMemBase[0], phm,phr);
                return;
            };
				break;
        default:
				Hpi56301_Message( pao->Pci.dwMemBase[0], phm,phr);
            break;
        }
        break;
    default:
        HPI_InitResponse(phr, phm->wObject, phm->wFunction, HPI_ERROR_INVALID_TYPE );
        break;
    }
}

////////////////////////////////////////////////////////////////////////////
// SUBSYSTEM

// This message gets processed by all HPIs
// and is used to initialise the objects within each HPI
void H400_SubSysOpen()
{
    HPIOS_DEBUG_STRING( " HPI4000_SubSysOpen\n" );

    gwNum450Adapters = 0;
    gwTotalOStreams = 0;
    memset( gao45, 0, sizeof(H400_ADAPTER_OBJ)*H400_MAX_ADAPTERS );
}

void H400_SubSysClose()
{
    HPIOS_DEBUG_STRING( " HPI4000_SubSysClose\n" );

    gwNum450Adapters = 0;
    gwTotalOStreams = 0;
    memset( gao45, 0, sizeof(H400_ADAPTER_OBJ)*H400_MAX_ADAPTERS );
}

void H400_SubSysGetAdapters(HPI_RESPONSE *phr)
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
    H400_ADAPTER_OBJ *pao=0;

    HPIOS_DEBUG_STRING( " HPI4000_SubSysGetAdapters\n" );

    // for each adapter, place it's type in the position of the array
    // corresponding to it's adapter number
    for(i=0; i<gwNum450Adapters; i++)
    {
        pao = &gao45[i];
        if( phr->u.s.awAdapterList[ pao->wIndex ] != 0)
        {
            phr->wError = HPI_DUPLICATE_ADAPTER_NUMBER;
            return;
        }
        phr->u.s.awAdapterList[ pao->wIndex ] = pao->wAdapterType;
    }

    // add the number of adapters recognised by this HPI to the system total
    phr->u.s.wNumAdapters += gwNum450Adapters;
    phr->wError = 0;        // the function completed OK;
}

// create an adapter object and initialise it based on resource information
// passed in in the message
// **** NOTE - you cannot use this function AND the FindAdapters function at the
// same time, the application must use only one of them to get the adapters ******


void H400_SubSysCreateAdapter(HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
    //H400_ADAPTER_OBJ *pao = &gao45[gwNum450Adapters];   // point to the next new adapter object
    H400_ADAPTER_OBJ ao;	// create temp adapter obj, because we don't know what index yet

    short nError=0;

    HPIOS_DEBUG_STRING( " HPI4000_SubSysCreateAdapter\n" );

    // this HPI only creates adapters for Motorola/PCI devices
    if((phm->u.s.Resource.wBusType != HPI_BUS_PCI)
            || ( phm->u.s.Resource.r.Pci.wVendorId != HPI_PCI_VENDOR_ID_MOTOROLA ))
        return;

	 memcpy(&ao.Pci,&phm->u.s.Resource.r.Pci,sizeof(ao.Pci));
    // create the adapter object based on the resource information
	 nError = H400_CreateAdapterObj( &ao,HPI_ADAPTER_ASI4501);   // dummy adapter type - not used
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
	 memcpy( &gao45[ ao.wIndex ], &ao, sizeof(H400_ADAPTER_OBJ));

	 gwNum450Adapters++;         // inc the number of adapters known by this HPI
	 phr->u.s.awAdapterList[ ao.wIndex ] = ao.wAdapterType;
	 phr->u.s.wAdapterIndex = ao.wIndex;
	 phr->u.s.wNumAdapters++;    // add the number of adapters recognised by this HPI to the system total
	 phr->wError = 0;            // the function completed OK;
}

void H400_SubSysDeleteAdapter(HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
    H400_ADAPTER_OBJ *pao=NULL;

    pao = H400_FindAdapter( phm->wAdapterIndex );
    if(!pao)
        return; // message probably meant for another HPI module

    gwNum450Adapters--;
    memset( pao, 0, sizeof(H400_ADAPTER_OBJ) );
}

void H400_SubSysFindAdapters(HPI_RESPONSE *phr)
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
    // i.e. if we have an ASI4501 with it's jumper set to
    // Adapter Number 2 then put an Adapter type ASI4501 in the
    // array in position 1
    // NOTE: AdapterNumber is 1..N, Index is 0..N-1

    short nIndex=0;
    short nError=0;

    HPIOS_DEBUG_STRING( " HPI4000_SubSysFindAdapters\n" );

    // Cycle through all the PCI bus slots looking for this adapters
    // vendor and device ID
    //
    gwNum450Adapters = 0;
    for(nIndex=0; nIndex<H400_MAX_ADAPTERS; nIndex++)
    {
        H400_ADAPTER_OBJ ao;
        HW16 wError=0;

        // AGE 10/3/98
        // Need to use temporary adapter object because we don't
        // know what index to assign it yet. Fixes bug in NT kernel driver.
        memset( &ao, 0, sizeof(H400_ADAPTER_OBJ) );

        // look for ASI cards that have sub-vendor-ID = 0, like the 4501, 4113 and 4215 revC and below
        wError = HpiPci_FindDeviceEx( &ao.Pci, nIndex, HPI_PCI_VENDOR_ID_MOTOROLA, HPI_ADAPTER_DSP56301,0 );
        if(wError==0)
            goto FoundAdapter;
        // look for ASI cards that have 0x12cf sub-vendor ID, like the 4300 and 4601
        wError = HpiPci_FindDeviceEx( &ao.Pci, nIndex, HPI_PCI_VENDOR_ID_MOTOROLA, HPI_ADAPTER_DSP56301,0x12CF);
        if(wError==0)
            goto FoundAdapter;
        // look for ASI cards that have an AudioScience sub-vendor ID, like the 4401 and 4215 revD
        wError = HpiPci_FindDeviceEx( &ao.Pci, nIndex, HPI_PCI_VENDOR_ID_MOTOROLA, HPI_ADAPTER_DSP56301, HPI_PCI_VENDOR_ID_AUDIOSCIENCE);
        if(wError==0)
            goto FoundAdapter;

        break;	// did not find any cards we recognise

FoundAdapter:
#ifdef HPI_OS_DOS ////////////////////////////// DOS ONLY!
        // for real mode DOS only - move the adapter base
        // address to 0xD0000-0xDFFFF (occupies 64K)
        // by changing the base address register in config space
        HpiPci_WriteConfig( &ao.Pci, HPIPCI_CBMA , 0xD0000L );

        // re-find the device to get the correct memory address
        if(HpiPci_FindDevice( &ao.Pci, nIndex, HPI_PCI_VENDOR_ID_MOTOROLA, HPI_ADAPTER_DSP56301))
        {
				phr->wError = HPI_ERROR_DOS_MEMORY_ALLOC;
            return;
        }
#endif ///////////////////////////////////////// DOS ONLY!

        HPIOS_DEBUG_STRING("Found DSP56301 based PCI adapter\n");

        // turn on the adapters memory address decoding (in PCI config space)
        // also enable parity error responses and bus mastering
        HpiPci_WriteConfig( &ao.Pci, HPIPCI_CCMR, HPIPCI_CCMR_MSE | HPIPCI_CCMR_PERR | HPIPCI_CCMR_BM );

        // create the adapter object based on the resource information
		  nError = H400_CreateAdapterObj( &ao,                // adapter obj
													 HPI_ADAPTER_ASI4501);     // adapter type

		  if(nError)
        {
			phr->wError = nError;
			continue;		// allow multiple adapters , even if one fails
			//return;
        }

        // add to adapter list - but don't allow two adapters of same number!
        if( phr->u.s.awAdapterList[ ao.wIndex ] != 0)
        {
            phr->wError = HPI_DUPLICATE_ADAPTER_NUMBER;
            return;
        }
        phr->u.s.awAdapterList[ ao.wIndex ] = ao.wAdapterType;

        memcpy( &gao45[ ao.wIndex ], &ao, sizeof(H400_ADAPTER_OBJ));
        gwNum450Adapters++; // inc the number of adapters known by this HPI
    }

    // add the number of adapters recognised by this HPI to the system total

    phr->u.s.wNumAdapters += gwNum450Adapters;
    //phr->wError = 0;  // SGT remove so that an error on one adapter is not overwritten
}

// this routine is called from SubSysFindAdapter and SubSysCreateAdapter

short H400_CreateAdapterObj( H400_ADAPTER_OBJ *pao, HW16 wAdapterType) //, HW32 dwMemoryBase, HW16 wInterrupt)
{
    short nBootError=0;

    // assign resources.
    pao->wOpen = 0;
    pao->wAdapterType = wAdapterType;
//?	 pao->dwMemBase = dwMemoryBase;
//?    pao->wInterrupt = wInterrupt;

    // Is it really a 301 chip?
	 if(Hpi56301_CheckAdapterPresent(pao->Pci.dwMemBase[0]))
    {
        //phr->wSpecificError = 1;
        return(HPI_ERROR_BAD_ADAPTER);  //error
    }
    HPI_PRINT_VERBOSE("H400_CreateAdapterObj - Adapter present OK\n" );

    if (0 != (nBootError = Hpi56301_BootLoadDsp(pao)))
    {
        //phr->wSpecificError = 2;
        return(nBootError); //error
    }
    HPI_PRINT_INFO("Bootload DSP OK\n" );

	 if (Hpi56301_SelfTest(pao->Pci.dwMemBase[0]))
    {
        //phr->wSpecificError = 3;
        return(HPI_ERROR_DSP_SELFTEST); //error
    }

    // check that the code got into the adapter Ok and is running by
    // getting back a checksum and comparing it to a local checksum
    // ** TODO **

    // get info about the adapter by asking the adapter
    // send a HPI_ADAPTER_GET_INFO message


    {
        HPI_MESSAGE     hM;
        HPI_RESPONSE    hR;

        HPIOS_DEBUG_STRING("H400_CreateAdapterObj - Send ADAPTER_GET_INFO\n" );
        memset(&hM, 0, sizeof(HPI_MESSAGE));
        hM.wType = HPI_TYPE_MESSAGE;
        hM.wSize = sizeof(HPI_MESSAGE);
        hM.wObject = HPI_OBJ_ADAPTER;
        hM.wFunction =  HPI_ADAPTER_GET_INFO;
        hM.wAdapterIndex = 0;
        memset(&hR, 0, sizeof(HPI_RESPONSE));
        hR.wSize = sizeof(HPI_RESPONSE);

		  Hpi56301_Message(pao->Pci.dwMemBase[0], &hM, &hR);

        if(hR.wError)
        {
            HPIOS_DEBUG_STRING( "HPI4000.C - message error\n" );
            return(hR.wError); //error
        }

        pao->wAdapterType = hR.u.a.wAdapterType;
        pao->wIndex = hR.u.a.wAdapterIndex;
    }
    HPIOS_DEBUG_STRING("H400_CreateAdapterObj - Get adapter info OK\n" );
    return(0);  //success!
}


////////////////////////////////////////////////////////////////////////////
// ADAPTER


void H400_AdapterOpen(H400_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
    // input:  wAdapterIndex
    // output: none
    HPIOS_DEBUG_STRING( "HPI4000_AdapterOpen\n" );

    // can't open adapter if already open
    if (pao->wOpen)
    {
        phr->wError = HPI_ERROR_OBJ_ALREADY_OPEN;
        return;
    }
    pao->wOpen = 1;         // adapter is now open
	 Hpi56301_Message(pao->Pci.dwMemBase[0], phm,phr);
}

void H400_AdapterClose(H400_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
    // input:  wAdapterIndex
    // output: none
    HPIOS_DEBUG_STRING( " HPI4000_AdapterClose\n" );

	 Hpi56301_Message( pao->Pci.dwMemBase[0], phm,phr);

#ifdef HPI_OS_DOS
    // disable PCI interface so other apps can't access 56301
    HpiPci_WriteConfig( &pao->Pci, HPIPCI_CCMR, 0 );
#endif

    pao->wOpen = 0;         // adapter is now closed
}

////////////////////////////////////////////////////////////////////////////
// LOW-LEVEL
///////////////////////////////////////////////////////////////////////////

void H400_AdapterIndex( HPI_RESOURCE * res, short * wAdapterIndex )
{
	int idx;

	HPIPCI_MATCH_RESOURCE( idx, H400_MAX_ADAPTERS, *wAdapterIndex, gao45, *res );
	HPI_PRINT_VERBOSE("H400_AdapterIndex %d\n",*wAdapterIndex);
}

H400_ADAPTER_OBJ* H400_FindAdapter( HW16 wAdapterIndex )
{
    H400_ADAPTER_OBJ *pao=NULL;

    //SGT old way - doesn't work with different HPI types!
    // find an adapter that matches this adapter index
    //for(i=0; i<gwNum450Adapters; i++)
    //{
    //    pao = &gao45[i];
    //    if( pao->wIndex == wAdapterIndex)
    //        return(pao);          // found the adapter so stop searching and leave
    //}
    //return(NULL);               // did not find this adapter index in this HPI

    //SGT new way
    pao = &gao45[wAdapterIndex];
    if(pao->wAdapterType != 0)
        return(pao);
    else
        return(NULL);
}

///////////////////////////////////////////////////////////////////////////
