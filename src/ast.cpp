#include "ast.h"

SymbolList     BaseAST::symbol_list;
BlockMaker     BaseAST::blocks_list;
LoopMaintainer BaseAST::loop_list;

void CompUnitAST::add_lib(std::vector<const void *> & funcs) const {
    koopa_raw_function_data_t * func;
    koopa_raw_type_kind_t *     ty;
    std::vector<const void *>   fparams;

    func                     = new koopa_raw_function_data_t();
    ty                       = new koopa_raw_type_kind_t();
    ty->tag                  = KOOPA_RTT_FUNCTION;
    ty->data.function.params = empty_koopa_rs(KOOPA_RSIK_TYPE);
    ty->data.function.ret    = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
    func->ty                 = ty;
    func->name               = "@getint";
    func->params             = empty_koopa_rs(KOOPA_RSIK_VALUE);
    func->bbs                = empty_koopa_rs(KOOPA_RSIK_BASIC_BLOCK);
    symbol_list.addSymbol("getint", LValSymbol(LValSymbol::SymbolType::Function, func));
    funcs.push_back(func);

    func                     = new koopa_raw_function_data_t();
    ty                       = new koopa_raw_type_kind_t();
    ty->tag                  = KOOPA_RTT_FUNCTION;
    ty->data.function.params = empty_koopa_rs(KOOPA_RSIK_TYPE);
    ty->data.function.ret    = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
    func->ty                 = ty;
    func->name               = "@getch";
    func->params             = empty_koopa_rs(KOOPA_RSIK_VALUE);
    func->bbs                = empty_koopa_rs(KOOPA_RSIK_BASIC_BLOCK);
    symbol_list.addSymbol("getch", LValSymbol(LValSymbol::SymbolType::Function, func));
    funcs.push_back(func);

    func    = new koopa_raw_function_data_t();
    ty      = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_FUNCTION;
    fparams.clear();
    fparams.push_back(make_int_pointer_type());
    ty->data.function.params = make_koopa_rs_from_vector(fparams, KOOPA_RSIK_TYPE);
    ty->data.function.ret    = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
    func->ty                 = ty;
    func->name               = "@getarray";
    func->params             = empty_koopa_rs(KOOPA_RSIK_VALUE);
    func->bbs                = empty_koopa_rs(KOOPA_RSIK_BASIC_BLOCK);
    symbol_list.addSymbol("getarray", LValSymbol(LValSymbol::SymbolType::Function, func));
    funcs.push_back(func);

    func    = new koopa_raw_function_data_t();
    ty      = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_FUNCTION;
    fparams.clear();
    fparams.push_back(simple_koopa_raw_type_kind(KOOPA_RTT_INT32));
    ty->data.function.params = make_koopa_rs_from_vector(fparams, KOOPA_RSIK_TYPE);
    ty->data.function.ret    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
    func->ty                 = ty;
    func->name               = "@putint";
    func->params             = empty_koopa_rs(KOOPA_RSIK_VALUE);
    func->bbs                = empty_koopa_rs(KOOPA_RSIK_BASIC_BLOCK);
    symbol_list.addSymbol("putint", LValSymbol(LValSymbol::SymbolType::Function, func));
    funcs.push_back(func);

    func    = new koopa_raw_function_data_t();
    ty      = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_FUNCTION;
    fparams.clear();
    fparams.push_back(simple_koopa_raw_type_kind(KOOPA_RTT_INT32));
    ty->data.function.params = make_koopa_rs_from_vector(fparams, KOOPA_RSIK_TYPE);
    ty->data.function.ret    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
    func->ty                 = ty;
    func->name               = "@putch";
    func->params             = empty_koopa_rs(KOOPA_RSIK_VALUE);
    func->bbs                = empty_koopa_rs(KOOPA_RSIK_BASIC_BLOCK);
    symbol_list.addSymbol("putch", LValSymbol(LValSymbol::SymbolType::Function, func));
    funcs.push_back(func);

    func    = new koopa_raw_function_data_t();
    ty      = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_FUNCTION;
    fparams.clear();
    fparams.push_back(simple_koopa_raw_type_kind(KOOPA_RTT_INT32));
    fparams.push_back(make_int_pointer_type());
    ty->data.function.params = make_koopa_rs_from_vector(fparams, KOOPA_RSIK_TYPE);
    ty->data.function.ret    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
    func->ty                 = ty;
    func->name               = "@putarray";
    func->params             = empty_koopa_rs(KOOPA_RSIK_VALUE);
    func->bbs                = empty_koopa_rs(KOOPA_RSIK_BASIC_BLOCK);
    symbol_list.addSymbol("putarray", LValSymbol(LValSymbol::SymbolType::Function, func));
    funcs.push_back(func);

    func                     = new koopa_raw_function_data_t();
    ty                       = new koopa_raw_type_kind_t();
    ty->tag                  = KOOPA_RTT_FUNCTION;
    ty->data.function.params = empty_koopa_rs(KOOPA_RSIK_TYPE);
    ty->data.function.ret    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
    func->ty                 = ty;
    func->name               = "@starttime";
    func->params             = empty_koopa_rs(KOOPA_RSIK_VALUE);
    func->bbs                = empty_koopa_rs(KOOPA_RSIK_BASIC_BLOCK);
    symbol_list.addSymbol("starttime", LValSymbol(LValSymbol::SymbolType::Function, func));
    funcs.push_back(func);

    func                     = new koopa_raw_function_data_t();
    ty                       = new koopa_raw_type_kind_t();
    ty->tag                  = KOOPA_RTT_FUNCTION;
    ty->data.function.params = empty_koopa_rs(KOOPA_RSIK_TYPE);
    ty->data.function.ret    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
    func->ty                 = ty;
    func->name               = "@stoptime";
    func->params             = empty_koopa_rs(KOOPA_RSIK_VALUE);
    func->bbs                = empty_koopa_rs(KOOPA_RSIK_BASIC_BLOCK);
    symbol_list.addSymbol("stoptime", LValSymbol(LValSymbol::SymbolType::Function, func));
    funcs.push_back(func);
}