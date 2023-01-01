#pragma once

#define STRSAFE_NO_DEPRECATE
#include <Windows.h>
#include <dshow.h>
#include "qedit.h"
#include "MySampleGrabberCB.h"
#include <map>
#include <string>
#include <list>
#include <stdint.h>
#include <iostream>

using std::string;
using std::map;
using std::pair;
using std::list;
using std::cout;
using std::endl;


extern "C"{
#include "x264.h"
#include "libyuv.h"
}



typedef map <string, map<DWORD, map<DWORD, list<pair<int, int>>>>> VideoInfoList;

//��󻺳�40MB,�㹻һ��ͼƬ��.
#define MAX_BUF_SIZE (40*1024*1024)
class CCameraGrab
{
private:	

	friend class CMySampleGrabberCB;

	DWORD m_dwHeight;
	DWORD m_dwWidth;

	HANDLE	m_hEvent;									//���FrameList �Ƿ�Ϊ��
	HANDLE	m_hMutex;

	list<pair<char*, DWORD>>	m_FrameList;

	//FrameList*		m_pFrameListHead;
	//FrameList*		m_pFrameListTail;

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

	//����ͷ��Ϣ
	//list<DeviceInfo> m_DeviceList;
	VideoInfoList m_device;
	void GetDeviceList(const string& device_name, IBaseFilter**ppBaseFilter);
public:
	VideoInfoList& GetDeviceList();

	int GrabberInit(const string&device_name, DWORD dwCompression, 
		DWORD dwBitCount, DWORD dwWidth, DWORD dwHeightm, string&err);
	BOOL StartGrab();
	void StopGrab();
	BOOL GetFrame(char**ppbuffer, DWORD* plen);

	void GrabberTerm();
	CCameraGrab();
	~CCameraGrab();
};

