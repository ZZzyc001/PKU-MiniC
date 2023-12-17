#include "koopa_riscv.h"
#include <assert.h>

std::string gen_riscv_from_koopa_raw_program(const koopa_raw_program_t & raw) {
    std::string res;
    res += ".text\n";
    Visit(raw.funcs, res);
    return res;
}

void Visit(const koopa_raw_slice_t & slice, std::string & res) {
    for (size_t i = 0; i < slice.len; ++i) {
        auto ptr = slice.buffer[i];
        // 根据 slice 的 kind 决定将 ptr 视作何种元素
        switch (slice.kind) {
        case KOOPA_RSIK_FUNCTION:
            // 访问函数
            Visit(reinterpret_cast<koopa_raw_function_t>(ptr), res);
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            // 访问基本块
            Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr), res);
            break;
        case KOOPA_RSIK_VALUE:
            // 访问指令
            Visit(reinterpret_cast<koopa_raw_value_t>(ptr), res);
            break;
        default:
            // 我们暂时不会遇到其他内容, 于是不对其做任何处理
            assert(false);
        }
    }
}

void Visit(const koopa_raw_function_t & func, std::string & res) {
    // 执行一些其他的必要操作
    const char * name = func->name + 1;
    res += std::string(".globl ") + name + "\n";
    res += std::string(name) + ":\n";

    // 访问所有基本块
    Visit(func->bbs, res);
}

void Visit(const koopa_raw_basic_block_t & bb, std::string & res) {
    // 执行一些其他的必要操作

    // 访问所有指令
    Visit(bb->insts, res);
}

void Visit(const koopa_raw_value_t & value, std::string & res) {
    // 根据指令类型判断后续需要如何访问
    const auto & kind = value->kind;
    switch (kind.tag) {
    case KOOPA_RVT_RETURN:
        // 访问 return 指令
        res += "li a0, ";
        Visit(kind.data.ret.value, res);
        res += "\nret\n";
        break;
    case KOOPA_RVT_INTEGER:
        // 访问 integer 指令
        res += std::to_string(kind.data.integer.value);
        break;
    default:
        // 其他类型暂时遇不到
        assert(false);
    }
}
