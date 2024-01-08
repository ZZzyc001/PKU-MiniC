#pragma once

#include <memory>
#include <string>
#include <vector>

#include "koopa_util.h"

struct KoopaWhile {
    koopa_raw_basic_block_data_t * while_entry;
    koopa_raw_basic_block_data_t * while_body;
    koopa_raw_basic_block_data_t * end_block;
};

class LoopMaintainer {
    std::vector<KoopaWhile> loop_stk;

public:
    void add(koopa_raw_basic_block_data_t * while_entry, koopa_raw_basic_block_data_t * while_body, koopa_raw_basic_block_data_t * end_block) {
        KoopaWhile kw;
        kw.while_entry = while_entry;
        kw.while_body  = while_body;
        kw.end_block   = end_block;
        loop_stk.push_back(kw);
    }
    KoopaWhile get() { return *loop_stk.rbegin(); }
    void       pop() { loop_stk.pop_back(); }
};