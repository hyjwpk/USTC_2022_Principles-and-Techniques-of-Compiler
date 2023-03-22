#include "Mem2Reg.hpp"

#include "IRBuilder.h"

#include <memory>

// 判断是否是全局变量地址
#define IS_GLOBAL_VARIABLE(l_val) dynamic_cast<GlobalVariable *>(l_val)
// 判断是否是 getelementptr 指令
#define IS_GEP_INSTR(l_val) dynamic_cast<GetElementPtrInst *>(l_val)

std::map<Value *, std::vector<Value *>> var_val_stack; // 全局变量初值提前存入栈中

void Mem2Reg::run() {
    // 创建支配树分析 Pass 的实例
    dominators_ = std::make_unique<Dominators>(m_);
    // 建立支配树
    dominators_->run();
    // 以函数为单元遍历实现 Mem2Reg 算法
    for (auto &f : m_->get_functions()) {
        func_ = &f;
        if (func_->get_basic_blocks().size() >= 1) {
            // 对应伪代码中 phi 指令插入的阶段
            generate_phi();
            // 对应伪代码中重命名阶段
            re_name(func_->get_entry_block());
        }
        // 移除冗余的局部变量的分配空间
        remove_alloca();
    }
}

void Mem2Reg::generate_phi() {
    // global_live_var_name 是全局名字集合，以 alloca 出的局部变量来统计。
    // 步骤一：找到活跃在多个 block 的全局名字集合，以及它们所属的 bb 块
    std::set<Value *> global_live_var_name;
    std::map<Value *, std::set<BasicBlock *>> live_var_2blocks;
    for (auto &bb1 : func_->get_basic_blocks()) {
        auto bb = &bb1;
        std::set<Value *> var_is_killed;
        for (auto &instr1 : bb->get_instructions()) {
            auto instr = &instr1;
            if (instr->is_store()) {
                // store i32 a, i32 *b
                // a is r_val, b is l_val
                auto r_val = static_cast<StoreInst *>(instr)->get_rval();
                auto l_val = static_cast<StoreInst *>(instr)->get_lval();

                if (!IS_GLOBAL_VARIABLE(l_val) && !IS_GEP_INSTR(l_val)) {
                    global_live_var_name.insert(l_val);
                    live_var_2blocks[l_val].insert(bb);
                }
            }
        }
    }

    // 步骤二：从支配树获取支配边界信息，并在对应位置插入 phi 指令
    std::map<std::pair<BasicBlock *, Value *>, bool> bb_has_var_phi; // bb has phi for var
    for (auto var : global_live_var_name) {
        std::vector<BasicBlock *> work_list;
        work_list.assign(live_var_2blocks[var].begin(), live_var_2blocks[var].end());
        for (int i = 0; i < work_list.size(); i++) {
            auto bb = work_list[i];
            for (auto bb_dominance_frontier_bb : dominators_->get_dominance_frontier(bb)) {
                if (bb_has_var_phi.find({bb_dominance_frontier_bb, var}) == bb_has_var_phi.end()) {
                    // generate phi for bb_dominance_frontier_bb & add bb_dominance_frontier_bb to work list
                    auto phi =
                        PhiInst::create_phi(var->get_type()->get_pointer_element_type(), bb_dominance_frontier_bb);
                    phi->set_lval(var);
                    bb_dominance_frontier_bb->add_instr_begin(phi);
                    work_list.push_back(bb_dominance_frontier_bb);
                    bb_has_var_phi[{bb_dominance_frontier_bb, var}] = true;
                }
            }
        }
    }
}

void Mem2Reg::re_name(BasicBlock *bb) {
    std::vector<Instruction *> wait_delete;

    // 步骤三：将 phi 指令作为 lval 的最新定值，lval 即是为局部变量 alloca 出的地址空间
    for (auto &instr1 : bb->get_instructions()) {
        auto instr = &instr1;
        if (instr->is_phi()) {
            auto l_val = static_cast<PhiInst *>(instr)->get_lval();
            var_val_stack[l_val].push_back(instr);
        }
    }

    for (auto &instr1 : bb->get_instructions()) {
        auto instr = &instr1;
        // 步骤四：用 lval 最新的定值替代对应的load指令
        if (instr->is_load()) {
            auto l_val = static_cast<LoadInst *>(instr)->get_lval();

            if (!IS_GLOBAL_VARIABLE(l_val) && !IS_GEP_INSTR(l_val)) {
                if (var_val_stack.find(l_val) != var_val_stack.end()) {
                    // 此处指令替换会维护 UD 链与 DU 链
                    instr->replace_all_use_with(var_val_stack[l_val].back());
                    wait_delete.push_back(instr);
                }
            }
        }
        // 步骤五：将 store 指令的 rval，也即被存入内存的值，作为 lval 的最新定值
        if (instr->is_store()) {
            auto l_val = static_cast<StoreInst *>(instr)->get_lval();
            auto r_val = static_cast<StoreInst *>(instr)->get_rval();

            if (!IS_GLOBAL_VARIABLE(l_val) && !IS_GEP_INSTR(l_val)) {
                var_val_stack[l_val].push_back(r_val);
                wait_delete.push_back(instr);
            }
        }
    }

    // 步骤六：为 lval 对应的 phi 指令参数补充完整
    for (auto succ_bb : bb->get_succ_basic_blocks()) {
        for (auto &instr1 : succ_bb->get_instructions()) {
            auto instr = &instr1;
            if (instr->is_phi()) {
                auto l_val = static_cast<PhiInst *>(instr)->get_lval();
                if (var_val_stack.find(l_val) != var_val_stack.end()) {
                    assert(var_val_stack[l_val].size() != 0);
                    static_cast<PhiInst *>(instr)->add_phi_pair_operand(var_val_stack[l_val].back(), bb);
                }
                // 对于 phi 参数只有一个前驱定值的情况，将会输出 [ undef, bb ] 的参数格式
            }
        }
    }

    // 步骤七：对 bb 在支配树上的所有后继节点，递归执行 re_name 操作
    for (auto dom_succ_bb : dominators_->get_dom_tree_succ_blocks(bb)) {
        re_name(dom_succ_bb);
    }

    // 步骤八：pop出 lval 的最新定值
    for (auto &instr1 : bb->get_instructions()) {
        auto instr = &instr1;

        if (instr->is_store()) {
            auto l_val = static_cast<StoreInst *>(instr)->get_lval();
            if (!IS_GLOBAL_VARIABLE(l_val) && !IS_GEP_INSTR(l_val)) {
                var_val_stack[l_val].pop_back();
            }
        } else if (instr->is_phi()) {
            auto l_val = static_cast<PhiInst *>(instr)->get_lval();
            if (var_val_stack.find(l_val) != var_val_stack.end()) {
                var_val_stack[l_val].pop_back();
            }
        }
    }

    // 清除冗余的指令
    for (auto instr : wait_delete) {
        bb->erase_instr(instr);
    }
}

void Mem2Reg::remove_alloca() {
    for (auto &bb1 : func_->get_basic_blocks()) {
        auto bb = &bb1;
        std::vector<Instruction *> wait_delete;
        for (auto &instr1 : bb->get_instructions()) {
            auto instr = &instr1;
            auto is_alloca = dynamic_cast<AllocaInst *>(instr);
            if (is_alloca) {
                bool is_int = is_alloca->get_type()->get_pointer_element_type()->is_integer_type();
                bool is_float = is_alloca->get_type()->get_pointer_element_type()->is_float_type();
                if (is_int || is_float) {
                    wait_delete.push_back(instr);
                }
            }
        }
        for (auto instr : wait_delete) {
            bb->erase_instr(instr);
        }
    }
}
