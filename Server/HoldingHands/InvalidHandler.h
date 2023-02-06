#pragma once
#include "MsgHandler.h"
class CInvalidHandler :
	public CMsgHandler
{
public:
	void OnOpen();
	void OnClose();

	void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer){};
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer){};

	CInvalidHandler(CClientContext *pClient);
	~CInvalidHandler();
};

