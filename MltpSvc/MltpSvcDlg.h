
// MltpSvcDlg.h: 头文件
//

#pragma once


// CMltpSvcDlg 对话框
class CMltpSvcDlg : public CDialogEx
{
// 构造
public:
	CMltpSvcDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MltpSvc_DIALOG };
	enum { IDD = IDD_DIALOG_REG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	CMenu m_Menu;
	NOTIFYICONDATA m_nid;
	

	CDialogEx m_TablDialog_Reg;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

public:

	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnClose(); //响应关闭事件！

	BOOL DestroyWindow();
	void OnSize(UINT nType, int cx, int cy);

	afx_msg void OnExit();
	afx_msg void AddTrayIcon();
	BOOL ShowBalloonTip(LPCWSTR szMsg, LPCWSTR szTitle, UINT uTimeOut, DWORD dwInfoFlags);
	LRESULT OnSystemTray(WPARAM wParam, LPARAM lParam);

	afx_msg void OnManual();
	afx_msg void OnVideoTutor();
	afx_msg void OnAbout();
	afx_msg void OnNMClickSyslinkWebsite(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClickedRegistry();
	afx_msg void OnNMClickSyslinkEula(NMHDR* pNMHDR, LRESULT* pResult);

	void init();
	void ShowImage();
	BOOL CMltpSvcDlg::LoadImageFromRes(CImage* pImage, UINT nResID, LPCTSTR lpTyp);

	CStatic m_Static_IconSmall;
	CStatic m_Static_Sketch;
	afx_msg void OnStnClickedStaticSketchimage();
	afx_msg void OnStnClickedStaticVer();
	CButton m_Button_Reg;
	CStatic m_Static_RegInfo;

	afx_msg void OnTimer(UINT_PTR nIDEvent);
	CStatic m_Static_Ver;
	void CheckRegStatus();
	bool IsRegistered;


	afx_msg void OnNMClickSyslinkVideotutor(NMHDR* pNMHDR, LRESULT* pResult);

	BOOL ReadBinReg(LPCWSTR szPath, LPCWSTR szKey, LPBYTE dwValue, DWORD* dwSize);

	BOOL LogFileExist(LPCWSTR szFileName);
};



