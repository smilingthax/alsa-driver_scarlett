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

struct hpi_adapter_obj {
	struct hpi_pci Pci;	/* PCI info - bus#,dev#,address etc */
	u16 wAdapterType;	/* ASI6701 etc */
	u16 wIndex;		/* */
	u16 wOpen;		/* =1 when adapter open */
	u16 wMixerOpen;

	struct hpios_spinlock dspLock;

	u16 wDspCrashed;
	u16 wHasControlCache;
	void *priv;
};

struct hpi_control_cache {
	u32 dwInit;	       /**< indicates whether the
				structures are initialized */
	u32 dwControlCount;
	u32 dwCacheSizeInBytes;
	struct hpi_control_cache_info
	**pInfo;		/**< pointer to allocated memory of
				lookup pointers. */
	struct hpi_control_cache_single
	*pCache;		/**< pointer to DSP's control cache. */
};

struct hpi_adapter_obj *HpiFindAdapter(
	u16 wAdapterIndex
);
u16 HpiAddAdapter(
	struct hpi_adapter_obj *pao
);

void HpiDeleteAdapter(
	struct hpi_adapter_obj *pao
);

short HpiCheckControlCache(
	struct hpi_control_cache *pC,
	struct hpi_message *phm,
	struct hpi_response *phr
);
struct hpi_control_cache *HpiAllocControlCache(
	const u32 dwNumberOfControls,
	const u32 dwSizeInBytes,
	struct hpi_control_cache_info
	*pDSPControlBuffer
);
void HpiFreeControlCache(
	struct hpi_control_cache *pCache
);

void HpiSyncControlCache(
	struct hpi_control_cache *pC,
	struct hpi_message *phm,
	struct hpi_response *phr
);
u16 HpiValidateResponse(
	struct hpi_message *phm,
	struct hpi_response *phr
);
short HpiCheckBufferMapping(
	struct hpi_control_cache *pCache,
	struct hpi_message *phm,
	void **p,
	unsigned int *pN
);
