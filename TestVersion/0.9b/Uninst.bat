echo off
echo �������ڡ�������Ӧ�ö�����豸���и��ġ� ��ѡ���ǡ��Ի�ȡ����ԱȨ�������б���װ�ű�
echo Pop up window "allow this app to make changes to your device" please select "yes" to obtain administrator rights to run this installation script
echo.

::��ȡ����ԱȨ��
%1 mshta vbscript:CreateObject("Shell.Application").ShellExecute("cmd.exe","/c %~s0 ::","","runas",1)(window.close)&&exit
::���ֵ�ǰĿ¼������
cd /d "%~dp0"

echo off
echo ��ʼ���а�װ�ű�����װ����ǰ��ȷ�����������ѹرջ��ĵ��ѱ���
echo Start running the installation script. Before installing the driver, make sure that other programs have been closed or the document has been saved.
echo.


::�����ӳٱ�����չ
setlocal enabledelayedexpansion
echo.

 ::ɾ����ʷ�����ļ�
 if exist LogFIle\hid_dev.txt (
    del/f /q LogFIle\hid_dev.txt
)
echo.


::ɾ����ʷ��¼�ļ�
if exist LogFIle\Return_UninstDrv.txt (
    del/f /q LogFIle\Return_UninstDrv.txt
)
if exist LogFIle\UninstDrvSuccess.txt (
    del/f /q LogFIle\UninstDrvSuccess.txt
)
echo.

::���Ŀ¼
if not exist LogFIle (
    md LogFIle
    echo.
)

if not exist LogFIle\OEMDriverName.txt (
     echo ��ȡOEM�����ļ���ʧ�ܡ�
     echo Failed to unload the driver. 
     echo OEM_FAILED >LogFIle\Return_UninstDrv.txt
     echo.
     exit
)

  ::��OEMDriverName.txt�ļ���ȡoemfilename
  for /f "delims=" %%i in (LogFIle\OEMDriverName.txt) do (
   set "oemfilename=%%i"
   echo oemfilename="!oemfilename!"
   echo.
 )

 :ж��oem����������
 pnputil /delete-driver "%oemfilename%" /uninstall /force
 echo delete-driverж������ok
echo.

pnputil /scan-devices
echo scan-devicesɨ���豸ok
echo.


::��֤�Ƿ�ж�سɹ���ע���/connected��ʾ�Ѿ�����
pnputil /enum-devices /connected /class {745a17a0-74d3-11d0-b6fe-00a0c90f57da}  /ids /relations >LogFIle\hid_dev.txt
 echo.
 
find/i "MouseLikeTouchPad_I2C" LogFIle\hid_dev.txt && (
     echo ж������ʧ�ܡ�
     echo Failed to unload the driver. 
     echo UNDRV_FAILED >LogFIle\Return_UninstDrv.txt
     echo.
     del/f /q LogFIle\hid_dev.txt
     exit
)

echo ��ʼж����ǩ��֤��Reg delete EVRootCA.reg
echo.
reg delete "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\SystemCertificates\ROOT\Certificates\E403A1DFC8F377E0F4AA43A83EE9EA079A1F55F2" /f && (
    echo EVRootCA.reg��ǩ��֤��ж�����
) || (
     echo EVRootCA.reg��ǩ��֤�鲻���ڻ���ע�����������
)
echo.

del/f /q LogFIle\hid_dev.txt
echo ж�������ɹ�
echo Unload driver succeeded.
echo.
echo UNDRV_OK >LogFIle\Return_UninstDrv.txt
echo UNDRV_OK >LogFIle\UninstDrvSuccess.txt
echo.
