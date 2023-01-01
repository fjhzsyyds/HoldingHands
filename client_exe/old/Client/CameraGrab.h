#pragma once

#define STRSAFE_NO_DEPRECATE
#include <Windows.h>
#include <dshow.h>
#include "qedit.h"
#include "MySampleGrabberCB.h"
#include <map>
#include <string>
#include <list>


using namespace std;
#include <stdint.h>

extern "C"{
#include "x264.h"
#include "libyuv.h"
}





//��󻺳�40MB,�㹻һ��ͼƬ��.
#define MAX_BUF_SIZE (40*1024*1024)
class CCameraGrab
{
public:
	struct Frame
	{
		char*	m_buffer;
		DWORD	m_dwLength;
	};
	
	typedef struct FrameList
	{
		Frame		frame;
		FrameList*	next;
	}FrameList;
	
	typedef struct DeviceInfo
	{
		WCHAR szName[128];
		map<DWORD, DWORD> VideoSize;
	}DeviceInfo;

private:	

	friend class CMySampleGrabberCB;
	DWORD m_defaultCompression;

	DWORD m_dwHeight;
	DWORD m_dwWidth;

	HANDLE	m_hEvent;						//���FrameList �Ƿ�Ϊ��
	HANDLE	m_hMutex;

	FrameList*		m_pFrameListHead;
	FrameList*		m_pFrameListTail;

	DWORD			m_dwBufSize;
	void AddTail(char*, DWORD);
	void RemoveHead(char**ppBuffer, DWORD*pLen);

	IGraphBuilder		  *m_pGraph;
	ICaptureGraphBuilder2 *m_pBuild;
	IBaseFilter			  *m_pCaptureFilter;
	IBaseFilter			  *m_pSampleGrabberFilter;
	ISampleGrabber		  *m_pSamplerGrabber;
	IBaseFilter		      *m_pNullRenderer;
	IMediaControl		  *m_pMediaControl;
	CMySampleGrabberCB	  *m_pCallback;
	//x264
	x264_t*	m_pEncoder;		//������ʵ��

	x264_picture_t *m_pPicIn;
	x264_picture_t *m_pPicOut;


	list<DeviceInfo> m_DeviceList;
	
public:
	void GetDeviceList(int DeviceIdx, IBaseFilter**ppBaseFilter);
	list<DeviceInfo>& GetDeviceList();

	BOOL GrabberInit(int idx,DWORD dwWidth, DWORD dwHeight);
	BOOL StartGrab();
	void StopGrab();
	BOOL GetFrame(char**ppbuffer, DWORD* plen);

	void GrabberTerm();
	CCameraGrab();
	~CCameraGrab();
};

