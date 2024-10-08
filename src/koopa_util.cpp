#include <assert.h>
#include <cstring>

#include "koopa_util.h"

char * make_char_arr(std::string str) {
    size_t n   = str.length();
    char * res = new char(n + 1);
    str.copy(res, n + 1);
    res[n] = 0;
    return res;
}

koopa_raw_value_t make_number_koopa(int number) {
    koopa_raw_value_data * res = new koopa_raw_value_data();

    res->ty                      = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
    res->name                    = nullptr;
    res->used_by                 = empty_koopa_rs(KOOPA_RSIK_VALUE);
    res->kind.tag                = KOOPA_RVT_INTEGER;
    res->kind.data.integer.value = number;
    return res;
}

koopa_raw_slice_t empty_koopa_rs(koopa_raw_slice_item_kind_t kind) {
    koopa_raw_slice_t res;
    res.buffer = nullptr;
    res.kind   = kind;
    res.len    = 0;
    return res;
}

koopa_raw_slice_t make_koopa_rs_single_element(const void * ele, koopa_raw_slice_item_kind_t kind) {
    koopa_raw_slice_t res;
    res.buffer    = new const void *[1];
    res.buffer[0] = ele;
    res.kind      = kind;
    res.len       = 1;
    return res;
}

koopa_raw_slice_t add_element(koopa_raw_slice_t origin, const void * ele) {
    koopa_raw_slice_t res;
    res.buffer = new const void *[origin.len + 1];
    memcpy(res.buffer, origin.buffer, sizeof(void *) * origin.len);
    res.buffer[origin.len] = ele;
    res.len                = origin.len + 1;
    res.kind               = origin.kind;
    delete origin.buffer;

    return res;
}

koopa_raw_slice_t make_koopa_rs_from_vector(const std::vector<const void *> & vec, koopa_raw_slice_item_kind_t kind) {
    koopa_raw_slice_t res;
    if (vec.size()) {
        res.buffer = new const void *[vec.size()];
        std::copy(vec.begin(), vec.end(), res.buffer);
    } else
        res.buffer = nullptr;
    res.kind = kind;
    res.len  = vec.size();
    return res;
}

koopa_raw_type_kind * simple_koopa_raw_type_kind(koopa_raw_type_tag_t tag) {
    assert(tag == KOOPA_RTT_INT32 || tag == KOOPA_RTT_UNIT);
    koopa_raw_type_kind * res = new koopa_raw_type_kind();
    res->tag                  = tag;
    return res;
}

koopa_raw_type_kind * make_array_type(const std::vector<int> & sz, int st_pos) {
    std::vector<koopa_raw_type_kind *> ty_list;
    for (size_t i = st_pos; i < sz.size(); ++i) {
        koopa_raw_type_kind * new_rt = new koopa_raw_type_kind();
        new_rt->tag                  = KOOPA_RTT_ARRAY;
        new_rt->data.array.len       = sz[i];
        ty_list.push_back(new_rt);
    }
    ty_list[ty_list.size() - 1]->data.array.base = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
    for (size_t i = 0; i < ty_list.size() - 1; i++)
        ty_list[i]->data.array.base = ty_list[i + 1];
    return ty_list[0];
}

koopa_raw_type_kind * make_int_pointer_type() {
    koopa_raw_type_kind * res = new koopa_raw_type_kind();
    res->tag                  = KOOPA_RTT_POINTER;
    res->data.pointer.base    = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
    return res;
}

koopa_raw_value_data * make_jump_block(koopa_raw_basic_block_t target) {
    koopa_raw_value_data * res = new koopa_raw_value_data();

    res->ty      = simple_koopa_raw_type_kind(KOOPA_RTT_UNIT);
    res->name    = nullptr;
    res->used_by = empty_koopa_rs(KOOPA_RSIK_VALUE);

    res->kind.tag              = KOOPA_RVT_JUMP;
    res->kind.data.jump.args   = empty_koopa_rs(KOOPA_RSIK_VALUE);
    res->kind.data.jump.target = target;
    return res;
}

koopa_raw_value_data * make_alloc_int(const std::string & name) {
    koopa_raw_value_data * res = new koopa_raw_value_data();

    res->ty       = make_int_pointer_type();
    res->name     = make_char_arr(name);
    res->used_by  = empty_koopa_rs(KOOPA_RSIK_VALUE);
    res->kind.tag = KOOPA_RVT_ALLOC;
    return res;
}

koopa_raw_value_data * make_alloc_type(const std::string & name, koopa_raw_type_t ty) {
    koopa_raw_value_data * res = new koopa_raw_value_data();
    koopa_raw_type_kind *  tty = new koopa_raw_type_kind();

    tty->tag               = KOOPA_RTT_POINTER;
    tty->data.pointer.base = ty;

    res->ty       = tty;
    res->name     = make_char_arr(name);
    res->used_by  = empty_koopa_rs(KOOPA_RSIK_VALUE);
    res->kind.tag = KOOPA_RVT_ALLOC;
    return res;
}

koopa_raw_value_data * make_zero_init(koopa_raw_type_kind * ty) {
    koopa_raw_value_data * res = new koopa_raw_value_data();

    if (ty)
        res->ty = ty;
    else
        res->ty = simple_koopa_raw_type_kind(KOOPA_RTT_INT32);
    res->name     = nullptr;
    res->used_by  = empty_koopa_rs(KOOPA_RSIK_VALUE);
    res->kind.tag = KOOPA_RVT_ZERO_INIT;
    return res;
}

koopa_raw_value_data_t * set_ptr(koopa_raw_value_t src, koopa_raw_value_t index, bool new_ty, koopa_raw_value_tag_t tag) {
    auto get = new koopa_raw_value_data();

    if (new_ty) {
        koopa_raw_type_kind * ty = new koopa_raw_type_kind();

        ty->tag               = KOOPA_RTT_POINTER;
        ty->data.pointer.base = src->ty->data.pointer.base->data.array.base;

        get->ty = ty;
    } else
        get->ty = src->ty;
    get->name                       = nullptr;
    get->used_by                    = empty_koopa_rs(KOOPA_RSIK_VALUE);
    get->kind.tag                   = tag;
    get->kind.data.get_elem_ptr.src = src;

    if (index)
        get->kind.data.get_elem_ptr.index = index;
    else
        get->kind.data.get_elem_ptr.index = make_number_koopa(0);

    return get;
}
