#pragma once
#include <afx.h>
#include <winsock2.h>
#include <MSWSock.h>
#include "Packet.h"
class CPacket;

#define REMOTE_AND_LOCAL_ADDR_LEN		(2 * (sizeof(sockaddr)+16))

class CEventHandler;
class CClientContext
{
private:
	friend class CManager;
	friend class CIOCPServer;
	friend	class  CEventHandler;
	//
	CIOCPServer*	m_pServer;
	//
	char		m_szPeerAddr[64];
	char		m_szSockAddr[64];
	USHORT		m_PeerPort;
	USHORT		m_SockPort;
	//
	//Handler
	DWORD			m_Identity;								//����
	CEventHandler*	m_pHandler;								//������
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
	BOOL BindHandler(CEventHandler*pHandler);
	BOOL UnbindHandler();
	//����һ����,ʧ�ܷ���-1,�ɹ�����0
	int SendPacket(DWORD command, char*data, int len);
	//�Ͽ�����
	void Disconnect();

	void GetPeerName(char* Addr, USHORT&Port);
	void GetSockName(char* Addr, USHORT&Port);

	CClientContext(SOCKET ClientSocket,CIOCPServer *pServer);
	~CClientContext();
};

