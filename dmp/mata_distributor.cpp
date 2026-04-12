#include <climits>
#include <hls_task.h>

#include "types.hpp"
#include "mata_distributor.hpp"

/*
TODO: Consider downstream sentinel processing for while(true)
*/

void rPtr_to_rIdx(hls::stream<unsigned int>& rPtr,
                  hls::stream<unsigned short>& rIdx) {
    unsigned int prev = rPtr.read();
    unsigned short row_id = 0;

    while (true) {
        unsigned int cur = rPtr.read();
        unsigned int count = cur - prev;
        for (unsigned int i = 0; i < count; i++) {
            #pragma HLS pipeline II=1
            rIdx.write(row_id);
        }
        row_id++;
        prev = cur;
    }
    /*
    A sentinel propagating downstream might be needed, else the 
    argument is that row_id keeps incrementing indefinitely. However,
    the function should just block at the rPtr.read() if there are no
    incoming values from the input stream.
    */
}

template<unsigned short p>
void rowIdxDistributor(hls::stream<unsigned short>& rIdx,
                       hls::stream<unsigned short> pmcu_in_row[p]) {
    unsigned int nz_id = 0;
    while (true) {
        unsigned short val = rIdx.read();
        pmcu_in_row[nz_id % p].write(val);
        nz_id++;
    }
}

template<unsigned short p>
void colIdxValDistributor(hls::stream<unsigned short>& colIdx,
                          hls::stream<A_dtype>& val,
                          hls::stream<unsigned short> pmcu_in_col[p],
                          hls::stream<A_dtype> pmcu_in_val[p]) {
    unsigned int nz_id = 0;

    while (true) {
        unsigned short col = colIdx.read();
        A_dtype v = val.read();
        pmcu_in_col[nz_id % p].write(col);
        pmcu_in_val[nz_id % p].write(v);
        nz_id++;
    }
}

template<unsigned short p>
void MatADistributor(// INPUT STREAMS
                     hls::stream<unsigned int>& rPtr,
                     hls::stream<unsigned short>& colIdx,
                     hls::stream<A_dtype>& val,
                     
                     // OUTPUT STREAMS
                     hls::stream<unsigned short> pmcu_in_row[p],
                     hls::stream<unsigned short> pmcu_in_col[p],
                     hls::stream<A_dtype> pmcu_in_val[p]){

    hls_thread_local hls::stream<unsigned short, BUFFER_SIZE> rIdx;
    
    hls_thread_local hls::task rptrTorId(rPtr_to_rIdx, rPtr, rIdx);
    hls_thread_local hls::task rDist(rowIdxDistributor<p>, rIdx, pmcu_in_row);
    hls_thread_local hls::task cvDist(colIdxValDistributor<p>, colIdx, val, pmcu_in_col, pmcu_in_val);
}
