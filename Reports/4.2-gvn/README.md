# Lab4.2 实验报告

## 实验要求

​	本次实验需要在提供的代码框架上完成gvn算法，通过gvn算法实现对冗余指令的替换和消除，在gvn算法执行的过程中附带进行纯函数的冗余调用消除与常量传播

## 实验难点

### 难点1

​	对顶元top的正确理解，顶元top应该使`Join(P, Top) = P = Join(Top, P)`，而不是为空

### 难点2

​	在 Intersect函数中正确生成phi函数

### 难点3

​	实现phi函数在对应的基本块的末尾的copy stmt

### 难点4

​	对phi函数中包含的常量的处理

### 难点5

​	对每次迭代前后pout变换的判断

### 难点6

​	值表达式的生成

### 难点7

​	转移函数中不同类型表达式的处理

### 难点8

​	phi函数的冗余检测

### 难点9

​	判断等价类是否相等的标准

### 难点10

​	在gvn算法中进行常量折叠

### 难点11

​	对纯函数的冗余判断

### 难点12

​	全局变量和函数参数的添加

## 实验设计

### `detectEquivalences(G)`的实现

伪代码如下

```c++
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

实现代码如下

```c++
void GVN::detectEquivalences() {
    bool changed = false;
    // initialize pout with top
    // iterate until converge
    partitions top;
    top.emplace(std::make_shared<CongruenceClass>(-1));
    for (auto &bb : func_->get_basic_blocks()) {
        if (bb.get_pre_basic_blocks().size() == 0) {
            Function *func = bb.get_parent();
            for (auto it = func->arg_begin(); it != func->arg_end(); it++) {
                Value *arg = *it;
                std::shared_ptr<CongruenceClass> newcc = std::make_shared<CongruenceClass>(next_value_number_++);
                newcc->leader_ = arg;
                newcc->members_.insert(arg);
                pin_[&bb].emplace(newcc);
            }
            for (auto &var : m_->get_global_variable()) {
                std::shared_ptr<CongruenceClass> newcc = std::make_shared<CongruenceClass>(next_value_number_++);
                newcc->leader_ = &var;
                newcc->members_.insert(&var);
                pin_[&bb].emplace(newcc);
            }
            pout_[&bb] = transferFunction(&bb, pin_[&bb]);
            copy_stmt(&bb);
        } else {
            pout_[&bb] = top;
        }
    }
    std::uint64_t cur_next_value_number_ = next_value_number_;
    do {
        changed = false;
        next_value_number_ = cur_next_value_number_;
        // see the pseudo code in documentation
        for (auto &bb : func_->get_basic_blocks()) { // you might need to visit the blocks in depth-first order
            // get PIN of bb by predecessor(s)
            // iterate through all instructions in the block
            // and the phi instruction in all the successors
            // check changes in pout
            auto pre_basic_blocks = bb.get_pre_basic_blocks();
            if (pre_basic_blocks.size() == 2) {
                pin_[&bb] = join(pout_[pre_basic_blocks.front()], pout_[pre_basic_blocks.back()]);
            } else if (pre_basic_blocks.size() == 1) {
                pin_[&bb] = clone(pout_[pre_basic_blocks.front()]);
            } else {
                continue;
            }
            partitions pre = clone(pout_[&bb]);
            pout_[&bb] = transferFunction(&bb, pin_[&bb]);
            copy_stmt(&bb);
            if (pre != pout_[&bb]) {
                changed = true;
            }
        }
    } while (changed);
}
```

- 通过一个特殊的值编号为-1的等价类标注顶元top
- 在起始块添加函数参数和全局变量
- 在`transferFunction`函数执行完成后寻找后继块，添加需要的copy stmt

### `Join(P1, P2)`的实现

伪代码如下

```c++
Join(P1, P2)
    P = {}
    for each pair of classes Ci ∈ P1 and Cj ∈ P2
        Ck = Intersect(Ci, Cj)
        P = P ∪ Ck // Ignore when Ck is empty
    return P
```

实现代码如下

```c++
GVN::partitions GVN::join(const partitions &P1, const partitions &P2) {
    // TODO: do intersection pair-wise
    GVN::partitions P = {};
    if ((*(*P1.begin())).index_ == (size_t)-1) {
        return P2;
    }
    if ((*(*P2.begin())).index_ == (size_t)-1) {
        return P1;
    }
    for (auto cc1 : P1) {
        for (auto cc2 : P2) {
            std::shared_ptr<CongruenceClass> Ck = intersect(cc1, cc2);
            if (Ck) {
                P.emplace(Ck);
            }
        }
    }
    return P;
}
```

- 对顶元top特殊判断

### `Intersect(Ci, Cj)`的实现

伪代码如下

```c++
Intersect(Ci, Cj)
    Ck = Ci ∩ Cj // set intersection
    if Ck ！= {} and Ck does not have value number
    then
        Ck = Ck ∪ {vk} // vk is new value number
        Ck = (Ck − {vpf}) ∪ {φb(vi, vj)}
        // vpf is value φ-function in Ck, vi ∈ Ci, vj ∈ Cj, b is join block
    return Ck
```

实现代码如下

```c++
std::shared_ptr<CongruenceClass> GVN::intersect(std::shared_ptr<CongruenceClass> Ci,
                                                std::shared_ptr<CongruenceClass> Cj) {
    // TODO:
    std::shared_ptr<CongruenceClass> newcc = std::make_shared<CongruenceClass>(0);
    if (Ci->index_ == Cj->index_) {
        newcc->index_ = Ci->index_;
    }
    if (Ci->leader_ == Cj->leader_) {
        newcc->leader_ = Ci->leader_;
    }
    if ((Ci->value_expr_ == Cj->value_expr_) ||
        (Ci->value_expr_ && Cj->value_expr_ && *Ci->value_expr_ == *Cj->value_expr_)) {
        newcc->value_expr_ = Ci->value_expr_;
    }
    if ((Ci->value_phi_ == Cj->value_phi_) ||
        (Ci->value_phi_ && Cj->value_phi_ && *Ci->value_phi_ == *Cj->value_phi_)) {
        newcc->value_phi_ = Ci->value_phi_;
    }
    for (auto &member : Ci->members_) {
        if (Cj->members_.find(member) != Cj->members_.end()) {
            newcc->members_.insert(member);
        }
    }
    if (newcc->members_.empty()) {
        return nullptr;
    } else {
        if (newcc->index_ == 0) {
            newcc->index_ = next_value_number_++;
            std::shared_ptr<Expression> lhs, rhs;
            lhs = ConstantExpression::create(Ci->leader_);
            rhs = ConstantExpression::create(Cj->leader_);
            if (newcc->leader_) {
                ;
            } else {
                newcc->leader_ = *newcc->members_.begin();
            }
            newcc->value_phi_ = PhiExpression::create(lhs, rhs);
        }
        return newcc;
    }
}
```

### `TransferFunction(x = e, PINs)`的实现

伪代码如下

```c++
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
```

实现代码如下

```c++
GVN::partitions GVN::transferFunction(BasicBlock *bb, partitions pin) {
    partitions pout = clone(pin);
    for (auto &instr : bb->get_instructions()) {
        if (instr.is_phi() || ((Value *)(&instr))->get_type()->is_void_type()) {
            continue;
        }
        for (auto cc = pout.begin() ; cc != pout.end(); cc++) {
            if ((*cc)->members_.find(&instr) != (*cc)->members_.end()) {
                (*cc)->members_.erase(&instr);
                if ((*cc)->members_.empty()) {
                    cc = pout.erase(cc);
                }
            }
        }
        shared_ptr<Expression> ve = valueExpr(pout, &instr);
        shared_ptr<PhiExpression> vpf = valuePhiFunc(bb, ve, pout);
        bool find_cc = false;
        if (ve != nullptr && !instr.is_load()) {
            for (auto &cc : pout) {
                if ((cc->value_expr_ && *cc->value_expr_ == *ve) || (vpf && cc->value_phi_ && *cc->value_phi_ == *vpf)) {
                    cc->members_.insert(&instr);
                    find_cc = true;
                    break;
                }
            }
        }
        if (find_cc == false) {
            std::shared_ptr<CongruenceClass> newcc = std::make_shared<CongruenceClass>(next_value_number_++);
            newcc->leader_ = &instr;
            if (instr.isBinary() || instr.is_cmp() || instr.is_fcmp()) {
                Value *op1 = instr.get_operand(0);
                Value *op2 = instr.get_operand(1);
                Constant *op1_const = nullptr, *op2_const = nullptr;
                op1_const = dynamic_cast<Constant *>(op1);
                op2_const = dynamic_cast<Constant *>(op2);
                for (auto &cc : pout) {
                    if (!op1_const && cc->members_.find(op1) != cc->members_.end()) {
                        op1_const = dynamic_cast<Constant *>(cc->leader_);
                    }
                    if (!op2_const && cc->members_.find(op2) != cc->members_.end()) {
                        op2_const = dynamic_cast<Constant *>(cc->leader_);
                    }
                }
                if (op1_const && op2_const) {
                    newcc->leader_ = folder_->compute(&instr, op1_const, op2_const);
                }
            } else if (instr.is_si2fp() || instr.is_fp2si() || instr.is_zext()) {
                Value *op = instr.get_operand(0);
                Constant *op_const = nullptr;
                op_const = dynamic_cast<Constant *>(op);
                for (auto &cc : pout) {
                    if (!op_const && cc->members_.find(op) != cc->members_.end()) {
                        op_const = dynamic_cast<Constant *>(cc->leader_);
                    }
                }
                if (op_const) {
                    newcc->leader_ = folder_->compute(&instr, op_const);
                }
            }
            newcc->value_expr_ = ve;
            newcc->members_.insert(&instr);
            pout.emplace(newcc);
        }
    }
    // TODO: get different ValueExpr by Instruction::OpID, modify pout
    return pout;
}
```

- 忽略phi语句、非赋值语句、alloca语句
- 在生成新的等价类时判断是否是常量表达式，对常量表达式进行折叠

### `valuePhiFunc(ve,P)的实现`

伪代码如下

```c++
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

实现代码如下

```c++
shared_ptr<PhiExpression> GVN::valuePhiFunc(BasicBlock *bb, shared_ptr<Expression> ve, partitions p) {
    // TODO:
    shared_ptr<GVNExpression::Expression> vi, vj = nullptr;
    shared_ptr<PhiExpression> vpf = nullptr;
    if (dynamic_cast<BinaryExpression *>(&*ve)) {
        BinaryExpression *ve_ = dynamic_cast<BinaryExpression *>(&*ve);
        if (ve_->op_ == Instruction::OpID::cmp || ve_->op_ == Instruction::OpID::fcmp) {
            return vpf;
        }
        ConstantExpression *lhs, *rhs;
        lhs = (ConstantExpression *)(&*ve_->lhs_);
        rhs = (ConstantExpression *)(&*ve_->rhs_);
        Value *lhs_value, *rhs_value;
        lhs_value = &*lhs->c_;
        rhs_value = &*rhs->c_;
        if (dynamic_cast<Constant *>(lhs_value) || dynamic_cast<Constant *>(rhs_value)) {
            return vpf;
        }
        shared_ptr<PhiExpression> lhs_phi, rhs_phi;
        for (auto &cc : p) {
            if (cc->value_phi_) {
                if (cc->leader_ == lhs_value) {
                    lhs_phi = cc->value_phi_;
                }
                if (cc->leader_ == rhs_value) {
                    rhs_phi = cc->value_phi_;
                }
            }
        }
        auto pre_basic_blocks = bb->get_pre_basic_blocks();
        if (pre_basic_blocks.size() != 2) {
            return vpf;
        }
        std::shared_ptr<BinaryExpression> pre_lhs, pre_rhs;
        if (lhs_phi != nullptr && rhs_phi != nullptr) {
            pre_lhs = BinaryExpression::create(ve_->op_, lhs_phi->lhs_, rhs_phi->lhs_);
            pre_rhs = BinaryExpression::create(ve_->op_, lhs_phi->rhs_, rhs_phi->rhs_);
        } else {
            return vpf;
        }
        vi = getVN(pout_[pre_basic_blocks.front()], pre_lhs);
        vj = getVN(pout_[pre_basic_blocks.back()], pre_rhs);
        if (vi == nullptr) {
            vi = valuePhiFunc(pre_basic_blocks.front(), pre_lhs, pout_[pre_basic_blocks.front()]);
            if (vi != nullptr) {
                for (auto &cc : pout_[pre_basic_blocks.front()]) {
                    if (cc->value_phi_ && *cc->value_phi_ == *vi) {
                        vi = ConstantExpression::create(cc->leader_);
                        break;
                    }
                }
            }
        }
        if (vj == nullptr) {
            vj = valuePhiFunc(pre_basic_blocks.back(), pre_rhs, pout_[pre_basic_blocks.back()]);
            if (vj != nullptr) {
                for (auto &cc : pout_[pre_basic_blocks.back()]) {
                    if (cc->value_phi_ && *cc->value_phi_ == *vj) {
                        vj = ConstantExpression::create(cc->leader_);
                        break;
                    }
                }
            }
        }
        if (vi != nullptr && vj != nullptr) {
            vpf = PhiExpression::create(vi, vj);
        }
    }
    return vpf;
}
```

- 只考察`phi(a+b, c+d)`与`phi(a,c)+phi(b,d)`之间的冗余
- 将原始表达式a+b进行拆解，转换为`phi(a,c)+phi(b,d)`，再寻找是否存在`a+b`和`c+d`，最后生成`phi(a+b, c+d)`

### `valueExpr(e)`的实现

实现代码如下

```c++
shared_ptr<Expression> GVN::valueExpr(partitions p, Instruction *instr) {
    // TODO
    if (instr->get_num_operand() > 2 || instr->is_call() || instr->is_gep() || instr->is_si2fp() || instr->is_fp2si() || instr->is_zext() || instr->is_alloca()) {
        int type = 0;
        int num = instr->get_num_operand();
        if (instr->is_gep()) {
            type = 0;
        } else if (instr->is_call()) {
            if (func_info_->is_pure_function(dynamic_cast<Function *>(instr->get_operand(0)))) {
                type = 1;
            } else {
                type = 2;
            }
        } else if (instr->is_si2fp()) {
            type = 3;
        } else if (instr->is_fp2si()) {
            type = 4;
        } else if (instr->is_zext()) {
            type = 5;
        } else if (instr->is_alloca()){
            type = 6;
        }
        std::vector<std::shared_ptr<Expression>> exprs;
        for (int i = 0; i < num; i++) {
            Value *operand = instr->get_operand(i);
            Value *op = nullptr;
            for (auto &cc : p) {
                if (cc->members_.find(operand) != cc->members_.end()) {
                    op = cc->leader_;
                }
            }
            if (op == nullptr) {
                op = operand;
            }
            std::shared_ptr<Expression> op_expr = nullptr;
            op_expr = ConstantExpression::create(op);
            exprs.push_back(op_expr);
        }
        return VariableExpression::create(exprs, type);
    } else if (instr->get_num_operand() == 2) {
        Value *op1 = instr->get_operand(0);
        Value *op2 = instr->get_operand(1);
        Value *lhs = nullptr, *rhs = nullptr;
        for (auto &cc : p) {
            if (cc->members_.find(op1) != cc->members_.end()) {
                lhs = cc->leader_;
            }
            if (cc->members_.find(op2) != cc->members_.end()) {
                rhs = cc->leader_;
            }
        }
        if (lhs == nullptr) {
            lhs = op1;
        }
        if (rhs == nullptr) {
            rhs = op2;
        }
        std::shared_ptr<Expression> lhs_expr = nullptr, rhs_expr = nullptr;
        lhs_expr = ConstantExpression::create(lhs);
        rhs_expr = ConstantExpression::create(rhs);
        std::shared_ptr<BinaryExpression> expr = BinaryExpression::create(instr->get_instr_type(), lhs_expr, rhs_expr);
        if (instr->is_cmp()) {
            expr->cmp = (BinaryExpression::CmpOp)(((CmpInst *)(instr))->get_cmp_op());
        }
        if (instr->is_fcmp()) {
            expr->cmp = (BinaryExpression::CmpOp)(((FCmpInst *)(instr))->get_cmp_op());
        }
        return expr;
    } else {
        return ConstantExpression::create(instr->get_operand(0));
    }
}
```

- 对非算数运算进行单独处理
- 编写了`VariableExpression`用于处理非算数运算
- 在`VariableExpression`用type标记纯函数
- 对比较运算，在`BinaryExpression`中添加了`cmp`用于记录类型

### `VariableExpression`类的实现

实现代码如下

```c++
class VariableExpression : public Expression {
public:
    static std::shared_ptr<VariableExpression> create(std::vector<std::shared_ptr<Expression>> op, int type) {
        return std::make_shared<VariableExpression>(op, type);
    }
    virtual std::string print() {
        std::string result = "(var ";
        for (auto &i : op_) {
            result += i->print() + " ";
        }
        result += ")";
        return result;
    }
    bool equiv(const VariableExpression *other) const {
        if (this == other) {
            return true;
        }
        if (op_.size() != other->op_.size()) {
            return false;
        }
        if (type_ != other->type_) {
            return false;
        }
        if (type_ == 2 || type_ == 6) {
            return false;
        }
        bool flag = true;
        for (size_t i = 0; i < op_.size(); i++) {
            if ((*op_[i]) == (*other->op_[i])) {
                flag = true;
            } else {
                flag = false;
                break;
            }
        }
        return flag;
    }
    VariableExpression(std::vector<std::shared_ptr<Expression>> op, int type) : Expression(e_var), op_(op), type_(type) {}
    std::vector<std::shared_ptr<Expression>> op_;
    int type_; // 0 getelementptr 1 pure func 2 not pure func 3 si2fp 4 fp2si 5 zext 6 alloca
};
```

- 用type来记录指令类型
- 在比较函数中对于非纯函数特殊判断

### 优化前后的IR对比

对以下c代码生成的IR进行优化

```c++
/* c and d are redundant, and also check for constant propagation */
int main(void) {
    int a;
    int b;
    int c;
    int d;
    if (input() > input()) {
        a = 33 + 33;
        b = 44 + 44;
        c = a + b;
    } else {
        a = 55 + 55;
        b = 66 + 66;
        c = a + b;
    }
    output(c);
    d = a + b;
    output(d);
}
```

gvn优化前的IR如下

```assembly
declare i32 @input()

declare void @output(i32)

declare void @outputFloat(float)

declare void @neg_idx_except()

define i32 @main() {
label_entry:
  %op0 = call i32 @input()
  %op1 = call i32 @input()
  %op2 = icmp sgt i32 %op0, %op1
  %op3 = zext i1 %op2 to i32
  %op4 = icmp ne i32 %op3, 0
  br i1 %op4, label %label5, label %label14
label5:                                                ; preds = %label_entry
  %op6 = add i32 33, 33
  %op7 = add i32 44, 44
  %op8 = add i32 %op6, %op7
  br label %label9
label9:                                                ; preds = %label5, %label14
  %op10 = phi i32 [ %op8, %label5 ], [ %op17, %label14 ]
  %op11 = phi i32 [ %op7, %label5 ], [ %op16, %label14 ]
  %op12 = phi i32 [ %op6, %label5 ], [ %op15, %label14 ]
  call void @output(i32 %op10)
  %op13 = add i32 %op12, %op11
  call void @output(i32 %op13)
  ret i32 0
label14:                                                ; preds = %label_entry
  %op15 = add i32 55, 55
  %op16 = add i32 66, 66
  %op17 = add i32 %op15, %op16
  br label %label9
}
```

gvn优化后的IR如下

```assembly
declare i32 @input()

declare void @output(i32)

declare void @outputFloat(float)

declare void @neg_idx_except()

define i32 @main() {
label_entry:
  %op0 = call i32 @input()
  %op1 = call i32 @input()
  %op2 = icmp sgt i32 %op0, %op1
  %op3 = zext i1 %op2 to i32
  %op4 = icmp ne i32 %op3, 0
  br i1 %op4, label %label5, label %label14
label5:                                                ; preds = %label_entry
  br label %label9
label9:                                                ; preds = %label5, %label14
  %op10 = phi i32 [ 154, %label5 ], [ 242, %label14 ]
  call void @output(i32 %op10)
  call void @output(i32 %op10)
  ret i32 0
label14:                                                ; preds = %label_entry
  br label %label9
}
```

- label5 的`%op6 = add i32 33, 33`和`%op7 = add i32 44, 44` label14的`%op15 = add i32 55, 55`和` %op16 = add i32 66, 66`是常量表达式，可以直接优化为常数，这四句变为死代码
- label5的`%op8 = add i32 %op6, %op7`和label14的`%op17 = add i32 %op15, %op16`的操作数都是常量表达式的结果，可以利用常量传播优化为常数，这两句变为死代码
- 利用`gvn`算法，发现`%op10 = phi i32 [ %op8, %label5 ], [ %op17, %label14 ]`与`%op13 = add i32 %op12, %op11`在同一个等价类中，`%op13 = add i32 %op12, %op11`和`%op12 = phi i32 [ %op6, %label5 ], [ %op15, %label14 ]`、`%op12 = phi i32 [ %op6, %label5 ], [ %op15, %label14 ]`变为死代码
- 删除死代码后得到优化后的IR

### 思考题

1.请简要分析你的算法复杂度

​		记程序中表达式的数量为n，则等价类的数量为$O(n)$。记程序中变量和常量的个数为v，则每个等价类的大小为$O(v)$，`join`函数的复杂度为$O(n^2v)$，记join函数执行的次数为j，则总复杂度为$O(n^2vj)$。`valueExpr`函数的复杂度为$O(n)$，`valuePhiFunc`函数的复杂度为$O(nj)$，则`TransferFunction`函数的复杂度为$O(n^2j)$，`detectEquivalences`函数的执行次数为$O(n)$，则整个算法的复杂度为$O(n^3vj)$

2.`std::shared_ptr`如果存在环形引用，则无法正确释放内存，你的 Expression 类是否存在 circular reference?

​	我的Expression类的引用结构是树状的，不存在循环引用。

3.尽管本次实验已经写了很多代码，但是在算法上和工程上仍然可以对 GVN 进行改进，请简述你的 GVN 实现可以改进的地方

​	对除了`phi(a+b, c+d)`与`phi(a,c)+phi(b,d)`之间的冗余的其他冗余的检测

​	按照特定顺序来对基本块进行迭代，减少迭代次数

​	在迭代时对没有依赖关系的部分并行进行

## 实验总结

​	在这次试验中实现了gvn算法，通过gvn算法实现对冗余指令的替换和消除，在gvn算法执行的过程中附带进行纯函数的冗余调用消除与常量传播，对gvn算法与数据流分析的过程有了更深的理解，同时学习了实验框架中c++的各种特性的使用。

## 实验反馈（可选 不会评分）

​	无


