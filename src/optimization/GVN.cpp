#include "GVN.h"

#include "BasicBlock.h"
#include "Constant.h"
#include "DeadCode.h"
#include "FuncInfo.h"
#include "Function.h"
#include "Instruction.h"
#include "logging.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

using namespace GVNExpression;
using std::string_literals::operator""s;
using std::shared_ptr;

static auto get_const_int_value = [](Value *v) { return dynamic_cast<ConstantInt *>(v)->get_value(); };
static auto get_const_fp_value = [](Value *v) { return dynamic_cast<ConstantFP *>(v)->get_value(); };
// Constant Propagation helper, folders are done for you
Constant *ConstFolder::compute(Instruction *instr, Constant *value1, Constant *value2) {
    auto op = instr->get_instr_type();
    switch (op) {
    case Instruction::add: return ConstantInt::get(get_const_int_value(value1) + get_const_int_value(value2), module_);
    case Instruction::sub: return ConstantInt::get(get_const_int_value(value1) - get_const_int_value(value2), module_);
    case Instruction::mul: return ConstantInt::get(get_const_int_value(value1) * get_const_int_value(value2), module_);
    case Instruction::sdiv: return ConstantInt::get(get_const_int_value(value1) / get_const_int_value(value2), module_);
    case Instruction::fadd: return ConstantFP::get(get_const_fp_value(value1) + get_const_fp_value(value2), module_);
    case Instruction::fsub: return ConstantFP::get(get_const_fp_value(value1) - get_const_fp_value(value2), module_);
    case Instruction::fmul: return ConstantFP::get(get_const_fp_value(value1) * get_const_fp_value(value2), module_);
    case Instruction::fdiv: return ConstantFP::get(get_const_fp_value(value1) / get_const_fp_value(value2), module_);

    case Instruction::cmp:
        switch (dynamic_cast<CmpInst *>(instr)->get_cmp_op()) {
        case CmpInst::EQ: return ConstantInt::get(get_const_int_value(value1) == get_const_int_value(value2), module_);
        case CmpInst::NE: return ConstantInt::get(get_const_int_value(value1) != get_const_int_value(value2), module_);
        case CmpInst::GT: return ConstantInt::get(get_const_int_value(value1) > get_const_int_value(value2), module_);
        case CmpInst::GE: return ConstantInt::get(get_const_int_value(value1) >= get_const_int_value(value2), module_);
        case CmpInst::LT: return ConstantInt::get(get_const_int_value(value1) < get_const_int_value(value2), module_);
        case CmpInst::LE: return ConstantInt::get(get_const_int_value(value1) <= get_const_int_value(value2), module_);
        }
    case Instruction::fcmp:
        switch (dynamic_cast<FCmpInst *>(instr)->get_cmp_op()) {
        case FCmpInst::EQ: return ConstantInt::get(get_const_fp_value(value1) == get_const_fp_value(value2), module_);
        case FCmpInst::NE: return ConstantInt::get(get_const_fp_value(value1) != get_const_fp_value(value2), module_);
        case FCmpInst::GT: return ConstantInt::get(get_const_fp_value(value1) > get_const_fp_value(value2), module_);
        case FCmpInst::GE: return ConstantInt::get(get_const_fp_value(value1) >= get_const_fp_value(value2), module_);
        case FCmpInst::LT: return ConstantInt::get(get_const_fp_value(value1) < get_const_fp_value(value2), module_);
        case FCmpInst::LE: return ConstantInt::get(get_const_fp_value(value1) <= get_const_fp_value(value2), module_);
        }
    default: return nullptr;
    }
}

Constant *ConstFolder::compute(Instruction *instr, Constant *value1) {
    auto op = instr->get_instr_type();
    switch (op) {
    case Instruction::sitofp: return ConstantFP::get((float)get_const_int_value(value1), module_);
    case Instruction::fptosi: return ConstantInt::get((int)get_const_fp_value(value1), module_);
    case Instruction::zext: return ConstantInt::get((int)get_const_int_value(value1), module_);
    default: return nullptr;
    }
}

namespace utils {
static std::string print_congruence_class(const CongruenceClass &cc) {
    std::stringstream ss;
    if (cc.index_ == 0) {
        ss << "top class\n";
        return ss.str();
    }
    ss << "\nindex: " << cc.index_ << "\nleader: " << cc.leader_->print()
       << "\nvalue phi: " << (cc.value_phi_ ? cc.value_phi_->print() : "nullptr"s)
       << "\nvalue expr: " << (cc.value_expr_ ? cc.value_expr_->print() : "nullptr"s) << "\nmembers: {";
    for (auto &member : cc.members_)
        ss << member->print() << "; ";
    ss << "}\n";
    return ss.str();
}

static std::string dump_cc_json(const CongruenceClass &cc) {
    std::string json;
    json += "[";
    for (auto member : cc.members_) {
        if (auto c = dynamic_cast<Constant *>(member))
            json += member->print() + ", ";
        else
            json += "\"%" + member->get_name() + "\", ";
    }
    json += "]";
    return json;
}

static std::string dump_partition_json(const GVN::partitions &p) {
    std::string json;
    json += "[";
    for (auto cc : p)
        json += dump_cc_json(*cc) + ", ";
    json += "]";
    return json;
}

static std::string dump_bb2partition(const std::map<BasicBlock *, GVN::partitions> &map) {
    std::string json;
    json += "{";
    for (auto [bb, p] : map)
        json += "\"" + bb->get_name() + "\": " + dump_partition_json(p) + ",";
    json += "}";
    return json;
}

// logging utility for you
static void print_partitions(const GVN::partitions &p) {
    if (p.empty()) {
        LOG_DEBUG << "empty partitions\n";
        return;
    }
    std::string log;
    for (auto &cc : p)
        log += print_congruence_class(*cc);
    LOG_DEBUG << log; // please don't use std::cout
}
} // namespace utils

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

void GVN::copy_stmt(BasicBlock *bb) {
    if (!bb->get_succ_basic_blocks().empty()) {
        for (auto &suc : bb->get_succ_basic_blocks()) {
            for (auto &instr : suc->get_instructions()) {
                if (instr.is_phi()) {
                    Value *op0 = instr.get_operand(0);
                    Value *op1 = instr.get_operand(1);
                    Value *op2 = instr.get_operand(2);
                    Value *op3 = instr.get_operand(3);
                    if (bb == op1) {
                        bool flag = false;
                        for (auto cc = pout_[bb].begin(); cc != pout_[bb].end(); cc++) {
                            if ((*cc)->members_.find(&instr) != (*cc)->members_.end()) {
                                (*cc)->members_.erase(&instr);
                                if ((*cc)->members_.empty()) {
                                    cc = pout_[bb].erase(cc);
                                }
                            }
                            if ((*cc)->members_.find(op0) != (*cc)->members_.end() ||
                                (dynamic_cast<Constant *>(op0) && dynamic_cast<ConstantExpression *>((*cc)->value_expr_.get()) && op0 == dynamic_cast<ConstantExpression *>((*cc)->value_expr_.get())->c_)) {
                                flag = true;
                                (*cc)->members_.insert(&instr);
                            }
                        }
                        if (!flag) {
                            auto newcc = std::make_shared<CongruenceClass>(next_value_number_++);
                            newcc->members_.insert(&instr);
                            newcc->leader_ = op0;
                            newcc->value_expr_ = ConstantExpression::create(op0);
                            pout_[bb].emplace(newcc);
                        }
                    }
                    if (bb == op3) {
                        bool flag = false;
                        for (auto cc = pout_[bb].begin(); cc != pout_[bb].end(); cc++) {
                            if ((*cc)->members_.find(&instr) != (*cc)->members_.end()) {
                                (*cc)->members_.erase(&instr);
                                if ((*cc)->members_.empty()) {
                                    cc = pout_[bb].erase(cc);
                                }
                            }
                            if ((*cc)->members_.find(op2) != (*cc)->members_.end() ||
                                (dynamic_cast<Constant *>(op2) && dynamic_cast<ConstantExpression *>((*cc)->value_expr_.get()) && op2 == dynamic_cast<ConstantExpression *>((*cc)->value_expr_.get())->c_)) {
                                flag = true;
                                (*cc)->members_.insert(&instr);
                            }
                        }
                        if (!flag) {
                            auto newcc = std::make_shared<CongruenceClass>(next_value_number_++);
                            newcc->members_.insert(&instr);
                            newcc->leader_ = op2;
                            newcc->value_expr_ = ConstantExpression::create(op2);
                            pout_[bb].emplace(newcc);
                        }
                    }
                }
            }
        }
    }
}

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

// instruction of the form `x = e`, mostly x is just e (SSA), but for copy stmt x is a phi instruction in the
// successor. Phi values (not copy stmt) should be handled in detectEquiv
/// \param bb basic block in which the transfer function is called
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

std::shared_ptr<GVNExpression::Expression> GVN::getVN(const partitions &pout, shared_ptr<Expression> ve) {
    // TODO: return what?
    for (auto &cc : pout) {
        if (cc->value_expr_ && *cc->value_expr_ == *ve) {
            return ConstantExpression::create(cc->leader_);
        }
    }
    return nullptr;
}

void GVN::initPerFunction() {
    next_value_number_ = 1;
    pin_.clear();
    pout_.clear();
}

void GVN::replace_cc_members() {
    for (auto &[_bb, part] : pout_) {
        auto bb = _bb; // workaround: structured bindings can't be captured in C++17
        for (auto &cc : part) {
            if (cc->index_ == 0)
                continue;
            // if you are planning to do constant propagation, leaders should be set to constant at some point
            for (auto &member : cc->members_) {
                bool member_is_phi = dynamic_cast<PhiInst *>(member);
                bool value_phi = cc->value_phi_ != nullptr;
                if (member != cc->leader_ and (value_phi or !member_is_phi)) {
                    // only replace the members if users are in the same block as bb
                    member->replace_use_with_when(cc->leader_, [bb](User *user) {
                        if (auto instr = dynamic_cast<Instruction *>(user)) {
                            auto parent = instr->get_parent();
                            auto &bb_pre = parent->get_pre_basic_blocks();
                            if (instr->is_phi()) // as copy stmt, the phi belongs to this block
                                return std::find(bb_pre.begin(), bb_pre.end(), bb) != bb_pre.end();
                            else
                                return parent == bb;
                        }
                        return false;
                    });
                }
            }
        }
    }
    return;
}

// top-level function, done for you
void GVN::run() {
    std::ofstream gvn_json;
    if (dump_json_) {
        gvn_json.open("gvn.json", std::ios::out);
        gvn_json << "[";
    }

    folder_ = std::make_unique<ConstFolder>(m_);
    func_info_ = std::make_unique<FuncInfo>(m_);
    func_info_->run();
    dce_ = std::make_unique<DeadCode>(m_);
    dce_->run(); // let dce take care of some dead phis with undef

    for (auto &f : m_->get_functions()) {
        if (f.get_basic_blocks().empty())
            continue;
        func_ = &f;
        initPerFunction();
        LOG_INFO << "Processing " << f.get_name();
        detectEquivalences();
        LOG_INFO << "===============pin=========================\n";
        for (auto &[bb, part] : pin_) {
            LOG_INFO << "\n===============bb: " << bb->get_name() << "=========================\npartitionIn: ";
            for (auto &cc : part)
                LOG_INFO << utils::print_congruence_class(*cc);
        }
        LOG_INFO << "\n===============pout=========================\n";
        for (auto &[bb, part] : pout_) {
            LOG_INFO << "\n=====bb: " << bb->get_name() << "=====\npartitionOut: ";
            for (auto &cc : part)
                LOG_INFO << utils::print_congruence_class(*cc);
        }
        if (dump_json_) {
            gvn_json << "{\n\"function\": ";
            gvn_json << "\"" << f.get_name() << "\", ";
            gvn_json << "\n\"pout\": " << utils::dump_bb2partition(pout_);
            gvn_json << "},";
        }
        replace_cc_members(); // don't delete instructions, just replace them
    }
    dce_->run(); // let dce do that for us
    if (dump_json_)
        gvn_json << "]";
}

template <typename T>
static bool equiv_as(const Expression &lhs, const Expression &rhs) {
    // we use static_cast because we are very sure that both operands are actually T, not other types.
    return static_cast<const T *>(&lhs)->equiv(static_cast<const T *>(&rhs));
}

bool GVNExpression::operator==(const Expression &lhs, const Expression &rhs) {
    if (lhs.get_expr_type() != rhs.get_expr_type())
        return false;
    switch (lhs.get_expr_type()) {
    case Expression::e_constant: return equiv_as<ConstantExpression>(lhs, rhs);
    case Expression::e_bin: return equiv_as<BinaryExpression>(lhs, rhs);
    case Expression::e_phi: return equiv_as<PhiExpression>(lhs, rhs);
    case Expression::e_var: return equiv_as<VariableExpression>(lhs, rhs);
    }
}

bool GVNExpression::operator==(const shared_ptr<Expression> &lhs, const shared_ptr<Expression> &rhs) {
    if (lhs == nullptr and rhs == nullptr) // is the nullptr check necessary here?
        return true;
    return lhs and rhs and *lhs == *rhs;
}

GVN::partitions GVN::clone(const partitions &p) {
    partitions data;
    for (auto &cc : p) {
        data.insert(std::make_shared<CongruenceClass>(*cc));
    }
    return data;
}

bool operator==(const GVN::partitions &p1, const GVN::partitions &p2) {
    // TODO: how to compare partitions?
    if (p1.size() != p2.size())
        return false;
    for (auto &cc : p1) {
        bool find = false;
        for (auto &cc2 : p2) {
            if (*cc == *cc2) {
                find = true;
                break;
            }
        }
        if (!find) {
            return false;
        }
    }
    return true;
}

bool CongruenceClass::operator==(const CongruenceClass &other) const {
    // TODO: which fields need to be compared?
    return this->leader_ == other.leader_;
}
