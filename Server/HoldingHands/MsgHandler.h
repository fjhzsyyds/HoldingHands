#pragma once
#include <afx.h>
#include <WinSock2.h>
#include <MSWSock.h>

#include <string>
using std::string;
using std::pair;

class CManager;
class CClientContext;
class CMsgHandler
{
protected:
	friend class CManager;
	friend class CClientContext;
	friend class CIOCPServer;

	CManager		*m_pManager;
	//所有模块应该在该函数里面就把相关资源清理调.析构函数可能不会被调用
	virtual void OnClose() = 0;					//当socket断开的时候调用这个函数
	virtual void OnOpen() = 0;				//当socket连接的时候调用这个函数

	//有数据到达的时候调用这两个函数.
	virtual void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer) = 0;
	//有数据发送完毕后调用这两个函数
	virtual void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer) = 0;

public:
	BOOL	SendMsg(WORD Msg, void*data, int len);
	void	Close();
	//这些接口该不该给????

	const pair<string,unsigned short> GetPeerName();
	const pair<string, unsigned short> GetSockName();

	CMsgHandler(CManager*pManager);
	virtual ~CMsgHandler();
};