#pragma once

#include <iostream>

#include "koopa_util.h"

// 所有 AST 的基类
class BaseAST {
public:
    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;

    virtual void * to_koopa_item() const = 0;
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

// Block
class BlockAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override {
        std::cout << "BlockAST { ";
        stmt->Dump();
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        koopa_raw_basic_block_data_t * res = new koopa_raw_basic_block_data_t();

        std::vector<const void *> stmts;
        stmts.push_back(stmt->to_koopa_item());
        res->insts = make_koopa_rs_from_vector(stmts, KOOPA_RSIK_VALUE);

        res->name    = "%entry";
        res->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);

        return res;
    }
};

// Stmt
class StmtAST : public BaseAST {
public:
    int number;

    void Dump() const override {
        std::cout << "StmtAST { ";
        std::cout << number;
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        koopa_raw_value_data * res = new koopa_raw_value_data();

        koopa_raw_value_data * ret   = new koopa_raw_value_data();
        ret->kind.tag                = KOOPA_RVT_INTEGER;
        ret->kind.data.integer.value = number;
        ret->name                    = nullptr;
        ret->ty                      = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
        ret->used_by                 = empty_koopa_rs(KOOPA_RSIK_VALUE);

        res->kind.tag            = KOOPA_RVT_RETURN;
        res->kind.data.ret.value = ret;

        res->ty      = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
        res->name    = nullptr;
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);
        return res;
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