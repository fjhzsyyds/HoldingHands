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
	: CDialogEx(CMiniFileTransDlg::IDD, pParent),
	m_DestroyAfterDisconnect(FALSE),
	m_pHandler(pHandler),
	m_ullTotalSize(0),
	m_ullFinishedSize(0),
	m_dwTotalCount(0),
	m_dwFinishedCount(0),
	m_dwFailedCount(0),
	m_ullCurrentFinishedSize(0),
	m_ullCurrentFileSize(0)
{
	
}

CMiniFileTransDlg::~CMiniFileTransDlg()
{
}

void CMiniFileTransDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TRANS_PROGRESS, m_Progress);
	DDX_Control(pDX, IDC_RICHEDIT21, m_TransLog);
}


BEGIN_MESSAGE_MAP(CMiniFileTransDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_MESSAGE(WM_MNFT_TRANS_INFO, OnTransInfo)
	ON_MESSAGE(WM_MNFT_FILE_TRANS_BEGIN, OnTransFileBegin)
	ON_MESSAGE(WM_MNFT_FILE_TRANS_FINISHED, OnTransFileFinished)
	ON_MESSAGE(WM_MNFT_FILE_DC_TRANSFERRED, OnTransFileDC)
	ON_MESSAGE(WM_MNFT_TRANS_FINISHED, OnTransFinished)
	ON_MESSAGE(WM_MNFT_ERROR,OnError)
END_MESSAGE_MAP()


// CMiniFileTransDlg ��Ϣ�������



void CMiniFileTransDlg::PostNcDestroy()
{
	// TODO:  �ڴ����ר�ô����/����û���
	CDialogEx::PostNcDestroy();
	if (!m_DestroyAfterDisconnect){
		delete this;
	}
}


void CMiniFileTransDlg::OnClose()
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (m_pHandler){
		m_DestroyAfterDisconnect = TRUE;
		m_pHandler->Close();
	}
	else{
		//m_pHandler�Ѿ�û��,����ֻ���Լ�����.
		DestroyWindow();
	}
}


BOOL CMiniFileTransDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	//
	auto const peer = m_pHandler->GetPeerName();
	m_IP = CA2W(peer.first.c_str());

	m_Progress.SetRange(0, 100);
	m_Progress.SetPos(0);

	m_TransLog.LimitText(0x7fffffff);
	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣:  OCX ����ҳӦ���� FALSE
}

LRESULT CMiniFileTransDlg::OnError(WPARAM wParam, LPARAM lParam){
	TCHAR * szError = (TCHAR*)wParam;
	MessageBox(szError, TEXT("Error"), MB_OK | MB_ICONINFORMATION);
	return 0;
}

CString CMiniFileTransDlg::GetProgressString(ULONGLONG ullFinished, ULONGLONG ullTotal){
	LARGE_INTEGER liTotalSize, liFinishedSize;
	TCHAR szTotalSize[64], szFinishedSize[64];
	liTotalSize.QuadPart = ullTotal;
	liFinishedSize.QuadPart = ullFinished;

	GetStorageSizeString(liFinishedSize, szFinishedSize);
	GetStorageSizeString(liTotalSize, szTotalSize);

	//��ֹΪ0....
	if (liTotalSize.QuadPart == 0){
		liTotalSize.QuadPart = 1;
	}
	CString Text;
	Text.Format(TEXT("%s / %s (%llu%%)"), szFinishedSize, szTotalSize,
		100 * liFinishedSize.QuadPart / liTotalSize.QuadPart);
	return Text;
}

//�ʼ��ʱ���ܵĴ�����Ϣ.
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

	//�����ܵĴ�����Ϣ.
	Text = GetProgressString(m_ullFinishedSize, m_ullTotalSize);
	GetDlgItem(IDC_TOTAL_PROGRESS)->SetWindowText(Text);
	return 0;
}


//��һ���ļ���ʼ��ʱ���������.
LRESULT CMiniFileTransDlg::OnTransFileBegin(WPARAM wParam, LPARAM lParam)
{
	CMiniFileTransSrv::FileInfo*pFile = (CMiniFileTransSrv::FileInfo*)wParam;
	CString Text;
	int length = m_TransLog.GetTextLength();
	Text.Format(TEXT("Processing: %s"), pFile->RelativeFilePath);
	m_TransLog.SetSel(length, length);

	m_TransLog.ReplaceSel(Text);

	//���õ�ǰ�ļ�
	Text.Format(TEXT("%s (%d/%d)"), pFile->RelativeFilePath,
		m_dwFinishedCount + 1, m_dwTotalCount);
	GetDlgItem(IDC_CURRENT_FILE)->SetWindowText(Text);

	//��ǰ�ļ�����.
	m_ullCurrentFileSize = pFile->dwFileLengthHi;
	m_ullCurrentFileSize <<= 32;
	m_ullCurrentFileSize |= pFile->dwFileLengthLo;

	m_ullCurrentFinishedSize = 0;
	
	Text = GetProgressString(m_ullCurrentFinishedSize, m_ullCurrentFileSize);
	GetDlgItem(IDC_CURRENT_PROGRESS)->SetWindowText(Text);
	return 0;
}

//ÿһ���ļ�����������������.


#define FILE_TRANS_MAX_LOG_SIZE		(50 * 1024)

LRESULT CMiniFileTransDlg::OnTransFileFinished(WPARAM wParam, LPARAM lParam)
{
	int length = m_TransLog.GetTextLength();
	CString Text = L"    -Successed\r\n";

	if (wParam != MNFT_STATU_SUCCESS){
		m_dwFailedCount++;
		Text = L"    -Failed\r\n";
	}

	//if ((length + Text.GetLength()) > FILE_TRANS_MAX_LOG_SIZE){
	//	//��֪��Ϊɶ���ݶ���ֱ������ ??????
	//	m_TransLog.SetWindowText(TEXT(""));
	//	length = 0;
	//}

	//��¼���.
	m_TransLog.SetSel(length, length);
	m_TransLog.ReplaceSel(Text);
	m_dwFinishedCount++;
	return 0;
}

//
LRESULT CMiniFileTransDlg::OnTransFileDC(WPARAM wParam, LPARAM lParam)
{
	m_ullFinishedSize += wParam;
	m_ullCurrentFinishedSize += wParam;
	CString Text;

	//���µ�ǰ�ļ�����.
	Text = GetProgressString(m_ullCurrentFinishedSize, m_ullCurrentFileSize);
	GetDlgItem(IDC_CURRENT_PROGRESS)->SetWindowTextW(Text);

	//�����ܽ���
	Text = GetProgressString(m_ullFinishedSize, m_ullTotalSize);
	GetDlgItem(IDC_TOTAL_PROGRESS)->SetWindowText(Text);

	//���½�����;
	m_Progress.SetPos(m_ullFinishedSize * 100 / m_ullTotalSize);
	return 0;
}

//���ս�����ʱ���������...
LRESULT CMiniFileTransDlg::OnTransFinished(WPARAM wParam, LPARAM lParam)
{
	CString Text;
	Text.Format(TEXT("[%s] Transfer Complete!  Failed : %d/%d "),
		m_IP.GetBuffer(),m_dwFailedCount,m_dwTotalCount);

	SetWindowText(Text);
	return 0;
}



void CMiniFileTransDlg::OnCancel()
{
}


void CMiniFileTransDlg::OnOK()
{
}
