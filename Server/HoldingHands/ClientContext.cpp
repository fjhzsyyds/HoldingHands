#include "stdafx.h"
#include "ClientContext.h"
#include "Packet.h"
#include "MsgHandler.h"
#include "IOCPServer.h"
#include "Manager.h"

CClientContext::CClientContext(SOCKET ClientSocket, CIOCPServer *pServer, CManager*pManager):
m_pServer(pServer),
m_pManager(pManager),
m_Identity(0),
m_ClientSocket(ClientSocket)
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
	InitializeCriticalSectionAndSpinCount(&m_csCheck,3000);
	m_SendPacketOver = CreateEvent(0, FALSE, TRUE, NULL);
}


CClientContext::~CClientContext()
{
	if (m_ClientSocket != INVALID_SOCKET){
		closesocket(m_ClientSocket);
		m_ClientSocket = INVALID_SOCKET;
	}
	if (m_SendPacketOver){
		CloseHandle(m_SendPacketOver);
		m_SendPacketOver = NULL;
	}
	DeleteCriticalSection(&m_csCheck);
}

int CClientContext::SendPacket(DWORD command,const char*data, int len,DWORD dwFlag)
{
	for (;;){
		//OnClose之后，会触发事件，这里就进不去了。
		DWORD dwResult = WaitForSingleObject(m_SendPacketOver, 500);
		if (dwResult == WAIT_OBJECT_0){
			if (m_ClientSocket == INVALID_SOCKET){
				SetEvent(m_SendPacketOver);
				return -1;
			}
			break;
		}
		if (m_ClientSocket == INVALID_SOCKET)
			return -1;
	}

	if (FALSE == m_WritePacket.AllocateMem(len)){
		SetEvent(m_SendPacketOver);//发送失败
		return -1;
	}
	//Set Header
	m_WritePacket.SetHeader(len, command, dwFlag);
	//Copy Data.
	memcpy(m_WritePacket.GetBody(), data, len);
	m_dwWrite = 0;
	m_wsaWriteBuf.buf = m_WritePacket.GetBuffer() + m_dwWrite;
	m_wsaWriteBuf.len = m_WritePacket.GetBodyLen() + PACKET_HEADER_LEN - m_dwWrite;
	
	//服务器投递发送请求.
	m_pServer->PostWrite(this);	
	return 0;
}

//断开一个连接
void CClientContext::Disconnect(){
	EnterCriticalSection(&m_csCheck);
	if (m_ClientSocket != INVALID_SOCKET){
		closesocket(m_ClientSocket);
		m_ClientSocket = INVALID_SOCKET;
	}
	LeaveCriticalSection(&m_csCheck);
}

//ReadIO请求完成
void CClientContext::OnReadComplete(DWORD nTransferredBytes, DWORD dwFailedReason){
	if (dwFailedReason){
		m_dwRead = 0;
		m_wsaReadBuf.buf = 0;
		m_wsaReadBuf.len = 0;
		//直接返回,不Post下一个Read
		return;
	}
	//成功,有数据到达：
	m_dwRead += nTransferredBytes;
	//读取完毕
	if (m_dwRead >= PACKET_HEADER_LEN
		&& (m_dwRead == PACKET_HEADER_LEN + m_ReadPacket.GetBodyLen()))
	{
		//同步操作
		m_pManager->ProcessCompletedPacket(PACKET_READ_COMPLETED, this, &m_ReadPacket);
		//继续读取下一个Packet
		m_dwRead = 0;
	}
	//先把PacketHeader接收完
	if (m_dwRead < PACKET_HEADER_LEN){
		m_wsaReadBuf.buf = m_ReadPacket.GetBuffer() + m_dwRead;
		m_wsaReadBuf.len = PACKET_HEADER_LEN - m_dwRead;
	}
	else
	{
		BOOL bOk = m_ReadPacket.Verify();
		BOOL bMemEnough = FALSE;
		//为空，先分配相应内存,在这之前先校验头部是否正确
		if (bOk){
			DWORD dwBodyLen = m_ReadPacket.GetBodyLen();
			bMemEnough = m_ReadPacket.AllocateMem(dwBodyLen);
		}

		if (bOk && (m_dwRead - PACKET_HEADER_LEN>0)){
			////通知接收了一部分
			//m_pManager->ProcessCompletedPacket(PACKET_READ_PARTIAL, this, &m_ReadPacket);
		}
		//如果bOk且，内存分配成功，最后会重新赋值，否则下一次Post会断开这个客户的连接
		m_wsaReadBuf.buf = NULL;
		m_wsaReadBuf.len = 0;
		//内存可能分配失败
		if (bOk && bMemEnough)
		{
			//设置偏移
			m_wsaReadBuf.buf = m_ReadPacket.GetBuffer() + m_dwRead;
			//计算剩余长度
			m_wsaReadBuf.len = m_ReadPacket.GetBodyLen() + PACKET_HEADER_LEN - m_dwRead;
		}
	}
	//投递下一个请求
	m_pServer->PostRead(this);
}

//Write IO请求完成
void CClientContext::OnWriteComplete(DWORD nTransferredBytes, DWORD dwFailedReason){

	//发送成功
	if (!dwFailedReason)
	{
		m_dwWrite += nTransferredBytes;
		//发送完成了.
		if (m_dwWrite == m_WritePacket.GetBodyLen() + PACKET_HEADER_LEN){
			//处理完事件后清理
			m_pManager->ProcessCompletedPacket(PACKET_WRITE_COMPLETED, this, &m_WritePacket);
		}
		else if (m_dwWrite< m_WritePacket.GetBodyLen() + PACKET_HEADER_LEN){
			////只发送了一部分
			//if (m_dwWrite > PACKET_HEADER_LEN){
			//	m_pManager->ProcessCompletedPacket(PACKET_WRITE_PARTIAL, this, &m_WritePacket);
			//}
			m_wsaWriteBuf.buf = m_WritePacket.GetBuffer() + m_dwWrite;
			m_wsaWriteBuf.len = m_WritePacket.GetBodyLen() + PACKET_HEADER_LEN - m_dwWrite;
			//继续发送数据
			m_pServer->PostWrite(this);
			return;
		}
	}

	m_wsaWriteBuf.buf = 0;
	m_wsaWriteBuf.len = 0;
	m_dwWrite = 0;
	SetEvent(m_SendPacketOver);
}


void CClientContext::OnConnect(){
	m_pManager->ProcessCompletedPacket(PACKET_CLIENT_CONNECT, this, NULL);
}

void CClientContext::OnClose(){
	//等待数据包发送完毕.
	WaitForSingleObject(m_SendPacketOver, INFINITE);
	m_pManager->ProcessCompletedPacket(PACKET_CLIENT_DISCONNECT, this, 0);
}

void CClientContext::GetPeerName(char* Addr, USHORT&Port){
	Port = m_PeerPort;
	strcpy(Addr, m_szPeerAddr);
	
}

void CClientContext::GetSockName(char* Addr, USHORT&Port){
	Port = m_SockPort;
	strcpy(Addr, m_szSockAddr);
}