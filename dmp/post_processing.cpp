#include <hls_task.h>
#include <limits.h>

#include "post_processing.hpp"
#include "types.hpp"

template<unsigned short CH, unsigned short CW>
void Adder(hls::stream<COO_Element>& in, hls::stream<COO_Element>& out){
    /*
    INPUT STREAM: A stream of merged elements, in COO format
    
    BEHAVIOR: We simply want to merge the p partial matrices. Elements 
    with the same (row, col) value are accumulated to produce C in COO 
    format. For this we must store the entire matrix C onto the local 
    memory of the FPGA; for this if we have matrices from SuiteSparse 
    then with the ones checked in the paper, we should need an upper bound
    of 70 KB of space, which should be easily doable, atleast on the 
    XC7A35T that I have right now.

    Since this needs the complete output of the previous stage before
    starting to stream its own output this might become a bottleneck.
    Can we optimize this?

    We cannot do this row by row because we have p PMCUs parallelly
    dump in (row, col, val) tuples. Suppose one stream, Si, has many
    values in row 1 while another one, Sj, does not. Then, it could be 
    the case that Si is emitting row 1 values while Sj has already progressed
    to, say, row 3. Synchronizing all the p parallel processes to wait
    till each of them has completed row 1 might be a viable alternative
    we might want to explore.
    */

    static C_dtype C[CH][CW];

    #pragma HLS bind_storage variable=C type=ram_2p impl=bram
    #pragma HLS ARRAY_PARTITION variable=C cyclic factor=8 dim=2 
    // no guarantees of better throughput since access is essentially random 
    
    init: for (int i = 0; i < CH; i++){
        for (int j = 0; j < CW; j++) {
            #pragma HLS PIPELINE II=1
            C[i][j] = 0;
        }
    }

    hls::stream<Index, CH*CW> touchedIndices;
    #pragma HLS STREAM variable=touchedIndices depth=CH*CW

    accumulate: while(true){
        #pragma HLS PIPELINE II=1
        COO_Element elem = in.read();
        if(elem.row == USHRT_MAX) break;
        
        C[elem.row][elem.col] += elem.val;
        touchedIndices.write(Index{
            elem.row,
            elem.col
        });
    }

    emit: while(!touchedIndices.empty()){
        Index idx = touchedIndices.read();
        out.write(COO_Element{
            idx.row,
            idx.col,
            C[idx.row][idx.col]
        });
    }   
    out.write(COO_Element{USHRT_MAX, 0, 0});
}

// We will just leave this here; this won't be synthesized.
void ZeroEliminator(hls::stream<COO_Element>& in, hls::stream<COO_Element>& out){
    COO_Element coo_elem = in.read();
    if (coo_elem.val != 0) {
        out.write(coo_elem);
    }
}

void COOToCSR(hls::stream<COO_Element>& coo, hls::stream<CSR_Element>& csr){
    /*
    ASSUMPTION: The COO elements are streamed to this function
    in a row major manner, or else the sizes of each row cannot be
    determined, leading to inefficiency.

    INPUT STREAM: A stream of COO elements in row-major manner
    OUTPUT STREAM: A stream of CSR elements in row-major manner
    */
    static unsigned int nz_id      = 0;
    static unsigned int prev_rptr  = UINT_MAX;
    static unsigned short prev_row = USHRT_MAX; // so that prev_row++ wraps around
    // UPDATE: Since we got rid of prev_row++, the initial value doesn't really matter

    COO_Element coo_elem = coo.read();
    if (coo_elem.row == USHRT_MAX) {
        nz_id     = 0;
        prev_rptr = UINT_MAX;
        prev_row  = USHRT_MAX;
        csr.write(CSR_Element{USHRT_MAX, 0, 0});
        return;
    }
    if(coo_elem.row == prev_row){
        csr.write(CSR_Element{
            prev_rptr,
            coo_elem.col,
            coo_elem.val
        });
    } else {
        csr.write(CSR_Element{
            nz_id,
            coo_elem.col,
            coo_elem.val
        });
        prev_rptr = nz_id;
        // prev_row++; This is incorrect in the case of skipped empty rows
        prev_row = coo_elem.row;
    }
    nz_id++;
} 

template<unsigned short CH, unsigned short CW>
void PostProcessing(hls::stream<COO_Element>& in, hls::stream<CSR_Element>& out){
    hls::stream<COO_Element, CH * CW> nonZeroC; // the design forces this huge buffer allocation
    #pragma HLS STREAM variable=nonZeroC depth=CH*CW
    
    // It is useless to create a task to handle Adder since there is no real
    // producer-consumer paradigm here.
    Adder<CH, CW>(in, nonZeroC); // Potential bottleneck due to depth-2 FIFO behavior of nonZeroC
    COOToCSR(nonZeroC, out); // no hls::task needed
}