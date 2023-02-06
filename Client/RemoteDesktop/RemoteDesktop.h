#pragma once
#include "MsgHandler.h"
#include "DesktopGrab.h"
#define REMOTEDESKTOP	('R'|('D'<<8)|('T'<<16)|('P'<<24))

//鼠标?,透明窗口?
#define REMOTEDESKTOP_INIT_RDP		(0xaa01)
#define REMOTEDESKTOP_DESKSIZE		(0xaa02)

#define REMOTEDESKTOP_NEXT_FRAME	(0xaaa1)
#define REMOTEDESKTOP_FRAME			(0xaaa2)
#define REMOTEDESKTOP_ERROR			(0xaaa3)

#define REMOTEDESKTOP_CTRL			(0xaaa4)

#define REMOTEDESKTOP_SETFLAG		(0xaaa6)

//设置剪切板数据.
#define REMOTEDESKTOP_SET_CLIPBOARDTEXT	(0xaaa7)
#define REMOTEDESKTOP_GET_BMP_FILE		(0xaaa8)
#define REMOTEDESKTOP_BMP_FILE			(0xaaa9)

#define REMOTEDESKTOP_FLAG_CAPTURE_MOUSE		(0x1)
#define REMOTEDESKTOP_FLAG_CAPTURE_TRANSPARENT	(0x2)


#define QUALITY_LOW		0
#define QUALITY_HIGH	2

class CRemoteDesktop :
	public CMsgHandler
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
	//DWORD		 m_dwLastTime;
	
	char*		 m_FrameBuffer;
	DWORD		 m_dwFrameSize;
	
	DWORD		 m_dwMaxFps;
	DWORD		 m_Quality;

	DWORD		 m_dwCaptureFlags;
	
	HANDLE		m_ClipbdListenerThread;
	HWND		m_hClipbdListenWnd;

	DWORD		m_dwWorkThreadId;
	HANDLE		m_hWorkThread;

	static void CALLBACK DesktopGrabThread(CRemoteDesktop*pThis);

	static void CALLBACK ClipdListenProc(CRemoteDesktop*pThis);
	static LRESULT CALLBACK WndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:

	void OnClose();
	void OnOpen();

	void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer){}

	void TermRD();
	void OnInitRD(DWORD dwFps,DWORD dwQuality);

	void OnScreenShot();
	void OnNextFrame();
	void OnControl(CtrlParam*Param);
	void OnSetFlag(DWORD dwFlag);

	
	void OnSetClipbdText(char*szText);
	void SetClipbdText(char*szText);

	CRemoteDesktop(CIOCPClient *pClient);
	~CRemoteDesktop();
};

