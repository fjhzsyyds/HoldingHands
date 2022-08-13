// SettingDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "SettingDlg.h"
#include "afxdialogex.h"
#include "MainFrm.h"

// CSettingDlg 对话框

IMPLEMENT_DYNAMIC(CSettingDlg, CDialogEx)

CSettingDlg::CSettingDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSettingDlg::IDD, pParent)
	, m_ListenPort(_T("10086"))
{

}

CSettingDlg::~CSettingDlg()
{
}

void CSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_ListenPort);
}


BEGIN_MESSAGE_MAP(CSettingDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSettingDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CSettingDlg 消息处理程序
void CSettingDlg::OnBnClickedOk()
{
	CMainFrame*pMainFrame = NULL;
	UpdateData(TRUE);
	if (m_ListenPort.GetLength() == NULL){
		return;
	}
	pMainFrame = ((CMainFrame*)GetParent());
	pMainFrame->SetListenPort(atoi(CW2A(m_ListenPort)));

	CDialogEx::OnOK();
}

