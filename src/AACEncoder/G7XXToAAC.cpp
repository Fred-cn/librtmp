/*
    Copyright (c) 2013-2016 EasyDarwin.ORG.  All rights reserved.
    Github: https://github.com/EasyDarwin
    WEChat: EasyDarwin
    Website: http://www.easydarwin.org
*/
/*
 * File:   EasyAACEncoder.cpp
 * Author: Wellsen@easydarwin.org
 *
 * Created on 2015-04-11, 11:44AM
 */

#include "G7XXToAAC.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

 //#include "g711.h"


#include "outDebug.h"
#include "G711AToPcm.h"
#include "G726ToPcm.h"
#include "condef.h"

//#define OUT_FILE_PCM_DEBUG

G7XXToAAC::G7XXToAAC()
{
    m_pbPCMBuffer = NULL;
    m_pbAACBuffer = NULL;
    m_pbG7FrameBuffer = NULL;
    m_pbPCMTmpBuffer = NULL;

    m_audio_buffer_ = NULL;

    m_pDecodeToPcm = NULL;

    m_pPCMToAAC = NULL;

#ifdef OUT_FILE_PCM_DEBUG
    fOutPcm = NULL;
#endif // OUT_FILE_PCM_DEBUG

}

G7XXToAAC::~G7XXToAAC()
{

    /*free the source of malloc*/
    SAFE_FREE_BUF(m_pbPCMBuffer);
    SAFE_FREE_BUF(m_pbAACBuffer);
    SAFE_FREE_BUF(m_pbG7FrameBuffer);
    SAFE_FREE_BUF(m_pbPCMTmpBuffer);

    SAFE_DELETE_OBJ(m_audio_buffer_);
    SAFE_DELETE_OBJ(m_pDecodeToPcm);
    SAFE_DELETE_OBJ(m_pPCMToAAC);
#ifdef OUT_FILE_PCM_DEBUG
    if (fOutPcm)
    {
        fclose(fOutPcm);
        fOutPcm = NULL;
    }
#endif
}
bool G7XXToAAC::init()
{
    m_nCount = 0;

    CreateBuffer();
#ifdef OUT_FILE_PCM_DEBUG
    if (!fOutPcm)
    {
        char szFileName[256] = { 0 };

        sprintf_s(szFileName, 256, "audio_%d.pcm", this);
        fOutPcm = fopen(szFileName, "wb");
    }
#endif
    return true;
}

bool G7XXToAAC::init(InAudioInfo info, bool bAudioFrame)
{
    m_inAudioInfo = info;
    m_bAudioFrame = bAudioFrame;

    bool ret = false;
    ret = CreateDecodePcm();

    ret = CreateEncodeAac();
    if (!ret)
    {
        return false;
    }
    return init();
}
bool G7XXToAAC::CreateDecodePcm()
{
    char szDecoder[15] = "G711A";
    if (AudioCodec_Law_G711A == m_inAudioInfo.CodecType())
    {
        m_pDecodeToPcm = new G711AToPcm();
    }
    else if (AudioCodec_Law_G711U == m_inAudioInfo.CodecType())
    {
        m_pDecodeToPcm = new G711UToPcm();
#ifdef _WIN32
        sprintf_s(szDecoder, "G711U");
#else
        sprintf(szDecoder, "G711U");
#endif
        //szDecoder = "G711U";
    }
    else if (AudioCodec_Law_G726 == m_inAudioInfo.CodecType())
    {
        m_pDecodeToPcm = new G726ToPcm();
#ifdef _WIN32
        sprintf_s(szDecoder, "G726");
#else
        sprintf(szDecoder, "G726");
#endif
        //szDecoder = "G726";
    }
    else
    {
        m_pDecodeToPcm = new G711UToPcm();
    }
    m_pDecodeToPcm->Init(m_inAudioInfo);
    if (1) INFO_USE2("%s:[%d] Init Decoder=%s Samplerate=%d, PCMBitSize=%d\n", __FUNCTION__, __LINE__,
        szDecoder, m_inAudioInfo.Samplerate(), m_inAudioInfo.PCMBitSize());

    return true;
}
bool G7XXToAAC::CreateEncodeAac()
{
    m_pPCMToAAC = new FAACEncoder();
    bool ret = m_pPCMToAAC->Init(&m_inAudioInfo);

    return ret;
}
bool G7XXToAAC::CreateBuffer()
{
    if (!m_bAudioFrame)
    {
        m_nPCMBufferSize = m_pPCMToAAC->GetPCMBufferSize();
        m_pbPCMBuffer = (unsigned char*)malloc(m_nPCMBufferSize * sizeof(unsigned char));
        memset(m_pbPCMBuffer, 0, m_nPCMBufferSize);

        m_nMaxOutputBytes = m_pPCMToAAC->GetMaxOutputBytes();
        m_pbAACBuffer = (unsigned char*)malloc(m_nMaxOutputBytes * sizeof(unsigned char));
        memset(m_pbAACBuffer, 0, m_nMaxOutputBytes);

        m_nG7FrameBufferSize = m_pDecodeToPcm->G7FrameSize();
        m_pbG7FrameBuffer = (unsigned char *)malloc(m_nG7FrameBufferSize * sizeof(unsigned char));
        memset(m_pbG7FrameBuffer, 0, m_nG7FrameBufferSize);

        m_audio_buffer_ = new audio_buffer();
    }

    m_nPCMSize = m_pDecodeToPcm->PCMSize();
    m_pbPCMTmpBuffer = (unsigned char *)malloc(m_nPCMSize * sizeof(unsigned char));
    memset(m_pbPCMTmpBuffer, 0, m_nPCMSize);

    return true;
}
int G7XXToAAC::aac_encode(unsigned char* inbuf, unsigned int inlen, unsigned char* outbuf, unsigned int* outlen)
{
    int encodeLen = 0;

    if (NULL != m_pDecodeToPcm)
    {
        if (m_bAudioFrame)
        {
            encodeLen = aac_encode_frame(inbuf, inlen, outbuf, outlen);
        }
        else
        {
            encodeLen = aac_encode_obj(inbuf, inlen, outbuf, outlen);
        }
    }

    return encodeLen;
}

int G7XXToAAC::aac_encode_obj(unsigned char* inbuf, unsigned int inlen, unsigned char* outbuf, unsigned int* outlen)
{
    m_audio_buffer_->write_data(inbuf, inlen);
    int buffer_len = 0;
    *outlen = 0;
    int nRet = 0;
    int nStatus = 0;
    int nPCMRead = 0;
    int nPCMSize = 0;

    if (AAC_DEBUG) INFO_USE("0. inlen = %d\n", inlen);

    while ((buffer_len = m_audio_buffer_->get_data(m_pbG7FrameBuffer, m_nG7FrameBufferSize)) > 0)
    {
        if (buffer_len <= 0)
        {
            if (AAC_DEBUG) INFO_USE("ERROR [%d] G7xx -> PCM  no G7xx data !\n", buffer_len);
            //Sleep(100);
            return -1;
        }

        nStatus = 0;
        memset(m_pbPCMTmpBuffer, 0, m_nPCMSize);
        nPCMSize = m_nPCMSize;

        if ((nPCMRead = m_pDecodeToPcm->Decode(m_pbPCMTmpBuffer, (unsigned int*)&nPCMSize, m_pbG7FrameBuffer, buffer_len)) < 0) // TODO: skip 4 byte?
        {
            if (AAC_DEBUG) INFO_USE(" G7xx -> PCM  failed buffer_len = %d !\n", buffer_len);
            return -1;
        }
        if (AAC_DEBUG) INFO_USE("1. nPCMRead = %d, PCMSize = %d, inlen =%d\n", nPCMRead, nPCMSize, inlen);
#ifdef OUT_FILE_PCM_DEBUG
        if (fOutPcm)
            fwrite(m_pbPCMTmpBuffer, 1, nPCMRead, fOutPcm);
#endif
        if ((m_nPCMBufferSize - m_nCount) < nPCMRead)
        {
            //if(AAC_DEBUG) INFO_USE("2.  G7xx -> PCM nPCMBufferSize = %d, nCount = %d, nPCMRead = %d\n", nPCMBufferSize, nCount, nPCMRead);
            nStatus = 1;
            memset(m_pbAACBuffer, 0, m_nMaxOutputBytes);
            memcpy(m_pbPCMBuffer + m_nCount, m_pbPCMTmpBuffer, (m_nPCMBufferSize - m_nCount));

            int iBytesWritten = m_pPCMToAAC->Encode(m_pbPCMBuffer, m_nPCMBufferSize, m_pbAACBuffer, m_nMaxOutputBytes);

            memcpy(outbuf + *outlen, m_pbAACBuffer, iBytesWritten);
            *outlen += iBytesWritten;

            int nTmp = nPCMRead - (m_nPCMBufferSize - m_nCount);
            memset(m_pbPCMBuffer, 0, m_nPCMBufferSize);
            memcpy(m_pbPCMBuffer, m_pbPCMTmpBuffer + (m_nPCMBufferSize - m_nCount), nTmp);
            if (AAC_DEBUG) INFO_USE("2. PCM -> AAC (nPCMBufferSize(%d) - nCount(%d)) < nPCMRead(%d) diff=%d, AACLen=%d\n",
                m_nPCMBufferSize, m_nCount, nPCMRead, nTmp, *outlen);
            m_nCount = 0;
            m_nCount += nTmp;
        }

        if (nStatus == 0)
        {
            memcpy(m_pbPCMBuffer + m_nCount, m_pbPCMTmpBuffer, nPCMRead);
            m_nCount += nPCMRead;
            if (AAC_DEBUG) INFO_USE("3. G7xx -> PCM nStatus = 0...nPCMRead=%d, nCount=%d\n",
                nPCMRead, m_nCount);
        }

        if (nPCMRead < /*320*/CON_PCM_SIZE)
        {
            //if(AAC_DEBUG) INFO_USE("3. PCM -> AAC nPCMRead = %d\n", nPCMRead);

            nRet = m_pPCMToAAC->Encode(m_pbPCMBuffer, m_nCount, m_pbAACBuffer, m_nMaxOutputBytes);
            //if (nRet>0)
            {
                memcpy(outbuf + *outlen, m_pbAACBuffer, nRet);
                *outlen += nRet;
            }

            INFO_D(AAC_DEBUG, "4. PCM -> AAC nPCMRead(%d) < CON_PCM_SIZE(%d)", nRet, CON_PCM_SIZE);
        }
    }
    if (AAC_DEBUG) INFO_USE("5. outlen = %d\n", *outlen);

    return *outlen;
}


int G7XXToAAC::aac_encode_frame(unsigned char* inbuf, unsigned int inlen, unsigned char* outbuf, unsigned int* outlen)
{

    int nMaxOutputBytes = *outlen;
    *outlen = 0;
    int nPCMSize = 0;
    int nAACSize = 0;
    int nPCMRead = 0;

    {
        if (inlen <= 0)
        {
            if (AAC_DEBUG) INFO_USE(" G7xx -> PCM  no G7xx data !\n");
            return -1;
        }

        memset(m_pbPCMTmpBuffer, 0, m_nPCMSize);
        nPCMSize = m_nPCMSize;
        // G711解码为PCM
        if ((nPCMRead = m_pDecodeToPcm->Decode(m_pbPCMTmpBuffer, (unsigned int*)&nPCMSize, inbuf, inlen)) < 0)
        {
            if (AAC_DEBUG) INFO_USE(" G7xx -> PCM  failed frame_len = %d !\n", inlen);
            return -1;
        }
#ifdef OUT_FILE_PCM_DEBUG
        if (fOutPcm)
            fwrite(m_pbPCMTmpBuffer, 1, nPCMRead, fOutPcm);
#endif
        if (nPCMRead > 0 && nPCMRead <= CON_PCM_SIZE/*320*/)
        {
            //PCM编码为AAC
            nAACSize = m_pPCMToAAC->Encode(m_pbPCMTmpBuffer, nPCMRead, outbuf, nMaxOutputBytes);
            if (nAACSize > 0)
            {
                *outlen = nAACSize;
            }
        }
    }

    return *outlen;
}
