#pragma once
#include "afxcmn.h"


// CSocksProxyAddrDlg �Ի���

class CSocksProxyAddrDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSocksProxyAddrDlg)

public:
	CSocksProxyAddrDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CSocksProxyAddrDlg();

// �Ի�������
	enum { IDD = IDD_SOCKS_ADDR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	CIPAddressCtrl m_Address;
	afx_msg void OnBnClickedOk();
	DWORD m_Port;
	DWORD m_Addr;
};
