#pragma once

#include <assert.h>
#include <iostream>
#include <memory>
#include <vector>

#include "block.h"
#include "koopa_util.h"
#include "symbol_list.h"
#include "while_container.h"

static char * make_char_arr(std::string str) {
    size_t n   = str.length();
    char * res = new char(n + 1);
    str.copy(res, n + 1);
    res[n] = 0;
    return res;
}

static koopa_raw_value_t make_number_koopa(int number) {
    koopa_raw_value_data * res = new koopa_raw_value_data();

    res->ty                      = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
    res->name                    = nullptr;
    res->used_by                 = empty_koopa_rs(KOOPA_RSIK_VALUE);
    res->kind.tag                = KOOPA_RVT_INTEGER;
    res->kind.data.integer.value = number;
    return res;
}

// 所有 AST 的基类
class BaseAST {
public:
    static SymbolList     symbol_list;
    static BlockMaker     blocks_list;
    static LoopMaintainer loop_list;

    virtual ~BaseAST() = default;

    virtual void Dump() const {}

    virtual void * to_koopa_item() const {
        std::cout << "Run into ERROR\n";
        return nullptr;
    }
};

class ValueBaseAST : public BaseAST {
public:
    virtual int get_value() const {
        std::cout << "Run into ERROR\n";
        return 0;
    }
};

class LValueBaseAST : public ValueBaseAST {
public:
    virtual void * build_left_value() const {
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

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::string              ident;
    std::unique_ptr<BaseAST> block;

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

        block->to_koopa_item();

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
class BlockAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST>> insts;

    void Dump() const override {
        std::cout << "BlockAST { ";
        for (const auto & it : insts)
            it->Dump();
        std::cout << " }";
    }

    virtual void * to_koopa_item() const override {
        symbol_list.newEnv();

        for (const auto & it : insts)
            it->to_koopa_item();

        symbol_list.deleteEnv();

        return nullptr;
    }
};

class AssignStmtAST : public BaseAST {
public:
    std::unique_ptr<LValueBaseAST> lval;

    std::unique_ptr<BaseAST> exp;

    void Dump() const override {
        std::cout << "AssignStmtAST { ";
        lval->Dump();
        std::cout << " = ";
        exp->Dump();
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        koopa_raw_value_data * res = new koopa_raw_value_data();

        res->ty      = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
        res->name    = nullptr;
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);

        res->kind.tag = KOOPA_RVT_STORE;

        res->kind.data.store.value = (koopa_raw_value_t) exp->to_koopa_item();
        res->kind.data.store.dest  = (koopa_raw_value_t) lval->build_left_value();
        blocks_list.addInst(res);
        assert(res->kind.data.store.dest);
        return nullptr;
    }
};

class ReturnStmtAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;

    void Dump() const override {
        std::cout << "ReturnStmtAST { ";
        exp->Dump();
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        koopa_raw_value_data * res = new koopa_raw_value_data();

        res->ty      = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
        res->name    = nullptr;
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);

        res->kind.tag = KOOPA_RVT_RETURN;

        res->kind.data.ret.value = (koopa_raw_value_t) exp->to_koopa_item();
        blocks_list.addInst(res);
        return res;
    }
};

// Stmt
class BranchStmtAST : public BaseAST {
public:
    int branch_id;

    std::unique_ptr<BaseAST> lval;

    std::unique_ptr<BaseAST> exp;

    std::unique_ptr<BaseAST> rval;

    void Dump() const override {
        std::cout << "StmtAST { ";
        std::cout << "if ( ";
        exp->Dump();
        std::cout << " ) ";
        if (lval)
            lval->Dump();
        if (rval) {
            std::cout << " else ";
            rval->Dump();
        }
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        koopa_raw_value_data * res = new koopa_raw_value_data();

        res->ty                    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
        res->name                  = nullptr;
        res->used_by               = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag              = KOOPA_RVT_BRANCH;
        res->kind.data.branch.cond = (koopa_raw_value_t) exp->to_koopa_item();

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
        true_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);

        blocks_list.addBlock(true_block);
        if (lval)
            lval->to_koopa_item();
        blocks_list.addInst(make_jump_block(end_block));

        false_block->name    = make_char_arr("%false_" + std::to_string(branch_id));
        false_block->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
        false_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);

        blocks_list.addBlock(false_block);
        if (rval)
            rval->to_koopa_item();

        blocks_list.addInst(make_jump_block(end_block));

        end_block->name    = make_char_arr("%end_" + std::to_string(branch_id));
        end_block->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
        end_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        blocks_list.addBlock(end_block);
        return nullptr;
    }
};

class WhileStmtAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;

    std::unique_ptr<BaseAST> stmt;

    int while_id;

    void Dump() const override {
        std::cout << "WhileStmtAST { ";
        std::cout << "While ( ";
        exp->Dump();
        std::cout << ") ";
        if (stmt)
            stmt->Dump();
        std::cout << " }";
    }

    virtual void * to_koopa_item() const override {
        koopa_raw_basic_block_data_t * while_entry = new koopa_raw_basic_block_data_t();
        koopa_raw_basic_block_data_t * while_body  = new koopa_raw_basic_block_data_t();
        koopa_raw_basic_block_data_t * end_block   = new koopa_raw_basic_block_data_t();

        loop_list.add(while_entry, while_body, end_block);

        blocks_list.addInst(make_jump_block(while_entry));

        while_entry->name    = make_char_arr("%while_entry_" + std::to_string(while_id));
        while_entry->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
        while_entry->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        blocks_list.addBlock(while_entry);

        koopa_raw_value_data * br       = new koopa_raw_value_data();
        br->ty                          = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
        br->name                        = nullptr;
        br->used_by                     = empty_koopa_rs(KOOPA_RSIK_VALUE);
        br->kind.tag                    = KOOPA_RVT_BRANCH;
        br->kind.data.branch.cond       = (koopa_raw_value_t) exp->to_koopa_item();
        br->kind.data.branch.true_bb    = while_body;
        br->kind.data.branch.false_bb   = end_block;
        br->kind.data.branch.true_args  = empty_koopa_rs(KOOPA_RSIK_VALUE);
        br->kind.data.branch.false_args = empty_koopa_rs(KOOPA_RSIK_VALUE);
        blocks_list.addInst(br);

        while_body->name    = make_char_arr("%while_body_" + std::to_string(while_id));
        while_body->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
        while_body->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        blocks_list.addBlock(while_body);
        if (stmt)
            stmt->to_koopa_item();
        blocks_list.addInst(make_jump_block(while_entry));

        end_block->name    = make_char_arr("%end_" + std::to_string(while_id));
        end_block->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
        end_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        blocks_list.addBlock(end_block);

        loop_list.pop();
        return nullptr;
    }
};

class BreakAST : public BaseAST {
public:
    void * to_koopa_item() const override {
        blocks_list.addInst(make_jump_block(loop_list.get().end_block));
        return nullptr;
    }
};

class ContinueAST : public BaseAST {
public:
    void * to_koopa_item() const override {
        blocks_list.addInst(make_jump_block(loop_list.get().while_entry));
        return nullptr;
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

    void * to_koopa_item() const override {
        return exp->to_koopa_item();
    }

    int get_value() const override {
        return exp->get_value();
    }
};

class LValAST : public LValueBaseAST {
public:
    std::string name;

    void Dump() const override {
        std::cout << "LValAST { " << name;
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        koopa_raw_value_data * res = new koopa_raw_value_data();

        auto var = symbol_list.getSymbol(name);
        if (var.type == LValSymbol::SymbolType::Const)
            return (void *) var.number;
        else if (var.type == LValSymbol::SymbolType::Var) {
            res->ty                 = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->name               = nullptr;
            res->used_by            = empty_koopa_rs(KOOPA_RSIK_VALUE);
            res->kind.tag           = KOOPA_RVT_LOAD;
            res->kind.data.load.src = var.number;
            blocks_list.addInst(res);
        }

        return res;
    }

    int get_value() const override {
        return ((koopa_raw_value_t) build_left_value())->kind.data.integer.value;
    }

    void * build_left_value() const override {
        return (void *) symbol_list.getSymbol(name).number;
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

    void * to_koopa_item() const override {
        if (type != PrimaryExpType::Number)
            return exp->to_koopa_item();
        else
            return (void *) make_number_koopa(number);
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

    void * to_koopa_item() const override {
        if (type == UnaryExpType::PrimaryExp || (type == UnaryExpType::UnaryExp && op == "+"))
            return exp->to_koopa_item();
        else {
            koopa_raw_value_data * res = new koopa_raw_value_data();

            res->ty       = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->name     = nullptr;
            res->used_by  = empty_koopa_rs(KOOPA_RSIK_VALUE);
            res->kind.tag = KOOPA_RVT_BINARY;
            auto & binary = res->kind.data.binary;
            if (op == "-")
                binary.op = KOOPA_RBO_SUB;
            else if (op == "!")
                binary.op = KOOPA_RBO_EQ;

            binary.lhs = make_number_koopa(0);
            binary.rhs = (koopa_raw_value_t) exp->to_koopa_item();

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

    void * to_koopa_item() const override {
        if (type == MulExpType::Unary)
            return exp->to_koopa_item();
        else {
            koopa_raw_value_data * res = new koopa_raw_value_data();

            res->ty       = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->name     = nullptr;
            res->used_by  = empty_koopa_rs(KOOPA_RSIK_VALUE);
            res->kind.tag = KOOPA_RVT_BINARY;
            auto & binary = res->kind.data.binary;
            if (op == "*")
                binary.op = KOOPA_RBO_MUL;
            else if (op == "/")
                binary.op = KOOPA_RBO_DIV;
            else if (op == "%")
                binary.op = KOOPA_RBO_MOD;

            binary.lhs = (koopa_raw_value_t) left_exp->to_koopa_item();
            binary.rhs = (koopa_raw_value_t) exp->to_koopa_item();

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

    void * to_koopa_item() const override {
        if (type == AddExpType::Unary)
            return exp->to_koopa_item();
        else {
            koopa_raw_value_data * res = new koopa_raw_value_data();

            res->ty       = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->name     = nullptr;
            res->used_by  = empty_koopa_rs(KOOPA_RSIK_VALUE);
            res->kind.tag = KOOPA_RVT_BINARY;
            auto & binary = res->kind.data.binary;
            if (op == "+")
                binary.op = KOOPA_RBO_ADD;
            else if (op == "-")
                binary.op = KOOPA_RBO_SUB;

            binary.lhs = (koopa_raw_value_t) left_exp->to_koopa_item();
            binary.rhs = (koopa_raw_value_t) exp->to_koopa_item();

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

    void * to_koopa_item() const override {
        if (type == RelExpType::Unary)
            return exp->to_koopa_item();
        else {
            koopa_raw_value_data * res = new koopa_raw_value_data();

            res->ty       = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->name     = nullptr;
            res->used_by  = empty_koopa_rs(KOOPA_RSIK_VALUE);
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

            binary.lhs = (koopa_raw_value_t) left_exp->to_koopa_item();
            binary.rhs = (koopa_raw_value_t) exp->to_koopa_item();

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

    void * to_koopa_item() const override {
        if (type == EqExpType::Unary)
            return exp->to_koopa_item();
        else {
            koopa_raw_value_data * res = new koopa_raw_value_data();

            res->ty       = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->name     = nullptr;
            res->used_by  = empty_koopa_rs(KOOPA_RSIK_VALUE);
            res->kind.tag = KOOPA_RVT_BINARY;
            auto & binary = res->kind.data.binary;
            if (op == "==")
                binary.op = KOOPA_RBO_EQ;
            else if (op == "!=")
                binary.op = KOOPA_RBO_NOT_EQ;

            binary.lhs = (koopa_raw_value_t) left_exp->to_koopa_item();
            binary.rhs = (koopa_raw_value_t) exp->to_koopa_item();

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
    koopa_raw_value_t to_bool_koopa(koopa_raw_value_t exp) const {
        koopa_raw_value_data * res = new koopa_raw_value_data();

        res->ty       = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
        res->name     = nullptr;
        res->used_by  = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag = KOOPA_RVT_BINARY;
        auto & binary = res->kind.data.binary;
        binary.op     = KOOPA_RBO_NOT_EQ;
        binary.lhs    = exp;
        binary.rhs    = make_number_koopa(0);
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

    void * to_koopa_item() const override {
        if (type == LAndExpType::Unary)
            return exp->to_koopa_item();
        else {
            koopa_raw_value_data * t = new koopa_raw_value_data();

            t->ty       = make_int_pointer_type();
            t->name     = make_char_arr("%t_" + std::to_string(branch_id));
            t->used_by  = empty_koopa_rs(KOOPA_RSIK_VALUE);
            t->kind.tag = KOOPA_RVT_ALLOC;
            blocks_list.addInst(t);

            koopa_raw_value_data * s = new koopa_raw_value_data();

            s->ty                    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
            s->name                  = nullptr;
            s->used_by               = empty_koopa_rs();
            s->kind.tag              = KOOPA_RVT_STORE;
            s->kind.data.store.dest  = t;
            s->kind.data.store.value = make_number_koopa(0);
            blocks_list.addInst(s);

            koopa_raw_value_data * br = new koopa_raw_value_data();

            br->ty                    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
            br->name                  = nullptr;
            br->used_by               = empty_koopa_rs(KOOPA_RSIK_VALUE);
            br->kind.tag              = KOOPA_RVT_BRANCH;
            br->kind.data.branch.cond = to_bool_koopa((koopa_raw_value_t) left_exp->to_koopa_item());

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

            b_store->ty                    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
            b_store->name                  = nullptr;
            b_store->used_by               = empty_koopa_rs();
            b_store->kind.tag              = KOOPA_RVT_STORE;
            b_store->kind.data.store.dest  = t;
            b_store->kind.data.store.value = to_bool_koopa((koopa_raw_value_t) exp->to_koopa_item());
            blocks_list.addInst(b_store);
            blocks_list.addInst(make_jump_block(end_block));

            end_block->name    = make_char_arr("%end_" + std::to_string(branch_id));
            end_block->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
            end_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
            blocks_list.addBlock(end_block);

            koopa_raw_value_data * res = new koopa_raw_value_data();

            res->ty                 = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->name               = nullptr;
            res->used_by            = empty_koopa_rs(KOOPA_RSIK_VALUE);
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
    koopa_raw_value_t to_bool_koopa(koopa_raw_value_t exp, bool eq) const {
        koopa_raw_value_data * res = new koopa_raw_value_data();

        res->ty       = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
        res->name     = nullptr;
        res->used_by  = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag = KOOPA_RVT_BINARY;
        auto & binary = res->kind.data.binary;
        if (eq)
            binary.op = KOOPA_RBO_EQ;
        else
            binary.op = KOOPA_RBO_NOT_EQ;
        binary.lhs = exp;
        binary.rhs = make_number_koopa(0);
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

    void * to_koopa_item() const override {
        if (type == LOrExpType::Unary)
            return exp->to_koopa_item();
        else {
            koopa_raw_value_data * t = new koopa_raw_value_data();

            t->ty       = make_int_pointer_type();
            t->name     = make_char_arr("%t_" + std::to_string(branch_id));
            t->used_by  = empty_koopa_rs(KOOPA_RSIK_VALUE);
            t->kind.tag = KOOPA_RVT_ALLOC;
            blocks_list.addInst(t);

            koopa_raw_value_data * s = new koopa_raw_value_data();

            s->ty                    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
            s->name                  = nullptr;
            s->used_by               = empty_koopa_rs();
            s->kind.tag              = KOOPA_RVT_STORE;
            s->kind.data.store.dest  = t;
            s->kind.data.store.value = make_number_koopa(1);
            blocks_list.addInst(s);

            koopa_raw_value_data * br = new koopa_raw_value_data();

            br->ty                    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
            br->name                  = nullptr;
            br->used_by               = empty_koopa_rs(KOOPA_RSIK_VALUE);
            br->kind.tag              = KOOPA_RVT_BRANCH;
            br->kind.data.branch.cond = to_bool_koopa((koopa_raw_value_t) left_exp->to_koopa_item(), true);

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

            b_store->ty                    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
            b_store->name                  = nullptr;
            b_store->used_by               = empty_koopa_rs();
            b_store->kind.tag              = KOOPA_RVT_STORE;
            b_store->kind.data.store.dest  = t;
            b_store->kind.data.store.value = to_bool_koopa((koopa_raw_value_t) exp->to_koopa_item(), false);
            blocks_list.addInst(b_store);
            blocks_list.addInst(make_jump_block(end_block));

            end_block->name    = make_char_arr("%end_" + std::to_string(branch_id));
            end_block->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
            end_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
            blocks_list.addBlock(end_block);

            koopa_raw_value_data * res = new koopa_raw_value_data();

            res->ty                 = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->name               = nullptr;
            res->used_by            = empty_koopa_rs(KOOPA_RSIK_VALUE);
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

class ConstDefAST : public BaseAST {
public:
    std::string                   name;
    std::unique_ptr<ValueBaseAST> exp;

    void Dump() const override {
        std::cout << "ConstDefAST { " << name;
        std::cout << " = ";
        exp->Dump();
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        koopa_raw_value_data * res = new koopa_raw_value_data();

        res->ty                      = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
        res->name                    = nullptr;
        res->used_by                 = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag                = KOOPA_RVT_INTEGER;
        res->kind.data.integer.value = exp->get_value();

        symbol_list.addSymbol(name, { LValSymbol::SymbolType::Const, res });
        return res;
    }
};

class VarDefAST : public BaseAST {
public:
    std::string              name;
    std::unique_ptr<BaseAST> exp;

    void Dump() const override {
        std::cout << "VarDefAST { " << name;
        if (exp) {
            std::cout << " = ";
            exp->Dump();
        }
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        koopa_raw_value_data * res = new koopa_raw_value_data();

        res->ty = make_int_pointer_type();

        res->name     = make_char_arr("@" + name);
        res->used_by  = empty_koopa_rs(KOOPA_RSIK_VALUE);
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
            store->kind.data.store.value = (koopa_raw_value_t) exp->to_koopa_item();

            blocks_list.addInst(store);
        }
        return res;
    }
};
