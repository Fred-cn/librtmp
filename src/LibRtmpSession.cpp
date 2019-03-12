//
//  LibRtmpSession.cpp
//  AudioEditX
//
//  Created by Alex.Shi on 16/3/8.
//  Copyright @2016  com.Alex. All rights reserved.
//

#include "LibRtmpSession.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <memory.h>
#include <time.h>
#include "librtmp/rtmp.h"
#include "librtmp/log.h"
#include "sps_decode.h"
#include "h264analyzer/h264_stream.h"
#include "h264analyzer/GetVPSSPSPPS.h"
#include "BigLittleSwap.h"
#include "FlvAacEncoder.hpp"
#include "AACEncoder.h" // G7XX to AAC
#include "adtsAAC/adts.h"
//#define SDK_DEBUG_OUT_TRACE
//#include "../CommonLib/DebugTrace/DebugTrace.h"

#include "srs_kernel_error.hpp"
#include "srs_librtmp.hpp"
#ifdef WIN32
#ifdef _DEBUG
#pragma comment(lib, "srs_librtmp.lib")
#else
#pragma comment(lib, "srs_librtmp.lib")
#endif // _DEBUG
#endif // WIN32


#define DATA_ITEMS_MAX_COUNT 100
#define PILI_RTMP_DATA_RESERVE_SIZE 400

#define PILI_RTMP_CONNECTION_TIMEOUT 1500
#define PILI_RTMP_RECEIVE_TIMEOUT    3


static int g_h264spaceNal = 0x01000000;//H264 NALU 00000001

#define LOG_TAG "PILI_RTMP_SESSION"

#define LOGI(...) //__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__);
#ifdef _DEBUG
#define LOGE printf
#else
#define LOGI(...)
#define LOGE(...)
#endif // _DEBUG


#ifndef NULL
#define NULL 0
#endif


typedef struct _DataItem
{
	char*           data;
	unsigned int    size;
	int             headlen;
}DataItem;

typedef struct _RTMPMetadata
{
	// video, must be h264 type
	unsigned int    nWidth;
	unsigned int    nHeight;
	unsigned int    nFrameRate;
	unsigned short  nSpsLen;
	unsigned char   *Sps;
	unsigned short  nPpsLen;
	unsigned char   *Pps;
	unsigned short  nVpsLen;
	unsigned char   *Vps;
} RTMPMetadata, *LPRTMPMetadata;


int ModifyNaluData(char* pBuff, int len);

LibRtmpSession::LibRtmpSession()
	:m_pRtmp(NULL)
	, m_pFileLog(0)
	, m_strRtmpURL("")
	, m_bPublishFlag(0)
	, m_iConnectFlag(0)
	, m_recv_timeout_ms(0)
	, m_send_timeout_ms(0)
	, m_iAacType(0)
	, m_iSampleRate(0)
	, m_iChannels(0)
	, m_iWidth(0)
	, m_iHeight(0)
	, m_iFps(0)
	, m_pAdtsItems(NULL)
	, m_pNaluItems(NULL)
	, m_iMetaDataFlag(0)
	, m_iASCSentFlag(0)
	, m_cFlvAudioHead0(0xAF)
	, m_uiStartTimestamp(0)
	, m_uiAudioDTS(0)
	, m_uiVideoLastAudioDTS(0)
	, m_uiAudioDTSNoChangeCnt(0)
	, m_pMetaData(NULL)
	, m_strAdts("")
	, m_pAACEncoder(NULL)
	, m_strAACBuf("")
	, m_bInitAudioParam(false)
	, m_strVideoIHead("")
	, m_pFlvAacEncoder(0)
{

}

LibRtmpSession::~LibRtmpSession()
{
	//if(m_iConnectFlag != 0) 
	{
		DisConnect();
		if (m_pRtmp) {
			LOGI("PILI_RTMP_Free: PILI_RTMP_Free...");
			PILI_RTMP_Free(m_pRtmp);
			LOGI("PILI_RTMP_Free: PILI_RTMP_Free...END");

			m_pRtmp = NULL;
		}
		if (m_pAdtsItems) {
			free(m_pAdtsItems);
		}
		if (m_pNaluItems) {
			free(m_pNaluItems);
		}
		if (m_pMetaData) {
			if (m_pMetaData->Pps)
			{
				free(m_pMetaData->Pps);
				m_pMetaData->Pps = NULL;
			}
			if (m_pMetaData->Sps)
			{
				free(m_pMetaData->Sps);
				m_pMetaData->Sps = NULL;
			}
			if (m_pMetaData->Vps)
			{
				free(m_pMetaData->Vps);
				m_pMetaData->Vps = NULL;
			}
			free(m_pMetaData);
			m_pMetaData = NULL;
		}
		if (m_pFlvAacEncoder) {
			delete m_pFlvAacEncoder;
			m_pFlvAacEncoder = NULL;
		}
	}
	if (m_pFileLog)
	{
		fclose(m_pFileLog);
		m_pFileLog = NULL;
	}
}

int LibRtmpSession::SetTimeout(int recv_timeout_ms, int send_timeout_ms)
{
	m_recv_timeout_ms = recv_timeout_ms;
	m_send_timeout_ms = send_timeout_ms;

	if (recv_timeout_ms < 30)
	{
		m_recv_timeout_ms = recv_timeout_ms;
	}
	else
	{
		m_recv_timeout_ms = recv_timeout_ms / 1000;
		if (m_recv_timeout_ms == 0)
		{
			m_recv_timeout_ms = PILI_RTMP_RECEIVE_TIMEOUT;
		}
	}

	return 0;
}

// 设置RTMP推送直播服务器的ChunkSize大小，用以解决在公网推送时，
// 服务器不能正确播放视频的问题 [7/29/2014-10:47:41 Dingshuai]
int LibRtmpSession::SendReSetChunkSize(int nChunkSize)
{
	RTMPPacket pack;
	int nVal = nChunkSize;
	PILI_RTMPPacket_Alloc(&pack, 4);
	pack.m_packetType = RTMP_PACKET_TYPE_CHUNK_SIZE;
	pack.m_nChannel = 0x02;
	pack.m_headerType = RTMP_PACKET_SIZE_LARGE;
	pack.m_nTimeStamp = 0;
	pack.m_nInfoField2 = 0;
	pack.m_nBodySize = 4;
	pack.m_body[3] = nVal & 0xff;
	pack.m_body[2] = nVal >> 8;
	pack.m_body[1] = nVal >> 16;
	pack.m_body[0] = nVal >> 24;

	m_pRtmp->m_outChunkSize = nVal;
	RTMPError err = { 0 };
	int nRet = PILI_RTMP_SendPacket(m_pRtmp, &pack, 0, &err);
	PILI_RTMPPacket_Free(&pack);

	return nRet;
}

int LibRtmpSession::Connect(int bPublishFlag, const char* szRtmpUrl, int nTimeoutMS,
	int nBufferMS, int bLiveStream, int iSeekPlayTime)
{
	int iRet = 0;
	m_bPublishFlag = bPublishFlag;
	m_strRtmpURL = szRtmpUrl;
	m_iConnectFlag = 0;

#if 0//def _DEBUG
	if (!bPublishFlag)
	{
		char szFileName[256] = { 0 };

		sprintf_s(szFileName, 256, "rtmp_%I64d.log", time(NULL));
		m_pFileLog = fopen(szFileName, "wb");

		PILI_RTMP_LogSetLevel(PILI_RTMP_LOGDEBUG);
		PILI_RTMP_LogSetOutput(m_pFileLog);
	}
#endif // _DEBUG

	if (m_pRtmp) {
		PILI_RTMP_Free(m_pRtmp);
		m_pRtmp = NULL;
	}
	if (!m_pRtmp)
	{
		m_pRtmp = PILI_RTMP_Alloc();
		if (m_pRtmp)
		{
			PILI_RTMP_Init(m_pRtmp);
		}
		else
		{
			free(m_pRtmp);
			m_pRtmp = NULL;
			iRet = ERROR_SOCKET_CREATE;

			return iRet;
		}
	}

	RTMPError err = { 0 };
	//LOGI("PILI_RTMP_SetupURL:%s", _szRtmpUrl);
	if (PILI_RTMP_SetupURL(m_pRtmp, (char*)m_strRtmpURL.c_str(), &err) == FALSE)
	{
		iRet = ERROR_INVALIDPARAM;
		free(m_pRtmp);
		m_pRtmp = NULL;
		//pthread_mutex_unlock(&_mConnstatMutex);
		return iRet;
	}
	//LOGI("PILI_RTMP_EnableWrite...");
	if (bPublishFlag)
	{
		PILI_RTMP_EnableWrite(m_pRtmp);

		m_pNaluItems = (DataItem*)malloc(sizeof(DataItem)*DATA_ITEMS_MAX_COUNT);
		memset((void*)m_pNaluItems, 0, sizeof(DataItem)*DATA_ITEMS_MAX_COUNT);

		m_pMetaData = (RTMPMetadata*)malloc(sizeof(RTMPMetadata));
		memset((void*)m_pMetaData, 0, sizeof(RTMPMetadata));
	}
	else
	{
		/* Try to keep the stream moving if it pauses on us */

		if (bLiveStream)
			//设置直播标志
			m_pRtmp->Link.lFlags |= RTMP_LF_LIVE;
		else
			m_pRtmp->Link.lFlags |= RTMP_LF_BUFX;

		PILI_RTMP_SetBufferMS(m_pRtmp, nBufferMS);//告诉服务器帮我缓存多久 
	}

	int connect_timeout_ms = PILI_RTMP_RECEIVE_TIMEOUT;
	if (nTimeoutMS > 0)
	{
		if (nTimeoutMS < 30)
		{
			connect_timeout_ms = nTimeoutMS;
		}
		else
		{
			connect_timeout_ms = nTimeoutMS / 1000;
			if (connect_timeout_ms == 0)
			{
				connect_timeout_ms = PILI_RTMP_RECEIVE_TIMEOUT;
			}
		}
	}

	//由于crtmpserver是每个一段时间(默认8s)发送数据包,需大于发送间隔才行
	if (m_recv_timeout_ms <= 0)
	{
		m_recv_timeout_ms = connect_timeout_ms;
	}

	m_pRtmp->Link.timeout = m_recv_timeout_ms;


	//LOGI("PILI_RTMP_Connect...");
	if (PILI_RTMP_Connect(m_pRtmp, NULL, &err) == FALSE)
	{
		PILI_RTMP_Close(m_pRtmp, &err);
		PILI_RTMP_Free(m_pRtmp);
		m_pRtmp = NULL;
		iRet = ERROR_SOCKET_CONNECT;
		LOGI("PILI_RTMP_Connect...error");
		return iRet;
	}

	//LOGI("PILI_RTMP_ConnectStream...");
	if (PILI_RTMP_ConnectStream(m_pRtmp, iSeekPlayTime, &err) == FALSE)
	{
		PILI_RTMP_Close(m_pRtmp, &err);
		PILI_RTMP_Free(m_pRtmp);
		m_pRtmp = NULL;
		iRet = ERROR_RTMP_STREAM_NOT_FOUND;
		LOGE("PILI_RTMP_ConnectStream...error. \n");
		return iRet;
	}
	//if (bPublishFlag)
	{
		//Chunk的默认大小是128字节，在传输过程中，通过一个叫做Set Chunk Size的控制信息可以设置Chunk数据量的最大值，
		//在发送端和接受端会各自维护一个Chunk Size，可以分别设置这个值来改变自己这一方发送的Chunk的最大大小。
		//大一点的Chunk减少了计算每个chunk的时间从而减少了CPU的占用率
		SendReSetChunkSize(1024 * 1024);
	}
	LOGE("PILI_RTMP_ConnectStream...ok. \n");
	//m_pRtmp->m_read.flags |= PILI_RTMP_READ_RESUME;
	m_iConnectFlag = 1;
	m_iMetaDataFlag = 0;

	return iRet;
}

void LibRtmpSession::DisConnect()
{
	if (m_pRtmp)
	{
		if (m_iConnectFlag)
		{
			LOGI("DisConnect: PILI_RTMP_Close...");
			RTMPError err = { 0 };
			PILI_RTMP_Close(m_pRtmp, &err);
			LOGI("DisConnect: PILI_RTMP_Close...END");
		}

		m_iConnectFlag = 0;
		m_iMetaDataFlag = 0;
	}
	if (m_pAACEncoder)
	{
		delete m_pAACEncoder;
		m_pAACEncoder = NULL;
		m_bInitAudioParam = false;
	}

#ifdef _DEBUG
	if (m_pFileLog)
	{
		fclose(m_pFileLog);
		m_pFileLog = NULL;
	}
#endif
}

int LibRtmpSession::IsConnected()
{
	if (m_iConnectFlag == 0)
	{
		return 0;
	}
	if (m_pRtmp == NULL) {
		return 0;
	}
	//pthread_mutex_lock(&_mConnstatMutex);
	int iRet = PILI_RTMP_IsConnected(m_pRtmp);
	//pthread_mutex_unlock(&_mConnstatMutex);
	return iRet;
}

int LibRtmpSession::InitMetadata(InitAudioParam* initAudio, int videoFrameRate)
{
	if (!initAudio)
	{
		return ERROR_INVALIDPARAM;
	}

	m_initParam = *initAudio;
	m_iASCSentFlag = 0;
	m_bInitAudioParam = true;

	return ERROR_SUCCESS;
}

int LibRtmpSession::getSampleRateByType(int iType)
{
	int iSampleRate = 44100;
	switch (iType) {
	case 0:
		iSampleRate = 96000;
		break;
	case 1:
		iSampleRate = 88200;
		break;
	case 2:
		iSampleRate = 64000;
		break;
	case 3:
		iSampleRate = 48000;
		break;
	case 4:
		iSampleRate = 44100;
		break;
	case 5:
		iSampleRate = 32000;
		break;
	case 6:
		iSampleRate = 24000;
		break;
	case 7:
		iSampleRate = 22050;
		break;
	case 8:
		iSampleRate = 16000;
		break;
	case 9:
		iSampleRate = 12000;
		break;
	case 10:
		iSampleRate = 11025;
		break;
	case 11:
		iSampleRate = 8000;
		break;
	case 12:
		iSampleRate = 7350;
		break;
	}

	return iSampleRate;
}

int LibRtmpSession::getSampleRateType(int iSampleRate) {
	int iRetType = 4;

	switch (iSampleRate) {
	case 96000:
		iRetType = 0;
		break;
	case 88200:
		iRetType = 1;
		break;
	case 64000:
		iRetType = 2;
		break;
	case 48000:
		iRetType = 3;
		break;
	case 44100:
		iRetType = 4;
		break;
	case 32000:
		iRetType = 5;
		break;
	case 24000:
		iRetType = 6;
		break;
	case 22050:
		iRetType = 7;
		break;
	case 16000:
		iRetType = 8;
		break;
	case 12000:
		iRetType = 9;
		break;
	case 11025:
		iRetType = 10;
		break;
	case 8000:
		iRetType = 11;
		break;
	case 7350:
		iRetType = 12;
		break;
	}
	return iRetType;
}

int LibRtmpSession::getFlvAudioHeadSampleRateType(int sampling_frequency_index)
{
	int nSoundRateType = RTMPCodecAudioSampleRate44100;
	if (sampling_frequency_index <= 0x0c && sampling_frequency_index > 0x0a) {
		nSoundRateType = RTMPCodecAudioSampleRate5512;
	}
	else if (sampling_frequency_index <= 0x0a && sampling_frequency_index > 0x07) {
		nSoundRateType = RTMPCodecAudioSampleRate11025;
	}
	else if (sampling_frequency_index <= 0x07 && sampling_frequency_index > 0x04) {
		nSoundRateType = RTMPCodecAudioSampleRate22050;
	}
	else if (sampling_frequency_index <= 0x04) {
		nSoundRateType = RTMPCodecAudioSampleRate44100;
	}

	return nSoundRateType;
}

int flv_codec_aac_ts2rtmp(int profile)
{
	switch (profile) {
	case RTMPAacProfileMain: return RTMPAacObjectTypeAacMain;
	case RTMPAacProfileLC: return RTMPAacObjectTypeAacLC;
	case RTMPAacProfileSSR: return RTMPAacObjectTypeAacSSR;
	default: return RTMPAacObjectTypeReserved;
	}
}


void LibRtmpSession::GetASCInfo(unsigned short usAscFlag)
{
	//ASC FLAG: xxxx xaaa aooo o111
	m_iAacType = (usAscFlag & 0xf800) >> 11;
	m_iSampleRate = (usAscFlag & 0x0780) >> 7;
	m_iChannels = (usAscFlag & 0x78) >> 3;
}

void LibRtmpSession::GetSpsInfo(unsigned char* pSpsData, int iLength)
{
	int* Width = &m_iWidth;
	int* Height = &m_iHeight;
	int* Fps = &m_iFps;
	h264_decode_sps(pSpsData, iLength, &Width, &Height, &Fps);
}

unsigned char MakeFlvAudioHead(int formatCode, int iSampleRateType, int bitSize, int channelType)
{
	char sound_format_code = formatCode;
	char sound_sample_rate = iSampleRateType;
	char sound_bit_size = bitSize;
	char sound_channel_type = channelType;

	// for audio frame, there is 1 or 2 bytes header:
	//      1bytes, SoundFormat|SoundRate|SoundSize|SoundType
	//      1bytes, AACPacketType for SoundFormat == 10, 0 is sequence header.
	// 根据 FLV 标准，[AACDecoderSpecificInfo] 
	// 第1个字节高 4 位(1010),10(0x0A)代表音频数据编码类型为 AAC，
	// 接下来 2 位(11)表示采样率为 44kHz，
	// 接下来 1 位(1)表示采样点位数 16bit，
	// 最低 1 位 (1) 表示双声道。
	u_int8_t audio_header = 0;
	audio_header |= (sound_format_code << 4) & 0xf0;   // 音频编码类型,AAC,MP3等
	audio_header |= (sound_sample_rate << 2) & 0x0c;   // 采样频率  
	audio_header |= (sound_bit_size << 1) & 0x02;   // 采样点位
	audio_header |= sound_channel_type & 0x01;   // 声道(1为立体声)

	return audio_header;

}

void LibRtmpSession::MakeAudioSpecificConfig(char* pConfig, int aactype, int sampleRate, int channels) {
	unsigned short result = 0;

	//ASC FLAG: xxxx xaaa aooo o111
	result += aactype;
	result = result << 4;
	result += sampleRate;
	result = result << 4;
	result += channels;
	result = result << 3;
	int size = sizeof(result);

	if ((aactype == RTMPAacObjectTypeAacHE) || (aactype == RTMPAacObjectTypeAacHEV2)) {
		result |= 0x01;
	}
	memcpy(pConfig, &result, size);

	unsigned char low, high;
	low = pConfig[0];
	high = pConfig[1];
	pConfig[0] = high;
	pConfig[1] = low;

}

void LibRtmpSession::MakeAudioSpecificConfigEx(char* pConfig, unsigned char audioObjectType, unsigned char samplingFrequencyIndex, unsigned char channels) {

	std::string sh = "";

	char ch = 0;
	// @see aac-mp4a-format-ISO_IEC_14496-3+2001.pdf
	// AudioSpecificConfig (), page 33
	// 1.6.2.1 AudioSpecificConfig

	// audioObjectType; 5 bslbf
	ch = (audioObjectType << 3) & 0xf8;// 3bits left.

	// samplingFrequencyIndex; 4 bslbf
	ch |= (samplingFrequencyIndex >> 1) & 0x07;
	sh += ch;
	ch = (samplingFrequencyIndex << 7) & 0x80; // 7bits left.

	// channelConfiguration; 4 bslbf
	ch |= (channels << 3) & 0x78; // 3bits left.

	// GASpecificConfig(), page 451
	// 4.4.1 Decoder configuration (GASpecificConfig)
	// frameLengthFlag; 1 bslbf
	// dependsOnCoreCoder; 1 bslbf
	// extensionFlag; 1 bslbf

	if ((audioObjectType == RTMPAacObjectTypeAacHE) || (audioObjectType == RTMPAacObjectTypeAacHEV2)) {
		ch |= 0x01;
	}
	sh += ch;
	memcpy(pConfig, sh.c_str(), sh.size());
}

int LibRtmpSession::SendAudioSpecificConfig(unsigned short usASCFlag)
{
	int iSpeclen = 2;
	unsigned char szAudioSpecData[2];

	usASCFlag = (usASCFlag >> 8) | (usASCFlag << 8);
	memcpy(szAudioSpecData, &usASCFlag, sizeof(usASCFlag));

	unsigned char* body;
	int len;
	len = iSpeclen + 2;

	int rtmpLength = len;
	RTMPPacket rtmp_pack;
	PILI_RTMPPacket_Reset(&rtmp_pack);
	PILI_RTMPPacket_Alloc(&rtmp_pack, rtmpLength);

	body = (unsigned char *)rtmp_pack.m_body;
	body[0] = m_cFlvAudioHead0;
	body[1] = RTMPCodecAudioTypeSequenceHeader;

	memcpy(&body[2], szAudioSpecData, sizeof(szAudioSpecData));

	rtmp_pack.m_packetType = RTMP_PACKET_TYPE_AUDIO;
	rtmp_pack.m_nBodySize = len;
	rtmp_pack.m_nChannel = 0x04;
	rtmp_pack.m_nTimeStamp = 0;
	rtmp_pack.m_hasAbsTimestamp = 0;
	rtmp_pack.m_headerType = RTMP_PACKET_SIZE_LARGE;

	if (m_pRtmp)
		rtmp_pack.m_nInfoField2 = m_pRtmp->m_stream_id;

	int nRet = RtmpPacketSend(&rtmp_pack);

	PILI_RTMPPacket_Free(&rtmp_pack);

	LOGI("SendAudioSpecificConfig: %02x %02x %02x %02x, return %d", body[0], body[1], body[2], body[3], nRet);
	m_iASCSentFlag = 1;
	return nRet;
}

int LibRtmpSession::SendAudioSpecificConfig(unsigned char audioObjectType, unsigned char samplingFrequencyIndex, unsigned char channels)
{
	char* szAudioSpecData;
	int iSpeclen = 0;

	if ((audioObjectType == RTMPAacObjectTypeAacHE) || (audioObjectType == RTMPAacObjectTypeAacHEV2)) {
		iSpeclen = 4;
	}
	else {
		iSpeclen = 2;
	}
	szAudioSpecData = (char*)malloc(iSpeclen);
	memset(szAudioSpecData, 0, iSpeclen);
	MakeAudioSpecificConfigEx(szAudioSpecData, audioObjectType, samplingFrequencyIndex/*getSampleRateType(sampleRate)*/, channels);

	m_iAacType = audioObjectType;
	m_iSampleRate = samplingFrequencyIndex;
	m_iChannels = channels;


	unsigned char* body;
	int len;
	len = iSpeclen + 2;

	int rtmpLength = len;
	RTMPPacket rtmp_pack;
	PILI_RTMPPacket_Reset(&rtmp_pack);
	PILI_RTMPPacket_Alloc(&rtmp_pack, rtmpLength);

	// FLV的格式进行封包AAC,前两个字节被称为 [AACDecoderSpecificInfo]
	body = (unsigned char *)rtmp_pack.m_body;
	body[0] = m_cFlvAudioHead0;
	body[1] = RTMPCodecAudioTypeSequenceHeader;

	memcpy(&body[2], szAudioSpecData, iSpeclen);
	free(szAudioSpecData);

	rtmp_pack.m_packetType = RTMP_PACKET_TYPE_AUDIO;
	rtmp_pack.m_nBodySize = len;
	rtmp_pack.m_nChannel = 0x04;
	rtmp_pack.m_nTimeStamp = 0;
	rtmp_pack.m_hasAbsTimestamp = 0;
	rtmp_pack.m_headerType = RTMP_PACKET_SIZE_LARGE;

	if (m_pRtmp)
		rtmp_pack.m_nInfoField2 = m_pRtmp->m_stream_id;

	int nRet = RtmpPacketSend(&rtmp_pack);

	PILI_RTMPPacket_Free(&rtmp_pack);

	LOGI("SendAudioSpecificConfig: %02x %02x %02x %02x, return %d", body[0], body[1], body[2], body[3], nRet);
	m_iASCSentFlag = 1;
	return nRet;
}

int LibRtmpSession::RtmpPacketSend(RTMPPacket* packet)
{
	if (!m_iConnectFlag)
	{
		return ERROR_SOCKET_CONNECT;
	}
	RTMPError err = { 0 };
	int iRet = PILI_RTMP_SendPacket(m_pRtmp, packet, 0, &err);

	return (iRet != TRUE) ? ERROR_SOCKET_WRITE : 0;
}

int LibRtmpSession::SendPacket(unsigned int nPacketType, unsigned char *data, unsigned int size, time_t nTimestamp)
{
	int rtmpLength = size;
	RTMPPacket rtmp_pack;
	PILI_RTMPPacket_Reset(&rtmp_pack);
	PILI_RTMPPacket_Alloc(&rtmp_pack, rtmpLength);

	rtmp_pack.m_nBodySize = size;
	memcpy(rtmp_pack.m_body, data, size);
	rtmp_pack.m_hasAbsTimestamp = 0;
	rtmp_pack.m_packetType = nPacketType;

	if (m_pRtmp)
		rtmp_pack.m_nInfoField2 = m_pRtmp->m_stream_id;

	rtmp_pack.m_nChannel = 0x04;

	rtmp_pack.m_headerType = RTMP_PACKET_SIZE_LARGE;
	if (RTMP_PACKET_TYPE_AUDIO == nPacketType && size != 4)
	{
		rtmp_pack.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	}
	rtmp_pack.m_nTimeStamp = (uint32_t)nTimestamp;

	int nRet = RtmpPacketSend(&rtmp_pack);

	PILI_RTMPPacket_Free(&rtmp_pack);
	return nRet;
}

int LibRtmpSession::SendH264VideoSequenceHeader(unsigned char *pps, unsigned short pps_len, unsigned char * sps, unsigned short sps_len)
{
	unsigned char * body = NULL;
	int iIndex = 0;

	int rtmpLength = 1025;
	RTMPPacket rtmp_pack;
	PILI_RTMPPacket_Reset(&rtmp_pack);
	PILI_RTMPPacket_Alloc(&rtmp_pack, rtmpLength);

	body = (unsigned char *)rtmp_pack.m_body;

	body[iIndex++] = 0x17;
	body[iIndex++] = 0x00;

	body[iIndex++] = 0x00;
	body[iIndex++] = 0x00;
	body[iIndex++] = 0x00;

	body[iIndex++] = 0x01;
	body[iIndex++] = sps[1];
	body[iIndex++] = sps[2];
	body[iIndex++] = sps[3];
	body[iIndex++] = 0xff;

	/*sps*/
	body[iIndex++] = 0xe1;
	body[iIndex++] = (sps_len >> 8) & 0xff;
	body[iIndex++] = sps_len & 0xff;
	memcpy(&body[iIndex], sps, sps_len);
	iIndex += sps_len;

	/*pps*/
	body[iIndex++] = 0x01;
	body[iIndex++] = (pps_len >> 8) & 0xff;
	body[iIndex++] = (pps_len) & 0xff;
	memcpy(&body[iIndex], pps, pps_len);
	iIndex += pps_len;

	rtmp_pack.m_packetType = RTMP_PACKET_TYPE_VIDEO;
	rtmp_pack.m_nBodySize = iIndex;
	rtmp_pack.m_nChannel = 0x04;
	rtmp_pack.m_nTimeStamp = 0;
	rtmp_pack.m_hasAbsTimestamp = 0;
	rtmp_pack.m_headerType = RTMP_PACKET_SIZE_MEDIUM;

	if (m_pRtmp)
		rtmp_pack.m_nInfoField2 = m_pRtmp->m_stream_id;

	int nRet = RtmpPacketSend(&rtmp_pack);
	if (nRet == ERROR_SUCCESS)
	{
		if (m_pMetaData->Pps != pps)
		{
			m_pMetaData->Pps = (unsigned char*)malloc(pps_len);
			memcpy(m_pMetaData->Pps, pps, pps_len);
		}
		if (m_pMetaData->Sps != sps)
		{
			m_pMetaData->Sps = (unsigned char*)malloc(sps_len);
			memcpy(m_pMetaData->Sps, sps, sps_len);
		}
		m_iMetaDataFlag = 1;
	}
	PILI_RTMPPacket_Free(&rtmp_pack);

	return nRet;
}


int LibRtmpSession::SendH265VideoSequenceHeader(unsigned char *pps, unsigned short pps_len, unsigned char * sps, unsigned short sps_len, unsigned char * vps, unsigned short vps_len)
{
	unsigned char * body = NULL;
	int iIndex = 0;

	int rtmpLength = 4096;
	RTMPPacket rtmp_pack;
	PILI_RTMPPacket_Reset(&rtmp_pack);
	PILI_RTMPPacket_Alloc(&rtmp_pack, rtmpLength);

	body = (unsigned char *)rtmp_pack.m_body;

	body[iIndex++] = 0x1C;
	body[iIndex++] = 0x00;// 0: AVC sequence header; 1: AVC NALU; 2: AVC end of sequence

	body[iIndex++] = 0x00;
	body[iIndex++] = 0x00;
	body[iIndex++] = 0x00;

	int len = get_sequence_header(&body[iIndex], pps, pps_len, sps, sps_len, vps, vps_len);
	if (len < 0)
	{
		return -1;
	}
	iIndex += len;
	rtmp_pack.m_packetType = RTMP_PACKET_TYPE_VIDEO;
	rtmp_pack.m_nBodySize = iIndex;
	rtmp_pack.m_nChannel = 0x04;
	rtmp_pack.m_nTimeStamp = 0;
	rtmp_pack.m_hasAbsTimestamp = 0;
	rtmp_pack.m_headerType = RTMP_PACKET_SIZE_MEDIUM;

	if (m_pRtmp)
		rtmp_pack.m_nInfoField2 = m_pRtmp->m_stream_id;

	int nRet = RtmpPacketSend(&rtmp_pack);
	if (nRet == ERROR_SUCCESS)
	{
		if (m_pMetaData->Pps != pps)
		{
			m_pMetaData->Pps = (unsigned char*)malloc(pps_len);
			memcpy(m_pMetaData->Pps, pps, pps_len);
		}
		if (m_pMetaData->Sps != sps)
		{
			m_pMetaData->Sps = (unsigned char*)malloc(sps_len);
			memcpy(m_pMetaData->Sps, sps, sps_len);
		}
		if (m_pMetaData->Vps != vps)
		{
			m_pMetaData->Vps = (unsigned char*)malloc(vps_len);
			memcpy(m_pMetaData->Vps, vps, vps_len);
		}
		m_iMetaDataFlag = 1;
	}
	PILI_RTMPPacket_Free(&rtmp_pack);

	return nRet;
}

int LibRtmpSession::SendH264Packet(unsigned char *data, unsigned int size, int bIsKeyFrame, time_t nTimeStamp)
{
	if (data == NULL && size < 11)
	{
		return ERROR_INVALIDPARAM;
	}

	unsigned char *body = (unsigned char*)malloc(size + 9);
	memset(body, 0, size + 9);

	int i = 0;
	if (bIsKeyFrame)
	{
		body[i++] = 0x17;// 1:Iframe  7:AVC
	}
	else
	{
		body[i++] = 0x27;// 2:Pframe  7:AVC
	}
	body[i++] = 0x01;// AVC NALU
	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	//     body[i++] = (size >> 24) & 0xFF;
	//     body[i++] = (size >> 16) & 0xFF;
	//     body[i++] = (size >> 8) & 0xFF;
	//     body[i++] = size & 0xFF;
	memcpy(&body[i], data, size);

	int iRet = SendPacket(RTMP_PACKET_TYPE_VIDEO, body, i + size, nTimeStamp);

	free(body);

	return iRet;
}


int LibRtmpSession::SendH265Packet(unsigned char *data, unsigned int size, int bIsKeyFrame, time_t nTimeStamp)
{
	if (data == NULL && size < 11)
	{
		return ERROR_INVALIDPARAM;
	}

	unsigned char *body = (unsigned char*)malloc(size + 9);
	memset(body, 0, size + 9);

	int i = 0;
	if (bIsKeyFrame)
	{
		body[i++] = 0x1C;// 1:Iframe  12:HEVC
	}
	else
	{
		body[i++] = 0x2C;// 2:Pframe  7:HEVC
	}
	body[i++] = 0x01;// HEVC NALU
	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	memcpy(&body[i], data, size);

	int iRet = SendPacket(RTMP_PACKET_TYPE_VIDEO, body, i + size, nTimeStamp);

	free(body);

	return iRet;
}

int LibRtmpSession::SendAACRawData(unsigned char* buf, int size, unsigned int timeStamp)
{
	if (m_pRtmp == NULL)
		return -1;

	if (size <= 0)
	{
		return -2;
	}

	unsigned char * body;

	int rtmpLength = size + 2;
	RTMPPacket rtmp_pack;
	PILI_RTMPPacket_Reset(&rtmp_pack);
	PILI_RTMPPacket_Alloc(&rtmp_pack, rtmpLength);

	body = (unsigned char *)rtmp_pack.m_body;

	/*AF 01 + AAC RAW data*/
	body[0] = m_cFlvAudioHead0;
	body[1] = RTMPCodecAudioTypeRawData;
	memcpy(&body[2], buf, size);

	rtmp_pack.m_packetType = RTMP_PACKET_TYPE_AUDIO;
	rtmp_pack.m_nBodySize = rtmpLength;
	rtmp_pack.m_nChannel = 0x04;
	rtmp_pack.m_nTimeStamp = timeStamp;
	rtmp_pack.m_hasAbsTimestamp = 0;
	rtmp_pack.m_headerType = RTMP_PACKET_SIZE_LARGE;

	if (m_pRtmp)
		rtmp_pack.m_nInfoField2 = m_pRtmp->m_stream_id;

	int nRet = RtmpPacketSend(&rtmp_pack);

	PILI_RTMPPacket_Free(&rtmp_pack);
	return nRet;
}

int LibRtmpSession::SendAudioRawData(unsigned char* pBuff, int len, unsigned int ts) {
	int rtmpLength = len;
	RTMPPacket* pRtmp_pack = (RTMPPacket*)malloc(sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE + rtmpLength + PILI_RTMP_DATA_RESERVE_SIZE);
	memset(pRtmp_pack, 0, sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE + rtmpLength + PILI_RTMP_DATA_RESERVE_SIZE);

	pRtmp_pack->m_body = ((char*)pRtmp_pack) + sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE + PILI_RTMP_DATA_RESERVE_SIZE / 2;

	/*AAC RAW data*/
	memcpy(pRtmp_pack->m_body, pBuff, rtmpLength);

	pRtmp_pack->m_packetType = RTMP_PACKET_TYPE_AUDIO;
	pRtmp_pack->m_nBodySize = rtmpLength;
	pRtmp_pack->m_nChannel = 0x04;
	pRtmp_pack->m_nTimeStamp = ts;
	pRtmp_pack->m_hasAbsTimestamp = 0;
	pRtmp_pack->m_headerType = RTMP_PACKET_SIZE_LARGE;

	if (m_pRtmp)
		pRtmp_pack->m_nInfoField2 = m_pRtmp->m_stream_id;

	int nRet = RtmpPacketSend(pRtmp_pack);

	PILI_RTMPPacket_Free(pRtmp_pack);
	free(pRtmp_pack);
	return nRet;
}

int LibRtmpSession::SendAudioAdtsAAC(unsigned char* pBuff, int len, time_t nTimeStamp)
{
	if (!m_iConnectFlag)
	{
		return ERROR_SOCKET_CONNECT;
	}
	// for aac, the frame must be ADTS format.
	if (!srs_aac_is_adts((char*)pBuff, len)) {
		return ERROR_AAC_REQUIRED_ADTS;
	}

	int nHeadLen = 7;

	int iRet = 0;

	if (!m_iASCSentFlag)
	{
		adts_fixed_header adtsheader = { 0 };
		m_strAdts.assign((char*)pBuff, nHeadLen);
		get_fixed_header((unsigned char*)&m_strAdts[0], &adtsheader);

		int sampling_frequency_index = adtsheader.sampling_frequency_index;
		int iSampleRateType = getFlvAudioHeadSampleRateType(sampling_frequency_index);
		int aacObjectType = flv_codec_aac_ts2rtmp(adtsheader.profile);

		m_cFlvAudioHead0 = MakeFlvAudioHead(AudioCodec_AAC,
			iSampleRateType,
			RTMPCodecAudioSampleSize16bit/*0-8bit,1-16bit,adtsheader.private_bit*/,
			(adtsheader.channel_configuration == 1) ? RTMPCodecAudioSoundTypeMono : RTMPCodecAudioSoundTypeStereo);

		iRet = SendAudioSpecificConfig(aacObjectType, sampling_frequency_index, adtsheader.channel_configuration);
	}

	if (len - nHeadLen > 0)
	{
		if (m_uiStartTimestamp == 0) {
			m_uiStartTimestamp = PILI_RTMP_GetTime();
		}
		else {
			m_uiAudioDTS = PILI_RTMP_GetTime() - m_uiStartTimestamp;
		}

		// 去掉前7个无用字节,aac编码带有adts头，则需要去掉头7个字节
		iRet = SendAACRawData((unsigned char*)(pBuff + nHeadLen), len - nHeadLen, m_uiAudioDTS);
		//LOGI("SendAudioData: SendAACData size=%u, DTS=%u return %d", pAdtsItems[i].size-7, _uiAudioDTS, iRet);
	}

	return iRet;
}


// 发送一个音频帧
int LibRtmpSession::SendAudioData(unsigned char* buff, int length, time_t timestamp, eAudioCodecType nAudioCodecType)
{
	if (!buff || length <= 0)
	{
		return ERROR_INVALIDPARAM;
	}

	if (AudioCodec_AAC == nAudioCodecType)
	{
		return SendAudioAdtsAAC(buff, length, timestamp);
	}

	if (!m_pAACEncoder)
	{
		if (!m_bInitAudioParam)
		{
			return ERROR_AUDIO_CODEC_NOT_SUPPORT;
		}
		switch (nAudioCodecType)
		{
		case AudioCodec_Law_G711U:
		case AudioCodec_Law_G711A:
		case AudioCodec_Law_G726:
		case AudioCodec_Law_PCM16:
			m_initParam.ucAudioCodec = nAudioCodecType;
			break;
		default:
			return ERROR_AUDIO_CODEC_NOT_SUPPORT;
			break;
		}

		m_pAACEncoder = new CAACEncoder();
		m_pAACEncoder->Init(&m_initParam);

		size_t nPcmBuf = 5 * length;
		if (m_strAACBuf.size() < nPcmBuf)
		{
			m_strAACBuf.resize(nPcmBuf);
		}
	}

	if (m_pAACEncoder)
	{
		size_t nPcmBuf = 5 * length;
		if (m_strAACBuf.size() < nPcmBuf)
		{
			m_strAACBuf.resize(nPcmBuf);
		}

		unsigned char* pbAACBuffer = (unsigned char*)&m_strAACBuf[0];
		unsigned int out_len = m_strAACBuf.size();
		// G7XX转为AAC，RTMP默认只支持AAC
		m_pAACEncoder->Encode(buff, length, pbAACBuffer, &out_len);
		if (out_len <= 0)
		{
			return ERROR_SUCCESS;
		}

		return SendAudioAdtsAAC(pbAACBuffer, out_len, timestamp);
	}

	return ERROR_SUCCESS;
}

int LibRtmpSession::SeparateNalus(unsigned char* pBuff, int len)
{
	int cnt = 0;
	int i = 0;
	if (!pBuff || len <= 3 || len > 100 * 1024 * 1024)
	{
		return 0;
	}
	for (i = 0; i < len; ++i)
	{
		if (pBuff[i] == 0)
		{
			//00 00 00 01
			if ((i + 3 < len) && (pBuff[i + 1] == 0) && (pBuff[i + 2] == 0) && (pBuff[i + 3] == 1))
			{
				m_pNaluItems[cnt].data = (char*)(pBuff + i + 4);
				m_pNaluItems[cnt].headlen = 4;
				i += 3;
				cnt++;
			}
			//00 00 01
			else if ((i + 2 < len) && (pBuff[i + 1] == 0) && (pBuff[i + 2] == 1))
			{
				m_pNaluItems[cnt].data = (char*)(pBuff + i + 3);
				m_pNaluItems[cnt].headlen = 3;
				i += 2;
				cnt++;
			}
			if (cnt >= DATA_ITEMS_MAX_COUNT)
			{
				break;
			}
		}
	}

	for (i = 0; i < cnt; ++i)
	{
		if (i < cnt - 1)
		{
			m_pNaluItems[i].size = m_pNaluItems[i + 1].data - m_pNaluItems[i].data - m_pNaluItems[i + 1].headlen;
		}
		else
		{
			m_pNaluItems[i].size = (char*)(pBuff + len) - m_pNaluItems[i].data;
		}
	}
	return cnt;
}

int LibRtmpSession::SendVideoRawData(unsigned char* buf, int videodatalen, unsigned int ts) {
	int rtmpLength = videodatalen;

	RTMPPacket* pRtmp_pack = (RTMPPacket*)malloc(sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE + rtmpLength + PILI_RTMP_DATA_RESERVE_SIZE);
	memset(pRtmp_pack, 0, sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE + rtmpLength + PILI_RTMP_DATA_RESERVE_SIZE);

	pRtmp_pack->m_nBodySize = videodatalen;
	pRtmp_pack->m_hasAbsTimestamp = 0;
	pRtmp_pack->m_packetType = RTMP_PACKET_TYPE_VIDEO;

	if (m_pRtmp)
		pRtmp_pack->m_nInfoField2 = m_pRtmp->m_stream_id;

	pRtmp_pack->m_nChannel = 0x04;

	pRtmp_pack->m_headerType = RTMP_PACKET_SIZE_LARGE;
	pRtmp_pack->m_nTimeStamp = ts;
	pRtmp_pack->m_body = ((char*)pRtmp_pack) + sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE + PILI_RTMP_DATA_RESERVE_SIZE / 2;
	memcpy(pRtmp_pack->m_body, buf, videodatalen);

	int nRet = RtmpPacketSend(pRtmp_pack);

	PILI_RTMPPacket_Free(pRtmp_pack);
	free(pRtmp_pack);
	return nRet;
}

int LibRtmpSession::GetASCSentFlag() {
	/*
		_iSendASCCount++;
		if(_iSendASCCount > 300){
			_iSendASCCount = 0;
			_iASCSentFlag = 0;
		}
		*/
	return m_iASCSentFlag;
}

int LibRtmpSession::SendH264VideoData(unsigned char* buf, int videodatalen, time_t nTimeStamp, bool bIsKeyFrame)
{
	if (!m_iConnectFlag)
	{
		return ERROR_SOCKET_CONNECT;
	}
	if (!buf || videodatalen <= 0 || videodatalen > 100 * 1024 * 1024)
	{
		return ERROR_INVALIDPARAM;
	}

	int itemscnt = SeparateNalus(buf, videodatalen);
	int iRet = 0;

	if (bIsKeyFrame && ((!m_pMetaData->Sps) || (!m_pMetaData->Pps)))
	{
		bool bHaveSps = false;
		bool bHavePps = false;

		if (itemscnt > 0)
		{
			int i = 0;
			for (i = 0; i < itemscnt; i++)
			{
				bool bSpsFlag = ((m_pNaluItems[i].data[0] & 0x1f) == 7);

				if (bSpsFlag && (!m_pMetaData->Sps))
				{
					bHaveSps = true;
					m_pMetaData->nSpsLen = m_pNaluItems[i].size;
					m_pMetaData->Sps = (unsigned char*)malloc(m_pMetaData->nSpsLen);
					memcpy(m_pMetaData->Sps, m_pNaluItems[i].data, m_pMetaData->nSpsLen);

					int* Width = &m_iWidth;
					int* Height = &m_iHeight;
					int* Fps = &m_iFps;
					h264_decode_sps(m_pMetaData->Sps, m_pMetaData->nSpsLen, &Width, &Height, &Fps);

					m_pMetaData->nWidth = m_iWidth;
					m_pMetaData->nHeight = m_iHeight;

					if (m_iFps)
						m_pMetaData->nFrameRate = m_iFps;
					else
						m_pMetaData->nFrameRate = 25;
				}

				bool bPpsFlag = ((m_pNaluItems[i].data[0] & 0x1f) == 8);

				if (bPpsFlag && (!m_pMetaData->Pps))
				{
					bHavePps = true;
					m_pMetaData->nPpsLen = m_pNaluItems[i].size;
					m_pMetaData->Pps = (unsigned char*)malloc(m_pMetaData->nPpsLen);
					memcpy(m_pMetaData->Pps, m_pNaluItems[i].data, m_pNaluItems[i].size);
				}
			}
		}

		if (bHaveSps && bHavePps && (m_pMetaData->Sps) && (m_pMetaData->Pps))
		{
			iRet = SendH264VideoSequenceHeader(m_pMetaData->Pps, m_pMetaData->nPpsLen, m_pMetaData->Sps, m_pMetaData->nSpsLen);
		}
	}
	if (!m_iMetaDataFlag)
	{
		return ERROR_H264_DROP_BEFORE_SPS_PPS;
	}
	if (itemscnt <= 0)
	{
		return ERROR_INVALIDPARAM;
	}

	int isKey = bIsKeyFrame;
	unsigned int iSendSize = 0;
	int iValidNaluCount = 0;
	unsigned char* pSendBuffer = (unsigned char*)malloc(videodatalen);
	memset(pSendBuffer, 0, videodatalen);

	for (int i = 0; i < itemscnt; ++i)
	{
		if (m_pNaluItems[i].size == 0)
			continue;

		unsigned char ucType = m_pNaluItems[i].data[0];

		if ((ucType & 0x1f) == 8)//pps
			continue;

		if ((ucType & 0x1f) == 7)//sps
			continue;

		if (isKey == 0)
			isKey = ((ucType & 0x1f) == 0x05) ? 1 : 0;

		unsigned int iNaluSize = m_pNaluItems[i].size;

		if (iNaluSize > 100 * 1024 * 1024)
		{
			return ERROR_INVALIDPARAM;
		}

		pSendBuffer[iSendSize++] = (iNaluSize >> 24) & 0xFF;
		pSendBuffer[iSendSize++] = (iNaluSize >> 16) & 0xFF;
		pSendBuffer[iSendSize++] = (iNaluSize >> 8) & 0xFF;
		pSendBuffer[iSendSize++] = iNaluSize & 0xFF;

		memcpy(pSendBuffer + iSendSize, m_pNaluItems[i].data, iNaluSize);
		iSendSize += iNaluSize;
		iValidNaluCount++;
	}

	unsigned int uiVideoTimestamp = 0;
	if (m_uiStartTimestamp == 0)
	{
		m_uiStartTimestamp = PILI_RTMP_GetTime();
	}
	else
	{
		uiVideoTimestamp = PILI_RTMP_GetTime() - m_uiStartTimestamp;
	}

	iRet = SendH264Packet(pSendBuffer, iSendSize, isKey, uiVideoTimestamp);
	if (isKey != 0)
	{
		LOGI("SCREEN_CONTENT_REAL_TIME(%d) I Frame return %d, 0x%02x, timestamp=%u, audio_ts=%d", iSendSize, iRet, pSendBuffer[0], uiVideoTimestamp, m_uiAudioDTS);
	}

	free(pSendBuffer);

	return iRet;
}


int LibRtmpSession::SendH265VideoData(unsigned char* buf, int videodatalen, time_t nTimeStamp, bool bIsKeyFrame)
{
	if (!m_iConnectFlag)
	{
		return ERROR_SOCKET_CONNECT;
	}
	if (!buf || videodatalen <= 0 || videodatalen > 100 * 1024 * 1024)
	{
		return ERROR_INVALIDPARAM;
	}

	int itemscnt = SeparateNalus(buf, videodatalen);
	int iRet = 0;

	if (bIsKeyFrame && ((!m_pMetaData->Sps) || (!m_pMetaData->Pps) || (!m_pMetaData->Vps)))
	{
		bool bHaveSps = false;
		bool bHavePps = false;
		bool bHaveVps = false;

		if (itemscnt > 0)
		{
			int i = 0;
			for (i = 0; i < itemscnt; i++)
			{

				bool bSpsFlag = (((m_pNaluItems[i].data[0] & 0x7E) >> 1) == 0x21);

				if (bSpsFlag && (!m_pMetaData->Sps))
				{
					bHaveSps = true;
					m_pMetaData->nSpsLen = m_pNaluItems[i].size;
					m_pMetaData->Sps = (unsigned char*)malloc(m_pMetaData->nSpsLen);
					memcpy(m_pMetaData->Sps, m_pNaluItems[i].data, m_pMetaData->nSpsLen);

// 					int* Width = &m_iWidth;
// 					int* Height = &m_iHeight;
// 					int* Fps = &m_iFps;
// 					h264_decode_sps(m_pMetaData->Sps, m_pMetaData->nSpsLen, &Width, &Height, &Fps);

					m_pMetaData->nWidth = 1280;//m_iWidth;
					m_pMetaData->nHeight = 1920;//m_iHeight;

// 					if (m_iFps)
// 						m_pMetaData->nFrameRate = m_iFps;
// 					else
						m_pMetaData->nFrameRate = 25;
						continue;

				}

				bool bPpsFlag = (((m_pNaluItems[i].data[0] & 0x7E) >> 1) == 0x22);

				if (bPpsFlag && (!m_pMetaData->Pps))
				{
					bHavePps = true;
					m_pMetaData->nPpsLen = m_pNaluItems[i].size;
					m_pMetaData->Pps = (unsigned char*)malloc(m_pMetaData->nPpsLen);
					memcpy(m_pMetaData->Pps, m_pNaluItems[i].data, m_pNaluItems[i].size);
					continue;
				}

				bool bVpsFlag = (((m_pNaluItems[i].data[0] & 0x7E) >> 1) == 0x20);

				if (bVpsFlag && (!m_pMetaData->Vps))
				{
					bHaveVps = true;
					m_pMetaData->nVpsLen = m_pNaluItems[i].size;
					m_pMetaData->Vps = (unsigned char*)malloc(m_pMetaData->nVpsLen);
					memcpy(m_pMetaData->Vps, m_pNaluItems[i].data, m_pNaluItems[i].size);
					continue;
				}

				if (bHaveSps && bHavePps && bHaveVps)
				{
					break;
				}
			}
		}

		if (bHaveSps && bHavePps && bHaveVps && (m_pMetaData->Sps) && (m_pMetaData->Pps) && (m_pMetaData->Vps))
		{
			iRet = SendH265VideoSequenceHeader(
				m_pMetaData->Pps, m_pMetaData->nPpsLen, 
				m_pMetaData->Sps, m_pMetaData->nSpsLen, 
				m_pMetaData->Vps, m_pMetaData->nVpsLen);
			if (iRet < 0)
			{
				return ERROR_H264_DROP_BEFORE_SPS_PPS;
			}
		}
	}
	if (!m_iMetaDataFlag)
	{
		return ERROR_H264_DROP_BEFORE_SPS_PPS;
	}
	if (itemscnt <= 0)
	{
		return ERROR_INVALIDPARAM;
	}

	int isKey = bIsKeyFrame;
	unsigned int iSendSize = 0;
	int iValidNaluCount = 0;
	unsigned char* pSendBuffer = (unsigned char*)malloc(videodatalen);
	memset(pSendBuffer, 0, videodatalen);

	for (int i = 0; i < itemscnt; ++i)
	{
		if (m_pNaluItems[i].size == 0)
			continue;

		unsigned char ucType = m_pNaluItems[i].data[0];

		if (((m_pNaluItems[i].data[0] & 0x7E) >> 1) == 0x22)//pps
			continue;

		if (((m_pNaluItems[i].data[0] & 0x7E) >> 1) == 0x21)//sps
			continue;

		if (((m_pNaluItems[i].data[0] & 0x7E) >> 1) == 0x20)
			continue;

		if (isKey == 0)
			isKey = (((m_pNaluItems[i].data[0] & 0x7E) >> 1) == 0x13) ? 1 : 0;

		unsigned int iNaluSize = m_pNaluItems[i].size;

		if (iNaluSize > 100 * 1024 * 1024)
		{
			return ERROR_INVALIDPARAM;
		}

		pSendBuffer[iSendSize++] = (iNaluSize >> 24) & 0xFF;
		pSendBuffer[iSendSize++] = (iNaluSize >> 16) & 0xFF;
		pSendBuffer[iSendSize++] = (iNaluSize >> 8) & 0xFF;
		pSendBuffer[iSendSize++] = iNaluSize & 0xFF;

		memcpy(pSendBuffer + iSendSize, m_pNaluItems[i].data, iNaluSize);
		iSendSize += iNaluSize;
		iValidNaluCount++;
	}

	unsigned int uiVideoTimestamp = 0;
	if (m_uiStartTimestamp == 0)
	{
		m_uiStartTimestamp = PILI_RTMP_GetTime();
	}
	else
	{
		uiVideoTimestamp = PILI_RTMP_GetTime() - m_uiStartTimestamp;
	}

	iRet = SendH265Packet(pSendBuffer, iSendSize, isKey, uiVideoTimestamp);
	if (isKey != 0)
	{
		LOGI("SCREEN_CONTENT_REAL_TIME(%d) I Frame return %d, 0x%02x, timestamp=%u, audio_ts=%d", iSendSize, iRet, pSendBuffer[0], uiVideoTimestamp, m_uiAudioDTS);
	}

	free(pSendBuffer);

	return iRet;
}

int LibRtmpSession::ReadDataEx(char* buff, int* size)
{
	int iRet = 0;
	if (m_pRtmp == NULL)
	{
		return ERROR_NETWORK_DISCONN;
	}

	int iSize = *size;
	*size = 0;

	iRet = PILI_RTMP_Read(m_pRtmp, (char*)buff, iSize);
	if (iRet > 0)
	{
		*size = iRet;
		return 0;
	}
	else
	{
		return ERROR_SOCKET_READ;
	}
}

int LibRtmpSession::ReadData(unsigned char* buff, int iSize)
{
	int iRet = 0;
	if (IsConnected())
	{
		return ERROR_NETWORK_DISCONN;
	}

	iRet = PILI_RTMP_Read(m_pRtmp, (char*)buff, iSize);
	return iRet;
}

int LibRtmpSession::GetReadStatus()
{
	int iStatus = m_pRtmp->m_read.status;

	return iStatus;
}

int LibRtmpSession::GetAACType()
{
	return m_iAacType;
}

int LibRtmpSession::GetSampleRate()
{
	return m_iSampleRate;
}
int LibRtmpSession::GetChannels()
{
	return m_iChannels;
}

double LibRtmpSession::GetDuration()
{
	if (!IsConnected())
	{
		return 0;
	}
	if (m_pRtmp->Link.lFlags & RTMP_LF_LIVE)
	{
		return 0;
	}
	return PILI_RTMP_GetDuration(m_pRtmp);
}

int LibRtmpSession::Pause(int DoPause)
{
	if (!IsConnected())
	{
		return ERROR_NETWORK_DISCONN;
	}
	if (m_pRtmp->Link.lFlags & RTMP_LF_LIVE)
	{
		return 0;
	}
	RTMPError err = { 0 };
	return PILI_RTMP_Pause(m_pRtmp, DoPause, &err);
}

int LibRtmpSession::Seek(int timestemp)
{
	if (!IsConnected())
	{
		return ERROR_NETWORK_DISCONN;
	}
	if (m_pRtmp->Link.lFlags & RTMP_LF_LIVE)
	{
		return 0;
	}
	RTMPError err = { 0 };
	return PILI_RTMP_SendSeek(m_pRtmp, timestemp & 0xffffffff, &err);
}


int  LibRtmpSession::ReadPacket(char* buff, int* buflen, time_t* timestamp, char* type)
{
	if (!IsConnected())
	{
		return ERROR_NETWORK_DISCONN;
	}
	if (!buff || !buflen)
	{
		return ERROR_INVALIDPARAM;
	}
	int nBuffSize = *buflen;
	*buflen = 0;
	*type = 0;
	*timestamp = 0;

	int rtnGetNextMediaPacket = 0, ret = RTMP_READ_EOF;
	RTMPPacket packet = { 0 };
	unsigned int size;
	uint32_t nTimeStamp = 0;


	//获取一个packet
	rtnGetNextMediaPacket = PILI_RTMP_GetNextMediaPacket(m_pRtmp, &packet);
	while (rtnGetNextMediaPacket)
	{
		char *packetBody = packet.m_body;
		unsigned int nPacketLen = packet.m_nBodySize;

		/* Return PILI_RTMP_READ_COMPLETE if this was completed nicely with
		* invoke message Play.Stop or Play.Complete
		*/
		if (rtnGetNextMediaPacket == 2)
		{
			LOGI("Got Play.Complete or Play.Stop from server. "
				"Assuming stream is complete");
			ret = RTMP_READ_COMPLETE;
			break;
		}

		m_pRtmp->m_read.dataType |= (((packet.m_packetType == RTMP_PACKET_TYPE_AUDIO) << 2) |
			(packet.m_packetType == RTMP_PACKET_TYPE_VIDEO));

		if (packet.m_packetType == RTMP_PACKET_TYPE_VIDEO && nPacketLen <= 5)
		{
			LOGI("ignoring too small video packet: size: %d",
				nPacketLen);
			ret = RTMP_READ_IGNORE;
			break;
		}
		if (packet.m_packetType == RTMP_PACKET_TYPE_AUDIO && nPacketLen <= 1)
		{
			LOGI("ignoring too small audio packet: size: %d",
				nPacketLen);
			ret = RTMP_READ_IGNORE;
			break;
		}

		if (m_pRtmp->m_read.flags & RTMP_READ_SEEKING)
		{
			ret = RTMP_READ_IGNORE;
			break;
		}
#ifdef _DEBUG
		LOGI("type: %02X, size: %d, TS: %d ms, abs TS: %d",
			packet.m_packetType, nPacketLen, packet.m_nTimeStamp,
			packet.m_hasAbsTimestamp);
		if (packet.m_packetType == RTMP_PACKET_TYPE_VIDEO) {
			LOGI("frametype: %02X", (*packetBody & 0xf0));
		}
#endif

		if (packet.m_nTimeStamp == 0)
		{
			/* check the header if we get one */
			if (m_pRtmp->m_read.nMetaHeaderSize > 0
				&& packet.m_packetType == RTMP_PACKET_TYPE_INFO)
			{
				//获取metadata
				PILI_AMFObject metaObj;
				int nRes =
					PILI_AMF_Decode(&metaObj, packetBody, nPacketLen, FALSE);
				if (nRes >= 0)
				{
					PILI_AVal metastring;
					PILI_AMFProp_GetString(PILI_AMF_GetProp(&metaObj, NULL, 0),
						&metastring);
					char strTmp[] = "onMetaData";
					PILI_AVal av_onMetaData = AVC(strTmp);
					if (AVMATCH(&metastring, &av_onMetaData))
					{
						/* compare */
						if ((m_pRtmp->m_read.nMetaHeaderSize != nPacketLen) ||
							(memcmp
							(m_pRtmp->m_read.metaHeader, packetBody,
								m_pRtmp->m_read.nMetaHeaderSize) != 0))
						{
							ret = RTMP_READ_ERROR;
						}
					}
					PILI_AMF_Reset(&metaObj);
					if (ret == RTMP_READ_ERROR)
						break;
				}
			}

		}

		/* calculate packet size and allocate slop buffer if necessary */
		size = nPacketLen +
			((packet.m_packetType == RTMP_PACKET_TYPE_AUDIO
				|| packet.m_packetType == RTMP_PACKET_TYPE_VIDEO
				|| packet.m_packetType == RTMP_PACKET_TYPE_INFO) ? 11 : 0) +
				(packet.m_packetType != RTMP_PACKET_TYPE_FLASH_VIDEO ? 4 : 0);

		/* use to return timestamp of last processed packet */

		/* audio (0x08), video (0x09) or metadata (0x12) packets :
		* construct 11 byte header then add rtmp packet's data */
		if (packet.m_packetType == RTMP_PACKET_TYPE_AUDIO
			|| packet.m_packetType == RTMP_PACKET_TYPE_VIDEO
			|| packet.m_packetType == RTMP_PACKET_TYPE_INFO)
		{
			nTimeStamp = m_pRtmp->m_read.nResumeTS + packet.m_nTimeStamp;
		}

		/* In non-live this nTimeStamp can contain an absolute TS.
		* Update ext timestamp with this absolute offset in non-live mode
		* otherwise report the relative one
		*/
		/* LOGI("type: %02X, size: %d, pktTS: %dms, TS: %dms, bLiveStream: %d", packet.m_packetType, nPacketLen, packet.m_nTimeStamp, nTimeStamp, m_pRtmp->Link.lFlags & PILI_RTMP_LF_LIVE); */
		m_pRtmp->m_read.timestamp = (m_pRtmp->Link.lFlags & RTMP_LF_LIVE) ? packet.m_nTimeStamp : nTimeStamp;

		ret = size;
		break;
	}

	int nRet = 0;

	if (ret > 0)
	{
		if ((int)packet.m_nBodySize <= nBuffSize && packet.m_body)
		{
			memcpy(buff, packet.m_body, packet.m_nBodySize);
			*buflen = packet.m_nBodySize;
			*type = packet.m_packetType;
			*timestamp = packet.m_nTimeStamp;
		}
		else
		{
			nRet = ERROR_BUFF_NOT_ENOUGHT;
		}
	}


	if (rtnGetNextMediaPacket)
		PILI_RTMPPacket_Free(&packet);

	return nRet;
}

int  LibRtmpSession::Flv2H264(char* pSrc, int nSrcLen, char* pDst, int* pDstLen, int* nFrameType)
{
	if (!pSrc || nSrcLen < 2 || !pDst || !pDstLen)
	{
		return -1;
	}


	*pDstLen = 0;
	int nDstLen = 0;
	//int headtype = pSrc[0] & 0xC0;//第一个字节Head_Type的前两个Bit决定了包头的长度.它可以用掩码0xC0进行"与"计算

	int avctype = srs_utils_flv_video_avc_packet_type(pSrc, nSrcLen);
	if (avctype < 0)
	{
		return -1;
	}
	*nFrameType = srs_utils_flv_video_frame_type(pSrc, nSrcLen); //RTMPCodecVideoAVCFrame

	int templength = 0;
	char* pbuff = pDst;

	if (avctype == RTMPCodecVideoAVCTypeSequenceHeader)
	{
		/*
			IF AVCPacketType ==0
				AVCDecoderConfigurationRecord（AVC sequence header）
			IF AVCPacketType == 1
				One or more NALUs (Full frames are required)
			AVCDecoderConfigurationRecord.包含着是H.264解码相关比较重要的sps和pps信息.
			SPS中包含了视频长、宽的信息.
			在给AVC解码器送数据流之前一定要把sps和pps信息送出，否则的话解码器不能正常解码。
			而且在解码器stop之后再次start之前，如seek、快进快退状态切换等，都需要重新送一遍sps和pps的信息.
			AVCDecoderConfigurationRecord在FLV文件中一般情况也是出现1次，也就是第一个video tag.

			// RTMP包写入的AVCDecoderConfigurationRecord格式
			// 参见WriteH264SPSandPPS,相当于流头
		*/

		pSrc += 10; // 跳过AVCDecoderConfigurationRecord
		Read8(templength, pSrc);//sps num

		// sps len
		Read16(templength, pSrc);
		//printf("sps size:%d\n", templength);

		memcpy(pbuff, &g_h264spaceNal, 4);          //写入H264内容NALU间隔标识00000001
		pbuff += 4;
		nDstLen += 4;

		memcpy(pbuff, pSrc, templength);            //从rtmp流中读取sps的数据
		pSrc += templength;

		ModifyNaluData(pbuff, templength);          // 起始码防冲突的处理

		pbuff += templength;
		nDstLen += templength;

		Read8(templength, pSrc);//pps num
		Read16(templength, pSrc);//pps size
		//printf("pps size:%d\n", templength);

		memcpy(pbuff, &g_h264spaceNal, 4);          //写入H264内容NALU间隔标识00000001
		pbuff += 4;
		nDstLen += 4;

		memcpy(pbuff, pSrc, templength);            //从rtmp流中读取pps的数据
		pSrc += templength;

		ModifyNaluData(pbuff, templength);          // 起始码防冲突的处理

		pbuff += templength;
		nDstLen += templength;

		m_strVideoIHead.assign(pDst, nDstLen);
	}
	else
	{
		pSrc += 5;  // 跳过videotag数据头,frametype + avc type + composition time(AVC时，全0，无意义,3字节)

		if (*nFrameType == RTMPCodecVideoAVCFrameKeyFrame)
		{
			size_t nHeaderLen = m_strVideoIHead.size();
			memcpy(pbuff, m_strVideoIHead.c_str(), nHeaderLen);          //写入H264IDR头
			pbuff += nHeaderLen;
			nDstLen += nHeaderLen;
		}

		//可能存在多个nal，需全部读取
		int countsize = 5;
		while (countsize < nSrcLen)
		{
			Read32(templength, pSrc);                   //从rtmp流中读取NALU的长度

			memcpy(pbuff, &g_h264spaceNal, 4);          //写入H264内容NALU间隔标识00000001
			pbuff += 4;
			nDstLen += 4;

			memcpy(pbuff, pSrc, templength);            //从rtmp流中读取NALU的数据
			pSrc += templength;

			pbuff += templength;
			nDstLen += templength;

			countsize += templength + 4;

			printf("NAL size:%d\n", templength);
		}
	}
	*pDstLen = nDstLen;

	return 0;
};

int LibRtmpSession::Flv2AAC(char* pSrc, int nSrcLen, char* pDst, int* nDstLen)
{
	if (!pSrc || nSrcLen < 2 || !pDst || !nDstLen)
	{
		return -1;
	}
	if (!m_pFlvAacEncoder)
	{
		m_pFlvAacEncoder = new FlvAacEncoder();
	}

	return m_pFlvAacEncoder->encode_audio(pSrc, nSrcLen, pDst, nDstLen);;
}


int ModifyNaluData(char* pBuff, int len)
{
	int cnt = 0;
	int i = 0;

	int nSrcPos = 0;
	int nDstPos = 0;

	char* pDstData = new char[len * 2];
	memset(pDstData, 0, len);


	// 起始码防冲突的处理
	for (i = 4; i < len - 3; ++i)
	{
		if ((pBuff[i] == 0x00) && (pBuff[i + 1] == 0x00))
		{
			if ((pBuff[i + 2] == 0x00)//00 00 00
				|| (pBuff[i + 2] == 0x01)//00 00 01
				|| (pBuff[i + 2] == 0x02)//00 00 02
				|| (pBuff[i + 2] == 0x03)//00 00 03
				)
			{
				char cTemp = pBuff[i + 2];

				i += 2;
				int nDstLen = i - nSrcPos;

				memcpy(pDstData + nDstPos, pBuff + nSrcPos, nDstLen);
				pDstData[nDstPos + nDstLen] = 0x03;    //在最后一个字节前插入一个新的字节：0x03
				pDstData[nDstPos + nDstLen + 1] = cTemp;

				nSrcPos = i;
				nDstPos += nDstLen + 2;

				++cnt;

				if ((len - nSrcPos) > 0)
					memcpy(pDstData + nDstPos, pBuff + nSrcPos, len - nSrcPos);// 追加后续数据

			}
		}
	}
	if (cnt)
	{
		memcpy(pBuff, pDstData, len);
	}

	delete[] pDstData;
	pDstData = NULL;

	return cnt;
}

int ParseH264SPSPPS(char* pSrc, int nSrcLen, VideoEncoderInfo* info)
{
	if (!pSrc || nSrcLen < 2 || !info)
	{
		return ERROR_INVALIDPARAM;
	}

	char sps[512] = { 0, };
	char pps[128] = { 0, };
	int spslen = 0;
	int ppslen = 0;

	int nRet = GetH264SPSandPPS(pSrc, nSrcLen, sps, &spslen, pps, &ppslen);
	if (nRet == -1)
	{
		return ERROR_INVALIDPARAM;
	}

	h264_stream_t* h = h264_new();
	read_nal_unit(h, (BYTE*)sps, spslen);

	read_nal_unit(h, (BYTE*)pps, ppslen);

	memcpy(info, h->info, sizeof(videoinfo_t));
	info->max_framerate = info->max_framerate ? info->max_framerate / 2 : 25;
	h264_free(h);

	return 0;
}
