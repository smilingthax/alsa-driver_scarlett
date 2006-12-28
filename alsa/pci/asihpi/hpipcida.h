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

Array initializer for PCI card IDs

(C) Copyright AudioScience Inc. 1998-2003
*******************************************************************************/

//NOTE: when adding new lines to this header file they MUST be grouped by HPI entry point.
//      (see how we propagate DRIVER_(UN)LOAD messages in hpimsgx.c)

#if ( defined ( HPI_INCLUDE_5000 ) + defined ( HPI_INCLUDE_6400 ) + defined ( HPI_INCLUDE_6600 ) + defined ( HPI_INCLUDE_8700 ) )
{
HPI_PCI_VENDOR_ID_TI, HPI_ADAPTER_DSP6205,
	    HPI_PCI_VENDOR_ID_AUDIOSCIENCE, PCI_ANY_ID, 0, 0, HPI_6205}
,
#endif
#if ( defined ( HPI_INCLUDE_6000 ) + defined ( HPI_INCLUDE_8800 ) + defined ( HPI_INCLUDE_8600 ) )
{
HPI_PCI_VENDOR_ID_TI, HPI_ADAPTER_PCI2040,
	    HPI_PCI_VENDOR_ID_AUDIOSCIENCE, PCI_ANY_ID, 0, 0, HPI_6000}
,
#endif
#if ( defined ( HPI_INCLUDE_4100 ) + defined ( HPI_INCLUDE_4300 ) + defined ( HPI_INCLUDE_4400 ) + defined ( HPI_INCLUDE_4500 ) + defined ( HPI_INCLUDE_4600 ) )
{
HPI_PCI_VENDOR_ID_MOTOROLA, HPI_ADAPTER_DSP56301,
	    HPI_PCI_VENDOR_ID_AUDIOSCIENCE, PCI_ANY_ID, 0, 0, HPI_4000}
,
// look for ASI cards that have 0x12cf sub-vendor ID, like the 4300 and 4601
{
HPI_PCI_VENDOR_ID_MOTOROLA, HPI_ADAPTER_DSP56301, 0x12CF, PCI_ANY_ID, 0,
	    0, HPI_4000}
,
// look for ASI cards that have sub-vendor-ID = 0, like the 4501, 4113 and 4215 revC and below
{
HPI_PCI_VENDOR_ID_MOTOROLA, HPI_ADAPTER_DSP56301, 0, PCI_ANY_ID, 0, 0,
	    HPI_4000}
,
#endif
{
0,}

///////////////////////////////////////////////////////////////////////////
