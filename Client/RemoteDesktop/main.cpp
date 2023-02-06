#include "IOCPClient.h"
#include "RemoteDesktop.h"
#include "Manager.h"


extern "C" __declspec(dllexport) void  ModuleEntry(char* szServerAddr, unsigned short uPort, DWORD dwParam){

	CIOCPClient *pClient = new CIOCPClient(
		szServerAddr, uPort);

	CMsgHandler *pHandler = new CRemoteDesktop(pClient);

	pClient->Run();

	delete pHandler;
	delete pClient;
}

#ifdef _DEBUG
int main(){
	CIOCPClient::SocketInit();
	ModuleEntry("49.235.129.40", 10086, 0);
	CIOCPClient::SocketTerm();
	return 0;
}
#endif