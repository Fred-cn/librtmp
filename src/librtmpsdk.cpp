// RTMPClientSDK.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <srs_kernel_error.hpp>
#include "librtmpsdk.h"
#include "LibRtmpSession.hpp"


#define CHECK_POINTER(lp) {if(!lp){ return ERROR_INVALIDHANDLE;}}

/* 创建RTMP会话Session 返回会话句柄 */
RTMPCLIENT_API HANDLE CALLMETHOD RTMPCLIENT_Session_Create()
{
	return new LibRtmpSession();
}

/* 停止RTMP会话，释放句柄 */
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_Session_Release(HANDLE hSession)
{
	if (hSession)
	{
		delete (LibRtmpSession*)hSession;
		hSession = NULL;
	}

	return ERROR_SUCCESS;
}

/* 设置超时参数 */
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_SetTimeout(IN HANDLE   hSession,
	IN int      nRecvTimeoutMsecs,
	IN int      nSendTimeoutMsecs
)
{
	CHECK_POINTER(hSession);

	return ((LibRtmpSession*)hSession)->SetTimeout(nRecvTimeoutMsecs, nSendTimeoutMsecs);
}

/* 连接RTMP服务器推流 */
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_ConnectPublish(IN HANDLE      hSession,
	IN const char* url,
	IN int         nTimeoutMS,
	IN int         nChunkSize
)
{
	CHECK_POINTER(hSession);
	if (!url)
	{
		return ERROR_INVALIDPARAM;
	}

	return ((LibRtmpSession*)hSession)->Connect(TRUE, url, nTimeoutMS, nChunkSize, 1, 0);
}

/* 连接RTMP服务器拉流 */
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_ConnectPlay(IN HANDLE      hSession,
	IN const char* url,
	IN int         nTimeoutMS,
	IN int         nBufferMS,
	IN int         bLiveStream,
	IN int         nSeekPlayTime/* = 0*/
)
{
	CHECK_POINTER(hSession);
	if (!url)
	{
		return ERROR_INVALIDPARAM;
	}

	return ((LibRtmpSession*)hSession)->Connect(FALSE, url, nTimeoutMS, nBufferMS, bLiveStream, nSeekPlayTime);
}

/* 连接RTMP是否正常 */
RTMPCLIENT_API BOOL CALLMETHOD RTMPCLIENT_IsLinked(HANDLE hSession)
{
	CHECK_POINTER(hSession);

	return ((LibRtmpSession*)hSession)->IsConnected() ? 1 : 0;
}

RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_DisConnect(IN HANDLE hSession)
{
	CHECK_POINTER(hSession);

	((LibRtmpSession*)hSession)->DisConnect();

	return 0;
}

///////////////////RTMP推送API begin///////////////////////////////////////////////////////

/* 创建RTMP推送的参数信息 */
RTMPCLIENT_API int CALLMETHOD  RTMPCLIENT_InitMetadata(HANDLE hSession, InitAudioParam* initAudio, int videoFrameRate)
{
	CHECK_POINTER(hSession);
	if (!initAudio)
	{
		return ERROR_INVALIDPARAM;
	}

	return ((LibRtmpSession*)hSession)->InitMetadata(initAudio, videoFrameRate);
}

/* 推送H264视频 */
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_SendH264Frame(HANDLE hSession, unsigned char* data, int size, time_t nTimeStamp, BOOL bIsKeyFrame)
{
	CHECK_POINTER(hSession);
	if (!data || size <= 0)
	{
		return ERROR_INVALIDPARAM;
	}

	return ((LibRtmpSession*)hSession)->SendH264VideoData(data, size, nTimeStamp, bIsKeyFrame ? true : false);
}

/* 推送H265视频 */
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_SendH265Frame(HANDLE hSession, unsigned char* data, int size, time_t nTimeStamp, BOOL bIsKeyFrame)
{
	CHECK_POINTER(hSession);
	if (!data || size <= 0)
	{
		return ERROR_INVALIDPARAM;
	}

	return ((LibRtmpSession*)hSession)->SendH265VideoData(data, size, nTimeStamp, bIsKeyFrame ? true : false);
}

/* 推送AAC音频 */
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_SendAACFrame(HANDLE hSession, unsigned char* data, int size, time_t nTimeStamp)
{
	CHECK_POINTER(hSession);
	if (!data || size <= 0)
	{
		return ERROR_INVALIDPARAM;
	}

	return ((LibRtmpSession*)hSession)->SendAudioAdtsAAC(data, size, nTimeStamp);
}

/* 推送音频 */
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_SendAudioFrame(IN HANDLE          hSession,
	IN unsigned char*  data,
	IN int             size,
	IN time_t          nTimeStamp,
	IN eAudioCodecType nAudioCodecType
)
{
	CHECK_POINTER(hSession);
	if (!data || size <= 0)
	{
		return ERROR_INVALIDPARAM;
	}

	return ((LibRtmpSession*)hSession)->SendAudioData(data, size, nTimeStamp, nAudioCodecType);//SendAudioRawData
}

/* 推送FlvTag */
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_SendFlvPacket(HANDLE hSession, unsigned char* data, int size, time_t nTimeStamp, int cTagType)
{
	CHECK_POINTER(hSession);
	if (!data || size <= 0)
	{
		return ERROR_INVALIDPARAM;
	}

	return ((LibRtmpSession*)hSession)->SendPacket(cTagType, data, size, nTimeStamp);
}

///////////////////RTMP推送API end///////////////////////////////////////////////////////


///////////////////RTMP读取API begin///////////////////////////////////////////////////////

/* 读取RTMP数据,返回一个完整的flvTag */
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_ReadPacket(IN    HANDLE            hSession,
	OUT   char*             buff,
	INOUT int*              size,
	OUT   time_t*           nTimeStamp,
	OUT   char*             cTagType
)
{
	CHECK_POINTER(hSession);
	if (!buff || !size || !nTimeStamp)
	{
		return ERROR_INVALIDPARAM;
	}

	return ((LibRtmpSession*)hSession)->ReadPacket(buff, size, nTimeStamp, cTagType);
}

RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_ReadData(IN    HANDLE            hSession,
	OUT   char*             buff,
	INOUT int*              size
)
{
	CHECK_POINTER(hSession);
	if (!buff || !size)
	{
		return ERROR_INVALIDPARAM;
	}

	return ((LibRtmpSession*)hSession)->ReadDataEx(buff, size);
}

RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_PlayPause(IN    HANDLE             hSession,
	IN    BOOL               bPause
)
{
	CHECK_POINTER(hSession);

	return ((LibRtmpSession*)hSession)->Pause(bPause);
}

/**
*  @brief        改变播放的位置
*  @param[in]    hSession     : RTMPCLIENT_Session_Create成功的返回值
*  @param[in]    seekTime     : seek vod play time,时间单位:毫秒
*  @return       成功返回0，否则为失败的错误代码
*  @remarks      vod play时有效，live play时无效
*/
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_PlaySeek(IN    HANDLE              hSession,
	IN    int                 seekTime
)
{
	CHECK_POINTER(hSession);

	return ((LibRtmpSession*)hSession)->Seek(seekTime);
}

//RTMP流包提取H264原始帧
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_FlvTag2H264(HANDLE hSession, char* pSrc, int nSrcLen, char* pDst, int* nDstLen, int* nFrameType)
{
	CHECK_POINTER(hSession);
	if (!pSrc || nSrcLen <= 0 || !pDst || !nDstLen || !nFrameType)
	{
		return ERROR_INVALIDPARAM;
	}

	return ((LibRtmpSession*)hSession)->Flv2H264(pSrc, nSrcLen, pDst, nDstLen, nFrameType);
}

//RTMP流包提取AAC帧
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_FlvTag2AAC(HANDLE hSession, char* pSrc, int nSrcLen, char* pDst, int* nDstLen)
{
	CHECK_POINTER(hSession);
	if (!pSrc || nSrcLen <= 0 || !pDst || !nDstLen)
	{
		return ERROR_INVALIDPARAM;
	}

	return ((LibRtmpSession*)hSession)->Flv2AAC(pSrc, nSrcLen, pDst, nDstLen);
}

//从H264原始帧提取视频信息
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_ParseH264SPSPPS(char* pSrc, int nSrcLen, VideoEncoderInfo* info)
{
	if (!pSrc || nSrcLen <= 0 || !info)
	{
		return ERROR_INVALIDPARAM;
	}
	return ParseH264SPSPPS(pSrc, nSrcLen, info);
}
