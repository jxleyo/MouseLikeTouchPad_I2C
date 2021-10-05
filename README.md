# MouseLikeTouchPad-Hidi2c_Driver
仿鼠标式触摸板MouseLikeTouchPad-Hidi2c_Driver on windows10驱动

仿鼠标式触摸板是一种模拟鼠标功能的触摸板技术实现，当前的逻辑实现版本是发明人基于仿鼠标触摸板专利技术根据人手指操作时自然状态再次优化改进而成，3指完成鼠标左键/右键/中键和指针的操作，手指与鼠标的各个按键等部件的功能一一对应，其中的中指对应鼠标的指针定位器，食指对应鼠标左键和中键（食指与中指分开时定义为鼠标左键，食指与中指并拢时定义为鼠标中键），无名指对应鼠标右键，中无名并拢时对应鼠标垂直滚轮和水平滚轮，触控板重按物理键为调节鼠标灵敏度（慢/中等/快3段灵敏度）。

手指与鼠标按键功能对应关系： 中指对应鼠标的指针定位器； 食指对应鼠标左键和中键，根据鼠标左键、中键普遍用食指操作的习惯及实际可行性来确定的，食指与中指分开时食指操作定义为鼠标左键，食指与中指并拢时食指操作定义为鼠标中键，食指与中指由分开状态切换为并拢状态时系统判定鼠标左键按下状态不变同时中键判定为按下状态，食指与中指由并拢状态切换为分开状态时系统判定鼠标中键释放同时左键判定为按下状态，食指与中指由并拢状态或者分开状态切换到食指离开触摸板时系统判定鼠标左键中键都是释放； 无名指对应鼠标右键；根据鼠标右键、屏幕滚动操作的习惯及实际可行性来确定的，无名指与中指分开时无名指操作定义为鼠标右键，无名指与中指并拢时无名指中指合并操作定义为鼠标垂直或水平滚轮同时中指的指针定义保持不变但不会移动指针直到两指分开或者离开触摸板（正常中指无名指并拢滑动操作舒适性操作性最高并且一般中指比无名指更早接触到触摸板使得系统判定不会有歧义）。

多点电容式触摸板根据触摸点接触面形状很容易解决手掌的误触（打字时触摸板支撑手掌的椭圆接触面的长宽比特征比正常手指大很容易排除过滤掉）；

本人尝试过联系笔记本厂商、触摸板方案商甚至微软来提供逻辑实现方法但是没有渠道联系不上或者人家及大量IT人士认为没有价值所以最终决定自己开发主流windows笔记本的触摸板驱动（因为windows系统比macos好用、笔记本硬件价格便宜并且普及适应性广），本驱动程序仅凭我一人之力历经半年多时间艰苦攻坚奋战开发而成，驱动基本框架为原生Windows系统触摸板驱动Hidi2c.sys的逆向，并参考了微软公开的SPB驱动Sample，所有数据结构、函数源代码精准还原，修改注释掉极少部分代码并增加大量仿鼠标触摸板逻辑实现代码，获取到手指触摸点数据并完美还原全部手势操作， 驱动带有数字签名证书安装简便安全，欢迎大家免费下载使用。

安装方法：
https://github.com/jxleyo/MouseLikeTouchPad-Hidi2c_Driver
下载驱动后解压到任意文件夹，打开MouseLikeTouchPad-Hidi2c_Driver/x64/Release/Hidi2c_TouchPad文件夹，双击打开VRootCA.reg导入数字签名证书到注册表，然后右键Hidi2c_TouchPad.inf文件选择安装点击信任即可安装完成，最后重启笔记本电脑就能正常使用了。

