#ifndef _UTILS_H_
#define _UIILS_H_
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef UNICODE
#define _t_sprintf	swprintf
#else
#define _t_sprintf	sprintf
#endif


BOOL MakesureDirExist(const TCHAR* Path, BOOL bIncludeFileName);

void GetStorageSizeString(LARGE_INTEGER Bytes, TCHAR* strBuffer);

char* convertUTF8ToGB2312(const char* utf8);

char* convertGB2312ToUTF8(const char* gb2312);

char* convertUtf16ToGB2312(const wchar_t* utf16);

wchar_t* convertGB2312ToUtf16(const char* gb2312);

void GetProcessDirectory(TCHAR* szPath);

#endif