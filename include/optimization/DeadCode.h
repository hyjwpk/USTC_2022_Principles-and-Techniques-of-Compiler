#pragma once
#include "FuncInfo.h"
#include "Function.h"
#include "Instruction.h"
#include "PassManager.hpp"
#include "logging.hpp"

/**
 * 死代码消除：参见 https://www.clear.rice.edu/comp512/Lectures/10Dead-Clean-SCCP.pdf
 **/
class DeadCode : public Pass {
  public:
    DeadCode(Module *m) : Pass(m), func_info(std::make_shared<FuncInfo>(m)) {}

    // 处理流程：两趟处理，mark 标记有用变量，sweep 删除无用指令
    void run() {
        bool changed{};
        func_info->run();
        do {
            changed = false;
            for (auto &F : m_->get_functions()) {
                auto func = &F;
                mark(func);
                changed |= sweep(func);
            }
        } while (changed);
        LOG_INFO << "dead code pass erased " << ins_count << " instructions";
    }

  private:
    std::shared_ptr<FuncInfo> func_info;
    void mark(Function *func) {
        work_list.clear();
        marked.clear();

        for (auto &bb : func->get_basic_blocks()) {
            for (auto &ins : bb.get_instructions()) {
                if (is_critical(&ins)) {
                    marked[&ins] = true;
                    work_list.push_back(&ins);
                }
            }
        }

        while (work_list.empty() == false) {
            auto now = work_list.front();
            work_list.pop_front();

            mark(now);
        }
    }
    void mark(Instruction *ins) {
        for (auto op : ins->get_operands()) {
            auto def = dynamic_cast<Instruction *>(op);
            if (def == nullptr)
                continue;
            if (marked[def])
                continue;
            if (def->get_function() != ins->get_function())
                continue;
            marked[def] = true;
            work_list.push_back(def);
        }
    }
    bool sweep(Function *func) {
        std::unordered_set<Instruction *> wait_del{};
        for (auto &bb : func->get_basic_blocks()) {
            for (auto it = bb.get_instructions().begin(); it != bb.get_instructions().end();) {
                if (marked[&*it]) {
                    ++it;
                    continue;
                } else {
                    auto tmp = &*it;
                    wait_del.insert(tmp);
                    it++;
                }
            }
        }
        for (auto inst : wait_del)
            inst->remove_use_of_ops();
        for (auto inst : wait_del)
            inst->get_parent()->get_instructions().erase(inst);
        ins_count += wait_del.size();
        return not wait_del.empty(); // changed
    }
    bool is_critical(Instruction *ins) {
        // 对纯函数的无用调用也可以在删除之列
        if (ins->is_call()) {
            auto call_inst = dynamic_cast<CallInst *>(ins);
            auto callee = dynamic_cast<Function *>(call_inst->get_operand(0));
            if (func_info->is_pure_function(callee))
                return false;
            return true;
        }
        if (ins->is_br() || ins->is_ret())
            return true;
        if (ins->is_store())
            return true;
        return false;
    }

    // 用以衡量死代码消除的性能
    int ins_count{0};
    std::deque<Instruction *> work_list{};
    std::unordered_map<Instruction *, bool> marked{};
};
