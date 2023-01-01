#include "stdafx.h"
#include "FileMgrSearchSrv.h"
#include "FileMgrSearchDlg.h"

CFileMgrSearchSrv::CFileMgrSearchSrv(CManager*pManager) :
	CMsgHandler(pManager)
{
	m_pDlg = NULL;
}


CFileMgrSearchSrv::~CFileMgrSearchSrv()
{
}

void CFileMgrSearchSrv::OnClose()
{
	if (m_pDlg){
		m_pDlg->SendMessage(WM_CLOSE);
		m_pDlg->DestroyWindow();
		delete m_pDlg;
		m_pDlg = NULL;
	}
}
void CFileMgrSearchSrv::OnOpen()
{
	m_pDlg = new CFileMgrSearchDlg(this);

	if (m_pDlg->Create(IDD_FM_SEARCH_DLG,CWnd::GetDesktopWindow()) == FALSE)
	{
		Close();
		return;
	}
	m_pDlg->ShowWindow(SW_SHOW);
}
//有数据到达的时候调用这两个函数.

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
//有数据发送完毕后调用这两个函数

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

void CFileMgrSearchSrv::Search(WCHAR*szParams)
{
	SendMsg(FILE_MGR_SEARCH_SEARCH, (char*)szParams, (sizeof(WCHAR) * (wcslen(szParams) + 1)));
}

void CFileMgrSearchSrv::Stop()
{
	SendMsg(FILE_MGR_SEARCH_STOP, 0, 0);
}