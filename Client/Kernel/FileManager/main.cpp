#include "FileManager.h"
#include "IOCPClient.h"

extern "C" __declspec(dllexport) void  ModuleEntry(char* szServerAddr, unsigned short uPort, DWORD dwParam){
	CFileManager filemgr((CModuleMgr*)dwParam);
	CIOCPClient io(szServerAddr, uPort);

	io.BindHandler(&filemgr);
	io.Run();
	io.UnbindHandler();

}