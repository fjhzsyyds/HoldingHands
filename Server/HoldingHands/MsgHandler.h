#pragma once
#include <afx.h>
#include <WinSock2.h>
#include <MSWSock.h>

#include <string>
using std::string;
using std::pair;

class CManager;
class CClientContext;
class CMsgHandler
{
protected:
	friend class CManager;
	friend class CClientContext;
	friend class CIOCPServer;

	CManager		*m_pManager;
	//����ģ��Ӧ���ڸú�������Ͱ������Դ�����.�����������ܲ��ᱻ����
	virtual void OnClose() = 0;					//��socket�Ͽ���ʱ������������
	virtual void OnOpen() = 0;				//��socket���ӵ�ʱ������������

	//�����ݵ����ʱ���������������.
	virtual void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer) = 0;
	//�����ݷ�����Ϻ��������������
	virtual void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer) = 0;

public:
	BOOL	SendMsg(WORD Msg, void*data, int len);
	void	Close();
	//��Щ�ӿڸò��ø�????

	const pair<string,unsigned short> GetPeerName();
	const pair<string, unsigned short> GetSockName();

	CMsgHandler(CManager*pManager);
	virtual ~CMsgHandler();
};