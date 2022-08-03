#pragma once
#include "..\Kernel\Kernel.h"

#define MAX_MODULE_COUNT 32

class CModuleMgr
{
	friend class CKernel;
	struct Module{
		char szName[32];
		LPVOID lpImageBase;
		LPVOID lpEntry;
	};
private:
	char*  m_pCurModule;
	HANDLE m_hModuleTrans;
	CKernel*m_pKernel;

	HANDLE m_hFree;
	Module m_Modules[MAX_MODULE_COUNT];


	BOOL ModuleLoaded(const char*name);
	BOOL IsBusy();

	struct RunCtx;
	static void  __stdcall ThreadProc(RunCtx*pRunCtx);

	void GetModule(const char*name);
	void WaitTransFinished();

	LPVOID MyGetProcAddress(HMODULE hModule, const char*ProcName);
	int LoadFromMem(const char*image, LPVOID *lppImageBase, LPVOID *lppEntry);

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

