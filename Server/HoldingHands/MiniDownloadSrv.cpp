#include "stdafx.h"
#include "MiniDownloadSrv.h"
#include "MiniDownloadDlg.h"
#include "json\json.h"
#include "utils.h"

CMiniDownloadSrv::CMiniDownloadSrv(CManager*pManager) :
	CMsgHandler(pManager),
	m_DownloadFinished(FALSE),
	m_pDlg(nullptr)
{
}


CMiniDownloadSrv::~CMiniDownloadSrv()
{

}

void CMiniDownloadSrv::OnClose()
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
			if (!m_DownloadFinished)
				m_pDlg->PostMessage(WM_MNDD_ERROR, (WPARAM)TEXT("Disconnect."));
			m_pDlg = nullptr;
		}
	}
}
void CMiniDownloadSrv::OnOpen()
{
	//发送请求.获取文件大小.
	SendMsg(MNDD_GET_FILE_INFO, 0, 0);
	//
	m_pDlg = new CMiniDownloadDlg(this);
	if (!m_pDlg->Create(IDD_MNDD_DLG,CWnd::GetDesktopWindow())){
		Close();
		return;
	}
	m_pDlg->ShowWindow(SW_SHOW);
}

void CMiniDownloadSrv::OnReadMsg(WORD Msg,  DWORD dwSize, char*Buffer)
{
	switch (Msg)
	{
	case MNDD_FILE_INFO:
		OnFileInfo(Buffer);
		break;
	case MNDD_DOWNLOAD_RESULT:
		OnDownloadResult(Buffer);
		break;
	default:
		break;
	}
}

void CMiniDownloadSrv::OnWriteMsg(WORD Msg,DWORD dwSize, char*Buffer)
{

}

void CMiniDownloadSrv::OnDownloadResult(char*result)
{
	Json::Value root;
	if (!Json::Reader().parse(result, root)){
		m_pDlg->SendMessage(WM_MNDD_ERROR, (WPARAM)TEXT("Parse Json Failed"));
		Close();
		return;
	}
	
	if (root["code"] != "0"){
		TCHAR * err = convertGB2312ToUtf16(root["err"].asCString());
		m_pDlg->SendMessage(WM_MNDD_ERROR, (WPARAM)err);
		delete[] err;
		Close();
		return;
	}
	/*
	{
		"code" : "",
		"err" : "",
		"total_size" :
		"finished_size" :
	}
	*/
	LPVOID ArgList[3] = { 0 };
	ArgList[0] = (LPVOID)root["total_size"].asInt();
	ArgList[1] = (LPVOID)root["finished_size"].asInt();
	
	if (root["err"] == "finished"){
		ArgList[2] = (LPVOID)1;
	}

	m_pDlg->SendMessage(WM_MNDD_DOWNLOAD_RESULT,
		sizeof(ArgList) / sizeof(ArgList[0]), (LPARAM)ArgList);
	//continue download....
	if (!ArgList[2]){
		SendMsg(MNDD_DOWNLOAD_CONTINUE, 0, 0);
	}
	else{
		m_DownloadFinished = TRUE;
		SendMsg(MNDD_DOWNLOAD_END, 0, 0);
	}
}

void CMiniDownloadSrv::OnFileInfo(char*fileinfo)
{
	Json::Value root;
	if (!Json::Reader().parse(fileinfo, root)){
		m_pDlg->SendMessage(WM_MNDD_ERROR, (WPARAM)TEXT("Parse Json Failed"));
		Close();
		return;
	}

	/*
	{
		"code" : ,
		"err" : "",
		"filename" : "",
		"url" : "",
		"filesize": -1,		//-1 表示未知
	}
	*/
	if (root["code"] != "0"){
		//失败了....,结束...
		TCHAR * err = convertGB2312ToUtf16(root["err"].asCString());
		m_pDlg->SendMessage(WM_MNDD_ERROR, (WPARAM)err);
		delete[] err;
		Close();
		return;
	}
	//
	LPVOID ArgList[3];
	ArgList[0] = (LPVOID)root["filename"].asCString();
	ArgList[1] = (LPVOID)root["url"].asCString();
	ArgList[2] = (LPVOID)root["filesize"].asInt();

	m_pDlg->SendMessage(WM_MNDD_FILEINFO,
		sizeof(ArgList) / sizeof(ArgList[0]), (LPARAM)ArgList);

	if (root["filesize"].asInt() != 0){
		SendMsg(MNDD_DOWNLOAD_CONTINUE, 0, 0);
	}
}