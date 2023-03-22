# lab2 实验文档
- [lab2 实验文档](#lab2-实验文档)
  - [0. 前言](#0-前言)
    - [主要工作](#主要工作)
  - [1. LLVM IR 部分](#1-llvm-ir-部分)
    - [1.1 LLVM IR 介绍](#11-llvm-ir-介绍)
    - [1.2 gcd 例子: 利用 clang 生成的 .ll](#12-gcd-例子-利用-clang-生成的-ll)
    - [1.3 你的提交1: 手动编写 .ll](#13-你的提交1-手动编写-ll)
  - [2. LightIR 部分](#2-lightir-部分)
    - [2.1 LightIR - LLVM IR 的 cpp 接口](#21-lightir---llvm-ir-的-cpp-接口)
    - [2.2 gcd 例子: 利用 LightIR + cpp 生成 .ll](#22-gcd-例子-利用-lightir--cpp-生成-ll)
    - [2.3 你的提交2: 利用 LightIR + cpp 编写生成 .ll 的程序](#23-你的提交2-利用-lightir--cpp-编写生成-ll-的程序)
  - [3. Lab3 的准备](#3-lab3-的准备)
    - [3.1 了解 Visitor Pattern](#31-了解-visitor-pattern)
  - [4. 实验要求](#4-实验要求)
    - [4.1 目录结构](#41-目录结构)
    - [4.2 编译、运行和验证](#42-编译运行和验证)
    - [4.3 提交要求和评分标准](#43-提交要求和评分标准)
## 0. 前言

本次实验作为 Lab3 的前驱实验，独立于 Lab1。
本次实验的目的是让大家熟悉 Lab3 所需要的相关知识: LLVM IR、 LightIR（LLVM IR 的轻量级 C++ 接口）和 Visitor Pattern（访问者模式）。  
在开始实验之前，请根据之前的[编译实验环境搭建](http://202.38.79.174/compiler_staff/2022fall-environment/-/blob/master/README.md)确保LLVM的版本为10.0.1，且PATH环境变量配置正确。可以通过`lli --version`命令是否可以输出10.0.1的版本信息来验证（其它版本不一定兼容）。  
本次实验设置的目的是为 Lab3 进行知识的准备与热身， coding 的工程量不大，但是需要一定的阅读、学习、理解。因此本次的实验报告相比之下内容要求会稍微多一些，以避免大家在 Lab3 时手足无措。
这里助教提供了简单的[C++简介](../../Documentations/common/simple_cpp.md)，对C++基本特性不熟悉的同学可以先阅读该文档。

### 主要工作

1. 第一部分: 了解 LLVM IR。通过 clang 生成的 .ll ，了解 LLVM IR 与 c 代码的对应关系，**完成1.3**。
2. 第二部分: 了解 LightIR。通过助教提供的 c++ 例子，了解 LightIR 的 c++ 接口及实现，**完成2.3**。
3. 第三部分: 理解 Visitor Pattern 。**完成3.1**
4. 实验报告：在 [report.md](../../Reports/2-ir-gen-warmup/report.md) 中**回答3个问题**。

## 1. LLVM IR 部分
### 1.1 LLVM IR 介绍
根据[维基百科](https://zh.wikipedia.org/zh-cn/LLVM)的介绍，LLVM是一个自由软件项目，它是一种编译器基础设施，以C++写成，包含一系列模块化的编译器组件和工具链，用来开发编译器前端和后端。IR的全称是Intermediate Representation，即中间表示。LLVM IR是一种类似于汇编的底层语言。

LLVM IR的具体指令可以参考 [Reference Manual](https://llvm.org/docs/LangRef.html)。由于其手册过于复杂，助教筛选了后续实验中将要用到的子集，总结为了 [Light IR 手册](../common/LightIR.md#ir-%E6%A0%BC%E5%BC%8F)。如果有感兴趣的同学可以阅读原手册作为拓展。

作为一开始的参考，你可以先阅读其中 `IR格式` 和 `IR指令` 两节，后续有需要再反复参考。实验的最后，你需要在 [report.md](../../Reports/2-ir-gen-warmup/report.md) 中**回答问题1**。

### 1.2 gcd 例子: 利用 clang 生成的 .ll
阅读 [tests/2-ir-gen-warmup/ta_gcd/gcd_array.c](../../tests/2-ir-gen-warmup/ta_gcd/gcd_array.c)。

根据 `clang -S -emit-llvm gcd_array.c` 指令，你可以得到对应的 `gcd_array.ll` 文件.你需要结合 [gcd_array.c](../../tests/2-ir-gen-warmup/ta_gcd/gcd_array.c) 阅读 `gcd_array.ll` ，理解其中每条LLVM IR指令与c代码的对应情况。

通过 `lli gcd_array.ll; echo $?` 指令，你可以测试 `gcd_array.ll` 执行结果的正确性。其中，

- `lli` 会运行 `*.ll` 文件
- `$?` 的内容是上一条命令所返回的结果，而 `echo $?` 可以将其输出到终端中

后续你会经常用到这两条指令。

### 1.3 你的提交1: 手动编写 .ll
助教提供了四个简单的c程序，分别是 `tests/2-ir-gen-warmup/c_cases/` 目录下的 [assign.c](../../tests/2-ir-gen-warmup/c_cases/assign.c)、 [fun.c](../../tests/2-ir-gen-warmup/c_cases/fun.c)、 [if.c](../../tests/2-ir-gen-warmup/c_cases/if.c) 和 [while.c](../../tests/2-ir-gen-warmup/c_cases/while.c)。你需要在 `tests/2-ir-gen-warmup/stu_ll/` 目录中，手工完成自己的 [assign_hand.ll](../../tests/2-ir-gen-warmup/stu_ll/assign_hand.ll)、 [fun_hand.ll](../../tests/2-ir-gen-warmup/stu_ll/fun_hand.ll)、 [if_handf.ll](../../tests/2-ir-gen-warmup/stu_ll/if_hand.ll) 和 [while_hand.ll](../../tests/2-ir-gen-warmup/stu_ll/while_hand.ll)，以实现与上述四个C程序相同的逻辑功能。我们鼓励添加必要的注释。`.ll` 文件的注释是以 ";" 开头的。

必要的情况下，你可以参考 `clang -S -emit-llvm` 的输出，但是你提交的结果必须避免同此输出一字不差。


## 2. LightIR 部分
### 2.1 LightIR - LLVM IR 的 cpp 接口
由于 LLVM IR 官方的 cpp 接口的调用链同样过于冗长，助教提供了 `LightIR` 这一 cpp 接口库。你需要阅读 [LightIR cpp APIs](../common/LightIR.md#c-apis)。
lab3 部分会要求大家通过 `LightIR` 根据 `AST` 构建生成 LLVM IR。所以你需要仔细阅读文档了解其接口的设计。

### 2.2 gcd 例子: 利用 LightIR + cpp 生成 .ll
为了让大家更直观地感受并学会 `LightIR` 接口的使用，助教提供了 [tests/2-ir-gen-warmup/ta_gcd/gcd_array_generator.cpp](../../tests/2-ir-gen-warmup/ta_gcd/gcd_array_generator.cpp)。该 cpp 程序会生成与 gcd_array.c 逻辑相同的 LLVM IR 文件。助教提供了非常详尽的注释，请认真阅读作为参考！
该程序的编译与运行请参考 4.2 节。

### 2.3 你的提交2: 利用 LightIR + cpp 编写生成 .ll 的程序
你需要在 `tests/2-ir-gen-warmup/stu_cpp/` 目录中，编写 [assign_generator.cpp](../../tests/2-ir-gen-warmup/stu_cpp/assign_generator.cpp)、 [fun_generator.cpp](../../tests/2-ir-gen-warmup/stu_cpp/fun_generator.cpp)、 [if_generator.cpp](../../tests/2-ir-gen-warmup/stu_cpp/if_generator.cpp) 和 [while_generator.cpp](../../tests/2-ir-gen-warmup/stu_cpp/while_generator.cpp)，以生成与 1.3 节的四个 c 程序相同逻辑功能的 `.ll` 文件。你需要完成 [report.md](../../Reports/2-ir-gen-warmup/report.md) 中的**问题2**。

## 3. Lab3 的准备
### 3.1 了解 Visitor Pattern
Visitor Pattern（访问者模式）是一种在 LLVM 项目源码中被广泛使用的设计模式。在遍历某个数据结构（比如树）时，如果我们需要对每个节点做一些额外的特定操作， Visitor Pattern 就是个不错的思路。
Visitor Pattern 是为了解决**稳定的数据结构**和**易变的操作耦合问题**而产生的一种设计模式。解决方法就是在被访问的类里面加一个对外提供接待访问者的接口，其关键在于在数据基础类里面有一个方法接受访问者，将自身引用传入访问者。这里举一个应用实例来帮助理解访问者模式: 您在朋友家做客，您是访问者；朋友接受您的访问，您通过朋友的描述，然后对朋友的描述做出一个判断，这就是访问者模式。
有关 Visitor Pattern 的含义、模式和特点，有梯子的同学可参考 [维基百科](https://en.wikipedia.org/wiki/Visitor_pattern#C++_example)。其中较为重要的一点原则在于， C++ 中对函数重载特性的支持。
在 `tests/2-ir-gen-warmup/calculator` 中，助教编写了一个利用访问者模式，产生计算算数表达式的中间代码的程序。该程序首先对算数表达式进行语法分析生成语法树，再使用访问者模式来遍历语法树，产生中间代码。在 [calc_ast.hpp](../../tests/2-ir-gen-warmup/calculator/calc_ast.hpp) 中，我们定义了语法树的不同节点类型。在 [calc_builder.cpp](../../tests/2-ir-gen-warmup/calculator/calc_builder.cpp) 中，我们使用 LightIR 编写了不同的 `visit` 函数。根据节点类型的不同，编译器会在多种 `visit` 函数中，选择对应的实现进行调用。请认真阅读这两个文件和其它相关代码，理解语法树是如何通过访问者模式被遍历的，并在 [report.md](../../Reports/2-ir-gen-warmup/report.md)中**回答问题3**。


该程序使用方法如下:
``` shell
# 在 build 目录下操作
$ make
$ ./calc
Input an arithmatic expression (press Ctrl+D in a new line after you finish the expression):
4 * (8 + 4 - 1) / 2
result and result.ll have been generated.
$ ./result
22
```
其中，`result.ll` 是程序产生的中间代码，`result` 是中间代码编译产生的二进制，运行它就可以输出算数表达的结果。注：单独运行 `lli result.ll` 是会报错的，那怎么才能解决报错的根源问题，通过 `lli` 得到正确的运行结果呢？感兴趣的同学可以思考调研一下。


## 4. 实验要求

### 4.1 目录结构
除了下面指明你所要修改或提交的文件，其他文件请勿修改。
``` log
.
├── CMakeLists.txt
├── Documentations
│   ├── ...
|   ├── common                          <- LightIR 相关文档
│   └── 2-ir-gen-warmup
│       └── README.md                   <- lab2 实验文档说明（你在这里）
├── include                             <- 实验所需的头文件
│   ├── ...
│   ├── lightir
├── README.md
├── Reports
│   ├── ...
│   └── 2-ir-gen-warmup
│       └── report.md                   <- lab2 所需提交的实验报告，含3个问题（你要交）
├── src
│   ├── ...
│   └── lightir
└── tests
    ├── CMakeLists.txt
    ├── ...
    └── 2-ir-gen-warmup                 <- lab2 文件夹
        ├── c_cases                     <- 4个 c 程序
        │   ├── assign.c
        │   ├── fun.c
        │   ├── if.c
        │   └── while.c
        ├── CMakeLists.txt              <- 你在2.3节需要去掉注释（你要交）
        ├── stu_cpp                     <- lab2 所需提交的 cpp 目录（你要交）
        │   ├── assign_generator.cpp
        │   ├── fun_generator.cpp
        │   ├── if_generator.cpp
        │   └── while_generator.cpp
        ├── stu_ll                      <- lab2 所需提交的 .ll 目录（你要交）
        │   ├── assign_hand.ll
        │   ├── fun_hand.ll
        │   ├── if_hand.ll
        │   └── while_hand.ll
        └── ta_gcd
            ├── gcd_array.c
            └── gcd_array_generator.cpp <- 助教提供的生成 gcd_array.ll 的 cpp
```

### 4.2 编译、运行和验证

* 编译与运行
  在 `${WORKSPACE}/build/` 下执行:
  ``` shell
  # 如果存在 CMakeCache.txt 要先删除
  # rm CMakeCache.txt
  cmake ..
  make
  make install
  ```
  你可以得到对应 `gcd_array_generator.cpp` 的可执行文件。
  在完成2.3时，在 `${WORKSPACE}/tests/2-ir-gen-warmup/CMakeLists.txt` 中去掉对应的注释，再在 `${WORKSPACE}/build/` 下执行 `cmake ..` 与 `make` 指令，即可得到对应的可执行文件。
* 验证
  本次试验测试案例只有`${WORKSPACE}/tests/2-ir-gen-warmup/c_cases`中的4个样例。请大家自行验证。
  助教会执行你们的代码，并使用 `diff` 命令进行验证。

### 4.3 提交要求和评分标准
* 提交要求
  本实验的提交要求分为两部分: 实验部分的文件和报告，git提交的规范性。
  * 实验部分:
    * 需要完成 `./tests/2-ir-gen-warmup/stu_ll` 目录下的4个文件
    * 需要完成 `./tests/2-ir-gen-warmup/stu_cpp` 目录下的4个文件
    * 需要在 `./Reports/2-ir-gen-warmup/` 目录下撰写实验报告
      * 实验报告内容包括:
        * 实验要求、3个问题、实验难点、实验反馈(具体参考 [report.md](../../Reports/2-ir-gen-warmup/report.md))
        * 本次实验报告**参与**评分标准.
  * git 提交规范:
    * 不破坏目录结构( `report.md` 如果需要放图片，请放在 `./Reports/2-ir-gen-warmup/figs/` 下)
    * 不上传临时文件(凡是自动生成的文件和临时文件请不要上传)
    * git log 言之有物(不强制，请不要 git commit -m 'commit 1'，git commit -m 'sdfsdf'，每次 commit 请提交有用的 comment 信息)
* 提交方式：
  * 代码提交：本次实验需要在希冀课程平台上发布的作业Lab2-代码提交提交自己仓库的 gitlab 链接（注：由于平台限制，请提交http协议格式的仓库链接。例：学号为 PB011001 的同学，Lab2 的实验仓库地址为`http://202.38.79.174/PB011001/2022fall-compiler_cminus.git`），我们会收集最后一次提交的评测分数，作为最终代码得分。
  * 报告提交：将 Reports/2-ir-gen-warmup/README.md 导出成 pdf 文件单独提交到Lab2-报告提交。
  * 提交异常：如果遇到在平台上提交异常的问题，请通过邮件联系助教，助教将收取截止日期之前，学生在 gitlab 仓库最近一次 commit 内容进行评测。
* 评分标准: 本次实验的测试样例较为简单，所以为了拿高分请注意 report.md。
  * 1.3节 `.ll` 运行结果正确(一个5分，共20分)
  * 2.3节 `.cpp` 运行结果正确(一个10分，共40分)
  * `report.md` (40分)
  * 禁止执行恶意代码，违者本次实验0分处理
* 迟交规定
  * `Soft Deadline` : 2021/10/23 23:59:59 (北京标准时间，UTC+8)
  * `Hard Deadline` : 2021/10/30 23:59:59 (北京标准时间，UTC+8)
  * 迟交需要邮件通知TA:
    * 邮箱: wch0925@mail.ustc.edu.cn
    * 邮件主题: lab2迟交-学号
    * 内容: 包括迟交原因、最后版本commitID、迟交时间等
  * 迟交分数
    * x为迟交天数(对于 `Soft Deadline` 而言)， grade 满分10
      ``` bash
      final_grade = grade, x = 0
      final_grade = grade * (0.9)^x, 0 < x <= 7
      final_grade = 0, x > 7 # 这一条严格执行,请对自己负责
      ```
* 关于抄袭和雷同  
  经过助教和老师判定属于作业抄袭或雷同情况，所有参与方一律零分，不接受任何解释和反驳。  

如有任何问题，欢迎在论坛提意见进行批判指正。
