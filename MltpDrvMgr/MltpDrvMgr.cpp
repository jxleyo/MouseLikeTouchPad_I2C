// MltpDrvMgr.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "MltpDrvMgr.h"

#define MAX_LOADSTRING 100

static wchar_t OEM_DevDescription[] = L"MouseLikeTouchPad_I2C";
static wchar_t OEM_ProviderName[] = L"jxleyo.HRP";
static wchar_t OEM_MfgName[] = L"jxleyo.HRP";
static wchar_t I2C_COMPATIBLE_hwID[] = L"ACPI\\PNP0C50";
static wchar_t TouchPad_COMPATIBLE_hwID[] = L"HID_DEVICE_UP:000D_U:0005";
static wchar_t TouchPad_hwID[MAX_DEVICE_ID_LEN];
static wchar_t TouchPad_I2C_hwID[MAX_DEVICE_ID_LEN];
static wchar_t inf_FullPathName[MAX_PATH];
static wchar_t inf_name[] = L"Driver\\MouseLikeTouchPad_I2C.inf";
static wchar_t OEMinf_FullName[MAX_PATH];
static wchar_t OEMinf_name[MAX_PATH];
static BOOLEAN bOEMDriverExist;//存在驱动标志

static BOOLEAN bTouchPad_FOUND = FALSE;
static BOOLEAN bTouchPad_I2C_FOUND = FALSE;
static INT ExitCode = EXIT_OK;



static wchar_t exeFilePathName[MAX_PATH + 1] = { 0 };
static wchar_t exeFilePath[MAX_PATH + 1] = { 0 };
static wchar_t exeFileName[MAX_PATH + 1] = { 0 };

static char cmdFilePath[MAX_PATH + 1] = { 0 };

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名


//

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    //获取系统版本
    OSVERSIONINFOW verInfo = {0};
    verInfo.dwOSVersionInfoSize = sizeof(verInfo);

    GetVersionEx(&verInfo);//需要添加manifest清单文件

    //ULONG a = (ULONG)(verInfo.dwMajorVersion); // 这是你的变量
    //CString str; // 这是MFC中的字符串变量
    //str.Format(L"dwMajorVersion=%d", a); // 调用Format方法将变量转换成字符串，第一个参数就是变量类型
    //MessageBox(NULL, str, L"MltpDrvMgr", MB_OK);

    //a = (ULONG)(verInfo.dwBuildNumber); // 这是你的变量
    //str.Format(L"dwBuildNumber=%d", a); // 调用Format方法将变量转换成字符串，第一个参数就是变量类型
    //MessageBox(NULL, str, L"MltpDrvMgr", MB_OK);

    if (verInfo.dwMajorVersion < 10) {
        MessageBox(NULL, L"不支持Windows10/11 x64以外的系统版本", L"MltpDrvMgr", MB_OK);
        return EXIT_FAIL;
    }

    if (verInfo.dwBuildNumber < 19041) {//Win10 Build 19041.208(v2004)正式版
        MessageBox(NULL, L"Windows版本低于Win10 Build 19041.208(v2004)正式版，请升级系统后再试。", L"MltpDrvMgr", MB_OK);
        return EXIT_FAIL;
    }

    //EnbalePrivileges();

    //GetModuleFileName(NULL, exeFilePath, MAX_PATH);
    //(wcsrchr(exeFilePath, '\\'))[0] = 0; // 删除文件名，只获得路径字串  
    CString m_FilePathName, m_FilePath;//当前程序全路径名、目录路径
    GetModuleFileName(NULL, m_FilePathName.GetBufferSetLength(MAX_PATH + 1), MAX_PATH);
    wcscpy_s(exeFilePathName, m_FilePathName.AllocSysString());//    PathRemoveFileSpec
    int pos = m_FilePathName.ReverseFind('\\');
    m_FilePath = m_FilePathName.Left(pos);
    wcscpy_s(exeFilePath, m_FilePath.AllocSysString());
    wcscpy_s(exeFileName, exeFilePathName + pos+1);

    //wchar_t str[MAX_PATH];
    //wcscpy_s(str, L"exeFilePathName=");
    //wcscat_s(str, exeFilePathName);
    //MessageBox(NULL, str, L"MltpDrvMgr", MB_OK);
    //memset(str, 0, MAX_PATH);
    //wcscpy_s(str, L"exeFilePath=");
    //wcscat_s(str, exeFilePath);
    //MessageBox(NULL, str, L"MltpDrvMgr", MB_OK);
    //memset(str, 0, MAX_PATH);
    //wcscpy_s(str, L"exeFileName=");
    //wcscat_s(str, exeFileName);
    //MessageBox(NULL, str, L"MltpDrvMgr", MB_OK);

    //
     // 获得INF目录
    char szPath[MAX_PATH];  // 获得系统目录
    GetWindowsDirectoryA(szPath, sizeof(szPath));
    // 格式化文件路径
    strcat_s(szPath, "\\system32");
    strcpy_s(cmdFilePath, szPath);

    //赋值源驱动inf路径
    wcscpy_s(inf_FullPathName, exeFilePath);
    wcscat_s(inf_FullPathName, L"\\");
    wcscat_s(inf_FullPathName, inf_name);

    
    //检测文件
    if (!MainFileExist(L"InstDrv.bat")) {
        MessageBox(NULL, L"InstDrv.bat文件丢失，请重新下载驱动包", L"MltpDrvMgr", MB_OK);
        return EXIT_FAIL;
    }
    if (!MainFileExist(L"Uninst.bat")) {
        MessageBox(NULL, L"Uninst.bat文件丢失，请重新下载驱动包", L"MltpDrvMgr", MB_OK);
        return EXIT_FAIL;
    }

    if (!DrvFileExist(L"MouseLikeTouchPad_I2C.inf")) {
        MessageBox(NULL, L"MouseLikeTouchPad_I2C.inf文件丢失，请重新下载驱动包", L"MltpDrvMgr", MB_OK);
        return EXIT_FAIL;
    }
    if (!DrvFileExist(L"MouseLikeTouchPad_I2C.cat")) {
        MessageBox(NULL, L"MouseLikeTouchPad_I2C.inf文件丢失，请重新下载驱动包", L"MltpDrvMgr", MB_OK);
        return EXIT_FAIL;
    }
    if (!DrvFileExist(L"MouseLikeTouchPad_I2C.sys")) {
        MessageBox(NULL, L"MouseLikeTouchPad_I2C.inf文件丢失，请重新下载驱动包", L"MltpDrvMgr", MB_OK);
        return EXIT_FAIL;
    }

    if (!DirExist(L"LogFile")) {
        TCHAR szPath[MAX_PATH];
        wcscpy_s(szPath, exeFilePath);
        wcscat_s(szPath, L"\\LogFile");
        CreateDirectory(szPath, NULL);
    }
    

    //无界面模式的命令
    if (wcscmp(lpCmdLine, L"Install") == 0) {
        Install();//二次调用本程序
    }
    else if (wcscmp(lpCmdLine, L"Uninstall") == 0) {
        Uninstall(); //调用本程序
    }
    else if (wcscmp(lpCmdLine, L"Rescan") == 0) {//实际执行Rescan
        Rescan();
    }
    else if (wcscmp(lpCmdLine, L"FindDevice") == 0) {//实际执行FindDevice
        FindDevice();
    }
    else if (wcscmp(lpCmdLine, L"InstallDriver") == 0) {//实际执行安装
        InstallDriver();
    }
    else if (wcscmp(lpCmdLine, L"UninstallDriver") == 0) {//实际执行卸载
        UninstallDriver();
    }
    else if (wcscmp(lpCmdLine, L"InstallShortcut") == 0) {//安装快捷方式
        InstallShortcut();
    }
    else if (wcscmp(lpCmdLine, L"UninstallShortcut") == 0) {//卸载快捷方式
        UninstallShortcut();
    }
    else if (wcscmp(lpCmdLine, L"Register") == 0) {//删除开机启动项
        Register();
    }
    else if (wcscmp(lpCmdLine, L"UnStartup") == 0) {//删除开机启动项
        UnStartup();
    }
    //else if (wcscmp(lpCmdLine, L"") == 0) {//无参数对话框模式

    //}
    else {//无参数对话框模式
        _tprintf(TEXT("%s [Command]\n"), exeFileName);
        printf("[Install]: Auto Find Device and InstallDriver\n");
        printf("[Uninstall]: Auto Find Device and UninstallDriver\n");
        printf("[Rescan]: Rescan Device\n");
        printf("[FindDevice]: Find Device and Save Result to File\n");
        printf("[InstallDriver]: InstallDriver\n");
        printf("[UninstallDriver]: UninstallDriver\n");
        ExitCode = EXIT_USAGE;
    }

    return ExitCode;
}


BOOL EnbalePrivileges()
{
    HANDLE Token;
    TOKEN_PRIVILEGES NewPrivileges;
    LUID Luid;

    //
    // we need to "turn on" reboot privilege
    // if any of this fails, try reboot anyway
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &Token)) {//TOKEN_ADJUST_PRIVILEGES//TOKEN_ALL_ACCESS 
        //printf("OpenProcessToken failed!\n");
        return FALSE;
    }

    if (!LookupPrivilegeValue(NULL, SE_LOAD_DRIVER_NAME, &Luid)) {//SE_LOAD_DRIVER_NAME//SE_SHUTDOWN_NAME 
        CloseHandle(Token);
        //printf("LookupPrivilegeValue failed!\n");
        return FALSE;
    }

    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Luid = Luid;
    NewPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    BOOL bRet = AdjustTokenPrivileges(Token, FALSE, &NewPrivileges, 0, NULL, NULL);
    if (bRet == FALSE)
    {
        CloseHandle(Token);
        //printf("AdjustTokenPrivileges failed!\n");
        return FALSE;
    }

    CloseHandle(Token);
    // 根据错误码判断是否特权都设置成功
    DWORD dwRet = 0;
    dwRet = GetLastError();
    if (ERROR_SUCCESS == dwRet)
    {
        //printf("EnbalePrivileges ok!\n");
        return TRUE;
    }
    else if (ERROR_NOT_ALL_ASSIGNED == dwRet)
    {
        //printf("ERROR_NOT_ALL_ASSIGNED!\n");
        return FALSE;
    }


    //printf("EnbalePrivileges end!\n");
    return TRUE;

}

BOOL Rescan()
{

    // reenumerate from the root of the devnode tree
    // totally CM based

    DEVINST devRoot;
    int failcode;

    failcode = CM_Locate_DevNode(&devRoot, NULL, CM_LOCATE_DEVNODE_NORMAL);
    if (failcode != CR_SUCCESS) {
        //printf("CM_Locate_DevNode failed!\n");
        return FALSE;
    }

    failcode = CM_Reenumerate_DevNode(devRoot, 0);
    if (failcode != CR_SUCCESS) {
        //printf("CM_Reenumerate_DevNode failed!\n");
        return FALSE;
    }

    //printf("Rescan ok!\n");
    return TRUE;

}


BOOL EnumerateDevices()
{
    BOOLEAN FOUND = FALSE;
    GUID     hidGuid;
    HidD_GetHidGuid(&hidGuid);

    HDEVINFO hDevInfo = SetupDiGetClassDevs(
        &hidGuid,
        NULL,
        NULL,
        (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        //printf("SetupDiGetClassDevs failed!\n");
        return 0;
    }

    SP_DEVICE_INTERFACE_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    int deviceNo = 0;

    SetLastError(NO_ERROR);
    //printf("while start\n");

    while (GetLastError() != ERROR_NO_MORE_ITEMS)
    {
        //printf("deviceNo=%d\n", deviceNo);
        if (SetupDiEnumInterfaceDevice(hDevInfo,
            0,
            &hidGuid,
            deviceNo,
            &devInfoData))
        {
            ULONG requiredLength = 0;
            SetupDiGetInterfaceDeviceDetail(hDevInfo,
                &devInfoData,
                NULL,
                0,
                &requiredLength,
                NULL);


            PSP_INTERFACE_DEVICE_DETAIL_DATA devDetail = (SP_INTERFACE_DEVICE_DETAIL_DATA*)malloc(requiredLength);
            devDetail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

            if (!SetupDiGetInterfaceDeviceDetail(hDevInfo,
                &devInfoData,
                devDetail,
                requiredLength,
                NULL,
                NULL))
            {
                //printf("SetupDiGetInterfaceDeviceDetail failed!\n");
                free(devDetail);
                SetupDiDestroyDeviceInfoList(hDevInfo);
                return 0;
            }

            //printf("DevicePath=%S\n", devDetail->DevicePath);

            ++deviceNo;

        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return FOUND;
}


BOOL FindDevice()
{
    //FindDevice函数返回值不代表找到设备只是表面是否执行完毕，通过bTouchPad_FOUND、bTouchPad_I2C_FOUND和bOEMDriverExist的值来判定是否找到匹配的设备。

    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD devIndex;

    bTouchPad_FOUND = FALSE;
    bTouchPad_I2C_FOUND = FALSE;

    TCHAR devInstance_ID[MAX_DEVICE_ID_LEN];//instance ID
    LPTSTR* devHwIDs;
    LPTSTR* devCompatibleIDs;

    SetLastError(NO_ERROR);

    // Create a Device Information Set with all present devices.

    DeviceInfoSet = SetupDiGetClassDevs(NULL, // All Classes
        0,
        0,
        DIGCF_ALLCLASSES | DIGCF_PRESENT); // All devices present on system
    if (DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        printf("SetupDiGetClassDevs err!\n");
        return FALSE;
    }

    //wprintf(TEXT("Search for TouchPad_hwID Device ID: [%s]\n"), TouchPad_COMPATIBLE_hwID);

    // Enumerate through all Devices.
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (devIndex = 0; SetupDiEnumDeviceInfo(DeviceInfoSet, devIndex, &DeviceInfoData); devIndex++)
    {
        devHwIDs = NULL;
        devCompatibleIDs = NULL;

        // determine instance ID
        if (CM_Get_Device_ID(DeviceInfoData.DevInst, devInstance_ID, MAX_DEVICE_ID_LEN, 0) != CR_SUCCESS) {
            devInstance_ID[0] = TEXT('\0');
        }
        ////wprintf(TEXT("TouchPad_hwID devInstance_ID: [%s]\n"), devInstance_ID); 

        devHwIDs = GetDevMultiSz(DeviceInfoSet, &DeviceInfoData, SPDRP_HARDWAREID);
        devCompatibleIDs = GetDevMultiSz(DeviceInfoSet, &DeviceInfoData, SPDRP_COMPATIBLEIDS);

        if (FuzzyCompareHwIds(devHwIDs, TouchPad_COMPATIBLE_hwID) || FuzzyCompareHwIds(devCompatibleIDs, TouchPad_COMPATIBLE_hwID))
        {
            //printf("找到TouchPad触控板设备！\n");
            //wprintf(TEXT("TouchPad Device devInstance_ID= [%s]\n"), devInstance_ID);
            //wprintf(TEXT("TouchPad Device devHwIDs= [%s]\n"), devHwIDs[0]);
            wcscpy_s(TouchPad_hwID, devHwIDs[0]);
            wprintf(TEXT("TouchPad_hwID= [%s]\n"), TouchPad_hwID);
            NewLogFile(L"TouchPad_FOUND.txt");
           
            DelMultiSz(devHwIDs);
            DelMultiSz(devCompatibleIDs);

            bTouchPad_FOUND = TRUE;
            break;
        }

        DelMultiSz(devHwIDs);
        DelMultiSz(devCompatibleIDs);
    }


    if (!bTouchPad_FOUND) {
        printf("未找到TouchPad触控板设备！\n");
    }
    else {
        //生成I2C_hwID
        wchar_t* pstr;
        pstr = mystrcat(TouchPad_hwID, L"HID\\", L"ACPI\\");
        pstr = mystrcat(pstr, L"&Col02", L"\0");
        wcscpy_s(TouchPad_I2C_hwID, pstr);
        //wprintf(TEXT("TouchPad I2C_hwID= [%s]\n"), I2C_hwID);

        //验证I2C设备
        //printf("开始查找TouchPad触控板的I2C设备！\n");
        //wprintf(TEXT("Search for I2C_hwID Device ID: [%s]\n"), I2C_COMPATIBLE_hwID);

        for (devIndex = 0; SetupDiEnumDeviceInfo(DeviceInfoSet, devIndex, &DeviceInfoData); devIndex++)
        {
            devHwIDs = NULL;
            devCompatibleIDs = NULL;
            //
            // determine instance ID
            //
            if (CM_Get_Device_ID(DeviceInfoData.DevInst, devInstance_ID, MAX_DEVICE_ID_LEN, 0) != CR_SUCCESS) {
                printf("CM_Get_Device_ID err！\n");
                devInstance_ID[0] = TEXT('\0');
            }
            ////wprintf(TEXT("TouchPad I2C devInstance_ID: [%s]\n"), devInstance_ID);

            devHwIDs = GetDevMultiSz(DeviceInfoSet, &DeviceInfoData, SPDRP_HARDWAREID);
            devCompatibleIDs = GetDevMultiSz(DeviceInfoSet, &DeviceInfoData, SPDRP_COMPATIBLEIDS);

            if (FuzzyCompareHwIds(devHwIDs, TouchPad_I2C_hwID) && FuzzyCompareHwIds(devCompatibleIDs, I2C_COMPATIBLE_hwID))
            {
                //printf("找到TouchPad触控板的I2C设备！\n");
                //wprintf(TEXT("TouchPad I2C Device devInstance_ID= [%s]\n"), devInstance_ID);
                //wprintf(TEXT("TouchPad I2C Device devHwIDs= [%s]\n"), devHwIDs[0]);
                wprintf(TEXT("TouchPad_I2C_hwID= [%s]\n"), TouchPad_I2C_hwID);
                SaveTouchPad_I2C_hwID();
   

                DelMultiSz(devHwIDs);
                DelMultiSz(devCompatibleIDs);

                bTouchPad_I2C_FOUND = TRUE;
                break;
            }

            DelMultiSz(devHwIDs);
            DelMultiSz(devCompatibleIDs);
        }

    }

    if (!bTouchPad_I2C_FOUND) {
        printf("未找到TouchPad触控板的I2C设备！\n");
    }
    else {
        BOOLEAN ret = GetDeviceOEMdriverInfo(DeviceInfoSet, &DeviceInfoData);
        if (!ret || !bOEMDriverExist) {//不存在第三方OEM驱动) 
            printf("OEM Driver Not Exist！\n");
        }

        ////
        //wchar_t szPath[MAX_PATH];
        //DWORD dwRet;
        //dwRet = GetCurrentDirectory(MAX_PATH, szPath);
        //if (dwRet == 0)    //返回零表示得到文件的当前路径失败
        //{
        //    printf("GetCurrentDirectory failed (%d)", GetLastError());
        //}
        //wprintf(TEXT("GetCurrentDirectory= [%s]\n"), szPath);

        ////设置安装路径
        //Set_InstallationSources_Directory(szPath);

        //DiShowUpdateDevice(NULL, DeviceInfoSet, &DeviceInfoData, 0, FALSE);
        ////DiShowUpdateDriver(NULL, OEMinf_FullName, 0, FALSE);

        //
    }

    //printf("FindDevice end\n");
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    NewLogFile(L"Return_FindDevice.txt");
    return TRUE;
}


BOOL FindCurrentDriver(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo, _In_ PSP_DRVINFO_DATA DriverInfoData)
{
    SP_DEVINSTALL_PARAMS deviceInstallParams;
    WCHAR SectionName[LINE_LEN];
    WCHAR DrvDescription[LINE_LEN];
    WCHAR MfgName[LINE_LEN];
    WCHAR ProviderName[LINE_LEN];
    HKEY hKey = NULL;

    DWORD c;
    BOOL match = FALSE;


    ZeroMemory(&deviceInstallParams, sizeof(deviceInstallParams));
    deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if (!SetupDiGetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams)) {
        return FALSE;
    }

#ifdef DI_FLAGSEX_INSTALLEDDRIVER
    //
    // Set the flags that tell SetupDiBuildDriverInfoList to just put the
    // currently installed driver node in the list, and that it should allow
    // excluded drivers. This flag introduced in WinXP.
    //
    deviceInstallParams.FlagsEx |= (DI_FLAGSEX_INSTALLEDDRIVER | DI_FLAGSEX_ALLOWEXCLUDEDDRVS);

    if (SetupDiSetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams)) {
        //
        // we were able to specify this flag, so proceed the easy way
        // we should get a list of no more than 1 driver
        //
        if (!SetupDiBuildDriverInfoList(Devs, DevInfo, SPDIT_CLASSDRIVER)) {
            return FALSE;
        }
        if (!SetupDiEnumDriverInfo(Devs, DevInfo, SPDIT_CLASSDRIVER,
            0, DriverInfoData)) {
            return FALSE;
        }
        //
        // we've selected the current driver
        //
        return TRUE;
    }
    deviceInstallParams.FlagsEx &= ~(DI_FLAGSEX_INSTALLEDDRIVER | DI_FLAGSEX_ALLOWEXCLUDEDDRVS);
#endif

    //
    // now search for drivers listed in the INF
    //
    //
    deviceInstallParams.Flags |= DI_ENUMSINGLEINF;
    deviceInstallParams.FlagsEx |= DI_FLAGSEX_ALLOWEXCLUDEDDRVS;

    if (!SetupDiSetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams)) {
        return FALSE;
    }
    if (!SetupDiBuildDriverInfoList(Devs, DevInfo, SPDIT_CLASSDRIVER)) {
        return FALSE;
    }

    //
    // find the entry in the INF that was used to install the driver for
    // this device
    //
    for (c = 0; SetupDiEnumDriverInfo(Devs, DevInfo, SPDIT_CLASSDRIVER, c, DriverInfoData); c++) {
        if ((_tcscmp(DriverInfoData->MfgName, MfgName) == 0)
            && (_tcscmp(DriverInfoData->ProviderName, ProviderName) == 0)) {
            //
            // these two fields match, try more detailed info
            // to ensure we have the exact driver entry used
            //
            SP_DRVINFO_DETAIL_DATA detail;
            detail.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
            if (!SetupDiGetDriverInfoDetail(Devs, DevInfo, DriverInfoData, &detail, sizeof(detail), NULL)
                && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
                continue;
            }
            if ((_tcscmp(detail.SectionName, SectionName) == 0) &&
                (_tcscmp(detail.DrvDescription, DrvDescription) == 0)) {
                match = TRUE;
                break;
            }
        }
    }
    if (!match) {
        SetupDiDestroyDriverInfoList(Devs, DevInfo, SPDIT_CLASSDRIVER);
    }
    return match;
}


BOOL GetDeviceOEMdriverInfo(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo)
{
    SP_DRVINFO_DATA driverInfoData;
    SP_DRVINFO_DETAIL_DATA driverInfoDetail;

    BOOL bSuccess = FALSE;
    bOEMDriverExist = FALSE;

    ZeroMemory(&driverInfoData, sizeof(driverInfoData));
    driverInfoData.cbSize = sizeof(driverInfoData);


    //FindCurrentDriver
    SP_DEVINSTALL_PARAMS deviceInstallParams;

    ZeroMemory(&deviceInstallParams, sizeof(deviceInstallParams));
    deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if (!SetupDiGetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams)) {
        printf("SetupDiGetDeviceInstallParams Failed!\n");
        return FALSE;
    }

    deviceInstallParams.FlagsEx |= (DI_FLAGSEX_INSTALLEDDRIVER | DI_FLAGSEX_ALLOWEXCLUDEDDRVS);

    if (SetupDiSetDeviceInstallParams(Devs, DevInfo, &deviceInstallParams)) {
        // we were able to specify this flag, so proceed the easy way
        // we should get a list of no more than 1 driver
        if (!SetupDiBuildDriverInfoList(Devs, DevInfo, SPDIT_CLASSDRIVER)) {
            printf("SetupDiBuildDriverInfoList Failed!\n");
            return FALSE;
        }
        if (!SetupDiEnumDriverInfo(Devs, DevInfo, SPDIT_CLASSDRIVER,
            0, &driverInfoData)) {

            printf("SetupDiEnumDriverInfo Failed!\n");
            SetupDiDestroyDriverInfoList(Devs, DevInfo, SPDIT_CLASSDRIVER);
            return FALSE;
        }
    }
    else {
        printf("SetupDiSetDeviceInstallParams Failed!\n");
        return FALSE;
    }


    // get useful driver information
    driverInfoDetail.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    if (!SetupDiGetDriverInfoDetail(Devs, DevInfo, &driverInfoData, &driverInfoDetail, sizeof(SP_DRVINFO_DETAIL_DATA), NULL) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER) {

        // no information about driver or section
        printf("SetupDiGetDriverInfoDetail err!\n");
        goto final;
    }
    if (!driverInfoDetail.InfFileName[0] || !driverInfoDetail.SectionName[0]) {
        printf("driverInfoDetail err!\n");
        goto final;
    }

    //wprintf(TEXT("driverInfoDetail.InfFileName[0]: [%s]\n"), driverInfoDetail.InfFileName);


    // pretend to do the file-copy part of a driver install
    // to determine what files are used
    // the specified driver must be selected as the active driver
    if (!SetupDiSetSelectedDriver(Devs, DevInfo, &driverInfoData)) {
        printf("SetupDiSetSelectedDriver err!\n");
        goto final;
    }

    if (wcscmp(driverInfoData.Description, OEM_DevDescription) == 0 && wcscmp(driverInfoData.MfgName, OEM_MfgName) == 0 && wcscmp(driverInfoData.ProviderName, OEM_ProviderName) == 0) {
        //wprintf(TEXT("driverInfoData.Description: [%s]\n"), driverInfoData.Description);
        //wprintf(TEXT("driverInfoData.MfgName: [%s]\n"), driverInfoData.MfgName);
        //wprintf(TEXT("driverInfoData.ProviderName: [%s]\n"), driverInfoData.ProviderName);

        bOEMDriverExist = TRUE;//本OEM驱动存在
        wcscpy_s(OEMinf_FullName, driverInfoDetail.InfFileName);
        //wprintf(TEXT("OEMinf_FullName= [%s]\n"), OEMinf_FullName);

        // 获得INF目录
        WCHAR szPath[MAX_PATH];  // 获得系统目录
        GetWindowsDirectory(szPath, sizeof(szPath));
        // 格式化文件路径
        wcscat_s(szPath, L"\\INF\\");
        //wprintf(TEXT("INF PATH= [%s]\n"), szPath);

        //生成OEMinf_name
        wchar_t* pstr;
        pstr = mystrcat(OEMinf_FullName, szPath, L"\0");//szPath//L"C:\\WINDOWS\\INF\\"
        wcscpy_s(OEMinf_name, pstr);
        wprintf(TEXT("OEMinf_name= [%s]\n"), OEMinf_name);
        SaveOEMDriverName();

        //printf("GetDeviceOEMdriverInfo ok\n");
        bSuccess = TRUE;
    }
    else {
        wprintf(TEXT("driverInfoDetail.InfFileName[0]: [%s]\n"), driverInfoDetail.InfFileName);
    }


    final:

    SetupDiDestroyDriverInfoList(Devs, DevInfo, SPDIT_CLASSDRIVER);
    return bSuccess;

}


BOOL DirExist(LPCWSTR szFilePathName)
{
    if (GetFileAttributes(szFilePathName) == FILE_ATTRIBUTE_DIRECTORY) {
        return TRUE;
    }

    return FALSE;
}


BOOL FileExist(LPCWSTR szFilePathName)
{
    HANDLE hFile = CreateFile(szFilePathName,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE || GetLastError() == ERROR_FILE_NOT_FOUND) {
        CloseHandle(hFile);//关闭句柄
        return FALSE;
    }

    CloseHandle(hFile);
    return TRUE;
}


BOOL MainFileExist(LPCWSTR szFileName)
{
    WCHAR szFilePath[MAX_PATH];
    wcscpy_s(szFilePath, exeFilePath);
    wcscat_s(szFilePath, L"\\");
    wcscat_s(szFilePath, szFileName);

    HANDLE hFile = CreateFile(szFilePath,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE || GetLastError() == ERROR_FILE_NOT_FOUND) {
        CloseHandle(hFile);//关闭句柄
        return FALSE;
    }

    CloseHandle(hFile);
    return TRUE;
}

BOOL DrvFileExist(LPCWSTR szFileName)
{
    WCHAR szFilePath[MAX_PATH];
    wcscpy_s(szFilePath, exeFilePath);
    wcscat_s(szFilePath, L"\\Driver\\");
    wcscat_s(szFilePath, szFileName);

    HANDLE hFile = CreateFile(szFilePath,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE || GetLastError() == ERROR_FILE_NOT_FOUND) {
        CloseHandle(hFile);//关闭句柄
        return FALSE;
    }

    CloseHandle(hFile);
    return TRUE;
}


BOOL LogFileExist(LPCWSTR szFileName)
{
    WCHAR szFilePath[MAX_PATH];
    wcscpy_s(szFilePath, exeFilePath);
    wcscat_s(szFilePath, L"\\LogFile\\");
    wcscat_s(szFilePath, szFileName);

    HANDLE hFile = CreateFile(szFilePath,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
 
    if (hFile == INVALID_HANDLE_VALUE || GetLastError() == ERROR_FILE_NOT_FOUND) {
        CloseHandle(hFile);//关闭句柄
        return FALSE;
    }

    CloseHandle(hFile);
    return TRUE;
}


void DelLogFile(LPCWSTR szFileName)
{
    WCHAR szFilePath[MAX_PATH];
    wcscpy_s(szFilePath, exeFilePath);
    wcscat_s(szFilePath, L"\\LogFile\\");
    wcscat_s(szFilePath, szFileName);

    DeleteFile(szFilePath);
}


BOOL NewLogFile(LPCWSTR szFileName)
{
    WCHAR szFilePath[MAX_PATH];
    wcscpy_s(szFilePath, exeFilePath);
    wcscat_s(szFilePath, L"\\LogFile\\");
    wcscat_s(szFilePath, szFileName);

    HANDLE hFile = CreateFile(szFilePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE || GetLastError() == ERROR_FILE_NOT_FOUND) {
        CloseHandle(hFile);//关闭句柄
        return FALSE;
    }

    CloseHandle(hFile);
    return TRUE;

}


BOOL GetTouchPad_I2C_hwID()
{
    WCHAR szFilePath[MAX_PATH];
    wcscpy_s(szFilePath, exeFilePath);
    wcscat_s(szFilePath, L"\\LogFile\\");
    wcscat_s(szFilePath, L"TouchPad_I2C_FOUND.txt");

    HANDLE hFile = CreateFile(szFilePath,
        GENERIC_READ,
        0, //FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE || GetLastError() == ERROR_FILE_NOT_FOUND) {
        CloseHandle(hFile);//关闭句柄
        return FALSE;
    }

    DWORD filesize = GetFileSize(hFile, NULL);
    char* buffer = new char[filesize + 1];//最后一位为'/0',C-Style字符串的结束符。
    memset(buffer, 0, filesize + 1);
    DWORD readsize = 0;
    if (!ReadFile(hFile, buffer, filesize, &readsize, NULL)) {
        CloseHandle(hFile);//关闭句柄
        return FALSE;
    }

    CloseHandle(hFile);//关闭句柄
    buffer[filesize] = 0;


    char* pszMultiByte = buffer;
    int iSize;
    wchar_t* pwszUnicode;

    //返回接受字符串所需缓冲区的大小，已经包含字符结尾符'\0'
    iSize = MultiByteToWideChar(CP_ACP, 0, pszMultiByte, -1, NULL, 0);
    pwszUnicode = (wchar_t*)malloc(iSize * sizeof(wchar_t));
    MultiByteToWideChar(CP_ACP, 0, pszMultiByte, -1, pwszUnicode, iSize);


    wcscpy_s(TouchPad_I2C_hwID, pwszUnicode);

    //善后工作
    delete[]buffer;//注意是delete[]而不是delete
    return TRUE;

}


BOOL GetOEMDriverName()
{
    WCHAR szFilePath[MAX_PATH];
    wcscpy_s(szFilePath, exeFilePath);
    wcscat_s(szFilePath, L"\\LogFile\\");
    wcscat_s(szFilePath, L"OEMDriverName.txt");


    HANDLE hFile = CreateFile(szFilePath,
        GENERIC_READ,
        0, //FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE || GetLastError() == ERROR_FILE_NOT_FOUND) {
        CloseHandle(hFile);//关闭句柄
        return FALSE;
    }

 

    DWORD filesize = GetFileSize(hFile, NULL);
    char* buffer = new char[filesize + 1];//最后一位为'/0',C-Style字符串的结束符。
    memset(buffer, 0, filesize + 1);
    DWORD readsize = 0;
    if (!ReadFile(hFile, buffer, filesize, &readsize, NULL)) {
        CloseHandle(hFile);//关闭句柄
        return FALSE;
    }

    CloseHandle(hFile);//关闭句柄
    buffer[filesize] = 0;


    char* pszMultiByte = buffer;
    int iSize;
    wchar_t* pwszUnicode;

    //返回接受字符串所需缓冲区的大小，已经包含字符结尾符'\0'
    iSize = MultiByteToWideChar(CP_ACP, 0, pszMultiByte, -1, NULL, 0); 
    pwszUnicode = (wchar_t*)malloc(iSize * sizeof(wchar_t));
    MultiByteToWideChar(CP_ACP, 0, pszMultiByte, -1, pwszUnicode, iSize);


    wcscpy_s(OEMinf_name, pwszUnicode);

     // 获得INF目录
    WCHAR szPath[MAX_PATH];  // 获得系统目录
    GetWindowsDirectory(szPath, sizeof(szPath));
    // 格式化文件路径
    wcscat_s(szPath, L"\\INF\\");
    wcscat_s(szPath, OEMinf_name);
    wcscpy_s(OEMinf_FullName, szPath);


    //善后工作
    delete[]buffer;//注意是delete[]而不是delete
    return TRUE;
}


BOOL SaveTouchPad_I2C_hwID()
{
    WCHAR szFilePath[MAX_PATH];
    wcscpy_s(szFilePath, exeFilePath);
    wcscat_s(szFilePath, L"\\LogFile\\");
    wcscat_s(szFilePath, L"TouchPad_I2C_FOUND.txt");

    HANDLE hFile = CreateFile(szFilePath,
        GENERIC_READ | GENERIC_WRITE, 
        0, //FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE || ::GetLastError() == ERROR_FILE_NOT_FOUND) {
        CloseHandle(hFile);//关闭句柄
        return FALSE;
    }

    int iSize;
    char* pszMultiByte;

    //返回接受字符串所需缓冲区的大小，已经包含字符结尾符'\0'
    iSize = WideCharToMultiByte(CP_ACP, 0, TouchPad_I2C_hwID, -1, NULL, 0, NULL, NULL); //iSize =wcslen(TouchPad_I2C_hwID)+1
    pszMultiByte = (char*)malloc(iSize * sizeof(char)); //不需要 pszMultiByte = (char*)malloc(iSize*sizeof(char)+1);
    WideCharToMultiByte(CP_ACP, 0, TouchPad_I2C_hwID, -1, pszMultiByte, iSize, NULL, NULL);

    DWORD writesize = 0;
    WriteFile(hFile, pszMultiByte, iSize-1, &writesize, NULL);
    CloseHandle(hFile);//关闭句柄


    return TRUE;
}


BOOL SaveOEMDriverName()
{
    WCHAR szFilePath[MAX_PATH];
    wcscpy_s(szFilePath, exeFilePath);
    wcscat_s(szFilePath, L"\\LogFile\\");
    wcscat_s(szFilePath, L"OEMDriverName.txt");

    HANDLE hFile = CreateFile(szFilePath,
        GENERIC_READ | GENERIC_WRITE,
        0, //FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE || ::GetLastError() == ERROR_FILE_NOT_FOUND) {
        CloseHandle(hFile);//关闭句柄
        return FALSE;
    }


    int iSize;
    char* pszMultiByte;

    //返回接受字符串所需缓冲区的大小，已经包含字符结尾符'\0'
    iSize = WideCharToMultiByte(CP_ACP, 0, OEMinf_name, -1, NULL, 0, NULL, NULL); //iSize =wcslen(OEMinf_name)+1
    pszMultiByte = (char*)malloc(iSize * sizeof(char)); //不需要 pszMultiByte = (char*)malloc(iSize*sizeof(char)+1);
    WideCharToMultiByte(CP_ACP, 0, OEMinf_name, -1, pszMultiByte, iSize, NULL, NULL);

    DWORD writesize = 0;
    WriteFile(hFile, pszMultiByte, iSize-1, &writesize, NULL);
    CloseHandle(hFile);//关闭句柄

    return TRUE;
}


void Install() {
    INT nRet = 0;////-0 系统内存或资源不足//--ERROR_BAD_FORMAT.EXE文件格式无效（比如不是32位应用程序）//--ERROR_FILE_NOT_FOUND 指定的文件设有找到//--ERROR_PATH_NOT_FOUND
    INT retry = 0;
    INT nTry = 0;

    while (Rescan())break;//重新扫描设备

    //批处理模式安装微软ACPI\MSFT0001标准硬件ID驱动

    //清理历史记录文件
    DelLogFile(L"Return_InstDrv.txt");
    DelLogFile(L"InstDrvSucceeded.txt");

    retry = 0;
    nTry = 5;
    //char cmdLine[MAX_PATH];
    //memset(cmdLine, 0, MAX_PATH);
    //strcat_s(cmdLine, cmdFilePath);
    //strcpy_s(cmdLine, "\\cmd.exe /c InstDrv.bat");
    //nRet = WinExec(cmdLine, SW_HIDE);

    nRet = WinExec("cmd.exe /c InstDrv.bat", SW_HIDE);
    if (nRet > 30) {
waitInstBAT:
        Sleep(500);
        retry++;

        if (retry > nTry) {
            goto NextInstStep;//跳过批处理安装方式
        }

        if (LogFileExist(L"TouchPad_I2C_FOUND.txt")) {
            nTry=20;
        }

        if (!LogFileExist(L"Return_InstDrv.txt")) {//InstDrv.bat未执行结束
            goto waitInstBAT;
        }

        if (LogFileExist(L"InstDrvSucceeded.txt")) {//InstDrv.bat安装驱动成功
            DelLogFile(L"Return_InstDrv.txt");
            DelLogFile(L"InstDrvSucceeded.txt");
            goto InstSuccess;//直接跳到结尾
        }

        DelLogFile(L"Return_InstDrv.txt");
        DelLogFile(L"InstDrvSucceeded.txt");
    }


NextInstStep:
    //清理历史记录文件
    while (LogFileExist(L"Return_FindDevice.txt"))DelLogFile(L"Return_FindDevice.txt");
    while (LogFileExist(L"Return_UninstallDriver.txt"))DelLogFile(L"Return_UninstallDriver.txt");
    while (LogFileExist(L"TouchPad_FOUND.txt"))DelLogFile(L"TouchPad_FOUND.txt");

    retry = 0;
    nTry = 3;
FindDev:
    nRet = WinExec("MltpDrvMgr.exe FindDevice", SW_HIDE);
    if (nRet < 31) {
        MessageBox(NULL, L"WinExec调用FindDevice子程序失败！取消安装，请稍后再试。", L"MltpDrvMgr", MB_OK);
        ExitCode = EXIT_FAIL;
        goto Clean;
    }

    retry++;
    Sleep(1500);
    if (retry > nTry) {
        MessageBox(NULL, L"查找设备失败！取消安装，请稍后再试。", L"MltpDrvMgr", MB_OK);
        ExitCode = EXIT_FAIL;
        goto Clean;
    }

    if (!LogFileExist(L"TouchPad_I2C_FOUND.txt")) {
        nTry= 5;
    }

    if (!LogFileExist(L"Return_FindDevice.txt")) {//FindDevice未执行结束
        goto FindDev;
    }

    //FindDevice执行完毕
    DelLogFile(L"Return_FindDevice.txt");

    if (!LogFileExist(L"TouchPad_FOUND.txt")) {
        MessageBox(NULL, L"未找到匹配的触控板设备，无法安装驱动。", L"MltpDrvMgr", MB_OK);
        ExitCode = EXIT_FAIL;
        goto Clean;
    }
    DelLogFile(L"TouchPad_FOUND.txt");

    if (!LogFileExist(L"TouchPad_I2C_FOUND.txt")) {
        MessageBox(NULL, L"未找到匹配的I2C总线触控板设备，无法安装驱动。", L"MltpDrvMgr", MB_OK);
        ExitCode = EXIT_FAIL;
        goto Clean;
    }



    retry = 0;
InstDrv:
    nRet = WinExec("MltpDrvMgr.exe InstallDriver", SW_HIDE);
    if (nRet < 31) {
        MessageBox(NULL, L"WinExec调用InstallDriver子程序失败！请稍后再试。", L"MltpDrvMgr", MB_OK);
    }
    else {
        Sleep(3000);
        retry++;
        if (retry > 5) {
            MessageBox(NULL, L"InstallDriver安装驱动失败！请稍后再试。", L"MltpDrvMgr", MB_OK);
            ExitCode = EXIT_FAIL;
            goto Clean;
        }

        if (!LogFileExist(L"Return_InstallDriver.txt")) {//InstallDriver未执行结束
            goto InstDrv;
        }     
    }

    if (!LogFileExist(L"Return_InstallDriver.txt")) {//InstallDriver未执行结束
        MessageBox(NULL, L"找到匹配的I2C总线触控板设备，安装驱动失败，请稍后再试。", L"MltpDrvMgr", MB_OK);
        ExitCode = EXIT_FAIL;
        goto Clean;
    }
    DelLogFile(L"Return_InstallDriver.txt");


    while (Rescan())break;//重新扫描设备



    //再次验证
CheckDev:
    nRet = WinExec("MltpDrvMgr.exe FindDevice", SW_HIDE);
    if (nRet < 31) {
        MessageBox(NULL, L"二次验证，WinExec调用FindDevice子程序失败！取消安装，请稍后再试。", L"MltpDrvMgr", MB_OK);
        ExitCode = EXIT_FAIL;
        goto Clean;
    }

    retry++;
    Sleep(1500);
    if (retry > 3) {
        MessageBox(NULL, L"二次验证，查找设备失败！请稍后再试。", L"MltpDrvMgr", MB_OK);
        ExitCode = EXIT_FAIL;
        goto Clean;
    }

    if (!LogFileExist(L"Return_FindDevice.txt")) {//FindDevice未执行结束
        goto FindDev;
    }

    //FindDevice执行完毕
    DelLogFile(L"Return_FindDevice.txt");

    if (!LogFileExist(L"OEMDriverName.txt")) {
        MessageBox(NULL, L"二次验证，安装驱动失败，请稍后再试。", L"MltpDrvMgr", MB_OK);
        ExitCode = EXIT_FAIL;
        goto Clean;
    }
    

InstSuccess:
    //清理历史记录文件
    while (LogFileExist(L"Return_FindDevice.txt"))DelLogFile(L"Return_FindDevice.txt");
    while (LogFileExist(L"Return_UninstallDriver.txt"))DelLogFile(L"Return_UninstallDriver.txt");
    while (LogFileExist(L"TouchPad_FOUND.txt"))DelLogFile(L"TouchPad_FOUND.txt");

    //存在驱动
    MessageBox(NULL, L"安装驱动成功！请重新启动电脑生效！", L"MltpDrvMgr", MB_OK | MB_DEFBUTTON1);
    ExitCode = EXIT_OK;

    //创建快捷方式
    InstallShortcut();

    if (!LogFileExist(L"Installed.dat")) {//首次安装记录文件不存在
        NewLogFile(L"Installed.dat");//创建安装记录文件，属性带有时间供注册检测程序读取使用
    }

    //启动服务程序
    WinExec("MltpSvc.exe ShowDialog", SW_NORMAL);
    return;


Clean:
    //删除程序目录文件
    DelProgramFilesDir(exeFilePath);//注意提示对话框在删除之前运行避免冲突
}

void Uninstall() {
    //终止服务程序运行
    WinExec("taskkill /f /im MltpSvc.exe", SW_HIDE);

    INT nRet = 0;////-0 系统内存或资源不足//--ERROR_BAD_FORMAT.EXE文件格式无效（比如不是32位应用程序）//--ERROR_FILE_NOT_FOUND 指定的文件设有找到//--ERROR_PATH_NOT_FOUND
    INT retry = 0;
    INT nTry = 0;

    while (Rescan())break;//重新扫描设备

    //清理历史记录文件
    while(LogFileExist(L"Return_FindDevice.txt"))DelLogFile(L"Return_FindDevice.txt");
    while (LogFileExist(L"Return_UninstallDriver.txt"))DelLogFile(L"Return_UninstallDriver.txt");
    while (LogFileExist(L"TouchPad_FOUND.txt"))DelLogFile(L"TouchPad_FOUND.txt");


    retry = 0;
    nTry = 3;
FindDev:
    nRet = WinExec("MltpDrvMgr.exe FindDevice", SW_HIDE);
    if (nRet < 31) {
        MessageBox(NULL, L"WinExec调用FindDevice子程序失败！取消卸载，请稍后再试。", L"MltpDrvMgr", MB_OK);
        ExitCode = EXIT_FAIL;
        return;
    }

    retry++;
    Sleep(1500);
    if (retry > nTry) {
        MessageBox(NULL, L"查找设备失败！取消卸载，请稍后再试。", L"MltpDrvMgr", MB_OK);
        ExitCode = EXIT_FAIL;
        return;
    }

    if (!LogFileExist(L"TouchPad_I2C_FOUND.txt")) {
        nTry = 5;
    }

    if (!LogFileExist(L"Return_FindDevice.txt")) {//FindDevice未执行结束
        goto FindDev;
    }

    //FindDevice执行完毕
    DelLogFile(L"Return_FindDevice.txt");

    if (!LogFileExist(L"TouchPad_FOUND.txt")) {
        MessageBox(NULL, L"未找到匹配的触控板设备，无需卸载驱动，可直接卸载程序。", L"MltpDrvMgr", MB_OK);
        ExitCode = EXIT_OK;//这里不用安装，直接返回成功，但是跳到末尾清理程序
        goto UninstDrvSuccess;
    }
    DelLogFile(L"TouchPad_FOUND.txt");

    if (!LogFileExist(L"TouchPad_I2C_FOUND.txt")) {
        MessageBox(NULL, L"未找到匹配的I2C总线触控板设备，无需卸载驱动，可直接卸载程序。", L"MltpDrvMgr", MB_OK);
        ExitCode = EXIT_OK;//这里不用安装，直接返回成功，但是跳到末尾清理程序
        goto UninstDrvSuccess;
    }



    retry = 0;
UninstDrv:
    nRet = WinExec("MltpDrvMgr.exe UninstallDriver", SW_HIDE);
    if (nRet < 31) {
        MessageBox(NULL, L"WinExec调用UninstallDriver子程序失败！请稍后再试。", L"MltpDrvMgr", MB_OK);
    }
    else {
        Sleep(2000);
        retry++;
        if (retry > 5) {//批处理模式卸载驱动
            nTry = 0;
            //char cmdLine[MAX_PATH];
            //memset(cmdLine, 0, MAX_PATH);
            //strcat_s(cmdLine, cmdFilePath);
            //strcpy_s(cmdLine, "\\cmd.exe /c UninstDrv.bat");
UninstBAT:     
            retry = 0;
            nTry++;

            //nRet = WinExec(cmdLine, SW_HIDE);
            nRet = WinExec("cmd.exe /c UninstDrv.bat", SW_HIDE);
waitUninstBAT:
                Sleep(500);
                retry++;

                if (retry > 10) {
                    goto UninstBAT;//超时再次批处理卸载方式
                }

                if (nTry > 3) {
                    goto NextUninstStep;//跳过批处理卸载方式
                }

                if (!LogFileExist(L"Return_UninstDrv.txt")) {//UninstDrv.bat未执行结束
                    goto waitUninstBAT;
                }

                if (LogFileExist(L"UninstDrvSucceeded.txt")) {//UninstDrv.bat卸载驱动成功
                    DelLogFile(L"Return_UninstDrv.txt");
                    DelLogFile(L"UninstDrvSucceeded.txt");
                    goto UninstDrvSuccess;//直接跳到结尾
                }

                DelLogFile(L"Return_UninstDrv.txt");
                DelLogFile(L"UninstDrvSucceeded.txt");


NextUninstStep:
            MessageBox(NULL, L"UninstallDriver卸载驱动失败！请稍后再试。", L"MltpDrvMgr", MB_OK);
            ExitCode = EXIT_FAIL;
            return;
        }

        if (!LogFileExist(L"Return_UninstallDriver.txt")) {//InstallDriver未执行结束
            goto UninstDrv;
        }        
    }

    if (!LogFileExist(L"Return_UninstallDriver.txt")) {//InstallDriver未执行结束
        MessageBox(NULL, L"找到匹配的I2C总线触控板设备，卸载驱动失败，请稍后再试。", L"MltpDrvMgr", MB_OK);
        ExitCode = EXIT_FAIL;
        return;
    }
    DelLogFile(L"Return_UninstallDriver.txt");


    while (Rescan())break;//重新扫描设备



    //再次验证
CheckDev:
    nRet = WinExec("MltpDrvMgr.exe FindDevice", SW_HIDE);
    if (nRet < 31) {
        MessageBox(NULL, L"二次验证，WinExec调用FindDevice子程序失败！取消安装，请稍后再试。", L"MltpDrvMgr", MB_OK);
        ExitCode = EXIT_FAIL;
        return;
    }

    retry++;
    Sleep(1500);
    if (retry > 3) {
        MessageBox(NULL, L"二次验证，查找设备失败！请稍后再试。", L"MltpDrvMgr", MB_OK);
        ExitCode = EXIT_FAIL;
        return;
    }

    if (!LogFileExist(L"Return_FindDevice.txt")) {//FindDevice未执行结束
        goto FindDev;
    }

    //FindDevice执行完毕
    DelLogFile(L"Return_FindDevice.txt");

    if (LogFileExist(L"OEMDriver.txt")) {
        MessageBox(NULL, L"二次验证，卸载驱动失败，请稍后再试。", L"MltpDrvMgr", MB_OK);
        ExitCode = EXIT_FAIL;
        return;
    }


UninstDrvSuccess:
    //清理历史记录文件
    while (LogFileExist(L"Return_FindDevice.txt"))DelLogFile(L"Return_FindDevice.txt");
    while (LogFileExist(L"Return_UninstallDriver.txt"))DelLogFile(L"Return_UninstallDriver.txt");
    while (LogFileExist(L"TouchPad_FOUND.txt"))DelLogFile(L"TouchPad_FOUND.txt");

    //
    //删除开机启动项
    DelRegkey(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", L"MltpSvc");

    //删除快捷方式和注册表卸载项
    UninstallShortcut();

    //不存在驱动
    ExitCode = EXIT_OK;
    MessageBox(NULL, L"卸载驱动成功！", L"MltpDrvMgr", MB_OK | MB_DEFBUTTON1);

    //删除程序目录文件
    DelProgramFilesDir(exeFilePath);//注意成功提示对话框在删除之前运行避免冲突
    //DelDir(exeFilePath);//程序目录中还在运行删除不了需要特殊处理

    //TCHAR szProgramFilePath[MAX_PATH];
    //if (GetProgramFilePath(szProgramFilePath)) {
    //    TCHAR szPath[MAX_PATH];
    //    wcscpy_s(szPath, szProgramFilePath);
    //    wcscat_s(szPath, L"\\MouseLikeTouchPad");
    //    DelDir(szPath);
    //}

}

BOOL InstallDriver() {
    //
    INT ret = 0;
    
    if (!GetTouchPad_I2C_hwID()) {
        goto END;
    }

    ret = UpdateDriver();
    if (!ret) {
        return FALSE;
    }


END:
    NewLogFile(L"Return_InstallDriver.txt");
    return ret;
}


BOOL UninstallDriver() {
    //
    INT ret = 0;

    if (GetOEMDriverName()) {
        bOEMDriverExist = TRUE;
    }

    ret = RemoveDriver();
    if (!ret) {
        return FALSE;
    }

    NewLogFile(L"Return_UninstallDriver.txt");
    return ret;

}


BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    if (CTRL_CLOSE_EVENT == dwCtrlType) {
        //printf("exit %d\n", dwCtrlType);
    }

    ::MessageBox(NULL, L"退出！", L"提示", MB_OKCANCEL);

    return TRUE;
}


BOOL FuzzyCompareHwIds(PZPWSTR Array, const wchar_t* MatchHwId)
{
    if (Array) {
        while (Array[0]) {
            if (wcscmp(Array[0], MatchHwId) == 0) {
                return TRUE;
            }
            Array++;
        }
    }
    return FALSE;
}

void DelMultiSz(_In_opt_ __drv_freesMem(object) PZPWSTR Array)
{
    if (Array) {
        Array--;
        if (Array[0]) {
            delete[] Array[0];
        }
        delete[] Array;
    }
}


__drv_allocatesMem(object)
LPTSTR* GetDevMultiSz(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo, _In_ DWORD Prop)
{
    LPTSTR buffer;
    DWORD size;
    DWORD reqSize;
    DWORD dataType;
    LPTSTR* array;
    DWORD szChars;

    size = 8192; // initial guess, nothing magic about this
    buffer = new TCHAR[(size / sizeof(TCHAR)) + 2];
    if (!buffer) {
        return NULL;
    }
    while (!SetupDiGetDeviceRegistryProperty(Devs, DevInfo, Prop, &dataType, (LPBYTE)buffer, size, &reqSize)) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            goto failed;
        }
        if (dataType != REG_MULTI_SZ) {
            goto failed;
        }
        size = reqSize;
        delete[] buffer;
        buffer = new TCHAR[(size / sizeof(TCHAR)) + 2];
        if (!buffer) {
            goto failed;
        }

        //wprintf(TEXT("Device ID is: [%s]\n"), buffer);
    }
    szChars = reqSize / sizeof(TCHAR);
    buffer[szChars] = TEXT('\0');
    buffer[szChars + 1] = TEXT('\0');
    array = GetMultiSzIndexArray(buffer);
    if (array) {
        return array;
    }

failed:
    if (buffer) {
        delete[] buffer;
    }
    return NULL;
}


__drv_allocatesMem(object)
LPTSTR* GetMultiSzIndexArray(_In_ __drv_aliasesMem LPTSTR MultiSz)
{
    LPTSTR scan;
    LPTSTR* array;
    int elements;

    for (scan = MultiSz, elements = 0; scan[0]; elements++) {
        scan += _tcslen(scan) + 1;
    }
    array = new LPTSTR[elements + 2];
    if (!array) {
        return NULL;
    }
    array[0] = MultiSz;
    array++;
    if (elements) {
        for (scan = MultiSz, elements = 0; scan[0]; elements++) {
            array[elements] = scan;
            scan += _tcslen(scan) + 1;
        }
    }
    array[elements] = NULL;
    return array;
}

wchar_t* mystrcat(const wchar_t* str1, const wchar_t* str2, const wchar_t* str3)//str1是原来的字符串，str2是str1中的某段需要替换字符串，str3是替换str2的字符串
{
    int strlen1 = 0, strlen2 = 0, strlen3 = 0;
    strlen1 = wcslen(str1);
    strlen2 = wcslen(str2);
    strlen3 = wcslen(str3);
    int num = 0, num1 = 0, num2 = 0, num3 = 0;//num1从str1起。。。
    int m = 0;//替代num1的值
    int count = 0;//记录有几个像str2的字符串
    int len = 2 * (count * (strlen3 - strlen2) + strlen1 + 1);
    len = (strlen1 * strlen3 / strlen2 + 1) * 2;
    wchar_t* p_str = (wchar_t*)malloc(len);
    wchar_t* str = p_str;
    for (num1 = 0; num1 < strlen1; num1++)
    {
        m = num1;
        num2 = 0;
        while (num2 < strlen2)
        {
            if (str1[m] != str2[num2])
                break;
            m++;
            num2++;
        }
        if (num2 >= strlen2)
        {
            for (num3 = 0; num3 < strlen3; num3++)
                str[num++] = str3[num3];
            num1 += (strlen2 - 1);
            count++;
        }
        else
            str[num++] = str1[num1];
    }
    len = 2 * (count * (strlen3 - strlen2) + strlen1 + 1);
    str[len / 2 - 1] = L'\0';
    return p_str;
}


BOOL RemoveDevice()
{
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD devIndex;
    BOOLEAN bMATCH = FALSE;
    BOOLEAN bFOUND = FALSE;
    BOOLEAN bSuccess = FALSE;


    // Create a Device Information Set with all present devices.

    DeviceInfoSet = SetupDiGetClassDevs(NULL, // All Classes
        0,
        0,
        DIGCF_ALLCLASSES | DIGCF_PRESENT); // All devices present on system
    if (DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        //printf("RemoveDevice SetupDiGetClassDevs err!\n");
        return FALSE;
    }

    //wprintf(TEXT("RemoveDevice Search for TouchPad_hwID Device ID: [%s]\n"), TouchPad_COMPATIBLE_hwID);

    // Enumerate through all Devices.
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (devIndex = 0; SetupDiEnumDeviceInfo(DeviceInfoSet, devIndex, &DeviceInfoData); devIndex++)
    {
        TCHAR devInstance_ID[MAX_DEVICE_ID_LEN];//instance ID
        LPTSTR* hwIds = NULL;
        LPTSTR* compatIds = NULL;
        //
        // determine instance ID
        //
        if (CM_Get_Device_ID(DeviceInfoData.DevInst, devInstance_ID, MAX_DEVICE_ID_LEN, 0) != CR_SUCCESS) {
            devInstance_ID[0] = TEXT('\0');
        }
        ////wprintf(TEXT("devInstance_ID: [%s]\n"), devInstance_ID); 

        hwIds = GetDevMultiSz(DeviceInfoSet, &DeviceInfoData, SPDRP_HARDWAREID);
        compatIds = GetDevMultiSz(DeviceInfoSet, &DeviceInfoData, SPDRP_COMPATIBLEIDS);

        if (FuzzyCompareHwIds(hwIds, TouchPad_COMPATIBLE_hwID) || FuzzyCompareHwIds(compatIds, TouchPad_COMPATIBLE_hwID))
        {
            //printf("RemoveDevice 找到TouchPad触控板设备！\n");
            //wprintf(TEXT("RemoveDevice TouchPad Device devInstance_ID= [%s]\n"), devInstance_ID);
            //wprintf(TEXT("RemoveDevice TouchPad Device hwIds= [%s]\n"), hwIds[0]);
            wcscpy_s(TouchPad_hwID, hwIds[0]);
            //wprintf(TEXT("RemoveDevice TouchPad_hwID= [%s]\n"), TouchPad_hwID);

            bMATCH = TRUE;
            DelMultiSz(hwIds);
            DelMultiSz(compatIds);
            break;
        }

        DelMultiSz(hwIds);
        DelMultiSz(compatIds);
    }


    if (bMATCH)
    {
        //生成I2C_hwID
        wchar_t* pstr;
        pstr = mystrcat(TouchPad_hwID, L"HID\\", L"ACPI\\");
        pstr = mystrcat(pstr, L"&Col02", L"\0");
        wcscpy_s(TouchPad_I2C_hwID, pstr);
        //wprintf(TEXT("RemoveDevice TouchPad I2C_hwID= [%s]\n"), I2C_hwID);

        //验证I2C设备
        //printf("RemoveDevice 开始查找TouchPad触控板的I2C设备！\n");
        //wprintf(TEXT("RemoveDevice Search for I2C_hwID Device ID: [%s]\n"), I2C_COMPATIBLE_hwID);

        for (devIndex = 0; SetupDiEnumDeviceInfo(DeviceInfoSet, devIndex, &DeviceInfoData); devIndex++)
        {
            TCHAR devInstID[MAX_DEVICE_ID_LEN];//instance ID
            LPTSTR* devHwIds = NULL;
            LPTSTR* devCompatIds = NULL;
            //
            // determine instance ID
            //
            if (CM_Get_Device_ID(DeviceInfoData.DevInst, devInstID, MAX_DEVICE_ID_LEN, 0) != CR_SUCCESS) {
                devInstID[0] = TEXT('\0');
            }
            ////wprintf(TEXT("devInstID: [%s]\n"), devInstID);

            devHwIds = GetDevMultiSz(DeviceInfoSet, &DeviceInfoData, SPDRP_HARDWAREID);
            devCompatIds = GetDevMultiSz(DeviceInfoSet, &DeviceInfoData, SPDRP_COMPATIBLEIDS);

            if (FuzzyCompareHwIds(devHwIds, TouchPad_I2C_hwID) && FuzzyCompareHwIds(devCompatIds, I2C_COMPATIBLE_hwID))
            {
                //printf("RemoveDevice 找到TouchPad触控板的I2C设备！\n");
                //wprintf(TEXT("RemoveDevice TouchPad I2C Device devInstID= [%s]\n"), devInstID);
                //wprintf(TEXT("RemoveDevice TouchPad I2C Device devHwIds= [%s]\n"), devHwIds[0]);

                BOOLEAN ret = GetDeviceOEMdriverInfo(DeviceInfoSet, &DeviceInfoData);

                bFOUND = TRUE;
                DelMultiSz(devHwIds);
                DelMultiSz(devCompatIds);
                break;
            }

            DelMultiSz(devHwIds);
            DelMultiSz(devCompatIds);
        }


        if (!bFOUND) {
            //printf("RemoveDevice 未找到TouchPad触控板的I2C设备！\n");
        }
        else {

            SP_REMOVEDEVICE_PARAMS rmdParams;
            SP_DEVINSTALL_PARAMS devParams;

            // need hardware ID before trying to remove, as we wont have it after
            SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;

            devInfoListDetail.cbSize = sizeof(devInfoListDetail);
            if (!SetupDiGetDeviceInfoListDetail(DeviceInfoSet, &devInfoListDetail)) {
                // skip this
                //printf("RemoveDevice SetupDiGetDeviceInfoListDetail not exist！\n");
                bSuccess = TRUE;//
                goto END;
            }

            rmdParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            rmdParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
            rmdParams.Scope = DI_REMOVEDEVICE_GLOBAL;
            rmdParams.HwProfile = 0;
            if (!SetupDiSetClassInstallParams(DeviceInfoSet, &DeviceInfoData, &rmdParams.ClassInstallHeader, sizeof(rmdParams)) ||
                !SetupDiCallClassInstaller(DIF_REMOVE, DeviceInfoSet, &DeviceInfoData)) {
                // failed to invoke DIF_REMOVE
                //printf("RemoveDevice failed to invoke DIF_REMOVE！\n");
            }
            else {
                // see if device needs reboot
                devParams.cbSize = sizeof(devParams);
                if (SetupDiGetDeviceInstallParams(DeviceInfoSet, &DeviceInfoData, &devParams) && (devParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT))) {
                    // reboot required
                    //printf("RemoveDevice reboot required！\n");
                }
                else {
                    // appears to have succeeded
                    //printf("RemoveDevice succeeded！\n");
                }

            }


            bSuccess = TRUE;//
        }
    }
    else {
        //printf("RemoveDevice 未找到TouchPad触控板设备！\n");
    }


END:
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    return bSuccess;
}

BOOL RemoveDriver()
{
    BOOLEAN bSuccess = FALSE;

    if (bOEMDriverExist) {//存在驱动
        printf("start RemoveDriver\n");

        //删除驱动 
        if (!DiUninstallDriver(NULL, OEMinf_FullName, DIURFLAG_NO_REMOVE_INF, FALSE)) {
            printf("DiUninstallDriver Fail %d!\n", GetLastError());
            return FALSE;
        }
        else {
            printf("DiUninstallDriver ok！\n");
        }

        //DPDelete
        if (!SetupUninstallOEMInf(OEMinf_name, MltpDrvMgr_FLAG_FORCE, NULL)) {
            if (GetLastError() == ERROR_INF_IN_USE_BY_DEVICES) {
                printf("ERROR_INF_IN_USE_BY_DEVICES！\n");
            }
            else if (GetLastError() == ERROR_NOT_AN_INSTALLED_OEM_INF) {
                printf("ERROR_NOT_AN_INSTALLED_OEM_INF！\n");
            }
            else {
                printf("SetupUninstallOEMInf FAILED！\n");
            }
        }
        else {
            printf("SetupUninstallOEMInf ok！\n");
            bSuccess = TRUE;
        }
    }

    return bSuccess;
}


BOOL UpdateDriver()
{

    BOOL reboot = FALSE;
    LPCTSTR hwid = NULL;
    LPCTSTR inf = NULL;
    DWORD flags = INSTALLFLAG_FORCE;

    BOOLEAN bSuccess = FALSE;

    //printf("start UpdateDriver\n");

    inf = inf_FullPathName;
    if (!inf[0]) {
        printf("inf err！\n");
        return FALSE;
    }
    //wprintf(TEXT("update inf= [%s]\n"), inf);

    hwid = TouchPad_I2C_hwID;
    if (!hwid[0]) {
        printf("hwid err！\n");
        return FALSE;
    }
    //wprintf(TEXT("update hwid= [%s]\n"), hwid);

    printf("start UpdateDriver\n");
    if (!UpdateDriverForPlugAndPlayDevices(NULL, hwid, inf, flags, &reboot)) {
        printf("UpdateDriverForPlugAndPlayDevices failed！\n");
        return FALSE;
    }

    if (reboot) {
        printf("A reboot is needed to complete driver install！\n");
    }

    bSuccess = TRUE;
    printf("UpdateDriverForPlugAndPlayDevices ok！\n");

    return bSuccess;
}


void Set_InstallationSources_Directory(wchar_t* szPath)
{
    wprintf(TEXT("Set_InstallationSources_Directory= [%s]\n"), szPath);
    HKEY hKey;

    int len = wcslen(szPath) + 1;

    // 使用RegCreateKey能保证如果Softwaredaheng_directx不存在的话，创建一个。
    if (RegCreateKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup", &hKey) == ERROR_SUCCESS) {
        if (RegSetValueEx(hKey, L"Installation Sources", 0, REG_MULTI_SZ, (CONST BYTE*)szPath, 2 * len) == ERROR_SUCCESS) {
            printf("RegSetValue: Installation Sources = %s ", szPath);
        }
    }
    RegCloseKey(hKey);
}



void InstallShortcut()
{
    //得到公共的桌面路径 
    //TCHAR szDesktopPath[MAX_PATH];
    //if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, 0, szDesktopPath))) {
    //    printf("SHGetFolderPath err");
    //    return;
    //}

    TCHAR szSourcePath[MAX_PATH];
    wcscpy_s(szSourcePath, exeFilePath);//不含/
    //wcscat_s(szSourcePath, L"\\MltpSvc.exe");

    //创建桌面快捷方式
    TCHAR szDesktopPath[MAX_PATH];
    if (GetDesktopPath(szDesktopPath)) {
        //MessageBox(NULL, szDesktopPath, L"GetDesktopPath", MB_OK);
        if (!CreateShotCut(szSourcePath, L"MltpSvc.exe", L"ShowDialog", L"仿鼠标触摸板服务指南", szDesktopPath)) {
            //MessageBox(NULL, szSourcePath, L"CreateShotCut szDesktopPath err", MB_OK);
        }
    }

    //创建程序组快捷方式
    TCHAR szProgramsPath[MAX_PATH];
    if (GetProgramsPath(szProgramsPath)) {
        //MessageBox(NULL, szProgramsPath, L"GetProgramsPath", MB_OK);
        TCHAR strDestDir[MAX_PATH];
        wcscpy_s(strDestDir, szProgramsPath);//不含/
        wcscat_s(strDestDir, L"\\MouseLikeTouchPad");//不含/

        //先删除程序组
        DelDir(strDestDir);

         //创建文件夹（子目录）
        if (CreateDirectory(strDestDir, NULL)) {//程序组文件夹必须存在才能建立快捷方式
            CreateShotCut(szSourcePath, L"MltpSvc.exe", L"ShowDialog", L"仿鼠标触摸板服务指南", strDestDir);//仿鼠标触摸板服务指南//MouseLikeTouchPad Service Information
            //MessageBox(NULL, szSourcePath, L"CreateShotCut szProgramsPath err", MB_OK);

            CreateShotCut(szSourcePath, L"MltpDrvMgr.exe", L"Uninstall", L"仿鼠标触摸板卸载程序", strDestDir);//仿鼠标触摸板卸载程序//MouseLikeTouchPad Uninstaller

        }   
    }

    //
    //创建开机启动项
    wcscpy_s(szSourcePath, exeFilePath);//不含/
    wcscat_s(szSourcePath, L"\\MltpSvc.exe");
    WriteSzReg(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", L"仿鼠标触摸板服务指南", szSourcePath);

    //创建注册表程序卸载项
    TCHAR strUninstDir[MAX_PATH];
    wcscpy_s(strUninstDir, exeFilePath);
    wcscat_s(strUninstDir, L"\\MltpDrvMgr.exe Uninstall");
    WriteSzReg(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MouseLikeTouchPad", L"DisplayName", L"仿鼠标触摸板卸载程序");//MouseLikeTouchPad Uninstaller
    WriteSzReg(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MouseLikeTouchPad", L"UninstallString", strUninstDir);

}

void UninstallShortcut()
{
    //删除桌面快捷方式
    TCHAR szDesktopPath[MAX_PATH];
    if (GetDesktopPath(szDesktopPath)) {
        wcscat_s(szDesktopPath, L"\\MltpSvc.lnk");
        DeleteFile(szDesktopPath);
    }

    //删除程序组快捷方式
    TCHAR szProgramsPath[MAX_PATH];
    if (GetProgramsPath(szProgramsPath)) {
        TCHAR szPath[MAX_PATH];
        wcscpy_s(szPath, szProgramsPath);
        wcscat_s(szPath, L"\\MouseLikeTouchPad");

        if (!DelDir(szPath)) {
            //MessageBox(NULL, szPath, L"DelTree err", MB_OK);
        }
    }

    //删除开机启动项
    DelRegkey(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", L"MltpSvc");

    //删除注册表程序卸载项
    SHDeleteKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MouseLikeTouchPad"); //删除项(包含子项)

    //DelRegkey(L"Software\\MouseLikeTouchPad", L"RegKey"); //删除键值
}

BOOL GetDesktopPath(wchar_t* szPath) {//得到公共的桌面路径 
    BOOL bRet = FALSE;

    TCHAR Path[MAX_PATH + 1];
    LPITEMIDLIST pidl;
    LPMALLOC pShell;
    if (SUCCEEDED(SHGetMalloc(&pShell)))
    {
        if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, &pidl)))
        {
            bRet = SHGetPathFromIDList(pidl, Path);
            if (!bRet)
            {
                pShell->Free(pidl);
            }
            else {
                wcscpy(szPath, Path);
            }
            pShell->Release();
        }
    }

    return bRet;
}

BOOL GetProgramsPath(wchar_t* szPath) {//得到公共的程序组路径 
    BOOL bRet = FALSE;

    TCHAR Path[MAX_PATH + 1];
    LPITEMIDLIST pidl;
    LPMALLOC pShell;
    if (SUCCEEDED(SHGetMalloc(&pShell)))
    {
        if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &pidl)))
        {
            bRet = SHGetPathFromIDList(pidl, Path);
            if (!bRet)
            {
                pShell->Free(pidl);
            }
            else {
                wcscpy(szPath, Path);
            }
            pShell->Release();
        }
    }

    return bRet;
}


BOOL CreateShotCut(LPCWSTR strSourcePath, LPCWSTR strSourceFileName, LPCWSTR strArg, LPCWSTR strShortcutName, LPCWSTR strDestDir)
{
    if (FAILED(CoInitialize(NULL)))
        return FALSE;
    BOOL bRet = FALSE;

    TCHAR strDestPathName[MAX_PATH];
    wcscpy_s(strDestPathName, strDestDir);
    wcscat_s(strDestPathName, L"\\");
    wcscat_s(strDestPathName, strShortcutName);////设置桌面快捷方式的名字
    wcscat_s(strDestPathName, L".lnk");

    TCHAR strSourcePathName[MAX_PATH];
    wcscpy_s(strSourcePathName, strSourcePath);
    wcscat_s(strSourcePathName, L"\\");
    wcscat_s(strSourcePathName, strSourceFileName);

    IShellLink* psl;

    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl)))
    {
        psl->SetPath(strSourcePathName);//设置快捷方式的目标位置 
        //比如目标位置为C:\windows\a.txt 起始位置就应该设置为C:\windows否则会导致不可预料的错误

        //设置启动参数
        psl->SetArguments(strArg);
    
        //如果是文件夹的快捷方式起始位置和目标位置可以设置为一样
        psl->SetWorkingDirectory(strSourcePath); //设置快捷方式的起始位置 
        IPersistFile* ppf;
        if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf)))
        {
           // MessageBox(NULL, strDestPathName, L"QueryInterface ok", MB_OK);
            LPCWSTR dest = strDestPathName;
            if (SUCCEEDED(ppf->Save(dest, TRUE)))//保存快捷方式到桌面 
            {
                //MessageBox(NULL, strDestPathName, L"ppf->Save ok", MB_OK);

                ppf->Release();
                psl->Release();
                bRet = TRUE;
            }
            else {
                ppf->Release();
                psl->Release();
            }
        }
        else {
            ppf->Release();
            psl->Release();
        }
    }

    CoUninitialize();

    if (!bRet)
    {
        LPVOID   lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language 
            (LPTSTR)&lpMsgBuf,
            0,
            NULL
        );

        LocalFree(lpMsgBuf);
    }
    return bRet;

}

void UnStartup()
{
    //删除开机启动项
    if (DelRegkey(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", L"MltpSvc")) {
        MessageBox(NULL, L"软件开机启动项删除成功!", L"MltpDrvMgr", MB_OK);
    }
    else {
        MessageBox(NULL, L"软件开机启动项删除失败!", L"MltpDrvMgr", MB_ICONSTOP);
    }
}

void Register()
{
    BOOL isRegistered = FALSE;
    TCHAR data[] = L"JXLEYO-HRP-MLTP-DRVR";//SN注册序列号字符串,实际软件注册验证不检测内容，只有注册文件存在即表示成功
    if (LogFileExist(L"RegKey.dat")) {
        isRegistered = TRUE;
    }
    else {
        if (!NewLogFile(L"RegKey.dat")) {
            MessageBox(NULL, L"软件注册失败，请重新尝试!", L"MltpDrvMgr", MB_ICONSTOP);
        }
        else {
            isRegistered = TRUE;
        }
    }
    
    if (isRegistered) {
        //终止和重启服务程序运行
        WinExec("taskkill /f /im MltpSvc.exe", SW_HIDE);
        MessageBox(NULL, L"软件注册成功!", L"MltpDrvMgr", MB_OK);

        //重启服务程序运行
        WinExec("MltpSvc.exe SHowDialog", SW_SHOW);

        int result = MessageBox(NULL,L"熟练使用本触摸板驱动后可以不需要开机启动帮助服务程序，确定删除开机启动项吗？", L"MltpSvc", MB_YESNO);
        switch (result)
        {
            case IDYES:
            {
                UnStartup();
                break;
            }
            case IDNO:
                break;
        }
    }

    //if (WriteBinReg(L"Software\\MouseLikeTouchPad", L"RegKey", (LPBYTE)data, sizeof(data))){
    //    MessageBox(NULL, L"软件注册成功!", L"MltpDrvMgr", MB_OK);
    //}
    //else {
    //    MessageBox(NULL, L"软件注册失败，请重新尝试!", L"MltpDrvMgr", MB_ICONSTOP);
    //}
}


BOOL WriteSzReg(LPCWSTR szPath, LPCWSTR szKey, LPCWSTR dwValue)
{
    BOOL bSuccess = FALSE;
    HKEY hKEY;
    DWORD dwDisp;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_SZ;
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, szPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKEY, &dwDisp))
    {
        if (RegSetValueEx(hKEY, szKey, 0, dwType, (LPBYTE)dwValue, 2*wcslen(dwValue)) != ERROR_SUCCESS)//MAX_PATH
        {
            printf("RegSetValueEx err！\n");

        }

        RegCloseKey(hKEY);
        bSuccess = TRUE;
    }

    return bSuccess;
}


BOOL ReadDWordReg(LPCWSTR szPath, LPCWSTR szKey, DWORD* dwValue)
{
    BOOL bSuccess = FALSE;
    HKEY hKEY;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPath, 0, KEY_READ, &hKEY))
    {
        DWORD dwSize = sizeof(DWORD);
        DWORD dwType = REG_DWORD;

        if (RegQueryValueEx(hKEY, szKey, 0, &dwType, (LPBYTE)dwValue, &dwSize) != ERROR_SUCCESS)
        {
            printf("RegQueryValueEx err！\n");
        }

        RegCloseKey(hKEY);
        bSuccess = TRUE;
    }

    return bSuccess;
}


BOOL WriteDWordReg(LPCWSTR szPath, LPCWSTR szKey, DWORD* dwValue)
{
    BOOL bSuccess = FALSE;
    HKEY hKEY;
    DWORD dwDisp;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, szPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKEY, &dwDisp))
    {
        if (RegSetValueEx(hKEY, szKey, 0, dwType, (LPBYTE)dwValue, dwSize) != ERROR_SUCCESS)
        {
            printf("RegSetValueEx err！\n");

        }

        RegCloseKey(hKEY);
        bSuccess = TRUE;
    }

    return bSuccess;
}

BOOL WriteBinReg(LPCWSTR szPath, LPCWSTR szKey, LPBYTE dwValue, DWORD dwSize)
{
    BOOL bSuccess = FALSE;
    HKEY hKEY;
    DWORD dwDisp;
    DWORD dwType = REG_BINARY;
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, szPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKEY, &dwDisp))
    {
        if (RegSetValueEx(hKEY, szKey, 0, dwType, (LPBYTE)dwValue, dwSize) != ERROR_SUCCESS)
        {
            printf("RegSetValueEx err！\n");

        }

        RegCloseKey(hKEY);
        bSuccess = TRUE;
    }

    return bSuccess;
}


BOOL ReadBinReg(LPCWSTR szPath, LPCWSTR szKey, LPBYTE dwValue, DWORD* dwSize)
{
    BOOL bSuccess = FALSE;
    HKEY hKEY;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPath, 0, KEY_READ, &hKEY))
    {
        *dwSize = 0;
        DWORD dwType = REG_BINARY;

        if (RegQueryValueEx(hKEY, szKey, 0, &dwType, dwValue, dwSize) != ERROR_SUCCESS)
        {
            printf("RegQueryValueEx err！\n");
        }

        RegCloseKey(hKEY);
        bSuccess = TRUE;
    }

    return bSuccess;
}


BOOL DelRegkey(LPCWSTR szPath, LPCWSTR szKey)
{
    BOOL bSuccess = FALSE;
    HKEY hKEY;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPath, 0, KEY_ALL_ACCESS, &hKEY))
    {
        DWORD dwSize = sizeof(DWORD);
        DWORD dwType = REG_DWORD;

        if (RegDeleteValue(hKEY, szKey) != ERROR_SUCCESS)
        {
            printf("RegDeleteValue err！\n");
        }

        RegCloseKey(hKEY);
        bSuccess = TRUE;
    }

    return bSuccess;
}


BOOL DelDir(LPCWSTR lpszPath)
{

    TCHAR szFromPath[_MAX_PATH];//源文件路径  
    wcscpy_s(szFromPath, lpszPath);
    wcscat_s(szFromPath, L"\0");//必须要以“\0\0”结尾，不然删除不了

    SHFILEOPSTRUCT FileOp;
    SecureZeroMemory((void*)&FileOp, sizeof(SHFILEOPSTRUCT));
    //secureZeroMemory和ZeroMerory的区别
    //根据MSDN上，ZeryMerory在当缓冲区的字符串超出生命周期的时候，
    //会被编译器优化，从而缓冲区的内容会被恶意软件捕捉到。
    //引起软件安全问题，特别是对于密码这些比较敏感的信息而说。
    //而SecureZeroMemory则不会引发此问题，保证缓冲区的内容会被正确的清零。
    //如果涉及到比较敏感的内容，尽量使用SecureZeroMemory函数。

    
    FileOp.fFlags = FOF_NOCONFIRMATION;//操作与确认标志 

    FileOp.hNameMappings = NULL; // 文件映射

    FileOp.hwnd = NULL;//消息发送的窗口句柄；

    FileOp.lpszProgressTitle = NULL; //文件操作进度窗口标题 

    FileOp.pFrom = szFromPath; //源文件及路径 

    FileOp.pTo = NULL;//目标文件及路径 

    FileOp.wFunc = FO_DELETE; //操作类型 

    return SHFileOperation(&FileOp) == 0;


    //int iSize;
    //char* pszMultiByte;

    ////返回接受字符串所需缓冲区的大小，已经包含字符结尾符'\0'
    //iSize = WideCharToMultiByte(CP_ACP, 0, lpszPath, -1, NULL, 0, NULL, NULL);
    //pszMultiByte = (char*)malloc(iSize * sizeof(char)); //不需要 pszMultiByte = (char*)malloc(iSize*sizeof(char)+1);
    //WideCharToMultiByte(CP_ACP, 0, lpszPath, -1, pszMultiByte, iSize, NULL, NULL);

    //char cmdline[_MAX_PATH];//源文件路径  
    //strcpy(cmdline, "cmd.exe /c rd /S /Q");
    //strcat(cmdline, " \"");
    //strcat(cmdline, pszMultiByte);
    //strcat(cmdline, "\"");

    ////wchar_t* pwszUnicode;
    //////返回接受字符串所需缓冲区的大小，已经包含字符结尾符'\0'
    ////iSize = MultiByteToWideChar(CP_ACP, 0, cmdline, -1, NULL, 0);
    ////pwszUnicode = (wchar_t*)malloc(iSize * sizeof(wchar_t)); //不需要 pwszUnicode = (wchar_t *)malloc((iSize+1)*sizeof(wchar_t))
    ////MultiByteToWideChar(CP_ACP, 0, cmdline, -1, pwszUnicode, iSize);
    ////MessageBox(NULL, pwszUnicode, L"cmdline", MB_OK);

    //INT nRet = 0;////-0 系统内存或资源不足//--ERROR_BAD_FORMAT.EXE文件格式无效（比如不是32位应用程序）//--ERROR_FILE_NOT_FOUND 指定的文件设有找到//--ERROR_PATH_NOT_FOUND
    //nRet = WinExec(cmdline, SW_SHOWNORMAL);
    //if (nRet < 31) {
    //    MessageBox(NULL, L"WinExec调用rd /S /Q子程序失败！", L"MltpDrvMgr", MB_OK);
    //    return FALSE;
    //}

    //printf(cmdline);

    //return TRUE;
}


BOOL GetProgramFilePath(wchar_t* szPath) {//得到程序安装路径 
    BOOL bRet = FALSE;

    TCHAR Path[MAX_PATH + 1];
    LPITEMIDLIST pidl;
    LPMALLOC pShell;
    if (SUCCEEDED(SHGetMalloc(&pShell)))
    {
        if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAM_FILES, &pidl)))
        {
            bRet = SHGetPathFromIDList(pidl, Path);
            if (!bRet)
            {
                pShell->Free(pidl);
            }
            else {
                wcscpy(szPath, Path);
            }
            pShell->Release();
        }
    }

    return bRet;
}

void DeleteApplicationSelf()
{
    TCHAR szCommandLine[MAX_PATH + 10];

    //设置本程序进程基本为实时执行，快速退出。
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    //通知资源管理器不显示本程序，当然如果程序没有真正的删除，刷新资源管理器后仍会显示出来的。
    SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, _pgmptr, NULL);

    //调用cmd传入参数以删除自己
    TCHAR szFilePath[MAX_PATH];
    wsprintf(szFilePath, L"\"%s\"", _pgmptr);
    wsprintf(szCommandLine, L"/c rd /S /Q ", szFilePath);
    ShellExecute(NULL, L"open", L"cmd.exe", szCommandLine, NULL, SW_HIDE);

    ExitProcess(0);
}

void DelProgramFilesDir(LPCWSTR lpszPath)
{
    TCHAR szArg[MAX_PATH + 10];

    //设置本程序进程基本为实时执行，快速退出。
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    //通知资源管理器不显示本程序，当然如果程序没有真正的删除，刷新资源管理器后仍会显示出来的。
    SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, exeFilePath, NULL);//设置为目录删除SHCNE_RMDIR

    //调用cmd传入参数以删除自己
    wcscpy_s(szArg, L"/c rd /S /Q");
    wcscat_s(szArg, L" \"");
    wcscat_s(szArg, exeFilePath);
    wcscat_s(szArg, L"\"");

    ShellExecute(NULL, L"open", L"cmd.exe", szArg, NULL, SW_HIDE);

    ExitProcess(0);
}
