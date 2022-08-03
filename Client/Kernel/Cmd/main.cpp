#include "Cmd.h"
#include "IOCPClient.h"

extern "C" __declspec(dllexport) void  ModuleEntry(char* szServerAddr, unsigned short uPort, DWORD dwParam){
	CCmd cmd;
	CIOCPClient io(szServerAddr,uPort);

	io.BindHandler(&cmd);
	io.Run();
	io.UnbindHandler();

}