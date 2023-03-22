#include "calc_ast.hpp"
#include <cstring>
#include <stack>
#include <iostream>
#define _AST_NODE_ERROR_ \
    std::cerr << "Abort due to node cast error."\
    "Contact with TAs to solve your problem."\
    << std::endl;\
    std::abort();
#define _STR_EQ(a, b) (strcmp((a), (b)) == 0)

void CalcAST::run_visitor(CalcASTVisitor &visitor) {
    root->accept(visitor);
}

CalcAST::CalcAST(syntax_tree* s) {
    if (s == nullptr) {
        std::cerr << "empty input tree!" << std::endl;
        std::abort();
    }
    auto node = transform_node_iter(s->root);
    del_syntax_tree(s);
    root = std::shared_ptr<CalcASTInput>(
            static_cast<CalcASTInput*>(node));
}

CalcASTNode *
CalcAST::transform_node_iter(syntax_tree_node *n) {
    if (_STR_EQ(n->name, "input")) {
        auto node = new CalcASTInput();
        auto expr_node =
            static_cast<CalcASTExpression *>(
                    transform_node_iter(n->children[0]));
        node->expression = std::shared_ptr<CalcASTExpression>(expr_node);
        return node;
    } else if (_STR_EQ(n->name, "expression")) {
        auto node = new CalcASTExpression();
        if (n->children_num == 3) {
            auto add_expr_node =
                static_cast<CalcASTExpression *>(
                        transform_node_iter(n->children[0]));
            node->expression =
                std::shared_ptr<CalcASTExpression>(add_expr_node);

            auto op_name = n->children[1]->children[0]->name;
            if (_STR_EQ(op_name, "+"))
                node->op = OP_PLUS;
            else if (_STR_EQ(op_name, "-"))
                node->op = OP_MINUS;

            auto term_node =
                static_cast<CalcASTTerm *>(
                        transform_node_iter(n->children[2]));
            node->term = std::shared_ptr<CalcASTTerm>(term_node);
        } else {
            auto term_node =
                static_cast<CalcASTTerm *>(
                        transform_node_iter(n->children[0]));
            node->term = std::shared_ptr<CalcASTTerm>(term_node);
        }
        return node;
    } else if (_STR_EQ(n->name, "term")) {
        auto node = new CalcASTTerm();
        if (n->children_num == 3) {
            auto term_node =
                static_cast<CalcASTTerm *>(
                        transform_node_iter(n->children[0]));
            node->term =
                std::shared_ptr<CalcASTTerm>(term_node);

            auto op_name = n->children[1]->children[0]->name;
            if (_STR_EQ(op_name, "*"))
                node->op = OP_MUL;
            else if (_STR_EQ(op_name, "/"))
                node->op = OP_DIV;

            auto factor_node =
                static_cast<CalcASTFactor *>(
                        transform_node_iter(n->children[2]));
            node->factor = std::shared_ptr<CalcASTFactor>(factor_node);
        } else {
            auto factor_node =
                static_cast<CalcASTFactor *>(
                        transform_node_iter(n->children[0]));
            node->factor = std::shared_ptr<CalcASTFactor>(factor_node);
        }
        return node;
    } else if (_STR_EQ(n->name, "factor")) {
        if (n->children_num == 3) {
            return transform_node_iter(n->children[1]);
        } else {
            auto num_node = new CalcASTNum();
            num_node->val = std::stoi(n->children[0]->children[0]->name);
            return num_node;
        }
    } else {
        std::cerr << "[calc_ast]: transform failure!" << std::endl;
        std::abort();
    }
}


void CalcASTNum::accept(CalcASTVisitor &visitor) { visitor.visit(*this); }
void CalcASTTerm::accept(CalcASTVisitor &visitor) { visitor.visit(*this); }
void CalcASTExpression::accept(CalcASTVisitor &visitor) { visitor.visit(*this); }

void CalcASTInput::accept(CalcASTVisitor &visitor) { expression->accept(visitor); }

void CalcASTFactor::accept(CalcASTVisitor &visitor) {
    auto expr =
        dynamic_cast<CalcASTExpression *>(this);
    if (expr) {
        expr->accept(visitor);
        return;
    }

    auto num =
        dynamic_cast<CalcASTNum *>(this);
    if (num) {
        num->accept(visitor);
        return;
    }

    _AST_NODE_ERROR_
}
