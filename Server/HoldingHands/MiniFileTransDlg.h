#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include "resource.h"

// CMiniFileTransDlg 对话框
class CMiniFileTransSrv;

#define WM_MNFT_TRANS_INFO					(WM_USER + 500)

#define WM_MNFT_FILE_TRANS_BEGIN			(WM_USER + 501)
#define WM_MNFT_FILE_DC_TRANSFERRED			(WM_USER + 502)
#define WM_MNFT_FILE_TRANS_FINISHED			(WM_USER + 503)

#define WM_MNFT_TRANS_FINISHED				(WM_USER + 504)
#define WM_MNFT_ERROR						(WM_USER + 505)


class CMiniFileTransDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CMiniFileTransDlg)

public:
	CMiniFileTransDlg(CMiniFileTransSrv*pHandler, CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CMiniFileTransDlg();

// 对话框数据
	enum { IDD = IDD_FILETRANS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	ULONGLONG		m_ullTotalSize;
	ULONGLONG		m_ullFinishedSize;

	ULONGLONG		m_ullCurrentFinishedSize;
	ULONGLONG		m_ullCurrentFileSize;

	DWORD			m_dwTotalCount;
	DWORD			m_dwFinishedCount;
	DWORD			m_dwFailedCount;

	BOOL			m_DestroyAfterDisconnect;
	CMiniFileTransSrv*m_pHandler;
	CString			m_IP;

	afx_msg void OnClose();

	afx_msg LRESULT OnTransInfo(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT OnTransFileBegin(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTransFileDC(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTransFileFinished(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT	OnTransFinished(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT OnError(WPARAM wParam, LPARAM lParam);

	static CString GetProgressString(ULONGLONG ullFinished, ULONGLONG ullTotal);
	virtual BOOL OnInitDialog();
	CProgressCtrl m_Progress;
	CEdit m_TransLog;
	virtual void PostNcDestroy();
	virtual void OnCancel();
	virtual void OnOK();
};
