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
echo.

::删除历史记录文件
if exist LogFIle\Return_InstDrv.txt (
    del/f /q LogFIle\Return_InstDrv.txt
)
if exist LogFIle\InstDrvSucceeded.txt (
    del/f /q LogFIle\InstDrvSucceeded.txt
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
     echo 未发现触控板设备，请安装原厂驱动后再次尝试
     echo No TouchPad device found. Please install the original driver and try again
     echo.
     echo INSTDRV_FAILED >LogFIle\Return_InstDrv.txt
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
 
 ::检查是否为i2c设备device
find/i "ACPI\PNP0C50" LogFIle\i2c_dev.txt || (
     echo 未发现i2c触控板设备，无法安装驱动
     echo No I2C TouchPad device found, unable to install driver.
     echo.
     del/f /q LogFIle\i2c_dev.txt
     del/f /q LogFIle\TouchPad_I2C_devInstanceID.txt
     echo INSTDRV_FAILED >LogFIle\Return_InstDrv.txt
     exit
)

echo 找到touchpad触控板I2C设备
echo TouchPad I2C device found.
echo.


if not exist LogFIle\TouchPad_I2C_devInstanceID.txt (
     echo 获取设备实例失败。
     echo Failed to get devInstanceID. 
     echo devInstanceID_FAILED >LogFIle\Return_InstDrv.txt
     echo.
     exit
)

  ::从TouchPad_I2C_devInstanceID.txt文件读取设备实例
  for /f "delims=" %%i in (LogFIle\TouchPad_I2C_devInstanceID.txt) do (
   set "i2c_devInstanceID=%%i"
   echo i2c_devInstanceID="!i2c_devInstanceID!"
   echo.
 )
 
::安装驱动，只添加到驱动库中不安装，注意后面一定不要加/install
  pnputil /add-driver Driver\MouseLikeTouchPad_I2C.inf
  echo add-driver安装驱动ok
  echo.
  
 ::删除i2c触控板设备
pnputil /remove-device "%i2c_devInstanceID%"
echo remove-device删除设备ok
echo.

 :扫描i2c触控板设备使其自动安装oem第三方驱动
pnputil /scan-devices
echo scan-devices扫描设备ok
echo.


::验证是否安装成功，注意加/connected表示已经启动
 pnputil /enum-devices /connected /instanceid "%i2c_devInstanceID%" /ids /relations /drivers >LogFIle\i2c_dev.txt
 echo.
 
find/i "MouseLikeTouchPad_I2C" LogFIle\hid_dev.txt || (
     echo 安装驱动失败，请稍后再试
     echo Failed to install the driver. Please  try again later.
     echo.
     echo INSTDRV_FAILED >LogFIle\Return_InstDrv.txt
     del/f /q LogFIle\hid_dev.txt
     exit
)

del/f /q LogFIle\hid_dev.txt
echo.


echo 安装驱动成功
echo Driver installed successfully.
echo.
echo INSTDRV_OK >LogFIle\Return_InstDrv.txt
echo INSTDRV_OK >LogFIle\InstDrvSucceeded.txt
echo.
