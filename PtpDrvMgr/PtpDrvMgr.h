#pragma once

#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <setupapi.h>
#include <regstr.h>
#include <infstr.h>
#include <cfgmgr32.h>
#include <string.h>
#include <malloc.h>
#include <newdev.h>
#include <objbase.h>
#include <strsafe.h>
#include <io.h>
#include <fcntl.h>

#include <hidsdi.h>



BOOL EnbalePrivileges();
BOOL EnumerateDevices();
BOOL FindDevice();
BOOL RemoveDevice();
BOOL update();
BOOL DPDelete();
BOOL UnInstall();
BOOL Install();
BOOL UnInstallDriver();
BOOL InstallDriver();

wchar_t* mystrcat(const wchar_t* str1, const wchar_t* str2, const wchar_t* str3);//str1是原来的字符串，str2是str1中的某段需要替换字符串，str3是替换str2的字符串
BOOL FuzzyCompareHwIds(PZPWSTR Array, const wchar_t* MatchHwId);
BOOL FindCurrentDriver(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo, _In_ PSP_DRVINFO_DATA DriverInfoData);
BOOL GetDeviceDriverFiles(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo);

void DelMultiSz(_In_opt_ __drv_freesMem(object) PZPWSTR Array);
__drv_allocatesMem(object) LPTSTR* GetDevMultiSz(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo, _In_ DWORD Prop);
__drv_allocatesMem(object) LPTSTR* GetMultiSzIndexArray(_In_ __drv_aliasesMem LPTSTR MultiSz);

#define PtpDrvMgr_FLAG_FORCE       0x00000001


#ifdef _UNICODE
#define UPDATEDRIVERFORPLUGANDPLAYDEVICES "UpdateDriverForPlugAndPlayDevicesW"
#define SETUPUNINSTALLOEMINF "SetupUninstallOEMInfW"
#else
#define UPDATEDRIVERFORPLUGANDPLAYDEVICES "UpdateDriverForPlugAndPlayDevicesA"
#define SETUPUNINSTALLOEMINF "SetupUninstallOEMInfA"
#endif

// UpdateDriverForPlugAndPlayDevices
typedef BOOL(WINAPI* UpdateDriverForPlugAndPlayDevicesProto)(_In_opt_ HWND hwndParent,
    _In_ LPCTSTR HardwareId,
    _In_ LPCTSTR FullInfPath,
    _In_ DWORD InstallFlags,
    _Out_opt_ PBOOL bRebootRequired
    );

typedef BOOL(WINAPI* SetupUninstallOEMInfProto)(_In_ LPCTSTR InfFileName,
    _In_ DWORD Flags,
    _Reserved_ PVOID Reserved
    );
