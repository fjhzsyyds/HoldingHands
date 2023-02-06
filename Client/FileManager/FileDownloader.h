#pragma once
#include "..\Common\MsgHandler.h"
#include <WinInet.h>
#define MINIDOWNLOAD	('M'|('N'<<8)|('D'<<16)|('D'<<24))



#define MNDD_GET_FILE_INFO		(0xabc0)
#define MNDD_FILE_INFO			(0xabd1)

#define MNDD_DOWNLOAD_CONTINUE	(0xabd3)

//成功,创建文件失败.;
#define MNDD_DOWNLOAD_RESULT	(0xabd4)

#define MNDD_DOWNLOAD_END		(0xabd6)


#define FILEDOWNLOADER_FLAG_RUNAFTERDOWNLOAD (0x1)

class CFileDownloader :
	public CMsgHandler
{
	/*
		0.Ok
		1.InternetReadFileFailed.
		2.WriteFileFailed.
		3.Finished.
	*/
private:
	HINTERNET	m_hInternet;
	HINTERNET	m_hConnect;
	HINTERNET	m_hRemoteFile;
	HANDLE		m_hLocalFile;

	BOOL	m_DownloadSuccess;

	int		m_iTotalSize;
	int		m_iFinishedSize;

	URL_COMPONENTSW url;

	char*	 m_Buffer;

	struct InitParam;
	InitParam*	 m_pInit;

	WCHAR*	 m_szSavePath;
	WCHAR*	 m_szUrlPath;
	WCHAR*	 m_szHost;
	WCHAR*	 m_szUser;
	WCHAR*	 m_szPassword;
	WCHAR*	 m_szExtraInfo;

public:
	struct InitParam{
		DWORD dwFlags;
		WCHAR szURL[4];
	};

	void OnOpen();
	void OnClose();
	void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer){}

	CFileDownloader(CIOCPClient*pClient,InitParam* pInitParam);

	void OnGetFileInfo();
	void OnContinueDownload();
	void OnEndDownload();

	~CFileDownloader();
};

