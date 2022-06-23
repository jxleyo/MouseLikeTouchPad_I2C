// CRegDialog.cpp: 实现文件
//

#include "pch.h"
#include "MltpSvc.h"
#include "CRegDialog.h"
#include "afxdialogex.h"


// CRegDialog 对话框

IMPLEMENT_DYNAMIC(CRegDialog, CDialogEx)

CRegDialog::CRegDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_REG, pParent)
{

}

CRegDialog::~CRegDialog()
{
}

void CRegDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_REGINPUT, m_Edit_RegInput);
}


BEGIN_MESSAGE_MAP(CRegDialog, CDialogEx)

	ON_BN_CLICKED(IDC_BUTTON_REGOK, &CRegDialog::OnBnClickedButtonRegok)
END_MESSAGE_MAP()



// CRegDialog 消息处理程序




void CRegDialog::OnBnClickedButtonRegok()
{
	CString strInput;
	m_Edit_RegInput.GetWindowText(strInput);
	if (strInput == L"JXLEYO-HRP-MLTP-DRVR") {//验证注册码输入//SN注册序列号字符串,实际软件注册验证不检测内容，只有注册文件存在即表示成功

		AfxMessageBox(L"注册软件需要管理员权限，点击确定后请选择允许更改计算机!", MB_OK, MB_ICONINFORMATION);
		INT ret = (INT)ShellExecute(NULL, L"open", L"MltpDrvMgr.exe", L"Register", NULL, SW_NORMAL);//调用注册程序
		if (ret <= 32) {
			AfxMessageBox(L"ShellExecute Call MltpDrvMgr.exe err!", MB_OK, MB_ICONSTOP);
		}

		//ShellExecute(NULL, L"open", L"https://github.com/jxleyo/MouseLikeTouchPad_I2C/releases", NULL, NULL, SW_SHOWNORMAL);
	}
	else {
		AfxMessageBox(L"验证码错误，请重新输入!", MB_OK, MB_ICONSTOP);
	}
	
}


