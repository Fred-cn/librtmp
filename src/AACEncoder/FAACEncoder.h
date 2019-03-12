/*
	Copyright (c) 2013-2016 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.easydarwin.org
*/

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "IDecodeToPcm.h"

extern "C"
{
#include <faac.h>
}

class FAACEncoder
{
public:
	FAACEncoder(void);
	~FAACEncoder(void);
public:
	bool Init(InAudioInfo* info);
	int Encode(unsigned char * pbPCMData, unsigned int nPCMDataSize, unsigned char *outputBuffer, unsigned int bufferSize);
	int GetInfo(unsigned char *data, int bufSize);
public:
	bool IsWorking(void)
	{
		return m_hfaac != NULL;
	}
	unsigned int GetPCMBitSize()
	{
		return m_nPCMBitSize;
	}
	unsigned int GetInputSamples()
	{
		return m_nInputSamples;
	}
	unsigned int GetMaxOutputBytes()
	{
		return m_nMaxOutputBytes;
	}
	unsigned int GetPCMBufferSize()
	{
		return (m_nInputSamples * (m_nPCMBitSize / 8));
	}


private:
	long m_nSampleRate;  // 采样率
	int m_nChannels;         // 声道数
	int m_nPCMBitSize;      // 单样本位数/*= 16*/
	unsigned long m_nInputSamples;	//最大输入样本数
	unsigned long m_nMaxOutputBytes;	//最大输出字节
	faacEncHandle m_hfaac;
	faacEncConfigurationPtr m_pfaacconf;


	unsigned char*m_pfaacinfobuffer;
	unsigned long m_nfaacinfosize;
};
