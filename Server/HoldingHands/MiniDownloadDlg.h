#pragma once
#include "afxcmn.h"
#include "resource.h"
#define WM_MNDD_FILEINFO			(WM_USER+701)
#define WM_MNDD_DOWNLOAD_RESULT		(WM_USER+702)

#define WM_MNDD_ERROR				(WM_USER + 703)

// CMiniDownloadDlg �Ի���

class CMiniDownloadSrv;

class CMiniDownloadDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CMiniDownloadDlg)

public:
	CMiniDownloadDlg(CMiniDownloadSrv* pHandler, CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CMiniDownloadDlg();

// �Ի�������
	enum { IDD = IDD_MNDD_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	CProgressCtrl		m_Progress;
	CMiniDownloadSrv*	m_pHandler;

	CString				m_IP;

	afx_msg LRESULT OnFileInfo(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDownloadResult(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnError(WPARAM wParam, LPARAM lParma);
	afx_msg void OnClose();
	virtual BOOL OnInitDialog();
};
