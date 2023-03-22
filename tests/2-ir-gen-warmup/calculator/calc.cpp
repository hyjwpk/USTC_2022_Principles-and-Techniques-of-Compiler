extern "C" {
    #include "syntax_tree.h"
    extern syntax_tree *parse(const char*);
}
#include <cstdio>
#include <fstream>
#include "calc_ast.hpp"
#include "calc_builder.hpp"
using namespace std::literals::string_literals;

int main(int argc, char *argv[])
{
    syntax_tree *tree = NULL;
    const char *input = NULL;

    if (argc >= 3) {
        printf("usage: %s\n", argv[0]);
        printf("usage: %s <cminus_file>\n", argv[0]);
        return 1;
    }

    if (argc == 2) {
        input = argv[1];
    } else {
        printf("Input an arithmatic expression (press Ctrl+D in a new line after you finish the expression):\n");
    }

    tree = parse(input);
    CalcAST ast(tree);
    CalcBuilder builder;
    auto module = builder.build(ast);
    auto IR = module->print();

    std::ofstream output_stream;
    auto output_file = "result.ll";
    output_stream.open(output_file, std::ios::out);
    output_stream << "; ModuleID = 'calculator'\n";
    output_stream << IR;
    output_stream.close();
    auto command_string = "clang -O0 -w "s + "result.ll -o result -L. -lcminus_io";
    auto ret = std::system(command_string.c_str());
    if (ret) {
        printf("something went wrong!\n");
    } else {
        printf("result and result.ll have been generated.\n");
    }
    return ret;
}
