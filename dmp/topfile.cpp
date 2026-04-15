#include "topfile.hpp"
#include "hls_stream.h"
#include "hls_task.h"
#include "mata_distributor.hpp"
#include "matb_loader.hpp"
#include "pmcu.hpp"
#include "post_processing.hpp"

void top(const unsigned int* a_rowptr,
         const unsigned short* a_col,
         const A_dtype* a_val,
         unsigned int a_rows,
         unsigned int a_nnz,
         const unsigned int* b_rowptr_0,
         const unsigned short* b_col_0,
         const B_dtype* b_val_0,
         const unsigned int* b_rowptr_1,
         const unsigned short* b_col_1,
         const B_dtype* b_val_1,
         CSR_Element* c_out,
         unsigned int* c_nnz_out) {

    #pragma HLS interface m_axi port=a_rowptr offset=slave bundle=gmem0
    #pragma HLS interface m_axi port=a_col offset=slave bundle=gmem0
    #pragma HLS interface m_axi port=a_val offset=slave bundle=gmem0
    #pragma HLS interface m_axi port=b_rowptr_0 offset=slave bundle=gmem1
    #pragma HLS interface m_axi port=b_col_0 offset=slave bundle=gmem1
    #pragma HLS interface m_axi port=b_val_0 offset=slave bundle=gmem1
    #pragma HLS interface m_axi port=b_rowptr_1 offset=slave bundle=gmem2
    #pragma HLS interface m_axi port=b_col_1 offset=slave bundle=gmem2
    #pragma HLS interface m_axi port=b_val_1 offset=slave bundle=gmem2
    #pragma HLS interface m_axi port=c_out offset=slave bundle=gmem3
    #pragma HLS interface m_axi port=c_nnz_out offset=slave bundle=gmem3
    #pragma HLS interface s_axilite port=a_rowptr bundle=control
    #pragma HLS interface s_axilite port=a_col bundle=control
    #pragma HLS interface s_axilite port=a_val bundle=control
    #pragma HLS interface s_axilite port=a_rows bundle=control
    #pragma HLS interface s_axilite port=a_nnz bundle=control
    #pragma HLS interface s_axilite port=b_rowptr_0 bundle=control
    #pragma HLS interface s_axilite port=b_col_0 bundle=control
    #pragma HLS interface s_axilite port=b_val_0 bundle=control
    #pragma HLS interface s_axilite port=b_rowptr_1 bundle=control
    #pragma HLS interface s_axilite port=b_col_1 bundle=control
    #pragma HLS interface s_axilite port=b_val_1 bundle=control
    #pragma HLS interface s_axilite port=c_out bundle=control
    #pragma HLS interface s_axilite port=c_nnz_out bundle=control
    #pragma HLS interface s_axilite port=return bundle=control

    #ifndef __SYNTHESIS__
    unsigned int total_requests = a_nnz;


    #endif


    #pragma HLS dataflow


    hls::stream<unsigned int, STREAM_DEPTH> a_rowptr_stream("a_rowptr_stream");
    hls::stream<unsigned short, STREAM_DEPTH> a_col_stream("a_col_stream");
    hls::stream<A_dtype, STREAM_DEPTH> a_val_stream("a_val_stream");


    hls::stream<MatA_Element, STREAM_DEPTH> pmcu_in[DMP_NUM_PMCUS];


    hls::stream<MatB_Request, STREAM_DEPTH> pmcu_req[DMP_NUM_PMCUS];
    hls::stream<MatB_Metadata, STREAM_DEPTH> pmcu_meta[DMP_NUM_PMCUS];
    hls::stream<unsigned short, STREAM_DEPTH> pmcu_b_col[DMP_NUM_PMCUS];
    hls::stream<B_dtype, STREAM_DEPTH> pmcu_b_val[DMP_NUM_PMCUS];


    hls::stream<COO_Element, STREAM_DEPTH> pmcu_out[DMP_NUM_PMCUS];


    hls::stream<CSR_Element, STREAM_DEPTH> csr_stream("csr_stream");




    for (unsigned int i = 0; i <= a_rows; ++i) {
        #pragma HLS pipeline II=1
        a_rowptr_stream.write(a_rowptr[i]);
    }
    for (unsigned int i = 0; i < a_nnz; ++i) {
        #pragma HLS pipeline II=1
        a_col_stream.write(a_col[i]);
        a_val_stream.write(a_val[i]);
    }




    MatADistributor<DMP_NUM_PMCUS>(a_rowptr_stream, a_col_stream, a_val_stream,
                                    pmcu_in, a_rows, a_nnz);




    for (unsigned short i = 0; i < DMP_NUM_PMCUS; ++i) {
        #pragma HLS unroll
        PMCU<DMP_NUM_MULTS>(pmcu_in[i], pmcu_req[i], pmcu_meta[i],
                            pmcu_b_col[i], pmcu_b_val[i], pmcu_out[i], DMP_MAX_BURST);
    }




    const unsigned int* b_rowptr[DMP_NUM_BANKS] = {b_rowptr_0, b_rowptr_1};
    const unsigned short* b_col[DMP_NUM_BANKS] = {b_col_0, b_col_1};
    const B_dtype* b_val[DMP_NUM_BANKS] = {b_val_0, b_val_1};

    MatBLoader<DMP_NUM_PMCUS, DMP_NUM_BANKS>(
        pmcu_req, pmcu_meta, pmcu_b_col, pmcu_b_val,
        b_rowptr, b_col, b_val
    );




    PostProcessing<DMP_NUM_PMCUS>(pmcu_out, csr_stream);




    unsigned int idx = 0;
    while (true) {
        #pragma HLS pipeline II=1
        CSR_Element elem = csr_stream.read();
        if (elem.rowptr == UINT_MAX) {
            *c_nnz_out = idx;
            break;
        }
        c_out[idx++] = elem;
    }
}