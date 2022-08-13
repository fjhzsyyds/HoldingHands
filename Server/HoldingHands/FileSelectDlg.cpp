// FileSelectDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "HoldingHands.h"
#include "FileSelectDlg.h"
#include "afxdialogex.h"


// CFileSelectDlg �Ի���

IMPLEMENT_DYNAMIC(CFileSelectDlg, CDialogEx)

CFileSelectDlg::CFileSelectDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFileSelectDlg::IDD, pParent)
	, m_FileList(_T(""))
{
}

CFileSelectDlg::~CFileSelectDlg()
{
}

void CFileSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCSHELLLIST1, m_List);
	DDX_Control(pDX, IDC_MFCSHELLTREE1, m_Tree);
	DDX_Control(pDX, IDC_EDIT1, m_SelFiles);
}


BEGIN_MESSAGE_MAP(CFileSelectDlg, CDialogEx)
	
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_MFCSHELLLIST1, &CFileSelectDlg::OnLvnItemchangedMfcshelllist1)
	ON_NOTIFY(LVN_DELETEALLITEMS, IDC_MFCSHELLLIST1, &CFileSelectDlg::OnLvnDeleteallitemsMfcshelllist1)
	ON_BN_CLICKED(IDOK, &CFileSelectDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CFileSelectDlg ��Ϣ�������



BOOL CFileSelectDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	
	m_List.DeleteAllItems();
	m_Tree.SetRelatedList(&m_List);

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣:  OCX ����ҳӦ���� FALSE
}


void CFileSelectDlg::OnLvnItemchangedMfcshelllist1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	*pResult = 0;
	CString Text;
	//��ѡ����
	if (!(pNMLV->uOldState &LVIS_SELECTED ) && (pNMLV->uNewState &LVIS_SELECTED))
	{
		m_MapFileList.SetAt(m_List.GetItemText(pNMLV->iItem, 0), 0);
		POSITION pos = m_MapFileList.GetStartPosition();
		while (pos)
		{
			CString Key; void* value;
			m_MapFileList.GetNextAssoc(pos, Key, value);
			Text += L"\"" + Key + L"\" ";
		}
		m_SelFiles.SetWindowTextW(Text);
	}
	//��ȡ����
	else if (!(pNMLV->uNewState &LVIS_SELECTED )&& (pNMLV->uOldState &LVIS_SELECTED))
	{
		m_MapFileList.RemoveKey(m_List.GetItemText(pNMLV->iItem, 0));
		POSITION pos = m_MapFileList.GetStartPosition();
		while (pos)
		{
			CString Key; void* value;
			m_MapFileList.GetNextAssoc(pos, Key, value);
			Text += L"\"" + Key + L"\"";
		}
		m_SelFiles.SetWindowTextW(Text);
	}
}


void CFileSelectDlg::OnLvnDeleteallitemsMfcshelllist1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	*pResult = 0;
	//��û�д���edit֮ǰҲ������������mmp.
	if (m_SelFiles.m_hWnd)
	{
		m_MapFileList.RemoveAll();
		m_SelFiles.SetWindowTextW(L"");
	}
}


void CFileSelectDlg::OnBnClickedOk()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	CString CurDir;
	if (m_List.GetCurrentFolder(CurDir))
	{
		m_FileList = CurDir;
		m_FileList += "\n";

		POSITION pos = m_MapFileList.GetStartPosition();
		while (pos)
		{
			CString Key; void* value;
			m_MapFileList.GetNextAssoc(pos, Key, value);
			m_FileList += Key;
			m_FileList +="\n";
		}
		if (m_FileList.GetLength())
			MessageBox(m_FileList,L"You has selected these files:");
	}
	else
		m_FileList = L"";		//û��·��
	CDialogEx::OnOK();
}
