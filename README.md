# MouseLikeTouchPad-Hidi2c_Driver
仿鼠标式触摸板MouseLikeTouchPad-Hidi2c_Driver on windows10驱动

仿鼠标式触摸板是一种模拟鼠标功能的触摸板技术实现，当前的逻辑实现版本是发明人基于仿鼠标触摸板专利技术根据人手指操作时自然状态再次优化改进而成，3指完成鼠标左键/右键/中键和指针的操作，手指与鼠标的各个按键等部件的功能一一对应，其中的中指对应鼠标的指针定位器，食指对应鼠标左键和中键（食指与中指分开时定义为鼠标左键，食指与中指并拢时定义为鼠标中键），无名指对应鼠标右键，中指无名指或者中指与食指2指一起快速触摸后滑动操作时对应鼠标垂直滚轮和水平滚轮，单指重按触控板左下角物理键为鼠标的后退功能键，单指重按触控板右下角物理键为鼠标的前进功能键，单指重按触控板下沿中间物理键为调节鼠标DPI灵敏度（慢/中等/快3段灵敏度），双指重按触控板下沿物理键为滚轮开关（开启鼠标滚轮及触摸板手势功能方便日常应用操作，临时关闭鼠标滚轮及触摸板手势功能以降低游戏误操作率），三指重按触控板下沿物理键为切换滚轮的操作方式（精确式触摸板双指滑动手势/模仿鼠标的滚轮操作，方便少数对触控板双指滑动支持差的应用如PTC Creo可以使用模仿的鼠标滚轮操作），四指重按触控板下沿物理键为切换仿鼠标式触摸板操作方式的开关（关闭时恢复为windows系统原本的触控板方式以方便其他未学习仿鼠标式触摸板操作方法的用户使用，同时也能通过对比体现出仿鼠标式触摸板驱动的强大之处）。

手指与鼠标按键功能定义规则： 
根据鼠标左键/中键普遍用食指操作的习惯、手指自然状态来区分确定鼠标左键/右键/中键操作与滚动操作；中指对应鼠标的指针定位器；中指被定义为指针后食指首次接触到触控板的时间与中指定义为指针的时间差超过阈值常量时食指操作定义 为鼠标左键或中键（食指与中指为分开状态时食指操作定义为鼠标左键，食指与中指并拢状态时食指操作定义为鼠标中键），食指与中指由分开状态切换为并拢状态时系统判定鼠标左键按下状态不变同时中键判定为按下状态，食指与中指由并拢状态切换为分开状态时系统判定鼠标中键释放同时左键判定为按下状态，食指与中指由并拢状态或者分开状态切换到食指离开触摸板时系统判定鼠标左键中键都是释放；中指被定义为指针后无名指首次接触到触控板的时间与中指定义为指针的时间差超过阈值常量时无名指操作定义为鼠标右键，指针被定义后无名指/食指首次接触到触控板的时间与中指定义为指针的时间差小于阈值常量时（中指无名指或者中指与食指2指一起快速触摸后滑动操作）定义为鼠标垂直或水平滚轮（正常情况中指无名指或者中指与食指2指一起滑动操作的舒适性最高）。

多点电容式触摸板根据触摸点接触面形状很容易解决手掌的误触（打字时触摸板支撑手掌的椭圆接触面的长宽比特征比正常手指大所以很容易排除过滤掉）；

本人于2012年左右就已经有这个想法但因为技术原因一直没有条件实现，最近机会成熟了才开始自己开发笔记本电脑的触摸板驱动，本驱动程序仅凭我一人之力历经近1年时间艰苦攻坚奋战开发而成，驱动基本框架为原生Windows系统触摸板驱动Hidi2c.sys的逆向，并参考了微软公开的SPB驱动Sample，所有数据结构、函数源代码精准还原，修改注释掉极少部分代码并增加大量仿鼠标触摸板逻辑实现代码，获取到手指触摸点数据并完美还原全部手势操作， 驱动带有数字签名证书安装简便安全，欢迎大家免费下载使用，另外只有采用Parallel Report Mode并行报告模式的触控板有手势和切换回windows原版触摸板操作方式的功能，混合报告模式的触摸板手势操作存在诸多问题未解决所以不实现其手势及切换回原版触摸板操作方式功能。


驱动下载：
(1)方法1：
复制下面的链接到浏览器地址栏后回车
https://github.com/jxleyo/MouseLikeTouchPad-Hidi2c_Driver/archive/refs/heads/main.zip
直接下载驱动

(1)方法2：
复制下面的链接到浏览器地址栏后回车进入下载页面
https://github.com/jxleyo/MouseLikeTouchPad-Hidi2c_Driver
点击绿色的code按钮后再点击Download ZIP进行下载

安装和卸载方法：
下载的驱动zip文件解压到任意位置后打开，进入Installation Package子文件夹，安装/卸载驱动分别打开Install安装脚本.bat/Uninstall卸载脚本.bat文件，根据窗口文字提示操作即可完成驱动安装/卸载。


使用操作视频教程网址：
https://space.bilibili.com/409976933
https://www.youtube.com/channel/UC3hQyN-2ZL_q7pCKoASAblQ



