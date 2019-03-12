
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "GetVPSSPSPPS.h"



//输入的pbuf必须包含start code(00 00 00 01)
int GetH265VPSandSPSandPPS(char *pbuf, int bufsize, char *_vps, int *_vpslen, char *_sps, int *_spslen, char *_pps, int *_ppslen)
{
	char vps[512] = { 0 }, sps[512] = { 0 }, pps[128] = { 0 };
	int vpslen = 0, spslen = 0, ppslen = 0, i = 0, iStartPos = 0, ret = -1;
	int iFoundVPS = 0, iFoundSPS = 0, iFoundPPS = 0, iFoundSEI = 0;
	if (NULL == pbuf || bufsize < 4)	return -1;

	for (i = 0; i < bufsize; i++)
	{
		if ((unsigned char)pbuf[i] == 0x00 && (unsigned char)pbuf[i + 1] == 0x00 &&
			(unsigned char)pbuf[i + 2] == 0x00 && (unsigned char)pbuf[i + 3] == 0x01)
		{
			printf("0x%X\n", (unsigned char)pbuf[i + 4]);
			switch ((unsigned char)pbuf[i + 4])
			{
			case 0x40:		//VPS
			{
				iFoundVPS = 1;
				iStartPos = i + 4;
			}
			break;
			case 0x42:		//SPS
			{
				if (iFoundVPS == 0x01 && i > 4)
				{
					vpslen = i - 4 - iStartPos;
					if (vpslen > 256)	return -1;          //vps长度超出范围
					memset(vps, 0x00, sizeof(vps));
					memcpy(vps, pbuf + iStartPos, vpslen);
				}

				iStartPos = i + 4;
				iFoundSPS = 1;
			}
			break;
			case 0x44:		//PPS
			{
				if (iFoundSPS == 0x01 && i > 4)
				{
					spslen = i - 4 - iStartPos;
					if (spslen > 256)	return -1;
					memset(sps, 0x0, sizeof(sps));
					memcpy(sps, pbuf + iStartPos, spslen);
				}

				iStartPos = i + 4;
				iFoundPPS = 1;
			}
			break;
			case 0x4E:		//SEI
			case 0x50:
			{
				if (iFoundPPS == 0x01 && i > 4)
				{
					ppslen = i - 4 - iStartPos;
					if (ppslen > 256)	return -1;
					memset(pps, 0x0, sizeof(pps));
					memcpy(pps, pbuf + iStartPos, ppslen);
				}
				iStartPos = i + 4;
				iFoundSEI = 1;
			}
			break;
			default:
				break;
			}
		}

		if (iFoundSEI == 0x01)		break;
	}


	if (iFoundVPS == 0x01)
	{
		if (vpslen < 1)
		{
			if (bufsize < sizeof(vps))
			{
				vpslen = bufsize - 4;
				memset(vps, 0x00, sizeof(vps));
				memcpy(vps, pbuf + 4, vpslen);
			}
		}

		if (vpslen > 0)
		{
			if (NULL != _vps)   memcpy(_vps, vps, vpslen);
			if (NULL != _vpslen)    *_vpslen = vpslen;
		}

		ret = 0;
	}


	if (iFoundSPS == 0x01)
	{
		if (spslen < 1)
		{
			if (bufsize < sizeof(sps))
			{
				spslen = bufsize - 4;
				memset(sps, 0x00, sizeof(sps));
				memcpy(sps, pbuf + 4, spslen);
			}
		}

		if (spslen > 0)
		{
			if (NULL != _sps)   memcpy(_sps, sps, spslen);
			if (NULL != _spslen)    *_spslen = spslen;
		}

		ret = 0;
	}

	if (iFoundPPS == 0x01)
	{
		if (ppslen < 1)
		{
			if (bufsize < sizeof(pps))
			{
				ppslen = bufsize - 4;
				memset(pps, 0x00, sizeof(pps));
				memcpy(pps, pbuf + 4, ppslen);	//pps
			}
		}
		if (ppslen > 0)
		{
			if (NULL != _pps)   memcpy(_pps, pps, ppslen);
			if (NULL != _ppslen)    *_ppslen = ppslen;
		}
		ret = 0;
	}

	return ret;
}


//int GetVPSandSPSandPPS(char *pbuf, int bufsize, char *_vps, int *_vpslen, char *_sps, int *_spslen, char *_pps, int *_ppslen)
int GetH264SPSandPPS(char *pbuf, int bufsize, char *_sps, int *_spslen, char *_pps, int *_ppslen)
{
	char sps[512] = { 0 }, pps[256] = { 0 };
	int spslen = 0, ppslen = 0, i = 0, iStartPos = 0, ret = -1;
	int iFoundSPS = 0, iFoundPPS = 0, iFoundNext = 0;
	if (NULL == pbuf || bufsize < 8)	return -1;

	for (i = 0; i < bufsize; i++)
	{
		if ((unsigned char)pbuf[i] == 0x00 && (unsigned char)pbuf[i + 1] == 0x00 &&
			(unsigned char)pbuf[i + 2] == 0x00 && (unsigned char)pbuf[i + 3] == 0x01)
		{
			unsigned char naltype = ((unsigned char)pbuf[i + 4] & 0x1F);
			if (naltype == 7)       //sps
			{
				iFoundSPS = 1;
				iStartPos = i + 4;
			}
			else if (naltype == 8)	//pps
			{
				//copy sps
				if (iFoundSPS == 1 && i > 4)
				{
					spslen = i - iStartPos;
					if (spslen > 256)	return -1;          //sps长度超出范围
					if (spslen > 0 && spslen < sizeof(sps))
					{
						memset(sps, 0x00, sizeof(sps));
						memcpy(sps, pbuf + iStartPos, spslen);
					}
				}
				iStartPos = i + 4;

				iFoundPPS = 1;
			}
			else if (iFoundPPS)	//
			{
				iFoundNext = 1;
				ppslen = i - iStartPos;
				if (ppslen > 0 && ppslen < sizeof(pps))
				{
					memset(pps, 0x00, sizeof(pps));
					memcpy(pps, pbuf + iStartPos, ppslen);	//pps
				}
				break;
			}
		}
		else
		{

		}
	}

	if (iFoundSPS == 1 && iFoundPPS == 0 && iFoundNext == 0)
	{
		spslen = i - iStartPos;
		if (spslen > 256)	return -1;          //sps长度超出范围
		if (spslen > 0 && spslen < sizeof(sps))
		{
			memset(sps, 0x00, sizeof(sps));
			memcpy(sps, pbuf + iStartPos, spslen);
		}
	}

	if (iFoundPPS == 1 && iFoundNext == 0)
	{
		ppslen = i - iStartPos;
		if (ppslen > 0 && ppslen < sizeof(pps))
		{
			memset(pps, 0x00, sizeof(pps));
			memcpy(pps, pbuf + iStartPos, ppslen);	//pps
		}
	}

	if (spslen > 0)
	{
		if (NULL != _sps)   memcpy(_sps, sps, spslen);
		if (NULL != _spslen)    *_spslen = spslen;

		ret = 0;
	}
	if (ppslen > 0)
	{
		if (NULL != _pps)   memcpy(_pps, pps, ppslen);
		if (NULL != _ppslen)    *_ppslen = ppslen;

		ret = 0;
	}

	return ret;
}
