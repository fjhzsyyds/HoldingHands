#include "stdafx.h"
#include "FileManagerSrv.h"
#include "FileManagerDlg.h"

CFileManagerSrv::CFileManagerSrv(CManager*pManager):
	CMsgHandler(pManager)
{
	m_pDlg = NULL;
}


CFileManagerSrv::~CFileManagerSrv()
{
}

void CFileManagerSrv::OnClose()
{
	if (m_pDlg){
		m_pDlg->SendMessage(WM_CLOSE);
		m_pDlg->DestroyWindow();

		delete m_pDlg;
		m_pDlg = NULL;
	}
}
void CFileManagerSrv::OnOpen()
{
	m_pDlg = new CFileManagerDlg(this);

	if (FALSE == m_pDlg->Create(IDD_FM_DLG, CWnd::GetDesktopWindow())){
		Close();
		return;
	}
	m_pDlg->ShowWindow(SW_SHOW);
	//禁用窗口,请求目录
	m_pDlg->GetDlgItem(IDC_EDIT1)->EnableWindow(FALSE);
	m_pDlg->m_FileList.EnableWindow(FALSE);
	//修改目录
	ChDir(m_pDlg->m_Location.GetBuffer());
}


void CFileManagerSrv::OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer)
{
	switch (Msg)
	{
	case FILE_MGR_CHDIR_RET:
		OnChangeDirRet(dwSize, Buffer);
		break;
	case FILE_MGR_GETLIST_RET:
		OnGetListRet(dwSize, Buffer);
		break;
		//prev 
	case FILE_MGR_PREV_DOWNLOAD:
		m_pDlg->SendMessage(WM_FMDLG_PREV_DOWNLOAD, 0, 0);
		break;
	case FILE_MGR_PREV_UPLOADFROMDISK:
		m_pDlg->SendMessage(WM_FMDLG_PREV_UPLOAD_FROMDISK, 0, 0);
		break;
	case FILE_MGR_PREV_UPLOADFRURL:
		m_pDlg->SendMessage(WM_FMDLG_PREV_UPLOAD_FROMURL, 0, 0);
		break;
	case FILE_MGR_PREV_NEWFOLDER:
		m_pDlg->SendMessage(WM_FMDLG_PREV_NEWFOLDER, 0, 0);
		break;
	case FILE_MGR_PREV_RENAME:
		m_pDlg->SendMessage(WM_FMDLG_PREV_RENAME, 0, 0);
		break;
	default:
		break;
	}
}

void CFileManagerSrv::OnWriteMsg(WORD Msg,DWORD dwSize, char*Buffer)
{

}

void CFileManagerSrv::OnChangeDirRet(DWORD dwRead, char*buffer)
{
	//成功
	if (buffer[0])
		SendMsg(FILE_MGR_GETLIST, 0, 0);
	//修改目录结果.
	m_pDlg->SendMessage(WM_FMDLG_CHDIR, buffer[0], (LPARAM)&buffer[1]);
}

void CFileManagerSrv::OnGetListRet(DWORD dwRead, char*buffer){
	//没什么要做的.直接丢给窗口去处理.
	m_pDlg->SendMessage(WM_FMDLG_LIST, dwRead, (LPARAM)buffer);
}

void CFileManagerSrv::Search(){
	SendMsg(FILE_MGR_SEARCH, 0, 0);
}
void CFileManagerSrv::Up(){
	SendMsg(FILE_MGR_UP, 0, 0);
}

void CFileManagerSrv::Refresh(){
	SendMsg(FILE_MGR_REFRESH, 0, 0);
}
void CFileManagerSrv::NewFolder(WCHAR*szName){
	SendMsg(FILE_MGR_NEWFOLDER, (char*)szName, sizeof(WCHAR)*(wcslen(szName) + 1));
}

void CFileManagerSrv::Rename(WCHAR*szNames){
	SendMsg(FILE_MGR_RENAME, (char*)szNames, sizeof(WCHAR)*(wcslen(szNames) + 1));
}

void CFileManagerSrv::ChDir(WCHAR*szNewDir){
	SendMsg(FILE_MGR_CHDIR, (char*)szNewDir, sizeof(WCHAR)*(wcslen(szNewDir) + 1));
}

void CFileManagerSrv::Delete(WCHAR*szFileList){
	SendMsg(FILE_MGR_DELETE, (char*)szFileList, sizeof(WCHAR)*(wcslen(szFileList) + 1));
}

void CFileManagerSrv::Copy(WCHAR*szFileList){
	SendMsg(FILE_MGR_COPY, (char*)szFileList, sizeof(WCHAR)*(wcslen(szFileList) + 1));
}
void CFileManagerSrv::Cut(WCHAR*szFileList){
	SendMsg(FILE_MGR_CUT, (char*)szFileList, sizeof(WCHAR)*(wcslen(szFileList) + 1));
}
void CFileManagerSrv::Paste(){
	SendMsg(FILE_MGR_PASTE, 0, 0);
}


void CFileManagerSrv::UploadFromUrl(WCHAR*szUrl){
	SendMsg(FILE_MGR_UPLOADFRURL, (char*)szUrl, sizeof(WCHAR) * (wcslen(szUrl) + 1));
}

void CFileManagerSrv::UploadFromDisk(WCHAR*szFileList){
	SendMsg(FILE_MGR_UPLOADFROMDISK, (char*)szFileList, sizeof(WCHAR) * (wcslen(szFileList) + 1));
}

void CFileManagerSrv::Download(WCHAR*szFileList){
	SendMsg(FILE_MGR_DOWNLOAD, (char*)szFileList, sizeof(WCHAR) * (wcslen(szFileList) + 1));
}

void CFileManagerSrv::RunFileNormal(WCHAR*szFileList){
	SendMsg(FILE_MGR_RUNFILE_NORMAL, (char*)szFileList, sizeof(WCHAR)*(wcslen(szFileList) + 1));
}
void CFileManagerSrv::RunFileHide(WCHAR*szFileList){
	SendMsg(FILE_MGR_RUNFILE_HIDE, (char*)szFileList, sizeof(WCHAR)*(wcslen(szFileList) + 1));
}

void CFileManagerSrv::PrevUploadFromUrl(){
	SendMsg(FILE_MGR_PREV_UPLOADFRURL, 0, 0);
}
void CFileManagerSrv::PrevUploadFromDisk(){
	SendMsg(FILE_MGR_PREV_UPLOADFROMDISK, 0, 0);
}
void CFileManagerSrv::PrevDownload(){
	SendMsg(FILE_MGR_PREV_DOWNLOAD, 0, 0);
}
void CFileManagerSrv::PrevRename(){
	SendMsg(FILE_MGR_PREV_RENAME, 0, 0);
}
void CFileManagerSrv::PrevNewFolder(){
	SendMsg(FILE_MGR_PREV_NEWFOLDER, 0, 0);
}