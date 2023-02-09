#include "RemoteDesktop.h"
#include "IOCPClient.h"

#include <stdint.h>
#include"x264.h"


#include <WinUser.h>

#define THREAD_MSG_NEXT_FRAME	(WM_USER + 123)
#define THREAD_MSG_GET_BMP_FILE	(WM_USER + 124)

#define FRAME_QUEUE_SIZE		3


volatile unsigned int CRemoteDesktop::nInstance = 0;

CRemoteDesktop::CRemoteDesktop(CIOCPClient *pClient) :
CMsgHandler(pClient, REMOTEDESKTOP)
{
	//m_dwLastTime = 0;
	
	m_dwFrameSize = 0;
	m_FrameBuffer = 0;
	
	m_dwMaxFps = 30;
	m_Quality = QUALITY_LOW;


	m_hClipbdListenWnd = NULL;
	m_ClipbdListenerThread = NULL;
	
	m_dwCaptureFlags &= 0;

	m_hWorkThread = NULL;
	m_dwWorkThreadId = 0;
}


CRemoteDesktop::~CRemoteDesktop()
{
}


void CRemoteDesktop::OnOpen()
{
	if (InterlockedExchangeAdd(&nInstance, 1) > 0){
		//已经有一个实例了.
		TCHAR szError[] = TEXT("One instance is already running");
		SendMsg(REMOTEDESKTOP_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
		SendMsg(-1, NULL, 0);
		Close();
		return;
	}

	//剪切板监视.
	m_ClipbdListenerThread = CreateThread(0, 0,
		(LPTHREAD_START_ROUTINE)ClipdListenProc, this, 0, 0);

}

void CRemoteDesktop::OnClose()
{
	//关闭监听剪切板.
	if (m_hClipbdListenWnd){
		PostMessage(m_hClipbdListenWnd, WM_CLOSE, 0, 0);
		m_hClipbdListenWnd = NULL;
	}

	//等待线程退出
	if (m_ClipbdListenerThread){
		WaitForSingleObject(m_ClipbdListenerThread, INFINITE);
		CloseHandle(m_ClipbdListenerThread);
		m_ClipbdListenerThread = NULL;
	}

	TermRD();

	InterlockedDecrement(&nInstance);
}




#define WM_REMOTE_DESKTOP_SET_CLIPBOARD_TEXT	(WM_USER + 72)

LRESULT CALLBACK CRemoteDesktop::WndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	MyData*pMyData = (MyData*)GetWindowLongPtr(hWnd,GWL_USERDATA);
	CREATESTRUCT*pCreateStruct = NULL;
	HWND hRemovedWnd = NULL;
	HWND hAfterWnd = NULL;

	switch(uMsg)
	{	
	case WM_CREATE:
		pCreateStruct = (CREATESTRUCT*)lParam;
		//设置UserData,
		SetWindowLong(hWnd,GWL_USERDATA,(LONG)pCreateStruct->lpCreateParams);
		pMyData = (MyData*)GetWindowLongPtr(hWnd,GWL_USERDATA);
		pMyData->m_hNextViewer = SetClipboardViewer(hWnd);
		break;
	case WM_CLOSE:
		//从Viewer chain中摘下
		ChangeClipboardChain(hWnd,pMyData->m_hNextViewer);
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_CHANGECBCHAIN:
		//从新调整hNextViewer
		hRemovedWnd = (HWND)wParam;
		hAfterWnd = (HWND)lParam;
		if(hRemovedWnd == pMyData->m_hNextViewer){
			pMyData->m_hNextViewer = hAfterWnd;
		}else if(pMyData->m_hNextViewer){
			::SendMessage(pMyData->m_hNextViewer,WM_CHANGECBCHAIN,wParam,lParam);
		}
		break;
	case WM_DRAWCLIPBOARD:
		//剪切板数据改变了:
		if(OpenClipboard(hWnd)){
			HGLOBAL hData = NULL;
			hData = GetClipboardData(CF_TEXT);
			if(hData){
				char*pBuff = (char*)GlobalLock(hData);
				if(pBuff && (pMyData->m_SetClipbdText == NULL || strcmp(pBuff,pMyData->m_SetClipbdText))){
					//printf("-----------Local ClipboardData Change!------------\n");
					pMyData->m_pThis->SetClipbdText(pBuff);
				}
				GlobalUnlock(hData);
			}
			CloseClipboard();
		}
		if(pMyData->m_hNextViewer){
			SendMessage(pMyData->m_hNextViewer,WM_DRAWCLIPBOARD,wParam,lParam);
		}
		break;

	case WM_REMOTE_DESKTOP_SET_CLIPBOARD_TEXT:
		if(OpenClipboard(hWnd)){
			HGLOBAL hData = NULL;
			int len = 0;
			//备份数据	
			if(pMyData->m_SetClipbdText){
				free(pMyData->m_SetClipbdText);
				pMyData->m_SetClipbdText = NULL;
			}
			len = lstrlenA((char*)wParam) + 1;
			pMyData->m_SetClipbdText = (char*)malloc(len);
			memcpy(pMyData->m_SetClipbdText,(char*)wParam,len);
			//
			EmptyClipboard();
			hData = GlobalAlloc(GHND|GMEM_SHARE,len);
			if(hData){
				char*pBuff = (char*)GlobalLock(hData);
				memcpy(pBuff,pMyData->m_SetClipbdText,len);
				GlobalUnlock(hData);
				//printf("-----------Reset Local ClipboardData !------------\n");
				SetClipboardData(CF_TEXT,hData);
			}
			CloseClipboard();
		}
		break;
	default:
		return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}
	return 0;
}
void CRemoteDesktop::ClipdListenProc(CRemoteDesktop*pThis)
{
	static int HasRegistered = 0;
	MSG msg = {0};
	HINSTANCE hInstance = GetModuleHandle(0);
	HWND hWnd = NULL;

	MyData*pMyData = (MyData*)malloc(sizeof(MyData));
	pMyData->m_pThis = pThis;
	pMyData->m_hNextViewer = NULL;
	pMyData->m_SetClipbdText = NULL;

	//注册窗口类
	if(HasRegistered == 0){
		WNDCLASSW wndclass = {0};
		wndclass.hInstance = hInstance;
		wndclass.lpszClassName = L"ClipbdListener";
		wndclass.lpfnWndProc = WndProc;
		wndclass.style = CS_HREDRAW|CS_VREDRAW;
		HasRegistered = (0 != RegisterClassW(&wndclass));
	}

	if(HasRegistered == 0){
		char szError[256] = {0};
		sprintf(szError,"Register Class Failed!:%d",GetLastError());
		return;
	}

	hWnd = CreateWindowW(L"ClipbdListener",L"",WS_OVERLAPPEDWINDOW,0,0,0,0,0,0,hInstance,pMyData);

	if(NULL == hWnd){
		char szError[256] = {0};
		sprintf(szError,"Create Window Failed!:%d",GetLastError());
		return;
	}

	ShowWindow(hWnd,SW_HIDE);
	pThis->m_hClipbdListenWnd = hWnd;

	while(GetMessage(&msg,0,0,0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (pMyData->m_SetClipbdText){
		free(pMyData->m_SetClipbdText);
		pMyData->m_SetClipbdText = NULL;
	}

	free(pMyData);
	pMyData = NULL;
}
void CRemoteDesktop::OnReadMsg(WORD Msg,DWORD dwSize ,char*Buffer)
{
	switch (Msg)
	{
	case REMOTEDESKTOP_NEXT_FRAME:
		OnNextFrame();
		break;
	case REMOTEDESKTOP_INIT_RDP:
		OnInitRD(((DWORD*)Buffer)[0], ((DWORD*)Buffer)[1]);
		break;
	case REMOTEDESKTOP_CTRL:
		OnControl((CtrlParam*)Buffer);
		break;
	case REMOTEDESKTOP_SET_CLIPBOARDTEXT:
		OnSetClipbdText(Buffer);
		break;
	case REMOTEDESKTOP_SETFLAG:
		OnSetFlag(*(DWORD*)Buffer);
		break;
	case REMOTEDESKTOP_GET_BMP_FILE:
		OnScreenShot();
		break;
	default:
		break;
	}
}

void CRemoteDesktop::OnSetClipbdText(char*szText)
{
	if(m_hClipbdListenWnd){
		SendMessage(m_hClipbdListenWnd,WM_REMOTE_DESKTOP_SET_CLIPBOARD_TEXT,
			(WPARAM)szText,(LPARAM)0);
	}
}
void CRemoteDesktop::SetClipbdText(char*szText)
{
	SendMsg(REMOTEDESKTOP_SET_CLIPBOARDTEXT,szText,lstrlenA(szText)+1);
}

void CRemoteDesktop::OnSetFlag(DWORD dwFlag){
	if (dwFlag & 0x80000000){		//Set Flag
		m_dwCaptureFlags |= (dwFlag & 0x7fffffff);
	}
	else{
		//Cancel Flag
		m_dwCaptureFlags &= (~dwFlag);
	}
}

void CRemoteDesktop::TermRD(){
	////线程 Desktop Grab,虽然能提高fps,但是延迟太难受了
	if (m_hWorkThread){
		while (!PostThreadMessage(m_dwWorkThreadId, WM_QUIT, 0, 0)){
			Sleep(1);
		}
		WaitForSingleObject(m_hWorkThread, INFINITE);
		CloseHandle(m_hWorkThread);
		m_hWorkThread = NULL;
		m_dwWorkThreadId = 0;
	}
	//必须先等待线程退出.,因为有GetFrame...
	m_grab.GrabTerm();
}

void CRemoteDesktop::OnInitRD(DWORD dwFps, DWORD dwQuality)
{
	TCHAR szError[] = L"desktop grab init failed!";
	DWORD buff[2];
	m_dwMaxFps = dwFps;
	m_Quality = dwQuality;

	TermRD();

	if (!m_grab.GrabInit(m_dwMaxFps,m_Quality)){
		SendMsg(REMOTEDESKTOP_ERROR, (char*)szError, (sizeof(TCHAR) * (lstrlen(szError) + 1)));
		Close();
		return;
	}
	//Send Video Size
	m_grab.GetDesktopSize(buff, buff + 1);
	SendMsg(REMOTEDESKTOP_DESKSIZE, (char*)buff, sizeof(DWORD) * 2);

	m_hWorkThread = CreateThread(0, 0,
		(LPTHREAD_START_ROUTINE)DesktopGrabThread, this,
		0, &m_dwWorkThreadId);

	for (int i = 0; i < FRAME_QUEUE_SIZE; i++){
		while (!PostThreadMessage(m_dwWorkThreadId, THREAD_MSG_NEXT_FRAME, 0, 0));
	}
}


void CALLBACK CRemoteDesktop::DesktopGrabThread(CRemoteDesktop*pThis){
	//成功,发送视频大小.
	TCHAR szError[] = L"desktop grab failed!";

	MSG msg;
	LARGE_INTEGER liFrequency,liLastSendTime;
	double Elapse = 0.0, ElaspePerFrame = 0.0;

	QueryPerformanceFrequency(&liFrequency);
	QueryPerformanceCounter(&liLastSendTime);

	while (GetMessage(&msg,0,0,0)){
		if (msg.message == THREAD_MSG_GET_BMP_FILE){
			char * buffer = 0;
			DWORD dwSize = 0;

			pThis->m_grab.GetBmpFile(&buffer, &dwSize);
			pThis->SendMsg(REMOTEDESKTOP_BMP_FILE, buffer, dwSize);
			delete[]buffer;
		}
		else if (msg.message == THREAD_MSG_NEXT_FRAME){
			if (!pThis->m_grab.GetFrame(&pThis->m_FrameBuffer, &pThis->m_dwFrameSize,
				pThis->m_dwCaptureFlags)){

				pThis->SendMsg(REMOTEDESKTOP_ERROR, szError,
					(sizeof(TCHAR) * (lstrlen(szError) + 1)));
				pThis->Close();
			}
			else{
				LARGE_INTEGER liCurTime;
				do{
					QueryPerformanceCounter(&liCurTime);
					if (liCurTime.QuadPart < liLastSendTime.QuadPart){
						break;
					}
					Elapse = 1.0 * (liCurTime.QuadPart - liLastSendTime.QuadPart) / liFrequency.QuadPart
						* 1000.0;
					ElaspePerFrame = 1000.0 / pThis->m_dwMaxFps;			//毫秒.
					__asm{
						pause;
					}
				} while (Elapse < ElaspePerFrame + 1);

				QueryPerformanceCounter(&liLastSendTime);
				//Send Frame
				pThis->SendMsg(REMOTEDESKTOP_FRAME, pThis->m_FrameBuffer, pThis->m_dwFrameSize,FALSE);//没必要压缩.
			}
		}
	}
}

void CRemoteDesktop::OnNextFrame()
{
	PostThreadMessage(m_dwWorkThreadId, THREAD_MSG_NEXT_FRAME, 0, 0);
}

void CRemoteDesktop::OnScreenShot(){
	PostThreadMessage(m_dwWorkThreadId, THREAD_MSG_GET_BMP_FILE, 0, 0);
}

void CRemoteDesktop::OnControl(CtrlParam*pParam)
{
	switch (pParam->dwType)
	{
	case WM_KEYDOWN:
		keybd_event(pParam->Param.VkCode, 0, 0, 0);
		break;
	case WM_KEYUP:
		keybd_event(pParam->Param.VkCode, 0, KEYEVENTF_KEYUP, 0);
		break;
		//鼠标移动
	case WM_MOUSEMOVE:
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, LOWORD(pParam->Param.dwCoor), 
			HIWORD(pParam->Param.dwCoor), 0, 0);
		break;
		//左键操作
	case WM_LBUTTONDOWN:
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, LOWORD(pParam->Param.dwCoor), 
			HIWORD(pParam->Param.dwCoor), 0, 0);
		break;
	case WM_LBUTTONUP:
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP, LOWORD(pParam->Param.dwCoor),
			HIWORD(pParam->Param.dwCoor), 0, 0);
		break;
	case WM_LBUTTONDBLCLK:
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN|MOUSEEVENTF_LEFTUP,
			LOWORD(pParam->Param.dwCoor), HIWORD(pParam->Param.dwCoor), 0, 0);
		Sleep(100);
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 
			LOWORD(pParam->Param.dwCoor), HIWORD(pParam->Param.dwCoor), 0, 0);
		break;
		//右键操作
	case WM_RBUTTONDOWN:
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTDOWN, LOWORD(pParam->Param.dwCoor),
			HIWORD(pParam->Param.dwCoor), 0, 0);
		break;
	case WM_RBUTTONUP:
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTUP, LOWORD(pParam->Param.dwCoor), 
			HIWORD(pParam->Param.dwCoor), 0, 0);
		break;
	case WM_RBUTTONDBLCLK:
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTDOWN|MOUSEEVENTF_RIGHTUP, 
			LOWORD(pParam->Param.dwCoor), HIWORD(pParam->Param.dwCoor), 0, 0);
		Sleep(100);
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP,
			LOWORD(pParam->Param.dwCoor), HIWORD(pParam->Param.dwCoor), 0, 0);
		break;
		//中键操作
	case WM_MBUTTONDOWN:
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MIDDLEDOWN,0, 0, 0, 0);
		break;
	case WM_MBUTTONUP:
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MIDDLEUP, 0, 0, 0, 0);
		break;
	case WM_MBUTTONDBLCLK:
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MIDDLEDOWN|MOUSEEVENTF_MIDDLEUP, 0, 0, 0, 0);
		Sleep(100);
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MIDDLEDOWN|MOUSEEVENTF_MIDDLEUP, 0, 0, 0, 0);
		break;
		//
	case WM_MOUSEWHEEL:		//mouse wheel
		mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_WHEEL, 0, 0, pParam->dwExtraData, 0);
		break;
	default:
		break;
	}
}
