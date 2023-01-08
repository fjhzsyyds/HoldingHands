#pragma once
#include "afxwin.h"

#define WM_CHATDLG_ERROR	(WM_USER + 101)

// CChatDlg �Ի���
class CChatSrv;

class CChatDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CChatDlg)

public:
	CChatDlg(CChatSrv*pHandler, CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CChatDlg();

// �Ի�������
	enum { IDD = IDD_CHAT_DLG };

	BOOL m_DestroyAfterDisconnect;
	CChatSrv*		m_pHandler;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	afx_msg LRESULT OnError(WPARAM wParam, LPARAM lParma);
	afx_msg void OnClose();
	afx_msg void OnBnClickedOk();
	CString m_Msg;
	CEdit m_MsgList;
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();
	virtual void OnOK();
	virtual void OnCancel();
};
