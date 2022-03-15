echo off
echo 弹出窗口“允许此应用对你的设备进行更改“ 请选择“是”以获取管理员权限来运行本卸载脚本
pause

::获取管理员权限
%1 mshta vbscript:CreateObject("Shell.Application").ShellExecute("cmd.exe","/c %~s0 ::","","runas",1)(window.close)&&exit
::保持当前目录下运行
cd /d "%~dp0"

echo off
echo 开始运行卸载脚本，卸载驱动前请确保其他程序已关闭或文档已保存
pause

::检查安装文件
if exist devcon.exe (
    echo devcon.exe文件正常
) else (
    echo devcon.exe程序丢失，请检查或者重新下载驱动安装包
    pause
    exit
)

::重新扫描硬件
devcon rescan

::删除旧版驱动备份
dir /b C:\Windows\System32\DriverStore\FileRepository\hidi2c_touchpad.inf* >dir_del_list.tmp
for /f "delims=" %%i in (dir_del_list.tmp) do (
    rd/s /q C:\Windows\System32\DriverStore\FileRepository\%%i
    echo 旧版驱动文件%%i已删除
)
del/f /q dir_del_list.tmp

::查找touchpad触控板设备device
:seek
devcon hwids *HID_DEVICE_UP:000D_U:0005* >hwid0.tmp

::检查是否找到touchpad触控板设备device
find/i "HID" hwid0.tmp && (
    echo 找到触控板设备
) || (
     echo 未发现触控板设备，按任意键尝试将自动安装windows原版驱动再次进行卸载本驱动，假如后面多次尝试失败则本驱动可能不兼容该笔记本电脑
     echo 或者关闭本窗口退出安装，
     del/f /q hwid*.tmp
     pause
     devcon update hidi2c.inf ACPI\PNP0C50
     devcon rescan
     goto seek
)

::查找有&COL字符的行并加入行号
find/i /n "&COL" hwid0.tmp >hwid1.tmp
::过滤其他行保留首行
find/i "[1]" hwid1.tmp >hwid2.tmp

::开启延迟变量扩展
setlocal enabledelayedexpansion

::替换&COL成特殊字符方便分割，删除行号，替换触控板设备号开头
for /f "delims=" %%i in (hwid2.tmp) do (
    set "str=%%i"
    set "str=!str:&COL=^!"
    set "str=!str:[1]=!"
    set "str=!str:HID=ACPI!"
    echo !str!>hwid3.tmp
)

::以^分割字符串获取开头即可生成touchpad所使用的i2c总线mini port设备id
for /f "delims=^" %%i in (hwid3.tmp) do (
    set "hwidstr=%%i"
    echo !hwidstr!>hwid.txt
)
echo 触控板touchpad设备所使用的i2c总线mini port设备id为%hwidstr%

::删除临时文件
del/f /q hwid*.tmp

::查询touchpad触控板使用的i2c总线mini port设备当前驱动inf文件
reg query "HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Services\Hidi2c_TouchPad" /v "DisplayName" >oemInfName.tmp
for /f "delims=@, tokens=2"  %%i in (oemInfName.tmp) do (
    set "oemInfName=%%i"
)
echo oem.inf安装文件为%oemInfName%
del/f /q oemInfName*.tmp

::卸载touchpad触控板使用的i2c总线mini port设备历史驱动并重新扫描硬件
devcon disable %hwidstr%
devcon remove %hwidstr%
del/f /q C:\Windows\System32\Drivers\Hidi2c_TouchPad.sys
del/f /q C:\Windows\System32\Drivers\%oemInfName%
devcon dp_delete %oemInfName%
devcon rescan

::卸载自签名证书Reg delete EVRootCA.reg
reg delete "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\SystemCertificates\ROOT\Certificates\E403A1DFC8F377E0F4AA43A83EE9EA079A1F55F2" /f
echo EVRootCA.reg自签名证书卸载完成

echo Hidi2c_TouchPad第三方驱动已经卸载完成

::二次查找touchpad触控板设备device
:seek2
devcon hwids *HID_DEVICE_UP:000D_U:0005* >hwid0.tmp

::二次检查是否找到touchpad触控板设备device
find/i "HID" hwid0.tmp && (
    echo 找到触控板设备
    echo 如果触控板不工作请按任意键重启电脑
    echo 如果触控板运行正常则关闭本窗口以取消重启
    del/f /q hwid*.tmp
    pause
) || (
     echo 二次未发现触控板设备，按任意键尝试将自动安装windows原版驱动，假如后面多次尝试失败则windows原版驱动可能不兼容该笔记本电脑
     echo 或者关闭本窗口退出
     del/f /q hwid*.tmp
     pause
     devcon update hidi2c.inf ACPI\PNP0C50
     devcon rescan
     goto seek2
)

pause
shutdown -r -f -t 0
