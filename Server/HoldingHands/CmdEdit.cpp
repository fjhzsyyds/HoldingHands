#include "stdafx.h"
#include "CmdEdit.h"
#include <imm.h>
#pragma comment(lib,"imm32")

CCmdEdit::CCmdEdit(CCmdSrv*	&pHandler) :
	m_pHandler(pHandler),
	m_ReadOnlyLength(0),
	m_LastCommand(NULL)
{
}


CCmdEdit::~CCmdEdit()
{

}


BOOL CCmdEdit::PreTranslateMessage(MSG* pMsg)
{
	int start, end, left;
	// TODO:  �ڴ����ר�ô����/����û���
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_BACK){
		GetSel(start, end);
		left = min(start, end);

		//���ڵĻ�������ʼ�����RecvSting �����һ��λ��
		if (left <= m_ReadOnlyLength){
			return TRUE;				//����
		}
	}

	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_DELETE){
		GetSel(start, end);
		left = min(start, end);
		if (left < m_ReadOnlyLength){
			return TRUE;
		}
	}

	//����һ��������Ľ���ַ���
	if ((pMsg->message == WM_IME_COMPOSITION) &&
		(pMsg->lParam & GCS_RESULTSTR)){
		GetSel(start, end);
		left = min(start, end);

		if (left < m_ReadOnlyLength){
			return TRUE;				//����
		}
	}

	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN){
		OnEnter();
		return TRUE;
	}

	if (pMsg->message == WM_KEYDOWN){
		if (pMsg->wParam == VK_UP)
		{
			if (m_LastCommand == NULL)
				m_LastCommand = m_Commands.GetTailPosition();

			if (m_LastCommand)
			{
				int length = GetWindowTextLength();
				SetSel(m_ReadOnlyLength, length);
				ReplaceSel(m_Commands.GetAt(m_LastCommand));
				//
				length = GetWindowTextLength();
				SetSel(length, length);

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

				int length = GetWindowTextLength();
				SetSel(m_ReadOnlyLength, length);
				ReplaceSel(m_Commands.GetAt(m_LastCommand));
				//
				length = GetWindowTextLength();
				SetSel(length, length);

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
	
	return CEdit::PreTranslateMessage(pMsg);
}

void CCmdEdit::OnEnter(){
	CStringA aCmd;
	CString Cmd;

	int Length = GetWindowTextLength();
	if (m_ReadOnlyLength < Length){
		SetSel(m_ReadOnlyLength, Length);
		Copy();
		if (OpenClipboard()){
			HGLOBAL hData = GetClipboardData(CF_UNICODETEXT);
			if (hData){
				WCHAR * lpBuffer = (WCHAR*)GlobalLock(hData);
				Cmd = lpBuffer;
				GlobalUnlock(hData);
			}
			CloseClipboard();
		}
	}
	if (Cmd == TEXT("cls") || Cmd == TEXT("clear")){
		m_ReadOnlyLength = 0;
		SetWindowText(TEXT(""));
		Cmd = TEXT("");
	}

	if (Cmd.GetLength()){
		m_Commands.AddTail(Cmd);
		m_LastCommand = NULL;
	}

	aCmd = CW2A(Cmd);
	aCmd += "\r\n";
	if (m_pHandler)
		m_pHandler->SendMsg(CMD_COMMAND, aCmd.GetBuffer(), aCmd.GetLength() + 1);
}

void CCmdEdit::OnCmdBegin(){
	EnableWindow(TRUE);
}

void CCmdEdit::OnCmdResult(const CString &strResult){

	SetSel(m_ReadOnlyLength, -1);			//��������ǲ���Ҳ�滻��.
	const TCHAR*p = strResult;
	const TCHAR * start = p;

	CString strFinalResult;

	while (*p){
		if (p[0] == '\n' && p > start && p[-1] != '\r'){
			strFinalResult += "\r\n";
			p += 2;
		}
		else{
			strFinalResult += *p;
			p++;
		}
	}
	//Sel ��ʱ���ǲ��� \n��..ֻ����һ�����з�.
	ReplaceSel(strFinalResult);
	m_ReadOnlyLength += strFinalResult.GetLength();
	SetFocus();
	SetSel(m_ReadOnlyLength, m_ReadOnlyLength);
	
}
LRESULT CCmdEdit::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO:  �ڴ����ר�ô����/����û���
	int start, end, left;
	if (message == EM_REPLACESEL){
		GetSel(start, end);
		left = min(start, end);
		if (left <m_ReadOnlyLength){
			return 0;
		}
	}
	else if (message == WM_CHAR){
		GetSel(start, end);
		left = min(start, end);

		if (left < m_ReadOnlyLength){
			return 0;
		}
	}
	else if (message == WM_PASTE || message == WM_CUT){

		GetSel(start, end);
		left = min(start, end);

		if (left < m_ReadOnlyLength){
			return 0;
		}
	}
	return CEdit::WindowProc(message, wParam, lParam);
}
BEGIN_MESSAGE_MAP(CCmdEdit, CEdit)
	ON_WM_CREATE()
END_MESSAGE_MAP()


int CCmdEdit::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CEdit::OnCreate(lpCreateStruct) == -1)
		return -1;

	CDC *pDC = GetDC();

	// TODO:  �ڴ������ר�õĴ�������
	m_Font.CreateFont(18, 9, 0, 0, FW_REGULAR, FALSE, FALSE,
		0, DEFAULT_CHARSET, DEFAULT_CHARSET,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SCRIPT, TEXT("������"));

	SetFont(&m_Font);
	SetLimitText(-1);
	EnableWindow(FALSE);
	return 0;
}

