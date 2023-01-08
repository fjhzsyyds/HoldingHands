// CameraDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "CameraDlg.h"
#include "afxdialogex.h"
#include "CameraSrv.h"
#include <string>
#include "MainFrm.h"
#include "Resource.h"
#include "json\json.h"
#include "utils.h"


#ifdef DEBUG
#pragma comment(lib,"jsond.lib")
#else
#pragma comment(lib,"json.lib")
#endif
using std::string;


// CCameraDlg 对话框

IMPLEMENT_DYNAMIC(CCameraDlg, CDialogEx)

CCameraDlg::CCameraDlg(CCameraSrv*pHandler, CWnd* pParent /*=NULL*/)
: CDialogEx(CCameraDlg::IDD, pParent),
	m_pHandler(pHandler),
	m_hdc(NULL),
	m_dwHeight(0),
	m_dwWidth(0),
	m_dwFps(0),
	m_dwLastTime(0),
	m_dwOrgX(0),
	m_dwOrgY(0),
	m_DestroyAfterDisconnect(FALSE)
{
}

CCameraDlg::~CCameraDlg()
{
}

void CCameraDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_DeviceList);
	DDX_Control(pDX, IDC_COMBO2, m_VideoSizeList);
	DDX_Control(pDX, IDC_COMBO3, m_FormatList);
	DDX_Control(pDX, IDC_COMBO4, m_BitCount);
}


BEGIN_MESSAGE_MAP(CCameraDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &CCameraDlg::OnBnClickedButton1)
	ON_MESSAGE(WM_CAMERA_DEVICELIST,OnDeviceList)
	ON_MESSAGE(WM_CAMERA_ERROR,OnError)
	ON_MESSAGE(WM_CAMERA_FRAME,OnFrame)
	ON_MESSAGE(WM_CAMERA_VIDEOSIZE,OnVideoSize)
	ON_MESSAGE(WM_CAMERA_STOP_OK,OnStopOk)
	ON_WM_CLOSE()
	ON_CBN_SELCHANGE(IDC_COMBO1, &CCameraDlg::OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDC_BUTTON2, &CCameraDlg::OnBnClickedButton2)
	ON_WM_SIZE()
	ON_CBN_SELCHANGE(IDC_COMBO3, &CCameraDlg::OnCbnSelchangeCombo3)
	ON_CBN_SELCHANGE(IDC_COMBO4, &CCameraDlg::OnCbnSelchangeCombo4)
END_MESSAGE_MAP()


// CCameraDlg 消息处理程序



void CCameraDlg::OnBnClickedButton1()
{
	CString Text;
	CWnd*pCtrl = GetDlgItem(IDC_BUTTON1);
	pCtrl->GetWindowTextW(Text);

	//
	if (Text == TEXT("Start")){
		if (m_DeviceList.GetCount() == 0 ||
			m_FormatList.GetCount() == 0 ||
			m_BitCount.GetCount() == 0 ||
			m_VideoSizeList.GetCount() == 0){
			return;
		}
		//get device name..
		CString deivce;
		DWORD dwFormat;
		DWORD dwBitCount;
		pair<int, int> size;

		m_DeviceList.GetWindowText(deivce);
		dwFormat = m_FormatList.GetItemData(m_FormatList.GetCurSel());
		dwBitCount = m_BitCount.GetItemData(m_BitCount.GetCurSel());
		size = *(pair<int, int>*)m_VideoSizeList.GetItemData(m_VideoSizeList.GetCurSel());
		//
		if (m_pHandler)
			m_pHandler->Start(CW2A(deivce).m_psz, dwFormat, dwBitCount, size.first, size.second);
	}
	else if(m_pHandler){
		m_pHandler->Stop();
	}

	pCtrl->EnableWindow(FALSE);
}

LRESULT CCameraDlg::OnDeviceList(WPARAM wParam, LPARAM lParam)
{
	string json_res = (char*)wParam;
	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(json_res, root)){
		MessageBox(TEXT("Parse Json Failed!"));
		return 0;
	}
	//解析数据..
	Json::Value::Members members = root.getMemberNames();
	for (auto it = members.begin(); it != members.end(); it++){
		map<DWORD, map<DWORD,list<pair<int, int>>>> mp;

		string device_name = *it;
		Json::Value::Members cmpr_members = root[device_name].getMemberNames();
		for (auto it2 = cmpr_members.begin(); it2 != cmpr_members.end(); it2++){
			string compress = *it2;
			Json::Value::Members bit_members = root[device_name][compress].getMemberNames();
			
			map<DWORD, list<pair<int, int>>> bitmp;
			for (auto it3 = bit_members.begin(); it3 != bit_members.end(); it3++){
				string bitcount = *it3;
				list <pair<int, int>> vs;
				for (auto&size : root[device_name][compress][bitcount]){
					vs.push_back(pair<int, int>(size["width"].asInt(), size["height"].asInt()));
				}
				DWORD dwBitCount = atoi(bitcount.c_str());
				bitmp[dwBitCount] = std::move(vs);
			}
			DWORD dwCompression = atoi(compress.c_str());
			mp[dwCompression] = std::move(bitmp);
		}
		m_device[device_name] = std::move(mp);
	}
	//
	//device_list 是不会变的.
	for (auto &device_name : m_device){
		m_DeviceList.AddString(CA2W(device_name.first.c_str()));
	}
	//
	if (m_DeviceList.GetCount()){
		m_DeviceList.SetCurSel(0);
		OnCbnSelchangeCombo1();
	}
	return 0;
}

BOOL CCameraDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	CWnd* pVideo = GetDlgItem(IDC_VIDEO);
	pVideo->GetWindowRect(m_minVideoSize);
	m_hdc = ::GetDC(pVideo->m_hWnd);
	auto const peer = m_pHandler->GetPeerName();

	m_Title.Format(TEXT("[%s] Camera"), CA2W(peer.first.c_str()).m_psz);
	SetWindowText(m_Title);

	SetTextColor(m_hdc, 0x000033ff);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常:  OCX 属性页应返回 FALSE
}
LRESULT CCameraDlg::OnFrame(WPARAM wParam, LPARAM lParam)
{
	HDC hMdc = (HDC)wParam;
	BITMAP*pBmp = (BITMAP*)lParam;
	//显示图像
	BitBlt(m_hdc, m_dwOrgX,m_dwOrgY , m_dwWidth, m_dwHeight, hMdc, 0, 0, SRCCOPY);	
	m_dwFps++;

	if ((GetTickCount() - m_dwLastTime) >= 1000){
		//更新FPS
		CString csNewTitle;
		csNewTitle.Format(TEXT("%s - [Fps: %d]"), m_Title.GetBuffer(), m_dwFps);
		
		SetWindowText(csNewTitle);
		m_dwLastTime = GetTickCount();
		m_dwFps = 0;
	}
	return 0;
}

LRESULT CCameraDlg::OnError(WPARAM wParam, LPARAM lParam)
{
	//启用控件.
	GetDlgItem(IDC_BUTTON1)->EnableWindow(TRUE);

	//失败
	TCHAR*szError = (TCHAR*)wParam;
	MessageBox(szError,TEXT("Error"),MB_OK|MB_ICONINFORMATION);
	return 0;
}



void CCameraDlg::PostNcDestroy()
{
	// TODO:  在此添加专用代码和/或调用基类

	CDialogEx::PostNcDestroy();

	if (!m_DestroyAfterDisconnect){
		delete this;
	}
}


void CCameraDlg::OnClose()
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (m_hdc){
		::ReleaseDC(GetDlgItem(IDC_VIDEO)->m_hWnd, m_hdc);
		m_hdc = NULL;
	}
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (m_pHandler){
		m_DestroyAfterDisconnect = TRUE;
		m_pHandler->Close();
	}
	else{
		//m_pHandler已经没了,现在只管自己就行.
		DestroyWindow();
	}
}


//BitCount选定内容改变了，在这里更新 VideoSize.
void CCameraDlg::OnCbnSelchangeCombo4()
{
	while (m_VideoSizeList.GetCount()){
		m_VideoSizeList.DeleteString(0);
	}

	// TODO:  在此添加控件通知处理程序代码
	CString csDeviceName;
	DWORD dwFormat,dwBitCount;
	m_DeviceList.GetWindowTextW(csDeviceName);
	dwFormat = m_FormatList.GetItemData(m_FormatList.GetCurSel());
	dwBitCount = m_BitCount.GetItemData(m_BitCount.GetCurSel());
	//
	auto & l = m_device[CW2A(csDeviceName).m_psz][dwFormat][dwBitCount];
	for (auto & size : l){
		CString csSize;
		int idx = m_VideoSizeList.GetCount();
		csSize.Format(TEXT("%d x %d"), size.first, size.second);
		m_VideoSizeList.InsertString(idx, csSize);
		m_VideoSizeList.SetItemData(idx, (DWORD)&size);
	}

	if (m_VideoSizeList.GetCount()){
		m_VideoSizeList.SetCurSel(0);
	}
}



//格式变了..,在这里要更新 BitCount的内容,
void CCameraDlg::OnCbnSelchangeCombo3()
{
	//
	CString csDeviceName;
	DWORD dwFormat;
	m_DeviceList.GetWindowTextW(csDeviceName);
	dwFormat = m_FormatList.GetItemData(m_FormatList.GetCurSel());
	//
	auto & l = m_device[CW2A(csDeviceName).m_psz][dwFormat];

	//l 是 BitCount -> Size;
	while (m_BitCount.GetCount()){
		m_BitCount.DeleteString(0);
	}

	for (auto & bit_mp : m_device[CW2A(csDeviceName).m_psz][dwFormat]){
		DWORD bitcount = bit_mp.first;
		CString strBitCount;
		strBitCount.Format(TEXT("%d"), bitcount);
		
		int idx = m_BitCount.GetCount();
		m_BitCount.InsertString(idx, strBitCount);
		m_BitCount.SetItemData(idx, bitcount);
	}



	if (m_BitCount.GetCount() > 0){
		m_BitCount.SetCurSel(0);	//默认选中第0项
		OnCbnSelchangeCombo4();		//除发事件.
	}
}

//设备选项变了....
void CCameraDlg::OnCbnSelchangeCombo1()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_DeviceList.m_hWnd &&m_VideoSizeList.m_hWnd){
		//删除format和video+_size;
		while (m_FormatList.GetCount()){
			m_FormatList.DeleteString(0);
		}
		CString csDeviceName;
		m_DeviceList.GetWindowTextW(csDeviceName);

		auto & device = m_device[CW2A(csDeviceName).m_psz];
		int idx = 0;
		for (auto & format : device){
			CString Format;
			char * name;
			unsigned long long ullFormat = format.first;
			switch (ullFormat)
			{
			case 0:
				Format = TEXT("RGB");
				break;
			default:
				name = (char*)& ullFormat;
				Format = name;
				break;
			}
			idx = m_FormatList.GetCount();
			m_FormatList.InsertString(idx, Format);
			m_FormatList.SetItemData(idx, ullFormat);
		}
		if (m_FormatList.GetCount()){
			m_FormatList.SetCurSel(0);
			OnCbnSelchangeCombo3();				//
		}
	}
}

LRESULT CCameraDlg::OnVideoSize(WPARAM Width, LPARAM Height)
{
	//Start 恢复按钮.
	CWnd*pCtrl = GetDlgItem(IDC_BUTTON1);
	CWnd* pVideo = GetDlgItem(IDC_VIDEO);

	pCtrl->SetWindowTextW(L"Stop");
	pCtrl->EnableWindow(TRUE);
	
	//保存视频宽高.
	m_dwWidth = Width;
	m_dwHeight = Height;
	//
	CRect rect;
	pVideo->GetWindowRect(rect);
	ScreenToClient(rect);

	//调整显示位置.
	if (Width < m_minVideoSize.Width()){
		//视频宽高比窗口的最小尺寸还要小.
		rect.right = rect.left + m_minVideoSize.Width();
		m_dwOrgX = (m_minVideoSize.Width() - Width) / 2;
	}else{
		rect.right = rect.left + Width;
		m_dwOrgX = 0;
	}

	if (Height < m_minVideoSize.Height()){
		//视频宽高比窗口的最小尺寸还要小.
		rect.bottom = rect.top + m_minVideoSize.Height();
		m_dwOrgY = (m_minVideoSize.Height() - Height) / 2;
	}
	else{
		rect.bottom = rect.top + Height;
		m_dwOrgY = 0;
	}
	pVideo->MoveWindow(rect);
	//调整其他窗口的位置.
	CRect groupRect;
	CWnd  * group = GetDlgItem(IDC_OPERATION);
	group->GetWindowRect(groupRect);
	ScreenToClient(groupRect);

	groupRect.bottom = rect.bottom;
	group->MoveWindow(groupRect);
	
	//调整主窗口大小.
	CRect dlgRect;
	GetWindowRect(dlgRect);
	ClientToScreen(rect);
	dlgRect.right = rect.right + 18;
	dlgRect.bottom = rect.bottom + 18;
	MoveWindow(dlgRect);
	
	//
	pVideo->GetClientRect(rect);
	FillRect(m_hdc, rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
	return 0;
}

LRESULT CCameraDlg::OnStopOk(WPARAM wParam, LPARAM lParam)
{
	CWnd*pCtrl = GetDlgItem(IDC_BUTTON1);
	pCtrl->SetWindowTextW(TEXT("Start"));
	pCtrl->EnableWindow(TRUE);
	return 0;
}

void CCameraDlg::OnBnClickedButton2()
{
	DWORD dwBmpSize;
	LPVOID Buffer = NULL;

	if (m_dwHeight == 0 || m_dwWidth == 0 || m_pHandler == NULL)
		return;

	Buffer = m_pHandler->GetBmpFile(&dwBmpSize);

	if (!Buffer){
		return;
	}

	CString FileName;
	CTime Time = CTime::GetTickCount();
	FileName.Format(TEXT("\\%s.bmp"), Time.Format(L"%Y-%m-%d_%H_%M_%S").GetBuffer());

	CMainFrame * pMainWnd = (CMainFrame*)AfxGetMainWnd();
	CConfig & config = pMainWnd->getConfig();
	const string & val = config.getconfig("cam", "screenshot_save_path");
	CString SavePath = CA2W(val.c_str());

	SavePath += FileName;

	if (SavePath[1] != ':'){
		CString csCurrentDir;
		csCurrentDir.Preallocate(MAX_PATH);
		GetCurrentDirectory(MAX_PATH, csCurrentDir.GetBuffer());
		SavePath = csCurrentDir + "\\" + SavePath;
	}

	MakesureDirExist(SavePath, TRUE);
	HANDLE hFile = CreateFile(SavePath, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	CString err;
	if (hFile == INVALID_HANDLE_VALUE){
		err.Format(TEXT("CreateFile failed with error: %d"), GetLastError());
	}

	if (hFile != INVALID_HANDLE_VALUE){
		DWORD dwWrite = 0;
		WriteFile(hFile, Buffer, dwBmpSize, &dwWrite, NULL);
		CloseHandle(hFile);

		err.Format(TEXT("Image has been save to %s"), SavePath);
	}

	delete[] Buffer;
	MessageBox(err, TEXT("Tips"), MB_OK | MB_ICONINFORMATION);
}


void CCameraDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
}





void CCameraDlg::OnOK()
{
}


void CCameraDlg::OnCancel()
{
}
