#include "Camera.h"
#include "..\Common\json\json.h"



#ifdef _DEBUG
#pragma comment(lib,"jsond.lib")
#else
#pragma comment(lib,"json.lib")
#endif

#define STRSAFE_NO_DEPRECATE

CCamera::CCamera(CManager*pManager):
CMsgHandler(pManager, CAMERA){
	m_bStop = FALSE;
	m_hWorkThread = NULL;
}


CCamera::~CCamera(){
	
}

void CCamera::OnOpen()
{
	const VideoInfoList&video_info = m_Grab.GetDeviceList();
	Json::FastWriter writer;
	Json::Value root;
	char key[128];

	for (auto & device : video_info){

		Json::Value json_device;
		for (auto&compression : device.second){

			Json::Value json_compression;
			for (auto&bitcount : compression.second){
				Json::Value vs;
				for (auto&video_size : bitcount.second){
					Json::Value size;
					size["width"] = video_size.first;
					size["height"] = video_size.second;
					cout << size << endl;
					vs.append(size);
				}
				sprintf_s(key, 128, "%d", bitcount.first);
				json_compression[key] = vs;
			}
			sprintf_s(key, 128, "%d", compression.first);
			json_device[key] = json_compression;
		}
		root[device.first] = json_device;
	}
	string response = writer.write(root);
	SendMsg(CAMERA_DEVICELIST, (void*)response.c_str(), response.length() + 1);
}

void CCamera::OnClose()
{
	OnStop();
	//һ��Ҫֹͣ��׽,�����̼߳������ͻ����.
}

void CCamera::OnStart(const string &device_name, int compression, int bitcount,int width, int height)
{
	DWORD dwResponse[3] = { 0 };
	string err;
	string data;
	Json::Value res;

	int code = m_Grab.GrabberInit(device_name, compression, bitcount,width, height, err);
	res["code"] = code;
	res["msg"] = err;
	res["width"] = width;
	res["height"] = height;

	data = Json::FastWriter().write(res);
	SendMsg(CAMERA_VIDEOSIZE, (void*)data.c_str(), data.length() + 1);
	
	if (code){
		return;			//Grab init failed....
	}
	m_Grab.StopGrab();						//ֹͣ��׽
	//����δ�ͷŵ��߳���Դ.
	InterlockedExchange(&m_bStop, TRUE);	//ֹͣ����
	//�ر���һ��δ�ͷŵ���Դ.
	if (m_hWorkThread){
		WaitForSingleObject(m_hWorkThread, INFINITE);		//�ȴ��߳��˳�
		CloseHandle(m_hWorkThread);
		m_hWorkThread = NULL;
	}

	//ȡ��ֹͣ���.
	InterlockedExchange(&m_bStop, FALSE);
	if (TRUE == m_Grab.StartGrab()){
		m_hWorkThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)WorkThread, (LPVOID)this, 0, 0);
	}
}

void CCamera::WorkThread(CCamera*pThis)
{
	BOOL bStop = FALSE;

	while (!InterlockedExchange(&pThis->m_bStop, FALSE)){
		char*Buffer = NULL;
		DWORD dwLen = NULL;

		bStop = !pThis->m_Grab.GetFrame(&Buffer, &dwLen);

		if (!bStop)
			pThis->SendMsg(CAMERA_FRAME, Buffer, dwLen);	//������,����free

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
	SendMsg(CAMERA_STOP_OK, 0, 0);
}

void CCamera::OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer)
{
	switch (Msg)
	{
	case CAMERA_START:
		do{
			Json::Value root;
			Json::Reader reader;
			if (reader.parse(Buffer, root)){
				
				OnStart(root["device"].asString(), root["format"].asInt(), root["bit"].asInt(),
					root["width"].asInt(), root["height"].asInt());
			}
		} while (0);
		break;
	case CAMERA_STOP:
		OnStop();
		break;
	case CAMERA_SCREENSHOT:
		SendMsg(CAMERA_SCREENSHOT, 0, 0);
		break;
	default:
		break;
	}
}