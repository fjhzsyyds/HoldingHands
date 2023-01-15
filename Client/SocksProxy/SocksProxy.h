#pragma once
#include "MsgHandler.h"


#define SOCKS_PROXY			(('S') | (('K') << 8) | (('P') << 16) | (('X') << 24))


#define MAX_CLIENT_COUNT	1024
#define WORK_THREAD_COUNT	8

#define SOCK_PROXY_ADD_CLIENT  (0xaa01)

#define SOCK_PROXY_REQUEST					(0xaa02)

//connect 命令.
#define SOCK_PROXY_CMD_CONNECT_RESULT		(0xaa04)
#define SOCK_PROXY_CMD_CONNECT_DATA			(0xaa03)
#define SOCK_PROXY_CMD_CONNECT_CLOSE		(0xaa05)



class CSocksProxy;

class ClientCtx{
public:
	DWORD			m_dwClientID;
	BOOL			m_bNoRequest;

	//Connect Command...
	SOCKET				m_RemoteSock;
	DWORD				m_AddressType;
	char				m_RemoteAddress[256];
	UINT				m_RemotePort;
	///
	HANDLE				m_hEvent;
	WSABUF				m_Buf;
	CRITICAL_SECTION	m_cs;

	CSocksProxy*m_pHandler;

	void Lock(){
		EnterCriticalSection(&m_cs);
	}

	void Unlock(){
		LeaveCriticalSection(&m_cs);
	}

	//请求处理.
	void RequestHandle(int Cmd, int addr_type, char * addr, int port);

	//Connect command handle.....
	void ConnectRemote();
	void OnConnect(BOOL bSuccess);

	void OnReadFromClient(char * data, size_t size);
	void OnReadFromRemote(BOOL Success, DWORD dwTransferBytes);

	ClientCtx(CSocksProxy*pHandler, DWORD dwID);
	~ClientCtx();
	static void __stdcall WorkThread(ClientCtx*pCtx);
};

class CSocksProxy :
	public CMsgHandler
{
private:
	ClientCtx*     m_Clients[MAX_CLIENT_COUNT];
	HANDLE m_hCompletionPort;
	HANDLE m_hWorkThreads[WORK_THREAD_COUNT];
public:
	void OnOpen();
	void OnClose();
	
	void OnAddClient(DWORD dwClientID);
	void OnRequest(DWORD * Args);


	void OnClientData(char * Buffer, size_t len);
	void OnConnectClose(DWORD ClientID);

	BOOL AssociateClient(ClientCtx*pCtx);

	//有数据到达的时候调用这两个函数.
	virtual void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	//有数据发送完毕后调用这两个函数
	virtual void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer){};


	static void __stdcall work_thread(CSocksProxy*pThis);
	CSocksProxy(CManager*pManager);
	~CSocksProxy();
};


