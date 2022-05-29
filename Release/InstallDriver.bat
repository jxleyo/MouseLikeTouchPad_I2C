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

echo 开始检查安装文件
echo Start checking installation files
echo.

if exist MouseLikeTouchPad_I2C.inf (
    echo MouseLikeTouchPad_I2C.inf文件正常
) else (
    echo MouseLikeTouchPad_I2C.inf文件丢失，请检查或者重新下载驱动安装包
    echo MouseLikeTouchPad_I2C.inf File Lost，Please check or download the driver installation package again.
    pause
    exit
)

if exist MouseLikeTouchPad_I2C.cat (
    echo MouseLikeTouchPad_I2C.cat文件正常
) else (
    echo MouseLikeTouchPad_I2C.cat文件丢失，请检查或者重新下载驱动安装包
    echo MouseLikeTouchPad_I2C.cat File Lost，Please check or download the driver installation package again.
    pause
    exit
)

if exist MouseLikeTouchPad_I2C.sys (
    echo MouseLikeTouchPad_I2C.sys文件正常
) else (
    echo MouseLikeTouchPad_I2C.sys文件丢失，请检查或者重新下载驱动安装包
    echo MouseLikeTouchPad_I2C.sys File Lost，Please check or download the driver installation package again.
    pause
    exit
)
echo.


::开启延迟变量扩展
setlocal enabledelayedexpansion
echo.

 ::删除历史残留文件
del/f /q hid_dev.txt
del/f /q i2c_dev.txt
del/f /q dev*.tmp
del/f /q drv*.tmp
echo.

::删除历史记录文件
if exist Return.txt (
    del/f /q Return.txt
)
echo.

::写入开始运行bat的信号
echo StartBAT>Return.txt
echo.
	
echo Check Windows Version..
ver>winver.txt
echo.

find "10.0." winver.txt || (
	 echo 当前系统不是windows10/11，无法安装！
 	echo Current OS is not Windows10/11，Can't Install the Driver！
	 echo.
 	del/f /q winver.txt
 	echo.
	set var=VER_OS_ERR
	echo !var!>>Return.txt 
 	exit
) 

 ::windows10v2004最早版本号10.0.19041.264 //10.0.a.b格式，直接判断a>=19041即可，注意delims会把回车换行符自动算作分隔符所以多行情况下tokens的数值设置需要计算换行情况下的列号
 for /f "delims=[.] tokens=4" %%i in (winver.txt) do (
   set "winver=%%i"
 )

::数值字符串的值大小比较，注意需要引号，并且实际上数字字符串位数不同时大小比较不是准确的，比如2041和19041相比较会是错误结果
if ("%winver%" LSS "19041") (
     echo 当前windows系统版本太低，请升级后再试
	echo The current version of windows system is too low. Please upgrade and try again.
	echo.
	del/f /q winver.txt
	echo.
	set var=VER_LOW_ERR
	echo !var!>>Return.txt 
	exit
)

echo 当前windows系统版本匹配ok
echo Current Windows system version matches OK.
echo.

del/f /q winver.txt
echo.
set var=VER_OK
echo !var!>>Return.txt 
echo.

echo 开始查找所有的HID设备device
pnputil /scan-devices
echo scan-devices扫描设备ok
echo.
pnputil /enum-devices /connected /class {745a17a0-74d3-11d0-b6fe-00a0c90f57da}  /ids /relations >hid_dev.txt
echo enum-devices枚举设备ok
echo.

::检查是否找到touchpad触控板设备device
find/i "HID_DEVICE_UP:000D_U:0005" hid_dev.txt || (
     echo 未发现触控板设备，按任意键退出，请安装原厂驱动后再次尝试
     echo No TouchPad device found. Press any key to exit. Please install the original driver and try again.
     echo.
     set var=TP_NODEV_ERR
     echo !var!>>Return.txt
     exit
)

echo 找到touchpad触控板设备
echo TouchPad device found.
echo.

echo 开始查找touchpad触控板对应的父级I2C设备实例InstanceID
echo.
 
 ::替换回车换行符为逗号方便分割，注意后面NUL后面为追加>>写入
for /f "delims=" %%i in (hid_dev.txt) do (
   set /p="%%i,"<nul>>dev0.tmp
 )

::替换HID_DEVICE_UP:000D_U:0005为#方便分割，注意set /p需要加逗号
for /f "delims=, tokens=*" %%i in (dev0.tmp) do (
    set "str=%%i"
    set "str=!str:HID_DEVICE_UP:000D_U:0005=#!"
    set /p="!str!,"<nul>>dev1.tmp
)

  ::获取#分隔符后面的文本，注意set /p需要加逗号
 for /f "delims=# tokens=2,*" %%i in (dev1.tmp) do (
   set /p="%%i,"<nul>>dev2.tmp
 )

  ::获取:分隔符后面的文本，注意set /p需要加逗号
 for /f "delims=: tokens=2" %%i in (dev2.tmp) do (
   set /p="%%i,"<nul>>dev3.tmp
 )

   ::获取,分隔符前面的文本
 for /f "delims=, tokens=1" %%i in (dev3.tmp) do (
  set /p="%%i"<nul>>dev4.tmp
 )

    ::删除空格
 for /f "delims= " %%i in (dev4.tmp) do (
   set "str=%%i"
   echo !str!>i2c_dev_InstanceID.txt
 )
echo.

del/f /q hid_dev.txt
del/f /q dev*.tmp
echo.
 

 ::验证InstanceID
  for /f "delims=" %%i in (i2c_dev_InstanceID.txt) do (
   set "i2c_dev_InstanceID=%%i"
   echo i2c_dev_InstanceID="!i2c_dev_InstanceID!"
   echo.
 )

 ::注意加/connected表示已经启动
 pnputil /enum-devices /connected /instanceid "%i2c_dev_InstanceID%" /ids /relations /drivers >i2c_dev.txt
 echo.
 
 ::检查是否为i2c设备device
find/i "ACPI\PNP0C50" i2c_dev.txt || (
     echo 未发现i2c触控板设备，无法安装驱动
     echo No I2C TouchPad device found, unable to install driver.
     echo.
     del/f /q i2c_dev.txt
      set var=TP_NOI2C_ERR
     echo !var!>>Return.txt
     exit
)

echo 找到i2c触控板设备
echo I2C TouchPad device found.
echo.
set var=TP_OK
echo !var!>>Return.txt
echo.
 
 ::安装驱动，只添加到驱动库中不安装，注意后面一定不要加/install
  pnputil /add-driver MouseLikeTouchPad_I2C.inf
  echo add-driver安装驱动ok
  echo.
  
 ::删除i2c触控板设备
pnputil /remove-device "%i2c_dev_InstanceID%"
echo remove-device删除设备ok
echo.

 :扫描i2c触控板设备使其自动安装oem第三方驱动
pnputil /scan-devices
echo scan-devices扫描设备ok
echo.


::验证是否安装成功，注意加/connected表示已经启动
 pnputil /enum-devices /connected /instanceid "%i2c_dev_InstanceID%" /ids /relations /drivers >i2c_dev.txt
 echo.
 
find/i "MouseLikeTouchPad_I2C" i2c_dev.txt || (
     echo 安装驱动失败，请卸载第三方驱动后重新再试
     echo Failed to install the driver. Please uninstall the third-party driver and try again.
     echo.
     del/f /q i2c_dev.txt
     set var=INSTDRV_ERR
     echo !var!>>Return.txt
     exit
)

del/f /q i2c_dev.txt
echo.

echo 安装驱动成功
echo Driver installed successfully.
echo.
set var=INSTDRV_OK
echo !var!>>Return.txt
echo.

