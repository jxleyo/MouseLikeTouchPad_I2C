// PtpDrvMgr.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//


#include "PtpDrvMgr.h"
wchar_t DevDescription[] = L"MouseLikeTouchPad_I2C";
wchar_t ProviderName[] = L"jxleyo.HRP";
wchar_t MfgName[] = L"jxleyo.HRP";
wchar_t I2C_COMPATIBLE_hwID[] = L"ACPI\\PNP0C50";
wchar_t TouchPad_COMPATIBLE_hwID[] = L"HID_DEVICE_UP:000D_U:0005";
wchar_t TouchPad_hwID[MAX_DEVICE_ID_LEN];
wchar_t I2C_hwID[MAX_DEVICE_ID_LEN];
wchar_t inf_name[] = L"MouseLikeTouchPad_I2C.inf";
wchar_t OEMinf_FullName[MAX_PATH];
wchar_t OEMinf_name[MAX_PATH];


BOOL EnbalePrivileges()
{
    HANDLE Token;
    TOKEN_PRIVILEGES NewPrivileges;
    LUID Luid;

    //
    // we need to "turn on" reboot privilege
    // if any of this fails, try reboot anyway
    //
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &Token)) {//TOKEN_ADJUST_PRIVILEGES//TOKEN_ALL_ACCESS 
        printf("OpenProcessToken failed!\n");
        return FALSE;
    }

    if (!LookupPrivilegeValue(NULL, SE_LOAD_DRIVER_NAME, &Luid)) {//SE_LOAD_DRIVER_NAME//SE_SHUTDOWN_NAME 
        CloseHandle(Token);
        printf("LookupPrivilegeValue failed!\n");
        return FALSE;
    }

    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Luid = Luid;
    NewPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    BOOL bRet = AdjustTokenPrivileges(Token, FALSE, &NewPrivileges, 0, NULL, NULL);
    if (bRet == FALSE)
    {
        CloseHandle(Token);
        printf("AdjustTokenPrivileges failed!\n");
        return FALSE;
    }
      
    CloseHandle(Token);
    // 根据错误码判断是否特权都设置成功
    DWORD dwRet = 0;
    dwRet = GetLastError();
    if (ERROR_SUCCESS == dwRet)
    {
        printf("EnbalePrivileges ok!v");
        return TRUE;
    }
    else if (ERROR_NOT_ALL_ASSIGNED == dwRet)
    {
        printf("ERROR_NOT_ALL_ASSIGNED!\n");
        return FALSE;
    }

    
    printf("EnbalePrivileges end!\n");
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
        printf("CM_Locate_DevNode failed!\n");
        return FALSE;
    }

    failcode = CM_Reenumerate_DevNode(devRoot, 0);
    if (failcode != CR_SUCCESS) {
        printf("CM_Reenumerate_DevNode failed!\n");
        return FALSE;
    }

    printf("Rescan ok!\n");
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
        printf("SetupDiGetClassDevs failed!\n");
        return 0;
    }

    SP_DEVICE_INTERFACE_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    int deviceNo = 0;

    SetLastError(NO_ERROR);
    printf("while start\n");

    while (GetLastError() != ERROR_NO_MORE_ITEMS)
    {
        printf("deviceNo=%d\n", deviceNo);
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
                printf("SetupDiGetInterfaceDeviceDetail failed!\n");
                free(devDetail);
                SetupDiDestroyDeviceInfoList(hDevInfo);
                return 0;
            }

            printf("DevicePath=%S\n", devDetail->DevicePath);

            ++deviceNo;

        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return FOUND;
}


BOOL FindDevice()
{
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD devIndex;
    BOOLEAN MATCH = FALSE;
    BOOLEAN FOUND = FALSE;
 

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

    _tprintf(TEXT("Search for TouchPad_hwID Device ID: [%s]\n"), TouchPad_COMPATIBLE_hwID);

    // Enumerate through all Devices.
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    TCHAR buffer[256] = { 0 };
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
        //_tprintf(TEXT("devInstance_ID: [%s]\n"), devInstance_ID); 

        hwIds = GetDevMultiSz(DeviceInfoSet, &DeviceInfoData, SPDRP_HARDWAREID);
        compatIds = GetDevMultiSz(DeviceInfoSet, &DeviceInfoData, SPDRP_COMPATIBLEIDS);

         if (FuzzyCompareHwIds(hwIds, TouchPad_COMPATIBLE_hwID) || FuzzyCompareHwIds(compatIds, TouchPad_COMPATIBLE_hwID))
         {
              printf("找到TouchPad触控板设备！\n");
              _tprintf(TEXT("TouchPad Device devInstance_ID= [%s]\n"), devInstance_ID);
              _tprintf(TEXT("TouchPad Device hwIds= [%s]\n"), hwIds[0]);
              wcscpy_s(TouchPad_hwID, hwIds[0]);
              _tprintf(TEXT("TouchPad_hwID= [%s]\n"), TouchPad_hwID);

              MATCH = TRUE;
              DelMultiSz(hwIds);
              DelMultiSz(compatIds);
              break;
          }

         DelMultiSz(hwIds);
         DelMultiSz(compatIds);
    }


    if (MATCH)
    {
        //生成I2C_hwID
        wchar_t* pstr;
        pstr = mystrcat(TouchPad_hwID, L"HID\\", L"ACPI\\");
        pstr = mystrcat(pstr, L"&Col02", L"\0");
        wcscpy_s(I2C_hwID, pstr);
        _tprintf(TEXT("TouchPad I2C_hwID= [%s]\n"), I2C_hwID);

        //验证I2C设备
        printf("开始查找TouchPad触控板的I2C设备！\n");
        _tprintf(TEXT("Search for I2C_hwID Device ID: [%s]\n"), I2C_COMPATIBLE_hwID);

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
            //_tprintf(TEXT("devInstID: [%s]\n"), devInstID);

            devHwIds = GetDevMultiSz(DeviceInfoSet, &DeviceInfoData, SPDRP_HARDWAREID);
            devCompatIds = GetDevMultiSz(DeviceInfoSet, &DeviceInfoData, SPDRP_COMPATIBLEIDS);

            if (FuzzyCompareHwIds(devHwIds, I2C_hwID) && FuzzyCompareHwIds(devCompatIds, I2C_COMPATIBLE_hwID))
            {
                printf("找到TouchPad触控板的I2C设备！\n");
                _tprintf(TEXT("TouchPad I2C Device devInstID= [%s]\n"), devInstID);
                _tprintf(TEXT("TouchPad I2C Device devHwIds= [%s]\n"), devHwIds[0]);


                GetDeviceDriverFiles(DeviceInfoSet, &DeviceInfoData);

                FOUND = TRUE;
                DelMultiSz(devHwIds);
                DelMultiSz(devCompatIds);
                break;
            }

            DelMultiSz(devHwIds);
            DelMultiSz(devCompatIds);
        }


        if (!FOUND) {
        printf("未找到TouchPad触控板的I2C设备！\n");
    }
    }
    else {
        printf("未找到TouchPad触控板设备！\n");
    }
     

    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    return FOUND;
}


BOOL FindCurrentDriver(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo, _In_ PSP_DRVINFO_DATA DriverInfoData)
{
    SP_DEVINSTALL_PARAMS deviceInstallParams;
    WCHAR SectionName[LINE_LEN];
    WCHAR DrvDescription[LINE_LEN];
    WCHAR MfgName[LINE_LEN];
    WCHAR ProviderName[LINE_LEN];
    HKEY hKey = NULL;
    DWORD RegDataLength;
    DWORD RegDataType;
    DWORD c;
    BOOL match = FALSE;
    long regerr;

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
    // The following method works in Win2k, but it's slow and painful.
    //
    // First, get driver key - if it doesn't exist, no driver
    //
    hKey = SetupDiOpenDevRegKey(Devs,
        DevInfo,
        DICS_FLAG_GLOBAL,
        0,
        DIREG_DRV,
        KEY_READ
    );

    if (hKey == INVALID_HANDLE_VALUE) {
        //
        // no such value exists, so there can't be an associated driver
        //
        RegCloseKey(hKey);
        return FALSE;
    }

    //
    // obtain path of INF - we'll do a search on this specific INF
    //
    RegDataLength = sizeof(deviceInstallParams.DriverPath); // bytes!!!
    regerr = RegQueryValueEx(hKey,
        REGSTR_VAL_INFPATH,
        NULL,
        &RegDataType,
        (PBYTE)deviceInstallParams.DriverPath,
        &RegDataLength
    );

    if ((regerr != ERROR_SUCCESS) || (RegDataType != REG_SZ)) {
        //
        // no such value exists, so no associated driver
        //
        RegCloseKey(hKey);
        return FALSE;
    }

    //
    // obtain name of Provider to fill into DriverInfoData
    //
    RegDataLength = sizeof(ProviderName); // bytes!!!
    regerr = RegQueryValueEx(hKey,
        REGSTR_VAL_PROVIDER_NAME,
        NULL,
        &RegDataType,
        (PBYTE)ProviderName,
        &RegDataLength
    );

    if ((regerr != ERROR_SUCCESS) || (RegDataType != REG_SZ)) {
        //
        // no such value exists, so we don't have a valid associated driver
        //
        RegCloseKey(hKey);
        return FALSE;
    }

    //
    // obtain name of section - for final verification
    //
    RegDataLength = sizeof(SectionName); // bytes!!!
    regerr = RegQueryValueEx(hKey,
        REGSTR_VAL_INFSECTION,
        NULL,
        &RegDataType,
        (PBYTE)SectionName,
        &RegDataLength
    );

    if ((regerr != ERROR_SUCCESS) || (RegDataType != REG_SZ)) {
        //
        // no such value exists, so we don't have a valid associated driver
        //
        RegCloseKey(hKey);
        return FALSE;
    }

    //
    // driver description (need not be same as device description)
    // - for final verification
    //
    RegDataLength = sizeof(DrvDescription); // bytes!!!
    regerr = RegQueryValueEx(hKey,
        REGSTR_VAL_DRVDESC,
        NULL,
        &RegDataType,
        (PBYTE)DrvDescription,
        &RegDataLength
    );

    RegCloseKey(hKey);

    if ((regerr != ERROR_SUCCESS) || (RegDataType != REG_SZ)) {
        //
        // no such value exists, so we don't have a valid associated driver
        //
        return FALSE;
    }

    //
    // Manufacturer (via SPDRP_MFG, don't access registry directly!)
    //

    if (!SetupDiGetDeviceRegistryProperty(Devs,
        DevInfo,
        SPDRP_MFG,
        NULL,      // datatype is guaranteed to always be REG_SZ.
        (PBYTE)MfgName,
        sizeof(MfgName), // bytes!!!
        NULL)) {
        //
        // no such value exists, so we don't have a valid associated driver
        //
        return FALSE;
    }

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


BOOL GetDeviceDriverFiles(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo)
{
    SP_DRVINFO_DATA driverInfoData;
    SP_DRVINFO_DETAIL_DATA driverInfoDetail;

    BOOL success = FALSE;

    ZeroMemory(&driverInfoData, sizeof(driverInfoData));
    driverInfoData.cbSize = sizeof(driverInfoData);

    if (!FindCurrentDriver(Devs, DevInfo, &driverInfoData)) {
        printf("FindCurrentDriver err!\n");
        return FALSE;
    }

    // get useful driver information
    //
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

    _tprintf(TEXT("driverInfoDetail.InfFileName[0]: [%s]\n"), driverInfoDetail.InfFileName);
    

    // pretend to do the file-copy part of a driver install
    // to determine what files are used
    // the specified driver must be selected as the active driver
    if (!SetupDiSetSelectedDriver(Devs, DevInfo, &driverInfoData)) {
        printf("SetupDiSetSelectedDriver err!\n");
        goto final;
    }

    if (wcscmp(driverInfoData.Description, DevDescription) == 0 && wcscmp(driverInfoData.MfgName, MfgName) == 0 && wcscmp(driverInfoData.ProviderName, ProviderName) == 0) {
        _tprintf(TEXT("driverInfoData.Description: [%s]\n"), driverInfoData.Description);
        _tprintf(TEXT("driverInfoData.MfgName: [%s]\n"), driverInfoData.MfgName);
        _tprintf(TEXT("driverInfoData.ProviderName: [%s]\n"), driverInfoData.ProviderName);

        wcscpy_s(OEMinf_FullName, driverInfoDetail.InfFileName);
        _tprintf(TEXT("OEMinf_FullName= [%s]\n"), OEMinf_FullName);

        // 获得INF目录
        WCHAR szPath[MAX_PATH];  // 获得系统目录
        GetWindowsDirectory( szPath, sizeof(szPath) );
        // 格式化文件路径
        wcscat_s( szPath, L"\\INF\\" ); 
        _tprintf(TEXT("INF PATH= [%s]\n"), szPath);

        //生成OEMinf_name
        wchar_t* pstr;
        pstr = mystrcat(OEMinf_FullName, szPath, L"\0");//szPath//L"C:\\WINDOWS\\INF\\"
        wcscpy_s(OEMinf_name, pstr);
        _tprintf(TEXT("OEMinf_name= [%s]\n"), OEMinf_name);

        printf("GetDeviceDriverFiles ok\n");
        success = TRUE;
    }
   

final:

    SetupDiDestroyDriverInfoList(Devs, DevInfo, SPDIT_CLASSDRIVER);

    if (!success) {
        printf("GetDeviceDriverFiles err!\n");
    }

    return success;

}


BOOL Install() {
    printf("start Install\n");

    BOOLEAN ret = FALSE;

    UnInstall();//先卸载再安装

    int retry = 0;
    Rescan();
    ret = InstallDriver();
    while (!ret) {
        Rescan();
        ret = InstallDriver();
        retry++;
        _tprintf(TEXT("InstallDriver retry= [%s]\n"), retry);

        if (retry > 10) {
            ret = FALSE;
            break;
        }
    }

    Rescan();
    return ret;

}

BOOL UnInstall() {
    printf("start UnInstall\n");

    BOOLEAN ret = FALSE;

    Rescan();

    ret = UnInstallDriver();

    //// 获得INF目录
    //WCHAR winPath[MAX_PATH];  // 获得系统目录
    //GetWindowsDirectory(winPath, sizeof(winPath));
    //// 格式化文件路径
    //WCHAR devSysPath[MAX_PATH];
    //wcscat_s(devSysPath, winPath);
    //wcscat_s(devSysPath, L"\\System32\\Drivers\\MouseLikeTouchPad_I2C.sys");
    //_tprintf(TEXT("devSysPath= [%s]\n"), devSysPath);

    //if (!DeleteFile(devSysPath)) {
    //    printf("DeleteFile err！\n");;
    //}

    Rescan();

    return ret;

}


BOOL InstallDriver() {
    printf("start InstallDriver\n");

    BOOLEAN ret = FALSE;

    ret = FindDevice();
     
    if (ret) {
        ret = update();
    }

    return ret;
}


BOOL UnInstallDriver() {
    printf("start UnInstallDriver\n");

    BOOLEAN ret = FALSE;

    ret = FindDevice();
    if (ret) {
        ret = DPDelete();
    }

    if (ret) {     
        ret = RemoveDevice();
    }

    return ret;
}


int
__cdecl
_tmain(_In_ int argc, _In_reads_(argc) PWSTR* argv)
{
    printf("start main\n");
    BOOLEAN ret;
    //ret = EnbalePrivileges();

    _tprintf(TEXT("argc= %d\n"), argc);
    _tprintf(TEXT("argv[0]= [%s]\n"), argv[0]);

    if (argc > 1) {
        _tprintf(TEXT("argv[1]= [%s]\n"), argv[1]);
        if (wcscmp(argv[1], L"Install") == 0) {
            ret = Install();
        }
        else if (wcscmp(argv[1], L"UnInstall") == 0) {
            ret = UnInstall();
        }
    }
    

    return ret;
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

        _tprintf(TEXT("Device ID is: [%s]\n"), buffer);
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
    BOOLEAN MATCH = FALSE;
    BOOLEAN FOUND = FALSE;
    BOOLEAN Success = FALSE;


    // Create a Device Information Set with all present devices.

    DeviceInfoSet = SetupDiGetClassDevs(NULL, // All Classes
        0,
        0,
        DIGCF_ALLCLASSES | DIGCF_PRESENT); // All devices present on system
    if (DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        printf("RemoveDevice SetupDiGetClassDevs err!\n");
        return FALSE;
    }

    _tprintf(TEXT("RemoveDevice Search for TouchPad_hwID Device ID: [%s]\n"), TouchPad_COMPATIBLE_hwID);

    // Enumerate through all Devices.
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    TCHAR buffer[256] = { 0 };
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
        //_tprintf(TEXT("devInstance_ID: [%s]\n"), devInstance_ID); 

        hwIds = GetDevMultiSz(DeviceInfoSet, &DeviceInfoData, SPDRP_HARDWAREID);
        compatIds = GetDevMultiSz(DeviceInfoSet, &DeviceInfoData, SPDRP_COMPATIBLEIDS);

        if (FuzzyCompareHwIds(hwIds, TouchPad_COMPATIBLE_hwID) || FuzzyCompareHwIds(compatIds, TouchPad_COMPATIBLE_hwID))
        {
            printf("RemoveDevice 找到TouchPad触控板设备！\n");
            _tprintf(TEXT("RemoveDevice TouchPad Device devInstance_ID= [%s]\n"), devInstance_ID);
            _tprintf(TEXT("RemoveDevice TouchPad Device hwIds= [%s]\n"), hwIds[0]);
            wcscpy_s(TouchPad_hwID, hwIds[0]);
            _tprintf(TEXT("RemoveDevice TouchPad_hwID= [%s]\n"), TouchPad_hwID);

            MATCH = TRUE;
            DelMultiSz(hwIds);
            DelMultiSz(compatIds);
            break;
        }

        DelMultiSz(hwIds);
        DelMultiSz(compatIds);
    }


    if (MATCH)
    {
        //生成I2C_hwID
        wchar_t* pstr;
        pstr = mystrcat(TouchPad_hwID, L"HID\\", L"ACPI\\");
        pstr = mystrcat(pstr, L"&Col02", L"\0");
        wcscpy_s(I2C_hwID, pstr);
        _tprintf(TEXT("RemoveDevice TouchPad I2C_hwID= [%s]\n"), I2C_hwID);

        //验证I2C设备
        printf("RemoveDevice 开始查找TouchPad触控板的I2C设备！\n");
        _tprintf(TEXT("RemoveDevice Search for I2C_hwID Device ID: [%s]\n"), I2C_COMPATIBLE_hwID);

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
            //_tprintf(TEXT("devInstID: [%s]\n"), devInstID);

            devHwIds = GetDevMultiSz(DeviceInfoSet, &DeviceInfoData, SPDRP_HARDWAREID);
            devCompatIds = GetDevMultiSz(DeviceInfoSet, &DeviceInfoData, SPDRP_COMPATIBLEIDS);

            if (FuzzyCompareHwIds(devHwIds, I2C_hwID) && FuzzyCompareHwIds(devCompatIds, I2C_COMPATIBLE_hwID))
            {
                printf("RemoveDevice 找到TouchPad触控板的I2C设备！\n");
                _tprintf(TEXT("RemoveDevice TouchPad I2C Device devInstID= [%s]\n"), devInstID);
                _tprintf(TEXT("RemoveDevice TouchPad I2C Device devHwIds= [%s]\n"), devHwIds[0]);

                BOOLEAN ret = GetDeviceDriverFiles(DeviceInfoSet, &DeviceInfoData);

                FOUND = TRUE;
                DelMultiSz(devHwIds);
                DelMultiSz(devCompatIds);
                break;
            }

            DelMultiSz(devHwIds);
            DelMultiSz(devCompatIds);
        }


        if (!FOUND) {
            printf("RemoveDevice 未找到TouchPad触控板的I2C设备！\n");
        }
        else {

            SP_REMOVEDEVICE_PARAMS rmdParams;
            SP_DEVINSTALL_PARAMS devParams;

            // need hardware ID before trying to remove, as we wont have it after
            SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;

            devInfoListDetail.cbSize = sizeof(devInfoListDetail);
            if (!SetupDiGetDeviceInfoListDetail(DeviceInfoSet, &devInfoListDetail)) {
                // skip this
                printf("RemoveDevice SetupDiGetDeviceInfoListDetail not exist！\n");
                Success = TRUE;//
                goto END;
            }

            rmdParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            rmdParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
            rmdParams.Scope = DI_REMOVEDEVICE_GLOBAL;
            rmdParams.HwProfile = 0;
            if (!SetupDiSetClassInstallParams(DeviceInfoSet, &DeviceInfoData, &rmdParams.ClassInstallHeader, sizeof(rmdParams)) ||
                !SetupDiCallClassInstaller(DIF_REMOVE, DeviceInfoSet, &DeviceInfoData)) {
                // failed to invoke DIF_REMOVE
                printf("RemoveDevice failed to invoke DIF_REMOVE！\n");
            }
            else {
                // see if device needs reboot
                devParams.cbSize = sizeof(devParams);
                if (SetupDiGetDeviceInstallParams(DeviceInfoSet, &DeviceInfoData, &devParams) && (devParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT))) {
                    // reboot required
                    printf("RemoveDevice reboot required！\n");
                }
                else {
                    // appears to have succeeded
                    printf("RemoveDevice succeeded！\n");
                }

            }


            Success = TRUE;//
        }
    }
    else {
        printf("RemoveDevice 未找到TouchPad触控板设备！\n");
    }


END:
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    return Success;
}


BOOL DPDelete()
{
    BOOLEAN failcode = FALSE;
    DWORD res;
    TCHAR InfFileName[MAX_PATH];
    LPCTSTR inf = NULL;
    PTSTR FilePart = NULL;

    inf = OEMinf_name;
    if (!inf[0]) {
        printf("inf err！\n");
        return FALSE;
    }
    _tprintf(TEXT("DP inf= [%s]\n"), inf);

    res = GetFullPathName(inf,
        ARRAYSIZE(InfFileName),
        InfFileName,
        &FilePart);
    if ((!res) || (!FilePart)) {
        printf("GetFullPathName err！\n");
        return FALSE;
    }

    _tprintf(TEXT("DP InfFileName: [%s]\n"), InfFileName);

    if (!SetupUninstallOEMInf(FilePart, PtpDrvMgr_FLAG_FORCE, NULL)) {
        if (GetLastError() == ERROR_INF_IN_USE_BY_DEVICES) {
            printf("ERROR_INF_IN_USE_BY_DEVICES！\n");
        }
        else if (GetLastError() == ERROR_NOT_AN_INSTALLED_OEM_INF) {
            printf("ERROR_NOT_AN_INSTALLED_OEM_INF！\n");
        }
        else {
            printf("DPDelete_FAILED！\n");
        }
        return FALSE;
    }

    printf("DPDelete ok！\n");
    return TRUE;

}



BOOL update()
{
    HMODULE newdevMod = NULL;
    UpdateDriverForPlugAndPlayDevicesProto UpdateFn;
    BOOL reboot = FALSE;
    LPCTSTR hwid = NULL;
    LPCTSTR inf = NULL;
    DWORD flags = 0;
    DWORD res;
    TCHAR InfPath[MAX_PATH];
    BOOLEAN Success = FALSE;

    printf("start update\n");

    inf = inf_name;
    if (!inf[0]) {
        printf("inf err！\n");
        return FALSE;
    }
    _tprintf(TEXT("update inf= [%s]\n"), inf);

    hwid = I2C_hwID;
    if (!hwid[0]) {
        printf("hwid err！\n");
        return FALSE;
    }
    _tprintf(TEXT("update hwid= [%s]\n"), hwid);

    // Inf must be a full pathname
    res = GetFullPathName(inf, MAX_PATH, InfPath, NULL);
    if ((res >= MAX_PATH) || (res == 0)) {

        // inf pathname too long
        printf("inf pathname too long！\n");
        return FALSE;
    }
    if (GetFileAttributes(InfPath) == (DWORD)(-1)) {

        // inf doesn't exist
        printf("inf doesn't exist！\n");
        return FALSE;
    }
    _tprintf(TEXT("update InfPath= [%s]\n"), InfPath);

    inf = InfPath;
    flags |= INSTALLFLAG_FORCE;

    // make use of UpdateDriverForPlugAndPlayDevices
    newdevMod = LoadLibrary(TEXT("newdev.dll"));
    if (!newdevMod) {
        printf("LoadLibrary newdev.dll failed！\n");
        goto final;
    }

    UpdateFn = (UpdateDriverForPlugAndPlayDevicesProto)GetProcAddress(newdevMod, UPDATEDRIVERFORPLUGANDPLAYDEVICES);
    if (!UpdateFn)
    {
        printf("Load UpdateFn failed！\n");
        goto final;
    }
    if (!UpdateFn(NULL, hwid, inf, flags, &reboot)) {
        printf("UpdateDriverForPlugAndPlayDevices failed！\n");
        return FALSE;
    }

    Success = TRUE;
    printf("UpdateFn ok！\n");


final:

    if (newdevMod) {
        FreeLibrary(newdevMod);
    }

    return Success;

}