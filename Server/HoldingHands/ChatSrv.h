#pragma once
#include "MsgHandler.h"



#define CHAT		('C'|('H'<<8)|('A'<<16)|('T'<<24))


#define CHAT_INIT		(0xaa00)			//client---svr
#define CHAT_BEGIN		(0xaa01)			//svr---client
#define CHAT_MSG		(0xaa02)			//p2p



class CChatDlg;

class CChatSrv :
	public CMsgHandler
{
public:

	CChatDlg*	m_pDlg;
	TCHAR		m_szNickName[128];



	void OnClose();					//��socket�Ͽ���ʱ������������
	void OnOpen();				//��socket���ӵ�ʱ������������

	//�����ݵ����ʱ���������������.
	void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer);



	void OnChatInit(DWORD dwRead, char*szBuffer);
	void OnChatMsg(DWORD dwRead, char*szbuffer);


	void SendMsgText(TCHAR*szMsg);
	CChatSrv(CManager*pManager);
	~CChatSrv();
};

