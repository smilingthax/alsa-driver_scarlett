/*
 *   Additional mixer mapping
 *
 *   Copyright (c) 2002 by Takashi Iwai <tiwai@suse.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */


struct usbmix_name_map {
	int id;
	const char *name;
};

struct usbmix_ctl_map {
	int vendor;
	int product;
	const struct usbmix_name_map *map;
};
	
/*
 * USB control mappers for SB Exitigy
 */

/*
 * Topology of SB Extigy (see on the wide screen :)

USB_IN[1] --->FU[2] -----------------------------+->MU[16] - PE[17]-+- FU[18] -+- EU[27] -+- EU[21] - FU[22] -+- FU[23] > Dig_OUT[24]
                                                 |                  |          |          |                   |
USB_IN[3] -+->SU[5] - FU[6] -+- MU[14] - PE[15] -+                  |          |          |                   +- FU[25] > Dig_OUT[26]
           ^                 |                   |                  |          |          |
Dig_IN[4] -+                 |                   |                  |          |          +- FU[28] --------------------> Spk_OUT[19]
                             |                   |                  |          |
Lin-IN[7] -+-- FU[8] --------+                   |                  |          +----------------------------------------> Hph_OUT[20]
           |                                     |                  |
Mic-IN[9] --+- FU[10] ---------------------------+                  |
           ||                                                       |
           ||  +----------------------------------------------------+
           VV  V
           ++--+->SU[11] ->FU[12] --------------------------------------------------------------------------------------> USB_OUT[13]
*/

static struct usbmix_name_map extigy_map[] = {
	{ 2, "PCM Playback" }, /* FU */
	{ 5, "Digital Playback Source" }, /* SU */
	{ 6, "Digital" }, /* FU */
	{ 8, "Line Playback" }, /* FU */
	{ 10, "Mic Playback" }, /* FU */
	{ 11, "Capture Source" }, /* SU */
	{ 12, "Capture" }, /* FU */
	{ 18, "Master Playback" }, /* FU */
	/* FIXME: more to come here... */
	{ 0 } /* terminator */
};


/*
 * Control map entries
 */

static struct usbmix_ctl_map usbmix_ctl_maps[] = {
	{ 0x41e, 0x3000, extigy_map },
	{ 0 } /* terminator */
};

