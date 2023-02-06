#pragma once
#include "MsgHandler.h"
#define CMD				('C'|('M')<<8|('D')<<16)

//0成功,-1失败
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
	void OnClose();					//当socket断开的时候调用这个函数
	void OnOpen();				//当socket连接的时候调用这个函数

	void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer);


	void OnCmdBegin(DWORD dwStatu);
	void OnCmdResult(char*szBuffer);
	CCmdSrv(CClientContext*pClient);
	~CCmdSrv();
};

