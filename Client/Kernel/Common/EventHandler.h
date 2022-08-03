#pragma once
#include <WinSock2.h>
#include <MSWSock.h>
/*
	��д������������;
	virtual void OnClose() = 0;
	virtual void OnReadPartial(CPacket*pPacket) = 0;
	virtual void OnReadComplete(CPacket*pPacket) = 0;
*/

class CIOCPClient;
class CEventHandler
{
protected:
	friend class CIOCPClient;
	friend class CManager;
	//Handler��Ӧ����������.Ӧ�������Լ������Լ�;
	CIOCPClient*m_pClient;				//Client;
	DWORD		m_Identity;				//������ʶ��ͬ�Ĺ���;
public:
	/*
	1.Event �¼�;
	2.�ܳ���;
	3.�Ѷ�ȡ����;
	4.������;
	*/
	//���й���ģ����������⼸�������Ϳ����ˡ���Ҫ��CIOCPClient;
	
	virtual void OnClose();						//����ģ��Ӧ���ڸú�������Ͱ������Դ�����.��Ϊ��ص������������ᱻ����;
	virtual void OnConnect();
	
	virtual void OnReadPartial(WORD Event, DWORD Total, DWORD dwRead, char*Buffer);
	virtual void OnReadComplete(WORD Event, DWORD Total, DWORD dwRead, char*Buffer);
	virtual void OnWritePartial(WORD Event, DWORD dwTotal, DWORD dwWrite, char*Buffer);
	virtual void OnWriteComplete(WORD Event, DWORD dwTotal, DWORD dwWrite, char*Buffer);

	void		Send(WORD Event,const char*data, int len);
	void		Disconnect();

	void GetPeerName(char* Addr, USHORT&Port);
	void GetSockName(char* Addr, USHORT&Port);
	void GetSrvName(char*Addr, USHORT&Port);

public:
	
	CEventHandler(DWORD Identity);
	~CEventHandler();
};

