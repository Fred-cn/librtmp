
#pragma once

typedef unsigned short int uint16;
typedef unsigned long int uint32;


#define HTON24(x)   ((x >> 16 & 0xff) | (x <<16 & 0xff0000) |(x & 0xff00))
#define HTONTIME(x) (HTON24(x) | (x & 0xff000000))

#ifndef ntoh24
#define ntoh24(p) (((p)[0] << 16) | ((p)[1] << 8) | ((p)[2]))
#endif


// 本机大端返回1，小端返回0
int checkCPUendian()
{
	union {
		unsigned long int i;
		unsigned char s[4];
	}c;

	c.i = 0x12345678;
	return (0x12 == c.s[0]);
}

// 模拟htons函数，本机字节序转网络字节序
unsigned short int t_htons(unsigned short int h)
{
	// 若本机为大端，与网络字节序同，直接返回
	// 若本机为小端，转换成大端再返回
	return checkCPUendian() ? h : BigLittleSwap16(h);
}

// 模拟ntohs函数，网络字节序转本机字节序
unsigned short int t_ntohs(unsigned short int n)
{
	// 若本机为大端，与网络字节序同，直接返回
	// 若本机为小端，网络数据转换成小端再返回
	return checkCPUendian() ? n : BigLittleSwap16(n);
}

// 模拟htonl函数，本机字节序转网络字节序
unsigned long int t_htonl(unsigned long int h)
{
	// 若本机为大端，与网络字节序同，直接返回
	// 若本机为小端，转换成大端再返回
	return checkCPUendian() ? h : BigLittleSwap32(h);
}

// 模拟ntohl函数，网络字节序转本机字节序
unsigned long int t_ntohl(unsigned long int n)
{
	// 若本机为大端，与网络字节序同，直接返回
	// 若本机为小端，网络数据转换成小端再返回
	return checkCPUendian() ? n : BigLittleSwap32(n);
}


bool Peek8(int &i8, char* &f)
{
	if (!f)
		return false;
	memcpy(&i8, f, 1);

	return true;
}

bool Read8(int &i8, char* &f)
{
	if (!f)
		return false;
	memcpy(&i8, f, 1);
	f += 1;
	return true;
}
bool Read16(int &i16, char* &f)
{
	if (!f)
		return false;
	memcpy(&i16, f, 2);
	i16 = t_ntohs(i16);
	f += 2;
	return true;
}
bool Read24(int &i24, char* &f)
{
	if (!f)
		return false;

	char p[3];
	memcpy(p, f, 3);
	i24 = ntoh24(p);
	f += 3;
	return true;
}
bool Read32(int &i32, char* &f)
{
	if (!f)
		return false;

	memcpy(&i32, f, 4);
	i32 = t_ntohl(i32);
	f += 4;
	return true;
}

bool ReadTime(int &itime, char* &f)
{
	if (!f)
		return false;

	int temp = 0;
	memcpy(&temp, f, 4);
	itime = HTON24(temp);
	itime |= (temp & 0xff000000);
	f += 4;
	return true;
}
