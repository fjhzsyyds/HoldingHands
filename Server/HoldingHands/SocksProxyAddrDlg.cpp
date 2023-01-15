// SocksProxyAddrDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "SocksProxyAddrDlg.h"
#include "afxdialogex.h"


// CSocksProxyAddrDlg 对话框

IMPLEMENT_DYNAMIC(CSocksProxyAddrDlg, CDialogEx)

CSocksProxyAddrDlg::CSocksProxyAddrDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSocksProxyAddrDlg::IDD, pParent)
	, m_Port(8080)
	, m_Addr(0)
{

}

CSocksProxyAddrDlg::~CSocksProxyAddrDlg()
{
}

void CSocksProxyAddrDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IPADDRESS1, m_Address);
	DDX_Text(pDX, IDC_EDIT1, m_Port);
	DDX_IPAddress(pDX, IDC_IPADDRESS1, m_Addr);
}


BEGIN_MESSAGE_MAP(CSocksProxyAddrDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSocksProxyAddrDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CSocksProxyAddrDlg 消息处理程序


void CSocksProxyAddrDlg::OnBnClickedOk()
{
	UpdateData();
	m_Address.GetAddress(m_Addr);
	CDialogEx::OnOK();
}
