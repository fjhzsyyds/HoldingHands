#include "SocksProxy.h"
#include "IOCPClient.h"

#include "utils.h"
#include <stdint.h>
#include <assert.h>
#include <winioctl.h>


#define SOCKS_READ			0
#define SOCKS_CONNECTED		1


typedef struct tagSocksOverlapped{
	OVERLAPPED Overlapped;
	DWORD	   IoType;

	tagSocksOverlapped(DWORD type){
		memset(&Overlapped, 0, sizeof(Overlapped));
		IoType = type;
	}

}SocksOverlapped;


CSocksProxy::CSocksProxy(CIOCPClient *pClient) :
CMsgHandler(pClient, SOCKS_PROXY)
{
	memset(m_Clients, 0, sizeof(m_Clients));
	m_hCompletionPort = INVALID_HANDLE_VALUE;
	memset(m_hWorkThreads, 0, sizeof(m_hWorkThreads));
}


CSocksProxy::~CSocksProxy()
{
	memset(m_Clients, 0, sizeof(m_Clients));
	m_hCompletionPort = INVALID_HANDLE_VALUE;
	memset(m_hWorkThreads, 0, sizeof(m_hWorkThreads));
}

ClientCtx::ClientCtx(CSocksProxy*pHandler, DWORD dwID) :
	m_pHandler(pHandler),
	m_dwClientID(dwID),
	m_RemotePort(0),
	m_AddressType(0),
	m_RemoteSock(INVALID_SOCKET),
	m_bNoRequest(TRUE),
	m_Cmd(0)
{
	m_hEvent = CreateEvent(0, TRUE, FALSE, NULL);			//ָʾ��socket �Ƿ��Ѿ�����.
	InitializeCriticalSection(&m_cs);

	m_Buf.buf = new char[0x1000];
	m_Buf.len = 0x1000;
}

ClientCtx::~ClientCtx(){
	DeleteCriticalSection(&m_cs);
	CloseHandle(m_hEvent);

	if (m_Buf.buf){
		delete[] m_Buf.buf;
		m_Buf.buf = NULL;
	}
	m_Buf.len = 0;
}

void CSocksProxy::OnOpen(){

	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
		NULL, NULL, WORK_THREAD_COUNT);

	for (int i = 0; i < WORK_THREAD_COUNT ; i++){
		m_hWorkThreads[i] = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)work_thread,
			this, 0, 0);
	}
}

void CSocksProxy::OnClose(){
	for (int i = 0; i < MAX_CLIENT_COUNT; i++){
		if (!m_Clients[i])
			continue;

		//Close Socket....
		m_Clients[i]->Lock();
		if (m_Clients[i]->m_RemoteSock != INVALID_SOCKET){
			closesocket(m_Clients[i]->m_RemoteSock);	
			m_Clients[i]->m_RemoteSock = INVALID_SOCKET;
		}

		m_Clients[i]->Unlock();

		if (!m_Clients[i]->m_bNoRequest)
			WaitForSingleObject(m_Clients[i]->m_hEvent,INFINITE);

		delete m_Clients[i];
		m_Clients[i] = NULL;
	}

	for (int i = 0; i < WORK_THREAD_COUNT; i++){
		PostQueuedCompletionStatus(m_hCompletionPort, 0, (ULONG_PTR)(INVALID_SOCKET), 0);
	}
	
	for (int i = 0; i < WORK_THREAD_COUNT; i++){
		WaitForSingleObject(m_hWorkThreads[i], INFINITE);
		CloseHandle(m_hWorkThreads[i]);
	}
	CloseHandle(m_hCompletionPort);
}

void CSocksProxy::OnRequest(DWORD * Args){

	DWORD ID =			Args[0];
	DWORD Cmd =			Args[1];
	DWORD AddrType =	Args[2];
	DWORD Port =		Args[3];

	char * Address = (char*)&Args[4];
	/*dbg_log("address: %s port: %d \n", inet_ntoa(*(in_addr*)Address),
		ntohs(Port));*/

	assert(ID < MAX_CLIENT_COUNT);
	assert(m_Clients[ID]);
	m_Clients[ID]->RequestHandle(Cmd, AddrType, Address, Port);
}

//�����ݵ����ʱ���������������.
void CSocksProxy::OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer){
	switch (Msg)
	{
	case SOCK_PROXY_ADD_CLIENT:
		OnAddClient(*(DWORD*)Buffer);
		break;
		
	case SOCK_PROXY_REQUEST:
		OnRequest((DWORD*)Buffer);
		break;
	
		//����connect ����.
	case SOCK_PROXY_DATA:
		OnClientData(Buffer, dwSize);
		break;

	case SOCK_PROXY_CLOSE:
		OnProxyClose(*(DWORD*)Buffer);
		break;
	default:
		break;
	}
}

//ֻ���ڱ��غ�Զ�̶����͹ر���Ϣ���ɾ��Ctx.
void CSocksProxy::OnProxyClose(DWORD ClientID){

	assert(ClientID < MAX_CLIENT_COUNT);
	assert(m_Clients[ClientID]);
	ClientCtx*pCtx = m_Clients[ClientID];

	if (!pCtx->m_bNoRequest){
		//�����������ɶ˿���..
		pCtx->Lock();
		if (pCtx->m_RemoteSock != INVALID_SOCKET){
			closesocket(pCtx->m_RemoteSock);
			pCtx->m_RemoteSock = INVALID_SOCKET;
			
			pCtx->m_pHandler->SendMsg(SOCK_PROXY_CLOSE,   //������....
				//,�п���֮���и�Request..
				&pCtx->m_dwClientID, sizeof(pCtx->m_dwClientID));
		}
		pCtx->Unlock();

		//dbg_log("WaitForSingleObject ,Ctx(%d)", pCtx->m_dwClientID);
		WaitForSingleObject(pCtx->m_hEvent,INFINITE);
		//dbg_log("WaitForSingleObject Finished,Ctx(%d)", pCtx->m_dwClientID);
	}
	else{
		//֮����û�л��ᷢ�� SOCK_PROXY_CMD_CONNECT_CLOSE
		pCtx->m_pHandler->SendMsg(SOCK_PROXY_CLOSE,		//������....
			//,�п���֮���и�Request..
			&pCtx->m_dwClientID, sizeof(pCtx->m_dwClientID));
	}

	dbg_log("Client[%d] Close",ClientID);
	m_Clients[ClientID] = NULL;
	delete pCtx;
}


void CSocksProxy::OnClientData(char * Buffer, size_t len){
	DWORD ClientID = *(DWORD*)Buffer;
	m_Clients[ClientID]->OnReadFromClient(Buffer + 4, len - 4);
}


void CSocksProxy::OnAddClient(DWORD ClientID){
	assert(m_Clients[ClientID] == NULL);
	m_Clients[ClientID] = new ClientCtx(this, ClientID);
}

void ClientCtx::RequestHandle(int Cmd, int addr_type, char * addr, int port){
	HOSTENT*pHost = NULL;
	switch (addr_type)
	{
	case 0x1:
		memcpy(m_RemoteAddress, addr, 4);
		break;
	case 0x3:
		pHost = gethostbyname(addr);
		if (pHost){
			memcpy(m_RemoteAddress, pHost->h_addr, 4);
			addr_type = 0x1;			// -> IPV4;
		}
		break;
	case 0x4:
		memcpy(m_RemoteAddress, addr, 16);
		break;
	default:
		break;
	}

	m_AddressType = addr_type;
	m_RemotePort = port;
	m_Cmd = Cmd;

	switch (Cmd)
	{
	case 0x01:			//Connect.
		ConnectRemote();
		m_bNoRequest = FALSE;
		break;
	case 0x3:
		UDPRecv();
		m_bNoRequest = FALSE;
		break;
	default:
		//Unsupported Command.....
		do{
			char  Buffer[4 + 1 + 4 + 2] = { 0 };
			memcpy(Buffer, &m_dwClientID, 4);
			Buffer[4] = 0x7;			//��֧�ֵ�����.

			if (m_RemoteSock != INVALID_SOCKET){
				m_pHandler->SendMsg(SOCK_PROXY_REQUEST_RESULT,
					Buffer, 4 + 1 + 4 + 2);
			}
		} while (0);
		break;
	}
}


BOOL CSocksProxy::AssociateClient(ClientCtx*pCtx){
	HANDLE hHandle = CreateIoCompletionPort((HANDLE)pCtx->m_RemoteSock,
		m_hCompletionPort, (ULONG_PTR)pCtx, WORK_THREAD_COUNT);

	return (hHandle == m_hCompletionPort);
}

void ClientCtx::UDPRecv(){
	DWORD Response[2];
	DWORD Error = 0;
	DWORD dwRecvBytes = 0;
	DWORD dwFlag = 0;
	int iResult;

	SocksOverlapped *pOverlapped = new SocksOverlapped(SOCKS_READ);
	SOCKADDR_IN addr = { 0 };
	Response[0] = m_dwClientID;

	m_RemoteSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (m_RemoteSock == INVALID_SOCKET){
		Error = WSAGetLastError();
		goto failed;
	}

	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	if (SOCKET_ERROR == bind(m_RemoteSock, (SOCKADDR*)&addr, sizeof(addr))){
		Error = WSAGetLastError();
		goto failed;
	}
	if (!m_pHandler->AssociateClient(this)){
		Error = WSAGetLastError();
		goto failed;
	}

	ZeroMemory(&m_RemoteAddr, sizeof(m_RemoteAddr));
	m_FromLen = sizeof(m_RemoteAddr);
	iResult = WSARecvFrom(m_RemoteSock, &m_Buf, 1, &dwRecvBytes, &dwFlag, (SOCKADDR*)&
		m_RemoteAddr, &m_FromLen, (WSAOVERLAPPED*)pOverlapped,NULL);

	if (iResult == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING){
		Error = WSAGetLastError();
		goto failed;
	}

	Response[1] = Error;
	m_pHandler->SendMsg(SOCK_PROXY_REQUEST_RESULT,
		Response, sizeof(Response));
	return;

failed:
	Response[1] = Error;
	if (m_RemoteSock != INVALID_SOCKET){
		m_pHandler->SendMsg(SOCK_PROXY_REQUEST_RESULT,
			Response, sizeof(Response));
	}
	else{
		m_pHandler->SendMsg(SOCK_PROXY_CLOSE,
			&m_dwClientID, sizeof(m_dwClientID));
	}
	if (pOverlapped)
		delete pOverlapped;

	SetEvent(m_hEvent);
}
//�������Work Thread��.
void ClientCtx::ConnectRemote(){
	DWORD Error = 0;
	DWORD Response[2];

	SOCKADDR_IN addr = { 0 };
	SOCKADDR_IN local = { 0 };

	SocksOverlapped * pOverlapped = NULL;

	if (m_AddressType == 0x1){
		addr.sin_family = AF_INET;
		addr.sin_port = m_RemotePort;
		memcpy(&addr.sin_addr.S_un.S_addr, m_RemoteAddress, 4);

		dbg_log("remote ip: %s", inet_ntoa(addr.sin_addr));

		m_RemoteSock = socket(AF_INET, SOCK_STREAM, 0);

		if (m_RemoteSock == INVALID_SOCKET){
			Error = WSAGetLastError();
			goto failed;
		}
		local.sin_family = AF_INET;
		local.sin_port = htons(0);
		local.sin_addr.S_un.S_addr = htonl(ADDR_ANY);
		if (SOCKET_ERROR == bind(m_RemoteSock, (SOCKADDR*)&local, sizeof(local))){
			Error = WSAGetLastError();
			goto failed;
		}
		if (!m_pHandler->AssociateClient(this)){
			Error = WSAGetLastError();
			goto failed;
		}

		pOverlapped = new SocksOverlapped(SOCKS_CONNECTED);
		BOOL bResult = CIOCPClient::lpfnConnectEx(m_RemoteSock, (SOCKADDR*)&addr,
			sizeof(addr), NULL, NULL, NULL, (OVERLAPPED*)pOverlapped);
		
		Error = WSAGetLastError();

		if (FALSE == bResult && Error != WSA_IO_PENDING){
			Error = WSAGetLastError();
			goto failed;
		}
	}
	else if(m_AddressType == 0x3){		//0x3��ԭ����gethostbynameʧ����.
		gethostbyname(m_RemoteAddress);
		Error = WSAGetLastError();
		goto failed;
	}
	else if (m_AddressType == 0x4){
		Error = -1;			//�ݲ�֧��.
		goto failed;
	}
	return;
failed:
	Response[0] = m_dwClientID;
	Response[1] = Error;

	//���socket ʧ�ܻ���ô��????

	if (m_RemoteSock != INVALID_SOCKET){
		m_pHandler->SendMsg(SOCK_PROXY_REQUEST_RESULT,
			Response, sizeof(Response));
	}
	else{
		m_pHandler->SendMsg(SOCK_PROXY_CLOSE,
			&m_dwClientID, sizeof(m_dwClientID));
	}

	if (pOverlapped)
		delete pOverlapped;
	SetEvent(m_hEvent);
}

//�ӿͻ����յ�������.
void ClientCtx::OnReadFromClient(char * data, size_t size){
	Lock();
	if (m_RemoteSock == INVALID_SOCKET){
		Unlock();
		return;
	}

	switch (m_Cmd)
	{
	case 0x1:
		send(m_RemoteSock, data, size, 0);
		break;
	case 0x3:
		do{
			SOCKADDR_IN TargetAddr = { 0 };
			HOSTENT *pHost = NULL;
			BYTE * lpData = (BYTE*)data;
			char name[256] = { 0 };
			int namelen = 0;

			TargetAddr.sin_family = AF_INET;

			switch (lpData[0])
			{
			case 0x1://		IPv4.
				memcpy(&TargetAddr.sin_addr.S_un.S_addr,
					&lpData[1], 4);
				memcpy(&TargetAddr.sin_port, &lpData[5], 2);
				sendto(m_RemoteSock, (char*)lpData + 7, size - 7, 0,
					(SOCKADDR*)&TargetAddr, sizeof(TargetAddr));
				dbg_log("udp sendto %s:%d", inet_ntoa(TargetAddr.sin_addr), htons(TargetAddr.sin_port));
				break;

			case 0x3:
				namelen = lpData[1];
				name[namelen] = 0;
				memcpy(name, lpData + 2, namelen);
				dbg_log("udp sendto %s", name);
				pHost = gethostbyname(name);
				if (pHost){
					memcpy(&TargetAddr.sin_addr.S_un.S_addr,
						pHost->h_addr, 4);
					memcpy(&TargetAddr.sin_port, &lpData[2 + namelen], 2);
					sendto(m_RemoteSock, (char*)lpData + 2 + namelen + 2,
						size - (2 + namelen + 2), 0,
						(SOCKADDR*)&TargetAddr, sizeof(TargetAddr));
					dbg_log("udp sendto %s:%d", inet_ntoa(TargetAddr.sin_addr), htons(TargetAddr.sin_port));
				}
				break;
			default:
				dbg_log("Unsupported Address Type.");
				break;
			}
		} while (0);
		break;
	default:
		break;
	}

	Unlock();
}

void ClientCtx::OnReadFromRemote(BOOL Success, DWORD dwTransferBytes){
	//dbg_log("Ctx: %d,Read Success: %d ,bytes: %d",m_dwID, Success, dwTransferBytes);
	DWORD dwReadBytes = 0;
	DWORD dwFlag = 0;
	int iResult = 0;
	SocksOverlapped * pOverlapped = NULL;

	if (!Success || !dwTransferBytes){
		Lock();
		if (m_RemoteSock != INVALID_SOCKET){
			closesocket(m_RemoteSock);
			m_RemoteSock = INVALID_SOCKET;

			m_pHandler->SendMsg(SOCK_PROXY_CLOSE,   //������....
				//,�п���֮���и�Request..
				&m_dwClientID, sizeof(m_dwClientID));
		}
		Unlock();
		SetEvent(m_hEvent);				//��˼�ǽ�����...
		return;
	}

	// forward data..
	Lock();
	if (m_RemoteSock != INVALID_SOCKET){
		switch (m_Cmd)
		{
		case 0x1:
			dbg_log("tcp read from remote :%d bytes", dwTransferBytes);
			if (m_pHandler->BeginMsg(SOCK_PROXY_DATA)){
				m_pHandler->WriteDword(m_dwClientID);
				m_pHandler->WriteBytes((BYTE*)m_Buf.buf, dwTransferBytes);
				m_pHandler->EndMsg();
			}
			break;
		case 0x3:
			dbg_log("udp read from remote :%d bytes", dwTransferBytes);
			if (m_pHandler->BeginMsg(SOCK_PROXY_DATA)){
				m_pHandler->WriteDword(m_dwClientID);
				m_pHandler->WriteWord(0);		//RSV
				m_pHandler->WriteByte(0);		//FRAG
				m_pHandler->WriteByte(1);		//ATYP
				m_pHandler->WriteDword(m_RemoteAddr.sin_addr.S_un.S_addr);
				m_pHandler->WriteWord(m_RemoteAddr.sin_port);
				m_pHandler->WriteBytes((BYTE*)m_Buf.buf, dwTransferBytes);
				m_pHandler->EndMsg();
			}
			break;
		default:
			break;
		}
	}
	//Post Next Read Request.....
	pOverlapped = new SocksOverlapped(SOCKS_READ);
	switch (m_Cmd)
	{
	case 0x1:
		//PostRead....
		iResult = WSARecv(m_RemoteSock, &m_Buf, 1, &dwReadBytes,
			&dwFlag, (OVERLAPPED*)pOverlapped, 0);
		break;
	case 0x3:
		ZeroMemory(&m_RemoteAddr, sizeof(m_RemoteAddr));
		m_FromLen = sizeof(m_RemoteAddr);
		iResult = WSARecvFrom(m_RemoteSock, &m_Buf, 1, &dwReadBytes, &dwFlag, (SOCKADDR*)&
			m_RemoteAddr, &m_FromLen, (WSAOVERLAPPED*)pOverlapped, NULL);
		break;
	default:
		break;
	}
	Unlock();

	if (SOCKET_ERROR == iResult && GetLastError() != WSA_IO_PENDING){
		Lock();
		if (m_RemoteSock != INVALID_SOCKET){
			closesocket(m_RemoteSock);
			m_RemoteSock = INVALID_SOCKET;
			m_pHandler->SendMsg(SOCK_PROXY_CLOSE,   //������....
				//,�п���֮���и�Request..
				&m_dwClientID, sizeof(m_dwClientID));
		}
		Unlock();

		if (pOverlapped)
			delete pOverlapped;

		SetEvent(m_hEvent);
		return;
	}
}



void ClientCtx::OnConnect(BOOL bSuccess){
	DWORD Response[2] = { 0 };
	DWORD Error = 0x0;
	DWORD dwReadBytes = 0;
	DWORD dwFlag = 0;
	SocksOverlapped * pOverlapped = NULL;
	int iResult = 0;

	if (!bSuccess){
		Error = WSAGetLastError();
	}
	else{
		int timeout = 10 * 1000;
		/*dbg_log("connect %s:%d success",
			inet_ntoa(*(in_addr*)m_RemoteAddress),ntohs(m_RemotePort));*/
		
		setsockopt(m_RemoteSock, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
		setsockopt(m_RemoteSock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
	}

	Response[0] = m_dwClientID;
	Response[1] = Error;
	
	//
	Lock();
	if (m_RemoteSock != INVALID_SOCKET){
		m_pHandler->SendMsg(SOCK_PROXY_REQUEST_RESULT,
			Response,sizeof(Response));
	}
	Unlock();
	
	if (Error){			//Failed..
		SetEvent(m_hEvent);
		return;
	}
	//Post Read..
	//connect success...., then post read....
	
	pOverlapped = new SocksOverlapped(SOCKS_READ);
	//PostRead....
	Lock();
	iResult = WSARecv(m_RemoteSock, &m_Buf, 1, &dwReadBytes,
		&dwFlag, (OVERLAPPED*)pOverlapped, 0);
	Unlock();

	if (SOCKET_ERROR == iResult && GetLastError() != WSA_IO_PENDING){
		delete pOverlapped;
		SetEvent(m_hEvent);
		return;
	}
}

//126 
void __stdcall CSocksProxy::work_thread(CSocksProxy*pThis){
	while (TRUE){
		DWORD dwTransferBytes = 0;
		ULONG_PTR CompletioKey = 0;
		BOOL bResult = 0;
		SocksOverlapped * pOverlapped = NULL;

		bResult = GetQueuedCompletionStatus(pThis->m_hCompletionPort, &dwTransferBytes,
			&CompletioKey, (LPOVERLAPPED*)&pOverlapped, INFINITE);

		if (CompletioKey == INVALID_SOCKET){
			break;
		}
		if (CompletioKey){
			ClientCtx*pCtx = (ClientCtx*)CompletioKey;
			switch (pOverlapped->IoType)
			{
			case SOCKS_READ:
				pCtx->OnReadFromRemote(bResult, dwTransferBytes);
				break;
			case SOCKS_CONNECTED:
				pCtx->OnConnect(bResult);
				break;
			default:
				break;
			}
		}
		if (pOverlapped)
			delete pOverlapped;
	}
}



