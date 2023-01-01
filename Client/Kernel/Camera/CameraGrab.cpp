#include "CameraGrab.h"

#include <stdint.h>
#include "x264.h"


#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "yuv.lib")
#pragma comment(lib,"libx264.lib")

CCameraGrab::CCameraGrab()
{
	m_dwHeight = 0;
	m_dwWidth = 0;

	m_hEvent = NULL;
	m_hMutex = NULL;

	m_pGraph = NULL;
	m_pBuild = NULL;
	m_pCaptureFilter = NULL;

	m_pSampleGrabberFilter = NULL;
	m_pSamplerGrabber = NULL;
	m_pNullRenderer = NULL;
	m_pMediaControl = NULL;

	m_pCallback = NULL;
	m_dwBufSize = NULL;

	m_pEncoder = NULL;
	m_pPicOut = NULL;
	m_pPicIn = NULL;
}


CCameraGrab::~CCameraGrab()
{
	GrabberTerm();
}

void CCameraGrab::GrabberTerm()
{
	if (m_pMediaControl){
		m_pMediaControl->Stop();
		m_pMediaControl->Release();
		m_pMediaControl = NULL;
	}

	if (m_hMutex){
		CloseHandle(m_hMutex);
		m_hMutex = NULL;
	}
	if (m_hEvent){
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}

	if (m_pGraph){
		m_pGraph->Release();
		m_pGraph = NULL;
	}
	if (m_pBuild){
		m_pBuild->Release();
		m_pBuild = NULL;
	}
	if (m_pCaptureFilter){
		m_pCaptureFilter->Release();
		m_pCaptureFilter = NULL;
	}

	if (m_pSamplerGrabber){
		m_pSamplerGrabber->Release();
		m_pSamplerGrabber = NULL;
	}

	if (m_pSampleGrabberFilter){
		m_pSampleGrabberFilter->Release();
		m_pSampleGrabberFilter = NULL;
	}
	if (m_pNullRenderer){
		m_pNullRenderer->Release();
		m_pNullRenderer = NULL;
	}
	if (m_pCallback){
		delete m_pCallback;
		m_pCallback = NULL;
	}

	for (auto &frame : m_FrameList){
		if (frame.first){
			cout << std::hex << frame.first << endl;
			delete[] frame.first;
		}
	}
	m_FrameList.clear();
	m_dwBufSize = NULL;

	//x264
	if (m_pEncoder){
		x264_encoder_close(m_pEncoder);
		m_pEncoder = NULL;
	}
	if (m_pPicIn){
		x264_picture_clean(m_pPicIn);
		free(m_pPicIn);
		m_pPicIn = NULL;
	}
	if (m_pPicOut){
		free(m_pPicOut);
		m_pPicOut = NULL;
	}
	m_dwHeight = 0;
	m_dwWidth = 0;
	//printf("Grabber Term Ok\n");
}

void CCameraGrab::AddTail(char*buffer,DWORD dwLen)
{
	if (m_dwBufSize > MAX_BUF_SIZE)
		return;
	
	//Save Data...
	pair<char*, DWORD> frame(new char[dwLen],dwLen);

	m_dwBufSize += dwLen;
	memcpy(frame.first, buffer, dwLen);
	
	//list lock....
	WaitForSingleObject(m_hMutex, INFINITE);
	m_FrameList.push_back(frame);

	SetEvent(m_hEvent);
	SetEvent(m_hMutex);
}
void CCameraGrab::RemoveHead(char**ppBuffer,DWORD*pLen)
{
	pair<char*, DWORD > frame(nullptr,0);
	while (true)
	{
		WaitForSingleObject(m_hMutex, INFINITE);
		if (m_FrameList.size()){
			frame = m_FrameList.front();
			m_FrameList.pop_front();
		}

		if (!frame.first)								//buffer 为空.
			ResetEvent(m_hEvent);						//如果没有数据了...,
		else
			m_dwBufSize -= frame.second;

		SetEvent(m_hMutex);

		if (!frame.first){
			//printf("No Frame,Wait ForSingle Object\n");
			if (WAIT_TIMEOUT == WaitForSingleObject(m_hEvent, 3000)){
				//有事件时,这时候链表里面就有数据了...
				*ppBuffer = NULL;
				*pLen = NULL;
				return;
			}
		}
		else
			break;
	}
	*ppBuffer = frame.first;
	*pLen = frame.second;
}

VideoInfoList& CCameraGrab::GetDeviceList()
{ 
	m_device.clear();
	GetDeviceList(string(""), 0);
	return m_device;
}

void CCameraGrab::GetDeviceList(const string& query_name, IBaseFilter**ppBaseFilter)
{
	HRESULT hr = NULL;
	ICreateDevEnum*pDevEnum = NULL;
	IEnumMoniker*pEnumMoniker = NULL;
	IMoniker*pMoniker = NULL;
	BOOL	bLoop = TRUE;
	int i = 0;

	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
		IID_ICreateDevEnum, (void **)&pDevEnum);
	if (FAILED(hr) || !pDevEnum){	
		goto Failed;
	}
	hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMoniker, 0);
	if (FAILED(hr) || !pEnumMoniker){	
		goto Failed;
	}

	for (hr = pEnumMoniker->Next(1, &pMoniker, NULL);
		hr == S_OK&&pMoniker&&bLoop;
		hr = pEnumMoniker->Next(1, &pMoniker, NULL)){

		VARIANT varTemp;
		IPropertyBag *pProBag = NULL;
		string device_name;

		hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (LPVOID*)&pProBag);
		if (hr != S_OK || pProBag == NULL){
			goto failed;
		}
		//获取设备名称....
		varTemp.vt = VT_BSTR;
		hr = pProBag->Read(TEXT("FriendlyName"), &varTemp, NULL);
		TCHAR * w_name = (TCHAR *)varTemp.bstrVal;
		//wide byte to g2312....
		int len = WideCharToMultiByte(CP_ACP, 0, w_name, lstrlenW(w_name), nullptr, 0, nullptr, nullptr);
		device_name.resize(len + 1,0);
		WideCharToMultiByte(CP_ACP, 0, w_name, lstrlenW(w_name), 
			(char*)device_name.c_str(), len, nullptr, nullptr);

		SysFreeString(varTemp.bstrVal);

		if (!SUCCEEDED(hr)){
			goto failed;
		}

		
		if (query_name.length() == 0 && m_device.size() == 0){			//获取所有设备信息..
			IBaseFilter*	pBaseFilter = NULL;
			hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (LPVOID*)&pBaseFilter);

			if (!SUCCEEDED(hr) || pBaseFilter == NULL){
				if (pBaseFilter) pBaseFilter->Release();
				goto failed;
			}
			///
			IEnumPins* pIEnumPins = 0;
			IPin *pIPin = NULL;
			hr = pBaseFilter->EnumPins(&pIEnumPins);				//枚举引脚??是啥意思
			/////
			if (!SUCCEEDED(hr) || pIEnumPins == NULL){
				if (pIEnumPins) pIEnumPins->Release();
				goto failed;
			}

			for (hr = pIEnumPins->Next(1, &pIPin, NULL); 
				SUCCEEDED(hr) && pIPin;
				hr = pIEnumPins->Next(1, &pIPin, NULL)){

				IEnumMediaTypes* pMt = NULL;
				AM_MEDIA_TYPE *pMediaType = NULL;

				hr = pIPin->EnumMediaTypes(&pMt);
				if (hr != S_OK){
					pIPin->Release();
					if (pMt){
						pMt->Release();
					}
					continue;
				}

				for (hr = pMt->Next(1, &pMediaType, NULL);
					SUCCEEDED(S_OK) && pMediaType;
					hr = pMt->Next(1, &pMediaType, NULL)){

					//emmmmmm还得考虑压缩的格式.
					if (pMediaType->formattype == FORMAT_VideoInfo && 
						pMediaType->cbFormat >= (sizeof(VIDEOINFOHEADER))
						&& pMediaType->pbFormat){
						VIDEOINFOHEADER* pVideoInfoHeader = (VIDEOINFOHEADER*)pMediaType->pbFormat;
						int found = 0;
						DWORD dwCompression = pVideoInfoHeader->bmiHeader.biCompression;
						int width = pVideoInfoHeader->bmiHeader.biWidth;
						int height = pVideoInfoHeader->bmiHeader.biHeight;
						int bitcount = pVideoInfoHeader->bmiHeader.biBitCount;
						
						//保存起来...
						for (auto & exist : m_device[device_name][dwCompression][bitcount]){
							if (exist == pair<int, int>(width, height)){
								found = 1;
								break;
							}
						}
						//
						if (!found){
							m_device[device_name][dwCompression][bitcount].push_back(pair<int, int>(width, height));
						}
					}
					//释放资源.
					if (pMediaType->cbFormat && pMediaType->pbFormat){
						CoTaskMemFree(pMediaType->pbFormat);
					}
					if (pMediaType->pUnk){
						pMediaType->pUnk->Release();
					}
					pMediaType = nullptr;
				}
				pMt->Release(),pMt = nullptr;
				pIPin->Release(), pIPin = nullptr;
			}

			pIEnumPins->Release(), pIEnumPins = nullptr;;
			pBaseFilter->Release(), pBaseFilter = nullptr;
		}
		else if (query_name.length()){				//查询指定的设备..
			IBaseFilter*	pBaseFilter = NULL;
			bLoop = FALSE;
			hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (LPVOID*)&pBaseFilter);
			if (SUCCEEDED(hr) && pBaseFilter){
				*ppBaseFilter = pBaseFilter;
			}
		}
		//
	failed:
		if (pMoniker)
			pMoniker->Release();
		if (pProBag)
			pProBag->Release();
	}

Failed:
	if (pEnumMoniker)
		pEnumMoniker->Release();

	if (pDevEnum)
		pDevEnum->Release();
}

int CCameraGrab::GrabberInit(const string&device_name, DWORD dwCompression,
	DWORD dwBitCount, DWORD dwWidth, DWORD dwHeight,string&err)
{
	HRESULT hr = NULL;
	AM_MEDIA_TYPE mt;
	AM_MEDIA_TYPE * mmt = NULL;
	IAMStreamConfig   *pConfig = NULL;
	VIDEOINFOHEADER * pvih = NULL;
	x264_param_t param = { 0 };

	GrabberTerm();

	m_dwHeight = dwHeight;
	m_dwWidth = dwWidth;
	//
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&m_pGraph);
	if (FAILED(hr)){
		err = __LINE__;
		goto Failed;
	}
	//
	hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void**)&m_pBuild);
	if (FAILED(hr)){
		err = __LINE__;
		goto Failed;
	}
	//
	hr = m_pBuild->SetFiltergraph(m_pGraph);
	if (FAILED(hr)){
		err = __LINE__;
		goto Failed;
	}
	//
	GetDeviceList(device_name,&m_pCaptureFilter);

	if (!m_pCaptureFilter || (hr = m_pGraph->AddFilter(m_pCaptureFilter, L"Video Capture"), FAILED(hr))){
		err = __LINE__;
		goto Failed;
	}
	//
	hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
		IID_IBaseFilter, (LPVOID*)&m_pSampleGrabberFilter);
	if (FAILED(hr)){
		err = __LINE__;
		goto Failed;
	}
	//
	hr = m_pSampleGrabberFilter->QueryInterface(IID_ISampleGrabber, (void**)&m_pSamplerGrabber);
	if (FAILED(hr)){
		err = __LINE__;
		goto Failed;
	}
	//
	
	ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
	//设置输出视频格式.,SamplerGrabber的输出格式.
	mt.majortype = MEDIATYPE_Video;
	mt.subtype = MEDIASUBTYPE_ARGB32;
	//
	m_pSamplerGrabber->SetMediaType(&mt);
	m_pSamplerGrabber->SetOneShot(FALSE);
	m_pSamplerGrabber->SetBufferSamples(FALSE);
	m_pCallback = new CMySampleGrabberCB(this);
	m_pSamplerGrabber->SetCallback(m_pCallback, 1);
	//

	hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
		IID_IBaseFilter, (LPVOID*)&m_pNullRenderer);
	if (FAILED(hr)){
		err = __LINE__;
		goto Failed;
	}
	//
	hr = m_pGraph->AddFilter(m_pSampleGrabberFilter, L"Sample Filter");
	if (FAILED(hr)){
		err = __LINE__;
		goto Failed;
	}
	hr = m_pGraph->AddFilter(m_pNullRenderer, L"Null Renderer");
	if (FAILED(hr)){
		err = __LINE__;
		goto Failed;
	}
	//
	hr = m_pBuild->FindInterface(&PIN_CATEGORY_CAPTURE,
		&MEDIATYPE_Video, m_pCaptureFilter, IID_IAMStreamConfig, (void **)&pConfig);
	if (FAILED(hr)){
		err = __LINE__;
		goto Failed;
	}


	//获取视频尺寸,先设置完尺寸再连接起来。
	//设置捕获尺寸.
	hr = pConfig->GetFormat(&mmt);    //取得默认参数
	if (FAILED(hr)){
		err = __LINE__;
		goto Failed;
	}
	//Set Capture Video Size;
	pvih = (VIDEOINFOHEADER*)mmt->pbFormat;
	pvih->bmiHeader.biHeight = dwHeight;
	pvih->bmiHeader.biWidth = dwWidth;
	pvih->bmiHeader.biCompression = dwCompression;
	pvih->bmiHeader.biBitCount = dwBitCount;

	hr = pConfig->SetFormat(mmt);

	if (FAILED(hr)){
		err = __LINE__;
		goto Failed;
	}

	if (mmt->cbFormat && mmt->pbFormat){
		CoTaskMemFree(mmt->pbFormat);
		mmt->pbFormat = NULL;
		mmt->cbFormat = NULL;
	}
	if (mmt->pUnk){
		mmt->pUnk->Release();
		mmt->pUnk = NULL;
	}
	pConfig->Release();

	if (FAILED(hr)){
		err = __LINE__;
		goto Failed;
	}
	//
	hr = m_pBuild->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_pCaptureFilter, 
		m_pSampleGrabberFilter, m_pNullRenderer);
	if (FAILED(hr)){
		err = __LINE__;
		goto Failed;
	}
	//获取控制接口
	hr = m_pGraph->QueryInterface(IID_IMediaControl, (void**)&m_pMediaControl);
	if (FAILED(hr)){
		err = __LINE__;
		goto Failed;
	}
	/*
			x264 encoder init....
	*/
	m_pPicIn = (x264_picture_t*)calloc(1,sizeof(x264_picture_t));
	m_pPicOut = (x264_picture_t*)calloc(1,sizeof(x264_picture_t));

	x264_picture_init(m_pPicIn);
	x264_picture_alloc(m_pPicIn, X264_CSP_I420, m_dwWidth, m_dwHeight);

	x264_param_default_preset(&param, "veryfast", "zerolatency");

	param.i_width = m_dwWidth;
	param.i_height = m_dwHeight;

	param.i_log_level = X264_LOG_NONE;
	param.i_threads = 1;
	param.i_frame_total = 0;
	param.i_keyint_max = 10;
	param.i_bframe = 0;					//不启用b帧
	param.b_open_gop = 0;

	param.i_fps_num = 1;
	param.i_csp = X264_CSP_I420;

	x264_param_apply_profile(&param, x264_profile_names[0]);

	m_pEncoder = x264_encoder_open(&param);
	if (!m_pEncoder){
		err = __LINE__;
		err += '\t';
		err += "x264 encoder init failed!";
		goto Failed;
	}
	//
	m_hMutex = CreateEvent(NULL, FALSE, TRUE, NULL);
	m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	err = "grab init succeeded.";
	return 0;
Failed:
	GrabberTerm();
	return -1;
}

BOOL CCameraGrab::GetFrame(char**ppbuffer,DWORD* plen)
{
	//printf("Get Frame()\n");
	char*Buffer = nullptr;
	DWORD dwLen = 0;
	x264_nal_t*pNal = NULL;
	int			iNal = 0;
	int			Size = 0;

	RemoveHead(&Buffer, &dwLen);
	if (Buffer == nullptr){
		//printf("No Buffer\n");
		return FALSE;
	}
	//to I420
	libyuv::ARGBToI420((uint8_t*)Buffer, m_dwWidth * 4, m_pPicIn->img.plane[0], m_pPicIn->img.i_stride[0],
		m_pPicIn->img.plane[1], m_pPicIn->img.i_stride[1], m_pPicIn->img.plane[2], 
		m_pPicIn->img.i_stride[2],m_dwWidth,m_dwHeight);
	
	
	Size = x264_encoder_encode(m_pEncoder, &pNal, &iNal, m_pPicIn, m_pPicOut);
	delete[] Buffer;

	if (Size < 0)
		return FALSE;
	*ppbuffer = (char*)pNal[0].p_payload;
	*plen = Size;
	return TRUE;
}

BOOL CCameraGrab::StartGrab()
{
	HRESULT hr = 0;
	if (m_pMediaControl &&
		(hr = m_pMediaControl->Run(), SUCCEEDED(hr)))
		return TRUE;

	return FALSE;
}

void CCameraGrab::StopGrab()
{
	if (m_pMediaControl){
		m_pMediaControl->Stop();
	}
}