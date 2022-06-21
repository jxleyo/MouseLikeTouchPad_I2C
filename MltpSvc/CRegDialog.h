#pragma once


// CRegDialog 对话框

class CRegDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CRegDialog)

public:
	CRegDialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CRegDialog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_REG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:

	afx_msg void OnBnClickedButtonRegok();
	CEdit m_Edit_RegInput;

};
