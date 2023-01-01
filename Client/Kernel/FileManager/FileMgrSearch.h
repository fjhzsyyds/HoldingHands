#pragma once
#include "MsgHandler.h"
#include "SearchFile.h"
#define FILEMGR_SEARCH			('S'|('R'<<8)|('C'<<16)|('H'<<24))

#define FILE_MGR_SEARCH_SEARCH		(0xaca1)

#define FILE_MGR_SEARCH_STOP		(0xaca2)

#define FILE_MGR_SEARCH_FOUND		(0xaca3)

#define FILE_MGR_SEARCH_OVER		(0xaca4)

class CFileMgrSearch :
	public CMsgHandler
{
private:

	CSearchFile m_searcher;

public:
	void OnClose();
	void OnOpen();

	void OnReadMsg(WORD Msg,  DWORD dwSize, char*Buffer);
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer){};


	void OnSearch(char*Buffer);
	void OnStop();

	CFileMgrSearch(CManager*pManager);
	~CFileMgrSearch();
};

