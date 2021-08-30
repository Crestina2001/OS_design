# unknownOS - 2021年操作系统课程设计



[TOC]



## 1. 项目组成与实现概述

### 1.0 项目简介与分工

本项目以 《Oranges’一个操作系统的实现》作为基本框架，使用相关的核心代码。主要开发了内核程序。完成了 3 个系统级应用：类Shell控制台、进程管理和文件管理。同时，还实现了 ？ 个用户级应用，包括？？？？和 5？个小游戏（？？？）。以类Shell方式与用户进行交互，通过命令行实现了简单的图形界面。

分工如下：

1952060 张佰一：组长，负责环境搭建、框架梳理、代码整合与项目管理工作。

195XXXX 黄茂森：组员，负责文件系统。

195XXXX 申博：组员，负责进程管理。

1952894 徐道晟：组员，负责编写小游戏推箱子。

1951015 余世璇：组员，负责编写用户级应用日历和小游戏2048。

### 1.1 项目框架

本项目基于《Oranges：一个操作系统的实现》。以Bochs虚拟机为环境，以NASM（Netwide Assembler）为编译器，采用保护模式进行调试、makefile方法完成统一编译。

在Linux环境下使用该操作系统时，建议使用Ubuntu进行以下操作：

* 安装必要插件

  ```shell
  sudo apt-get install build-essential #开发必要的软件包
  sudo apt-get install xorg-dev #给Bochs配图形界面
  sudo apt-get install bison #必要的依赖包
  sudo apt-get install libgtk2.0-dev #必要的依赖包
  ```

* 安装Bochs

  - 下载Bochs-2.6.11.tar.gz包
  - 解压后进入文件夹

  ```shell
  ./configure --enable-debugger --enable-disasm
  ```

  - 打开Makefile，在92行左右的LIBS=...行尾添加-pthread。否则make时会报错。

  ```shell
  make
  sudo make install
  ```

* 安装NASM

  - 下载NASM-2.15.05.tar.gz包
  - 解压后进入文件夹

  ```shell
  ./configure
  make 
  sudo make install
  ```

* 修改工作路径

  - 进入到/unknownOS目录中

  - 修改`bochsrc`，将如下的三个路径更换为实际的路径

    ![](..\unknownOS\assets\Inked3_LI.jpg)

* 运行操作系统

  - 在`../../unknownOS`中启用Terminal

  - 输入bochs

  - 选择`[6. Begin simulation]`，或直接回车

  - 此时进入了bochs，但是处于黑屏状态。由于在引导扇区之前，为了调试方便增加了一个断点：

    ```assembly
    ...
    b			;ea5be000f0
    <bochs:1>b 0x7c00
    <bochs:2>c
    ...
    ```
  ```
  
  - 只需要在控制台输入`c`，就完成了引导扇区操作，我们的操作系统从这里开始运行。
  ```

### 1.2 项目结构

```shell
├── 80m.img
├── 80m.img.gz
├── a.img
├── BIOS-bochs-latest
├── bochsdbg-win64.exe
├── bochsrc
├── bochsrc.bxrc
├── boot
│   ├── boot.asm
│   ├── boot.bin
│   ├── include
│   │   ├── fat12hdr.inc
│   │   ├── load.inc
│   │   └── pm.inc
│   ├── loader.asm
│   └── loader.bin
├── fs
│   ├── disklog.c
│   ├── link.c
│   ├── main.c
│   ├── misc.c
│   ├── open.c
│   └── read_write.c
├── godbg.bat
├── include
│   ├── ConcurrencySal.h
│   ├── corecrt.h
│   ├── math.h
│   ├── sal.h
│   ├── stdio.h
│   ├── string.h
│   ├── sys
│   │   ├── config.h
│   │   ├── console.h
│   │   ├── const.h
│   │   ├── fs.h
│   │   ├── global.h
│   │   ├── hd.h
│   │   ├── keyboard.h
│   │   ├── keymap.h
│   │   ├── proc.h
│   │   ├── proc.h.bak
│   │   ├── protect.h
│   │   ├── proto.h
│   │   ├── sconst.inc
│   │   └── tty.h
│   ├── type.h
│   ├── vadefs.h
│   └── vcruntime.h
├── kernel
│   ├── clock.c
│   ├── console.c
│   ├── global.c
│   ├── hd.c
│   ├── i8259.c
│   ├── kernel.asm
│   ├── keyboard.c
│   ├── main.c
│   ├── proc.c
│   ├── protect.c
│   ├── start.c
│   ├── systask.c
│   └── tty.c
├── kernel.bin
├── krnl.map
├── lib
│   ├── close.c
│   ├── fileTree.c
│   ├── getpid.c
│   ├── kliba.asm
│   ├── klib.c
│   ├── misc.c
│   ├── open.c
│   ├── printf.c
│   ├── read.c
│   ├── string.asm
│   ├── syscall.asm
│   ├── syslog.c
│   ├── unlink.c
│   ├── vsprintf.c
│   └── write.c
├── Makefile
├── scripts
│   ├── genlog
│   └── splitgraphs
├── VGABIOS-lgpl-latest
└── x11-pc-us.map

8 directories, 79 files

```

其中，`a.img`是操作系统所在的软盘，Bochs引导扇区引导指向这个软盘。

其余的文件，主要问这几类：`.asm`, `.inc`, `.c`, `.h`和它们生成的`.o`, `.bin`。

我们将`boot.asm`和`loader.asm`放在单独的目录/boot中，作为配置模式、启动操作的文件夹。/lib中存放使用到的库，项目核心的内核程序都放在/kernel里面。其余的是硬盘：`80m.img`，系统的文件系统/fs和Makefile、bochsrc。

### 1.3 Makefile编译

编译的对象文件如下

```makefile
# This Program
ORANGESBOOT	= boot/boot.bin boot/loader.bin
ORANGESKERNEL	= kernel.bin
OBJS		= kernel/kernel.o lib/syscall.o kernel/start.o kernel/main.o\
			kernel/clock.o kernel/keyboard.o kernel/tty.o kernel/console.o\
			kernel/i8259.o kernel/global.o kernel/protect.o kernel/proc.o\
			kernel/systask.o kernel/hd.o\
			lib/printf.o lib/vsprintf.o\
			lib/kliba.o lib/klib.o lib/string.o lib/misc.o\
			lib/open.o lib/read.o lib/write.o lib/close.o lib/unlink.o\
			lib/getpid.o lib/syslog.o\
			fs/main.o fs/open.o fs/misc.o fs/read_write.o\
			fs/link.o\
			fs/disklog.o
DASMOUTPUT	= kernel.bin.asm
```

除了基本的kernel，还有一些操作系统必要的组件，比如clock处理时钟中断、tty处理终端操作以及其他的I/O必要函数。

### 1.4 内核汇编文件

```assembly
...
extern	k_reenter
extern	sys_call_table
...
global _start	; 导出 _start
global restart
global sys_call
...
```

当我们把控制权交给内核之后，利用`extern`和`global`关键字可以实现汇编语言和C语言的来去切换。在我们的程序中，/kernel/main.c是内核的主要组成部分。它控制了与用户交互和显示的内容。在该文件中，可以找到`kernel_main()`函数，其中的`restart()`是进程调度的一部分，也是我们的操作系统启动第一个进程时的入口。

### 1.5 I/O的实现

这一部分以调用《一个操作系统的实现》作者的代码为主。

本操作系统采用类Shell的操作方式，相应用户的输入。这就要求我们在bochs上实现输入输出函数。本项目使用TTY作为任务进程，肩负着配置键盘输入的重大任务。

输出的操作，采用了`printf()`函数，调用过程如下：

![](..\unknownOS\assets\1.jpg)

输入的操作，结合文件系统进行。主要是TTY与文件系统进行结合，为了完成类似Shell的输入方式，实现时有以下的关键代码：

```c
char tty_name[] = "/dev_tty1";
/*
 *	寻找到在根目录的文件：dev_tty0
 */
int fd_stdin = open(tty_name, O_RDWR);
/*
 *	以可读写的方式打开tty文件
 */
assert(fd_stdin == 0);
/*
 *	handle 异常情况
 */
char rdbuf[128];
/*
 *	在内存中开通一个数组，接收dev_tty0中的用户输入内容。根据实际情况变化
 */

/* ======================================================= */
/*  	        当需要读取用户输入时，使用以下代码               */
/* ======================================================= */

int r = read(fd_stdin, rdbuf, 70);
/*
 *	在内存中开通一个数组，接收dev_tty0中的用户输入内容。
 *  最后一个参数为读取的内容大小，根据实际情况变化。
 *  该函数的返回值为读取到内容的长度。
 */
rdbuf[r] = 0;
/*
 *	在之前开通的接收数组中，加入0值来确定实际内容的结尾
 */

/* ======================================================= */
/*  当需要比较用户输入与预设功能提供相应功能时，使用以下代码         */
/* ======================================================= */

if(strcmp(rdbuf, "hello") == 0){}
/*
 *	使用strcmp()函数来比较。
 */
```



## 2. 系统级应用



## 3. 用户级应用

### 3.1 帮助界面
### 3.2 日历
本项目实现了日历功能，通过输入年份以及月份，可以输出该月的日历，并对不符的输入进行检测与提示。
显示某月日历：

![](..\unknownOS\assets\cal_init.png)

检测年份是否有误：

![](..\unknownOS\assets\cal_wrong_year.png)

检测月份是否有误:

![](..\unknownOS\assets\cal_wrong_month.png)


### 3.3 2048
本项目实现了经典的2048小游戏，通过 wsad 进行上下左右移动的操作，并给出实时分数，并可通过 q 退出（quit）。
初始化：![](..\unknownOS\assets\2048.png)

通过不断操作，分数不断上升，当游戏无法再进行下去时，或者要主动退出，按q可退出游戏

![](..\unknownOS\assets\2048_exit.png)

### 3.4 推箱子
本项目实现了经典的推箱子小游戏，通过 wsad 控制人物（P）上下左右移动的操作，并给出实时分数，每有一个箱子在目标区域（D）得一分，并可通过 q 退出（quit）。
初始化：

![](..\unknownOS\assets\box.jpg)

通过不断操作，分数不断上升，当游戏无法再进行下去时，或者要主动退出，按q可退出游戏

![](..\unknownOS\assets\box-exit.jpg)
