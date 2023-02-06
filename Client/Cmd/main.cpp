#include "IOCPClient.h"
#include "Cmd.h"

extern "C" __declspec(dllexport) void  ModuleEntry(char* szServerAddr,
	unsigned short uPort, DWORD dwParam = 0){
	CIOCPClient *pClient = new CIOCPClient(szServerAddr, uPort);

	CMsgHandler *pHandler = new CCmd(pClient);

	pClient->Run();

	delete pHandler;
	delete pClient;
}

#ifdef _DEBUG
int main(){
	CIOCPClient::SocketInit();
	ModuleEntry("127.0.0.1", 10086);
	CIOCPClient::SocketTerm();
}
#endif