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
		if (m_pDlg->m_DestroyAfterDisconnect){
			//窗口先关闭的.
			m_pDlg->DestroyWindow();
			delete m_pDlg;
		}
		else{
			// pHandler先关闭的,那么就不管窗口了
			m_pDlg->m_pHandler = nullptr;
			m_pDlg->PostMessage(WM_FMDLG_ERROR,(WPARAM)TEXT("Disconnect."));
			m_pDlg = nullptr;
		}
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
	case FILE_MGR_NEW_FOLDER_SUCCESS:
		m_pDlg->SendMessage(WM_FMDLG_NEWFOLDER_SUCCESS, (WPARAM)Buffer, 0);
		break;
	case FILE_MGR_ERROR:
		m_pDlg->SendMessage(WM_FMDLG_ERROR, (WPARAM)Buffer);
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
void CFileManagerSrv::NewFolder(){
	SendMsg(FILE_MGR_NEWFOLDER, 0 , 0);
}

void CFileManagerSrv::Rename(TCHAR*szNames){
	SendMsg(FILE_MGR_RENAME, (char*)szNames, sizeof(TCHAR)*(lstrlen(szNames) + 1));
}

void CFileManagerSrv::ChDir(TCHAR*szNewDir){
	SendMsg(FILE_MGR_CHDIR, (char*)szNewDir, sizeof(TCHAR)*(lstrlen(szNewDir) + 1));
}

void CFileManagerSrv::Delete(TCHAR*szFileList){
	SendMsg(FILE_MGR_DELETE, (char*)szFileList, sizeof(TCHAR)*(lstrlen(szFileList) + 1));
}

void CFileManagerSrv::Copy(TCHAR*szFileList){
	SendMsg(FILE_MGR_COPY, (char*)szFileList, sizeof(TCHAR)*(lstrlen(szFileList) + 1));
}
void CFileManagerSrv::Cut(TCHAR*szFileList){
	SendMsg(FILE_MGR_CUT, (char*)szFileList, sizeof(TCHAR)*(lstrlen(szFileList) + 1));
}
void CFileManagerSrv::Paste(){
	SendMsg(FILE_MGR_PASTE, 0, 0);
}


void CFileManagerSrv::UploadFromUrl(TCHAR*szUrl){
	SendMsg(FILE_MGR_UPLOADFRURL, (char*)szUrl, sizeof(TCHAR) * (lstrlen(szUrl) + 1));
}

void CFileManagerSrv::UploadFromDisk(TCHAR*szFileList){
	SendMsg(FILE_MGR_UPLOADFROMDISK, (char*)szFileList, sizeof(TCHAR) * (lstrlen(szFileList) + 1));
}

void CFileManagerSrv::Download(TCHAR*szFileList){
	SendMsg(FILE_MGR_DOWNLOAD, (char*)szFileList, sizeof(TCHAR) * (lstrlen(szFileList) + 1));
}

void CFileManagerSrv::RunFileNormal(TCHAR*szFileList){
	SendMsg(FILE_MGR_RUNFILE_NORMAL, (char*)szFileList, sizeof(TCHAR)*(lstrlen(szFileList) + 1));
}
void CFileManagerSrv::RunFileHide(TCHAR*szFileList){
	SendMsg(FILE_MGR_RUNFILE_HIDE, (char*)szFileList, sizeof(TCHAR)*(lstrlen(szFileList) + 1));
}

