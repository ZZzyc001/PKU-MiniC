#include "koopa_riscv.h"
#include <assert.h>
#include <iostream>
#include <map>

static std::map<koopa_raw_value_t, int> addr;

static int cur    = 0;
static int t_size = 0;

int getAddr(koopa_raw_value_t val) {
    if (addr.count(val))
        return addr[val];
    int t = cal_size(val);
    assert(t > 0);

    int t_cur = cur;
    cur += t;
    addr[val] = t_cur;
    return t_cur;
}

void load_reg(koopa_raw_value_t val, const std::string & reg, std::string & res) {
    if (val->kind.tag == KOOPA_RVT_INTEGER)
        res += "li " + reg + ", " + std::to_string(val->kind.data.integer.value) + "\n";
    else
        res += "lw " + reg + ", " + std::to_string(getAddr(val)) + "(sp)\n";
}

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

    int size = cal_size(func);

    size = ((size - 15) / 16 + 1) * 16;

    res += "addi sp, sp, -" + std::to_string(size) + "\n";

    t_size = size;
    cur    = 0;
    addr.clear();

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
    case KOOPA_RVT_INTEGER:
        res += std::to_string(kind.data.integer.value) + "\n";
        break;
    case KOOPA_RVT_ALLOC:
        std::cerr << "Begin ALLOC" << std::endl;
        break;
    case KOOPA_RVT_LOAD:
        load_reg(kind.data.load.src, "t0", res);
        res += "sw t0, " + std::to_string(getAddr(value)) + "(sp)\n";
        break;
    case KOOPA_RVT_STORE:
        load_reg(kind.data.store.value, "t0", res);
        assert(kind.data.store.dest);
        res += "sw t0, " + std::to_string(getAddr(kind.data.store.dest)) + "(sp)\n";
        break;
    case KOOPA_RVT_RETURN:
        // 访问 return 指令
        if (kind.data.ret.value->kind.tag == KOOPA_RVT_INTEGER)
            res += "li a0, " + std::to_string(kind.data.ret.value->kind.data.integer.value) + "\n";
        else
            res += "lw a0, " + std::to_string(getAddr(kind.data.ret.value)) + "(sp)\n";

        res += "addi sp, sp, " + std::to_string(t_size) + "\nret\n";
        break;
    case KOOPA_RVT_BINARY: {
        auto & binary = kind.data.binary;

        load_reg(binary.lhs, "t0", res);
        load_reg(binary.rhs, "t1", res);

        std::string result("t0"), lhs("t0"), rhs("t1");

        switch (binary.op) {
        case KOOPA_RBO_SUB:
            res += "sub " + result + ", " + lhs + ", " + rhs + "\n";
            break;
        case KOOPA_RBO_ADD:
            res += "add " + result + ", " + lhs + ", " + rhs + "\n";
            break;
        case KOOPA_RBO_MUL:
            res += "mul " + result + ", " + lhs + ", " + rhs + "\n";
            break;
        case KOOPA_RBO_DIV:
            res += "div " + result + ", " + lhs + ", " + rhs + "\n";
            break;
        case KOOPA_RBO_MOD:
            res += "rem " + result + ", " + lhs + ", " + rhs + "\n";
            break;
        case KOOPA_RBO_LT:
            res += "slt " + result + ", " + lhs + ", " + rhs + "\n";
            break;
        case KOOPA_RBO_LE:
            res += "sgt " + result + ", " + rhs + ", " + lhs + "\n";
            break;
        case KOOPA_RBO_GT:
            res += "sgt " + result + ", " + lhs + ", " + rhs + "\n";
            break;
        case KOOPA_RBO_GE:
            res += "slt " + result + ", " + rhs + ", " + lhs + "\n";
            break;
        case KOOPA_RBO_AND:
            res += "and " + result + ", " + rhs + ", " + lhs + "\n";
            break;
        case KOOPA_RBO_OR:
            res += "or " + result + ", " + rhs + ", " + lhs + "\n";
            break;
        case KOOPA_RBO_NOT_EQ:
            res += "xor " + result + ", " + lhs + ", " + rhs + "\n";
            res += "snez " + result + ", " + result + "\n";
            break;
        case KOOPA_RBO_EQ:
            res += "xor " + result + ", " + lhs + ", " + rhs + "\n";
            res += "seqz " + result + ", " + result + "\n";
            break;

        default:
            break;
        }
        res += "sw t0, " + std::to_string(getAddr(value)) + "(sp)\n";
        break;
    }
    default:
        assert(false);
    }
}

int cal_size(const koopa_raw_slice_t & slice) {
    int tot = 0;
    for (size_t i = 0; i < slice.len; ++i) {
        auto ptr = slice.buffer[i];
        switch (slice.kind) {
        case KOOPA_RSIK_FUNCTION:
            tot += cal_size(reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            tot += cal_size(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            tot += cal_size(reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        default:
            assert(false);
        }
    }
    return tot;
}

int cal_size(const koopa_raw_function_t & func) {
    return cal_size(func->bbs);
}

int cal_size(const koopa_raw_basic_block_t & bb) {
    return cal_size(bb->insts);
}

int cal_size(const koopa_raw_value_t & value) {
    switch (value->ty->tag) {
    case KOOPA_RTT_INT32:
    case KOOPA_RTT_POINTER:
        return 4;
    case KOOPA_RTT_UNIT:
        return 0;
    }
    return 0;
}
