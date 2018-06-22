# 个人所有的项目都在这里

## FkExecutePaser
花了一周时间 边学 PE 格式的时候边写的
QT开发，本来最开始打算写的是一个安全管家的、后来发现其实安全管家这种基本就没啥用嘛
所以改成了写一个 PE 工具、但是 windows 下已经有 pebear 了，所以写了一周也没动力了
下一步应该会把这个改成一个跨平台的 PE/ELF/DEX 解析工具

目前实现的功能：
- PE 基本字段/区段表/导出表/导入表/资源表/重定位表/TLS 的解析
- 系统垃圾目录文件的清理
- 进程/服务/线程/模块

[TODO]:
- 重新在 pebear 的基础上实现 elf/dex 的解析
- 去掉别的乱七八糟的功能，修改为一个单纯的 PE 工具
- 收集查壳工具的 Signature, 然后将 peid 的查壳等功能集成进来
- 跨平台的考虑
- 界面的重新设计

## FkPack
一周左右边学壳边写的一个简单加密壳，毕竟练手向，除了让我学习了一下壳的原理以外目前来看用处不大
在对于指令虚拟化、混淆这些没有更深的理解前不打算继续

已实现的功能：
- 代码段加密
- IAT 加密
- 通过 IsDebuge 这个 APi 做的简单反调试
- ...

## FKScripts_x64dbg
脱壳时写的脱壳脚本，反正脱一个就留一个脚本在这里，持续更新

目前有：
- ASpack
- Upx
- FSG
- PESpin
- Armadillo

[TODO]:
- Themida 
- ASProtect
- Enigma
- Vmp

## Fkdbg
大概一周吧，学调试机制、windows 的异常处理流程的时候写的一个 Ring3 命令行调试器
代码写得很丑，也不打算重新写一个调试器了
x64dbg 这么好用还造什么自行车？

实现的功能：
- 软件断点
- 硬件断点
- 内存断点
- 条件断点

[TODO]: 
- 有时间写 x64dbg 的插件才是正道
- x64dbg 下还没有什么特别好的分析虚拟机的工具，这是一点
- x64dbg 的反反调试插件也是一个值得做的东西，这方面得参考大牛了，唉、没怎么写过驱动对于内核就很弱啊
- 然后还有一个方向是 x64dbg 的脚本编辑体验不太好，每次都要在文档里编辑感觉就很蛋疼
- 应该考虑把 x64dbg-python 的脚本编辑与正常的脚本编辑整合到一起
