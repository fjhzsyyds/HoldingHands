#pragma once
#include "afxwin.h"
#include "resource.h"
class CRemoteDesktopSrv;

#define WM_REMOTE_DESKTOP_ERROR			(WM_USER + 69)
#define WM_REMOTE_DESKTOP_SIZE			(WM_USER + 71)
#define WM_REMOTE_DESKTOP_DRAW			(WM_USER + 70)

#define WM_REMOTE_DESKTOP_SET_CLIPBOARD_TEXT	(WM_USER + 72)
#define SCROLL_UINT		5.0

#define CONTROL_MOUSE (0x1)
#define CONTROL_KEYBOARD (0x2)

#define DISPLAY_FULLSCREEN  0x1
#define DISPLAY_STRETCH		0x2
#define DISPLAY_TILE		0x4

#define CAPTURE_MOUSE		0x1
#define CAPTURE_TRANSPARENT 0x2

class CRemoteDesktopWnd :
	public CFrameWnd
{
public:
	CRemoteDesktopSrv*m_pHandler;
	//FPS 
	DWORD	m_dwLastTime;
	DWORD	m_dwFps;
	CString m_Title;
	//
	DWORD	m_dwDeskWidth;		//�ͻ������ߴ�
	DWORD	m_dwDeskHeight;
							
	DWORD	m_dwMaxHeight;		//�������ߴ�
	DWORD	m_dwMaxWidth;

	HDC		m_hDC;
	POINT	m_OrgPt;			//������λ��.

	DWORD	m_dwCaptureFlags;
	// CONTROL
	BOOL m_ControlFlags;		//

	CMenu	m_Menu;
	CStringA m_SetClipbdText;		//�Լ����õ�����;
	HWND	m_hNextViewer;
	//
	DWORD	m_DisplayMode;			//ȫ����Ҫ����Ϣ
	DWORD	m_OldDisplayMode;
	WINDOWPLACEMENT m_old;
	CRect	m_FullScreen;
	CPoint	m_FullScreenDrawOrg;	//
	BOOL	m_bFullScreenStretchMode;

	CRemoteDesktopWnd(CRemoteDesktopSrv*pHandler);
	~CRemoteDesktopWnd();
	
	DECLARE_MESSAGE_MAP()
	afx_msg void OnClose();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDisplayStretch();
	afx_msg void OnDisplayTile();
	afx_msg void OnCaptureMouse();
	afx_msg void OnControlKeyboard();
	afx_msg void OnControlMouse();
	afx_msg void OnCaptureTransparentwindow();
	afx_msg void OnUpdateControlMouse(CCmdUI *pCmdUI);
	afx_msg void OnUpdateControlKeyboard(CCmdUI *pCmdUI);
	afx_msg void OnUpdateCaptureMouse(CCmdUI *pCmdUI);
	afx_msg void OnUpdateCaptureTransparentwindow(CCmdUI *pCmdUI);

	LRESULT	OnDraw(WPARAM wParam, LPARAM lParam);
	LRESULT OnError(WPARAM wParam, LPARAM lParam);
	LRESULT OnDesktopSize(WPARAM wParam, LPARAM lParam);
	LRESULT OnSetClipbdText(WPARAM wParam, LPARAM lParam);

	virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnMaxfps10();
	afx_msg void OnMaxfps20();
	afx_msg void OnMaxfps30();
	afx_msg void OnMaxfpsNolimit();
	afx_msg void OnChangeCbChain(HWND hWndRemove, HWND hWndAfter);
	afx_msg void OnDrawClipboard();


	afx_msg void OnDisplayFullscreen();
};
