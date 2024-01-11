#pragma once

#include <string>
#include <vector>

#include "koopa.h"

char * make_char_arr(std::string str);

koopa_raw_value_t make_number_koopa(int number);

koopa_raw_slice_t empty_koopa_rs(koopa_raw_slice_item_kind_t kind = KOOPA_RSIK_UNKNOWN);

koopa_raw_slice_t make_koopa_rs_single_element(const void * ele, koopa_raw_slice_item_kind_t kind = KOOPA_RSIK_UNKNOWN);

koopa_raw_slice_t make_koopa_rs_from_vector(const std::vector<const void *> & vec, koopa_raw_slice_item_kind_t kind = KOOPA_RSIK_UNKNOWN);

koopa_raw_type_kind * simple_koopa_raw_type_kind(koopa_raw_type_tag_t tag);

koopa_raw_type_kind * make_int_pointer_type();

koopa_raw_value_data * make_jump_block(koopa_raw_basic_block_t target);

koopa_raw_value_data * make_alloc_int(const std::string & name);

koopa_raw_value_data * make_zero_init();