#include "stdafx.h"
#include "CameraSrv.h"
#include "CameraDlg.h"
#include "json\json.h"
#include "utils.h"

CCameraSrv::CCameraSrv(CManager*pManager) :
	CMsgHandler(pManager),
	m_pDlg(NULL),
	m_pCodec(NULL),
	m_pCodecContext(NULL),
	m_hBmp(NULL),
	m_hMemDC(NULL),
	m_Buffer(NULL)
{
	memset(&m_Bmp, 0, sizeof(m_Bmp));
	memset(&m_AVPacket, 0, sizeof(m_AVPacket));
	memset(&m_AVFrame, 0, sizeof(m_AVFrame));

	m_hMutex = CreateEvent(0, TRUE, TRUE, NULL);
}


CCameraSrv::~CCameraSrv()
{
	CloseHandle(m_hMutex);
}

void CCameraSrv::OnClose()
{
	CameraTerm();
	if (m_pDlg){
		if (m_pDlg->m_DestroyAfterDisconnect){
			//窗口先关闭的.
			m_pDlg->DestroyWindow();
			delete m_pDlg;
		}
		else{
			// pHandler先关闭的,那么就不管窗口了
			m_pDlg->m_pHandler = nullptr;
			m_pDlg->PostMessage(WM_CAMERA_ERROR, (WPARAM)TEXT("Disconnect."));
		}
		m_pDlg = nullptr;
	}
}

void CCameraSrv::OnOpen()
{
	m_pDlg = new CCameraDlg(this);
	ASSERT(m_pDlg->Create(IDD_CAM_DLG, CWnd::GetDesktopWindow()));

	m_pDlg->ShowWindow(SW_SHOW);
}


void CCameraSrv::OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer)
{
	switch (Msg)
	{
	case CAMERA_DEVICELIST:
		OnDeviceList(Buffer);
		break;
	case CAMERA_VIDEOSIZE:
		do{
			Json::Value res;
			if (Json::Reader().parse(Buffer, res)){
				int code = res["code"].asInt();
				string err = res["err"].asString();
				int width = res["width"].asInt();
				int height = res["height"].asInt();

				OnVideoSize(code, err, width, height);
			}
		} while (0); 
		//OnVideoSize((DWORD*)Buffer);
		break;
	case CAMERA_FRAME:
		OnFrame(Buffer, dwSize);
		break;
	case CAMERA_ERROR:
		OnError((TCHAR*)Buffer);
		break;
	case CAMERA_STOP_OK:
		OnStopOk();
		break;
	default:
		break;
	}
}

void CCameraSrv::OnDeviceList(char*DeviceList)
{
	m_pDlg->SendMessage(WM_CAMERA_DEVICELIST, (WPARAM)DeviceList, NULL);
}

void CCameraSrv::Start(const string& device, int format, int bitcount,int width, int height)
{
	CameraTerm();
	//
	Json::Value res;
	string data;
	res["device"] = device;
	res["format"] = format;
	res["width"] = width;
	res["height"] = height;
	res["bit"] = bitcount;
	data = Json::FastWriter().write(res);

	SendMsg(CAMERA_START, (char*)data.c_str(), data.length() + 1);
}

void CCameraSrv::Stop()
{
	SendMsg(CAMERA_STOP, 0, 0);
}

void CCameraSrv::OnStopOk()
{
	CameraTerm();
	m_pDlg->SendMessage(WM_CAMERA_STOP_OK, 0, 0);
}

int CCameraSrv::CameraInit(int width, int height)
{
	CameraTerm();
	BITMAPINFO bmi = { 0 };
	//创建内存dc
	m_hMemDC = CreateCompatibleDC(m_pDlg->m_hdc);
	if (m_hMemDC == NULL)
		goto Failed;

	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;				//4字节貌似解码快
	bmi.bmiHeader.biCompression = BI_RGB;

	//创建DIBSection
	m_hBmp = CreateDIBSection(m_hMemDC, &bmi, DIB_RGB_COLORS, &m_Buffer, 0, 0);
	if (m_hBmp == NULL || !SelectObject(m_hMemDC, m_hBmp))
		goto Failed;
	//
	GetObject(m_hBmp, sizeof(BITMAP), &m_Bmp);
	//
	//创建解码器.
	m_pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (m_pCodec == NULL)
		goto Failed;
	//
	m_pCodecContext = avcodec_alloc_context3(m_pCodec);
	if (m_pCodecContext == NULL)
		goto Failed;
	//
	if (0 != avcodec_open2(m_pCodecContext, m_pCodec, 0))
		goto Failed;
	return 0;
Failed:
	CameraTerm();
	return -1;
}


void CCameraSrv::CameraTerm()
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

	memset(&m_AVPacket, 0, sizeof(m_AVPacket));
	memset(&m_AVFrame, 0, sizeof(m_AVFrame));
}

void CCameraSrv::OnVideoSize(int code, string&err, int width, int height){
	if (code){
#ifdef UNICODE
		wchar_t* error = convertAnsiToUtf16(err.c_str());
		m_pDlg->SendMessage(WM_CAMERA_ERROR, (WPARAM)error, NULL);
		delete[] error;
#else
		m_pDlg->SendMessage(WM_CAMERA_ERROR, (WPARAM)err.c_str(), NULL);
#endif
		return;
	}

	if (CameraInit(width, height)){
		m_pDlg->SendMessage(WM_CAMERA_ERROR, (WPARAM)TEXT("CameraSrv Init Failed!"), NULL);
		return;
	}

	m_pDlg->SendMessage(WM_CAMERA_VIDEOSIZE, width, height);
}

void CCameraSrv::OnFrame(char*buffer,DWORD dwLen)
{
	if (m_pCodecContext == NULL){
		//解码器还没有创建,丢弃,
		return;
	}

	av_init_packet(&m_AVPacket);
	m_AVPacket.data = (uint8_t*)buffer;
	m_AVPacket.size = dwLen;

	//SendMsg(CAMERA_GETFRAME, 0, 0);
	if (!avcodec_send_packet(m_pCodecContext, &m_AVPacket)){
		if (!avcodec_receive_frame(m_pCodecContext, &m_AVFrame)){
			//成功.
			//I420 ---> ARGB.
			WaitForSingleObject(m_hMutex, INFINITE);

			libyuv::I420ToARGB(m_AVFrame.data[0], m_AVFrame.linesize[0], 
				m_AVFrame.data[1], m_AVFrame.linesize[1],
				m_AVFrame.data[2], m_AVFrame.linesize[2],
				(uint8_t*)m_Buffer, m_Bmp.bmWidthBytes, m_Bmp.bmWidth, m_Bmp.bmHeight);
			//显示到窗口上
			m_pDlg->SendMessage(WM_CAMERA_FRAME, (WPARAM)m_hMemDC, (LPARAM)&m_Bmp);
			return;
		}
	}
	m_pDlg->SendMessage(WM_CAMERA_ERROR, (WPARAM)TEXT("Decode Frame Failed!"), NULL);
	return;
}

char * CCameraSrv::GetBmpFile(DWORD * lpDataSize){
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

void CCameraSrv::OnError(TCHAR*szError)
{
	m_pDlg->SendMessage(WM_CAMERA_ERROR, (WPARAM)szError, 0);
}