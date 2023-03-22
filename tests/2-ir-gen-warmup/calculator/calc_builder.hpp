#ifndef _CALC_VISITOR_HPP_
#define _CALC_VISITOR_HPP_
#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Module.h"
#include "Type.h"
#include "calc_ast.hpp"
class CalcBuilder: public CalcASTVisitor {
public:
    std::unique_ptr<Module> build(CalcAST &ast);
private:
    virtual void visit(CalcASTInput &) override final;
    virtual void visit(CalcASTNum &) override final;
    virtual void visit(CalcASTExpression &) override final;
    virtual void visit(CalcASTTerm &) override final;

    IRBuilder *builder;
    Value *val;
    Type *TyInt32;
    std::unique_ptr<Module> module;
};
#endif
