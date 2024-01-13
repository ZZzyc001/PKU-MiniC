#pragma once

#include <assert.h>
#include <iostream>
#include <memory>
#include <vector>

#include "block.h"
#include "koopa_util.h"
#include "symbol_list.h"
#include "while_container.h"

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
    void add_lib(std::vector<const void *> & funcs) const;

public:
    std::vector<std::unique_ptr<BaseAST>> func_list;
    std::vector<std::unique_ptr<BaseAST>> const_value_list;
    std::vector<std::unique_ptr<BaseAST>> var_value_list;

    void Dump() const override {
        std::cout << "CompUnitAST { ";
        std::cout << "Const Values: ";
        for (const auto & it : const_value_list)
            it->Dump();
        std::cout << "Var Values: ";
        for (const auto & it : var_value_list)
            it->Dump();
        std::cout << "Funcs: ";
        for (const auto & it : func_list)
            it->Dump();
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        return nullptr;
    }

    koopa_raw_program_t to_koopa_raw_program() const {
        symbol_list.newEnv();
        std::vector<const void *> values;
        std::vector<const void *> funcs;

        add_lib(funcs);

        for (const auto & it : const_value_list)
            it->to_koopa_item();

        for (const auto & it : var_value_list)
            values.push_back(it->to_koopa_item());

        for (const auto & it : func_list)
            funcs.push_back(it->to_koopa_item());

        symbol_list.deleteEnv();

        koopa_raw_program_t res;
        res.values = make_koopa_rs_from_vector(values, KOOPA_RSIK_VALUE);
        res.funcs  = make_koopa_rs_from_vector(funcs, KOOPA_RSIK_FUNCTION);

        return res;
    }
};

class TypeAST : public BaseAST {
public:
    std::string name;

    void Dump() const override {
        std::cout << "TypeAST { ";
        std::cout << name;
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        if (name == "int")
            return simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
        else if (name == "void")
            return simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
        return nullptr;
    }
};

class ParamAST : public BaseAST {
public:
    enum class ParamType {
        Int,
        Array
    } type;

    std::string name;

    int index;

    std::vector<std::unique_ptr<ValueBaseAST>> sz_exp;

    void Dump() const override {
        std::cout << "TypeAST { ";
        std::cout << index << ' ' << name;
        std::cout << " }";
    }

    void * get_type() const {
        if (type == ParamType::Int)
            return simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
        else {
            if (sz_exp.empty())
                return make_int_pointer_type();
            else {
                std::vector<int> sz;
                for (const auto & e : sz_exp)
                    sz.push_back(e->get_value());
                koopa_raw_type_kind * ty  = make_array_type(sz);
                koopa_raw_type_kind * tty = new koopa_raw_type_kind();
                tty->tag                  = KOOPA_RTT_POINTER;
                tty->data.pointer.base    = ty;
                return tty;
            }
        }
    }

    void * to_koopa_item() const override {
        koopa_raw_value_data * res = new koopa_raw_value_data();

        res->ty                           = (koopa_raw_type_kind *) get_type();
        res->name                         = make_char_arr("@" + name);
        res->used_by                      = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag                     = KOOPA_RVT_FUNC_ARG_REF;
        res->kind.data.func_arg_ref.index = index;
        return res;
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

    void * to_koopa_item_no_env() const {
        for (const auto & it : insts)
            it->to_koopa_item();

        return nullptr;
    }

    void * to_koopa_item() const override {
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
        if (exp)
            exp->Dump();
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        koopa_raw_value_data * res = new koopa_raw_value_data();

        res->ty      = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
        res->name    = nullptr;
        res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);

        res->kind.tag = KOOPA_RVT_RETURN;
        if (exp)
            res->kind.data.ret.value = (koopa_raw_value_t) exp->to_koopa_item();
        else
            res->kind.data.ret.value = nullptr;

        blocks_list.addInst(res);
        return res;
    }
};

class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<TypeAST>               func_type;
    std::string                            ident;
    std::vector<std::unique_ptr<ParamAST>> func_params;
    std::unique_ptr<BlockAST>              block;

    void Dump() const override {
        std::cout << "FuncDefAST { ";
        func_type->Dump();
        std::cout << " " << ident << " (";
        for (const auto & it : func_params)
            it->Dump();
        std::cout << ") ";
        block->Dump();
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        koopa_raw_function_data_t * res = new koopa_raw_function_data_t();

        symbol_list.addSymbol(ident, LValSymbol(LValSymbol::SymbolType::Function, res));

        koopa_raw_type_kind_t * ty = new koopa_raw_type_kind_t();

        ty->tag = KOOPA_RTT_FUNCTION;

        std::vector<const void *> par;
        for (auto & fp : func_params)
            par.push_back(fp->get_type());

        ty->data.function.params = make_koopa_rs_from_vector(par, KOOPA_RSIK_TYPE);
        ty->data.function.ret    = (const koopa_raw_type_kind *) func_type->to_koopa_item();

        res->ty   = ty;
        res->name = make_char_arr("@" + ident);

        par.clear();

        for (auto & fp : func_params)
            par.push_back(fp->to_koopa_item());

        res->params = make_koopa_rs_from_vector(par, KOOPA_RSIK_VALUE);

        std::vector<const void *> blocks;
        blocks_list.setBlockBuf(&blocks);

        koopa_raw_basic_block_data_t * entry_block = new koopa_raw_basic_block_data_t();

        entry_block->name    = make_char_arr("%entry_" + ident);
        entry_block->params  = empty_koopa_rs(KOOPA_RSIK_VALUE);
        entry_block->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);

        symbol_list.newEnv();

        blocks_list.addBlock(entry_block);
        blocks_list.setFunc(res);

        for (size_t i = 0; i < func_params.size(); ++i) {
            auto & fp = func_params[i];

            koopa_raw_value_data * allo = make_alloc_type("@" + fp->name, ((koopa_raw_value_t) par[i])->ty);

            if (allo->ty->data.pointer.base->tag == KOOPA_RTT_POINTER)
                symbol_list.addSymbol(fp->name, LValSymbol(LValSymbol::SymbolType::Pointer, allo));
            else
                symbol_list.addSymbol(fp->name, LValSymbol(LValSymbol::SymbolType::Var, allo));

            blocks_list.addInst(allo);

            koopa_raw_value_data * sto = new koopa_raw_value_data();

            sto->ty                    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
            sto->name                  = nullptr;
            sto->used_by               = empty_koopa_rs(KOOPA_RSIK_VALUE);
            sto->kind.tag              = KOOPA_RVT_STORE;
            sto->kind.data.store.value = (koopa_raw_value_t) par[i];
            sto->kind.data.store.dest  = allo;

            blocks_list.addInst(sto);
        }
        block->to_koopa_item_no_env();

        symbol_list.deleteEnv();

        blocks_list.finishBlock();

        res->bbs = make_koopa_rs_from_vector(blocks, KOOPA_RSIK_BASIC_BLOCK);

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
    enum class ValType {
        Num,
        Array
    } type;

    std::string name;

    std::vector<std::unique_ptr<BaseAST>> idx;

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
            res->kind.data.load.src = (koopa_raw_value_t) var.number;
            blocks_list.addInst(res);
        } else if (var.type == LValSymbol::SymbolType::Array) {
            bool need_load = false;

            koopa_raw_value_data * get;

            koopa_raw_value_data_t * src = (koopa_raw_value_data *) var.number;
            if (idx.empty())
                blocks_list.addInst(set_ptr(src));
            else
                for (auto & i : idx) {
                    get = set_ptr(src, (koopa_raw_value_t) i->to_koopa_item());
                    blocks_list.addInst(get);
                    if (get->ty->data.pointer.base->tag == KOOPA_RTT_INT32)
                        need_load = true;
                    src = get;
                }

            if (need_load) {
                res->ty                 = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
                res->name               = nullptr;
                res->used_by            = empty_koopa_rs(KOOPA_RSIK_VALUE);
                res->kind.tag           = KOOPA_RVT_LOAD;
                res->kind.data.load.src = get;
                blocks_list.addInst(res);
            } else if (src->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY) {
                res = set_ptr(src);
                blocks_list.addInst(res);
            } else
                res = src;
        } else if (var.type == LValSymbol::SymbolType::Pointer) {
            koopa_raw_value_data * src = (koopa_raw_value_data *) var.number;

            koopa_raw_value_data * load0 = new koopa_raw_value_data();

            load0->ty                 = src->ty->data.pointer.base;
            load0->name               = nullptr;
            load0->used_by            = empty_koopa_rs(KOOPA_RSIK_VALUE);
            load0->kind.tag           = KOOPA_RVT_LOAD;
            load0->kind.data.load.src = src;
            blocks_list.addInst(load0);

            bool need_load = false;

            bool first = true;

            koopa_raw_value_data * get;
            src = load0;

            for (auto & i : idx) {
                if (first) {
                    get = set_ptr(src, (koopa_raw_value_t) i->to_koopa_item(), false, KOOPA_RVT_GET_PTR);

                    first = false;
                } else
                    get = set_ptr(src, (koopa_raw_value_t) i->to_koopa_item());

                blocks_list.addInst(get);
                src = get;
                if (get->ty->data.pointer.base->tag == KOOPA_RTT_INT32)
                    need_load = true;
            }

            if (need_load) {
                res->ty                 = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
                res->name               = nullptr;
                res->used_by            = empty_koopa_rs(KOOPA_RSIK_VALUE);
                res->kind.tag           = KOOPA_RVT_LOAD;
                res->kind.data.load.src = get;
                blocks_list.addInst(res);
            } else if (src->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY) {
                res = set_ptr(src);

                blocks_list.addInst(res);
            } else
                res = src;
        }

        return res;
    }

    int get_value() const override {
        return ((koopa_raw_value_t) build_left_value())->kind.data.integer.value;
    }

    void * build_left_value() const override {
        if (type == ValType::Num)
            return (void *) symbol_list.getSymbol(name).number;
        else {
            koopa_raw_value_t src = (koopa_raw_value_t) symbol_list.getSymbol(name).number;

            koopa_raw_value_data * get;
            if (src->ty->data.pointer.base->tag == KOOPA_RTT_POINTER) {
                koopa_raw_value_data * load0 = new koopa_raw_value_data();
                load0->ty                    = src->ty->data.pointer.base;
                load0->name                  = nullptr;
                load0->used_by               = empty_koopa_rs(KOOPA_RSIK_VALUE);
                load0->kind.tag              = KOOPA_RVT_LOAD;
                load0->kind.data.load.src    = src;
                blocks_list.addInst(load0);

                bool first = true;

                src = load0;
                for (auto & i : idx) {
                    if (first) {
                        get = set_ptr(src, (koopa_raw_value_t) i->to_koopa_item(), false, KOOPA_RVT_GET_PTR);

                        first = false;
                    } else
                        get = set_ptr(src, (koopa_raw_value_t) i->to_koopa_item());

                    blocks_list.addInst(get);
                    src = get;
                }
            } else {
                for (auto & i : idx) {
                    get = set_ptr(src, (koopa_raw_value_t) i->to_koopa_item());
                    blocks_list.addInst(get);
                    src = get;
                }
            }
            return get;
        }
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
        UnaryExp,
        Function
    } type;

    std::string                   op;
    std::unique_ptr<ValueBaseAST> exp;

    std::vector<std::unique_ptr<BaseAST>> func_r_params;

    void Dump() const override {
        std::cout << "UnaryExpAST { ";
        if (type == UnaryExpType::Function) {
            std::cout << op << "(";
            for (const auto & it : func_r_params)
                it->Dump();
            std::cout << ")";
        } else {
            if (type == UnaryExpType::UnaryExp)
                std::cout << op << ' ';
            exp->Dump();
        }
        std::cout << " }";
    }

    void * to_koopa_item() const override {
        if (type == UnaryExpType::PrimaryExp || (type == UnaryExpType::UnaryExp && op == "+"))
            return exp->to_koopa_item();
        else if (type == UnaryExpType::Function) {
            std::vector<const void *> rpa;

            koopa_raw_function_data_t * func = (koopa_raw_function_data_t *) symbol_list.getSymbol(op).number;

            for (const auto & rp : func_r_params)
                rpa.push_back(rp->to_koopa_item());

            koopa_raw_value_data * res = new koopa_raw_value_data();

            res->ty                    = func->ty->data.function.ret;
            res->name                  = nullptr;
            res->used_by               = empty_koopa_rs(KOOPA_RSIK_VALUE);
            res->kind.tag              = KOOPA_RVT_CALL;
            res->kind.data.call.callee = func;
            res->kind.data.call.args   = make_koopa_rs_from_vector(rpa, KOOPA_RSIK_VALUE);
            blocks_list.addInst(res);
            return res;
        } else {
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

class GlobalVarDefAST : public BaseAST {
public:
    std::string              name;
    std::unique_ptr<BaseAST> exp;

    void Dump() const override {
        std::cout << "GlobalDefAST { " << name;
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
        res->kind.tag = KOOPA_RVT_GLOBAL_ALLOC;

        if (exp)
            res->kind.data.global_alloc.init = (koopa_raw_value_t) exp->to_koopa_item();
        else
            res->kind.data.global_alloc.init = make_zero_init();

        blocks_list.addInst(res);

        symbol_list.addSymbol(name, { LValSymbol::SymbolType::Var, res });

        return res;
    }
};

class InitValAST : public BaseAST {
public:
    enum class InitValType {
        Exp,
        Array
    } type;

    std::vector<koopa_raw_value_t> cache;

    bool is_const = false;

    std::unique_ptr<ValueBaseAST> exp;

    std::vector<std::unique_ptr<InitValAST>> arr_list;

    void Dump() const override {
        std::cout << "InitValAST { ";
        if (type == InitValType::Exp)
            exp->Dump();
        else
            for (const auto & it : arr_list)
                it->Dump();

        std::cout << " }";
    }

    void sub_preprocess(std::vector<int> & pro, int align_pos, std::vector<koopa_raw_value_t> & buf) {
        int target_size = buf.size() + pro[align_pos];

        for (auto & t : arr_list)
            if (t->type == InitValType::Exp)
                if (is_const)
                    buf.push_back(make_number_koopa(t->exp->get_value()));
                else
                    buf.push_back((koopa_raw_value_t) t->exp->to_koopa_item());
            else {
                int new_align_pos = align_pos + 1;

                while (cache.size() % pro[new_align_pos] != 0)
                    ++new_align_pos;

                t->sub_preprocess(pro, new_align_pos, buf);
            }

        while (buf.size() < target_size)
            buf.push_back(make_number_koopa(0));
    }

    void preprocess(const std::vector<int> & sz) {
        std::vector<int> pro(sz.size() + 1);

        pro[sz.size()] = 1;
        for (int i = sz.size() - 1; i >= 0; --i)
            pro[i] = pro[i + 1] * sz[i];

        sub_preprocess(pro, 0, cache);
    }

    koopa_raw_value_t at(int idx) {
        if (type == InitValType::Array)
            return cache[idx];
        else
            return (koopa_raw_value_t) exp->to_koopa_item();
    }

    koopa_raw_value_t sub_make_aggerate(const std::vector<int> & sz, std::vector<int> & pro, int align_pos, std::vector<koopa_raw_value_t> & buf, int st_pos) {
        if (pro[align_pos] == 1)
            return buf[st_pos];

        koopa_raw_value_data * res = new koopa_raw_value_data();

        res->ty       = make_array_type(sz, align_pos);
        res->name     = nullptr;
        res->used_by  = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag = KOOPA_RVT_AGGREGATE;

        std::vector<const void *> elems;

        for (int i = 0; i < sz[align_pos]; ++i)
            elems.push_back(sub_make_aggerate(sz, pro, align_pos + 1, buf, st_pos + pro[align_pos + 1] * i));

        res->kind.data.aggregate.elems = make_koopa_rs_from_vector(elems, KOOPA_RSIK_VALUE);
        return res;
    }

    koopa_raw_value_t make_aggerate(const std::vector<int> & sz) {
        std::vector<int> pro(sz.size() + 1);

        pro[sz.size()] = 1;

        for (int i = sz.size() - 1; i >= 0; --i)
            pro[i] = pro[i + 1] * sz[i];

        return sub_make_aggerate(sz, pro, 0, cache, 0);
    }
};

class ArrayDefAST : public BaseAST {
    koopa_raw_value_data * get_index(int i, std::vector<int> & pro, koopa_raw_value_data * src, int cur_pos = 0) const {
        if (cur_pos >= pro.size())
            return src;

        koopa_raw_value_data * get = set_ptr(src, make_number_koopa(i / pro[cur_pos]));

        blocks_list.addInst(get);

        return get_index(i % pro[cur_pos], pro, get, cur_pos + 1);
    }

public:
    std::string                 name;
    std::unique_ptr<InitValAST> init_val;

    std::vector<std::unique_ptr<ValueBaseAST>> sz_exp;

    void * to_koopa_item() const override {
        int total_size = 1;

        std::vector<int> sz;
        for (const auto & e : sz_exp) {
            int tmp = e->get_value();
            total_size *= tmp;
            sz.push_back(tmp);
        }

        koopa_raw_value_data * res = new koopa_raw_value_data();
        koopa_raw_type_kind *  ty  = make_array_type(sz);
        koopa_raw_type_kind *  tty = new koopa_raw_type_kind();
        tty->tag                   = KOOPA_RTT_POINTER;
        tty->data.pointer.base     = ty;
        res->ty                    = tty;
        res->name                  = make_char_arr("@" + name);
        res->used_by               = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag              = KOOPA_RVT_ALLOC;

        blocks_list.addInst(res);
        symbol_list.addSymbol(name, LValSymbol(LValSymbol::SymbolType::Array, res));

        if (init_val) {
            init_val->preprocess(sz);

            std::vector<int> pro(sz.size());

            pro[sz.size() - 1] = 1;
            for (int i = sz.size() - 2; i >= 0; --i)
                pro[i] = pro[i + 1] * sz[i + 1];

            for (int i = 0; i < total_size; ++i) {
                koopa_raw_value_data * get = get_index(i, pro, res);

                koopa_raw_value_data * st = new koopa_raw_value_data();
                st->ty                    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
                st->name                  = nullptr;
                st->used_by               = empty_koopa_rs(KOOPA_RSIK_VALUE);
                st->kind.tag              = KOOPA_RVT_STORE;
                st->kind.data.store.dest  = get;
                st->kind.data.store.value = init_val->at(i);
                blocks_list.addInst(st);
            }
        }
        return res;
    }
};

class GlobalArrayDefAST : public BaseAST {
public:
    std::string                 name;
    std::unique_ptr<InitValAST> init_val;

    std::vector<std::unique_ptr<ValueBaseAST>> sz_exp;

    void * to_koopa_item() const override {
        std::vector<int> sz;
        for (const auto & e : sz_exp)
            sz.push_back(e->get_value());

        koopa_raw_value_data * res = new koopa_raw_value_data();
        koopa_raw_type_kind *  ty  = make_array_type(sz);
        koopa_raw_type_kind *  tty = new koopa_raw_type_kind();
        tty->tag                   = KOOPA_RTT_POINTER;
        tty->data.pointer.base     = ty;
        res->ty                    = tty;
        res->name                  = make_char_arr("@" + name);
        res->used_by               = empty_koopa_rs(KOOPA_RSIK_VALUE);
        res->kind.tag              = KOOPA_RVT_GLOBAL_ALLOC;

        if (init_val) {
            init_val->is_const = true;
            init_val->preprocess(sz);
            res->kind.data.global_alloc.init = init_val->make_aggerate(sz);
        } else
            res->kind.data.global_alloc.init = make_zero_init(ty);

        symbol_list.addSymbol(name, LValSymbol(LValSymbol::SymbolType::Array, res));

        return res;
    }
};
