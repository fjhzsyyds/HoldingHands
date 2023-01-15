#include "stdafx.h"
#include "SocksProxyWnd.h"
#include "SocksProxySrv.h"
#include "SocksProxyAddrDlg.h"

CSocksProxyWnd::CSocksProxyWnd(CSocksProxySrv*pHandler) :
m_pHandler(pHandler),
m_DestroyAfterDisconnect(FALSE)
{
	m_IsRunning = FALSE;
	auto const peer = m_pHandler->GetPeerName();
	m_IP = CA2W(peer.first.c_str());
}


CSocksProxyWnd::~CSocksProxyWnd()
{
}

BEGIN_MESSAGE_MAP(CSocksProxyWnd, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_MESSAGE(WM_SOCKS_PROXY_LOG,OnLog)
	ON_MESSAGE(WM_SOCKS_PROXY_ERROR, OnError)
	ON_COMMAND(ID_VER_SOCKS4, &CSocksProxyWnd::OnVerSocks4)
	ON_COMMAND(ID_VER_SOCKS5, &CSocksProxyWnd::OnVerSocks5)
	ON_COMMAND(ID_MAIN_STARTPROXY, &CSocksProxyWnd::OnMainStartproxy)
	ON_COMMAND(ID_MAIN_STOP, &CSocksProxyWnd::OnMainStop)
	ON_UPDATE_COMMAND_UI(ID_MAIN_STARTPROXY, &CSocksProxyWnd::OnUpdateMainStartproxy)
	ON_UPDATE_COMMAND_UI(ID_MAIN_STOP, &CSocksProxyWnd::OnUpdateMainStop)
	ON_WM_TIMER()
END_MESSAGE_MAP()

#define SOCKS_PROXY_ID_LOG	100

int CSocksProxyWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rect;
	CString Text;
	m_Menu.LoadMenuW(IDR_SOCKS_PROXY_MENU);
	SetMenu(&m_Menu);
	//
	m_Font.CreateFont(18, 9, 0, 0, FW_REGULAR, FALSE, FALSE,
		0, DEFAULT_CHARSET, DEFAULT_CHARSET,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SCRIPT, TEXT("Consolas"));

	//默认是Socks5
	CMenu*pMenu = m_Menu.GetSubMenu(1);
	pMenu->CheckMenuRadioItem(ID_VER_SOCKS5, ID_VER_SOCKS4,
		ID_VER_SOCKS5, MF_BYCOMMAND);
	
	GetClientRect(&rect);
	m_Log.Create(WS_CHILD | WS_VISIBLE | ES_MULTILINE
		| ES_AUTOHSCROLL | WS_VSCROLL|ES_READONLY,
		rect, this, SOCKS_PROXY_ID_LOG);

	m_Log.LimitText(MAXDWORD);
	m_Log.SetFont(&m_Font);
	//
	
	Text.Format(TEXT("[%s] SocksProxy"), m_IP.GetBuffer());
	SetWindowText(Text);
	//
	GetWindowRect(rect);
	rect.right = rect.left + 670;
	rect.bottom = rect.top + 400;
	MoveWindow(rect);
	return 0;
}

LRESULT CSocksProxyWnd::OnLog(WPARAM wParam, LPARAM lParam){
	TCHAR* szLog = (TCHAR*)wParam;
	DWORD Length = m_Log.GetWindowTextLengthW();
	m_Log.SetSel(Length, Length);
	m_Log.ReplaceSel(szLog);
	return 0;
}

LRESULT CSocksProxyWnd::OnError(WPARAM wParam, LPARAM lParam){
	TCHAR* szError = (TCHAR*)wParam;
	MessageBox(szError, TEXT("Tips"), MB_OK | MB_ICONINFORMATION);
	return 0;
}

void CSocksProxyWnd::OnClose()
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (m_pHandler){
		m_DestroyAfterDisconnect = TRUE;
		m_pHandler->Close();
	}
	else{
		//m_pHandler已经没了,现在只管自己就行.
		DestroyWindow();
	}
}


void CSocksProxyWnd::PostNcDestroy()
{
	if (!m_DestroyAfterDisconnect){
		delete this;
	}
}


void CSocksProxyWnd::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);
	RECT rect;

	if (m_hWnd && m_Log.m_hWnd){	
		GetClientRect(&rect);
		m_Log.MoveWindow(&rect);
	}
}


void CSocksProxyWnd::OnVerSocks4()
{
	if (m_pHandler){
		m_pHandler->SetSocksVersion(0x4);

		GetMenu()->GetSubMenu(1)->
			CheckMenuRadioItem(ID_VER_SOCKS5, ID_VER_SOCKS4,
			ID_VER_SOCKS4, MF_BYCOMMAND);
	}
}


void CSocksProxyWnd::OnVerSocks5()
{
	if (m_pHandler){
		m_pHandler->SetSocksVersion(0x5);

		GetMenu()->GetSubMenu(1)->
			CheckMenuRadioItem(ID_VER_SOCKS5, ID_VER_SOCKS4,
			ID_VER_SOCKS5, MF_BYCOMMAND);
	}
}


void CSocksProxyWnd::OnMainStartproxy()
{
	CSocksProxyAddrDlg dlg;
	if (dlg.DoModal() == IDOK){

		DWORD Addr = dlg.m_Addr;
		DWORD Port = dlg.m_Port;
		if (m_pHandler){
			if (m_pHandler->StartProxyServer(Port, Addr)){
				MessageBox(TEXT("Start Proxy Server Success!"),
					TEXT("Tips"), MB_OK | MB_ICONINFORMATION);
				
				m_IsRunning = TRUE;
				m_BindPort = Port;
				m_BindAddress = CA2W(inet_ntoa(*(in_addr*)&Addr));
				SetTimer(10086, 1000, 0);
				return;
			}
		}

		MessageBox(TEXT("Start Proxy Server Failed!"),
			TEXT("Tips"), MB_OK | MB_ICONINFORMATION);
	}
}


void CSocksProxyWnd::OnMainStop()
{
	if (m_IsRunning){
		if (m_pHandler){
			m_pHandler->StopProxyServer();
			MessageBox(TEXT("Proxy server has been stopped!"),
				TEXT("Tips"), MB_OK | MB_ICONINFORMATION);

			CString Text;
			Text.Format(TEXT("[%s] SocksProxy"), m_IP.GetBuffer());
			SetWindowText(Text);

			m_IsRunning = FALSE;
			KillTimer(10086);
		}
	}
}


void CSocksProxyWnd::OnUpdateMainStartproxy(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!m_IsRunning);
}


void CSocksProxyWnd::OnUpdateMainStop(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_IsRunning);
}


void CSocksProxyWnd::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (m_pHandler){
		CString Text;
		DWORD Connections = m_pHandler->GetConnections();
		Text.Format(TEXT("[%s] SocksProxy - Listen: %s.%d Clients:%d"),
			m_IP.GetBuffer(), m_BindAddress.GetBuffer(),
			m_BindPort,Connections);

		SetWindowText(Text);
	}
	CFrameWnd::OnTimer(nIDEvent);
}
