#pragma once
#include "afxwin.h"


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


	CChatSrv*		m_pHandler;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnClose();
	afx_msg void OnBnClickedOk();
	CString m_Msg;
	CEdit m_MsgList;
	virtual BOOL OnInitDialog();
};
