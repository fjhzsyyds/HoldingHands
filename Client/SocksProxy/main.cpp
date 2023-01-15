#include "IOCPClient.h"
#include "SocksProxy.h"
#include "Manager.h"
#include "utils.h"

extern "C" __declspec(dllexport) void  ModuleEntry(char* szServerAddr, unsigned short uPort, DWORD dwParam){
	CManager *pManager = new CManager();

	CIOCPClient *pClient = new CIOCPClient(pManager,
		szServerAddr, uPort);

	CMsgHandler *pHandler = new CSocksProxy(pManager);

	pManager->Associate(pClient, pHandler);
	pClient->Run();

	delete pHandler;
	delete pClient;
	delete pManager;
}


LONG WINAPI Filter(
	_In_ struct _EXCEPTION_POINTERS *ExceptionInfo
	){
	dbg_log("eip: 0x%p\n", ExceptionInfo->ContextRecord->Eip);
	dbg_log("ecx: 0x%p\n", ExceptionInfo->ContextRecord->Ecx);
	dbg_log("eax: 0x%p\n", ExceptionInfo->ContextRecord->Eax);		//Msg
	ExitProcess(0);
}


int main(){
	SetUnhandledExceptionFilter(Filter);
	CIOCPClient::SocketInit();
	ModuleEntry("127.0.0.1", 10086, 0);
	//ModuleEntry("49.235.129.40", 10086, 0);
	//CIOCPClient::SocketTerm();
	return 0;
}
