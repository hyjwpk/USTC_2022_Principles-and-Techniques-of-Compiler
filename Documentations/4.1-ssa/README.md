# Lab4.1 实验文档
- [Lab4.1 实验文档](#lab41-实验文档)
  - [0. 前言](#0-前言)
  - [1. 实验内容](#1-实验内容)
    - [1.1 阅读相关材料与代码](#11-阅读相关材料与代码)
      - [1.1.1 SSA IR 材料阅读](#111-ssa-ir-材料阅读)
      - [1.1.2 Mem2Reg Pass 代码阅读](#112-mem2reg-pass-代码阅读)
          - [Pass 概念](#pass-概念)
          - [UD 链(Use-Define Chain)与 DU 链(Definition-Use Chain)](#ud-链use-define-chain与-du-链definition-use-chain)
          - [Mem2Reg](#mem2reg)
    - [1.2 注册及运行 Mem2Reg Pass](#12-注册及运行-mem2reg-pass)
      - [注册 Pass](#注册-pass)
      - [运行 Pass](#运行-pass)
    - [1.3 实验任务](#13-实验任务)
      - [思考题](#思考题)
  - [2. 提交要求](#2-提交要求)
    - [2.1 目录结构](#21-目录结构)
    - [2.2 提交要求和评分标准](#22-提交要求和评分标准)

## 0. 前言

在 lab3 中，我们通过访问者模式遍历抽象语法树，实现了一个中间代码自动生成工具。在 lab4，我们将对 lab3 生成的 IR 进行优化。本次实验是一个预热实验，为下一阶段编写优化方案提供必要的基础知识：SSA 格式的 IR ，以及优化 Pass 的概念。

## 1. 实验内容

### 1.1 阅读相关材料与代码

#### 1.1.1 SSA IR 材料阅读
Static single-assignment form（静态单赋值形式，通常缩写为SSA形式）是 IR 的一个属性，它要求每个变量在使用前被精确赋值一次并被定义。对原有变量的多次赋值会被分割成不同*版本*，这样每个定义都有自己的版本。SSA 形式下，[UD 链](#ud-链use-define-chain与-du-链definition-use-chain)是显式的，并且只含有一个元素。

观察下面的 `cminus` 程序，局部变量 `a` 经历了多次赋值。

```c
int main(void) {
    int a;
    a = 1 + 2;
    a = a * 4;
    return 0;
}
```

如下 IR 由 lab3 的中间代码生成器得出。虽然该段 IR 中的虚拟寄存器仅被赋值一次，属于 SSA 格式，但该 `cminus` 程序对局部变量`a`的多次赋值，在 IR 中仍以多次冗余的访存体现，存在不必要的 `alloca\load\store` 指令。这些指令会严重影响程序性能，所以我们需要移除这些冗余指令。

```asm
define i32 @main() {
label_entry:
  %op0 = alloca i32
  %op1 = add i32 1, 2
  store i32 %op1, i32* %op0
  %op2 = load i32, i32* %op0
  %op3 = mul i32 %op2, 4
  store i32 %op3, i32* %op0
  ret i32 0
}
```

我们将冗余的 load，store 删除，可以得到如下 SSA IR 格式：

```
define i32 @main() {
label_entry:
  %op1 = add i32 1, 2
  %op3 = mul i32 %op1, 4
  ret i32 0
}
```

我们将利用 Mem2Reg Pass 将对变量的冗余`load/store`指令删除。其中 Mem2Reg Pass 参考的算法详见[附件](./ssa.pdf)。

**Note:**

1. 更详细的 SSA 格式的细节请仔细阅读[附件](./ssa.pdf)。
2. 补充附件中的概念：
   - 支配性：在入口节点为 b0 的流图中，当且仅当 bi 位于从 b0 到 bj 的每条路径上时，结点 bi 支配结点 bj。
   - Dom(bi) 集合：该集合包含了支配 bi 的所有结点的名字。
3. 我们的实验设计从编译器前端分离了构造 SSA 过程，使用 `alloca` 来分配局部变量，保留了局部变量的访存操作，用 Mem2Reg Pass 来实现构造 SSA 的算法，这是保持了与 LLVM 一致的策略。

#### 1.1.2 Mem2Reg Pass 代码阅读

在[1.1.1](#111-ssa-ir-材料阅读) 我们已经简单介绍了 Mem2Reg Pass 的相关功能，接下来我们将介绍关于 Pass 优化的基础概念。

###### Pass 概念

Pass 是编译器中优化方案的基本单元，它对 IR 进行一定的变换，从而提高运行时程序性能，比如上文的 Mem2Reg Pass。在我们的框架中， 可以将`Pass`理解为一个函数，`Pass` 会接受一个 `module` 作为参数，在运行时`Pass`将遍历整个`module`，对`module`内的指令和 bb 上做一些变换来实现优化。`module`内指令与 bb 的结构层次可以参考[LightIR核心类介绍.md](../common/LightIR.md)。

###### UD 链(Use-Define Chain)与 DU 链(Definition-Use Chain)

Pass 在对指令进行替换与修改时，需要查找相关变量的定义和使用。在这里，我们引入UD 链（Use-Define Chain）与 DU 链（Definition-Use Chain）的概念。UD链描述了一个变量的使用（use）和其所有定义（def），DU链描述了一个变量的所有定义（def）和该定义能到达的所有使用（use）。在我们的框架中，UD 链和 DU 链的实现可以阅读`LightIR`中的[LightIR核心类介绍.md](../common/LightIR.md)中的User类的`operands_`成员，Value类的`use_list_`成员。

###### Mem2Reg

我们依据[附件](./ssa.pdf)中的 SSA 构造算法实现了Mem2Reg Pass，可以将 Lab3 中自动化生成的伪 SSA IR 转换成 SSA IR，代码相关的四个文件分别是

-  `src/optimization/Dominators.cpp`
-  `src/optimization/Mem2Reg.cpp`
-  `include/optimization/Dominators.hpp`
-  `include/optimization/Mem2Reg.hpp`

请对照[附件](./ssa.pdf)算法伪代码，仔细阅读上述代码

### 1.2 注册及运行 Mem2Reg Pass

#### 注册 Pass

本次实验使用了由 C++ 编写的 `LightIR` 来在 IR 层面完成优化化简，在`include/optimization/PassManager.hpp`中，定义了一个用于管理 Pass 的类`PassManager`。它的作用是注册与运行 Pass 。它提供了以下接口：

```cpp
PassManager pm(module.get())
pm.add_Pass<Mem2Reg>(emit)    // 注册Pass，emit为true时打印优化后的IR
pm.run()    // 按照注册的顺序运行Pass的run()函数
```

#### 运行 Pass

```sh
mkdir build && cd build
cmake ..
make -j
make install
```

为了便于大家进行实验，助教对之前的`cminusfc`增加了选项，用来选择是否开启某种优化，通过`[-mem2reg]`开关来控制优化 Pass 的使用，当需要对 `.cminus` 文件测试时，可以这样使用：

```bash
./cminusfc [-mem2reg] <input-file>
```

### 1.3 实验任务

阅读相关材料与代码，结合自己运行 Mem2Reg Pass 的体验，在实验报告中回答以下思考题。

#### 思考题

1. 请简述概念：支配性、严格支配性、直接支配性、支配边界。

2. `phi`节点是SSA的关键特征，请简述`phi`节点的概念，以及引入`phi`节点的理由。

3. 观察下面给出的`cminus`程序对应的 LLVM IR，与**开启**`Mem2Reg`生成的LLVM IR对比，每条`load`, `store`指令发生了变化吗？变化或者没变化的原因是什么？请分类解释。

   ```c
   int globVar;
   int func(int x){
       if(x > 0){
           x = 0;
       }
       return x;
   }
   int main(void){
       int arr[10];
       int b;
       globVar = 1;
       arr[5] = 999;
       b = 2333;
       func(b);
       func(globVar);
       return 0;
   }
   ```

   before `Mem2Reg`：

   ```asm
   @globVar = global i32 zeroinitializer
   declare void @neg_idx_except()
   define i32 @func(i32 %arg0) {
   label_entry:
     %op1 = alloca i32
     store i32 %arg0, i32* %op1
     %op2 = load i32, i32* %op1
     %op3 = icmp sgt i32 %op2, 0
     %op4 = zext i1 %op3 to i32
     %op5 = icmp ne i32 %op4, 0
     br i1 %op5, label %label6, label %label7
   label6:                                                ; preds = %label_entry
     store i32 0, i32* %op1
     br label %label7
   label7:                                                ; preds = %label_entry, %label6
     %op8 = load i32, i32* %op1
     ret i32 %op8
   }
   define i32 @main() {
   label_entry:
     %op0 = alloca [10 x i32]
     %op1 = alloca i32
     store i32 1, i32* @globVar
     %op2 = icmp slt i32 5, 0
     br i1 %op2, label %label3, label %label4
   label3:                                                ; preds = %label_entry
     call void @neg_idx_except()
     ret i32 0
   label4:                                                ; preds = %label_entry
     %op5 = getelementptr [10 x i32], [10 x i32]* %op0, i32 0, i32 5
     store i32 999, i32* %op5
     store i32 2333, i32* %op1
     %op6 = load i32, i32* %op1
     %op7 = call i32 @func(i32 %op6)
     %op8 = load i32, i32* @globVar
     %op9 = call i32 @func(i32 %op8)
     ret i32 0
   }
   ```

   After `Mem2Reg`：

   ```asm
   @globVar = global i32 zeroinitializer
   declare void @neg_idx_except()
   define i32 @func(i32 %arg0) {
   label_entry:
     %op3 = icmp sgt i32 %arg0, 0
     %op4 = zext i1 %op3 to i32
     %op5 = icmp ne i32 %op4, 0
     br i1 %op5, label %label6, label %label7
   label6:                                                ; preds = %label_entry
     br label %label7
   label7:                                                ; preds = %label_entry, %label6
     %op9 = phi i32 [ %arg0, %label_entry ], [ 0, %label6 ]
     ret i32 %op9
   }
   define i32 @main() {
   label_entry:
     %op0 = alloca [10 x i32]
     store i32 1, i32* @globVar
     %op2 = icmp slt i32 5, 0
     br i1 %op2, label %label3, label %label4
   label3:                                                ; preds = %label_entry
     call void @neg_idx_except()
     ret i32 0
   label4:                                                ; preds = %label_entry
     %op5 = getelementptr [10 x i32], [10 x i32]* %op0, i32 0, i32 5
     store i32 999, i32* %op5
     %op7 = call i32 @func(i32 2333)
     %op8 = load i32, i32* @globVar
     %op9 = call i32 @func(i32 %op8)
     ret i32 0
   }
   ```

4. 指出放置phi节点的代码，并解释是如何使用支配树的信息的。（需要给出代码中的成员变量或成员函数名称）

5. 算法是如何选择`value`(变量最新的值)来替换`load`指令的？（描述清楚对应变量与维护该变量的位置）



## 2. 提交要求

### 2.1 目录结构

与实验相关的文件如下

```
.
├── CMakeLists.txt
├── Documentations
│   ├── ...
│   ├── common
│   |   ├── LightIR.md                    LightIR 相关文档
│   |   ├── logging.md                    logging 工具相关文档
│   |   └── cminusf.md                    cminus-f 的语法和语义文档
│   └── 4.1-IR-opt
│       ├── ssa.pdf                       SSA 格式简介及构造算法材料
│       └── README.md                     Lab4.1 实验说明（你在这里）
├── include
│   ├── ...
│   └── optimization/*				 	  Mem2Reg.h & Dominators.h
├── Reports
│   ├── ...
│   └── 4.1-IR-opt
│       └── report.md                     lab4.1 实验阅读部分报告 					<--- 修改并提交
└── src
    ├── ...
    └── optimization
        ├── Mem2Reg.cpp                   Mem2Reg.cpp
        └── Dominators.cpp                Dominators.cpp
```
### 2.2 提交要求和评分标准

* 提交要求

  本次实验，只需要提交实验报告。

  * 在 `Reports/4.1-IR-opt/report.md` 中撰写实验报告，并在希冀平台对应提交 pdf 格式实验报告

* 评分标准:
  * 由助教阅读实验报告并根据思考题回答正误给分。本次实验在 Lab4 实验中占40分。

* 迟交规定
  * `Soft Deadline` :2022/11/27 23:59:59 (北京标准时间，UTC+8)

  * `Hard Deadline`：2022/12/04 23:59:59 (北京标准时间，UTC+8)

  * 迟交需要邮件通知 TA :
    * 邮箱:
    chen16614@mail.ustc.edu.cn 抄送 farmerzhang1@mail.ustc.edu.cn
    * 邮件主题: lab4-1迟交-学号
    * 内容: 包括迟交原因、最后版本 `commitID`、迟交时间等

  * 迟交分数
    * x为迟交天数(对于`Soft Deadline`而言)，grade为满分
      ``` bash
      final_grade = grade, x = 0
      final_grade = grade * (0.9)^x, 0 < x <= 7
      final_grade = 0, x > 7 # 这一条严格执行,请对自己负责
      ```

* 关于抄袭和雷同
  经过助教和老师判定属于实验抄袭或雷同情况，所有参与方一律零分，不接受任何解释和反驳（严禁对开源代码或者其他同学代码的直接搬运）。
  如有任何问题，欢迎提issue进行批判指正。
