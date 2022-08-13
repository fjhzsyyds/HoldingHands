#include "stdafx.h"
#include "RemoteDesktopSrv.h"
#include "RemoteDesktopWnd.h"

#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"yuv.lib")

CRemoteDesktopSrv::CRemoteDesktopSrv(DWORD dwIdentidy):
CEventHandler(dwIdentidy)
{
	m_pCodec = NULL;
	m_pCodecContext = NULL;
	memset(&m_AVPacket, 0, sizeof(AVPacket));
	memset(&m_AVFrame, 0, sizeof(AVFrame));

	m_hBmp = NULL;
	m_hMemDC = NULL;
	m_pWnd = NULL;
	memset(&m_Bmp, 0, sizeof(m_Bmp));
}


CRemoteDesktopSrv::~CRemoteDesktopSrv()
{
	RemoteDesktopSrvTerm();
}


BOOL CRemoteDesktopSrv::RemoteDesktopSrvInit(DWORD dwWidth, DWORD dwHeight)
{
	BITMAPINFO bmi = { 0 };
	CDC*pDc = m_pWnd->GetDC();
	//�����ڴ�dc
	m_hMemDC = CreateCompatibleDC(pDc->GetSafeHdc());
	if (m_hMemDC == NULL)
		return FALSE;
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = dwWidth;
	bmi.bmiHeader.biHeight = dwHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;				//4�ֽ�ò�ƽ����
	bmi.bmiHeader.biCompression = BI_RGB;
	
	//����DIBSection
	m_hBmp = CreateDIBSection(m_hMemDC, &bmi, DIB_RGB_COLORS, &m_Buffer, 0, 0);
	if (m_hBmp == NULL || !SelectObject(m_hMemDC, m_hBmp))
		return FALSE;
	//
	GetObject(m_hBmp, sizeof(BITMAP), &m_Bmp);
	//
	//����������.
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
	//AVFrame��Ҫ���
	av_frame_unref(&m_AVFrame);
	//
	memset(&m_AVPacket, 0, sizeof(m_AVPacket));
	memset(&m_AVFrame, 0, sizeof(m_AVFrame));
}
void CRemoteDesktopSrv::NextFrame()
{
	Send(REMOTEDESKTOP_NEXT_FRAME,NULL, 0);
}

void CRemoteDesktopSrv::SetMaxFps(DWORD dwMaxFps)
{
	Send(REMOTEDESKTOP_SETMAXFPS, (char*)&dwMaxFps, sizeof(DWORD));
}
void CRemoteDesktopSrv::OnDeskSize(char*DeskSize)
{
	DWORD dwWidth = ((DWORD*)(DeskSize))[0];
	DWORD dwHeight = ((DWORD*)(DeskSize))[1];

	if (RemoteDesktopSrvInit(dwWidth, dwHeight) == FALSE){
		m_pWnd->SendMessage(WM_REMOTE_DESKTOP_ERROR, (WPARAM)L"RemoteDesktopSrvInit Error", 0);
		Disconnect();
		return;
	}

	NextFrame();
	m_pWnd->SendMessage(WM_REMOTE_DESKTOP_SIZE, m_Bmp.bmWidth, m_Bmp.bmHeight);
}
void CRemoteDesktopSrv::OnError(WCHAR*szError)
{
	//����ɼ�ʧ��.
	m_pWnd->SendMessage(WM_REMOTE_DESKTOP_ERROR, (WPARAM)szError, 0);
}
void CRemoteDesktopSrv::OnFrame(DWORD dwRead, char*Buffer)
{
	if (m_pCodecContext == NULL)
	{
		m_pWnd->SendMessage(WM_REMOTE_DESKTOP_ERROR, (WPARAM)L"RemoteDesktopSrv Not Init", 0);
		Disconnect();
		return;
	}
	//��������.
	av_init_packet(&m_AVPacket);
	//
	m_AVPacket.data = (uint8_t*)Buffer;
	m_AVPacket.size = dwRead;
	//��ȡ��һ֡
	NextFrame();

	if (!avcodec_send_packet(m_pCodecContext, &m_AVPacket))
	{
		//��������ǰ�����m_AVFrame������.
		if (!avcodec_receive_frame(m_pCodecContext, &m_AVFrame))
		{
			//�ɹ�.
			//I420 ---> ARGB.
			libyuv::I420ToARGB(m_AVFrame.data[0], m_AVFrame.linesize[0], m_AVFrame.data[1], m_AVFrame.linesize[1],
				m_AVFrame.data[2], m_AVFrame.linesize[2],
				(uint8_t*)m_Buffer, m_Bmp.bmWidthBytes,m_Bmp.bmWidth, m_Bmp.bmHeight);
			//��ʾ��������
			m_pWnd->SendMessage(WM_REMOTE_DESKTOP_DRAW, (WPARAM)m_hMemDC, (LPARAM)&m_Bmp);
			return;
		}
	}
	//ʧ��
	m_pWnd->SendMessage(WM_REMOTE_DESKTOP_ERROR, (WPARAM)L"Decode Frame Error", 0);
	Disconnect();
	return;
}

void CRemoteDesktopSrv::Control(CtrlParam*pParam)
{
	Send(REMOTEDESKTOP_CTRL, (char*)pParam, sizeof(CtrlParam));
}
/***************************************************************************
*	Event Handler
*
**************************************************************************/
void CRemoteDesktopSrv::OnClose()
{
	if (m_pWnd){
		//OnClose 
		m_pWnd->SendMessage(WM_CLOSE);
		
		//CFrameWnd will delete this on postncdestroy.
		m_pWnd->DestroyWindow();

		m_pWnd = NULL;
	}
}

void CRemoteDesktopSrv::OnConnect()
{
	m_pWnd = new CRemoteDesktopWnd(this);
	if (m_pWnd->Create(NULL, NULL) == FALSE)
	{
		Disconnect();
		return;
	}
	m_pWnd->ShowWindow(SW_SHOW);
	//��ȡ�����С.
	Send(REMOTEDESKTOP_GETSIZE, 0, 0);
}

void CRemoteDesktopSrv::OnReadPartial(WORD Event, DWORD Total, DWORD nRead, char*Buffer)
{

}
void CRemoteDesktopSrv::OnReadComplete(WORD Event, DWORD Total, DWORD nRead, char*Buffer)
{
	switch (Event)
	{
	case REMOTEDESKTOP_ERROR:
		OnError((WCHAR*)Buffer);
		break;
	case REMOTEDESKTOP_DESKSIZE:
		OnDeskSize(Buffer);
		break;
	case REMOTEDESKTOP_FRAME:
		OnFrame(nRead, Buffer);
		break;
	case REMOTEDESKTOP_SET_CLIPBOARDTEXT:
		OnSetClipboardText(Buffer);
		break;
	default:
		break;
	}
}
//�����ݷ�����Ϻ��������������
void CRemoteDesktopSrv::OnWritePartial(WORD Event, DWORD Total, DWORD nWrite, char*Buffer)
{

}
void CRemoteDesktopSrv::OnWriteComplete(WORD Event, DWORD Total, DWORD nWrite, char*Buffer)
{

}

void CRemoteDesktopSrv::SetClipboardText(char*szText)
{
	//���ü��а�����
	Send(REMOTEDESKTOP_SET_CLIPBOARDTEXT, szText, strlen(szText) + 1);
}

void CRemoteDesktopSrv::OnSetClipboardText(char*Text)
{
	m_pWnd->SendMessage(WM_REMOTE_DESKTOP_SET_CLIPBOARD_TEXT, (WPARAM)Text, (LPARAM)0);
}

void CRemoteDesktopSrv::SetCaptureFlag(DWORD dwFlag){
	Send(REMOTEDESKTOP_SETFLAG, (char*)&dwFlag, sizeof(dwFlag));
}