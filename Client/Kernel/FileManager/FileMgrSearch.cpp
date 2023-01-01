#include "IOCPClient.h"
#include "FileMgrSearch.h"


CFileMgrSearch::CFileMgrSearch(CManager*pManager) :
CMsgHandler(pManager,FILEMGR_SEARCH)
{
}


CFileMgrSearch::~CFileMgrSearch()
{
}


void CFileMgrSearch::OnClose()
{
	OnStop();
}

void CFileMgrSearch::OnOpen()
{

}


void CFileMgrSearch::OnReadMsg(WORD Msg,DWORD dwSize, char*Buffer)
{
	switch (Msg)
	{
	case FILE_MGR_SEARCH_SEARCH:
		OnSearch(Buffer);
		break;
	case FILE_MGR_SEARCH_STOP:
		OnStop();
		break;
	default:
		break;
	}
}


void OnFoundFile(TCHAR* path, WIN32_FIND_DATAW* pfd, LPVOID Param)
{
	//printf("OnFoundFile!");
	struct FindFile
	{
		DWORD dwFileAttribute;
		TCHAR szFileName[2];
	};
	//找到一个就发送一个.
	CFileMgrSearch*pMgrSearch = (CFileMgrSearch*)Param;
	DWORD dwLen = sizeof(DWORD) + sizeof(TCHAR)*(lstrlenW(path) + 1 + lstrlenW(pfd->cFileName) + 1);
	FindFile*pFindFile = (FindFile*)malloc(dwLen);

	pFindFile->dwFileAttribute = pFindFile->dwFileAttribute;
	lstrcpyW(pFindFile->szFileName, path);
	lstrcatW(pFindFile->szFileName, L"\n");
	lstrcatW(pFindFile->szFileName, pfd->cFileName);

	pMgrSearch->SendMsg(FILE_MGR_SEARCH_FOUND, (char*)pFindFile, dwLen);
	free(pFindFile);
	//printf("OnFoundFile OK!");
}
void OnSearchOver(LPVOID Param)
{
	CFileMgrSearch*pMgrSearch = (CFileMgrSearch*)Param;
	pMgrSearch->SendMsg(FILE_MGR_SEARCH_OVER, 0, 0);
}

void CFileMgrSearch::OnSearch(char*Buffer)
{
	TCHAR*szStartLocation = (TCHAR*)Buffer;

	TCHAR*szFileName = wcsstr((TCHAR*)Buffer, L"\n");
	if (!szFileName)
		return;
	*szFileName++ = NULL;
	//
	wprintf(szStartLocation);
	wprintf(szFileName);
	//
	m_searcher.Search(szFileName, szStartLocation, 0, OnFoundFile, OnSearchOver, this);

}
void CFileMgrSearch::OnStop()
{
	m_searcher.StopSearching();
}
