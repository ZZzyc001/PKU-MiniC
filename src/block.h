#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "koopa_util.h"

class BlockMaker {
public:
    koopa_raw_function_t        func;
    std::vector<const void *>   insts_buf;
    std::vector<const void *> * block_buf;

    void setFunc(koopa_raw_function_t _func) {
        func = _func;
    }
    void setBlockBuf(std::vector<const void *> * _block_buf) {
        block_buf = _block_buf;
    }
    void finishBlock() {
        if (block_buf->size()) {
            koopa_raw_basic_block_data_t * last_block = (koopa_raw_basic_block_data_t *) (*block_buf)[block_buf->size() - 1];

            bool found = false;

            int i = 0;
            for (auto it : insts_buf) {
                ++i;
                auto value = (koopa_raw_value_t) it;
                if (value->kind.tag == KOOPA_RVT_BRANCH || value->kind.tag == KOOPA_RVT_RETURN || value->kind.tag == KOOPA_RVT_JUMP) {
                    found = true;
                    break;
                }
            }

            insts_buf.resize(i);

            if (! found) {
                koopa_raw_value_data * ret = new koopa_raw_value_data();
                ret->ty                    = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
                ret->name                  = nullptr;
                ret->used_by               = empty_koopa_rs(KOOPA_RSIK_VALUE);
                ret->kind.tag              = KOOPA_RVT_RETURN;
                if (func->ty->data.function.ret->tag == KOOPA_RTT_UNIT)
                    ret->kind.data.ret.value = nullptr;
                else
                    ret->kind.data.ret.value = make_number_koopa(0);
                insts_buf.push_back(ret);
            }

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