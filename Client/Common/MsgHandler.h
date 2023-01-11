#pragma once
#include <WinSock2.h>
#include <MSWSock.h>
/*
	��д������������;
	virtual void OnClose() = 0;
	virtual void OnReadPartial(CPacket*pPacket) = 0;
	virtual void OnReadComplete(CPacket*pPacket) = 0;
*/
#include <string>

using std::pair;
using std::string;

class CIOCPClient;
class CMsgHandler
{
protected:
	friend class CIOCPClient;
	friend class CManager;
	//Handler��Ӧ����������.Ӧ�������Լ������Լ�;
	DWORD		m_Identity;				//������ʶ��ͬ�Ĺ���;
	CManager	*m_pManager;
public:
	//����ģ��Ӧ���ڸú�������Ͱ������Դ�����.�����������ܲ��ᱻ����
	virtual void OnClose() = 0;					//��socket�Ͽ���ʱ������������
	virtual void OnOpen() = 0;				//��socket���ӵ�ʱ������������

	//�����ݵ����ʱ���������������.
	virtual void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer) = 0;
	//�����ݷ�����Ϻ��������������
	virtual void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer) = 0;

public:
	BOOL	SendMsg(WORD Msg, void* data, int len, BOOL Compress = TRUE);
	void	Close();
	//��Щ�ӿڸò��ø�????

	const pair<string, unsigned short> GetPeerName();
	const pair<string, unsigned short> GetSockName();

	CMsgHandler(CManager*pManager, DWORD dwIdentity);

	virtual ~CMsgHandler();
};

