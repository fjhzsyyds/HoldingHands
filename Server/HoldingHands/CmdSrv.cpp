#include "stdafx.h"
#include "CmdSrv.h"
#include "CmdWnd.h"
#include "resource.h"
CCmdSrv::CCmdSrv(CManager*pManager):
	CMsgHandler(pManager)
{
	m_pWnd = NULL;
}


CCmdSrv::~CCmdSrv()
{
}

void CCmdSrv::OnClose()
{
	if (m_pWnd){
		if (m_pWnd->m_DestroyAfterDisconnect){
			//窗口先关闭的.
			m_pWnd->DestroyWindow();
			delete m_pWnd;
		}
		else{
			// pHandler先关闭的,那么就不管窗口了
			m_pWnd->m_pHandler = nullptr;
			m_pWnd->PostMessage(WM_CMD_ERROR, (WPARAM)TEXT("Disconnect."));
			m_pWnd = nullptr;
		}
	}
}
void CCmdSrv::OnOpen(){
	m_pWnd = new CCmdWnd(this);

	if (FALSE == m_pWnd->Create(NULL, L"Cmd", WS_OVERLAPPEDWINDOW)){
		Close();
		return;
	}
	RECT rect;
	m_pWnd->GetWindowRect(&rect);
	rect.right = rect.left + 860;
	rect.bottom = rect.top + 540;
	m_pWnd->MoveWindow(&rect);

	m_pWnd->ShowWindow(SW_SHOW);
	m_pWnd->UpdateWindow();
}

void CCmdSrv::OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer)
{

}

void CCmdSrv::OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer)
{
	switch (Msg)
	{
	case CMD_BEGIN:
		OnCmdBegin(*((DWORD*)Buffer));
		break;
	case CMD_RESULT:
		OnCmdResult(Buffer);
		break;
	default:
		break;
	}
}

void CCmdSrv::OnCmdResult(char*szBuffer)
{
	m_pWnd->SendMessage(WM_CMD_RESULT, (WPARAM)szBuffer, 0);
}
void CCmdSrv::OnCmdBegin(DWORD dwStatu)
{
	//启用输入.
	if (dwStatu == 0){
		m_pWnd->m_Cmd.EnableWindow(TRUE);
		m_pWnd->m_Cmd.SetFocus();
	}
}
