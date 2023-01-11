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
	if (dlg.m_NickName.GetLength() != 0){
		lstrcpy(m_szNickName,dlg.m_NickName);
	}
	m_pDlg = new CChatDlg(this);
	ASSERT(m_pDlg->Create(IDD_CHAT_DLG, CWnd::GetDesktopWindow()));

	//禁用SendMsg button.
	m_pDlg->GetDlgItem(IDOK)->EnableWindow(FALSE);
	m_pDlg->ShowWindow(SW_SHOW);
	//
}

void CChatSrv::OnClose()
{
	if (m_pDlg){
		if (m_pDlg->m_DestroyAfterDisconnect){
			//窗口先关闭的.
			m_pDlg->DestroyWindow();
			delete m_pDlg;
		}
		else{
			// pHandler先关闭的,那么就不管窗口了
			m_pDlg->m_pHandler = nullptr;
			m_pDlg->PostMessage(WM_CHATDLG_ERROR, (WPARAM)TEXT("Disconnect."));
		}
		m_pDlg = nullptr;
	}
}



void CChatSrv::OnChatInit(DWORD dwRead, char*szBuffer)
{
	DWORD dwStatu = *(DWORD*)szBuffer;
	if (dwStatu)
	{
		//开始聊天
		SendMsg(CHAT_BEGIN, (char*)m_szNickName, sizeof(TCHAR)*((lstrlen(m_szNickName)+1)));
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

void CChatSrv::SendMsgText(TCHAR*szMsg)
{
	SendMsg(CHAT_MSG, (char*)szMsg, sizeof(TCHAR)*(lstrlen(szMsg) + 1));
}