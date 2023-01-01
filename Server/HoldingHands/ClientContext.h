#pragma once
#include <afx.h>
#include <winsock2.h>
#include <MSWSock.h>
#include "Packet.h"
class CPacket;

#define REMOTE_AND_LOCAL_ADDR_LEN		(2 * (sizeof(sockaddr)+16))

class CMsgHandler;
class CClientContext
{
private:
	friend class CManager;
	friend class CIOCPServer;
	friend	class  CMsgHandler;
	//
	CIOCPServer*	m_pServer;

	CManager *		m_pManager;
	//
	char		m_szPeerAddr[64];
	char		m_szSockAddr[64];
	USHORT		m_PeerPort;
	USHORT		m_SockPort;
	//
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
	CRITICAL_SECTION m_csCheck;								//��ֹ�ظ��ر�socket
	HANDLE			 m_SendPacketOver;						//�Ƿ������
	//
public:
	//����һ����,ʧ�ܷ���-1,�ɹ�����0
	int SendPacket(DWORD command,const char*data, int len,DWORD dwFlag);
	//�Ͽ�����
	void Disconnect();


	//ReadIO�������
	void OnReadComplete(DWORD nTransferredBytes, DWORD dwFailedReason);
	//Write IO�������
	void OnWriteComplete(DWORD nTransferredBytes, DWORD dwFailedReason);
	//�Ͽ�����
	void OnClose();
	void OnConnect();

	//
	void GetPeerName(char* Addr, USHORT&Port);
	void GetSockName(char* Addr, USHORT&Port);

	CClientContext(SOCKET ClientSocket,CIOCPServer *pServer,CManager*pManager);
	~CClientContext();
};

