// MiniFileTransDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "MiniFileTransDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "MiniFileTransSrv.h"
#include "utils.h"
// CMiniFileTransDlg �Ի���

IMPLEMENT_DYNAMIC(CMiniFileTransDlg, CDialogEx)

CMiniFileTransDlg::CMiniFileTransDlg(CMiniFileTransSrv*pHandler, CWnd* pParent /*=NULL*/)
	: CDialogEx(CMiniFileTransDlg::IDD, pParent)
{
	m_pHandler = pHandler;

	m_ullTotalSize = 0;
	m_ullFinishedSize = 0;

	m_dwTotalCount = 0;
	m_dwFinishedCount = 0;
	m_dwFailedCount = 0;

	auto const peer = m_pHandler->GetPeerName();
	m_IP = CA2W(peer.first.c_str());
}

CMiniFileTransDlg::~CMiniFileTransDlg()
{
}

void CMiniFileTransDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TRANS_PROGRESS, m_Progress);
	DDX_Control(pDX, IDC_EDIT1, m_TransLog);
}


BEGIN_MESSAGE_MAP(CMiniFileTransDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CMiniFileTransDlg::OnBnClickedOk)
	ON_WM_CLOSE()
	ON_MESSAGE(WM_MNFT_TRANS_INFO, OnTransInfo)
	ON_MESSAGE(WM_MNFT_FILE_TRANS_BEGIN, OnTransFileBegin)
	ON_MESSAGE(WM_MNFT_FILE_TRANS_FINISHED, OnTransFileFinished)
	ON_MESSAGE(WM_MNFT_FILE_DC_TRANSFERRED, OnTransFileDC)
	ON_MESSAGE(WM_MNFT_TRANS_FINISHED, OnTransFinished)
END_MESSAGE_MAP()


// CMiniFileTransDlg ��Ϣ�������


void CMiniFileTransDlg::OnBnClickedOk()
{
	//��ֹ�˳�.
}


void CMiniFileTransDlg::OnClose()
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (m_pHandler != NULL){
		//�Ͽ�����
		m_pHandler->Close();
		m_pHandler = NULL;
	}
}


BOOL CMiniFileTransDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	//
	m_Progress.SetRange(0, 100);
	m_Progress.SetPos(0);
	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣:  OCX ����ҳӦ���� FALSE
}

LRESULT CMiniFileTransDlg::OnTransInfo(WPARAM wParam, LPARAM lParam)
{
	CMiniFileTransSrv::MNFT_Trans_Info*pTransInfo = (CMiniFileTransSrv::MNFT_Trans_Info*)wParam;
	m_ullTotalSize = pTransInfo->TotalSizeHi;
	m_ullTotalSize <<= 32;
	m_ullTotalSize |= pTransInfo->TotalSizeLo;

	m_dwTotalCount = pTransInfo->dwFileCount;
	//
	CString Text;
	if (lParam == MNFT_DUTY_RECEIVER)
		Text.Format(TEXT("[%s] Downloading..."), m_IP.GetBuffer());
	else
		Text.Format(TEXT("[%s] Uploading..."), m_IP.GetBuffer());
	SetWindowText(Text);

	Text.Format(TEXT("%d / %d"), m_dwFinishedCount, m_dwTotalCount);
	GetDlgItem(IDC_COUNT_PROGRESS)->SetWindowTextW(Text);
	return 0;
}
LRESULT CMiniFileTransDlg::OnTransFileBegin(WPARAM wParam, LPARAM lParam)
{
	CMiniFileTransSrv::FileInfo*pFile = (CMiniFileTransSrv::FileInfo*)wParam;
	CString Text;
	Text.Format(TEXT("Processing:%s"), pFile->RelativeFilePath);
	m_TransLog.SetSel(-1);
	m_TransLog.ReplaceSel(Text);

	GetDlgItem(IDC_CURRENT_FILE)->SetWindowTextW(pFile->RelativeFilePath);
	return 0;
}

LRESULT CMiniFileTransDlg::OnTransFileFinished(WPARAM wParam, LPARAM lParam)
{
	m_TransLog.SetSel(-1);
	CString Text = L"   -OK\r\n";

	if (wParam != MNFT_STATU_SUCCESS){
		m_dwFailedCount++;
		Text = L"   -Failed\r\n";
	}
	//��¼���.
	m_TransLog.ReplaceSel(Text);
	m_dwFinishedCount++;
	Text.Format(TEXT("%d / %d"), m_dwFinishedCount, m_dwTotalCount);
	GetDlgItem(IDC_COUNT_PROGRESS)->SetWindowTextW(Text);
	return 0;
}

LRESULT CMiniFileTransDlg::OnTransFileDC(WPARAM wParam, LPARAM lParam)
{
	m_ullFinishedSize += wParam;

	LARGE_INTEGER liFinished, liTotal;
	TCHAR strFinished[128], strTotal[128];
	liFinished.QuadPart = m_ullFinishedSize, liTotal.QuadPart = m_ullTotalSize;
	GetStorageSizeString(liFinished, strFinished);
	GetStorageSizeString(liTotal, strTotal);

	//SetText
	CString Text;
	Text.Format(TEXT("%s/%s"), strFinished, strTotal);
	GetDlgItem(IDC_SZIE_PROGRESS)->SetWindowTextW(Text);
	//SetProgressorPos;
	m_Progress.SetPos(m_ullFinishedSize * 100 / m_ullTotalSize);
	return 0;
}

LRESULT CMiniFileTransDlg::OnTransFinished(WPARAM wParam, LPARAM lParam)
{
	CString Text;
	Text.Format(TEXT("[%s] Transfer Complete!"), m_IP.GetBuffer());
	SetWindowText(Text);
	Text.Format(TEXT("Totoal File Count:%d \r\nSuccess:%d \r\nFailed:%d \r\n"), 
		m_dwTotalCount, m_dwFinishedCount - m_dwFailedCount, m_dwFailedCount);
	MessageBox(Text,TEXT("Transfer Complete!"));
	return 0;
}
