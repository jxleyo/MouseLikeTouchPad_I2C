// MouseLikeTouchPad_Svc.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "MouseLikeTouchPad_Svc.h"

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

HWND hMainDlgWnd;
HICON hDlgIcon;


// 此代码模块中包含的函数的前向声明:
LRESULT CALLBACK    MainWndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    
    //只允许程序有一个实例运行
    HANDLE hMutex = CreateMutex(NULL, FALSE, TEXT("OnlyMouseLikeTouchPad_DrvSvcTag"));//第一个参数一定是NULL不然没有用
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        if (hMutex)CloseHandle(hMutex);
        return 0;
    }

    hInst = hInstance; // 将实例句柄存储在全局变量中

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MOUSELIKETOUCHPADSVC, szWindowClass, MAX_LOADSTRING);

    // 执行应用程序初始化:
    //创建对话框
    hDlgIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_MOUSELIKETOUCHPADSVC));//MAKEINTRESOURCE(IDI_SMALL)//(LPCWSTR)IDI_SMALL)
    HWND hDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, (DLGPROC)MainWndProc);//NULL//hMainWnd//GetDesktopWindow()

    hMainDlgWnd = hDlg;
    SetWindowText(hDlg, szTitle);//设置标题栏名称szTitle
     //设置标题栏图标
    SendMessage(hMainDlgWnd, WM_SETICON, ICON_SMALL, (long)hDlgIcon);//ICON_BIG 设置窗口的大图标（即Alt + Tab图标）/ICON_SMALL 设置窗口的小图标（即窗口标题栏图标）

    ShowWindow(hDlg, SW_SHOWNORMAL);//显示主对话框窗口
    UpdateWindow(hDlg);

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}



//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
                case IDOK:
                    DestroyWindow(hWnd);
                    break;

                case IDC_BUTTON_REG:
                    //MessageBox(hWnd,L"test",L"",MB_OK);
                    break;

                case IDCANCEL:
                    DestroyWindow(hWnd);
                    break;

            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
