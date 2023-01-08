

#include "..\Common\IOCPClient.h"
#include "..\Common\zlib\zlib.h"
#include "..\Common\MsgHandler.h"
#include "..\Common\Manager.h"

#include <iostream>


#define ENABLE_COMPRESS		1
#define MSG_COMPRESS		0x80000000

#ifdef DEBUG
#pragma comment(lib,"..\\Common\\zlibd.lib")
#else
#pragma comment(lib,"..\\Common\\zlib.lib")
#endif

CManager::CManager():
m_pClient(nullptr),
m_pMsgHandler(nullptr){
	
}

CManager::~CManager(){
	
	
}

const pair<string, unsigned short> CManager::GetPeerName(){
	SOCKADDR_IN  addr = { 0 };
	int namelen = sizeof(addr);
	getpeername(m_pClient->m_ClientSocket, (sockaddr*)&addr, &namelen);
	return pair<string, unsigned short>(inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
}
const pair<string, unsigned short> CManager::GetSockName(){
	SOCKADDR_IN  addr = { 0 };
	int namelen = sizeof(addr);
	getsockname(m_pClient->m_ClientSocket, (sockaddr*)&addr, &namelen);
	return pair<string, unsigned short>(inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
}

void CManager::Close(){
	m_pClient->Disconnect();
}

BOOL CManager::SendMsg(WORD Msg, char*data, size_t len){
	CIOCPClient *pClient = m_pClient;
	Byte * source = (Byte*)data;
	uLong source_len = len;
	DWORD dwFlag = 0;

	if (ENABLE_COMPRESS){
		uLong bufferSize = compressBound(source_len);
		Byte * compreeBuffer = new Byte[bufferSize];

		int err = Z_OK;
		err = compress(compreeBuffer, &bufferSize, source, source_len);
		if (err == S_OK){
			
			dwFlag = source_len;				//保存原始长度
			dwFlag |= MSG_COMPRESS;

			source = compreeBuffer;
			source_len = bufferSize;

		}
		else{
			//失败的话就不压缩....
			delete[] compreeBuffer;
		}

	}

	pClient->SendPacket(Msg, (const char*)source, source_len, dwFlag);
	if (dwFlag & MSG_COMPRESS){
		delete[] source;
	}	
	return TRUE;
}

void CManager::DispatchMsg(CPacket*pPkt){
	CMsgHandler * pHandler = m_pMsgHandler;

	WORD Msg = pPkt->GetCommand();
	Byte * source = (Byte*)pPkt->GetBody();
	uLong source_len = pPkt->GetBodyLen();
	DWORD dwFlag = pPkt->GetFlag();

	if (dwFlag & MSG_COMPRESS){
		uLong bufferSize = dwFlag & (~MSG_COMPRESS);
		Byte * uncompressBuffer = new Byte[bufferSize];
		int err = Z_OK;

		err = uncompress(uncompressBuffer, &bufferSize, source, source_len);
		if (err == Z_OK){
			dwFlag = source_len;			//保存原始长度
			dwFlag |= MSG_COMPRESS;

			source = uncompressBuffer;
			source_len = bufferSize;
		}
		else{
			delete[] uncompressBuffer;
			return;
		}
	}
	
	pHandler->OnReadMsg(Msg, source_len, (char*)source);
	if (dwFlag  &MSG_COMPRESS){
		delete[] source;
	}
}

void CManager::handler_init(){	
	send(m_pClient->m_ClientSocket, (char*)&m_pMsgHandler->m_Identity, 4, 0);
	std::cout << "CManager::handler_init()" << std::endl;
	m_pMsgHandler->OnOpen();
}

void CManager::handler_term(){
	std::cout << "CManager::handler_term()" << std::endl;
	m_pMsgHandler->OnClose();
}

void CManager::ProcessCompletedPacket(int type, CPacket*pPkt){
	if (PACKET_CLIENT_CONNECT == type){
		handler_init();
		return;
	}

	if (PACKET_CLIENT_DISCONNECT == type){
		handler_term();
		return;
	}

	if (PACKET_READ_COMPLETED == type){
		std::cout << "Read Packet (type : " << pPkt->GetCommand() <<
			" size : "<< pPkt->GetBodyLen() << std::endl;
		DispatchMsg(pPkt);
		return;
	}
}