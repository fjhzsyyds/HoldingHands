#pragma once
#ifndef _MINI_FILE_TRANS_SVR
#define _MINI_FILE_TRANS_SVR

#include "MsgHandler.h"

#define MINIFILETRANS	('M'|('N'<<8)|('F')<<16|('T')<<24)


#define MNFT_DUTY_SENDER			(0xabcdef11)
#define MNFT_DUTY_RECEIVER			(~MNFT_DUTY_SENDER)

#define MNFT_INIT					(0xab00)

#define MNFT_TRANS_INFO_GET			(0xab01)
#define MNFT_TRANS_INFO_RPL			(0xab02)

#define MNFT_FILE_INFO_GET			(0xab03)
#define MNFT_FILE_INFO_RPL			(0xab04)

#define MNFT_FILE_DATA_CHUNK_GET	(0xab05)
#define MNFT_FILE_DATA_CHUNK_RPL	(0xab06)

//recv----->send
#define MNFT_FILE_TRANS_FINISHED	(0xab07)

//send ---> recv
#define MNFT_TRANS_FINISHED			(0xab08)


#define MNFT_STATU_SUCCESS			(0x01010101)
#define MNFT_STATU_FAILED			(0x02020202)

/************************************************************************************/


class CManager;

class CFileTrans :
	public CMsgHandler
{
public:
	struct CMiniFileTransInit
	{
		DWORD	m_dwDuty;
		WCHAR	m_Buffer[2];		//Dest,Src,FileList;
	};
private:
	//some structures will be used when we thansfer files;
	struct FileInfo
	{
		DWORD	  dwFileLengthHi;
		DWORD	  dwFileLengthLo;
		DWORD	  Attribute;
		WCHAR	  RelativeFilePath[2];			//2是为了对齐.
	};
	/***************************************************************************/

	struct MNFT_Trans_Info_Rpy
	{
		DWORD		TotalSizeHi;		//文件总大小;
		DWORD		TotalSizeLo;		//文件总大小;
		DWORD		dwFileCount;		//文件个数;
	};
	/***************************************************************************/
	struct MNFT_File_Info_Rpl
	{
		DWORD		dwFileIdentity;
		FileInfo	fiFileInfo;
	};

	struct MNFT_File_Info_Get
	{
		DWORD		dwFileIdentity;
	};
	/***************************************************************************/

	struct MNFT_File_Data_Chunk_Data_Get
	{
		DWORD		dwFileIdentity;
	};

	struct MNFT_File_Data_Chunk_Data_Rpl
	{
		DWORD		dwFileIdentity;
		DWORD		dwChunkSize;
		char		FileDataChunk[4];		//4是为了对齐.
	};
	/***************************************************************************/

	struct MNFT_File_Trans_Finished
	{
		DWORD		dwFileIdentity;		//
		DWORD		dwStatu;			//SUCCESS ,Failed
	};

	/***************************************************************************/

	class FileInfoList
	{
	private:
		struct Node
		{
			FileInfo*pFileInfo;
			Node*next;
		};
		ULONGLONG	m_ullTotalLength;
		DWORD		m_Count;
		Node*		m_pHead;
		Node*		m_pTail;
	public:
		FileInfoList(){
			m_ullTotalLength = 0;
			m_Count = 0;
			m_pHead = m_pTail = NULL;
		}
		~FileInfoList()
		{
			while (m_pHead){
				Node*pTemp = m_pHead;
				m_pHead = m_pHead->next;
				free(pTemp);
			}
		}
		void AddTail(FileInfo*pFileInfo){
			Node*pNewNode = (Node*)malloc(sizeof(Node));
			pNewNode->next = 0;
			pNewNode->pFileInfo = pFileInfo;
			
			ULONGLONG FileSize = pFileInfo->dwFileLengthHi;
			FileSize <<= 32;
			FileSize |= pFileInfo->dwFileLengthLo;

			m_ullTotalLength += FileSize;

			if (m_Count == 0){
				m_pHead = m_pTail = pNewNode;
			}
			else{
				m_pTail->next = pNewNode;
				m_pTail = pNewNode;
			}
			m_Count++;
		}
		FileInfo* RemoveHead(){
			if (!m_Count)
				return NULL;
			m_Count--;
			FileInfo*pResult = m_pHead->pFileInfo;

			ULONGLONG FileSize = pResult->dwFileLengthHi;
			FileSize <<= 32;
			FileSize |= pResult->dwFileLengthLo;

			m_ullTotalLength -= FileSize;
			Node*pTemp = m_pHead;
			if (m_pHead == m_pTail)
			{
				m_pHead = m_pTail = NULL;
			}
			else
			{
				m_pHead = m_pHead->next;
			}
			free(pTemp);
			return pResult;
		}
		FileInfo*GetHead(){
			if (m_pHead)
				return m_pHead->pFileInfo;
			return NULL;
		}
		FileInfo*GetTail(){
			if (m_pTail)
				return m_pTail->pFileInfo;
			return NULL;
		}
		DWORD GetCount(){
			return m_Count;
		}
		ULONGLONG GetTotalSize(){
			return m_ullTotalLength;
		}
	};


private:
	CMiniFileTransInit*	m_pInit;

	//common
	DWORD			m_dwCurFileIdentity;
	HANDLE			m_hCurFile;
	//recv use
	ULONGLONG		m_ullLeftFileLength;
	DWORD			m_dwCurFileAttribute;
	//send use
	FileInfoList	m_JobList;

	//common
	WCHAR			m_Path[4096];

private:

	void OnClose();	
	void OnOpen();

	void OnReadMsg(WORD Msg,DWORD dwSize, char*Buffer);
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer){}

	void Clean();
	//

	/***************************************************************************/
	//作为接收端;
	//获取文件列表;
	void BFS_GetFileList(WCHAR*Path, WCHAR*FileNameList, FileInfoList*pFileInfoList);
	void OnGetTransInfo(DWORD Read, char*Buffer);
	//获取文件信息
	void OnGetFileInfo(DWORD Read, char*Buffer);
	//获取文件数据.;
	void OnGetFileDataChunk(DWORD Read, char*Buffer);
	//结束当前文件的发送.;
	void OnFileTransFinished(DWORD Read, char*Buffer);
	//传输结束;
	void OnTransFinished();
	/***************************************************************************/
	
	//作为发送端.;
	void OnGetTransInfoRpl(DWORD Read, char*Buffer);
	void OnGetFileInfoRpl(DWORD Read, char*Buffer);
	void OnGetFileDataChunkRpl(DWORD Read, char*Buffer);

public:
	CFileTrans(CIOCPClient*pClient, CMiniFileTransInit*pInit);
	~CFileTrans();
};

#endif