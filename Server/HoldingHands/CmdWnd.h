#pragma once
#include "afxwin.h"

#define WM_CMD_RESULT	(WM_USER + 126)

#define WM_CMD_ERROR	(WM_USER + 127)

class CCmdSrv;


class CCmdWnd :
	public CFrameWnd
{
public:
	CCmdWnd(CCmdSrv*pHandler);
	~CCmdWnd();

	BOOL m_DestroyAfterDisconnect;
	CCmdSrv *	m_pHandler;
	CFont m_Font;
	CEdit m_CmdShow;
	CEdit m_Cmd;


	CList<CString>	m_Commands;
	POSITION		m_LastCommand;
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	LRESULT OnError(WPARAM wParam, LPARAM lParam);
	LRESULT OnCmdResult(WPARAM wParam, LPARAM lParam);

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PostNcDestroy();
};

