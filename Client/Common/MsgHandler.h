#pragma once
#include <WinSock2.h>
#include <MSWSock.h>
#include <string>
#include"IOCPClient.h"

using std::pair;
using std::string;

class CIOCPClient;
class CMsgHandler
{
protected:
	friend class CIOCPClient;
	DWORD		 m_Identity;				//用来标识不同的功能;
	CIOCPClient	*m_pClient;

private:
	void OnRead(BYTE* lpData, UINT Size);
	void OnWrite(BYTE*lpData, UINT Size);
	void OnConncet();
	void OnDisconnect();

public:
	//所有模块应该在该函数里面就把相关资源清理调.析构函数可能不会被调用
	virtual void OnClose() = 0;					//当socket断开的时候调用这个函数
	virtual void OnOpen() = 0;				//当socket连接的时候调用这个函数

	//有数据到达的时候调用这两个函数.
	virtual void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer) = 0;
	
	//有数据发送完毕后调用这两个函数
	virtual void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer) = 0;

public:
	BOOL BeginMsg(WORD Msg);
	void EndMsg(BOOL Compress = TRUE);

	void WriteByte(BYTE value){
		m_pClient->WriteBytes(&value, sizeof(BYTE));
	}
	void WriteWord(WORD value){
		m_pClient->WriteBytes((BYTE*)&value, sizeof(WORD));
	}
	void WriteDword(DWORD value){
		m_pClient->WriteBytes((BYTE*)&value, sizeof(DWORD));
	}
	void WriteBytes(BYTE*lpData, UINT Size){
		m_pClient->WriteBytes(lpData, Size);
	}


	

	BOOL	SendMsg(WORD Msg, void* data, int len, BOOL Compress = TRUE);

	void	Close();

	//这些接口该不该给????
	const pair<string, unsigned short> GetPeerName();
	const pair<string, unsigned short> GetSockName();

	CMsgHandler(CIOCPClient*pClient, DWORD dwIdentity);

	virtual ~CMsgHandler();
};

