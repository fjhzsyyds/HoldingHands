#pragma once
#include <afx.h>
#include <winsock2.h>
#include <MSWSock.h>
#include "Packet.h"


#define REMOTE_AND_LOCAL_ADDR_LEN		(2 * (sizeof(sockaddr)+16))

class CManager;
class CPacket;
class CMsgHandler;

class CClientContext
{
private:
	friend class	CIOCPServer;
	//
	CIOCPServer*	m_pServer;
	CManager *		m_pManager;
	CMsgHandler*	m_pHandler;

	//Handler
	DWORD			m_Identity;								//功能
	//
	SOCKET			m_ClientSocket;	
	//与接受有关的buffer
	WSABUF			m_wsaReadBuf;							//每次投递请求时用于储存数据信息
	CPacket			m_ReadPacket;							//每次投递请求时用于储存数据信息	
	DWORD			m_dwRead;								//已经读取的数据
	//与发送有关的buffer
	WSABUF			m_wsaWriteBuf;
	CPacket			m_WritePacket;
	DWORD			m_dwWrite;								//已经发送的长度
	//
	POSITION		m_PosInList;							//保存在client list 中 context 的位置
	//sockaddr_in		m_sockaddr;
	CRITICAL_SECTION m_cs;								//防止重复关闭socket
	HANDLE			 m_SendPacketComplete;					//是否发送完毕
	//
	DWORD			 m_LastHeartBeat;

	//投递一个ReadIO请求
	void PostRead();
	//投递一个WriteIO请求
	void PostWrite();

	void OnReadComplete(DWORD nTransferredBytes, DWORD dwFailedReason);
	void OnWriteComplete(DWORD nTransferredBytes, DWORD dwFailedReason);
	void OnClose();
	void OnConnect();

public:
	//断开连接
	void Disconnect();

	SOCKET		GetSocket(){
		return m_ClientSocket;
	}

	DWORD		GetIdentity(){
		return m_Identity;
	}

	void SetCallbackHandler(CMsgHandler*pHandler){
		m_pHandler = pHandler;
	}

	CMsgHandler* GetCallbackHandler(){
		return m_pHandler;
	}

	//Send Data...
	BOOL	BeginWrite();
	void	WriteBytes(BYTE*lpData, UINT Size);
	void	EndWrite(BOOL bCompress = TRUE);
	
	//Send Data.....
	BOOL	Send(BYTE*lpData, UINT Size, BOOL bCompress = TRUE);

	CClientContext(SOCKET ClientSocket,CIOCPServer *pServer,CManager*pManager);
	~CClientContext();
};

