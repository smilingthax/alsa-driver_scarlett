/*
 *  Copyright (c) 1994-98 by Jaroslav Kysela <perex@jcu.cz>
 *
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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __ASOUNDID_H
#define __ASOUNDID_H

/*
 *  Types of soundcards...
 *  Note: Don't assign new number to 100% clones...
 *  Note: Order shouldn't be preserved, but assigment must be!!!
 */

/* Gravis UltraSound */
#define SND_CARD_TYPE_GUS_CLASSIC	1
#define SND_CARD_TYPE_GUS_EXTREME	2
#define SND_CARD_TYPE_GUS_ACE		3
#define SND_CARD_TYPE_GUS_MAX		4
#define SND_CARD_TYPE_AMD_INTERWAVE	5
/* Sound Blaster */
#define SND_CARD_TYPE_SB_10		6
#define SND_CARD_TYPE_SB_20		7
#define SND_CARD_TYPE_SB_PRO		8
#define SND_CARD_TYPE_SB_16		9
#define SND_CARD_TYPE_SB_AWE		10
/* Various */
#define SND_CARD_TYPE_ESS_ES1688	11	/* ESS AudioDrive ESx688 */
#define SND_CARD_TYPE_OPL3_SA		12	/* Yamaha OPL3 SA */
#define SND_CARD_TYPE_MOZART		13	/* OAK Mozart */
#define SND_CARD_TYPE_S3_SONICVIBES	14	/* S3 SonicVibes */
#define SND_CARD_TYPE_ES1370		15	/* Ensoniq ES1370 */
#define SND_CARD_TYPE_ES1371		16	/* Ensoniq ES1371 */
/* --- */
#define SND_CARD_TYPE_LAST		16

#endif /* __ASOUNDID_H */
