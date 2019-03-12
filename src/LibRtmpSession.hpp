//
//  LibRtmpSession.hpp
//  AudioEditX
//
//  Created by Alex.Shi on 16/3/8.
//  2016 com.Alex. All rights reserved.
//

#ifndef RtmpSession_hpp
#define RtmpSession_hpp

#include <stdio.h>
#include <string>
#include "rtmp_codec_define.h"


#ifdef __cplusplus
extern "C" {
#endif

#define RTMP_TYPE_PLAY 0
#define RTMP_TYPE_PUSH 1

typedef struct PILI_RTMP RTMP;
typedef struct PILI_RTMPPacket RTMPPacket;
typedef struct _RTMPMetadata RTMPMetadata;
typedef struct _DataItem DataItem;

class CAACEncoder;
class FlvAacEncoder;
    
class LibRtmpSession
{
public:
    LibRtmpSession();
    ~LibRtmpSession();
    
    int Connect(int bPublishFlag, const char* szRtmpUrl, int nTimeoutMS, 
        int nBufferMS, int bLiveStream, int iSeekPlayTime);
    void DisConnect();
    int IsConnected();

    int SetTimeout(int recv_timeout_ms, int send_timeout_ms);
    int SendReSetChunkSize(int nChunkSize);

    int InitMetadata(InitAudioParam* initAudio, int videoFrameRate);

    int SendAudioRawData(unsigned char* pBuff, int len, unsigned int ts);
    
    int SendVideoRawData(unsigned char* buf, int videodatalen, unsigned int ts);

    int SendPacket(unsigned int nPacketType,unsigned char *data,unsigned int size, time_t nTimestamp);  

    int  GetConnectedFlag(){return  m_iConnectFlag;};
    void SetConnectedFlag(int iConnectFlag){m_iConnectFlag=iConnectFlag;};

    int  GetASCSentFlag();
    void GetASCInfo(unsigned short usAscFlag);
    int  SendAudioSpecificConfig(unsigned char audioObjectType, unsigned char samplingFrequencyIndex, unsigned char channels);
    void MakeAudioSpecificConfig(char* pData, int aactype, int sampleRate, int channels);
    void MakeAudioSpecificConfigEx(char* pData, unsigned char audioObjectType, unsigned char samplingFrequencyIndex, unsigned char channels);
    
    int SendAudioData(unsigned char* buf, int size, time_t nTimeStamp, eAudioCodecType nAudioCodecType);    
    int SendAudioSpecificConfig(unsigned short usASCFlag);
    int SendAudioAdtsAAC(unsigned char* buf, int size, time_t nTimeStamp);
    int SendAACRawData(unsigned char* buf, int size, unsigned int timeStamp);

    int SendH264VideoSequenceHeader(unsigned char *pps,unsigned short pps_len,unsigned char * sps,unsigned short sps_len);
	int SendH265VideoSequenceHeader(unsigned char *pps, unsigned short pps_len, unsigned char * sps, unsigned short sps_len, unsigned char * vps, unsigned short vps_len);
    int SendH264VideoData(unsigned char* buf, int size, time_t nTimeStamp, bool bIsKeyFrame);
	int SendH265VideoData(unsigned char* buf, int size, time_t nTimeStamp, bool bIsKeyFrame);
    int SendH264Packet(unsigned char *data,unsigned int size,int bIsKeyFrame,time_t nTimeStamp);
	int SendH265Packet(unsigned char *data, unsigned int size, int bIsKeyFrame, time_t nTimeStamp);

    //////////////////////////////////////////////////////////////////////////

    int ReadPacket(char* buff, int* size, time_t* timestamp, char* type);
    int ReadDataEx(char* buff, int* size);
    int ReadData(unsigned char* buff, int iSize);
    int GetReadStatus();

    void GetSpsInfo(unsigned char* pSpsData, int iLength);
    int GetAACType();
    int GetSampleRate();
    int GetChannels();
    int getSampleRateByType(int iType);

    double GetDuration();
    int Pause(int DoPause);
    int Seek(int seekTime);
    
    int Flv2H264(char* pSrc, int nSrcLen, char* pDst, int* nDstLen, int* nFrameType);
    
    int Flv2AAC(char* pSrc, int nSrcLen, char* pDst, int* nDstLen);

private:

    int RtmpPacketSend(RTMPPacket* packet);

    int getSampleRateType(int iSampleRate);
    int getFlvAudioHeadSampleRateType(int sampling_frequency_index);
    
    int SeparateNalus(unsigned char* pBuff, int len);

private:
    RTMP*           m_pRtmp;
    FILE*           m_pFileLog;

    std::string     m_strRtmpURL;
    int             m_bPublishFlag;
    int             m_iConnectFlag;

    int             m_recv_timeout_ms;
    int             m_send_timeout_ms;

    int             m_iAacType;
    int             m_iSampleRate;
    int             m_iChannels;

    int             m_iWidth;
    int             m_iHeight;
    int             m_iFps;
    
    DataItem*       m_pAdtsItems;
    DataItem*       m_pNaluItems;

    int             m_iMetaDataFlag;
    int             m_iASCSentFlag;
    unsigned char   m_cFlvAudioHead0;

    unsigned int    m_uiStartTimestamp;
    unsigned int    m_uiAudioDTS;
    unsigned int    m_uiVideoLastAudioDTS;
    unsigned int    m_uiAudioDTSNoChangeCnt;

    RTMPMetadata*   m_pMetaData;
    std::string     m_strAdts;

    CAACEncoder*    m_pAACEncoder;
    InitAudioParam  m_initParam;
    std::string     m_strAACBuf;
    bool            m_bInitAudioParam;
    
    std::string     m_strVideoIHead;
    FlvAacEncoder*  m_pFlvAacEncoder;

    //pthread_mutex_t _mConnstatMutex;
};

int ParseH264SPSPPS(char* pSrc, int nSrcLen, VideoEncoderInfo* info);

#ifdef __cplusplus
}
#endif
#endif /* RtmpSession_hpp */
