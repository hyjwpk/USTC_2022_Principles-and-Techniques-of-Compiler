#ifndef SYSYC_MODULE_H
#define SYSYC_MODULE_H

#include "Function.h"
#include "GlobalVariable.h"
#include "Instruction.h"
#include "Type.h"
#include "Value.h"

#include <list>
#include <llvm/ADT/ilist.h>
#include <llvm/ADT/ilist_node.h>
#include <map>
#include <memory>
#include <string>

class GlobalVariable;
class Function;
class Module {
  public:
    explicit Module(std::string name);
    ~Module();

    Type *get_void_type();
    Type *get_label_type();
    IntegerType *get_int1_type();
    IntegerType *get_int32_type();
    PointerType *get_int32_ptr_type();
    FloatType *get_float_type();
    PointerType *get_float_ptr_type();

    PointerType *get_pointer_type(Type *contained);
    ArrayType *get_array_type(Type *contained, unsigned num_elements);
    FunctionType *get_function_type(Type *retty, std::vector<Type *> &args);

    void add_function(Function *f);
    llvm::ilist<Function> &get_functions();
    void add_global_variable(GlobalVariable *g);
    llvm::ilist<GlobalVariable> &get_global_variable();
    std::string get_instr_op_name(Instruction::OpID instr) { return instr_id2string_[instr]; }
    void set_print_name();
    std::string print();

  private:
    llvm::ilist<GlobalVariable> global_list_;                  // The Global Variables in the module
    llvm::ilist<Function> function_list_;                      // The Functions in the module
    std::map<std::string, Value *> value_sym_;                 // Symbol table for values
    std::map<Instruction::OpID, std::string> instr_id2string_; // Instruction from opid to string

    std::string module_name_;      // Human readable identifier for the module
    std::string source_file_name_; // Original source file name for module, for test and debug

  private:
    std::unique_ptr<IntegerType> int1_ty_;
    std::unique_ptr<IntegerType> int32_ty_;
    std::unique_ptr<Type> label_ty_;
    std::unique_ptr<Type> void_ty_;
    std::unique_ptr<FloatType> float32_ty_;
    std::map<Type *, std::unique_ptr<PointerType>> pointer_map_;
    std::map<std::pair<Type *, int>, std::unique_ptr<ArrayType>> array_map_;
    std::map<std::pair<Type *, std::vector<Type *>>, std::unique_ptr<FunctionType>> function_map_;
};

#endif // SYSYC_MODULE_H
