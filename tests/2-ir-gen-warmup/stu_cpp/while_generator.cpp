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

  // main函数
  auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                  "main", module);
  auto bb = BasicBlock::create(module, "entry", mainFun);
  auto trueBB = BasicBlock::create(module, "trueBB", mainFun);    // true分支
  auto falseBB = BasicBlock::create(module, "falseBB", mainFun);  // false分支
  // BasicBlock的名字在生成中无所谓,但是可以方便阅读
  builder->set_insert_point(bb);
  
  auto aAlloca = builder->create_alloca(Int32Type);
  auto iAlloca = builder->create_alloca(Int32Type);
  builder->create_store(CONST_INT(10), aAlloca);
  builder->create_store(CONST_INT(0), iAlloca);
  auto iLoad = builder->create_load(iAlloca);
  auto icmp = builder->create_icmp_lt(iLoad, CONST_INT(10));
  auto br = builder->create_cond_br(icmp, trueBB, falseBB);  // 条件BR

  builder->set_insert_point(trueBB);
  iLoad = builder->create_load(iAlloca);
  auto iadd = builder->create_iadd(iLoad,CONST_INT(1));
  builder->create_store(iadd, iAlloca);
  auto aLoad = builder->create_load(aAlloca);
  iLoad = builder->create_load(iAlloca);
  iadd = builder->create_iadd(aLoad,iLoad);
  builder->create_store(iadd, aAlloca);
  icmp = builder->create_icmp_lt(iLoad, CONST_INT(10));
  br = builder->create_cond_br(icmp, trueBB, falseBB);  // 条件BR

  builder->set_insert_point(falseBB);
  aLoad = builder->create_load(aAlloca);
  builder->create_ret(aLoad);
  

  std::cout << module->print();
  delete module;
  return 0;
}
