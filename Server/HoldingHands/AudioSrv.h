#pragma once
#include "MsgHandler.h"

#include<mmreg.h>
#include<mmsystem.h>

#define AUDIO		('A'|'U'<<8|'D'<<16|'O'<<24)
#define AUDIO_ERROR (0x0000)

#define AUDIO_BEGIN (0xaaa0)
#define AUDIO_DATA	(0xaaa1)
#define AUDIO_STOP  (0xaaa2)

#define BUFF_COUNT 16
#define LEN_PER_BUFF 0x2000


class CAudioDlg;
class CAudioSrv :
	public CMsgHandler
{
private:
	WAVEHDR	 m_hdrs[BUFF_COUNT];
	char	 buffs[BUFF_COUNT][LEN_PER_BUFF];
	HWAVEOUT m_hWaveOut;
	WAVEFORMATEX m_WaveFmt;	//¸ñÊ½
	DWORD	 m_Idx;

	CAudioDlg*	m_pDlg;
	BOOL AudioOutInit();
	void AudioOutTerm();
public:
	void OnOpen();
	void OnClose();
	void OnAudioData(char*Buffer,DWORD dwLen);

	void OnAudioError(TCHAR*szError);

	void OnReadMsg(WORD Msg, DWORD dwSize, char*Buffer);
	void OnWriteMsg(WORD Msg, DWORD dwSize, char*Buffer) { }; 

	CAudioSrv(CManager*pManager);
	~CAudioSrv();
};

