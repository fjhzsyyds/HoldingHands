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
		//OnClose֮�󣬻ᴥ���¼�������ͽ���ȥ�ˡ�
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
		SetEvent(m_SendPacketOver);//����ʧ��
		return -1;
	}
	//Set Header
	m_WritePacket.SetHeader(len, command, dwFlag);
	//Copy Data.
	memcpy(m_WritePacket.GetBody(), data, len);
	m_dwWrite = 0;
	m_wsaWriteBuf.buf = m_WritePacket.GetBuffer() + m_dwWrite;
	m_wsaWriteBuf.len = m_WritePacket.GetBodyLen() + PACKET_HEADER_LEN - m_dwWrite;
	
	//������Ͷ�ݷ�������.
	m_pServer->PostWrite(this);	
	return 0;
}

//�Ͽ�һ������
void CClientContext::Disconnect(){
	EnterCriticalSection(&m_csCheck);
	if (m_ClientSocket != INVALID_SOCKET){
		closesocket(m_ClientSocket);
		m_ClientSocket = INVALID_SOCKET;
	}
	LeaveCriticalSection(&m_csCheck);
}

//ReadIO�������
void CClientContext::OnReadComplete(DWORD nTransferredBytes, DWORD dwFailedReason){
	if (dwFailedReason){
		m_dwRead = 0;
		m_wsaReadBuf.buf = 0;
		m_wsaReadBuf.len = 0;
		//ֱ�ӷ���,��Post��һ��Read
		return;
	}
	//�ɹ�,�����ݵ��
	m_dwRead += nTransferredBytes;
	//��ȡ���
	if (m_dwRead >= PACKET_HEADER_LEN
		&& (m_dwRead == PACKET_HEADER_LEN + m_ReadPacket.GetBodyLen()))
	{
		//ͬ������
		m_pManager->ProcessCompletedPacket(PACKET_READ_COMPLETED, this, &m_ReadPacket);
		//������ȡ��һ��Packet
		m_dwRead = 0;
	}
	//�Ȱ�PacketHeader������
	if (m_dwRead < PACKET_HEADER_LEN){
		m_wsaReadBuf.buf = m_ReadPacket.GetBuffer() + m_dwRead;
		m_wsaReadBuf.len = PACKET_HEADER_LEN - m_dwRead;
	}
	else
	{
		BOOL bOk = m_ReadPacket.Verify();
		BOOL bMemEnough = FALSE;
		//Ϊ�գ��ȷ�����Ӧ�ڴ�,����֮ǰ��У��ͷ���Ƿ���ȷ
		if (bOk){
			DWORD dwBodyLen = m_ReadPacket.GetBodyLen();
			bMemEnough = m_ReadPacket.AllocateMem(dwBodyLen);
		}

		if (bOk && (m_dwRead - PACKET_HEADER_LEN>0)){
			////֪ͨ������һ����
			//m_pManager->ProcessCompletedPacket(PACKET_READ_PARTIAL, this, &m_ReadPacket);
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
	m_pServer->PostRead(this);
}

//Write IO�������
void CClientContext::OnWriteComplete(DWORD nTransferredBytes, DWORD dwFailedReason){

	//���ͳɹ�
	if (!dwFailedReason)
	{
		m_dwWrite += nTransferredBytes;
		//���������.
		if (m_dwWrite == m_WritePacket.GetBodyLen() + PACKET_HEADER_LEN){
			//�������¼�������
			m_pManager->ProcessCompletedPacket(PACKET_WRITE_COMPLETED, this, &m_WritePacket);
		}
		else if (m_dwWrite< m_WritePacket.GetBodyLen() + PACKET_HEADER_LEN){
			////ֻ������һ����
			//if (m_dwWrite > PACKET_HEADER_LEN){
			//	m_pManager->ProcessCompletedPacket(PACKET_WRITE_PARTIAL, this, &m_WritePacket);
			//}
			m_wsaWriteBuf.buf = m_WritePacket.GetBuffer() + m_dwWrite;
			m_wsaWriteBuf.len = m_WritePacket.GetBodyLen() + PACKET_HEADER_LEN - m_dwWrite;
			//������������
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
	//�ȴ����ݰ��������.
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