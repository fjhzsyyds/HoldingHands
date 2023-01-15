#pragma once
#include "afxwin.h"
#include "resource.h"

#define WM_SOCKS_PROXY_ERROR	(WM_USER + 122)
#define WM_SOCKS_PROXY_LOG		(WM_USER + 123)

class CSocksProxySrv;
class CSocksProxyWnd :
	public CFrameWnd
{
	
public:
	BOOL			 m_DestroyAfterDisconnect;
	CSocksProxySrv * m_pHandler;
	CRichEditCtrl	 m_Log;
	CFont			 m_Font;
	BOOL			 m_IsRunning;

	CString			m_IP;

	CString			m_BindAddress;
	DWORD			m_BindPort;
	
	CSocksProxyWnd(CSocksProxySrv*pHandler);
	~CSocksProxyWnd();

	CMenu	m_Menu;
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	virtual void PostNcDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	LRESULT OnError(WPARAM wParam, LPARAM lParam);
	LRESULT OnLog(WPARAM wParam, LPARAM lParam);

	afx_msg void OnVerSocks4();
	afx_msg void OnVerSocks5();
	afx_msg void OnMainStartproxy();
	afx_msg void OnMainStop();
	afx_msg void OnUpdateMainStartproxy(CCmdUI *pCmdUI);
	afx_msg void OnUpdateMainStop(CCmdUI *pCmdUI);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};

