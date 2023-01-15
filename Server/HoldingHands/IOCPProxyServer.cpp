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
	m_RefCount = 2;

	InterlockedIncrement(&m_pServer->m_ClientObjCount);
}

ClientCtx::~ClientCtx(){
	DeleteCriticalSection(&m_cs);
	ASSERT(m_ClientSocket == INVALID_SOCKET);
	InterlockedDecrement(&m_pServer->m_ClientObjCount);
}

void ClientCtx::OnClose(){
	BOOL bRelease = FALSE;

	//通知对方本地关闭.
	if (m_pHandler){
		m_pHandler->SendMsg(SOCK_PROXY_CMD_CONNECT_CLOSE,
			&m_ClientID, sizeof(m_ClientID));
	}
	
	EnterCriticalSection(&m_cs);
	InterlockedDecrement(&m_RefCount);
	bRelease = !m_RefCount;
	LeaveCriticalSection(&m_cs);

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
	char  buff[0x1004];
	switch (m_Cmd)
	{
	case 0x1:					//connect.
		memcpy(buff, &m_ClientID, sizeof(m_ClientID));
		memcpy(buff + sizeof(m_ClientID), m_Buffer, m_ReadSize);
		
		if (m_pHandler){
			m_pHandler->SendMsg(SOCK_PROXY_CMD_CONNECT_DATA, 
				buff, 4 + m_ReadSize);
		}
		Read(FALSE);
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
			case 0:
				if (m_Buffer[0] != 0x4){
					m_pHandler->Log(TEXT("Client[%d] - Unmatched version : %d"),m_ClientID, m_Buffer[0]);
					CloseSocket();
					goto Failed;
				}
				m_SubState++;
				break;
			case 1:
				if (m_Buffer[1] != 0x1 && m_Buffer[1] != 0x2){
					m_pHandler->Log(TEXT("Client[%d] - Unsupported Command : %d"), m_ClientID, m_Buffer[1]);
					CloseSocket();
					goto Failed;
				}
				m_Cmd = m_Buffer[1];
				m_SubState++;
				break;
			case 2:
				memcpy(&m_Port, m_Buffer + 2, 2);
				m_SubState++;
				break;
			case 3:
				memcpy(&m_Address, m_Buffer + 4, 4);
				m_SubState++;
				break;
			case 4:
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
				if (m_Buffer[1] != 0x0 && m_Buffer[1] != 0x1
					&& m_Buffer[1] != 0x2){
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
	if (m_pHandler){
		DWORD* Args = (DWORD*)calloc(1, 16 + AddrLength + 1);
		Args[0] = m_ClientID;
		Args[1] = m_Cmd;
		Args[2] = m_AddrType;
		Args[3] = m_Port;

		memcpy(&Args[4], m_Address, AddrLength);
		m_pHandler->SendMsg(SOCK_PROXY_REQUEST, Args, 16 + AddrLength + 1);
		free(Args);
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
		send(m_ClientSocket,(char*)Packet, sizeof(Packet),0);
	}
	else if (m_Ver == 0x4){
		BYTE Packet[8] = { 0 };
		Packet[0] = 0x00;
		Packet[1] = 0x5a;			//Successed.

		if (Error){
			Packet[1] = 0x5b;
		}
		send(m_ClientSocket, (char*)Packet, sizeof(Packet), 0);
	}
	//

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

void ClientCtx::OnRemoteClose(){
	BOOL bRelease = FALSE;
	EnterCriticalSection(&m_cs);
	InterlockedDecrement(&m_RefCount);
	bRelease = !m_RefCount;
	LeaveCriticalSection(&m_cs);

	if (bRelease){
		m_pServer->ReleaseClient(this);
	}
}

void ClientCtx::OnRemoteData(char* Buffer, DWORD Size){
	send(m_ClientSocket, Buffer, Size,0);
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
			m_pHandler->Log(TEXT("Accept - Client[%d]"), ClientID);
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
			else if (pSocksOverlapped->m_IOtype == SOCKS_READ){
				if (bResult && nTransferredBytes){
					pServer->OnReadBytes(pClientCtx, nTransferredBytes);
				}
				if (!bResult || !nTransferredBytes ||pSocksOverlapped
					->m_bManualPost){
					
					pClientCtx->CloseSocket();
					pClientCtx->OnClose();

					InterlockedDecrement(&pServer->m_AliveClientCount);
				}
			}
		}
		if (pSocksOverlapped)
			delete pSocksOverlapped;
	}
	return 0;
}

BOOL CIOCPProxyServer::StartServer(DWORD Port, DWORD Address){

	//initial the vars;
	SYSTEM_INFO si = { 0 };
	sockaddr_in addr = { 0 };
	int i = 0;
	//Create Listen socket;
	m_ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == m_ListenSocket){
		dbg_log("Create ListenSocket failed with error: %u", WSAGetLastError());
		goto Error;
	}
	//Set server's params;
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = Address;
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
	return;
}