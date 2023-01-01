#pragma once
#include "EventHandler.h"
#include "DesktopGrab.h"
#define REMOTEDESKTOP	('R'|('D'<<8)|('T'<<16)|('P'<<24))

//鼠标?,透明窗口?
#define REMOTEDESKTOP_GETSIZE		(0xaa01)
#define REMOTEDESKTOP_DESKSIZE		(0xaa02)

#define REMOTEDESKTOP_NEXT_FRAME	(0xaaa1)
#define REMOTEDESKTOP_FRAME			(0xaaa2)
#define REMOTEDESKTOP_ERROR			(0xaaa3)

#define REMOTEDESKTOP_CTRL			(0xaaa4)

#define REMOTEDESKTOP_SETMAXFPS		(0xaaa5)
#define REMOTEDESKTOP_SETFLAG		(0xaaa6)

//设置剪切板数据.
#define REMOTEDESKTOP_SET_CLIPBOARDTEXT	(0xaaa7)

#define REMOTEDESKTOP_FLAG_CAPTURE_MOUSE		(0x1)
#define REMOTEDESKTOP_FLAG_CAPTURE_TRANSPARENT	(0x2)

class CRemoteDesktop :
	public CEventHandler
{
private:
	typedef struct
	{
		DWORD dwType;
		union
		{
			DWORD dwCoor;
			DWORD VkCode;
		}Param;
		DWORD dwExtraData;
	}CtrlParam;
	typedef struct MyData
	{
		CRemoteDesktop* m_pThis;
		HWND			m_hNextViewer;
		char*			m_SetClipbdText;
	}MyData;

private:
	CDesktopGrab m_grab;
	DWORD		 m_dwLastTime;
	char*		 m_FrameBuffer;
	DWORD		 m_dwFrameSize;
	DWORD		 m_dwMaxFps;

	DWORD		 m_dwCaptureFlags;
	

	HANDLE		m_ClipbdListenerThread;
	HWND		m_hClipbdListenWnd;

	//BOOL		 m_bStopCapture;
	//HANDLE	 m_hWorkThread;

	/*static void CALLBACK DesktopGrabThread(CRemoteDesktop*pThis);*/

	static void CALLBACK ClipdListenProc(CRemoteDesktop*pThis);
	static LRESULT CALLBACK WndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	void OnClose();
	void OnConnect();

	void OnReadPartial(WORD Event, DWORD Total, DWORD Read, char*Buffer);
	void OnReadComplete(WORD Event, DWORD Total, DWORD Read, char*Buffer);

	void OnGetSize();

	void OnNextFrame();
	void OnControl(CtrlParam*Param);
	void OnSetMaxFps(DWORD dwMaxFps);
	
	void OnSetFlag(DWORD dwFlag);

	void OnSetClipbdText(char*szText);
	void SetClipbdText(char*szText);

	CRemoteDesktop(DWORD dwIdentity);
	~CRemoteDesktop();
};

