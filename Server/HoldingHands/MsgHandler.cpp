#include "stdafx.h"
#include "MsgHandler.h"
#include "IOCPServer.h"
#include "Packet.h"
#include "ClientContext.h"
#include "Manager.h"

CMsgHandler::CMsgHandler(CClientContext*pClient) :
m_pClient(pClient){
	
	m_pClient->SetCallbackHandler(this);
}

CMsgHandler::~CMsgHandler(){

}


void CMsgHandler::Close(){
	m_pClient->Disconnect();
}


BOOL CMsgHandler::SendMsg(WORD Msg, void *data, int len, BOOL Compress){
	if (BeginMsg(Msg)){
		WriteBytes((BYTE*)data, len);
		EndMsg(Compress);
		return TRUE;
	}
	return FALSE;
}

BOOL CMsgHandler::BeginMsg(WORD Msg){
	if (m_pClient->BeginWrite()){
		WriteWord(Msg);
		return TRUE;
	}
	return FALSE;
}

void CMsgHandler::EndMsg(BOOL Compress){
	m_pClient->EndWrite(Compress);
}


const pair<string, unsigned short> CMsgHandler::GetPeerName(){
	SOCKET s = m_pClient->GetSocket();
	SOCKADDR_IN addr = { 0 };
	int namelen = sizeof(addr);
	getpeername(s, (SOCKADDR*)&addr, &namelen);

	return pair<string, unsigned short>(inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
}

const pair<string, unsigned short> CMsgHandler::GetSockName(){
	SOCKET s = m_pClient->GetSocket();
	SOCKADDR_IN addr = { 0 };
	int namelen = sizeof(addr);
	getsockname(s, (SOCKADDR*)&addr, &namelen);

	return pair<string, unsigned short>(inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
}

void CMsgHandler::OnRead(BYTE* lpData, UINT Size){
	WORD Msg = *(WORD*)lpData;
	OnReadMsg(Msg, Size - sizeof(WORD), (char*)lpData + sizeof(WORD));
}

void CMsgHandler::OnWrite(BYTE*lpData, UINT Size){
	///
}

void CMsgHandler::OnConncet(){
	OnOpen();
}
void CMsgHandler::OnDisconnect(){
	OnClose();
}

