#include "stdafx.h"
#include "CmdSrv.h"
#include "CmdWnd.h"
#include "resource.h"
CCmdSrv::CCmdSrv(DWORD dwIdentity) :
CEventHandler(dwIdentity)
{
	m_pWnd = NULL;
}


CCmdSrv::~CCmdSrv()
{
}

void CCmdSrv::OnClose()
{
	if (m_pWnd){
		m_pWnd->SendMessage(WM_CLOSE);
		m_pWnd->DestroyWindow();

		m_pWnd = NULL;
	}
}
void CCmdSrv::OnConnect()
{
	m_pWnd = new CCmdWnd(this);

	if (FALSE == m_pWnd->Create(NULL, L"Cmd", WS_OVERLAPPEDWINDOW)){
		Disconnect();
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
void CCmdSrv::OnReadPartial(WORD Event, DWORD Total, DWORD nRead, char*Buffer)
{

}
//有数据发送完毕后调用这两个函数
void CCmdSrv::OnWritePartial(WORD Event, DWORD Total, DWORD nWrite, char*Buffer)
{

}
void CCmdSrv::OnWriteComplete(WORD Event, DWORD Total, DWORD nWrite, char*Buffer)
{

}

void CCmdSrv::OnReadComplete(WORD Event, DWORD Total, DWORD nRead, char*Buffer)
{
	switch (Event)
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
