#pragma once
#include <winsock2.h>
#include <MSWSock.h>

#define MAX_BUFFER_SIZE					(32*1024*1024)
#define PACKET_CHECK					0x534E4942


/*
struct CPacketHeader
{
DWORD			m_Magic;						//数据校验
DWORD			m_DataLength;					//Buffer长度
DWORD			m_RawDataLength;				//原始长度.
DWORD			m_reserved;						//保留
};
*/

#define PACKET_HEADER_LEN				16
#define KEY	"HoldingHandsYYDS"

class CPacket
{
private:
	BYTE*	m_pBuffer;					//用来储存Header + Body
	DWORD	m_dwBufferLen;				//缓冲区长度

	inline DWORD	GetRawDataLength()
	{
		return ((DWORD*)m_pBuffer)[2];
	}

public:
	BOOL			Reserve(DWORD DataLength);
	//校验数据
	inline BOOL		Verify()
	{
		return ((DWORD*)m_pBuffer)[0] == PACKET_CHECK;
	}
	//
	BYTE*			GetBuffer(){
		return m_pBuffer;
	}

	inline BYTE*	GetData()
	{
		return m_pBuffer + PACKET_HEADER_LEN;
	}

	inline DWORD	GetDataLength()
	{
		return ((DWORD*)m_pBuffer)[1];
	}

	inline DWORD	GetReserved(){
		return ((DWORD*)m_pBuffer)[3];
	}

	inline void SetReserved(DWORD val){
		((DWORD*)m_pBuffer)[3] = val;
	}

	void	ClearData(){
		((DWORD*)m_pBuffer)[1] = 0;
		((DWORD*)m_pBuffer)[2] = 0;
	}

	BOOL	Compress();
	BOOL	Decompress();

	BOOL Write(BYTE* lpData, UINT nSize);

	CPacket();
	~CPacket();
};

