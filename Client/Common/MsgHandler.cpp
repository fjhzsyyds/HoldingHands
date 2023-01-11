#include "MsgHandler.h"
#include "IOCPClient.h"
#include "Packet.h"
#include "..\Common\Manager.h"

CMsgHandler::CMsgHandler(CManager*pManager,DWORD dwIdentity):
	m_pManager(pManager),
	m_Identity(dwIdentity)
{
}

CMsgHandler::~CMsgHandler(){
}

BOOL CMsgHandler::SendMsg(WORD Msg,void *data, int len,BOOL Compress){
	return m_pManager->SendMsg(Msg, (char*)data, len, Compress);
}

void CMsgHandler::Close(){
	m_pManager->Close();
}

const pair<string, unsigned short> CMsgHandler::GetPeerName(){
	return m_pManager->GetPeerName();
}

const pair<string, unsigned short> CMsgHandler::GetSockName(){
	return m_pManager->GetSockName();
}

void CMsgHandler::OnClose(){
}

void CMsgHandler::OnOpen(){
}
