#pragma once
#include "MsgHandler.h"
#include "SocksProxyWnd.h"

#define SOCKS_PROXY			(('S') | (('K') << 8) | (('P') << 16) | (('X') << 24))
#define MAX_CLIENT_COUNT	1024

#define SOCK_PROXY_ADD_CLIENT  (0xaa01)

#define SOCK_PROXY_REQUEST					(0xaa02)


#define SOCK_PROXY_REQUEST_RESULT		(0xaa04)
#define SOCK_PROXY_DATA					(0xaa03)
#define SOCK_PROXY_CLOSE				(0xaa05)

class CSocksProxyWnd;
class CIOCPProxyServer;

class CSocksProxySrv :
	public CMsgHandler
{
private:
	UINT			    m_ClientID;
	CSocksProxyWnd*		m_pWnd;

	CIOCPProxyServer	*m_pServer;

	BYTE	m_Clients[MAX_CLIENT_COUNT];
	void OnRemoteResponse(DWORD * Response);
	void OnRemoteData(char * Buffer, DWORD Size);
	void OnRemoteClose(DWORD ID);

public:
	UINT AllocClientID();

	void FreeClientID(UINT ClientID){
		m_Clients[ClientID] = 0;
	}

	DWORD GetConnections();

	BOOL StartProxyServer(UINT Port, DWORD Address,DWORD UDPAssociateAddr);
	void SetSocksVersion(BYTE Version);
	void StopProxyServer();
	void Disconnect(DWORD ClientID);
	void UpdateInfo(void * param1, void * param2);

	void OnClose();					//��socket�Ͽ���ʱ������������
	void OnOpen() ;				//��socket���ӵ�ʱ������������

	//�����ݵ����ʱ���������������.
	 void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	//�����ݷ�����Ϻ��������������
	 void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer){};

	CSocksProxySrv(CClientContext*pClient);
	~CSocksProxySrv();
};

