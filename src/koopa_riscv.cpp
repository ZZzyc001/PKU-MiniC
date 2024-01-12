#include "koopa_riscv.h"
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <map>

static std::map<koopa_raw_value_t, int> addr;

static int  cur     = 0;
static int  t_size  = 0;
static int  cnt_num = 0;
static bool calling = false;

static std::string func_name;

int getAddr(koopa_raw_value_t val) {
    if (addr.count(val))
        return addr[val];

    int t = cal_size(val);
    assert(t > 0);

    // int t_cur = cur;
    cur -= t;
    addr[val] = cur;
    return cur;
}

void split(int addr, const std::string & reg, const std::string & t, std::string & res, bool save) {
    std::string op = save ? "sw" : "lw";
    if (addr < -2048 || addr > 2047) {
        res += "li " + t + ", " + std::to_string(addr) + "\n";
        res += "add " + t + ", " + t + ", sp\n";
        res += op + " " + reg + ", 0(" + t + ")\n";
    } else
        res += op + " " + reg + ", " + std::to_string(addr) + "(sp)\n";
}

void load_reg(koopa_raw_value_t val, const std::string & reg, std::string & res) {
    if (val->kind.tag == KOOPA_RVT_INTEGER)
        res += "li " + reg + ", " + std::to_string(val->kind.data.integer.value) + "\n";
    else if (val->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        res += "la t0, " + std::string(val->name).substr(1) + "\n";
        res += "lw " + reg + ", 0(t0)\n";
    } else {
        int addr = getAddr(val);
        split(addr, reg, "t6", res, false);
    }
}

std::string gen_riscv_from_koopa_raw_program(const koopa_raw_program_t & raw) {
    std::string res;
    res += ".data\n";

    Visit(raw.values, res);

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
    if (func->bbs.len == 0)
        return;

    const char * name = func->name + 1;
    res += std::string(".globl ") + name + "\n";
    res += std::string(name) + ":\n";

    bool call = false;
    int  size = cal_size(func, call);

    if (size) {
        size = ((size - 1) / 16 + 1) * 16;
        if (-size < -2048 || -size > 2047) {
            res += std::string("li t0, ") + std::to_string(-size) + "\n";
            res += std::string("add sp, sp, t0") + "\n";
        } else
            res += std::string("addi sp, sp, ") + std::to_string(-size) + "\n";
    }

    if (call) {
        int offset = size - 4;
        if (offset < -2048 || offset > 2047) {
            res += std::string("li t0, ") + std::to_string(offset) + "\n";
            res += std::string("add t0, sp, t0") + "\n";
            res += std::string("sw ra, 0(t0)") + "\n";
        } else
            res += std::string("sw ra, ") + std::to_string(offset) + "(sp)\n";
    }

    t_size  = size;
    calling = call;
    cur     = size - (calling ? 4 : 0);
    addr.clear();

    func_name = std::string(func->name).substr(1);

    // 访问所有基本块
    Visit(func->bbs, res);
}

void Visit(const koopa_raw_basic_block_t & bb, std::string & res) {
    // 执行一些其他的必要操作
    res += func_name + "_" + std::string(bb->name).substr(1) + ":\n";

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
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        gen_global_alloc(value, res);
        break;
    case KOOPA_RVT_LOAD:
        load_reg(kind.data.load.src, "t0", res);
        if (kind.data.load.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR || kind.data.load.src->kind.tag == KOOPA_RVT_GET_PTR)
            res += "lw t0, 0(t0)\n";
        split(getAddr(value), "t0", "t6", res, true);
        break;
    case KOOPA_RVT_STORE:
        gen_store(kind.data.store, res);
        break;
    case KOOPA_RVT_GET_PTR:
        gen_get_ptr(kind.data.get_ptr, getAddr(value), res);
        break;
    case KOOPA_RVT_GET_ELEM_PTR:
        gen_get_elem_ptr(kind.data.get_elem_ptr, getAddr(value), res);
        break;
    case KOOPA_RVT_RETURN:
        gen_return(kind.data.ret, res);
        break;
    case KOOPA_RVT_BRANCH:
        gen_branch(kind.data.branch, res);
        break;

    case KOOPA_RVT_JUMP:
        res += "j " + func_name + "_" + std::string(kind.data.jump.target->name).substr(1) + "\n";
        break;

    case KOOPA_RVT_CALL:
        gen_call(kind.data.call, value->ty->tag == KOOPA_RTT_UNIT ? -1 : getAddr(value), res);
        break;

    case KOOPA_RVT_BINARY:
        gen_binary(kind.data.binary, getAddr(value), res);
        break;

    default:
        assert(false);
    }
}

int cal_size(const koopa_raw_function_t & func, bool & call) {
    int total_size = 0;
    for (size_t i = 0; i < func->bbs.len; ++i) {
        const void * data = func->bbs.buffer[i];
        total_size += cal_size((koopa_raw_basic_block_t) data, call);
    }
    total_size += call ? 4 : 0;
    return total_size;
}

int cal_size(const koopa_raw_basic_block_t & bb, bool & call) {
    int total_size = 0;
    for (size_t i = 0; i < bb->insts.len; ++i) {
        const void * data = bb->insts.buffer[i];
        if (((koopa_raw_value_t) data)->kind.tag == KOOPA_RVT_CALL)
            call = true;
        total_size += cal_size((koopa_raw_value_t) data);
    }
    return total_size;
}

int cal_size(const koopa_raw_value_t & value) {
    if (value->kind.tag == KOOPA_RVT_ALLOC)
        return cal_size(value->ty->data.pointer.base);
    return cal_size(value->ty);
}

int cal_size(const koopa_raw_type_t & type) {
    switch (type->tag) {
    case KOOPA_RTT_INT32:
        return 4;
    case KOOPA_RTT_UNIT:
        return 0;
    case KOOPA_RTT_POINTER:
        return 4;
    case KOOPA_RTT_ARRAY:
        return cal_size(type->data.array.base) * (type->data.array.len);
    case KOOPA_RTT_FUNCTION:
        return 0;
    }
}

void gen_global_alloc(koopa_raw_value_t alloc, std::string & res) {
    res += ".globl " + std::string(alloc->name).substr(1) + "\n";
    res += std::string(alloc->name).substr(1) + ":\n";
    if (alloc->kind.data.global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT)
        res += ".zero " + std::to_string(cal_size(alloc->ty->data.pointer.base)) + "\n";
    else if (alloc->kind.data.global_alloc.init->kind.tag == KOOPA_RVT_AGGREGATE)
        gen_aggregate(alloc->kind.data.global_alloc.init, res);
    else
        res += ".word " + std::to_string(alloc->kind.data.global_alloc.init->kind.data.integer.value) + "\n";
}

void gen_store(const koopa_raw_store_t & store, std::string & res) {
    std::string dest;

    if (store.dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
        res += "la t1, " + std::string(store.dest->name).substr(1) + "\n";
        dest = "0(t1)";
    } else if (store.dest->kind.tag == KOOPA_RVT_GET_ELEM_PTR || store.dest->kind.tag == KOOPA_RVT_GET_PTR) {
        load_reg(store.dest, "t1", res);
        dest = "0(t1)";
    } else {
        int addr = getAddr(store.dest);
        if (addr < -2048 || addr > 2047) {
            res += "li t1, " + std::to_string(addr) + "\n";
            res += "add t1, t1, sp\n";
            dest = "0(t1)";
        } else
            dest = std::to_string(addr) + "(sp)";
    }

    if (store.value->kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
        if (store.value->kind.data.func_arg_ref.index < 8)
            res += "sw a" + std::to_string(store.value->kind.data.func_arg_ref.index) + ", " + dest + "\n";
        else {
            int offset = (store.value->kind.data.func_arg_ref.index - 8) * 4;
            split(offset, "t0", "t2", res, false);
            res += "sw t0, " + dest + "\n";
        }
    } else {
        load_reg(store.value, "t0", res);
        res += "sw t0, " + dest + "\n";
    }
}

void gen_get_ptr(const koopa_raw_get_ptr_t & get, int addr, std::string & res) {
    int src = getAddr(get.src);
    split(src, "t0", "t0", res, false);

    load_reg(get.index, "t1", res);

    int n = cal_size(get.src->ty->data.pointer.base);
    res += "li t2, " + std::to_string(n) + "\n";
    res += "mul t1, t1, t2\n";
    res += "add t0, t0, t1\n";

    split(addr, "t0", "t6", res, true);
}

void gen_get_elem_ptr(const koopa_raw_get_elem_ptr_t & get, int addr, std::string & res) {
    if (get.src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
        res += "la t0, " + std::string(get.src->name).substr(1) + "\n";
    else {
        int src = getAddr(get.src);
        if (src < -2048 || src > 2047) {
            res += "li t0, " + std::to_string(src) + "\n";
            res += "add t0, sp, t0\n";
        } else
            res += "addi t0, sp, " + std::to_string(src) + "\n";

        if (get.src->kind.tag == KOOPA_RVT_GET_ELEM_PTR || get.src->kind.tag == KOOPA_RVT_GET_PTR)
            res += "lw t0, 0(t0)\n";
    }

    load_reg(get.index, "t1", res);
    int n = cal_size(get.src->ty->data.pointer.base->data.array.base);
    res += "li t2, " + std::to_string(n) + "\n";
    res += "mul t1, t1, t2\n";
    res += "add t0, t0, t1\n";

    split(addr, "t0", "t6", res, true);
}

void gen_branch(const koopa_raw_branch_t & branch, std::string & res) {
    load_reg(branch.cond, "t0", res);
    res += "beqz t0, " + func_name + "_skip" + std::to_string(cnt_num) + "\n";
    res += "j " + func_name + "_" + std::string(branch.true_bb->name).substr(1) + "\n";
    res += func_name + "_skip" + std::to_string(cnt_num++) + ":\n";
    res += "j " + func_name + "_" + std::string(branch.false_bb->name).substr(1) + "\n";
}

void gen_call(const koopa_raw_call_t & call, int addr, std::string & res) {
    for (int i = 0; i < call.args.len && i < 8; ++i) {
        char reg[3] = "a0";
        reg[1] += i;
        load_reg((koopa_raw_value_t) call.args.buffer[i], reg, res);
    }

    bool func_call = false;
    int  func_sz   = cal_size(call.callee, func_call);
    if (func_sz)
        func_sz = ((func_sz - 1) / 16 + 1) * 16;
    for (int i = 8; i < call.args.len; ++i) {
        load_reg((koopa_raw_value_t) call.args.buffer[i], "t0", res);
        int offset = (i - 8) * 4 - func_sz;
        split(offset, "t0", "t6", res, true);
    }
    res += "call " + std::string(call.callee->name).substr(1) + "\n";
    if (addr != -1)
        split(addr, "a0", "t6", res, true);
}

void gen_return(const koopa_raw_return_t & ret, std::string & res) {
    if (ret.value)
        load_reg(ret.value, "a0", res);
    if (calling)
        split(t_size - 4, "ra", "t6", res, false);

    if (t_size) {
        int sz = t_size;
        if (sz < -2048 || sz > 2047) {
            res += "li t0, " + std::to_string(sz) + "\n";
            res += "add sp, sp, t0\n";
        } else
            res += "addi sp, sp, " + std::to_string(sz) + "\n";
    }
    res += "ret\n";
}

void gen_binary(const koopa_raw_binary_t & binary, int addr, std::string & res) {
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
    case KOOPA_RBO_XOR:
        res += "xor " + result + ", " + lhs + ", " + rhs + "\n";
        break;
    case KOOPA_RBO_SHL:
        res += "sll " + result + ", " + lhs + ", " + rhs + "\n";
        break;
    case KOOPA_RBO_SHR:
        res += "srl " + result + ", " + lhs + ", " + rhs + "\n";
        break;
    case KOOPA_RBO_SAR:
        res += "sra " + result + ", " + lhs + ", " + rhs + "\n";
        break;
    }
    split(addr, "t0", "t6", res, true);
}

void gen_aggregate(koopa_raw_value_t val, std::string & res) {
    if (val->ty->tag == KOOPA_RTT_ARRAY) {
        for (int i = 0; i < val->kind.data.aggregate.elems.len; ++i)
            gen_aggregate((koopa_raw_value_t) val->kind.data.aggregate.elems.buffer[i], res);
    } else
        res += ".word " + std::to_string(val->kind.data.integer.value) + "\n";
}
