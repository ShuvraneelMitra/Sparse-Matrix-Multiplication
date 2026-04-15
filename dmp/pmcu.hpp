#pragma once

#include "hls_stream.h"
#include "mata_distributor.hpp"
#include "matb_loader.hpp"
#include "types.hpp"

template<unsigned short k>
void PMCU(hls::stream<MatA_Element>& mata_in,
          hls::stream<MatB_Request>& matb_req_out,
          hls::stream<MatB_Metadata>& matb_meta_in,
          hls::stream<unsigned short>& matb_col_in,
          hls::stream<B_dtype>& matb_val_in,
          hls::stream<COO_Element>& output,
          unsigned short max_burst = DMP_MAX_BURST) {
    (void)k;

    while (true) {
        MatA_Element a = mata_in.read();
        if (a.last) {
            output.write(COO_Element{DMP_ROW_SENTINEL, 0, 0});
            break;
        }

        bool first = true;
        unsigned int next_addr = 0;
        unsigned int remaining = 0;

        do {
            matb_req_out.write(MatB_Request{
                0,
                a.col,
                first,
                next_addr,
                remaining,
                max_burst
            });

            MatB_Metadata meta = matb_meta_in.read();
            for (unsigned short i = 0; i < meta.burst_len; ++i) {
                #pragma HLS pipeline II=1
                unsigned short b_col = matb_col_in.read();
                B_dtype b_val = matb_val_in.read();

                C_dtype prod = (C_dtype)(a.val) * (C_dtype)(b_val);
                output.write(COO_Element{
                    a.row,
                    b_col,
                    prod
                });
            }

            first = false;
            next_addr = meta.cv_addr;
            remaining = meta.remaining;
        } while (remaining != 0);
    }
}