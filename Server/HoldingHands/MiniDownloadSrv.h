#pragma once
#include "MsgHandler.h"
#include <WinInet.h>
#define MINIDOWNLOAD	('M'|('N'<<8)|('D'<<16)|('D'<<24))

#define MNDD_GET_FILE_INFO		(0xabc0)
#define MNDD_FILE_INFO			(0xabd1)

#define MNDD_DOWNLOAD_CONTINUE	(0xabd3)

//成功,创建文件失败.;
#define MNDD_DOWNLOAD_RESULT	(0xabd4)

#define MNDD_DOWNLOAD_END		(0xabd6)


#define FILEDOWNLOADER_FLAG_RUNAFTERDOWNLOAD (0x1)
	

class CMiniDownloadDlg;

class CMiniDownloadSrv :
	public CMsgHandler
{
public:

	/*
	0.Ok
	1.InternetReadFileFailed.
	2.WriteFileFailed.
	3.Finished.
	*/
private:
	BOOL			m_DownloadFinished;
	CMiniDownloadDlg*m_pDlg;
public:
	void OnClose();					//当socket断开的时候调用这个函数
	void OnOpen();				//当socket连接的时候调用这个函数

	void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer);


	void OnFileInfo(char * FileInfo);
	void OnDownloadResult(char * result);

	CMiniDownloadSrv(CClientContext*pClient);

	~CMiniDownloadSrv();
};

