#pragma once
#include "afxwin.h"
#include "resource.h"

#define WM_SOCKS_PROXY_ERROR	(WM_USER + 122)
#define WM_SOCKS_PROXY_LOG		(WM_USER + 123)
#define WM_SOCKS_PROXY_UPDATE	(WM_USER + 124)


#define UPDATE_ADDCON	0x0
#define UPDATE_DELCON	0x1
#define UPDATE_UPLOAD	0x2
#define UPDATE_DOWNLOAD 0x3

struct ConInfo{
	char	m_type;
	char	m_Source[256];
	char	m_Remote[256];
	DWORD	m_ClientID;
};

struct FlowInfo{
	LARGE_INTEGER liFlow;
	DWORD ClientID;
};

class CSocksProxySrv;
class CSocksProxyWnd :
	public CFrameWnd
{
	
public:
	BOOL			 m_DestroyAfterDisconnect;
	CSocksProxySrv * m_pHandler;
	BOOL			 m_IsRunning;

	CString			m_IP;

	CString			m_BindAddress;
	DWORD			m_BindPort;
	
	CListCtrl		m_Connections;

	CSocksProxyWnd(CSocksProxySrv*pHandler);
	~CSocksProxyWnd();

	CMenu	m_Menu;
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	virtual void PostNcDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);

	LRESULT OnError(WPARAM wParam, LPARAM lParam);
	LRESULT OnUpdateList(WPARAM wParam, LPARAM lParam);

	afx_msg void OnVerSocks4();
	afx_msg void OnVerSocks5();
	afx_msg void OnMainStartproxy();
	afx_msg void OnMainStop();
	afx_msg void OnUpdateMainStartproxy(CCmdUI *pCmdUI);
	afx_msg void OnUpdateMainStop(CCmdUI *pCmdUI);
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	afx_msg void OnNMRClickList(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnConnectionsDisconnectall();
	afx_msg void OnConnectionsDisconnect();
};

