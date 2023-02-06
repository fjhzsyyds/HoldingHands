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

//��Ҫ����ռ��.
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

	m_WritePacket.ClearData();				//��վ�����.
	m_WritePacket.SetReserved(0);
	return TRUE;
}

void CClientContext::WriteBytes(BYTE*lpData, UINT Size){
	m_WritePacket.Write(lpData, Size);		//ʧ����Ҳ�����ˣ�
}

void CClientContext::EndWrite(BOOL bCompress){
	//����ȥѹ��,ʧ����Ҳû��(ʧ���˾ͷ���ԭʼ����.)
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

//�Ͽ�һ������
void CClientContext::Disconnect(){
	EnterCriticalSection(&m_cs);
	if (m_ClientSocket != INVALID_SOCKET){
		closesocket(m_ClientSocket);
		m_ClientSocket = INVALID_SOCKET;
	}
	LeaveCriticalSection(&m_cs);
}

//ReadIO�������
void CClientContext::OnReadComplete(DWORD nTransferredBytes, DWORD dwFailedReason){
	
	//ʧ�ܻ����յ� EOF��,ֱ�ӷ���.
	if (dwFailedReason || !nTransferredBytes){
		m_dwRead = 0;
		m_wsaReadBuf.buf = 0;
		m_wsaReadBuf.len = 0;
		return;
	}
	
	//�ɹ�,�����ݵ��
	m_dwRead += nTransferredBytes;
	//��ȡ���
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
	//�Ȱ�PacketHeader������
	if (m_dwRead < PACKET_HEADER_LEN){
		m_wsaReadBuf.buf = (char*)m_ReadPacket.GetBuffer() + m_dwRead;
		m_wsaReadBuf.len = PACKET_HEADER_LEN - m_dwRead;
	}
	else
	{
		BOOL bOk = m_ReadPacket.Verify();
		BOOL bMemEnough = FALSE;
		//Ϊ�գ��ȷ�����Ӧ�ڴ�,����֮ǰ��У��ͷ���Ƿ���ȷ
		if (bOk){
			DWORD dwBodyLen = m_ReadPacket.GetDataLength();
			bMemEnough = m_ReadPacket.Reserve(dwBodyLen);
		}

		if (bOk && (m_dwRead - PACKET_HEADER_LEN>0)){
			////֪ͨ������һ����
		}

		//���bOk�ң��ڴ����ɹ����������¸�ֵ��������һ��Post��Ͽ�����ͻ�������
		m_wsaReadBuf.buf = NULL;
		m_wsaReadBuf.len = 0;
		//�ڴ���ܷ���ʧ��
		if (bOk && bMemEnough)
		{
			//����ƫ��
			m_wsaReadBuf.buf = (char*)m_ReadPacket.GetBuffer() + m_dwRead;
			//����ʣ�೤��
			m_wsaReadBuf.len = m_ReadPacket.GetDataLength() + PACKET_HEADER_LEN - m_dwRead;
		}
	}
	
	//������
	PostRead();
}

//Write IO�������
void CClientContext::OnWriteComplete(DWORD nTransferredBytes, DWORD dwFailedReason){

	//���ͳɹ�
	/*
		IOCP�Ƿ�����ݶ�������ɲŻ��յ������Ϣ?
	*/
	if (!dwFailedReason)
	{
		m_dwWrite += nTransferredBytes;
		//���������.
		if (m_dwWrite == m_WritePacket.GetDataLength() + PACKET_HEADER_LEN){
			m_pHandler->OnWrite(m_WritePacket.GetData(), m_WritePacket.GetDataLength());
		}
		else if (m_dwWrite< m_WritePacket.GetDataLength() + PACKET_HEADER_LEN){
			/*
					...���ݺܴ��ʱ��Ҳ����ֿ�����.....
			*/
			//////ֻ������һ����
			//if (m_dwWrite > PACKET_HEADER_LEN){
			//	m_pManager->ProcessCompletedPacket(PACKET_WRITE_PARTIAL, this, &m_WritePacket);
			//}

			m_wsaWriteBuf.buf = (char*)m_WritePacket.GetBuffer() + m_dwWrite;
			m_wsaWriteBuf.len = m_WritePacket.GetDataLength() + PACKET_HEADER_LEN - m_dwWrite;
			//������������
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
	//�ȴ����ݰ��������.
	WaitForSingleObject(m_SendPacketComplete, INFINITE);
	dbg_log("CClientContext::OnClose()");
	m_pManager->OnClientDisconnect(this);
}

//Ͷ��һ��ReadIO����
void CClientContext::PostRead()
{
	DWORD nReadBytes = 0;
	DWORD flag = 0;
	int nIORet, nErrorCode;
	OVERLAPPEDPLUS*pOverlapped = new OVERLAPPEDPLUS(IO_READ);
	//assert(pClientContext->m_ClientSocket == INVALID_SOCKET);

	EnterCriticalSection(&m_cs);

	//��ֹWSARecv��ʱ��m_ClientSocketֵ�ı�.
	nIORet = WSARecv(m_ClientSocket, &m_wsaReadBuf,
		1, &nReadBytes, &flag, (LPOVERLAPPED)pOverlapped, NULL);

	nErrorCode = WSAGetLastError();
	//IO����,��SOCKET���ر���,����buffΪNULL;
	if (nIORet == SOCKET_ERROR	&& nErrorCode != WSA_IO_PENDING){
		//��������Ϊ�ֶ�Ͷ�ݵ�
		dbg_log("CIOCPServer::PostRead WSARecv Failed with : %d ", nErrorCode);
		pOverlapped->m_bManualPost = TRUE;
		PostQueuedCompletionStatus(m_pServer->m_hCompletionPort, 0, 
			(ULONG_PTR)this, (LPOVERLAPPED)pOverlapped);
	}

	LeaveCriticalSection(&m_cs);
}

//--------------------------------------------------------------------------------------------------------------
//Ͷ��һ��WriteIO����
void CClientContext::PostWrite()
{
	DWORD nWriteBytes = 0;
	DWORD flag = 0;
	int nIORet, nErrorCode;
	OVERLAPPEDPLUS*pOverlapped = new OVERLAPPEDPLUS(IO_WRITE);
	//assert(pClientContext->m_ClientSocket == INVALID_SOCKET);

	/*
		ʵ���֪,���WSASend���̾ͳɹ��Ļ�,
		�ڷ���֮ǰ�ͻ�ȥPostһ��CompletionStatu.
	*/
	EnterCriticalSection(&m_cs);

	nIORet = WSASend(m_ClientSocket, &m_wsaWriteBuf, 1,
		&nWriteBytes, flag, (LPOVERLAPPED)pOverlapped, NULL);
	nErrorCode = WSAGetLastError();
	//IO����,��SOCKET���ر���,����buffΪNULL;

	if (nIORet == SOCKET_ERROR	&& nErrorCode != WSA_IO_PENDING){
		//��������Ϊ�ֶ�Ͷ�ݵ�
		dbg_log("CIOCPServer::PostWrite WSASend Failed with : %d ", nErrorCode);
		pOverlapped->m_bManualPost = TRUE;
		PostQueuedCompletionStatus(m_pServer->m_hCompletionPort, 
			0, (ULONG_PTR)this, (LPOVERLAPPED)pOverlapped);
	}

	LeaveCriticalSection(&m_cs);
}