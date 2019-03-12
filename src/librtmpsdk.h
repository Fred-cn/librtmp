/**
*  COPYRIGHT NOTICE 
*  Copyright (c) 2015, Diveo Technology Co.,Ltd (www.diveo.com.cn)
*  All rights reserved.
*
*  @file        RTMPClientSDK.h
*  @author      roger
*  @date        2016/08/26
*
*  @brief       RTMPClientSDK头文件
*  @note        C++ RTMP Client SDK.

                Support:
                Publish stream:H2164+AAC, 
                Read RTMP Pakcet, 

                Features:
                FLVTag to H264 frame
                FLVTag to ADTS AAC
                Parse H264 SPS and PPS to videoinfo.
*
*  @version
*    - v1.0.0.1    2016/08/26
*/

#ifndef __RTMP_CLIENT_SDK_H_
#define __RTMP_CLIENT_SDK_H_

#if (defined(WIN32) || defined(WIN64))

#ifdef RTMPCLIENTSDK_EXPORTS

    #ifndef RTMPCLIENT_API
    #define RTMPCLIENT_API extern "C" __declspec(dllexport)
    #endif

#else

    #ifndef RTMPCLIENT_API
    #define RTMPCLIENT_API __declspec(dllimport)
    #endif

#endif

#define CALLMETHOD __stdcall
#define CALLBACK   __stdcall

#else	//linux

#define RTMPCLIENT_API extern "C"
#define CALLMETHOD
#define CALLBACK

#define BOOL	            int
#define TRUE	            1
#define FALSE	            0
typedef void* HANDLE;

#endif

#ifndef IN
#define IN		///< 输入参数
#endif

#ifndef OUT
#define OUT		///< 输出参数
#endif

#ifndef INOUT
#define INOUT	///< 输入/输出参数
#endif


#include "rtmp_codec_define.h"
#include <time.h>

#ifdef __cplusplus
extern "C" 
{
#endif


/**
*  @brief        创建RTMP会话Session 返回会话句柄
*  @return       HANDLE,失败返回NULL，成功返回设备ID     
*  @remarks      SDK所有与RTMP会话相关的交互接口，都必须使用此接口返回的有效会话句柄
*/
RTMPCLIENT_API HANDLE CALLMETHOD RTMPCLIENT_Session_Create();

/**
*  @brief        停止RTMP会话，释放会话句柄 
*  @param[in]    hSession            : RTMPCLIENT_Session_Create成功的返回值
*  @return       成功返回0，否则为失败的错误代码   
*  @remarks      此后，该句柄不应该再被使用，否则会出现野指针操作错误。
*/
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_Session_Release(IN HANDLE hSession);

/**
*  @brief        设置网络超时参数
*  @param[in]    hSession           : RTMPCLIENT_Session_Create成功的返回值
*  @param[in]    nRecvTimeoutMsecs  : 接收超时，单位为毫秒
*  @param[in]    nSendTimeoutMsecs  : 发送超时，单位为毫秒
*  @return       成功返回0，否则为失败的错误代码
*  @remarks      如果需要改变超时参数，须在RTMPCLIENT_Connect之前调用   
*/
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_SetTimeout(IN HANDLE   hSession,
                                                    IN int      nRecvTimeoutMsecs, 
                                                    IN int      nSendTimeoutMsecs
                                                    );

/**
*  @brief        连接RTMP服务器推流
*  @param[in]    hSession   : RTMPCLIENT_Session_Create成功的返回值
*  @param[in]    url        : rtmp url, normal: rtmp://ip:port/app/stream 
*  @param[in]    nTimeoutMS : 超时时间,单位:毫秒.
*  @param[in]    nChunkSize : 设置RTMP推送直播服务器的ChunkSize大小
*  @return       成功返回0，否则为失败的错误代码
*  @remarks      
*/
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_ConnectPublish(IN HANDLE      hSession, 
                                                        IN const char* url, 
                                                        IN int         nTimeoutMS,
                                                        IN int         nChunkSize
                                                        );

/**
*  @brief        连接RTMP服务器拉流
*  @param[in]    hSession   : RTMPCLIENT_Session_Create成功的返回值
*  @param[in]    url        : rtmp url, normal: rtmp://ip:port/app/stream 
*  @param[in]    nTimeoutMS : 超时时间,单位:毫秒.
*  @param[in]    nBufferMS  : 设置服务器缓存大小,时间单位:毫秒.
*  @param[in]    bLiveStream: 是否直播流，1为live直播，0为vod点播.
*  @param[in]    nSeekPlayTime  : vod play start time,时间单位:毫秒,仅点播拉流时有效.为0是从头开始
*  @return       成功返回0，否则为失败的错误代码
*  @remarks      
*/
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_ConnectPlay(IN HANDLE      hSession, 
                                                     IN const char* url, 
                                                     IN int         nTimeoutMS,
                                                     IN int         nBufferMS,
                                                     IN BOOL        bLiveStream,
                                                     IN int         nSeekPlayTime
                                                     );

/**
*  @brief        获取连接RTMP是否正常
*  @param[in]    hSession     : RTMPCLIENT_Session_Create成功的返回值
*  @return       连接正常返回1，否则为连接异常
*  @remarks        
*/
RTMPCLIENT_API BOOL CALLMETHOD RTMPCLIENT_IsLinked(IN HANDLE hSession);

/**
*  @brief        断开连接RTMP
*  @param[in]    hSession     : RTMPCLIENT_Session_Create成功的返回值
*  @return       成功返回0，否则为失败的错误代码
*  @remarks        
*/
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_DisConnect(IN HANDLE hSession);

///////////////////RTMP Publish API begin///////////////////////////////////////////////////////

/**
*  @brief        创建RTMP推送的参数信息
*  @param[in]    hSession     : RTMPCLIENT_Session_Create成功的返回值
*  @param[in]    initAudio    : 音频参数，InitAudioParam
*  @param[in]    videoFrameRate : 视频帧率
*  @return       成功返回0，否则为失败的错误代码 
*  @remarks        
*/
RTMPCLIENT_API int CALLMETHOD  RTMPCLIENT_InitMetadata(IN HANDLE            hSession, 
                                                       IN InitAudioParam*   initAudio, 
                                                       IN int               videoFrameRate
                                                       );

/**
*  @brief        推送H264视频
*  @param[in]    hSession     : RTMPCLIENT_Session_Create成功的返回值
*  @param[in]    data         : 视频数据帧的首地址
*  @param[in]    size         : 视频数据帧的长度
*  @param[in]    nTimeStamp   : 时间戳
*  @param[in]    bIsKeyFrame  : 是否为关键帧（1：I帧/IDR帧，0：P/B帧）
*  @return       成功返回0，否则为失败的错误代码 
*  @remarks        
*/
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_SendH264Frame(IN HANDLE           hSession, 
                                                       IN unsigned char*   data, 
                                                       IN int              size, 
                                                       IN time_t           nTimeStamp, 
                                                       IN BOOL             bIsKeyFrame
                                                       );

/**
*  @brief        推送H265视频
*  @param[in]    hSession     : RTMPCLIENT_Session_Create成功的返回值
*  @param[in]    data         : 视频数据帧的首地址
*  @param[in]    size         : 视频数据帧的长度
*  @param[in]    nTimeStamp   : 时间戳
*  @param[in]    bIsKeyFrame  : 是否为关键帧（1：I帧/IDR帧，0：P/B帧）
*  @return       成功返回0，否则为失败的错误代码
*  @remarks
*/
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_SendH265Frame(IN HANDLE           hSession,
	IN unsigned char*   data,
	IN int              size,
	IN time_t           nTimeStamp,
	IN BOOL             bIsKeyFrame
);

/**
*  @brief        推送AAC音频
*  @param[in]    hSession     : RTMPCLIENT_Session_Create成功的返回值
*  @param[in]    data         : AAC音频数据的首地址
*  @param[in]    size         : AAC音频数据的长度
*  @param[in]    nTimeStamp   : 时间戳
*  @return       成功返回0，否则为失败的错误代码 
*  @remarks      data需要为adts封装的AAC数据帧,时间戳与视频的时间戳统一排序   
*/
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_SendAACFrame(IN HANDLE            hSession, 
                                                      IN unsigned char*    data, 
                                                      IN int               size, 
                                                      IN time_t            nTimeStamp
                                                      );

/**
*  @brief        推送音频帧
*  @param[in]    hSession     : RTMPCLIENT_Session_Create成功的返回值
*  @param[in]    data         : 音频数据的首地址
*  @param[in]    size         : 音频数据的长度
*  @param[in]    nTimeStamp   : 时间戳
*  @param[in]    nAudioCodecType : 音频编码类型
*  @return       成功返回0，否则为失败的错误代码 
*  @remarks      非AAC的data会被转码为adts封装的AAC数据帧,时间戳与视频的时间戳统一排序。
*                nAudioCodecType音频编码类型，填充值为enum eAudioCodecType
*/
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_SendAudioFrame(IN HANDLE          hSession, 
                                                        IN unsigned char*  data, 
                                                        IN int             size, 
                                                        IN time_t          nTimeStamp, 
                                                        IN eAudioCodecType nAudioCodecType
                                                        );

/**
*  @brief        推送FlvTag
*  @param[in]    hSession     : RTMPCLIENT_Session_Create成功的返回值
*  @param[in]    data         : flv数据的首地址
*  @param[in]    size         : flv数据的长度
*  @param[in]    nTimeStamp   : 时间戳
*  @param[in]    cTagType     : flv packet type,
*  @return       成功返回0，否则为失败的错误代码 
*  @remarks      此接口在转发时使用。
                 cTagType值为enum RTMPCodecFlvTag:8-AUDIO,9-VIDEO,18-SCRIPT
*/
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_SendFlvPacket(IN HANDLE               hSession, 
                                                       IN unsigned char*       data, 
                                                       IN int                  size, 
                                                       IN time_t               nTimeStamp, 
                                                       IN int                  cTagType
                                                       );

///////////////////RTMP Publish API end///////////////////////////////////////////////////////


///////////////////RTMP Play API begin///////////////////////////////////////////////////////

/**
*  @brief        read a audio/video/script-data packet from rtmp stream.
*  @param[in]    hSession     : RTMPCLIENT_Session_Create成功的返回值
*  @param[out]   buff         : the packet data
*  @param[inout] size         : size of packet.
*  @param[out]   nTimeStamp   : 时间戳, in ms, overflow in 50days
*  @param[out]   cTagType     : output the packet type
*  @return       成功返回0，否则为失败的错误代码 
*  @remarks      1.cTagType, output the packet type, macros:
*                  SRS_RTMP_TYPE_AUDIO, FlvTagAudio
*                  SRS_RTMP_TYPE_VIDEO, FlvTagVideo
*                  SRS_RTMP_TYPE_SCRIPT, FlvTagScript
*                  otherswise, invalid type.
*
*                2.buff, the packet data, according to type:
*                  FlvTagAudio, @see "E.4.2.1 AUDIODATA"
*                  FlvTagVideo, @see "E.4.3.1 VIDEODATA"
*                  FlvTagScript, @see "E.4.4.1 SCRIPTDATA"
*                3.size, input max length of data buffer ,output size of packet.
*/
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_ReadPacket(IN    HANDLE            hSession, 
                                                    OUT   char*             buff, 
                                                    INOUT int*              size, 
                                                    OUT   time_t*           nTimeStamp, 
                                                    OUT   char*             cTagType
                                                    );

/**
*  @brief        read data from rtmp stream.
*  @param[in]    hSession     : RTMPCLIENT_Session_Create成功的返回值
*  @param[out]   buff         : data buffer
*  @param[inout] size         : 读取不超过指定大小的数据,返回实际读到的数据大小,数据存放到buff中返回.
*  @return       成功返回0，否则为失败的错误代码 
*  @remarks      1.buff, the rtmp data, 
*                3.size, input max length of data buffer ,output size of packet.
*/
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_ReadData(IN    HANDLE            hSession, 
                                                  OUT   char*             buff, 
                                                  INOUT int*              size
                                                  );

/**
*  @brief        暂停播放/恢复播放
*  @param[in]    hSession     : RTMPCLIENT_Session_Create成功的返回值
*  @param[in]    bPause       : pause/resume
*  @return       成功返回0，否则为失败的错误代码 
*  @remarks      vod play时有效，live play时无效
*/
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_PlayPause(IN    HANDLE             hSession, 
                                                   IN    BOOL               bPause
                                                   );

/**
*  @brief        改变播放的位置
*  @param[in]    hSession     : RTMPCLIENT_Session_Create成功的返回值
*  @param[in]    seekTime     : seek vod play time,时间单位:毫秒
*  @return       成功返回0，否则为失败的错误代码 
*  @remarks      vod play时有效，live play时无效
*/
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_PlaySeek(IN    HANDLE              hSession, 
                                                  IN    int                 seekTime
                                                  );


///////////////////RTMP Play API flv tag to av packet///////////////////////////////////////////////////////

/**
*  @brief        decode a video flv packet to a H264 frame.RTMP流包提取H264原始帧
*  @param[in]    hSession     : RTMPCLIENT_Session_Create成功的返回值
*  @param[in]    pSrc         : the packet src data
*  @param[in]    nSrcLen      : size of packet.
*  @param[out]   pDst         : buffer of H264 frame
*  @param[inout] nDstLen      : output size of the H264 frame
*  @param[out]   nFrameType   : type of H264 frame,enum RTMPCodecVideoAVCFrame
*  @return       成功返回0，否则为失败的错误代码 
*  @remarks      nDstLen, input max length of frame buffer,output size of frame.
*/
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_FlvTag2H264(IN    HANDLE           hSession, 
                                                     IN    char*            pSrc, 
                                                     IN    int              nSrcLen, 
                                                     OUT   char*            pDst, 
                                                     INOUT int*             nDstLen, 
                                                     OUT   int*             nFrameType
                                                     );

/**
*  @brief        decode a audio flv packet to aac frame.RTMP流包提取AAC帧
*  @param[in]    hSession     : RTMPCLIENT_Session_Create成功的返回值
*  @param[in]    pSrc         : the packet src data
*  @param[in]    nSrcLen      : size of packet.
*  @param[out]   pDst         : buffer of aac frame
*  @param[inout] nDstLen      : output size of the aac frame
*  @return       成功返回0，否则为失败的错误代码 
*  @remarks      nDstLen, input max length of frame buffer,output size of frame.
*/
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_FlvTag2AAC(IN    HANDLE            hSession, 
                                                    IN    char*             pSrc, 
                                                    IN    int               nSrcLen, 
                                                    OUT   char*             pDst, 
                                                    INOUT int*              nDstLen
                                                    );

/**
*  @brief        从H264原始帧提取视频信息
*  @param[in]    hSession     : RTMPCLIENT_Session_Create成功的返回值
*  @param[in]    pSrc         : the h264 frame(I/IDR帧)
*  @param[in]    nSrcLen      : size of packet.
*  @param[out]   info         : VideoEncoderInfo类型，包括视频宽高，帧率等信息
*  @return       成功返回0，否则为失败的错误代码 
*  @remarks      
*/
RTMPCLIENT_API int CALLMETHOD RTMPCLIENT_ParseH264SPSPPS(IN char*           pSrc, 
                                                         int                nSrcLen, 
                                                         OUT VideoEncoderInfo* info);


///////////////////RTMP Play API end///////////////////////////////////////////////////////


#ifdef __cplusplus
};
#endif

#endif
