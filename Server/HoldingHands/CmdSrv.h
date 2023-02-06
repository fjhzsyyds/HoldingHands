#pragma once
#include "MsgHandler.h"
#define CMD				('C'|('M')<<8|('D')<<16)

//0�ɹ�,-1ʧ��
#define CMD_BEGIN		(0xcad0)

#define CMD_COMMAND		(0xcad1)
#define CMD_RESULT		(0xcad2)

class CCmdWnd;
class CCmdSrv :
	public CMsgHandler
{
private:
	CCmdWnd *	m_pWnd;

public:
	void OnClose();					//��socket�Ͽ���ʱ������������
	void OnOpen();				//��socket���ӵ�ʱ������������

	void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer);


	void OnCmdBegin(DWORD dwStatu);
	void OnCmdResult(char*szBuffer);
	CCmdSrv(CClientContext*pClient);
	~CCmdSrv();
};

