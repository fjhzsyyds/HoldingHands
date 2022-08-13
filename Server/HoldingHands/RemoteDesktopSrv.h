#pragma once
extern "C"
{
#include <libavcodec\avcodec.h>
#include <libavutil\avutil.h>
#include <libyuv.h>
}

#define REMOTEDESKTOP	('R'|('D'<<8)|('T'<<16)|('P'<<24))

#define REMOTEDESKTOP_GETSIZE		(0xaa01)
#define REMOTEDESKTOP_DESKSIZE		(0xaa02)

#define REMOTEDESKTOP_NEXT_FRAME			(0xaaa1)
#define REMOTEDESKTOP_FRAME			(0xaaa2)
#define REMOTEDESKTOP_ERROR			(0xaaa3)



//
#define REMOTEDESKTOP_CTRL			(0xaaa4)
#define REMOTEDESKTOP_SETMAXFPS		(0xaaa5)
#define REMOTEDESKTOP_SETFLAG		(0xaaa6)


#define REMOTEDESKTOP_SET_CLIPBOARDTEXT	(0xaaa7)


#define REMOTEDESKTOP_FLAG_CAPTURE_MOUSE		(0x1)
#define REMOTEDESKTOP_FLAG_CAPTURE_TRANSPARENT	(0x2)

#include "EventHandler.h"

class CRemoteDesktopWnd;



class CRemoteDesktopSrv :
	public CEventHandler
{
public:
	struct CtrlParam
	{
		DWORD dwType;
		union
		{
			DWORD dwCoor;
			DWORD VkCode;
		}Param;
		DWORD dwExtraData;
	};
private:
	AVCodec*			m_pCodec;
	AVCodecContext*		m_pCodecContext;
	AVPacket			m_AVPacket;
	AVFrame				m_AVFrame;

	HBITMAP				m_hBmp;
	BITMAP				m_Bmp;
	HDC					m_hMemDC;
	void*				m_Buffer;

	CRemoteDesktopWnd*	m_pWnd;

	BOOL RemoteDesktopSrvInit(DWORD dwWidth,DWORD dwHeight);
	void RemoteDesktopSrvTerm();
	
	//Event
	void OnDeskSize(char*DeskSize);
	void OnError(WCHAR* szError);
	void OnFrame(DWORD dwRead,char*Buffer);
	//
	void OnSetClipboardText(char*Text);
public:
	
	//
	void NextFrame();
	void Control(CtrlParam*pParam);
	void SetClipboardText(char*szText);
	void SetMaxFps(DWORD dwMaxFps);
	void SetCaptureFlag(DWORD dwFlag);

	void OnClose();					//��socket�Ͽ���ʱ������������
	void OnConnect();				//��socket���ӵ�ʱ������������
	//�����ݵ����ʱ���������������.
	void OnReadPartial(WORD Event, DWORD Total, DWORD nRead, char*Buffer);
	void OnReadComplete(WORD Event, DWORD Total, DWORD nRead, char*Buffer);
	//�����ݷ�����Ϻ��������������
	void OnWritePartial(WORD Event, DWORD Total, DWORD nWrite, char*Buffer);
	void OnWriteComplete(WORD Event, DWORD Total, DWORD nWrite, char*Buffer);
	CRemoteDesktopSrv(DWORD dwIdentidy);
	~CRemoteDesktopSrv();
};
