#pragma once

#include "koopa.h"
#include <map>
#include <string>

std::string gen_riscv_from_koopa_raw_program(const koopa_raw_program_t & raw);

void Visit(const koopa_raw_slice_t & slice, std::string & res);
void Visit(const koopa_raw_function_t & func, std::string & res);
void Visit(const koopa_raw_basic_block_t & bb, std::string & res);
void Visit(const koopa_raw_value_t & value, std::string & res);

int cal_size(const koopa_raw_function_t & func, bool & call);
int cal_size(const koopa_raw_basic_block_t & bb, bool & call);
int cal_size(const koopa_raw_value_t & value);
int cal_size(const koopa_raw_type_t & type);

void gen_aggregate(koopa_raw_value_t val, std::string & res);
void gen_global_alloc(koopa_raw_value_t alloc, std::string & res);
void gen_store(const koopa_raw_store_t & store, std::string & res);
void gen_get_ptr(const koopa_raw_get_ptr_t & get, int addr, std::string & res);
void gen_get_elem_ptr(const koopa_raw_get_elem_ptr_t & get, int addr, std::string & res);
void gen_binary(const koopa_raw_binary_t & binary, int addr, std::string & res);
void gen_branch(const koopa_raw_branch_t & branch, std::string & res);
void gen_call(const koopa_raw_call_t & call, int addr, std::string & res);
void gen_return(const koopa_raw_return_t & ret, std::string & res);
