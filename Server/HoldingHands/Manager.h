#pragma once

class CPacket;
class CIOCPServer;
class CClientContext;
class CMsgHandler;

enum COMPLETED_PACKET_TYPE
{
	PACKET_READ_COMPLETED = 0x01,
	PACKET_WRITE_COMPLETED,
	PACKET_CLIENT_CONNECT,
	PACKET_CLIENT_DISCONNECT,
	PACKET_ACCEPT_ABORT,

	PACKET_READ_PARTIAL,
	PACKET_WRITE_PARTIAL,
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

	HWND	m_hNotifyWnd;
public:

	BOOL handler_init(CClientContext*pClientContext, DWORD Identity);
	BOOL handler_term(CClientContext*pClientContext, DWORD Identity);

	void OnClientConnect(CClientContext*pClientCtx);
	void OnClientDisconnect(CClientContext*pClientCtx);
	void OnAcceptAboart();

	CManager(HWND hNotifyWnd);
	~CManager();
};

