#include "stdafx.h"
#include "FileMgrSearchSrv.h"
#include "FileMgrSearchDlg.h"

CFileMgrSearchSrv::CFileMgrSearchSrv(CManager*pManager) :
	CMsgHandler(pManager)
	, m_pDlg(nullptr)
{
}


CFileMgrSearchSrv::~CFileMgrSearchSrv()
{
}

void CFileMgrSearchSrv::OnClose()
{
	if (m_pDlg){
		if (m_pDlg->m_DestroyAfterDisconnect){
			//�����ȹرյ�.
			m_pDlg->DestroyWindow();
			delete m_pDlg;
		}
		else{
			// pHandler�ȹرյ�,��ô�Ͳ��ܴ�����
			m_pDlg->m_pHandler = nullptr;
			m_pDlg->PostMessage(WM_FILE_MGR_SEARCH_ERROR, (WPARAM)TEXT("Disconnect."));
		}
		m_pDlg = nullptr;
	}
}
void CFileMgrSearchSrv::OnOpen()
{
	m_pDlg = new CFileMgrSearchDlg(this);

	if (!m_pDlg->Create(IDD_FM_SEARCH_DLG,CWnd::GetDesktopWindow()))
	{
		Close();
		return;
	}
	m_pDlg->ShowWindow(SW_SHOW);
}
//�����ݵ����ʱ���������������.

void CFileMgrSearchSrv::OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer)
{
	switch (Msg)
	{
	case FILE_MGR_SEARCH_FOUND:
		OnFound(Buffer);
		break;
	case FILE_MGR_SEARCH_OVER:
		OnOver();
		break;
	default:
		break;
	}
}
//�����ݷ�����Ϻ��������������

void CFileMgrSearchSrv::OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer)
{

}

void CFileMgrSearchSrv::OnFound(char*Buffer)
{	
	m_pDlg->SendMessage(WM_FILE_MGR_SEARCH_FOUND, (WPARAM)Buffer, 0);
}
void CFileMgrSearchSrv::OnOver()
{
	m_pDlg->SendMessage(WM_FILE_MGR_SEARCH_OVER, 0, 0);
}

void CFileMgrSearchSrv::Search(TCHAR*szParams)
{
	SendMsg(FILE_MGR_SEARCH_SEARCH, (char*)szParams, (sizeof(TCHAR) * (lstrlen(szParams) + 1)));
}

void CFileMgrSearchSrv::Stop()
{
	SendMsg(FILE_MGR_SEARCH_STOP, 0, 0);
}