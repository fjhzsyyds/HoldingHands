#include "Camera.h"
#include "..\Common\json\json.h"



#ifdef _DEBUG
#pragma comment(lib,"jsond.lib")
#else
#pragma comment(lib,"json.lib")
#endif

#define STRSAFE_NO_DEPRECATE

CCamera::CCamera(CIOCPClient *pClient) :
CMsgHandler(pClient, CAMERA){
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

	for (auto & device : video_info){
		Json::Value size;
		for (auto&video_size : device.second){
			size["width"] = video_size.first;
			size["height"] = video_size.second;

			root[device.first].append(size);
		}
	}
	string response = writer.write(root);
	SendMsg(CAMERA_DEVICELIST, (void*)response.c_str(), response.length() + 1);
}

void CCamera::OnClose()
{
	OnStop();
	//一定要停止捕捉,否则线程继续发送会崩掉.
}

void CCamera::OnStart(const string &device_name,int width, int height)
{
	DWORD dwResponse[3] = { 0 };
	string data;
	Json::Value res;

	char szError[0x100];
	
	//停止发送。
	m_Grab.StopGrab();						//停止捕捉
	//清理未释放的线程资源.
	InterlockedExchange(&m_bStop, TRUE);	//停止发送
	//关闭上一次未释放的资源.
	if (m_hWorkThread){
		WaitForSingleObject(m_hWorkThread, INFINITE);		//等待线程退出
		CloseHandle(m_hWorkThread);
		m_hWorkThread = NULL;
	}
	//取消停止标记.
	InterlockedExchange(&m_bStop, FALSE);


	int code = m_Grab.GrabberInit(device_name,width, height, szError);
	res["code"] = code;
	res["err"] = szError;
	res["width"] = width;
	res["height"] = height;

	data = Json::FastWriter().write(res);
	SendMsg(CAMERA_VIDEOSIZE, (void*)data.c_str(), data.length() + 1);
	
	if (code){
		return;			//Grab init failed....
	}


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

		if (!bStop){
			pThis->SendMsg(CAMERA_FRAME, Buffer, dwLen, FALSE);	//编码后的,不要压缩.
		}
		//采集速度好像就是30fps/s,这里不用限制了,
		//网络延迟加队列缓存造成服务器不能实时显示画面,图像越大,由于带宽较小,延迟越高
		/*DWORD dwUsedTime = (GetTickCount() - dwLastTime);
		while ((GetTickCount() - dwLastTime) < (1000 / 30))
			Sleep(1);*/
	}
	InterlockedExchange(&pThis->m_bStop, TRUE);
}

void CCamera::OnStop()
{
	m_Grab.StopGrab();						//停止捕捉
	InterlockedExchange(&m_bStop, TRUE);	//停止发送

	if (m_hWorkThread){
		WaitForSingleObject(m_hWorkThread,INFINITE);		//等待线程退出
		CloseHandle(m_hWorkThread);
		m_hWorkThread = NULL;
	}
	//关闭完成.
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
				OnStart(root["device"].asString(),
					root["width"].asInt(), root["height"].asInt());
			}
		} while (0);
		break;
	case CAMERA_STOP:
		OnStop();
		break;
	default:
		break;
	}
}