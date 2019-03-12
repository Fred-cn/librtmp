
#include "AACEncoder.h"
#include "./AACEncoder/condef.h"


CAACEncoder::CAACEncoder(void)
	:m_pG7XXToAAC(NULL)
	, m_pFOutAAC(NULL)
	, m_pFOutG7xx(NULL)
{
}

CAACEncoder::~CAACEncoder(void)
{
	SAFE_DELETE_OBJ(m_pG7XXToAAC);

#if OUT_AAC_DEBUG
	if (m_pFOutAAC)
	{
		fclose(m_pFOutAAC);
		m_pFOutAAC = NULL;
	}
	if (m_pFOutG7xx)
	{
		fclose(m_pFOutG7xx);
		m_pFOutG7xx = NULL;
	}
#endif
}

int CAACEncoder::Init(InitAudioParam* initPara)
{
	if (!m_pG7XXToAAC)
	{
		m_pG7XXToAAC = new G7XXToAAC();
	}

	InAudioInfo info(*initPara);
	bool ret = m_pG7XXToAAC->init(info, initPara->ucInAudioFrame ? true : false);

#if OUT_AAC_DEBUG
	time_t tt = time(NULL);
	if (!m_pFOutAAC)
	{
		char szFileName[256] = { 0 };

		sprintf_s(szFileName, 256, "audio_%d.aac", tt);
		m_pFOutAAC = fopen(szFileName, "wb");
	}
#if 0
	if (!m_pFOutG7xx)
	{
		char szFileName[256] = { 0 };

		sprintf_s(szFileName, 256, "audio_%d.g711u", tt);
		m_pFOutG7xx = fopen(szFileName, "wb");
	}
#endif

#endif 

	return 0;
}

int CAACEncoder::Encode(unsigned char* inbuf, unsigned int inlen, unsigned char* outbuf, unsigned int* outlen)
{

#if OUT_AAC_DEBUG
	if (m_pFOutG7xx)
		fwrite(pData, 1, nStreamLen, m_pFOutG7xx);
#endif
	if (m_pG7XXToAAC)
	{

		// G7XX转为AAC，RTMP默认只支持AAC
		int nLen = m_pG7XXToAAC->aac_encode(inbuf, inlen, outbuf, outlen);
		if (*outlen > 0)
		{
#if OUT_AAC_DEBUG
			if (m_pFOutAAC)
				fwrite(pData, 1, nStreamLen, m_pFOutAAC);
#endif
		}

	}

	return 0;
}
