#pragma once

class CPacket;
class CIOCPServer;
class CClientContext;
class CMsgHandler;

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

	HWND	m_hNotifyWnd;

	CRITICAL_SECTION m_cs;

	std::map<void*, void*> m_Ctx2Handler;
	std::map<void*, void*> m_Handler2Ctx;

	CClientContext * handler2ctx(CMsgHandler*pHandler);
	CMsgHandler	   * ctx2handler(CClientContext*pCtx);

	void lock(){
		EnterCriticalSection(&m_cs);
	}
	void unlock(){
		LeaveCriticalSection(&m_cs);
	}
	void add(CClientContext * pCtx,CMsgHandler*pHandler);
	void remove(CClientContext * pCtx, CMsgHandler*pHandler);
public:

	BOOL handler_init(CClientContext*pClientContext, DWORD Identity);
	BOOL handler_term(CClientContext*pClientContext, DWORD Identity);

	void ProcessCompletedPacket(int type, CClientContext*pContext, CPacket*pPacket);
	void DispatchMsg(CClientContext*pContext, CPacket*pPkt);

	const pair<string, unsigned short> GetPeerName(CMsgHandler * pMsgHandler);
	const pair<string, unsigned short> GetSockName(CMsgHandler * pMsgHandler);

	BOOL SendMsg(CMsgHandler*pHandler, WORD Msg, char*data, size_t len);
	void Close(CMsgHandler*pHandler);

	CManager(HWND hNotifyWnd);
	~CManager();
};

