#include "stdafx.h"
#include "IOCPServer.h"
#include "Packet.h"
#include "Manager.h"
#include "ClientContext.h"
#include "MsgHandler.h"

#include "utils.h"
//��ʼ��ΪNULL
CIOCPServer*	CIOCPServer::hInstance = NULL;
LPFN_ACCEPTEX	CIOCPServer::lpfnAcceptEx = NULL;

//����һ��������ʵ��
CIOCPServer* CIOCPServer::CreateServer(HWND hNotifyWnd)
{
	if (hInstance == NULL){
		hInstance = new CIOCPServer(hNotifyWnd);
	}
	return hInstance;
}

void CIOCPServer::DeleteServer()
{
	if (hInstance){
		delete hInstance;
		hInstance = NULL;
	}
}

CIOCPServer::CIOCPServer(HWND hWnd)
{
	m_hNotifyWnd = hWnd;
	//Listen Socket
	m_ListenSocket = INVALID_SOCKET;
	//Accept Socket
	m_AcceptSocket = INVALID_SOCKET;
	memset(m_AcceptBuf, 0, sizeof(m_AcceptBuf));
	//Context:
	m_pClientContextList = new CPtrList;
	m_pFreeContextList = new CPtrList;
	InitializeCriticalSection(&m_csContext);
	//
	m_hCompletionPort = INVALID_HANDLE_VALUE;
	m_AssociateClientCount = 0;
	//
	m_hStopRunning = CreateEvent(0, TRUE, TRUE, NULL);			//��ʼ״̬��TRUE,�Ѿ�ֹͣ
	//Event Dispatch
	m_pManager = new CManager(this);
	m_pManager->m_pServer = this;


	m_pThreadList = new CMapPtrToPtr;
	m_BusyCount = 0;
	m_MaxConcurrent = 0;
	m_MaxThreadCount = 0;

	m_ReadSpeed = 0;
	m_WriteSpeed = 0;

	m_SpeedLock = 0;

	InitializeCriticalSectionAndSpinCount(&m_csThread,5000);
}

CIOCPServer::~CIOCPServer()
{
	//if server is running,stop the server;
	StopServer();
	//release resource
	//Release context:
	if (m_pClientContextList){
		while (m_pClientContextList->GetCount() > 0)
			delete (CClientContext*)m_pClientContextList->RemoveHead();
		delete m_pClientContextList;
		m_pClientContextList = NULL;
	}
	m_pClientContextList = NULL;

	if (m_pFreeContextList){
		while (m_pFreeContextList->GetCount() > 0)
			delete (CClientContext*)m_pFreeContextList->RemoveHead();
		delete m_pFreeContextList;
		m_pFreeContextList = NULL;
	}
	m_pFreeContextList = NULL;

	DeleteCriticalSection(&m_csContext);

	//
	if (m_pThreadList){
		delete m_pThreadList;
		m_pThreadList = NULL;
	}
	DeleteCriticalSection(&m_csThread);
	//
	if (m_pManager){
		delete m_pManager;
		m_pManager = NULL;
	}
	//
	if (m_hStopRunning){
		CloseHandle(m_hStopRunning);
		m_hStopRunning = NULL;
	}
	m_hStopRunning = NULL;
}

//�ӵ�list����
void CIOCPServer::AddToList(CClientContext*pContext)
{
	//Add pClientContext to ClientContextList and save the position(to remove this context from the list quickly)
	EnterCriticalSection(&m_csContext);
	pContext->m_PosInList = m_pClientContextList->AddTail(pContext);
	LeaveCriticalSection(&m_csContext);
}

//�Ƴ�
BOOL CIOCPServer::RemoveFromList(CClientContext*pContext)
{
	BOOL bRet = FALSE;
	EnterCriticalSection(&m_csContext);
	if (pContext->m_PosInList)
	{
		m_pClientContextList->RemoveAt(pContext->m_PosInList);
		pContext->m_PosInList = NULL;
		bRet = TRUE;
	}
	LeaveCriticalSection(&m_csContext);
	return bRet;
}
DWORD CIOCPServer::GetReadSpeed()
{
	return m_ReadSpeed;
}
DWORD CIOCPServer::GetWriteSpeed()
{
	return m_WriteSpeed;
}

void CIOCPServer::UpdateSpeed(DWORD io_type, DWORD TranferredBytes)
{
	static DWORD dwReadBytes = 0;
	static DWORD dwWriteBytes = 0;
	static DWORD LastUpdateReadTime = 0;
	static DWORD LastUpdateWriteTime = 0;

	static DWORD dwCurTime = 0;

	dwCurTime = GetTickCount();
	if (io_type&IO_READ)
	{
		dwReadBytes += TranferredBytes;
		if (dwCurTime - LastUpdateReadTime >= 1000)
		{
			InterlockedExchange(&m_ReadSpeed, dwReadBytes/(dwCurTime - LastUpdateReadTime));
			dwReadBytes = 0;
			LastUpdateReadTime = dwCurTime;
		}
	}
	if (io_type&IO_WRITE)
	{
		dwWriteBytes += TranferredBytes;
		if (dwCurTime - LastUpdateWriteTime >= 1000)
		{
			InterlockedExchange(&m_WriteSpeed, dwWriteBytes/(dwCurTime - LastUpdateWriteTime));
			dwWriteBytes = 0;			//ˢ��
			LastUpdateWriteTime = dwCurTime;
		}
	}
}


#define _FLAG_GQCS_FAILED			0x1
#define _FLAG_MANUAL_POST			0x2
#define _FLAG_NO_BYTES				0x4
#define _FLAG_IO_ABORTED			0x8							//������ֹ
//�����̺߳���
unsigned int __stdcall CIOCPServer::WorkerThread(void*)
{
	//
	HANDLE hCurrentThread;
	DWORD			CurrentThreadId;
	CIOCPServer*	pServer;
	BOOL			bResult;
	DWORD			nTransferredBytes;
	CClientContext *pClientContext;
	OVERLAPPEDPLUS *pOverlappedplus;

	BOOL			bLoop;
	DWORD			dwResult;
	//�����߳��������GetCurrentThread�������ص���һ��α���
	//�������GetCurrentThreadId;
	hCurrentThread = NULL;
	CurrentThreadId = GetCurrentThreadId();
	pServer = CIOCPServer::CreateServer(NULL);
	bLoop = TRUE;
	while (bLoop)
	{
		bResult = FALSE;
		nTransferredBytes = 0;
		pClientContext = NULL;
		pOverlappedplus = NULL;

		dwResult = 0;

		bResult = GetQueuedCompletionStatus(pServer->m_hCompletionPort,
			&nTransferredBytes, (PULONG_PTR)&pClientContext, (LPOVERLAPPED*)&pOverlappedplus, 4000);
		//��ʱ
		if (!bResult && GetLastError() == WAIT_TIMEOUT){
			if (!InterlockedExchange(&pServer->m_SpeedLock, 1)){
				pServer->UpdateSpeed(IO_READ | IO_WRITE, 0);
				InterlockedExchange(&pServer->m_SpeedLock, 0);
			}
			continue;
		}
		//�˳��߳�,Ͷ�����˳���Ϣ
		if (INVALID_SOCKET == (SOCKET)pClientContext)
			break;
		
		
		dbg_log("GetQueuedCompletionStatus");
		//1.bRet == false,nTransferredBytes==0,pOverlappedplus==0,pClientContext==0,				û��ȡ��
		//2.bRet == false,pOverlappedplus��0,pClientContext��0��										ȡ����һ��ʧ�ܵ�IO��ɰ�
		//3.bRet == false,nTransferredBytes==0,pOverlappedplus��0,pClientContext��0					���� socket ������ر�
		
		//ʵ���֪,AcceptEx����ɼ� ��ListenSocket;
		//ȡ����һ��IO��ɰ������������������ʧ�ܵĽ���������ǳɹ��Ľ��
		if (pClientContext)
		{
			if (!bResult)
				dwResult |= _FLAG_GQCS_FAILED;
			if (pOverlappedplus->m_bManualPost)
				dwResult |= _FLAG_MANUAL_POST;
			if (!nTransferredBytes)
				dwResult |= _FLAG_NO_BYTES;
			if (WSAGetLastError() == 995)
				dwResult |= _FLAG_IO_ABORTED;

			/*
				����IO_READ,IO_WRITE,IO_ACCEPT;
				�˴���Ҫ���򵥵��̵߳���,��nBusyCount == ThreadCount,��ô�ʹ���һ���µ��߳�
				(Ϊ�˷�ֹ�����̶߳����������writeIOMsg�޷�������,emmm���������������ļ��ʺ�С)
				ֻҪ�̴߳��ڹ���(SendMessage,SuspenThread,WaitForSingleObject��ʱ),�´������̲߳ŻᱻIOCP���
				�˿�ʹ��.�����µ��߳�ûʲô����,ʼ�մ��ڹ���״̬.ԭ����Ϊ�˱�֤��󲢷�����С�� ��ʼ�趨ֵ��
			*/
			//update speed
			if (0 == InterlockedExchange(&pServer->m_SpeedLock, 1)){
				if (pOverlappedplus != NULL && (pOverlappedplus->m_IOtype == IO_READ
					|| pOverlappedplus->m_IOtype == IO_WRITE)){
					pServer->UpdateSpeed(pOverlappedplus->m_IOtype, nTransferredBytes);
				}
				InterlockedExchange(&pServer->m_SpeedLock, 0);
			}
			
			//����߳�.
			EnterCriticalSection(&pServer->m_csThread);
			++pServer->m_BusyCount;

			if (pServer->m_BusyCount == pServer->m_pThreadList->GetCount()){
				//û�п�����߳�.Ϊ�˷�ֹ�����̶߳��ǹ���(��GQCP)��״̬,Ҫ�����µ��߳�
				unsigned int ThreadId = 0;
				HANDLE hThread = 0;
				hThread = (HANDLE)_beginthreadex(0, 0, WorkerThread, 0, CREATE_SUSPENDED, &ThreadId);
				
				if (hThread != NULL && ThreadId !=NULL)
				{
					pServer->m_pThreadList->SetAt((void*)ThreadId, hThread);
					ResumeThread(hThread);
				}
				//���ʧ�ܵĻ�,emmm.......
			}
			LeaveCriticalSection(&pServer->m_csThread);

			//dispatch io message 
			pServer->HandlerIOMsg(pClientContext, nTransferredBytes, pOverlappedplus->m_IOtype, dwResult);
			
			//���socket�Ƿ�Ͽ�����.
			if (pOverlappedplus->m_IOtype == IO_READ && dwResult){
				pClientContext->Disconnect();
				pServer->RemoveFromList(pClientContext);
				//����IO_CLOSE
				pServer->HandlerIOMsg(pClientContext, 0, IO_CLOSE, 0);
			}

			//������IO�¼�
			EnterCriticalSection(&pServer->m_csThread);
			pServer->m_BusyCount--;					//�߳�һ��������IO��Ϣ,�͵ݼ�
			//��ǰ�߳�����������߳���.��ô���õ�ǰ�߳��˳���
			if (pServer->m_pThreadList->GetCount() > pServer->m_MaxThreadCount)
			{
				if(pServer->m_pThreadList->Lookup((void*)CurrentThreadId,hCurrentThread)){
					pServer->m_pThreadList->RemoveKey((void*)CurrentThreadId);
				}
				bLoop = FALSE;//������߳̽�����
			}
			LeaveCriticalSection(&pServer->m_csThread);
		}
		if (pOverlappedplus) 
			delete pOverlappedplus;
	}
	if (!bLoop){	
		CloseHandle(hCurrentThread);	//��ǰ�߳�����������߳��������.�Լ��˳�,��Ҫ�Լ��رվ��.
	}
	//
	_endthreadex(0);					//_beginthreadex֮���������
	return 0;
}

void CIOCPServer::HandlerIOMsg(CClientContext*pClientContext, DWORD nTransferredBytes, DWORD IoType, DWORD dwFailedReason)
{
	switch (IoType)
	{
	case IO_ESTABLISHED:
		OnAcceptComplete(dwFailedReason);
		break;
	case IO_WRITE:
		pClientContext->OnWriteComplete(nTransferredBytes, dwFailedReason);
		break;
	case IO_READ:
		pClientContext->OnReadComplete(nTransferredBytes, dwFailedReason);
		break;
	case IO_CLOSE:
		pClientContext->OnClose();
		InterlockedDecrement(&m_AssociateClientCount);
		FreeContext(pClientContext);
		break;
	}
}



//Ͷ��һ��ReadIO����
void CIOCPServer::PostRead(CClientContext*pClientContext)
{
	DWORD nReadBytes = 0;
	DWORD flag = 0;
	int nIORet, nErrorCode;
	OVERLAPPEDPLUS*pOverlapped = new OVERLAPPEDPLUS(IO_READ);
	//assert(pClientContext->m_ClientSocket == INVALID_SOCKET);

	EnterCriticalSection(&pClientContext->m_csCheck);

	//��ֹWSARecv��ʱ��m_ClientSocketֵ�ı�.
	nIORet = WSARecv(pClientContext->m_ClientSocket, &pClientContext->m_wsaReadBuf, 1, &nReadBytes, &flag, (LPOVERLAPPED)pOverlapped, NULL);
	nErrorCode = WSAGetLastError();
	//IO����,��SOCKET���ر���,����buffΪNULL;
	if (nIORet == SOCKET_ERROR	&& nErrorCode != WSA_IO_PENDING){
		//��������Ϊ�ֶ�Ͷ�ݵ�
		pOverlapped->m_bManualPost = TRUE;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, (ULONG_PTR)pClientContext, (LPOVERLAPPED)pOverlapped);
	}
	LeaveCriticalSection(&pClientContext->m_csCheck);
}
//--------------------------------------------------------------------------------------------------------------
//Ͷ��һ��WriteIO����
void CIOCPServer::PostWrite(CClientContext*pClientContext)
{
	DWORD nWriteBytes = 0;
	DWORD flag = 0;
	int nIORet, nErrorCode;
	OVERLAPPEDPLUS*pOverlapped = new OVERLAPPEDPLUS(IO_WRITE);
	//assert(pClientContext->m_ClientSocket == INVALID_SOCKET);
	EnterCriticalSection(&pClientContext->m_csCheck);
	nIORet = WSASend(pClientContext->m_ClientSocket, &pClientContext->m_wsaWriteBuf, 1, 
		&nWriteBytes, flag, (LPOVERLAPPED)pOverlapped, NULL);
	nErrorCode = WSAGetLastError();
	//IO����,��SOCKET���ر���,����buffΪNULL;
	if (nIORet == SOCKET_ERROR	&& nErrorCode != WSA_IO_PENDING){
		//��������Ϊ�ֶ�Ͷ�ݵ�
		pOverlapped->m_bManualPost = TRUE;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, (ULONG_PTR)pClientContext, (LPOVERLAPPED)pOverlapped);
	}
	LeaveCriticalSection(&pClientContext->m_csCheck);
}

//----------------------------------------------------------------------------------------------------------
//��������
void CIOCPServer::OnAcceptComplete(DWORD dwFailedReason)
{
	//˵����һ��Ͷ�ݳɹ�������m_AcceptSocket��һ����Ч��socket
	//allocate a new context to save the accept socket
	CClientContext*pClientContext = AllocateContext(m_AcceptSocket);
	//reset the m_AcceptSocket
	m_AcceptSocket = INVALID_SOCKET;
	HANDLE hRet = NULL;
	//�󶨶˿�

	if (!dwFailedReason){
		hRet = CreateIoCompletionPort((HANDLE)pClientContext->m_ClientSocket,
			m_hCompletionPort, (ULONG_PTR)pClientContext, NULL);
	}
	if (!dwFailedReason && hRet == m_hCompletionPort)
	{
		//��������˵��������ճɹ��˶��ҳɹ��İ�����ɶ˿�
		AddToList(pClientContext);
		InterlockedIncrement(&m_AssociateClientCount);
		//update context:
		setsockopt(pClientContext->m_ClientSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
			(char *)&m_ListenSocket, sizeof(m_ListenSocket));
		//save remote addr and local addr
		memcpy(&pClientContext->m_Identity, m_AcceptBuf, IDENTITY_LEN);
		//save addr and port
		sockaddr_in addr = { 0 };
		int namelen = sizeof(addr);
		getsockname(pClientContext->m_ClientSocket, (sockaddr*)&addr, &namelen);
		strcpy(pClientContext->m_szSockAddr, inet_ntoa(addr.sin_addr));
		pClientContext->m_SockPort = addr.sin_port;

		memset(&addr, 0, sizeof(addr));
		namelen = sizeof(addr);
		getpeername(pClientContext->m_ClientSocket, (sockaddr*)&addr, &namelen);
		strcpy(pClientContext->m_szPeerAddr, inet_ntoa(addr.sin_addr));
		pClientContext->m_PeerPort = addr.sin_port;
		//set keep alive
		int keepalive = 1;
		if (SOCKET_ERROR != setsockopt(pClientContext->m_ClientSocket, SOL_SOCKET, 
			SO_KEEPALIVE, (char*)&keepalive, sizeof(keepalive))){
			tcp_keepalive in_keep_alive = { 0 };
			unsigned long ul_in_len = sizeof(struct tcp_keepalive);

			tcp_keepalive out_keep_alive = { 0 };
			unsigned long ul_out_len = sizeof(struct tcp_keepalive);

			unsigned long ul_bytes_return = 0;

			in_keep_alive.onoff = 1;						/*��keepalive*/
			in_keep_alive.keepaliveinterval = 5000;			/*����keepalive����ʱ����-��λΪ����*/				
			in_keep_alive.keepalivetime = 40000;			/* 40s û�б��Ŀ�ʼ����keepalive������-��λΪ����*/

			WSAIoctl(pClientContext->m_ClientSocket, SIO_KEEPALIVE_VALS, (LPVOID)&in_keep_alive, ul_in_len,
				(LPVOID)&out_keep_alive, ul_out_len, &ul_bytes_return, NULL, NULL);
		}
		//
		//-----------------------------------------------------------------------------------------
		pClientContext->OnConnect();
		//-----------------------------------------------------------------------------------------
		//�Ƚ������ݰ���Header
		pClientContext->m_dwRead = 0;
		pClientContext->m_wsaReadBuf.buf = pClientContext->m_ReadPacket.GetBuffer();
		pClientContext->m_wsaReadBuf.len = PACKET_HEADER_LEN;
		//Post first read,then the socket will continuously process the read completion statu.;
		PostRead(pClientContext);
		//Accept next client
		PostAccept();
		return;
	}
	//��ʧ��,�رս��յ�socket
	if (pClientContext->m_ClientSocket != INVALID_SOCKET){
		closesocket(pClientContext->m_ClientSocket);
		pClientContext->m_ClientSocket = INVALID_SOCKET;
	}
	FreeContext(pClientContext);
	//������ܵ�һ��socket ,���ǶԷ�û�з������ݶ���ֱ�ӶϿ�������Ҳ��ʧ��.
	//����Ҫ���յ�4 ���ֽڵ� identity�Ż�ɹ�.
	if ((dwFailedReason & _FLAG_IO_ABORTED) || (dwFailedReason & _FLAG_MANUAL_POST)){
		//��ֹaccept
		SetEvent(m_hStopRunning);
		//֪ͨ������ֹ
		m_pManager->ProcessCompletedPacket(PACKET_ACCEPT_ABORT, pClientContext, NULL);
	}
	else{
		PostAccept();		//PostNext Accept;
	}
}

void CIOCPServer::PostAccept()
{
	//if m_AcceptSocket == INVALID_SOCKET,then AcceptEx will return FALSE
	m_AcceptSocket = socket(AF_INET, SOCK_STREAM, 0);
	//setsockopt(m_AcceptSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&m_ListenSocket, sizeof(m_ListenSocket));
	memset(m_AcceptBuf, 0, REMOTE_AND_LOCAL_ADDR_LEN);
	OVERLAPPEDPLUS *pOverlappedPlus = new OVERLAPPEDPLUS(IO_ESTABLISHED);

	BOOL bAcceptRet = FALSE;
	int nErrorCode = 0;

	bAcceptRet = lpfnAcceptEx(m_ListenSocket, m_AcceptSocket, m_AcceptBuf,IDENTITY_LEN
		, REMOTE_AND_LOCAL_ADDR_LEN / 2, REMOTE_AND_LOCAL_ADDR_LEN / 2,
		NULL, (OVERLAPPED*)pOverlappedPlus);

	nErrorCode = WSAGetLastError();

	if (bAcceptRet == FALSE && nErrorCode != WSA_IO_PENDING){
		//Post accept error,m_bManualPost == TRUE means PostAcceptError,
		//then the valid socket will be closed by OnAcceptCompletion
		pOverlappedPlus->m_bManualPost = TRUE;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, (ULONG_PTR)m_ListenSocket, (OVERLAPPED*)pOverlappedPlus);
	}
}

BOOL CIOCPServer::SocketInit()
{
	WSADATA			wsadata = { 0 };
	WORD			RequestedVersion = MAKEWORD(2, 2);
	INT				iResult = 0;
	LPFN_ACCEPTEX	lpfnAcceptEx = NULL;
	GUID			GuidAcceptEx = WSAID_ACCEPTEX;
	DWORD			dwBytes;
	SOCKET			temp;
	//��ʼ��WinSock
	if (SOCKET_ERROR == WSAStartup(RequestedVersion, &wsadata)){
		dbg_log("WSAStartup failed with error: %u", WSAGetLastError());
		return FALSE;
	}
	iResult = INVALID_SOCKET;
	//��ȡAcceptEx����ָ��
	temp = socket(AF_INET,SOCK_STREAM,NULL);
	if (temp != INVALID_SOCKET){
		iResult = WSAIoctl(temp, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx, sizeof(GuidAcceptEx),
			&lpfnAcceptEx, sizeof(lpfnAcceptEx), &dwBytes, NULL, NULL);
		closesocket(temp);
	}
	if (iResult == SOCKET_ERROR) {
		dbg_log("WSAIoctl failed with error: %u", WSAGetLastError());
		WSACleanup();
		return FALSE;
	}
	CIOCPServer::lpfnAcceptEx = lpfnAcceptEx;
	dbg_log("SocketInit Succeed!, CIOCPServer::lpfnAcceptEx: %p", lpfnAcceptEx);
	return TRUE;
}

void CIOCPServer::SocketTerm()
{
	WSACleanup();
}

BOOL CIOCPServer::StartServer(USHORT uPort)
{
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
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	addr.sin_port = htons(uPort);
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
	//Set thread counts;
	GetSystemInfo(&si);
	//��󲢷�������12��cpu ,24
	int MaxConcurrent = si.dwNumberOfProcessors * 2;

	//Create completion port
	m_hCompletionPort = CreateIoCompletionPort((HANDLE)m_ListenSocket, NULL, 
		(ULONG_PTR)m_ListenSocket, MaxConcurrent);
	if (NULL == m_hCompletionPort)
		goto Error;

	m_AssociateClientCount = 0;
	m_BusyCount = 0;
	m_MaxConcurrent = MaxConcurrent;
	m_MaxThreadCount = m_MaxConcurrent + 4;//����߳���������Ϊ��󲢷�������4;
	//
	//-------------------------------------------------------------------------------------------------------------
	//Create Threads;
	for (i = 0; i < MaxConcurrent; i++){
		//Create threads and save their handles and ids;
		unsigned int ThreadId = 0;
		HANDLE hThread = 0;
		hThread = (HANDLE)_beginthreadex(0, 0, WorkerThread, 0, 0, &ThreadId);
		if (hThread != NULL && ThreadId!=NULL){
			m_pThreadList->SetAt((void*)ThreadId, hThread);
		}
	}

	//-------------------------------------------------------------------------------------------------------------
	//Beign accepting client socket;
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

void CIOCPServer::StopServer()
{
	//�Ѿ����ر�
	if (m_ListenSocket == INVALID_SOCKET)
		return;
	//close ListenSocket��AcceptEx will return false or GetCompletionStatus will return 0
	closesocket(m_ListenSocket);
	m_ListenSocket = INVALID_SOCKET;

	//wait AcceptCompletion is handled,�ȴ�m_bAccept���FALSE
	WaitForSingleObject(m_hStopRunning,INFINITE);
	
	//close all sockets;

	EnterCriticalSection(&m_csContext);
	for (POSITION pos = m_pClientContextList->GetHeadPosition(); pos != NULL; m_pClientContextList->GetNext(pos)){
		CClientContext*pContext = (CClientContext*)m_pClientContextList->GetAt(pos);
		while (true){
			if (TryEnterCriticalSection(&pContext->m_csCheck)){
				pContext->Disconnect();								//�����ر�socket,����Ĺ������̺߳���ȥ����
				LeaveCriticalSection(&pContext->m_csCheck);
				break;												//����ѭ��
			}
			else if (pContext->m_ClientSocket == INVALID_SOCKET){	//û�н���,���ǿ���ȷ����Context�Ѿ����ر��ˡ�{
				break;
			}
			Sleep(10);
		}
	}
	//�����д������֮ǰ�����ܻ��кܶ๤���߳̿�����RemoveFromList����
	LeaveCriticalSection(&m_csContext);

	//�ȴ����пͻ��˵�IO�������
	dbg_log("Wait all ctx exit...");
	while (m_AssociateClientCount>0) Sleep(10);
	
	//�ȴ������̴߳���GCQS
	dbg_log("Wait all busy thread...");
	while (m_BusyCount > 0)	Sleep(10);
	//�����˳���Ϣ

	dbg_log("PostQueuedCompletionStatus To All Threads.");
	for (int i = 0; i < m_pThreadList->GetCount(); i++)		{
		PostQueuedCompletionStatus(m_hCompletionPort, 0, INVALID_SOCKET, NULL);
	}
	//�ȴ������߳��˳���
	//�ȴ��߳��˳�.
	dbg_log("Wait All Threads Exit.");
	POSITION pos = m_pThreadList->GetStartPosition();
	while (pos != NULL){
		void* handle = 0,* threadid = 0;
		m_pThreadList->GetNextAssoc(pos, threadid, handle);
		WaitForSingleObject(handle, INFINITE);
		CloseHandle(handle);
	}
	m_pThreadList->RemoveAll();
	m_MaxThreadCount = 0;
	m_MaxConcurrent = 0;
	m_BusyCount = 0;
	//�ر���ɶ˿�
	m_ReadSpeed = 0;
	m_WriteSpeed = 0;

	CloseHandle(m_hCompletionPort);
	m_hCompletionPort = NULL;
	dbg_log("Stop Server Succeed!");
}


CClientContext* CIOCPServer::AllocateContext(SOCKET ClientSocket)
{
	CClientContext*pClientContext = NULL;

	EnterCriticalSection(&m_csContext);
	if (m_pFreeContextList->GetCount() > 0){
		pClientContext = (CClientContext*)m_pFreeContextList->RemoveHead();
		pClientContext->m_ClientSocket = ClientSocket;
		pClientContext->m_pServer = this;
		
		dbg_log("CIOCPServer::AllocateContext : Use Free Context.");
		ASSERT(SetEvent(pClientContext->m_SendPacketOver));				//���½��¼���Ϊ����״̬��

	}
	LeaveCriticalSection(&m_csContext);

	if (pClientContext == NULL){
		pClientContext = new CClientContext(ClientSocket,this,m_pManager);
	}
	return pClientContext;
}

void CIOCPServer::FreeContext(CClientContext*pContext){
	//�ƺ�����
	SetEvent(pContext->m_SendPacketOver);			//���½��¼���Ϊ����״̬��
	//
	pContext->m_PosInList = NULL;
	//clean
	if (pContext->m_ClientSocket != INVALID_SOCKET){
		closesocket(pContext->m_ClientSocket);
		pContext->m_ClientSocket = INVALID_SOCKET;
	}

	//don't clean pReadPacket and pWritePacket,because they can be used when they 
	//are removed from FreeContext list;
	pContext->m_wsaReadBuf.buf = 0;
	pContext->m_wsaReadBuf.len = 0;
	pContext->m_dwRead = 0;

	pContext->m_wsaWriteBuf.buf = 0;
	pContext->m_wsaWriteBuf.len = 0;
	pContext->m_dwWrite = 0;
	//

	pContext->m_Identity = 0;
	pContext->m_szPeerAddr[0] = 0;
	pContext->m_szSockAddr[0] = 0;

	pContext->m_PeerPort = 0;
	pContext->m_SockPort = 0;

	EnterCriticalSection(&m_csContext);
	m_pFreeContextList->AddTail(pContext);
	LeaveCriticalSection(&m_csContext);
}



void CIOCPServer::async_svr_ctrl_proc(DWORD dwParam)
{
	DWORD dwResult = 0;
	CIOCPServer*pServer = CIOCPServer::CreateServer(NULL);
	
	if (HIWORD(dwParam) == 1){
		dwResult = pServer->StartServer(LOWORD(dwParam));
		SendMessage(pServer->m_hNotifyWnd, WM_IOCPSVR_START, dwResult, 0);
	}
	else{
		pServer->StopServer();
		SendMessage(pServer->m_hNotifyWnd,WM_IOCPSVR_CLOSE, dwResult, 0);
	}	
}

void CIOCPServer::AsyncStopSrv()
{
	HANDLE hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)async_svr_ctrl_proc, 0, 0, 0);
	if (hThread){
		CloseHandle(hThread);
	}
}

void CIOCPServer::AsyncStartSrv(USHORT Port)
{
	HANDLE hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)async_svr_ctrl_proc,(void*)(0x00010000 | Port), 0, 0);
	if (hThread){
		CloseHandle(hThread);
	}
}