//#include "DxgiScreenGrab.h"
//
//#pragma comment(lib, "d3d11.lib")
//#pragma comment(lib, "dxgi.lib")
//
//DxgiScreenGrab::DxgiScreenGrab() :
//m_device(nullptr),
//m_deviceContext(nullptr),
//m_desktopDupl(nullptr),
//m_pImageBuffer(nullptr),
//m_dwHeight(0),
//m_dwWidth(0)
//{
//
//}
//
//DxgiScreenGrab::~DxgiScreenGrab(){
//	GrabTerm();
//}
//
//BOOL DxgiScreenGrab::initDXGIResources(){
//	D3D_DRIVER_TYPE DriverTypes[] =
//	{
//		D3D_DRIVER_TYPE_HARDWARE,
//		D3D_DRIVER_TYPE_WARP,
//		D3D_DRIVER_TYPE_REFERENCE,
//	};
//	UINT NumDriverTypes = ARRAYSIZE(DriverTypes);
//	D3D_FEATURE_LEVEL FeatureLevels[] =
//	{
//		D3D_FEATURE_LEVEL_11_0,
//		D3D_FEATURE_LEVEL_10_1,
//		D3D_FEATURE_LEVEL_10_0,
//		D3D_FEATURE_LEVEL_9_1
//	};
//	UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);
//	D3D_FEATURE_LEVEL FeatureLevel;
//
//	for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
//	{
//		HRESULT ret = D3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, 0, FeatureLevels, NumFeatureLevels,
//			D3D11_SDK_VERSION, &m_device, &FeatureLevel, &m_deviceContext);
//		if (SUCCEEDED(ret)){
//			return TRUE;
//		}
//	}
//	return FALSE;
//}
//
//void DxgiScreenGrab::clearDXGIResources()
//{
//	if (m_device)
//	{
//		m_device->Release();
//		m_device = nullptr;
//	}
//
//	if (m_device)
//	{
//		m_device->Release();
//		m_device = nullptr;
//	}
//}
//// 获取IDXGIOutputDuplication
//BOOL DxgiScreenGrab::initDuplication()
//{
//	IDXGIDevice* DxgiDevice = nullptr;
//	HRESULT ret = m_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));
//	if (FAILED(ret))
//	{
//		return FALSE;
//	}
//	//
//	IDXGIAdapter* DxgiAdapter = nullptr;
//	ret = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
//	DxgiDevice->Release();
//	DxgiDevice = nullptr;
//
//	if (FAILED(ret))
//	{
//		return FALSE;
//	}
//	//
//	IDXGIOutput* DxgiOutput = nullptr;
//	ret = DxgiAdapter->EnumOutputs(0, &DxgiOutput);
//	DxgiAdapter->Release();
//	DxgiAdapter = nullptr;
//	if (FAILED(ret))
//	{
//		return FALSE;
//	}
//	//
//	IDXGIOutput1* DxgiOutput1 = nullptr;
//	ret = DxgiOutput->QueryInterface(__uuidof(DxgiOutput1), reinterpret_cast<void**>(&DxgiOutput1));
//	DxgiOutput->Release();
//	DxgiOutput = nullptr;
//	if (FAILED(ret))
//	{
//		return FALSE;
//	}
//	ret = DxgiOutput1->DuplicateOutput(m_device, &m_desktopDupl);
//	DxgiOutput1->Release();
//	DxgiOutput1 = nullptr;
//	if (FAILED(ret))
//	{
//		return FALSE;
//	}
//	//get screen size:
//	DXGI_OUTDUPL_DESC out_desc = { 0 };
//	m_desktopDupl->GetDesc(&out_desc);
//	m_dwWidth = out_desc.ModeDesc.Width;
//	m_dwHeight = out_desc.ModeDesc.Height;
//	//printf("Screen Height: %d\nScreen Width: %d\n", m_dwHeight, m_dwWidth);
//	//alloc image buffer
//	m_pImageBuffer = (BYTE*)calloc(1, m_dwWidth * m_dwHeight * 4);
//	return TRUE;
//}
//
//void DxgiScreenGrab::clearDuplication()
//{
//	if (m_desktopDupl)
//	{
//		m_desktopDupl->Release();
//		m_desktopDupl = nullptr;
//	}
//}
//
//void DxgiScreenGrab::GetScreenSize(DWORD &dwWidth, DWORD&dwHeigth){
//	dwWidth = m_dwWidth;
//	dwHeigth = m_dwHeight;
//}
//
//
//BOOL DxgiScreenGrab::GetFrame(void**buffer,DWORD dwTimeout){
//	IDXGIResource* desktopResource;
//	DXGI_OUTDUPL_FRAME_INFO frameInfo;
//	ID3D11Texture2D*acquiredDesktopImage = nullptr;
//	D3D11_TEXTURE2D_DESC desc;
//	D3D11_MAPPED_SUBRESOURCE mappedResource;
//	DXGI_OUTDUPL_DESC lOutputDuplDesc;
//	ID3D11Texture2D* copyDesktop = nullptr;
//	//获取桌面数据
//	HRESULT ret = m_desktopDupl->AcquireNextFrame(dwTimeout, &frameInfo, &desktopResource);
//	// 当画面无变化会返回超时,直接返回上一张图片.
//	if (ret == DXGI_ERROR_WAIT_TIMEOUT)
//	{
//		//返回上一张图片.
//		*buffer = m_pImageBuffer;
//		return TRUE;
//	}
//	if (FAILED(ret)){
//		*buffer = NULL;
//		return FALSE;
//	}
//
//	//-> Texture2D
//	ret = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&acquiredDesktopImage));
//	desktopResource->Release();
//	desktopResource = nullptr;
//	if (FAILED(ret))
//	{
//		return FALSE;
//	}
//	//map 
//	// 获取纹理(图像)的相关信息
//	m_desktopDupl->GetDesc(&lOutputDuplDesc);
//
//
//	desc.Width = lOutputDuplDesc.ModeDesc.Width;
//	desc.Height = lOutputDuplDesc.ModeDesc.Height;
//	desc.Format = lOutputDuplDesc.ModeDesc.Format;
//	desc.ArraySize = 1;
//	desc.BindFlags = 0;
//	desc.MiscFlags = 0;
//	desc.SampleDesc.Count = 1;
//	desc.SampleDesc.Quality = 0;
//	desc.MipLevels = 1;
//
//	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
//	desc.Usage = D3D11_USAGE_STAGING;
//	//创建纹理????
//	ret = m_device->CreateTexture2D(&desc, NULL, &copyDesktop);
//	if (FAILED(ret)){
//		return FALSE;
//	}
//	//拷贝.???为毛还得拷贝一次??,好像是没有读权限.....
//	m_deviceContext->CopyResource(copyDesktop, acquiredDesktopImage);
//	acquiredDesktopImage->Release();
//	acquiredDesktopImage = nullptr;
//
//	//映射到内存.这块内存不需要管.
//	ret = m_deviceContext->Map(copyDesktop, 0, D3D11_MAP_READ, 0, &mappedResource);
//	if (FAILED(ret)){
//		copyDesktop->Release();
//		return FALSE;
//	}
//	DWORD dwImageSize = m_dwWidth * m_dwHeight * 4;
//	RtlCopyMemory(m_pImageBuffer, mappedResource.pData, dwImageSize);
//	copyDesktop->Release();
//
//	//end cur frame
//	m_desktopDupl->ReleaseFrame();
//	//return image buffer
//	*buffer = m_pImageBuffer;
//	return TRUE;
//}
//
//BOOL DxgiScreenGrab::GrabInit(){
//	if (FALSE == initDXGIResources())
//		return FALSE;
//	if (FALSE == initDuplication())
//		return FALSE;
//	return TRUE;
//}
//
//void DxgiScreenGrab::GrabTerm(){
//	//
//	clearDuplication();
//	clearDXGIResources();
//
//	// release image buffer
//	if (m_pImageBuffer){
//		free(m_pImageBuffer);
//		m_pImageBuffer = NULL;
//	}
//}