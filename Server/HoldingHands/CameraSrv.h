#pragma once
#include "MsgHandler.h"
#include "resource.h"
#include<string>

using std::string;

extern"C"
{
#include <libavcodec\avcodec.h>
#include <libavutil\avutil.h>
#include <libyuv.h>
}


#define CAMERA			('C'|('A'<<8)|('M'<<16)|('E')<<24)
#define CAMERA_DEVICELIST			(0xabc1)
#define CAMERA_START				(0xabc2)
#define CAMERA_VIDEOSIZE			(0xabc3)

#define CAMERA_STOP					(0xabc4)
#define CAMERA_STOP_OK				(0xabdd)


#define CAMERA_GETFRAME				(0xabc5)
#define CAMERA_FRAME				(0xabc6)
#define CAMERA_SCREENSHOT			(0xabc7)


#define CAMERA_ERROR				(0x0000)

class CCameraWnd;

class CCameraSrv :
	public CMsgHandler
{
private:
	CCameraWnd*	m_pWnd;
	
	AVCodec*			m_pCodec;
	AVCodecContext*		m_pCodecContext;
	AVPacket			m_AVPacket;
	AVFrame				m_AVFrame;

	HBITMAP				m_hBmp;
	BITMAP				m_Bmp;
	HDC					m_hMemDC;
	void*				m_Buffer;

	HANDLE				m_hMutex;

	int	CameraInit(int width, int height);
	void	CameraTerm();
public:
	void OnOpen();
	void OnClose();
	
	void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer) {};


	void OnFrame(char*Buffer, DWORD dwLen);
	void OnVideoSize(int code,string&err,int width,int height);
	void OnStopOk();
	void OnDeviceList(char*DeviceList);

	void Start(const char* device, int Width, int Height);
	void Stop();
	void OnError(TCHAR*szError);


	char * GetBmpFile(DWORD * lpDataSize);

	CCameraSrv(CClientContext*pClient);
	~CCameraSrv();
};

