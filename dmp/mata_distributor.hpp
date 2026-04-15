#pragma once

#include "hls_stream.h"
#include "types.hpp"

struct MatA_Element {
    unsigned short row;
    unsigned short col;
    A_dtype        val;
    bool           last;
};

inline void rPtr_to_rIdx(hls::stream<unsigned int>& rPtr,
                         hls::stream<unsigned short>& rIdx,
                         unsigned int n_rows) {
    unsigned int prev = rPtr.read();
    for (unsigned int row = 0; row < n_rows; ++row) {
        unsigned int cur = rPtr.read();
        unsigned int count = cur - prev;
        for (unsigned int i = 0; i < count; ++i) {
            #pragma HLS pipeline II=1
            rIdx.write((unsigned short)row);
        }
        prev = cur;
    }
}

template<unsigned short p>
void elementDistributor(hls::stream<unsigned short>& rIdx,
                        hls::stream<unsigned short>& colIdx,
                        hls::stream<A_dtype>& val,
                        hls::stream<MatA_Element> pmcu_in[p],
                        unsigned int a_nnz) {
    for (unsigned int nz = 0; nz < a_nnz; ++nz) {
        #pragma HLS pipeline II=1
        unsigned short target = (unsigned short)(nz % p);
        pmcu_in[target].write(MatA_Element{
            rIdx.read(),
            colIdx.read(),
            val.read(),
            false
        });
    }

    for (unsigned short i = 0; i < p; ++i) {
        #pragma HLS unroll
        pmcu_in[i].write(MatA_Element{0, 0, 0, true});
    }
}

template<unsigned short p>
void MatADistributor(hls::stream<unsigned int>& rPtr,
                     hls::stream<unsigned short>& colIdx,
                     hls::stream<A_dtype>& val,
                     hls::stream<MatA_Element> pmcu_in[p],
                     unsigned int a_rows,
                     unsigned int a_nnz) {
    hls::stream<unsigned short, STREAM_DEPTH> rIdx("mata_row_idx");
    #pragma HLS STREAM variable=rIdx depth=STREAM_DEPTH

    rPtr_to_rIdx(rPtr, rIdx, a_rows);
    elementDistributor<p>(rIdx, colIdx, val, pmcu_in, a_nnz);
}
