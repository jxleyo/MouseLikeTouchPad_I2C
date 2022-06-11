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
echo.

::删除历史记录文件
if exist LogFIle\Return_InstDrv.txt (
    del/f /q LogFIle\Return_InstDrv.txt
)
if exist LogFIle\InstDrvSuccess.txt (
    del/f /q LogFIle\InstDrvSuccess.txt
)
echo.

::检测目录
if not exist LogFIle (
    md LogFIle
    echo.
)

echo 开始查找所有的HID设备device
pnputil /scan-devices
echo scan-devices扫描设备ok
echo.
pnputil /enum-devices /connected /class {745a17a0-74d3-11d0-b6fe-00a0c90f57da}  /ids /relations >LogFIle\hid_dev.txt
echo enum-devices枚举设备ok
echo.

::检查是否找到微软ACPI\MSFT0001标准硬件ID的touchpad触控板设备ACPI\VEN_MSFT&DEV_0001
find/i "ACPI\MSFT0001" LogFIle\hid_dev.txt || (
     echo 未发现触控板设备。
     echo No TouchPad device found. 
     echo.
     echo NotFoundTP >LogFIle\Return_InstDrv.txt
     exit
)

echo 找到touchpad触控板设备
echo TouchPad device found.
echo ACPI\MSFT0001 >LogFIle\TouchPad_I2C_FOUND.txt
echo.

 ::安装驱动，添加到驱动库中并且安装
  pnputil /add-driver Driver\MouseLikeTouchPad_I2C.inf /install
  echo add-driver安装驱动ok
  echo.
  

 :扫描i2c触控板设备
pnputil /scan-devices
echo scan-devices扫描设备ok
echo.

::验证是否安装成功，注意加/connected表示已经启动
pnputil /enum-devices /connected /class {745a17a0-74d3-11d0-b6fe-00a0c90f57da}  /ids /relations >LogFIle\hid_dev.txt
echo enum-devices枚举设备ok
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
echo INSTDRV_OK >LogFIle\InstDrvSuccess.txt
echo.

