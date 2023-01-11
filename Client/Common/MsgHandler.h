#pragma once
#include <WinSock2.h>
#include <MSWSock.h>
/*
	重写下面三个函数;
	virtual void OnClose() = 0;
	virtual void OnReadPartial(CPacket*pPacket) = 0;
	virtual void OnReadComplete(CPacket*pPacket) = 0;
*/
#include <string>

using std::pair;
using std::string;

class CIOCPClient;
class CMsgHandler
{
protected:
	friend class CIOCPClient;
	friend class CManager;
	//Handler不应清理这两个.应该由它自己清理自己;
	DWORD		m_Identity;				//用来标识不同的功能;
	CManager	*m_pManager;
public:
	//所有模块应该在该函数里面就把相关资源清理调.析构函数可能不会被调用
	virtual void OnClose() = 0;					//当socket断开的时候调用这个函数
	virtual void OnOpen() = 0;				//当socket连接的时候调用这个函数

	//有数据到达的时候调用这两个函数.
	virtual void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer) = 0;
	//有数据发送完毕后调用这两个函数
	virtual void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer) = 0;

public:
	BOOL	SendMsg(WORD Msg, void* data, int len, BOOL Compress = TRUE);
	void	Close();
	//这些接口该不该给????

	const pair<string, unsigned short> GetPeerName();
	const pair<string, unsigned short> GetSockName();

	CMsgHandler(CManager*pManager, DWORD dwIdentity);

	virtual ~CMsgHandler();
};

