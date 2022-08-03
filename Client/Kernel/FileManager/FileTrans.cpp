#include "FileTrans.h"
#include "IOCPClient.h"

BOOL MakesureDirExist(const WCHAR* Path,BOOL bIncludeFileName = FALSE);


CFileTrans::CFileTrans(CMiniFileTransInit*pInit, DWORD Identity) :
CEventHandler(Identity)
{
	memset(m_Path, 0, sizeof(m_Path));
	
	m_dwCurFileIdentity = 0;
	m_hCurFile = INVALID_HANDLE_VALUE;
	m_ullLeftFileLength = 0;			//ʣ�೤��;
	m_dwCurFileAttribute = 0;			//��ǰ�ļ�����;


	//
	m_pInit = pInit;
}


CFileTrans::~CFileTrans()
{
	m_pInit = NULL;
}

void CFileTrans::OnClose()
{
	Clean();
}


void CFileTrans::OnConnect()
{
	//
	m_pInit->m_dwDuty = ~m_pInit->m_dwDuty;			
	Send(MNFT_INIT,(char*)m_pInit,sizeof(DWORD) + sizeof(WCHAR)*(lstrlenW(m_pInit->m_Buffer)+1));
	m_pInit->m_dwDuty = ~m_pInit->m_dwDuty;
	//
	WCHAR*pSavePath = NULL;
	if (m_pInit->m_dwDuty == MNFT_DUTY_RECEIVER)
	{
		//�������Ƿ��Ͷ�,��ʼ�����ļ�;
		if (NULL != (pSavePath = wcsstr(m_pInit->m_Buffer, L"\n")))
		{
			*pSavePath = NULL;
			//����Dest·��.;
			lstrcpyW(m_Path,m_pInit->m_Buffer);
			//
			Send(MNFT_TRANS_INFO_GET, (char*)(pSavePath + 1), sizeof(WCHAR)*(lstrlenW(pSavePath + 1) + 1));//����\n֮�������.;
		}
		else
			Disconnect();//��Ч��szBuffer;
	}
	else
	{
		//ʲôҲ������,�ȴ��Է������ļ�.;
	}
}


void CFileTrans::OnReadPartial(WORD Event, DWORD Total, DWORD Read, char*Buffer)
{

}
void CFileTrans::OnReadComplete(WORD Event, DWORD Total, DWORD Read, char*Buffer)
{
	switch (Event)
	{
		/*******************************************************************************/
		/*******************************************************************************/
		//when we Get File from peer.
	case MNFT_TRANS_INFO_RPL:
		OnGetTransInfoRpl(Read,Buffer);			//��ȡ���˴�����Ϣ;
		break;
	case MNFT_FILE_INFO_RPL:
		OnGetFileInfoRpl(Read, Buffer);
		break;
	case MNFT_FILE_DATA_CHUNK_RPL:
		OnGetFileDataChunkRpl(Read, Buffer);
		break;
	case MNFT_TRANS_FINISHED:
		OnTransFinished();						//�������;
		break;
		/*******************************************************************************/
		//when we Send File to peer.
	case MNFT_TRANS_INFO_GET:					//��ȡ������Ϣ(�ļ�����,�ܴ�С);
		OnGetTransInfo(Read, Buffer);
		break;
	case MNFT_FILE_INFO_GET:					//�Է���ʼ�����ļ��ˡ�;
		OnGetFileInfo(Read, Buffer);
		break;
	case MNFT_FILE_DATA_CHUNK_GET:				//��ȡ�ļ�����;
		OnGetFileDataChunk(Read, Buffer);
		break;
	case MNFT_FILE_TRANS_FINISHED:				//��ǰ�ļ��������(���ܳɹ�,Ҳ����ʧ��);
		OnFileTransFinished(Read, Buffer);
		break;
	}
}

void CFileTrans::OnTransFinished()
{
	Disconnect();
}

void CFileTrans::Clean()
{
	while (m_JobList.GetCount())
	{
		free(m_JobList.RemoveHead());
	}
	if (m_hCurFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hCurFile);
		m_hCurFile = INVALID_HANDLE_VALUE;
	}
	memset(m_Path, 0, sizeof(m_Path));

	m_dwCurFileAttribute = 0;
	m_dwCurFileIdentity = 0;
	m_ullLeftFileLength = 0;

}
//�ļ����ݿ鵽����;
void CFileTrans::OnGetFileDataChunkRpl(DWORD Read, char*Buffer)
{
	MNFT_File_Data_Chunk_Data_Rpl*pRpl = (MNFT_File_Data_Chunk_Data_Rpl*)Buffer;
	if (pRpl->dwFileIdentity != m_dwCurFileIdentity)
	{
		Disconnect();
		return;
	}
	//д���ļ�����;
	DWORD dwWrite = 0;
	BOOL bFailed = (pRpl->dwChunkSize == 0 ||
		(!WriteFile(m_hCurFile, pRpl->FileDataChunk, pRpl->dwChunkSize, &dwWrite, NULL)) || 
		(dwWrite == 0));
	if (!bFailed)
	{
		//д��ɹ�;
		m_ullLeftFileLength -= dwWrite;
		if (m_ullLeftFileLength)
		{
			//��������;
			MNFT_File_Data_Chunk_Data_Get fdcdg;
			fdcdg.dwFileIdentity = m_dwCurFileIdentity;
			Send(MNFT_FILE_DATA_CHUNK_GET, (char*)&fdcdg, sizeof(fdcdg));
		}
		//����;
	}
	//���ʧ����,���߽�������.��ô�ͽ���ֹ��ǰ�Ĵ���,������һ���ļ�.;
	if (bFailed || !m_ullLeftFileLength)
	{
		//�رվ��;
		if (m_hCurFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hCurFile);
			m_hCurFile = INVALID_HANDLE_VALUE;
		}
		//������ǰ�ļ��Ĵ���.;
		MNFT_File_Trans_Finished ftf;
		ftf.dwFileIdentity = m_dwCurFileIdentity;
		ftf.dwStatu = bFailed ? MNFT_STATU_FAILED : MNFT_STATU_SUCCESS;
		Send(MNFT_FILE_TRANS_FINISHED, (char*)&ftf, sizeof(ftf));
		/*****************************************************************************/
		/*****************************************************************************/
		m_dwCurFileAttribute = 0;
		m_dwCurFileIdentity = 0;
		m_ullLeftFileLength = 0;
		//������һ��;
		MNFT_File_Info_Get fig;
		m_dwCurFileIdentity = GetTickCount();
		fig.dwFileIdentity = m_dwCurFileIdentity;
		Send(MNFT_FILE_INFO_GET, (char*)&fig, sizeof(fig));
	}
}

void CFileTrans::OnGetFileInfoRpl(DWORD Read, char*Buffer)
{
	MNFT_File_Info_Rpl *pRpl = (MNFT_File_Info_Rpl*)Buffer;
	if (pRpl->dwFileIdentity != m_dwCurFileIdentity)
	{
		Disconnect();
		return;
	}
	//
	FileInfo*pFileInfo = &pRpl->fiFileInfo;

	m_dwCurFileAttribute = pFileInfo->Attribute;
	m_ullLeftFileLength = pFileInfo->dwFileLengthHi;
	m_ullLeftFileLength<<=32;
	m_ullLeftFileLength |= pFileInfo->dwFileLengthLo;
	//
	DWORD FullPathLen = lstrlenW(m_Path) + 1 + lstrlenW(pFileInfo->RelativeFilePath) + 1;
	PWCHAR FullPath = (WCHAR*)malloc(FullPathLen*sizeof(WCHAR));
	lstrcpyW(FullPath, m_Path);
	lstrcatW(FullPath, L"\\");
	lstrcatW(FullPath, pFileInfo->RelativeFilePath);
	//
	if (!(pFileInfo->Attribute&FILE_ATTRIBUTE_DIRECTORY))
	{
		if (m_hCurFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hCurFile);
			m_hCurFile = INVALID_HANDLE_VALUE;
		}
		if (m_ullLeftFileLength != 0)
		{
			//
			MakesureDirExist(FullPath,TRUE);
			//CreateFile.
			m_hCurFile = CreateFile(FullPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, m_dwCurFileAttribute, NULL);
			//GetFileChunk.;
			if (m_hCurFile != INVALID_HANDLE_VALUE)
			{
				//��ȡ���ݿ�;
				MNFT_File_Data_Chunk_Data_Get fdcdg;
				fdcdg.dwFileIdentity = m_dwCurFileIdentity;
				Send(MNFT_FILE_DATA_CHUNK_GET, (char*)&fdcdg, sizeof(fdcdg));
			}
		}
	}
	else
	{
		MakesureDirExist(FullPath);
	}
	//�ļ�����Ϊ0,���ߵ�ǰ�����ʧ��,��ô��������һ���ļ�.;
	if (pFileInfo->Attribute&FILE_ATTRIBUTE_DIRECTORY || m_ullLeftFileLength == 0 || m_hCurFile == INVALID_HANDLE_VALUE)
	{

		//������ǰ�ļ��Ĵ���.;
		MNFT_File_Trans_Finished ftf;
		ftf.dwFileIdentity = m_dwCurFileIdentity;
		ftf.dwStatu = (pFileInfo->Attribute&FILE_ATTRIBUTE_DIRECTORY || m_ullLeftFileLength == 0) ? MNFT_STATU_SUCCESS : MNFT_STATU_FAILED;
		Send(MNFT_FILE_TRANS_FINISHED, (char*)&ftf, sizeof(ftf));
		//
		m_dwCurFileAttribute = 0;
		m_dwCurFileIdentity = 0;
		m_ullLeftFileLength = 0;
		//������һ��;
		MNFT_File_Info_Get fig;
		m_dwCurFileIdentity = GetTickCount();
		fig.dwFileIdentity = m_dwCurFileIdentity;
		Send(MNFT_FILE_INFO_GET, (char*)&fig, sizeof(fig));
	}
	free(FullPath);
}

void CFileTrans::OnGetTransInfoRpl(DWORD Read, char*Buffer)
{
	/************************************************************************/
	//Get First File,use timestamp as the file identity.
	MNFT_File_Info_Get fig;
	m_dwCurFileIdentity = GetTickCount();
	fig.dwFileIdentity = m_dwCurFileIdentity;
	Send(MNFT_FILE_INFO_GET, (char*)&fig, sizeof(fig));
}


//--------------------------------------������Щ����Ϊ���Ͷ�Ҫ��������------------------------------------------;
//������ǰ�ļ��ķ���.;
void CFileTrans::OnFileTransFinished(DWORD Read, char*Buffer)
{
	MNFT_File_Trans_Finished*pftf = (MNFT_File_Trans_Finished*)Buffer;
	if (pftf->dwFileIdentity != m_dwCurFileIdentity)
	{
		Disconnect();
		return;
	}
	if (m_hCurFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hCurFile);
		m_hCurFile = INVALID_HANDLE_VALUE;
	}
	m_dwCurFileIdentity = 0;
	m_ullLeftFileLength = 0;
	m_dwCurFileAttribute = 0;
	//free memory;
	FileInfo*pFile = m_JobList.RemoveHead();
	if (pFile)
	{
		free(pFile);
	}
}
void CFileTrans::OnGetFileDataChunk(DWORD Read, char*Buffer)
{
	if (((MNFT_File_Data_Chunk_Data_Get*)Buffer)->dwFileIdentity != m_dwCurFileIdentity)
	{
		//�������.�Ͽ�;
		Disconnect();
		return;
	}
	//��������;
	MNFT_File_Data_Chunk_Data_Rpl*pRpl = (MNFT_File_Data_Chunk_Data_Rpl*)malloc(sizeof(DWORD) * 2 + 0x10000);
	pRpl->dwFileIdentity = m_dwCurFileIdentity;
	pRpl->dwChunkSize = 0;
	//�����ȡ������,Ҳ�᷵��TRUE,;
	ReadFile(m_hCurFile, pRpl->FileDataChunk, 0x10000, &pRpl->dwChunkSize, NULL);
	//���˶��پͷ��Ͷ��١���ȡʧ�ܾ���0,�öԷ�  ������ǰ�ļ��Ķ�ȡ.;
	Send(MNFT_FILE_DATA_CHUNK_RPL, (char*)pRpl, sizeof(DWORD) * 2 + pRpl->dwChunkSize);
	free(pRpl);
}

//�Է������ļ���;
void CFileTrans::OnGetFileInfo(DWORD Read, char*Buffer)
{
	MNFT_File_Info_Get *pfig = (MNFT_File_Info_Get*)Buffer;
	if (!m_JobList.GetCount())
	{
		Send(MNFT_TRANS_FINISHED, 0, 0);//�������.;
		Disconnect();
	}
	else
	{
		//save file identity;
		m_dwCurFileIdentity = pfig->dwFileIdentity;
		//
		DWORD dwRplLen = sizeof(DWORD) * 2 + sizeof(ULONGLONG) +
			sizeof(WCHAR)* (lstrlenW(m_JobList.GetHead()->RelativeFilePath) + 1);

		MNFT_File_Info_Rpl*pReply = (MNFT_File_Info_Rpl*)malloc(dwRplLen);

		pReply->dwFileIdentity = m_dwCurFileIdentity;
		pReply->fiFileInfo.Attribute = m_JobList.GetHead()->Attribute;
		pReply->fiFileInfo.dwFileLengthHi = m_JobList.GetHead()->dwFileLengthHi;
		pReply->fiFileInfo.dwFileLengthLo = m_JobList.GetHead()->dwFileLengthLo;

		lstrcpyW(pReply->fiFileInfo.RelativeFilePath, m_JobList.GetHead()->RelativeFilePath);
		Send(MNFT_FILE_INFO_RPL, (char*)pReply, dwRplLen);
		free(pReply);
		//�������������Ĵ����ǲ���ִ�е�.��Ϊ��ʼ�����ÿ�ν������ͺ󶼻����;
		if (m_hCurFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hCurFile);
			m_hCurFile = INVALID_HANDLE_VALUE;
		}
		//�������Ŀ¼,��ô�򿪶�Ӧ���ļ� ;
		if (!(m_JobList.GetHead()->Attribute & FILE_ATTRIBUTE_DIRECTORY))
		{
			DWORD dwFullPathLen = lstrlenW(m_Path) + 1 + lstrlenW(m_JobList.GetHead()->RelativeFilePath) + 1;
			PWCHAR FullPath = (WCHAR*)malloc(dwFullPathLen*sizeof(WCHAR));
			lstrcpyW(FullPath, m_Path);
			lstrcatW(FullPath, L"\\");
			lstrcatW(FullPath, m_JobList.GetHead()->RelativeFilePath);
			//���ļ����;
			m_hCurFile = CreateFile(FullPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			//
			free(FullPath);
		}
	}
}


//�Է���������Ϣ��,������Ϊ���ͷ�;
void CFileTrans::OnGetTransInfo(DWORD Read, char*Buffer)
{
	WCHAR*pFileList = wcsstr((WCHAR*)Buffer, L"\n");
	if (pFileList == NULL || pFileList[1] == NULL)
	{
		//��Ч,ֱ�ӶϿ�;
		Disconnect();
		return;
	}
	pFileList[0] = 0;
	pFileList++;
	//����·��.֮�����ļ��õ�.��ΪJobList���汣��Ķ������·��.;
	lstrcpyW(m_Path,(WCHAR*)Buffer);
	//
	BFS_GetFileList((WCHAR*)Buffer, pFileList, &m_JobList);
	//
	//�ظ�TransInfo;
	MNFT_Trans_Info_Rpy TransInfoRpl;
	TransInfoRpl.dwFileCount =  m_JobList.GetCount();
	TransInfoRpl.TotalSizeLo = m_JobList.GetTotalSize()&0xffffffff;
	TransInfoRpl.TotalSizeHi = (m_JobList.GetTotalSize()&0xffffffff00000000)>>32;
	Send(MNFT_TRANS_INFO_RPL, (char*)&TransInfoRpl, sizeof(MNFT_Trans_Info_Rpy));
}

//��ȱ�����ȡ�ļ��б�.;
void CFileTrans::BFS_GetFileList(WCHAR*Path, WCHAR*FileNameList, FileInfoList*pFileInfoList)
{
	//empty file list:
	if (FileNameList == NULL || FileNameList[0] == 0 || Path == NULL || Path[0] == NULL)
		return;
	//
	//Get file Names:
	WCHAR*szFileNameEnd = FileNameList;
	WCHAR*szFileName = NULL;

	FileInfoList IterQueue;
	//����\n
	while (szFileNameEnd[0] && szFileNameEnd[0] == L'\n')
		szFileNameEnd++;

	while (szFileNameEnd[0])
	{
		szFileName = szFileNameEnd;
		//�ҵ���β
		while (szFileNameEnd[0] && szFileNameEnd[0] != L'\n')
			szFileNameEnd++;

		//save file 
		DWORD dwNameLen = ((DWORD)szFileNameEnd - (DWORD)szFileName)/sizeof(WCHAR);

		DWORD FileInfoLen = 3 * sizeof(DWORD) + (dwNameLen + 1) * sizeof(WCHAR);
		FileInfo*pNewFileInfo = (FileInfo*)malloc(FileInfoLen);

		pNewFileInfo->Attribute = 0;
		pNewFileInfo->dwFileLengthLo = 0;
		pNewFileInfo->dwFileLengthHi = 0;
		memcpy(pNewFileInfo->RelativeFilePath, szFileName, dwNameLen * sizeof(WCHAR));
		pNewFileInfo->RelativeFilePath[dwNameLen] = 0;
		//Get File Attribute:
		//
		DWORD len = lstrlenW(Path) + 1 + lstrlenW(pNewFileInfo->RelativeFilePath) + 1;

		WCHAR* FullPath = (WCHAR*)malloc(len*sizeof(WCHAR));
		lstrcpyW(FullPath, Path);
		lstrcatW(FullPath, L"\\");
		lstrcatW(FullPath, pNewFileInfo->RelativeFilePath);
		WIN32_FIND_DATAW fd;
		HANDLE hFirst = FindFirstFileW(FullPath, &fd);
		if (hFirst != INVALID_HANDLE_VALUE)
		{
			pNewFileInfo->Attribute = fd.dwFileAttributes;
			pNewFileInfo->dwFileLengthHi = fd.nFileSizeHigh;
			pNewFileInfo->dwFileLengthLo = fd.nFileSizeLow;
			//
			FindClose(hFirst);
			IterQueue.AddTail(pNewFileInfo);
		}
		else
		{
			free(pNewFileInfo);
		}
		free(FullPath);
		//NextFileName,����\n
		while (szFileNameEnd[0] && szFileNameEnd[0] == '\n')
			szFileNameEnd++;
	}
	//BFS
	while (IterQueue.GetCount())
	{
		pFileInfoList->AddTail(IterQueue.RemoveHead());
		FileInfo*pFile = pFileInfoList->GetTail();
		if (!(pFile->Attribute &FILE_ATTRIBUTE_DIRECTORY))
			continue;
		//It is a dir.browse it;
		DWORD len = lstrlenW(Path) + 1 + lstrlenW(pFile->RelativeFilePath) + 2 + 1;
		WCHAR* StartDir = (WCHAR*)malloc(len*sizeof(WCHAR));
		lstrcpyW(StartDir, Path);
		lstrcatW(StartDir, L"\\");
		lstrcatW(StartDir, (pFile->RelativeFilePath));
		lstrcatW(StartDir, L"\\*");
		//path\\filepath
		WIN32_FIND_DATAW fd = { 0 };
		HANDLE hFirst = FindFirstFileW(StartDir, &fd);
		BOOL bNext = TRUE;
		//trave this dir:
		while (hFirst != INVALID_HANDLE_VALUE && bNext)
		{
			if (!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ||
				(wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L"..")))
			{

				//allocate path
				DWORD NewPathLen = lstrlenW(pFile->RelativeFilePath) + 1 + lstrlenW(fd.cFileName) + 1;

				DWORD FileInfoLen = 3*sizeof(DWORD) + NewPathLen * sizeof(WCHAR);

				FileInfo*pNewFileInfo = (FileInfo*)malloc(FileInfoLen);
				//copy path
				lstrcpyW(pNewFileInfo->RelativeFilePath, pFile->RelativeFilePath);
				lstrcatW(pNewFileInfo->RelativeFilePath, L"\\");
				lstrcatW(pNewFileInfo->RelativeFilePath, fd.cFileName);

				pNewFileInfo->Attribute = fd.dwFileAttributes;
				pNewFileInfo->dwFileLengthHi = (fd.nFileSizeHigh);
				pNewFileInfo->dwFileLengthLo = fd.nFileSizeLow;
				//
				IterQueue.AddTail(pNewFileInfo);
			}
			//Find Next file;
			bNext = FindNextFileW(hFirst, &fd);
		}
		FindClose(hFirst);
		free(StartDir);
	}
}
