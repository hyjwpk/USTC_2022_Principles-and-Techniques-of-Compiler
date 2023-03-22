# Lab4.1 实验报告

## 实验要求

本次实验要求理解IR的优化过程，学习如何将lab3 生成的伪 SSA IR转化为SSA IR，并在这个过程中移除冗余指令。需要学习其中涉及的支配性以及$\phi$函数等概念，并阅读实验框架中的代码，了解附件中的构造静态单赋值的伪代码算法是如何具体实现的，以及如何进行支配树的构建和分析。

## 思考题
### Mem2reg
1.请简述概念：支配性、严格支配性、直接支配性、支配边界。

​	支配性：若从初始结点起，每条到达b的路径都要经过a，a是b的支配节点。节点间的支配关系被称为支配性。

​	严格支配性：当且仅当a是b的支配节点且a不等于b的时候a严格支配b。

​	直接支配性：在支配a的节点集合中，最接近a的那个是a的直接支配节点，在支配者树中，是a所在的节点的父节点。

​	支配边界：如果节点a支配节点b的一个前驱节点，但a并不严格支配b，那么b在a的支配边界中。

2.`phi`节点是SSA的关键特征，请简述`phi`节点的概念，以及引入`phi`节点的理由。

​	`phi`节点是用以将不同路径的静态单赋值名调和为一个名字的函数，形式为$x \leftarrow  \phi (x,x)$

​	在SSA中，所有赋值指令都是对不同名字的变量的赋值，在将非SSA转换为SSA时，会把同一个变量重命名为不同的名字，但在引用处只能使用一个定义，因此需要引入`phi`节点调和同一变量的不同的静态单赋值名。

3.观察下面给出的`cminus`程序对应的 LLVM IR，与**开启**`Mem2Reg`生成的LLVM IR对比，每条`load`, `store`指令发生了变化吗？变化或者没变化的原因是什么？请分类解释。

- 变化：删除了func中的 `store i32 %arg0, i32* %op1`     删除了func中的`%op2 = load i32, i32* %op1`

  原因：函数的参数可以直接使用，不用再存储到局部变量再取值

- 变化：删除了func中的`store i32 0, i32* %op1`，删除了main中的 `store i32 2333, i32* %op1` `%op6 = load i32, i32* %op1`

  原因：常量传递

- 变化：修改func中的`%op8 = load i32, i32* %op1`为`%op9 = phi i32 [ %arg0, %label_entry ], [ 0, %label6 ]`

  原因：引入`phi`函数调和同一变量的不同的静态单赋值名

- 未变化：main中的`store i32 1, i32* @globVar`，`%op8 = load i32, i32* @globVar`

  原因：`@globVar`为全局变量

- 未变化：main中的`store i32 999, i32* %op5`

  原因：修改的为数组元素

4.指出放置phi节点的代码，并解释是如何使用支配树的信息的。（需要给出代码中的成员变量或成员函数名称）

​	放置phi节点的代码如下，在`void Mem2Reg::generate_phi()`中

```c++
                    auto phi =
                        PhiInst::create_phi(var->get_type()->get_pointer_element_type(), bb_dominance_frontier_bb);
                    phi->set_lval(var);
                    bb_dominance_frontier_bb->add_instr_begin(phi);
```

​	使用`dominators_->get_dominance_frontier(bb)`获取先前生成的`dominators_`中支配树中bb节点所对应的支配边界DF(bb)，在代码中的变量为`bb_dominance_frontier_bb`

5.算法是如何选择`value`(变量最新的值)来替换`load`指令的？（描述清楚对应变量与维护该变量的位置）

​	算法维护了变量`std::map<Value *, std::vector<Value *>> var_val_stack`，为每一个变量维护一个`std::vector<Value *>`，并用，并用`std::map`实现变量和`std::vector`的关系

​	在每个基本块，将 `phi` 指令作为 `lval` 的最新定值，并将其加入value对应的vector中，在对应的load指令处在value对应的vector尾部的值即为对应的value最新的值。

### 代码阅读总结

在本次实验中学习了支配性以及$\phi$函数等概念，以及构造静态单赋值形式的方法和伪代码，阅读了实验框架中将lab3 产生的 伪 SSR IR转换为SSA IR的实现，对数据流分析有了更深入的理解。

代码中的`Dominators.cpp`实现了支配树的生成与遍历，节点的Dom、Idom、DF获取。

代码中的`Mem2Reg.cpp`实现了利用支配树中的信息进行`phi`函数的生成以及重命名。

### 实验反馈 （可选 不会评分）

无
