
// HoldingHands.h : HoldingHands Ӧ�ó������ͷ�ļ�
//
#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"       // ������


// CHoldingHandsApp:
// �йش����ʵ�֣������ HoldingHands.cpp
//

class CHoldingHandsApp : public CWinApp
{
public:
	CHoldingHandsApp();
// ��д
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// ʵ��

public:
	CShellManager*	m_pShellManager;
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CHoldingHandsApp theApp;
