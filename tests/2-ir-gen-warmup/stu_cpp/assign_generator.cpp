#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Module.h"
#include "Type.h"

#include <iostream>
#include <memory>

#ifdef DEBUG // 用于调试信息,大家可以在编译过程中通过" -DDEBUG"来开启这一选项
#define DEBUG_OUTPUT std::cout << __LINE__ << std::endl; // 输出行号的简单示例
#else
#define DEBUG_OUTPUT
#endif

#define CONST_INT(num) ConstantInt::get(num, module)

#define CONST_FP(num) ConstantFP::get(num, module) // 得到常数值的表示,方便后面多次用到

int main() {
  auto module = new Module("Cminus code");  // module name是什么无关紧要
  auto builder = new IRBuilder(nullptr, module);
  Type *Int32Type = Type::get_int32_type(module);
  auto *arrayType = ArrayType::get(Int32Type, 10);

  // main函数
  auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                  "main", module);
  auto bb = BasicBlock::create(module, "entry", mainFun);
  // BasicBlock的名字在生成中无所谓,但是可以方便阅读
  builder->set_insert_point(bb);
  
  auto aAlloca = builder->create_alloca(arrayType);
  auto a0GEP = builder->create_gep(aAlloca, {CONST_INT(0), CONST_INT(0)});
  auto a1GEP = builder->create_gep(aAlloca, {CONST_INT(0), CONST_INT(1)});
  builder->create_store(CONST_INT(10), a0GEP);
  auto a0Load = builder->create_load(a0GEP);
  auto mul = builder->create_imul(a0Load, CONST_INT(2));
  builder->create_store(mul, a1GEP);
  auto a1Load = builder->create_load(a1GEP);
  builder->create_ret(a1Load);

  std::cout << module->print();
  delete module;
  return 0;
}
