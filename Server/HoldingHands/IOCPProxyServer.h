#pragma once
#include "SocksProxySrv.h"

#define SOCKS_ACCEPT	0
#define SOCKS_READ		1
#define SOCKS_RECVFROM	2



#define SOCKS_PROXY_HANDSHAKE		0
#define SOCKS_PROXY_REQUEST			1
#define SOCKS_PROXY_PROCESS_CMD		2


//socks4 state...
#define SOCKS4_VER		0
#define SOCKS4_CMD		1
#define SOCKS4_PORT		2
#define SOCKS4_ADDR		3
#define SOCKS4_USERID	4

////
#define SOCKS5_VER			0

//握手阶段.
#define SOCKS5_METHOD_LEN	1
#define SOCKS5_AUTH_METHOD	2
//请求阶段.

//Request :
//	ver,cmd,\x00,atyp,				port...

#define SOCKS5_CMD			1
#define SOCKS5_RSV			2
#define SOCKS5_ATYP			3

#define SOCKS5_ADDRLEN		4
#define SOCKS5_ADDR_PORT	5




typedef struct tagSocksOverlapped
{
	OVERLAPPED	m_overlapped;
	DWORD		m_IOtype;
	BOOL		m_bManualPost;			//用来记录是否为手动触发。

	tagSocksOverlapped(DWORD IO_TYPE, BOOL bManualPost = FALSE)
	{
		m_IOtype = IO_TYPE;
		m_bManualPost = bManualPost;
		m_overlapped.hEvent = 0;
		m_overlapped.Internal = 0;
		m_overlapped.InternalHigh = 0;
		m_overlapped.Offset = 0;
		m_overlapped.OffsetHigh = 0;
		m_overlapped.Pointer = 0;
	}
}SocksOverlapped;


class ClientCtx{
public:
	DWORD			 m_LocalRefCount;	//.2,当为0的时候自动释放.
	DWORD		     m_RemoteRefCount;
	
	CRITICAL_SECTION m_cs;
	
	DWORD			m_ClientID;
	DWORD			m_State;		//阶段
	DWORD			m_SubState;

	CIOCPProxyServer*	m_pServer;
	CSocksProxySrv*		m_pHandler;

	//
	LARGE_INTEGER	m_liUpload;
	LARGE_INTEGER	m_liDownload;

	//
	SOCKET		m_ClientSocket;
	DWORD		m_Cmd;			//命令类型.
	//
	BYTE		m_Ver;
	DWORD		m_AddrType;
	BYTE		m_Address[256];			//UDP中代表客户端地址
	UINT		m_Port;					//UDP中代表客户端端口号.
	//

	//TCP .Buf ....
	DWORD		m_NeedBytes;
	BYTE		m_Buffer[0x2000];
	DWORD		m_ReadSize;

	//UDP 
	SOCKET		m_UDPSocket;
	SOCKADDR_IN	m_ClientAddr;
	int			m_FromLen;
	DWORD		m_DatagramSize;
	BYTE		m_Datagram[0x2000];

	//阶段
	void HandshakeHandle();
	void RequestHandle();
	void ProcessCmd();
	//

	void OnClose();

	//local....
	void OnReadNeedBytes();
	void OnRecvFrom();

	void Read(BOOL bContinue = FALSE, DWORD Bytes = 0);
	void RecvFrom();
	//Remote...
	void OnRequestResponse(DWORD Error);
	//
	void OnRemoteClose();
	void OnRemoteData(char* Buffer, DWORD Size);
	
	//
	ClientCtx(CIOCPProxyServer*pServer,CSocksProxySrv*pHandler
		,SOCKET ClientSocket, DWORD ClientID, BYTE Ver);

	~ClientCtx();

	void CloseSocket(){
		EnterCriticalSection(&m_cs);
		if (m_ClientSocket != INVALID_SOCKET){
			closesocket(m_ClientSocket);
			m_ClientSocket = INVALID_SOCKET;
		}
		if (m_UDPSocket != INVALID_SOCKET){
			closesocket(m_UDPSocket);
			m_UDPSocket = INVALID_SOCKET;
		}
		LeaveCriticalSection(&m_cs);
	}
};

class CSocksProxySrv;

#define REMOTE_AND_LOCAL_ADDR_LEN		(2 * (sizeof(sockaddr)+16))
#define MAX_THREAD_COUNT	8

class CIOCPProxyServer
{
	friend class ClientCtx;
public:
	static LPFN_ACCEPTEX	lpfnAcceptEx;

private:
	HANDLE			m_hCompletionPort;							//完成端口句柄
	HANDLE			m_hStopRunning;								//通知已经停止运行服务器

	DWORD			m_UDPAssociateAddr;

	CRITICAL_SECTION m_cs;
	SOCKET			m_ListenSocket;
	volatile SOCKET	m_AcceptSocket;								//用于接收客户端的socket
	char			m_AcceptBuf[REMOTE_AND_LOCAL_ADDR_LEN];
	HANDLE			m_hThread[MAX_THREAD_COUNT];


	ClientCtx*		m_Clients[MAX_CLIENT_COUNT];				//
	volatile DWORD	m_AliveClientCount;
	//
	void OnReadBytes(ClientCtx*pCtx, DWORD dwBytes);
	void OnReadDatagram(ClientCtx*pCtx, DWORD dwBytes);
	void OnAccept(BOOL bSuccess);


	void ReleaseClient(ClientCtx*pClientCtx);

	BYTE			m_SocksVersion;
	CSocksProxySrv*	m_pHandler;

#ifdef _DEBUG
	//Obj 计数
	volatile DWORD  m_ClientObjCount;
#endif

public:
	void PostRecvFrom(ClientCtx*pClientCtx);
	void PostRead(ClientCtx*pClientCtx);
	void PostAccept();

	static unsigned int __stdcall WorkerThread(CIOCPProxyServer*pServer);
	
	void Disconnect(DWORD ClientID);
	BOOL StartServer(DWORD Port, DWORD Address, DWORD UDPAssociateAddr);
	void StopServer();

	void OnRemoteClose(DWORD ClientID);
	void OnRemoteData(DWORD ClientID, void * Buffer, DWORD Size);
	void OnRequestResponse(DWORD ClientID, DWORD Error);

	void SetSocksVer(BYTE Version){
		m_SocksVersion = Version;
	}

	DWORD	GetConnections() { return m_AliveClientCount; }

	CIOCPProxyServer(CSocksProxySrv*	pHandler);
	~CIOCPProxyServer();
};

#define Lock(x) EnterCriticalSection(&x)
#define Unlock(x) LeaveCriticalSection(&x)