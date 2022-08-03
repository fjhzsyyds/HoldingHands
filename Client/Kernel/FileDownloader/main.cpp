#include "..\FileManager\FileDownloader.h"
#include "IOCPClient.h"

extern "C" __declspec(dllexport) void  ModuleEntry(char* szServerAddr, unsigned short uPort, DWORD dwParam){
	CFileDownloader downloader((CFileDownloader::InitParam*)dwParam);
	CIOCPClient io(szServerAddr, uPort);

	io.BindHandler(&downloader);
	io.Run();
	io.UnbindHandler();

}