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


HPI PCI interface function definitions

(C) Copyright AudioScience Inc. 1996,1997,2003
******************************************************************************/

#ifndef _HPIPCI_H_
#define _HPIPCI_H_

#include <hpi.h>
#include <hpidebug.h>

// PCI config reg defines
#define HPIPCI_CDID 0x0     // Vendor/Device Id
#define HPIPCI_CSTR 0x0004
#define HPIPCI_CCMR 0x0004
#define HPIPCI_CCCR 0x0008
#define HPIPCI_CLAT 0x000C
#define HPIPCI_CBMA 0x0010  // base memory address BAR0
#define HPIPCI_CBMB 0x0014  // base memory address BAR1
#define HPIPCI_CBMC 0x0018  // base memory address BAR2
#define HPIPCI_CBMD 0x001c  // base memory address BAR3
#define HPIPCI_CBME 0x0020  // base memory address BAR4
#define HPIPCI_CSUB 0x002C  // sub-system and sub-vendor ID
#define HPIPCI_CILP 0x00FC


// bits in command register
#define HPIPCI_CCMR_MSE 		0x00000002
#define HPIPCI_CCMR_BM 			0x00000004
#define HPIPCI_CCMR_PERR 		0x0000040

//#if defined HPI_OS_LINUX
//#include <linux/pci.h>

#define HPIPCI_MATCH_RESOURCE( idx, iterMax, wAdapterIndex, adapterObjectsArray, hpiResource ) \
	(wAdapterIndex) = -1;\
	for( idx = 0; idx < (iterMax); idx++ ) {\
		HPI_PRINT_VERBOSE("adapter (%d)->(%04x,%04x:%04x,%04x:%04x,%04x) \
		matches with (%04x,%04x:%04x,%04x:%04x,%04x)?",\
		idx,\
		adapterObjectsArray[(idx)].Pci.wBusNumber,\
		adapterObjectsArray[(idx)].Pci.wVendorId,\
		adapterObjectsArray[(idx)].Pci.wDeviceId,\
		adapterObjectsArray[(idx)].Pci.wSubSysVendorId,\
		adapterObjectsArray[(idx)].Pci.wSubSysDeviceId,\
		adapterObjectsArray[(idx)].Pci.wDeviceNumber,\
		(hpiResource).r.Pci.wBusNumber,\
		(hpiResource).r.Pci.wVendorId,\
		(hpiResource).r.Pci.wDeviceId,\
		(hpiResource).r.Pci.wSubSysVendorId,\
		(hpiResource).r.Pci.wSubSysDeviceId,\
		(hpiResource).r.Pci.wDeviceNumber\
		);\
		if ( adapterObjectsArray[(idx)].Pci.wVendorId == (hpiResource).r.Pci.wVendorId &&\
			adapterObjectsArray[(idx)].Pci.wDeviceId == (hpiResource).r.Pci.wDeviceId &&\
			adapterObjectsArray[(idx)].Pci.wSubSysVendorId == (hpiResource).r.Pci.wSubSysVendorId &&\
			adapterObjectsArray[(idx)].Pci.wSubSysDeviceId == (hpiResource).r.Pci.wSubSysDeviceId &&\
			adapterObjectsArray[(idx)].Pci.wBusNumber == (hpiResource).r.Pci.wBusNumber &&\
			adapterObjectsArray[(idx)].Pci.wDeviceNumber == (hpiResource).r.Pci.wDeviceNumber ) {\
			HPI_PRINT_VERBOSE(" yes\n");\
			(wAdapterIndex) = idx;\
			break;\
		}\
		HPI_PRINT_VERBOSE(" no\n");\
	}

//#endif

#if (0)
// structure for HPI PCI bus object
typedef struct
{
	HW16 	wVendorId;
	HW16 	wDeviceId;
    HW16   wSubSysVendorId;
    HW16   wSubSysDeviceId;
	HW16	wBusNumber;
	HW16	wDeviceNumber;
	HW32	dwMemBase[HPI_MAX_ADAPTER_MEM_SPACES];
	HW32	dwPortBase;
	HW32	wInterrupt;
#if defined HPI_OS_LINUX
    struct pci_dev * pOsData;
#else
    void * pOsData;
#endif
} HPI_PCI;
#endif

//DWORD MapPhysicalToLinear(DWORD dwPhysical,DWORD dwLength);

// these functions are called by a PnP type driver to register
// PCI resources that can later be "found" by HpiPci_FindDevice
void HpiPci_Init(void);
void HpiPci_CreateDevice( HW32 dwMemAddr, HW16 wInterrupt);


// given the device index (Nth occurance), vendor and device id, returns the bus
// ,device number and resources (port,memory,irq) if present
short HpiPci_FindDevice
(
	HPI_PCI	*pHpiPci,

	HW16 wDevIndex,
	HW16 wPciVendorId,
	HW16 wPciDevId
);

// given the device index (Nth occurance), vendor, device id and sub-vendor,
// returns the bus, device number and resources (port,memory,irq) if present
short HpiPci_FindDeviceEx
(
	HPI_PCI	*pHpiPci,

	HW16 wDevIndex,
	HW16 wPciVendorId,
	HW16 wPciDevId,
	HW16 wPciSubVendorId
);

short HpiPci_GetMemoryBase
(
	HPI_PCI	*pHpiPci,

	HW32 *pdwMemoryBase
);



short HpiPci_WriteConfig
(
	HPI_PCI	*pHpiPci,

	HW16 wPciConfigReg,
	HW32 dwData
);

short HpiPci_WriteConfigFast
(
	HPI_PCI	*pHpiPci,

	HW16 wPciConfigReg,
	HW32 dwData
);


short HpiPci_ReadConfig
(
	HPI_PCI	*pHpiPci,

	HW16 wPciConfigReg,
	HW32 *dwData
);

void HpiPci_TranslateAddressRange
(
	HPI_PCI	*Pci,
	int		nBar,
	HW16		*wSelectors,
	HW16		wNumberOf64kSelectors
);

void HpiPci_FreeSelectors
(
	HW16		*wSelectors,
	HW16		wNumberOf64kSelectors
);

#endif /* _HPIPCI_H_ */

///////////////////////////////////////////////////////////////////////////
