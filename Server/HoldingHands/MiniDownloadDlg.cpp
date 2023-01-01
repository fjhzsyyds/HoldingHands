// MiniDownloadDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "MiniDownloadDlg.h"
#include "afxdialogex.h"
#include "MiniDownloadSrv.h"
#include "utils.h"

// CMiniDownloadDlg 对话框

IMPLEMENT_DYNAMIC(CMiniDownloadDlg, CDialogEx)

CMiniDownloadDlg::CMiniDownloadDlg(CMiniDownloadSrv* pHandler, CWnd* pParent /*=NULL*/)
	: CDialogEx(CMiniDownloadDlg::IDD, pParent)
{
	m_pHandler = pHandler;
	auto const peer = m_pHandler->GetPeerName();
	m_IP = CA2W(peer.first.c_str());;
}

CMiniDownloadDlg::~CMiniDownloadDlg()
{
}

void CMiniDownloadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DOWNLOAD_PROGRESS, m_Progress);
}


BEGIN_MESSAGE_MAP(CMiniDownloadDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CMiniDownloadDlg::OnBnClickedOk)
	ON_MESSAGE(WM_MNDD_FILEINFO, OnFileInfo)
	ON_MESSAGE(WM_MNDD_DOWNLOAD_RESULT, OnDownloadResult)
	ON_MESSAGE(WM_MNDD_ERROR,OnError)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CMiniDownloadDlg 消息处理程序


void CMiniDownloadDlg::OnBnClickedOk()
{
}


LRESULT CMiniDownloadDlg::OnError(WPARAM wParam, LPARAM lParam){
	TCHAR * szError = (TCHAR*)wParam;
	MessageBox(szError, TEXT("Error"), MB_OK | MB_ICONERROR);
	return 0;
}

LRESULT CMiniDownloadDlg::OnFileInfo(WPARAM wParam, LPARAM lParam)
{
	/*
	LPVOID ArgList[3];
	ArgList[0] = (LPVOID)root["filename"].asCString();
	ArgList[1] = (LPVOID)root["url"].asCString();
	ArgList[2] = (LPVOID)root["filesize"].asInt();
	*/
	int Argc = wParam;
	LPVOID * ArgList = (LPVOID*)lParam;

	CString FileName = CA2W((char*)ArgList[0]);
	CString Url = CA2W((char*)ArgList[1]);
	//
	int TotalSize = (int)ArgList[2];

	GetDlgItem(IDC_URL)->SetWindowText(Url);
	GetDlgItem(IDC_FILE_NAME)->SetWindowText(FileName);

	if (TotalSize == -1){
		m_Progress.SetMarquee(TRUE, 30);//进度条设置为未定义.
	}
	return 0;
}

LRESULT CMiniDownloadDlg::OnDownloadResult(WPARAM wParam, LPARAM lParam)
{

	/*
	LPVOID ArgList[2];
	ArgList[0] = (LPVOID)root["total_size"].asInt();
	ArgList[1] = (LPVOID)root["finished_size"].asInt();
	*/
	int Argc = wParam;
	LPVOID * ArgList = (LPVOID*)lParam;

	int TotalSize = (int)ArgList[0];
	int FinishedSize = (int)ArgList[1];
	int Finished = (int)ArgList[2];
	long long Progress = 0;

	LARGE_INTEGER liFinished, liTotal;
	TCHAR strFinished[128], strTotal[128];
	
	liFinished.QuadPart = FinishedSize;
	liTotal.QuadPart = TotalSize;

	GetStorageSizeString(liFinished, strFinished);

	if (TotalSize != -1 && TotalSize){	
		GetStorageSizeString(liTotal, strTotal);
		CString strText;
		strText.Format(TEXT("%s / %s"), strFinished, strTotal);
		GetDlgItem(IDC_PROGRESS)->SetWindowText(strText);

		Progress = FinishedSize;
		Progress *= 100;
		Progress /= TotalSize;
		m_Progress.SetPos(Progress);
	}
	else{
		CString strText;
		strText.Format(TEXT("%s / unknown"), strFinished);
		GetDlgItem(IDC_PROGRESS)->SetWindowText(strText);
	}

	if (Finished){
		MessageBox(TEXT("Download Finished"), TEXT("Tips"), MB_OK | MB_ICONINFORMATION);
	}
	return 0;
}

void CMiniDownloadDlg::OnClose()
{
	if (m_pHandler){
		m_pHandler->Close();
		m_pHandler = NULL;
	}
}


BOOL CMiniDownloadDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	CString Title;
	Title.Format(TEXT("[%s] Download...."), m_IP.GetBuffer());
	SetWindowText(Title);

	m_Progress.SetRange(0, 100);
	m_Progress.SetPos(0);
	return TRUE;  // return TRUE unless you set the focus to a control
}
