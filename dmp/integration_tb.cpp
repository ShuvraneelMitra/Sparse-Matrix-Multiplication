#include "hls_stream.h"
#include "mata_distributor.hpp"
#include "pmcu.hpp"
#include "post_processing.hpp"
#include <iostream>
#include <vector>

using namespace std;


void SimpleBProvider(hls::stream<MatB_Request>& req_in,
                     hls::stream<MatB_Metadata>& meta_out,
                     hls::stream<unsigned short>& col_out,
                     hls::stream<B_dtype>& val_out,
                     const unsigned int* b_rowptr,
                     const unsigned short* b_col,
                     const B_dtype* b_val) {

    while (true) {
        if (req_in.empty()) {
            break;
        }

        MatB_Request req = req_in.read();

        unsigned int start = b_rowptr[req.rowidx];
        unsigned int end = b_rowptr[req.rowidx + 1];
        unsigned int row_len = end - start;

        unsigned int send_start = req.first ? start : req.cv_addr;
        unsigned int remaining = req.first ? row_len : req.remaining;
        unsigned short to_send = (remaining < req.burst_len) ? remaining : req.burst_len;

        meta_out.write(MatB_Metadata{
            req.src,
            send_start + to_send,
            (unsigned short)to_send,
            remaining - to_send
        });

        for (unsigned int i = 0; i < to_send; i++) {
            col_out.write(b_col[send_start + i]);
            val_out.write(b_val[send_start + i]);
        }
    }
}


void RunPMCU(unsigned short p,
             hls::stream<MatA_Element>& in_stream,
             hls::stream<COO_Element>& out_stream,
             const unsigned int* b_rowptr,
             const unsigned short* b_col,
             const B_dtype* b_val) {

    hls::stream<MatB_Request> req_stream;
    hls::stream<MatB_Metadata> meta_stream;
    hls::stream<unsigned short> col_stream;
    hls::stream<B_dtype> val_stream;



    vector<MatA_Element> a_elements;
    while (!in_stream.empty()) {
        MatA_Element elem = in_stream.read();
        if (elem.last) break;
        a_elements.push_back(elem);
    }


    for (auto& a : a_elements) {
        bool first = true;
        unsigned int next_addr = 0;
        unsigned int remaining = 0;

        do {
            MatB_Request req;
            req.src = p;
            req.rowidx = a.col;
            req.first = first;
            req.cv_addr = next_addr;
            req.remaining = remaining;
            req.burst_len = DMP_MAX_BURST;

            req_stream.write(req);
            SimpleBProvider(req_stream, meta_stream, col_stream, val_stream,
                            b_rowptr, b_col, b_val);

            MatB_Metadata meta = meta_stream.read();

            for (unsigned short i = 0; i < meta.burst_len; ++i) {
                unsigned short b_col_val = col_stream.read();
                B_dtype b_val_val = val_stream.read();
                C_dtype prod = (C_dtype)a.val * (C_dtype)b_val_val;
                out_stream.write(COO_Element{a.row, b_col_val, prod});
            }

            first = false;
            next_addr = meta.cv_addr;
            remaining = meta.remaining;

        } while (remaining != 0);
    }

    out_stream.write(COO_Element{DMP_ROW_SENTINEL, 0, 0});
}

int main() {
    cout << "=== Step 3: Full DMP with Post Processing ===" << endl;


    vector<unsigned int> a_rowptr = {0, 2, 3, 5};
    vector<unsigned short> a_col = {0, 2, 1, 0, 2};
    vector<A_dtype> a_val = {1, 2, 3, 4, 5};
    unsigned int a_rows = 3;
    unsigned int a_nnz = 5;


    vector<unsigned int> b_rowptr = {0, 2, 3, 5};
    vector<unsigned short> b_col = {0, 2, 1, 0, 2};
    vector<B_dtype> b_val = {1, 2, 3, 4, 5};


    hls::stream<unsigned int> a_rowptr_stream("a_rowptr");
    hls::stream<unsigned short> a_col_stream("a_col");
    hls::stream<A_dtype> a_val_stream("a_val");
    hls::stream<MatA_Element> pmcu_in[DMP_NUM_PMCUS];
    hls::stream<COO_Element> pmcu_out[DMP_NUM_PMCUS];
    hls::stream<CSR_Element> csr_stream("csr");


    cout << "\nFeeding Matrix A..." << endl;
    for (unsigned int i = 0; i <= a_rows; ++i) {
        a_rowptr_stream.write(a_rowptr[i]);
    }
    for (unsigned int i = 0; i < a_nnz; ++i) {
        a_col_stream.write(a_col[i]);
        a_val_stream.write(a_val[i]);
        cout << "  A[" << i << "]: col=" << a_col[i] << " val=" << (unsigned int)a_val[i] << endl;
    }


    cout << "\nRunning MatA Distributor..." << endl;
    MatADistributor<DMP_NUM_PMCUS>(a_rowptr_stream, a_col_stream, a_val_stream,
                                    pmcu_in, a_rows, a_nnz);


    cout << "\nRunning PMCUs..." << endl;
    for (unsigned short p = 0; p < DMP_NUM_PMCUS; ++p) {
        cout << "  Processing PMCU " << p << "..." << endl;
        RunPMCU(p, pmcu_in[p], pmcu_out[p],
                b_rowptr.data(), b_col.data(), b_val.data());
    }


    cout << "\nRunning Post Processing (Parallel Merger + COO to CSR)..." << endl;
    PostProcessing<DMP_NUM_PMCUS>(pmcu_out, csr_stream);


    cout << "\n=== Final CSR Output ===" << endl;
    unsigned int nz_count = 0;
    unsigned int current_rowptr = 0;

    while (true) {
        CSR_Element elem = csr_stream.read();
        if (elem.rowptr == UINT_MAX) {
            break;
        }

        if (elem.rowptr != current_rowptr) {
            current_rowptr = elem.rowptr;
            cout << "\nRow " << current_rowptr << ": ";
        }

        cout << "(" << (unsigned int)elem.col << ", " << (unsigned int)elem.val << ") ";
        nz_count++;
    }

    cout << "\n\nTotal nonzeros: " << nz_count << endl;


    cout << "\n=== Verification ===" << endl;
    cout << "Expected C = A * A:" << endl;
    cout << "  Row 0: (0,9) (2,12)" << endl;
    cout << "  Row 1: (1,9)" << endl;
    cout << "  Row 2: (0,24) (2,33)" << endl;

    if (nz_count == 5) {
        cout << "\n SUCCESS: Got 5 nonzeros as expected!" << endl;
    } else {
        cout << "\n FAIL: Expected 5 nonzeros, got " << nz_count << endl;
    }

    return 0;
}