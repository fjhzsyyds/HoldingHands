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
	void OnClose();					//��socket�Ͽ���ʱ������������
	void OnOpen();				//��socket���ӵ�ʱ������������
	//�����ݵ����ʱ���������������.
	void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer);



	void OnFound(char*Buffer);
	void OnOver();

	void Search(TCHAR*szParams);
	void Stop();

	CFileMgrSearchSrv(CClientContext *pClient);
	~CFileMgrSearchSrv();
};

