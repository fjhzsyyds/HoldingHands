#include "IOCPClient.h"
#include "Packet.h"
#include "EventHandler.h"
#include <PROCESS.H>

#pragma comment(lib,"ws2_32.lib")

CIOCPClient::CIOCPClient(const char*ServerAddr, int ServerPort, BOOL bReConnect, UINT MaxTryCount)
{
	//�Զ���������
	m_isConnecting = FALSE;
	m_bReConnect = bReConnect;
	m_MaxTryCount = MaxTryCount;

	//��������Ϣ
	lstrcpyA(m_ServerAddr, ServerAddr);
	m_ServerPort = ServerPort;
	//
	m_ClientSocket = INVALID_SOCKET;
	//
	m_dwRead = 0;
	m_wsaReadBuf.buf = 0;
	m_wsaReadBuf.len = 0;
	//
	m_SendPacketOver = CreateEvent(NULL, FALSE, TRUE, NULL);
	//
	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 2);

	m_pHandler = 0;
	InitializeCriticalSection(&m_csCheck);
}

CIOCPClient::~CIOCPClient()
{
	//
	if (m_ClientSocket != INVALID_SOCKET)
	{
		closesocket(m_ClientSocket);
		m_ClientSocket = INVALID_SOCKET;
	}
	//
	//
	if (m_SendPacketOver != NULL)
	{
		CloseHandle(m_SendPacketOver);
		m_SendPacketOver = NULL;
	}
	//
	if (m_hCompletionPort != NULL)
	{
		CloseHandle(m_hCompletionPort);
		m_hCompletionPort = NULL;
	}
	//
	m_pHandler = NULL;

	DeleteCriticalSection(&m_csCheck);
}

BOOL CIOCPClient::BindHandler(CEventHandler*pHandler)
{
	if (m_pHandler != NULL)
		return FALSE;
	m_pHandler =  pHandler;
	m_pHandler->m_pClient = this;
	return TRUE;
}

BOOL CIOCPClient::UnbindHandler()
{
	if (m_pHandler == NULL)
		return FALSE;
	m_pHandler->m_pClient = NULL;
	m_pHandler = NULL;
	return TRUE;
}
void CIOCPClient::Run()
{
	if (m_hCompletionPort == NULL || m_pHandler == NULL 
		|| m_SendPacketOver == NULL){
		return;
	}
	IocpRun();
}

BOOL CIOCPClient::SocketInit()
{
	WSADATA			wsadata = { 0 };
	WORD			RequestedVersion = MAKEWORD(2, 2);
	//��ʼ��WinSock
	if (SOCKET_ERROR == WSAStartup(RequestedVersion, &wsadata)){
		wprintf(L"WSAStartup failed with error: %u\n", WSAGetLastError());
		return FALSE;
	}
	return TRUE;
}

void CIOCPClient::SocketTerm()
{
	WSACleanup();
}

void CIOCPClient::Disconnect()
{
	EnterCriticalSection(&m_csCheck);
	if (m_ClientSocket != INVALID_SOCKET){
		closesocket(m_ClientSocket);
		m_ClientSocket = INVALID_SOCKET;
	}
	LeaveCriticalSection(&m_csCheck);
}
//���ӷ�����
BOOL CIOCPClient::TryConnect()
{

	INT				iResult = 0;
	LPFN_CONNECTEX	lpfnConnectEx = NULL;
	GUID			GuidConnectEx = WSAID_CONNECTEX;
	DWORD			dwBytes;
	SOCKET			temp;
	
	//printf("TryConnect!\n");
	//�Ȼ�ȡConnectExָ��
	if (lpfnConnectEx == NULL)
	{
		iResult = INVALID_SOCKET;
		temp = socket(AF_INET, SOCK_STREAM, NULL);
		if (temp != INVALID_SOCKET)
		{
			iResult = WSAIoctl(temp, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidConnectEx, sizeof(GuidConnectEx),
				&lpfnConnectEx, sizeof(lpfnConnectEx), &dwBytes, NULL, NULL);
			closesocket(temp);

			if (iResult == SOCKET_ERROR){
				//printf("WSAIoctl failed with error: %u\n", WSAGetLastError());
				WSACleanup();
				return FALSE;
			}
		}
	}
	if (lpfnConnectEx == NULL)
		return FALSE;
	//��һ������ʧ�ܺ�,����ر�socket,���Լ���ʹ��
	if (m_ClientSocket == INVALID_SOCKET)
	{
		m_ClientSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (m_ClientSocket == INVALID_SOCKET)
			return FALSE;
		//�󶨱���
		SOCKADDR_IN local;
		local.sin_family = AF_INET;
		local.sin_addr.S_un.S_addr = INADDR_ANY;
		local.sin_port = 0;
		if (SOCKET_ERROR == bind(m_ClientSocket, (LPSOCKADDR)&local, sizeof(local)))
		{
			closesocket(m_ClientSocket);
			m_ClientSocket = INVALID_SOCKET;
			return FALSE;
		}
		//�����socket �󶨵���ɶ˿�
		if (m_hCompletionPort != CreateIoCompletionPort((HANDLE)m_ClientSocket, m_hCompletionPort, NULL, 0))
		{
			closesocket(m_ClientSocket);
			m_ClientSocket = INVALID_SOCKET;
			return FALSE;
		}//
	}
	//��ȡĿ������ip
	HOSTENT*pHost = gethostbyname(m_ServerAddr);
	SOCKADDR_IN ServerAddr = { 0 };
	ServerAddr.sin_family = AF_INET;
	memcpy(&ServerAddr.sin_addr.S_un.S_addr, pHost->h_addr, 4);
	ServerAddr.sin_port = htons(m_ServerPort);
	//�����������
	m_isConnecting = TRUE;
	//-------------------------Post Connect------------------------ 
	OVERLAPPEDPLUS*pOverlapped = new OVERLAPPEDPLUS(IO_CONNECT);
	//int ret = connect(m_ClientSocket, (sockaddr*)&ServerAddr, sizeof(ServerAddr));
	BOOL bResult = lpfnConnectEx(m_ClientSocket, (sockaddr*)&ServerAddr, sizeof(ServerAddr),&m_pHandler->m_Identity,4,NULL, (OVERLAPPED*)pOverlapped);
	int Error = WSAGetLastError();
	if (FALSE == bResult && Error != WSA_IO_PENDING)
	{
#ifdef _DEBUG
		printf("SOCKET :%u\n", m_ClientSocket);
		printf("WSAGetLastError : %d \n", WSAGetLastError());
#endif
		pOverlapped->m_bManualPost = TRUE;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, 0, (OVERLAPPED*)pOverlapped);
	}
	return TRUE;
}

unsigned int __stdcall CIOCPClient::WorkThread(void*Param)
{
	DWORD dwTransferBytes;
	ULONG ulCompletionKey;
	OVERLAPPEDPLUS *pOverlapped;
	BOOL bResult;
	BOOL bEOF;
	BOOL bLoop = TRUE;
	CIOCPClient*pClient = (CIOCPClient*)Param;	
	HANDLE hCompletionPort = pClient->m_hCompletionPort;
	
	//some module will use 
	CoInitialize(NULL);

	while (bLoop)
	{
		for (;;)
		{
			dwTransferBytes = 0;
			ulCompletionKey = 0;
			pOverlapped = NULL;
			bResult = FALSE;
			bEOF = FALSE;
			bResult = GetQueuedCompletionStatus(hCompletionPort, &dwTransferBytes, &ulCompletionKey, 
				(LPOVERLAPPED*)&pOverlapped, INFINITE);
			//�˳��߳�.
			if (ulCompletionKey == INVALID_SOCKET)
			{
				bLoop = FALSE;
				break;
			}
			//-------------------------------����IO��Ϣ-----------------------------
			bEOF = ((!bResult) || (!dwTransferBytes) || pOverlapped->m_bManualPost);

			if (pOverlapped->m_IOtype == IO_CONNECT)
			{
				int seconds = 0;
				int bytes = sizeof(seconds);
				int iResult = 0;
				if (!bResult || pOverlapped->m_bManualPost || dwTransferBytes != 4) {		//ConnectExʧ��
					break;
				}
				//Update properties;
				setsockopt(pClient->m_ClientSocket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
				//-------------------------------
				pClient->OnConnect();
				//-------------------------------
				//begin recv data:
				pClient->m_wsaReadBuf.buf = pClient->m_ReadPacket.GetBuffer();
				pClient->m_wsaReadBuf.len = PACKET_HEADER_LEN;
				pClient->m_dwRead = 0;
				//Post first read,then the socket will continuously process the read completion statu.;
				pClient->PostRead();
			}
			else if (pOverlapped->m_IOtype == IO_READ){
				pClient->OnReadComplete(dwTransferBytes, bEOF);
				//�Ͽ�������.(ֻ����IO_READ�ж�.)
				if (bEOF){
					pClient->Disconnect();
					pClient->OnClose();
					break;
				}
			}
			else{
				pClient->OnWriteComplete(dwTransferBytes, bEOF);
			}

			if (pOverlapped) delete pOverlapped;
		}
		//���˳��߳�.
		if (bLoop)
		{
			//����ʧ�ܻ��߶Ͽ����ӻ�ִ������
			if (pClient->m_isConnecting)
				pClient->m_FailedCount++;

			//if we will reconnect server.
			if (pClient->m_bReConnect == FALSE || pClient->m_FailedCount >= pClient->m_MaxTryCount){
				//Ͷ��������Ϣ,�˳�.
				for (int i = 0; i < 2; i++){
					PostQueuedCompletionStatus(pClient->m_hCompletionPort, 0,INVALID_SOCKET,0 );
				}
			}
			else{
				Sleep(1000 * ( (rand()%60) + 60 ));
				//���·�������.
				pClient->TryConnect();
			}
		}
	}
	//
	CoUninitialize();
	//clean resources;
	_endthreadex(0);
	return 0;
}

void CIOCPClient::IocpRun()
{
	//һ������IOCP �Ķ�����һ������д����,(ֻ��һ���̻߳��������,Send��ȴ���һ������,�������̴߳����˲���֪������û)
	int i;
	for (i = 0; i < 2; i++)
		m_hWorkThreads[i] = (HANDLE)_beginthreadex(0, 0, WorkThread, (LPVOID)this, 0, 0);

	//ʧ��,ֱ���˳�.
	if (FALSE == TryConnect())
	{
		for (i = 0; i < 2;i++)
			PostQueuedCompletionStatus(m_hCompletionPort, 0, (ULONG_PTR)INVALID_SOCKET, 0);
	}

	while (1){
		DWORD dwResult = WaitForMultipleObjects(2, m_hWorkThreads, TRUE, 60000);
		if (dwResult == WAIT_OBJECT_0)
			break;
		//��ʱ��.
		if (m_ClientSocket != INVALID_SOCKET && !m_isConnecting){
			BOOL bHeartBeatReply = InterlockedExchange((volatile long*)&m_bHeartBeatReply, FALSE);
			if (!bHeartBeatReply){
				Disconnect();
			}
			else{
				SendPacket(HEART_BEAT, 0, 0);
			}
		}
	}

	//������
	for (i = 0; i < 2; i++)
		CloseHandle(m_hWorkThreads[i]);
}

//���Ͱ�.
void CIOCPClient::SendPacket(DWORD command,const char*data, int len)
{
	//�ȴ���һ�����������.
	for (;;)
	{
		DWORD dwResult = WaitForSingleObject(m_SendPacketOver, 500);
		if (dwResult == WAIT_TIMEOUT && m_ClientSocket == INVALID_SOCKET)
			return;
		if (dwResult == WAIT_OBJECT_0)
			break;
	}
	
	if (FALSE == m_WritePacket.AllocateMem(len))
	{
		SetEvent(m_SendPacketOver);//����ʧ��
		return ;
	}
	//Set Header
	m_WritePacket.SetHeader(len, command);
	//Copy Data.
	memcpy(m_WritePacket.GetBody(), data, len);
	//����Header:
	m_dwWrite = 0;
	m_wsaWriteBuf.buf = m_WritePacket.GetBuffer();
	m_wsaWriteBuf.len = m_WritePacket.GetBodyLen() + PACKET_HEADER_LEN;

	//m_WritePacket.Encrypt(0,m_wsaWriteBuf.len);
	PostWrite();
}


void CIOCPClient::OnWriteComplete(DWORD nTransferredBytes, BOOL bEOF)
{
	if (!bEOF){
		m_dwWrite += nTransferredBytes;
		//�������������Դ���.
		if (m_dwWrite == m_WritePacket.GetBodyLen() + PACKET_HEADER_LEN){
			if (m_WritePacket.GetCommand()!=HEART_BEAT)
				m_pHandler->OnWriteComplete(m_WritePacket.GetCommand(), m_WritePacket.GetBodyLen(), 
				m_dwWrite, m_WritePacket.GetBody());
		}
		else if (m_dwWrite < m_WritePacket.GetBodyLen() + PACKET_HEADER_LEN){
			if (m_dwWrite > PACKET_HEADER_LEN && m_WritePacket.GetCommand() != HEART_BEAT){
				m_pHandler->OnWritePartial(m_WritePacket.GetCommand(), m_WritePacket.GetBodyLen(),
					m_dwWrite, m_WritePacket.GetBody());
			}
			m_wsaWriteBuf.buf = m_WritePacket.GetBuffer() + m_dwWrite;
			m_wsaWriteBuf.len = m_WritePacket.GetBodyLen() + PACKET_HEADER_LEN - m_dwWrite;
			//continue send data
			PostWrite();
			return;
		}
	}
	m_dwWrite = 0;
	m_wsaWriteBuf.buf = 0;
	m_wsaWriteBuf.len = 0;
	SetEvent(m_SendPacketOver);
}
//ReadIO�������
void CIOCPClient::OnReadComplete(DWORD nTransferredBytes, BOOL bEOF)
{
	if (bEOF)
	{
		m_dwRead = 0;
		return;
	}
	//m_ReadPacket.Encrypt(m_dwRead,nTransferredBytes);
	//�ɹ�,�����ݵ��
	m_dwRead += nTransferredBytes;
	//��ȡ���
	if (m_dwRead >= PACKET_HEADER_LEN
		&& (m_dwRead == PACKET_HEADER_LEN + m_ReadPacket.GetBodyLen()))
	{
		//������ɵİ�
		if (m_ReadPacket.GetCommand() == HEART_BEAT){
#ifdef _DEBUG
			printf("Recv Heart Beat\n");
#endif
			m_bHeartBeatReply = TRUE;
		}
		else{

			WORD Event = m_ReadPacket.GetCommand();
			DWORD dwTotal = m_ReadPacket.GetBodyLen();						//buffer�ܳ���
			DWORD dwRead = m_dwRead - PACKET_HEADER_LEN;					//�Ѿ���ȡ�ĳ���
			m_pHandler->OnReadComplete(Event, dwTotal, dwRead, m_ReadPacket.GetBody());
		}
		m_dwRead = 0;
	}
	//�Ȱ�PacketHeader������
	if (m_dwRead < PACKET_HEADER_LEN)
	{
		m_wsaReadBuf.buf = m_ReadPacket.GetBuffer();
		m_wsaReadBuf.len = PACKET_HEADER_LEN - m_dwRead;
	}
	else
	{
		BOOL bOk = m_ReadPacket.Verify();
		BOOL bMemEnough = TRUE;
		//Ϊ�գ��ȷ�����Ӧ�ڴ�,����֮ǰ��У��ͷ���Ƿ���ȷ
		if (bOk)
		{
			bMemEnough = m_ReadPacket.AllocateMem(m_ReadPacket.GetBodyLen());
		}
		if (bOk && m_dwRead > PACKET_HEADER_LEN)
		{
			//������ɵİ�
			WORD Event = m_ReadPacket.GetCommand();
			DWORD dwTotal = m_ReadPacket.GetBodyLen();
			DWORD dwRead = m_dwRead - PACKET_HEADER_LEN;
			m_pHandler->OnReadPartial(Event, dwTotal, dwRead, m_ReadPacket.GetBody());
		}
		//���bOk�ң��ڴ����ɹ����������¸�ֵ��������һ��Post��Ͽ�����ͻ�������
		m_wsaReadBuf.buf = NULL;
		m_wsaReadBuf.len = 0;
		//�ڴ���ܷ���ʧ��
		if (bOk && bMemEnough)
		{
			//����ƫ��
			m_wsaReadBuf.buf = m_ReadPacket.GetBuffer() + m_dwRead;
			//����ʣ�೤��
			m_wsaReadBuf.len = m_ReadPacket.GetBodyLen() + PACKET_HEADER_LEN - m_dwRead;
		}
	}
	//Ͷ����һ������
	PostRead();
}
//Ͷ��һ��ReadIO����
void CIOCPClient::PostRead()
{
	DWORD nReadBytes = 0;
	DWORD flag = 0;
	int nIORet, nErrorCode;
	OVERLAPPEDPLUS*pOverlapped = new OVERLAPPEDPLUS(IO_READ);
	//printf("CIOCPClient::PostRead()!\n");
	EnterCriticalSection(&m_csCheck);
	//assert(pClientContext->m_ClientSocket == INVALID_SOCKET);
	nIORet = WSARecv(m_ClientSocket, &m_wsaReadBuf, 1, &nReadBytes, &flag, (LPOVERLAPPED)pOverlapped, NULL);
	nErrorCode = WSAGetLastError();
	//IO����,��SOCKET���ر���,����buffΪNULL;
	if (nIORet == SOCKET_ERROR	&& nErrorCode != WSA_IO_PENDING)
	{
		//��������Ϊ�ֶ�Ͷ�ݵ�
		pOverlapped->m_bManualPost = TRUE;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, 0, (LPOVERLAPPED)pOverlapped);
	}
	LeaveCriticalSection(&m_csCheck);
}

void CIOCPClient::PostWrite()
{
	DWORD nWriteBytes = 0;
	DWORD flag = 0;
	int nIORet, nErrorCode;
	OVERLAPPEDPLUS*pOverlapped = new OVERLAPPEDPLUS(IO_WRITE);

	EnterCriticalSection(&m_csCheck);

	nIORet = WSASend(m_ClientSocket, &m_wsaWriteBuf, 1, &nWriteBytes, flag, (LPOVERLAPPED)pOverlapped, NULL);
	nErrorCode = WSAGetLastError();
	//IO����,��SOCKET���ر���,����buffΪNULL;
	if (nIORet == SOCKET_ERROR	&& nErrorCode != WSA_IO_PENDING)
	{
		pOverlapped->m_bManualPost = TRUE;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, 0, (LPOVERLAPPED)pOverlapped);
	}
	LeaveCriticalSection(&m_csCheck);
}
void CIOCPClient::OnClose()
{
	//�ȴ�Write���.
	WaitForSingleObject(m_SendPacketOver, INFINITE);
	m_pHandler->OnClose();
}

void CIOCPClient::OnConnect()
{
	m_FailedCount = 0;				
	m_isConnecting = FALSE;
	//���ó�ʼֵΪTRUE;
	InterlockedExchange((volatile long*)&m_bHeartBeatReply, TRUE);
	SetEvent(m_SendPacketOver);	

	//����sockaddr��peer addr
	m_PeerPort = 0;
	m_SockPort = 0;

	m_szPeerAddr[0] = 0;
	m_szSockAddr[0] = 0;

	sockaddr_in addr = { 0 };
	int namelen = sizeof(addr);
	//����
	getsockname(m_ClientSocket, (sockaddr*)&addr, &namelen);
	lstrcpyA(m_szSockAddr, inet_ntoa(addr.sin_addr));
	m_SockPort = addr.sin_port;
	//�Է�
	namelen = sizeof(addr);
	getpeername(m_ClientSocket, (sockaddr*)&addr, &namelen);
	lstrcpyA(m_szPeerAddr, inet_ntoa(addr.sin_addr));
	m_PeerPort = addr.sin_port;
	//����Handler����
	printf("CIOCPClient::OnConnect()\n");

	m_pHandler->OnConnect();
}

void CIOCPClient::GetPeerName(char* Addr, USHORT&Port)
{
	Port = m_PeerPort;
	lstrcpyA(Addr, m_szPeerAddr);
}
void CIOCPClient::GetSockName(char* Addr, USHORT&Port)
{
	Port = m_SockPort;
	lstrcpyA(Addr, m_szSockAddr);
}

void CIOCPClient::GetSrvName(char*Addr, USHORT&Port)
{
	Port = m_ServerPort;
	lstrcpyA(Addr, m_ServerAddr);
}