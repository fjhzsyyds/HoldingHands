#include "stdafx.h"
#include "MsgHandler.h"
#include "IOCPServer.h"
#include "Packet.h"
#include "ClientContext.h"
#include "Manager.h"

CMsgHandler::CMsgHandler(CManager*pManager) :
m_pManager(pManager){
}

CMsgHandler::~CMsgHandler(){

}

BOOL CMsgHandler::SendMsg(WORD Msg,void*data, int len)
{
	return m_pManager->SendMsg(this, Msg, (char*)data,len);
}

void CMsgHandler::Close(){
	m_pManager->Close(this);
}

const pair<string, unsigned short> CMsgHandler::GetPeerName(){

	return m_pManager->GetPeerName(this);
}

const pair<string, unsigned short> CMsgHandler::GetSockName(){
	return m_pManager->GetSockName(this);
}

 void CMsgHandler::OnClose(){

}

 void CMsgHandler::OnOpen(){

}
