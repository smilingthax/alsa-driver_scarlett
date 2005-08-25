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

 Hardware Programming Interface (HPI) functions

 (C) Copyright AudioScience Inc. 1996,1997, 1998
*******************************************************************************/

#include <hpi.h>

#ifndef HPI_KERNEL_MODE
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>   // for log10 in meter get level
#endif

// local prototypes
#ifdef HPI_WDM_MONOLITHIC
void HPIMSGX_MessageEx(const HPI_HSUBSYS *phSubSys,HPI_MESSAGE *phm,HPI_RESPONSE *phr,PLONG hOwner);
#define HPI_Message( a, b, c ) HPIMSGX_MessageEx((a),(b),(c),(PLONG)NULL)
#endif

void HPI_InitMessage( HPI_MESSAGE *phm, HW16 wObject, HW16 wFunction );
HW32 HPI_IndexesToHandle( const char cObject, const HW16 wIndex1, const HW16 wIndex2 );
void HPI_HandleToIndexes( const HW32 dwHandle, HW16 *pwIndex1, HW16 *pwIndex2 );
HW32 HPI_IndexesToHandle3( const char cObject,const HW16 wIndex1, const HW16 wIndex2, const HW16 wIndex0 );
void HPI_HandleToIndexes3( const HW32 dwHandle, HW16 *pwIndex1, HW16 *pwIndex2 , HW16 *pwIndex0 );

#ifdef HAS_AES18_ON_HOST
void AES18_Message(int wControlType, HPI_MESSAGE *phm, HPI_RESPONSE *phr);
void Aes18_Init(void);
#endif

#define HPI_HANDLETOINDEXES(h,i1,i2) if (h==0) return HPI_ERROR_INVALID_OBJ; else HPI_HandleToIndexes(h,i1,i2)
#define HPI_HANDLETOINDEXES3(h,i1,i2,i0) if (h==0) return HPI_ERROR_INVALID_OBJ; else HPI_HandleToIndexes3(h,i1,i2,i0)

/// global to store specifc errors for use by HPI_GetLastErrorDetail
// HW16 gwHpiLastError=0;
HW32 gadwHpiSpecificError[8]={0,0,0,0,0,0,0,0};

///////////////////////////////////////////////////////////////////////////
/**\defgroup subsys HPI Subsystem
\{
*/

static HPI_HSUBSYS ghSubSys;   //global!!!!!!!!!!!!!!!!!!!

HPI_HSUBSYS* HPI_SubSysCreate( void )
{
    HPI_MESSAGE     hm;
    HPI_RESPONSE    hr;

	memset(&ghSubSys,0,sizeof(HPI_HSUBSYS));
#ifndef HPI_KERNEL_MODE //----------------- not Win95,WinNT,WDM kernel

    if (HPI_DriverOpen(&ghSubSys))
#endif

    {
		  HPI_InitMessage(
				&hm,
				HPI_OBJ_SUBSYSTEM,
				HPI_SUBSYS_OPEN);
		  HPI_Message( &ghSubSys, &hm, &hr );

        if (hr.wError==0)
            return( &ghSubSys );
#ifndef HPI_KERNEL_MODE //----------------- not Win95,WinNT,WDM kernel

        else
            HPI_DriverClose(&ghSubSys);
#endif

    }
    return(NULL);
}

void HPI_SubSysFree(
    HPI_HSUBSYS *phSubSys
)
{
    HPI_MESSAGE     hm;
	 HPI_RESPONSE    hr;

    // tell HPI to shutdown
	 HPI_InitMessage(
		  &hm,
		  HPI_OBJ_SUBSYSTEM,
        HPI_SUBSYS_CLOSE
    );
    HPI_Message( phSubSys, &hm, &hr );

#ifndef HPI_KERNEL_MODE // not Win95,WinNT,WDM kernel

    HPI_DriverClose(phSubSys);
#endif
}



HW16 HPI_SubSysGetVersion(
    HPI_HSUBSYS *phSubSys,
    HW32 *pdwVersion
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    HPI_InitMessage( &hm,  HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_GET_VERSION );
    HPI_Message( phSubSys, &hm, &hr );
    *pdwVersion = hr.u.s.dwVersion;
    return(hr.wError);
}

HW16 HPI_SubSysGetInfo(
    HPI_HSUBSYS *phSubSys,
    HW32 *pdwVersion,
    HW16 *pwNumAdapters,
    HW16 awAdapterList[],
	 HW16 wListLength
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_GET_INFO );

    HPI_Message( phSubSys, &hm, &hr );

    *pdwVersion = hr.u.s.dwVersion;
    if(wListLength>HPI_MAX_ADAPTERS)
        memcpy( awAdapterList, &hr.u.s.awAdapterList, HPI_MAX_ADAPTERS);
    else
        memcpy( awAdapterList, &hr.u.s.awAdapterList, wListLength);
    *pwNumAdapters = hr.u.s.wNumAdapters;
    return(hr.wError);
}

HW16 HPI_SubSysCreateAdapter(
    HPI_HSUBSYS *phSubSys,
    HPI_RESOURCE *pResource,
    HW16            *pwAdapterIndex
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_CREATE_ADAPTER );
    memcpy( &hm.u.s.Resource, pResource, sizeof(HPI_RESOURCE) );

    HPI_Message( phSubSys, &hm, &hr );

    *pwAdapterIndex = hr.u.s.wAdapterIndex;
    return(hr.wError);
}

HW16 HPI_SubSysDeleteAdapter(
    HPI_HSUBSYS *phSubSys,
	 HW16          wAdapterIndex
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_DELETE_ADAPTER );
    hm.wAdapterIndex=wAdapterIndex;
    HPI_Message( phSubSys, &hm, &hr );
    return(hr.wError);
}

HW16 HPI_SubSysFindAdapters(
    HPI_HSUBSYS *phSubSys,
    HW16 *pwNumAdapters,
    HW16 awAdapterList[],
    HW16 wListLength
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_FIND_ADAPTERS );

    HPI_Message( phSubSys, &hm, &hr );

    if(wListLength>HPI_MAX_ADAPTERS) {
        memcpy( awAdapterList, &hr.u.s.awAdapterList, HPI_MAX_ADAPTERS*sizeof(HW16));
		memset( &awAdapterList[HPI_MAX_ADAPTERS],0, (wListLength-HPI_MAX_ADAPTERS)*sizeof(HW16));
    } else
        memcpy( awAdapterList, &hr.u.s.awAdapterList, wListLength*sizeof(HW16));
    *pwNumAdapters = hr.u.s.wNumAdapters;

    return(hr.wError);
}

/// port read/write functions (for use through a VxD or Kernel driver)
HW16 HPI_SubSysReadPort8(
    HPI_HSUBSYS *phSubSys,
    HW16    wAddress,
	 HW16    *pwData
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_READ_PORT_8 );
    hm.u.s.Resource.r.PortIO.dwAddress = (HW32)wAddress;

    HPI_Message( phSubSys, &hm, &hr );

    *pwData = (HW16)hr.u.s.dwData;
    return(hr.wError);
}

HW16 HPI_SubSysWritePort8(
    HPI_HSUBSYS *phSubSys,
    HW16    wAddress,
    HW16    wData
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_SUBSYSTEM, HPI_SUBSYS_WRITE_PORT_8 );
    hm.u.s.Resource.r.PortIO.dwAddress = (HW32)wAddress;
    hm.u.s.Resource.r.PortIO.dwData = (HW32)wData;

    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);
}
///\}
///////////////////////////////////////////////////////////////////////////
/** \defgroup adapter Adapter

@{
*/
HW16 HPI_AdapterOpen(
    HPI_HSUBSYS *phSubSys,
    HW16 wAdapterIndex
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_ADAPTER, HPI_ADAPTER_OPEN );
    hm.wAdapterIndex = wAdapterIndex;

    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);

}

HW16 HPI_AdapterClose(
    HPI_HSUBSYS *phSubSys,
    HW16 wAdapterIndex
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_ADAPTER, HPI_ADAPTER_CLOSE );
    hm.wAdapterIndex = wAdapterIndex;

    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);
}

/*!
	Given an object type HPI_OBJ_*, index, and adapter index,
	Determine if the object exists, and if so the dsp index of the object.
	Implementation is non-trivial only for multi-DSP adapters

	/return HW16 an error code HPI_ERROR_*
*/
HW16 HPI_AdapterFindObject(
    const HPI_HSUBSYS *phSubSys,
    HW16        wAdapterIndex,  //< Index of adapter to search
	 HW16        wObjectType,    //< Type of object HPI_OBJ_*
    HW16        wObjectIndex,	//< Index of object
    HW16 *pDspIndex				//< Output the index of the DSP containing the object
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    // contruct the HPI message from the function parameters
    HPI_InitMessage( &hm,  HPI_OBJ_ADAPTER, HPI_ADAPTER_FIND_OBJECT );
    hm.wAdapterIndex = wAdapterIndex;
    hm.u.a.wAssertId = wObjectIndex;
    hm.u.a.wObjectType = wObjectType;

    HPI_Message( phSubSys, &hm, &hr );   // Find out where the object is
    /* HPIs that serve only a single DSP can process this message on the PC, returning
         0 in wDspIndex */

	if (hr.wError==0)
		*pDspIndex = hr.u.a.wAdapterIndex;
	else if (hr.wError==HPI_ERROR_INVALID_FUNC)
	{
		// for backwards compatibility
		*pDspIndex = 0;
		hr.wError = 0;
	}

	return hr.wError;
}

HW16 HPI_AdapterSetMode(
	 HPI_HSUBSYS *phSubSys,
	 HW16 wAdapterIndex,
	 HW32 dwAdapterMode
)
{
	 return HPI_AdapterSetModeEx(phSubSys,wAdapterIndex,dwAdapterMode,HPI_ADAPTER_MODE_SET);
}

HW16 HPI_AdapterSetModeEx(
	 HPI_HSUBSYS *phSubSys,
	 HW16 wAdapterIndex,
	 HW32 dwAdapterMode,
	 HW16 wQueryOrSet
)
{
	 HPI_MESSAGE hm;
	 HPI_RESPONSE hr;
	 HPI_InitMessage( &hm,  HPI_OBJ_ADAPTER, HPI_ADAPTER_SET_MODE );
	 hm.wAdapterIndex = wAdapterIndex;
	 hm.u.a.dwAdapterMode = dwAdapterMode;
	 hm.u.a.wAssertId = wQueryOrSet;
	 HPI_Message( phSubSys, &hm, &hr );
	 return(hr.wError);
}

HW16 HPI_AdapterGetMode(
    HPI_HSUBSYS *phSubSys,
    HW16 wAdapterIndex,
    HW32 *pdwAdapterMode
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_ADAPTER, HPI_ADAPTER_GET_MODE );
    hm.wAdapterIndex = wAdapterIndex;
    HPI_Message( phSubSys, &hm, &hr );
    if(pdwAdapterMode)
        *pdwAdapterMode = hr.u.a.dwSerialNumber;
    return(hr.wError);
}

HW16 HPI_AdapterGetInfo(
    HPI_HSUBSYS *phSubSys,
    HW16 wAdapterIndex,
    HW16 *pwNumOutStreams,
    HW16 *pwNumInStreams,
    //HW8   szAdapterName[],
	 //HW16 wStringLen
    HW16 *pwVersion,
	 HW32 *pdwSerialNumber,
    HW16 *pwAdapterType
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_ADAPTER, HPI_ADAPTER_GET_INFO );
    hm.wAdapterIndex = wAdapterIndex;

    HPI_Message( phSubSys, &hm, &hr );

    *pwAdapterType = hr.u.a.wAdapterType;
    *pwNumOutStreams = hr.u.a.wNumOStreams;
    *pwNumInStreams = hr.u.a.wNumIStreams;
    *pwVersion = hr.u.a.wVersion;
    *pdwSerialNumber = hr.u.a.dwSerialNumber;
    //strncpy( szAdapterName, hr.u.a.szAdapterName, wStringLen );
    return(hr.wError);
}

/*

OR

Gets various attributes of an adapter, such as serial number

HW16 HPI_AdapterGetAttribute(
        HPI_HSUBSYS *phSubSys,
          HW16    wAdapterIndex,      // 0..N-1
        HW16    wAttribute,         // HPI_ATTRIBUTE_SERIALNUMBER
        HW32    *pdwValue
          )
{
}
*/

HW16 HPI_AdapterGetAssert(
	 HPI_HSUBSYS *phSubSysHandle,
    HW16    wAdapterIndex,
    HW16    *wAssertPresent,
    char    *pszAssert,
    HW16    *pwLineNumber
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_ADAPTER, HPI_ADAPTER_GET_ASSERT );
    hm.wAdapterIndex = wAdapterIndex;
    HPI_Message( phSubSysHandle, &hm, &hr );

	 // no assert
	 *wAssertPresent = 0;

	 if(!hr.wError)
    {
        *pwLineNumber = (HW16)hr.u.a.dwSerialNumber;    // send line number back in the dwSerialNumber field
        if(*pwLineNumber)
        {
            // we have an assert
            int i;
            char *Src = (char *)hr.u.a.szAdapterAssert;
            char *Dst = pszAssert;

            *wAssertPresent = 1;

            // simple version of strncpy (saves linking to a library)
            for(i=0;i<STR_SIZE(HPI_STRING_LEN); i++)
            {
                char c;
                c= *Src++;
                *Dst++ = c;
                if(c==0)
                    break;
            }

        }
	 }
    return(hr.wError);
}

/** Extended Get Assert
adds 32 bit 'line number' and dsp index to standard assert

\return HW16 error code HPI_ERROR_*
*/
HW16 HPI_AdapterGetAssertEx(
	 HPI_HSUBSYS *phSubSysHandle, ///< HPI subsystem handle
	 HW16    wAdapterIndex,   ///<  Adapter to query
	 HW16    *wAssertPresent, ///< OUT* The number of asserts waiting including this one
    char    *pszAssert,      ///< OUT* Assert message, traditionally file name
    HW32    *pdwLineNumber,  ///< OUT* Assert number, traditionally line number in file
    HW16    *pwAssertOnDsp   ///< OUT* The index of the DSP that generated the assert
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
	 HPI_InitMessage( &hm,  HPI_OBJ_ADAPTER,      HPI_ADAPTER_GET_ASSERT );
    hm.wAdapterIndex = wAdapterIndex;

	 HPI_Message( phSubSysHandle, &hm, &hr );

	 *wAssertPresent=0;

	 if(!hr.wError)
	 {
		  *pdwLineNumber = hr.u.a.dwSerialNumber; // send line number back in the dwSerialNumber field
			*wAssertPresent= hr.u.a.wAdapterType;   // count of asserts BEFORE getting this one
			*pwAssertOnDsp = hr.u.a.wAdapterIndex;  // DSP where error occurred

			if (!*wAssertPresent && *pdwLineNumber)
				*wAssertPresent=1;   // for backward compatibility. See also hpi4000.c HPI_4000()

		  if (*wAssertPresent)
        {
            // we have an assert
				int i;
            char *Src = (char *)hr.u.a.szAdapterAssert;
            char *Dst = pszAssert;

            // simple version of strncpy (saves linking to a library)
            for(i=0;i<STR_SIZE(HPI_STRING_LEN); i++)
            {
                char c;
                c= *Src++;
                *Dst++ = c;
                if(c==0)
                    break;
            }

        }
    }
    return(hr.wError);
}


HW16 HPI_AdapterTestAssert(
    HPI_HSUBSYS *phSubSysHandle,
    HW16    wAdapterIndex,
    HW16    wAssertId
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_ADAPTER, HPI_ADAPTER_TEST_ASSERT );
    hm.wAdapterIndex = wAdapterIndex;
    hm.u.a.wAssertId  = wAssertId;

    HPI_Message( phSubSysHandle, &hm, &hr );

    return(hr.wError);
}

HW16 HPI_AdapterEnableCapability(
    HPI_HSUBSYS *phSubSysHandle,
    HW16 wAdapterIndex,
    HW16 wCapability,
    HW32 dwKey
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_ADAPTER, HPI_ADAPTER_ENABLE_CAPABILITY );
    hm.wAdapterIndex = wAdapterIndex;
    hm.u.a.wAssertId=wCapability;
    hm.u.a.dwAdapterMode=dwKey;

	 HPI_Message( phSubSysHandle, &hm, &hr );

    return(hr.wError);
}

HW16 HPI_AdapterSelfTest(
    HPI_HSUBSYS *phSubSysHandle,
    HW16 wAdapterIndex
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_ADAPTER, HPI_ADAPTER_SELFTEST );
    hm.wAdapterIndex = wAdapterIndex;
    HPI_Message( phSubSysHandle, &hm, &hr );
    return(hr.wError);
}

/** @} */

/** \defgroup stream Streams
Perform audio I/O and format conversion
@{
*/
HW16 HPI_StreamEstimateBufferSize(
					 HPI_FORMAT *pF,
					 HW32 dwHostPollingRateInMilliSeconds,
					 HW32 *dwRecommendedBufferSize)
{
	 // compute bytes per second
	 HW32 dwBytesPerSecond;
	 HW32 dwSize;

	 switch(pF->wFormat)
	 {
	 case HPI_FORMAT_PCM16_BIGENDIAN:
	 case HPI_FORMAT_PCM16_SIGNED:
		 dwBytesPerSecond = pF->dwSampleRate * 2L * pF->wChannels;
		 break;
	 case HPI_FORMAT_PCM24_SIGNED:
		 dwBytesPerSecond = pF->dwSampleRate * 3L * pF->wChannels;
		 break;
	 case HPI_FORMAT_PCM32_SIGNED:
	 case HPI_FORMAT_PCM32_FLOAT:
		 dwBytesPerSecond = pF->dwSampleRate * 4L * pF->wChannels;
		 break;
	 case HPI_FORMAT_PCM8_UNSIGNED:
		 dwBytesPerSecond = pF->dwSampleRate * 1L * pF->wChannels;
		 break;
	 case HPI_FORMAT_MPEG_L1:
	 case HPI_FORMAT_MPEG_L2:
	 case HPI_FORMAT_MPEG_L3:
		 dwBytesPerSecond = pF->dwBitRate/8L;
		 break;
	 case HPI_FORMAT_DOLBY_AC2:
		// Dolby AC-2 is around 256 kBytes/second
		 dwBytesPerSecond = 256000L/8L;
		  break;
	 default:
		  return HPI_ERROR_INVALID_FORMAT;
	 }
	 dwSize =  (dwBytesPerSecond * dwHostPollingRateInMilliSeconds * 2)/1000L;
	 *dwRecommendedBufferSize = (dwSize + 4095L) & ~4095L;	// round up to nearest 4 K bounday.
	 return 0;
}

////////////////////////////////////////////////////////////////////////////
/** \defgroup outstream Ouput Stream

@{
*/
HW16 HPI_OutStreamOpen(
	 HPI_HSUBSYS *phSubSys,
    HW16 wAdapterIndex,
    HW16 wOutStreamIndex,
    HPI_HOSTREAM *phOutStream
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_OSTREAM, HPI_OSTREAM_OPEN );
    hm.wAdapterIndex = wAdapterIndex;
    hm.u.d.wOStreamIndex = wOutStreamIndex;

    HPI_Message( phSubSys, &hm, &hr );

    if(hr.wError==0)
        *phOutStream = HPI_IndexesToHandle( 'O', wAdapterIndex, wOutStreamIndex );
    else
        *phOutStream = 0;
    return(hr.wError);
}

HW16 HPI_OutStreamClose(
    HPI_HSUBSYS *phSubSys,
    HPI_HOSTREAM hOutStream
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    HPI_InitMessage( &hm,  HPI_OBJ_OSTREAM, HPI_OSTREAM_HOSTBUFFER_FREE);
    HPI_HANDLETOINDEXES( hOutStream, &hm.wAdapterIndex, &hm.u.d.wOStreamIndex );
    HPI_Message( phSubSys, &hm, &hr );

    HPI_InitMessage( &hm,  HPI_OBJ_OSTREAM, HPI_OSTREAM_CLOSE );
    HPI_HANDLETOINDEXES( hOutStream, &hm.wAdapterIndex, &hm.u.d.wOStreamIndex );
    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);
}
/**
 \deprecated This function has been superseded by HPI_OutStreamGetInfoEx()
*/
HW16 HPI_OutStreamGetInfo(
	 HPI_HSUBSYS *phSubSys,
    HPI_HOSTREAM hOutStream,
    HW16 *pwState,
    HW32 *pdwBufferSize,
    HW32 *pdwDataToPlay
)
{
    return(
              HPI_OutStreamGetInfoEx(
                  phSubSys,
                  hOutStream,
                  pwState,
                  pdwBufferSize,
                  pdwDataToPlay,
                  NULL,
                  NULL)
          );
}

/** Get information about attributes and state of output stream
*/

HW16 HPI_OutStreamGetInfoEx(
    HPI_HSUBSYS     *phSubSys,
    HPI_HOSTREAM    hOutStream,
    HW16            *pwState,
    HW32            *pdwBufferSize,
    HW32            *pdwDataToPlay,
    HW32            *pdwSamplesPlayed,
    HW32            *pdwAuxiliaryDataToPlay)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_OSTREAM, HPI_OSTREAM_GET_INFO );
    HPI_HANDLETOINDEXES( hOutStream, &hm.wAdapterIndex, &hm.u.d.wOStreamIndex );

    HPI_Message( phSubSys, &hm, &hr );

    // only send back data if valid pointers supplied!!
    if(pwState)
		  *pwState = hr.u.d.wState;
    if(pdwBufferSize)
        *pdwBufferSize = hr.u.d.dwBufferSize;
    if(pdwDataToPlay)
        *pdwDataToPlay = hr.u.d.dwDataAvailable;
    if(pdwSamplesPlayed)
        *pdwSamplesPlayed = hr.u.d.dwSamplesTransfered;
    if(pdwAuxiliaryDataToPlay)
        *pdwAuxiliaryDataToPlay = hr.u.d.dwAuxilaryDataAvailable;
    return(hr.wError);
}


HW16 HPI_OutStreamWrite(
    HPI_HSUBSYS *phSubSys,
    HPI_HOSTREAM hOutStream,
    HPI_DATA *pData
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_OSTREAM, HPI_OSTREAM_WRITE );
    HPI_HANDLETOINDEXES( hOutStream, &hm.wAdapterIndex, &hm.u.d.wOStreamIndex );
    memcpy( &hm.u.d.u.Data, pData, sizeof(HPI_DATA) );

    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);
}

HW16 HPI_OutStreamStart(
    HPI_HSUBSYS *phSubSys,
    HPI_HOSTREAM hOutStream
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
	 HPI_InitMessage( &hm,  HPI_OBJ_OSTREAM, HPI_OSTREAM_START );
    HPI_HANDLETOINDEXES( hOutStream, &hm.wAdapterIndex, &hm.u.d.wOStreamIndex );

    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);
}

HW16 HPI_OutStreamStop(
    HPI_HSUBSYS *phSubSys,
    HPI_HOSTREAM hOutStream
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_OSTREAM, HPI_OSTREAM_STOP );
    HPI_HANDLETOINDEXES( hOutStream, &hm.wAdapterIndex, &hm.u.d.wOStreamIndex );

    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);
}

HW16 HPI_OutStreamSinegen(
    HPI_HSUBSYS *phSubSys,
    HPI_HOSTREAM hOutStream
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_OSTREAM, HPI_OSTREAM_SINEGEN );
    HPI_HANDLETOINDEXES( hOutStream, &hm.wAdapterIndex, &hm.u.d.wOStreamIndex );

    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);
}

HW16 HPI_OutStreamReset(
    HPI_HSUBSYS *phSubSys,
    HPI_HOSTREAM hOutStream
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_OSTREAM, HPI_OSTREAM_RESET );
    HPI_HANDLETOINDEXES( hOutStream, &hm.wAdapterIndex, &hm.u.d.wOStreamIndex );

    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);
}


HW16 HPI_OutStreamQueryFormat(
    HPI_HSUBSYS     *phSubSys,
    HPI_HOSTREAM    hOutStream,
    HPI_FORMAT  *pFormat)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    HPI_InitMessage( &hm,  HPI_OBJ_OSTREAM, HPI_OSTREAM_QUERY_FORMAT );
    HPI_HANDLETOINDEXES( hOutStream, &hm.wAdapterIndex, &hm.u.d.wOStreamIndex );
    memcpy( &hm.u.d.u.Data.Format, pFormat, sizeof(HPI_FORMAT) );

    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);
}

// SGT feb-26-99
HW16 HPI_OutStreamSetVelocity(
    HPI_HSUBSYS     *phSubSys,
	 HPI_HOSTREAM    hOutStream,
    short           nVelocity)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    HPI_InitMessage( &hm,  HPI_OBJ_OSTREAM, HPI_OSTREAM_SET_VELOCITY );
    HPI_HANDLETOINDEXES( hOutStream, &hm.wAdapterIndex, &hm.u.d.wOStreamIndex );
    hm.u.d.u.wVelocity = nVelocity;

    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);
}

HW16 HPI_OutStreamSetPunchInOut(
    HPI_HSUBSYS     *phSubSys,
    HPI_HOSTREAM    hOutStream,
    HW32            dwPunchInSample,
    HW32            dwPunchOutSample )
{
	 HPI_MESSAGE hm;
	 HPI_RESPONSE hr;

	 HPI_InitMessage( &hm,  HPI_OBJ_OSTREAM, HPI_OSTREAM_SET_PUNCHINOUT );
	 HPI_HANDLETOINDEXES( hOutStream, &hm.wAdapterIndex, &hm.u.d.wOStreamIndex );

	 hm.u.d.u.Pio.dwPunchInSample=dwPunchInSample;
	 hm.u.d.u.Pio.dwPunchOutSample=dwPunchOutSample;

	 HPI_Message( phSubSys, &hm, &hr );

	 return(hr.wError);
}
HW16 HPI_OutStreamAncillaryReset(
    HPI_HSUBSYS 	*phSubSys,
    HPI_HOSTREAM 	hOutStream,
	 HW16			wMode)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    HPI_InitMessage( &hm,  HPI_OBJ_OSTREAM, HPI_OSTREAM_ANC_RESET );
    HPI_HANDLETOINDEXES( hOutStream, &hm.wAdapterIndex, &hm.u.d.wOStreamIndex );
    hm.u.d.u.Data.Format.wMode = wMode;
    HPI_Message( phSubSys, &hm, &hr );
    return(hr.wError);
}


HW16 HPI_OutStreamAncillaryGetInfo(
    HPI_HSUBSYS *phSubSys,
    HPI_HOSTREAM hOutStream,
    HW32	*pdwFramesAvailable)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    HPI_InitMessage( &hm,  HPI_OBJ_OSTREAM, HPI_OSTREAM_ANC_GET_INFO );
    HPI_HANDLETOINDEXES( hOutStream, &hm.wAdapterIndex, &hm.u.d.wOStreamIndex );
    HPI_Message( phSubSys, &hm, &hr );
	 if(hr.wError==0)
    {
        if(pdwFramesAvailable)
            *pdwFramesAvailable = hr.u.d.dwDataAvailable/sizeof(HPI_ANC_FRAME);
    }
    return(hr.wError);
}

HW16 HPI_OutStreamAncillaryRead(
    HPI_HSUBSYS *phSubSys,
    HPI_HOSTREAM hOutStream,
    HPI_ANC_FRAME	*pAncFrameBuffer,
    HW32	dwAncFrameBufferSizeInBytes,
	 HW32	dwNumberOfAncillaryFramesToRead)
{
	 HPI_MESSAGE hm;
	 HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_OSTREAM, HPI_OSTREAM_ANC_READ );
    HPI_HANDLETOINDEXES( hOutStream, &hm.wAdapterIndex, &hm.u.d.wOStreamIndex );
    hm.u.d.u.Data.dwpbData = (HW32)pAncFrameBuffer;
    hm.u.d.u.Data.dwDataSize = dwNumberOfAncillaryFramesToRead*sizeof(HPI_ANC_FRAME);
    if( hm.u.d.u.Data.dwDataSize <= dwAncFrameBufferSizeInBytes)
        HPI_Message( phSubSys, &hm, &hr );
    else
        hr.wError = HPI_ERROR_INVALID_DATA_TRANSFER;
    return(hr.wError);
}

HW16 HPI_OutStreamSetTimeScale(
					 HPI_HSUBSYS  *phSubSys,
					 HPI_HOSTREAM hOutStream,
					 HW32         dwTimeScale)
{
	 HPI_MESSAGE hm;
	 HPI_RESPONSE hr;

	 HPI_InitMessage( &hm,  HPI_OBJ_OSTREAM, HPI_OSTREAM_SET_TIMESCALE );
	 HPI_HANDLETOINDEXES( hOutStream, &hm.wAdapterIndex, &hm.u.d.wOStreamIndex );

	 hm.u.d.u.dwTimeScale=dwTimeScale;

	 HPI_Message( phSubSys, &hm, &hr );

	 return(hr.wError);
}
HW16 HPI_OutStreamHostBufferAllocate(
				HPI_HSUBSYS *phSubSys,
				HPI_HOSTREAM hOutStream,
				HW32	dwSizeInBytes)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    HPI_InitMessage( &hm,  HPI_OBJ_OSTREAM, HPI_OSTREAM_HOSTBUFFER_ALLOC );
    HPI_HANDLETOINDEXES( hOutStream, &hm.wAdapterIndex, &hm.u.d.wOStreamIndex );
    hm.u.d.u.Data.dwDataSize = dwSizeInBytes;
    HPI_Message( phSubSys, &hm, &hr );
    return(hr.wError);
}

HW16 HPI_OutStreamHostBufferFree(
				HPI_HSUBSYS *phSubSys,
				HPI_HOSTREAM hOutStream)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    HPI_InitMessage( &hm,  HPI_OBJ_OSTREAM, HPI_OSTREAM_HOSTBUFFER_FREE );
    HPI_HANDLETOINDEXES( hOutStream, &hm.wAdapterIndex, &hm.u.d.wOStreamIndex );
    HPI_Message( phSubSys, &hm, &hr );
    return(hr.wError);
}
/** @} */ // outstream
///////////////////////////////////////////////////////////////////////////
/** \defgroup instream Input Stream

@{
*/
HW16 HPI_InStreamOpen(
	 HPI_HSUBSYS *phSubSys,
    HW16        wAdapterIndex,
    HW16        wInStreamIndex,
    HPI_HISTREAM *phInStream
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HW16 wDspIndex;


    // only need to make this call for objects that can be distributed
    hr.wError = HPI_AdapterFindObject(phSubSys, wAdapterIndex,
                                      HPI_OBJ_ISTREAM, wInStreamIndex, &wDspIndex);

    if(hr.wError==0)
	 {
        HPI_InitMessage( &hm,  HPI_OBJ_ISTREAM, HPI_ISTREAM_OPEN );
        hm.wDspIndex = wDspIndex; // overloaded member
        hm.wAdapterIndex = wAdapterIndex;
        hm.u.d.wIStreamIndex = wInStreamIndex;

        HPI_Message( phSubSys, &hm, &hr );

        // construct a global (to the audio subsystem) handle from the adapter,DSP
        // and stream index
        if(hr.wError==0)
            *phInStream = HPI_IndexesToHandle3( 'I', wAdapterIndex, wInStreamIndex , wDspIndex);
        else
            *phInStream = 0;
    }
    else
    {
        *phInStream = 0;
    }
    return(hr.wError);
}

HW16 HPI_InStreamClose(
    HPI_HSUBSYS *phSubSys,
    HPI_HISTREAM hInStream
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    HPI_InitMessage( &hm,  HPI_OBJ_ISTREAM, HPI_ISTREAM_HOSTBUFFER_FREE);
    HPI_HANDLETOINDEXES3( hInStream, &hm.wAdapterIndex, &hm.u.d.wIStreamIndex , &hm.wDspIndex);
    HPI_Message( phSubSys, &hm, &hr );

    HPI_InitMessage( &hm,  HPI_OBJ_ISTREAM, HPI_ISTREAM_CLOSE );
    HPI_HANDLETOINDEXES3( hInStream, &hm.wAdapterIndex, &hm.u.d.wIStreamIndex , &hm.wDspIndex);
    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);
}

HW16 HPI_InStreamQueryFormat(
    HPI_HSUBSYS *phSubSys,
    HPI_HISTREAM hInStream,
    HPI_FORMAT  *pFormat
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    // contruct the HPI message from the function parameters
    HPI_InitMessage( &hm,  HPI_OBJ_ISTREAM, HPI_ISTREAM_QUERY_FORMAT );
    HPI_HANDLETOINDEXES3( hInStream, &hm.wAdapterIndex, &hm.u.d.wIStreamIndex , &hm.wDspIndex);
    memcpy( &hm.u.d.u.Data.Format, pFormat, sizeof(HPI_FORMAT) );

    HPI_Message( phSubSys, &hm, &hr );   // send the message to all the HPIs

    return(hr.wError);
}


HW16 HPI_InStreamSetFormat(
    HPI_HSUBSYS *phSubSys,
    HPI_HISTREAM hInStream,
    HPI_FORMAT  *pFormat
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    // contruct the HPI message from the function parameters
    HPI_InitMessage( &hm,  HPI_OBJ_ISTREAM, HPI_ISTREAM_SET_FORMAT );
    HPI_HANDLETOINDEXES3( hInStream, &hm.wAdapterIndex, &hm.u.d.wIStreamIndex , &hm.wDspIndex);
    memcpy( &hm.u.d.u.Data.Format, pFormat, sizeof(HPI_FORMAT) );

    HPI_Message( phSubSys, &hm, &hr );   // send the message to all the HPIs

    return(hr.wError);
}


HW16 HPI_InStreamRead(
    HPI_HSUBSYS *phSubSys,
    HPI_HISTREAM hInStream,
    HPI_DATA    *pData
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    // contruct the HPI message from the function parameters
    HPI_InitMessage( &hm,  HPI_OBJ_ISTREAM, HPI_ISTREAM_READ );
    HPI_HANDLETOINDEXES3( hInStream, &hm.wAdapterIndex, &hm.u.d.wIStreamIndex , &hm.wDspIndex);
    memcpy( &hm.u.d.u.Data, pData, sizeof(HPI_DATA) );

    HPI_Message( phSubSys, &hm, &hr );   // send the message to all the HPIs

	 // get the return parameters from the HPI response
    memcpy( pData, &hm.u.d.u.Data, sizeof(HPI_DATA) );
    return(hr.wError);
}


HW16 HPI_InStreamStart(
    HPI_HSUBSYS *phSubSys,
    HPI_HISTREAM hInStream
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    // contruct the HPI message from the function parameters
    HPI_InitMessage( &hm,  HPI_OBJ_ISTREAM, HPI_ISTREAM_START );
    HPI_HANDLETOINDEXES3( hInStream, &hm.wAdapterIndex, &hm.u.d.wIStreamIndex , &hm.wDspIndex);

    HPI_Message( phSubSys, &hm, &hr );   // send the message to all the HPIs

    return(hr.wError);
}

HW16 HPI_InStreamStop(
    HPI_HSUBSYS *phSubSys,
    HPI_HISTREAM hInStream
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    // contruct the HPI message from the function parameters
    HPI_InitMessage( &hm,  HPI_OBJ_ISTREAM, HPI_ISTREAM_STOP );
    HPI_HANDLETOINDEXES3( hInStream, &hm.wAdapterIndex, &hm.u.d.wIStreamIndex , &hm.wDspIndex);

    HPI_Message( phSubSys, &hm, &hr );   // send the message to all the HPIs

	 return(hr.wError);
}

HW16 HPI_InStreamReset(
    HPI_HSUBSYS *phSubSys,
    HPI_HISTREAM hInStream
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    // contruct the HPI message from the function parameters
    HPI_InitMessage( &hm,  HPI_OBJ_ISTREAM, HPI_ISTREAM_RESET );
    HPI_HANDLETOINDEXES3( hInStream, &hm.wAdapterIndex, &hm.u.d.wIStreamIndex , &hm.wDspIndex);

    HPI_Message( phSubSys, &hm, &hr );   // send the message to all the HPIs

    return(hr.wError);
}

/**
 \deprecated This function has been superseded by HPI_InStreamGetInfoEx()
*/
HW16 HPI_InStreamGetInfo(
    HPI_HSUBSYS *phSubSys,
    HPI_HOSTREAM hOutStream,
    HW16 *pwState,
    HW32 *pdwBufferSize,
    HW32 *pdwDataRecorded
)
{
    return(
              HPI_InStreamGetInfoEx(
                  phSubSys,
                  hOutStream,
                  pwState,
                  pdwBufferSize,
                  pdwDataRecorded,
                  NULL,
                  NULL)
			 );
}


HW16 HPI_InStreamGetInfoEx(
    HPI_HSUBSYS *phSubSys,
    HPI_HISTREAM hInStream,
    HW16        *pwState,
    HW32        *pdwBufferSize,
    HW32        *pdwDataRecorded,
    HW32        *pdwSamplesRecorded,
    HW32        *pdwAuxilaryDataRecorded
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_ISTREAM, HPI_ISTREAM_GET_INFO );
    HPI_HANDLETOINDEXES3( hInStream, &hm.wAdapterIndex, &hm.u.d.wIStreamIndex , &hm.wDspIndex);

    HPI_Message( phSubSys, &hm, &hr );

    // only send back data if valid pointers supplied!!
    if(pwState)
        *pwState = hr.u.d.wState;
    if(pdwBufferSize)
        *pdwBufferSize = hr.u.d.dwBufferSize;
    if(pdwDataRecorded)
        *pdwDataRecorded = hr.u.d.dwDataAvailable;
    if(pdwSamplesRecorded)
        *pdwSamplesRecorded = hr.u.d.dwSamplesTransfered;
    if(pdwAuxilaryDataRecorded)
        *pdwAuxilaryDataRecorded = hr.u.d.dwAuxilaryDataAvailable;
    return(hr.wError);
}

HW16 HPI_InStreamAncillaryReset(
    HPI_HSUBSYS 	*phSubSys,
	 HPI_HISTREAM 	hInStream,
    HW16			wBytesPerFrame,
    HW16 			wMode,    			// = HPI_MPEG_ANC_XXX
    HW16        wAlignment,
    HW16			wIdleBit)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_ISTREAM, HPI_ISTREAM_ANC_RESET);
    HPI_HANDLETOINDEXES3( hInStream, &hm.wAdapterIndex, &hm.u.d.wIStreamIndex , &hm.wDspIndex);
    /*
     * Format structure packing
     * dwAttributes = wBytesPerFrame
     * wFormat = wMode
     * wMode = wIdleBit
     */
    hm.u.d.u.Data.Format.dwAttributes = wBytesPerFrame;
    hm.u.d.u.Data.Format.wFormat = (wMode<<8) | (wAlignment & 0xff);
    hm.u.d.u.Data.Format.wMode = wIdleBit;
    HPI_Message( phSubSys, &hm, &hr );
    return(hr.wError);
}

HW16 HPI_InStreamAncillaryGetInfo(
    HPI_HSUBSYS *phSubSys,
    HPI_HISTREAM hInStream,
    HW32	*pdwFrameSpace)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_ISTREAM, HPI_ISTREAM_ANC_GET_INFO );
    HPI_HANDLETOINDEXES3( hInStream, &hm.wAdapterIndex, &hm.u.d.wIStreamIndex , &hm.wDspIndex);
    HPI_Message( phSubSys, &hm, &hr );
    if(pdwFrameSpace)
        *pdwFrameSpace = (hr.u.d.dwBufferSize-hr.u.d.dwDataAvailable)/sizeof(HPI_ANC_FRAME);
    return(hr.wError);
}

HW16 HPI_InStreamAncillaryWrite(
    HPI_HSUBSYS *phSubSys,
    HPI_HISTREAM hInStream,
    HPI_ANC_FRAME	*pAncFrameBuffer,
    HW32	dwAncFrameBufferSizeInBytes,
    HW32	dwNumberOfAncillaryFramesToWrite)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    HPI_InitMessage( &hm,  HPI_OBJ_ISTREAM, HPI_ISTREAM_ANC_WRITE );
    HPI_HANDLETOINDEXES3( hInStream, &hm.wAdapterIndex, &hm.u.d.wIStreamIndex , &hm.wDspIndex);
    hm.u.d.u.Data.dwpbData = (HW32)pAncFrameBuffer;
    hm.u.d.u.Data.dwDataSize = dwNumberOfAncillaryFramesToWrite*sizeof(HPI_ANC_FRAME);
    if( hm.u.d.u.Data.dwDataSize <= dwAncFrameBufferSizeInBytes)
        HPI_Message( phSubSys, &hm, &hr );
    else
        hr.wError = HPI_ERROR_INVALID_DATA_TRANSFER;
    return(hr.wError);
}

HW16 HPI_InStreamHostBufferAllocate(
				HPI_HSUBSYS *phSubSys,
				HPI_HISTREAM hInStream,
				HW32	dwSizeInBytes)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    HPI_InitMessage( &hm,  HPI_OBJ_ISTREAM, HPI_ISTREAM_HOSTBUFFER_ALLOC );
    HPI_HANDLETOINDEXES3( hInStream, &hm.wAdapterIndex, &hm.u.d.wIStreamIndex , &hm.wDspIndex);
    hm.u.d.u.Data.dwDataSize = dwSizeInBytes;
    HPI_Message( phSubSys, &hm, &hr );
    return(hr.wError);
}

HW16 HPI_InStreamHostBufferFree(
				HPI_HSUBSYS *phSubSys,
				HPI_HISTREAM hInStream)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;

    HPI_InitMessage( &hm,  HPI_OBJ_ISTREAM, HPI_ISTREAM_HOSTBUFFER_FREE );
    HPI_HANDLETOINDEXES3( hInStream, &hm.wAdapterIndex, &hm.u.d.wIStreamIndex , &hm.wDspIndex);
    HPI_Message( phSubSys, &hm, &hr );
    return(hr.wError);
}

/** @} */ // Instream
/** @} */ // Streams

///////////////////////////////////////////////////////////////////////////
/** \defgroup mixer Mixer and Controls

@{
*/
	
HW16 HPI_MixerOpen(
    HPI_HSUBSYS *phSubSysHandle,
    HW16    wAdapterIndex,
    HPI_HMIXER *phMixerHandle
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_MIXER, HPI_MIXER_OPEN );
    hm.wAdapterIndex = wAdapterIndex;

    HPI_Message( phSubSysHandle, &hm, &hr );

    if(hr.wError==0)
        *phMixerHandle = HPI_IndexesToHandle( 'M', wAdapterIndex, 0 );
    else
        *phMixerHandle = 0;
    return(hr.wError);
}

HW16 HPI_MixerClose(
    HPI_HSUBSYS *phSubSysHandle,
    HPI_HMIXER hMixerHandle
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_MIXER, HPI_MIXER_CLOSE );
    HPI_HANDLETOINDEXES( hMixerHandle, &hm.wAdapterIndex, NULL );

    HPI_Message( phSubSysHandle, &hm, &hr );

    return(hr.wError);
}

HW16 HPI_MixerGetControl(
    HPI_HSUBSYS *phSubSysHandle,
    HPI_HMIXER      hMixerHandle,
    HW16            wSrcNodeType,
    HW16            wSrcNodeTypeIndex,
    HW16            wDstNodeType,
    HW16            wDstNodeTypeIndex,
    HW16            wControlType,           // HPI_CONTROL_METER, _VOLUME etc
    HPI_HCONTROL *phControlHandle
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_MIXER, HPI_MIXER_GET_CONTROL);
    HPI_HANDLETOINDEXES( hMixerHandle, &hm.wAdapterIndex, NULL );
    hm.u.m.wNodeType1 = wSrcNodeType;
    hm.u.m.wNodeIndex1 = wSrcNodeTypeIndex;
    hm.u.m.wNodeType2 = wDstNodeType;
    hm.u.m.wNodeIndex2 = wDstNodeTypeIndex;
    hm.u.m.wControlType = wControlType;

    HPI_Message( phSubSysHandle, &hm, &hr );

    // each control in an adapter/mixer has a unique index.
    if(hr.wError==0)
        *phControlHandle = HPI_IndexesToHandle( 'C', hm.wAdapterIndex, hr.u.m.wControlIndex );
    else
        *phControlHandle = 0;
    return(hr.wError);
}
/** Get the location and type of a mixer control by index.
    To enumerate all the mixer controls of an adapter, iterate wControlIndex
   from 0 until the function returns HPI_ERROR_INVALID_OBJ_INDEX

    A control may exist between two nodes, or on a single node in which case
    either source or destination node type is zero.

	\return HPI_ERROR_*
	\retval HPI_ERROR_INVALID_OBJ_INDEX when wControlIndex > number of mixer controls
*/
HW16 HPI_MixerGetControlByIndex(
    HPI_HSUBSYS *phSubSysHandle,	   ///<  HPI subsystem handle
    HPI_HMIXER      hMixerHandle,	   ///<  Mixer Handle
    HW16            wControlIndex,	   ///<  Control Index to query
	HW16            *pwSrcNodeType,     ///< [out] Source node type
	HW16            *pwSrcNodeIndex,    ///< [out] Source ndoe index
    HW16            *pwDstNodeType,     ///< [out] Destination node type
	HW16            *pwDstNodeIndex,    ///< [out] Destination node index
    HW16            *pwControlType,     ///< [out] Control Type
    HPI_HCONTROL *phControlHandle      ///< [out] Control handle
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_MIXER, HPI_MIXER_GET_CONTROL_BY_INDEX);
    HPI_HANDLETOINDEXES( hMixerHandle, &hm.wAdapterIndex, NULL );
    hm.u.m.wControlIndex=wControlIndex;
    HPI_Message( phSubSysHandle, &hm, &hr );

	if (pwSrcNodeType) /* return all or none of info fields */
	{
	 *pwSrcNodeType =hr.u.m.wSrcNodeType + HPI_SOURCENODE_BASE;
	 *pwSrcNodeIndex=hr.u.m.wSrcNodeIndex;
	 *pwDstNodeType =hr.u.m.wDstNodeType + HPI_DESTNODE_BASE;
	 *pwDstNodeIndex=hr.u.m.wDstNodeIndex;
	 *pwControlType =hr.u.m.wControlIndex; /* Actually Type */
 	}

	 // each control in an adapter/mixer has a unique index.
	 if (phControlHandle)
	 {
		 if(hr.wError==0)
			  *phControlHandle = HPI_IndexesToHandle( 'C', hm.wAdapterIndex, wControlIndex );
		 else
			  *phControlHandle = 0;
	 }
	 return(hr.wError);
}

/////////////////////////////////////////////////////////////////////////
// MIXER CONTROLS
/** General function for setting common control parameters
 Still need specific code to set analog values
*/
HW16 HPI_ControlParamSet(
	 const HPI_HSUBSYS     *phSubSysHandle,
	 const HPI_HCONTROL    hControlHandle,
	 const HW16            wAttrib,
	 const HW32            dwParam1,
	 const HW32            dwParam2
)
{
	 HPI_MESSAGE hm;
	 HPI_RESPONSE hr;
	 HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_SET_STATE);
	 HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
	 hm.u.c.wAttribute = wAttrib;
	 hm.u.c.dwParam1 = dwParam1;
	 hm.u.c.dwParam2 = dwParam2;
	 HPI_Message( phSubSysHandle, &hm, &hr );
	 return(hr.wError);
}
HW16 HPI_ControlExParamSet(
	 const HPI_HSUBSYS     *phSubSysHandle,
	 const HPI_HCONTROL    hControlHandle,
	 const HW16            wAttrib,
	 const HW32            dwParam1,
	 const HW32            dwParam2
)
{
	 HPI_MESSAGE hm;
	 HPI_RESPONSE hr;
	 HPI_InitMessage( &hm,  HPI_OBJ_CONTROLEX, HPI_CONTROL_SET_STATE);
	 HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.cx.wControlIndex );
	 hm.u.cx.wAttribute = wAttrib;
	 hm.u.cx.u.generic.dwParam1 = dwParam1;
	 hm.u.cx.u.generic.dwParam2 = dwParam2;
	 HPI_Message( phSubSysHandle, &hm, &hr );
	 return(hr.wError);
}

/** General function for getting up to 2 HW32 return values
*/
HW16 HPI_ControlParamGet(
	 const HPI_HSUBSYS     *phSubSysHandle,
	 const HPI_HCONTROL    hControlHandle,
	 const HW16            wAttrib,
	 HW32                  dwParam1,
	 HW32                  dwParam2,
	 HW32        		  *pdwParam1,
	 HW32                  *pdwParam2
)
{
	 HPI_MESSAGE hm;
	 HPI_RESPONSE hr;
	 HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	 HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
	 hm.u.c.wAttribute = wAttrib;
	 hm.u.c.dwParam1=dwParam1;
	 hm.u.c.dwParam2=dwParam2;
	 HPI_Message( phSubSysHandle, &hm, &hr );
	 if (pdwParam1)
		  *pdwParam1 = hr.u.c.dwParam1;
	 if(pdwParam2)
		  *pdwParam2 = hr.u.c.dwParam2;

	 return(hr.wError);
}
HW16 HPI_ControlExParamGet(
	 const HPI_HSUBSYS     *phSubSysHandle,
	 const HPI_HCONTROL    hControlHandle,
	 const HW16            wAttrib,
	 HW32                  dwParam1,
	 HW32                  dwParam2,
	 HW32        		  *pdwParam1,
	 HW32                  *pdwParam2
)
{
	 HPI_MESSAGE hm;
	 HPI_RESPONSE hr;
	 HPI_InitMessage( &hm,  HPI_OBJ_CONTROLEX, HPI_CONTROL_GET_STATE);
	 HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.cx.wControlIndex );
	 hm.u.cx.wAttribute = wAttrib;
	 hm.u.cx.u.generic.dwParam1=dwParam1;
	 hm.u.cx.u.generic.dwParam2=dwParam2;
	 HPI_Message( phSubSysHandle, &hm, &hr );
	 if (pdwParam1)
		  *pdwParam1 = hr.u.cx.u.generic.dwParam1;
	 if(pdwParam2)
		  *pdwParam2 = hr.u.cx.u.generic.dwParam2;

	 return(hr.wError);
}
#if 1
#define HPI_ControlParam1Get(s,h,a,p1) HPI_ControlParamGet(s,h,a,0,0,p1,0)
#define HPI_ControlParam2Get(s,h,a,p1,p2) HPI_ControlParamGet(s,h,a,0,0,p1,p2)
#define HPI_ControlExParam1Get(s,h,a,p1) HPI_ControlExParamGet(s,h,a,0,0,p1,0)
#define HPI_ControlExParam2Get(s,h,a,p1,p2) HPI_ControlExParamGet(s,h,a,0,0,p1,p2)
#else
HW16 HPI_ControlParam2Get(
	 const HPI_HSUBSYS     *phSubSysHandle,
	 const HPI_HCONTROL    hControlHandle,
	 const HW16            wAttrib,
	 HW32        		  *pdwParam1,
	 HW32                  *pdwParam2
)
{
	return HPI_ControlParamGet(phSubSysHandle,hControlHandle,wAttrib,0,0,pdwParam1,pdwParam2);
}


HW16 HPI_ControlParam1Get(
    const HPI_HSUBSYS     *phSubSysHandle,
    const HPI_HCONTROL    hControlHandle,
    const HW16            wAttrib,
    HW32                  *pdwParam1
)
{
	return HPI_ControlParamGet(phSubSysHandle,hControlHandle,wAttrib,0,0,pdwParam1,0L);
}
#endif


/** Get the possible settings of an attribute of a mixer control given its index.

This is done without disturbing the current setting of the control.  
Do this by iterating dwIndex from 0 until the function returns HPI_ERROR_INVALID_OBJ_INDEX.  

For example, to determine how what bands are supported by a particular tuner, do the following:
\code
for (dwIndex=0; dwIndex<10; dwIndex++) {
	wErr= HPI_ControlQuery(phSS,hC, HPI_TUNER_BAND, dwIndex, 0 , AvailableBands[dwIndex]);
	if (wErr !=0) break;
}
numBands=dwIndex;
\endcode

For attributes that have a range, 3 values will be returned for indices 0 to 2: minimum, maximum and step.
The supplementary parameter dwParam is used where the possible settings for one attribute depend on the setting of another attribute, e.g. a tuners frequency range depends on which band is selected.
For example, to determine the frequency range of the AM band, do the following:

\code
wErr= HPI_ControlQuery(phSS,hC, HPI_TUNER_FREQ, 0, HPI_TUNER_BAND_AM , pdwMinFreq);
wErr= HPI_ControlQuery(phSS,hC, HPI_TUNER_FREQ, 1, HPI_TUNER_BAND_AM , pdwMaxFreq);
wErr= HPI_ControlQuery(phSS,hC, HPI_TUNER_FREQ, 2, HPI_TUNER_BAND_AM , pdwFreqStep);
\endcode

<table>
<caption>Supported attributes</caption>
<tr><td> Associated set function</td><td>wAttrib	</td><td>dwParam	</td><td>*dwSetting</td></tr>
<tr><td> HPI_Tuner_SetBand()</td>       <td>HPI_TUNER_BAND</td><td>0	</td><td>HPI_TUNER_BAND_*</td></tr>
<tr><td> HPI_Tuner_SetFrequency()</td><td>HPI_TUNER_FREQ</td><td>HPI_TUNER_BAND_*</td><td>Range in kHz</td></tr>
<tr><td> HPI_Tuner_SetGain()</td>        <td>HPI_TUNER_GAIN</td><td>0</td><td>Range in milliBels</td></tr>
</table>

*/
HW16 HPI_ControlQuery(
	 const HPI_HSUBSYS     *phSubSysHandle,
	 const HPI_HCONTROL    hControlHandle,
	 const HW16                wAttrib, ///< A control attribute
	 const HW32	          dwIndex, ///< Index for possible attribute values
	 const HW32                dwParam, ///< Supplementary parameter
	 HW32        		  *pdwSetting ///< [out] One of N possible settings for the control attribute, specified by dwIndex=0..N-1(and possibly depending on the value of the supplementary parameter)
)
{
	 HPI_MESSAGE hm;
	 HPI_RESPONSE hr;
	 HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_GET_INFO);
	 HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );

	 hm.u.c.wAttribute = wAttrib;
	 hm.u.c.dwParam1   = dwIndex;
	 hm.u.c.dwParam2   = dwParam;

	 HPI_Message( phSubSysHandle, &hm, &hr );
	 if (pdwSetting)
		  *pdwSetting = hr.u.c.dwParam1;

	 return(hr.wError);
}
/////////////////////////////////////////////////////////////////////////
/** \defgroup aesrx AES/EBU Digital audio receiver controls
\{
*/

/** set whether to input from the professional AES/EBU input
 or the consumer S/PDIF input
*/
HW16 HPI_AESEBU_Receiver_SetSource(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            wSource     //  HPI_AESEBU_SOURCE_AESEBU
    //  HPI_AESEBU_SOURCE_SPDIF
)
{
    return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_AESEBU_SOURCE, wSource, 0);
}

HW16 HPI_AESEBU_Receiver_GetSource(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            *pwSource     //  HPI_AESEBU_SOURCE_AESEBU
    //  HPI_AESEBU_SOURCE_SPDIF
)
{
	HW16 wErr;
	HW32 dwParam;

	wErr = HPI_ControlParam1Get(phSubSysHandle, hControlHandle, HPI_AESEBU_SOURCE, &dwParam);
	if(!wErr && pwSource)
		*pwSource = (HW16)dwParam;

    return wErr;
}

/// get the sample rate of the current AES/EBU input
HW16 HPI_AESEBU_Receiver_GetSampleRate(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW32            *pdwSampleRate     //0, 32000,44100 or 48000 returned
)
{
    return HPI_ControlParam1Get(phSubSysHandle, hControlHandle, HPI_AESEBU_SAMPLERATE,pdwSampleRate);
}

/// get a byte of user data from the AES/EBU stream
HW16 HPI_AESEBU_Receiver_GetUserData(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            wIndex,       // ranges from 0..3
    HW16            *pwData       // returned user data
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
    HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
    hm.u.c.wAttribute = HPI_AESEBU_USERDATA;
    hm.u.c.dwParam1 = wIndex;

    HPI_Message( phSubSysHandle, &hm, &hr );

    if(pwData)
		  *pwData = (HW16)hr.u.c.dwParam2;
    return(hr.wError);
}

///get a byte of channel status from the AES/EBU stream
HW16 HPI_AESEBU_Receiver_GetChannelStatus(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            wIndex,       // ranges from 0..23
    HW16            *pwData       // returned channel status data
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	 HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
    hm.u.c.wAttribute = HPI_AESEBU_CHANNELSTATUS;
    hm.u.c.dwParam1 = wIndex;

    HPI_Message( phSubSysHandle, &hm, &hr );

    if(pwData)
		  *pwData = (HW16)hr.u.c.dwParam2;
    return(hr.wError);
}

/** get error status from the AES/EBU stream

*pwErrorData:    bit0: 1 when PLL is not locked
                 bit1: 1 when signal quality is poor
                 bit2: 1 when there is a parity error
                 bit3: 1 when there is a bi-phase coding violation
                 bit4: 1 whne the validity bit is high
*/

HW16 HPI_AESEBU_Receiver_GetErrorStatus(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            *pwErrorData       ///< returned error data
)
{
	HW32 dwErrorData=0;
    HW16 wError=0;

	wError = HPI_ControlParam1Get(phSubSysHandle, hControlHandle, HPI_AESEBU_ERRORSTATUS,&dwErrorData);
    if(pwErrorData)
        *pwErrorData = (HW16)dwErrorData;
    return(wError);
}
/*\}*/
///////////////////////////////////////////////////////////
/**\defgroup aestx AES/EBU Digital audio transmitter control
\{
*/
/** set the AES/EBU transmitters sample rate
 this is only valid if the source is the analog mixer
 if the source is an outstream, then the samplerate will
 be that of the outstream.
*/
HW16 HPI_AESEBU_Transmitter_SetSampleRate(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW32            dwSampleRate       //32000,44100 or 48000
)
{
    return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_AESEBU_SAMPLERATE, dwSampleRate, 0);
}

HW16 HPI_AESEBU_Transmitter_SetUserData(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            wIndex,       // ranges from 0..3
    HW16            wData         // user data to set
)
{
    return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_AESEBU_USERDATA, wIndex, wData);
}

/// set a byte of channel status in the AES/EBU stream
HW16 HPI_AESEBU_Transmitter_SetChannelStatus(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            wIndex,       // ranges from 0..23
    HW16            wData         // channel status data to write
)
{
    return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_AESEBU_CHANNELSTATUS, wIndex, wData);
}


/** get a byte of channel status in the AES/EBU stream
\warning Currently disabled pending debug of DSP code
*/
HW16 HPI_AESEBU_Transmitter_GetChannelStatus(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            wIndex,       // ranges from 0..23
    HW16            *pwData       // channel status data to write
)
{
#if 0
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
    HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
    hm.u.c.wAttribute = HPI_AESEBU_CHANNELSTATUS;
    hm.u.c.dwParam1 = wIndex;

    HPI_Message( phSubSysHandle, &hm, &hr );

	if(!hr.wError && pwData)
		*pwData = (HW16)hr.u.c.dwParam2;

    return hr.wError;
#else
	return HPI_ERROR_INVALID_OPERATION;
#endif
}

/** sets the AES3 Transmitter clock source to be say the adapter or external sync 
*/
HW16 HPI_AESEBU_Transmitter_SetClockSource(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            wClockSource       /* SYNC, ADAPTER */
)
{
    return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_AESEBU_CLOCKSOURCE, wClockSource, 0);
}


HW16 HPI_AESEBU_Transmitter_GetClockSource(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            *pwClockSource       /* SYNC, ADAPTER */
)
{
	HW16 wErr;
	HW32 dwParam;

	wErr = HPI_ControlParam1Get(phSubSysHandle, hControlHandle, HPI_AESEBU_CLOCKSOURCE, &dwParam);
	if(!wErr && pwClockSource)
		*pwClockSource = (HW16)dwParam;

    return wErr;
}


HW16 HPI_AESEBU_Transmitter_SetFormat(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            wOutputFormat       /* HPI_AESEBU_SOURCE_AESEBU, _SPDIF */
)
{
    // we use the HPI_AESEBU_SOURCE attribute, because thats used on the receiver side - _FORMAT would be a better descriptor
    return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_AESEBU_SOURCE, wOutputFormat, 0);
}


HW16 HPI_AESEBU_Transmitter_GetFormat(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            *pwOutputFormat       /* HPI_AESEBU_SOURCE_AESEBU, _SPDIF */
)
{
	HW16 wErr;
	HW32 dwParam;

    // we use the HPI_AESEBU_SOURCE attribute, because thats used on the receiver side - _FORMAT would be a better descriptor
	wErr = HPI_ControlParam1Get(phSubSysHandle, hControlHandle, HPI_AESEBU_SOURCE, &dwParam);
	if(!wErr && pwOutputFormat)
		*pwOutputFormat = (HW16)dwParam;

    return wErr;
}
/*\}*/
/////////////////////////////////////////////////////////////////////////
/**\defgroup bitstream Bitstream control
 Control synchronous bitstream I/O
\{
*/
HW16 HPI_Bitstream_SetClockEdge(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            wEdgeType
)
{
    return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_BITSTREAM_CLOCK_EDGE, wEdgeType,0);
}

HW16 HPI_Bitstream_SetDataPolarity(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            wPolarity
)
{
    return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_BITSTREAM_DATA_POLARITY, wPolarity,0);
}

HW16 HPI_Bitstream_GetActivity(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            * pwClkActivity,
	 HW16            * pwDataActivity
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
    HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
    hm.u.c.wAttribute = HPI_BITSTREAM_ACTIVITY;
    HPI_Message( phSubSysHandle, &hm, &hr );
    if (pwClkActivity)
		  *pwClkActivity = (HW16)hr.u.c.dwParam1;
	 if (pwDataActivity)
		  *pwDataActivity = (HW16)hr.u.c.dwParam2;
    return(hr.wError);
}
///\}

/////////////////////////////////////////////////////////////////////////
/**\defgroup channelmode Channel Mode control
\{
*/
HW16 HPI_ChannelModeSet(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            wMode
)
{
    return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_MULTIPLEXER_SOURCE, wMode, 0);
}
HW16 HPI_ChannelModeGet(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            *wMode
)
{
    HW32 dwMode=0;
	HW16 wError=HPI_ControlParam1Get(phSubSysHandle, hControlHandle, HPI_MULTIPLEXER_SOURCE, &dwMode);
    if(wMode)
        *wMode = (HW16)dwMode;
    return(wError);
}
/**\}*/
////////////////////////////////////////////////////////////////////////////////
/**\defgroup cobranet Cobranet control
A cobranet adapter has one cobranet control for each cobranet interface (usually only one).
The cobranet control is located on (HPI_SOURCENODE_COBRANET,0,HPI_DESTNODE_COBRANET,0)
The cobranet control allows reading and writing of the cobranet HMI variables 
(See Cirrus cobranet documentation for details)

@{
*/

/** Write to an HMI variable.
\return 0=success or HPI_ERROR_*
\retval HPI_ERROR_INVALID_CONTROL if type is not cobranet
\retval HPI_ERROR_INVALID_OPERATION if HMI variable is not writeable
\retval HPI_ERROR_INVALID_DATASIZE if requested size is greater than the HMI variable size
*/

HW16 HPI_Cobranet_HmiWrite(
                HPI_HSUBSYS     *phSubSysHandle, ///<Subsystem handle
                HPI_HCONTROL    hControlHandle, ///<Handle of a cobranet control
                HW32 dwHmiAddress, /// dwHmiAddress HMI address
                HW32 dwByteCount, ///<Number of bytes to send to the control
                HW8 * pbData ///<pointer to data to send
                )

{
	 HPI_MESSAGE hm;
	 HPI_RESPONSE hr;
	 HPI_InitMessage( &hm,  HPI_OBJ_CONTROLEX, HPI_CONTROL_SET_STATE);
	 HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.cx.wControlIndex );

	 hm.u.cx.u.cobranet_data.dwByteCount = dwByteCount;
	 hm.u.cx.u.cobranet_data.dwHmiAddress = dwHmiAddress;

	 if (dwByteCount <= 8) {
	 	memcpy(hm.u.cx.u.cobranet_data.dwData,pbData,dwByteCount);
	 	hm.u.cx.wAttribute = HPI_COBRANET_SET;
	 } else {
	 	hm.u.cx.u.cobranet_bigdata.dwpbData = (HW32)pbData;
	 	hm.u.cx.wAttribute = HPI_COBRANET_SET_DATA;
	 }

	 HPI_Message( phSubSysHandle, &hm, &hr );

	 return(hr.wError);
}
/** Read from an HMI variable.
\return 0==success or HPI_ERROR_*
\retval HPI_ERROR_INVALID_CONTROL if type is not cobranet

The amount of data returned will be the minimum of the input dwMaxByteCount and the actual
size of the variable reported by the HMI
*/

HW16 HPI_Cobranet_HmiRead(
                HPI_HSUBSYS     *phSubSysHandle,///< Subsystem handle
                HPI_HCONTROL    hControlHandle,///< Handle of a cobranet control
                HW32 dwHmiAddress,///<HMI address
                HW32 dwMaxByteCount, ///<maximum number of bytes to return (<= buffer size of pbData)
                HW32* pdwByteCount, ///<[out]actual number of bytes returned
                HW8 * pbData ///<[out] data read from HMI variable
                )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage( &hm,  HPI_OBJ_CONTROLEX, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.cx.wControlIndex );

	hm.u.cx.u.cobranet_data.dwByteCount = dwMaxByteCount;
	hm.u.cx.u.cobranet_data.dwHmiAddress = dwHmiAddress;

	if (dwMaxByteCount <= 8) {
	 	hm.u.cx.wAttribute = HPI_COBRANET_GET;
	} else {
	    hm.u.cx.u.cobranet_bigdata.dwpbData = (HW32)pbData;
        hm.u.cx.wAttribute = HPI_COBRANET_GET_DATA;
	}

	HPI_Message( phSubSysHandle, &hm, &hr );
	if (!hr.wError && pbData) {

	     *pdwByteCount= hr.u.cx.u.cobranet_data.dwByteCount;

		 if (*pdwByteCount < dwMaxByteCount)
			 dwMaxByteCount = *pdwByteCount;

         if (	hm.u.cx.wAttribute == HPI_COBRANET_GET) {
             memcpy(pbData, hr.u.cx.u.cobranet_data.dwData, dwMaxByteCount);
	     } else {
	 	     /* data was already copied in hpi */
	     }

    }
	return(hr.wError);
}
/** Get the status of the last cobranet operation
*/
HW16 HPI_Cobranet_HmiGetStatus(
                HPI_HSUBSYS     *phSubSysHandle,///< Subsystem handle
                HPI_HCONTROL    hControlHandle,///< Handle of a cobranet control
                HW32* pdwStatus, ///<[out]the raw status word from the HMI
		HW32* pdwReadableSize, ///<[out] the reported readable size from the last variable access
		HW32* pdwWriteableSize ///<[out] the reported writeable size from the last variable access
                )
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage( &hm,  HPI_OBJ_CONTROLEX, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.cx.wControlIndex );

    hm.u.cx.wAttribute = HPI_COBRANET_GET_STATUS;

	HPI_Message( phSubSysHandle, &hm, &hr );
	if (!hr.wError) {
		if (pdwStatus)
			*pdwStatus = hr.u.cx.u.cobranet_status.dwStatus;
		if (pdwReadableSize)
			*pdwReadableSize = hr.u.cx.u.cobranet_status.dwReadableSize;
		if (pdwWriteableSize)
			*pdwWriteableSize = hr.u.cx.u.cobranet_status.dwWriteableSize;
	}
	return(hr.wError);
}

/** Set the CobraNet mode.
\return 0==success or HPI_ERROR_*
\retval HPI_ERROR_INVALID_CONTROL if type is not cobranet

Allows the user to set the mode of CobraNet operation. This changes the way that a ASI6416 and ASI2416
work together. A reload of the DSP code is required for the new setting to take effect. In Windows
this requires a reboot of the computer. In Linux this requires that driver be unloaded and reloaded.
*/
HW16 HPI_Cobranet_SetMode(
                HPI_HSUBSYS     *phSubSysHandle,///< Subsystem handle
                HPI_HCONTROL    hControlHandle,///< Handle of a CobraNet control
                HW32  dwMode,	///< Mode should be one of HPI_COBRANET_MODE_NETWORK, HPI_COBRANET_MODE_TETHERED
                HW32  dwSetOrQuery	///< Use HPI_COBRANET_MODE_QUERY to query or HPI_COBRANET_MODE_SET to set
				)
{
	return HPI_ControlExParamSet(phSubSysHandle, hControlHandle, HPI_COBRANET_MODE, dwMode, dwSetOrQuery);
}

/** Get the CobraNet mode.
\return 0==success or HPI_ERROR_*
\retval HPI_ERROR_INVALID_CONTROL if type is not cobranet

Allows the user to get the mode of CobraNet operation. This changes the way that a ASI6416 and ASI2416
work together.
*/
HW16 HPI_Cobranet_GetMode(
					 HPI_HSUBSYS     *phSubSysHandle,	///< Subsystem handle
					 HPI_HCONTROL    hControlHandle,	///< Handle of a CobraNet control
					 HW32  *pdwMode		///< Returns HPI_COBRANET_MODE_NETWORK or HPI_COBRANET_MODE_TETHERED
				)
{
	HW16 nError=0;
	HW32 dwMode=0;
	nError = HPI_ControlExParam1Get(phSubSysHandle, hControlHandle, HPI_COBRANET_MODE, &dwMode);
	if(pdwMode) *pdwMode = (HW16)dwMode;
	return(nError);
}

/** Get the CobraNet node's IP address.
\return 0==success or HPI_ERROR_*
\retval HPI_ERROR_INVALID_CONTROL if type is not cobranet

Allows the user to get the IP address of the CobraNet node.
*/
HW16 HPI_Cobranet_GetIPaddress(
                HPI_HSUBSYS     *phSubSysHandle,///< Subsystem handle
                HPI_HCONTROL    hControlHandle,///< Handle of a cobranet control
                HW32* pdwIPaddress///< OUT: the IP address
				)
{
	HW32 dwByteCount;
	HW32 dwIP;
	HW16 wError;
	wError = HPI_Cobranet_HmiRead(phSubSysHandle, hControlHandle,
                HPI_COBRANET_HMI_cobraIpMonCurrentIP,
                4,&dwByteCount,(HW8 *)&dwIP);
    // byte swap the IP address
    *pdwIPaddress =
    	((dwIP & 0xff000000)>>8) |
    	((dwIP & 0x00ff0000)<<8) |
    	((dwIP & 0x0000ff00)>>8) |
    	((dwIP & 0x000000ff)<<8);

    return wError;

}
/** Get the CobraNet node's MAC address.
\return 0==success or HPI_ERROR_*
\retval HPI_ERROR_INVALID_CONTROL if type is not cobranet

Allows the user to get the MAC address of the CobraNet node.
*/
HW16 HPI_Cobranet_GetMACaddress(
                HPI_HSUBSYS     *phSubSysHandle, ///< Subsystem handle
                HPI_HCONTROL    hControlHandle, ///< Handle of a cobranet control
                HW32* pdwMAC_MSBs, ///< OUT: the first 4 bytes of the MAC address.
                HW32* pdwMAC_LSBs  ///< OUT: the last 2 bytes of the MAC address returned in the upper 2 bytes.
				)
{
	HW32 dwByteCount;
	HW16 wError;
	HW32 dwMAC;
	wError = HPI_Cobranet_HmiRead(phSubSysHandle, hControlHandle,
                HPI_COBRANET_HMI_cobraIfPhyAddress,
                4,&dwByteCount,(HW8 *)&dwMAC);
    *pdwMAC_MSBs =
    	((dwMAC & 0xff000000)>>8) |
    	((dwMAC & 0x00ff0000)<<8) |
    	((dwMAC & 0x0000ff00)>>8) |
    	((dwMAC & 0x000000ff)<<8);
	wError += HPI_Cobranet_HmiRead(phSubSysHandle, hControlHandle,
                HPI_COBRANET_HMI_cobraIfPhyAddress+1,
                4,&dwByteCount,(HW8 *)&dwMAC);
    *pdwMAC_LSBs =
    	((dwMAC & 0xff000000)>>8) |
    	((dwMAC & 0x00ff0000)<<8) |
    	((dwMAC & 0x0000ff00)>>8) |
    	((dwMAC & 0x000000ff)<<8);
    return wError;

}

/** @} */ // group cobranet
/////////////////////////////////////////////////////////////////////////////////
/**\defgroup compand  Compressor Expander control
///\{
*/
/*! Set up a compressor expander
	\return HPI_ERROR_*
*/
HW16 HPI_Compander_Set(
	HPI_HSUBSYS		*phSubSysHandle, //!<HPI subsystem handle
	HPI_HCONTROL	hControlHandle,	 //!<Equalizer control handle
	HW16            wAttack,          //!<attack time in milliseconds
	HW16            wDecay,          //!<decay time in milliseconds
	short           wRatio100,          //!<gain ratio * 100
	short           nThreshold0_01dB,    //!<threshold in 100ths of a dB
	short           nMakeupGain0_01dB    //!<makeup gain in 100ths of a dB
	)
{
	 HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	 HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_SET_STATE);
	 HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );

	hm.u.c.dwParam1= wAttack + ((HW32)wRatio100 << 16);
	hm.u.c.dwParam2= (wDecay & 0xFFFFL);
	 hm.u.c.anLogValue[0]=nThreshold0_01dB;
	 hm.u.c.anLogValue[1]=nMakeupGain0_01dB;
	 hm.u.c.wAttribute = 0;

	 HPI_Message( phSubSysHandle, &hm, &hr );

	 return(hr.wError);
}

/*! Get the settings of a compressor expander
	\return HPI_ERROR_*
*/
HW16 HPI_Compander_Get(
	HPI_HSUBSYS		*phSubSysHandle, //!<HPI subsystem handle
	HPI_HCONTROL	hControlHandle,	 //!<Equalizer control handle
	HW16            *pwAttack,          //!<[out] attack time in milliseconds
	HW16            *pwDecay,          //!<[out] decay time in milliseconds
	short           *pwRatio100,          //!<[out] gain ratio * 100
	short           *pnThreshold0_01dB,    //!<[out] threshold in 100ths of a dB
	short           *pnMakeupGain0_01dB    //!<[out] makeup gain in 100ths of a dB
	)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
	hm.u.c.wAttribute = 0;

	HPI_Message( phSubSysHandle, &hm, &hr );

	if (pwAttack) *pwAttack=(short)(hr.u.c.dwParam1 & 0xFFFF);
	if (pwDecay)  *pwDecay=(short)(hr.u.c.dwParam2 & 0xFFFF);
	if (pwRatio100) *pwRatio100=(short)(hr.u.c.dwParam1 >> 16);

	if (pnThreshold0_01dB) *pnThreshold0_01dB=hr.u.c.anLogValue[0];
	if (pnMakeupGain0_01dB) *pnMakeupGain0_01dB=hr.u.c.anLogValue[1];

	return(hr.wError);
}
///\}
/////////////////////////////////////////////////////////////////////////
/** \defgroup level Level Control
The level control sets the gain of the line (analog) inputs and outputs
The level is in units of 0.01dBu (0dBu = 0.775 VRMS)
 \{
*/

HW16 HPI_LevelSetGain(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    short           anGain0_01dB[HPI_MAX_CHANNELS]
)
{
    HPI_MESSAGE hm;
	 HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_SET_STATE);
    HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
    memcpy( hm.u.c.anLogValue, anGain0_01dB, sizeof(short)*HPI_MAX_CHANNELS);
    hm.u.c.wAttribute = HPI_VOLUME_GAIN;    // should be HPI_LEVEL_GAIN !!

    HPI_Message( phSubSysHandle, &hm, &hr );

    return(hr.wError);
}


HW16 HPI_LevelGetGain(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
	 short           anGain0_01dB[HPI_MAX_CHANNELS]
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
    HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
    hm.u.c.wAttribute = HPI_VOLUME_GAIN;    // should be HPI_LEVEL_GAIN !!

    HPI_Message( phSubSysHandle, &hm, &hr );

	 memcpy( anGain0_01dB, hr.u.c.anLogValue, sizeof(short)*HPI_MAX_CHANNELS);
    return(hr.wError);
}
///\}
/////////////////////////////////////////////////////////////////////////
/** \defgroup meter  Meter Control
 The meter control returns information about the level of the audio signal
 at the position of the control.

Depending on the adapter, Peak and RMS readings are available.
On some adapters meter ballistics may be set (on older adapters the meter gives an instantaneous reading)

Readings are in units of 0.01dB

\todo Implement linear to log conversion in fixed point math so that it can be included with kernel code.

@{
*/

/** Get the meter peak reading
*/
HW16 HPI_MeterGetPeak(
	 HPI_HSUBSYS     *phSubSysHandle, ///< HPI subsystem handle
	 HPI_HCONTROL    hControlHandle, ///< meter control handle
	 short   anPeakdB[HPI_MAX_CHANNELS]  ///< [out] meter peaks
)
{
#ifndef HPI_KERNEL_MODE
	 short nLinear=0;
#endif
	 short  i=0;

	 HPI_MESSAGE hm;
	 HPI_RESPONSE hr;

	 HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	 HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
	 hm.u.c.wAttribute = HPI_METER_PEAK;

	 HPI_Message( phSubSysHandle, &hm, &hr );

	 if(!hr.wError)
	 {
		  // Method to return meter values from DSP
		  // If return value is -ve (-1 to -32767) then treat as Log(peak)
		  // range of log values will be -1 to -20000 (-0.01 to -200.00 dB)
		  // 0 will never be returned for log (-1 = -0.01dB is max)

#ifndef HPI_KERNEL_MODE 
		 /// \bug Kernel mode cant do floating point log.  +ve linear value is returned instead.
		 
		  // convert 0..32767 level to 0.01dB (20log10), 0 is -100.00dB
		  for(i=0; i<HPI_MAX_CHANNELS; i++)
		  {
				nLinear = hr.u.c.anLogValue[i];
				// don't have to touch the LogValue when it is -ve since it is already a log value
				if(nLinear>=0)
				{
					 if(nLinear==0)
						  hr.u.c.anLogValue[i] = -100*100;  	// units are 0.01dB
					 else
						  hr.u.c.anLogValue[i] = (short)((float)(20*log10((float)nLinear/32767.0))*100.0);   // units are 0.01dB
				}
		  }
#endif

		  memcpy( anPeakdB, hr.u.c.anLogValue, sizeof(short)*HPI_MAX_CHANNELS);
	 }
	 else
		for(i=0; i<HPI_MAX_CHANNELS; i++)
			anPeakdB[i] = -100*100;   	// return -100dB in case function is not supported.
	 return(hr.wError);
}

/** Get the meter RMS reading in 100ths of a dB
*/
HW16 HPI_MeterGetRms(
	 HPI_HSUBSYS     *phSubSysHandle,
	 HPI_HCONTROL    hControlHandle,
	 short   anRmsdB[HPI_MAX_CHANNELS]
)
{
#ifndef HPI_KERNEL_MODE
	 short nLinear=0;
#endif
	 short  i=0;

	 HPI_MESSAGE hm;
	 HPI_RESPONSE hr;

	 HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	 HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
	 hm.u.c.wAttribute = HPI_METER_RMS;

	 HPI_Message( phSubSysHandle, &hm, &hr );

	 if(!hr.wError)
	 {
#ifndef HPI_KERNEL_MODE
		 /// \bug Kernel mode cant do floating point log.  +ve linear value is returned instead.
		  // convert 0..32767 level to 0.01dB (20log10), 0 is -100.00dB
		  for(i=0; i<HPI_MAX_CHANNELS; i++)
		  {
				nLinear = hr.u.c.anLogValue[i];
				// don't have to touch the LogValue when it is -ve since it is already a log value
				if(nLinear>=0)
				{
					 if(nLinear==0)
						  hr.u.c.anLogValue[i] = -100*100;  	// units are 0.01dB
					 else
						  hr.u.c.anLogValue[i] = (short)((float)(20*log10((float)nLinear/32767.0))*100.0);   // units are 0.01dB
				}
		  }
#endif
		  memcpy( anRmsdB, hr.u.c.anLogValue, sizeof(short)*HPI_MAX_CHANNELS);
	 }
	 else
		for(i=0; i<HPI_MAX_CHANNELS; i++)
			anRmsdB[i] = -100*100;   	// return -100dB in case function is not supported.

	 return(hr.wError);
}

/** Set the ballistics of the RMS part of a meter.

The attack and decay values represent the time constants of the equivalent single pole low pass filter used to create the ballistics. 
With a time constant of T, if the meter is stable at full scale and the input is suddenly removed, the meter will decay.  

Setting nAttack to 0 gives the meter instantaneous rise time.
Setting nDecay to a value smaller than a few times your  meter polling interval is not advised.  
The meter will appear to read something approaching the instantaneous value at the time of polling
rather than the maximum peak since the previous reading.
*/
HW16 HPI_MeterSetRmsBallistics(
	HPI_HSUBSYS 	*phSubSysHandle,
	HPI_HCONTROL	hControlHandle,
	unsigned short	nAttack, ///< Attack timeconstant in milliseconds
	unsigned short	nDecay ///< Decay timeconstant in milliseconds
	)
{
	return HPI_ControlParamSet(phSubSysHandle, hControlHandle,
		HPI_METER_RMS_BALLISTICS,nAttack, nDecay);
}

/** Get the ballistics settings of the RMS part of a meter.
*/
HW16 HPI_MeterGetRmsBallistics(
	HPI_HSUBSYS 	*phSubSysHandle,
	HPI_HCONTROL	hControlHandle,
	unsigned short	*pnAttack,///< Attack timeconstant in milliseconds
	unsigned short	*pnDecay///< Decay timeconstant in milliseconds
	)
{
	HW32 dwAttack;
	HW32 dwDecay;
	HW16 nError;

	nError=HPI_ControlParam2Get(phSubSysHandle,hControlHandle,HPI_METER_RMS_BALLISTICS,
					&dwAttack, &dwDecay);

	 if (pnAttack)
		*pnAttack = (unsigned short)dwAttack;
	 if (pnDecay)
		*pnDecay = (unsigned short)dwDecay;

	 return nError;
}
/** Set the ballistics of the Peak part of a meter.
*/
HW16 HPI_MeterSetPeakBallistics(
	HPI_HSUBSYS 	*phSubSysHandle,
	HPI_HCONTROL	hControlHandle,
	unsigned short	nAttack,
	unsigned short	nDecay
	)
{
	return HPI_ControlParamSet(phSubSysHandle, hControlHandle,
		HPI_METER_PEAK_BALLISTICS,nAttack, nDecay);
}

/** Get the ballistics settings of the Peak part of a meter.
*/
HW16 HPI_MeterGetPeakBallistics(
	HPI_HSUBSYS 	*phSubSysHandle,
	HPI_HCONTROL	hControlHandle,
	unsigned short	*pnAttack,
	unsigned short	*pnDecay
	)
{
	HW32 dwAttack;
	HW32 dwDecay;
	HW16 nError;

	nError=HPI_ControlParam2Get(phSubSysHandle,hControlHandle,HPI_METER_PEAK_BALLISTICS,
					&dwAttack, &dwDecay);

	 if (pnAttack)
		*pnAttack = (short)dwAttack;
	 if (pnDecay)
		*pnDecay = (short)dwDecay;

	 return nError;
}

/** @} */

/* future function to convert linear to log using integers

long lLogC[16] = {
5184138,
-95824768,
-196833680,
-297842592,
-398851488,
-499860384,
-600869312,
-701878208,
-802887104,
-903896000,
-1004904896,
-1105913856,
-1206922752,
-1307931648,
-1408940544,
30};
double __fastcall TFLogDemo::LinearToLogQuadApproxInt(short nPeak)
{
     short   nBeforeFirstBit;
     short   nR;
     short nSample;
     long    lAcc;

     nBeforeFirstBit = 0;
     nSample=nPeak;
     while(!(nSample&0x8000) )
     {
          nSample <<= 1;
          nBeforeFirstBit++;
     }
     nSample &= ~0x8000;
     nR = nBeforeFirstBit;
     lAcc = (long)nSample * -1047L;  // Q15 * Q9 = Q24
     lAcc = lAcc + 135336768L;
     lAcc = lAcc >> 15;              // Q24 > Q9
     lAcc = lAcc * (long)nSample;    // Q9 * Q15 = Q24
     lAcc = lAcc + lLogC[nR];        // lAcc has result in Q24
     // lAcc = (lAcc>>8)*100;        // result is 100ths of dB in Q16 format
     // dB100 = (short)(lAcc>>16L);
     return (double)lAcc / pow(2.0,24.0);
}
*/
/////////////////////////////////////////////////////////////////////////////////
/**\defgroup mic Microphone control 
\{
*/
HW16 HPI_Microphone_SetPhantomPower(
				HPI_HSUBSYS     *phSubSysHandle,
				HPI_HCONTROL    hControlHandle,
				HW16            wOnOff
				)
{
	return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_MICROPHONE_PHANTOM_POWER, (HW32)wOnOff, 0);
}
HW16 HPI_Microphone_GetPhantomPower(
				HPI_HSUBSYS     *phSubSysHandle,
				HPI_HCONTROL    hControlHandle,
				HW16            *pwOnOff
				)
{
	HW16 nError=0;
	HW32 dwOnOff=0;
	nError = HPI_ControlParam1Get(phSubSysHandle, hControlHandle, HPI_MICROPHONE_PHANTOM_POWER, &dwOnOff);
	if(pwOnOff) *pwOnOff = (HW16)dwOnOff;
	return(nError);
}
///\}
/////////////////////////////////////////////////////////////////////////
/** \defgroup mux Multiplexer control
 This control allows one of many sources to be connected to a destination
 Typically used on the instream (record) side to select a record input
or on the linein to select analog or digital input.
\{
*/

HW16 HPI_Multiplexer_SetSource(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            wSourceNodeType,      // any source node
    HW16            wSourceNodeIndex      // any source node
)
{
    return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_MULTIPLEXER_SOURCE, wSourceNodeType, wSourceNodeIndex);
}
HW16 HPI_Multiplexer_GetSource(
                HPI_HSUBSYS     *phSubSysHandle,
                HPI_HCONTROL    hControlHandle,
                HW16            *wSourceNodeType,      /* any source node */
                HW16            *wSourceNodeIndex      /* any source node  */
)
{
    HW32 dwNode,dwIndex;
	 HW16 wError=HPI_ControlParam2Get(phSubSysHandle, hControlHandle, HPI_MULTIPLEXER_SOURCE, &dwNode, &dwIndex);
    if(wSourceNodeType)
        *wSourceNodeType = (HW16)dwNode;
    if(wSourceNodeIndex)
        *wSourceNodeIndex = (HW16)dwIndex;
    return wError;
}
HW16 HPI_Multiplexer_QuerySource(
					 HPI_HSUBSYS     *phSubSysHandle,
					 HPI_HCONTROL    hControlHandle,
					 HW16            wIndex,
					 HW16            *wSourceNodeType,      /* any source node */
					 HW16            *wSourceNodeIndex      /* any source node  */
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
	 hm.u.c.wAttribute = HPI_MULTIPLEXER_QUERYSOURCE;
	 hm.u.c.dwParam1 = wIndex;

    HPI_Message( phSubSysHandle, &hm, &hr );

    if(wSourceNodeType)
		  *wSourceNodeType = (HW16)hr.u.c.dwParam1;
    if(wSourceNodeIndex)
		  *wSourceNodeIndex = (HW16)hr.u.c.dwParam2;
    return(hr.wError);
}
/**\}*/
/////////////////////////////////////////////////////////////////////////
/** \defgroup onoff On/off switch control
This control allows make/break connections to be supported.
\{
*/

HW16 HPI_OnOffSwitch_SetState(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            wState                   /* 1=on, 0=off */
)
{
    return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_ONOFFSWITCH_STATE, wState, 0);
}

HW16 HPI_OnOffSwitch_GetState(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            *wState                  /* 1=on, 0=off */
)
{
    HW32 dwState=0;
	HW16 wError=HPI_ControlParam1Get(phSubSysHandle, hControlHandle, HPI_ONOFFSWITCH_STATE, &dwState);
    if(wState)
        *wState = (HW16)dwState;
    return(wError);
}
/**\}*/
/////////////////////////////////////////////////////////////////////////////////
/**\addtogroup parmeq  Parametric Equalizer control
\{
*/

/*!
	Find out the number of available bands of a parametric equalizer,
	and whether it is enabled or not

	\return HPI_ERROR_*
*/
HW16 HPI_ParametricEQ_GetInfo(
		HPI_HSUBSYS 	*phSubSysHandle,  //!<  HPI subsystem handle
		HPI_HCONTROL	hControlHandle,	  //!<  Equalizer control handle
		HW16			*pwNumberOfBands,  //!< [out] number of bands available
		HW16			*pwOnOff  //!< [out] enabled status
		)
{
	HW32    dwNOB=0;
	HW32    dwOO=0;
	 HW16    nError=0;

	nError = HPI_ControlParam2Get(
		phSubSysHandle, hControlHandle, HPI_EQUALIZER_NUM_FILTERS,
		&dwOO, &dwNOB);
	 if(pwNumberOfBands)
		  *pwNumberOfBands = (HW16)dwNOB;
	 if(pwOnOff)
		  *pwOnOff = (HW16)dwOO;
	 return nError;
}

/*!
	Turn a parametric equalizer on or off

	\return HPI_ERROR_*
*/
HW16 HPI_ParametricEQ_SetState(
		HPI_HSUBSYS 	*phSubSysHandle,  //!<  HPI subsystem handle
		HPI_HCONTROL	hControlHandle,	  //!<  Equalizer control handle
		HW16			   wOnOff  //!<  1=on, 0=off
		)
{
	return HPI_ControlParamSet(
	 phSubSysHandle,
	 hControlHandle,
	 HPI_EQUALIZER_NUM_FILTERS,
	 wOnOff,
	0
	);
}

/*! Set up one of the filters in a parametric equalizer
	\return HPI_ERROR_*
*/
HW16 HPI_ParametricEQ_SetBand(
	HPI_HSUBSYS		*phSubSysHandle, //!<  HPI subsystem handle
	HPI_HCONTROL	hControlHandle,	 //!<  Equalizer control handle
	HW16            wIndex,          //!<  index of band to set
	HW16            nType,           //!<  band type, One of the \ref eq_filter_types
	HW32            dwFrequencyHz,   //!<  band frequency
	short           nQ100,         //!<  filter Q * 100
	short           nGain0_01dB      //!<  filter gain in 100ths of a dB
	)
{
	 HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	 HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_SET_STATE);
	 HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );

	hm.u.c.dwParam1=dwFrequencyHz;
	hm.u.c.dwParam2= (wIndex & 0xFFFFL) + ((HW32)nType << 16);
	 hm.u.c.anLogValue[0]=nGain0_01dB;
	 hm.u.c.anLogValue[1]=nQ100;
	 hm.u.c.wAttribute = HPI_EQUALIZER_FILTER;

	 HPI_Message( phSubSysHandle, &hm, &hr );

	 return(hr.wError);
}

/*! Get the settings of one of the filters in a parametric equalizer
	\return HPI_ERROR_*
*/
HW16 HPI_ParametricEQ_GetBand(
	HPI_HSUBSYS		*phSubSysHandle, //!<  HPI subsystem handle
	HPI_HCONTROL	hControlHandle,	 //!<  Equalizer control handle
	HW16            wIndex,          //!<  index of band to Get
	HW16            *pnType,		 //!< [out] band type
	HW32            *pdwFrequencyHz, //!< [out] band frequency
	short           *pnQ100,		 //!< [out] filter Q * 100
	short           *pnGain0_01dB	 //!< [out] filter gain in 100ths of a dB
	)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
	hm.u.c.wAttribute = HPI_EQUALIZER_FILTER;
	hm.u.c.dwParam2=wIndex;

	HPI_Message( phSubSysHandle, &hm, &hr );

	if (pdwFrequencyHz) *pdwFrequencyHz=hr.u.c.dwParam1;
	if (pnType) *pnType= (HW16)(hr.u.c.dwParam2 >> 16);
	if (pnQ100) *pnQ100=hr.u.c.anLogValue[1];
	if (pnGain0_01dB) *pnGain0_01dB=hr.u.c.anLogValue[0];

	return(hr.wError);
}

/*! Retrieve the calculated filter coefficients (scaled by 1000 into integers)
*/
HW16 HPI_ParametricEQ_GetCoeffs(
	HPI_HSUBSYS		*phSubSysHandle, //!<  HPI subsystem handle
	HPI_HCONTROL	hControlHandle,	 //!<  Equalizer control handle
	HW16           wIndex,          //!<  index of band to Get
	short			   coeffs[5]	//!< [out] filter coefficients * 1000 a1,a2,b0,b1,b2 (a0==0)
	)
{
	HPI_MESSAGE hm;
	HPI_RESPONSE hr;
	HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
	hm.u.c.wAttribute = HPI_EQUALIZER_COEFFICIENTS;
	hm.u.c.dwParam2=wIndex;

	HPI_Message( phSubSysHandle, &hm, &hr );

	coeffs[0]=(short)hr.u.c.anLogValue[0];
	coeffs[1]=(short)hr.u.c.anLogValue[1];
	coeffs[2]=(short)hr.u.c.dwParam1;
	coeffs[3]=(short)(hr.u.c.dwParam1 >> 16);
	coeffs[4]=(short)hr.u.c.dwParam2;

	return(hr.wError);
}
///\}
/////////////////////////////////////////////////////////////////////////////////
/**\defgroup sampleclock SampleClock control 
\{
*/
HW16 HPI_SampleClock_SetSource(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            wSource		// HPI_SAMPLECLOCK_SOURCE_ADAPTER, _AESEBU etc
)
{
    return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_SAMPLECLOCK_SOURCE, wSource,0);
}
HW16 HPI_SampleClock_SetSourceIndex(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            wSourceIndex		// index of the source to use
)
{
    return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_SAMPLECLOCK_SOURCE_INDEX, wSourceIndex,0);
}

HW16 HPI_SampleClock_GetSource(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
	HW16            *pwSource		// HPI_SAMPLECLOCK_SOURCE_ADAPTER, _AESEBU etc
)
{
	HW16 wError=0;
	HW32 dwSource=0;
	wError = HPI_ControlParam1Get(phSubSysHandle, hControlHandle, HPI_SAMPLECLOCK_SOURCE, &dwSource);
	if(!wError)
		if(pwSource)
			*pwSource = (HW16)dwSource;
	return(wError);
}
HW16 HPI_SampleClock_GetSourceIndex(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
	HW16            *pwSourceIndex
)
{
	HW16 wError=0;
	HW32 dwSourceIndex=0;
	wError = HPI_ControlParam1Get(phSubSysHandle, hControlHandle, HPI_SAMPLECLOCK_SOURCE_INDEX, &dwSourceIndex);
	if(!wError)
		if(pwSourceIndex)
			*pwSourceIndex = (HW16)dwSourceIndex;
	return(wError);
}

HW16 HPI_SampleClock_SetSampleRate(
	 HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW32            dwSampleRate
)
{
	return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_SAMPLECLOCK_SAMPLERATE, dwSampleRate,0);
}

HW16 HPI_SampleClock_GetSampleRate(
	 HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW32            * pdwSampleRate
)
{
	HW16 wError=0;
	HW32 dwSampleRate=0;
	wError = HPI_ControlParam1Get(phSubSysHandle, hControlHandle, HPI_SAMPLECLOCK_SAMPLERATE, &dwSampleRate);
	if(!wError)
		if(pdwSampleRate)
			*pdwSampleRate = dwSampleRate;
	return(wError);
}
///\}
///////////////////////////////////////////////////////////////////////////////
/** \defgroup tuner Tuner Controls

The tuner control sets the band and frequency of a tuner, and measures the RF level
\{
*/

/** Select the tuner band.

Not all tuners support all bands. (Currently either AM+FM or TV+FM).

Note that with the exception of HPI_TUNER_BAND_AUX,
the tuner frequency must subsequently be set using HPI_Tuner_SetFrequency().
\return 0 or HPI_ERROR_*
\retval HPI_ERROR_INVALID_CONTROL_VALUE if tuner does not support the requested band

\sa HPI_ControlQuery() for details on determining the bands supported by a particular tuner.
*/
HW16 HPI_Tuner_SetBand(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            wBand   /**< one of the \ref tuner_bands */
)
{
    return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_TUNER_BAND, wBand, 0);
}

/** Set the RF gain of the tuner front end.
*/
HW16 HPI_Tuner_SetGain(
	 HPI_HSUBSYS     *phSubSysHandle,
	 HPI_HCONTROL    hControlHandle,
	 short           nGain
)
{
	 return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_TUNER_GAIN, nGain, 0);
}

HW16 HPI_Tuner_SetFrequency(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW32            wFreqInkHz
)
{
    return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_TUNER_FREQ, wFreqInkHz, 0);
}

HW16 HPI_Tuner_GetBand(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            *pwBand
)
{
	HW32    dwBand=0;
	 HW16    nError=0;

	nError = HPI_ControlParam1Get(phSubSysHandle, hControlHandle, HPI_TUNER_BAND, &dwBand);
	 if(pwBand)
		  *pwBand = (HW16)dwBand;
	 return nError;
}
HW16 HPI_Tuner_GetGain(
	 HPI_HSUBSYS     *phSubSysHandle,
	 HPI_HCONTROL    hControlHandle,
	 short           *pnGain
)
{
	HW32    dwGain=0;
	HW16    nError=0;

	nError = HPI_ControlParam1Get(phSubSysHandle, hControlHandle, HPI_TUNER_GAIN, &dwGain);
	 if(pnGain)
		  *pnGain = (HW16)dwGain;
	 return nError;
}
HW16 HPI_Tuner_GetFrequency(
	 HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW32            *pwFreqInkHz
)
{
    return HPI_ControlParam1Get(phSubSysHandle, hControlHandle, HPI_TUNER_FREQ, pwFreqInkHz);
}

HW16 HPI_Tuner_GetRFLevel(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    short            *pwLevel
)
{
	 HPI_MESSAGE hm;
	 HPI_RESPONSE hr;
	 HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	 HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
	 hm.u.c.wAttribute = HPI_TUNER_LEVEL;
	 hm.u.c.dwParam1=HPI_TUNER_LEVEL_AVERAGE;
	 HPI_Message( phSubSysHandle, &hm, &hr );
	 if (pwLevel)
	 	*pwLevel = (short)hr.u.c.dwParam1;
	return(hr.wError);
}


HW16 HPI_Tuner_GetRawRFLevel(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    short            *pwLevel
)
{
	 HPI_MESSAGE hm;
	 HPI_RESPONSE hr;
	 HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	 HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
	 hm.u.c.wAttribute = HPI_TUNER_LEVEL;
	 hm.u.c.dwParam1=HPI_TUNER_LEVEL_RAW;
	 HPI_Message( phSubSysHandle, &hm, &hr );
	 if (pwLevel)
	 	*pwLevel = (short)hr.u.c.dwParam1;
	 return(hr.wError);
}

/** 
\deprecated This function has been superceded by HPI_Tuner_GetStatus()
*/
HW16 HPI_Tuner_GetVideoStatus(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            *pwStatus
)
{
	HW32    dwStatus=0;
    HW16    nError=0;

	 nError = HPI_ControlParam1Get(phSubSysHandle, hControlHandle, HPI_TUNER_VIDEO_STATUS, &dwStatus);
    if(pwStatus)
        *pwStatus = (HW16)dwStatus;
    return nError;
}

HW16 HPI_Tuner_GetStatus(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    HW16            *pwStatusMask,
    HW16            *pwStatus
)
{
	HW32    dwStatus=0;
    HW16    nError=0;

	// wStatusmask is in high 16bit word, wStatus is in low 16bit word
	nError = HPI_ControlParam1Get(phSubSysHandle, hControlHandle, HPI_TUNER_STATUS, &dwStatus);
    if(pwStatus)
    {
		if(!nError)
		{
        	*pwStatusMask = (HW16)(dwStatus>>16);
			*pwStatus = (HW16)(dwStatus & 0xFFFF);
		}
		else
		{
        	*pwStatusMask = 0;
			*pwStatus = 0;
		}
	}
    return nError;
}

HW16 HPI_Tuner_SetMode(
	 HPI_HSUBSYS     *phSubSysHandle,
	 HPI_HCONTROL    hControlHandle,
	 HW32            nMode,
	 HW32            nValue
)
{
	 return HPI_ControlParamSet(phSubSysHandle, hControlHandle, HPI_TUNER_MODE, nMode, nValue);
}

HW16 HPI_Tuner_GetMode(
	 HPI_HSUBSYS     *phSubSysHandle,
	 HPI_HCONTROL    hControlHandle,
	 HW32            nMode,
	 HW32            *pnValue
)
{
	 return HPI_ControlParamGet(phSubSysHandle, hControlHandle, HPI_TUNER_MODE, nMode,0, pnValue,0);
}
/**\}*/
/////////////////////////////////////////////////////////////////////////
/** \defgroup volume Volume Control
\{
*/
HW16 HPI_VolumeSetGain(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    short   anLogGain[HPI_MAX_CHANNELS]
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_SET_STATE);
    HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
    memcpy( hm.u.c.anLogValue, anLogGain, sizeof(short)*HPI_MAX_CHANNELS);
    hm.u.c.wAttribute = HPI_VOLUME_GAIN;

    HPI_Message( phSubSysHandle, &hm, &hr );

    return(hr.wError);
}

HW16 HPI_VolumeGetGain(
    HPI_HSUBSYS     *phSubSysHandle,
    HPI_HCONTROL    hControlHandle,
    short   anLogGain[HPI_MAX_CHANNELS]
)
{
	 HPI_MESSAGE hm;
	 HPI_RESPONSE hr;
	 HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	 HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
	 hm.u.c.wAttribute = HPI_VOLUME_GAIN;

	 HPI_Message( phSubSysHandle, &hm, &hr );

	 memcpy( anLogGain, hr.u.c.anLogValue, sizeof(short)*HPI_MAX_CHANNELS);
	 return(hr.wError);
}

HW16 HPI_VolumeQueryRange(
					 HPI_HSUBSYS     *phSubSysHandle,
					 HPI_HCONTROL    hControlHandle,
					 short           *nMinGain_01dB,
					 short           *nMaxGain_01dB,
					 short           *nStepGain_01dB
					 )
{
	 HPI_MESSAGE hm;
	 HPI_RESPONSE hr;
	 HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
	 HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
	 hm.u.c.wAttribute = HPI_VOLUME_RANGE;

	 HPI_Message( phSubSysHandle, &hm, &hr );
	 if(hr.wError)
	 {
		hr.u.c.anLogValue[0] = hr.u.c.anLogValue[1] = 0;
		hr.u.c.dwParam1 = (HW32)0;
	 }
	 if(nMinGain_01dB)
		*nMinGain_01dB = hr.u.c.anLogValue[0];
	 if(nMaxGain_01dB)
		*nMaxGain_01dB = hr.u.c.anLogValue[1];
	 if(nStepGain_01dB)
		*nStepGain_01dB = (short)hr.u.c.dwParam1;
	 return(hr.wError);
}


/* starts an automatic ramp of the volume control from the current gain setting to
	the specified setting over the specified duration (in milliseconds )
	The profile can be either log or linear
*/

HW16 HPI_VolumeAutoFadeProfile(
					 HPI_HSUBSYS     *phSubSysHandle,
					 HPI_HCONTROL    hControlHandle,
					 short           anStopGain0_01dB[HPI_MAX_CHANNELS],
					 HW32            dwDurationMs,
					 HW16            wProfile /* HPI_VOLUME_AUTOFADE_??? */
					 )
{
	 HPI_MESSAGE hm;
	 HPI_RESPONSE hr;
	 HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_SET_STATE);
	 HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
	 memcpy( hm.u.c.anLogValue, anStopGain0_01dB, sizeof(short)*HPI_MAX_CHANNELS);

	 if (wProfile < HPI_VOLUME_AUTOFADE)
		  wProfile = HPI_VOLUME_AUTOFADE;

	 hm.u.c.wAttribute = wProfile;
	 hm.u.c.dwParam1 = dwDurationMs;

	 HPI_Message( phSubSysHandle, &hm, &hr );

	 return(hr.wError);
}

HW16 HPI_VolumeAutoFade(
	 HPI_HSUBSYS     *phSubSysHandle,
	 HPI_HCONTROL    hControlHandle,
	 short           anStopGain0_01dB[HPI_MAX_CHANNELS],
	 HW32            dwDurationMs
)
{
	 return HPI_VolumeAutoFadeProfile(
					 phSubSysHandle,
					 hControlHandle,
					 anStopGain0_01dB,
					 dwDurationMs,
					 HPI_VOLUME_AUTOFADE
					 );
}
/** \} */
/////////////////////////////////////////////////////////////////////////
/**  \defgroup Vox Vox control
VOX controls are always situated on a destination node of type HPI_DESTNODE_ISTREAM.  
The VOX control specifies the signal level required for recording to begin. 
Until the signal exceeds the VOX level no samples are recorded to the record stream. 
After the VOX level has been exceeded, recording continues until the stream is stopped or reset.

A trigger level of -100 dB indicates that recording begins with any level audio signal. 
A single threshold level is used for both left and right channels. 
If either channel exceeds the threshold, recording will proceed.

@{
 */

/**
Sets the threshold of a VOX control.  Note the threshold is in units of 0.01dB. 
The threshold will typically range between 0 and -100dB.   
On startup the VOX control is set to -100 dB.
*/

HW16 HPI_VoxSetThreshold(
    HPI_HSUBSYS    *phSubSysHandle,  ///< Subsystem handle
    HPI_HCONTROL   hControlHandle, ///< Handle of a vox control
    short          anGain0_01dB ///<  Trigger level in 100ths of a dB
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_SET_STATE);
    HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
    hm.u.c.wAttribute = HPI_VOX_THRESHOLD;

#ifndef HPI_KERNEL_MODE 
    /// \bug Requires floating point operations, not usable in kernel mode. Kernel mode uses raw linear value instead.

	// convert db range 0..-100 dB , 0.01dB (20log10) to 0..32767 (-96 dB = 0 )
    {
        /*
                fDB = 20 * log10( fLinear/32767 )

                fLinear = 10^(fDB/20) * 32767

                Want to avoid using pow() routine, because math libraries in the past
                have not handled it well.

                Re-arrange to us log()/exp() (natural log) routines.

                Re-write 10^(fDB/20) using natural logs.

                fLinear = exp( log(10.0) * fDB/20.0 ) * 32767.0;

          */
        float fDB=(float)anGain0_01dB/100.0f;          // units are 0.01dB
        float fLinear=0.0;
		  fLinear = (float)(exp( log(10.0) * fDB/20.0 ) * 32767.0);
        hm.u.c.anLogValue[0] = (short)(fLinear);     // only use the first index (LEFT)
    }
#else

    hm.u.c.anLogValue[0] = anGain0_01dB;     // only use the first index (LEFT)
#endif

    HPI_Message( phSubSysHandle, &hm, &hr );

    return(hr.wError);
}

/** Get the current threshold setting of a vox control
*/
HW16 HPI_VoxGetThreshold( 
    HPI_HSUBSYS    *phSubSysHandle, ///< Subsystem handle
    HPI_HCONTROL   hControlHandle, ///< Handle of a vox contrl
    short          *anGain0_01dB ///<  [out] Trigger level in 100ths of a dB
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_CONTROL, HPI_CONTROL_GET_STATE);
    HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.c.wControlIndex );
    hm.u.c.wAttribute = HPI_VOX_THRESHOLD;

    HPI_Message( phSubSysHandle, &hm, &hr );

#ifndef HPI_KERNEL_MODE 
    /// \bug Requires floating point operations, not usable in kernel mode
	
    // convert db range 0..-100 dB , 0.01dB (20log10) to 0..32767 (-96 dB = 0 )
    {
        /*
                fDB = 20 * log10( fLinear/32767 )

                fLinear = 10^(fDB/20) * 32767

                Want to avoid using pow() routine, because math libraries in the past
                have not handled it well.

                Re-arrange to us log()/exp() (natural log) routines.

                Re-write 10^(fDB/20) using natural logs.

                fLinear = exp( log(10.0) * fDB/20.0 ) * 32767.0;

          */
        float fDB,fLinear;

		if(hr.u.c.anLogValue[0]==0)
		{
			fDB = -100.0;
		}
		else
		{
			fLinear = (float)hr.u.c.anLogValue[0];
			fDB = (float)(log(fLinear/32767.0)*20.0/log(10.0));
		}

        *anGain0_01dB = (short)(fDB*100.0);     // only use the first index (LEFT)
    }
#else

	*anGain0_01dB = hr.u.c.anLogValue[0];     // only use the first index (LEFT)
#endif

    return(hr.wError);
}
/** @}*/
/** @} */ // group mixer
////////////////////////////////////////////////////////////////////////////
/**\defgroup clock Clock
Realtime clock 
\note Not currently implemented anywhere???
Only one clock per adapter
\{
*/
/// open the clock and get a handle
HW16 HPI_ClockOpen(
	 HPI_HSUBSYS     *phSubSys,
    HW16            wAdapterIndex,
    HPI_HCLOCK  *phClock
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_CLOCK, HPI_CLOCK_OPEN );
    hm.wAdapterIndex = wAdapterIndex;

    HPI_Message( phSubSys, &hm, &hr );

    if(hr.wError==0)
        *phClock = HPI_IndexesToHandle( 'T', wAdapterIndex, 0 );    // only 1 clock obj per adapter
    else
        *phClock = 0;
    return(hr.wError);
}

/// set the time of the DSP clock object.
HW16 HPI_ClockSetTime(
    HPI_HSUBSYS     *phSubSys,
    HPI_HCLOCK  hClock,
    HW16            wHour,
    HW16            wMinute,
    HW16            wSecond,
    HW16            wMilliSeconds
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_CLOCK, HPI_CLOCK_SET_TIME );
    HPI_HANDLETOINDEXES( hClock, &hm.wAdapterIndex, NULL );
    hm.u.t.wMilliSeconds = wMilliSeconds;
    hm.u.t.wSeconds = wSecond;
    hm.u.t.wMinutes = wMinute;
    hm.u.t.wHours = wHour;
    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);
}

/// set the time of the DSP clock object.
HW16 HPI_ClockGetTime(
    HPI_HSUBSYS     *phSubSys,
    HPI_HCLOCK      hClock,
    HW16            *pwHour,
    HW16            *pwMinute,
    HW16            *pwSecond,
    HW16            *pwMilliSecond
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_CLOCK, HPI_CLOCK_GET_TIME );
    HPI_HANDLETOINDEXES( hClock, &hm.wAdapterIndex, NULL );
    HPI_Message( phSubSys, &hm, &hr );
    if(pwMilliSecond)
        *pwMilliSecond = hr.u.t.wMilliSeconds;
    if(pwSecond)
        *pwSecond = hr.u.t.wSeconds;
    if(pwMinute)
        *pwMinute = hr.u.t.wMinutes;
    if(pwHour)
        *pwHour = hr.u.t.wHours;

    return(hr.wError);
}
///\}
////////////////////////////////////////////////////////////////////////////
/**\defgroup gpio Digital I/O
 The digital I/O object on an adapter reperesents a number of input bits
 that may be individually sensed and a number of digital output bits that
 may be individually set.

There is at most one gpio object per adapter.
 \{
*/

HW16 HPI_GpioOpen(
    HPI_HSUBSYS     *phSubSys,
    HW16            wAdapterIndex,
    HPI_HGPIO   *phGpio,
    HW16            *pwNumberInputBits,
    HW16            *pwNumberOutputBits
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_GPIO, HPI_GPIO_OPEN );
    hm.wAdapterIndex = wAdapterIndex;

    HPI_Message( phSubSys, &hm, &hr );

    if(hr.wError==0)
    {
        *phGpio = HPI_IndexesToHandle( 'L', wAdapterIndex, 0 );    // only 1 digital i/o obj per adapter
        if(pwNumberInputBits)
            *pwNumberInputBits = hr.u.l.wNumberInputBits;
        if(pwNumberOutputBits)
            *pwNumberOutputBits = hr.u.l.wNumberOutputBits;
    }
    else
        *phGpio = 0;
    return(hr.wError);
}


/// read a particular bit from an adapters digital input port
HW16 HPI_GpioReadBit(
	 HPI_HSUBSYS     *phSubSys,
    HPI_HGPIO   hGpio,
    HW16            wBitIndex,
    HW16            *pwBitData
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_GPIO, HPI_GPIO_READ_BIT );
    HPI_HANDLETOINDEXES( hGpio, &hm.wAdapterIndex, NULL );       // only one dig i/o obj per adapter
    hm.u.l.wBitIndex = wBitIndex;

    HPI_Message( phSubSys, &hm, &hr );

    *pwBitData = hr.u.l.wBitData;
    return(hr.wError);
}

/// read all bits from an adapters digital input port (upto 16)
HW16 HPI_GpioReadAllBits(
	 HPI_HSUBSYS     *phSubSys,
    HPI_HGPIO   hGpio,
    HW16            *pwBitData
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_GPIO, HPI_GPIO_READ_ALL );
    HPI_HANDLETOINDEXES( hGpio, &hm.wAdapterIndex, NULL );       // only one dig i/o obj per adapter

	 HPI_Message( phSubSys, &hm, &hr );

    *pwBitData = hr.u.l.wBitData;
    return(hr.wError);
}


/// write a particular bit to an adapters digital output port
HW16 HPI_GpioWriteBit(
    HPI_HSUBSYS     *phSubSys,
    HPI_HGPIO   hGpio,
    HW16            wBitIndex,
    HW16            wBitData
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_GPIO, HPI_GPIO_WRITE_BIT );
    HPI_HANDLETOINDEXES( hGpio, &hm.wAdapterIndex, NULL );       // only one dig i/o obj per adapter
    hm.u.l.wBitIndex = wBitIndex;
	 hm.u.l.wBitData = wBitData;

    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);
}

///\}
////////////////////////////////////////////////////////////////////////////
/** \defgroup nvmem Nonvolatile memory

 Some adapters contain non-volatile memory containing a number of bytes
 The number of data words is adapter dependant and can be obtained
 from the *pwSizeInBytes parameter returned from the Open function

There can be at most one nvmemory object per adapter.

@{
*/
HW16 HPI_NvMemoryOpen(
	 HPI_HSUBSYS *phSubSys,
	 HW16    wAdapterIndex,
	 HPI_HNVMEMORY *phNvMemory,
	 HW16    *pwSizeInBytes
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_NVMEMORY, HPI_NVMEMORY_OPEN );
    hm.wAdapterIndex = wAdapterIndex;

    HPI_Message( phSubSys, &hm, &hr );

    if(hr.wError==0)
    {
        *phNvMemory = HPI_IndexesToHandle( 'N', wAdapterIndex, 0 );    // only 1 nv-memory obj per adapter
        if(pwSizeInBytes)
            *pwSizeInBytes = hr.u.n.wSizeInBytes;
    }
    else
        *phNvMemory = 0;
    return(hr.wError);
}

HW16 HPI_NvMemoryReadByte(
    HPI_HSUBSYS *phSubSys,
    HPI_HNVMEMORY hNvMemory,
    HW16    wIndex,
    HW16    *pwData
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_NVMEMORY, HPI_NVMEMORY_READ_BYTE );
    HPI_HANDLETOINDEXES( hNvMemory, &hm.wAdapterIndex, NULL );       // only one NvMem obj per adapter
    hm.u.n.wIndex = wIndex;

    HPI_Message( phSubSys, &hm, &hr );

    *pwData = hr.u.n.wData;
    return(hr.wError);
}

HW16 HPI_NvMemoryWriteByte(
    HPI_HSUBSYS *phSubSys,
    HPI_HNVMEMORY hNvMemory,
    HW16    wIndex,
    HW16    wData
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_NVMEMORY, HPI_NVMEMORY_WRITE_BYTE );
    HPI_HANDLETOINDEXES( hNvMemory, &hm.wAdapterIndex, NULL );       // only one NvMem obj per adapter
    hm.u.n.wIndex = wIndex;
    hm.u.n.wData = wData;

    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);
}
/** @}  */
////////////////////////////////////////////////////////////////////////////
/** \defgroup profile Profile
The Profile object supports profiling of the DSP code. 
It should be used as a development tool for measuring DSP code operation. 
Comments in AXPROF.H describe the DSP side of the profiling operation
@{
*/
/** open all the profiles on a particular adapter.  

 If the adapter does not have profiling enabled, the function will return an error.

The complete profile set of all profiles can be thought of as any array of execution timings/profiles. 
Each indexed profile corresponds to the execution of a particular segment of DSP code.

Note that HPI_ProfileStartAll() must be called after HPI_ProfileOpenAll() to start the profiling operation on the DSP.
*/
HW16 HPI_ProfileOpenAll(
    HPI_HSUBSYS     *phSubSys,     ///<[in] HPI subsystem handle
    HW16            wAdapterIndex,    ///<[in] adapter index
    HW16		    wProfileIndex,  ///< corresponds to DSP index
    HPI_HPROFILE    *phProfile, ///<[out] profile handle
    HW16            *pwMaxProfiles ///<[out] maximum number of profile bins supported
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_PROFILE, HPI_PROFILE_OPEN_ALL );
    hm.wAdapterIndex = wAdapterIndex;
    hm.wDspIndex = wProfileIndex;
    HPI_Message( phSubSys, &hm, &hr );

    *pwMaxProfiles = hr.u.p.u.o.wMaxProfiles;
    if(hr.wError==0)
        *phProfile = HPI_IndexesToHandle( 'P', wAdapterIndex, wProfileIndex );
    else
        *phProfile = 0;
    return(hr.wError);
}

HW16 HPI_ProfileGet(
    HPI_HSUBSYS     *phSubSys,
    HPI_HPROFILE    hProfile,
    HW16            wIndex,
    HW16            *pwSeconds,
    HW32            *pdwMicroSeconds,
    HW32            *pdwCallCount,
    HW32            *pdwMaxMicroSeconds,
    HW32            *pdwMinMicroSeconds
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_PROFILE, HPI_PROFILE_GET );
    HPI_HANDLETOINDEXES( hProfile, &hm.wAdapterIndex, &hm.wDspIndex );
    hm.u.p.wIndex = wIndex;
    HPI_Message( phSubSys, &hm, &hr );
    if(pwSeconds)
        *pwSeconds = hr.u.p.u.t.wSeconds;
    if(pdwMicroSeconds)
        *pdwMicroSeconds = hr.u.p.u.t.dwMicroSeconds;
    if(pdwCallCount)
        *pdwCallCount = hr.u.p.u.t.dwCallCount;
    if(pdwMaxMicroSeconds)
        *pdwMaxMicroSeconds = hr.u.p.u.t.dwMaxMicroSeconds;
    if(pdwMinMicroSeconds)
        *pdwMinMicroSeconds = hr.u.p.u.t.dwMinMicroSeconds;
    return(hr.wError);
}
/** 
\deprecated this function is no longer supported. Please use HPI_ProfileGetUtilization() instead.
*/
HW16 HPI_ProfileGetIdleCount(
    HPI_HSUBSYS     *phSubSys,
    HPI_HPROFILE    hProfile,
    HW16            wIndex,
    HW32            *pdwIdleCycles,
    HW32            *pdwCount
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_PROFILE, HPI_PROFILE_GET_IDLECOUNT );
    HPI_HANDLETOINDEXES( hProfile, &hm.wAdapterIndex,  &hm.wDspIndex );
    hm.u.p.wIndex = wIndex;
    HPI_Message( phSubSys, &hm, &hr );
    if(hr.wError)
    {
        if(pdwIdleCycles)
            *pdwIdleCycles = 0;
        if(pdwCount)
            *pdwCount = 0;
    }
    else
    {
        if(pdwIdleCycles)
            *pdwIdleCycles = hr.u.p.u.t.dwMicroSeconds;
        if(pdwCount)
            *pdwCount = hr.u.p.u.t.dwCallCount;
    }
    return(hr.wError);
}

/** Get the DSP utilization in 1/100 of a percent 
*/
HW16 HPI_ProfileGetUtilization(
    HPI_HSUBSYS     *phSubSys,  ///<[in] HPI subsystem handle
    HPI_HPROFILE    hProfile,        ///<[in] handle of profile object
    HW32            *pdwUtilization ///<[out] DSP utilization in 100ths of a percent
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_PROFILE, HPI_PROFILE_GET_UTILIZATION );
    HPI_HANDLETOINDEXES( hProfile, &hm.wAdapterIndex,  &hm.wDspIndex );
    HPI_Message( phSubSys, &hm, &hr );
    if(hr.wError)
    {
        if(pdwUtilization)
            *pdwUtilization = 0;
    }
    else
    {
        if(pdwUtilization)
            *pdwUtilization = hr.u.p.u.t.dwCallCount;
    }
    return(hr.wError);
}

HW16 HPI_ProfileGetName(
    HPI_HSUBSYS     *phSubSys,
    HPI_HPROFILE    hProfile,
    HW16            wIndex,
    HW8             *szName,
    HW16            nNameLength
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_PROFILE, HPI_PROFILE_GET_NAME );
    HPI_HANDLETOINDEXES( hProfile, &hm.wAdapterIndex,  &hm.wDspIndex );
	 hm.u.p.wIndex = wIndex;
    HPI_Message( phSubSys, &hm, &hr );
    if(hr.wError)
    {
        if(szName)
				strcpy(szName,"??");
    }
	 else
    {
        if(szName)
            memcpy(szName,(char *)hr.u.p.u.n.szName,nNameLength);
    }
    return(hr.wError);
}

HW16 HPI_ProfileStartAll(
    HPI_HSUBSYS     *phSubSys,
    HPI_HPROFILE    hProfile
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_PROFILE, HPI_PROFILE_START_ALL );
    HPI_HANDLETOINDEXES( hProfile, &hm.wAdapterIndex,  &hm.wDspIndex);
    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);
}

HW16 HPI_ProfileStopAll(
    HPI_HSUBSYS     *phSubSys,
    HPI_HPROFILE    hProfile
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_PROFILE, HPI_PROFILE_STOP_ALL );
    HPI_HANDLETOINDEXES( hProfile, &hm.wAdapterIndex,  &hm.wDspIndex );
    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);
}
/** @} */
////////////////////////////////////////////////////////////////////////////
/**\defgroup watchdog  Watchdog timer
Only one watch-dog per adapter.  
\note Not implemented on any adapters as of 2005-06-01
\{
*/
/// Open a watch dog object and return a handle
HW16 HPI_WatchdogOpen(
    HPI_HSUBSYS     *phSubSys,
    HW16            wAdapterIndex,
    HPI_HWATCHDOG   *phWatchdog
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_WATCHDOG, HPI_WATCHDOG_OPEN );
    hm.wAdapterIndex = wAdapterIndex;

    HPI_Message( phSubSys, &hm, &hr );

    if(hr.wError==0)
        *phWatchdog = HPI_IndexesToHandle( 'W', wAdapterIndex, 0 );    // only 1 watchdog obj per adapter
    else
        *phWatchdog = 0;
    return(hr.wError);
}

/** set the time of the watch dog object.
 Time=0 will disable the watchdog
*/
HW16 HPI_WatchdogSetTime(
    HPI_HSUBSYS     *phSubSys,
    HPI_HWATCHDOG   hWatchdog,
	 HW32            dwTimeMillisec
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_WATCHDOG, HPI_WATCHDOG_SET_TIME );
    HPI_HANDLETOINDEXES( hWatchdog, &hm.wAdapterIndex, NULL );       // only one watchdog obj per adapter
    hm.u.w.dwTimeMs = dwTimeMillisec;

    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);
}

/// reset watchdog timer
HW16 HPI_WatchdogPing(
    HPI_HSUBSYS     *phSubSys,
    HPI_HWATCHDOG   hWatchdog
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_InitMessage( &hm,  HPI_OBJ_WATCHDOG, HPI_WATCHDOG_PING );
    HPI_HANDLETOINDEXES( hWatchdog, &hm.wAdapterIndex, NULL );       // only one watchdog obj per adapter

    HPI_Message( phSubSys, &hm, &hr );

    return(hr.wError);
}
///\}

/////////////////////////////////////////////////////////////////////////////////
/**\defgroup aes18 AES18 data transfer over AES/EBU link
Currently only relevant to ASI4601 adapters.
\{
*/

#ifdef HAS_AES18_ON_HOST

//****** test vars for dummy code
HW8     awTest[200];
HW16    wLength;
//**********

/* !!! Test only !!! */
void HPI_Aes18Init(void)
{
    Aes18_Init();
}

#endif

HW16 HPI_AES18BGSetConfiguration(
    HPI_HSUBSYS     *phSubSys,
    HPI_HCONTROL    hControlHandle,
    HW16            wBlocksPerSec [HPI_AES18_MAX_CHANNELS],
    HW16            wPriorityEnableMask [HPI_AES18_MAX_CHANNELS],
    HW16            wOperatingMode [HPI_AES18_MAX_CHANNELS],
    HW16            wChannelMode
)
{
    HW16 i;
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_CONTROLX_MSG_AES18BG *pAes18BGSetConfiguration =
        &hm.u.cx.u.aes18bg;
    HPI_InitMessage( &hm,  HPI_OBJ_CONTROLEX, HPI_CONTROL_SET_STATE );
    HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.cx.wControlIndex );
    hm.u.cx.wAttribute = HPI_AES18_CONFIG;

    // Input bounds check
    for(i=0;i<HPI_AES18_MAX_CHANNELS;i++)
    {
        switch (wBlocksPerSec[i])
        {
        case 2:             // these are all ok
        case 5:
        case 24:
        case 25:
        case 33:
        case 100:
            break;
        default:
            return(HPI_ERROR_AES18BG_BLOCKSPERSEC);
        }

        if(wPriorityEnableMask[i] > HPI_AES18_MAX_PRIORITYMASK)
            return(HPI_ERROR_AES18BG_PRIORITYMASK);
        switch(wOperatingMode[i])
        {
        case HPI_AES18_MODE_MASTER:
        case HPI_AES18_MODE_SLAVE:
            break;
        default:
            return(HPI_ERROR_AES18BG_MODE);
        }
        switch(wChannelMode)
        {
        case HPI_AES18_CHANNEL_MODE_JOINT:
        case HPI_AES18_CHANNEL_MODE_INDEPENDENT:
            break;
        default:
            return(HPI_ERROR_AES18BG_CHANNEL_MODE);
        }
    }

    for(i=0;i<HPI_AES18_MAX_CHANNELS;i++)
    {
        pAes18BGSetConfiguration->wBlocksPerSec[i] = wBlocksPerSec[i];
        pAes18BGSetConfiguration->wPriorityMask[i] = wPriorityEnableMask[i];
        pAes18BGSetConfiguration->wOperatingMode[i] = wOperatingMode[i];
    }
    pAes18BGSetConfiguration->wChannelMode = wChannelMode;

#ifndef HAS_AES18_ON_HOST

    HPI_Message( phSubSys, &hm, &hr );
#else

    AES18_Message(HPI_CONTROL_AES18_BLOCKGENERATOR,&hm, &hr);
#endif

    return(hr.wError);
}


HW16 HPI_AES18RxSetAddress(
    HPI_HSUBSYS     *phSubSys,
    HPI_HCONTROL    hControlHandle,
    HW16            awDecoderAddress [HPI_AES18_MAX_CHANNELS]
)
{
    HW16 i;
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_CONTROLX_MSG_AES18RX_ADDRESS *pAes18RxSetAddress =
        &hm.u.cx.u.aes18rx_address;
    HPI_InitMessage( &hm,  HPI_OBJ_CONTROLEX, HPI_CONTROL_SET_STATE );
    HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.cx.wControlIndex );
    hm.u.cx.wAttribute = HPI_AES18_ADDRESS;

    // Check input bounds
    /* HPI_AES18_MAX_ADDRESS = 0x10000, so following check is not required
    for (i=0;i<HPI_AES18_MAX_CHANNELS;i++)
{
        if(awDecoderAddress[i] >= HPI_AES18_MAX_ADDRESS)
            return(HPI_ERROR_AES18_INVALID_ADDRESS);
}
    */

    for (i=0;i<HPI_AES18_MAX_CHANNELS;i++)
    {
        pAes18RxSetAddress->wAddress[i] = awDecoderAddress[i];
    }
#ifndef HAS_AES18_ON_HOST
    HPI_Message( phSubSys, &hm, &hr );
#else

    AES18_Message(HPI_CONTROL_AES18_RECEIVER,&hm, &hr);
#endif

    return(hr.wError);
}

/* Receiver */

HW16 HPI_AES18RxGetInternalBufferState(
    HPI_HSUBSYS     *phSubSys,
    HPI_HCONTROL    hControlHandle,
    HW16            awFrameError[HPI_AES18_MAX_CHANNELS],
    HW16            awMessageWaiting [HPI_AES18_MAX_CHANNELS][HPI_AES18_MAX_PRIORITIES],
    HW16            awInternalBufferOverFlow [HPI_AES18_MAX_CHANNELS][HPI_AES18_MAX_PRIORITIES],
    HW16            awMissedMessage [HPI_AES18_MAX_CHANNELS][HPI_AES18_MAX_PRIORITIES]
)
{
    HW16 wChannelCount, wPriorityCount, wBitShift;
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_CONTROLX_RES_AES18RX_BUFFER_STATE *pAes18RxGetIBState =
        &hr.u.cx.u.aes18rx_internal_buffer_state;
    HPI_InitMessage( &hm,  HPI_OBJ_CONTROLEX, HPI_CONTROL_GET_STATE );
    HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.cx.wControlIndex );
    hm.u.cx.wAttribute = HPI_AES18_INTERNAL_BUFFER_STATE;

#ifndef HAS_AES18_ON_HOST

    HPI_Message( phSubSys, &hm, &hr );
#else

    AES18_Message(HPI_CONTROL_AES18_RECEIVER,&hm, &hr);
#endif

    /* Unpack the status bits from the response */
    if(awFrameError)
    {
        for(wChannelCount=0;wChannelCount<HPI_AES18_MAX_CHANNELS;wChannelCount++)
        {
            awFrameError[wChannelCount] =
                (pAes18RxGetIBState->awFrameErrorPacked >> wChannelCount) & 1;
        }
    }

    /* NOTE!!! Also need to check awInternalBufferOverFlow and awMissedMessage */
    wBitShift = 0;
    if(awMessageWaiting)
    {
        for(wChannelCount=0;wChannelCount<HPI_AES18_MAX_CHANNELS;wChannelCount++)
        {
            for(wPriorityCount=0;wPriorityCount<HPI_AES18_MAX_PRIORITIES;wPriorityCount++)
            {
                awMessageWaiting[wChannelCount][wPriorityCount] =
                    (pAes18RxGetIBState->awMessageWaitingPacked >> wBitShift) & 1;

                awInternalBufferOverFlow[wChannelCount][wPriorityCount] =
                    (pAes18RxGetIBState->awInternalBufferOverFlowPacked >> wBitShift) & 1;

                awMissedMessage[wChannelCount][wPriorityCount] =
                    (pAes18RxGetIBState->awMissedMessagePacked >> (wBitShift++)) & 1;
            }
        }
    }



    return(hr.wError);

}


HW16 HPI_AES18RxGetInternalBufferSize(
    HPI_HSUBSYS     *phSubSys,
    HPI_HCONTROL    hControlHandle,
    HW16            awBytesPerBuffer[HPI_AES18_MAX_PRIORITIES]
)
{
    HW16 wPriorityCount;
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_CONTROLX_RES_AES18RX_BUFFER_SIZE *pAes18RxGetIBSize =
        &hr.u.cx.u.aes18rx_internal_buffer_size;
    HPI_InitMessage( &hm,  HPI_OBJ_CONTROLEX, HPI_CONTROL_GET_STATE );
    HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.cx.wControlIndex );
    hm.u.cx.wAttribute = HPI_AES18_INTERNAL_BUFFER_SIZE;

#ifndef HAS_AES18_ON_HOST

    HPI_Message( phSubSys, &hm, &hr );
#else

    AES18_Message(HPI_CONTROL_AES18_RECEIVER,&hm, &hr);
#endif

    if(awBytesPerBuffer)
    {
        for(wPriorityCount=0;wPriorityCount<HPI_AES18_MAX_PRIORITIES;wPriorityCount++)
        {
            awBytesPerBuffer[wPriorityCount] =
                pAes18RxGetIBSize->awBytesPerBuffer[wPriorityCount];
        }
    }
    return(hr.wError);
}


HW16  HPI_AES18RxGetMessage(
    HPI_HSUBSYS     *phSubSys,
    HPI_HCONTROL    hControlHandle,
    HW16            wChannel,
    HW16            wPriority,
    HW16            wQueueSize,         // In bytes
    HW32            dwpbMessage,        // Actually a pointer to bytes
    HW16            *pwMessageLength    // in bytes
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_CONTROLX_RES_AES18RX_GET_MESSAGE *pAes18ResRxGetMessage =
        &hr.u.cx.u.aes18rx_get_message;
    HPI_CONTROLX_MSG_AES18RX_GET_MESSAGE *pAes18MsgRxGetMessage =
        &hm.u.cx.u.aes18rx_get_message;

    HPI_InitMessage( &hm,  HPI_OBJ_CONTROLEX, HPI_CONTROL_GET_STATE );
    HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.cx.wControlIndex );
    hm.u.cx.wAttribute = HPI_AES18_MESSAGE;

    // Input bounds check
    if(wChannel >= HPI_AES18_MAX_CHANNELS)
        return(HPI_ERROR_INVALID_CHANNELS);

    if(wPriority > HPI_AES18_MAX_PRIORITIES)
        return(HPI_ERROR_AES18_INVALID_PRIORITY);

    pAes18MsgRxGetMessage->wChannel = wChannel;
    pAes18MsgRxGetMessage->wPriority = wPriority;
    pAes18MsgRxGetMessage->wQueueSize = wQueueSize;
    pAes18MsgRxGetMessage->dwpbMessage = dwpbMessage;

#ifndef HAS_AES18_ON_HOST

    HPI_Message( phSubSys, &hm, &hr );
#else

    AES18_Message(HPI_CONTROL_AES18_RECEIVER,&hm, &hr);
#endif

    *pwMessageLength=pAes18ResRxGetMessage->wReturnedMessageSize;
    return(hr.wError);
}


// Transmitter

HW16  HPI_AES18TxSendMessage(
    HPI_HSUBSYS     *phSubSys,
    HPI_HCONTROL    hControlHandle,
    HW16            wChannel,
    HW32            dwpbMessage,
    HW16            wMessageLength,
    HW16            wDestinationAddress,
    HW16            wPriorityIndex,
    HW16            wRepetitionIndex
)
{
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_CONTROLX_MSG_AES18TX_SEND_MESSAGE *pAes18SendMsg =
        &hm.u.cx.u.aes18tx_send_message;

    HPI_InitMessage( &hm,  HPI_OBJ_CONTROLEX, HPI_CONTROL_SET_STATE );
    HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.cx.wControlIndex );
    hm.u.cx.wAttribute = HPI_AES18_MESSAGE;

    // Input bounds check

    if(wChannel >= HPI_AES18_MAX_CHANNELS)
        return(HPI_ERROR_INVALID_CHANNELS);

    if(wPriorityIndex > HPI_AES18_MAX_PRIORITIES)
        return(HPI_ERROR_AES18_INVALID_PRIORITY);

    /* HPI_AES18_MAX_ADDRESS = 0x10000, so following test is not required
    if(wDestinationAddress > HPI_AES18_MAX_ADDRESS)
    return(HPI_ERROR_AES18_INVALID_ADDRESS);
    */

    if(wRepetitionIndex > HPI_AES18_MAX_REPETITION)
        return(HPI_ERROR_AES18_INVALID_REPETITION);

    pAes18SendMsg->wChannel = wChannel;
    pAes18SendMsg->dwpbMessage = dwpbMessage;
    pAes18SendMsg->wMessageLength = wMessageLength;
    pAes18SendMsg->wDestinationAddress = wDestinationAddress;
    pAes18SendMsg->wPriorityIndex = wPriorityIndex;
    pAes18SendMsg->wRepetitionIndex = wRepetitionIndex;

#ifndef HAS_AES18_ON_HOST

    HPI_Message( phSubSys, &hm, &hr );
#else

    AES18_Message(HPI_CONTROL_AES18_TRANSMITTER,&hm, &hr);
#endif

    return(hr.wError);
}


HW16 HPI_AES18TxGetInternalBufferState(
    HPI_HSUBSYS     *phSubSys,
    HPI_HCONTROL    hControlHandle,
    HW16            awInternalBufferBusy [HPI_AES18_MAX_CHANNELS][HPI_AES18_MAX_PRIORITIES]
)
{
    HW16 wChannelCount, wPriorityCount, wBitShift, wPackedBits;
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_CONTROLX_RES_AES18TX_BUFFER_STATE *pAes18TxGetIBState =
        &hr.u.cx.u.aes18tx_internal_buffer_state;
    HPI_InitMessage( &hm,  HPI_OBJ_CONTROLEX, HPI_CONTROL_GET_STATE );
    HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.cx.wControlIndex );
    hm.u.cx.wAttribute = HPI_AES18_INTERNAL_BUFFER_STATE;

    wBitShift = 0;

#ifndef HAS_AES18_ON_HOST

    HPI_Message( phSubSys, &hm, &hr );
#else

    AES18_Message(HPI_CONTROL_AES18_TRANSMITTER,&hm, &hr);
#endif

    wPackedBits = pAes18TxGetIBState->wInternalBufferBusyPacked;

    /* Unpack the status bits from the response */
    if(awInternalBufferBusy)
    {
        for(wChannelCount=0;wChannelCount<HPI_AES18_MAX_CHANNELS;wChannelCount++)
        {
            for(wPriorityCount=0;wPriorityCount<HPI_AES18_MAX_PRIORITIES;wPriorityCount++)
            {
                awInternalBufferBusy[wChannelCount][wPriorityCount] =
                    (wPackedBits >> (wBitShift++)) & 1;
            }
        }
    }
    return(hr.wError);

};

HW16 HPI_AES18TxGetInternalBufferSize(
    HPI_HSUBSYS     *phSubSys,
    HPI_HCONTROL    hControlHandle,
    HW16            awBytesPerBuffer[HPI_AES18_MAX_PRIORITIES]
)
{
    HW16 wPriorityCount;
    HPI_MESSAGE hm;
    HPI_RESPONSE hr;
    HPI_CONTROLX_RES_AES18TX_BUFFER_SIZE *pAes18TxGetIBSize =
        &hr.u.cx.u.aes18tx_internal_buffer_size;
	 HPI_InitMessage( &hm,  HPI_OBJ_CONTROLEX, HPI_CONTROL_GET_STATE );
    HPI_HANDLETOINDEXES( hControlHandle, &hm.wAdapterIndex, &hm.u.cx.wControlIndex );
    hm.u.cx.wAttribute = HPI_AES18_INTERNAL_BUFFER_SIZE;

#ifndef HAS_AES18_ON_HOST

    HPI_Message( phSubSys, &hm, &hr );
#else

    AES18_Message(HPI_CONTROL_AES18_TRANSMITTER,&hm, &hr);
#endif

    if(awBytesPerBuffer)
    {
        for(wPriorityCount=0;wPriorityCount<HPI_AES18_MAX_PRIORITIES;wPriorityCount++)
        {
            awBytesPerBuffer[wPriorityCount] =
                pAes18TxGetIBSize->awBytesPerBuffer[wPriorityCount];
        }
    }
    return(hr.wError);
}

#ifdef HAS_AES18_ON_HOST
/* prototype the functions in aes18.c */
int Aes18BgCtrl_Set(int nIndex,  HPI_CONTROL_MSG *pCm, HPI_CONTROL_RES *pCr);
int Aes18RxCtrl_Set(int nIndex,  HPI_CONTROL_MSG *pCm, HPI_CONTROL_RES *pCr);
int Aes18RxCtrl_Get(int nIndex,  HPI_CONTROL_MSG *pCm, HPI_CONTROL_RES *pCr);
int Aes18TxCtrl_Set(int nIndex,  HPI_CONTROL_MSG *pCm, HPI_CONTROL_RES *pCr);
int Aes18TxCtrl_Get(int nIndex,  HPI_CONTROL_MSG *pCm, HPI_CONTROL_RES *pCr);

// Copied from ax4\axctrl.c DSP code.
// this function takes the control type as a parameter because when running
// on the pc, we dont get a correct control index

void AES18_Message(int wControlType, HPI_MESSAGE *pHm, HPI_RESPONSE *pHr)
{
    HPI_CONTROL_MSG * pCm= &pHm->u.c;
    HPI_CONTROL_RES * pCr= &pHr->u.c;
    int nIndex=0;

    pHr->wError = HPI_ERROR_INVALID_CONTROL;
    //  if( pCm->wControlIndex > nNumControls )
    //      return;

    switch(pHm->wFunction)
    {
    case HPI_CONTROL_GET_STATE:
        switch (wControlType)
        {
        case HPI_CONTROL_AES18_TRANSMITTER:
            Aes18TxCtrl_Get(nIndex,pCm,pCr);
            break;
        case HPI_CONTROL_AES18_RECEIVER:
            Aes18RxCtrl_Get(nIndex,pCm,pCr);
            break;
        case HPI_CONTROL_AES18_BLOCKGENERATOR:
            break;
        }
        break;
    case HPI_CONTROL_SET_STATE:
        switch (wControlType)
        {
        case HPI_CONTROL_AES18_TRANSMITTER:
            Aes18TxCtrl_Set(nIndex,pCm,pCr);
            break;
        case HPI_CONTROL_AES18_RECEIVER:
            Aes18RxCtrl_Set(nIndex,pCm,pCr);
            break;
        case HPI_CONTROL_AES18_BLOCKGENERATOR:
            Aes18BgCtrl_Set(nIndex,pCm,pCr);
            break;
        }
        break;
    default:
        return;
    }

}
#endif /* HAS_AES18_ON_HOST */
///\}
/////////////////////////////////////////////////////////////////////////
/** \defgroup utility Utility Functions 
@{
*/

/// Get HW specific errors
void HPI_GetLastErrorDetail(HW16 wError, char *pszErrorText,  HW32 **padwSpecificError )
{
    *padwSpecificError = &gadwHpiSpecificError[0]; 	// send back pointer to array of errors
    HPI_GetErrorText( wError, pszErrorText);
}

/**
\deprecated  This function was marked obsolete - don't know why?
*/
void HPI_GetErrorText( HW16 wError, char *pszErrorText)
{
    strcpy(pszErrorText," ");
#ifndef HPI_OS_WIN95_KERN
	#ifndef HPI_OS_WINNT_KERN
	#ifndef HPI_OS_WDM

    sprintf(pszErrorText, "#%3d - ", wError);
#endif
	#endif
	#endif

    switch(wError)
	 {
	 case 0:
		  strcat( pszErrorText, "No Error");
		  break;
	 case HPI_ERROR_INVALID_TYPE:            //100 // message type does not exist
		  strcat( pszErrorText, "Invalid message type");
		  break;
	 case HPI_ERROR_INVALID_OBJ:             //101 // object type does not exist
        strcat( pszErrorText, "Invalid object type");
        break;
    case HPI_ERROR_INVALID_FUNC:            //102 // function does not exist
        strcat( pszErrorText, "Invalid function");
        break;
    case HPI_ERROR_INVALID_OBJ_INDEX:     //103 // the specified object (adapter/Stream) does not exist
        strcat( pszErrorText, "Invalid object (adapter/stream/mixer/control) index");
        break;
    case HPI_ERROR_OBJ_NOT_OPEN:          //104
        strcat( pszErrorText, "Object not open");
        break;
    case HPI_ERROR_OBJ_ALREADY_OPEN:                //105
        strcat( pszErrorText, "Object already in use (opened)");
        break;
    case HPI_ERROR_INVALID_RESOURCE:                //106 // PCI, ISA resource not valid
        strcat( pszErrorText, "Invalid bus/port resource");
        break;
    case HPI_ERROR_SUBSYSFINDADAPTERS_GETINFO:                //107
        strcat( pszErrorText, "GetInfo call from SubSysFindAdapters failed");
        break;
    case HPI_ERROR_INVALID_RESPONSE:                // 108
        strcat( pszErrorText, "Response structure not updated");
        break;
	case HPI_ERROR_PROCESSING_MESSAGE:    // 109
		strcat( pszErrorText, "wSize field of response was not updated");
		break;
	case HPI_ERROR_NETWORK_TIMEOUT:      //110
		strcat( pszErrorText, "Network timeout waiting for response");
		break;

    case HPI_ERROR_TOO_MANY_ADAPTERS:     //200
        strcat( pszErrorText, "Too many adapters");
        break;
    case HPI_ERROR_BAD_ADAPTER:                     //201
        strcat( pszErrorText, "Bad adapter");
        break;
    case HPI_ERROR_BAD_ADAPTER_NUMBER:      //202    // adapter number out of range or not set properly
        strcat( pszErrorText, "Invalid adapter index");
        break;
    case HPI_DUPLICATE_ADAPTER_NUMBER:      //203    // 2 adapters with the same adapter number
        strcat( pszErrorText, "Duplicate adapter index");
        break;
    case HPI_ERROR_DSP_BOOTLOAD:      //204
        strcat( pszErrorText, "DSP failed bootload");
        break;
    case HPI_ERROR_DSP_SELFTEST:      //205
        strcat( pszErrorText, "DSP failed selftest");
        break;
    case HPI_ERROR_DSP_FILE_NOT_FOUND:      //206
        strcat( pszErrorText, "Failed to find/open DSP code file");
        break;
    case HPI_ERROR_PLD_LOAD:      //209
        strcat( pszErrorText, "PLD could not be configured");
        break;
    case HPI_ERROR_DSP_FILE_FORMAT:      //210
        strcat( pszErrorText, "Invalid DSP code file format (corrupt file?)");
        break;

    case HPI_ERROR_INVALID_STREAM:                      //300
        strcat( pszErrorText, "Invalid stream");
        break;
    case HPI_ERROR_INVALID_FORMAT:  //301
        strcat( pszErrorText, "Invalid Format");
        break;
    case HPI_ERROR_INVALID_SAMPLERATE:  //  302
        strcat( pszErrorText, "Invalid format sample rate");
        break;
    case HPI_ERROR_INVALID_CHANNELS:    //  303
        strcat( pszErrorText, "Invalid format number of channels");
        break;
    case HPI_ERROR_INVALID_BITRATE:     //  304
        strcat( pszErrorText, "Invalid format bitrate");
        break;
    case HPI_ERROR_INVALID_DATASIZE:    // 305
        strcat( pszErrorText, "Invalid datasize used for stream read/write");
        break;
    case HPI_ERROR_BUFFER_FULL:    // 306
        strcat( pszErrorText, "Stream buffer is full during stream write");
        break;
    case HPI_ERROR_BUFFER_EMPTY:    // 307
        strcat( pszErrorText, "stream buffer is empty during stream read");
        break;
    case HPI_ERROR_INVALID_DATA_TRANSFER:    // 308
        strcat( pszErrorText, "invalid datasize used for stream read/write");
        break;
    case HPI_ERROR_INVALID_OPERATION:    // 310
        strcat( pszErrorText, "Invalid operation - object can't do requested operation in its current state");
        break;
    case HPI_ERROR_INCOMPATIBLE_SAMPLERATE:    // 311
        strcat( pszErrorText, "Cannot change to requested samplerate");
        break;
    case HPI_ERROR_BAD_ADAPTER_MODE:    // 312
        strcat( pszErrorText, "Invalid adapter mode");
        break;

    case HPI_ERROR_INVALID_NODE:                 //400
        strcat( pszErrorText, "Invalid mixer node");
        break;
    case HPI_ERROR_INVALID_CONTROL:                 //401
        strcat( pszErrorText, "Invalid mixer control");
        break;
    case HPI_ERROR_INVALID_CONTROL_VALUE:    // 402
        strcat( pszErrorText, "Invalid mixer control value");
        break;
	case HPI_ERROR_INVALID_CONTROL_ATTRIBUTE: // 403
        strcat( pszErrorText, "Invalid mixer control attribute");
        break;
	case HPI_ERROR_CONTROL_DISABLED: // 404
        strcat( pszErrorText, "Mixer control disabled");
        break;

	case HPI_ERROR_NVMEM_BUSY: // 450
        strcat( pszErrorText, "Non-volatile memory is busy");
        break;

	case HPI_ERROR_AES18BG_BLOCKSPERSEC: //  500
	case HPI_ERROR_AES18BG_PRIORITYMASK: //  501
	case HPI_ERROR_AES18BG_MODE: //          502
	case HPI_ERROR_AES18_INVALID_PRIORITY: //    503
	case HPI_ERROR_AES18_INVALID_ADDRESS: // 504
	case HPI_ERROR_AES18_INVALID_REPETITION: //  505
	case HPI_ERROR_AES18BG_CHANNEL_MODE: //      506
	case HPI_ERROR_AES18_INVALID_CHANNEL: //  507
        strcat( pszErrorText, "an AES18 error");
        break;

    default:
        strcat(pszErrorText, "Unknown Error");
    }
}


/** Initialize an audio format structure, given various defining parameters

*/
HW16 HPI_FormatCreate(
    HPI_FORMAT *pFormat, ///< [out] Format structure to be initialized
    HW16 wChannels, ///< From 1 to 8
    HW16 wFormat, ///< One of the \ref formats
    HW32 dwSampleRate, ///< Hz. Samplerate must be between 8000 and 200000
    HW32 dwBitRate, ///< bits per second, must be supplied for MPEG formats, it is calculated for PCM formats
    HW32 dwAttributes ///< format dependent attributes.  E.g. \ref mpegmodes
)
{
    HW16 wError=0;

    // can be mono or stereo
    if(wChannels<1)
        wChannels=1;
    if(wChannels>8)
        wChannels=8;
    pFormat->wChannels = wChannels;

    // make sure we have a valid audio format
    switch(wFormat)
    {
    case HPI_FORMAT_PCM16_SIGNED:
    case HPI_FORMAT_PCM24_SIGNED:
    case HPI_FORMAT_PCM32_SIGNED:
    case HPI_FORMAT_PCM32_FLOAT:
	case HPI_FORMAT_PCM16_BIGENDIAN:
	case HPI_FORMAT_PCM8_UNSIGNED:
	case HPI_FORMAT_MPEG_L1:
	case HPI_FORMAT_MPEG_L2:
	case HPI_FORMAT_MPEG_L3:
	case HPI_FORMAT_DOLBY_AC2:
	case HPI_FORMAT_AA_TAGIT1_HITS:
	case HPI_FORMAT_AA_TAGIT1_INSERTS:
    case HPI_FORMAT_RAW_BITSTREAM:
    case HPI_FORMAT_AA_TAGIT1_HITS_EX1:
    case HPI_FORMAT_OEM1:
    case HPI_FORMAT_OEM2:
        break;
    default:
        wError = HPI_ERROR_INVALID_FORMAT;
        return(wError);
    }
    pFormat->wFormat = wFormat;

    //sample rate can be between 8kHz and 200kHz
    if(dwSampleRate<8000L)
    {
        wError=HPI_ERROR_INCOMPATIBLE_SAMPLERATE;
        dwSampleRate = 8000L;
    }
	 if(dwSampleRate>200000L)
	 {
		  wError=HPI_ERROR_INCOMPATIBLE_SAMPLERATE;
		dwSampleRate = 200000L;
    }
    pFormat->dwSampleRate = dwSampleRate;

    // for some formats (MPEG) we accept a bitrate
    // for some (PCM) we calculate the bit rate
    switch(wFormat)
    {
    case HPI_FORMAT_MPEG_L1:
    case HPI_FORMAT_MPEG_L2:
    case HPI_FORMAT_MPEG_L3:
        pFormat->dwBitRate = dwBitRate; // should validate!!!!!!!
        break;
    case HPI_FORMAT_PCM16_SIGNED:
	case HPI_FORMAT_PCM16_BIGENDIAN:
		pFormat->dwBitRate = (HW32)wChannels * dwSampleRate * 2;
		break;
	case HPI_FORMAT_PCM32_SIGNED:
	case HPI_FORMAT_PCM32_FLOAT:
		pFormat->dwBitRate = (HW32)wChannels * dwSampleRate * 4;
        break;
	case HPI_FORMAT_PCM8_UNSIGNED:
        pFormat->dwBitRate = (HW32)wChannels * dwSampleRate;
        break;
    default:
        pFormat->dwBitRate=0;
	 }

    // Set the dwAttributes field.
    // The attributes are format dependent.
    switch(wFormat)
    {
    case HPI_FORMAT_MPEG_L2:
    	if( (wChannels==1) && (dwAttributes!=HPI_MPEG_MODE_DEFAULT) )
    	{
            dwAttributes = HPI_MPEG_MODE_DEFAULT;	// correct the error anyway !
	        wError = HPI_ERROR_INVALID_FORMAT;
	    }
	    else if(dwAttributes > HPI_MPEG_MODE_DUALCHANNEL)
	    {
		    dwAttributes = HPI_MPEG_MODE_DEFAULT;	// correct the error anyway !
	        wError = HPI_ERROR_INVALID_FORMAT;
		}
	    pFormat->dwAttributes = dwAttributes;
        break;
    default:
	    pFormat->dwAttributes = dwAttributes;
    }

    return(wError);
}

#if defined(HPI_OS_WIN16) || defined(HPI_OS_WIN32_USER)  || defined(INCLUDE_WINDOWS_ON_LINUX)
static unsigned char PcmSubformatGUID[16] = { 1,0,0,0,0,0,0x10,0,0x80,0,0,0xAA,0,0x38,0x9B,0x71 };

HW16 HPI_WaveFormatToHpiFormat(
		const WAVEFORMATEX *lpFormatEx,
		HPI_FORMAT *pHpiFormat
)
{
	HW16 wError = 0;

	switch(lpFormatEx->wFormatTag)
	{
#if defined( WAVE_FORMAT_EXTENSIBLE ) && defined( _WAVEFORMATEXTENSIBLE_ )
	case WAVE_FORMAT_EXTENSIBLE:
		// Make sure the subformat is PCM
		if(memcmp(&((PWAVEFORMATEXTENSIBLE)lpFormatEx)->SubFormat,PcmSubformatGUID,16)!=0)
		{
			wError = HPI_ERROR_INVALID_FORMAT;
			break;
		}
		// else fallthrough
#endif
	case WAVE_FORMAT_PCM:
		//DBGPRINTF0(DEBUG_MASK_WOD_CUSTOM,TEXT("PCM"));
		if(lpFormatEx->wBitsPerSample == 16)
			pHpiFormat->wFormat = HPI_FORMAT_PCM16_SIGNED;
		else if (lpFormatEx->wBitsPerSample == 8)
			pHpiFormat->wFormat = HPI_FORMAT_PCM8_UNSIGNED;
		else if (lpFormatEx->wBitsPerSample == 24)
			pHpiFormat->wFormat = HPI_FORMAT_PCM24_SIGNED;
		else if (lpFormatEx->wBitsPerSample == 32)
			pHpiFormat->wFormat = HPI_FORMAT_PCM32_SIGNED;
		else
		{
			pHpiFormat->wFormat = 0;
			wError = HPI_ERROR_INVALID_FORMAT;
		}
		break;
	case WAVE_FORMAT_IEEE_FLOAT:
		if(lpFormatEx->wBitsPerSample == 32)
			pHpiFormat->wFormat = HPI_FORMAT_PCM32_FLOAT;
		else
		{
			pHpiFormat->wFormat = 0;
			wError = HPI_ERROR_INVALID_FORMAT;
		}
		break;

	case WAVE_FORMAT_DOLBY_AC2:
		//DBGPRINTF0(DEBUG_MASK_WOD_CUSTOM,TEXT("AC2"));
		pHpiFormat->wFormat = HPI_FORMAT_DOLBY_AC2;
		break;

	case WAVE_FORMAT_MPEG:
		switch(((MPEG1WAVEFORMAT*)lpFormatEx)->fwHeadLayer)
		{
		case ACM_MPEG_LAYER1: pHpiFormat->wFormat = HPI_FORMAT_MPEG_L1; break;
		case ACM_MPEG_LAYER2: pHpiFormat->wFormat = HPI_FORMAT_MPEG_L2; break;
		case ACM_MPEG_LAYER3: pHpiFormat->wFormat = HPI_FORMAT_MPEG_L3; break;
		default:
			pHpiFormat->wFormat = HPI_FORMAT_MPEG_L2; break;   // really should be error
		}
		pHpiFormat->dwBitRate = ((MPEG1WAVEFORMAT*)lpFormatEx)->dwHeadBitrate;
		if(pHpiFormat->dwBitRate==0)
			pHpiFormat->dwBitRate= 256000L;		// must have a default
		pHpiFormat->wMode = 0; //((MPEG1WAVEFORMAT*)lpFormatEx)->fwHeadMode;
		break;
	case WAVE_FORMAT_MPEGLAYER3:
		pHpiFormat->wFormat = HPI_FORMAT_MPEG_L3;
		pHpiFormat->dwBitRate = ((MPEGLAYER3WAVEFORMAT*)lpFormatEx)->wfx.nAvgBytesPerSec * 8;
		if(pHpiFormat->dwBitRate==0)
			pHpiFormat->dwBitRate = 256000L;	// must have a default
		pHpiFormat->wMode = 0; //((MPEG1WAVEFORMAT*)lpFormatEx)->fwHeadMode;
		break;

	default:
		wError = HPI_ERROR_INVALID_FORMAT;
	}
	pHpiFormat->wChannels = lpFormatEx->nChannels;
	pHpiFormat->dwSampleRate = lpFormatEx->nSamplesPerSec;
	pHpiFormat->wMode = 0;
	pHpiFormat->dwAttributes=0;

	return(wError);
}

HW16 HPI_HpiFormatToWaveFormat(
		const HPI_FORMAT *pHpiFormat,
		WAVEFORMATEX *lpFormatEx
)
{
	HW16 wError = 0;

    lpFormatEx->cbSize = 0;

    lpFormatEx->nChannels		= pHpiFormat->wChannels;
    lpFormatEx->nSamplesPerSec	= pHpiFormat->dwSampleRate;

    switch(pHpiFormat->wFormat)
    {
    case HPI_FORMAT_PCM8_UNSIGNED:
        lpFormatEx->nAvgBytesPerSec	= pHpiFormat->wChannels*pHpiFormat->dwSampleRate;
        lpFormatEx->nBlockAlign		= pHpiFormat->wChannels;
        lpFormatEx->wBitsPerSample	= 8;
        lpFormatEx->wFormatTag = WAVE_FORMAT_PCM;
        break;
    case HPI_FORMAT_PCM16_SIGNED:
        lpFormatEx->nAvgBytesPerSec	= 2*pHpiFormat->wChannels*pHpiFormat->dwSampleRate;
        lpFormatEx->nBlockAlign		= 2*pHpiFormat->wChannels;
        lpFormatEx->wBitsPerSample	= 16;
        lpFormatEx->wFormatTag = WAVE_FORMAT_PCM;
        break;
	case HPI_FORMAT_PCM24_SIGNED:
		lpFormatEx->nAvgBytesPerSec	= 3*pHpiFormat->wChannels*pHpiFormat->dwSampleRate;
		lpFormatEx->nBlockAlign		= 3*pHpiFormat->wChannels;
		lpFormatEx->wBitsPerSample	= 24;
		lpFormatEx->wFormatTag = WAVE_FORMAT_PCM;
		break;

    case HPI_FORMAT_PCM32_SIGNED:
        lpFormatEx->nAvgBytesPerSec	= 4*pHpiFormat->wChannels*pHpiFormat->dwSampleRate;
        lpFormatEx->nBlockAlign		= 4*pHpiFormat->wChannels;
        lpFormatEx->wBitsPerSample	= 32;
        lpFormatEx->wFormatTag = WAVE_FORMAT_PCM;
        break;

    case HPI_FORMAT_PCM32_FLOAT:
        lpFormatEx->nAvgBytesPerSec	= 4*pHpiFormat->wChannels*pHpiFormat->dwSampleRate;
        lpFormatEx->nBlockAlign		= 4*pHpiFormat->wChannels;
        lpFormatEx->wBitsPerSample	= 32;
        lpFormatEx->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        break;

    case HPI_FORMAT_AA_TAGIT1_HITS:
		  lpFormatEx->nAvgBytesPerSec	= 2*pHpiFormat->wChannels*pHpiFormat->dwSampleRate;
        lpFormatEx->nBlockAlign		= 2*pHpiFormat->wChannels;
        lpFormatEx->wBitsPerSample	= 16;
        lpFormatEx->wFormatTag = WAVE_FORMAT_DEVELOPMENT;
        break;


        //this is setup as MPEG layer 2
    case HPI_FORMAT_MPEG_L2:
    case HPI_FORMAT_MPEG_L3:
        lpFormatEx->wFormatTag = WAVE_FORMAT_MPEG;
        lpFormatEx->wBitsPerSample	= 0;
        // this is for 44.1kHz, stereo, 256kbs
        lpFormatEx->nBlockAlign
        = (unsigned short)((144*pHpiFormat->dwBitRate)/pHpiFormat->dwSampleRate);
        lpFormatEx->nAvgBytesPerSec	= pHpiFormat->dwBitRate/8;
        lpFormatEx->cbSize = 22;
        if(pHpiFormat->wFormat==HPI_FORMAT_MPEG_L2)
            ((MPEG1WAVEFORMAT*)lpFormatEx)->fwHeadLayer = ACM_MPEG_LAYER2;
        else
            ((MPEG1WAVEFORMAT*)lpFormatEx)->fwHeadLayer = ACM_MPEG_LAYER3;
        //this makes the desired rather than actual
		((MPEG1WAVEFORMAT*)lpFormatEx)->dwHeadBitrate = pHpiFormat->dwBitRate;
        if(pHpiFormat->wChannels==2)
				((MPEG1WAVEFORMAT*)lpFormatEx)->fwHeadMode = ACM_MPEG_JOINTSTEREO;
        else
            ((MPEG1WAVEFORMAT*)lpFormatEx)->fwHeadMode = ACM_MPEG_SINGLECHANNEL;

        ((MPEG1WAVEFORMAT*)lpFormatEx)->fwHeadModeExt = 0; //0x000F;   //??
        ((MPEG1WAVEFORMAT*)lpFormatEx)->wHeadEmphasis = 0;
        ((MPEG1WAVEFORMAT*)lpFormatEx)->fwHeadFlags = 0; //ACM_MPEG_ID_MPEG1;
        ((MPEG1WAVEFORMAT*)lpFormatEx)->dwPTSLow = 0;
        ((MPEG1WAVEFORMAT*)lpFormatEx)->dwPTSHigh = 0;
        break;

    default:
		wError = HPI_ERROR_INVALID_FORMAT;
    }

	return(wError);
}

#endif /* defined(HPI_OS_WIN16) || defined(HPI_OS_WIN32_USER) */

/// Initialize a HPI_DATA structure
HW16 HPI_DataCreate(
    HPI_DATA *pData, ///<[inout] Structure to be initialised
    HPI_FORMAT *pFormat, ///<[in] format of the data
    HW8 *pbData,///<[in] pointer to data buffer
    HW32 dwDataSize///<[in] amount of data in buffer
)
{
	 memcpy( &pData->Format, pFormat, sizeof(HPI_FORMAT));
	 pData->dwpbData = (HW32)pbData;
	 pData->dwDataSize = dwDataSize;
	 return(0);
}

/// The actual message size for each object type
static HW16 aMsgSize[HPI_OBJ_MAXINDEX +1] = HPI_MESSAGE_SIZE_BY_OBJECT;
/// The actual response size for each object type
static HW16 aResSize[HPI_OBJ_MAXINDEX +1] = HPI_RESPONSE_SIZE_BY_OBJECT;

/// initialize the HPI message structure
void HPI_InitMessage( HPI_MESSAGE *phm, HW16 wObject, HW16 wFunction )
{
	memset(phm,0,sizeof(HPI_MESSAGE));
	if ((wObject > 0) && (wObject <= HPI_OBJ_MAXINDEX))
	{
		phm->wType = HPI_TYPE_MESSAGE;
		phm->wSize = aMsgSize[wObject];
		phm->wObject = wObject;
		phm->wFunction = wFunction;
		phm->wDspIndex = 0;
		// Expect adapter index to be set by caller
	}
	else
	{
		phm->wType = 0;
		phm->wSize = 0;
	}
}

/// initialize the HPI response structure
void HPI_InitResponse( HPI_RESPONSE *phr, HW16 wObject, HW16 wFunction, HW16 wError)
{
	 memset(phr,0,sizeof(HPI_RESPONSE));
	 phr->wType = HPI_TYPE_RESPONSE;
	 phr->wSize = aResSize[wObject];
	 phr->wObject = wObject;
	 phr->wFunction = wFunction;
	 phr->wError = wError;
	 phr->wSpecificError = 0;
}

/** @} */ // group utility
////////////////////////////////////////////////////////////////////////
// local functions

/** make a handle to an object from the indexes that reference it
	\sa HPI_HandleToIndexes()
*/
HW32 HPI_IndexesToHandle( 
	const char cObject, ///< a character representing the type of object
	const HW16 wIndex1, ///< The Adapter index
	const HW16 wIndex2 ///< The stream or control index, if used
)
{
    HW32 dwHandle=0;
    dwHandle = (((HW32)cObject)<<24) | ((HW32)(wIndex1 & 0xFFF) << 12) | ((HW32)(wIndex2 & 0xFFF));
    return(dwHandle);
}

/** turn a handle back into indexes
	/sa HPI_IndexesToHandle()
*/
void HPI_HandleToIndexes( const HW32 dwHandle, HW16 *pwIndex1, HW16 *pwIndex2 )
{
    if(pwIndex1)
        *pwIndex1 = (HW16)((dwHandle >> 12) & 0xFFF);
    if(pwIndex2)
        *pwIndex2 = (HW16)(dwHandle & 0xFFF);
}


/**
   Encode 3 indices and an object type into a 32 bit handle.

	\return Object handle
	\sa HPI_HandleToIndexes3()
*/
HW32 HPI_IndexesToHandle3(
    const char cObject, ///< a character representing the type of object
    const HW16 wAdapterIndex, ///< The Adapter index
    const HW16 wObjectIndex, ///< The stream or control index, if used
    const HW16 wDspIndex  ///< The index of the DSP which implements the object
)
{
    HW32 dwHandle=0;
    dwHandle = (((HW32)cObject)<<24) |
               ((HW32)(wDspIndex & 0xFF) << 16) |
               ((HW32)(wAdapterIndex & 0xFF) << 8) |
               ((HW32)(wObjectIndex & 0xFF));
    return(dwHandle);
}

/**
	Extract up to 3 indices from an object handle, if non-NULL pointers are supplied

	/sa HPI_IndexesToHandle3(), HPI_IndexesToHandle()
*/

void HPI_HandleToIndexes3(
    const HW32 dwHandle, ///< The handle to decode
    HW16 *pwAdapterIndex,  ///< Where to store the Adapter index
    HW16 *pwObjectIndex,  ///< Where to store the stream or control index, if used
    HW16 *pwDspIndex 	 ///< Where to store the index of the DSP which implements the object
)
{
    if(pwDspIndex)
        *pwDspIndex = (HW16)((dwHandle >> 16) & 0xFF);
    if(pwAdapterIndex)
        *pwAdapterIndex = (HW16)((dwHandle >> 8) & 0xFF);
    if(pwObjectIndex)
        *pwObjectIndex = (HW16)(dwHandle & 0xFF);
}

///////////////////////////////////////////////////////////////////////////
