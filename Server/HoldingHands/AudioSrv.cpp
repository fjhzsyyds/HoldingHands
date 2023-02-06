#include "stdafx.h"
#include "AudioSrv.h"
#include "AudioDlg.h"
#pragma comment(lib,"Winmm.lib")


CAudioSrv::CAudioSrv(CClientContext*pClient) :
CMsgHandler(pClient)
{
	//
	m_pDlg = NULL;

	m_IsWorking = FALSE;
	m_hWorkThread = NULL;
}


CAudioSrv::~CAudioSrv()
{
}

void CAudioSrv::OnOpen()
{
	m_pDlg = new CAudioDlg(this);
	ASSERT(m_pDlg->Create(IDD_AUDIODLG, CWnd::GetDesktopWindow()));

	m_pDlg->ShowWindow(SW_SHOW);
	//��ʼ
	SendMsg(AUDIO_BEGIN, 0, 0);
}

void CAudioSrv::OnClose()
{
	StopSendLocalVoice();
	OnAudioPlayStop();

	if (m_pDlg){
		if (m_pDlg->m_DestroyAfterDisconnect){
			//�����ȹرյ�.
			m_pDlg->DestroyWindow();
			delete m_pDlg;
		}
		else{
			// pHandler�ȹرյ�,��ô�Ͳ��ܴ�����
			m_pDlg->m_pHandler = nullptr;
			m_pDlg->PostMessage(WM_AUDIO_ERROR, (WPARAM)TEXT("Disconnect."));
		}
		m_pDlg = nullptr;
	}
}



void CAudioSrv::OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer)
{
	switch (Msg)
	{
	case AUDIO_PLAY_BEGIN:
		OnAudioPlayBegin();
		break;
	case AUDIO_PLAY_DATA:
		OnAudioPlayData(Buffer, dwSize);
		break;
	case AUDIO_PLAY_STOP:
		OnAudioPlayStop();
		break;
	case AUDIO_ERROR:
		OnAudioError((TCHAR*)Buffer);
		break;
	default:
		break;
	}
}


void CAudioSrv::OnAudioError(TCHAR*szError)
{
	m_pDlg->SendMessage(WM_AUDIO_ERROR, (WPARAM)szError, 0);
}

void CAudioSrv::OnAudioPlayBegin(){
	OnAudioPlayStop();
	m_AudioPlay.InitPlayer();
}

void CAudioSrv::OnAudioPlayData(char* buffer, DWORD size){
	if (!m_AudioPlay.IsWorking()){
		m_pDlg->SendMessage(WM_AUDIO_ERROR,
			(WPARAM)TEXT("m_AudioPlay.IsWorking() Is False"));
		return;
	}

	if (!m_AudioPlay.PlayBuffer(buffer, size)){
		m_pDlg->SendMessage(WM_AUDIO_ERROR,
			(WPARAM)TEXT("m_AudioPlay.PlayBuffer() Failed!"));
		return;
	}
}

void CAudioSrv::OnAudioPlayStop(){
	m_AudioPlay.ClosePlayer();
}

//�����Ƿ��ͱ���������Զ��.
void CAudioSrv::StartSendLocalVoice(){
	StopSendLocalVoice();

	InterlockedExchange(&m_IsWorking, TRUE);
	if (m_AudioGrab.InitGrabber()){
		SendMsg(AUDIO_PLAY_BEGIN, 0, 0);		//֪ͨ�Է��򿪲�����...
		m_hWorkThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)SendThread,
			this, 0, 0);
	}
	else{
		InterlockedExchange(&m_IsWorking, FALSE);
		m_pDlg->SendMessage(WM_AUDIO_ERROR, (WPARAM)TEXT("Local Audio Grab Init Failed"));
	}
}

void CAudioSrv::StopSendLocalVoice(){
	InterlockedExchange(&m_IsWorking, FALSE);

	if (m_hWorkThread){
		WaitForSingleObject(m_hWorkThread, INFINITE);
		CloseHandle(m_hWorkThread);
		m_hWorkThread = NULL;
	}
	m_AudioGrab.CloseGrabber();
	SendMsg(AUDIO_PLAY_STOP, 0, 0);
}

void __stdcall CAudioSrv::SendThread(CAudioSrv*pThis)
{
	TCHAR szError[] = TEXT("Get Audio Buffer Error!");
	while (InterlockedExchange(&pThis->m_IsWorking, TRUE)){
		char*Buffer = NULL;
		DWORD dwLen = NULL;
		//��ȡbuffer����
		if (pThis->m_AudioGrab.GetBuffer((void**)&Buffer, &dwLen)){
			pThis->SendMsg(AUDIO_PLAY_DATA, Buffer, dwLen);
			free(Buffer);
		}
	}
	InterlockedExchange(&pThis->m_IsWorking, FALSE);
}