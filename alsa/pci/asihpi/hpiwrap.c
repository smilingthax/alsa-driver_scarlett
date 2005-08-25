/******************************************************************************
Copyright (C) 1997-2003 AudioScience, Inc. All rights reserved.

This software is provided 'as-is', without any express or implied warranty.
In no event will AudioScience Inc. be held liable for any damages arising
from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This copyright notice and list of conditions may not be altered or removed 
   from any source distribution.

AudioScience, Inc. <support@audioscience.com>

( This license is GPL compatible see http://www.gnu.org/licenses/license-list.html#GPLCompatibleLicenses )

ASI Linux Hardware Programming Interface (HPI) Wrapper
This module contains the main HPI entry point HPI_Message which uses ioctl
to pass the message to the driver.

******************************************************************************/
#include <hpi.h>
#include <hpios.h>
#include <hpidebug.h>
#include <unistd.h>		/* for close */
#include <fcntl.h>		/* for open */
#include <sys/ioctl.h>		/* for ioctl */
#include <stdio.h>

/* Generic HPI device.  */
#define HPI_DEVICE_NAME "/dev/asihpi"

HW16 HPI_DriverOpen(HPI_HSUBSYS *phSubSys)
{
    int fd;

    /* We could force the loading of the driver module here.  */

    // HPI_PRINT_INFO ("Opening %s...\n", HPI_DEVICE_NAME);

    fd = open(HPI_DEVICE_NAME, O_RDWR);
    phSubSys->nHandle = fd;
    phSubSys->nOs=0; // unused for Linux
    phSubSys->dwIoctlCode=0; // unused for Linux

    if (fd < 0)
    {
	if (hpiDebugLevel)
	    perror("HPI open error");
	return 0;
    }
    return 1;
}

void HPI_Message(const HPI_HSUBSYS *phSubSys,
		 HPI_MESSAGE *phm,
		 HPI_RESPONSE *phr)
{
    int status;
    struct hpi_ioctl_linux hpi_ioctl_data;

    /* Check that device opened...  */
    if (phSubSys->nHandle == 0 && !HPI_DriverOpen((HPI_HSUBSYS *)phSubSys))
	return;

    if (phSubSys->nHandle < 0)
	return;

    hpi_ioctl_data.phm = phm;
    hpi_ioctl_data.phr = phr;
    HPI_DEBUG_MESSAGE (phm);

    status = ioctl(phSubSys->nHandle, HPI_IOCTL_LINUX,
		   (unsigned long)&hpi_ioctl_data);
    if (status < 0 && hpiDebugLevel)
	perror("HPI ioctl error");
    HPI_DEBUG_RESPONSE(phr);
}


void HPI_DriverClose(HPI_HSUBSYS *phSubSys)
{
    int status;

    /* Check that device opened...  */
    if (phSubSys->nHandle <= 0)
	return;

    status = close(phSubSys->nHandle);
    if (status < 0 && hpiDebugLevel)
	perror("HPI close error");
    phSubSys->nHandle = -1;
}
