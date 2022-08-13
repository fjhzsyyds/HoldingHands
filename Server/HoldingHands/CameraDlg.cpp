// CameraDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "CameraDlg.h"
#include "afxdialogex.h"
#include "CameraSrv.h"

// CCameraDlg �Ի���

IMPLEMENT_DYNAMIC(CCameraDlg, CDialogEx)

CCameraDlg::CCameraDlg(CCameraSrv*pHandler,CWnd* pParent /*=NULL*/)
	: CDialogEx(CCameraDlg::IDD, pParent)
{
	m_pHandler = pHandler;
	m_hdc = NULL;
	m_dwHeight = 0;
	m_dwWidth = 0;
	m_dwFps = 0;
	m_dwLastTime = 0;

	m_dwOrgX = 0;
	m_dwOrgY = 0;

}

CCameraDlg::~CCameraDlg()
{
	while (m_Devices.GetCount())
		delete m_Devices.RemoveHead();
}

void CCameraDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_DeviceList);
	DDX_Control(pDX, IDC_COMBO2, m_VideoSizeList);
}


BEGIN_MESSAGE_MAP(CCameraDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CCameraDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON1, &CCameraDlg::OnBnClickedButton1)
	ON_MESSAGE(WM_CAMERA_DEVICELIST,OnDeviceList)
	ON_MESSAGE(WM_CAMERA_ERROR,OnError)
	ON_MESSAGE(WM_CAMERA_FRAME,OnFrame)
	ON_MESSAGE(WM_CAMERA_VIDEOSIZE,OnVideoSize)
	ON_MESSAGE(WM_CAMERA_SCREENSHOT,OnScreenShot)
	ON_MESSAGE(WM_CAMERA_STOP_OK,OnStopOk)
	ON_WM_CLOSE()
	ON_CBN_SELCHANGE(IDC_COMBO1, &CCameraDlg::OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDC_BUTTON2, &CCameraDlg::OnBnClickedButton2)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CCameraDlg ��Ϣ�������


void CCameraDlg::OnBnClickedOk()
{
}


void CCameraDlg::OnBnClickedButton1()
{
	DWORD dwWidth = 0, dwHeight = 0;
	CString Size;
	CString Text;
	CWnd*pCtrl = GetDlgItem(IDC_BUTTON1);
	pCtrl->GetWindowTextW(Text);
	//��ȡdevice ID
	int idx = m_DeviceList.GetCurSel();

	//��ȡ��Ƶ�ߴ�
	m_VideoSizeList.GetWindowTextW(Size);
	swscanf(Size.GetBuffer(), L"%d%*c%*c%*c%d", &dwWidth, &dwHeight);
	//
	if (Text == "Start"){
		// TODO:  �ڴ���ӿؼ�֪ͨ����������
		if (m_DeviceList.GetCount() == 0 || m_VideoSizeList.GetCount() == 0)
			return;

		m_pHandler->Start(idx,dwWidth,dwHeight);
	}
	else{
		m_pHandler->Stop();
	}
	pCtrl->EnableWindow(0);
}

LRESULT CCameraDlg::OnDeviceList(WPARAM wParam, LPARAM lParam)
{
	WCHAR*szDeviceList = (WCHAR*)wParam;
	WCHAR*pIt = szDeviceList;
	int idx = 0;
	while (pIt[0])
	{
		//����\n
		while (pIt[0] && pIt[0] == '\n')
			pIt++;
		if (pIt[0])
		{
			WCHAR*pDeviceName = pIt;
			WCHAR old = 0;
			CString Text;
			while (pIt[0] && pIt[0] != '\n')
				pIt++;
			old = pIt[0];
			pIt[0] = 0;
			//�ָ����ƺ�size
			WCHAR*split = wcsstr(pDeviceName, L"\t");
			
			if (split)
			{
				DeviceInfo* pDevice = new DeviceInfo;
				*split++ = NULL;
				lstrcpyW(pDevice->m_szDeviceName, pDeviceName);
				WCHAR* size = split;
				while (size[0])
				{
					split = wcsstr(size, L",");
					if (split)
					{
						*split++ = NULL;
					}
					pDevice->m_pVideoSizes->AddTail(size);
					size += (wcslen(size) + 1);
				}
				m_Devices.AddTail(pDevice);
				Text.Format(L"[%d] %s", idx++, pDeviceName);
				m_DeviceList.AddString(Text);
			}
			pIt[0] = old;
		}
	}
	if (m_DeviceList.GetCount())
	{
		m_DeviceList.SetCurSel(0);
		OnCbnSelchangeCombo1();
	}
	return 0;
}

BOOL CCameraDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	m_hdc = ::GetDC(m_hWnd);
	SetStretchBltMode(m_hdc, HALFTONE);

	char Addr[128];
	USHORT Port = 0;
	m_pHandler->GetPeerName(Addr, Port);

	m_Title.Format(L"[%s] Camera", CA2W(Addr).m_szBuffer);
	SetWindowText(m_Title);

	SetBkMode(m_hdc, TRANSPARENT);
	SetTextColor(m_hdc, 0x000033ff);
	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣:  OCX ����ҳӦ���� FALSE
}
LRESULT CCameraDlg::OnFrame(WPARAM wParam, LPARAM lParam)
{
	HDC hMdc = (HDC)wParam;
	BITMAP*pBmp = (BITMAP*)lParam;
	//��ʾͼ��
	BitBlt(m_hdc, m_dwOrgX,m_dwOrgY , m_dwWidth, m_dwHeight, hMdc, 0, 0, SRCCOPY);	
	m_dwFps++;

	if ((GetTickCount() - m_dwLastTime) >= 1000)
	{
		//����FPS
		CString sNewTitle;
		sNewTitle.Format(L"%s - [Fps: %d]",m_Title.GetBuffer(), m_dwFps);
		
		SetWindowText(sNewTitle);
		m_dwLastTime = GetTickCount();
		m_dwFps = 0;
	}
	return 0;
}

LRESULT CCameraDlg::OnError(WPARAM wParam, LPARAM lParam)
{
	//ʧ��
	WCHAR *szError = (WCHAR*)wParam;
	MessageBox(szError);
	//���ÿؼ�.
	CWnd*pCtrl = GetDlgItem(IDC_BUTTON1);
	pCtrl->EnableWindow(TRUE);
	return 0;
}

void CCameraDlg::OnClose()
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (m_pHandler){
		m_pHandler->Disconnect();
		m_pHandler = NULL;
	}

	if (m_hdc){
		::ReleaseDC(m_hWnd, m_hdc);
		m_hdc = NULL;
	}
}


void CCameraDlg::OnCbnSelchangeCombo1()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	if (m_DeviceList.m_hWnd &&m_VideoSizeList.m_hWnd)
	{
		while (m_VideoSizeList.GetCount())
			m_VideoSizeList.DeleteString(0);

		int idx = m_DeviceList.GetCurSel();
		POSITION pos = m_Devices.FindIndex(idx);
		DeviceInfo* pDevice = m_Devices.GetAt(pos);

		POSITION p = m_Devices.GetAt(pos)->m_pVideoSizes->GetHeadPosition();
		while (p)
		{
			m_VideoSizeList.AddString(pDevice->m_pVideoSizes->GetNext(p));
		}
		if (m_VideoSizeList.GetCount())
			m_VideoSizeList.SetCurSel(0);
	}
}

LRESULT CCameraDlg::OnVideoSize(WPARAM Width, LPARAM Height)
{
	//Start �ɹ���ʱ��
	CWnd*pCtrl = GetDlgItem(IDC_BUTTON1);
	pCtrl->SetWindowTextW(L"Stop");
	pCtrl->EnableWindow(TRUE);
	//
	m_dwWidth = Width;
	m_dwHeight = Height;

	m_dwOrgX = 0;
	m_dwOrgY = 45;
	//
	CRect rect;
	GetClientRect(rect);
	rect.top += 45;
	FillRect(m_hdc, rect, (HBRUSH)GetStockObject(WHITE_BRUSH));

	//���¼�����ʾ����.
	if (rect.Width() > m_dwWidth)
		m_dwOrgX = (rect.Width() - m_dwWidth) / 2;
	if (rect.Height() > m_dwHeight)
		m_dwOrgY = (rect.Height() - m_dwHeight) / 2 + 45;
	return 0;
}

LRESULT CCameraDlg::OnStopOk(WPARAM wParam, LPARAM lParam)
{
	CWnd*pCtrl = GetDlgItem(IDC_BUTTON1);
	pCtrl->SetWindowTextW(L"Start");
	pCtrl->EnableWindow(TRUE);
	return 0;
}

void CCameraDlg::OnBnClickedButton2()
{
	m_pHandler->ScreenShot();
}

LRESULT CCameraDlg::OnScreenShot(WPARAM wParam, LPARAM lParam)
{
	if (m_dwHeight == 0 || m_dwWidth == 0)
		return 0;
	//
	HBITMAP hBmp;
	BITMAP bmp;
	BITMAPINFOHEADER bi = { 0 };
	LPVOID Buffer = NULL;
	HDC hMdc;
	hMdc = CreateCompatibleDC(m_hdc);
	hBmp = CreateCompatibleBitmap(m_hdc, m_dwWidth, m_dwHeight);
	SelectObject(hMdc, hBmp);

	BitBlt(hMdc, 0, 0, m_dwWidth, m_dwHeight, m_hdc, m_dwOrgX, m_dwOrgY, SRCCOPY);

	GetObject(hBmp, sizeof(BITMAP), &bmp);

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmp.bmWidth;
	bi.biHeight = bmp.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 24;
	bi.biCompression = BI_RGB;

	DWORD dwBmpSize = ((bmp.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmp.bmHeight;
	DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	
	BITMAPFILEHEADER bmfHeader = { 0 };
	Buffer = malloc(dwBmpSize);

	int Result = GetDIBits(hMdc, hBmp, 0, bmp.bmHeight, Buffer, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

	bmfHeader.bfType = 0x4D42;
	bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
	bmfHeader.bfSize = dwSizeofDIB;

	CString FileName;
	CTime Time = CTime::GetTickCount();
	FileName.Format(L"%s.bmp", Time.Format(L"%Y-%m-%d_%H_%M_%S").GetBuffer());

	CFileDialog FileDlg(FALSE, L"*.bmp", FileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, L"bmp file(*.bmp)|*.bmp", this);
	if (IDOK == FileDlg.DoModal())
	{
		HANDLE hFile = CreateFile(FileDlg.GetPathName(), GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD dwWrite = 0;
			WriteFile(hFile, &bmfHeader, sizeof(bmfHeader), &dwWrite, NULL);
			WriteFile(hFile, &bi, sizeof(bi), &dwWrite, NULL);
			WriteFile(hFile, Buffer, dwBmpSize, &dwWrite, NULL);

			CloseHandle(hFile);
			MessageBox(L"Save bmp success!");
		}
		else
			MessageBox(L"Couldn't Write File!");
	}
	free(Buffer);
	DeleteObject(hBmp);
	DeleteDC(hMdc);	
	return 0;
}

void CCameraDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	//
	CRect rect;
	GetClientRect(rect);
	rect.top += 45;

	FillRect(m_hdc, rect, (HBRUSH)GetStockObject(WHITE_BRUSH));

	//���¼�����ʾ����.
	if (rect.Width() > m_dwWidth)
		m_dwOrgX = (rect.Width() - m_dwWidth) / 2;
	if (rect.Height() > m_dwHeight)
		m_dwOrgY = (rect.Height() - m_dwHeight) / 2 + 45;
}
