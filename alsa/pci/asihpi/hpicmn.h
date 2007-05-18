/**

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

*/

typedef struct {
	HPI_PCI Pci;		// PCI info - bus#,dev#,address etc
	u16 wAdapterType;	// ASI6701 etc
	u16 wIndex;		//
	u16 wOpen;		// =1 when adapter open
	u16 wMixerOpen;

	HPIOS_SPINLOCK dspLock;

	u16 wDspCrashed;
	u16 wHasControlCache;
	void *priv;
} HPI_ADAPTER_OBJ;

typedef struct {
	HPIOS_SPINLOCK aListLock;
	HPI_ADAPTER_OBJ adapter[HPI_MAX_ADAPTERS];
	u16 gwNumAdapters;	// total number of adapters created in this HPI
} HPI_ADAPTERS_LIST;

HPI_ADAPTER_OBJ *FindAdapter(u16 wAdapterIndex);
void WipeAdapterList(void);
void SubSysGetAdapters(HPI_RESPONSE * phr);
u16 AddAdapter(HPI_ADAPTER_OBJ * pao);
void DeleteAdapter(HPI_ADAPTER_OBJ * pao);

short CheckControlCache(volatile tHPIControlCacheSingle * pC, HPI_MESSAGE * phm,
			HPI_RESPONSE * phr);
void SyncControlCache(volatile tHPIControlCacheSingle * pC, HPI_MESSAGE * phm,
		      HPI_RESPONSE * phr);
u16 HpiValidateResponse(HPI_MESSAGE * phm, HPI_RESPONSE * phr);

/*
*/
