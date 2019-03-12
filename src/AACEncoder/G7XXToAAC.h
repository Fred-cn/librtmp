/*
	Copyright (c) 2013-2016 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.easydarwin.org
*/
/* 
 * File:   EasyAACEncoder.h
 * Author: Wellsen@easydarwin.org
 *
 * Created on 2015-04-11, 11:44AM
 */

#ifndef EasyAACEncoder_H
#define	EasyAACEncoder_H


#include "audio_buffer.h"
#include "IDecodeToPcm.h"
#include "FAACEncoder.h"

class G7XXToAAC
{
public:
    G7XXToAAC();
    virtual ~G7XXToAAC();
    
	bool init();
	bool init(InAudioInfo info, bool bAudioFrame);
    
    int aac_encode(unsigned char* inbuf, unsigned int inlen, unsigned char* outbuf, unsigned int* outlen);

private:
	int aac_encode_frame(unsigned char* inbuf, unsigned int inlen, unsigned char* outbuf, unsigned int* outlen );
	int aac_encode_obj(unsigned char* inbuf, unsigned int inlen, unsigned char* outbuf, unsigned int* outlen );
    
	bool CreateDecodePcm();
	bool CreateEncodeAac();
	bool CreateBuffer();

private:        
    int             m_nCount;
 
    int             m_nPCMBufferSize;
    unsigned char  *m_pbPCMBuffer;

    unsigned long   m_nMaxOutputBytes;
    unsigned char  *m_pbAACBuffer;

    int             m_nPCMSize;   
    unsigned char  *m_pbPCMTmpBuffer; 

	unsigned char  *m_pbG7FrameBuffer;
	unsigned long   m_nG7FrameBufferSize;

    audio_buffer   *m_audio_buffer_;
	//------
	InAudioInfo     m_inAudioInfo;
    bool            m_bAudioFrame;

	IDecodeToPcm   *m_pDecodeToPcm;
	FAACEncoder    *m_pPCMToAAC;

    FILE*           fOutPcm;
};

#endif	/* EasyAACEncoder_H */

