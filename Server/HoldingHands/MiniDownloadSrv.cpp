#include "stdafx.h"
#include "MiniDownloadSrv.h"
#include "MiniDownloadDlg.h"

CMiniDownloadSrv::CMiniDownloadSrv(DWORD Identity) :
CEventHandler(Identity)
{
	m_pDlg = NULL;
}


CMiniDownloadSrv::~CMiniDownloadSrv()
{

}

void CMiniDownloadSrv::OnClose()
{
	if (m_pDlg){
		m_pDlg->SendMessage(WM_CLOSE);
		m_pDlg->DestroyWindow();
		delete m_pDlg;
		m_pDlg = NULL;
	}
}
void CMiniDownloadSrv::OnConnect()
{
	//��������.��ȡ�ļ���С.
	Send(MNDD_GET_FILE_INFO, 0, 0);
	//
	m_pDlg = new CMiniDownloadDlg(this);
	if (FALSE == m_pDlg->Create(IDD_MNDD_DLG,CWnd::GetDesktopWindow()))
	{
		Disconnect();
		return;
	}
	m_pDlg->ShowWindow(SW_SHOW);
}

void CMiniDownloadSrv::OnReadPartial(WORD Event, DWORD Total, DWORD dwRead, char*Buffer)
{

}
void CMiniDownloadSrv::OnReadComplete(WORD Event, DWORD Total, DWORD dwRead, char*Buffer)
{
	switch (Event)
	{
	case MNDD_FILE_INFO:
		OnGetFileInfoRpl(dwRead, Buffer);
		break;
	case MNDD_DOWNLOAD_RESULT:
		OnDownloadRpl(dwRead, Buffer);
		break;
	default:
		break;
	}
}

void CMiniDownloadSrv::OnWritePartial(WORD Event, DWORD Total, DWORD dwWrite, char*Buffer)
{

}
void CMiniDownloadSrv::OnWriteComplete(WORD Event, DWORD Total, DWORD dwWrite, char*Buffer)
{

}

void CMiniDownloadSrv::OnDownloadRpl(DWORD dwRead, char*Buffer)
{
	DownloadResult* pResult = (DownloadResult*)Buffer;
	switch (pResult->dwStatu)
	{
	case 0:										//���سɹ�
		Send(MNDD_DOWNLOAD_CONTINUE, 0, 0);
		break;
	default:
		Send(MNDD_DOWNLOAD_END, 0, 0);			//�������,��������������Ϻͳ��ִ���,ֱ����ֹ����.
		break;
	}
	m_pDlg->SendMessage(WM_MNDD_DOWNLOAD_RESULT, (WPARAM)pResult, 0);
}

void CMiniDownloadSrv::OnGetFileInfoRpl(DWORD dwRead, char*Buffer)
{
	MnddFileInfo*pFileInfo = (MnddFileInfo*)Buffer;
	switch (pFileInfo->dwStatu)
	{
	case MNDD_STATU_OK:	//ok
	case MNDD_STATU_UNKNOWN_FILE_SIZE://��Сδ֪.
		Send(MNDD_DOWNLOAD_CONTINUE, 0, 0);//��ʼ����
		break;
	default:
		Send(MNDD_DOWNLOAD_END, 0, 0);		//��ֹ����.
		break;
	}
	m_pDlg->SendMessage(WM_MNDD_FILEINFO, (WPARAM)pFileInfo, 0);
}