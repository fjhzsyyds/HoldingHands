// ChatDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "ChatDlg.h"
#include "afxdialogex.h"
#include "ChatSrv.h"

// CChatDlg �Ի���

IMPLEMENT_DYNAMIC(CChatDlg, CDialogEx)

CChatDlg::CChatDlg(CChatSrv*pHandler, CWnd* pParent /*=NULL*/)
	: CDialogEx(CChatDlg::IDD, pParent)
	, m_Msg(_T(""))
{
	m_pHandler = pHandler;
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
	ON_BN_CLICKED(IDOK, &CChatDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CChatDlg ��Ϣ�������


void CChatDlg::OnClose()
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (m_pHandler){
		m_pHandler->Close();
		m_pHandler = NULL;
	}
}


void CChatDlg::OnBnClickedOk()
{
	UpdateData();
	if (m_Msg.GetLength() == 0)
		return;
	if (m_pHandler)
	{
		m_pHandler->SendMsgMsg(m_Msg.GetBuffer());
		//���Լ�����Ϣ��ʾ����Ļ��.
		CString MyMsg;
		MyMsg.Format(L"[me]:%s\r\n", m_Msg.GetBuffer());

		m_MsgList.SetSel(-1);
		m_MsgList.ReplaceSel(MyMsg);
	}
	m_Msg = L"";
	UpdateData(FALSE);
}


BOOL CChatDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	ASSERT(m_pHandler != NULL);
	auto const peer = m_pHandler->GetPeerName();

	CString Title;
	Title.Format(L"[%s] Chat ", CA2W(peer.first.c_str()).m_psz);
	SetWindowText(Title);
	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣:  OCX ����ҳӦ���� FALSE
}
