#ifndef SYSYC_MEM2REG_HPP
#define SYSYC_MEM2REG_HPP

#include "BasicBlock.h"
#include "Dominators.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Instruction.h"
#include "Module.h"
#include "PassManager.hpp"

#include <memory>

class Mem2Reg : public Pass {
  private:
    Function *func_;
    std::unique_ptr<Dominators> dominators_;

  public:
    Mem2Reg(Module *m) : Pass(m) {}
    ~Mem2Reg(){};
    void run() override;
    void generate_phi();
    void re_name(BasicBlock *bb);
    void remove_alloca();
};

#endif