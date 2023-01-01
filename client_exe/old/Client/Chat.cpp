#include "Chat.h"



#define IDD_CHAT_DLG                    101
#define ID_MSGLIST                      1000
#define ID_INPUT                        1001


CChat::CChat(DWORD dwIdentity):
CEventHandler(dwIdentity)
{
	m_hInit = NULL;
	m_hDlg = NULL;
	m_hWndThread = NULL;
	m_dwThreadId = 0;
	memset(m_szPeerName, 0, sizeof(m_szPeerName));
}


CChat::~CChat()
{
}

//假设对话框不可能由client关闭,只能由客户端断开.;


LRESULT CALLBACK CChat::DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WCHAR Msg[8192] = { 0 };
	WCHAR buffer[4096];
	HWND hInput;
	HWND hMsgList;
	LPCREATESTRUCTW pCreateStruct = NULL;
	//不能用static 变量(开启多个对话框的时候会乱套)
	CChat*pChat = (CChat*)GetWindowLong(hWnd,GWL_USERDATA);

	switch (uMsg)
	{
	case WM_CREATE:
		//
		pCreateStruct = (LPCREATESTRUCTW) lParam;
		SetWindowLong(hWnd,GWL_USERDATA,(LONG)pCreateStruct->lpCreateParams);
		break;
	case WM_CLOSE:		//ignore close msg;
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			if(pChat)
			{
				hInput= GetDlgItem(pChat->m_hDlg, ID_INPUT);
				hMsgList = GetDlgItem(pChat->m_hDlg, ID_MSGLIST);
				GetWindowTextW(hInput, buffer, 4095);
				if(lstrlenW(buffer) == 0)
					break;
				SetWindowTextW(hInput, L"");
				pChat->Send(CHAT_MSG,(char*)buffer,sizeof(WCHAR)*(lstrlenW(buffer) + 1));

				//把自己发送的内容显示到屏幕上;
				lstrcatW(Msg, L"[me]:");
				lstrcatW(Msg, buffer);
				lstrcatW(Msg, L"\r\n");
				SendMessageW(hMsgList, EM_SETSEL, -1, 0);
				SendMessageW(hMsgList, EM_REPLACESEL, FALSE, (LPARAM)Msg);
			}
			break;
		case IDCANCEL:
			break;
		}
		break;
	default:
		return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}
	return 0;
}


void CChat::ThreadProc(CChat*pChat)
{
	HINSTANCE hInstance = GetModuleHandleW(NULL);
	static bool HasRegister = false;

	if(HasRegister == false){
		WNDCLASSW wndclass = {0};
		wndclass.hInstance = hInstance;
		wndclass.lpfnWndProc = DlgProc;
		wndclass.lpszClassName = L"ChatBoxDlgClass";
		wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wndclass.hCursor = (HCURSOR)LoadCursorW(hInstance,MAKEINTRESOURCEW(IDC_ARROW));
	
		wndclass.style = CS_HREDRAW|CS_VREDRAW;	
		HasRegister = (RegisterClassW(&wndclass) != 0);
	}
	pChat->m_hDlg = NULL;

	if(HasRegister){
		RECT rect;
		pChat->m_hDlg  = CreateWindowW(L"ChatBoxDlgClass",L"Chat Box",DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
		,CW_USEDEFAULT,CW_USEDEFAULT,466,266,NULL,NULL,hInstance,(void*)pChat);
		
		GetClientRect(pChat->m_hDlg,&rect);
		//hide window
		
		//Create Button
		CreateWindowW(L"button",L"Send",WS_CHILD|WS_VISIBLE,rect.right - 60,rect.bottom - 24,60,24,pChat->m_hDlg,(HMENU)IDOK,hInstance,0);
		//Create MsgList
		CreateWindowW(L"Edit",L"",WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_READONLY | ES_AUTOHSCROLL | 
                    WS_VSCROLL,0,0,rect.right,rect.bottom - 24,pChat->m_hDlg,(HMENU)ID_MSGLIST,hInstance,NULL);
		//Create Input Box
		CreateWindowW(L"Edit",L"",WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,0,rect.bottom - 24,rect.right - 60,24,pChat->m_hDlg,(HMENU)ID_INPUT,hInstance,0);
	}
	//
	//enable(false) send button
	HWND hCtrl = GetDlgItem(pChat->m_hDlg, IDOK);
	EnableWindow(hCtrl, FALSE);

	SetEvent(pChat->m_hInit);
	MSG msg = { 0 };

	while (GetMessage(&msg, 0, 0, 0))
	{
		if (!IsDialogMessage(pChat->m_hDlg, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	//退出了.;
	DestroyWindow(pChat->m_hDlg);
	pChat->m_hDlg = NULL;
}

BOOL CChat::ChatInit()
{
	BOOL bResult = FALSE;
	m_hInit = CreateEvent(0, 0, FALSE, NULL);
	if (m_hInit == NULL)
		return FALSE;
	//创建线程;
	m_hWndThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadProc, this, 0, &m_dwThreadId);
	if (m_hWndThread == 0)
	{
		return FALSE;
	}
	//等待线程创建窗口完毕;
	WaitForSingleObject(m_hInit, INFINITE);
	CloseHandle(m_hInit);
	m_hInit = NULL;
	if (m_hDlg == NULL)				//失败.;
		return FALSE;
	//
	return TRUE;
}
void CChat::OnConnect()
{
	DWORD dwStatu = 0;
	if (ChatInit())
	{
		dwStatu = 1;
	}
	Send(CHAT_INIT, (char*)&dwStatu, sizeof(dwStatu));
}
void CChat::OnClose()
{
	//结束了.;
	if (m_dwThreadId)
	{
		//
		PostThreadMessage(m_dwThreadId, WM_QUIT, 0, 0);
		WaitForSingleObject(m_hWndThread,INFINITE);
		CloseHandle(m_hWndThread);

		m_hWndThread = NULL;
		m_dwThreadId = NULL;
	}
	if (m_hDlg)
	{
		DestroyWindow(m_hDlg);
		m_hDlg = NULL;
	}
	if (m_hInit)
	{
		CloseHandle(m_hInit);
		m_hInit = NULL;
	}
}

void CChat::OnChatBegin(DWORD dwRead, char*szBuffer)
{
	if (dwRead == 0 || !szBuffer[0])
	{
		lstrcpyW(m_szPeerName, L"Hacker");
	}
	else
	{
		lstrcpyW(m_szPeerName, (WCHAR*)szBuffer);
	}
	if (m_hDlg)
	{
		WCHAR Title[256] = {0};
		lstrcpyW(Title,L"Chating with ");
		lstrcatW(Title,m_szPeerName);
		SetWindowTextW(m_hDlg,Title);
		//Show dlg
		ShowWindow(m_hDlg, SW_SHOW);
		//enable (true) send button.
		HWND hCtrl = GetDlgItem(m_hDlg, IDOK);
		EnableWindow(hCtrl, TRUE);
	}
}

void CChat::OnChatMsg(DWORD dwRead, char*szBuffer)
{
	WCHAR Msg[8192] = { 0 };
	//显示到对话框里面;
	if (m_hDlg)
	{
		HWND hCtrl = GetDlgItem(m_hDlg, ID_MSGLIST);
		//末尾追加数据.;
		Msg[0] = '[';
		lstrcatW(Msg, m_szPeerName);
		lstrcatW(Msg, L"]:");
		lstrcatW(Msg, (WCHAR*)szBuffer);
		lstrcatW(Msg,L"\r\n");
		SendMessageW(hCtrl, EM_SETSEL, -1, 0);
		SendMessageW(hCtrl, EM_REPLACESEL, FALSE, (LPARAM)Msg);
	}
}
void CChat::OnReadComplete(WORD Event, DWORD Total, DWORD Read, char*Buffer)
{
	switch (Event)
	{
	case CHAT_BEGIN:
		OnChatBegin(Read,Buffer);
		break;
	case CHAT_MSG:
		OnChatMsg(Read, Buffer);
		break;
	default:
		break;
	}
}

void CChat::OnReadPartial(WORD Event, DWORD Total, DWORD Read, char*Buffer)
{


}