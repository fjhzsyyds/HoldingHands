#pragma once
#include "..\Kernel\Kernel.h"

#define MAX_MODULE_COUNT 32
#include <map>
#include <string>

using std::string;
using std::map;
using std::pair;


typedef void(*ModuleEntry)(char* szServerAddr, unsigned short uPort, LPVOID lpParam);

class CModuleMgr
{
	friend class CKernel;
	class Module{
	public:
		LPVOID		lpImageBase;
		ModuleEntry lpEntry;

		Module():
		lpImageBase(0),lpEntry(0){
		}
		Module(LPVOID ImageBase, ModuleEntry Entry):
			lpImageBase(ImageBase),
			lpEntry(Entry){
		}
	};
private:
	string m_current_module_name;
	HANDLE m_hModuleTrans;
	CKernel*m_pKernel;

	HANDLE m_hFree;
	//ÒÑ¼ÓÔØÄ£¿é...
	map<string, Module> m_loaded;

	BOOL ModuleLoaded(const char*name);
	BOOL IsBusy();

	struct RunCtx;
	static void  __stdcall ThreadProc(RunCtx*pRunCtx);

	void GetModule(const char*name);
	void WaitTransFinished();

	LPVOID MyGetProcAddress(HMODULE hModule, const char*ProcName);
	int LoadFromMem(const char*image, LPVOID *lppImageBase, ModuleEntry *lppEntry);

public:
	struct RunCtx{
		CModuleMgr*pMgr;
		char szName[32];
		char szServerAddr[32];
		UINT uPort;
		LPVOID lpParam;
	};

	BOOL RunModule(const char*name, const char*ServerAddr, UINT uPort, LPVOID lpParam);
	void LoadModule(const char*buff,int size);
	
	CModuleMgr(CKernel*pKernel);
	~CModuleMgr();
};

