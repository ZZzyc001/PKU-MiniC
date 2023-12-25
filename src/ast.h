#pragma once

#include <iostream>
#include <memory>
#include <vector>

#include "koopa_util.h"

#include "symbol_list.h"

// 所有 AST 的基类
class BaseAST {
public:
    static SymbolList symbol_list;

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

    virtual int get_value() const {
        std::cout << "Run into ERROR\n";
        return 0;
    }
};

// Block
class BlockAST : public BaseAST {
public:
    std::vector<std::unique_ptr<ValueBaseAST>> insts;

    void Dump() const override {
        std::cout << "BlockAST { ";
        for (const auto & it : insts)
            it->Dump();
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        symbol_list.newEnv();

        koopa_raw_basic_block_data_t * res = new koopa_raw_basic_block_data_t();

        std::vector<const void *> stmts;
        koopa_raw_slice_t         node = empty_koopa_rs();

        for (const auto & it : insts)
            it->build_value_ast(stmts, node);

        res->insts = make_koopa_rs_from_vector(stmts, KOOPA_RSIK_VALUE);

        res->name    = "%entry";
        res->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);

        symbol_list.deleteEnv();

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

    void * build_value_ast(std::vector<const void *> & buf, const koopa_raw_slice_t & parent) const override {
        return exp->build_value_ast(buf, parent);
    }

    int get_value() const override {
        return exp->get_value();
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

    int get_value() const override {
        if (type == PrimaryExpType::Exp)
            return exp->get_value();
        return number;
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

    void * build_value_ast(std::vector<const void *> & buf, const koopa_raw_slice_t & parent) const override {
        if (type == RelExpType::Unary)
            return exp->build_value_ast(buf, parent);
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

            binary.lhs = (koopa_raw_value_t) left_exp->build_value_ast(buf, node);
            binary.rhs = (koopa_raw_value_t) exp->build_value_ast(buf, node);

            buf.push_back(res);
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

    void * build_value_ast(std::vector<const void *> & buf, const koopa_raw_slice_t & parent) const override {
        if (type == EqExpType::Unary)
            return exp->build_value_ast(buf, parent);
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

            binary.lhs = (koopa_raw_value_t) left_exp->build_value_ast(buf, node);
            binary.rhs = (koopa_raw_value_t) exp->build_value_ast(buf, node);

            buf.push_back(res);
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
    void * to_bool_koopa(std::vector<const void *> & buf, const koopa_raw_slice_t & parent, koopa_raw_value_t exp) const {
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
        buf.push_back(res);
        return res;
    }

public:
    enum class LAndExpType {
        Unary,
        Binary
    } type;

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

    void * build_value_ast(std::vector<const void *> & buf, const koopa_raw_slice_t & parent) const override {
        if (type == LAndExpType::Unary)
            return exp->build_value_ast(buf, parent);
        else {
            koopa_raw_value_data * res  = new koopa_raw_value_data();
            koopa_raw_slice_t      node = make_koopa_rs_single_element(res, KOOPA_RSIK_VALUE);

            res->ty       = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->name     = nullptr;
            res->used_by  = parent;
            res->kind.tag = KOOPA_RVT_BINARY;
            auto & binary = res->kind.data.binary;
            binary.op     = KOOPA_RBO_AND;

            binary.lhs = (koopa_raw_value_t) to_bool_koopa(buf, node, (koopa_raw_value_t) left_exp->build_value_ast(buf, node));
            binary.rhs = (koopa_raw_value_t) to_bool_koopa(buf, node, (koopa_raw_value_t) exp->build_value_ast(buf, node));

            buf.push_back(res);
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
    void * to_bool_koopa(std::vector<const void *> & buf, const koopa_raw_slice_t & parent, koopa_raw_value_t exp) const {
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
        buf.push_back(res);
        return res;
    }

public:
    enum class LOrExpType {
        Unary,
        Binary
    } type;

    std::unique_ptr<ValueBaseAST> left_exp;
    std::unique_ptr<ValueBaseAST> exp;

    void Dump() const override {
        std::cout << "LOrExpAST { ";
        if (type == LOrExpType::Binary) {
            left_exp->Dump();
            std::cout << "||";
        }
        exp->Dump();
        std::cout << " }";
    }

    void * build_value_ast(std::vector<const void *> & buf, const koopa_raw_slice_t & parent) const override {
        if (type == LOrExpType::Unary)
            return exp->build_value_ast(buf, parent);
        else {
            koopa_raw_value_data * res  = new koopa_raw_value_data();
            koopa_raw_slice_t      node = make_koopa_rs_single_element(res, KOOPA_RSIK_VALUE);

            res->ty       = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
            res->name     = nullptr;
            res->used_by  = parent;
            res->kind.tag = KOOPA_RVT_BINARY;
            auto & binary = res->kind.data.binary;
            binary.op     = KOOPA_RBO_OR;

            binary.lhs = (koopa_raw_value_t) to_bool_koopa(buf, node, (koopa_raw_value_t) left_exp->build_value_ast(buf, node));
            binary.rhs = (koopa_raw_value_t) to_bool_koopa(buf, node, (koopa_raw_value_t) exp->build_value_ast(buf, node));

            buf.push_back(res);
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

    void * build_value_ast(std::vector<const void *> & buf, const koopa_raw_slice_t & parent) const override {
        koopa_raw_value_data * res = new koopa_raw_value_data();

        res->ty                      = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
        res->name                    = nullptr;
        res->used_by                 = parent;
        res->kind.tag                = KOOPA_RVT_INTEGER;
        res->kind.data.integer.value = exp->get_value();

        symbol_list.addSymbol(name, res);
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
        return symbol_list.getSymbol(name);
    }

    void * build_value_ast(std::vector<const void *> & buf, const koopa_raw_slice_t & parent) const override {
        return to_koopa_item();
    }

    int get_value() const override {
        return ((koopa_raw_value_data *) to_koopa_item())->kind.data.integer.value;
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