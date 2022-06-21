#pragma once

#include "resource.h"



//
#include <strsafe.h>

#include <setupapi.h>
#include <cfgmgr32.h>
#include <newdev.h>

#include <hidsdi.h>

#include <atlstr.h>
#include <shellapi.h>

#include "shlobj.h" 
#pragma comment(lib, "shell32.lib")




#define MltpDrvMgr_FLAG_FORCE       0x00000001

// exit codes
#define EXIT_OK      (0)
#define EXIT_REBOOT  (1)
#define EXIT_FAIL    (2)
#define EXIT_USAGE   (3)


BOOL EnbalePrivileges();
BOOL EnumerateDevices();
BOOL Rescan();
BOOL FindDevice();
BOOL RemoveDriver();
BOOL UpdateDriver();
BOOL UninstallDriver();
BOOL InstallDriver();
void Install();
void Uninstall();

void InstallShortcut();
void UninstallShortcut();
void Register();
BOOL WriteSzReg(LPCWSTR szPath, LPCWSTR szKey, LPCWSTR dwValue);
BOOL ReadDWordReg(LPCWSTR szPath, LPCWSTR szKey, DWORD* dwValue);
BOOL WriteDWordReg(LPCWSTR szPath, LPCWSTR szKey, DWORD* dwValue);
BOOL WriteBinReg(LPCWSTR szPath, LPCWSTR szKey, LPBYTE dwValue, DWORD dwSize);
BOOL ReadBinReg(LPCWSTR szPath, LPCWSTR szKey, DWORD* dwValue, DWORD* dwSize);
BOOL DelRegkey(LPCWSTR szPath, LPCWSTR szKey);
BOOL DelDir(LPCWSTR lpszPath);
BOOL GetDesktopPath(wchar_t* szPath);
BOOL GetProgramsPath(wchar_t* szPath);
BOOL CreateShotCut(LPCWSTR strSourcePath, LPCWSTR strSourceFileName, LPCWSTR strShortcutName, LPCWSTR strDestDir);

BOOL DirExist(LPCWSTR szFilePathName);
BOOL FileExist(LPCWSTR szFilePathName);
BOOL MainFileExist(LPCWSTR szFileName);
BOOL DrvFileExist(LPCWSTR szFileName);
BOOL LogFileExist(LPCWSTR szFileName);
BOOL NewLogFile(LPCWSTR szFileName);
void DelLogFile(LPCWSTR szFileName);
BOOL GetTouchPad_I2C_hwID();
BOOL GetOEMDriverName();
BOOL SaveOEMDriverName();
BOOL SaveTouchPad_I2C_hwID();


wchar_t* mystrcat(const wchar_t* str1, const wchar_t* str2, const wchar_t* str3);//str1是原来的字符串，str2是str1中的某段需要替换字符串，str3是替换str2的字符串
BOOL FuzzyCompareHwIds(PZPWSTR Array, const wchar_t* MatchHwId);
BOOL FindCurrentDriver(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo, _In_ PSP_DRVINFO_DATA DriverInfoData);
BOOL GetDeviceOEMdriverInfo(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo);
void Set_InstallationSources_Directory(wchar_t* szPath);

void DelMultiSz(_In_opt_ __drv_freesMem(object) PZPWSTR Array);
__drv_allocatesMem(object) LPTSTR* GetDevMultiSz(_In_ HDEVINFO Devs, _In_ PSP_DEVINFO_DATA DevInfo, _In_ DWORD Prop);
__drv_allocatesMem(object) LPTSTR* GetMultiSzIndexArray(_In_ __drv_aliasesMem LPTSTR MultiSz);



