#include "stdafx.h"
#include "SocksProxySrv.h"
#include "IOCPProxyServer.h"

#include "utils.h"
#include "SocksProxyWnd.h"

CSocksProxySrv::CSocksProxySrv(CClientContext*pClient) :
CMsgHandler(pClient)
{
	m_ClientID = 0;
	m_pServer = NULL;
	memset(m_Clients, 0, sizeof(m_Clients));
}


CSocksProxySrv::~CSocksProxySrv()
{

}


void CSocksProxySrv::OnClose(){
	if (m_pServer){
		delete m_pServer;
		m_pServer = NULL;
	}

	if (m_pWnd){
		if (m_pWnd->m_DestroyAfterDisconnect){
			//窗口先关闭的.
			m_pWnd->DestroyWindow();
			delete m_pWnd;
		}
		else{
			// pHandler先关闭的,那么就不管窗口了
			m_pWnd->m_pHandler = nullptr;
			m_pWnd->PostMessage(WM_SOCKS_PROXY_ERROR, (WPARAM)TEXT("Disconnect."));
		}
		m_pWnd = nullptr;
	}
	return;
}

void CSocksProxySrv::OnOpen(){
	m_pServer = new CIOCPProxyServer(this);
	m_pServer->SetSocksVer(0x5);
	
	m_pWnd = new CSocksProxyWnd(this);
	ASSERT(m_pWnd->Create(NULL,NULL));
	m_pWnd->ShowWindow(SW_SHOW);
}

DWORD CSocksProxySrv::GetConnections(){
	return m_pServer->GetConnections();
}

void CSocksProxySrv::Disconnect(DWORD ClientID){
	m_pServer->Disconnect(ClientID);
}

void CSocksProxySrv::UpdateInfo(void * param1, void * param2){
	m_pWnd->SendMessage(WM_SOCKS_PROXY_UPDATE, (WPARAM)param1, (LPARAM)param2);
}

void CSocksProxySrv::OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer){
	switch (Msg)
	{
	case SOCK_PROXY_REQUEST_RESULT:
		OnRemoteResponse((DWORD*)Buffer);
		break;
	case SOCK_PROXY_DATA:
		OnRemoteData(Buffer, dwSize);
		break;
	case SOCK_PROXY_CLOSE:
		OnRemoteClose(*(DWORD*)Buffer);
		break;
	default:
		break;
	}

}

void CSocksProxySrv::StopProxyServer(){
	m_pServer->StopServer();
}

BOOL CSocksProxySrv::StartProxyServer(UINT Port,DWORD Address,DWORD UDPAssociateAddr){
	return m_pServer->StartServer(Port, Address, UDPAssociateAddr);
}

void CSocksProxySrv::SetSocksVersion(BYTE Version){
	m_pServer->SetSocksVer(Version);
}

void CSocksProxySrv::OnRemoteClose(DWORD ID){
	m_pServer->OnRemoteClose(ID);
}

void CSocksProxySrv::OnRemoteData(char * Buffer, DWORD Size){
	DWORD ClientID = *(DWORD*)Buffer;
	m_pServer->OnRemoteData(ClientID, Buffer + 4, Size - 4);
}

void CSocksProxySrv::OnRemoteResponse(DWORD * Response){
	DWORD ClientID = Response[0];
	DWORD Error = Response[1];
	m_pServer->OnRequestResponse(ClientID, Error);
}

UINT CSocksProxySrv::AllocClientID(){
	DWORD Used = 0;
	DWORD Result = 0;

	while (Used < MAX_CLIENT_COUNT){
		if (!m_Clients[m_ClientID]){
			m_Clients[m_ClientID] = TRUE;
			Result = m_ClientID;
			SendMsg(SOCK_PROXY_ADD_CLIENT, &m_ClientID, sizeof(&m_ClientID));
			return Result;
		}
		Used++;
		m_ClientID = (m_ClientID + 1) % MAX_CLIENT_COUNT;
	}
	dbg_log("Alloc ClientID Failed");
	return -1;
}