#include "stdafx.h"
#include "IOCPProxyServer.h"
#include "SocksProxySrv.h"
#include <assert.h>
#include "IOCPServer.h"
#include "utils.h"

LPFN_ACCEPTEX CIOCPProxyServer::lpfnAcceptEx = NULL;

CIOCPProxyServer::CIOCPProxyServer(CSocksProxySrv*	pHandler){

	if (!lpfnAcceptEx){
		lpfnAcceptEx = CIOCPServer::lpfnAcceptEx;
	}

	m_pHandler = pHandler;
	m_ListenSocket = INVALID_SOCKET;
	m_AcceptSocket = INVALID_SOCKET;

	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 
		NULL, NULL, MAX_THREAD_COUNT);

	m_hStopRunning = CreateEvent(0, TRUE, TRUE, NULL);
	
	for (int i = 0; i < MAX_THREAD_COUNT; i++){
		m_hThread[i] = CreateThread(0, 0, 
			(LPTHREAD_START_ROUTINE)WorkerThread,
			this, 0, 0);
	}

	m_SocksVersion = 0x5;								//0x5
	m_AliveClientCount = 0;

	for (int i = 0; i < MAX_CLIENT_COUNT; i++){
		m_Clients[i] = 0;
	}

	InitializeCriticalSection(&m_cs);
	//
	m_ClientObjCount = 0;

	m_UDPAssociateAddr = 0;
}


CIOCPProxyServer::~CIOCPProxyServer()
{
	StopServer();
	//
	for (int i = 0; i < MAX_THREAD_COUNT; i++){
		PostQueuedCompletionStatus(m_hCompletionPort,
			0, (ULONG_PTR)INVALID_SOCKET, 0);
	}
	
	dbg_log("Wait all thread exit..");

	WaitForMultipleObjects(MAX_THREAD_COUNT, m_hThread, TRUE, INFINITE);
	for (int i = 0; i < MAX_THREAD_COUNT; i++){
		CloseHandle(m_hThread[i]);
		m_hThread[i] = NULL;
	}

	//Free Clients...(由于突然断开连接导致没有机会引用计数变为0)
	for (int i = 0; i < MAX_CLIENT_COUNT; i++){
		if (m_Clients[i]){
			dbg_log("delete ClientCtx Obj Client[%d]", i);
			m_Clients[i]->OnRemoteClose();
			m_Clients[i] = 0;
		}
	}

	ASSERT(m_ClientObjCount == 0);
	dbg_log("m_ClientObjCount == 0");

	CloseHandle(m_hCompletionPort);
	m_hCompletionPort = INVALID_HANDLE_VALUE;

	CloseHandle(m_hStopRunning);
	DeleteCriticalSection(&m_cs);
}

ClientCtx::ClientCtx(CIOCPProxyServer*pServer, CSocksProxySrv*pHandler
	, SOCKET ClientSocket, DWORD ClientID, BYTE Ver){
	//

	m_pServer = pServer;
	m_pHandler = pHandler;

	m_ClientSocket = ClientSocket;
	m_ClientID = ClientID;
	m_Ver = Ver;
	m_State = 0;
	m_SubState = 0;


	m_ReadSize = 0;
	m_NeedBytes = 0;

	if (m_Ver == 0x4){
		m_NeedBytes = 0x9;
		m_State = SOCKS_PROXY_REQUEST;
		m_SubState = SOCKS4_VER;
	}
	else{
		m_NeedBytes = 0x2;
		m_State = SOCKS_PROXY_HANDSHAKE;
		m_SubState = SOCKS5_VER;
	}
	
	m_Port = 0;
	m_AddrType = 0;
	memset(m_Address, 0, sizeof(m_Address));

	InitializeCriticalSection(&m_cs);
	m_LocalRefCount = 1;
	m_RemoteRefCount = 1;

	InterlockedIncrement(&m_pServer->m_ClientObjCount);
}

ClientCtx::~ClientCtx(){
	DeleteCriticalSection(&m_cs);
	ASSERT(m_ClientSocket == INVALID_SOCKET);
	InterlockedDecrement(&m_pServer->m_ClientObjCount);
}

void ClientCtx::OnClose(){
	BOOL bRelease = FALSE;
	BOOL bLocalClose = FALSE;

	Lock(m_cs);
	--m_LocalRefCount;
	bLocalClose = !m_LocalRefCount;
	bRelease = !m_LocalRefCount && !m_RemoteRefCount;

	//通知对方本地关闭.
	if (bLocalClose){
		//注意对方可能会主动发送,导致本地对象被释放,
		//需要把这个操作放到临界区里面.
		ASSERT(m_ClientID < MAX_CLIENT_COUNT);
		m_pHandler->SendMsg(SOCK_PROXY_CLOSE,
			&m_ClientID, sizeof(m_ClientID));
	}
	Unlock(m_cs);

	if (bRelease){
		m_pServer->ReleaseClient(this);
	}
}

void ClientCtx::OnReadNeedBytes(){
	switch (m_State)
	{
	case SOCKS_PROXY_HANDSHAKE:
		HandshakeHandle();
		break;
	case SOCKS_PROXY_REQUEST:
		RequestHandle();
		break;
	case SOCKS_PROXY_PROCESS_CMD:
		ProcessCmd();
		break;
	default:
		break;
	}
}

void ClientCtx::HandshakeHandle(){
	DWORD len = 0;
	while (m_SubState < 3){
		switch (m_SubState)
		{
		case SOCKS5_VER:
			if (m_Buffer[0] != 0x5){
				CloseSocket();
				m_pHandler->Log(TEXT("Client[%d] - Handshake Unmatched version : %d"), m_ClientID, m_Buffer[0]);
				goto Failed;
			}
			m_SubState++;
			break;
		case SOCKS5_METHOD_LEN:
			len = m_Buffer[1];
			m_SubState++;
			if (len){
				Read(TRUE, len);
				return;
			}
			break;
		case SOCKS5_AUTH_METHOD:
			m_SubState++;			///
			break;
		default:
			break;
		}
	}
	//成功
	char Response[2];
	Response[0] = 0x5;
	Response[1] = 0x0;		//No Auth...
	send(m_ClientSocket, Response, sizeof(Response), 0);

	m_State = SOCKS_PROXY_REQUEST;
	m_SubState = SOCKS5_VER;

	Read(FALSE, 4);
	return;

Failed:
	Read(FALSE);
}

void ClientCtx::ProcessCmd(){
	switch (m_Cmd)
	{
	case 0x1:					//connect.
		if (m_pHandler->BeginMsg(SOCK_PROXY_DATA)){
			m_pHandler->WriteDword(m_ClientID);
			m_pHandler->WriteBytes(m_Buffer, m_ReadSize);
			m_pHandler->EndMsg();
		}
		Read(FALSE);
		break;
	case 0x3:
		//UDP不会走这条路....,而是去从一个新的socket上收发数据.
		break;
	default:
		CloseSocket();
		Read(FALSE);				//
		break;
	}
}

void ClientCtx::RequestHandle(){
	DWORD AddrLength = 0;

	if (m_Ver == 0x4){
		while (m_SubState < 5){
			switch (m_SubState)
			{
			case SOCKS4_VER:
				if (m_Buffer[0] != 0x4){
					m_pHandler->Log(TEXT("Client[%d] - Unmatched version : %d"),m_ClientID, m_Buffer[0]);
					CloseSocket();
					goto Failed;
				}
				m_SubState++;
				break;
			case SOCKS4_CMD:
				if (m_Buffer[1] != 0x1 && m_Buffer[1] != 0x2){
					m_pHandler->Log(TEXT("Client[%d] - Unsupported Command : %d"), m_ClientID, m_Buffer[1]);
					CloseSocket();
					goto Failed;
				}
				m_Cmd = m_Buffer[1];
				m_SubState++;
				break;
			case SOCKS4_PORT:
				memcpy(&m_Port, m_Buffer + 2, 2);
				m_SubState++;
				break;
			case SOCKS4_ADDR:
				memcpy(&m_Address, m_Buffer + 4, 4);
				m_SubState++;
				break;
			case SOCKS4_USERID:
				if (m_Buffer[m_ReadSize - 1] != NULL){
					if (m_ReadSize + 1 > sizeof(m_Buffer)){
						//userid too long......
						CloseSocket();
						goto Failed;
					}
					Read(TRUE,1);
					return;
				}
				m_SubState++;
				break;
			}
		}
		//
		m_AddrType = 0x1;		//IPV4
		AddrLength = 0x4;
	}
	else if (m_Ver == 0x5){
		while (m_SubState < 6){
			switch (m_SubState)
			{
			case SOCKS5_VER:
				if (m_Buffer[0] != 0x5){
					CloseSocket();
					m_pHandler->Log(TEXT("Client[%d] - Unmatched version : %d"), m_ClientID, m_Buffer[0]);
					goto Failed;
				}
				m_SubState++;
				break;
			case SOCKS5_CMD:
				if (m_Buffer[1] != 0x1 && m_Buffer[1] != 0x2
					&& m_Buffer[1] != 0x3){
					m_pHandler->Log(TEXT("Client[%d] - Unsupported Command : %d"), m_ClientID, m_Buffer[1]);
					CloseSocket();
					goto Failed;
				}
				m_Cmd = m_Buffer[1];		
				m_SubState++;
				break;
			case SOCKS5_RSV:
				if (m_Buffer[2]){
					CloseSocket();
					goto Failed;
				}
				m_SubState++;
				break;
			case SOCKS5_ATYP:
				if (m_Buffer[3] == 0x1){
					m_SubState++;
					Read(TRUE, 0x4 + 2);			//IPV4
				}
				else if (m_Buffer[3] == 0x4){
					m_SubState++;
					Read(TRUE, 16 + 2);				//IPV6			
				}
				else if(m_Buffer[3] == 0x3){		//域名
					Read(TRUE, 1);
				}
				else{
					m_pHandler->Log(TEXT("Client[%d] - Unsupported Address Type : %d"), m_ClientID, m_Buffer[3]);
					CloseSocket();
					goto Failed;
				}
				m_SubState++;
				m_AddrType = m_Buffer[3];
				return;

			case SOCKS5_ADDRLEN:
				Read(TRUE, m_Buffer[4] + 2);		//读取指定长度的域名.
				m_SubState++;
				return;
			case SOCKS5_ADDR_PORT:
				switch (m_AddrType)
				{
				case 0x1:
					memcpy(m_Address, m_Buffer + 4, 4);
					memcpy(&m_Port, m_Buffer + 4 + 4, 2);
					AddrLength = 0x4;
					break;
				case 0x3:
					memcpy(m_Address, m_Buffer + 5, m_Buffer[4]);
					memcpy(&m_Port, m_Buffer + 5 + m_Buffer[4],2);
					AddrLength = m_Buffer[4];
					break;
				case 0x4:
					memcpy(m_Address, m_Buffer + 4, 16);
					memcpy(&m_Port, m_Buffer + 4 + 16, 2);
					AddrLength = 16;
					break;
				}
				m_SubState++;
				break;
			}
		}
	}
	if (m_Cmd == 0x3){
		m_Port = 0;		//忽略Port,而是使用 第一次udp recvfrom 收到的client addr和port.
	}

	if (m_pHandler){
		if (m_pHandler->BeginMsg(SOCK_PROXY_REQUEST)){
			m_pHandler->WriteDword(m_ClientID);
			m_pHandler->WriteDword(m_Cmd);
			m_pHandler->WriteDword(m_AddrType);
			m_pHandler->WriteDword(m_Port);
			m_pHandler->WriteBytes(m_Address, AddrLength);
			m_pHandler->WriteByte(0);
			m_pHandler->EndMsg();
		}
	}
	m_State++;
	//读取数据...
Failed:
	Read(FALSE);		//重新读数据.
}

void ClientCtx::Read(BOOL bContinue, DWORD Bytes){
	if (bContinue){
		m_NeedBytes += Bytes;	
	}
	else if(Bytes){
		m_ReadSize = 0;
		m_NeedBytes = Bytes;
	}
	else{
		m_ReadSize = 0;
		m_NeedBytes = 0;
	}
	m_pServer->PostRead(this);
}

void ClientCtx::OnRequestResponse(DWORD Error){
	TCHAR szAddr[256] = { 0 };
	char *pAddr = NULL;
	//UDP.
	if (m_Cmd == 0x3 && m_Ver == 0x5){
		while (!Error){
			SOCKADDR_IN addr = { 0 };
			Lock(m_cs);
			m_UDPSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

			if (m_UDPSocket == INVALID_SOCKET){
				Error = WSAGetLastError();
				Unlock(m_cs);
				break;
			}

			addr.sin_addr.S_un.S_addr = INADDR_ANY;
			addr.sin_family = AF_INET;
			addr.sin_port = 0;

			if (SOCKET_ERROR == bind(m_UDPSocket, (SOCKADDR*)&addr, sizeof(addr))){
				Error = WSAGetLastError();
				Unlock(m_cs);
				break;
			}

			if (m_pServer->m_hCompletionPort !=
				CreateIoCompletionPort((HANDLE)m_UDPSocket,
				m_pServer->m_hCompletionPort, (ULONG_PTR)this, 0)){
				Error = WSAGetLastError();
				Unlock(m_cs);
				break;
			}

			++m_LocalRefCount;
			RecvFrom();
			Unlock(m_cs);
			break;
		}
	}

	//Response.
	if (m_Ver == 0x5){
		BYTE Packet[10] = { 0 };
		Packet[0] = 0x5;
		Packet[1] = 0x0;
		Packet[2] = 0x0;
		Packet[3] = 0x1;			//BIND.Address和BIND.Port 忽略.

		if (Error){
			switch (Error)
			{
			default:
				Packet[1] = 0x5;
				break;
			}
		}
		if (!Error && m_Cmd == 0x3){
			SOCKADDR_IN addr = { 0 };
			int namelen = sizeof(addr);

			//还是手动填写地址吧.
			
			//获取UDPsocket 端口号.
			getsockname(m_UDPSocket, (SOCKADDR*)&addr, &namelen);
			addr.sin_addr.S_un.S_addr = m_pServer->m_UDPAssociateAddr;

			memcpy(&Packet[4], &addr.sin_addr, 4);
			memcpy(&Packet[8], &addr.sin_port, 2);
			
			pAddr = inet_ntoa(addr.sin_addr);
			MultiByteToWideChar(CP_ACP, 0, pAddr, strlen(pAddr), szAddr, 256);
			m_pHandler->Log(TEXT("Client[%d] - UDP Relay: %s:%d"), m_ClientID,
				szAddr, ntohs(addr.sin_port));
		}

		send(m_ClientSocket,(char*)Packet, sizeof(Packet),0);
	}
	else if (m_Ver == 0x4){
		BYTE Packet[8] = { 0 };
		Packet[0] = 0x00;
		Packet[1] = 0x5a;			//Successed.

		if (Error){
			Packet[1] = 0x5b;
		}
		//socks4 不支持udp.
		send(m_ClientSocket, (char*)Packet, sizeof(Packet), 0);
	}

	//这里还得修改....
	if (m_Cmd == 0x1){
		switch (m_AddrType)
		{
		case 0x1:
			pAddr = inet_ntoa(*(in_addr*)m_Address);
			MultiByteToWideChar(CP_ACP, 0, pAddr, strlen(pAddr), szAddr, 256);
			m_pHandler->Log(TEXT("Client[%d] - connect to %s ,error: %d"),
				m_ClientID, szAddr, Error);
			//dbg_log("connect to %s ,error: %d", inet_ntoa(*(in_addr*)m_Address), Error);
			break;
		case 0x3:
			MultiByteToWideChar(CP_ACP, 0, (char*)m_Address, strlen((char*)m_Address), szAddr, 256);
			m_pHandler->Log(TEXT("Client[%d] - connect to %s ,error: %d"),
				m_ClientID, szAddr, Error);
			//dbg_log("connect to %s ,error: %d", m_Address,Error);
			break;
		default:
			break;
		}
	}
}

void ClientCtx::OnRemoteClose(){
	BOOL bRelease = FALSE;

	Lock(m_cs);
	--m_RemoteRefCount;
	bRelease = !m_LocalRefCount && !m_RemoteRefCount;
	Unlock(m_cs);

	if (bRelease){
		m_pServer->ReleaseClient(this);
	}
}

void ClientCtx::OnRemoteData(char* Buffer, DWORD Size){
	switch (m_Cmd)
	{
	case 0x1:
		send(m_ClientSocket, Buffer, Size, 0);
		break;
	case 0x3:
		dbg_log("udp recv from remote (%d bytes)", Size);
		//再次打包数据发送.
		do{
			SOCKADDR_IN ClientAddr = { 0 };
			memcpy(&ClientAddr.sin_addr.S_un.S_addr, m_Address, 4);
			ClientAddr.sin_family = AF_INET;
			ClientAddr.sin_port = m_Port;

			Buffer[3] = 0x1;
			memcpy(&Buffer[4], m_Address, 4);
			memcpy(&Buffer[8], &m_Port, 2);
			dbg_log("udp send to client (%s:%d)", inet_ntoa(ClientAddr.sin_addr), ntohs(m_Port));
			sendto(m_UDPSocket, Buffer, Size, 0, (SOCKADDR*)&ClientAddr, sizeof(ClientAddr));
		} while (0);
		break;
	default:
		dbg_log("Unsupported Command.");
		break;
	}
}

void ClientCtx::RecvFrom(){
	ZeroMemory(&m_ClientAddr, sizeof(m_ClientAddr));
	m_FromLen = sizeof(m_ClientAddr);
	m_DatagramSize = 0;

	m_pServer->PostRecvFrom(this);
}


void ClientCtx::OnRecvFrom(){
	while (m_DatagramSize > 10){
		DWORD ExpectAddr = *(DWORD*)m_Address;
		//忽略...
		if (m_Datagram[0] || m_Datagram[1]){
			break;
		}
		if (m_Datagram[2]){
			break;
		}
		
		//一个 UDP Associate 到底上面可以有几个udp客户端????.
		//按道理只能有一个?....
		if (!m_Port){
			memcpy(m_Address, &m_ClientAddr.sin_addr.S_un.S_addr, 4);
			m_Port = m_ClientAddr.sin_port;
		}

		if (m_ClientAddr.sin_port != m_Port){
			break;
		}
		dbg_log("udp recv from client %s:%d", inet_ntoa(m_ClientAddr.sin_addr), ntohs(m_ClientAddr.sin_port));
		//发送给client去处理.
		if (m_pHandler->BeginMsg(SOCK_PROXY_DATA)){
			m_pHandler->WriteDword(m_ClientID);
			m_pHandler->WriteBytes(m_Datagram + 3, m_DatagramSize - 3);
			m_pHandler->EndMsg();
		}
		break;
	}
	RecvFrom();
}


void CIOCPProxyServer::PostRecvFrom(ClientCtx*pClientCtx){
	DWORD nReadBytes = 0;
	DWORD flag = 0;
	int nIORet;
	WSABUF buf;
	buf.buf = (char*)pClientCtx->m_Datagram;
	buf.len = sizeof(pClientCtx->m_Datagram);

	SocksOverlapped*pOverlapped = new SocksOverlapped(SOCKS_RECVFROM);

	EnterCriticalSection(&pClientCtx->m_cs);
	nIORet = WSARecvFrom(pClientCtx->m_UDPSocket,
		&buf, 1, &nReadBytes, &flag, (SOCKADDR*)&pClientCtx->m_ClientAddr,
		&pClientCtx->m_FromLen, (OVERLAPPED*)pOverlapped, NULL);

	if (nIORet == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING){
		pOverlapped->m_bManualPost = TRUE;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, (ULONG_PTR)pClientCtx,
			(LPOVERLAPPED)pOverlapped);
	}
	LeaveCriticalSection(&pClientCtx->m_cs);
}

//投递一个ReadIO请求
void CIOCPProxyServer::PostRead(ClientCtx*pClientCtx)
{
	DWORD nReadBytes = 0;
	DWORD flag = 0;
	int nIORet, nErrorCode;
	WSABUF wsaBuf;
	SocksOverlapped*pOverlapped = new SocksOverlapped(SOCKS_READ);
	//assert(pClientContext->m_ClientSocket == INVALID_SOCKET);

	EnterCriticalSection(&pClientCtx->m_cs);
	
	if (pClientCtx->m_NeedBytes){
		wsaBuf.buf = (char*) pClientCtx->m_Buffer + pClientCtx->m_ReadSize;
		wsaBuf.len = pClientCtx->m_NeedBytes - pClientCtx->m_ReadSize;
	}
	else{
		wsaBuf.buf = (char*)pClientCtx->m_Buffer;
		wsaBuf.len = sizeof(pClientCtx->m_Buffer);
	}
	//防止WSARecv的时候m_ClientSocket值改变.
	nIORet = WSARecv(pClientCtx->m_ClientSocket, 
		&wsaBuf, 1, &nReadBytes, &flag, (LPOVERLAPPED)pOverlapped, NULL);

	nErrorCode = WSAGetLastError();
	//IO错误,该SOCKET被关闭了,或者buff为NULL;
	if (nIORet == SOCKET_ERROR	&& nErrorCode != WSA_IO_PENDING){
		pOverlapped->m_bManualPost = TRUE;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, (ULONG_PTR)pClientCtx, 
			(LPOVERLAPPED)pOverlapped);
	}
	LeaveCriticalSection(&pClientCtx->m_cs);
}


void CIOCPProxyServer::PostAccept(){
	assert (m_AcceptSocket == INVALID_SOCKET);
	m_AcceptSocket = socket(AF_INET, SOCK_STREAM, 0);
	SocksOverlapped *pOverlappedPlus = new SocksOverlapped(SOCKS_ACCEPT);

	BOOL bAcceptRet = FALSE;
	int nErrorCode = 0;

	bAcceptRet = lpfnAcceptEx(m_ListenSocket, m_AcceptSocket, m_AcceptBuf, 0
		, REMOTE_AND_LOCAL_ADDR_LEN / 2, REMOTE_AND_LOCAL_ADDR_LEN / 2,
		NULL, (OVERLAPPED*)pOverlappedPlus);

	nErrorCode = WSAGetLastError();

	if (bAcceptRet == FALSE && nErrorCode != WSA_IO_PENDING){
		pOverlappedPlus->m_bManualPost = TRUE;
		dbg_log("CIOCPProxyServer::PostAccep Failed.,use manual post.");
		PostQueuedCompletionStatus(m_hCompletionPort, 0,
			(ULONG_PTR)this, (OVERLAPPED*)pOverlappedPlus);
		return;
	}
}

void CIOCPProxyServer::OnAccept(BOOL bSuccess){
	if (!bSuccess){
		if (m_AcceptSocket != INVALID_SOCKET){
			closesocket(m_AcceptSocket);
			m_AcceptSocket = INVALID_SOCKET;
		}
		SetEvent(m_hStopRunning);
		return;
	}

	setsockopt(m_AcceptSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
		(char *)&m_ListenSocket, sizeof(m_ListenSocket));

	UINT ClientID = m_pHandler->AllocClientID();
	if (ClientID == -1){
		closesocket(m_AcceptSocket);
	}
	else{
		ClientCtx*pCtx = new ClientCtx(this,m_pHandler,
			m_AcceptSocket, ClientID, m_SocksVersion);
		//
		Lock(m_cs);
		m_Clients[ClientID] = pCtx;
		Unlock(m_cs);

		InterlockedIncrement(&m_AliveClientCount);

		if (m_hCompletionPort != CreateIoCompletionPort(
			(HANDLE)m_AcceptSocket,
			m_hCompletionPort, (ULONG_PTR)pCtx, 0)){
			dbg_log("CreateIoCompletionPort Failed");
			InterlockedDecrement(&m_AliveClientCount);
			
			pCtx->CloseSocket();
			pCtx->OnClose();			//.
		}
		else{
			//
			PostRead(pCtx);
			//m_pHandler->Log(TEXT("Accept a connection - Client[%d]"), ClientID);
		}
	}
	m_AcceptSocket = INVALID_SOCKET;
	PostAccept();
}

void CIOCPProxyServer::ReleaseClient(ClientCtx*pClientCtx){
	Lock(m_cs);

	DWORD ClientID = pClientCtx->m_ClientID;
	m_Clients[ClientID] = NULL;
	m_pHandler->FreeClientID(ClientID);
	delete pClientCtx;
	
	Unlock(m_cs);
}

void CIOCPProxyServer::OnReadBytes(ClientCtx*pCtx, DWORD dwBytes){
	pCtx->m_ReadSize += dwBytes;

	if (!pCtx->m_NeedBytes || 
		pCtx->m_ReadSize == pCtx->m_NeedBytes){
		pCtx->OnReadNeedBytes();
	}
	else{
		PostRead(pCtx);			//继续读....
	}
}

void CIOCPProxyServer::OnReadDatagram(ClientCtx*pCtx, DWORD dwBytes){
	pCtx->m_DatagramSize = dwBytes;
	pCtx->OnRecvFrom();
}

void CIOCPProxyServer::OnRemoteData(DWORD ClientID, void * Buffer, DWORD Size){
	ASSERT(m_Clients[ClientID]);
	m_Clients[ClientID]->OnRemoteData((char*)Buffer, Size);
}

void CIOCPProxyServer::OnRemoteClose(DWORD ClientID){
	ASSERT(m_Clients[ClientID]);
	m_Clients[ClientID]->CloseSocket();				//关闭连接.
	m_Clients[ClientID]->OnRemoteClose();
}

void CIOCPProxyServer::OnRequestResponse(DWORD ClientID, DWORD Error){
	ASSERT(m_Clients[ClientID]);
	m_Clients[ClientID]->OnRequestResponse(Error);
}

//工作线程函数
unsigned int __stdcall CIOCPProxyServer::WorkerThread(CIOCPProxyServer*pServer)
{
	BOOL			bResult;
	DWORD			nTransferredBytes;
	ClientCtx *		pClientCtx;
	SocksOverlapped *pSocksOverlapped;

	while (TRUE)
	{
		nTransferredBytes = 0;
		pClientCtx = NULL;
		pSocksOverlapped = NULL;

		bResult = GetQueuedCompletionStatus(pServer->m_hCompletionPort,
			&nTransferredBytes, (PULONG_PTR)&pClientCtx, 
			(LPOVERLAPPED*)&pSocksOverlapped, INFINITE);
		
		//退出线程,投递了退出消息
		if (INVALID_SOCKET == (SOCKET)pClientCtx)
			break;

		if (pClientCtx)
		{
			if (pSocksOverlapped->m_IOtype == SOCKS_ACCEPT){
				BOOL bSuccess = bResult && !pSocksOverlapped->m_bManualPost;
				pServer->OnAccept(bSuccess);
			}
			else{
				switch (pSocksOverlapped->m_IOtype)
				{
				case SOCKS_READ:
					if (bResult && nTransferredBytes){
						pServer->OnReadBytes(pClientCtx, nTransferredBytes);
					}
					break;
				case SOCKS_RECVFROM:
					if (bResult && nTransferredBytes){
						pServer->OnReadDatagram(pClientCtx, nTransferredBytes);
					}
					break;
				}
				//判断是否关闭了..
				if (!bResult || !nTransferredBytes || pSocksOverlapped
					->m_bManualPost){
					pClientCtx->CloseSocket();
					pClientCtx->OnClose();

					//只算TCP的连接关闭.
					if (pSocksOverlapped->m_IOtype == SOCKS_READ)
						InterlockedDecrement(&pServer->m_AliveClientCount);
				}
			}
		}
		if (pSocksOverlapped)
			delete pSocksOverlapped;
	}
	return 0;
}

BOOL CIOCPProxyServer::StartServer(DWORD Port, DWORD Address, DWORD UDPAssociateAddr){

	//initial the vars;
	SYSTEM_INFO si = { 0 };
	sockaddr_in addr = { 0 };
	int i = 0;
	m_UDPAssociateAddr = htonl(UDPAssociateAddr);

	//Create Listen socket;
	m_ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == m_ListenSocket){
		dbg_log("Create ListenSocket failed with error: %u", WSAGetLastError());
		goto Error;
	}
	//Set server's params;
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = htonl(Address);
	addr.sin_port = htons(Port);
	//bind 
	if (INVALID_SOCKET == bind(m_ListenSocket, (sockaddr*)&addr, sizeof(sockaddr)))
	{
		dbg_log("Bind failed with error: %u", WSAGetLastError());
		goto Error;
	}

	//start listen;
	if (INVALID_SOCKET == listen(m_ListenSocket, SOMAXCONN))
	{
		dbg_log("Listen failed with error: %u", WSAGetLastError());
		goto Error;
	}
	//绑定到完成端口.
	if (m_hCompletionPort != CreateIoCompletionPort((HANDLE)m_ListenSocket,
		m_hCompletionPort, (ULONG_PTR)this, 0)){
		
		dbg_log("CreateIoCompletionPort failed with error: %u", WSAGetLastError());
		goto Error;
	}
	ResetEvent(m_hStopRunning);				//Means the server is ready for accepting client 
	PostAccept();							//Post first accept request

	dbg_log("Start Server Successed!");
	return TRUE;

Error:
	if (m_ListenSocket != INVALID_SOCKET){
		closesocket(m_ListenSocket);
		m_ListenSocket = INVALID_SOCKET;
	}
	return FALSE;

}


void CIOCPProxyServer::StopServer(){
	//only close listen socket....
	if (m_ListenSocket != INVALID_SOCKET){
		closesocket(m_ListenSocket);
		m_ListenSocket = INVALID_SOCKET;
		WaitForSingleObject(m_hStopRunning,INFINITE);
	}
	//Close All Clients...
	Lock(m_cs);
	for (int i = 0; i < MAX_CLIENT_COUNT; i++){
		if (m_Clients[i]){
			m_Clients[i]->CloseSocket();
		}
	}
	Unlock(m_cs);

	//Wait All Client is free..
	dbg_log("wait disconnect all connections");

	while (m_AliveClientCount) Sleep(1);

	ASSERT(m_ListenSocket == INVALID_SOCKET);
	ASSERT(m_AcceptSocket == INVALID_SOCKET);
	dbg_log("Proxy server has been stopped.");
	return;
}