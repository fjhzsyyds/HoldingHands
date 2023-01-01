#pragma once
#include <winsock2.h>
#include <MSWSock.h>

#define MAX_BUFFER_SIZE (32*1024*1024)
#define PACKET_CHECK					0x534E4942

//��16λ��¼����ID����16λ��¼ÿһ�����������֪ͨ

#define MakeRemoteCommand(Identity,Event)	((Identity<<16)|(Event&0x0000FFFF))
#define GetCommandEvent(cmd)				(cmd&0x0000FFFF)

/*
struct CPacketHeader
{
DWORD			m_dwCheck;						//����У��
DWORD			m_dwBufferLen;					//Buffer����
DWORD			m_dwCommand;					//Զ������
DWORD			m_reserved;						//����
};
*/
#define PACKET_HEADER_LEN				16
#define KEY	"HoldingHandsYYDS"

class CPacket
{
private:
	char* m_pBuffer;					//��������Header + Body
	DWORD m_dwBufferLen;				//����������
public:

	BOOL			AllocateMem(DWORD BodyLen);

	//У������
	inline BOOL		Verify()
	{
		return ((DWORD*)m_pBuffer)[0] == PACKET_CHECK;
	}
	//
	inline char*	GetBody()
	{
		return m_pBuffer + PACKET_HEADER_LEN;
	}

	inline char*	GetBuffer()
	{
		return m_pBuffer;
	}
	inline DWORD	GetBodyLen()
	{
		return ((DWORD*)m_pBuffer)[1];
	}

	inline DWORD	GetCommand()
	{
		return ((DWORD*)m_pBuffer)[2];
	}

	inline DWORD GetFlag(){
		return ((DWORD*)m_pBuffer)[3];
	}

	inline BOOL		SetHeader(DWORD BodyLen, DWORD Command,DWORD dwFlag = 0)
	{
		if (m_pBuffer == NULL || m_dwBufferLen<PACKET_HEADER_LEN)
		{
			return FALSE;
		}
		((DWORD*)m_pBuffer)[0] = PACKET_CHECK;
		((DWORD*)m_pBuffer)[1] = BodyLen;
		((DWORD*)m_pBuffer)[2] = Command;
		((DWORD*)m_pBuffer)[3] = dwFlag;
		return TRUE;
	}
	
	CPacket();
	~CPacket();
};

