#include "Dominators.h"
#include "GVN.h"
#include "Mem2Reg.hpp"
#include "PassManager.hpp"
#include "cminusf_builder.hpp"
#include "logging.hpp"

#include <fstream>
#include <iostream>
#include <memory>

using namespace std::literals::string_literals;

void print_help(std::string exe_name) {
    std::cout << "Usage: " << exe_name
              << " [ -h | --help ] [ -o <target-file> ] [ -emit-llvm ] [-mem2reg] [-gvn] [-dump-json] <input-file>"
              << std::endl;
}

int main(int argc, char **argv) {
    std::string target_path;
    std::string input_path;

    bool mem2reg = false;
    bool gvn = false;
    bool dump_json = false;
    bool emit = false;

    for (int i = 1; i < argc; ++i) {
        if (argv[i] == "-h"s || argv[i] == "--help"s) {
            print_help(argv[0]);
            return 0;
        } else if (argv[i] == "-o"s) {
            if (target_path.empty() && i + 1 < argc) {
                target_path = argv[i + 1];
                i += 1;
            } else {
                print_help(argv[0]);
                return 0;
            }
        } else if (argv[i] == "-emit-llvm"s) {
            emit = true;
        } else if (argv[i] == "-mem2reg"s) {
            mem2reg = true;
        } else if (argv[i] == "-gvn"s) {
            gvn = true;
        } else if (argv[i] == "-dump-json"s) {
            dump_json = true;
        } else {
            if (input_path.empty()) {
                input_path = argv[i];
            } else {
                print_help(argv[0]);
                return 0;
            }
        }
    }
    if (gvn and not mem2reg)
        LOG_WARNING << "Enabling GVN without mem2reg";
    if (input_path.empty()) {
        print_help(argv[0]);
        return 0;
    }

    if (target_path.empty()) {
        auto pos = input_path.rfind('.');
        if (pos == std::string::npos) {
            std::cerr << argv[0] << ": input file " << input_path << " has unknown filetype!" << std::endl;
            return -1;
        } else {
            if (input_path.substr(pos) != ".cminus") {
                std::cerr << argv[0] << ": input file " << input_path << " has unknown filetype!" << std::endl;
                return -1;
            }
            if (emit) {
                target_path = input_path.substr(0, pos);
            } else {
                target_path = input_path.substr(0, pos);
            }
        }
    }

    auto s = parse(input_path.c_str());
    auto a = AST(s);
    CminusfBuilder builder;
    a.run_visitor(builder);

    auto m = builder.getModule();

    // m->set_print_name();
    PassManager PM(m.get());

    if (mem2reg) {
        PM.add_pass<Mem2Reg>(emit);
    }
    if (gvn)
        PM.add_pass<GVN>(emit, dump_json);

    PM.run();

    auto IR = m->print();

    std::ofstream output_stream;
    auto output_file = target_path + ".ll";
    output_stream.open(output_file, std::ios::out);
    output_stream << "; ModuleID = 'cminus'\n";
    output_stream << "source_filename = \"" + input_path + "\"\n\n";
    output_stream << IR;
    output_stream.close();
    if (!emit) {
        std::string lib_path = " -L"s + argv[0];
        auto idx = lib_path.rfind('/');
        if (idx != std::string::npos)
            lib_path.erase(idx);
        else
            lib_path.clear();
        auto cmd_str = "clang -O0 -w -no-pie "s + target_path + ".ll -o " + target_path + lib_path + " -lcminus_io";
        int re_code0 = std::system(cmd_str.c_str());
        cmd_str = "rm "s + target_path + ".ll";
        int re_code1 = std::system(cmd_str.c_str());
        if (re_code0 == 0 && re_code1 == 0)
            return 0;
        else
            return 1;
    }

    return 0;
}
