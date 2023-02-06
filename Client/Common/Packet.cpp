#include "Packet.h"
#include "zlib\zlib.h"
#pragma comment(lib,"zlib.lib")

CPacket::CPacket()
{
	//��ʼ����Ӧ�øպÿ��Դ���һ��Header
	m_pBuffer = (BYTE*)calloc(1,PACKET_HEADER_LEN);
	m_dwBufferLen = PACKET_HEADER_LEN;

	((DWORD*)m_pBuffer)[0] = PACKET_CHECK;			//Set Magic.
}

CPacket::~CPacket()
{
	if (m_pBuffer){
		free(m_pBuffer);
		m_pBuffer = NULL;
	}
	m_dwBufferLen = 0;
}


//����Buffer
BOOL CPacket::Reserve(DWORD DataLength)
{
	//������������,�����·���
	DWORD NewSize = GetDataLength() + PACKET_HEADER_LEN;
	if (NewSize > MAX_BUFFER_SIZE)
		return FALSE;//̫����,������.

	if (NewSize > m_dwBufferLen)
	{
		//realloc memory
		BYTE*pNewBuffer;
		pNewBuffer = (BYTE*)realloc(m_pBuffer, NewSize);
		if (pNewBuffer == NULL){
			return FALSE;
		}
		m_pBuffer = pNewBuffer;
		m_dwBufferLen = NewSize;
	}
	return m_pBuffer != NULL;
}

BOOL CPacket::Write(BYTE* lpData, UINT nSize){
	DWORD newLength = PACKET_HEADER_LEN + GetDataLength() + nSize;
	DWORD newDataLength = GetDataLength() + nSize;

	if (newLength > MAX_BUFFER_SIZE){
		return FALSE;
	}

	while (newLength > m_dwBufferLen ){
		m_dwBufferLen *= 2;
	}
	m_pBuffer = (BYTE*)realloc(m_pBuffer, m_dwBufferLen);
	if (!m_pBuffer){
		return FALSE;
	}

	memcpy(GetData() + GetDataLength(), lpData, nSize);
	((DWORD*)m_pBuffer)[1] = newDataLength;			//set new data length.
	return TRUE;
}

BOOL CPacket::Compress(){
	uLong source_len = GetDataLength();
	uLong bufferSize = compressBound(source_len);
	Byte * compreeBuffer = (Byte*)calloc(1, bufferSize);

	int err = Z_OK;

	err = compress(compreeBuffer, &bufferSize, GetData(), source_len);

	if (err == S_OK){
		Reserve(bufferSize);
		
		ClearData();
		Write(compreeBuffer, bufferSize);

		((DWORD*)m_pBuffer)[1] = bufferSize;			//�������ݳ���.
		((DWORD*)m_pBuffer)[2] = source_len;			//ԭʼ���ݳ���.
	}
	free(compreeBuffer);
	return TRUE;									
}

BOOL CPacket::Decompress(){
	BOOL bRet = TRUE;
	if (GetRawDataLength()){
		uLong bufferSize = GetRawDataLength();
		Byte * uncompressBuffer = (Byte*)calloc(1, bufferSize);
		int err = Z_OK;

		err = uncompress(uncompressBuffer, &bufferSize, GetData(), GetDataLength());
		
		if (err == Z_OK){
			Reserve(bufferSize);
			ClearData();
			Write(uncompressBuffer, bufferSize);

			((DWORD*)m_pBuffer)[1] = bufferSize;
			((DWORD*)m_pBuffer)[2] = 0;
		}
		else{
			bRet = FALSE;
		}
		free(uncompressBuffer);
	}
	return bRet;
}