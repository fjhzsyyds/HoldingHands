#pragma once
#include <afx.h>
#include <winsock2.h>
#include <MSWSock.h>
#include "Packet.h"


#define REMOTE_AND_LOCAL_ADDR_LEN		(2 * (sizeof(sockaddr)+16))

class CManager;
class CPacket;
class CMsgHandler;

class CClientContext
{
private:
	friend class	CIOCPServer;
	//
	CIOCPServer*	m_pServer;
	CManager *		m_pManager;
	CMsgHandler*	m_pHandler;

	//Handler
	DWORD			m_Identity;								//����
	//
	SOCKET			m_ClientSocket;	
	//������йص�buffer
	WSABUF			m_wsaReadBuf;							//ÿ��Ͷ������ʱ���ڴ���������Ϣ
	CPacket			m_ReadPacket;							//ÿ��Ͷ������ʱ���ڴ���������Ϣ	
	DWORD			m_dwRead;								//�Ѿ���ȡ������
	//�뷢���йص�buffer
	WSABUF			m_wsaWriteBuf;
	CPacket			m_WritePacket;
	DWORD			m_dwWrite;								//�Ѿ����͵ĳ���
	//
	POSITION		m_PosInList;							//������client list �� context ��λ��
	//sockaddr_in		m_sockaddr;
	CRITICAL_SECTION m_cs;								//��ֹ�ظ��ر�socket
	HANDLE			 m_SendPacketComplete;					//�Ƿ������
	//
	DWORD			 m_LastHeartBeat;

	//Ͷ��һ��ReadIO����
	void PostRead();
	//Ͷ��һ��WriteIO����
	void PostWrite();

	void OnReadComplete(DWORD nTransferredBytes, DWORD dwFailedReason);
	void OnWriteComplete(DWORD nTransferredBytes, DWORD dwFailedReason);
	void OnClose();
	void OnConnect();

public:
	//�Ͽ�����
	void Disconnect();

	SOCKET		GetSocket(){
		return m_ClientSocket;
	}

	DWORD		GetIdentity(){
		return m_Identity;
	}

	void SetCallbackHandler(CMsgHandler*pHandler){
		m_pHandler = pHandler;
	}

	CMsgHandler* GetCallbackHandler(){
		return m_pHandler;
	}

	//Send Data...
	BOOL	BeginWrite();
	void	WriteBytes(BYTE*lpData, UINT Size);
	void	EndWrite(BOOL bCompress = TRUE);
	
	//Send Data.....
	BOOL	Send(BYTE*lpData, UINT Size, BOOL bCompress = TRUE);

	CClientContext(SOCKET ClientSocket,CIOCPServer *pServer,CManager*pManager);
	~CClientContext();
};

