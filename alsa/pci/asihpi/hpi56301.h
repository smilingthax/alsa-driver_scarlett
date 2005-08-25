/* ****************************************************************************

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


Public declarations for DSP Proramming Interface to PCI-based 56301 DSP

**************************************************************************** */

#ifndef _HPI56301_H_
#define _HPI56301_H_

typedef struct
{
    HPI_PCI Pci;        // PCI info - bus#,dev#,address etc
	 HW16    wAdapterType;   // A4501 etc
	 HW16    wIndex;     // 0..3
	 HW16    wOpen;      // =1 when adapter open
	 HW16    wMixerOpen;
}
H400_ADAPTER_OBJ;

short Hpi56301_CheckAdapterPresent( HW32 dwMemBase );
short Hpi56301_BootLoadDsp( H400_ADAPTER_OBJ * pao );
short Hpi56301_SelfTest( HW32 dwMemBase);

void  Hpi56301_Message( HW32 dwMemBase, HPI_MESSAGE *phm, HPI_RESPONSE *phr);
short Hpi56301_SendMessage( HW32 dwMemBase, HPI_MESSAGE *phm);
short Hpi56301_GetResponse( HW32 dwMemBase, HPI_RESPONSE *phr);

#define DPI_ERROR           900 /* non-specific error */
#define DPI_ERROR_SEND      910
#define DPI_ERROR_GET       950
#define DPI_ERROR_DOWNLOAD  930

#endif // _HPI56301_H_

////////////////////////////////////////////////////////////
