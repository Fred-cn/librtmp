========================================================================
    动态链接库：librtmpsdk 项目概述
========================================================================

RTMP的客户端SDK，支持推流，也支持拉流，并支持设置连接超时和接收超时的参数。
目前推拉流的视频只支持H264。
推流端的音频支持AAC，G711A，G711U，G726。
拉流端的音频只支持AAC。


========================================================================
更新日志：


2018-04-28
1.librtmpsdk V1.0.0.21，修正在系统缺少resolv.conf配置文件时，librtmp通信库调用add_addr_info解析域名可能出现崩溃的问题。

2018-04-27
1.librtmpsdk V1.0.0.20，增加log4z日志输出功能,用于调试librtmp通信库add_addr_info可能出现崩溃的问题。

2018-01-13
1.librtmpsdk V1.0.0.19，更换librtmp通信库,用于支持嵌入式arm64位的linux下正常运行。

2018-01-12
1.librtmpsdk V1.0.0.18，修正RTMPCLIENT_SendH264Frame和RTMPCLIENT_SendAudioFrame的返回值，成功返回0，否则为失败的错误代码

2018-01-04
1.librtmpsdk V1.0.0.17，修正推流在解析码流数据时在一些特殊情况下SeparateNalus会越界访问导致崩溃的bug.

2017-12-28
1.librtmpsdk V1.0.0.16，修正在linux下64位交叉编译出现的类型错误.

2017-12-19
1.librtmpsdk V1.0.0.15，修正在linux下编译出现缺少类型的错误.

2017-06-21
1.librtmpsdk V1.0.0.13，修正解析SPS和PPS时带有06 SEI增强信息NAL会出错误的bug.

2017-05-08
1.librtmpsdk V1.0.0.12，加强发送H264码流数据的合法性检测.

2017-04-21
1.librtmpsdk V1.0.0.11，修正接收到SPS加上NALU起始码防冲突处理ModifyNaluData可能引起崩溃的问题.

2017-04-21
1.librtmpsdk V1.0.0.10，修正接收到SPS和PPS信息时，加上H264编码NALU起始码防冲突处理ModifyNaluData().

2017-04-20
1.librtmpsdk V1.0.0.9，修正发送SPS和PPS信息时的长度参数，由有符号int改为无符号short.

2017-04-11
1.librtmpsdk V1.0.0.8,解决同为球机的视频码流推送后拉流端无法解码播放的问题，每个I帧都发送SPS和PPS信息.

2017-04-10
1.librtmpsdk V1.0.0.7,修正发送空数据导致崩溃的问题.

2017-03-31
1.librtmpsdk V1.0.0.6,修改明日球机的视频在同一个I帧内有多个NALU片时发送的问题.

2017-01-07
1.librtmpsdk V1.0.0.5,修正音频参数未初始化前推音频流会导致崩溃的问题.
  改正为音频参数未初始化前进行推流，接口返回错误号ERROR_AUDIO_CODEC_NOT_SUPPORT(4)

2016-12-23
1.V1.0.0.3,修正推流造成内存泄漏的bug.

2016-12-22
1.V1.0.0.2,完成推流接口

2016-12-15
1.V1.0.0.1,完成拉流接口

========================================================================
使用步骤：

一、推流步骤：
1.创建会话，创建后的会话句柄，由上层保证其有效性。
    RTMPCLIENT_Session_Create
    RTMPCLIENT_SetTimeout
2.推流连接
    RTMPCLIENT_ConnectPublish
3.连接状态判断
    RTMPCLIENT_IsLinked
4.初始化音频参数(推流G711等非AAC音频时必须先调用)    
    RTMPCLIENT_InitMetadata
    
5.发送码流数据
    发送H264视频帧
    RTMPCLIENT_SendH264Frame
    发送adts格式的AAC音频帧
    RTMPCLIENT_SendAACFrame
    发送G711音频帧
    RTMPCLIENT_SendAudioFrame
    发送flv数据包
    RTMPCLIENT_SendFlvPacket
    
6.断开连接
    RTMPCLIENT_DisConnect    
7.释放会话
    RTMPCLIENT_Session_Release
    
二、拉流步骤：    
1.创建会话
    RTMPCLIENT_Session_Create
    RTMPCLIENT_SetTimeout
2.拉流连接
    RTMPCLIENT_ConnectPlay
3.连接状态判断
    RTMPCLIENT_IsLinked    
    
4.读取RTMP数据    
    RTMPCLIENT_ReadPacket
    RTMPCLIENT_ReadData
5.点播控制
    RTMPCLIENT_PlayPause
    RTMPCLIENT_PlaySeek
    
6.读取RTMP的flv包转换为码流帧    
    RTMPCLIENT_FlvTag2H264
    RTMPCLIENT_FlvTag2AAC
    RTMPCLIENT_ParseH264SPSPPS
    
7.断开连接
    RTMPCLIENT_DisConnect    
8.释放会话
    RTMPCLIENT_Session_Release
    