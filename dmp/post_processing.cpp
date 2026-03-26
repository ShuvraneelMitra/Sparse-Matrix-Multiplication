#include <hls_task.h>
#include <limits.h>

#include "post_processing.hpp"
#include "types.hpp"

template<unsigned short CH, unsigned short CW>
void Adder(hls::stream<COO_Element> in, hls::stream<COO_Element> out){
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
    */
    static C_dtype C[CH][CW];
    while(1){
        COO_Element elem = in.read();
        if(elem.row == USHRT_MAX) break;
        C[elem.row][elem.col] += elem.val;
    }

    
    for(int row = 0; row < CH; row++){
        for(int col = 0; col < CW; col++){
            auto v = C[row][col];
            /*
            The following if condition should ideally eliminate the 
            need for a separate ZeroEliminator function.
            */
            if(v != 0) out.write(COO_Element{
                row, 
                col,
                v
            });
        }
    }
}

// We will just leave this here; this won't be synthesized.
void ZeroEliminator(hls::stream<COO_Element> in, hls::stream<COO_Element> out){
    COO_Element coo_elem = in.read();
    if (coo_elem.val != 0) {
        out.write(coo_elem);
    }
}

void COOToCSR(hls::stream<COO_Element> coo, hls::stream<CSR_Element> csr){
    /*
    ASSUMPTION: The COO elements are streamed to this function
    in a row major manner, or else the sizes of each row cannot be
    determined, leading to inefficiency.

    INPUT STREAM: A stream of COO elements in row-major manner
    OUTPUT STREAM: A stream of CSR elements in row-major manner
    */
    static unsigned int nz_id = 0;
    static unsigned int prev_rptr = UINT_MAX;
    static unsigned short prev_row = USHRT_MAX; // so that prev_row++ wraps around

    COO_Element coo_elem = coo.read();
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
        prev_row++;
    }
    nz_id++;
    
    // Add this later: hls_thread_local hls::task(COO_to_CSR,...)
} 

template<unsigned short CH, unsigned short CW>
void PostProcessing(hls::stream<COO_Element> in, hls::stream<CSR_Element> out){
    hls_thread_local hls::stream<COO_Element, CH * CW> nonZeroC;
    
    hls_thread_local hls::task adder(Adder<CH, CW>, in, nonZeroC);
    hls_thread_local hls::task convert(COOToCSR(nonZeroC, out));
}