#include "stdafx.h"
#include "CmdEdit.h"
#include <imm.h>
#pragma comment(lib,"imm32")

CCmdEdit::CCmdEdit(CCmdSrv*	&pHandler) :
	m_pHandler(pHandler),
	m_ReadOnlyLength(0),
	m_LastCommand(NULL),
	m_LastLineTextLength(0)
{
}


CCmdEdit::~CCmdEdit()
{

}


BOOL CCmdEdit::PreTranslateMessage(MSG* pMsg)
{
	int start, end, left;
	// TODO:  在此添加专用代码和/或调用基类
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_BACK){
		GetSel(start, end);
		left = min(start, end);

		//等于的话代表起始光标在RecvSting 的最后一个位置
		if (left <= m_ReadOnlyLength){
			return TRUE;				//忽略
		}
	}

	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_DELETE){
		GetSel(start, end);
		left = min(start, end);
		if (left < m_ReadOnlyLength){
			return TRUE;
		}
	}

	//会有一个填充最后的结果字符串
	if ((pMsg->message == WM_IME_COMPOSITION) &&
		(pMsg->lParam & GCS_RESULTSTR)){
		GetSel(start, end);
		left = min(start, end);

		if (left < m_ReadOnlyLength){
			return TRUE;				//忽略
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

	//LARGE_INTEGER Frequency, Start, End;
	//QueryPerformanceFrequency(&Frequency);
	//QueryPerformanceCounter(&Start);

	int lastLine = GetLineCount() - 1;
	int length = LineLength(-1);

	TCHAR * lpBuffer = new TCHAR[length + 1];
	GetLine(lastLine, lpBuffer);
	lpBuffer[length] = 0;
	Cmd = lpBuffer + m_LastLineTextLength;
	delete[]lpBuffer;


	//int Length = GetWindowTextLength();
	//if (m_ReadOnlyLength < Length){
	//	SetSel(m_ReadOnlyLength, Length);
	//	Cut();
	//	if (OpenClipboard()){
	//		HGLOBAL hData = GetClipboardData(CF_UNICODETEXT);
	//		if (hData){
	//			WCHAR * lpBuffer = (WCHAR*)GlobalLock(hData);
	//			Cmd = lpBuffer;
	//			GlobalUnlock(hData);
	//		}
	//		CloseClipboard();
	//	}
	//}

	//QueryPerformanceCounter(&End);
	//double time = 1.0 * (End.QuadPart - Start.QuadPart) / Frequency.QuadPart;


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

	SetSel(m_ReadOnlyLength, -1);			//把输入的那部分也替换掉.
	const TCHAR*p = strResult;
	const TCHAR * start = p;

	CString strFinalResult;

	while (*p){
		if (p[0] == '\n' && p > start && p[-1] != '\r'){
			strFinalResult += "\r\n";
		}
		else{
			strFinalResult += *p;
		}
		p++;
	}

	ReplaceSel(strFinalResult);
	m_ReadOnlyLength += strFinalResult.GetLength();

	SetSel(m_ReadOnlyLength, m_ReadOnlyLength);			//光标移到最后.
	PostMessage(WM_VSCROLL, SB_BOTTOM);
	//
	m_LastLineTextLength = LineLength();				//Get last line text length.
}
LRESULT CCmdEdit::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO:  在此添加专用代码和/或调用基类
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

	// TODO:  在此添加您专用的创建代码
	m_Font.CreateFont(18, 9, 0, 0, FW_REGULAR, FALSE, FALSE,
		0, DEFAULT_CHARSET, DEFAULT_CHARSET,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SCRIPT, TEXT("新宋体"));

	SetFont(&m_Font);
	SetLimitText(-1);
	EnableWindow(FALSE);
	return 0;
}

