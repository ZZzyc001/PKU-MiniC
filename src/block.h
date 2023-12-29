#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "koopa_util.h"

class BlockMaker {
public:
    std::vector<const void *>   insts_buf;
    std::vector<const void *> * block_buf;

    void setBlockBuf(std::vector<const void *> * _block_buf) {
        block_buf = _block_buf;
    }
    void finishBlock() {
        if (block_buf->size()) {
            koopa_raw_basic_block_data_t * last_block = (koopa_raw_basic_block_data_t *) (*block_buf)[block_buf->size() - 1];

            int i = 0;
            for (auto it : insts_buf) {
                ++i;
                auto value = (koopa_raw_value_t) it;
                if (value->kind.tag == KOOPA_RVT_BRANCH || value->kind.tag == KOOPA_RVT_RETURN || value->kind.tag == KOOPA_RVT_JUMP)
                    break;
            }

            insts_buf.resize(i);

            if (! last_block->insts.buffer)
                last_block->insts = make_koopa_rs_from_vector(insts_buf, KOOPA_RSIK_VALUE);
        }
        insts_buf.clear();
    }
    void addBlock(koopa_raw_basic_block_data_t * basic_block) {
        finishBlock();
        basic_block->insts.buffer = nullptr;
        block_buf->push_back(basic_block);
    }

    void addInst(const void * inst) {
        insts_buf.push_back(inst);
    }
};