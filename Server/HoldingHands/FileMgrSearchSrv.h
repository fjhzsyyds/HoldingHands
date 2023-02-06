#pragma once
#include "MsgHandler.h"

#define FILEMGR_SEARCH			('S'|('R'<<8)|('C'<<16)|('H'<<24))

#define FILE_MGR_SEARCH_SEARCH		(0xaca1)

#define FILE_MGR_SEARCH_STOP		(0xaca2)

#define FILE_MGR_SEARCH_FOUND		(0xaca3)

#define FILE_MGR_SEARCH_OVER		(0xaca4)

class CFileMgrSearchDlg;

class CFileMgrSearchSrv :
	public CMsgHandler
{
private:
	
	CFileMgrSearchDlg*	m_pDlg;

public:
	void OnClose();					//当socket断开的时候调用这个函数
	void OnOpen();				//当socket连接的时候调用这个函数
	//有数据到达的时候调用这两个函数.
	void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer);



	void OnFound(char*Buffer);
	void OnOver();

	void Search(TCHAR*szParams);
	void Stop();

	CFileMgrSearchSrv(CClientContext *pClient);
	~CFileMgrSearchSrv();
};

