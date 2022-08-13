#pragma once
#include "EventHandler.h"
#define CMD				('C'|('M')<<8|('D')<<16)

//0�ɹ�,-1ʧ��
#define CMD_BEGIN		(0xcad0)

#define CMD_COMMAND		(0xcad1)
#define CMD_RESULT		(0xcad2)

class CCmdWnd;
class CCmdSrv :
	public CEventHandler
{
private:
	CCmdWnd *	m_pWnd;

public:
	void OnClose();					//��socket�Ͽ���ʱ������������
	void OnConnect();				//��socket���ӵ�ʱ������������
	//�����ݵ����ʱ���������������.
	void OnReadPartial(WORD Event, DWORD Total, DWORD nRead, char*Buffer);
	void OnReadComplete(WORD Event, DWORD Total, DWORD nRead, char*Buffer);
	//�����ݷ�����Ϻ��������������
	void OnWritePartial(WORD Event, DWORD Total, DWORD nWrite, char*Buffer);
	void OnWriteComplete(WORD Event, DWORD Total, DWORD nWrite, char*Buffer);
	//

	void OnCmdBegin(DWORD dwStatu);
	void OnCmdResult(char*szBuffer);
	CCmdSrv(DWORD dwIdentity);
	~CCmdSrv();
};

