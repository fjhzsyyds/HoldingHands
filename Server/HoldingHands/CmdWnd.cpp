#include "stdafx.h"
#include "CmdWnd.h"
#include "CmdSrv.h"

CCmdWnd::CCmdWnd(CCmdSrv*pHandler) :
	m_DestroyAfterDisconnect(FALSE),
	m_pHandler(pHandler),
	m_CmdShow(m_pHandler)
{
}


CCmdWnd::~CCmdWnd()
{
}

BEGIN_MESSAGE_MAP(CCmdWnd, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_MESSAGE(WM_CMD_ERROR,OnError)
	ON_MESSAGE(WM_CMD_RESULT,OnCmdResult)
	ON_MESSAGE(WM_CMD_BEGIN,OnCmdBegin)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

#define ID_CMD_INPUT		15234
#define ID_CMD_RESULT		15235

int CCmdWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  �ڴ������ר�õĴ�������
	RECT rect;
	GetClientRect(&rect);

	m_BkBrush.CreateSolidBrush(RGB(12, 12, 12));
	//������ʾ��
	m_CmdShow.Create(WS_CHILD | WS_VISIBLE | ES_MULTILINE 
		| ES_AUTOHSCROLL | WS_VSCROLL|WS_HSCROLL,
		rect, this, ID_CMD_RESULT);
	//���ô��ڱ���
	CString Title;
	auto const peer = m_pHandler->GetPeerName();
	Title.Format(L"[%s] cmd ", CA2W(peer.first.c_str()).m_psz);
	SetWindowText(Title);
	return 0;
}


LRESULT CCmdWnd::OnError(WPARAM wParam, LPARAM lParam){
	TCHAR*szError = (TCHAR*)wParam;
	MessageBox(szError, TEXT("Error"), MB_OK | MB_ICONINFORMATION);
	return 0;
}
void CCmdWnd::PostNcDestroy()
{
	if (!m_DestroyAfterDisconnect){
		delete this;
	}
}



void CCmdWnd::OnClose()
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (m_pHandler){
		m_DestroyAfterDisconnect = TRUE;
		m_pHandler->Close();
	}
	else{
		//m_pHandler�Ѿ�û��,����ֻ���Լ�����.
		DestroyWindow();
	}
}


void CCmdWnd::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);

	// TODO:  �ڴ˴������Ϣ����������
	RECT rect;
	GetClientRect(&rect);
	m_CmdShow.MoveWindow(&rect);
}

LRESULT CCmdWnd::OnCmdBegin(WPARAM wParam, LPARAM lParam){
	m_CmdShow.OnCmdBegin();
	return 0;
}

LRESULT CCmdWnd::OnCmdResult(WPARAM wParam, LPARAM lParam)
{
	char*szBuffer = (char*)wParam;
	CString strResult = CA2W(szBuffer);
	m_CmdShow.OnCmdResult(strResult);
	return 0;
}

BOOL CCmdWnd::PreTranslateMessage(MSG* pMsg)
{
	return CFrameWnd::PreTranslateMessage(pMsg);
}



HBRUSH CCmdWnd::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CFrameWnd::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  ���Ĭ�ϵĲ������軭�ʣ��򷵻���һ������
	if (nCtlColor == CTLCOLOR_EDIT){
		pDC->SetBkColor(RGB(12, 12, 12));
		pDC->SetTextColor(RGB(204,204,204));
	}
	return m_BkBrush;
}
