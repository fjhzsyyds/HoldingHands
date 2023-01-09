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
DWORD getProcessId(TCHAR* szProcessName);
DWORD getParentProcessId(TCHAR* szProcessName);
DWORD getParentProcessId(DWORD dwPid);

char* convertUTF8ToAnsi(const char* utf8);
char* convertAnsiToUTF8(const char* gb2312);
char* convertUtf16ToAnsi(const wchar_t* utf16);
wchar_t* convertAnsiToUtf16(const char* gb2312);
void GetProcessDirectory(TCHAR* szPath);;
#endif