
#include "hpi_internal.h"
#include "hpimsginit.h"

#include "hpidebug.h"

struct hpi_handle {
	unsigned int objIndex:12;
	unsigned int objType:4;
	unsigned int adapterIndex:14;
	unsigned int spare:1;
	unsigned int readOnly:1;
};

union handle_word {
	struct hpi_handle h;
	u32 w;
};

u32 HPI_IndexesToHandle(
	const char cObject,
	const u16 wAdapterIndex,
	const u16 wObjectIndex
)
{
	union handle_word handle;

	handle.h.adapterIndex = wAdapterIndex;
	handle.h.spare = 0;
	handle.h.readOnly = 0;
	handle.h.objType = cObject;
	handle.h.objIndex = wObjectIndex;
	return handle.w;
}

void HPI_HandleToIndexes(
	const u32 dwHandle,
	u16 *pwAdapterIndex,
	u16 *pwObjectIndex
)
{
	union handle_word handle;
	handle.w = dwHandle;

	if (pwAdapterIndex)
		*pwAdapterIndex = (u16)handle.h.adapterIndex;
	if (pwObjectIndex)
		*pwObjectIndex = (u16)handle.h.objIndex;
}

char HPI_HandleObject(
	const u32 dwHandle
)
{
	union handle_word handle;
	handle.w = dwHandle;
	return (char)handle.h.objType;
}

#define u32TOINDEX(h, i1) \
do {\
	if (h == 0) \
		return HPI_ERROR_INVALID_OBJ; \
	else \
		HPI_HandleToIndexes(h, i1, NULL); \
} while (0)

#define u32TOINDEXES(h, i1, i2) \
do {\
	if (h == 0) \
		return HPI_ERROR_INVALID_OBJ; \
	else \
		HPI_HandleToIndexes(h, i1, i2);\
} while (0)

void HPI_FormatToMsg(
	struct hpi_msg_format *pMF,
	struct hpi_format *pF
)
{
	pMF->dwSampleRate = pF->dwSampleRate;
	pMF->dwBitRate = pF->dwBitRate;
	pMF->dwAttributes = pF->dwAttributes;
	pMF->wChannels = pF->wChannels;
	pMF->wFormat = pF->wFormat;
}

static void HPI_MsgToFormat(
	struct hpi_format *pF,
	struct hpi_msg_format *pMF
)
{
	pF->dwSampleRate = pMF->dwSampleRate;
	pF->dwBitRate = pMF->dwBitRate;
	pF->dwAttributes = pMF->dwAttributes;
	pF->wChannels = pMF->wChannels;
	pF->wFormat = pMF->wFormat;
	pF->wModeLegacy = 0;
	pF->wUnused = 0;
}

void HPI_StreamResponseToLegacy(
	struct hpi_stream_res *pSR
)
{
	pSR->u.legacy_stream_info.dwAuxiliaryDataAvailable =
		pSR->u.stream_info.dwAuxiliaryDataAvailable;
	pSR->u.legacy_stream_info.wState = pSR->u.stream_info.wState;
}

static struct hpi_hsubsys ghSubSys;

struct hpi_hsubsys *HPI_SubSysCreate(
	void
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	memset(&ghSubSys, 0, sizeof(struct hpi_hsubsys));

	{
		HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_SUBSYSTEM,
			HPI_SUBSYS_OPEN);
		HPI_Message(&hm, &hr);

		if (hr.wError == 0)
			return (&ghSubSys);

	}
	return (NULL);
}

void HPI_SubSysFree(
	struct hpi_hsubsys *phSubSys
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_SUBSYSTEM,
		HPI_SUBSYS_CLOSE);
	HPI_Message(&hm, &hr);

}

u16 HPI_SubSysGetVersion(
	struct hpi_hsubsys *phSubSys,
	u32 *pdwVersion
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_SUBSYSTEM,
		HPI_SUBSYS_GET_VERSION);
	HPI_Message(&hm, &hr);
	*pdwVersion = hr.u.s.dwVersion;
	return hr.wError;
}

u16 HPI_SubSysGetVersionEx(
	struct hpi_hsubsys *phSubSys,
	u32 *pdwVersionEx
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_SUBSYSTEM,
		HPI_SUBSYS_GET_VERSION);
	HPI_Message(&hm, &hr);
	*pdwVersionEx = hr.u.s.dwData;
	return hr.wError;
}

u16 HPI_SubSysGetInfo(
	struct hpi_hsubsys *phSubSys,
	u32 *pdwVersion,
	u16 *pwNumAdapters,
	u16 awAdapterList[],
	u16 wListLength
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_SUBSYSTEM,
		HPI_SUBSYS_GET_INFO);

	HPI_Message(&hm, &hr);

	*pdwVersion = hr.u.s.dwVersion;
	if (wListLength > HPI_MAX_ADAPTERS)
		memcpy(awAdapterList, &hr.u.s.awAdapterList,
			HPI_MAX_ADAPTERS);
	else
		memcpy(awAdapterList, &hr.u.s.awAdapterList, wListLength);
	*pwNumAdapters = hr.u.s.wNumAdapters;
	return hr.wError;
}

u16 HPI_SubSysFindAdapters(
	struct hpi_hsubsys *phSubSys,
	u16 *pwNumAdapters,
	u16 awAdapterList[],
	u16 wListLength
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_SUBSYSTEM,
		HPI_SUBSYS_FIND_ADAPTERS);

	HPI_Message(&hm, &hr);

	if (wListLength > HPI_MAX_ADAPTERS) {
		memcpy(awAdapterList, &hr.u.s.awAdapterList,
			HPI_MAX_ADAPTERS * sizeof(u16));
		memset(&awAdapterList[HPI_MAX_ADAPTERS], 0,
			(wListLength - HPI_MAX_ADAPTERS) * sizeof(u16));
	} else
		memcpy(awAdapterList, &hr.u.s.awAdapterList,
			wListLength * sizeof(u16));
	*pwNumAdapters = hr.u.s.wNumAdapters;

	return hr.wError;
}

u16 HPI_SubSysCreateAdapter(
	struct hpi_hsubsys *phSubSys,
	struct hpi_resource *pResource,
	u16 *pwAdapterIndex
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_SUBSYSTEM,
		HPI_SUBSYS_CREATE_ADAPTER);
	hm.u.s.Resource = *pResource;

	HPI_Message(&hm, &hr);

	*pwAdapterIndex = hr.u.s.wAdapterIndex;
	return hr.wError;
}

u16 HPI_SubSysDeleteAdapter(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_SUBSYSTEM,
		HPI_SUBSYS_DELETE_ADAPTER);
	hm.wAdapterIndex = wAdapterIndex;
	HPI_Message(&hm, &hr);
	return hr.wError;
}

u16 HPI_SubSysGetNumAdapters(
	struct hpi_hsubsys *phSubSys,
	int *pnNumAdapters
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_SUBSYSTEM,
		HPI_SUBSYS_GET_NUM_ADAPTERS);
	HPI_Message(&hm, &hr);
	*pnNumAdapters = (int)hr.u.s.wNumAdapters;
	return hr.wError;
}

u16 HPI_SubSysGetAdapter(
	struct hpi_hsubsys *phSubSys,
	int nIterator,
	u32 *pdwAdapterIndex,
	u16 *pwAdapterType
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_SUBSYSTEM,
		HPI_SUBSYS_GET_ADAPTER);
	hm.wAdapterIndex = (u16)nIterator;
	HPI_Message(&hm, &hr);
	*pdwAdapterIndex = (int)hr.u.s.wAdapterIndex;
	*pwAdapterType = hr.u.s.awAdapterList[0];
	return hr.wError;
}

u16 HPI_SubSysSetHostNetworkInterface(
	struct hpi_hsubsys *phSubSys,
	char *szInterface
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_SUBSYSTEM,
		HPI_SUBSYS_SET_NETWORK_INTERFACE);
	if (szInterface == NULL)
		return HPI_ERROR_INVALID_RESOURCE;
	hm.u.s.Resource.r.net_if = szInterface;
	HPI_Message(&hm, &hr);
	return hr.wError;
}

u16 HPI_AdapterOpen(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ADAPTER, HPI_ADAPTER_OPEN);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	return hr.wError;

}

u16 HPI_AdapterClose(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ADAPTER, HPI_ADAPTER_CLOSE);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_AdapterFindObject(
	const struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wObjectType,
	u16 wObjectIndex,
	u16 *pDspIndex
)
{
	return HPI_ERROR_UNIMPLEMENTED;
}

u16 HPI_AdapterSetMode(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 dwAdapterMode
)
{
	return HPI_AdapterSetModeEx(phSubSys, wAdapterIndex, dwAdapterMode,
		HPI_ADAPTER_MODE_SET);
}

u16 HPI_AdapterSetModeEx(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 dwAdapterMode,
	u16 wQueryOrSet
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ADAPTER,
		HPI_ADAPTER_SET_MODE);
	hm.wAdapterIndex = wAdapterIndex;
	hm.u.a.dwAdapterMode = dwAdapterMode;
	hm.u.a.wAssertId = wQueryOrSet;
	HPI_Message(&hm, &hr);
	return hr.wError;
}

u16 HPI_AdapterGetMode(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 *pdwAdapterMode
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ADAPTER,
		HPI_ADAPTER_GET_MODE);
	hm.wAdapterIndex = wAdapterIndex;
	HPI_Message(&hm, &hr);
	if (pdwAdapterMode)
		*pdwAdapterMode = hr.u.a.dwSerialNumber;
	return hr.wError;
}

u16 HPI_AdapterGetInfo(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 *pwNumOutStreams,
	u16 *pwNumInStreams,
	u16 *pwVersion,
	u32 *pdwSerialNumber,
	u16 *pwAdapterType
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ADAPTER,
		HPI_ADAPTER_GET_INFO);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	*pwAdapterType = hr.u.a.wAdapterType;
	*pwNumOutStreams = hr.u.a.wNumOStreams;
	*pwNumInStreams = hr.u.a.wNumIStreams;
	*pwVersion = hr.u.a.wVersion;
	*pdwSerialNumber = hr.u.a.dwSerialNumber;
	return hr.wError;
}

u16 HPI_AdapterGetModuleByIndex(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wModuleIndex,
	u16 *pwNumOutputs,
	u16 *pwNumInputs,
	u16 *pwVersion,
	u32 *pdwSerialNumber,
	u16 *pwModuleType,
	u32 *phModule
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ADAPTER,
		HPI_ADAPTER_MODULE_INFO);
	hm.wAdapterIndex = wAdapterIndex;
	hm.u.ax.module_info.index = wModuleIndex;

	HPI_Message(&hm, &hr);

	*pwModuleType = hr.u.a.wAdapterType;
	*pwNumOutputs = hr.u.a.wNumOStreams;
	*pwNumInputs = hr.u.a.wNumIStreams;
	*pwVersion = hr.u.a.wVersion;
	*pdwSerialNumber = hr.u.a.dwSerialNumber;
	*phModule = 0;

	return hr.wError;
}

u16 HPI_AdapterGetAssert(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 *wAssertPresent,
	char *pszAssert,
	u16 *pwLineNumber
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ADAPTER,
		HPI_ADAPTER_GET_ASSERT);
	hm.wAdapterIndex = wAdapterIndex;
	HPI_Message(&hm, &hr);

	*wAssertPresent = 0;

	if (!hr.wError) {

		*pwLineNumber = (u16)hr.u.a.dwSerialNumber;
		if (*pwLineNumber) {

			int i;
			char *Src = (char *)hr.u.a.szAdapterAssert;
			char *Dst = pszAssert;

			*wAssertPresent = 1;

			for (i = 0; i < HPI_STRING_LEN; i++) {
				char c;
				c = *Src++;
				*Dst++ = c;
				if (c == 0)
					break;
			}

		}
	}
	return hr.wError;
}

u16 HPI_AdapterGetAssertEx(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 *wAssertPresent,
	char *pszAssert,
	u32 *pdwLineNumber,
	u16 *pwAssertOnDsp
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ADAPTER,
		HPI_ADAPTER_GET_ASSERT);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	*wAssertPresent = 0;

	if (!hr.wError) {

		*pdwLineNumber = hr.u.a.dwSerialNumber;

		*wAssertPresent = hr.u.a.wAdapterType;

		*pwAssertOnDsp = hr.u.a.wAdapterIndex;

		if (!*wAssertPresent && *pdwLineNumber)

			*wAssertPresent = 1;

		if (*wAssertPresent) {

			int i;
			char *Src = (char *)hr.u.a.szAdapterAssert;
			char *Dst = pszAssert;

			for (i = 0; i < HPI_STRING_LEN; i++) {
				char c;
				c = *Src++;
				*Dst++ = c;
				if (c == 0)
					break;
			}

		} else {
			*pszAssert = 0;
		}
	}
	return hr.wError;
}

u16 HPI_AdapterTestAssert(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wAssertId
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ADAPTER,
		HPI_ADAPTER_TEST_ASSERT);
	hm.wAdapterIndex = wAdapterIndex;
	hm.u.a.wAssertId = wAssertId;

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_AdapterEnableCapability(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wCapability,
	u32 dwKey
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ADAPTER,
		HPI_ADAPTER_ENABLE_CAPABILITY);
	hm.wAdapterIndex = wAdapterIndex;
	hm.u.a.wAssertId = wCapability;
	hm.u.a.dwAdapterMode = dwKey;

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_AdapterSelfTest(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ADAPTER,
		HPI_ADAPTER_SELFTEST);
	hm.wAdapterIndex = wAdapterIndex;
	HPI_Message(&hm, &hr);
	return hr.wError;
}

u16 HPI_AdapterDebugRead(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 dwDspAddress,
	char *pBuffer,
	int *dwCountBytes
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ADAPTER,
		HPI_ADAPTER_DEBUG_READ);

	hr.wSize = sizeof(hr);

	hm.wAdapterIndex = wAdapterIndex;
	hm.u.ax.debug_read.dwDspAddress = dwDspAddress;

	if (*dwCountBytes > sizeof(hr.u.bytes))
		*dwCountBytes = sizeof(hr.u.bytes);

	hm.u.ax.debug_read.dwCountBytes = *dwCountBytes;

	HPI_Message(&hm, &hr);

	if (!hr.wError) {
		*dwCountBytes = hr.wSize - 12;
		memcpy(pBuffer, &hr.u.bytes, *dwCountBytes);
	} else
		*dwCountBytes = 0;
	return hr.wError;
}

u16 HPI_AdapterSetProperty(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wProperty,
	u16 wParameter1,
	u16 wParameter2
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ADAPTER,
		HPI_ADAPTER_SET_PROPERTY);
	hm.wAdapterIndex = wAdapterIndex;
	hm.u.ax.property_set.wProperty = wProperty;
	hm.u.ax.property_set.wParameter1 = wParameter1;
	hm.u.ax.property_set.wParameter2 = wParameter2;

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_AdapterGetProperty(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wProperty,
	u16 *pwParameter1,
	u16 *pwParameter2
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ADAPTER,
		HPI_ADAPTER_GET_PROPERTY);
	hm.wAdapterIndex = wAdapterIndex;
	hm.u.ax.property_set.wProperty = wProperty;

	HPI_Message(&hm, &hr);
	if (!hr.wError) {
		if (pwParameter1)
			*pwParameter1 = hr.u.ax.property_get.wParameter1;
		if (pwParameter2)
			*pwParameter2 = hr.u.ax.property_get.wParameter2;
	}

	return hr.wError;
}

u16 HPI_AdapterEnumerateProperty(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wIndex,
	u16 wWhatToEnumerate,
	u16 wPropertyIndex,
	u32 *pdwSetting
)
{
	return 0;
}

u16 HPI_FormatCreate(
	struct hpi_format *pFormat,
	u16 wChannels,
	u16 wFormat,
	u32 dwSampleRate,
	u32 dwBitRate,
	u32 dwAttributes
)
{
	u16 wError = 0;
	struct hpi_msg_format Format;

	if (wChannels < 1)
		wChannels = 1;
	if (wChannels > 8)
		wChannels = 8;

	Format.wChannels = wChannels;

	switch (wFormat) {
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
		return wError;
	}
	Format.wFormat = wFormat;

	if (dwSampleRate < 8000L) {
		wError = HPI_ERROR_INCOMPATIBLE_SAMPLERATE;
		dwSampleRate = 8000L;
	}
	if (dwSampleRate > 200000L) {
		wError = HPI_ERROR_INCOMPATIBLE_SAMPLERATE;
		dwSampleRate = 200000L;
	}
	Format.dwSampleRate = dwSampleRate;

	switch (wFormat) {
	case HPI_FORMAT_MPEG_L1:
	case HPI_FORMAT_MPEG_L2:
	case HPI_FORMAT_MPEG_L3:
		Format.dwBitRate = dwBitRate;
		break;
	case HPI_FORMAT_PCM16_SIGNED:
	case HPI_FORMAT_PCM16_BIGENDIAN:
		Format.dwBitRate = wChannels * dwSampleRate * 2;
		break;
	case HPI_FORMAT_PCM32_SIGNED:
	case HPI_FORMAT_PCM32_FLOAT:
		Format.dwBitRate = wChannels * dwSampleRate * 4;
		break;
	case HPI_FORMAT_PCM8_UNSIGNED:
		Format.dwBitRate = wChannels * dwSampleRate;
		break;
	default:
		Format.dwBitRate = 0;
	}

	switch (wFormat) {
	case HPI_FORMAT_MPEG_L2:
		if ((wChannels == 1)
			&& (dwAttributes != HPI_MPEG_MODE_DEFAULT)) {
			dwAttributes = HPI_MPEG_MODE_DEFAULT;
			wError = HPI_ERROR_INVALID_FORMAT;
		} else if (dwAttributes > HPI_MPEG_MODE_DUALCHANNEL) {
			dwAttributes = HPI_MPEG_MODE_DEFAULT;
			wError = HPI_ERROR_INVALID_FORMAT;
		}
		Format.dwAttributes = dwAttributes;
		break;
	default:
		Format.dwAttributes = dwAttributes;
	}

	HPI_MsgToFormat(pFormat, &Format);
	return wError;
}

u16 HPI_StreamEstimateBufferSize(
	struct hpi_format *pFormat,
	u32 dwHostPollingRateInMilliSeconds,
	u32 *dwRecommendedBufferSize
)
{

	u32 dwBytesPerSecond;
	u32 dwSize;
	u16 wChannels;
	struct hpi_format *pF = pFormat;

	wChannels = pF->wChannels;

	switch (pF->wFormat) {
	case HPI_FORMAT_PCM16_BIGENDIAN:
	case HPI_FORMAT_PCM16_SIGNED:
		dwBytesPerSecond = pF->dwSampleRate * 2L * wChannels;
		break;
	case HPI_FORMAT_PCM24_SIGNED:
		dwBytesPerSecond = pF->dwSampleRate * 3L * wChannels;
		break;
	case HPI_FORMAT_PCM32_SIGNED:
	case HPI_FORMAT_PCM32_FLOAT:
		dwBytesPerSecond = pF->dwSampleRate * 4L * wChannels;
		break;
	case HPI_FORMAT_PCM8_UNSIGNED:
		dwBytesPerSecond = pF->dwSampleRate * 1L * wChannels;
		break;
	case HPI_FORMAT_MPEG_L1:
	case HPI_FORMAT_MPEG_L2:
	case HPI_FORMAT_MPEG_L3:
		dwBytesPerSecond = pF->dwBitRate / 8L;
		break;
	case HPI_FORMAT_DOLBY_AC2:

		dwBytesPerSecond = 256000L / 8L;
		break;
	default:
		return HPI_ERROR_INVALID_FORMAT;
	}
	dwSize = (dwBytesPerSecond * dwHostPollingRateInMilliSeconds * 2) /
		1000L;

	*dwRecommendedBufferSize =
		roundup_pow_of_two(((dwSize + 4095L) & ~4095L));
	return 0;
}

u16 HPI_OutStreamOpen(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wOutStreamIndex,
	u32 *phOutStream
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM, HPI_OSTREAM_OPEN);
	hm.wAdapterIndex = wAdapterIndex;
	hm.wObjIndex = wOutStreamIndex;

	HPI_Message(&hm, &hr);

	if (hr.wError == 0)
		*phOutStream =
			HPI_IndexesToHandle(HPI_OBJ_OSTREAM, wAdapterIndex,
			wOutStreamIndex);
	else
		*phOutStream = 0;
	return hr.wError;
}

u16 HPI_OutStreamClose(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM,
		HPI_OSTREAM_HOSTBUFFER_FREE);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);
	HPI_Message(&hm, &hr);

	hm.wFunction = HPI_OSTREAM_GROUP_RESET;
	HPI_Message(&hm, &hr);

	hm.wFunction = HPI_OSTREAM_CLOSE;
	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_OutStreamGetInfoEx(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	u16 *pwState,
	u32 *pdwBufferSize,
	u32 *pdwDataToPlay,
	u32 *pdwSamplesPlayed,
	u32 *pdwAuxiliaryDataToPlay
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM,
		HPI_OSTREAM_GET_INFO);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);

	HPI_Message(&hm, &hr);

	if (pwState)
		*pwState = hr.u.d.u.stream_info.wState;
	if (pdwBufferSize)
		*pdwBufferSize = hr.u.d.u.stream_info.dwBufferSize;
	if (pdwDataToPlay)
		*pdwDataToPlay = hr.u.d.u.stream_info.dwDataAvailable;
	if (pdwSamplesPlayed)
		*pdwSamplesPlayed = hr.u.d.u.stream_info.dwSamplesTransferred;
	if (pdwAuxiliaryDataToPlay)
		*pdwAuxiliaryDataToPlay =
			hr.u.d.u.stream_info.dwAuxiliaryDataAvailable;
	return hr.wError;
}

u16 HPI_OutStreamWriteBuf(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	u8 *pbData,
	u32 dwBytesToWrite,
	struct hpi_format *pFormat
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM, HPI_OSTREAM_WRITE);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.d.u.Data.pbData = pbData;
	hm.u.d.u.Data.dwDataSize = dwBytesToWrite;

	HPI_FormatToMsg(&hm.u.d.u.Data.Format, pFormat);

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_OutStreamStart(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM, HPI_OSTREAM_START);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_OutStreamStop(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM, HPI_OSTREAM_STOP);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_OutStreamSinegen(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM,
		HPI_OSTREAM_SINEGEN);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_OutStreamReset(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM, HPI_OSTREAM_RESET);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_OutStreamQueryFormat(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	struct hpi_format *pFormat
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM,
		HPI_OSTREAM_QUERY_FORMAT);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);

	HPI_FormatToMsg(&hm.u.d.u.Data.Format, pFormat);

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_OutStreamSetFormat(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	struct hpi_format *pFormat
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM,
		HPI_OSTREAM_SET_FORMAT);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);

	HPI_FormatToMsg(&hm.u.d.u.Data.Format, pFormat);

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_OutStreamSetVelocity(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	short nVelocity
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM,
		HPI_OSTREAM_SET_VELOCITY);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.d.u.wVelocity = nVelocity;

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_OutStreamSetPunchInOut(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	u32 dwPunchInSample,
	u32 dwPunchOutSample
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM,
		HPI_OSTREAM_SET_PUNCHINOUT);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);

	hm.u.d.u.Pio.dwPunchInSample = dwPunchInSample;
	hm.u.d.u.Pio.dwPunchOutSample = dwPunchOutSample;

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_OutStreamAncillaryReset(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	u16 wMode
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM,
		HPI_OSTREAM_ANC_RESET);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.d.u.Data.Format.wChannels = wMode;
	HPI_Message(&hm, &hr);
	return hr.wError;
}

u16 HPI_OutStreamAncillaryGetInfo(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	u32 *pdwFramesAvailable
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM,
		HPI_OSTREAM_ANC_GET_INFO);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);
	HPI_Message(&hm, &hr);
	if (hr.wError == 0) {
		if (pdwFramesAvailable)
			*pdwFramesAvailable =
				hr.u.d.u.stream_info.dwDataAvailable /
				sizeof(struct hpi_anc_frame);
	}
	return hr.wError;
}

u16 HPI_OutStreamAncillaryRead(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	struct hpi_anc_frame *pAncFrameBuffer,
	u32 dwAncFrameBufferSizeInBytes,
	u32 dwNumberOfAncillaryFramesToRead
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM,
		HPI_OSTREAM_ANC_READ);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.d.u.Data.pbData = (u8 *)pAncFrameBuffer;
	hm.u.d.u.Data.dwDataSize =
		dwNumberOfAncillaryFramesToRead *
		sizeof(struct hpi_anc_frame);
	if (hm.u.d.u.Data.dwDataSize <= dwAncFrameBufferSizeInBytes)
		HPI_Message(&hm, &hr);
	else
		hr.wError = HPI_ERROR_INVALID_DATA_TRANSFER;
	return hr.wError;
}

u16 HPI_OutStreamSetTimeScale(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	u32 dwTimeScale
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM,
		HPI_OSTREAM_SET_TIMESCALE);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);

	hm.u.d.u.dwTimeScale = dwTimeScale;

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_OutStreamHostBufferAllocate(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	u32 dwSizeInBytes
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM,
		HPI_OSTREAM_HOSTBUFFER_ALLOC);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.d.u.Data.dwDataSize = dwSizeInBytes;
	HPI_Message(&hm, &hr);
	return hr.wError;
}

u16 HPI_OutStreamHostBufferGetInfo(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	u8 **ppBuffer,
	struct hpi_hostbuffer_status **ppStatus
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM,
		HPI_OSTREAM_HOSTBUFFER_GET_INFO);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);
	HPI_Message(&hm, &hr);

	if (hr.wError == 0) {
		if (ppBuffer)
			*ppBuffer = hr.u.d.u.hostbuffer_info.pBuffer;
		if (ppStatus)
			*ppStatus = hr.u.d.u.hostbuffer_info.pStatus;
	}
	return hr.wError;
}

u16 HPI_OutStreamHostBufferFree(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM,
		HPI_OSTREAM_HOSTBUFFER_FREE);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);
	HPI_Message(&hm, &hr);
	return hr.wError;
}

u16 HPI_OutStreamGroupAdd(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	u32 hStream
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	u16 wAdapter;
	char cObjType;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM,
		HPI_OSTREAM_GROUP_ADD);
	hr.wError = 0;
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);
	cObjType = HPI_HandleObject(hStream);
	switch (cObjType) {
	case HPI_OBJ_OSTREAM:
		hm.u.d.u.Stream.wObjectType = HPI_OBJ_OSTREAM;
		u32TOINDEXES(hStream, &wAdapter,
			&hm.u.d.u.Stream.wStreamIndex);
		break;
	case HPI_OBJ_ISTREAM:
		hm.u.d.u.Stream.wObjectType = HPI_OBJ_ISTREAM;
		u32TOINDEXES(hStream, &wAdapter,
			&hm.u.d.u.Stream.wStreamIndex);
		break;
	default:
		return HPI_ERROR_INVALID_STREAM;
	}
	if (wAdapter != hm.wAdapterIndex)
		return HPI_ERROR_NO_INTERADAPTER_GROUPS;

	HPI_Message(&hm, &hr);
	return hr.wError;
}

u16 HPI_OutStreamGroupGetMap(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream,
	u32 *pdwOutStreamMap,
	u32 *pdwInStreamMap
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM,
		HPI_OSTREAM_GROUP_GETMAP);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);
	HPI_Message(&hm, &hr);

	if (pdwOutStreamMap)
		*pdwOutStreamMap = hr.u.d.u.group_info.dwOutStreamGroupMap;
	if (pdwInStreamMap)
		*pdwInStreamMap = hr.u.d.u.group_info.dwInStreamGroupMap;

	return hr.wError;
}

u16 HPI_OutStreamGroupReset(
	struct hpi_hsubsys *phSubSys,
	u32 hOutStream
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_OSTREAM,
		HPI_OSTREAM_GROUP_RESET);
	u32TOINDEXES(hOutStream, &hm.wAdapterIndex, &hm.wObjIndex);
	HPI_Message(&hm, &hr);
	return hr.wError;
}

u16 HPI_InStreamOpen(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wInStreamIndex,
	u32 *phInStream
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ISTREAM, HPI_ISTREAM_OPEN);
	hm.wAdapterIndex = wAdapterIndex;
	hm.wObjIndex = wInStreamIndex;

	HPI_Message(&hm, &hr);

	if (hr.wError == 0)
		*phInStream =
			HPI_IndexesToHandle(HPI_OBJ_ISTREAM,
			wAdapterIndex, wInStreamIndex);
	else
		*phInStream = 0;

	return hr.wError;
}

u16 HPI_InStreamClose(
	struct hpi_hsubsys *phSubSys,
	u32 hInStream
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ISTREAM,
		HPI_ISTREAM_HOSTBUFFER_FREE);
	u32TOINDEXES(hInStream, &hm.wAdapterIndex, &hm.wObjIndex);
	HPI_Message(&hm, &hr);

	hm.wFunction = HPI_ISTREAM_GROUP_RESET;
	HPI_Message(&hm, &hr);

	hm.wFunction = HPI_ISTREAM_CLOSE;
	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_InStreamQueryFormat(
	struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	struct hpi_format *pFormat
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ISTREAM,
		HPI_ISTREAM_QUERY_FORMAT);
	u32TOINDEXES(hInStream, &hm.wAdapterIndex, &hm.wObjIndex);
	HPI_FormatToMsg(&hm.u.d.u.Data.Format, pFormat);

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_InStreamSetFormat(
	struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	struct hpi_format *pFormat
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ISTREAM,
		HPI_ISTREAM_SET_FORMAT);
	u32TOINDEXES(hInStream, &hm.wAdapterIndex, &hm.wObjIndex);
	HPI_FormatToMsg(&hm.u.d.u.Data.Format, pFormat);

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_InStreamReadBuf(
	struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	u8 *pbData,
	u32 dwBytesToRead
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ISTREAM, HPI_ISTREAM_READ);
	u32TOINDEXES(hInStream, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.d.u.Data.dwDataSize = dwBytesToRead;
	hm.u.d.u.Data.pbData = pbData;

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_InStreamStart(
	struct hpi_hsubsys *phSubSys,
	u32 hInStream
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ISTREAM, HPI_ISTREAM_START);
	u32TOINDEXES(hInStream, &hm.wAdapterIndex, &hm.wObjIndex);

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_InStreamStop(
	struct hpi_hsubsys *phSubSys,
	u32 hInStream
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ISTREAM, HPI_ISTREAM_STOP);
	u32TOINDEXES(hInStream, &hm.wAdapterIndex, &hm.wObjIndex);

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_InStreamReset(
	struct hpi_hsubsys *phSubSys,
	u32 hInStream
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ISTREAM, HPI_ISTREAM_RESET);
	u32TOINDEXES(hInStream, &hm.wAdapterIndex, &hm.wObjIndex);

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_InStreamGetInfoEx(
	struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	u16 *pwState,
	u32 *pdwBufferSize,
	u32 *pdwDataRecorded,
	u32 *pdwSamplesRecorded,
	u32 *pdwAuxiliaryDataRecorded
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ISTREAM,
		HPI_ISTREAM_GET_INFO);
	u32TOINDEXES(hInStream, &hm.wAdapterIndex, &hm.wObjIndex);

	HPI_Message(&hm, &hr);

	if (pwState)
		*pwState = hr.u.d.u.stream_info.wState;
	if (pdwBufferSize)
		*pdwBufferSize = hr.u.d.u.stream_info.dwBufferSize;
	if (pdwDataRecorded)
		*pdwDataRecorded = hr.u.d.u.stream_info.dwDataAvailable;
	if (pdwSamplesRecorded)
		*pdwSamplesRecorded =
			hr.u.d.u.stream_info.dwSamplesTransferred;
	if (pdwAuxiliaryDataRecorded)
		*pdwAuxiliaryDataRecorded =
			hr.u.d.u.stream_info.dwAuxiliaryDataAvailable;
	return hr.wError;
}

u16 HPI_InStreamAncillaryReset(
	struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	u16 wBytesPerFrame,
	u16 wMode,
	u16 wAlignment,
	u16 wIdleBit
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ISTREAM,
		HPI_ISTREAM_ANC_RESET);
	u32TOINDEXES(hInStream, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.d.u.Data.Format.dwAttributes = wBytesPerFrame;
	hm.u.d.u.Data.Format.wFormat = (wMode << 8) | (wAlignment & 0xff);
	hm.u.d.u.Data.Format.wChannels = wIdleBit;
	HPI_Message(&hm, &hr);
	return hr.wError;
}

u16 HPI_InStreamAncillaryGetInfo(
	struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	u32 *pdwFrameSpace
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ISTREAM,
		HPI_ISTREAM_ANC_GET_INFO);
	u32TOINDEXES(hInStream, &hm.wAdapterIndex, &hm.wObjIndex);
	HPI_Message(&hm, &hr);
	if (pdwFrameSpace)
		*pdwFrameSpace =
			(hr.u.d.u.stream_info.dwBufferSize -
			hr.u.d.u.stream_info.dwDataAvailable) /
			sizeof(struct hpi_anc_frame);
	return hr.wError;
}

u16 HPI_InStreamAncillaryWrite(
	struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	struct hpi_anc_frame *pAncFrameBuffer,
	u32 dwAncFrameBufferSizeInBytes,
	u32 dwNumberOfAncillaryFramesToWrite
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ISTREAM,
		HPI_ISTREAM_ANC_WRITE);
	u32TOINDEXES(hInStream, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.d.u.Data.pbData = (u8 *)pAncFrameBuffer;
	hm.u.d.u.Data.dwDataSize =
		dwNumberOfAncillaryFramesToWrite *
		sizeof(struct hpi_anc_frame);
	if (hm.u.d.u.Data.dwDataSize <= dwAncFrameBufferSizeInBytes)
		HPI_Message(&hm, &hr);
	else
		hr.wError = HPI_ERROR_INVALID_DATA_TRANSFER;
	return hr.wError;
}

u16 HPI_InStreamHostBufferAllocate(
	struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	u32 dwSizeInBytes
)
{

	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ISTREAM,
		HPI_ISTREAM_HOSTBUFFER_ALLOC);
	u32TOINDEXES(hInStream, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.d.u.Data.dwDataSize = dwSizeInBytes;
	HPI_Message(&hm, &hr);
	return hr.wError;
}

u16 HPI_InStreamHostBufferGetInfo(
	struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	u8 **ppBuffer,
	struct hpi_hostbuffer_status **ppStatus
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ISTREAM,
		HPI_ISTREAM_HOSTBUFFER_GET_INFO);
	u32TOINDEXES(hInStream, &hm.wAdapterIndex, &hm.wObjIndex);
	HPI_Message(&hm, &hr);

	if (hr.wError == 0) {
		if (ppBuffer)
			*ppBuffer = hr.u.d.u.hostbuffer_info.pBuffer;
		if (ppStatus)
			*ppStatus = hr.u.d.u.hostbuffer_info.pStatus;
	}
	return hr.wError;
}

u16 HPI_InStreamHostBufferFree(
	struct hpi_hsubsys *phSubSys,
	u32 hInStream
)
{

	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ISTREAM,
		HPI_ISTREAM_HOSTBUFFER_FREE);
	u32TOINDEXES(hInStream, &hm.wAdapterIndex, &hm.wObjIndex);
	HPI_Message(&hm, &hr);
	return hr.wError;
}

u16 HPI_InStreamGroupAdd(
	struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	u32 hStream
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	u16 wAdapter;
	char cObjType;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ISTREAM,
		HPI_ISTREAM_GROUP_ADD);
	hr.wError = 0;
	u32TOINDEXES(hInStream, &hm.wAdapterIndex, &hm.wObjIndex);
	cObjType = HPI_HandleObject(hStream);

	switch (cObjType) {
	case HPI_OBJ_OSTREAM:
		hm.u.d.u.Stream.wObjectType = HPI_OBJ_OSTREAM;
		u32TOINDEXES(hStream, &wAdapter,
			&hm.u.d.u.Stream.wStreamIndex);
		break;
	case HPI_OBJ_ISTREAM:
		hm.u.d.u.Stream.wObjectType = HPI_OBJ_ISTREAM;
		u32TOINDEXES(hStream, &wAdapter,
			&hm.u.d.u.Stream.wStreamIndex);
		break;
	default:
		return HPI_ERROR_INVALID_STREAM;
	}

	if (wAdapter != hm.wAdapterIndex)
		return HPI_ERROR_NO_INTERADAPTER_GROUPS;

	HPI_Message(&hm, &hr);
	return hr.wError;
}

u16 HPI_InStreamGroupGetMap(
	struct hpi_hsubsys *phSubSys,
	u32 hInStream,
	u32 *pdwOutStreamMap,
	u32 *pdwInStreamMap
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ISTREAM,
		HPI_ISTREAM_HOSTBUFFER_FREE);
	u32TOINDEXES(hInStream, &hm.wAdapterIndex, &hm.wObjIndex);
	HPI_Message(&hm, &hr);

	if (pdwOutStreamMap)
		*pdwOutStreamMap = hr.u.d.u.group_info.dwOutStreamGroupMap;
	if (pdwInStreamMap)
		*pdwInStreamMap = hr.u.d.u.group_info.dwInStreamGroupMap;

	return hr.wError;
}

u16 HPI_InStreamGroupReset(
	struct hpi_hsubsys *phSubSys,
	u32 hInStream
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ISTREAM,
		HPI_ISTREAM_GROUP_RESET);
	u32TOINDEXES(hInStream, &hm.wAdapterIndex, &hm.wObjIndex);
	HPI_Message(&hm, &hr);
	return hr.wError;
}

u16 HPI_MixerOpen(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 *phMixer
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_MIXER, HPI_MIXER_OPEN);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	if (hr.wError == 0)
		*phMixer =
			HPI_IndexesToHandle(HPI_OBJ_MIXER, wAdapterIndex, 0);
	else
		*phMixer = 0;
	return hr.wError;
}

u16 HPI_MixerClose(
	struct hpi_hsubsys *phSubSys,
	u32 hMixer
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_MIXER, HPI_MIXER_CLOSE);
	u32TOINDEX(hMixer, &hm.wAdapterIndex);
	HPI_Message(&hm, &hr);
	return hr.wError;
}

u16 HPI_MixerGetControl(
	struct hpi_hsubsys *phSubSys,
	u32 hMixer,
	u16 wSrcNodeType,
	u16 wSrcNodeTypeIndex,
	u16 wDstNodeType,
	u16 wDstNodeTypeIndex,
	u16 wControlType,
	u32 *phControl
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_MIXER,
		HPI_MIXER_GET_CONTROL);
	u32TOINDEX(hMixer, &hm.wAdapterIndex);
	hm.u.m.wNodeType1 = wSrcNodeType;
	hm.u.m.wNodeIndex1 = wSrcNodeTypeIndex;
	hm.u.m.wNodeType2 = wDstNodeType;
	hm.u.m.wNodeIndex2 = wDstNodeTypeIndex;
	hm.u.m.wControlType = wControlType;

	HPI_Message(&hm, &hr);

	if (hr.wError == 0)
		*phControl =
			HPI_IndexesToHandle(HPI_OBJ_CONTROL, hm.wAdapterIndex,
			hr.u.m.wControlIndex);
	else
		*phControl = 0;
	return hr.wError;
}

u16 HPI_MixerGetControlByIndex(
	struct hpi_hsubsys *phSubSys,
	u32 hMixer,
	u16 wControlIndex,
	u16 *pwSrcNodeType,
	u16 *pwSrcNodeIndex,
	u16 *pwDstNodeType,
	u16 *pwDstNodeIndex,
	u16 *pwControlType,
	u32 *phControl
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_MIXER,
		HPI_MIXER_GET_CONTROL_BY_INDEX);
	u32TOINDEX(hMixer, &hm.wAdapterIndex);
	hm.u.m.wControlIndex = wControlIndex;
	HPI_Message(&hm, &hr);

	if (pwSrcNodeType) {
		*pwSrcNodeType = hr.u.m.wSrcNodeType + HPI_SOURCENODE_NONE;
		*pwSrcNodeIndex = hr.u.m.wSrcNodeIndex;
		*pwDstNodeType = hr.u.m.wDstNodeType + HPI_DESTNODE_NONE;
		*pwDstNodeIndex = hr.u.m.wDstNodeIndex;
	}
	if (pwControlType)
		*pwControlType = hr.u.m.wControlIndex;

	if (phControl) {
		if (hr.wError == 0)
			*phControl =
				HPI_IndexesToHandle(HPI_OBJ_CONTROL,
				hm.wAdapterIndex, wControlIndex);
		else
			*phControl = 0;
	}
	return hr.wError;
}

u16 HPI_MixerStore(
	struct hpi_hsubsys *phSubSys,
	u32 hMixer,
	enum HPI_MIXER_STORE_COMMAND command,
	u16 wIndex
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_MIXER, HPI_MIXER_STORE);
	u32TOINDEX(hMixer, &hm.wAdapterIndex);
	hm.u.mx.store.wCommand = command;
	hm.u.mx.store.wIndex = wIndex;
	HPI_Message(&hm, &hr);
	return hr.wError;
}

static
u16 HPI_ControlParamSet(
	const struct hpi_hsubsys *phSubSys,
	const u32 hControl,
	const u16 wAttrib,
	const u32 dwParam1,
	const u32 dwParam2
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_SET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = wAttrib;
	hm.u.c.dwParam1 = dwParam1;
	hm.u.c.dwParam2 = dwParam2;
	HPI_Message(&hm, &hr);
	return hr.wError;
}

static
u16 HPI_ControlParamGet(
	const struct hpi_hsubsys *phSubSys,
	const u32 hControl,
	const u16 wAttrib,
	u32 dwParam1,
	u32 dwParam2,
	u32 *pdwParam1,
	u32 *pdwParam2
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = wAttrib;
	hm.u.c.dwParam1 = dwParam1;
	hm.u.c.dwParam2 = dwParam2;
	HPI_Message(&hm, &hr);
	if (pdwParam1)
		*pdwParam1 = hr.u.c.dwParam1;
	if (pdwParam2)
		*pdwParam2 = hr.u.c.dwParam2;

	return hr.wError;
}

#define HPI_ControlParam1Get(s, h, a, p1) \
		HPI_ControlParamGet(s, h, a, 0, 0, p1, NULL)
#define HPI_ControlParam2Get(s, h, a, p1, p2) \
		HPI_ControlParamGet(s, h, a, 0, 0, p1, p2)
#define HPI_ControlExParam1Get(s, h, a, p1) \
		HPI_ControlExParamGet(s, h, a, 0, 0, p1, NULL)
#define HPI_ControlExParam2Get(s, h, a, p1, p2) \
		HPI_ControlExParamGet(s, h, a, 0, 0, p1, p2)

static
u16 HPI_ControlQuery(
	const struct hpi_hsubsys *phSubSys,
	const u32 hControl,
	const u16 wAttrib,
	const u32 dwIndex,
	const u32 dwParam,
	u32 *pdwSetting
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_INFO);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);

	hm.u.c.wAttribute = wAttrib;
	hm.u.c.dwParam1 = dwIndex;
	hm.u.c.dwParam2 = dwParam;

	HPI_Message(&hm, &hr);
	*pdwSetting = hr.u.c.dwParam1;

	return hr.wError;
}

static u16 HPI_Control_GetString(
	const struct hpi_hsubsys *phSubSys,
	const u32 hControl,
	const u16 wAttribute,
	char *pszString,
	const u32 wStringLength
)
{
	unsigned int subStringIndex = 0, j = 0;
	char c = 0;
	unsigned int n = 0;
	u16 wHE = 0;

	if ((wStringLength < 1) || (wStringLength > 256))
		return HPI_ERROR_INVALID_CONTROL_VALUE;
	for (subStringIndex = 0; subStringIndex < wStringLength;
		subStringIndex += 8) {
		struct hpi_message hm;
		struct hpi_response hr;

		HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
			HPI_CONTROL_GET_STATE);
		u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
		hm.u.c.wAttribute = wAttribute;
		hm.u.c.dwParam1 = subStringIndex;
		hm.u.c.dwParam2 = 0;
		HPI_Message(&hm, &hr);

		if (subStringIndex == 0
			&& (hr.u.cu.chars8.dwRemainingChars + 8) >
			wStringLength)
			return HPI_ERROR_INVALID_CONTROL_VALUE;

		if (hr.wError) {
			wHE = hr.wError;
			break;
		}
		for (j = 0; j < 8; j++) {
			c = hr.u.cu.chars8.szData[j];
			pszString[subStringIndex + j] = c;
			n++;
			if (n >= wStringLength) {
				pszString[wStringLength - 1] = 0;
				wHE = HPI_ERROR_INVALID_CONTROL_VALUE;
				break;
			}
			if (c == 0)
				break;
		}

		if ((hr.u.cu.chars8.dwRemainingChars == 0) &&
			((subStringIndex + j) < wStringLength) && (c != 0)) {
			c = 0;
			pszString[subStringIndex + j] = c;
		}
		if (c == 0)
			break;
	}
	return wHE;
}

u16 HPI_AESEBU_Receiver_QueryFormat(
	const struct hpi_hsubsys *phSubSys,
	const u32 hAesRx,
	const u32 dwIndex,
	u16 *pwFormat
)
{
	u32 qr;
	u16 err;

	err = HPI_ControlQuery(phSubSys, hAesRx, HPI_AESEBURX_FORMAT,
		dwIndex, 0, &qr);
	*pwFormat = (u16)qr;
	return err;
}

u16 HPI_AESEBU_Receiver_SetFormat(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wFormat
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_AESEBURX_FORMAT, wFormat, 0);
}

u16 HPI_AESEBU_Receiver_GetFormat(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwFormat
)
{
	u16 wErr;
	u32 dwParam;

	wErr = HPI_ControlParam1Get(phSubSys, hControl,
		HPI_AESEBURX_FORMAT, &dwParam);
	if (!wErr && pwFormat)
		*pwFormat = (u16)dwParam;

	return wErr;
}

u16 HPI_AESEBU_Receiver_GetSampleRate(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwSampleRate
)
{
	return HPI_ControlParam1Get(phSubSys, hControl,
		HPI_AESEBURX_SAMPLERATE, pdwSampleRate);
}

u16 HPI_AESEBU_Receiver_GetUserData(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wIndex,
	u16 *pwData
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = HPI_AESEBURX_USERDATA;
	hm.u.c.dwParam1 = wIndex;

	HPI_Message(&hm, &hr);

	if (pwData)
		*pwData = (u16)hr.u.c.dwParam2;
	return hr.wError;
}

u16 HPI_AESEBU_Receiver_GetChannelStatus(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wIndex,
	u16 *pwData
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = HPI_AESEBURX_CHANNELSTATUS;
	hm.u.c.dwParam1 = wIndex;

	HPI_Message(&hm, &hr);

	if (pwData)
		*pwData = (u16)hr.u.c.dwParam2;
	return hr.wError;
}

u16 HPI_AESEBU_Receiver_GetErrorStatus(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwErrorData
)
{
	u32 dwErrorData = 0;
	u16 wError = 0;

	wError = HPI_ControlParam1Get(phSubSys, hControl,
		HPI_AESEBURX_ERRORSTATUS, &dwErrorData);
	if (pwErrorData)
		*pwErrorData = (u16)dwErrorData;
	return wError;
}

u16 HPI_AESEBU_Transmitter_SetSampleRate(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwSampleRate
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_AESEBUTX_SAMPLERATE, dwSampleRate, 0);
}

u16 HPI_AESEBU_Transmitter_SetUserData(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wIndex,
	u16 wData
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_AESEBUTX_USERDATA, wIndex, wData);
}

u16 HPI_AESEBU_Transmitter_SetChannelStatus(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wIndex,
	u16 wData
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_AESEBUTX_CHANNELSTATUS, wIndex, wData);
}

u16 HPI_AESEBU_Transmitter_GetChannelStatus(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wIndex,
	u16 *pwData
)
{
	return HPI_ERROR_INVALID_OPERATION;
}

u16 HPI_AESEBU_Transmitter_QueryFormat(
	const struct hpi_hsubsys *phSubSys,
	const u32 hAesTx,
	const u32 dwIndex,
	u16 *pwFormat
)
{
	u32 qr;
	u16 err;

	err = HPI_ControlQuery(phSubSys, hAesTx, HPI_AESEBUTX_FORMAT,
		dwIndex, 0, &qr);
	*pwFormat = (u16)qr;
	return err;
}

u16 HPI_AESEBU_Transmitter_SetFormat(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wOutputFormat
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_AESEBUTX_FORMAT, wOutputFormat, 0);
}

u16 HPI_AESEBU_Transmitter_GetFormat(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwOutputFormat
)
{
	u16 wErr;
	u32 dwParam;

	wErr = HPI_ControlParam1Get(phSubSys, hControl,
		HPI_AESEBUTX_FORMAT, &dwParam);
	if (!wErr && pwOutputFormat)
		*pwOutputFormat = (u16)dwParam;

	return wErr;
}

u16 HPI_Bitstream_SetClockEdge(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wEdgeType
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_BITSTREAM_CLOCK_EDGE, wEdgeType, 0);
}

u16 HPI_Bitstream_SetDataPolarity(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wPolarity
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_BITSTREAM_DATA_POLARITY, wPolarity, 0);
}

u16 HPI_Bitstream_GetActivity(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwClkActivity,
	u16 *pwDataActivity
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = HPI_BITSTREAM_ACTIVITY;
	HPI_Message(&hm, &hr);
	if (pwClkActivity)
		*pwClkActivity = (u16)hr.u.c.dwParam1;
	if (pwDataActivity)
		*pwDataActivity = (u16)hr.u.c.dwParam2;
	return hr.wError;
}

u16 HPI_ChannelMode_QueryMode(
	const struct hpi_hsubsys *phSubSys,
	const u32 hMode,
	const u32 dwIndex,
	u16 *pwMode
)
{
	u32 qr;
	u16 err;

	err = HPI_ControlQuery(phSubSys, hMode, HPI_CHANNEL_MODE_MODE,
		dwIndex, 0, &qr);
	*pwMode = (u16)qr;
	return err;
}

u16 HPI_ChannelModeSet(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wMode
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_CHANNEL_MODE_MODE, wMode, 0);
}

u16 HPI_ChannelModeGet(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *wMode
)
{
	u32 dwMode = 0;
	u16 wError = HPI_ControlParam1Get(phSubSys, hControl,
		HPI_CHANNEL_MODE_MODE, &dwMode);
	if (wMode)
		*wMode = (u16)dwMode;
	return wError;
}

u16 HPI_Cobranet_HmiWrite(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwHmiAddress,
	u32 dwByteCount,
	u8 *pbData
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROLEX,
		HPI_CONTROL_SET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);

	hm.u.cx.u.cobranet_data.dwByteCount = dwByteCount;
	hm.u.cx.u.cobranet_data.dwHmiAddress = dwHmiAddress;

	if (dwByteCount <= 8) {
		memcpy(hm.u.cx.u.cobranet_data.dwData, pbData, dwByteCount);
		hm.u.cx.wAttribute = HPI_COBRANET_SET;
	} else {
		hm.u.cx.u.cobranet_bigdata.pbData = pbData;
		hm.u.cx.wAttribute = HPI_COBRANET_SET_DATA;
	}

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_Cobranet_HmiRead(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwHmiAddress,
	u32 dwMaxByteCount,
	u32 *pdwByteCount,
	u8 *pbData
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROLEX,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);

	hm.u.cx.u.cobranet_data.dwByteCount = dwMaxByteCount;
	hm.u.cx.u.cobranet_data.dwHmiAddress = dwHmiAddress;

	if (dwMaxByteCount <= 8) {
		hm.u.cx.wAttribute = HPI_COBRANET_GET;
	} else {
		hm.u.cx.u.cobranet_bigdata.pbData = pbData;
		hm.u.cx.wAttribute = HPI_COBRANET_GET_DATA;
	}

	HPI_Message(&hm, &hr);
	if (!hr.wError && pbData) {

		*pdwByteCount = hr.u.cx.u.cobranet_data.dwByteCount;

		if (*pdwByteCount < dwMaxByteCount)
			dwMaxByteCount = *pdwByteCount;

		if (hm.u.cx.wAttribute == HPI_COBRANET_GET) {
			memcpy(pbData, hr.u.cx.u.cobranet_data.dwData,
				dwMaxByteCount);
		} else {

		}

	}
	return hr.wError;
}

u16 HPI_Cobranet_HmiGetStatus(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwStatus,
	u32 *pdwReadableSize,
	u32 *pdwWriteableSize
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROLEX,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);

	hm.u.cx.wAttribute = HPI_COBRANET_GET_STATUS;

	HPI_Message(&hm, &hr);
	if (!hr.wError) {
		if (pdwStatus)
			*pdwStatus = hr.u.cx.u.cobranet_status.dwStatus;
		if (pdwReadableSize)
			*pdwReadableSize =
				hr.u.cx.u.cobranet_status.dwReadableSize;
		if (pdwWriteableSize)
			*pdwWriteableSize =
				hr.u.cx.u.cobranet_status.dwWriteableSize;
	}
	return hr.wError;
}

u16 HPI_Cobranet_GetIPaddress(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwIPaddress
)
{
	u32 dwByteCount;
	u32 dwIP;
	u16 wError;
	wError = HPI_Cobranet_HmiRead(phSubSys, hControl,
		HPI_COBRANET_HMI_cobraIpMonCurrentIP,
		4, &dwByteCount, (u8 *)&dwIP);

	*pdwIPaddress =
		((dwIP & 0xff000000) >> 8) |
		((dwIP & 0x00ff0000) << 8) |
		((dwIP & 0x0000ff00) >> 8) | ((dwIP & 0x000000ff) << 8);

	if (wError)
		*pdwIPaddress = 0;

	return wError;

}

u16 HPI_Cobranet_SetIPaddress(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwIPaddress
)
{
	u32 dwIP;
	u16 wError;

	dwIP = ((dwIPaddress & 0xff000000) >> 8) |
		((dwIPaddress & 0x00ff0000) << 8) |
		((dwIPaddress & 0x0000ff00) >> 8) |
		((dwIPaddress & 0x000000ff) << 8);

	wError = HPI_Cobranet_HmiWrite(phSubSys, hControl,
		HPI_COBRANET_HMI_cobraIpMonCurrentIP, 4, (u8 *)&dwIP);

	return wError;

}

u16 HPI_Cobranet_GetStaticIPaddress(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwIPaddress
)
{
	u32 dwByteCount;
	u32 dwIP;
	u16 wError;
	wError = HPI_Cobranet_HmiRead(phSubSys, hControl,
		HPI_COBRANET_HMI_cobraIpMonStaticIP,
		4, &dwByteCount, (u8 *)&dwIP);

	*pdwIPaddress =
		((dwIP & 0xff000000) >> 8) |
		((dwIP & 0x00ff0000) << 8) |
		((dwIP & 0x0000ff00) >> 8) | ((dwIP & 0x000000ff) << 8);

	if (wError)
		*pdwIPaddress = 0;

	return wError;

}

u16 HPI_Cobranet_SetStaticIPaddress(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwIPaddress
)
{
	u32 dwIP;
	u16 wError;

	dwIP = ((dwIPaddress & 0xff000000) >> 8) |
		((dwIPaddress & 0x00ff0000) << 8) |
		((dwIPaddress & 0x0000ff00) >> 8) |
		((dwIPaddress & 0x000000ff) << 8);

	wError = HPI_Cobranet_HmiWrite(phSubSys, hControl,
		HPI_COBRANET_HMI_cobraIpMonStaticIP, 4, (u8 *)&dwIP);

	return wError;

}

u16 HPI_Cobranet_GetMACaddress(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwMAC_MSBs,
	u32 *pdwMAC_LSBs
)
{
	u32 dwByteCount;
	u16 wError;
	u32 dwMAC;
	wError = HPI_Cobranet_HmiRead(phSubSys, hControl,
		HPI_COBRANET_HMI_cobraIfPhyAddress,
		4, &dwByteCount, (u8 *)&dwMAC);
	*pdwMAC_MSBs =
		((dwMAC & 0xff000000) >> 8) |
		((dwMAC & 0x00ff0000) << 8) |
		((dwMAC & 0x0000ff00) >> 8) | ((dwMAC & 0x000000ff) << 8);
	wError += HPI_Cobranet_HmiRead(phSubSys, hControl,
		HPI_COBRANET_HMI_cobraIfPhyAddress + 1,
		4, &dwByteCount, (u8 *)&dwMAC);
	*pdwMAC_LSBs =
		((dwMAC & 0xff000000) >> 8) |
		((dwMAC & 0x00ff0000) << 8) |
		((dwMAC & 0x0000ff00) >> 8) | ((dwMAC & 0x000000ff) << 8);

	if (wError) {
		*pdwMAC_MSBs = 0;
		*pdwMAC_LSBs = 0;
	}

	return wError;
}

u16 HPI_Compander_Set(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wAttack,
	u16 wDecay,
	short wRatio100,
	short nThreshold0_01dB,
	short nMakeupGain0_01dB
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_SET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);

	hm.u.c.dwParam1 = wAttack + ((u32)wRatio100 << 16);
	hm.u.c.dwParam2 = (wDecay & 0xFFFFL);
	hm.u.c.anLogValue[0] = nThreshold0_01dB;
	hm.u.c.anLogValue[1] = nMakeupGain0_01dB;
	hm.u.c.wAttribute = HPI_COMPANDER_PARAMS;

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_Compander_Get(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwAttack,
	u16 *pwDecay,
	short *pwRatio100,
	short *pnThreshold0_01dB,
	short *pnMakeupGain0_01dB
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = HPI_COMPANDER_PARAMS;

	HPI_Message(&hm, &hr);

	if (pwAttack)
		*pwAttack = (short)(hr.u.c.dwParam1 & 0xFFFF);
	if (pwDecay)
		*pwDecay = (short)(hr.u.c.dwParam2 & 0xFFFF);
	if (pwRatio100)
		*pwRatio100 = (short)(hr.u.c.dwParam1 >> 16);

	if (pnThreshold0_01dB)
		*pnThreshold0_01dB = hr.u.c.anLogValue[0];
	if (pnMakeupGain0_01dB)
		*pnMakeupGain0_01dB = hr.u.c.anLogValue[1];

	return hr.wError;
}

u16 HPI_LevelQueryRange(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short *nMinGain_01dB,
	short *nMaxGain_01dB,
	short *nStepGain_01dB
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = HPI_LEVEL_RANGE;

	HPI_Message(&hm, &hr);
	if (hr.wError) {
		hr.u.c.anLogValue[0] = 0;
		hr.u.c.anLogValue[1] = 0;
		hr.u.c.dwParam1 = 0;
	}
	if (nMinGain_01dB)
		*nMinGain_01dB = hr.u.c.anLogValue[0];
	if (nMaxGain_01dB)
		*nMaxGain_01dB = hr.u.c.anLogValue[1];
	if (nStepGain_01dB)
		*nStepGain_01dB = (short)hr.u.c.dwParam1;
	return hr.wError;
}

u16 HPI_LevelSetGain(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short anGain0_01dB[HPI_MAX_CHANNELS]
)
{
	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_SET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	memcpy(hm.u.c.anLogValue, anGain0_01dB,
		sizeof(short) * HPI_MAX_CHANNELS);
	hm.u.c.wAttribute = HPI_LEVEL_GAIN;

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_LevelGetGain(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short anGain0_01dB[HPI_MAX_CHANNELS]
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = HPI_LEVEL_GAIN;

	HPI_Message(&hm, &hr);

	memcpy(anGain0_01dB, hr.u.c.anLogValue,
		sizeof(short) * HPI_MAX_CHANNELS);
	return hr.wError;
}

u16 HPI_Meter_QueryChannels(
	const struct hpi_hsubsys *phSubSys,
	const u32 hMeter,
	u32 *pChannels
)
{
	return HPI_ControlQuery(phSubSys, hMeter, HPI_METER_NUM_CHANNELS,
		0, 0, pChannels);
}

u16 HPI_MeterGetPeak(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short anPeakdB[HPI_MAX_CHANNELS]
)
{
	short i = 0;

	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.wObjIndex = hm.wObjIndex;
	hm.u.c.wAttribute = HPI_METER_PEAK;

	HPI_Message(&hm, &hr);

	if (!hr.wError)
		memcpy(anPeakdB, hr.u.c.anLogValue,
			sizeof(short) * HPI_MAX_CHANNELS);
	else
		for (i = 0; i < HPI_MAX_CHANNELS; i++)
			anPeakdB[i] = HPI_METER_MINIMUM;
	return hr.wError;
}

u16 HPI_MeterGetRms(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short anRmsdB[HPI_MAX_CHANNELS]
)
{
	short i = 0;

	struct hpi_message hm;
	struct hpi_response hr;

	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = HPI_METER_RMS;

	HPI_Message(&hm, &hr);

	if (!hr.wError)
		memcpy(anRmsdB, hr.u.c.anLogValue,
			sizeof(short) * HPI_MAX_CHANNELS);
	else
		for (i = 0; i < HPI_MAX_CHANNELS; i++)
			anRmsdB[i] = HPI_METER_MINIMUM;

	return hr.wError;
}

u16 HPI_MeterSetRmsBallistics(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 nAttack,
	u16 nDecay
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_METER_RMS_BALLISTICS, nAttack, nDecay);
}

u16 HPI_MeterGetRmsBallistics(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pnAttack,
	u16 *pnDecay
)
{
	u32 dwAttack;
	u32 dwDecay;
	u16 nError;

	nError = HPI_ControlParam2Get(phSubSys, hControl,
		HPI_METER_RMS_BALLISTICS, &dwAttack, &dwDecay);

	if (pnAttack)
		*pnAttack = (unsigned short)dwAttack;
	if (pnDecay)
		*pnDecay = (unsigned short)dwDecay;

	return nError;
}

u16 HPI_MeterSetPeakBallistics(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 nAttack,
	u16 nDecay
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_METER_PEAK_BALLISTICS, nAttack, nDecay);
}

u16 HPI_MeterGetPeakBallistics(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pnAttack,
	u16 *pnDecay
)
{
	u32 dwAttack;
	u32 dwDecay;
	u16 nError;

	nError = HPI_ControlParam2Get(phSubSys, hControl,
		HPI_METER_PEAK_BALLISTICS, &dwAttack, &dwDecay);

	if (pnAttack)
		*pnAttack = (short)dwAttack;
	if (pnDecay)
		*pnDecay = (short)dwDecay;

	return nError;
}

u16 HPI_Microphone_SetPhantomPower(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wOnOff
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_MICROPHONE_PHANTOM_POWER, (u32)wOnOff, 0);
}

u16 HPI_Microphone_GetPhantomPower(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwOnOff
)
{
	u16 nError = 0;
	u32 dwOnOff = 0;
	nError = HPI_ControlParam1Get(phSubSys, hControl,
		HPI_MICROPHONE_PHANTOM_POWER, &dwOnOff);
	if (pwOnOff)
		*pwOnOff = (u16)dwOnOff;
	return (nError);
}

u16 HPI_Multiplexer_SetSource(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wSourceNodeType,
	u16 wSourceNodeIndex
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_MULTIPLEXER_SOURCE, wSourceNodeType, wSourceNodeIndex);
}

u16 HPI_Multiplexer_GetSource(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *wSourceNodeType,
	u16 *wSourceNodeIndex
)
{
	u32 dwNode, dwIndex;
	u16 wError = HPI_ControlParam2Get(phSubSys, hControl,
		HPI_MULTIPLEXER_SOURCE, &dwNode,
		&dwIndex);
	if (wSourceNodeType)
		*wSourceNodeType = (u16)dwNode;
	if (wSourceNodeIndex)
		*wSourceNodeIndex = (u16)dwIndex;
	return wError;
}

u16 HPI_Multiplexer_QuerySource(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wIndex,
	u16 *wSourceNodeType,
	u16 *wSourceNodeIndex
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = HPI_MULTIPLEXER_QUERYSOURCE;
	hm.u.c.dwParam1 = wIndex;

	HPI_Message(&hm, &hr);

	if (wSourceNodeType)
		*wSourceNodeType = (u16)hr.u.c.dwParam1;
	if (wSourceNodeIndex)
		*wSourceNodeIndex = (u16)hr.u.c.dwParam2;
	return hr.wError;
}

u16 HPI_ParametricEQ_GetInfo(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwNumberOfBands,
	u16 *pwOnOff
)
{
	u32 dwNOB = 0;
	u32 dwOO = 0;
	u16 nError = 0;

	nError = HPI_ControlParam2Get(phSubSys, hControl,
		HPI_EQUALIZER_NUM_FILTERS, &dwOO, &dwNOB);
	if (pwNumberOfBands)
		*pwNumberOfBands = (u16)dwNOB;
	if (pwOnOff)
		*pwOnOff = (u16)dwOO;
	return nError;
}

u16 HPI_ParametricEQ_SetState(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wOnOff
)
{
	return HPI_ControlParamSet(phSubSys,
		hControl, HPI_EQUALIZER_NUM_FILTERS, wOnOff, 0);
}

u16 HPI_ParametricEQ_GetBand(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wIndex,
	u16 *pnType,
	u32 *pdwFrequencyHz,
	short *pnQ100,
	short *pnGain0_01dB
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = HPI_EQUALIZER_FILTER;
	hm.u.c.dwParam2 = wIndex;

	HPI_Message(&hm, &hr);

	if (pdwFrequencyHz)
		*pdwFrequencyHz = hr.u.c.dwParam1;
	if (pnType)
		*pnType = (u16)(hr.u.c.dwParam2 >> 16);
	if (pnQ100)
		*pnQ100 = hr.u.c.anLogValue[1];
	if (pnGain0_01dB)
		*pnGain0_01dB = hr.u.c.anLogValue[0];

	return hr.wError;
}

u16 HPI_ParametricEQ_SetBand(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wIndex,
	u16 nType,
	u32 dwFrequencyHz,
	short nQ100,
	short nGain0_01dB
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_SET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);

	hm.u.c.dwParam1 = dwFrequencyHz;
	hm.u.c.dwParam2 = (wIndex & 0xFFFFL) + ((u32)nType << 16);
	hm.u.c.anLogValue[0] = nGain0_01dB;
	hm.u.c.anLogValue[1] = nQ100;
	hm.u.c.wAttribute = HPI_EQUALIZER_FILTER;

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_ParametricEQ_GetCoeffs(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wIndex,
	short coeffs[5]
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = HPI_EQUALIZER_COEFFICIENTS;
	hm.u.c.dwParam2 = wIndex;

	HPI_Message(&hm, &hr);

	coeffs[0] = (short)hr.u.c.anLogValue[0];
	coeffs[1] = (short)hr.u.c.anLogValue[1];
	coeffs[2] = (short)hr.u.c.dwParam1;
	coeffs[3] = (short)(hr.u.c.dwParam1 >> 16);
	coeffs[4] = (short)hr.u.c.dwParam2;

	return hr.wError;
}

u16 HPI_SampleClock_QuerySource(
	const struct hpi_hsubsys *phSubSys,
	const u32 hClock,
	const u32 dwIndex,
	u16 *pwSource
)
{
	u32 qr;
	u16 err;

	err = HPI_ControlQuery(phSubSys, hClock, HPI_SAMPLECLOCK_SOURCE,
		dwIndex, 0, &qr);
	*pwSource = (u16)qr;
	return err;
}

u16 HPI_SampleClock_SetSource(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wSource
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_SAMPLECLOCK_SOURCE, wSource, 0);
}

u16 HPI_SampleClock_GetSource(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwSource
)
{
	u16 wError = 0;
	u32 dwSource = 0;
	wError = HPI_ControlParam1Get(phSubSys, hControl,
		HPI_SAMPLECLOCK_SOURCE, &dwSource);
	if (!wError)
		if (pwSource)
			*pwSource = (u16)dwSource;
	return wError;
}

u16 HPI_SampleClock_QuerySourceIndex(
	const struct hpi_hsubsys *phSubSys,
	const u32 hClock,
	const u32 dwIndex,
	const u32 dwSource,
	u16 *pwSourceIndex
)
{
	u32 qr;
	u16 err;

	err = HPI_ControlQuery(phSubSys, hClock, HPI_SAMPLECLOCK_SOURCE_INDEX,
		dwIndex, dwSource, &qr);
	*pwSourceIndex = (u16)qr;
	return err;
}

u16 HPI_SampleClock_SetSourceIndex(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wSourceIndex
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_SAMPLECLOCK_SOURCE_INDEX, wSourceIndex, 0);
}

u16 HPI_SampleClock_GetSourceIndex(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwSourceIndex
)
{
	u16 wError = 0;
	u32 dwSourceIndex = 0;
	wError = HPI_ControlParam1Get(phSubSys, hControl,
		HPI_SAMPLECLOCK_SOURCE_INDEX, &dwSourceIndex);
	if (!wError)
		if (pwSourceIndex)
			*pwSourceIndex = (u16)dwSourceIndex;
	return wError;
}

u16 HPI_SampleClock_QueryLocalRate(
	const struct hpi_hsubsys *phSubSys,
	const u32 hClock,
	const u32 dwIndex,
	u32 *pdwRate
)
{
	u16 err;
	err = HPI_ControlQuery(phSubSys, hClock,
		HPI_SAMPLECLOCK_LOCAL_SAMPLERATE, dwIndex, 0, pdwRate);

	return err;
}

u16 HPI_SampleClock_SetLocalRate(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwSampleRate
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_SAMPLECLOCK_LOCAL_SAMPLERATE, dwSampleRate, 0);
}

u16 HPI_SampleClock_GetLocalRate(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwSampleRate
)
{
	u16 wError = 0;
	u32 dwSampleRate = 0;
	wError = HPI_ControlParam1Get(phSubSys, hControl,
		HPI_SAMPLECLOCK_LOCAL_SAMPLERATE, &dwSampleRate);
	if (!wError)
		if (pdwSampleRate)
			*pdwSampleRate = dwSampleRate;
	return wError;
}

u16 HPI_SampleClock_GetSampleRate(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwSampleRate
)
{
	u16 wError = 0;
	u32 dwSampleRate = 0;
	wError = HPI_ControlParam1Get(phSubSys, hControl,
		HPI_SAMPLECLOCK_SAMPLERATE, &dwSampleRate);
	if (!wError)
		if (pdwSampleRate)
			*pdwSampleRate = dwSampleRate;
	return wError;
}

u16 HPI_SampleClock_SetAuto(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwAuto
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_SAMPLECLOCK_AUTO, dwAuto, 0);
}

u16 HPI_SampleClock_GetAuto(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwAuto
)
{
	return HPI_ControlParam1Get(phSubSys, hControl,
		HPI_SAMPLECLOCK_AUTO, pdwAuto);
}

u16 HPI_SampleClock_SetLocalRateLock(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwLock
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_SAMPLECLOCK_LOCAL_LOCK, dwLock, 0);
}

u16 HPI_SampleClock_GetLocalRateLock(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwLock
)
{
	return HPI_ControlParam1Get(phSubSys, hControl,
		HPI_SAMPLECLOCK_LOCAL_LOCK, pdwLock);
}

u16 HPI_ToneDetector_GetFrequency(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 nIndex,
	u32 *dwFrequency
)
{
	return HPI_ControlParamGet(phSubSys, hControl,
		HPI_TONEDETECTOR_FREQUENCY, nIndex, 0, dwFrequency, NULL);
}

u16 HPI_ToneDetector_GetState(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *State
)
{
	return HPI_ControlParamGet(phSubSys, hControl, HPI_TONEDETECTOR_STATE,
		0, 0, (u32 *)State, NULL);
}

u16 HPI_ToneDetector_SetEnable(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 Enable
)
{
	return HPI_ControlParamSet(phSubSys, hControl, HPI_GENERIC_ENABLE,
		(u32)Enable, 0);
}

u16 HPI_ToneDetector_GetEnable(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *Enable
)
{
	return HPI_ControlParamGet(phSubSys, hControl, HPI_GENERIC_ENABLE, 0,
		0, (u32 *)Enable, NULL);
}

u16 HPI_ToneDetector_SetEventEnable(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 EventEnable
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_GENERIC_EVENT_ENABLE, (u32)EventEnable, 0);
}

u16 HPI_ToneDetector_GetEventEnable(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *EventEnable
)
{
	return HPI_ControlParamGet(phSubSys, hControl,
		HPI_GENERIC_EVENT_ENABLE, 0, 0, (u32 *)EventEnable, NULL);
}

u16 HPI_ToneDetector_SetThreshold(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	int Threshold
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_TONEDETECTOR_THRESHOLD, (u32)Threshold, 0);
}

u16 HPI_ToneDetector_GetThreshold(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	int *Threshold
)
{
	return HPI_ControlParamGet(phSubSys, hControl,
		HPI_TONEDETECTOR_THRESHOLD, 0, 0, (u32 *)Threshold, NULL);
}

u16 HPI_SilenceDetector_GetState(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *State
)
{
	return HPI_ControlParamGet(phSubSys, hControl,
		HPI_SILENCEDETECTOR_STATE, 0, 0, (u32 *)State, NULL);
}

u16 HPI_SilenceDetector_SetEnable(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 Enable
)
{
	return HPI_ControlParamSet(phSubSys, hControl, HPI_GENERIC_ENABLE,
		(u32)Enable, 0);
}

u16 HPI_SilenceDetector_GetEnable(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *Enable
)
{
	return HPI_ControlParamGet(phSubSys, hControl, HPI_GENERIC_ENABLE, 0,
		0, (u32 *)Enable, NULL);
}

u16 HPI_SilenceDetector_SetEventEnable(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 EventEnable
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_GENERIC_EVENT_ENABLE, (u32)EventEnable, 0);
}

u16 HPI_SilenceDetector_GetEventEnable(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *EventEnable
)
{
	return HPI_ControlParamGet(phSubSys, hControl,
		HPI_GENERIC_EVENT_ENABLE, 0, 0, (u32 *)EventEnable, NULL);
}

u16 HPI_SilenceDetector_SetDelay(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 Delay
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_SILENCEDETECTOR_DELAY, (u32)Delay, 0);
}

u16 HPI_SilenceDetector_GetDelay(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *Delay
)
{
	return HPI_ControlParamGet(phSubSys, hControl,
		HPI_SILENCEDETECTOR_DELAY, 0, 0, (u32 *)Delay, NULL);
}

u16 HPI_SilenceDetector_SetThreshold(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	int Threshold
)
{
	return HPI_ControlParamSet(phSubSys, hControl,
		HPI_SILENCEDETECTOR_THRESHOLD, (u32)Threshold, 0);
}

u16 HPI_SilenceDetector_GetThreshold(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	int *Threshold
)
{
	return HPI_ControlParamGet(phSubSys, hControl,
		HPI_SILENCEDETECTOR_THRESHOLD, 0, 0, (u32 *)Threshold, NULL);
}

u16 HPI_Tuner_QueryBand(
	const struct hpi_hsubsys *phSubSys,
	const u32 hTuner,
	const u32 dwIndex,
	u16 *pwBand
)
{
	u32 qr;
	u16 err;

	err = HPI_ControlQuery(phSubSys, hTuner, HPI_TUNER_BAND,
		dwIndex, 0, &qr);
	*pwBand = (u16)qr;
	return err;
}

u16 HPI_Tuner_SetBand(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 wBand
)
{
	return HPI_ControlParamSet(phSubSys, hControl, HPI_TUNER_BAND,
		wBand, 0);
}

u16 HPI_Tuner_GetBand(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwBand
)
{
	u32 dwBand = 0;
	u16 nError = 0;

	nError = HPI_ControlParam1Get(phSubSys, hControl,
		HPI_TUNER_BAND, &dwBand);
	if (pwBand)
		*pwBand = (u16)dwBand;
	return nError;
}

u16 HPI_Tuner_QueryFrequency(
	const struct hpi_hsubsys *phSubSys,
	const u32 hTuner,
	const u32 dwIndex,
	const u16 band,
	u32 *pdwFreq
)
{
	return HPI_ControlQuery(phSubSys, hTuner, HPI_TUNER_FREQ,
		dwIndex, band, pdwFreq);
}

u16 HPI_Tuner_SetFrequency(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 wFreqInkHz
)
{
	return HPI_ControlParamSet(phSubSys, hControl, HPI_TUNER_FREQ,
		wFreqInkHz, 0);
}

u16 HPI_Tuner_GetFrequency(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pwFreqInkHz
)
{
	return HPI_ControlParam1Get(phSubSys, hControl, HPI_TUNER_FREQ,
		pwFreqInkHz);
}

u16 HPI_Tuner_QueryGain(
	const struct hpi_hsubsys *phSubSys,
	const u32 hTuner,
	const u32 dwIndex,
	u16 *pwGain
)
{
	u32 qr;
	u16 err;

	err = HPI_ControlQuery(phSubSys, hTuner, HPI_TUNER_BAND,
		dwIndex, 0, &qr);
	*pwGain = (u16)qr;
	return err;
}

u16 HPI_Tuner_SetGain(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short nGain
)
{
	return HPI_ControlParamSet(phSubSys, hControl, HPI_TUNER_GAIN,
		nGain, 0);
}

u16 HPI_Tuner_GetGain(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short *pnGain
)
{
	u32 dwGain = 0;
	u16 nError = 0;

	nError = HPI_ControlParam1Get(phSubSys, hControl,
		HPI_TUNER_GAIN, &dwGain);
	if (pnGain)
		*pnGain = (u16)dwGain;
	return nError;
}

u16 HPI_Tuner_GetRFLevel(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short *pwLevel
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = HPI_TUNER_LEVEL;
	hm.u.c.dwParam1 = HPI_TUNER_LEVEL_AVERAGE;
	HPI_Message(&hm, &hr);
	if (pwLevel)
		*pwLevel = (short)hr.u.c.dwParam1;
	return hr.wError;
}

u16 HPI_Tuner_GetRawRFLevel(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short *pwLevel
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = HPI_TUNER_LEVEL;
	hm.u.c.dwParam1 = HPI_TUNER_LEVEL_RAW;
	HPI_Message(&hm, &hr);
	if (pwLevel)
		*pwLevel = (short)hr.u.c.dwParam1;
	return hr.wError;
}

u16 HPI_Tuner_QueryDeemphasis(
	const struct hpi_hsubsys *phSubSys,
	const u32 hTuner,
	const u32 dwIndex,
	const u16 band,
	u32 *pdwDeemphasis
)
{
	return HPI_ControlQuery(phSubSys, hTuner, HPI_TUNER_DEEMPHASIS,
		dwIndex, band, pdwDeemphasis);
}

u16 HPI_Tuner_SetDeemphasis(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwDeemphasis
)
{
	return HPI_ControlParamSet(phSubSys, hControl, HPI_TUNER_DEEMPHASIS,
		dwDeemphasis, 0);
}

u16 HPI_Tuner_GetDeemphasis(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwDeemphasis
)
{
	return HPI_ControlParam1Get(phSubSys, hControl, HPI_TUNER_DEEMPHASIS,
		pdwDeemphasis);
}

u16 HPI_Tuner_QueryProgram(
	const struct hpi_hsubsys *phSubSys,
	const u32 hTuner,
	u32 *pbitmapProgram
)
{
	return HPI_ControlQuery(phSubSys, hTuner, HPI_TUNER_PROGRAM,
		0, 0, pbitmapProgram);
}

u16 HPI_Tuner_SetProgram(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 dwProgram
)
{
	return HPI_ControlParamSet(phSubSys, hControl, HPI_TUNER_PROGRAM,
		dwProgram, 0);
}

u16 HPI_Tuner_GetProgram(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwProgram
)
{
	return HPI_ControlParam1Get(phSubSys, hControl, HPI_TUNER_PROGRAM,
		pdwProgram);
}

u16 HPI_Tuner_GetHdRadioDspVersion(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	char *pszDspVersion,
	const u32 dwStringSize
)
{
	return HPI_Control_GetString(phSubSys,
		hControl,
		HPI_TUNER_HDRADIO_DSP_VERSION, pszDspVersion, dwStringSize);
}

u16 HPI_Tuner_GetHdRadioSdkVersion(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	char *pszSdkVersion,
	const u32 dwStringSize
)
{
	return HPI_Control_GetString(phSubSys,
		hControl,
		HPI_TUNER_HDRADIO_SDK_VERSION, pszSdkVersion, dwStringSize);
}

u16 HPI_Tuner_GetStatus(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u16 *pwStatusMask,
	u16 *pwStatus
)
{
	u32 dwStatus = 0;
	u16 nError = 0;

	nError = HPI_ControlParam1Get(phSubSys, hControl,
		HPI_TUNER_STATUS, &dwStatus);
	if (pwStatus) {
		if (!nError) {
			*pwStatusMask = (u16)(dwStatus >> 16);
			*pwStatus = (u16)(dwStatus & 0xFFFF);
		} else {
			*pwStatusMask = 0;
			*pwStatus = 0;
		}
	}
	return nError;
}

u16 HPI_Tuner_SetMode(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 nMode,
	u32 nValue
)
{
	return HPI_ControlParamSet(phSubSys, hControl, HPI_TUNER_MODE,
		nMode, nValue);
}

u16 HPI_Tuner_GetMode(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 nMode,
	u32 *pnValue
)
{
	return HPI_ControlParamGet(phSubSys, hControl, HPI_TUNER_MODE,
		nMode, 0, pnValue, NULL);
}

u16 HPI_Tuner_GetHdRadioSignalQuality(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwQuality
)
{
	return HPI_ControlParamGet(phSubSys, hControl,
		HPI_TUNER_HDRADIO_SIGNAL_QUALITY, 0, 0, pdwQuality, NULL);
}

u16 HPI_Tuner_GetRDS(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	char *pData
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = HPI_TUNER_RDS;
	HPI_Message(&hm, &hr);
	if (pData) {
		*(u32 *)&pData[0] = hr.u.cu.tuner.rds.dwData[0];
		*(u32 *)&pData[4] = hr.u.cu.tuner.rds.dwData[1];
		*(u32 *)&pData[8] = hr.u.cu.tuner.rds.dwBLER;
	}
	return hr.wError;
}

u16 HPI_PAD_GetChannelName(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	char *pszString,
	const u32 dwDataLength
)
{
	return HPI_Control_GetString(phSubSys,
		hControl, HPI_PAD_CHANNEL_NAME, pszString, dwDataLength);
}

u16 HPI_PAD_GetArtist(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	char *pszString,
	const u32 dwDataLength
)
{
	return HPI_Control_GetString(phSubSys,
		hControl, HPI_PAD_ARTIST, pszString, dwDataLength);
}

u16 HPI_PAD_GetTitle(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	char *pszString,
	const u32 dwDataLength
)
{
	return HPI_Control_GetString(phSubSys,
		hControl, HPI_PAD_TITLE, pszString, dwDataLength);
}

u16 HPI_PAD_GetComment(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	char *pszString,
	const u32 dwDataLength
)
{
	return HPI_Control_GetString(phSubSys,
		hControl, HPI_PAD_COMMENT, pszString, dwDataLength);
}

u16 HPI_PAD_GetProgramType(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwPTY
)
{
	return HPI_ControlParamGet(phSubSys, hControl, HPI_PAD_PROGRAM_TYPE,
		0, 0, pdwPTY, NULL);
}

u16 HPI_PAD_GetRdsPI(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	u32 *pdwPI
)
{
	return HPI_ControlParamGet(phSubSys, hControl, HPI_PAD_PROGRAM_ID,
		0, 0, pdwPI, NULL);
}

u16 HPI_Volume_QueryChannels(
	const struct hpi_hsubsys *phSubSys,
	const u32 hVolume,
	u32 *pChannels
)
{
	return HPI_ControlQuery(phSubSys, hVolume, HPI_VOLUME_NUM_CHANNELS,
		0, 0, pChannels);
}

u16 HPI_VolumeSetGain(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short anLogGain[HPI_MAX_CHANNELS]
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_SET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	memcpy(hm.u.c.anLogValue, anLogGain,
		sizeof(short) * HPI_MAX_CHANNELS);
	hm.u.c.wAttribute = HPI_VOLUME_GAIN;

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_VolumeGetGain(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short anLogGain[HPI_MAX_CHANNELS]
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = HPI_VOLUME_GAIN;

	HPI_Message(&hm, &hr);

	memcpy(anLogGain, hr.u.c.anLogValue,
		sizeof(short) * HPI_MAX_CHANNELS);
	return hr.wError;
}

u16 HPI_VolumeQueryRange(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short *nMinGain_01dB,
	short *nMaxGain_01dB,
	short *nStepGain_01dB
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = HPI_VOLUME_RANGE;

	HPI_Message(&hm, &hr);
	if (hr.wError) {
		hr.u.c.anLogValue[0] = 0;
		hr.u.c.anLogValue[1] = 0;
		hr.u.c.dwParam1 = 0;
	}
	if (nMinGain_01dB)
		*nMinGain_01dB = hr.u.c.anLogValue[0];
	if (nMaxGain_01dB)
		*nMaxGain_01dB = hr.u.c.anLogValue[1];
	if (nStepGain_01dB)
		*nStepGain_01dB = (short)hr.u.c.dwParam1;
	return hr.wError;
}

u16 HPI_VolumeAutoFadeProfile(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short anStopGain0_01dB[HPI_MAX_CHANNELS],
	u32 dwDurationMs,
	u16 wProfile
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_SET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);

	memcpy(hm.u.c.anLogValue, anStopGain0_01dB,
		sizeof(short) * HPI_MAX_CHANNELS);

	hm.u.c.wAttribute = HPI_VOLUME_AUTOFADE;
	hm.u.c.dwParam1 = dwDurationMs;
	hm.u.c.dwParam2 = wProfile;

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_VolumeAutoFade(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short anStopGain0_01dB[HPI_MAX_CHANNELS],
	u32 dwDurationMs
)
{
	return HPI_VolumeAutoFadeProfile(phSubSys,
		hControl,
		anStopGain0_01dB, dwDurationMs, HPI_VOLUME_AUTOFADE_LOG);
}

u16 HPI_VoxSetThreshold(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short anGain0_01dB
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_SET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = HPI_VOX_THRESHOLD;

	hm.u.c.anLogValue[0] = anGain0_01dB;

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_VoxGetThreshold(
	struct hpi_hsubsys *phSubSys,
	u32 hControl,
	short *anGain0_01dB
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_CONTROL,
		HPI_CONTROL_GET_STATE);
	u32TOINDEXES(hControl, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.c.wAttribute = HPI_VOX_THRESHOLD;

	HPI_Message(&hm, &hr);

	*anGain0_01dB = hr.u.c.anLogValue[0];

	return hr.wError;
}

static size_t strv_packet_size = MIN_STRV_PACKET_SIZE;

static size_t entity_type_to_size[LAST_ENTITY_TYPE] = {
	0,
	sizeof(struct hpi_entity),
	sizeof(void *),

	sizeof(int),
	sizeof(float),
	sizeof(double),

	sizeof(char),
	sizeof(char),

	4 * sizeof(char),
	16 * sizeof(char),
	6 * sizeof(char),
};

inline size_t hpi_entity_size(
	struct hpi_entity *entity_ptr
)
{
	return entity_ptr->header.size;
}

inline size_t hpi_entity_header_size(
	struct hpi_entity *entity_ptr
)
{
	return sizeof(entity_ptr->header);
}

inline size_t hpi_entity_value_size(
	struct hpi_entity *entity_ptr
)
{
	return hpi_entity_size(entity_ptr) -
		hpi_entity_header_size(entity_ptr);
}

inline size_t hpi_entity_item_count(
	struct hpi_entity *entity_ptr
)
{
	return hpi_entity_value_size(entity_ptr) /
		entity_type_to_size[entity_ptr->header.type];
}

inline struct hpi_entity *hpi_entity_ptr_to_next(
	struct hpi_entity *entity_ptr
)
{
	return (void *)(((uint8_t *) entity_ptr) +
		hpi_entity_size(entity_ptr));
}

inline u16 hpi_entity_check_type(
	const enum e_entity_type t
)
{
	if (t >= 0 && t < STR_TYPE_FIELD_MAX)
		return 0;
	return HPI_ERROR_ENTITY_TYPE_INVALID;
}

inline u16 hpi_entity_check_role(
	const enum e_entity_role r
)
{
	if (r >= 0 && r < STR_ROLE_FIELD_MAX)
		return 0;
	return HPI_ERROR_ENTITY_ROLE_INVALID;
}

static u16 HPI_Entity_GetNext(
	struct hpi_entity *entity,
	int recursive_flag,
	void *guard_p,
	struct hpi_entity **next
)
{
	HPI_DEBUG_ASSERT(entity != NULL);
	HPI_DEBUG_ASSERT(next != NULL);
	HPI_DEBUG_ASSERT(hpi_entity_size(entity) != 0);

	if (guard_p <= (void *)entity) {
		*next = NULL;
		return 0;
	}

	if (recursive_flag && entity->header.type == entity_type_sequence)
		*next = (struct hpi_entity *)entity->value;
	else
		*next = (struct hpi_entity *)hpi_entity_ptr_to_next(entity);

	if (guard_p <= (void *)*next) {
		*next = NULL;
		return 0;
	}

	HPI_DEBUG_ASSERT(guard_p >= (void *)hpi_entity_ptr_to_next(*next));
	return 0;
}

u16 HPI_Entity_FindNext(
	struct hpi_entity *container_entity,
	enum e_entity_type type,
	enum e_entity_role role,
	int recursive_flag,
	struct hpi_entity **current_match
)
{
	struct hpi_entity *tmp = NULL;
	void *guard_p = NULL;

	HPI_DEBUG_ASSERT(container_entity != NULL);
	guard_p = hpi_entity_ptr_to_next(container_entity);

	if (*current_match != NULL)
		HPI_Entity_GetNext(*current_match, recursive_flag, guard_p,
			&tmp);
	else
		HPI_Entity_GetNext(container_entity, 1, guard_p, &tmp);

	while (tmp) {
		u16 err;

		HPI_DEBUG_ASSERT((void *)tmp >= (void *)container_entity);

		if ((!type || tmp->header.type == type) && (!role
				|| tmp->header.role == role)) {
			*current_match = tmp;
			return 0;
		}

		err = HPI_Entity_GetNext(tmp, recursive_flag, guard_p,
			current_match);
		if (err)
			return err;

		tmp = *current_match;
	}

	*current_match = NULL;
	return 0;
}

void HPI_Entity_Free(
	struct hpi_entity *entity
)
{
	if (entity != NULL)
		kfree(entity);
}

u16 HPI_Entity_AllocAndCopy(
	struct hpi_entity *src,
	struct hpi_entity **dst
)
{
	size_t buf_size;
	HPI_DEBUG_ASSERT(dst != NULL);
	HPI_DEBUG_ASSERT(src != NULL);

	buf_size = hpi_entity_size(src);
	*dst = kmalloc(buf_size, GFP_KERNEL);
	if (dst == NULL)
		return HPI_ERROR_MEMORY_ALLOC;
	memcpy(*dst, src, buf_size);
	return 0;
}

u16 HPI_Universal_Info(
	struct hpi_hsubsys *phSubSys,
	u32 hC,
	struct hpi_entity **info
)
{
	struct hpi_msg_strv hm;
	struct hpi_res_strv *phr;
	u16 hpi_err;
	int remaining_attempts = 2;
	size_t resp_packet_size = 1024;

	*info = NULL;

	while (remaining_attempts--) {
		phr = kmalloc(resp_packet_size, GFP_KERNEL);
		HPI_DEBUG_ASSERT(phr != NULL);

		HPI_InitMessageResponseV1(&hm.h, (u16)sizeof(hm), &phr->h,
			(u16)resp_packet_size, HPI_OBJ_CONTROL,
			HPI_CONTROL_GET_INFO);
		u32TOINDEXES(hC, &hm.h.wAdapterIndex, &hm.h.wObjIndex);

		hm.strv.header.size = sizeof(hm.strv);
		phr->strv.header.size = resp_packet_size - sizeof(phr->h);

		HPI_Message((struct hpi_message *)&hm.h,
			(struct hpi_response *)&phr->h);
		if (phr->h.wError == HPI_ERROR_RESPONSE_BUFFER_TOO_SMALL) {

			HPI_DEBUG_ASSERT(phr->h.wSpecificError >
				MIN_STRV_PACKET_SIZE
				&& phr->h.wSpecificError < 1500);
			resp_packet_size = phr->h.wSpecificError;
		} else {
			remaining_attempts = 0;
			if (!phr->h.wError)
				HPI_Entity_AllocAndCopy(&phr->strv, info);
		}

		hpi_err = phr->h.wError;
		kfree(phr);
	}

	return hpi_err;
}

u16 HPI_Universal_Get(
	struct hpi_hsubsys *phSubSys,
	u32 hC,
	struct hpi_entity **value
)
{
	struct hpi_msg_strv hm;
	struct hpi_res_strv *phr;
	u16 hpi_err;
	int remaining_attempts = 2;

	*value = NULL;

	while (remaining_attempts--) {
		phr = kmalloc(strv_packet_size, GFP_KERNEL);
		if (!phr)
			return HPI_ERROR_MEMORY_ALLOC;

		HPI_InitMessageResponseV1(&hm.h, (u16)sizeof(hm), &phr->h,
			(u16)strv_packet_size, HPI_OBJ_CONTROL,
			HPI_CONTROL_GET_STATE);
		u32TOINDEXES(hC, &hm.h.wAdapterIndex, &hm.h.wObjIndex);

		hm.strv.header.size = sizeof(hm.strv);
		phr->strv.header.size = strv_packet_size - sizeof(phr->h);

		HPI_Message((struct hpi_message *)&hm.h,
			(struct hpi_response *)&phr->h);
		if (phr->h.wError == HPI_ERROR_RESPONSE_BUFFER_TOO_SMALL) {

			HPI_DEBUG_ASSERT(phr->h.wSpecificError >
				MIN_STRV_PACKET_SIZE
				&& phr->h.wSpecificError < 1000);
			strv_packet_size = phr->h.wSpecificError;
		} else {
			remaining_attempts = 0;
			if (!phr->h.wError)
				HPI_Entity_AllocAndCopy(&phr->strv, value);
		}

		hpi_err = phr->h.wError;
		kfree(phr);
	}

	return hpi_err;
}

u16 HPI_Universal_Set(
	struct hpi_hsubsys *phSubSys,
	u32 hC,
	struct hpi_entity *value
)
{
	struct hpi_msg_strv *phm;
	struct hpi_res_strv hr;

	phm = kmalloc(sizeof(phm->h) + value->header.size, GFP_KERNEL);
	HPI_DEBUG_ASSERT(phm != NULL);

	HPI_InitMessageResponseV1(&phm->h,
		sizeof(phm->h) + value->header.size, &hr.h, sizeof(hr),
		HPI_OBJ_CONTROL, HPI_CONTROL_SET_STATE);
	u32TOINDEXES(hC, &phm->h.wAdapterIndex, &phm->h.wObjIndex);
	hr.strv.header.size = sizeof(hr.strv);

	memcpy(&phm->strv, value, value->header.size);
	HPI_Message((struct hpi_message *)&phm->h,
		(struct hpi_response *)&hr.h);

	return hr.h.wError;
}

u16 HPI_Entity_AllocAndPack(
	const enum e_entity_type type,
	const size_t item_count,
	const enum e_entity_role role,
	void *value,
	struct hpi_entity **entity
)
{
	size_t bytes_to_copy, total_size;
	u16 wHE = 0;
	*entity = 0;

	wHE = hpi_entity_check_type(type);
	if (wHE)
		return wHE;

	HPI_DEBUG_ASSERT(role > entity_role_null && type < LAST_ENTITY_ROLE);

	bytes_to_copy = entity_type_to_size[type] * item_count;
	total_size = hpi_entity_header_size(*entity) + bytes_to_copy;

	HPI_DEBUG_ASSERT(total_size >= hpi_entity_header_size(*entity)
		&& total_size < STR_SIZE_FIELD_MAX);

	*entity = kmalloc(total_size, GFP_KERNEL);
	if (*entity == NULL)
		return HPI_ERROR_MEMORY_ALLOC;
	memcpy((*entity)->value, value, bytes_to_copy);
	(*entity)->header.size =
		hpi_entity_header_size(*entity) + bytes_to_copy;
	(*entity)->header.type = type;
	(*entity)->header.role = role;
	return 0;
}

u16 HPI_Entity_CopyValueFrom(
	struct hpi_entity *entity,
	enum e_entity_type type,
	size_t item_count,
	void *value_dst_p
)
{
	size_t bytes_to_copy;

	if (entity->header.type != type)
		return HPI_ERROR_ENTITY_TYPE_MISMATCH;

	if (hpi_entity_item_count(entity) != item_count)
		return HPI_ERROR_ENTITY_ITEM_COUNT;

	bytes_to_copy = entity_type_to_size[type] * item_count;
	memcpy(value_dst_p, entity->value, bytes_to_copy);
	return 0;
}

u16 HPI_Entity_Unpack(
	struct hpi_entity *entity,
	enum e_entity_type *type,
	size_t *item_count,
	enum e_entity_role *role,
	void **value
)
{
	u16 err = 0;
	HPI_DEBUG_ASSERT(entity != NULL);

	if (type)
		*type = entity->header.type;

	if (role)
		*role = entity->header.role;

	if (value)
		*value = entity->value;

	if (item_count != NULL) {
		if (entity->header.type == entity_type_sequence) {
			void *guard_p = hpi_entity_ptr_to_next(entity);
			struct hpi_entity *next = NULL;
			void *contents = entity->value;

			*item_count = 0;
			while (contents < guard_p) {
				(*item_count)++;
				err = HPI_Entity_GetNext(contents, 0, guard_p,
					&next);
				if (next == NULL || err)
					break;
				contents = next;
			}
		} else {
			*item_count = hpi_entity_item_count(entity);
		}
	}
	return err;
}

u16 HPI_GpioOpen(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 *phGpio,
	u16 *pwNumberInputBits,
	u16 *pwNumberOutputBits
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_GPIO, HPI_GPIO_OPEN);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	if (hr.wError == 0) {
		*phGpio = HPI_IndexesToHandle(HPI_OBJ_GPIO, wAdapterIndex, 0);
		if (pwNumberInputBits)
			*pwNumberInputBits = hr.u.l.wNumberInputBits;
		if (pwNumberOutputBits)
			*pwNumberOutputBits = hr.u.l.wNumberOutputBits;
	} else
		*phGpio = 0;
	return hr.wError;
}

u16 HPI_GpioReadBit(
	struct hpi_hsubsys *phSubSys,
	u32 hGpio,
	u16 wBitIndex,
	u16 *pwBitData
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_GPIO, HPI_GPIO_READ_BIT);
	u32TOINDEX(hGpio, &hm.wAdapterIndex);
	hm.u.l.wBitIndex = wBitIndex;

	HPI_Message(&hm, &hr);

	*pwBitData = hr.u.l.wBitData[0];
	return hr.wError;
}

u16 HPI_GpioReadAllBits(
	struct hpi_hsubsys *phSubSys,
	u32 hGpio,
	u16 awAllBitData[4]
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_GPIO, HPI_GPIO_READ_ALL);
	u32TOINDEX(hGpio, &hm.wAdapterIndex);

	HPI_Message(&hm, &hr);

	if (awAllBitData) {
		awAllBitData[0] = hr.u.l.wBitData[0];
		awAllBitData[1] = hr.u.l.wBitData[1];
		awAllBitData[2] = hr.u.l.wBitData[2];
		awAllBitData[3] = hr.u.l.wBitData[3];
	}
	return hr.wError;
}

u16 HPI_GpioWriteBit(
	struct hpi_hsubsys *phSubSys,
	u32 hGpio,
	u16 wBitIndex,
	u16 wBitData
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_GPIO, HPI_GPIO_WRITE_BIT);
	u32TOINDEX(hGpio, &hm.wAdapterIndex);
	hm.u.l.wBitIndex = wBitIndex;
	hm.u.l.wBitData = wBitData;

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_GpioWriteStatus(
	struct hpi_hsubsys *phSubSys,
	u32 hGpio,
	u16 awAllBitData[4]
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_GPIO,
		HPI_GPIO_WRITE_STATUS);
	u32TOINDEX(hGpio, &hm.wAdapterIndex);

	HPI_Message(&hm, &hr);

	if (awAllBitData) {
		awAllBitData[0] = hr.u.l.wBitData[0];
		awAllBitData[1] = hr.u.l.wBitData[1];
		awAllBitData[2] = hr.u.l.wBitData[2];
		awAllBitData[3] = hr.u.l.wBitData[3];
	}
	return hr.wError;
}

u16 HPI_AsyncEventOpen(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 *phAsync
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ASYNCEVENT,
		HPI_ASYNCEVENT_OPEN);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	if (hr.wError == 0)

		*phAsync =
			HPI_IndexesToHandle(HPI_OBJ_ASYNCEVENT, wAdapterIndex,
			0);
	else
		*phAsync = 0;
	return hr.wError;

}

u16 HPI_AsyncEventClose(
	struct hpi_hsubsys *phSubSys,
	u32 hAsync
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ASYNCEVENT,
		HPI_ASYNCEVENT_OPEN);
	u32TOINDEX(hAsync, &hm.wAdapterIndex);

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_AsyncEventWait(
	struct hpi_hsubsys *phSubSys,
	u32 hAsync,
	u16 wMaximumEvents,
	struct hpi_async_event *pEvents,
	u16 *pwNumberReturned
)
{
	return (0);
}

u16 HPI_AsyncEventGetCount(
	struct hpi_hsubsys *phSubSys,
	u32 hAsync,
	u16 *pwCount
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ASYNCEVENT,
		HPI_ASYNCEVENT_GETCOUNT);
	u32TOINDEX(hAsync, &hm.wAdapterIndex);

	HPI_Message(&hm, &hr);

	if (hr.wError == 0)
		if (pwCount)
			*pwCount = hr.u.as.u.count.wCount;

	return hr.wError;
}

u16 HPI_AsyncEventGet(
	struct hpi_hsubsys *phSubSys,
	u32 hAsync,
	u16 wMaximumEvents,
	struct hpi_async_event *pEvents,
	u16 *pwNumberReturned
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_ASYNCEVENT,
		HPI_ASYNCEVENT_GET);
	u32TOINDEX(hAsync, &hm.wAdapterIndex);

	HPI_Message(&hm, &hr);
	if (!hr.wError) {
		memcpy(pEvents, &hr.u.as.u.event,
			sizeof(struct hpi_async_event));
		*pwNumberReturned = 1;
	}

	return hr.wError;
}

u16 HPI_NvMemoryOpen(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 *phNvMemory,
	u16 *pwSizeInBytes
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_NVMEMORY,
		HPI_NVMEMORY_OPEN);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	if (hr.wError == 0) {
		*phNvMemory =
			HPI_IndexesToHandle(HPI_OBJ_NVMEMORY, wAdapterIndex,
			0);
		if (pwSizeInBytes)
			*pwSizeInBytes = hr.u.n.wSizeInBytes;
	} else
		*phNvMemory = 0;
	return hr.wError;
}

u16 HPI_NvMemoryReadByte(
	struct hpi_hsubsys *phSubSys,
	u32 hNvMemory,
	u16 wIndex,
	u16 *pwData
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_NVMEMORY,
		HPI_NVMEMORY_READ_BYTE);
	u32TOINDEX(hNvMemory, &hm.wAdapterIndex);
	hm.u.n.wAddress = wIndex;

	HPI_Message(&hm, &hr);

	*pwData = hr.u.n.wData;
	return hr.wError;
}

u16 HPI_NvMemoryWriteByte(
	struct hpi_hsubsys *phSubSys,
	u32 hNvMemory,
	u16 wIndex,
	u16 wData
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_NVMEMORY,
		HPI_NVMEMORY_WRITE_BYTE);
	u32TOINDEX(hNvMemory, &hm.wAdapterIndex);
	hm.u.n.wAddress = wIndex;
	hm.u.n.wData = wData;

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_ProfileOpenAll(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u16 wProfileIndex,
	u32 *phProfile,
	u16 *pwMaxProfiles
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_PROFILE,
		HPI_PROFILE_OPEN_ALL);
	hm.wAdapterIndex = wAdapterIndex;
	hm.wObjIndex = wProfileIndex;
	HPI_Message(&hm, &hr);

	*pwMaxProfiles = hr.u.p.u.o.wMaxProfiles;
	if (hr.wError == 0)
		*phProfile =
			HPI_IndexesToHandle(HPI_OBJ_PROFILE, wAdapterIndex,
			wProfileIndex);
	else
		*phProfile = 0;
	return hr.wError;
}

u16 HPI_ProfileGet(
	struct hpi_hsubsys *phSubSys,
	u32 hProfile,
	u16 wBinIndex,
	u16 *pwSeconds,
	u32 *pdwMicroSeconds,
	u32 *pdwCallCount,
	u32 *pdwMaxMicroSeconds,
	u32 *pdwMinMicroSeconds
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_PROFILE, HPI_PROFILE_GET);
	u32TOINDEXES(hProfile, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.p.wBinIndex = wBinIndex;
	HPI_Message(&hm, &hr);
	if (pwSeconds)
		*pwSeconds = hr.u.p.u.t.wSeconds;
	if (pdwMicroSeconds)
		*pdwMicroSeconds = hr.u.p.u.t.dwMicroSeconds;
	if (pdwCallCount)
		*pdwCallCount = hr.u.p.u.t.dwCallCount;
	if (pdwMaxMicroSeconds)
		*pdwMaxMicroSeconds = hr.u.p.u.t.dwMaxMicroSeconds;
	if (pdwMinMicroSeconds)
		*pdwMinMicroSeconds = hr.u.p.u.t.dwMinMicroSeconds;
	return hr.wError;
}

u16 HPI_ProfileGetUtilization(
	struct hpi_hsubsys *phSubSys,
	u32 hProfile,
	u32 *pdwUtilization
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_PROFILE,
		HPI_PROFILE_GET_UTILIZATION);
	u32TOINDEXES(hProfile, &hm.wAdapterIndex, &hm.wObjIndex);
	HPI_Message(&hm, &hr);
	if (hr.wError) {
		if (pdwUtilization)
			*pdwUtilization = 0;
	} else {
		if (pdwUtilization)
			*pdwUtilization = hr.u.p.u.t.dwCallCount;
	}
	return hr.wError;
}

u16 HPI_ProfileGetName(
	struct hpi_hsubsys *phSubSys,
	u32 hProfile,
	u16 wBinIndex,
	char *szName,
	u16 nNameLength
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_PROFILE,
		HPI_PROFILE_GET_NAME);
	u32TOINDEXES(hProfile, &hm.wAdapterIndex, &hm.wObjIndex);
	hm.u.p.wBinIndex = wBinIndex;
	HPI_Message(&hm, &hr);
	if (hr.wError) {
		if (szName)
			strcpy(szName, "??");
	} else {
		if (szName)
			memcpy(szName, (char *)hr.u.p.u.n.szName,
				nNameLength);
	}
	return hr.wError;
}

u16 HPI_ProfileStartAll(
	struct hpi_hsubsys *phSubSys,
	u32 hProfile
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_PROFILE,
		HPI_PROFILE_START_ALL);
	u32TOINDEXES(hProfile, &hm.wAdapterIndex, &hm.wObjIndex);
	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_ProfileStopAll(
	struct hpi_hsubsys *phSubSys,
	u32 hProfile
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_PROFILE,
		HPI_PROFILE_STOP_ALL);
	u32TOINDEXES(hProfile, &hm.wAdapterIndex, &hm.wObjIndex);
	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_WatchdogOpen(
	struct hpi_hsubsys *phSubSys,
	u16 wAdapterIndex,
	u32 *phWatchdog
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_WATCHDOG,
		HPI_WATCHDOG_OPEN);
	hm.wAdapterIndex = wAdapterIndex;

	HPI_Message(&hm, &hr);

	if (hr.wError == 0)
		*phWatchdog =
			HPI_IndexesToHandle(HPI_OBJ_WATCHDOG, wAdapterIndex,
			0);
	else
		*phWatchdog = 0;
	return hr.wError;
}

u16 HPI_WatchdogSetTime(
	struct hpi_hsubsys *phSubSys,
	u32 hWatchdog,
	u32 dwTimeMillisec
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_WATCHDOG,
		HPI_WATCHDOG_SET_TIME);
	u32TOINDEX(hWatchdog, &hm.wAdapterIndex);
	hm.u.w.dwTimeMs = dwTimeMillisec;

	HPI_Message(&hm, &hr);

	return hr.wError;
}

u16 HPI_WatchdogPing(
	struct hpi_hsubsys *phSubSys,
	u32 hWatchdog
)
{
	struct hpi_message hm;
	struct hpi_response hr;
	HPI_InitMessageResponse(&hm, &hr, HPI_OBJ_WATCHDOG,
		HPI_WATCHDOG_PING);
	u32TOINDEX(hWatchdog, &hm.wAdapterIndex);

	HPI_Message(&hm, &hr);

	return hr.wError;
}
