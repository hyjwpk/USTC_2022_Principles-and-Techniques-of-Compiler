/*
 * 声明：本代码为 2020 秋 中国科大编译原理（李诚）课程实验参考实现。
 * 请不要以任何方式，将本代码上传到可以公开访问的站点或仓库
 */

#include "cminusf_builder.hpp"

#define CONST_FP(num) ConstantFP::get((float)num, module.get())
#define CONST_INT(num) ConstantInt::get(num, module.get())

// TODO: Global Variable Declarations
// You can define global variables here
// to store state. You can expand these
// definitions if you need to.

// function that is being built
Function *cur_fun = nullptr; //当前函数
Value *cur_value = nullptr;  //当前值
bool cur_lvalue = false;     //当前是否是左值

// types
Type *VOID_T;
Type *INT1_T;
Type *INT32_T;
Type *INT32PTR_T;
Type *FLOAT_T;
Type *FLOATPTR_T;

/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

void CminusfBuilder::visit(ASTProgram &node) {
    VOID_T = Type::get_void_type(module.get());
    INT1_T = Type::get_int1_type(module.get());
    INT32_T = Type::get_int32_type(module.get());
    INT32PTR_T = Type::get_int32_ptr_type(module.get());
    FLOAT_T = Type::get_float_type(module.get());
    FLOATPTR_T = Type::get_float_ptr_type(module.get());

    for (auto decl : node.declarations) {
        decl->accept(*this);
    }
}

void CminusfBuilder::visit(ASTNum &node) {
    //! TODO: This function is empty now.
    // Add some code here.
    // 生成常数
    if (node.type == TYPE_INT) {
        cur_value = CONST_INT(node.i_val);
    } else {
        cur_value = CONST_FP(node.f_val);
    }
}

void CminusfBuilder::visit(ASTVarDeclaration &node) {
    //! TODO: This function is empty now.
    // Add some code here.
    Value *varAlloca;
    Type *type;
    // 生成变量或数组
    if (node.num == nullptr) {
        if (node.type == TYPE_INT) {
            type = INT32_T;
        } else {
            type = FLOAT_T;
        }
    } else {
        if (node.type == TYPE_INT) {
            type = ArrayType::get(INT32_T, node.num->i_val);
        } else {
            type = ArrayType::get(FLOAT_T, node.num->i_val);
        }
    }
    // 是否全局变量
    if (scope.in_global()) {
        varAlloca = GlobalVariable::create(node.id, module.get(), type, false, ConstantZero::get(type, module.get()));
    } else {
        varAlloca = builder->create_alloca(type);
    }
    scope.push(node.id, varAlloca);
}

void CminusfBuilder::visit(ASTFunDeclaration &node) {
    FunctionType *fun_type;
    Type *ret_type;
    std::vector<Type *> param_types;
    if (node.type == TYPE_INT)
        ret_type = INT32_T;
    else if (node.type == TYPE_FLOAT)
        ret_type = FLOAT_T;
    else
        ret_type = VOID_T;

    for (auto &param : node.params) {
        //! TODO: Please accomplish param_types.
        // 若参数为数组类型生成指针
        if (param->type == TYPE_INT) {
            if (param->isarray) {
                param_types.push_back(INT32PTR_T);
            } else {
                param_types.push_back(INT32_T);
            }
        } else {
            if (param->isarray) {
                param_types.push_back(FLOATPTR_T);
            } else {
                param_types.push_back(FLOAT_T);
            }
        }
    }

    fun_type = FunctionType::get(ret_type, param_types);
    auto fun = Function::create(fun_type, node.id, module.get());
    scope.push(node.id, fun);
    cur_fun = fun;
    auto funBB = BasicBlock::create(module.get(), "entry", fun);
    builder->set_insert_point(funBB);
    scope.enter();
    std::vector<Value *> args;
    for (auto arg = fun->arg_begin(); arg != fun->arg_end(); arg++) {
        args.push_back(*arg);
    }
    for (int i = 0; i < node.params.size(); ++i) {
        //! TODO: You need to deal with params
        // and store them in the scope.
        Type *type;
        // 若参数为数组类型，生成存储指针的变量
        if (node.params[i]->type == TYPE_INT) {
            if (node.params[i]->isarray) {
                type = INT32PTR_T;
            } else {
                type = INT32_T;
            }
        } else {
            if (node.params[i]->isarray) {
                type = FLOATPTR_T;
            } else {
                type = FLOAT_T;
            }
        }
        auto arg_Alloca = builder->create_alloca(type);
        builder->create_store(args[i], arg_Alloca);
        scope.push(node.params[i]->id, arg_Alloca);
    }
    node.compound_stmt->accept(*this);
    if (builder->get_insert_block()->get_terminator() == nullptr) {
        if (cur_fun->get_return_type()->is_void_type())
            builder->create_void_ret();
        else if (cur_fun->get_return_type()->is_float_type())
            builder->create_ret(CONST_FP(0.));
        else
            builder->create_ret(CONST_INT(0));
    }
    scope.exit();
}

void CminusfBuilder::visit(ASTParam &node) {
    //! TODO: This function is empty now.
    // Add some code here.
}

void CminusfBuilder::visit(ASTCompoundStmt &node) {
    //! TODO: This function is not complete.
    // You may need to add some code here
    // to deal with complex statements.
    // 进入新的作用域
    scope.enter();
    for (auto &decl : node.local_declarations) {
        decl->accept(*this);
    }

    for (auto &stmt : node.statement_list) {
        stmt->accept(*this);
        if (builder->get_insert_block()->get_terminator() != nullptr)
            break;
    }
    // 退出作用域
    scope.exit();
}

void CminusfBuilder::visit(ASTExpressionStmt &node) {
    //! TODO: This function is empty now.
    // Add some code here.
    if (node.expression != nullptr) {
        node.expression->accept(*this);
    }
}

void CminusfBuilder::visit(ASTSelectionStmt &node) {
    //! TODO: This function is empty now.
    // Add some code here.
    auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
    auto falseBB = BasicBlock::create(module.get(), "", cur_fun);
    auto endBB = BasicBlock::create(module.get(), "", cur_fun);
    node.expression->accept(*this);
    if (cur_value->get_type()->is_integer_type()) {
        cur_value = builder->create_icmp_ne(cur_value, CONST_INT(0));
    } else {
        cur_value = builder->create_fcmp_ne(cur_value, CONST_FP(0));
    }
    builder->create_cond_br(cur_value, trueBB, falseBB);
    builder->set_insert_point(trueBB);
    scope.enter();
    node.if_statement->accept(*this);
    scope.exit();
    // 基本块没有终止时无条件跳转
    if (builder->get_insert_block()->get_terminator() == nullptr) {
        builder->create_br(endBB);
    }
    builder->set_insert_point(falseBB);
    if (node.else_statement != nullptr) {
        scope.enter();
        node.else_statement->accept(*this);
        scope.exit();
        // 基本块没有终止时无条件跳转
        if (builder->get_insert_block()->get_terminator() == nullptr) {
            builder->create_br(endBB);
        }
    } else {
        builder->create_br(endBB);
    }
    builder->set_insert_point(endBB);
}

void CminusfBuilder::visit(ASTIterationStmt &node) {
    //! TODO: This function is empty now.
    // Add some code here.
    auto testBB = BasicBlock::create(module.get(), "", cur_fun); //判断分支
    auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
    auto falseBB = BasicBlock::create(module.get(), "", cur_fun);
    builder->create_br(testBB);
    builder->set_insert_point(testBB);
    node.expression->accept(*this);
    cur_value = builder->create_icmp_ne(cur_value, CONST_INT(0));
    builder->create_cond_br(cur_value, trueBB, falseBB);
    builder->set_insert_point(trueBB);
    scope.enter();
    node.statement->accept(*this);
    scope.exit();
    // 基本块没有终止时无条件跳转
    if (builder->get_insert_block()->get_terminator() == nullptr) {
        builder->create_br(testBB);
    }
    builder->set_insert_point(falseBB);
}

void CminusfBuilder::visit(ASTReturnStmt &node) {
    if (node.expression == nullptr) {
        builder->create_void_ret();
    } else {
        //! TODO: The given code is incomplete.
        // You need to solve other return cases (e.g. return an integer).
        node.expression->accept(*this);
        // 返回值类型不同时转换类型
        if (cur_fun->get_function_type()->get_return_type() != cur_value->get_type()) {
            if (cur_value->get_type()->is_integer_type()) {
                cur_value = builder->create_sitofp(cur_value, FLOAT_T);
            } else {
                cur_value = builder->create_fptosi(cur_value, INT32_T);
            }
        }
        builder->create_ret(cur_value);
    }
}

void CminusfBuilder::visit(ASTVar &node) {
    //! TODO: This function is empty now.
    // Add some code here.
    cur_value = scope.find(node.id);
    if (node.expression == nullptr) {
        // 不返回左值时取值
        if (!cur_lvalue) {
            if (cur_value->get_type()->get_pointer_element_type()->is_array_type()) {
                cur_value = builder->create_gep(cur_value, {CONST_INT(0), CONST_INT(0)});
            } else {
                cur_value = builder->create_load(cur_value);
            }
        }
    } else {
        Value *var1 = cur_value;
        bool post_lvalue = cur_lvalue;
        cur_lvalue = false;
        node.expression->accept(*this);
        cur_lvalue = post_lvalue;
        Value *var2 = cur_value;
        if (var2->get_type()->is_float_type()) {
            var2 = builder->create_fptosi(var2, INT32_T);
        }
        auto trueBB = BasicBlock::create(module.get(), "", cur_fun);
        auto falseBB = BasicBlock::create(module.get(), "", cur_fun);
        auto endBB = BasicBlock::create(module.get(), "", cur_fun);
        Value *icmp = builder->create_icmp_ge(var2, CONST_INT(0));
        builder->create_cond_br(icmp, trueBB, falseBB);
        builder->set_insert_point(trueBB);
        // 根据指针类型生成新的指针
        if (var1->get_type()->get_pointer_element_type()->is_integer_type() || var1->get_type()->get_pointer_element_type()->is_float_type()) {
            cur_value = builder->create_gep(var1, {var2});
        } else if (var1->get_type()->get_pointer_element_type()->is_pointer_type()) {
            var1 = builder->create_load(var1);
            cur_value = builder->create_gep(var1, {var2});
        } else {
            cur_value = builder->create_gep(var1, {CONST_INT(0), var2});
        }
        // 不返回左值时取值
        if (!cur_lvalue) {
            cur_value = builder->create_load(cur_value);
        }
        builder->create_br(endBB);
        builder->set_insert_point(falseBB);
        builder->create_call(scope.find("neg_idx_except"), {});
        builder->create_br(endBB);
        builder->set_insert_point(endBB);
    }
}

void CminusfBuilder::visit(ASTAssignExpression &node) {
    //! TODO: This function is empty now.
    // Add some code here.
    // 赋值时需要左值
    cur_lvalue = true;
    node.var->accept(*this);
    cur_lvalue = false;
    Value *var1 = cur_value;
    node.expression->accept(*this);
    Value *var2 = cur_value;
    // 赋值两边类型不同时转换类型
    if (var1->get_type()->get_pointer_element_type() != var2->get_type()) {
        if (var2->get_type()->is_integer_type()) {
            cur_value = builder->create_sitofp(var2, FLOAT_T);
        } else {
            cur_value = builder->create_fptosi(var2, INT32_T);
        }
    }
    builder->create_store(cur_value, var1);
}

void CminusfBuilder::visit(ASTSimpleExpression &node) {
    //! TODO: This function is empty now.
    // Add some code here.
    if (node.additive_expression_r == nullptr) {
        node.additive_expression_l->accept(*this);
    } else {
        CminusType type = TYPE_INT;
        node.additive_expression_l->accept(*this);
        Value *var1 = cur_value;
        node.additive_expression_r->accept(*this);
        Value *var2 = cur_value;
        // 有浮点数时都转换为浮点数
        if (var1->get_type()->is_float_type() || cur_value->get_type()->is_float_type()) {
            type = TYPE_FLOAT;
            if (var1->get_type()->is_integer_type()) {
                var1 = builder->create_sitofp(var1, FLOAT_T);
            }
            if (var2->get_type()->is_integer_type()) {
                var2 = builder->create_sitofp(var2, FLOAT_T);
            }
        }
        switch (node.op) {
        case OP_LE:
            if (type == TYPE_INT) {
                cur_value = builder->create_icmp_le(var1, var2);
            } else {
                cur_value = builder->create_fcmp_le(var1, var2);
            }
            break;
        case OP_LT:
            if (type == TYPE_INT) {
                cur_value = builder->create_icmp_lt(var1, var2);
            } else {
                cur_value = builder->create_fcmp_lt(var1, var2);
            }
            break;
        case OP_GT:
            if (type == TYPE_INT) {
                cur_value = builder->create_icmp_gt(var1, var2);
            } else {
                cur_value = builder->create_fcmp_gt(var1, var2);
            }
            break;
        case OP_GE:
            if (type == TYPE_INT) {
                cur_value = builder->create_icmp_ge(var1, var2);
            } else {
                cur_value = builder->create_fcmp_ge(var1, var2);
            }
            break;
        case OP_EQ:
            if (type == TYPE_INT) {
                cur_value = builder->create_icmp_eq(var1, var2);
            } else {
                cur_value = builder->create_fcmp_eq(var1, var2);
            }
            break;
        case OP_NEQ:
            if (type == TYPE_INT) {
                cur_value = builder->create_icmp_ne(var1, var2);
            } else {
                cur_value = builder->create_fcmp_ne(var1, var2);
            }
            break;
        default:
            break;
        }
        cur_value = builder->create_zext(cur_value, INT32_T);
    }
}

void CminusfBuilder::visit(ASTAdditiveExpression &node) {
    //! TODO: This function is empty now.
    // Add some code here.
    if (node.additive_expression == nullptr) {
        node.term->accept(*this);
    } else {
        CminusType type = TYPE_INT;
        node.additive_expression->accept(*this);
        Value *var1 = cur_value;
        node.term->accept(*this);
        Value *var2 = cur_value;
        // 有浮点数时都转换为浮点数
        if (var1->get_type()->is_float_type() || cur_value->get_type()->is_float_type()) {
            type = TYPE_FLOAT;
            if (var1->get_type()->is_integer_type()) {
                var1 = builder->create_sitofp(var1, FLOAT_T);
            }
            if (var2->get_type()->is_integer_type()) {
                var2 = builder->create_sitofp(var2, FLOAT_T);
            }
        }
        if (node.op == OP_PLUS) {
            if (type == TYPE_INT) {
                cur_value = builder->create_iadd(var1, var2);
            } else {
                cur_value = builder->create_fadd(var1, var2);
            }
        } else {
            if (type == TYPE_INT) {
                cur_value = builder->create_isub(var1, var2);
            } else {
                cur_value = builder->create_fsub(var1, var2);
            }
        }
    }
}

void CminusfBuilder::visit(ASTTerm &node) {
    //! TODO: This function is empty now.
    // Add some code here.
    if (node.term == nullptr) {
        node.factor->accept(*this);
    } else {
        CminusType type = TYPE_INT;
        node.term->accept(*this);
        Value *var1 = cur_value;
        node.factor->accept(*this);
        Value *var2 = cur_value;
        // 有浮点数时都转换为浮点数
        if (var1->get_type()->is_float_type() || cur_value->get_type()->is_float_type()) {
            type = TYPE_FLOAT;
            if (var1->get_type()->is_integer_type()) {
                var1 = builder->create_sitofp(var1, FLOAT_T);
            }
            if (var2->get_type()->is_integer_type()) {
                var2 = builder->create_sitofp(var2, FLOAT_T);
            }
        }
        if (node.op == OP_MUL) {
            if (type == TYPE_INT) {
                cur_value = builder->create_imul(var1, var2);
            } else {
                cur_value = builder->create_fmul(var1, var2);
            }
        } else {
            if (type == TYPE_INT) {
                cur_value = builder->create_isdiv(var1, var2);
            } else {
                cur_value = builder->create_fdiv(var1, var2);
            }
        }
    }
}

void CminusfBuilder::visit(ASTCall &node) {
    //! TODO: This function is empty now.
    // Add some code here.
    Function *fun = (Function *)(scope.find(node.id));
    std::vector<Value *> args;
    auto param_it = fun->get_function_type()->param_begin();
    for (auto &arg : node.args) {
        arg->accept(*this);
        if (cur_value->get_type() != *param_it) {
            // 类型不同时转换类型
            if (cur_value->get_type()->is_integer_type()) {
                cur_value = builder->create_sitofp(cur_value, *param_it);
            } else {
                cur_value = builder->create_fptosi(cur_value, *param_it);
            }
        }
        param_it++;
        args.push_back(cur_value);
    }
    cur_value = builder->create_call(fun, args);
}
