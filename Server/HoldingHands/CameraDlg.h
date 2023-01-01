#pragma once

#include <string>
#include <map>
#include <list>
#include "afxwin.h"


using std::string;
using std::map;
using std::list;
using std::pair;


#define WM_CAMERA_DEVICELIST (WM_USER+352)
#define WM_CAMERA_ERROR		(WM_USER+353)
#define WM_CAMERA_FRAME		(WM_USER+354)
#define WM_CAMERA_VIDEOSIZE (WM_USER+355)
#define WM_CAMERA_SCREENSHOT (WM_USER+356)
#define WM_CAMERA_STOP_OK	(WM_USER + 357)

// CCameraDlg �Ի���

class CCameraSrv;
typedef map <string, map<DWORD, map<DWORD,list<pair<int, int>>>>> VideoInfoList;
class CCameraDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CCameraDlg)

public:
	CCameraDlg(CCameraSrv*pHandler,CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CCameraDlg();

// �Ի�������
	enum { IDD = IDD_CAM_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	HDC			m_hdc;
	DWORD		m_dwFps;
	DWORD		m_dwLastTime;

	DWORD		m_dwWidth;
	DWORD		m_dwHeight;

	DWORD		m_dwOrgX;
	DWORD		m_dwOrgY;

	CRect		m_minVideoSize;

	CString m_Title;

	CCameraSrv*	m_pHandler;

	CComboBox m_FormatList;
	CComboBox m_DeviceList;
	CComboBox m_VideoSizeList;

	VideoInfoList		m_device;

	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButton1();

	afx_msg LRESULT OnDeviceList(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnFrame(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnError(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnVideoSize(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnStopOk(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnScreenShot(WPARAM wParam, LPARAM lParam);

	virtual BOOL OnInitDialog();
	afx_msg void OnClose();

	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	
	afx_msg void OnCbnSelchangeCombo3();
	CComboBox m_BitCount;
	afx_msg void OnCbnSelchangeCombo4();
};
