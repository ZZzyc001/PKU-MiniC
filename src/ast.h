#pragma once

#include <assert.h>
#include <iostream>
#include <memory>
#include <vector>

#include "block.h"
#include "koopa_util.h"
#include "symbol_list.h"

static char * make_char_arr(std::string str) {
    size_t n   = str.length();
    char * res = new char(n + 1);
    str.copy(res, n + 1);
    res[n] = 0;
    return res;
}

// 所有 AST 的基类
class BaseAST {
public:
    static SymbolList symbol_list;
    static BlockMaker blocks_list;

    virtual ~BaseAST() = default;

    virtual void Dump() const {}

    virtual void * to_koopa_item() const {
        std::cout << "Run into ERROR\n";
        return nullptr;
    }
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override {
        std::cout << "CompUnitAST { ";
        func_def->Dump();
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        return nullptr;
    }

    koopa_raw_program_t to_koopa_raw_program() const {
        std::vector<const void *> funcs;
        funcs.push_back(func_def->to_koopa_item());

        koopa_raw_program_t res;
        res.values = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res.funcs  = make_koopa_rs_from_vector(funcs, KOOPA_RSIK_FUNCTION);

        return res;
    }
};

class ValueBaseAST : public BaseAST {
public:
    virtual void * build_value_ast(const koopa_raw_slice_t & parent) const {
        std::cout << "Run into ERROR\n";
        return nullptr;
    }

    virtual int get_value() const {
        std::cout << "Run into ERROR\n";
        return 0;
    }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST>      func_type;
    std::string                   ident;
    std::unique_ptr<ValueBaseAST> block;

    void Dump() const override {
        std::cout << "FuncDefAST { ";
        func_type->Dump();
        std::cout << ", " << ident << ", ";
        block->Dump();
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        koopa_raw_function_data_t * res = new koopa_raw_function_data_t();

        std::vector<const void *> blocks;

        blocks_list.setBlockBuf(&blocks);

        koopa_raw_basic_block_data_t * entry_block = new koopa_raw_basic_block_data_t();

        entry_block->name    = make_char_arr("%entry_" + ident);
        entry_block->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
        entry_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);

        blocks_list.addBlock(entry_block);

        block->build_value_ast(empty_koopa_rs());

        blocks_list.finishBlock();

        res->bbs = make_koopa_rs_from_vector(blocks, KOOPA_RSIK_BASIC_BLOCK);

        res->name = make_char_arr("@" + ident);

        res->params = empty_koopa_rs(KOOPA_RSIK_VALUE);

        koopa_raw_type_kind_t * ty = new koopa_raw_type_kind_t();
        ty->tag                    = KOOPA_RTT_FUNCTION;
        ty->data.function.params   = empty_koopa_rs(KOOPA_RSIK_TYPE);
        ty->data.function.ret      = (const struct koopa_raw_type_kind *) func_type->to_koopa_item();
        res->ty                    = ty;

        return res;
    }
};

// FuncType
class FuncTypeAST : public BaseAST {
public:
    void Dump() const override {
        std::cout << "FuncTypeAST { ";
        std::cout << "int";
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        return simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
    }
};

// Block
class BlockAST : public ValueBaseAST {
public:
    std::vector<std::unique_ptr<ValueBaseAST>> insts;

    void Dump() const override {
        std::cout << "BlockAST { ";
        for (const auto & it : insts)
            it->Dump();
        std::cout << " }";
    }

    virtual void * build_value_ast(const koopa_raw_slice_t & parent) const override {
        symbol_list.newEnv();

        for (const auto & it : insts)
            it->build_value_ast(empty_koopa_rs());

        symbol_list.deleteEnv();

        return nullptr;
    }
};

// Stmt
class StmtAST : public ValueBaseAST {
public:
    enum class StmtType {
        Assign,
        Return,
        Branch
    } type;

    int branch_id;

    std::unique_ptr<ValueBaseAST> lval;

    std::unique_ptr<ValueBaseAST> exp;

    std::unique_ptr<ValueBaseAST> rval;

    void Dump() const override {
        std::cout << "StmtAST { ";
        if (type == StmtType::Assign) {
            lval->Dump();
            std::cout << " = ";
        }
        if (type == StmtType::Branch)
            std::cout << "if ( ";
        exp->Dump();
        if (type == StmtType::Branch) {
            std::cout << " ) ";
            if (lval)
                lval->Dump();
            if (rval) {
                std::cout << " else ";
                rval->Dump();
            }
        }
        std::cout << " }";
    }

    void * build_value_ast(const koopa_raw_slice_t & parent) const override {
        koopa_raw_value_data * res  = new koopa_raw_value_data();
        koopa_raw_slice_t      node = make_koopa_rs_single_element(res, KOOPA_RSIK_VALUE);

        res->ty      = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
        res->name    = nullptr;
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);

        if (type == StmtType::Return) {
            res->kind.tag = KOOPA_RVT_RETURN;

            res->kind.data.ret.value = (koopa_raw_value_t) exp->build_value_ast(node);
            blocks_list.addInst(res);
            return res;
        } else if (type == StmtType::Assign) {
            res->kind.tag = KOOPA_RVT_STORE;

            res->kind.data.store.value = (koopa_raw_value_t) exp->build_value_ast(node);
            res->kind.data.store.dest  = (koopa_raw_value_t) lval->to_koopa_item();
            blocks_list.addInst(res);
            assert(res->kind.data.store.dest);
            return nullptr;
        } else {
            res->kind.tag              = KOOPA_RVT_BRANCH;
            res->kind.data.branch.cond = (koopa_raw_value_t) exp->build_value_ast(node);

            koopa_raw_basic_block_data_t * true_block  = new koopa_raw_basic_block_data_t();
            koopa_raw_basic_block_data_t * false_block = new koopa_raw_basic_block_data_t();
            koopa_raw_basic_block_data_t * end_block   = new koopa_raw_basic_block_data_t();

            res->kind.data.branch.true_bb    = true_block;
            res->kind.data.branch.false_bb   = false_block;
            res->kind.data.branch.true_args  = empty_koopa_rs(KOOPA_RSIK_VALUE);
            res->kind.data.branch.false_args = empty_koopa_rs(KOOPA_RSIK_VALUE);

            blocks_list.addInst(res);

            true_block->name    = make_char_arr("%true_" + std::to_string(branch_id));
            true_block->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
            true_block->used_by = node;

            blocks_list.addBlock(true_block);
            if (lval)
                lval->build_value_ast(empty_koopa_rs());
            blocks_list.addInst(make_jump_block(end_block));

            false_block->name    = make_char_arr("%false_" + std::to_string(branch_id));
            false_block->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
            false_block->used_by = node;

            blocks_list.addBlock(false_block);
            if (rval)
                rval->build_value_ast(empty_koopa_rs(KOOPA_RSIK_VALUE));

            blocks_list.addInst(make_jump_block(end_block));

            end_block->name    = make_char_arr("%end_" + std::to_string(branch_id));
            end_block->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
            end_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
            blocks_list.addBlock(end_block);
            return nullptr;
        }
    }

    int get_value() const override {
        return exp->get_value();
    }
};

class ExpAST : public ValueBaseAST {
public:
    std::unique_ptr<ValueBaseAST> exp;

    void Dump() const override {
        std::cout << "ExpAST { ";
        exp->Dump();
        std::cout << " }";
    }

    void * build_value_ast(const koopa_raw_slice_t & parent) const override {
        return exp->build_value_ast(parent);
    }

    int get_value() const override {
        return exp->get_value();
    }
};

class PrimaryExpAST : public ValueBaseAST {
public:
    enum class PrimaryExpType {
        Exp,
        Lval,
        Number
    } type;
    std::unique_ptr<ValueBaseAST> exp;
    int                           number;

    void Dump() const override {
        std::cout << "PrimaryExpAST { ";
        if (type == PrimaryExpType::Number)
            std::cout << number;
        else
            exp->Dump();
        std::cout << " }";
    }

    void * build_value_ast(const koopa_raw_slice_t & parent) const override {
        if (type != PrimaryExpType::Number)
            return exp->build_value_ast(parent);
        else {
            koopa_raw_value_data * res   = new koopa_raw_value_data();
            res->kind.tag                = KOOPA_RVT_INTEGER;
            res->kind.data.integer.value = number;
            res->name                    = nullptr;
            res->ty                      = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->used_by                 = parent;
            return res;
        }
    }

    int get_value() const override {
        if (type == PrimaryExpType::Number)
            return number;
        return exp->get_value();
    }
};

class UnaryExpAST : public ValueBaseAST {
public:
    enum class UnaryExpType {
        PrimaryExp,
        UnaryExp
    } type;
    std::string                   op;
    std::unique_ptr<ValueBaseAST> exp;

    void Dump() const override {
        std::cout << "UnaryExpAST { ";
        if (type == UnaryExpType::UnaryExp)
            std::cout << op << ' ';
        exp->Dump();
        std::cout << " }";
    }

    void * build_value_ast(const koopa_raw_slice_t & parent) const override {
        if (type == UnaryExpType::PrimaryExp || (type == UnaryExpType::UnaryExp && op == "+"))
            return exp->build_value_ast(parent);
        else {
            koopa_raw_value_data * res  = new koopa_raw_value_data();
            koopa_raw_slice_t      node = make_koopa_rs_single_element(res, KOOPA_RSIK_VALUE);

            koopa_raw_value_data * zero   = new koopa_raw_value_data();
            zero->kind.tag                = KOOPA_RVT_INTEGER;
            zero->kind.data.integer.value = 0;
            zero->name                    = nullptr;
            zero->ty                      = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            zero->used_by                 = node;

            res->ty       = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->name     = nullptr;
            res->used_by  = parent;
            res->kind.tag = KOOPA_RVT_BINARY;
            auto & binary = res->kind.data.binary;
            if (op == "-")
                binary.op = KOOPA_RBO_SUB;
            else if (op == "!")
                binary.op = KOOPA_RBO_EQ;

            binary.lhs = (koopa_raw_value_t) zero;
            binary.rhs = (koopa_raw_value_t) exp->build_value_ast(node);

            blocks_list.addInst(res);
            return res;
        }
    }

    int get_value() const override {
        if (type == UnaryExpType::PrimaryExp || (type == UnaryExpType::UnaryExp && op == "+"))
            return exp->get_value();
        else if (op == "-")
            return -exp->get_value();
        else
            return ! exp->get_value();
    }
};

class MulExpAST : public ValueBaseAST {
public:
    enum class MulExpType {
        Unary,
        Binary
    } type;
    std::string op;

    std::unique_ptr<ValueBaseAST> left_exp;
    std::unique_ptr<ValueBaseAST> exp;

    void Dump() const override {
        std::cout << "MulExpAST { ";
        if (type == MulExpType::Binary) {
            left_exp->Dump();
            std::cout << op;
        }
        exp->Dump();
        std::cout << " }";
    }

    void * build_value_ast(const koopa_raw_slice_t & parent) const override {
        if (type == MulExpType::Unary)
            return exp->build_value_ast(parent);
        else {
            koopa_raw_value_data * res  = new koopa_raw_value_data();
            koopa_raw_slice_t      node = make_koopa_rs_single_element(res, KOOPA_RSIK_VALUE);

            res->ty       = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->name     = nullptr;
            res->used_by  = parent;
            res->kind.tag = KOOPA_RVT_BINARY;
            auto & binary = res->kind.data.binary;
            if (op == "*")
                binary.op = KOOPA_RBO_MUL;
            else if (op == "/")
                binary.op = KOOPA_RBO_DIV;
            else if (op == "%")
                binary.op = KOOPA_RBO_MOD;

            binary.lhs = (koopa_raw_value_t) left_exp->build_value_ast(node);
            binary.rhs = (koopa_raw_value_t) exp->build_value_ast(node);

            blocks_list.addInst(res);
            return res;
        }
    }

    int get_value() const override {
        if (type == MulExpType::Unary)
            return exp->get_value();
        else if (op == "*")
            return left_exp->get_value() * exp->get_value();
        else if (op == "/")
            return left_exp->get_value() / exp->get_value();
        else
            return left_exp->get_value() % exp->get_value();
    }
};

class AddExpAST : public ValueBaseAST {
public:
    enum class AddExpType {
        Unary,
        Binary
    } type;
    std::string op;

    std::unique_ptr<ValueBaseAST> left_exp;
    std::unique_ptr<ValueBaseAST> exp;

    void Dump() const override {
        std::cout << "AddExpAST { ";
        if (type == AddExpType::Binary) {
            left_exp->Dump();
            std::cout << op;
        }
        exp->Dump();
        std::cout << " }";
    }

    void * build_value_ast(const koopa_raw_slice_t & parent) const override {
        if (type == AddExpType::Unary)
            return exp->build_value_ast(parent);
        else {
            koopa_raw_value_data * res  = new koopa_raw_value_data();
            koopa_raw_slice_t      node = make_koopa_rs_single_element(res, KOOPA_RSIK_VALUE);

            res->ty       = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->name     = nullptr;
            res->used_by  = parent;
            res->kind.tag = KOOPA_RVT_BINARY;
            auto & binary = res->kind.data.binary;
            if (op == "+")
                binary.op = KOOPA_RBO_ADD;
            else if (op == "-")
                binary.op = KOOPA_RBO_SUB;

            binary.lhs = (koopa_raw_value_t) left_exp->build_value_ast(node);
            binary.rhs = (koopa_raw_value_t) exp->build_value_ast(node);

            blocks_list.addInst(res);
            return res;
        }
    }

    int get_value() const override {
        if (type == AddExpType::Unary)
            return exp->get_value();
        else if (op == "+")
            return left_exp->get_value() + exp->get_value();
        else
            return left_exp->get_value() - exp->get_value();
    }
};

class RelExpAST : public ValueBaseAST {
public:
    enum class RelExpType {
        Unary,
        Binary
    } type;
    std::string op;

    std::unique_ptr<ValueBaseAST> left_exp;
    std::unique_ptr<ValueBaseAST> exp;

    void Dump() const override {
        std::cout << "RelExpAST { ";
        if (type == RelExpType::Binary) {
            left_exp->Dump();
            std::cout << op;
        }
        exp->Dump();
        std::cout << " }";
    }

    void * build_value_ast(const koopa_raw_slice_t & parent) const override {
        if (type == RelExpType::Unary)
            return exp->build_value_ast(parent);
        else {
            koopa_raw_value_data * res  = new koopa_raw_value_data();
            koopa_raw_slice_t      node = make_koopa_rs_single_element(res, KOOPA_RSIK_VALUE);

            res->ty       = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->name     = nullptr;
            res->used_by  = parent;
            res->kind.tag = KOOPA_RVT_BINARY;
            auto & binary = res->kind.data.binary;
            if (op == "<")
                binary.op = KOOPA_RBO_LT;
            else if (op == "<=")
                binary.op = KOOPA_RBO_LE;
            else if (op == ">")
                binary.op = KOOPA_RBO_GT;
            else if (op == ">=")
                binary.op = KOOPA_RBO_GE;

            binary.lhs = (koopa_raw_value_t) left_exp->build_value_ast(node);
            binary.rhs = (koopa_raw_value_t) exp->build_value_ast(node);

            blocks_list.addInst(res);
            return res;
        }
    }

    int get_value() const override {
        if (type == RelExpType::Unary)
            return exp->get_value();
        else if (op == "<")
            return left_exp->get_value() < exp->get_value();
        else if (op == "<=")
            return left_exp->get_value() <= exp->get_value();
        else if (op == ">")
            return left_exp->get_value() > exp->get_value();
        else
            return left_exp->get_value() >= exp->get_value();
    }
};

class EqExpAST : public ValueBaseAST {
public:
    enum class EqExpType {
        Unary,
        Binary
    } type;
    std::string op;

    std::unique_ptr<ValueBaseAST> left_exp;
    std::unique_ptr<ValueBaseAST> exp;

    void Dump() const override {
        std::cout << "EqExpAST { ";
        if (type == EqExpType::Binary) {
            left_exp->Dump();
            std::cout << op;
        }
        exp->Dump();
        std::cout << " }";
    }

    void * build_value_ast(const koopa_raw_slice_t & parent) const override {
        if (type == EqExpType::Unary)
            return exp->build_value_ast(parent);
        else {
            koopa_raw_value_data * res  = new koopa_raw_value_data();
            koopa_raw_slice_t      node = make_koopa_rs_single_element(res, KOOPA_RSIK_VALUE);

            res->ty       = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->name     = nullptr;
            res->used_by  = parent;
            res->kind.tag = KOOPA_RVT_BINARY;
            auto & binary = res->kind.data.binary;
            if (op == "==")
                binary.op = KOOPA_RBO_EQ;
            else if (op == "!=")
                binary.op = KOOPA_RBO_NOT_EQ;

            binary.lhs = (koopa_raw_value_t) left_exp->build_value_ast(node);
            binary.rhs = (koopa_raw_value_t) exp->build_value_ast(node);

            blocks_list.addInst(res);
            return res;
        }
    }

    int get_value() const override {
        if (type == EqExpType::Unary)
            return exp->get_value();
        else if (op == "==")
            return left_exp->get_value() == exp->get_value();
        else
            return left_exp->get_value() != exp->get_value();
    }
};

class LAndExpAST : public ValueBaseAST {
    koopa_raw_value_t to_bool_koopa(const koopa_raw_slice_t & parent, koopa_raw_value_t exp) const {
        koopa_raw_value_data * res  = new koopa_raw_value_data();
        koopa_raw_slice_t      node = make_koopa_rs_single_element(res, KOOPA_RSIK_VALUE);

        koopa_raw_value_data * zero   = new koopa_raw_value_data();
        zero->kind.tag                = KOOPA_RVT_INTEGER;
        zero->kind.data.integer.value = 0;
        zero->name                    = nullptr;
        zero->ty                      = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
        zero->used_by                 = node;

        res->ty       = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
        res->name     = nullptr;
        res->used_by  = parent;
        res->kind.tag = KOOPA_RVT_BINARY;
        auto & binary = res->kind.data.binary;
        binary.op     = KOOPA_RBO_NOT_EQ;
        binary.lhs    = exp;
        binary.rhs    = zero;
        blocks_list.addInst(res);
        return res;
    }

public:
    enum class LAndExpType {
        Unary,
        Binary
    } type;

    int branch_id;

    std::unique_ptr<ValueBaseAST> left_exp;
    std::unique_ptr<ValueBaseAST> exp;

    void Dump() const override {
        std::cout << "LAndExpAST { ";
        if (type == LAndExpType::Binary) {
            left_exp->Dump();
            std::cout << "&&";
        }
        exp->Dump();
        std::cout << " }";
    }

    void * build_value_ast(const koopa_raw_slice_t & parent) const override {
        if (type == LAndExpType::Unary)
            return exp->build_value_ast(parent);
        else {
            koopa_raw_value_data * t = new koopa_raw_value_data();

            t->ty       = make_int_pointer_type();
            t->name     = make_char_arr("%t_" + std::to_string(branch_id));
            t->used_by  = parent;
            t->kind.tag = KOOPA_RVT_ALLOC;
            blocks_list.addInst(t);

            koopa_raw_value_data * s = new koopa_raw_value_data();

            koopa_raw_slice_t node = make_koopa_rs_single_element(s, KOOPA_RSIK_VALUE);

            koopa_raw_value_data * zero   = new koopa_raw_value_data();
            zero->kind.tag                = KOOPA_RVT_INTEGER;
            zero->kind.data.integer.value = 0;
            zero->name                    = nullptr;
            zero->ty                      = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            zero->used_by                 = node;

            s->ty                    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
            s->name                  = nullptr;
            s->used_by               = empty_koopa_rs();
            s->kind.tag              = KOOPA_RVT_STORE;
            s->kind.data.store.dest  = t;
            s->kind.data.store.value = zero;
            blocks_list.addInst(s);

            koopa_raw_value_data * br = new koopa_raw_value_data();

            node = make_koopa_rs_single_element(br, KOOPA_RSIK_VALUE);

            br->ty                    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
            br->name                  = nullptr;
            br->used_by               = parent;
            br->kind.tag              = KOOPA_RVT_BRANCH;
            br->kind.data.branch.cond = to_bool_koopa(node, (koopa_raw_value_t) left_exp->build_value_ast(node));

            koopa_raw_basic_block_data_t * true_block = new koopa_raw_basic_block_data_t();
            koopa_raw_basic_block_data_t * end_block  = new koopa_raw_basic_block_data_t();
            br->kind.data.branch.true_bb              = true_block;
            br->kind.data.branch.false_bb             = end_block;
            br->kind.data.branch.true_args            = empty_koopa_rs(KOOPA_RSIK_VALUE);
            br->kind.data.branch.false_args           = empty_koopa_rs(KOOPA_RSIK_VALUE);
            blocks_list.addInst(br);

            true_block->name    = make_char_arr("%true_" + std::to_string(branch_id));
            true_block->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
            true_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
            blocks_list.addBlock(true_block);

            koopa_raw_value_data * b_store = new koopa_raw_value_data();

            node = make_koopa_rs_single_element(s, KOOPA_RSIK_VALUE);

            b_store->ty                    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
            b_store->name                  = nullptr;
            b_store->used_by               = empty_koopa_rs();
            b_store->kind.tag              = KOOPA_RVT_STORE;
            b_store->kind.data.store.dest  = t;
            b_store->kind.data.store.value = to_bool_koopa(node, (koopa_raw_value_t) exp->build_value_ast(node));
            blocks_list.addInst(b_store);
            blocks_list.addInst(make_jump_block(end_block));

            end_block->name    = make_char_arr("%end_" + std::to_string(branch_id));
            end_block->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
            end_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
            blocks_list.addBlock(end_block);

            koopa_raw_value_data * res = new koopa_raw_value_data();

            res->ty                 = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->name               = nullptr;
            res->used_by            = parent;
            res->kind.tag           = KOOPA_RVT_LOAD;
            res->kind.data.load.src = t;

            blocks_list.addInst(res);
            return res;
        }
    }

    int get_value() const override {
        if (type == LAndExpType::Unary)
            return exp->get_value();
        else
            return left_exp->get_value() && exp->get_value();
    }
};

class LOrExpAST : public ValueBaseAST {
    koopa_raw_value_t to_bool_koopa(const koopa_raw_slice_t & parent, koopa_raw_value_t exp, bool eq) const {
        koopa_raw_value_data * res  = new koopa_raw_value_data();
        koopa_raw_slice_t      node = make_koopa_rs_single_element(res, KOOPA_RSIK_VALUE);

        koopa_raw_value_data * zero   = new koopa_raw_value_data();
        zero->kind.tag                = KOOPA_RVT_INTEGER;
        zero->kind.data.integer.value = 0;
        zero->name                    = nullptr;
        zero->ty                      = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
        zero->used_by                 = node;

        res->ty       = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
        res->name     = nullptr;
        res->used_by  = parent;
        res->kind.tag = KOOPA_RVT_BINARY;
        auto & binary = res->kind.data.binary;
        if (eq)
            binary.op = KOOPA_RBO_EQ;
        else
            binary.op = KOOPA_RBO_NOT_EQ;
        binary.lhs = exp;
        binary.rhs = zero;
        blocks_list.addInst(res);
        return res;
    }

public:
    enum class LOrExpType {
        Unary,
        Binary
    } type;

    std::unique_ptr<ValueBaseAST> left_exp;
    std::unique_ptr<ValueBaseAST> exp;

    int branch_id;

    void Dump() const override {
        std::cout << "LOrExpAST { ";
        if (type == LOrExpType::Binary) {
            left_exp->Dump();
            std::cout << "||";
        }
        exp->Dump();
        std::cout << " }";
    }

    void * build_value_ast(const koopa_raw_slice_t & parent) const override {
        if (type == LOrExpType::Unary)
            return exp->build_value_ast(parent);
        else {
            koopa_raw_value_data * t = new koopa_raw_value_data();

            t->ty       = make_int_pointer_type();
            t->name     = make_char_arr("%t_" + std::to_string(branch_id));
            t->used_by  = parent;
            t->kind.tag = KOOPA_RVT_ALLOC;
            blocks_list.addInst(t);

            koopa_raw_value_data * s = new koopa_raw_value_data();

            koopa_raw_slice_t node = make_koopa_rs_single_element(s, KOOPA_RSIK_VALUE);

            koopa_raw_value_data * one   = new koopa_raw_value_data();
            one->kind.tag                = KOOPA_RVT_INTEGER;
            one->kind.data.integer.value = 1;
            one->name                    = nullptr;
            one->ty                      = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            one->used_by                 = node;

            s->ty                    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
            s->name                  = nullptr;
            s->used_by               = empty_koopa_rs();
            s->kind.tag              = KOOPA_RVT_STORE;
            s->kind.data.store.dest  = t;
            s->kind.data.store.value = one;
            blocks_list.addInst(s);

            koopa_raw_value_data * br = new koopa_raw_value_data();

            node = make_koopa_rs_single_element(br, KOOPA_RSIK_VALUE);

            br->ty                    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
            br->name                  = nullptr;
            br->used_by               = parent;
            br->kind.tag              = KOOPA_RVT_BRANCH;
            br->kind.data.branch.cond = to_bool_koopa(node, (koopa_raw_value_t) left_exp->build_value_ast(node), true);

            koopa_raw_basic_block_data_t * true_block = new koopa_raw_basic_block_data_t();
            koopa_raw_basic_block_data_t * end_block  = new koopa_raw_basic_block_data_t();
            br->kind.data.branch.true_bb              = true_block;
            br->kind.data.branch.false_bb             = end_block;
            br->kind.data.branch.true_args            = empty_koopa_rs(KOOPA_RSIK_VALUE);
            br->kind.data.branch.false_args           = empty_koopa_rs(KOOPA_RSIK_VALUE);
            blocks_list.addInst(br);

            true_block->name    = make_char_arr("%true_" + std::to_string(branch_id));
            true_block->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
            true_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
            blocks_list.addBlock(true_block);

            koopa_raw_value_data * b_store = new koopa_raw_value_data();

            node = make_koopa_rs_single_element(s, KOOPA_RSIK_VALUE);

            b_store->ty                    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
            b_store->name                  = nullptr;
            b_store->used_by               = empty_koopa_rs();
            b_store->kind.tag              = KOOPA_RVT_STORE;
            b_store->kind.data.store.dest  = t;
            b_store->kind.data.store.value = to_bool_koopa(node, (koopa_raw_value_t) exp->build_value_ast(node), false);
            blocks_list.addInst(b_store);
            blocks_list.addInst(make_jump_block(end_block));

            end_block->name    = make_char_arr("%end_" + std::to_string(branch_id));
            end_block->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
            end_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
            blocks_list.addBlock(end_block);

            koopa_raw_value_data * res = new koopa_raw_value_data();

            res->ty                 = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->name               = nullptr;
            res->used_by            = parent;
            res->kind.tag           = KOOPA_RVT_LOAD;
            res->kind.data.load.src = t;

            blocks_list.addInst(res);
            return res;
        }
    }

    int get_value() const override {
        if (type == LOrExpType::Unary)
            return exp->get_value();
        else
            return left_exp->get_value() || exp->get_value();
    }
};

class ConstDefAST : public ValueBaseAST {
public:
    std::string                   name;
    std::unique_ptr<ValueBaseAST> exp;

    void Dump() const override {
        std::cout << "ConstDefAST { " << name;
        std::cout << " = ";
        exp->Dump();
        std::cout << " }";
    }

    void * build_value_ast(const koopa_raw_slice_t & parent) const override {
        koopa_raw_value_data * res = new koopa_raw_value_data();

        res->ty                      = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
        res->name                    = nullptr;
        res->used_by                 = parent;
        res->kind.tag                = KOOPA_RVT_INTEGER;
        res->kind.data.integer.value = exp->get_value();

        symbol_list.addSymbol(name, { LValSymbol::SymbolType::Const, res });
        return res;
    }

    int get_value() const override {
        return exp->get_value();
    }
};

class VarDefAST : public ValueBaseAST {
public:
    std::string                   name;
    std::unique_ptr<ValueBaseAST> exp;

    void Dump() const override {
        std::cout << "VarDefAST { " << name;
        if (exp) {
            std::cout << " = ";
            exp->Dump();
        }
        std::cout << " }";
    }

    void * build_value_ast(const koopa_raw_slice_t & parent) const override {
        koopa_raw_value_data * res = new koopa_raw_value_data();

        koopa_raw_slice_t node = make_koopa_rs_single_element(res, KOOPA_RSIK_VALUE);

        res->ty = make_int_pointer_type();

        res->name     = make_char_arr("@" + name);
        res->used_by  = parent;
        res->kind.tag = KOOPA_RVT_ALLOC;

        blocks_list.addInst(res);

        symbol_list.addSymbol(name, { LValSymbol::SymbolType::Var, res });

        if (exp) {
            koopa_raw_value_data * store = new koopa_raw_value_data();

            store->ty                    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
            store->name                  = nullptr;
            store->used_by               = empty_koopa_rs();
            store->kind.tag              = KOOPA_RVT_STORE;
            store->kind.data.store.dest  = res;
            store->kind.data.store.value = (koopa_raw_value_t) exp->build_value_ast(node);

            blocks_list.addInst(store);
        }
        return res;
    }

    int get_value() const override {
        return exp->get_value();
    }
};

class LValAST : public ValueBaseAST {
public:
    std::string name;

    void Dump() const override {
        std::cout << "LValAST { " << name;
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        return (void *) symbol_list.getSymbol(name).number;
    }

    void * build_value_ast(const koopa_raw_slice_t & parent) const override {
        koopa_raw_value_data * res = new koopa_raw_value_data();

        auto var = symbol_list.getSymbol(name);
        if (var.type == LValSymbol::SymbolType::Const)
            return (void *) var.number;
        else if (var.type == LValSymbol::SymbolType::Var) {
            res->ty                 = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->name               = nullptr;
            res->used_by            = parent;
            res->kind.tag           = KOOPA_RVT_LOAD;
            res->kind.data.load.src = var.number;
            blocks_list.addInst(res);
        }

        return res;
    }

    int get_value() const override {
        return ((koopa_raw_value_t) to_koopa_item())->kind.data.integer.value;
    }
};

// // Number
// class NumberAST : public BaseAST {
//     public:
//     int int_const;

//     void Dump() const override {
//         std::cout << int_const;
//     }
// }

// CompUnitAST { FuncDefAST { FuncTypeAST { int }, main, BlockAST { StmtAST { 0 } } } }

/*CompUnit {
    func_def: FuncDef {
        func_type: Int,
        ident: "main",
        block: Block {
            stmt: Stmt {
                num: 0,
            },
        },
    },
}*/