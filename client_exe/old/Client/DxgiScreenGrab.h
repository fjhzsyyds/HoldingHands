//#include <d3d11.h>
//#include <dxgi1_2.h>
//#include <stdio.h>
//
//
//class DxgiScreenGrab{
//private:
//	ID3D11Device* m_device;
//	ID3D11DeviceContext* m_deviceContext;
//	IDXGIOutputDuplication* m_desktopDupl;
//	DWORD m_dwWidth, m_dwHeight;
//	BYTE*m_pImageBuffer;
//
//	BOOL initDXGIResources();
//	void clearDXGIResources();
//	BOOL initDuplication();
//	void clearDuplication();
//
//public:
//	//获取的都是 32位的图像
//	BOOL GrabInit();
//	
//	void GetScreenSize(DWORD&dwWidth, DWORD&dwHeight);
//	BOOL GetFrame(void**buffer,DWORD dwTimeout);
//	
//	void GrabTerm();
//
//	DxgiScreenGrab();
//	~DxgiScreenGrab();
//};