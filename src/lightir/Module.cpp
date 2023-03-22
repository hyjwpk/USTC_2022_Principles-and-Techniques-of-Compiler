#include "Module.h"

#include "Function.h"
#include "GlobalVariable.h"

#include <memory>

Module::Module(std::string name) : module_name_(name) {
    void_ty_ = std::make_unique<Type>(Type::VoidTyID, this);
    label_ty_ = std::make_unique<Type>(Type::LabelTyID, this);
    int1_ty_ = std::make_unique<IntegerType>(1, this);
    int32_ty_ = std::make_unique<IntegerType>(32, this);
    float32_ty_ = std::make_unique<FloatType>(this);
    // init instr_id2string
    instr_id2string_.insert({Instruction::ret, "ret"});
    instr_id2string_.insert({Instruction::br, "br"});

    instr_id2string_.insert({Instruction::add, "add"});
    instr_id2string_.insert({Instruction::sub, "sub"});
    instr_id2string_.insert({Instruction::mul, "mul"});
    instr_id2string_.insert({Instruction::sdiv, "sdiv"});

    instr_id2string_.insert({Instruction::fadd, "fadd"});
    instr_id2string_.insert({Instruction::fsub, "fsub"});
    instr_id2string_.insert({Instruction::fmul, "fmul"});
    instr_id2string_.insert({Instruction::fdiv, "fdiv"});

    instr_id2string_.insert({Instruction::alloca, "alloca"});
    instr_id2string_.insert({Instruction::load, "load"});
    instr_id2string_.insert({Instruction::store, "store"});
    instr_id2string_.insert({Instruction::cmp, "icmp"});
    instr_id2string_.insert({Instruction::fcmp, "fcmp"});
    instr_id2string_.insert({Instruction::phi, "phi"});
    instr_id2string_.insert({Instruction::call, "call"});
    instr_id2string_.insert({Instruction::getelementptr, "getelementptr"});
    instr_id2string_.insert({Instruction::zext, "zext"});
    instr_id2string_.insert({Instruction::sitofp, "sitofp"});
    instr_id2string_.insert({Instruction::fptosi, "fptosi"});
}

Module::~Module() {}

Type *Module::get_void_type() { return void_ty_.get(); }

Type *Module::get_label_type() { return label_ty_.get(); }

IntegerType *Module::get_int1_type() { return int1_ty_.get(); }

IntegerType *Module::get_int32_type() { return int32_ty_.get(); }

PointerType *Module::get_pointer_type(Type *contained) {
    if (pointer_map_.find(contained) == pointer_map_.end()) {
        pointer_map_[contained] = std::make_unique<PointerType>(contained);
    }
    return pointer_map_[contained].get();
}

ArrayType *Module::get_array_type(Type *contained, unsigned num_elements) {
    if (array_map_.find({contained, num_elements}) == array_map_.end()) {
        array_map_[{contained, num_elements}] = std::make_unique<ArrayType>(contained, num_elements);
    }
    return array_map_[{contained, num_elements}].get();
}
FunctionType *Module::get_function_type(Type *retty, std::vector<Type *> &args) {
    if (not function_map_.count({retty, args})) {
        function_map_[{retty, args}] = std::make_unique<FunctionType>(retty, args);
    }
    return function_map_[{retty, args}].get();
}

PointerType *Module::get_int32_ptr_type() { return get_pointer_type(int32_ty_.get()); }

FloatType *Module::get_float_type() { return float32_ty_.get(); }

PointerType *Module::get_float_ptr_type() { return get_pointer_type(float32_ty_.get()); }

void Module::add_function(Function *f) { function_list_.push_back(f); }
llvm::ilist<Function> &Module::get_functions() { return function_list_; }
void Module::add_global_variable(GlobalVariable *g) { global_list_.push_back(g); }
llvm::ilist<GlobalVariable> &Module::get_global_variable() { return global_list_; }

void Module::set_print_name() {
    for (auto &func : this->get_functions()) {
        func.set_instr_name();
    }
    return;
}

std::string Module::print() {
    std::string module_ir;
    for (auto &global_val : this->global_list_) {
        module_ir += global_val.print();
        module_ir += "\n";
    }
    for (auto &func : this->function_list_) {
        module_ir += func.print();
        module_ir += "\n";
    }
    return module_ir;
}
