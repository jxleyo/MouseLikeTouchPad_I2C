echo off
echo 弹出窗口“允许此应用对你的设备进行更改“ 请选择“是”以获取管理员权限来运行本安装脚本
pause
echo.

::获取管理员权限
%1 mshta vbscript:CreateObject("Shell.Application").ShellExecute("cmd.exe","/c %~s0 ::","","runas",1)(window.close)&&exit
::保持当前目录下运行
cd /d "%~dp0"

echo off
echo 开始运行安装脚本，安装驱动前请确保其他程序已关闭或文档已保存
pause
echo.

echo 开始检查安装文件
echo.
if exist devcon.exe (
    echo devcon.exe文件正常
) else (
    echo devcon.exe程序丢失，请检查或者重新下载驱动安装包
    pause
    exit
)

if exist MouseLikeTouchPad_Hidi2c.inf (
    echo MouseLikeTouchPad_Hidi2c.inf文件正常
) else (
    echo MouseLikeTouchPad_Hidi2c.inf文件丢失，请检查或者重新下载驱动安装包
    pause
    exit
)

if exist MouseLikeTouchPad_Hidi2c.cat (
    echo MouseLikeTouchPad_Hidi2c.cat文件正常
) else (
    echo MouseLikeTouchPad_Hidi2c.cat文件丢失，请检查或者重新下载驱动安装包
    pause
    exit
)

if exist MouseLikeTouchPad_Hidi2c.sys (
    echo MouseLikeTouchPad_Hidi2c.sys文件正常
) else (
    echo MouseLikeTouchPad_Hidi2c.sys文件丢失，请检查或者重新下载驱动安装包
    pause
    exit
)

if exist EVRootCA.reg (
    echo EVRootCA.reg文件正常
) else (
    echo EVRootCA.reg文件丢失，请检查或者重新下载驱动安装包
    pause
    exit
)
echo.


echo 开始删除驱动文件备份
dir /b C:\Windows\System32\DriverStore\FileRepository\MouseLikeTouchPad_Hidi2c.inf* >dir_del_list.tmp
for /f "delims=" %%i in (dir_del_list.tmp) do (
    rd/s /q C:\Windows\System32\DriverStore\FileRepository\%%i
    echo 旧版驱动文件备份%%i已删除
    echo.
)
del/f /q dir_del_list.tmp
echo.

echo 重新扫描硬件
devcon rescan
echo.

::开启延迟变量扩展
setlocal enabledelayedexpansion

echo 开始查找所有的I2C设备device
devcon hwids ACPI\PNP0C50 >i2c_dev_all.txt
echo.

::检查是否找到I2C设备device
find/i "PNP0C50" i2c_dev_all.txt && (
    echo.
    echo 找到I2C设备
    echo.
) || (
     echo.
     echo 未发现I2C设备，本驱动与此笔记本电脑触控板硬件所采用的bus总线不兼容
     echo 无法安装，按任意键退出
     del/f /q i2c_dev_all.txt
     pause
     exit
)

:seek
echo 开始安装windows原版触控板驱动
devcon update C:\Windows\INF\hidi2c.inf ACPI\PNP0C50
devcon rescan
devcon update hidi2c.inf ACPI\PNP0C50
devcon rescan
echo.

echo 开始查找touchpad触控板设备device
devcon hwids *HID_DEVICE_UP:000D_U:0005* >hwid0.tmp
echo.
     
::检查是否找到touchpad触控板设备device
find/i "HID" hwid0.tmp && (
    echo.
    echo 找到touchpad触控板设备
    echo.
) || (
     echo.
     echo 未发现touchpad触控板设备，按任意键将自动安装Windows原版驱动后再次尝试查找触控板设备
     echo 多次尝试都是 未发现触控板设备 则本驱动不兼容该笔记本电脑硬件
     del/f /q hwid*.tmp
     echo.
     pause
     goto seek
)


echo 开始查找touchpad触控板对应的I2C设备id
::查找有&COL字符的行并加入行号
find/i /n "&COL" hwid0.tmp >hwid1.tmp
::过滤其他行保留首行
find/i "[1]" hwid1.tmp >hwid2.tmp

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
    set "hwIDstr=%%i"
    echo !hwIDstr!>hwID.txt
)
echo 人体学输入设备/人机接口设备列表内touchpad触控板对应的I2C设备id为  %hwIDstr%
echo 按任意键继续安装
pause
echo.

::删除临时文件
del/f /q hwid*.tmp


echo 开始查找touchpad触控板对应的I2C设备名称
devcon hwids %hwIDstr% >i2c_dev_tp.txt
echo.

::检查是否找到touchpad触控板对应的I2C设备device
find/i "PNP0C50" i2c_dev_tp.txt && (
    echo.
    echo 找到touchpad触控板对应的I2C设备
    echo.
) || (
     echo.
     echo 未发现touchpad触控板对应的I2C设备，无法安装，按任意键退出
     del/f /q i2c_dev_tp.txt
     pause
     exit
)

::查找有name字符的行
find/i "name:" i2c_dev_tp.txt >i2c_dev_tp0.tmp

::替换Hardware IDs成特殊字符方便分割
for /f "delims=" %%i in (i2c_dev_tp0.tmp) do (
    set "str=%%i"
    set "str=!str:Hardware IDs=:!"
    echo !str!>i2c_dev_tp1.tmp
)

::以“:"分割字符串获取开头即可生成touchpad触控板对应的I2C设备名称
for /f "delims=: tokens=2"  %%i in (i2c_dev_tp1.tmp) do (
    set "hwNameStr=%%i" 
    echo !hwNameStr!>hwName.txt
)
echo 人体学输入设备/人机接口设备列表内touchpad触控板对应的I2C设备名称为  %hwNameStr%
echo 按任意键继续安装
pause
echo.

::删除临时文件
del/f /q i2c_dev_*.*


echo 开始查询是否安装了第三方驱动MouseLikeTouchPad_Hidi2c以及oem*.inf文件名称
::注意重定向输出文件名不要保护oem字样否则注册表未找到时会输出该文件名到保存的文件内容中干扰后续判断
for /f "delims=@, tokens=2"  %%i in ('reg query "HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Services\MouseLikeTouchPad_Hidi2c" /v "DisplayName"') do (
    set "infFileName=%%i" 
    echo !infFileName!>infResult.txt
)
echo.
    
find/i "oem" infResult.txt && (
    echo.
    echo 找到触控板设备第三方驱动MouseLikeTouchPad_Hidi2c
    echo.
    for /f "delims=" %%j in (infResult.txt) do (
    set "infFileName=%%j"
    )
    echo oem安装文件为%infFileName%
    echo.
) || (
     echo.
     echo 未发现第三方驱动MouseLikeTouchPad_Hidi2c
     echo.
     goto inst
)


:uninst
echo 开始卸载旧版touchpad触控板驱动
devcon -f dp_delete %infFileName% && (
    echo.
    echo 旧版触控板驱动卸载成功
) || (
     echo 旧版触控板驱动卸载失败
)
echo.

echo 开始删除遗留的驱动文件
del/f /q C:\Windows\INF\%infFileName%
del/f /q C:\Windows\System32\Drivers\MouseLikeTouchPad_Hidi2c.sys
echo.

echo 开始再次删除驱动文件备份
dir /b C:\Windows\System32\DriverStore\FileRepository\MouseLikeTouchPad_Hidi2c.inf* >dir_del_list.tmp
for /f "delims=" %%i in (dir_del_list.tmp) do (
    rd/s /q C:\Windows\System32\DriverStore\FileRepository\%%i
    echo 遗留的驱动文件备份%%i已删除
    echo.
)
del/f /q dir_del_list.tmp
echo.

echo 开始删除驱动的注册表信息
echo.
reg delete "HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Services\MouseLikeTouchPad_Hidi2c" /f && (
    echo 驱动注册表信息已删除
) || (
     echo 驱动的注册表信息不存在或者注册表操作错误
)
echo.

echo 旧版MouseLikeTouchPad_Hidi2c第三方驱动已经卸载完成
echo.


:inst
echo 开始安装自签名证书Reg import EVRootCA.reg
regedit /s EVRootCA.reg && (
    echo EVRootCA.reg自签名证书安装完成
) || (
     echo EVRootCA.reg注册表操作错误，安装失败
)
echo.

echo 开始安装新驱动
devcon update MouseLikeTouchPad_Hidi2c.inf %hwIDstr%  && (
    echo.
    devcon rescan
    echo MouseLikeTouchPad_Hidi2c第三方驱动已经安装完成
    echo.
    echo 请重压一下触控板按键查看是否工作正常，如果正常请关闭本窗口以取消重启
    echo 如果触控板不工作请按任意键重启电脑
    pause

) || (
     echo.
     devcon update C:\Windows\INF\hidi2c.inf %hwIDstr%
     devcon rescan
     devcon update hidi2c.inf %hwIDstr%
     devcon rescan
     echo.
     echo MouseLikeTouchPad_Hidi2c第三方驱动安装准备就绪，按任意键重启电脑后再次运行本安装程序即可成功安装
     pause 
)
echo.

shutdown -r -f -t 0



