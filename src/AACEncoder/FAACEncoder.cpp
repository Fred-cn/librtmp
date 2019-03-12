/*
	Copyright (c) 2013-2016 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.easydarwin.org
*/

#include "FAACEncoder.h"

#include "condef.h"
#include "outDebug.h"

FAACEncoder::FAACEncoder(void)
	:m_hfaac(NULL)
	, m_pfaacinfobuffer(NULL)
	, m_nfaacinfosize(0)
{
	m_nSampleRate = 8000;//44100;  // 采样率
	m_nChannels = 1;         // 声道数
	m_nPCMBitSize = 16;      // 单样本位数
	m_nInputSamples = 2048;	//最大输入样本数
	m_nMaxOutputBytes = 0;	//最大输出字节

}


FAACEncoder::~FAACEncoder(void)
{
	if (NULL != m_hfaac)
	{
		/*Close FAAC engine*/
		faacEncClose(m_hfaac);
		SAFE_FREE_BUF(m_pfaacinfobuffer);
	}

}

bool FAACEncoder::Init(InAudioInfo* info)
{
	if (IsWorking())
	{
		return 0;
	}

	//TODO: config this
	m_nSampleRate = info->Samplerate();/*8000*/
	m_nChannels = info->Channel();/*1*/
	m_nPCMBitSize = info->PCMBitSize();/*16*/

	/*open FAAC engine*/
	//初始化aac句柄，同时获取最大输入样本，及编码所需最小字节，参数 samplerate 为要编码的音频pcm流的采样率，channels为要编码的音频pcm流的的频道数
	m_hfaac = faacEncOpen(m_nSampleRate, m_nChannels, &m_nInputSamples, &m_nMaxOutputBytes);
	if (m_hfaac == NULL)
	{
		if (1) INFO_USE2("%s:[%d] failed to call faacEncOpen !\n", __FUNCTION__, __LINE__);
		//return -1;
		return false;
	}

	/*get current encoding configuration*/
	m_pfaacconf = faacEncGetCurrentConfiguration(m_hfaac);//获取配置结构指针

	//* 设置配置参数
	//AAC - 使用MPEG-2 Audio Transport Stream( ADTS，参见MPEG-2 )容器，区别于使用MPEG-4容器的MP4/M4A格式，
	//属于传统的AAC编码（FAAC默认的封装，但FAAC亦可输出 MPEG-4 封装的AAC）
	m_pfaacconf->mpegVersion = MPEG2;//MPEG4;  MPEG2

	int nInputFormat = FAAC_INPUT_16BIT;
	switch (info->PCMBitSize())
	{
	case 16:
		nInputFormat = FAAC_INPUT_16BIT;
		break;
	case 24:
		nInputFormat = FAAC_INPUT_24BIT;
		break;
	case 32:
		nInputFormat = FAAC_INPUT_32BIT;
		break;
	case 40:
		nInputFormat = FAAC_INPUT_FLOAT;
		break;
	default:
		break;
	}
	m_pfaacconf->inputFormat = nInputFormat;
	m_pfaacconf->outputFormat = 1;            //输出是否包含ADTS头,/*0 - raw; 1 - ADTS*/
	m_pfaacconf->useTns = 0;            //时域噪音控制,大概就是消爆音#define DEFAULT_TNS     0
	m_pfaacconf->aacObjectType = LOW;          //LC编码,MAIN
	//m_pfaacconf->useLfe         = false;
	//m_pfaacconf->shortctl       = SHORTCTL_NORMAL;
	//m_pfaacconf->bandWidth      = 0 ;           //频宽
	//m_pfaacconf->quantqual      = 100 ;
	//m_pfaacconf->bitRate        = 0;

	/*set encoding configuretion*/
	faacEncSetConfiguration(m_hfaac, m_pfaacconf);//设置配置，根据不同设置，耗时不一样

	faacEncGetDecoderSpecificInfo(m_hfaac, &m_pfaacinfobuffer, &m_nfaacinfosize);

	int nMaxInputBytes = m_nInputSamples * m_nPCMBitSize / 8;//计算最大输入字节,跟据最大输入样本数
	if (AAC_DEBUG) INFO_USE(" faacEncEncode open, InputSamples=%d MaxOutputBytes=%d\n", m_nInputSamples, m_nMaxOutputBytes);


	/*
	默认配置
	//(0 = Raw; 1 = ADTS)
	hEncoder->config.outputFormat = 1;
			PCM Sample Input Format
			0	FAAC_INPUT_NULL			invalid, signifies a misconfigured config
			1	FAAC_INPUT_16BIT		native endian 16bit
			2	FAAC_INPUT_24BIT		native endian 24bit in 24 bits		(not implemented)
			3	FAAC_INPUT_32BIT		native endian 24bit in 32 bits		(DEFAULT)
			4	FAAC_INPUT_FLOAT		32bit floating point
	hEncoder->config.inputFormat = FAAC_INPUT_32BIT;
	hEncoder->config.version = FAAC_CFG_VERSION;
	hEncoder->config.name = libfaacName;
	hEncoder->config.copyright = libCopyright;
	hEncoder->config.mpegVersion = MPEG4;
	hEncoder->config.aacObjectType = LTP;
	hEncoder->config.allowMidside = 1;
	hEncoder->config.useLfe = 1;
	hEncoder->config.useTns = 0;
	hEncoder->config.bitRate = 0;
	hEncoder->config.bandWidth = 0.45 * hEncoder->sampleRate;
	if(hEncoder->config.bandWidth > 16000)
		hEncoder->config.bandWidth = 16000;
	hEncoder->config.quantqual = 100;
	hEncoder->config.shortctl = SHORTCTL_NORMAL;
	*/

	return true;
}


int FAACEncoder::GetInfo(unsigned char* data, int bufSize)
{
	if (data && m_nfaacinfosize)
	{
		memcpy(data, m_pfaacinfobuffer, m_nfaacinfosize);
		return m_nfaacinfosize;
	}

	return -1;
}

int FAACEncoder::Encode(unsigned char * pbPCMData, unsigned int nPCMDataSize, unsigned char * pbAACBuffer, unsigned int nMaxOutputBytes)
{
	if (!m_hfaac)
	{
		return 0;
	}
	//if(AAC_DEBUG) INFO_USE2("%s:[%d] G7xx -> PCM faacEncEncode....\n",  __FUNCTION__, __LINE__);

	//计算实际输入样本数:PCM数据长度/采样位数8bit 16bit/8：nInputSamples = (nPCMDataSize / (nPCMBitSize / 8));
	unsigned int nInputSamples = (nPCMDataSize / (m_nPCMBitSize / 8));

	//编码,判断下iBytesWritten初始编码的几帧数据会返回0，0是数据被缓冲，并不是错误
	int iBytesWritten = faacEncEncode(m_hfaac, (int32_t*)pbPCMData, nInputSamples, pbAACBuffer, nMaxOutputBytes);

	if (AAC_DEBUG) INFO_USE(" PCM -> AAC faacEncEncode in_len=%d out_len=%d\n", nInputSamples, iBytesWritten);

	return iBytesWritten;
}
