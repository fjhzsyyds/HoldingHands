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
CIOCPServer* CIOCPServer::CreateServer(CManager* pManager)
{
	if (hInstance == NULL){
		hInstance = new CIOCPServer(pManager);
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

CIOCPServer::CIOCPServer(CManager* pManager)
{
	//Listen Socket
	m_ListenSocket = INVALID_SOCKET;
	//Accept Socket
	m_AcceptSocket = INVALID_SOCKET;

	memset(m_AcceptBuf, 0, sizeof(m_AcceptBuf));
	//Context:
	InitializeCriticalSection(&m_CtxListLock);
	//
	m_hCompletionPort = INVALID_HANDLE_VALUE;
	m_AssociateClientCount = 0;
	//
	m_hStopRunning = CreateEvent(0, TRUE, TRUE, NULL);			//��ʼ״̬��TRUE,�Ѿ�ֹͣ
	//Event Dispatch
	m_pManager = pManager;

	m_BusyCount = 0;
	m_MaxConcurrent = 0;
	m_MaxThreadCount = 0;

	m_ReadSpeed = 0;
	m_WriteSpeed = 0;

	m_SpeedLock = 0;

	m_hScanThread = NULL;
	m_nThreadCount = 0;

	InitializeCriticalSectionAndSpinCount(&m_csThread,5000);
}

CIOCPServer::~CIOCPServer()
{
	//if server is running,stop the server;
	StopServer();
	//release resource
	//Release context:
	
	while (m_ClientContextList.GetCount() > 0)
		delete (CClientContext*)m_ClientContextList.RemoveHead();

	while (m_FreeContextList.GetCount() > 0)
		delete (CClientContext*)m_FreeContextList.RemoveHead();
	

	DeleteCriticalSection(&m_CtxListLock);
	DeleteCriticalSection(&m_csThread);
	//
	m_pManager = NULL;

	//
	if (m_hStopRunning){
		CloseHandle(m_hStopRunning);
		m_hStopRunning = NULL;
	}
}

//�ӵ�list����
void CIOCPServer::AddToList(CClientContext*pContext)
{
	//Add pClientContext to ClientContextList and 
	//save the position(to remove this context from the list quickly)
	EnterCriticalSection(&m_CtxListLock);
	pContext->m_PosInList = m_ClientContextList.AddTail(pContext);
	LeaveCriticalSection(&m_CtxListLock);
}

//�Ƴ�
void CIOCPServer::RemoveFromList(CClientContext*pContext)
{
	EnterCriticalSection(&m_CtxListLock);
	m_ClientContextList.RemoveAt(pContext->m_PosInList);
	pContext->m_PosInList = NULL;
	LeaveCriticalSection(&m_CtxListLock);
}

void CIOCPServer::GetSpeed(DWORD *lpReadSpeed , DWORD*lpWriteSpeed )
{
	if (lpReadSpeed)
		*lpReadSpeed = m_ReadSpeed;
	if (lpWriteSpeed)
		*lpWriteSpeed = m_WriteSpeed;
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
			InterlockedExchange(&m_ReadSpeed, dwReadBytes);			//�������һ����,�͸���Read Speed Ϊ�ղŵ�1 s�ڶ�ȡ���ֽ���.
			dwReadBytes = 0;
			LastUpdateReadTime = dwCurTime;
		}
	}
	if (io_type&IO_WRITE)
	{
		dwWriteBytes += TranferredBytes;
		if (dwCurTime - LastUpdateWriteTime >= 1000)
		{
			InterlockedExchange(&m_WriteSpeed, dwWriteBytes);
			dwWriteBytes = 0;			//ˢ��
			LastUpdateWriteTime = dwCurTime;
		}
	}
}


#define _FLAG_GQCS_FAILED			0x1
#define _FLAG_MANUAL_POST			0x2
#define _FLAG_IO_ABORTED			0x4							//������ֹ
//�����̺߳���
unsigned int __stdcall CIOCPServer::WorkerThread(void*)
{
	//
	HANDLE			hCurrentThread;
	DWORD			CurrentThreadId;
	CIOCPServer*	pServer;
	BOOL			bResult;
	DWORD			nTransferredBytes;
	CClientContext *pClientContext;
	OVERLAPPEDPLUS *pOverlappedplus;

	BOOL			bInPool;
	DWORD			dwResult;
	//�����߳��������GetCurrentThread�������ص���һ��α���
	//�������GetCurrentThreadId;
	hCurrentThread = NULL;
	CurrentThreadId = GetCurrentThreadId();
	pServer = CIOCPServer::CreateServer(NULL);
	bInPool = TRUE;
	while (bInPool)
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
			if (!InterlockedExchange(&pServer->m_SpeedLock, 1)){
				if (pOverlappedplus != NULL && (pOverlappedplus->m_IOtype == IO_READ
					|| pOverlappedplus->m_IOtype == IO_WRITE)){
					pServer->UpdateSpeed(pOverlappedplus->m_IOtype, nTransferredBytes);
				}
				InterlockedExchange(&pServer->m_SpeedLock, 0);
			}
			
			//����߳�.
			if (InterlockedExchangeAdd(&pServer->m_BusyCount, 1) + 1 >= pServer->m_nThreadCount){

				//û�п�����߳�.Ϊ�˷�ֹ�����̶߳��ǹ���(��GQCP)��״̬�����˼����,
				//Ҫ�����µ��߳�
				unsigned int ThreadId = 0;
				HANDLE hThread = 0;
				hThread = (HANDLE)_beginthreadex(0, 0, WorkerThread, 0, CREATE_SUSPENDED, &ThreadId);
				
				if (hThread != NULL && ThreadId !=NULL)
				{
					InterlockedIncrement(&pServer->m_nThreadCount);

					EnterCriticalSection(&pServer->m_csThread);
					pServer->m_ThreadList.SetAt((void*)ThreadId, hThread);
					LeaveCriticalSection(&pServer->m_csThread);

					ResumeThread(hThread);
				}
				//���ʧ�ܵĻ�,emmm.......
			}

			//dispatch io message 
			pServer->HandlerIOMsg(pClientContext, nTransferredBytes, pOverlappedplus->m_IOtype, dwResult);
			
			//���socket�Ƿ�Ͽ�����.
			if (pOverlappedplus->m_IOtype == IO_READ && ((!nTransferredBytes) || dwResult)){
				//�Ͽ�������.
				pClientContext->Disconnect();
				pServer->RemoveFromList(pClientContext);
				//����IO_CLOSE
				dbg_log("Disconnect,Reason:%x", dwResult);
				pServer->HandlerIOMsg(pClientContext, 0, IO_CLOSE, 0);
				
				//�ͷ�ctx
				InterlockedDecrement(&pServer->m_AssociateClientCount);
				pServer->FreeContext(pClientContext);
			}

			//������IO�¼�
			InterlockedDecrement(&pServer->m_BusyCount);

			if (InterlockedExchangeAdd(&pServer->m_nThreadCount, -1) > pServer->m_MaxThreadCount){	
				EnterCriticalSection(&pServer->m_csThread);
				pServer->m_ThreadList.Lookup((void*)CurrentThreadId, hCurrentThread);
				pServer->m_ThreadList.RemoveKey((void*)CurrentThreadId);
				LeaveCriticalSection(&pServer->m_csThread);

				bInPool = FALSE;//������߳̽�����
			}
			else{
				InterlockedIncrement(&pServer->m_nThreadCount);
			}
		}
		if (pOverlappedplus) 
			delete pOverlappedplus;
	}
	if (!bInPool){
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
		this->OnAcceptComplete(dwFailedReason);
		break;
	case IO_WRITE:
		pClientContext->OnWriteComplete(nTransferredBytes, dwFailedReason);
		break;
	case IO_READ:
		pClientContext->OnReadComplete(nTransferredBytes, dwFailedReason);
		break;
	case IO_CLOSE:
		pClientContext->OnClose();
		break;
	}
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
		//Accept next client
		PostAccept();

		//-----------------------------------------------------------------------------------------
		pClientContext->OnConnect();
		//-----------------------------------------------------------------------------------------
	
		//�Ƚ������ݰ���Header
		pClientContext->m_dwRead = 0;
		pClientContext->m_wsaReadBuf.buf = (char*)pClientContext->m_ReadPacket.GetBuffer();
		pClientContext->m_wsaReadBuf.len = PACKET_HEADER_LEN;
		
		//Post first read,then the socket will continuously process the read completion statu.;
		pClientContext->PostRead();	
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
		dbg_log("Error : Accept Aboarted!");
		//��ֹaccept
		SetEvent(m_hStopRunning);
		//֪ͨ������ֹ
		m_pManager->OnAcceptAboart();
		return;
	}
	
	//Continue accept client...
	PostAccept();	
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

	//��Ҫ����identity.....���ױ�DDOS.

	bAcceptRet = lpfnAcceptEx(m_ListenSocket, m_AcceptSocket, m_AcceptBuf,0
		, REMOTE_AND_LOCAL_ADDR_LEN / 2, REMOTE_AND_LOCAL_ADDR_LEN / 2,
		NULL, (OVERLAPPED*)pOverlappedPlus);

	nErrorCode = WSAGetLastError();

	if (bAcceptRet == FALSE && nErrorCode != WSA_IO_PENDING){
		//Post accept error,m_bManualPost == TRUE means PostAcceptError,
		//then the valid socket will be closed by OnAcceptCompletion
		dbg_log("CIOCPServer::PostAccept lpfnAcceptEx Failed with %d", nErrorCode);
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
		if (hThread && ThreadId){
			m_ThreadList.SetAt((void*)ThreadId, hThread);
			++m_nThreadCount;
		}
	}
	//-------------------------------------------------------------------------------------------------------------
	//Beign accepting client socket;
	ResetEvent(m_hStopRunning);				//Means the server is ready for accepting client 
	
	//Timeout Scan.
	m_hScanThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ScanThread, this, 0, 0);
	//
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
	
	//Wait Scan Thread exit..
	dbg_log("Wait scan thread exit.");
	WaitForSingleObject(m_hScanThread, INFINITE);
	CloseHandle(m_hScanThread);
	m_hScanThread = NULL;
	
	//close all sockets;
	EnterCriticalSection(&m_CtxListLock);
	for (POSITION pos = m_ClientContextList.GetHeadPosition();
		pos != NULL; m_ClientContextList.GetNext(pos)){

		CClientContext*pContext = (CClientContext*)m_ClientContextList.GetAt(pos);
		while (true){
			//1.0�汾��д��,����û��Ҫ�ˡ�
			if (TryEnterCriticalSection(&pContext->m_cs)){
				pContext->Disconnect();								//�����ر�socket,����Ĺ������̺߳���ȥ����
				LeaveCriticalSection(&pContext->m_cs);
				break;												//����ѭ��
			}
			else if (pContext->m_ClientSocket == INVALID_SOCKET){	//û�н���,���ǿ���ȷ����Context�Ѿ����ر��ˡ�{
				break;
			}
			Sleep(10);
		}
	}
	//�����д������֮ǰ�����ܻ��кܶ๤���߳̿�����RemoveFromList����
	LeaveCriticalSection(&m_CtxListLock);

	//�ȴ����пͻ��˵�IO�������
	dbg_log("Wait all ctx exit...");
	while (m_AssociateClientCount>0) Sleep(10);
	
	//�ȴ������̴߳���GCQS
	dbg_log("Wait all busy thread...");
	while (m_BusyCount > 0)	Sleep(10);
	//�����˳���Ϣ

	dbg_log("PostQueuedCompletionStatus To All Threads.");
	for (int i = 0; i < m_ThreadList.GetCount(); i++)		{
		PostQueuedCompletionStatus(m_hCompletionPort, 0, INVALID_SOCKET, NULL);
	}
	//�ȴ������߳��˳���
	//�ȴ��߳��˳�.
	dbg_log("Wait All Threads Exit.");
	POSITION pos = m_ThreadList.GetStartPosition();
	while (pos != NULL){
		void* handle = 0,* threadid = 0;
		m_ThreadList.GetNextAssoc(pos, threadid, handle);
		WaitForSingleObject(handle, INFINITE);
		CloseHandle(handle);
	}

	m_ThreadList.RemoveAll();

	m_MaxThreadCount = 0;
	m_MaxConcurrent = 0;
	m_BusyCount = 0;
	m_nThreadCount = 0;

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

	EnterCriticalSection(&m_CtxListLock);
	if (m_FreeContextList.GetCount() > 0){
		pClientContext = (CClientContext*)m_FreeContextList.RemoveHead();
		pClientContext->m_ClientSocket = ClientSocket;
		pClientContext->m_pServer = this;
		
		dbg_log("CIOCPServer::AllocateContext : Use Free Context.");
		ASSERT(SetEvent(pClientContext->m_SendPacketComplete));				//���½��¼���Ϊ����״̬��

	}
	LeaveCriticalSection(&m_CtxListLock);

	if (pClientContext == NULL){
		pClientContext = new CClientContext(ClientSocket,this,m_pManager);
	}
	return pClientContext;
}

void CIOCPServer::FreeContext(CClientContext*pContext){
	//�ƺ�����
	SetEvent(pContext->m_SendPacketComplete);			//���½��¼���Ϊ����״̬��
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

	pContext->m_Identity = 0xffffffff;
	pContext->m_LastHeartBeat = 0;			//��������,�����Ͽ�

	EnterCriticalSection(&m_CtxListLock);
	m_FreeContextList.AddTail(pContext);
	LeaveCriticalSection(&m_CtxListLock);
}


#define IOCP_START_SRV	1
#define IOCP_STOP_SRV	0

void CIOCPServer::async_svr_ctrl_proc(LPVOID * ArgList)
{
	CIOCPServer * pThis = (CIOCPServer*)ArgList[0];
	int				Opt	= (int)ArgList[1];
	HWND	hNotifyWnd	= (HWND)ArgList[2];
	
	if (Opt == IOCP_START_SRV){
		DWORD Port = (DWORD)ArgList[3];
		DWORD Result = pThis->StartServer(Port);
		SendMessage(hNotifyWnd, WM_IOCP_SRV_START, Result, 0);
	}
	else if (Opt == IOCP_STOP_SRV){
		pThis->StopServer();
		SendMessage(hNotifyWnd, WM_IOCP_SRV_CLOSE, 0, 0);
	}
	delete[] ArgList;
}


void CIOCPServer::AsyncStopSrv(HWND hNotifyWnd)
{
	LPVOID * ArgList = new LPVOID[3];

	ArgList[0] = this;
	ArgList[1] = (LPVOID)IOCP_STOP_SRV;
	ArgList[2] = (LPVOID)hNotifyWnd;

	HANDLE hThread = CreateThread(NULL, NULL, 
		(LPTHREAD_START_ROUTINE)async_svr_ctrl_proc, ArgList, 0, 0);
	if (hThread){
		CloseHandle(hThread);
	}
}

void CIOCPServer::AsyncStartSrv(HWND hNotifyWnd,USHORT Port)
{
	LPVOID * ArgList = new LPVOID[4];

	ArgList[0] = this;
	ArgList[1] = (LPVOID)IOCP_START_SRV;
	ArgList[2] = (LPVOID)hNotifyWnd;
	ArgList[3] = (LPVOID)Port;

	HANDLE hThread = CreateThread(NULL, NULL, 
		(LPTHREAD_START_ROUTINE)async_svr_ctrl_proc, ArgList, 0, 0);

	if (hThread){
		CloseHandle(hThread);
	}
}

unsigned int __stdcall CIOCPServer::ScanThread(void* lpParam){
	CIOCPServer*pServer = (CIOCPServer*)lpParam;

	while (1){
		DWORD dwResult = WaitForSingleObject(pServer->m_hStopRunning, 60000);
		if (dwResult != WAIT_TIMEOUT)
			break;
		
		EnterCriticalSection(&pServer->m_CtxListLock);
		POSITION pos = pServer->m_ClientContextList.GetHeadPosition();
		while (pos){
			CClientContext*pClientCtx =(CClientContext*)
				pServer->m_ClientContextList.GetNext(pos);

				
			if (pClientCtx->m_LastHeartBeat){
				DWORD dwElapse = GetTickCount() - pClientCtx->m_LastHeartBeat;
				if (dwElapse > 60 * 1000){
					dbg_log("Heartbeat time out ,Elapse: %u",dwElapse);
					pClientCtx->Disconnect();
				}
			}
		}
		LeaveCriticalSection(&pServer->m_CtxListLock);
	}
	return 0;
}