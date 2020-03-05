# 环境

1. 操作系统：Ubuntu 16.04 LTS 64位

2. 编译的内核版本： 4.14.2

3. cmake version: 3.5.1

4. gcc version: 5.4.0

# 步骤

1. 从清华镜像源下载内核源码 https://mirrors.tuna.tsinghua.edu.cn/kernel/  version 4.14.2

2. 将linux/目录下文件拷贝覆盖到/usr/src/linux-4.14.2/下对应目录下

3. 编译内核，需要安装libncurses5-dev, libssl-dev依赖

```
cd /usr/src/linux-4.14.2
sudo make mrproper
sudo make menuconfig
sudo make
sudo make modules_install
sudo make insall
sudo update-grub
```
