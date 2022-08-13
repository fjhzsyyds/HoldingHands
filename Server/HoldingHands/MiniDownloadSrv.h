#pragma once
#include "EventHandler.h"
#include <WinInet.h>
#define MINIDOWNLOAD	('M'|('N'<<8)|('D'<<16)|('D'<<24))

//OnConnect
#define MNDD_STATU_OK						(0x0)
#define MNDD_STATU_ANALYSE_URL_FAILED		(0x1)
#define MNDD_STATU_INTERNETOPEN_FAILED		(0x2)
#define MNDD_STATU_INTERNETCONNECT_FAILED	(0x3)
#define MNDD_STATU_OPENREMOTEFILE_FILED		(0x4)
#define MNDD_STATU_UNKNOWN_FILE_SIZE		(0x5)	//�ļ���Сδ֪
#define MNDD_STATU_UNSUPPORTED_PROTOCOL		(0x6)
#define MNDD_STATU_HTTPSENDREQ_FAILED		(0x7)

#define MNDD_STATU_OPENLOCALFILE_FAILED		(0x8)

#define MNDD_GET_FILE_INFO		(0xabc0)
#define MNDD_FILE_INFO			(0xabd1)

#define MNDD_DOWNLOAD_CONTINUE	(0xabd3)

//�ɹ�,�����ļ�ʧ��.
#define MNDD_DOWNLOAD_RESULT	(0xabd4)

#define MNDD_DOWNLOAD_END		(0xabd6)



class CMiniDownloadDlg;

class CMiniDownloadSrv :
	public CEventHandler
{
public:
	typedef struct MnddFileInfo{
		DWORD dwStatu;
		DWORD dwFileSizeLo;
		DWORD dwFileSizeHi;
	}MnddFileInfo;

	/*
	0.Ok
	1.InternetReadFileFailed.
	2.WriteFileFailed.
	3.Finished.
	*/
	typedef struct _DownloadResult
	{
		DWORD dwStatu;
		DWORD dwWriteSize;
	}DownloadResult;

private:
	CMiniDownloadDlg*m_pDlg;
public:
	void OnClose();					//��socket�Ͽ���ʱ������������
	void OnConnect();				//��socket���ӵ�ʱ������������
	//�����ݵ����ʱ���������������.
	void OnReadPartial(WORD Event, DWORD Total, DWORD nRead, char*Buffer);
	void OnReadComplete(WORD Event, DWORD Total, DWORD nRead, char*Buffer);
	//�����ݷ�����Ϻ��������������
	void OnWritePartial(WORD Event, DWORD Total, DWORD nWrite, char*Buffer);
	void OnWriteComplete(WORD Event, DWORD Total, DWORD nWrite, char*Buffer);

	void OnGetFileInfoRpl(DWORD dwRead, char*Buffer);
	void OnDownloadRpl(DWORD dwRead, char*Buffer);
	CMiniDownloadSrv(DWORD Identity);
	~CMiniDownloadSrv();
};

