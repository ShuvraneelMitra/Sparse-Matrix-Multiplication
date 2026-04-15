#pragma once

#include "hls_stream.h"
#include "types.hpp"



template<unsigned short FIFO_DEPTH = 16>
class MergeNode2Way {
private:
    struct Entry {
        COO_Element elem;
        bool valid;
        bool last;
    };

    Entry left_entry;
    Entry right_entry;
    bool left_ready;
    bool right_ready;

public:
    MergeNode2Way() : left_ready(false), right_ready(false) {
        left_entry.valid = false;
        right_entry.valid = false;
    }

    void process(hls::stream<COO_Element>& left_in,
                 hls::stream<COO_Element>& right_in,
                 hls::stream<COO_Element>& out) {

        #pragma HLS pipeline II=1


        if (!left_ready && !left_in.empty()) {
            left_entry.elem = left_in.read();
            left_entry.valid = true;
            left_entry.last = is_coo_sentinel(left_entry.elem);
            left_ready = true;
        }


        if (!right_ready && !right_in.empty()) {
            right_entry.elem = right_in.read();
            right_entry.valid = true;
            right_entry.last = is_coo_sentinel(right_entry.elem);
            right_ready = true;
        }


        if (left_ready && right_ready) {
            if (coo_less(left_entry.elem, right_entry.elem)) {
                out.write(left_entry.elem);
                left_ready = false;
                left_entry.valid = false;
            } else {
                out.write(right_entry.elem);
                right_ready = false;
                right_entry.valid = false;
            }
        } else if (left_ready) {
            out.write(left_entry.elem);
            left_ready = false;
            left_entry.valid = false;
        } else if (right_ready) {
            out.write(right_entry.elem);
            right_ready = false;
            right_entry.valid = false;
        }
    }
};


template<unsigned short p>
void MergeTree(hls::stream<COO_Element> in[p],
               hls::stream<COO_Element>& out) {




    constexpr unsigned short levels = (p <= 2) ? 1 :
                                      (p <= 4) ? 2 :
                                      (p <= 8) ? 3 : 4;


    hls::stream<COO_Element> level_streams[levels][p];

    #pragma HLS stream variable=level_streams depth=STREAM_DEPTH


    for (unsigned short i = 0; i < p; ++i) {
        #pragma HLS unroll
        level_streams[0][i] = in[i];
    }


    unsigned short curr_width = p;
    for (unsigned short lvl = 0; lvl < levels - 1; ++lvl) {
        unsigned short next_width = (curr_width + 1) / 2;

        for (unsigned short i = 0; i < next_width; ++i) {
            #pragma HLS unroll
            if (2*i + 1 < curr_width) {

                MergeNode2Way<> node;


            } else {

                level_streams[lvl+1][i] = level_streams[lvl][2*i];
            }
        }
        curr_width = next_width;
    }



}