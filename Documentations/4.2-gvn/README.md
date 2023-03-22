# Lab4 实验文档
- [Lab4 实验文档](#lab4-实验文档)
  - [0. 前言](#0-前言)
  - [1. GVN 基础知识](#1-gvn-基础知识)
    - [1.1 GVN 简介](#11-gvn-简介)
    - [1.2 GVN 相关概念](#12-gvn-相关概念)
      - [1. IR 假设](#1-ir-假设)
      - [2. Expression 概念](#2-expression-概念)
      - [3. 等价概念](#3-等价概念)
      - [4. Value Expression 概念](#4-value-expression-概念)
      - [5. Value phi-function 概念](#5-value-phi-function-概念)
      - [6. Partition](#6-partition)
      - [7. Join 操作](#7-join-操作)
      - [8. 变量等价与表达式等价](#8-变量等价与表达式等价)
      - [9. Transfer Function](#9-transfer-function)
  - [2. GVN 算法（论文中提供的伪代码）](#2-gvn-算法论文中提供的伪代码)
  - [3. 实验内容](#3-实验内容)
    - [3.1 GVN pass 实现内容要求](#31-gvn-pass-实现内容要求)
    - [3.2 GVN 辅助类](#32-gvn-辅助类)
    - [3.3 注册及运行 GVN Pass](#33-注册及运行-gvn-pass)
      - [注册 Pass](#注册-pass)
      - [运行 Pass](#运行-pass)
    - [3.3 自动测试](#33-自动测试)
    - [3.4 Tips](#34-tips)
  - [4. 提交要求](#4-提交要求)
    - [目录结构](#目录结构)
    - [提交要求和评分标准](#提交要求和评分标准)
## 0. 前言

在 Lab4.1 中，我们介绍了 SSA IR，并阐明了其优势。本次实验中我们需要在 SSA IR 基础上，实现一个基于数据流分析的冗余消除的优化 Pass : Global Value Numbering（全局值编号）。
## 1. GVN 基础知识

### 1.1 GVN 简介

GVN(Global Value Numbering) 全局值编号，是一个基于 SSA 格式的优化，通过建立变量，表达式到值编号的映射关系，从而检测到冗余计算代码并删除。本次实验采用的算法参考论文为：**[Detection of Redundant Expressions: A Complete and Polynomial-Time Algorithm in SSA](./gvn.pdf)** 在该论文中，提出了一种适合 SSA IR ，多项式算法复杂度的数据流分析方式的算法，能够实现对冗余代码完备的检测。本次实验中，我们将在`Light IR` 上实现该数据流分析算法，并根据数据流分析结果删掉冗余代码，达到优化目的。

### 1.2 GVN 相关概念

#### 1. IR 假设

将IR抽象为具有空的`entry block`与`exit block`的控制流图（CFG）。块内包含形式为`x = e`的赋值语句，其中`e`为表达式，`x`为变量。每个bb块最多可以有两个前驱，有两个前驱的bb块被称为`join block`。

#### 2. Expression 概念

一个`Expression`（表达式）可以是一个常量，一个变量，或者是$`x⊕y`$的形式，其中 x 与 y 是常量或者变量， ⊕ 代表一个通用的二元运算符。一个`Expression`也可以是 $`\phi_k(x,y)`$ 的形式，其中 x, y 是变量，并且 k 表示 join block。这种形式的 `Expression`被称作 `phi` 函数。基于 Lab4.1 对`phi`指令概念的理解，我们知道 phi 函数是出现在 join block 的起始处，而在实际的数据流分析过程中，我们会将 `phi` 指令作为 join block 前驱的赋值指令进行处理。

**注**：如下例子所示：形如$`x_3=\phi(x_{1},x_{2})`$的 phi 指令。

```asm
bb1:
    x1 = 1 + 1
    br bb3

bb2:
    x2 = 2 + 2
    br bb3

bb3:
    x3=phi(x1,x2)
    ...
```

为了实现 SSA 上 GVN 分析的完备性，我们在数据流分析中，将 phi 指令在对应前驱中转化为 $`x_3 = x_1`$, $`x_3 = x_2`$ 两条赋值语句处理。如下例子所示：

```asm
bb1:
    x1 = 1 + 1
    x3 = x1
    br bb3

bb2:
    x2 = 2 + 2
    x3 = x2
    br bb3

bb3:
	...
```

#### 3. 等价概念

两个表达式 `e1`,`e2` 如果被称作等价的，那么他们有着相同的运算符，并且操作数也是对应等价的。

#### 4. Value Expression 概念

一个 Value Expression（值表达式），$`vi⊕vj`$ 代表了值编号为vi，vj的两个等价类之间的运算。$`vi⊕vj=\{x⊕y;x\in C_i,C_i是一系列变量等价的等价类集合，值编号为v_i;y\in C_j,C_j是一系列变量等价的等价类的集合, 值编号是v_j\}`$

#### 5. Value phi-function 概念

与值表达式类似，value phi-function 视作为一系列等价的 phi-function 的抽象。在本次 GVN 实验中，需要扩展 phi-function 的理解，例如如下ir片段的例子：

```asm
bb1:
    y=x1+1
    br bb3

bb2:
    z=x2+1
    br bb3

bb3:
    x3=phi(x1,x2)
    w3=x3+1
```

对于 变量 w3 对应的 Expression x3+1，由于x3是phi function，因此可以通过[expression 概念](#2-expression-概念)中对phi function的处理探测出：w3，y在bb1中等价，w3，z在bb2中等价，因此可以将 w3 视作是变量 y 与变量 z 在 bb3 处的合并，可以用 $`\phi_{bb_3}(y,z)`$ 来表示w3对应的 value phi-function。

#### 6. Partition

一个 Partition （分区）由一系列等价类组成，一个等价类是由一个值编号，和一系列成员组成。每个成员可以是：变量，常量，值表达式。同时，一个等价类可能会关联一个 value-phi-function。

#### 7. Join 操作
Join 操作检测到达此处的所有路径共有的等价项。在 SSA 格式的 IR 中，变量只会被赋值一次，当程序点 p 支配 join block 时，在 p 点成立的等价关系，在 join block 处仍然成立。通过对 join block 的前驱 Partition 取交集，可以保留所有支配 join block 的程序点的等价关系。对于在每个路径的等价的探测，我们将在[2.GVN算法](#2-gvn-算法论文中提供的伪代码)中通过伪代码进行阐述。对于表达式的等价关系与变量等价关系的检测与判定，我们会分别阐述。

#### 8. 变量等价与表达式等价

例如如下代码段：

```asm
bb1:
    x1=1+1
    y1=2+2
    z1=x1+y1
    br bb3

bb2:
    x2=2+2
    y2=3+3
    z2=x2+y2
    br bb3

bb3:
    x3=phi(x1,x2)
    y3=phi(y1,y2)
    z3=phi(z1,z2)
    z4=x3+y3
```

在左分支bb1中，通过将 x3=phi(x1, x2)语句转换为前驱的复制语句 x3=x1 ，可以检测出，x3与x1在左分支路径上的等价性。同理也有x3与x2在右分支上的等价性。此为**变量等价**

同时，通过对z4=x3+y3的分析，可以看出，x3+y3 这个表达式，在bb1中与x1+y1等价，在bb2中与x2+y2等价，此为**表达式等价**，通过这个表达式等价的关系，可以分析出，z4与z1在bb1中变量等价，z4与z2在bb2中变量等价，因此可以建立z4的phi函数为phi(z1,z2)，进而可以导出z3与z4的**变量等价**，从而消除z3与z4的冗余。

#### 9. Transfer Function

TransferFunction 作用于一条语句s：x=e 上，接受 partition 记为 $`PIN_s`$ 并探测 e 与到达此处程序点所有 path 上是否有等价的 expression。TransferFunction 通过更新原有的等价类，或者创建新的等价类来在 partition $`PIN_s`$ 基础上修改并输出 partition 记为 $`POUT_s`$。在后面给出的伪代码可以看出，TransferFunction先在$`PIN_s`$检测是否存在 expression e 的 value expression，然后会继续检查 e 是否可以表达为不同表达式的合并。考虑如下例子（沿用了在变量等价中的例子）：

```asm
bb1:
    x1=1+1
    y1=2+2
    z1=x1+y1
    br bb3

bb2:
    x2=2+2
    y2=3+3
    z2=x2+y2
    br bb3

bb3:
    x3=phi(x1,x2)
    y3=phi(y1,y2)
    z3=phi(z1,z2)
    z4=x3+y3
```

在对`z4=x3+y3`这个表达式执行$`\text{TransferFunction}(x=e, PIN_s)`$:

其中，x 为 z4, e 为 `x3+y3`
$`PIN_{bb3}=\{\{v_1,x_1,1+1:\text{non-phi}\},\{v_2,y_1,2+2:\text{non-phi}\},\{v_3,z_1,x_1+y_1:\text{non-phi}\},\{v_4,x_2,2+2:\text{non-phi}\},\{v_5,y_2,3+3:\text{non-phi}\},\{v_6,z_2,x_2+y_2:\text{non-phi}\},\{v_7,x_3:\phi(x_1,x_2)\},\{v_8,y_3:\phi(y_1,y_2)\},\{v_9,z_3:\phi(z_1,z_2)\}\}`$

在 $`PIN_{bb3}`$ 中找不到 `x3+y3` 对应的 expression 表达式。

但是，对 `x3+y3` 进一步分析可以得到：因为`x3`和`y3`都是`phi`，`x3+y3`可以表示为 `bb1` 中 `x1+y1` 与 `bb2` 中 `x2+y2` 的两个表达式的合并，因此可以记为 phi(`x1+y1`, `x2+y2`)

接下来，可以检测到变量 z1 与 `x1+y1` expression 在`bb3`前驱`bb1`中的等价关系，变量 z2 与 `x2+y2` expression 在`bb3`前驱`bb2`中的等价关系。因此可以检测到 phi(`x1+y1`, `x2+y2`)与 phi（z1,z2）的等价关系。

进而得到 变量 z3 与 z4 是等价的，`x3 + y3` expression 的计算是冗余的。

注：这里的 $`PIN_{bb3}`$ 的例子仅仅是为了说明 TransferFunction 的职能和 partition 的结构式样，不代表最终迭代稳定后的结果。

## 2. GVN 算法（论文中提供的伪代码）

本小节附上了原论文中附的伪代码，伪代码中给出了五个主函数的逻辑，这里自顶向下梳理一下：

`detectEquivalences(G)`  包含了最主要的数据流迭代过程，传入参数 G 为抽象的数据流分析的 CFG 图结构，实现中可以以函数为分析的基本单位来执行此函数，迭代初始时，将各个 Partition 初始化为顶元`Top`， 定义为`Join(P, Top) = P = Join(Top, P)`

注：此函数中的 statement 的概念对应前述的一条语句

```clike
detectEquivalences(G)
    PIN1 = {} // “1” is the first statement in the program
    POUT1 = transferFunction(PIN1)
    for each statement s other than the first statement in the program
        POUTs = Top
    while changes to any POUT occur // i.e. changes in equivalences
        for each statement s other than the first statement in the program
            if s appears in block b that has two predecessors
            then
                PINs = Join(POUTs1, POUTs2) // s1 and s2 are last statements in respective predecessors
            else
                PINs = POUTs3    // s3 the statement just before s
            POUTs = transferFunction(PINs) // apply transferFunction on each statement in the block
```

`Join` 操作仅仅在某个语句有两个前驱时被触发，注意：在join操作执行之前，join block 中的 phi 语句会作为 copy statement 加入 join block 对应的两个前驱末尾处。细节处理方式见[expression 概念](#2-expression-概念)的注栏。

```clike
Join(P1, P2)
    P = {}
    for each pair of classes Ci ∈ P1 and Cj ∈ P2
        Ck = Intersect(Ci, Cj)
        P = P ∪ Ck // Ignore when Ck is empty
    return P

Intersect(Ci, Cj)
    Ck = Ci ∩ Cj // set intersection
    if Ck ！= {} and Ck does not have value number
    then
        Ck = Ck ∪ {vk} // vk is new value number
        Ck = (Ck − {vpf}) ∪ {φb(vi, vj)}
        // vpf is value φ-function in Ck, vi ∈ Ci, vj ∈ Cj, b is join block
    return Ck
```

`TransferFunction` 接受一个赋值语句 x=e（x是变量，e为表达式），与一个 partition $`PIN_s`$ 。其中 valueExpr 接受一个 expression 返回其对应的 value expression，由于 value expression 是由变量和操作符以及value expression 递归定义的，因此需要建立 e 中的操作数变量与其对应的 value expression 关联，在伪代码的基础上，需要为 valueExpr 函数添加 partition $`PIN_s`$ 作为参数。而 getVN 从一个 partition 中根据 e 来找到对应的编号。  

**注1**： 在 lightir 的设计中，普通语句的 x 与 e 存在同一个 Instruction 类中，参考[lightir核心类]()，Instruction 本身可以视作为 x，而通过 Instruction 成员运算符，和成员操作数可以抽象的组成 expression e。  

**注2**： phi 语句中需要经转换为 copy 语句，x 与 e 将不存在同一个 Instruction 中，请仔细思考区分一下。

```clike
TransferFunction(x = e, PINs)
    POUTs = PINs
    if x is in a class Ci in POUTs
        then Ci = Ci − {x}
    ve = valueExpr(e)
    vpf = valuePhiFunc(ve,PINs) // can be NULL
    if ve or vpf is in a class Ci in POUTs // ignore vpf when NULL
    then
        Ci = Ci ∪ {x, ve} // set union
    else
        POUTs = POUTs ∪ {vn, x, ve : vpf} // vn is new value number
    return POUTs

valuePhiFunc(ve,P)
    if ve is of the form φk(vi1, vj1) ⊕ φk(vi2, vj2)
    then
        // process left edge
        vi = getVN(POUTkl, vi1 ⊕ vi2)
        if vi is NULL
        then vi = valuePhiFunc(vi1 ⊕ vi2, POUTkl)
        // process right edge
        vj = getVN(POUTkr, vj1 ⊕ vj2)
        if vj is NULL
        then vj = valuePhiFunc(vj1 ⊕ vj2, POUTkr)

    if vi is not NULL and vj is not NULL
    then return φk(vi, vj)
    else return NULL
```

## 3. 实验内容

在本次实验中，请仔细阅读[3.1 GVN pass 实现内容要求](#31-gvn-pass-实现内容要求)，根据要求补全`src/optimization/GVN.cpp`，`include/optimization/GVN.h`中关于 GVN pass 数据流分析部分，同时需要在 `Reports/4-ir-opt/` 目录下撰写实验报告。**为了在评测中统一分析结果，请大家采用 lab3 的 TA-impl 分支提供的[答案](http://202.38.79.174/compiler_staff/2022fall-compiler_cminus/-/blob/TA-impl/src/cminusfc/cminusf_builder.cpp)来完成后续实验。**
### 3.1 GVN pass 实现内容要求

GVN 通过数据流分析来检测冗余的变量和计算，通过替换和死代码删除结合，实现优化效果。前述的例子中主要以二元运算语句来解释原理，且助教为大家提供了代码替换和删除的逻辑，除此之外，需要完成的方向有：

1. 对冗余指令的检测与消除包括（二元运算指令，cmp，gep，类型转换指令） 

2. 对纯函数的冗余调用消除（助教提供了纯函数分析，见[FuncInfo.h](../../include/optimization/FuncInfo.h)）

    该 Pass 的接口`is_pure_function`接受一个lightIR Function，判断该函数是否为纯函数；对于纯函数，如果其参数等价，对这个纯函数的不同调用也等价。

3. 常量传播

   在数据流分析的过程中，可以使用常量折叠来实现常量传播，从而将可在编译时计算的结果计算好，减少运行时开销。（助教提供了常量折叠类，在辅助类的介绍中）

我们会在测试样例中对这三点进行考察。

**case 考察范围说明：**在 Lab4-2 的公开 case 与隐藏 case 中，以下情况不会出现：

1. 不会对加法乘法运算的交换律，结合律造成的冗余进行考察。
2. 不会对访存指令之间的等价性进行考察。
3. 对于 value phi function 的冗余仅仅考察`phi(a+b, c+d)`与`phi(a,c)+phi(b,d)`之间的冗余。（其中 + 代表四则二元运算+ - * /，value phi function 上某一路径的常量传播可以不考虑，例如 case 不会涉及如下情况冗余`phi(0, c+d)`与 `phi(0,c)+phi(0,d)`）

**注**：我们为大家提供了冗余删除的函数 `GVN::replace_cc_members` ，只需要正确填充在 `GVN` 类中的 `pout` 变量，我们的替换逻辑将会根据每个 bb 的 pout 自行使用`CongruenceClass` 的 `leader_` 成员来替换在此 bb 内与其等价其他指令。

### 3.2 GVN 辅助类

在上述对 GVN 概念的介绍中，为了能让大家专注于核心数据流分析逻辑的实现，我们为大家提供了一些相应实现的辅助类，并在注释里解释了其相应用途，请注意与前述的 GVN 的抽象概念结合，理解其设计，并补充必要的类成员的实现。

`GVN.h`:

```c++
class ConstFolder; // 常量折叠类，用于折叠操作数都是常量的指令

// 该 namespace 下，包含了用于判断 expression 等价的结构，我们提供了 binary expression，phi expression，constant expression 的结构，请根据测试用例添加你需要的其他 expression 的结构，具体细节请据 GVN.h 结合代码与注释理解
namespace GVNExpression {
class Expression; // 所有 expression 类型的基类
class ConstantExpression; // 常量 expression 类型
class BinaryExpression; // 二元运算 expression 包括 + - * /
class PhiExpression; // phi expression 类型，表示不同路径的 expression 在此的合并
}

struct CongruenceClass; // 对应伪代码中等价类的概念，分析结果会根据此类 dump 至 json 文件中，代码替换与消除逻辑也根据此结构实现

class GVN; // GVN pass核心实现逻辑，除一些用的上的辅助函数外，重点补齐此处与伪代码对应的核心函数，其中run()函数是 pass 启动的入口，已经为大家补充好。
```

`GVN.cpp`:

```c++
namespace utils // 一些用于输出的函数，可方便调试，以及将结果 dump 到 json 文件中的方法
```

### 3.3 注册及运行 GVN Pass

#### 注册 Pass

本次实验使用了由 C++ 编写的 `LightIR` 来在 IR 层面完成优化化简，在`include/optimization/PassManager.hpp`中，定义了一个用于管理 Pass 的类`PassManager`。它的作用是注册与运行 Pass 。它提供了以下接口：

```cpp
PassManager pm(module.get())
pm.add_Pass<GVN>(emit, dump_json)    // 注册Pass，emit为true时打印优化后的IR
pm.run()    // 按照注册的顺序运行 Pass 的 run() 函数
```

#### 运行 Pass

```sh
mkdir build && cd build
cmake ..
make -j
make install
```

为了便于大家进行实验，助教对之前的`cminusfc`增加了选项，用来选择是否开启某种优化，通过`-mem2reg`开关来控制优化 Pass 的使用，当需要对 `.cminus` 文件测试时，可以这样使用：

```bash
./cminusfc [ -mem2reg ] [ -gvn [ -dump-json ] ] <input-file>
```

其中，gvn pass 需要在 mem2reg pass 运行后运行。

### 3.3 自动测试

助教贴心地为大家准备了自动测试脚本，它在 `tests/4-ir-opt` 目录下，使用方法如下：

```bash
python3 lab4_evals.py [ -gvn-analysis ] [ -gvn ]
```

该脚本可以在任意目录下运行

```bash
python3 tests/4-ir-opt/lab4_evals.py [ -gvn-analysis ] [ -gvn ]
```

其中 `-gvn-analysis` 对 GVN 分析结果的正确性进行判断，执行结果如下所示：

```bash
========== GlobalValueNumberAnalysis ==========
Compiling  -mem2reg -emit-llvm -gvn -dump-json
  0%|                       | 0/4 [00:00<?, ?it/s]
Compile bin.cminus  success
generate json bin.cminus  success
 25%|███▊           | 1/4 [00:00<00:00,  8.38it/s]
Compile loop3.cminus  success
generate json loop3.cminus  success
 50%|███████▌       | 2/4 [00:00<00:00,  7.21it/s]
Compile pure_func.cminus  success
generate json pure_func.cminus  success
 75%|███████████▎   | 3/4 [00:00<00:00,  4.08it/s]
Compile single_bb1.cminus  success
generate json single_bb1.cminus  success
100%|███████████████| 4/4 [00:00<00:00,  4.64it/s]
bin.cminus: 1.0
loop3.cminus: 1.0
pure_func.cminus: 1.0
single_bb1.cminus: 1.0
```

`-gvn` 会执行测试文件，比对你实现的 GVN 算法优化后的程序，与助教的基准程序的运行时间。

```bash
========== GlobalValueNumber ==========
Compiling  -mem2reg
100%|███████████████| 2/2 [00:00<00:00,  8.22it/s]
Evalution
100%|███████████████| 2/2 [00:17<00:00,  6.44s/it]
Compiling  -mem2reg -gvn
100%|███████████████| 2/2 [00:00<00:00,  5.67it/s]
Evalution
100%|███████████████| 2/2 [00:13<00:00,  4.36s/it]
Compiling baseline files
100%|███████████████| 2/2 [00:00<00:00, 13.70it/s]
Evalution
100%|███████████████| 2/2 [00:09<00:00,  3.24s/it]
testcase         before optimization     after optimization      baseline
constant.cminus                 0.70                    0.24              0.24
transpose.cminus                3.72                    3.04              3.03
root@3fd22a9ed627:/labs/2022fall-compiler_cminus-taversion/tests/4-ir-opt#
```

如果要增加样例，直接在样例目录中添加文件即可，命名参考目录下的其他文件。

### 3.4 Tips

1. 为了大家能够循序渐进地实现 GVN pass，助教给出一个实现上的规划：

   1. 使用 GVN pass 分析单一基本块的程序 case
      
        补充 `detectEquivalences` 函数（无需处理 Join ），在转移方程`transferFunction`中，为每个变量创建等价类。

   2. 使用 GVN pass 分析带有冗余计算的单一基本块的程序 case，并在分析过程中进行常量传播

        依照伪代码补全转移方程`transferFunction`，得到正确的等价类，将输出结构 dump 至 json 文件，与手动推算结果进行比对。

   3. 使用 GVN pass 分析带有选择语句的程序 case，处理phi语句之间的冗余

        完善 `detectEquivalences` ，实现合并两个前驱分区的合并算法 `Join`，和 `valuePhiFunc`，注意本次实验只考察`phi(a+b, c+d)`与`phi(a,c)+phi(b,d)`之间的冗余。

   4. 正确分析带有循环的程序

        完成了前三点后，你的 GVN 应该比较完整了，可以根据带循环的简单程序来调试。 

2. **Lab3** 的测试样例仍然可以用来测试优化正确性

3. 使用 logging 工具来打印调试信息，以及 GDB 等软件进行单步调试来检查错误的原因

    [logging](../common/logging.md) 是帮助大家打印调试信息的工具，如有需求可以阅读文档后进行使用

4. 你有可能会遇到智能指针相关的问题，详见[C++文档](https://zh.cppreference.com/w/cpp/memory/shared_ptr)，以及[段错误调试指北](../common/simple_cpp.md#debugging-seg-fault)

## 4. 提交要求

### 目录结构

```
.
├── CMakeLists.txt
├── Documentations
│   ├── ...
│   ├── common
│   |   ├── LightIR.md                  LightIR 相关文档
│   |   ├── logging.md                  logging 工具相关文档
│   |   └── cminusf.md                  cminus-f 的语法和语义文档
│   └── 4.2-gvn
│       ├── gvn.pdf                     本实验参考文献原文
│       └── README.md                   Lab4 实验文档说明（你在这里）
├── include                             实验所需的头文件
│   ├── ...
│   ├── optimization
│   │   ├── Mem2Reg.hpp                 Mem2Reg
│   │   ├── Dominators.hpp              支配树
│   │   └── GVN.h                       GVN								 	 <--- 修改并提交
│   ├── cminusf_builder.hpp
|   └── ast.hpp
├── Reports
│   ├── ...
│   └── 4.2-gvn
│       └── report.md            		lab4 本次实验报告模板					<--- 修改并提交
│  
├── src
│   ├── ...
│   └── optimization
│       ├── Mem2Reg.cpp                  Mem2Reg
│       ├── Dominators.cpp               支配树
│       └── GVN.cpp                      GVN								 <--- 修改并提交
│
└── tests
    ├── ...
    └── 4-ir-opt
        ├── testcases                    助教提供的测试样例
        └── lab4_test.py                 助教提供的测试脚本

```
### 提交要求和评分标准

* 提交要求
  本实验是个人实验，需要将代码上传至gitlab实验仓库，并在希冀评测平台上提交仓库链接。

  * 需要填补

    `include/optimization/GVN.h`, `src/optimization/GVN.cpp`，在 `Reports/4.2-gvn/report.md` 中撰写实验报告。

    - 实验报告内容包括
        实验要求、实验难点、实验设计、实验结果验证、思考题、实验反馈等，具体见 `Reports/4.2-gvn/report.md`

  * 本次实验不提供希冀平台在线测试通道，将在 soft ddl 时收取同学仓库 `master` 分支下`include/optimization/GVN.h`，`src/optimization/GVN.cpp` 文件，请同学们在ddl前将自己的最新分支push到 git 仓库上

  * 本次实验报告以 pdf 格式提交到希冀平台对应提交通道

* 评分标准: 实验完成分（总分 60 分）组成如下：
  * 实验报告 (5 分) 

    需要回答 Reports 目录下实验报告模板的思考题。

  * easy case（40分）

    助教提供了 4 个公开测试用例，并保留 4 个隐藏用例，根据每个用例 dump 出来的分析结果正误给分。json 文件判断的逻辑如下，仅对每个bb的 pout 进行判断。

    例如：
  
    ```json
    "pout": {
        "label_entry": [["%op0", ], ["%op1", ], ["%op2", ], ],
        "label3": [["%op0", ], ["%op1", ], ["%op2", ], ["%op4", "%op10", ], ["%op5", "%op9", ], ["%op6", "%op8", ], ],
        "label7": [["%op0", ], ["%op1", ], ["%op2", ], ["%op10", ], ["%op9", ], ["%op11", "%op8", ], ],
        "label12": [["%op0", ], ["%op1", ], ["%op2", ], ["%op13", "%op10", ], ["%op14", "%op9", ], ["%op15", "%op8", ], ],}
    ```
    对于分值为 x，n 个基本块的程序，每个 bb 分析结果为$`x/n`$分，某个bb的分析结果多或者少一个等价类，或有分析错误的等价类，该 bb 分析结果没有分值。


  * performance case（15分）

    助教提供了 2 个公开case，并保留 2 个隐藏用例。以及助教实现优化后的 baseline.ll ，优化效果按照如下方式给分（执行结果不正确则此项分数为0）

    ```
    对于每一个testcase:
    (before_optimization-after_optimization)/(before_optimization-baseline) > 0.8 得满分
    (before_optimization-after_optimization)/(before_optimization-baseline) > 0.5 得85%分数
    (before_optimization-after_optimization)/(before_optimization-baseline) > 0.2 得60%分数
    ```

  * 禁止执行恶意代码，违者本次实验0分处理

* 迟交规定

  * `Deadline`: 2023/01/14 23:59:59 (北京标准时间，UTC+8)

    本次实验不接受补交

* 关于抄袭和雷同
  经过助教和老师判定属于实验抄袭或雷同情况，所有参与方一律零分，不接受任何解释和反驳（严禁对开源代码或者其他同学代码的直接搬运）。
  如有任何问题，欢迎提issue进行批判指正。
