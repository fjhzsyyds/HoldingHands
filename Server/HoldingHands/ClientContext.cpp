#include "stdafx.h"
#include "ClientContext.h"
#include "Packet.h"
#include "MsgHandler.h"
#include "IOCPServer.h"
#include "Manager.h"
#include "utils.h"

CClientContext::CClientContext(SOCKET ClientSocket, CIOCPServer *pServer, CManager*pManager):
m_pServer(pServer),
m_pManager(pManager),
m_Identity(0xffffffff),
m_ClientSocket(ClientSocket),
m_LastHeartBeat(0)
{
	//-------------------------------------------------------------------------------------------
	m_wsaReadBuf.len = 0;
	m_wsaReadBuf.buf = 0;
	m_dwRead = 0;
	//
	m_wsaWriteBuf.buf = 0;
	m_wsaWriteBuf.len = 0;
	m_dwWrite = 0;
	//-------------------------------------------------------------------------------------------
	m_PosInList = 0;

	InitializeCriticalSectionAndSpinCount(&m_cs, 3000);
	m_SendPacketComplete = CreateEvent(0, FALSE, TRUE, NULL);
}


CClientContext::~CClientContext()
{
	if (m_ClientSocket != INVALID_SOCKET){
		closesocket(m_ClientSocket);
		m_ClientSocket = INVALID_SOCKET;
	}
	if (m_SendPacketComplete){
		CloseHandle(m_SendPacketComplete);
		m_SendPacketComplete = NULL;
	}
	DeleteCriticalSection(&m_cs);
}

//主要是抢占锁.
BOOL CClientContext::BeginWrite(){
	DWORD dwResult = 0;

	if (m_ClientSocket == INVALID_SOCKET){
		return FALSE;
	}

	for (;;){
		dwResult = WaitForSingleObject(m_SendPacketComplete, 0);
		if (dwResult == WAIT_OBJECT_0){
			if (m_ClientSocket == INVALID_SOCKET){
				SetEvent(m_SendPacketComplete);
				return FALSE;
			}
			break;
		}
		else if (dwResult == WAIT_TIMEOUT){
			if (m_ClientSocket == INVALID_SOCKET){
				return FALSE;
			}
			Sleep(1);
		}
	}

	m_WritePacket.ClearData();				//清空旧数据.
	m_WritePacket.SetReserved(0);
	return TRUE;
}

void CClientContext::WriteBytes(BYTE*lpData, UINT Size){
	m_WritePacket.Write(lpData, Size);		//失败了也不管了，
}

void CClientContext::EndWrite(BOOL bCompress){
	//尽力去压缩,失败了也没事(失败了就发送原始数据.)
	if (bCompress)
		m_WritePacket.Compress();

	//
	m_dwWrite = 0;
	m_wsaWriteBuf.buf = (char*)m_WritePacket.GetBuffer();
	m_wsaWriteBuf.len = m_WritePacket.GetDataLength() + PACKET_HEADER_LEN;
	PostWrite();
}

BOOL CClientContext::Send(BYTE*lpData, UINT Size, BOOL bCompress){
	if (BeginWrite()){
		WriteBytes(lpData, Size);
		EndWrite(bCompress);
		return TRUE;
	}
	return FALSE;
}

//断开一个连接
void CClientContext::Disconnect(){
	EnterCriticalSection(&m_cs);
	if (m_ClientSocket != INVALID_SOCKET){
		closesocket(m_ClientSocket);
		m_ClientSocket = INVALID_SOCKET;
	}
	LeaveCriticalSection(&m_cs);
}

//ReadIO请求完成
void CClientContext::OnReadComplete(DWORD nTransferredBytes, DWORD dwFailedReason){
	
	//失败或者收到 EOF了,直接返回.
	if (dwFailedReason || !nTransferredBytes){
		m_dwRead = 0;
		m_wsaReadBuf.buf = 0;
		m_wsaReadBuf.len = 0;
		return;
	}
	
	//成功,有数据到达：
	m_dwRead += nTransferredBytes;
	//读取完毕
	if (m_dwRead >= PACKET_HEADER_LEN
		&& (m_dwRead == PACKET_HEADER_LEN + m_ReadPacket.GetDataLength()))
	{
		if (m_ReadPacket.GetReserved() == HEART_BEAT){
			m_LastHeartBeat = GetTickCount();
			if (BeginWrite()){
				m_WritePacket.ClearData();
				m_WritePacket.SetReserved(HEART_BEAT);
				EndWrite();
			}
		}
		else{
			if (m_ReadPacket.Decompress()){
				m_pHandler->OnRead(m_ReadPacket.GetData(), m_ReadPacket.GetDataLength());
			}
		}
		m_dwRead = 0;
	}
	//先把PacketHeader接收完
	if (m_dwRead < PACKET_HEADER_LEN){
		m_wsaReadBuf.buf = (char*)m_ReadPacket.GetBuffer() + m_dwRead;
		m_wsaReadBuf.len = PACKET_HEADER_LEN - m_dwRead;
	}
	else
	{
		BOOL bOk = m_ReadPacket.Verify();
		BOOL bMemEnough = FALSE;
		//为空，先分配相应内存,在这之前先校验头部是否正确
		if (bOk){
			DWORD dwBodyLen = m_ReadPacket.GetDataLength();
			bMemEnough = m_ReadPacket.Reserve(dwBodyLen);
		}

		if (bOk && (m_dwRead - PACKET_HEADER_LEN>0)){
			////通知接收了一部分
		}

		//如果bOk且，内存分配成功，最后会重新赋值，否则下一次Post会断开这个客户的连接
		m_wsaReadBuf.buf = NULL;
		m_wsaReadBuf.len = 0;
		//内存可能分配失败
		if (bOk && bMemEnough)
		{
			//设置偏移
			m_wsaReadBuf.buf = (char*)m_ReadPacket.GetBuffer() + m_dwRead;
			//计算剩余长度
			m_wsaReadBuf.len = m_ReadPacket.GetDataLength() + PACKET_HEADER_LEN - m_dwRead;
		}
	}
	
	//继续读
	PostRead();
}

//Write IO请求完成
void CClientContext::OnWriteComplete(DWORD nTransferredBytes, DWORD dwFailedReason){

	//发送成功
	/*
		IOCP是否把数据都发送完成才会收到这个消息?
	*/
	if (!dwFailedReason)
	{
		m_dwWrite += nTransferredBytes;
		//发送完成了.
		if (m_dwWrite == m_WritePacket.GetDataLength() + PACKET_HEADER_LEN){
			m_pHandler->OnWrite(m_WritePacket.GetData(), m_WritePacket.GetDataLength());
		}
		else if (m_dwWrite< m_WritePacket.GetDataLength() + PACKET_HEADER_LEN){
			/*
					...数据很大的时候也不会分开发送.....
			*/
			//////只发送了一部分
			//if (m_dwWrite > PACKET_HEADER_LEN){
			//	m_pManager->ProcessCompletedPacket(PACKET_WRITE_PARTIAL, this, &m_WritePacket);
			//}

			m_wsaWriteBuf.buf = (char*)m_WritePacket.GetBuffer() + m_dwWrite;
			m_wsaWriteBuf.len = m_WritePacket.GetDataLength() + PACKET_HEADER_LEN - m_dwWrite;
			//继续发送数据
			PostWrite();
			return;
		}
	}

	m_wsaWriteBuf.buf = 0;
	m_wsaWriteBuf.len = 0;
	m_dwWrite = 0;

	SetEvent(m_SendPacketComplete);
}


void CClientContext::OnConnect(){
	DWORD timeout = 3 * 1000;
	dbg_log("CClientContext::OnConnect()");

	if (!setsockopt(m_ClientSocket, SOL_SOCKET,
		SO_RCVTIMEO, (char*)&timeout, sizeof(timeout))){
		recv(m_ClientSocket, (char*)&m_Identity, sizeof(m_Identity), 0);
	}
	m_pManager->OnClientConnect(this);
}

void CClientContext::OnClose(){
	//等待数据包发送完毕.
	WaitForSingleObject(m_SendPacketComplete, INFINITE);
	dbg_log("CClientContext::OnClose()");
	m_pManager->OnClientDisconnect(this);
}

//投递一个ReadIO请求
void CClientContext::PostRead()
{
	DWORD nReadBytes = 0;
	DWORD flag = 0;
	int nIORet, nErrorCode;
	OVERLAPPEDPLUS*pOverlapped = new OVERLAPPEDPLUS(IO_READ);
	//assert(pClientContext->m_ClientSocket == INVALID_SOCKET);

	EnterCriticalSection(&m_cs);

	//防止WSARecv的时候m_ClientSocket值改变.
	nIORet = WSARecv(m_ClientSocket, &m_wsaReadBuf,
		1, &nReadBytes, &flag, (LPOVERLAPPED)pOverlapped, NULL);

	nErrorCode = WSAGetLastError();
	//IO错误,该SOCKET被关闭了,或者buff为NULL;
	if (nIORet == SOCKET_ERROR	&& nErrorCode != WSA_IO_PENDING){
		//将这个标记为手动投递的
		dbg_log("CIOCPServer::PostRead WSARecv Failed with : %d ", nErrorCode);
		pOverlapped->m_bManualPost = TRUE;
		PostQueuedCompletionStatus(m_pServer->m_hCompletionPort, 0, 
			(ULONG_PTR)this, (LPOVERLAPPED)pOverlapped);
	}

	LeaveCriticalSection(&m_cs);
}

//--------------------------------------------------------------------------------------------------------------
//投递一个WriteIO请求
void CClientContext::PostWrite()
{
	DWORD nWriteBytes = 0;
	DWORD flag = 0;
	int nIORet, nErrorCode;
	OVERLAPPEDPLUS*pOverlapped = new OVERLAPPEDPLUS(IO_WRITE);
	//assert(pClientContext->m_ClientSocket == INVALID_SOCKET);

	/*
		实验得知,如果WSASend立刻就成功的话,
		在返回之前就会去Post一个CompletionStatu.
	*/
	EnterCriticalSection(&m_cs);

	nIORet = WSASend(m_ClientSocket, &m_wsaWriteBuf, 1,
		&nWriteBytes, flag, (LPOVERLAPPED)pOverlapped, NULL);
	nErrorCode = WSAGetLastError();
	//IO错误,该SOCKET被关闭了,或者buff为NULL;

	if (nIORet == SOCKET_ERROR	&& nErrorCode != WSA_IO_PENDING){
		//将这个标记为手动投递的
		dbg_log("CIOCPServer::PostWrite WSASend Failed with : %d ", nErrorCode);
		pOverlapped->m_bManualPost = TRUE;
		PostQueuedCompletionStatus(m_pServer->m_hCompletionPort, 
			0, (ULONG_PTR)this, (LPOVERLAPPED)pOverlapped);
	}

	LeaveCriticalSection(&m_cs);
}