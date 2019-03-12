#pragma once

#include "AACEncoder/G7XXToAAC.h"


class G7XXToAAC;

class CAACEncoder
{
public:
    CAACEncoder(void);
    ~CAACEncoder(void);

    int Init(InitAudioParam* initPara);
    int Encode(unsigned char* inbuf, unsigned int inlen, unsigned char* outbuf, unsigned int* outlen);

private:

    G7XXToAAC*                  m_pG7XXToAAC;
    FILE*                       m_pFOutAAC;
    FILE*                       m_pFOutG7xx;

};
