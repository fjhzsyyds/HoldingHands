#pragma once

#include "..\Common\IOCPClient.h"
class CPacket;
class CMsgHandler;
class CIOCPClient;

enum COMPLETED_PACKET_TYPE
{
	PACKET_READ_COMPLETED = 0x01,
	PACKET_READ_PARTIAL,
	PACKET_WRITE_COMPLETED,
	PACKET_WRITE_PARTIAL,
	PACKET_CLIENT_CONNECT,
	PACKET_CLIENT_DISCONNECT,
	PACKET_ACCEPT_ABORT
};

#define WM_SOCKET_CONNECT (WM_USER+201)
#define WM_SOCKET_CLOSE (WM_USER+202)


#include <map>
#include <string>


using std::map;
using std::string;
using std::pair;

class CManager
{
private:
	friend class CIOCPServer;

	CIOCPClient* m_pClient;
	CMsgHandler* m_pMsgHandler;

public:
	void Associate(CIOCPClient * pClient, CMsgHandler*pMsgHandler){
		m_pClient = pClient;
		m_pMsgHandler = pMsgHandler;
	}

	void ProcessCompletedPacket(int type,CPacket*pPacket);
	void DispatchMsg(CPacket*pPkt);

	void handler_init();
	void handler_term();

	BOOL SendMsg(WORD Msg, char*data, size_t len,BOOL Compress);
	void Close();

	const pair<string, unsigned short> GetPeerName();
	const pair<string, unsigned short> GetSockName();

	CManager();
	~CManager();
};