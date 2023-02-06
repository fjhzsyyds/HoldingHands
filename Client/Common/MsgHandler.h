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
	DWORD		 m_Identity;				//������ʶ��ͬ�Ĺ���;
	CIOCPClient	*m_pClient;

private:
	void OnRead(BYTE* lpData, UINT Size);
	void OnWrite(BYTE*lpData, UINT Size);
	void OnConncet();
	void OnDisconnect();

public:
	//����ģ��Ӧ���ڸú�������Ͱ������Դ�����.�����������ܲ��ᱻ����
	virtual void OnClose() = 0;					//��socket�Ͽ���ʱ������������
	virtual void OnOpen() = 0;				//��socket���ӵ�ʱ������������

	//�����ݵ����ʱ���������������.
	virtual void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer) = 0;
	
	//�����ݷ�����Ϻ��������������
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

	//��Щ�ӿڸò��ø�????
	const pair<string, unsigned short> GetPeerName();
	const pair<string, unsigned short> GetSockName();

	CMsgHandler(CIOCPClient*pClient, DWORD dwIdentity);

	virtual ~CMsgHandler();
};

