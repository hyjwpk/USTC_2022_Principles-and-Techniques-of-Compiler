#ifndef _CALC_AST_HPP_
#define _CALC_AST_HPP_
extern "C" {
    #include "syntax_tree.h"
    extern syntax_tree *parse(const char *input);
}
#include <vector>
#include <memory>

enum AddOp {
    // +
    OP_PLUS,
    // -
    OP_MINUS
};

enum MulOp {
    // *
    OP_MUL,
    // /
    OP_DIV
};

class CalcAST;

struct CalcASTNode;
struct CalcASTInput;
struct CalcASTExpression;
struct CalcASTNum;
struct CalcASTTerm;
struct CalcASTFactor;

class CalcASTVisitor;

class CalcAST {
public:
    CalcAST() = delete;
    CalcAST(syntax_tree *);
    CalcAST(CalcAST &&tree) {
        root = tree.root;
        tree.root = nullptr;
    };
    CalcASTInput* get_root() { return root.get(); }
    void run_visitor(CalcASTVisitor& visitor);
private:
    CalcASTNode* transform_node_iter(syntax_tree_node *);
    std::shared_ptr<CalcASTInput> root = nullptr;
};

struct CalcASTNode {
    virtual void accept(CalcASTVisitor &) = 0;
};

struct CalcASTInput: CalcASTNode {
    virtual void accept(CalcASTVisitor &) override final;
    std::shared_ptr<CalcASTExpression> expression;
};

struct CalcASTFactor: CalcASTNode {
    virtual void accept(CalcASTVisitor &) override;
};

struct CalcASTNum: CalcASTFactor {
    virtual void accept(CalcASTVisitor &) override final;
    int val;
};

struct CalcASTExpression: CalcASTFactor {
    virtual void accept(CalcASTVisitor &) override final;
    std::shared_ptr<CalcASTExpression> expression;
    AddOp op;
    std::shared_ptr<CalcASTTerm> term;
};

struct CalcASTTerm : CalcASTNode {
    virtual void accept(CalcASTVisitor &) override final;
    std::shared_ptr<CalcASTTerm> term;
    MulOp op;
    std::shared_ptr<CalcASTFactor> factor;
};


class CalcASTVisitor {
public:
    virtual void visit(CalcASTInput &) = 0;
    virtual void visit(CalcASTNum &) = 0;
    virtual void visit(CalcASTExpression &) = 0;
    virtual void visit(CalcASTTerm &) = 0;
};
#endif
