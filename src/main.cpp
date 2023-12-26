#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "ast.h"
#include "koopa_riscv.h"

using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE * yyin;
extern int    yyparse(unique_ptr<BaseAST> & ast);

int main(int argc, const char * argv[]) {
    // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
    // compiler 模式 输入文件 -o 输出文件
    assert(argc == 5);
    auto mode   = argv[1];
    auto input  = argv[2];
    auto output = argv[4];

    // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
    yyin = fopen(input, "r");
    assert(yyin);

    // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
    unique_ptr<BaseAST> ast;
    auto                ret = yyparse(ast);
    assert(! ret);

    // 输出解析得到的 AST, 其实就是个字符串
    std::cout << "AST:" << std::endl
              << std::endl;
    ast->Dump();
    cout << endl;

    unique_ptr<CompUnitAST> comp(dynamic_cast<CompUnitAST *>(ast.release()));
    koopa_raw_program_t     krp = comp->to_koopa_raw_program();

    if (mode == string("-koopa")) {
        koopa_program_t    kp;
        koopa_error_code_t eno = koopa_generate_raw_to_koopa(&krp, &kp);
        if (eno != KOOPA_EC_SUCCESS) {
            std::cout << "generate raw to koopa error: " << (int) eno << std::endl;
            return 0;
        }

        char * buffer = new char[3001];
        size_t sz     = 3000;
        eno           = koopa_dump_to_string(kp, buffer, &sz);
        if (eno != KOOPA_EC_SUCCESS) {
            std::cout << "dump to string error: " << (int) eno << std::endl;
            return 0;
        }
        std::cout << "koopa: " << sz << std::endl
                  << std::endl
                  << buffer;

        ofstream yyout(output);
        yyout << buffer;
        yyout.close();
    }

    if (mode == string("-riscv")) {
        std::string str_riscv = gen_riscv_from_koopa_raw_program(krp);
        std::cout << "riscv:" << std::endl
                  << std::endl
                  << str_riscv;

        std::ofstream yyout(output);
        yyout << str_riscv;
        yyout.close();
    }

    return 0;
}