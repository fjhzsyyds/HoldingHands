#pragma once
#include <afx.h>
#include <WinSock2.h>
#include <MSWSock.h>

class CClientContext;
class CEventHandler
{
private:
	friend class CManager;
	friend class CClientContext;
	friend class CIOCPServer;
	//Handler��Ӧ����������.Ӧ�������Լ������Լ�
	CClientContext*	m_pClient;				//Client
	DWORD			m_Identity;				//������ʶ��ͬ�Ĺ���
public:
	/*
	1.Event �¼�
	2.�ܳ���
	3.�Ѷ�ȡ����
	4.������
	*/
	//���й���ģ����������⼸�������Ϳ����ˡ���Ҫ��CIOCPClient
	BOOL	Send(WORD Event, char*data, int len);
	
	//����ģ��Ӧ���ڸú�������Ͱ������Դ�����.�����������ܲ��ᱻ����

	virtual void OnClose();					//��socket�Ͽ���ʱ������������
	virtual void OnConnect();				//��socket���ӵ�ʱ������������
	//�����ݵ����ʱ���������������.
	virtual void OnReadPartial(WORD Event, DWORD Total, DWORD nRead, char*Buffer);
	virtual void OnReadComplete(WORD Event, DWORD Total, DWORD nRead, char*Buffer);
	//�����ݷ�����Ϻ��������������
	virtual void OnWritePartial(WORD Event, DWORD Total, DWORD nWrite, char*Buffer);
	virtual void OnWriteComplete(WORD Event, DWORD Total, DWORD nWrite, char*Buffer);

	//�Ͽ�����
public:
	void Disconnect();
	//
	SOCKET GetSocket();
	void GetPeerName(char* Addr, USHORT&Port);
	void GetSockName(char* Addr, USHORT&Port);

	CEventHandler(DWORD Identity);
	virtual ~CEventHandler();
};