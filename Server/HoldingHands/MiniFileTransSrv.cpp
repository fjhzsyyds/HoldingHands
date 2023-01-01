#include "stdafx.h"
#include "MiniFileTransSrv.h"
#include "MiniFileTransDlg.h"
#include "resource.h"

CMiniFileTransSrv::CMiniFileTransSrv(CManager*pManager) :
	CMsgHandler(pManager)
{
	memset(m_Path, 0, sizeof(m_Path));
	m_dwCurFileIdentity = 0;
	m_hCurFile = INVALID_HANDLE_VALUE;
	m_ullLeftFileLength = 0;			//ʣ�೤��
	m_dwCurFileAttribute = 0;			//��ǰ�ļ�����

	//
	m_pInit = NULL;
	m_pDlg = NULL;
}


CMiniFileTransSrv::~CMiniFileTransSrv()
{
	if (m_pInit)
	{
		free(m_pInit);
		m_pInit = NULL;
	}
}



void CMiniFileTransSrv::OnClose()
{
	Clean();
	//
	if (m_pDlg){
		m_pDlg->SendMessage(WM_CLOSE);
		m_pDlg->DestroyWindow();
		delete m_pDlg;
		m_pDlg = NULL;
	}
}

void CMiniFileTransSrv::OnOpen()
{
	m_pDlg = new CMiniFileTransDlg(this);
	if (FALSE == m_pDlg->Create(IDD_FILETRANS,CWnd::GetDesktopWindow()))
	{
		Close();
		return;
	}
	m_pDlg->ShowWindow(SW_SHOW);
}

//����ֻ�з������Ż����.
void CMiniFileTransSrv::OnMINIInit(DWORD Read, char*Buffer)
{
	CMiniFileTransInit*pInit = (CMiniFileTransInit*)Buffer;

	if (m_pInit == NULL)
	{
		m_pInit = (CMiniFileTransInit*)malloc(Read);
		memcpy(m_pInit, Buffer, Read);
	}
	if (m_pInit->m_dwDuty == MNFT_DUTY_RECEIVER)
	{
		//������ǽ��ն�,��ʼ�����ļ�.
		TCHAR*pPath = NULL;
		if (NULL != (pPath = wcsstr(m_pInit->m_szBuffer, L"\n")))
		{
			*pPath = NULL;
			//����Dest·��.
			lstrcpy(m_Path, m_pInit->m_szBuffer);
			SendMsg(MNFT_TRANS_INFO_GET, (char*)(pPath+1),sizeof(TCHAR)*(lstrlen(pPath + 1) + 1));//����\n֮�������.
		}
		else
			Close();//��Ч��szBuffer;
	}
	//����Ƿ��Ͷ�,�ȴ��Է��������ļ��Ϳ�����.
}

void CMiniFileTransSrv::OnWriteMsg(WORD Msg,DWORD dwSize, char*buffer)
{

}

void CMiniFileTransSrv::OnReadMsg(WORD Msg,  DWORD dwSize, char*Buffer)
{
	switch (Msg)
	{
	/*******************************************************************************/
	//ֻ�з������Ż����յ������Ϣ.
	case MNFT_INIT:
		OnMINIInit(dwSize, Buffer);
		break;
	/*******************************************************************************/
		//when we Get File from peer.
	case MNFT_TRANS_INFO_RPL:
		OnGetTransInfoRpl(dwSize, Buffer);							//��ȡ���˴�����Ϣ
		break;
	case MNFT_FILE_INFO_RPL:
		OnGetFileInfoRpl(dwSize, Buffer);
		break;
	case MNFT_FILE_DATA_CHUNK_RPL:
		OnGetFileDataChunkRpl(dwSize, Buffer);
		break;
	case MNFT_TRANS_FINISHED:
		m_pDlg->SendMessage(WM_MNFT_TRANS_FINISHED, 0, 0);		//�������
		break;
	/*******************************************************************************/
		//when we SendMsg File to peer.
	case MNFT_TRANS_INFO_GET:					//��ȡ������Ϣ(�ļ�����,�ܴ�С)
		OnGetTransInfo(dwSize, Buffer);
		break;
	case MNFT_FILE_INFO_GET:					//�Է���ʼ�����ļ��ˡ�
		OnGetFileInfo(dwSize, Buffer);
		break;
	case MNFT_FILE_DATA_CHUNK_GET:				//��ȡ�ļ�����
		OnGetFileDataChunk(dwSize, Buffer);
		break;
	case MNFT_FILE_TRANS_FINISHED:				//��ǰ�ļ��������(���ܳɹ�,Ҳ����ʧ��)
		OnFileTransFinished(dwSize, Buffer);
		break;
	}
}


void CMiniFileTransSrv::Clean()
{
	while (m_JobList.GetCount()){
		free(m_JobList.RemoveHead());
	}

	if (m_hCurFile != INVALID_HANDLE_VALUE){
		CloseHandle(m_hCurFile);
		m_hCurFile = INVALID_HANDLE_VALUE;
	}
}
//�ļ����ݿ鵽����
void CMiniFileTransSrv::OnGetFileDataChunkRpl(DWORD Read, char*Buffer)
{
	MNFT_File_Data_Chunk_Data*pRpl = (MNFT_File_Data_Chunk_Data*)Buffer;
	if (pRpl->dwFileIdentity != 
		m_dwCurFileIdentity){
		Close();
		return;
	}
	//д���ļ�����
	DWORD dwWrite = 0;
	BOOL bFailed = (pRpl->dwChunkSize == 0 ||
		(!WriteFile(m_hCurFile, pRpl->FileDataChunk, pRpl->dwChunkSize, &dwWrite, NULL)) || 
		(dwWrite == 0));
	if (!bFailed)
	{
		//д��ɹ�
		m_ullLeftFileLength -= dwWrite;
		if (m_ullLeftFileLength)
		{
			//��������
			MNFT_File_Data_Chunk_Data_Get fdcdg;
			fdcdg.dwFileIdentity = m_dwCurFileIdentity;
			SendMsg(MNFT_FILE_DATA_CHUNK_GET, (char*)&fdcdg, sizeof(fdcdg));
		}
		//����
		/*****************************************************************************/
		//��������ȥ����
		m_pDlg->SendMessage(WM_MNFT_FILE_DC_TRANSFERRED, dwWrite, 0);
		/*****************************************************************************/
	}
	//���ʧ����,���߽�������.��ô�ͽ���ֹ��ǰ�Ĵ���,������һ���ļ�.
	if (bFailed || !m_ullLeftFileLength)
	{
		//�رվ��
		if (m_hCurFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hCurFile);
			m_hCurFile = INVALID_HANDLE_VALUE;
		}
		//������ǰ�ļ��Ĵ���.
		MNFT_File_Trans_Finished ftf;
		ftf.dwFileIdentity = m_dwCurFileIdentity;
		ftf.dwStatu = bFailed ? MNFT_STATU_FAILED : MNFT_STATU_SUCCESS;
		SendMsg(MNFT_FILE_TRANS_FINISHED, (char*)&ftf, sizeof(ftf));
		/*****************************************************************************/
		//��������ȥ����
		m_pDlg->SendMessage(WM_MNFT_FILE_TRANS_FINISHED, ftf.dwStatu, 0);
		/*****************************************************************************/
		m_dwCurFileAttribute = 0;
		m_dwCurFileIdentity = 0;
		m_ullLeftFileLength = 0;
		//������һ��
		MNFT_File_Info_Get fig;
		m_dwCurFileIdentity = GetTickCount();
		fig.dwFileIdentity = m_dwCurFileIdentity;
		SendMsg(MNFT_FILE_INFO_GET, (char*)&fig, sizeof(fig));
	}
}

void CMiniFileTransSrv::OnGetFileInfoRpl(DWORD Read, char*Buffer)
{
	MNFT_File_Info *pRpl = (MNFT_File_Info*)Buffer;
	if (pRpl->dwFileIdentity != m_dwCurFileIdentity)
	{
		Close();
		return;
	}
	//
	FileInfo*pFileInfo = &pRpl->fiFileInfo;

	m_dwCurFileAttribute = pFileInfo->Attribute;
	m_ullLeftFileLength = pFileInfo->dwFileLengthHi;
	m_ullLeftFileLength = (m_ullLeftFileLength << 32) + pFileInfo->dwFileLengthLo;
	//
	DWORD FullPathLen = lstrlen(m_Path) + 1 + lstrlen(pFileInfo->RelativeFilePath) + 1;
	PTCHAR FullPath = (TCHAR*)malloc(FullPathLen*sizeof(TCHAR));
	lstrcpy(FullPath, m_Path);
	lstrcat(FullPath, L"\\");
	lstrcat(FullPath, pFileInfo->RelativeFilePath);
	////��������ȥ����
	m_pDlg->SendMessage(WM_MNFT_FILE_TRANS_BEGIN, (WPARAM)pFileInfo, 0);
	if (!(pFileInfo->Attribute&FILE_ATTRIBUTE_DIRECTORY))
	{
		if (m_hCurFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hCurFile);
			m_hCurFile = INVALID_HANDLE_VALUE;
		}
		if (m_ullLeftFileLength != 0)
		{
			//CreateFile.
			m_hCurFile = CreateFile(FullPath, GENERIC_WRITE, 0, NULL, 
				CREATE_ALWAYS, m_dwCurFileAttribute, NULL);
			//GetFileChunk.
			if (m_hCurFile != INVALID_HANDLE_VALUE)
			{
				//��ȡ���ݿ�
				MNFT_File_Data_Chunk_Data_Get fdcdg;
				fdcdg.dwFileIdentity = m_dwCurFileIdentity;
				SendMsg(MNFT_FILE_DATA_CHUNK_GET, (char*)&fdcdg, sizeof(fdcdg));
			}
		}
	}
	else
	{
		CreateDirectoryW(FullPath, NULL);		//MakesureDirectoryExist
	}
	//�ļ�����Ϊ0,���ߵ�ǰ�����ʧ��,��ô��������һ���ļ�.
	if (pFileInfo->Attribute&FILE_ATTRIBUTE_DIRECTORY || 
		m_ullLeftFileLength == 0 || m_hCurFile == INVALID_HANDLE_VALUE)
	{
		
		//������ǰ�ļ��Ĵ���.
		MNFT_File_Trans_Finished ftf;
		ftf.dwFileIdentity = m_dwCurFileIdentity;
		ftf.dwStatu = (pFileInfo->Attribute&FILE_ATTRIBUTE_DIRECTORY 
			|| m_ullLeftFileLength == 0) ? MNFT_STATU_SUCCESS : MNFT_STATU_FAILED;
		SendMsg(MNFT_FILE_TRANS_FINISHED, (char*)&ftf, sizeof(ftf));
		//
		/*****************************************************************************/
		m_pDlg->SendMessage(WM_MNFT_FILE_TRANS_FINISHED, ftf.dwStatu, 0);
		/*****************************************************************************/
		m_dwCurFileAttribute = 0;
		m_dwCurFileIdentity = 0;
		m_ullLeftFileLength = 0;
		//������һ��
		MNFT_File_Info_Get fig;
		m_dwCurFileIdentity = GetTickCount();
		fig.dwFileIdentity = m_dwCurFileIdentity;
		SendMsg(MNFT_FILE_INFO_GET, (char*)&fig, sizeof(fig));
	}
	free(FullPath);
}

void CMiniFileTransSrv::OnGetTransInfoRpl(DWORD Read, char*Buffer)
{
	MNFT_Trans_Info*pTransInfo = (MNFT_Trans_Info*)Buffer;
	//Save trans info.
	/************************************************************************/
	m_pDlg->SendMessage(WM_MNFT_TRANS_INFO, (WPARAM)pTransInfo, m_pInit->m_dwDuty);
	/************************************************************************/
	//Get First File,use timestamp as the file identity.
	MNFT_File_Info_Get fig;
	m_dwCurFileIdentity = GetTickCount();
	fig.dwFileIdentity = m_dwCurFileIdentity;
	SendMsg(MNFT_FILE_INFO_GET, (char*)&fig, sizeof(fig));
}


//--------------------------------------������Щ����Ϊ���Ͷ�Ҫ��������------------------------------------------
//������ǰ�ļ��ķ���.
void CMiniFileTransSrv::OnFileTransFinished(DWORD Read, char*Buffer)
{
	MNFT_File_Trans_Finished*pftf = (MNFT_File_Trans_Finished*)Buffer;
	if (pftf->dwFileIdentity != m_dwCurFileIdentity)
	{
		Close();
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
	/***********************************************************************************/
	m_pDlg->SendMessage(WM_MNFT_FILE_TRANS_FINISHED, pftf->dwStatu, 0);
	/***********************************************************************************/
}
void CMiniFileTransSrv::OnGetFileDataChunk(DWORD Read, char*Buffer)
{
	if (((MNFT_File_Data_Chunk_Data_Get*)Buffer)->dwFileIdentity != m_dwCurFileIdentity)
	{
		//�������.�Ͽ�
		Close();
		return;
	}
	//��������,64kb�Ļ�����.
	MNFT_File_Data_Chunk_Data*pRpl = (MNFT_File_Data_Chunk_Data*)malloc(sizeof(DWORD) * 2 + 0x10000);
	pRpl->dwFileIdentity = m_dwCurFileIdentity;
	pRpl->dwChunkSize = 0;
	//�����ȡ������,Ҳ�᷵��TRUE,
	ReadFile(m_hCurFile, pRpl->FileDataChunk, 0x10000, &pRpl->dwChunkSize, NULL);
	//���˶��پͷ��Ͷ��١���ȡʧ�ܾ���0,�öԷ�  ������ǰ�ļ��Ķ�ȡ.
	SendMsg(MNFT_FILE_DATA_CHUNK_RPL, (char*)pRpl, sizeof(DWORD) * 2 + pRpl->dwChunkSize);
	/**********************************************************************/
	m_pDlg->SendMessage(WM_MNFT_FILE_DC_TRANSFERRED, pRpl->dwChunkSize, 0);
	/**********************************************************************/
	free(pRpl);
}

//�Է������ļ���
void CMiniFileTransSrv::OnGetFileInfo(DWORD Read, char*Buffer)
{
	MNFT_File_Info_Get *pfig = (MNFT_File_Info_Get*)Buffer;
	if (!m_JobList.GetCount())
	{
		SendMsg(MNFT_TRANS_FINISHED, 0, 0);//�������.
		m_pDlg->SendMessage(WM_MNFT_TRANS_FINISHED, 0, 0); 
	}
	else
	{
		//save file identity;
		m_dwCurFileIdentity = pfig->dwFileIdentity;
		//
		DWORD dwRplLen = sizeof(DWORD) * 4 +
			sizeof(TCHAR)* (lstrlen(m_JobList.GetHead()->RelativeFilePath) + 1);

		MNFT_File_Info*pReply = (MNFT_File_Info*)malloc(dwRplLen);

		pReply->dwFileIdentity = m_dwCurFileIdentity;
		pReply->fiFileInfo.Attribute = m_JobList.GetHead()->Attribute;
		pReply->fiFileInfo.dwFileLengthLo = m_JobList.GetHead()->dwFileLengthLo;
		pReply->fiFileInfo.dwFileLengthHi = m_JobList.GetHead()->dwFileLengthHi;

		lstrcpy(pReply->fiFileInfo.RelativeFilePath, m_JobList.GetHead()->RelativeFilePath);
		/*************************************************************************/
		//��������ȥ����
		m_pDlg->SendMessage(WM_MNFT_FILE_TRANS_BEGIN, (WPARAM)&pReply->fiFileInfo, 0);
		/*************************************************************************/
		SendMsg(MNFT_FILE_INFO_RPL, (char*)pReply, dwRplLen);
		free(pReply);
		//�������������Ĵ����ǲ���ִ�е�.��Ϊ��ʼ�����ÿ�ν������ͺ󶼻����
		if (m_hCurFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hCurFile);
			m_hCurFile = INVALID_HANDLE_VALUE;
		}
		//�������Ŀ¼,��ô�򿪶�Ӧ���ļ� 
		if (!(m_JobList.GetHead()->Attribute & FILE_ATTRIBUTE_DIRECTORY))
		{
			DWORD dwFullPathLen = lstrlen(m_Path) + 1 + lstrlen(m_JobList.GetHead()->RelativeFilePath) + 1;
			PTCHAR FullPath = (TCHAR*)malloc(dwFullPathLen*sizeof(TCHAR));
			lstrcpy(FullPath, m_Path);
			lstrcat(FullPath, L"\\");
			lstrcat(FullPath, m_JobList.GetHead()->RelativeFilePath);
			//���ļ����
			m_hCurFile = CreateFileW(FullPath, GENERIC_READ, FILE_SHARE_READ,
				NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			//
			free(FullPath);
		}
	}
}


//�Է���������Ϣ��,������Ϊ���ͷ�
void CMiniFileTransSrv::OnGetTransInfo(DWORD dwRead, char*Buffer)
{
	TCHAR*pFileList = wcsstr((TCHAR*)Buffer, L"\n");
	if (pFileList == NULL || pFileList[1] == NULL)
	{
		//��Ч,ֱ�ӶϿ�
		Close();
		return;
	}
	pFileList[0] = 0;
	pFileList++;
	//����·��.֮�����ļ��õ�.��ΪJobList���汣��Ķ������·��.
	lstrcpy(m_Path, (TCHAR*)Buffer);
	//
	BFS_GetFileList((TCHAR*)Buffer, pFileList, &m_JobList);
	//	
	//�ظ�TransInfo
	MNFT_Trans_Info TransInfoRpl;
	TransInfoRpl.dwFileCount = m_JobList.GetCount();
	TransInfoRpl.TotalSizeLo = m_JobList.GetTotalSize()&0xFFFFFFFF;
	TransInfoRpl.TotalSizeHi = (m_JobList.GetTotalSize() & 0xFFFFFFFF00000000) >> 32;
	//
	m_pDlg->SendMessage(WM_MNFT_TRANS_INFO, (WPARAM)&TransInfoRpl, m_pInit->m_dwDuty);
	SendMsg(MNFT_TRANS_INFO_RPL, (char*)&TransInfoRpl, sizeof(MNFT_Trans_Info));
}

//��ȱ�����ȡ�ļ��б�.
void CMiniFileTransSrv::BFS_GetFileList(TCHAR*Path, TCHAR*FileNameList, FileInfoList*pFileInfoList)
{
	//empty file list:
	if (FileNameList == NULL || FileNameList[0] == 0 || Path == NULL || Path[0] == NULL)
		return;
	//
	//Get file Names:
	TCHAR*szFileNameEnd = FileNameList;
	TCHAR*szFileName = NULL;

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
		DWORD dwNameLen = ((DWORD)szFileNameEnd - (DWORD)szFileName)/sizeof(TCHAR);

		DWORD FileInfoLen = 3 * sizeof(DWORD) + (dwNameLen + 1) * sizeof(TCHAR);
		FileInfo*pNewFileInfo = (FileInfo*)malloc(FileInfoLen);

		pNewFileInfo->Attribute = 0;
		pNewFileInfo->dwFileLengthLo = 0;
		pNewFileInfo->dwFileLengthHi = 0;
		memcpy(pNewFileInfo->RelativeFilePath, szFileName, dwNameLen * sizeof(TCHAR));
		pNewFileInfo->RelativeFilePath[dwNameLen] = 0;
		//Get File Attribute:
		//
		DWORD len = lstrlen(Path) + 1 + lstrlen(pNewFileInfo->RelativeFilePath) + 1;

		TCHAR* FullPath = (TCHAR*)malloc(len*sizeof(TCHAR));
		lstrcpy(FullPath, Path);
		lstrcat(FullPath, L"\\");
		lstrcat(FullPath, pNewFileInfo->RelativeFilePath);
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
		DWORD len = lstrlen(Path) + 1 + lstrlen(pFile->RelativeFilePath) + 2 + 1;
		TCHAR* StartDir = (TCHAR*)malloc(len*sizeof(TCHAR));
		lstrcpy(StartDir, Path);
		lstrcat(StartDir, L"\\");
		lstrcat(StartDir, (pFile->RelativeFilePath));
		lstrcat(StartDir, L"\\*");
		//path\\filepath
		WIN32_FIND_DATAW fd = { 0 };
		HANDLE hFirst = FindFirstFileW(StartDir, &fd);
		BOOL bNext = TRUE;
		//trave this dir:
		while (hFirst != INVALID_HANDLE_VALUE && bNext)
		{
			if (!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ||
				(lstrcmp(fd.cFileName, L".") && lstrcmp(fd.cFileName, L"..")))
			{
				
				//allocate path
				DWORD NewPathLen = lstrlen(pFile->RelativeFilePath) + 1 + lstrlen(fd.cFileName) + 1;

				DWORD FileInfoLen = 3*sizeof(DWORD) + NewPathLen * sizeof(TCHAR);
				
				FileInfo*pNewFileInfo = (FileInfo*)malloc(FileInfoLen);
				//copy path
				lstrcpy(pNewFileInfo->RelativeFilePath, pFile->RelativeFilePath);
				lstrcat(pNewFileInfo->RelativeFilePath, L"\\");
				lstrcat(pNewFileInfo->RelativeFilePath, fd.cFileName);

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

