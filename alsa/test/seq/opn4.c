/*
 *  Simple test for ALSA sequencer
 *  Copyright (c) 1998 by Frank van de Pol <frank@vande-pol.demon.nl>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "seq.h"

void main(void)
{
	int f;
	snd_seq_client_info_t inf;
	snd_seq_port_info_t   pinf;
	int myid;
	int c,p;

	f = open("/dev/sndseq", O_RDWR);

	if (ioctl(f, SND_SEQ_IOCTL_CLIENT_ID, &myid) < 0) {
		perror("ioctl");
		exit(1);
	}
	printf("My client id = %d\n", myid);

	/* set name */
	memset(&inf, 0, sizeof(snd_seq_client_info_t));
	strcpy(inf.name, "Test Program");
	if (ioctl(f, SND_SEQ_IOCTL_SET_CLIENT_INFO, &inf) < 0) {
		perror("ioctl");
		exit(1);
	}
	
	/* probe what clients are present */
	for (c = 0; c < SND_SEQ_MAX_CLIENTS; c++) {
		inf.client = c;
		if (ioctl(f, SND_SEQ_IOCTL_GET_CLIENT_INFO, &inf) < 0) {
			perror("ioctl");
			exit(1);
		}
		if (inf.type) {
			printf("%d : %s\n", inf.client, inf.name);
			
			/* list ports */
			for (p=0; p < 255; p++) {
				pinf.client = c;
				pinf.port = p;
				if (ioctl(f, SND_SEQ_IOCTL_GET_PORT_INFO, &pinf) >=0 ) {
					printf("   %s, %d, %d\n",
						pinf.name,
						pinf.capability,
						pinf.port_type);
				}
				
			}
		}
	}
	close(f);
}
