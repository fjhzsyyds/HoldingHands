#pragma once


// CFileMgrInputNameDlg �Ի���

class CFileMgrInputNameDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFileMgrInputNameDlg)

public:
	CFileMgrInputNameDlg(CString FileName,CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CFileMgrInputNameDlg();

// �Ի�������
	enum { IDD = IDD_FILE_NAME_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	CString m_FileName;
	afx_msg void OnBnClickedOk();
};
