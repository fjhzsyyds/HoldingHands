#pragma once
#include <Windows.h>
#include<mmreg.h>
#include <list>

using std::list;
using std::pair;


#define BUFF_COUNT 16
#define LEN_PER_BUFF 0x2000

class CAudioGrab
{

private:
	DWORD		  m_dwBufSize;
	
	list<pair<char*, DWORD>> m_buffer_list;
	
	HANDLE		  m_hEvent;
	HANDLE		  m_hMutex;

	HWAVEIN		  m_hWaveIn;		//�����豸

	volatile long m_bStop;

	WAVEFORMATEX	 m_Wavefmt;	//��ʽ
	
	void  RemoveHead(char**ppBuffer, DWORD*pLen);
	void  AddTail(char*Buffer, DWORD dwLen);

	WAVEHDR		m_hdrs[BUFF_COUNT];
	char		m_Buffers[BUFF_COUNT][LEN_PER_BUFF];	//
	UINT		m_Idx;

	HANDLE		m_hWorkThread;
	DWORD		m_dwThreadID;
	static void __stdcall WorkThread(CAudioGrab*pThis);
	
public:
	BOOL	GrabInit();
	void	GrabTerm();

	BOOL	StartGrab();
	void	StopGrab();

	BOOL	GetBuffer(void**ppBuffer,DWORD*pBufLen);

	CAudioGrab();
	~CAudioGrab();
};

