// AudioDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "AudioDlg.h"
#include "afxdialogex.h"
#include "AudioSrv.h"

// CAudioDlg �Ի���

IMPLEMENT_DYNAMIC(CAudioDlg, CDialogEx)

CAudioDlg::CAudioDlg(CAudioSrv*pHandler, CWnd* pParent /*=NULL*/)
	: CDialogEx(CAudioDlg::IDD, pParent)
{
	m_pHandler = pHandler;
}

CAudioDlg::~CAudioDlg()
{
}

void CAudioDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CAudioDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_MESSAGE(WM_AUDIO_ERROR,OnError)
END_MESSAGE_MAP()


// CAudioDlg ��Ϣ��������


void CAudioDlg::OnClose()
{
	if (m_pHandler){
		m_pHandler->Disconnect();
		m_pHandler = NULL;
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

	// TODO:  �ڴ����Ӷ���ĳ�ʼ��
	char IP[128] = { 0 };
	USHORT uPort;
	m_pHandler->GetPeerName(IP, uPort);
	//
	CString Title;
	Title.Format(L"[%s] Microphone", CA2W(IP).m_szBuffer);
	SetWindowText(Title);
	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣:  OCX ����ҳӦ���� FALSE
}

LRESULT CAudioDlg::OnError(WPARAM wParam, LPARAM lParam)
{
	WCHAR*szError = (WCHAR*)wParam;
	MessageBox(szError, L"Error");
	return 0;
}