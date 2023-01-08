#pragma once
#include "afxcmn.h"
#include "resource.h"

// CFileMgrSearchDlg 对话框
#define WM_FILE_MGR_SEARCH_FOUND		(WM_USER+195)
#define WM_FILE_MGR_SEARCH_OVER			(WM_USER+196)

#define WM_FILE_MGR_SEARCH_ERROR		(WM_USER + 197)

class CFileMgrSearchSrv;

class CFileMgrSearchDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFileMgrSearchDlg)

public:
	CFileMgrSearchDlg(CFileMgrSearchSrv*pHandler,CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CFileMgrSearchDlg();

// 对话框数据
	enum { IDD = IDD_FM_SEARCH_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:

	BOOL m_DestroyAfterDisconnect;
	CFileMgrSearchSrv*m_pHandler;

	CString m_IP;
	CString m_TargetName;
	CString m_StartLocation;
	CListCtrl m_ResultList;
	afx_msg LRESULT OnFound(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnOver(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnError(WPARAM wParam, LPARAM lParam);

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnClose();
	afx_msg void OnNMDblclkList1(NMHDR *pNMHDR, LRESULT *pResult);
	virtual void OnOK();
	virtual void OnCancel();
	virtual void PostNcDestroy();
};
