// ChatDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "ChatDlg.h"
#include "afxdialogex.h"
#include "ChatSrv.h"

// CChatDlg 对话框

IMPLEMENT_DYNAMIC(CChatDlg, CDialogEx)

CChatDlg::CChatDlg(CChatSrv*pHandler, CWnd* pParent /*=NULL*/)
	: CDialogEx(CChatDlg::IDD, pParent)
	, m_Msg(_T("")),
	m_DestroyAfterDisconnect(FALSE),
	m_pHandler(pHandler)
{
}

CChatDlg::~CChatDlg()
{
}

void CChatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT2, m_Msg);
	DDX_Control(pDX, IDC_EDIT1, m_MsgList);
}


BEGIN_MESSAGE_MAP(CChatDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_MESSAGE(WM_CHATDLG_ERROR,OnError)
	ON_BN_CLICKED(IDOK, &CChatDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CChatDlg 消息处理程序

void CChatDlg::PostNcDestroy()
{
	// TODO:  在此添加专用代码和/或调用基类
	CDialogEx::PostNcDestroy();
	if (!m_DestroyAfterDisconnect){
		delete this;
	}
}

void CChatDlg::OnClose()
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

LRESULT CChatDlg::OnError(WPARAM wParam, LPARAM lParma){
	TCHAR*szError = (TCHAR*)wParam;
	MessageBox(szError, TEXT("Error"), MB_OK | MB_ICONINFORMATION);
	return 0;
}
void CChatDlg::OnBnClickedOk()
{
	UpdateData();
	if (m_Msg.GetLength() == 0)
		return;
	if (m_pHandler)
	{
		m_pHandler->SendMsgText(m_Msg.GetBuffer());
		//把自己的消息显示到屏幕上.
		CString MyMsg;
		MyMsg.Format(TEXT("[me]:%s\r\n"), m_Msg.GetBuffer());

		m_MsgList.SetSel(-1);
		m_MsgList.ReplaceSel(MyMsg);
	}
	m_Msg = L"";
	UpdateData(FALSE);
}


BOOL CChatDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pHandler != NULL);
	auto const peer = m_pHandler->GetPeerName();

	CString Title;
	Title.Format(L"[%s] Chat ", CA2W(peer.first.c_str()).m_psz);
	SetWindowText(Title);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常:  OCX 属性页应返回 FALSE
}




void CChatDlg::OnOK()
{
}


void CChatDlg::OnCancel()
{
}
