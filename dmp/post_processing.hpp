#pragma once

#include "hls_stream.h"
#include "types.hpp"

inline bool coo_less(const COO_Element& a, const COO_Element& b) {
    if (a.row != b.row) {
        return a.row < b.row;
    }
    return a.col < b.col;
}

template<unsigned short p>
void ParallelMerger(hls::stream<COO_Element> in[p],
                    hls::stream<COO_Element>& out) {
    COO_Element head[p];
    bool valid[p] = {};
    bool done[p] = {};
    unsigned short done_count = 0;

    #pragma HLS array_partition variable=head complete
    #pragma HLS array_partition variable=valid complete
    #pragma HLS array_partition variable=done complete

    while (done_count < p) {
        for (unsigned short i = 0; i < p; ++i) {
            #pragma HLS unroll
            if (!valid[i] && !done[i] && !in[i].empty()) {
                COO_Element elem = in[i].read();
                if (is_coo_sentinel(elem)) {
                    done[i] = true;
                    ++done_count;
                } else {
                    head[i] = elem;
                    valid[i] = true;
                }
            }
        }

        unsigned short min_idx = p;
        for (unsigned short i = 0; i < p; ++i) {
            #pragma HLS unroll
            if (valid[i] && (min_idx == p || coo_less(head[i], head[min_idx]))) {
                min_idx = i;
            }
        }

        if (min_idx != p) {
            out.write(head[min_idx]);
            valid[min_idx] = false;
        }
    }

    out.write(COO_Element{DMP_ROW_SENTINEL, 0, 0});
}

inline void COOToCSR(hls::stream<COO_Element>& coo,
                     hls::stream<CSR_Element>& csr) {
    unsigned int nz_id = 0;
    bool have_acc = false;
    unsigned short acc_row = 0;
    unsigned short acc_col = 0;
    C_dtype acc_val = 0;
    unsigned short current_row = 0;
    unsigned int current_rowptr = 0;

    while (true) {
        COO_Element elem = coo.read();
        if (is_coo_sentinel(elem)) {
            if (have_acc) {
                csr.write(CSR_Element{current_rowptr, acc_col, acc_val});
                ++nz_id;
            }
            csr.write(CSR_Element{UINT_MAX, 0, 0});
            break;
        }

        if (!have_acc) {
            have_acc = true;
            acc_row = elem.row;
            acc_col = elem.col;
            acc_val = elem.val;
            current_row = elem.row;
            current_rowptr = nz_id;
            continue;
        }

        if (elem.row == acc_row && elem.col == acc_col) {
            acc_val += elem.val;
            continue;
        }


        csr.write(CSR_Element{current_rowptr, acc_col, acc_val});
        ++nz_id;


        acc_row = elem.row;
        acc_col = elem.col;
        acc_val = elem.val;

        if (elem.row != current_row) {
            current_row = elem.row;
            current_rowptr = nz_id;
        }
    }
}

template<unsigned short p>
void PostProcessing(hls::stream<COO_Element> in[p],
                    hls::stream<CSR_Element>& out) {
    hls::stream<COO_Element, STREAM_DEPTH> merged("merged_partial_matrix");
    #pragma HLS STREAM variable=merged depth=STREAM_DEPTH

    ParallelMerger<p>(in, merged);
    COOToCSR(merged, out);
}