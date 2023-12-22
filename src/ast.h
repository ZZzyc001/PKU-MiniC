#pragma once

#include <iostream>

#include "koopa_util.h"

// 所有 AST 的基类
class BaseAST {
public:
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
        blocks.push_back(block->to_koopa_item());
        res->bbs = make_koopa_rs_from_vector(blocks, KOOPA_RSIK_BASIC_BLOCK);

        char * tname = new char(ident.length() + 1);
        ("@" + ident).copy(tname, sizeof(tname));
        res->name = tname;

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

class ValueBaseAST : public BaseAST {
public:
    virtual void * build_value_ast(std::vector<const void *> & buf, const koopa_raw_slice_t & parent) const {
        std::cout << "Run into ERROR\n";
        return nullptr;
    }
};

// Block
class BlockAST : public BaseAST {
public:
    std::unique_ptr<ValueBaseAST> stmt;

    void Dump() const override {
        std::cout << "BlockAST { ";
        stmt->Dump();
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        koopa_raw_basic_block_data_t * res = new koopa_raw_basic_block_data_t();

        std::vector<const void *> stmts;
        koopa_raw_slice_t         node = empty_koopa_rs();
        stmt->build_value_ast(stmts, node);

        res->insts = make_koopa_rs_from_vector(stmts, KOOPA_RSIK_VALUE);

        res->name    = "%entry";
        res->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);

        return res;
    }
};

// Stmt
class StmtAST : public ValueBaseAST {
public:
    std::unique_ptr<ValueBaseAST> exp;

    void Dump() const override {
        std::cout << "StmtAST { ";
        exp->Dump();
        std::cout << " }";
    }

    void * build_value_ast(std::vector<const void *> & buf, const koopa_raw_slice_t & parent) const override {
        koopa_raw_value_data * res  = new koopa_raw_value_data();
        koopa_raw_slice_t      node = make_koopa_rs_single_element(res, KOOPA_RSIK_VALUE);

        res->kind.tag = KOOPA_RVT_RETURN;

        res->kind.data.ret.value = (koopa_raw_value_t) exp->build_value_ast(buf, node);

        res->ty      = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
        res->name    = nullptr;
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);

        buf.push_back(res);
        return res;
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

    void * build_value_ast(std::vector<const void *> & buf, const koopa_raw_slice_t & parent) const override {
        return exp->build_value_ast(buf, parent);
    }
};

class PrimaryExpAST : public ValueBaseAST {
public:
    enum class PrimaryExpType {
        Exp,
        Number
    } type;
    std::unique_ptr<ValueBaseAST> exp;
    int                           number;

    void Dump() const override {
        std::cout << "PrimaryExpAST { ";
        if (type == PrimaryExpType::Exp)
            exp->Dump();
        else
            std::cout << number;
        std::cout << " }";
    }

    void * build_value_ast(std::vector<const void *> & buf, const koopa_raw_slice_t & parent) const override {
        if (type == PrimaryExpType::Exp)
            return exp->build_value_ast(buf, parent);
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

    void * build_value_ast(std::vector<const void *> & buf, const koopa_raw_slice_t & parent) const override {
        if (type == UnaryExpType::PrimaryExp || (type == UnaryExpType::UnaryExp && op == "+"))
            return exp->build_value_ast(buf, parent);
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
            binary.rhs = (koopa_raw_value_t) exp->build_value_ast(buf, node);

            buf.push_back(res);
            return res;
        }
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

    void * build_value_ast(std::vector<const void *> & buf, const koopa_raw_slice_t & parent) const override {
        if (type == MulExpType::Unary)
            return exp->build_value_ast(buf, parent);
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

            binary.lhs = (koopa_raw_value_t) left_exp->build_value_ast(buf, node);
            binary.rhs = (koopa_raw_value_t) exp->build_value_ast(buf, node);

            buf.push_back(res);
            return res;
        }
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

    void * build_value_ast(std::vector<const void *> & buf, const koopa_raw_slice_t & parent) const override {
        if (type == AddExpType::Unary)
            return exp->build_value_ast(buf, parent);
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

            binary.lhs = (koopa_raw_value_t) left_exp->build_value_ast(buf, node);
            binary.rhs = (koopa_raw_value_t) exp->build_value_ast(buf, node);

            buf.push_back(res);
            return res;
        }
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