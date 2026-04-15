#pragma once

#include <ap_int.h>
#include <limits.h>

typedef ap_uint<8> A_dtype;
typedef A_dtype B_dtype;
typedef ap_uint<16 + sizeof(short) * CHAR_BIT> C_dtype;

struct Index {
    unsigned short row;
    unsigned short col;
};

struct CSR_Element {
    unsigned int   rowptr;
    unsigned short col;
    C_dtype        val;
};

struct COO_Element {
    unsigned short row;
    unsigned short col;
    C_dtype        val;
};

constexpr unsigned short DMP_NUM_PMCUS   = 4;
constexpr unsigned short DMP_NUM_BANKS   = 2;
constexpr unsigned short DMP_NUM_MULTS   = 8;
constexpr unsigned short DMP_MAX_BURST   = 64;
constexpr unsigned short ROWPTR_CACHE_SIZE    = 256;
constexpr unsigned short COLIDXVAL_CACHE_SIZE = 512;
constexpr unsigned short STREAM_DEPTH         = 64;
constexpr unsigned short DMP_ROW_SENTINEL = USHRT_MAX;

inline bool is_coo_sentinel(const COO_Element& elem) {
    return elem.row == DMP_ROW_SENTINEL;
}