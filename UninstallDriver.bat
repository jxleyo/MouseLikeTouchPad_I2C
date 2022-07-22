echo off
echo 弹出窗口“允许此应用对你的设备进行更改“ 请选择“是”以获取管理员权限来运行本安装脚本
echo Pop up window "allow this app to make changes to your device" please select "yes" to obtain administrator rights to run this installation script
echo.

::获取管理员权限
%1 mshta vbscript:CreateObject("Shell.Application").ShellExecute("cmd.exe","/c %~s0 ::","","runas",1)(window.close)&&exit
::保持当前目录下运行
cd /d "%~dp0"

echo off
echo 开始运行安装脚本，安装驱动前请确保其他程序已关闭或文档已保存
echo Start running the installation script. Before installing the driver, make sure that other programs have been closed or the document has been saved.
echo.


::开启延迟变量扩展
setlocal enabledelayedexpansion
echo.

 ::删除历史残留文件
 if exist LogFIle\hid_dev.txt (
    del/f /q LogFIle\hid_dev.txt
)
 if exist LogFIle\i2c_dev.txt (
    del/f /q LogFIle\i2c_dev.txt
)
del/f /q LogFIle\dev*.tmp
del/f /q LogFIle\drv*.tmp
echo.

::删除历史记录文件
if exist LogFIle\Return_UninstDrv.txt (
    del/f /q LogFIle\Return_UninstDrv.txt
)
if exist LogFIle\UninstDrvSucceeded.txt (
    del/f /q LogFIle\UninstDrvSucceeded.txt
)
echo.


::检测目录
if not exist LogFIle (
    md LogFIle
    echo.
)


pnputil /scan-devices
echo scan-devices扫描设备ok
echo.

echo 开始查找所有的HID设备device，注意加/connected表示已经连接
pnputil /enum-devices /connected /class {745a17a0-74d3-11d0-b6fe-00a0c90f57da}  /ids /relations >LogFIle\hid_dev.txt
echo enum-devices枚举设备ok
echo.

::检查是否找到touchpad触控板设备device
find/i "HID_DEVICE_UP:000D_U:0005" LogFIle\hid_dev.txt || (
     echo 未发现触控板设备，无需卸载驱动
     echo No TouchPad device found, no need to unload the driver.
     echo.
     echo UNDRV_OK >LogFIle\Return_UninstDrv.txt
     echo UNDRV_OK >LogFIle\UninstDrvSucceeded.txt
     echo.
     exit
)

echo 找到touchpad触控板设备
echo TouchPad device found.
echo.


echo 开始查找touchpad触控板对应的父级I2C设备实例InstanceID
echo.
 
 ::替换回车换行符为逗号方便分割，注意后面NUL后面为追加>>写入
for /f "delims=" %%i in (LogFIle\hid_dev.txt) do (
   set /p="%%i,"<nul>>LogFIle\dev0.tmp
 )

::替换HID_DEVICE_UP:000D_U:0005为#方便分割，注意set /p需要加逗号
for /f "delims=, tokens=*" %%i in (LogFIle\dev0.tmp) do (
    set "str=%%i"
    set "str=!str:HID_DEVICE_UP:000D_U:0005=#!"
    set /p="!str!,"<nul>>LogFIle\dev1.tmp
)

  ::获取#分隔符后面的文本，注意set /p需要加逗号
 for /f "delims=# tokens=2,*" %%i in (LogFIle\dev1.tmp) do (
   set /p="%%i,"<nul>>LogFIle\dev2.tmp
 )

  ::获取:分隔符后面的文本，注意set /p需要加逗号
 for /f "delims=: tokens=2" %%i in (LogFIle\dev2.tmp) do (
   set /p="%%i,"<nul>>LogFIle\dev3.tmp
 )

   ::获取,分隔符前面的文本
 for /f "delims=, tokens=1" %%i in (LogFIle\dev3.tmp) do (
  set /p="%%i"<nul>>LogFIle\dev4.tmp
 )

    ::删除空格
 for /f "delims= " %%i in (LogFIle\dev4.tmp) do (
   set "str=%%i"
   echo !str!>LogFIle\TouchPad_I2C_devInstanceID.txt
 )
echo.

del/f /q LogFIle\hid_dev.txt
del/f /q LogFIle\dev*.tmp
echo.
 

 ::验证InstanceID
  for /f "delims=" %%i in (LogFIle\TouchPad_I2C_devInstanceID.txt) do (
   set "i2c_dev_InstanceID=%%i"
   echo i2c_dev_InstanceID="!i2c_dev_InstanceID!"
   echo.
 )

 ::注意加/connected表示已经启动
 pnputil /enum-devices /connected /instanceid "%i2c_dev_InstanceID%" /ids /relations /drivers >LogFIle\i2c_dev.txt
 echo.
 
::检查是否安装MouseLikeTouchPad_I2C驱动
find/i "MouseLikeTouchPad_I2C" LogFIle\i2c_dev.txt || (
     echo 未发现MouseLikeTouchPad_I2C驱动，无需卸载驱动
     echo No MouseLikeTouchPad_I2C driver found, no need to unload the driver.
     echo.
     del/f /q LogFIle\i2c_dev.txt
     del/f /q LogFIle\TouchPad_I2C_devInstanceID.txt
     echo UNDRV_OK >LogFIle\Return_UninstDrv.txt
     echo UNDRV_OK >LogFIle\UninstDrvSucceeded.txt
     exit
)

echo 找到MouseLikeTouchPad_I2C驱动
echo.

echo 开始查找MouseLikeTouchPad_I2C驱动oem文件名
echo.

 ::删除历史残留文件
del/f /q LogFIle\drv*.tmp
 echo.
 
 ::替换回车换行符为逗号方便分割，注意后面NUL后面为追加>>写入
for /f "delims=" %%i in (LogFIle\i2c_dev.txt) do (
   set /p="%%i,"<nul>>LogFIle\drv0.tmp
 )

::替换mouseliketouchpad_i2c.inf为#方便分割，注意set /p需要加逗号
for /f "delims=, tokens=*" %%i in (LogFIle\drv0.tmp) do (
    set "str=%%i"
    set "str=!str:mouseliketouchpad_i2c.inf=#!"
    set /p="!str!,"<nul>>LogFIle\drv1.tmp
)

  ::获取#分隔符前面的文本
 for /f "delims=# tokens=1" %%i in (LogFIle\drv1.tmp) do (
   set /p="%%i"<nul>>LogFIle\drv2.tmp
 )

::替换oem为[方便分割，注意set /p需要加逗号
for /f "delims=, tokens=*" %%i in (LogFIle\drv2.tmp) do (
    set "str=%%i"
    set "str=!str:oem=[!"
    set /p="!str!,"<nul>>LogFIle\drv3.tmp
)

  ::获取最后一个[分隔符后面的文本，注意tokens要选2及后面的所有列并且nul后面不是追加而是>
 for /f "delims=[ tokens=2,*" %%i in (LogFIle\drv3.tmp) do (
   set /p="%%i,"<nul>LogFIle\drv4.tmp
 )

   ::获取,分隔符前面的文本
 for /f "delims=, tokens=1" %%i in (LogFIle\drv4.tmp) do (
  set /p="oem%%i"<nul>LogFIle\oemfilename.txt
 )
echo.

 ::删除历史残留文件
del/f /q LogFIle\drv*.tmp
echo.


  ::读取oemfilename
  for /f "delims=" %%i in (LogFIle\oemfilename.txt) do (
   set "oemfilename=%%i"
   echo oemfilename="!oemfilename!"
   echo.
 )

 :卸载oem第三方驱动，
 pnputil /delete-driver "%oemfilename%" /uninstall /force
 echo delete-driver卸载驱动ok
echo.

pnputil /scan-devices
echo scan-devices扫描设备ok
echo.


::验证是否卸载成功，注意加/connected表示已经启动
 pnputil /enum-devices /connected /instanceid "%i2c_dev_InstanceID%" /ids /relations /drivers >LogFIle\i2c_dev.txt
 echo.
 
find/i "MouseLikeTouchPad_I2C" LogFIle\i2c_dev.txt && (
     echo 卸载驱动失败，请重新再试
     echo Failed to unload the driver. Please try again.
     echo.
     del/f /q LogFIle\i2c_dev.txt
     echo UNDRV_FAILED >LogFIle\Return_UninstDrv.txt
     exit
)

del/f /q LogFIle\i2c_dev.txt
del/f /q LogFIle\TouchPad_I2C_devInstanceID.txt
echo 卸载驱动成功
echo Unload driver succeeded.
echo.
echo UNDRV_OK >LogFIle\Return_UninstDrv.txt
echo UNDRV_OK >LogFIle\UninstDrvSucceeded.txt
echo.