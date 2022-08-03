#include "Chat.h"
#include "IOCPClient.h"

extern "C" __declspec(dllexport) void  ModuleEntry(char* szServerAddr, unsigned short uPort, DWORD dwParam){
	CChat chat;
	CIOCPClient io(szServerAddr, uPort);

	io.BindHandler(&chat);
	io.Run();
	io.UnbindHandler();
}