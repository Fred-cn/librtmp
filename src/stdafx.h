// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#if (defined(WIN32) || defined(WIN64))
#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头中排除极少使用的资料
// Windows 头文件:
#include <windows.h>
#pragma warning(disable: 4996)
#endif


#ifndef RTMPCLIENTSDK_EXPORTS
#define RTMPCLIENTSDK_EXPORTS
#endif



