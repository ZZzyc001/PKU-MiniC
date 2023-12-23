#include "koopa_riscv.h"
#include <assert.h>
#include <iostream>
#include <map>

std::string gen_riscv_from_koopa_raw_program(const koopa_raw_program_t & raw) {
    int s = 0;

    std::map<const void *, int> buf;

    std::string res;
    res += ".text\n";
    Visit(raw.funcs, res, s, buf);
    return res;
}

void Visit(const koopa_raw_slice_t & slice, std::string & res, int & s, std::map<const void *, int> & buf) {
    for (size_t i = 0; i < slice.len; ++i) {
        auto ptr = slice.buffer[i];
        // 根据 slice 的 kind 决定将 ptr 视作何种元素
        switch (slice.kind) {
        case KOOPA_RSIK_FUNCTION:
            // 访问函数
            Visit(reinterpret_cast<koopa_raw_function_t>(ptr), res, s, buf);
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            // 访问基本块
            Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr), res, s, buf);
            break;
        case KOOPA_RSIK_VALUE:
            // 访问指令
            Visit(reinterpret_cast<koopa_raw_value_t>(ptr), res, s, buf);
            break;
        default:
            // 我们暂时不会遇到其他内容, 于是不对其做任何处理
            assert(false);
        }
    }
}

void Visit(const koopa_raw_function_t & func, std::string & res, int & s, std::map<const void *, int> & buf) {
    // 执行一些其他的必要操作
    const char * name = func->name + 1;
    res += std::string(".globl ") + name + "\n";
    res += std::string(name) + ":\n";

    // 访问所有基本块
    Visit(func->bbs, res, s, buf);
}

void Visit(const koopa_raw_basic_block_t & bb, std::string & res, int & s, std::map<const void *, int> & buf) {
    // 执行一些其他的必要操作

    // 访问所有指令
    Visit(bb->insts, res, s, buf);
}

std::string get_temp_register(int n) {
    if (n < 7)
        return "t" + std::to_string(n);
    else
        return "a" + std::to_string(n - 7);
}

void Visit(const koopa_raw_value_t & value, std::string & res, int & s, std::map<const void *, int> & buf) {
    // 根据指令类型判断后续需要如何访问
    const auto & kind = value->kind;
    switch (kind.tag) {
    case KOOPA_RVT_RETURN:
        // 访问 return 指令
        if (buf.find(kind.data.ret.value) != buf.end())
            res += "mv a0, " + get_temp_register(buf[kind.data.ret.value]) + "\n";
        else if (kind.data.ret.value->kind.tag == KOOPA_RVT_INTEGER)
            res += "li a0, " + std::to_string(kind.data.ret.value->kind.data.integer.value) + "\n";
        else
            // Visit(kind.data.ret.value, res, s, buf);
            assert(false);

        res += "ret\n";
        break;
    case KOOPA_RVT_INTEGER:
        // 访问 integer 指令
        res += "li " + get_temp_register(s) + ", ";
        res += std::to_string(kind.data.integer.value);
        res += "\n";
        buf[value] = s;
        break;
    case KOOPA_RVT_BINARY: {
        auto &      binary = kind.data.binary;
        std::string lhs, rhs;
        if (buf.find(binary.lhs) == buf.end())
            if (binary.lhs->kind.tag == KOOPA_RVT_INTEGER && binary.lhs->kind.data.integer.value == 0)
                lhs = "x0";
            else {
                Visit(binary.lhs, res, s, buf);
                lhs = get_temp_register(buf[binary.lhs]);
            }
        else
            lhs = get_temp_register(buf[binary.lhs]);

        if (buf.find(binary.rhs) == buf.end())
            if (binary.rhs->kind.tag == KOOPA_RVT_INTEGER && binary.rhs->kind.data.integer.value == 0)
                rhs = "x0";
            else {
                if (binary.lhs->kind.tag == KOOPA_RVT_INTEGER && binary.lhs->kind.data.integer.value != 0)
                    ++s;
                Visit(binary.rhs, res, s, buf);
                rhs = get_temp_register(buf[binary.rhs]);
            }
        else
            rhs = get_temp_register(buf[binary.rhs]);

        std::string result = get_temp_register(s);

        if (binary.op == KOOPA_RBO_SUB)
            res += "sub " + result + ", " + lhs + ", " + rhs + "\n";
        else if (binary.op == KOOPA_RBO_ADD)
            res += "add " + result + ", " + lhs + ", " + rhs + "\n";
        else if (binary.op == KOOPA_RBO_SUB)
            res += "sub " + result + ", " + lhs + ", " + rhs + "\n";
        else if (binary.op == KOOPA_RBO_MUL)
            res += "mul " + result + ", " + lhs + ", " + rhs + "\n";
        else if (binary.op == KOOPA_RBO_DIV)
            res += "div " + result + ", " + lhs + ", " + rhs + "\n";
        else if (binary.op == KOOPA_RBO_MOD)
            res += "rem " + result + ", " + lhs + ", " + rhs + "\n";
        else if (binary.op == KOOPA_RBO_LT)
            res += "slt " + result + ", " + lhs + ", " + rhs + "\n";
        else if (binary.op == KOOPA_RBO_LE)
            res += "sgt " + result + ", " + rhs + ", " + lhs + "\n";
        else if (binary.op == KOOPA_RBO_GT)
            res += "sgt " + result + ", " + lhs + ", " + rhs + "\n";
        else if (binary.op == KOOPA_RBO_GE)
            res += "slt " + result + ", " + rhs + ", " + lhs + "\n";
        else if (binary.op == KOOPA_RBO_AND)
            res += "and " + result + ", " + rhs + ", " + lhs + "\n";
        else if (binary.op == KOOPA_RBO_OR)
            res += "or " + result + ", " + rhs + ", " + lhs + "\n";
        else if (binary.op == KOOPA_RBO_NOT_EQ) {
            res += "xor " + result + ", " + lhs + ", " + rhs + "\n";
            res += "snez " + result + ", " + result + "\n";
        } else if (binary.op == KOOPA_RBO_EQ) {
            res += "xor " + result + ", " + lhs + ", " + rhs + "\n";
            res += "seqz " + result + ", " + result + "\n";
        }

        buf[value] = s;
        ++s;
        break;
    }
    default:
        assert(false);
    }
}
