#include "stdafx.h"
#include "IOCPServer.h"
#include "Packet.h"
#include "Manager.h"
#include "ClientContext.h"
#include "MsgHandler.h"

#include "utils.h"
//初始化为NULL
CIOCPServer*	CIOCPServer::hInstance = NULL;
LPFN_ACCEPTEX	CIOCPServer::lpfnAcceptEx = NULL;

//创建一个服务器实例
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
	m_hStopRunning = CreateEvent(0, TRUE, TRUE, NULL);			//初始状态是TRUE,已经停止
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

//加到list里面
void CIOCPServer::AddToList(CClientContext*pContext)
{
	//Add pClientContext to ClientContextList and 
	//save the position(to remove this context from the list quickly)
	EnterCriticalSection(&m_CtxListLock);
	pContext->m_PosInList = m_ClientContextList.AddTail(pContext);
	LeaveCriticalSection(&m_CtxListLock);
}

//移除
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
			InterlockedExchange(&m_ReadSpeed, dwReadBytes);			//如果超过一秒了,就更新Read Speed 为刚才的1 s内读取的字节数.
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
			dwWriteBytes = 0;			//刷新
			LastUpdateWriteTime = dwCurTime;
		}
	}
}


#define _FLAG_GQCS_FAILED			0x1
#define _FLAG_MANUAL_POST			0x2
#define _FLAG_IO_ABORTED			0x4							//请求终止
//工作线程函数
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
	//在子线程里面调用GetCurrentThread函数返回的是一个伪句柄
	//这里调用GetCurrentThreadId;
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
		//超时
		if (!bResult && GetLastError() == WAIT_TIMEOUT){
			if (!InterlockedExchange(&pServer->m_SpeedLock, 1)){
				pServer->UpdateSpeed(IO_READ | IO_WRITE, 0);
				InterlockedExchange(&pServer->m_SpeedLock, 0);
			}
			continue;
		}
		//退出线程,投递了退出消息
		if (INVALID_SOCKET == (SOCKET)pClientContext)
			break;
		
	
		//1.bRet == false,nTransferredBytes==0,pOverlappedplus==0,pClientContext==0,				没有取出
		//2.bRet == false,pOverlappedplus≠0,pClientContext≠0，										取出了一个失败的IO完成包
		//3.bRet == false,nTransferredBytes==0,pOverlappedplus≠0,pClientContext≠0					本地 socket 句柄被关闭
		
		//实验得知,AcceptEx的完成键 是ListenSocket;
		//取出了一个IO完成包，但是这个包可能是失败的结果，可能是成功的结果
		if (pClientContext)
		{
			if (!bResult)
				dwResult |= _FLAG_GQCS_FAILED;
			if (pOverlappedplus->m_bManualPost)
				dwResult |= _FLAG_MANUAL_POST;
			if (WSAGetLastError() == 995)
				dwResult |= _FLAG_IO_ABORTED;

			/*
				处理IO_READ,IO_WRITE,IO_ACCEPT;
				此处需要。简单的线程调度,若nBusyCount == ThreadCount,那么就创建一个新的线程
				(为了防止所有线程都挂起而导致writeIOMsg无法被处理,emmm可能这个情况发生的几率很小)
				只要线程处于挂起(SendMessage,SuspenThread,WaitForSingleObject等时),新创建的线程才会被IOCP完成
				端口使用.否则新的线程没什么作用,始终处于挂起状态.原因是为了保证最大并发数量小于 初始设定值。
			*/
			//update speed
			if (!InterlockedExchange(&pServer->m_SpeedLock, 1)){
				if (pOverlappedplus != NULL && (pOverlappedplus->m_IOtype == IO_READ
					|| pOverlappedplus->m_IOtype == IO_WRITE)){
					pServer->UpdateSpeed(pOverlappedplus->m_IOtype, nTransferredBytes);
				}
				InterlockedExchange(&pServer->m_SpeedLock, 0);
			}
			
			//检测线程.
			if (InterlockedExchangeAdd(&pServer->m_BusyCount, 1) + 1 >= pServer->m_nThreadCount){

				//没有空余的线程.为了防止所有线程都是挂起(除GQCP)的状态而造成思死锁,
				//要创建新的线程
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
				//如果失败的话,emmm.......
			}

			//dispatch io message 
			pServer->HandlerIOMsg(pClientContext, nTransferredBytes, pOverlappedplus->m_IOtype, dwResult);
			
			//检测socket是否断开连接.
			if (pOverlappedplus->m_IOtype == IO_READ && ((!nTransferredBytes) || dwResult)){
				//断开连接了.
				pClientContext->Disconnect();
				pServer->RemoveFromList(pClientContext);
				//处理IO_CLOSE
				dbg_log("Disconnect,Reason:%x", dwResult);
				pServer->HandlerIOMsg(pClientContext, 0, IO_CLOSE, 0);
				
				//释放ctx
				InterlockedDecrement(&pServer->m_AssociateClientCount);
				pServer->FreeContext(pClientContext);
			}

			//处理完IO事件
			InterlockedDecrement(&pServer->m_BusyCount);

			if (InterlockedExchangeAdd(&pServer->m_nThreadCount, -1) > pServer->m_MaxThreadCount){	
				EnterCriticalSection(&pServer->m_csThread);
				pServer->m_ThreadList.Lookup((void*)CurrentThreadId, hCurrentThread);
				pServer->m_ThreadList.RemoveKey((void*)CurrentThreadId);
				LeaveCriticalSection(&pServer->m_csThread);

				bInPool = FALSE;//让这个线程结束吧
			}
			else{
				InterlockedIncrement(&pServer->m_nThreadCount);
			}
		}
		if (pOverlappedplus) 
			delete pOverlappedplus;
	}
	if (!bInPool){
		CloseHandle(hCurrentThread);	//当前线程数大于最大线程数的情况.自己退出,需要自己关闭句柄.
	}
	//
	_endthreadex(0);					//_beginthreadex之后的清理工作
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
//有连接了
void CIOCPServer::OnAcceptComplete(DWORD dwFailedReason)
{
	//说明上一个投递成功，而且m_AcceptSocket是一个有效的socket
	//allocate a new context to save the accept socket
	CClientContext*pClientContext = AllocateContext(m_AcceptSocket);
	//reset the m_AcceptSocket
	m_AcceptSocket = INVALID_SOCKET;
	HANDLE hRet = NULL;
	//绑定端口

	if (!dwFailedReason){
		hRet = CreateIoCompletionPort((HANDLE)pClientContext->m_ClientSocket,
			m_hCompletionPort, (ULONG_PTR)pClientContext, NULL);
	}

	if (!dwFailedReason && hRet == m_hCompletionPort)
	{
		//进到这里说明这个接收成功了而且成功的绑定了完成端口
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

			in_keep_alive.onoff = 1;						/*打开keepalive*/
			in_keep_alive.keepaliveinterval = 5000;			/*发送keepalive心跳时间间隔-单位为毫秒*/				
			in_keep_alive.keepalivetime = 40000;			/* 40s 没有报文开始发送keepalive心跳包-单位为毫秒*/

			WSAIoctl(pClientContext->m_ClientSocket, SIO_KEEPALIVE_VALS, (LPVOID)&in_keep_alive, ul_in_len,
				(LPVOID)&out_keep_alive, ul_out_len, &ul_bytes_return, NULL, NULL);
		}
		//Accept next client
		PostAccept();

		//-----------------------------------------------------------------------------------------
		pClientContext->OnConnect();
		//-----------------------------------------------------------------------------------------
	
		//先接收数据包的Header
		pClientContext->m_dwRead = 0;
		pClientContext->m_wsaReadBuf.buf = (char*)pClientContext->m_ReadPacket.GetBuffer();
		pClientContext->m_wsaReadBuf.len = PACKET_HEADER_LEN;
		
		//Post first read,then the socket will continuously process the read completion statu.;
		pClientContext->PostRead();	
		return;
	}

	//绑定失败,关闭接收的socket
	if (pClientContext->m_ClientSocket != INVALID_SOCKET){
		closesocket(pClientContext->m_ClientSocket);
		pClientContext->m_ClientSocket = INVALID_SOCKET;
	}

	FreeContext(pClientContext);
	//如果接受到一个socket ,但是对方没有发送数据而是直接断开，这里也会失败.
	//必须要接收到4 个字节的 identity才会成功.
	if ((dwFailedReason & _FLAG_IO_ABORTED) || (dwFailedReason & _FLAG_MANUAL_POST)){
		dbg_log("Error : Accept Aboarted!");
		//终止accept
		SetEvent(m_hStopRunning);
		//通知接受终止
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

	//不要接收identity.....容易被DDOS.

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
	//初始化WinSock
	if (SOCKET_ERROR == WSAStartup(RequestedVersion, &wsadata)){
		dbg_log("WSAStartup failed with error: %u", WSAGetLastError());
		return FALSE;
	}
	iResult = INVALID_SOCKET;
	//获取AcceptEx函数指针
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
	//最大并发数本机12个cpu ,24
	int MaxConcurrent = si.dwNumberOfProcessors * 2;

	//Create completion port
	m_hCompletionPort = CreateIoCompletionPort((HANDLE)m_ListenSocket, NULL, 
		(ULONG_PTR)m_ListenSocket, MaxConcurrent);
	if (NULL == m_hCompletionPort)
		goto Error;

	m_AssociateClientCount = 0;
	m_BusyCount = 0;
	m_MaxConcurrent = MaxConcurrent;
	m_MaxThreadCount = m_MaxConcurrent + 4;//最大线程数量设置为最大并发数量加4;
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
	//已经被关闭
	if (m_ListenSocket == INVALID_SOCKET)
		return;
	//close ListenSocket，AcceptEx will return false or GetCompletionStatus will return 0
	closesocket(m_ListenSocket);
	m_ListenSocket = INVALID_SOCKET;

	//wait AcceptCompletion is handled,等待m_bAccept变成FALSE
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
			//1.0版本的写法,现在没必要了。
			if (TryEnterCriticalSection(&pContext->m_cs)){
				pContext->Disconnect();								//仅仅关闭socket,其余的工作由线程函数去处理
				LeaveCriticalSection(&pContext->m_cs);
				break;												//跳出循环
			}
			else if (pContext->m_ClientSocket == INVALID_SOCKET){	//没有进入,但是可以确定该Context已经被关闭了。{
				break;
			}
			Sleep(10);
		}
	}
	//在这行代码结束之前，可能会有很多工作线程卡死在RemoveFromList里面
	LeaveCriticalSection(&m_CtxListLock);

	//等待所有客户端的IO处理完毕
	dbg_log("Wait all ctx exit...");
	while (m_AssociateClientCount>0) Sleep(10);
	
	//等待所有线程处于GCQS
	dbg_log("Wait all busy thread...");
	while (m_BusyCount > 0)	Sleep(10);
	//发送退出消息

	dbg_log("PostQueuedCompletionStatus To All Threads.");
	for (int i = 0; i < m_ThreadList.GetCount(); i++)		{
		PostQueuedCompletionStatus(m_hCompletionPort, 0, INVALID_SOCKET, NULL);
	}
	//等待所有线程退出。
	//等待线程退出.
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

	//关闭完成端口
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
		ASSERT(SetEvent(pClientContext->m_SendPacketComplete));				//重新将事件置为触发状态。

	}
	LeaveCriticalSection(&m_CtxListLock);

	if (pClientContext == NULL){
		pClientContext = new CClientContext(ClientSocket,this,m_pManager);
	}
	return pClientContext;
}

void CIOCPServer::FreeContext(CClientContext*pContext){
	//善后清理
	SetEvent(pContext->m_SendPacketComplete);			//重新将事件置为触发状态。
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
	pContext->m_LastHeartBeat = 0;			//必须重置,否则会断开

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