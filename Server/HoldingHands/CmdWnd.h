#pragma once
#include "afxwin.h"
#include "CmdEdit.h"

#define WM_CMD_BEGIN	(WM_USER + 125)
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

	CCmdEdit		m_CmdShow;
	CBrush			m_BkBrush;
	CCmdSrv			*m_pHandler;

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	LRESULT OnCmdBegin(WPARAM wParam, LPARAM lParam);
	LRESULT OnError(WPARAM wParam, LPARAM lParam);
	LRESULT OnCmdResult(WPARAM wParam, LPARAM lParam);

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PostNcDestroy();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};

