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
	ModuleEntry("192.168.0.110", 10086);
	CIOCPClient::SocketTerm(); 
}
#endif