#include "stdafx.h"
#include "RemoteDesktopSrv.h"
#include "RemoteDesktopWnd.h"

#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"yuv.lib")

CRemoteDesktopSrv::CRemoteDesktopSrv(CManager*pManager):
	CMsgHandler(pManager),
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

void CRemoteDesktopSrv::SetMaxFps(DWORD dwMaxFps)
{
	SendMsg(REMOTEDESKTOP_SETMAXFPS, (char*)&dwMaxFps, sizeof(DWORD));
}
void CRemoteDesktopSrv::OnDeskSize(char*DeskSize)
{
	DWORD dwWidth = ((DWORD*)(DeskSize))[0];
	DWORD dwHeight = ((DWORD*)(DeskSize))[1];

	if (RemoteDesktopSrvInit(dwWidth, dwHeight) == FALSE){
		m_pWnd->SendMessage(WM_REMOTE_DESKTOP_ERROR, (WPARAM)L"RemoteDesktopSrvInit Error", 0);
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
	if (m_pCodecContext == NULL)
	{
		m_pWnd->SendMessage(WM_REMOTE_DESKTOP_ERROR, (WPARAM)L"RemoteDesktopSrv Not Init", 0);
		Close();
		return;
	}
	//解码数据.
	av_init_packet(&m_AVPacket);
	//
	m_AVPacket.data = (uint8_t*)Buffer;
	m_AVPacket.size = dwRead;
	//获取下一帧
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

char * CRemoteDesktopSrv::GetBmpFile(DWORD * lpDataSize){
	DWORD dwBitsSize = 0, dwBufferSize = 0;
	BITMAPINFOHEADER bi = { 0 };
	BITMAPFILEHEADER bmfHeader = { 0 };
	char * lpBuffer = NULL;
	int Result = 0;

	if (!m_Bmp.bmHeight || !m_Bmp.bmWidth){
		return nullptr;
	}

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = m_Bmp.bmWidth;
	bi.biHeight = m_Bmp.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 24;
	bi.biCompression = BI_RGB;

	dwBitsSize = ((m_Bmp.bmWidth * bi.biBitCount + 31) / 32) * 4 * m_Bmp.bmHeight;

	dwBufferSize += sizeof(BITMAPFILEHEADER);
	dwBufferSize += sizeof(BITMAPINFOHEADER);
	dwBufferSize += dwBitsSize;

	lpBuffer = new char[dwBufferSize];

	bmfHeader.bfType = 0x4D42;
	bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
	bmfHeader.bfSize = dwBufferSize;

	memcpy(lpBuffer, &bmfHeader, sizeof(bmfHeader));
	memcpy(lpBuffer + sizeof(bmfHeader), &bi, sizeof(bi));

	ResetEvent(m_hMutex);		//lock
	//get bits 
	Result = GetDIBits(m_hMemDC, m_hBmp, 0,
		m_Bmp.bmHeight, lpBuffer +
		sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)
		, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
	SetEvent(m_hMutex);			//unlock

	if (Result != m_Bmp.bmHeight){
		delete[] lpBuffer;
		return NULL;
	}
	*lpDataSize = dwBufferSize;
	return lpBuffer;
}

void CRemoteDesktopSrv::Control(CtrlParam*pParam)
{
	SendMsg(REMOTEDESKTOP_CTRL, (char*)pParam, sizeof(CtrlParam));
}
/***************************************************************************
*	Event Handler
*
**************************************************************************/
void CRemoteDesktopSrv::OnClose()
{
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
			m_pWnd = nullptr;
		}
	}
}

void CRemoteDesktopSrv::OnOpen()
{
	m_pWnd = new CRemoteDesktopWnd(this);
	if (m_pWnd->Create(NULL, NULL) == FALSE){
		Close();
		return;
	}
	m_pWnd->ShowWindow(SW_SHOW);
	//获取桌面大小.
	SendMsg(REMOTEDESKTOP_GETSIZE, 0, 0);
}

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