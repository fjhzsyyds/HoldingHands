#include "stdafx.h"
#include "utils.h"

BOOL MakesureDirExist(const TCHAR* Path, BOOL bIncludeFileName = FALSE)
{
	TCHAR*pTempDir = (TCHAR*)malloc((lstrlen(Path) + 1)*sizeof(TCHAR));
	lstrcpy(pTempDir, Path);
	BOOL bResult = FALSE;
	TCHAR* pIt = NULL;
	//找到文件名.;
	if (bIncludeFileName){
		pIt = pTempDir + lstrlen(pTempDir) - 1;
		while (pIt[0] != '\\' && pIt[0] != '/' && pIt > pTempDir) pIt--;
		if (pIt[0] != '/' && pIt[0] != '\\')
			goto Return;
		//'/' ---> 0
		pIt[0] = 0;
	}
	//找到':';
	if ((pIt = wcsstr(pTempDir, L":")) == NULL || (pIt[1] != '\\' && pIt[1] != '/'))
		goto Return;
	pIt++;

	while (pIt[0]){
		TCHAR oldCh;
		//跳过'/'或'\\';
		while (pIt[0] && (pIt[0] == '\\' || pIt[0] == '/'))
			pIt++;
		//找到结尾.;
		while (pIt[0] && (pIt[0] != '\\' && pIt[0] != '/'))
			pIt++;
		//
		oldCh = pIt[0];
		pIt[0] = 0;

		if (!CreateDirectoryW(pTempDir, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
			goto Return;

		pIt[0] = oldCh;
	}
	bResult = TRUE;
Return:
	free(pTempDir);
	return bResult;
}

static TCHAR* MemUnits[] =
{
	TEXT("B"),
	TEXT("KB"),
	TEXT("MB"),
	TEXT("GB"),
	TEXT("TB")
};

void GetStorageSizeString(LARGE_INTEGER Bytes, TCHAR* strBuffer){

	double bytes = Bytes.QuadPart;
	int MemUnitIdx = 0;

	while (bytes > 1024 && MemUnitIdx <
		((sizeof(MemUnits) / sizeof(MemUnits[0])) - 1)
		){
		MemUnitIdx++;
		bytes /= 1000;
	}

	_t_sprintf(strBuffer, TEXT("%.2lf %s"), bytes, MemUnits[MemUnitIdx]);
}


#include <Tlhelp32.h.>


DWORD getProcessId(TCHAR* szProcessName){
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof PROCESSENTRY32;
	DWORD pid = 0;

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	bool bFind = Process32First(hSnap, &pe);
	while (bFind) {
		if (!lstrcmpi(pe.szExeFile, szProcessName)){
			pid = pe.th32ProcessID;
			break;
		}
		bFind = Process32Next(hSnap, &pe);
	}
	CloseHandle(hSnap);
	return pid;
}

DWORD getParentProcessId(TCHAR* szProcessName){
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof PROCESSENTRY32;
	DWORD ppid = 0;
	DWORD pid = getProcessId(szProcessName);

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	bool bFind = Process32First(hSnap, &pe);
	while (bFind) {
		if (pe.th32ProcessID == pid){
			ppid = pe.th32ParentProcessID;
			break;
		}
		bFind = Process32Next(hSnap, &pe);
	}
	CloseHandle(hSnap);
	return ppid;
}


DWORD getParentProcessId(DWORD dwPid){
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof PROCESSENTRY32;
	DWORD ppid = 0;

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	bool bFind = Process32First(hSnap, &pe);
	while (bFind) {
		if (pe.th32ProcessID == dwPid){
			ppid = pe.th32ParentProcessID;
			break;
		}
		bFind = Process32Next(hSnap, &pe);
	}
	CloseHandle(hSnap);
	return ppid;
}


char* convertUTF8ToAnsi(const char* utf8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);
	len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);
	if (wstr) delete[] wstr;
	return str;
}

char* convertAnsiToUTF8(const char* gb2312)
{
	int len = MultiByteToWideChar(CP_ACP, 0, gb2312, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_ACP, 0, gb2312, -1, wstr, len);
	len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
	if (wstr) delete[] wstr;
	return str;
}


wchar_t* convertAnsiToUtf16(const char* gb2312){
	int len = MultiByteToWideChar(CP_ACP, 0, gb2312, -1, NULL, 0);
	wchar_t * wstr = new wchar_t[len + 1];
	MultiByteToWideChar(CP_ACP, 0, gb2312, -1, wstr, len);
	return wstr;
}

char* convertUtf16ToAnsi(const wchar_t* utf16){
	int len = WideCharToMultiByte(CP_ACP, 0, utf16, -1, NULL, 0, NULL, NULL);
	char * str = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, utf16, -1, str, len, NULL, NULL);
	return str;
}

/* 获取exe 所在的目录 */
void GetProcessDirectory(TCHAR* szPath){
	HMODULE hMod = GetModuleHandle(0);
	TCHAR szModuleFileName[MAX_PATH];
	TCHAR * it = nullptr;
	szPath[0] = '\0';

	GetModuleFileName(hMod, szModuleFileName, MAX_PATH);
	it = szModuleFileName + lstrlen(szModuleFileName) - 1;

	while (it >= szModuleFileName && *it != '\\'){
		--it;
	}
	if (it >= szModuleFileName){
		*it = '\0';
		lstrcpy(szPath, szModuleFileName);
	}
}


#ifdef _DEBUG
#include <stdio.h>
#include <stdarg.h>

void dbg_log(const char * format, ...){
	char buffer[0x2000];
	char date[0x100];
	char log[0x1f00];

	va_list list;
	va_start(list, format);
	vsnprintf(log, 0x1f00, format, list);
	va_end(list);


	time_t t = time(NULL);
	tm*pTm = localtime(&t);
	sprintf(date, "[%d/%d/%d %d:%d:%d] ", 1900 + pTm->tm_year, 1 + pTm->tm_mon,
		pTm->tm_mday, pTm->tm_hour, pTm->tm_min, pTm->tm_sec);

	sprintf(buffer, "%s %s \r\n", date, log);

	HANDLE hFile = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwWriteBytes = 0;

	WriteFile(hFile, buffer, strlen(buffer), &dwWriteBytes, 0);
}
#else
void dbg_log(const char * format, ...){

}
#endif