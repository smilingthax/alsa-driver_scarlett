/*
 *  Copyright (c) 1994-98 by Jaroslav Kysela <perex@suse.cz>
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
#define SND_CARD_TYPE_GUS_CLASSIC	0x00000001
#define SND_CARD_TYPE_GUS_EXTREME	0x00000002
#define SND_CARD_TYPE_GUS_ACE		0x00000003
#define SND_CARD_TYPE_GUS_MAX		0x00000004
#define SND_CARD_TYPE_AMD_INTERWAVE	0x00000005
/* Sound Blaster */
#define SND_CARD_TYPE_SB_10		0x00000006
#define SND_CARD_TYPE_SB_20		0x00000007
#define SND_CARD_TYPE_SB_PRO		0x00000008
#define SND_CARD_TYPE_SB_16		0x00000009
#define SND_CARD_TYPE_SB_AWE		0x0000000a
/* Various */
#define SND_CARD_TYPE_ESS_ES1688	0x0000000b	/* ESS AudioDrive ESx688 */
#define SND_CARD_TYPE_OPL3_SA		0x0000000c	/* Yamaha OPL3 SA */
#define SND_CARD_TYPE_MOZART		0x0000000d	/* OAK Mozart */
#define SND_CARD_TYPE_S3_SONICVIBES	0x0000000e	/* S3 SonicVibes */
#define SND_CARD_TYPE_ENS1370		0x0000000f	/* Ensoniq ES1370 */
#define SND_CARD_TYPE_ENS1371		0x00000010	/* Ensoniq ES1371 */
#define SND_CARD_TYPE_CS4232		0x00000011	/* CS4232/CS4232A */
#define SND_CARD_TYPE_CS4236		0x00000012	/* CS4235/CS4236B/CS4237B/CS4238B/CS4239 */
#define SND_CARD_TYPE_AMD_INTERWAVE_STB	0x00000013	/* AMD InterWave + TEA6330T */
#define SND_CARD_TYPE_ESS_ES1938	0x00000014	/* ESS Solo-1 ES1938 */
#define SND_CARD_TYPE_ESS_ES18XX	0x00000015	/* ESS AudioDrive ES18XX */
#define SND_CARD_TYPE_CS4231		0x00000016      /* CS4231 */
#define SND_CARD_TYPE_OPTI92X		0x00000017	/* OPTi 92x chipset */
#define SND_CARD_TYPE_SERIAL		0x00000018	/* Serial MIDI driver */
#define SND_CARD_TYPE_AD1848		0x00000019	/* Generic AD1848 driver */
#define SND_CARD_TYPE_TRID4DWAVEDX	0x0000001A	/* Trident 4DWave DX */
#define SND_CARD_TYPE_TRID4DWAVENX	0x0000001B	/* Trident 4DWave NX */
#define SND_CARD_TYPE_SGALAXY           0x0000001C      /* Aztech Sound Galaxy */
#define SND_CARD_TYPE_CS461X		0x0000001D	/* Sound Fusion CS4610/12/15 */
/* Turtle Beach WaveFront series */
#define SND_CARD_TYPE_WAVEFRONT         0x0000001E      /* TB WaveFront generic */
#define SND_CARD_TYPE_TROPEZ            0x0000001F      /* TB Tropez */
#define SND_CARD_TYPE_TROPEZPLUS        0x00000020      /* TB Tropez+ */
#define SND_CARD_TYPE_MAUI              0x00000021      /* TB Maui */
#define SND_CARD_TYPE_CMI8330           0x00000022      /* C-Media CMI8330 */
/* Various */
#define SND_CARD_TYPE_DUMMY		0x00000023	/* dummy soundcard */
#define SND_CARD_TYPE_ALS100		0x00000024	/* Avance Logic ALS100 */
#define SND_CARD_TYPE_SHARE		0x00000025	/* share soundcard */
#define SND_CARD_TYPE_SI_7018		0x00000026	/* SiS 7018 */
#define SND_CARD_TYPE_OPTI93X		0x00000027	/* OPTi 93x chipset */
#define SND_CARD_TYPE_MTPAV		0x00000028	/* MOTU MidiTimePiece AV multiport MIDI */
#define SND_CARD_TYPE_VIRMIDI		0x00000029	/* Virtual MIDI */
#define SND_CARD_TYPE_EMU10K1		0x0000002a	/* EMU10K1 */
#define SND_CARD_TYPE_HAMMERFALL	0x0000002b	/* RME Digi9652  */
#define SND_CARD_TYPE_HAMMERFALL_LIGHT	0x0000002c	/* RME Digi9652, but no expansion card */
#define SND_CARD_TYPE_ICE1712		0x0000002d	/* ICE1712 */
/* --- */
#define SND_CARD_TYPE_LAST		0x0000002d

#endif				/* __ASOUNDID_H */
