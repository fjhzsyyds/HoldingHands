#include "IOCPClient.h"
#include "SocksProxy.h"
#include "Manager.h"
#include "utils.h"

extern "C" __declspec(dllexport) void  ModuleEntry(char* szServerAddr, 
	unsigned short uPort, DWORD dwParam){

	CIOCPClient *pClient = new CIOCPClient(szServerAddr, uPort);

	CMsgHandler *pHandler = new CSocksProxy(pClient);

	pClient->Run();

	delete pHandler;
	delete pClient;
}

#ifdef _DEBUG
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
	ModuleEntry("192.168.0.110", 10086, 0);
	//ModuleEntry("49.235.129.40", 8081, 0);
	CIOCPClient::SocketTerm();
	return 0;
}
#endif