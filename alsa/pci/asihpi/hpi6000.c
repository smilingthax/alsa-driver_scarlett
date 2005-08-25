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

#include <hpi.h>
#include <hpios.h>  // for debug
#include <hpipci.h>
#include <hpi6000.h>

#include <hpidspcd.h>

////////////////////////////////////////////////////////////////////////////
// local defines
//#define REVB_C6711
//#define BIG_ENDIAN
//#define USE_REVA - turn off run time detection - always use revA
//#define ASI8801
#define HIDE_PCI_ASSERTS
#define PROFILE_DSP2

// for PCI2040 i/f chip
// HPI CSR registers
#define INTERRUPT_EVENT_SET 	0x00
#define INTERRUPT_EVENT_CLEAR 	0x04
#define INTERRUPT_MASK_SET 		0x08
#define INTERRUPT_MASK_CLEAR	0x0C
#define HPI_ERROR_REPORT		0x10
#define HPI_RESET				0x14
#define HPI_NUM_DSP				0x16
#define HPI_DATA_WIDTH			0x18

// HPI registers, spaced 2K apart - NOTE only one DSP defined at present
#define CONTROL				0x0000
#define ADDRESS				0x0800
#define DATA_AUTOINC		0x1000
#define DATA				0x1800


#define TIMEOUT 500000L

typedef struct
{
    HW32	dwHPIControl;
    HW32	dwHPIAddress;
    HW32	dwHPIData;
    HW32	dwHPIDataAutoInc;
    char	cDspRev;  //A, B
}
H600_DSP_OBJ;

typedef struct
{
	 HPI_PCI Pci;        // PCI info - bus#,dev#,address etc
	 HW16    wAdapterType;   // ASI6701 etc
	 HW16    wIndex;     //
	 HW16    wOpen;      // =1 when adapter open
	 HW16    wMixerOpen;

	 HW32	dw2040_HPICSR;
	 HW32	dw2040_HPIDSP;

	 HW16	wNumDsp;
	 H600_DSP_OBJ ado[4];


    HW32	dwMessageBufferAddressOnDSP;
    HW32	dwResponseBufferAddressOnDSP;
    HW32  dwPCI2040HPIErrorCount;

    HW16	wNumErrors;	//counts number of consecutive communications errors reported from DSP
    HW16	wDspCrashed; // when '1' DSP has crashed/died/OTL

    HW16 wHasControlCache;
    tHPIControlCacheSingle aControlCache[HPI_NMIXER_CONTROLS];
}
H600_ADAPTER_OBJ;


////////////////////////////////////////////////////////////////////////////
static HW16 Hpi6000_DspBlockWrite32( H600_ADAPTER_OBJ *pao, HW16 wDspIndex, HW32 dwHpiAddress, HW32 dwSource, HW32 dwCount);
static HW16 Hpi6000_DspBlockRead32( H600_ADAPTER_OBJ *pao, HW16 wDspIndex, HW32 dwHpiAddress, HW32 dwDest, HW32 dwCount);

static short Hpi6000_AdapterBootLoadDsp( H600_ADAPTER_OBJ *pao );
static short Hpi6000_Check_PCI2040_ErrorFlag(H600_ADAPTER_OBJ *pao, HW16 nReadOrWrite);
#define H6READ 1
#define H6WRITE 0

static short Hpi6000_MessageResponseSequence(H600_ADAPTER_OBJ *pao,HW16 wDspIndex,HPI_MESSAGE *phm,HPI_RESPONSE *phr);

static void  Hpi6000_Message( H600_ADAPTER_OBJ *pao, HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static short Hpi6000_WaitDspAck(H600_ADAPTER_OBJ *pao,HW16 wDspIndex, HW32 dwAckValue);
static short Hpi6000_SendHostCommand(H600_ADAPTER_OBJ *pao,HW16 wDspIndex, HW32 dwHostCmd);
static void Hpi6000_SendDspInterrupt(H600_DSP_OBJ *pdo);
static short Hpi6000_SendData( H600_ADAPTER_OBJ *pao,HW16 wDspIndex,HPI_MESSAGE *phm,HPI_RESPONSE *phr);
static short Hpi6000_GetData( H600_ADAPTER_OBJ *pao, HW16 wDspIndex, HPI_MESSAGE *phm, HPI_RESPONSE *phr);

static void HpiWriteWord( H600_DSP_OBJ *pdo, HW32 dwAddress, HW32 dwData);
static HW32 HpiReadWord( H600_DSP_OBJ *pdo, HW32 dwAddress );
static void HpiWriteBlock( H600_DSP_OBJ *pdo, HW32 dwAddress, HW32 *pdwData, HW32 dwLength);
static void HpiReadBlock( H600_DSP_OBJ *pdo, HW32 dwAddress, HW32 *pdwData, HW32 dwLength );

#define DPI_ERROR           900 /* non-specific error */
#define DPI_ERROR_SEND      910
#define DPI_ERROR_GET       950  //EWB more space for subcodes
#define DPI_ERROR_DOWNLOAD  930

////////////////////////////////////////////////////////////////////////////
// local prototypes
static void H600_SubSysOpen(void);
static void H600_SubSysClose(void);
static void H600_SubSysGetAdapters(HPI_RESPONSE *phr);
static void H600_SubSysCreateAdapter(HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H600_SubSysDeleteAdapter(HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H600_SubSysFindAdapters(HPI_RESPONSE *phr);

static void H600_AdapterOpen(H600_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H600_AdapterClose(H600_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H600_AdapterGetInfo(H600_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);
static void H600_AdapterGetAsserts(H600_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr);

static H600_ADAPTER_OBJ* H600_FindAdapter( HW16 wAdapterIndex );
static short H600_CreateAdapterObj( H600_ADAPTER_OBJ *pao);

////////////////////////////////////////////////////////////////////////////
// external globals
// extern HW16 gwHpiLastError;
extern HW32 gadwHpiSpecificError[8];

////////////////////////////////////////////////////////////////////////////
// local globals
#define H600_MAX_ADAPTERS HPI_MAX_ADAPTERS
static H600_ADAPTER_OBJ gao60[H600_MAX_ADAPTERS];

static HW16 gwNum60Adapters;       // total number of adapters created in this HPI
static HW16 gwTotalOStreams;      // total number of devices created in this HPI
static HW16 gwPciReadAsserts=0;		// used to count PCI2040 errors
static HW16 gwPciWriteAsserts=0;		// used to count PCI2040 errors


////////////////////////////////////////////////////////////////////////////
// HPI_6000()
// Entry point from HPIMAN
// All calls to the HPI start here

void HPI_6000( HPI_MESSAGE *phm, HPI_RESPONSE *phr )
{
    H600_ADAPTER_OBJ *pao;

    // subsytem messages get executed by every HPI.
    // All other messages are ignored unless the adapter index matches
    // an adapter in the HPI
    if (phm->wObject==HPI_OBJ_SUBSYSTEM)
    {
		  HPI_PRINT_VERBOSE("subsys message\n");
        pao=NULL;
    }
    else
    {
        pao = H600_FindAdapter( phm->wAdapterIndex );
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
    switch(phm->wType)
    {
    case HPI_TYPE_MESSAGE:
        switch(phm->wObject)
        {
        case HPI_OBJ_SUBSYSTEM:
            switch(phm->wFunction)
            {
            case HPI_SUBSYS_OPEN:
                H600_SubSysOpen();
                phr->wError=0;
                return;    // note that error is cleared
            case HPI_SUBSYS_CLOSE:
                H600_SubSysClose();
                phr->wError=0;
                return;    // note that error is cleared
            case HPI_SUBSYS_GET_INFO:
                H600_SubSysGetAdapters(phr);
                return;
            case HPI_SUBSYS_FIND_ADAPTERS:
                H600_SubSysFindAdapters(phr);
                return;
            case HPI_SUBSYS_CREATE_ADAPTER:
                H600_SubSysCreateAdapter(phm, phr);
                return;
            case HPI_SUBSYS_DELETE_ADAPTER:
                H600_SubSysDeleteAdapter(phm, phr);
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
                H600_AdapterOpen(pao,phm,phr);
                return;
            case HPI_ADAPTER_CLOSE:
                H600_AdapterClose(pao,phm,phr);
                return;
            case HPI_ADAPTER_GET_INFO:
                H600_AdapterGetInfo(pao,phm,phr);
                return;
            case HPI_ADAPTER_GET_ASSERT:
                H600_AdapterGetAsserts(pao,phm, phr);
                return;
            case HPI_ADAPTER_TEST_ASSERT:
            case HPI_ADAPTER_SELFTEST:
            case HPI_ADAPTER_GET_MODE:
            case HPI_ADAPTER_SET_MODE:
            case HPI_ADAPTER_FIND_OBJECT:
                Hpi6000_Message( pao, phm,phr);
                return;
            default:
                break;
            }
            phr->wError = HPI_ERROR_INVALID_FUNC;
            break;
        default:
            Hpi6000_Message( pao, phm,phr);
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
void H600_SubSysOpen()
{
    HPI_PRINT_VERBOSE( "HPI6000_SubSysOpen\n" );

    gwNum60Adapters = 0;
    gwTotalOStreams = 0;
    memset( gao60, 0, sizeof(H600_ADAPTER_OBJ)*H600_MAX_ADAPTERS );
}

void H600_SubSysClose()
{
    HPI_PRINT_VERBOSE( " HPI6000_SubSysClose\n" );

    gwNum60Adapters = 0;
    gwTotalOStreams = 0;
    memset( gao60, 0, sizeof(H600_ADAPTER_OBJ)*H600_MAX_ADAPTERS );
}


void H600_SubSysGetAdapters(HPI_RESPONSE *phr)
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
    H600_ADAPTER_OBJ *pao=0;

    HPI_PRINT_VERBOSE( " HPI6000_SubSysGetAdapters\n" );

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

void H600_SubSysFindAdapters(HPI_RESPONSE *phr)
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
    // i.e. if we have an ASI6101 with it's jumper set to
    // Adapter Number 2 then put an Adapter type ASI6101 in the
    // array in position 1
    // NOTE: AdapterNumber is 1..N, Index is 0..N-1
    //
    //-----------------------------------------------------------------
    // ASI6100/6200 specific error codes - SGT 6/4/02
    //
    // 920 - invalid PCI subsys device ID
    // 930 - could not find DSP code to match subsys device ID
    // 931 - could not r/w to PCI2040 registers
    // 932 - could not r/w DSP HPI registers
    // 933 - could not r/w DSP internal SRAM
    // 934 - could not r/w external SDRAM (bit test)
    // 935 - could not r/w external SDRAM (address test)
    // 938 - error during write of DSP code to SDRAM
    // 939 - timeout while waiting for DSP code to init
    // 940 - could not r/w FPGA
    // 95X - could not find/open DSP code
    //

    short nIndex=0;
    short nError=0;

    HPI_PRINT_VERBOSE( " HPI6000_SubSysFindAdapters\n" );

    // Cycle through all the PCI bus slots looking for this adapters
    // vendor and device ID
    //
    gwNum60Adapters = 0;
    for(nIndex=0; nIndex<H600_MAX_ADAPTERS; nIndex++)
    {
        H600_ADAPTER_OBJ Adap;
        // AGE 10/3/98
        // Need to use temporary adapter object because we don't
        // know what index to assign it yet. Fixes bug in NT kernel driver.

        memset( &Adap, 0, sizeof(H600_ADAPTER_OBJ) );
        if( HpiPci_FindDeviceEx( &Adap.Pci, nIndex, HPI_PCI_VENDOR_ID_TI, HPI_ADAPTER_PCI2040, 0 ) )// prototype sub-vendor ID=0
            if( HpiPci_FindDeviceEx( &Adap.Pci, nIndex, HPI_PCI_VENDOR_ID_TI, HPI_ADAPTER_PCI2040,HPI_PCI_VENDOR_ID_AUDIOSCIENCE ) )
                break; // no adapter found!

#ifdef HPI_OS_DOS ////////////////////////////// DOS ONLY, ASI6000 ONLY, NO ASI8800 SUPPORT!
        // for real mode DOS only - move the PCI addresses to 0xD0000-0xDFFFF (64K)
        // by changing the base address register in PCI config space
        // The PCI2040 has two blocks, BAR0=4K and BAR1=32K
        // Move the 4K block to 0xD0000 and the 32K block to 0xD8000
        HpiPci_WriteConfig( &Adap.Pci, HPIPCI_CBMA,     0xD0000L );
        HpiPci_WriteConfig( &Adap.Pci, HPIPCI_CBMA+0x4, 0xD8000L );
        if( HpiPci_FindDeviceEx( &Adap.Pci, nIndex, HPI_PCI_VENDOR_ID_TI, HPI_ADAPTER_PCI2040,HPI_PCI_VENDOR_ID_AUDIOSCIENCE ) )
        {
            phr->wError = HPI_ERROR_DOS_MEMORY_ALLOC;
            return;
        }
        // get subsys dev id, to figure out if 6100 or 6200
        {
            HW32 dwSubDev=0;

            // fill in the same stuff as the PnP subsystem
            HpiPci_ReadConfig( &Adap.Pci, HPIPCI_CSUB, &dwSubDev );
            Adap.Pci.wVendorId = HPI_PCI_VENDOR_ID_TI;
            Adap.Pci.wDeviceId = HPI_ADAPTER_PCI2040;
            Adap.Pci.wSubSysVendorId = dwSubDev&0xffff;
            Adap.Pci.wSubSysDeviceId = dwSubDev>>16;
        }
#endif

        // under WIN16, get the subsys device ID, if present
        // find out if we have a 8801 present
        // because it uses different DSP code than the 6200 series
#ifdef HPI_OS_WIN16
        {
            HW32 dwSubDev=0;

            // fill in the same stuff as the PnP subsystem
            HpiPci_ReadConfig( &Adap.Pci, HPIPCI_CSUB, &dwSubDev );

            Adap.Pci.wVendorId = HPI_PCI_VENDOR_ID_TI;
            Adap.Pci.wDeviceId = HPI_ADAPTER_PCI2040;
            Adap.Pci.wSubSysVendorId = dwSubDev&0xffff;
            Adap.Pci.wSubSysDeviceId = dwSubDev>>16;

            switch(Adap.Pci.wSubSysDeviceId)
            {
                //case 0x0000:	//prototypes.
            case 0x5100:
            case 0x6100:
            case 0x6200:
            case 0x8800:
                break;	// valid values of Subdevice Id
            default:
                phr->wError = 920;
                gadwHpiSpecificError[0] = dwSubDev;
                return;  // Error - did not find a valid subsys device ID
            }
        }
#endif
        /*  Note NT kernel implementation of HpiPci_FindDeviceEx() fills in
              	Adap.Pci.wVendorId
              	Adap.Pci.wDeviceId;
              	Adap.Pci.wSubSysVendorId
              	Adap.Pci.wSubSysDeviceId
        */

        // create the adapter object based on the resource information inside Adap.PCI
        nError = H600_CreateAdapterObj( &Adap);                // adapter obj
        if(nError)
        {
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
        }

        phr->u.s.awAdapterList[ Adap.wIndex ] = Adap.wAdapterType;

        gao60[ Adap.wIndex ] = Adap;
        memcpy( &gao60[ Adap.wIndex ], &Adap, sizeof(H600_ADAPTER_OBJ));
        gwNum60Adapters++; // inc the number of adapters known by this HPI
    }

    // add the number of adapters recognised by this HPI to the system total

    phr->u.s.wNumAdapters += gwNum60Adapters;
    //phr->wError = 0;   // SGT remove so that an error on one adapter is not overwritten
}

// create an adapter object and initialise it based on resource information
// passed in in the message
// **** NOTE - you cannot use this function AND the FindAdapters function at the
// same time, the application must use only one of them to get the adapters ******

void H600_SubSysCreateAdapter(HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
    H600_ADAPTER_OBJ ao;	// create temp adapter obj, because we don't know what index yet
    short nError=0;

    HPI_PRINT_VERBOSE( " HPI6000_SubSysCreateAdapter\n" );

    // this HPI only creates adapters for TI/PCI2040 based devices
    if((phm->u.s.Resource.wBusType != HPI_BUS_PCI)
            || ( phm->u.s.Resource.r.Pci.wVendorId != HPI_PCI_VENDOR_ID_TI )
            || ( phm->u.s.Resource.r.Pci.wDeviceId != HPI_ADAPTER_PCI2040 ))
        return;

    // create the adapter object based on the resource information
    memcpy(&ao.Pci,&phm->u.s.Resource.r.Pci,sizeof(ao.Pci));

    nError = H600_CreateAdapterObj( &ao );
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
    memcpy( &gao60[ ao.wIndex ], &ao, sizeof(H600_ADAPTER_OBJ));

    gwNum60Adapters++;         // inc the number of adapters known by this HPI
    phr->u.s.awAdapterList[ ao.wIndex ] = ao.wAdapterType;
    phr->u.s.wAdapterIndex = ao.wIndex;
    phr->u.s.wNumAdapters++;    // add the number of adapters recognised by this HPI to the system total
    phr->wError = 0;            // the function completed OK;
}

// delete an adapter - required by WDM driver


void H600_SubSysDeleteAdapter(HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
    H600_ADAPTER_OBJ *pao=NULL;

    pao = H600_FindAdapter( phm->wAdapterIndex );
    if(!pao)
        return; // message probably meant for another HPI module

    gwNum60Adapters--;
    memset( pao, 0, sizeof(H600_ADAPTER_OBJ) );
}


// this routine is called from SubSysFindAdapter and SubSysCreateAdapter
short H600_CreateAdapterObj( H600_ADAPTER_OBJ *pao )
{
    short	nBootError=0;
    HW32	dwHpiSpacing = 0x2000; //8K
    HW32	dwDspIndex=0;

    // init error reporting
    pao->wNumErrors = 0;
    pao->wDspCrashed = 0;

    // The PCI2040 has the following address map
    // BAR0 - 4K = HPI control and status registers on PCI2040  (HPI CSR)
    // BAR1 - 32K = HPI registers on DSP
    // convert the physical address in a 16bit protected mode address (Selector:Offset)
    pao->dw2040_HPICSR = pao->Pci.dwMemBase[0];
    pao->dw2040_HPIDSP = pao->Pci.dwMemBase[1];

    // set addresses for the 4 possible DSP HPI interfaces
    for(dwDspIndex=0; dwDspIndex<4; dwDspIndex++)
    {
        pao->ado[dwDspIndex].dwHPIControl =      pao->dw2040_HPIDSP + CONTROL + dwHpiSpacing*dwDspIndex;
        pao->ado[dwDspIndex].dwHPIAddress =      pao->dw2040_HPIDSP + ADDRESS + dwHpiSpacing*dwDspIndex;
        pao->ado[dwDspIndex].dwHPIData =         pao->dw2040_HPIDSP + DATA    + dwHpiSpacing*dwDspIndex;
        pao->ado[dwDspIndex].dwHPIDataAutoInc =  pao->dw2040_HPIDSP + DATA_AUTOINC + dwHpiSpacing*dwDspIndex;
    }

    pao->dwPCI2040HPIErrorCount = 0;		// set error count to 0
    pao->wHasControlCache = 0;


    // Set the default number of DSPs on this card
    // This is (conditionally) adjusted after bootloading of the first DSP in the bootload section.
    pao->wNumDsp = 1;

    if (0 != (nBootError = Hpi6000_AdapterBootLoadDsp(pao))){
        return(nBootError); //error
    }
    HPI_PRINT_INFO("Bootload DSP OK\n" );

    pao->dwMessageBufferAddressOnDSP=0L;
    pao->dwResponseBufferAddressOnDSP=0L;

    // get info about the adapter by asking the adapter
    // send a HPI_ADAPTER_GET_INFO message
    {
        HPI_MESSAGE     hM;
        HPI_RESPONSE    hR0;		// response from DSP 0
        HPI_RESPONSE    hR1;		// response from DSP 1
        HW16 wError=0;

        //HpiOs_Printf("GetInfo-"); //*************** debug
        HPI_PRINT_VERBOSE( "HPI6000.C - send ADAPTER_GET_INFO\n" );
        memset(&hM, 0, sizeof(HPI_MESSAGE));
        hM.wType = HPI_TYPE_MESSAGE;
        hM.wSize = sizeof(HPI_MESSAGE);
        hM.wObject = HPI_OBJ_ADAPTER;
        hM.wFunction =  HPI_ADAPTER_GET_INFO;
        hM.wAdapterIndex = 0;
        memset(&hR0, 0, sizeof(HPI_RESPONSE));
        memset(&hR1, 0, sizeof(HPI_RESPONSE));
        hR0.wSize = sizeof(HPI_RESPONSE);
        hR1.wSize = sizeof(HPI_RESPONSE);

        wError = Hpi6000_MessageResponseSequence( pao,0, &hM,&hR0 );
        if(hR0.wError)
        {
            HPI_PRINT_DEBUG( "HPI6000.C - message error\n" );
            return(hR0.wError); //error
        }
        if(pao->wNumDsp==2)
        {
            wError = Hpi6000_MessageResponseSequence( pao,1, &hM,&hR1 );
            if(wError)
                return wError;
        }
        pao->wAdapterType = hR0.u.a.wAdapterType;
        pao->wIndex = hR0.u.a.wAdapterIndex;
    }

    memset(&pao->aControlCache[0],0,sizeof(tHPIControlCacheSingle)*HPI_NMIXER_CONTROLS);
   	// Read the control cache length to figure out if it is turned on.....
	if( HpiReadWord(&pao->ado[0],HPI_HIF_BASE + HPI_HIF_OFS_CONTROL_CACHE_SIZE_IN_BYTES))
        pao->wHasControlCache = 1;
    else
        pao->wHasControlCache = 0;




    HPI_PRINT_VERBOSE( "HPI6000.C- Get adapter info OK\n" );
    pao->wOpen=0;	// upon creation the adapter is closed
    return(0);  //sucess!
}

////////////////////////////////////////////////////////////////////////////
// ADAPTER


void H600_AdapterOpen(H600_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
    // input:  wAdapterIndex
    // output: none
    HPI_PRINT_VERBOSE( "HPI6000_AdapterOpen\n" );

    // can't open adapter if already open
    if(pao->wOpen)
    {
        phr->wError = HPI_ERROR_OBJ_ALREADY_OPEN;
        return;
    }
    pao->wOpen = 1;         // adapter is now open
    Hpi6000_Message( pao, phm,phr);
}

void H600_AdapterClose(H600_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
    // input:  wAdapterIndex
    // output: none
    HPI_PRINT_VERBOSE( " HPI6000_AdapterClose\n" );

    Hpi6000_Message( pao, phm,phr);

#ifdef HPI_OS_DOS
    // disable PCI interface so other apps can't access 6701
    HpiPci_WriteConfig( &pao->Pci, HPIPCI_CCMR, 0 );
#endif

    pao->wOpen = 0;         // adapter is now closed
}

void H600_AdapterGetInfo(H600_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
    Hpi6000_Message( pao, phm,phr);
}

void H600_AdapterGetAsserts(H600_ADAPTER_OBJ *pao,HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
#ifndef HIDE_PCI_ASSERTS
    // if we have PCI2040 asserts then collect them
    if((gwPciReadAsserts>0) || (gwPciWriteAsserts>0))
    {
        phr->u.a.dwSerialNumber = gwPciReadAsserts*100 + gwPciWriteAsserts;
        phr->u.a.wAdapterIndex=1; // assert count
        phr->u.a.wAdapterType=-1; // "dsp index"
        strcpy(phr->u.a.szAdapterAssert, "PCI2040 error");
        gwPciReadAsserts=0;
        gwPciWriteAsserts=0;
        phr->wError=0;
    }
    else
#endif

        Hpi6000_Message( pao, phm,phr);    //get DSP asserts
    return;
}


////////////////////////////////////////////////////////////////////////////
// LOW-LEVEL
///////////////////////////////////////////////////////////////////////////

void H600_AdapterIndex( HPI_RESOURCE * res, short * wAdapterIndex )
{
	int idx;

	HPIPCI_MATCH_RESOURCE( idx, H600_MAX_ADAPTERS, *wAdapterIndex, gao60, *res );
	HPI_PRINT_VERBOSE("H600_AdapterIndex %d\n",*wAdapterIndex);
}

H600_ADAPTER_OBJ* H600_FindAdapter( HW16 wAdapterIndex )
{
    H600_ADAPTER_OBJ *pao=NULL;
    //	HPI_PRINT_VERBOSE("H600_FindAdapter %d ",wAdapterIndex);
    pao = &gao60[wAdapterIndex];
    if(pao->wAdapterType != 0)
    {
        //	    HPI_PRINT_VERBOSE("found\n");
        return(pao);
    }
    else
    {
        //	    HPI_PRINT_VERBOSE("not found\n");
        return(NULL);
    }
}

////////////////////////////////////////////////////////////////////////////
// code that may end up in hpi6701.c

#ifdef WANT_UNUSED_FUNTION_DEFINED
static short Hpi6000_AdapterCheckPresent( H600_ADAPTER_OBJ *pao )
{
    return 0;
}
#endif

static short Hpi6000_AdapterBootLoadDsp( H600_ADAPTER_OBJ *pao)
{
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
        //nBootLoadFamily = Load5100;
        //break;
    case 0x6100:
    case 0x6200:
        nBootLoadFamily = Load6200;
        break;
    case 0x8800:
#if defined (HPI_INCLUDE_8800)
        nBootLoadFamily = Load8800;
        break;
#elif defined (HPI_INCLUDE_8600)
        nBootLoadFamily = Load8600;
        break;
#endif
    default:
        return(930);
    }

    {
        ///////////////////////////////////////////////////////////
        // reset all DSPs, indicate two DSPs are present
        // set RST3-=1 to disconnect HAD8 to set DSP in little endian mode

#ifdef BIG_ENDIAN
        dwEndian = 1;
#else

        dwEndian = 0;
#endif

        dw2040Reset = 0x0003000F;
        HPIOS_MEMWRITE32(pao->dw2040_HPICSR+HPI_RESET, dw2040Reset );

        // read back register to make sure PCI2040 chip is functioning
        // note that bits 4..15 are read-only and so should always return zero, even though we wrote 1 to them
        for(i=0; i<1000; i++)
            dwDelay = HPIOS_MEMREAD32(pao->dw2040_HPICSR+HPI_RESET );  //delay
        if(dwDelay != dw2040Reset)
        {
            gadwHpiSpecificError[0] = 0;
            gadwHpiSpecificError[1] = dw2040Reset;
            gadwHpiSpecificError[2] = dwDelay;
            return(931);
        }

        HPIOS_MEMWRITE32(pao->dw2040_HPICSR + HPI_DATA_WIDTH, 0x00000003 );	// Indicate that DSP#0,1 is a C6X
        HPIOS_MEMWRITE32(pao->dw2040_HPICSR + INTERRUPT_MASK_SET, 0x60000000 );	// set Bit30 and 29 - which will prevent Target aborts from being issued upon HPI or GP error

        ////////////////////////////////////////////////////////////////////////////
        // determine what rev DSP we have by reading back HAD8.  Its pulled high for a revA DSP
        dw2040Reset = dw2040Reset & (~(dwEndian<<3)); 		// isolate DSP HAD8 line from PCI2040 so that Little endian can be set by pullup
        HPIOS_MEMWRITE32(pao->dw2040_HPICSR + HPI_RESET, dw2040Reset );

        //*************************************************************** sgt test  ** must delete
        /*
        dw2040Reset = dw2040Reset & (~0x00000008);      // set HAD8 back to PCI2040, now that DSP has been set to little endian mode
        HPIOS_MEMWRITE32(pao->dw2040_HPICSR + HPI_RESET, dw2040Reset );
        //delay to allow DSP to get going
        for(i=0; i<100; i++)
        dwDelay = HPIOS_MEMREAD32(pao->dw2040_HPICSR+HPI_RESET );  // *********** delay

        dwData = HPIOS_MEMREAD32(pao->ado[0].dwHPIAddress);
        if(dwData & 0x0100)
        */
        //{
        //	pao->ado[0].cDspRev = 'A';	// revA      // **** SGT - remove revA DSP support jan-28-2002
        //	pao->ado[1].cDspRev = 'A';	// revA
        //}
        //else
        {
            pao->ado[0].cDspRev = 'B';	// revB
            pao->ado[1].cDspRev = 'B';	// revB
        }
#ifdef USE_REVA

        pao->ado[0].cDspRev = 'A';	// revA
        pao->ado[1].cDspRev = 'A';	// revA
#endif

        ////////////////////////////////////////////////////////////////////////////////
        // Take both DSPs out of reset while setting HAD8 to set the correct Endian
        dw2040Reset = dw2040Reset & (~0x00000001); 		// start DSP 0
        HPIOS_MEMWRITE32(pao->dw2040_HPICSR + HPI_RESET, dw2040Reset );
        dw2040Reset = dw2040Reset & (~0x00000002); 		// start DSP 1
        HPIOS_MEMWRITE32(pao->dw2040_HPICSR + HPI_RESET, dw2040Reset );

        dw2040Reset = dw2040Reset & (~0x00000008);      // set HAD8 back to PCI2040, now that DSP has been set to little endian mode
        HPIOS_MEMWRITE32(pao->dw2040_HPICSR + HPI_RESET, dw2040Reset );
        //delay to allow DSP to get going
        for(i=0; i<100; i++)
            dwDelay = HPIOS_MEMREAD32(pao->dw2040_HPICSR+HPI_RESET );  //*********** delay


        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // loop through all DSPs, downloading DSP code
        for(dwDspIndex=0; dwDspIndex<pao->wNumDsp; dwDspIndex++)
        {
            H600_DSP_OBJ *pdo = &pao->ado[dwDspIndex];

            ///////////////////////////////////////////////////////////
            // configure DSP so that we download code into the SRAM
            HPIOS_MEMWRITE32(pdo->dwHPIControl, 0x00010001 );	// set control reg for little endian, HWOB=1

            // test access to the HPI address register (HPIA)
            dwTestData=0x00000001;
            for(j=0; j<32; j++)
            {
                HPIOS_MEMWRITE32(pdo->dwHPIAddress, dwTestData);
                dwData = HPIOS_MEMREAD32(pdo->dwHPIAddress);
                if(dwData != dwTestData)
                {
                    gadwHpiSpecificError[0] = 0;
                    gadwHpiSpecificError[1] = dwTestData;
                    gadwHpiSpecificError[2] = dwData;
                    gadwHpiSpecificError[3] = dwDspIndex;
                    return(932);       //error
                }
                dwTestData = dwTestData << 1;
            }

			#if 1
            // if C6713 the setup PLL to generate 225MHz from 25MHz.
            if( HpiReadWord( pdo,0x01B7C118 ) == 0x8000 )	// PLLDIV1 should have a value of 8000 after reset
            {
                // ** C6713 datasheet says we cannot program PLL from HPI, and indeed if we try to set the
                // PLL multiply from the HPI, the PLL does not seem to lock, so we enable the PLL and use the default
                // multiply of x 7
                HpiWriteWord( pdo, 0x01B7C100, 0x0000 );  // bypass PLL
                for(i=0; i<100; i++) dwDelay = HPIOS_MEMREAD32(pao->dw2040_HPICSR+HPI_RESET );  //*********** delay

                //HpiWriteWord( pdo, 0x01B7C110, 0x1001 );  // PLL multiply=x9, 25x9=225MHz
                //** use default of  x7 **
                HpiWriteWord( pdo, 0x01B7C120, 0x8002 );  // EMIF = 225/3=75MHz
                HpiWriteWord( pdo, 0x01B7C11C, 0x8001 );  // peri = 225/2
                HpiWriteWord( pdo, 0x01B7C118, 0x8000 );  // cpu  = 225/1

                for(i=0; i<100; i++) dwDelay = HPIOS_MEMREAD32(pao->dw2040_HPICSR+HPI_RESET );  //*********** delay
                HpiWriteWord( pdo, 0x01B7C100, 0x0001 );  // PLL not bypassed
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
                        return(933);	// error
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
                dwDelay = HPIOS_MEMREAD32(pao->dw2040_HPICSR+HPI_RESET );

#if 1
            // test access to SDRAM
            {
                short j=0;
                HW32 dwTestAddr=0x80000000;
                HW32 dwTestData=0x00000001;
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
                        return(934);     // error
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
                        return(935);     // error
                    }
                    dwTestData++;
                }

            }
#endif

            ///////////////////////////////////////////////////////////
            // write the DSP code down into the DSPs memory
            //HpiDspCode_Open(nBootLoadFamily,&DspCode);
#if defined DSPCODE_FIRMWARE
	    DspCode.psDev = pao->Pci.pOsData;
#endif

            if ((nError=HpiDspCode_Open(nBootLoadFamily,&DspCode))!= 0)
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
                if ((nError=Hpi6000_DspBlockWrite32( pao, (HW16)dwDspIndex, dwAddress, (HW32)pdwCode, dwLength))
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
                        nError = 938;
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

            /*
            ///////////////////////////////////////////////////////////
            // verify that code was written correctly
            nArray=0;
            HEX_OPEN_ARRAY;
            while ( ((dwLength=*pdwCode++) != 0xffffffffL) && (nArray<nNumArrays) )
            {
            	if(dwLength==0xfffffffeL)   // check for the start of another array
            	{
            		nArray++;
            		HEX_OPEN_ARRAY;
            		dwLength=*pdwCode++;
            }
            	dwAddress=*pdwCode++;
            pdwCode++;    //skip Type
            	for(i=0; i<dwLength; i++)
            	{
            		dwData = HpiReadWord( pdo, dwAddress);
            		if(dwData != *pdwCode)
            			return(938);
            pdwCode++;
            		dwAddress += 4;
            }
            }
            */

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
            HPIOS_MEMWRITE32(pdo->dwHPIControl, 0x00030003 );
            for(i=0; i<10000; i++)
                dwDelay = HPIOS_MEMREAD32(pao->dw2040_HPICSR+HPI_RESET );  // delay

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
                return(939);
            }

            // read the DSP adapter Info from the hostmailbox structure after starting the DSP
            if(dwDspIndex==0)
            {
                HW32 dwTestData=0;
                HW32 dwMask=0;

                dwAdapterInfo = HpiReadWord( pdo, HPI_HIF_BASE + HPI_HIF_OFS_ADAPTER_INFO);
                if( (HPI_HIF_ADAPTER_INFO_EXTRACT_ADAPTER(dwAdapterInfo)&HPI_ADAPTER_FAMILY_MASK)==HPI_ADAPTER_ASI6200 )
                {
                    // we have a 2 DSP adapter
                    pao->wNumDsp = 2;		//?????????????????? do all 6200 cards have this many DSPs? - YES - SGT
                }
                // test that the PLD is programmed and we can read/write 24bits
#define PLD_BASE_ADDRESS 0x90000000L  //for ASI6100/6200/8800

                switch(nBootLoadFamily)
                {
                case Load6200:
                    dwMask = 0xFFFFFF00L; 	// ASI6100/6200 has 24bit path to FPGA
                    if(pao->Pci.wSubSysDeviceId == 0x5100)
                        dwMask = 0x00000000L;	// ASI5100 uses AX6 code, but has no PLD r/w register to test
                    break;
                case Load8800:
                case Load8600:
                    dwMask = 0xFFFF0000L;    // ASI8800 has 16bit path to FPGA
                    break;
                default:
                    return(940);
                }
                dwTestData = 0xAAAAAA00L & dwMask;
                HpiWriteWord( pdo, PLD_BASE_ADDRESS+4L, dwTestData); 	// write to Debug register which is 24bits wide (D31-D8)
                dwRead = HpiReadWord( pdo, PLD_BASE_ADDRESS+4L) & dwMask;
                if(dwRead != dwTestData)
                {
                    gadwHpiSpecificError[0] = 0;
                    gadwHpiSpecificError[1] = dwTestData;
                    gadwHpiSpecificError[2] = dwRead;
                    return(940);
                }
                dwTestData = 0x55555500L & dwMask;
                HpiWriteWord( pdo, PLD_BASE_ADDRESS+4L, dwTestData); 	// write to Debug register which is 24bits wide (D31-D8)
                dwRead = HpiReadWord( pdo, PLD_BASE_ADDRESS+4L) & dwMask;
                if(dwRead != dwTestData)
                {
                    gadwHpiSpecificError[0] = 0;
                    gadwHpiSpecificError[1] = dwTestData;
                    gadwHpiSpecificError[2] = dwRead;
                    return(940);
                }
            }
        } // for wNumDSP
        /////////////////////////////////////////////////////////////

    }
    return 0;
}


static void EndianSwap(HW32 *pData, HW32 nBytes)
{
#ifdef BIG_ENDIAN
    int i;
    for(i=0;i<nBytes/4;i++)
        pData[i]= (pData[i]>>16) | (pData[i]<<16);
#endif
}

//////////////////////////////////////////////////////////////////////////////////

// write one word to the HPI port
static void HpiWriteWord( H600_DSP_OBJ *pdo, HW32 dwAddress, HW32 dwData)
{
    HPIOS_MEMWRITE32(pdo->dwHPIAddress, dwAddress);
    HPIOS_MEMWRITE32(pdo->dwHPIData, dwData);
}

// read one word from the HPI port
static HW32 HpiReadWord( H600_DSP_OBJ *pdo, HW32 dwAddress )
{
    HW32 dwData=0;
    HPIOS_MEMWRITE32(pdo->dwHPIAddress, dwAddress);

    // take care of errata in revB DSP (2.0.1)
    dwData = HPIOS_MEMREAD32(pdo->dwHPIData);
    return(dwData);
}

// write a block of 32bit words to the DSP HPI port using auto-inc mode
static  void HpiWriteBlock( H600_DSP_OBJ *pdo, HW32 dwAddress, HW32 *pdwData, HW32 dwLength)
{
    HW32 i=0;
    if(dwLength==0)
        return;
    HPIOS_MEMWRITE32(pdo->dwHPIAddress, dwAddress);
    for(i=0; i<dwLength-1; i++)
        HPIOS_MEMWRITE32(pdo->dwHPIDataAutoInc, *pdwData++);
    // take care of errata in revB DSP (2.0.1)
    HPIOS_MEMWRITE32(pdo->dwHPIData, *pdwData++);  // take care of errata in revB DSP
}

/** read a block of 32bit words from the DSP HPI port using auto-inc mode
 */
static void HpiReadBlock( H600_DSP_OBJ *pdo, HW32 dwAddress, HW32 *pdwData, HW32 dwLength )
{
    if(dwLength==0)
        return;
    HPIOS_MEMWRITE32(pdo->dwHPIAddress, dwAddress);

#if ((defined HPIOS_MEMWRITEBLK32) && (defined HPI_OS_LINUX))
    {
        HW32 *pdwSource = (HW32 *)pdo->dwHPIDataAutoInc;
        HW16 wLength = dwLength-1;
        HPIOS_MEMWRITEBLK32(pdwSource, pdwData , wLength );  // translates to a REP MOVSD

        // take care of errata in revB DSP (2.0.1)
        *(pdwData+dwLength-1) = HPIOS_MEMREAD32(pdo->dwHPIData);  // must end with non auto-inc
    }
#else	 // use C copy
    //#warning HPIOS_MEMWRITEBLK32 is not defined for this OS
    {
        HW32 i=0;
        for(i=0; i<dwLength-1; i++)
            *pdwData++ = HPIOS_MEMREAD32(pdo->dwHPIDataAutoInc);

        // take care of errata in revB DSP (2.0.1)
        *pdwData = HPIOS_MEMREAD32(pdo->dwHPIData);  // must end with non auto-inc
    }
#endif
}


#define PCI_TIMEOUT 100

static HW16 Hpi6000_DspBlockWrite32( H600_ADAPTER_OBJ *pao, HW16 wDspIndex, HW32 dwHpiAddress, HW32 dwSource, HW32 dwCount)
{
    //#define USE_BLOCKS - can't use this at the moment because of revB bug 3.0.1
    H600_DSP_OBJ *pdo = &pao->ado[wDspIndex];
    HW32	dwTimeOut = PCI_TIMEOUT;
    int	nC6711BurstSize=128;

    // write data as blocks of N words so that DSP-HPI is used most efficiently - also seems to prevents crashes??????????
    do
    {
        HW32 	dwLocalHpiAddress=dwHpiAddress;
        int 	wLocalCount=dwCount;
        int 	wXferSize;
        HW32	*pdwData = (HW32*)dwSource;

        while(wLocalCount)
        {
            if(wLocalCount>nC6711BurstSize)
                wXferSize=nC6711BurstSize;
            else
                wXferSize=wLocalCount;

            HpiWriteBlock( pdo, dwLocalHpiAddress, pdwData, wXferSize);
            pdwData += wXferSize;
            dwLocalHpiAddress+=sizeof(HW32)*wXferSize;
            wLocalCount-=wXferSize;
        }
    }
    while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6WRITE) && --dwTimeOut);

    if(dwTimeOut)
        return 0;  // no error
    else
        return 1;  // error
}

static HW16 Hpi6000_DspBlockRead32( H600_ADAPTER_OBJ *pao, HW16 wDspIndex, HW32 dwHpiAddress, HW32 dwDest, HW32 dwCount)
{
    H600_DSP_OBJ *pdo = &pao->ado[wDspIndex];
    HW32	dwTimeOut = PCI_TIMEOUT;
    int	nC6711BurstSize=128;

    do
    {
        HW32 dwLocalHpiAddress=dwHpiAddress;
        int wLocalCount=dwCount;
        int wXferSize;
        HW32 *pdwData = (HW32*)dwDest;

        while(wLocalCount)
        {
            if(wLocalCount>nC6711BurstSize)
                wXferSize=nC6711BurstSize;
            else
                wXferSize=wLocalCount;

            HpiReadBlock( pdo, dwLocalHpiAddress, pdwData, wXferSize);
            pdwData += wXferSize;
            dwLocalHpiAddress+=sizeof(HW32)*wXferSize;
            wLocalCount-=wXferSize;
        }
    }
    while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6READ) && --dwTimeOut);

    if(dwTimeOut)
        return 0;  // no error
    else
        return 1;  // error
}

/////////////////////////////////////////////////////////////////////////////////



static short Hpi6000_MessageResponseSequence(
    H600_ADAPTER_OBJ *pao,
    HW16 wDspIndex,
    HPI_MESSAGE *phm,
    HPI_RESPONSE *phr)
{
    H600_DSP_OBJ *pdo = &pao->ado[wDspIndex];
    HW32 dwTimeout;
    HW16 wAck;
    HW32 dwAddress;
    HW32 dwLength;
    HW32 *pData;

    // does the DSP we are referencing exist?
    if(wDspIndex >= pao->wNumDsp )
        return HPI6000_ERROR_MSG_INVALID_DSP_INDEX;

    wAck = Hpi6000_WaitDspAck(pao, wDspIndex, HPI_HIF_IDLE);
    if(wAck&HPI_HIF_ERROR_MASK)
    {
        pao->wNumErrors++;
        if(pao->wNumErrors == 10)
            pao->wDspCrashed = 1;
        return HPI6000_ERROR_MSG_RESP_IDLE_TIMEOUT;
    }
    pao->wNumErrors = 0;

    // send the message

    // get the address and size
    if(pao->dwMessageBufferAddressOnDSP==0)
    {
        dwTimeout = TIMEOUT;
        do
        {
            dwAddress = HpiReadWord( pdo, HPI_HIF_BASE + HPI_HIF_OFS_MSG_BUFFER_ADR);
            pao->dwMessageBufferAddressOnDSP=dwAddress;
        }
        while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6READ) && --dwTimeout);
        if(!dwTimeout)
            return HPI6000_ERROR_MSG_GET_ADR;
    }
    else
        dwAddress = pao->dwMessageBufferAddressOnDSP;

    //    dwLength = sizeof(HPI_MESSAGE);
    dwLength = phm->wSize;

    EndianSwap((HW32 *)phm, dwLength);

    // send it
    pData=(HW32 *)phm;
    if( Hpi6000_DspBlockWrite32( pao, wDspIndex, dwAddress, (HW32)pData, (HW16)dwLength/4) )
        return HPI6000_ERROR_MSG_RESP_BLOCKWRITE32;

    Hpi6000_SendHostCommand(pao, wDspIndex, HPI_HIF_GET_RESP);
    Hpi6000_SendDspInterrupt(pdo);				// ***** FOR DSP/BIOS version only

    // swap the message back again - why??
    EndianSwap((HW32 *)phm, dwLength);

    wAck = Hpi6000_WaitDspAck(pao, wDspIndex, HPI_HIF_GET_RESP);
    if(wAck&HPI_HIF_ERROR_MASK)
        return HPI6000_ERROR_MSG_RESP_GET_RESP_ACK;

    // get the address and size
    if(pao->dwResponseBufferAddressOnDSP==0)
    {
        dwTimeout = TIMEOUT;
        do
        {
            dwAddress = HpiReadWord( pdo, HPI_HIF_BASE + HPI_HIF_OFS_RESP_BUFFER_ADR);
        }
        while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6READ) && --dwTimeout);
        pao->dwResponseBufferAddressOnDSP = dwAddress;
        if(!dwTimeout)
            return HPI6000_ERROR_RESP_GET_ADR;
    }
    else
        dwAddress = pao->dwResponseBufferAddressOnDSP;

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
    if( Hpi6000_DspBlockRead32( pao, wDspIndex, dwAddress, (HW32)pData, (HW16)dwLength/4) )
        return HPI6000_ERROR_MSG_RESP_BLOCKREAD32;

    EndianSwap((HW32 *)phr, dwLength);

    // set i/f back to idle
    Hpi6000_SendHostCommand(pao, wDspIndex,HPI_HIF_IDLE);
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
    if(dwAddress < (unsigned long)MSG_ADDRESS)
        return 0;
    if(dwAddress > (unsigned long)(QUEUE_START+QUEUE_SIZE))
        return 0;
    if( (dwAddress + (dwLengthInDwords<<2)) > (unsigned long)(QUEUE_START+QUEUE_SIZE))
        return 0;
#endif

    return 1;
}

#ifdef WANT_UNUSED_FUNTION_DEFINED
static short Hpi6000_SendDataDone_CheckAdr(HW32 dwAddress)
{
#ifdef CHECKING
    if(dwAddress < (unsigned long)MSG_ADDRESS)
        return 0;
#endif

    return 1;
}
#endif

#define READONLY_not

static short Hpi6000_SendData( H600_ADAPTER_OBJ *pao,
                               HW16 wDspIndex,
                               HPI_MESSAGE *phm,
                               HPI_RESPONSE *phr)
{
    H600_DSP_OBJ *pdo = &pao->ado[wDspIndex];
    HW32				dwDataSent=0;
    HW16				wAck;
    HW32				dwLength,dwAddress;
    HW32 HUGE *pData =	(HW32 HUGE *)phm->u.d.u.Data.dwpbData;
    HW16				wTimeOut=8;

    while( (dwDataSent < (phm->u.d.u.Data.dwDataSize & ~3L)) && --wTimeOut )  // round dwDataSize down to nearest 4 bytes
    {
        wAck = Hpi6000_WaitDspAck(pao, wDspIndex,HPI_HIF_IDLE);
        if(wAck&HPI_HIF_ERROR_MASK)
            return HPI6000_ERROR_SEND_DATA_IDLE_TIMEOUT;

        Hpi6000_SendHostCommand(pao, wDspIndex,HPI_HIF_SEND_DATA);
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
                Hpi6000_DspBlockWrite32( pao, wDspIndex, dwAddress, (HW32)pData, dwBlkLen);
                dwAddress += dwBlkLen*4;
                pData += dwBlkLen;
                dwLen -= dwBlkLen;
            }
        }

        Hpi6000_SendHostCommand(pao, wDspIndex,HPI_HIF_IDLE);
        Hpi6000_SendDspInterrupt(pdo);

        dwDataSent += dwLength*4;
    }
    if(!wTimeOut)
        return HPI6000_ERROR_SEND_DATA_TIMEOUT;
    return 0;
}

static short Hpi6000_GetData( H600_ADAPTER_OBJ *pao, HW16 wDspIndex, HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
    H600_DSP_OBJ *pdo = &pao->ado[wDspIndex];
    HW32	dwDataGot=0;
    HW16	wAck;
    HW32	dwLength,dwAddress;
    HW32  HUGE *pData =	(HW32 HUGE *)phm->u.d.u.Data.dwpbData;

    while( dwDataGot < (phm->u.d.u.Data.dwDataSize & ~3L) )  // round dwDataSize down to nearest 4 bytes
    {
        wAck = Hpi6000_WaitDspAck(pao, wDspIndex,HPI_HIF_IDLE);
        if(wAck&HPI_HIF_ERROR_MASK)
            return HPI6000_ERROR_GET_DATA_IDLE_TIMEOUT;

        Hpi6000_SendHostCommand(pao, wDspIndex,HPI_HIF_GET_DATA);
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
                Hpi6000_DspBlockRead32( pao, wDspIndex, dwAddress, (HW32)pData, dwBlkLen);
                dwAddress += dwBlkLen*4;
                pData += dwBlkLen;
                dwLen -= dwBlkLen;
            }
        }

        Hpi6000_SendHostCommand(pao, wDspIndex,HPI_HIF_IDLE);
        Hpi6000_SendDspInterrupt(pdo);

        dwDataGot += dwLength*4;
    }
    return 0;
}

static void Hpi6000_SendDspInterrupt(H600_DSP_OBJ *pdo)
{
    HPIOS_MEMWRITE32(pdo->dwHPIControl, 0x00030003 );	// DSPINT
}

static short Hpi6000_SendHostCommand(H600_ADAPTER_OBJ *pao, HW16 wDspIndex, HW32 dwHostCmd)
{
    H600_DSP_OBJ *pdo = &pao->ado[wDspIndex];
    HW32 dwTimeout=TIMEOUT;
    //volatile HW32 dwAddr;     // SGT commented out because not used???
    //volatile HW32 dwControl;

    // set command
    do
    {
        HpiWriteWord( pdo, HPI_HIF_BASE + HPI_HIF_OFS_HOSTCMD, dwHostCmd);
        HPIOS_MEMWRITE32(pdo->dwHPIAddress, HPI_HIF_BASE + HPI_HIF_OFS_HOSTCMD);    // flush the FIFO
    }
    while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6WRITE) && --dwTimeout);
    //dwAddr = HPIOS_MEMREAD32(pdo->dwHPIAddress);
    //dwControl = HPIOS_MEMREAD32(pdo->dwHPIControl);

    // reset the interrupt bit
    HPIOS_MEMWRITE32(pdo->dwHPIControl, 0x00040004);


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
    if( wAdapter < H600_MAX_ADAPTERS)
        return gao60[wAdapter].dwPCI2040HPIErrorCount;
    else
        return 0;
}
#endif

// if the PCI2040 has recorded an HPI timeout, reset the error and return 1
static short Hpi6000_Check_PCI2040_ErrorFlag(H600_ADAPTER_OBJ *pao, HW16 nReadOrWrite)
{
    HW32 dwHPIError;

    dwHPIError = HPIOS_MEMREAD32(pao->dw2040_HPICSR+HPI_ERROR_REPORT);		// read the error bits from the PCI2040
    if(dwHPIError)
    {
        HPIOS_MEMWRITE32(pao->dw2040_HPICSR+HPI_ERROR_REPORT,0L);	// reset the error flag
        pao->dwPCI2040HPIErrorCount++;
        if(nReadOrWrite == 1)
            gwPciReadAsserts++;		// *********************************************************************** inc global
        else
            gwPciWriteAsserts++;
        return 1;
    }
    else
        return 0;
}

static short Hpi6000_WaitDspAck(H600_ADAPTER_OBJ *pao, HW16 wDspIndex, HW32 dwAckValue)
{
    H600_DSP_OBJ *pdo = &pao->ado[wDspIndex];
    HW32 dwAck=0L;
    HW32 dwTimeout;
    HW32 dwHPIC=0L;

    // wait for host interrupt to signal ack is ready
    dwTimeout = TIMEOUT;
    while(--dwTimeout)
    {
        dwHPIC =	HPIOS_MEMREAD32(pdo->dwHPIControl);
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

static short Hpi6000_ControlMessage( H600_ADAPTER_OBJ *pao, HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
    HW16 	nError=0;
    HW32	dwControlType;
    const HW16	wDspIndex = phm->wDspIndex;		// ###### have to think about this
    const H600_DSP_OBJ *pdo = &pao->ado[wDspIndex];
    HW32 dwTimeout;
    HW32 dwCacheDirtyFlag;

    dwTimeout=TIMEOUT;
    do
    {
        dwCacheDirtyFlag = HpiReadWord((H600_DSP_OBJ *)pdo,
                                       HPI_HIF_BASE + HPI_HIF_OFS_CONTROL_CACHE_IS_DIRTY);
    }
    while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6READ) && --dwTimeout);
    if(!dwTimeout)
        return HPI6000_ERROR_CONTROL_CACHE_PARAMS;

    if(dwCacheDirtyFlag)
    {
        // read the cached controls
        HW32 dwAddress;
        HW32 dwLength;

        dwTimeout=TIMEOUT;
        do
        {
            dwAddress = HpiReadWord((H600_DSP_OBJ *)pdo,
                                    HPI_HIF_BASE + HPI_HIF_OFS_CONTROL_CACHE_ADDRESS);
            dwLength = HpiReadWord((H600_DSP_OBJ *)pdo,
                                   HPI_HIF_BASE + HPI_HIF_OFS_CONTROL_CACHE_SIZE_IN_BYTES);
        }
        while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6READ) && --dwTimeout);
        if(!dwTimeout)
            return HPI6000_ERROR_CONTROL_CACHE_PARAMS;

        Hpi6000_DspBlockRead32( pao, wDspIndex, dwAddress, (HW32)&pao->aControlCache[0], dwLength/sizeof(HW32));
        do
        {
            HpiWriteWord((H600_DSP_OBJ *)pdo,
                         HPI_HIF_BASE + HPI_HIF_OFS_CONTROL_CACHE_IS_DIRTY, 0);
            HPIOS_MEMWRITE32(pdo->dwHPIAddress, HPI_HIF_BASE + HPI_HIF_OFS_HOSTCMD);    // flush the FIFO by writing a new address
        }
        while( Hpi6000_Check_PCI2040_ErrorFlag(pao, H6WRITE) && --dwTimeout);
        if(!dwTimeout)
            return HPI6000_ERROR_CONTROL_CACHE_PARAMS;

    }
    // if the control type in the cache is non-zero then we have cached control information to process
    dwControlType = pao->aControlCache[phm->u.c.wControlIndex].ControlType;
    phr->wSize = 	HPI_RESPONSE_FIXED_SIZE + sizeof(HPI_CONTROL_RES);
    phr->wError = 0;
    switch( dwControlType )
    {
    case HPI_CONTROL_METER:
        if (phm->u.c.wAttribute == HPI_METER_RMS)
        {
            phr->u.c.anLogValue[0] = pao->aControlCache[phm->u.c.wControlIndex].u.p.anLogRMS[0];
            phr->u.c.anLogValue[1] = pao->aControlCache[phm->u.c.wControlIndex].u.p.anLogRMS[1];
        }
        else if (phm->u.c.wAttribute == HPI_METER_PEAK)
        {
            phr->u.c.anLogValue[0] = pao->aControlCache[phm->u.c.wControlIndex].u.p.anLogPeak[0];
            phr->u.c.anLogValue[1] = pao->aControlCache[phm->u.c.wControlIndex].u.p.anLogPeak[1];
        }
        else
            dwControlType = 0;	// signal that message was not cached
        break;
    case HPI_CONTROL_VOLUME:
        if (phm->u.c.wAttribute == HPI_VOLUME_GAIN)
        {
            phr->u.c.anLogValue[0] = pao->aControlCache[phm->u.c.wControlIndex].u.v.anLog[0];
            phr->u.c.anLogValue[1] = pao->aControlCache[phm->u.c.wControlIndex].u.v.anLog[1];
        }
        else
            dwControlType = 0;	// signal that message was not cached
        break;
    case HPI_CONTROL_MULTIPLEXER:
        if (phm->u.c.wAttribute == HPI_MULTIPLEXER_SOURCE)
        {
            phr->u.c.dwParam1 = pao->aControlCache[phm->u.c.wControlIndex].u.x.wSourceNodeType;
            phr->u.c.dwParam2 = pao->aControlCache[phm->u.c.wControlIndex].u.x.wSourceNodeIndex;
        }
        else
            dwControlType = 0;	// signal that message was not cached
        break;
    case HPI_CONTROL_LEVEL:
        if (phm->u.c.wAttribute == HPI_CONTROL_LEVEL)
        {
            phr->u.c.anLogValue[0] = pao->aControlCache[phm->u.c.wControlIndex].u.l.anLog[0];
            phr->u.c.anLogValue[1] = pao->aControlCache[phm->u.c.wControlIndex].u.l.anLog[1];
        }
        else
            dwControlType = 0;	// signal that message was not cached
        break;
    case HPI_CONTROL_TUNER:
        if (phm->u.c.wAttribute == HPI_TUNER_FREQ)
            phr->u.c.dwParam1 = pao->aControlCache[phm->u.c.wControlIndex].u.t.dwFreqInkHz;
        else if (phm->u.c.wAttribute == HPI_TUNER_BAND)
            phr->u.c.dwParam1 = pao->aControlCache[phm->u.c.wControlIndex].u.t.wBand;
        else if (phm->u.c.wAttribute == HPI_TUNER_LEVEL)
            phr->u.c.dwParam1 = pao->aControlCache[phm->u.c.wControlIndex].u.t.wLevel;
        else
            dwControlType = 0;	// signal that message was not cached
        break;
    default:
        dwControlType = 0;	// signal that message was not cached
        break;
    }
    if(dwControlType==0)	// value of 0 indicates that control is not cached.
        nError = Hpi6000_MessageResponseSequence( pao,wDspIndex, phm,phr );

    return nError;
}

static void  Hpi6000_Message( H600_ADAPTER_OBJ *pao, HPI_MESSAGE *phm, HPI_RESPONSE *phr)
{
    HW16	nError;
    HW16 	wDspIndex = phm->wDspIndex;

    if(pao->wHasControlCache && (phm->wFunction==HPI_CONTROL_GET_STATE))
        nError = Hpi6000_ControlMessage( pao, phm, phr);
    else
        nError = Hpi6000_MessageResponseSequence( pao,wDspIndex, phm,phr );

    /* maybe an error response */
    if( nError )
    {
        phr->wError = nError;		// something failed in the HPI/DSP interface
        phr->wSize = HPI_RESPONSE_FIXED_SIZE;  // just the header of the response is valid
        return;
    }
    if  (phr->wError != 0)          // something failed in the DSP
        return;


    if (
        (phm->wFunction==HPI_OSTREAM_WRITE)
        ||
        (phm->wFunction==HPI_ISTREAM_ANC_WRITE)
    )
    {
        nError = Hpi6000_SendData( pao, wDspIndex, phm, phr);
        if(nError)
        {
            phr->wError=nError;
            return;
        }
    }
    else if (
        (phm->wFunction==HPI_ISTREAM_READ)
        ||
        (phm->wFunction==HPI_OSTREAM_ANC_READ)
    )
    {
        nError = Hpi6000_GetData( pao, wDspIndex, phm, phr);
        if(nError)
        {
            phr->wError=nError;
            return;
        }
    }
    else if (phm->wFunction==HPI_ADAPTER_GET_ASSERT)
    {
        phr->u.a.wAdapterIndex=0; // dsp 0 default
        if (pao->wNumDsp == 2)
        {
            if (!phr->u.a.wAdapterType)	// no assert from dsp 0, check dsp 1
            {
                nError = Hpi6000_MessageResponseSequence( pao,1, phm,phr );
                phr->u.a.wAdapterIndex=1;
                if( nError )
                {
                    phr->wError = nError;
                    return;
                }

                // dsp number returned in wAdapterIndex
                //					 if (phr->u.a.wAdapterType)
                //						  phr->u.a.dwSerialNumber += 10000; // differentiate messages from DSP2
            }
        }
    }

    return;
}

///////////////////////////////////////////////////////////////////////////
