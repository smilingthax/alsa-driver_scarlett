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
	HPI_PCI Pci;        // PCI info - bus#,dev#,address etc
	HW16    wAdapterType;   // ASI6701 etc
	HW16    wIndex;     //
	HW16    wOpen;      // =1 when adapter open
	HW16    wMixerOpen;

#ifdef HPI_LOCKING
	HPIOS_SPINLOCK dspLock;
#endif

	HW16    wDspCrashed;
	HW16    wHasControlCache;
	void    *priv;
} HPI_ADAPTER_OBJ;

typedef struct {
	HPI_ADAPTER_OBJ adapter[HPI_MAX_ADAPTERS];
	HW16 gwNumAdapters; // total number of adapters created in this HPI
} HPI_ADAPTERS_LIST;

HPI_ADAPTER_OBJ* FindAdapter(HPI_ADAPTERS_LIST *adaptersList, HW16 wAdapterIndex);

void WipeAdapterList(HPI_ADAPTERS_LIST *adaptersList);
void SubSysGetAdapters(HPI_ADAPTERS_LIST *adaptersList, HPI_RESPONSE *phr);
short CheckControlCache( volatile tHPIControlCacheSingle *pC, HPI_MESSAGE *phm, HPI_RESPONSE *phr);
void SyncControlCache( volatile tHPIControlCacheSingle *pC, HPI_MESSAGE *phm, HPI_RESPONSE *phr);
HW16 HpiValidateResponse( HPI_MESSAGE *phm, HPI_RESPONSE *phr);


/*
*/
