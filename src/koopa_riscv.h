#pragma once

#include "koopa.h"
#include <map>
#include <string>

std::string gen_riscv_from_koopa_raw_program(const koopa_raw_program_t & raw);

void Visit(const koopa_raw_slice_t & slice, std::string & res, int & s, std::map<const void *, int> & buf);
void Visit(const koopa_raw_function_t & func, std::string & res, int & s, std::map<const void *, int> & buf);
void Visit(const koopa_raw_basic_block_t & bb, std::string & res, int & s, std::map<const void *, int> & buf);
void Visit(const koopa_raw_value_t & value, std::string & res, int & s, std::map<const void *, int> & buf);