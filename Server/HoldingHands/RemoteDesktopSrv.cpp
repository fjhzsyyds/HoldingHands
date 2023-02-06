#include "stdafx.h"
#include "RemoteDesktopSrv.h"
#include "RemoteDesktopWnd.h"

#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"yuv.lib")

//#pragma comment(lib,"avformat.lib")

CRemoteDesktopSrv::CRemoteDesktopSrv(CClientContext*pClient) :
	CMsgHandler(pClient),
	m_pCodec(NULL),
	m_pCodecContext(NULL),
	m_hBmp(NULL),
	m_hMemDC(NULL),
	m_pWnd(NULL)
{
	memset(&m_AVPacket, 0, sizeof(AVPacket));
	memset(&m_AVFrame, 0, sizeof(AVFrame));
	memset(&m_Bmp, 0, sizeof(m_Bmp));

	m_hMutex = CreateEvent(0, TRUE, TRUE, NULL);
}


CRemoteDesktopSrv::~CRemoteDesktopSrv()
{
	CloseHandle(m_hMutex);
}


BOOL CRemoteDesktopSrv::RemoteDesktopSrvInit(DWORD dwWidth, DWORD dwHeight)
{
	BITMAPINFO bmi = { 0 };
	CDC*pDc = m_pWnd->GetDC();
	//创建内存dc
	m_hMemDC = CreateCompatibleDC(pDc->GetSafeHdc());
	if (m_hMemDC == NULL)
		return FALSE;
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = dwWidth;
	bmi.bmiHeader.biHeight = dwHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;				//4字节貌似解码快
	bmi.bmiHeader.biCompression = BI_RGB;
	
	//创建DIBSection
	m_hBmp = CreateDIBSection(m_hMemDC, &bmi, DIB_RGB_COLORS, &m_Buffer, 0, 0);
	if (m_hBmp == NULL || !SelectObject(m_hMemDC, m_hBmp))
		return FALSE;
	//
	GetObject(m_hBmp, sizeof(BITMAP), &m_Bmp);
	//
	//创建解码器.
	m_pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (m_pCodec == NULL)
		return FALSE;
	//
	m_pCodecContext = avcodec_alloc_context3(m_pCodec);
	if (m_pCodecContext == NULL)
		return FALSE;
	//
	if (0 != avcodec_open2(m_pCodecContext, m_pCodec, 0))
		return FALSE;
	return TRUE;
}

void CRemoteDesktopSrv::RemoteDesktopSrvTerm()
{
	if (m_hMemDC){
		DeleteDC(m_hMemDC);
		m_hMemDC = NULL;
	}
	if (m_hBmp){
		DeleteObject(m_hBmp);
		memset(&m_Bmp, 0, sizeof(m_Bmp));
		m_hBmp = NULL;
	}

	if (m_pCodecContext){
		avcodec_free_context(&m_pCodecContext);
		m_pCodecContext = 0;
	}
	m_pCodec = 0;
	//AVFrame需要清除
	av_frame_unref(&m_AVFrame);
	//
	memset(&m_AVPacket, 0, sizeof(m_AVPacket));
	memset(&m_AVFrame, 0, sizeof(m_AVFrame));

}


void CRemoteDesktopSrv::NextFrame()
{
	SendMsg(REMOTEDESKTOP_NEXT_FRAME,NULL, 0);
}

void CRemoteDesktopSrv::ScreenShot(){
	SendMsg(REMOTEDESKTOP_GET_BMP_FILE, 0, 0);
}


void CRemoteDesktopSrv::OnOpen()
{
	m_pWnd = new CRemoteDesktopWnd(this);
	ASSERT(m_pWnd->Create(NULL, NULL));

	m_pWnd->ShowWindow(SW_SHOW);
}


void CRemoteDesktopSrv::OnClose()
{
	//
	RemoteDesktopSrvTerm();
	if (m_pWnd){
		if (m_pWnd->m_DestroyAfterDisconnect){
			//窗口先关闭的.
			m_pWnd->DestroyWindow();
			delete m_pWnd;
		}
		else{
			// pHandler先关闭的,那么就不管窗口了
			m_pWnd->m_pHandler = nullptr;
			m_pWnd->PostMessage(WM_REMOTE_DESKTOP_ERROR, (WPARAM)TEXT("Disconnect."));
		}
		m_pWnd = nullptr;
	}
}

void CRemoteDesktopSrv::OnBmpFile(char * Buffer, DWORD dwSize){

	m_pWnd->SendMessage(WM_REMOTE_DESKTOP_SCREENSHOT, (WPARAM)Buffer, dwSize);
}


void CRemoteDesktopSrv::OnDeskSize(char*DeskSize)
{
	DWORD dwWidth = ((DWORD*)(DeskSize))[0];
	DWORD dwHeight = ((DWORD*)(DeskSize))[1];

	/*************************************************************/
	RemoteDesktopSrvTerm();

	if (!RemoteDesktopSrvInit(dwWidth, dwHeight)){
		m_pWnd->SendMessage(WM_REMOTE_DESKTOP_ERROR, 
			(WPARAM)L"RemoteDesktopSrvInit Error", 0);
		Close();
		return;
	}

	NextFrame();
	m_pWnd->SendMessage(WM_REMOTE_DESKTOP_SIZE, m_Bmp.bmWidth, m_Bmp.bmHeight);
}

void CRemoteDesktopSrv::OnError(TCHAR*szError)
{
	//桌面采集失败.
	m_pWnd->SendMessage(WM_REMOTE_DESKTOP_ERROR, (WPARAM)szError, 0);
}


void CRemoteDesktopSrv::OnFrame(DWORD dwRead, char*Buffer)
{
	if (m_pCodecContext == NULL){
		return;
	}
	//解码数据.
	av_init_packet(&m_AVPacket);
	//
	m_AVPacket.data = (uint8_t*)Buffer;
	m_AVPacket.size = dwRead;

	////获取下一帧
	NextFrame();

	if (!avcodec_send_packet(m_pCodecContext, &m_AVPacket)){
		//解码数据前会清除m_AVFrame的内容.
		if (!avcodec_receive_frame(m_pCodecContext, &m_AVFrame)){
			//成功.
			//I420 ---> ARGB.
			WaitForSingleObject(m_hMutex,INFINITE);
			libyuv::I420ToARGB(m_AVFrame.data[0], m_AVFrame.linesize[0],
				m_AVFrame.data[1], m_AVFrame.linesize[1],
				m_AVFrame.data[2], m_AVFrame.linesize[2],
				(uint8_t*)m_Buffer, m_Bmp.bmWidthBytes,m_Bmp.bmWidth, m_Bmp.bmHeight);
			
			//显示到窗口上
			m_pWnd->SendMessage(WM_REMOTE_DESKTOP_DRAW, (WPARAM)m_hMemDC, (LPARAM)&m_Bmp);
			return;
		}
	}
	//失败
	m_pWnd->SendMessage(WM_REMOTE_DESKTOP_ERROR, (WPARAM)TEXT("Decode Frame Error"), 0);
	Close();
	return;
}


void CRemoteDesktopSrv::Control(CtrlParam*pParam)
{
	SendMsg(REMOTEDESKTOP_CTRL, (char*)pParam, sizeof(CtrlParam));
}
/***************************************************************************
*	Event Handler
*
**************************************************************************/

void CRemoteDesktopSrv::OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer)
{
	switch (Msg)
	{
	case REMOTEDESKTOP_ERROR:
		OnError((TCHAR*)Buffer);
		break;
	case REMOTEDESKTOP_DESKSIZE:
		OnDeskSize(Buffer);
		break;
	case REMOTEDESKTOP_FRAME:
		OnFrame(dwSize, Buffer);
		break;
	case REMOTEDESKTOP_SET_CLIPBOARDTEXT:
		OnSetClipboardText(Buffer);
		break;
	case REMOTEDESKTOP_BMP_FILE:
		OnBmpFile(Buffer, dwSize);
		break;
	default:
		break;
	}
}


void CRemoteDesktopSrv::OnWriteMsg(WORD Msg,DWORD dwSize, char*Buffer){

}


void CRemoteDesktopSrv::SetClipboardText(char*szText){
	//设置剪切板内容
	SendMsg(REMOTEDESKTOP_SET_CLIPBOARDTEXT, szText, strlen(szText) + 1);
}

void CRemoteDesktopSrv::OnSetClipboardText(char*Text){
	m_pWnd->SendMessage(WM_REMOTE_DESKTOP_SET_CLIPBOARD_TEXT, (WPARAM)Text, (LPARAM)0);
}

void CRemoteDesktopSrv::SetCaptureFlag(DWORD dwFlag){
	SendMsg(REMOTEDESKTOP_SETFLAG, (char*)&dwFlag, sizeof(dwFlag));
}

void CRemoteDesktopSrv::StartRDP(DWORD dwMaxFps, DWORD dwQuality){
	DWORD Args[2] = { dwMaxFps, dwQuality };
	SendMsg(REMOTEDESKTOP_INIT_RDP, Args, sizeof(Args));
}