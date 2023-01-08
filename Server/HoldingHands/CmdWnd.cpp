#include "stdafx.h"
#include "CmdWnd.h"
#include "CmdSrv.h"

CCmdWnd::CCmdWnd(CCmdSrv*pHandler):
m_DestroyAfterDisconnect(FALSE),
m_pHandler(pHandler),
m_LastCommand(NULL)
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
	//��������
	m_Font.CreateFontW(18, 8, 0, 0, FW_THIN, FALSE, FALSE, 0, DEFAULT_CHARSET, DEFAULT_CHARSET,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SCRIPT, L"consolas");
	
	//������ʾ��
	rect.bottom -= 28;
	m_CmdShow.Create(WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOHSCROLL|ES_READONLY | WS_VSCROLL,
		rect, this, ID_CMD_RESULT);

	m_CmdShow.SetFont(&m_Font);
	m_CmdShow.SetLimitText(0);

	//�������������.
	rect.bottom += 28;
	rect.top = rect.bottom - 28;
	
	m_Cmd.Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_WANTRETURN | WS_BORDER, 
		rect, this, ID_CMD_INPUT);
	m_Cmd.SetFont(&m_Font);

	m_Cmd.EnableWindow(FALSE);//�ȶԷ�׼�������ڿ�ʼ
	//
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
	//
	rect.bottom -= 28;
	m_CmdShow.MoveWindow(&rect);
	rect.bottom += 28;
	rect.top = rect.bottom - 28;
	m_Cmd.MoveWindow(&rect);
}

LRESULT CCmdWnd::OnCmdResult(WPARAM wParam, LPARAM lParam)
{
	char*szBuffer = (char*)wParam;
	DWORD dwLenght = m_CmdShow.GetWindowTextLengthW();
	m_CmdShow.SetSel(dwLenght, dwLenght,0);
	m_CmdShow.ReplaceSel(CA2W(szBuffer));
	return 0;
}

BOOL CCmdWnd::PreTranslateMessage(MSG* pMsg)
{
	// TODO:  �ڴ����ר�ô����/����û���
	if (pMsg->message == WM_KEYDOWN)
	{
		CWnd*pWnd = GetFocus();
		if (pWnd && pWnd->GetDlgCtrlID() == ID_CMD_INPUT)
		{
			if (pMsg->wParam == VK_RETURN)
			{
				//���»س�����
				CString Cmd;
				CStringA aCmd;
				m_Cmd.GetWindowText(Cmd);		//
				m_Cmd.SetWindowTextW(L"");		//�������.
				//����
				if (Cmd == L"cls"){
					m_CmdShow.SetWindowTextW(L"");
					//buffer[0] = 0;
					Cmd = L"";
				}
				//��������
				aCmd = (CW2A(Cmd));
				aCmd += "\r\n";
				if (m_pHandler)
					m_pHandler->SendMsg(CMD_COMMAND, aCmd.GetBuffer(), aCmd.GetLength() + 1);
				if (Cmd.GetLength())
				{
					//��¼һ������.
					m_Commands.AddTail(Cmd);
					m_LastCommand = NULL;
				}
				return TRUE;
			}
			if (pMsg->wParam == VK_UP)
			{
				if (m_LastCommand == NULL)
					m_LastCommand = m_Commands.GetTailPosition();

				if (m_LastCommand)
				{
					m_Cmd.SetWindowTextW(m_Commands.GetAt(m_LastCommand));
					m_Cmd.SetSel(m_Commands.GetAt(m_LastCommand).GetLength(),
						-1);

					if (m_LastCommand == m_Commands.GetHeadPosition()){
						m_LastCommand = m_Commands.GetTailPosition();
					}
					else{
						m_Commands.GetPrev(m_LastCommand);
					}
					
				}
				return TRUE;
			}
			if (pMsg->wParam == VK_DOWN)
			{
				if (m_LastCommand == NULL)
					m_LastCommand = m_Commands.GetTailPosition();

				if (m_LastCommand){
					m_Cmd.SetWindowTextW(m_Commands.GetAt(m_LastCommand));
					m_Cmd.SetSel(m_Commands.GetAt(m_LastCommand).GetLength(),
						-1);

					if (m_LastCommand == m_Commands.GetTailPosition()){
						m_LastCommand = m_Commands.GetHeadPosition();
					}
					else{
						m_Commands.GetNext(m_LastCommand);
					}
				}
				return TRUE;
			}
		}
	}
	return CFrameWnd::PreTranslateMessage(pMsg);
}


