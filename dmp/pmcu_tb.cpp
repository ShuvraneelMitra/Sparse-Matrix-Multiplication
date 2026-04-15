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

int main() {
    cout << "=== Step 2: Testing PMCU with Simple B Provider ===" << endl;


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
    hls::stream<MatB_Request> pmcu_req[DMP_NUM_PMCUS];
    hls::stream<MatB_Metadata> pmcu_meta[DMP_NUM_PMCUS];
    hls::stream<unsigned short> pmcu_col[DMP_NUM_PMCUS];
    hls::stream<B_dtype> pmcu_val[DMP_NUM_PMCUS];
    hls::stream<COO_Element> pmcu_out[DMP_NUM_PMCUS];


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




    for (unsigned short p = 0; p < DMP_NUM_PMCUS; ++p) {
        cout << "\n=== Processing PMCU " << p << " ===" << endl;


        hls::stream<MatB_Request> req_stream;
        hls::stream<MatB_Metadata> meta_stream;
        hls::stream<unsigned short> col_stream;
        hls::stream<B_dtype> val_stream;
        hls::stream<COO_Element> out_stream;





        vector<MatA_Element> a_elements;
        while (!pmcu_in[p].empty()) {
            MatA_Element elem = pmcu_in[p].read();
            if (elem.last) {
                cout << "  Received LAST marker" << endl;
                break;
            }
            a_elements.push_back(elem);
            cout << "  Received A: row=" << elem.row << " col=" << elem.col
                 << " val=" << (unsigned int)elem.val << endl;
        }


        for (auto& a : a_elements) {
            cout << "\n  Processing A(" << a.row << "," << a.col << ") = " << (unsigned int)a.val << endl;

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
                                b_rowptr.data(), b_col.data(), b_val.data());


                MatB_Metadata meta = meta_stream.read();
                cout << "    B provider returned: burst_len=" << meta.burst_len
                     << " remaining=" << meta.remaining << endl;


                for (unsigned short i = 0; i < meta.burst_len; ++i) {
                    unsigned short b_col_val = col_stream.read();
                    B_dtype b_val_val = val_stream.read();

                    C_dtype prod = (C_dtype)a.val * (C_dtype)b_val_val;
                    cout << "      C(" << a.row << "," << b_col_val << ") += "
                         << (unsigned int)a.val << " * " << (unsigned int)b_val_val
                         << " = " << (unsigned int)prod << endl;

                    out_stream.write(COO_Element{a.row, b_col_val, prod});
                }

                first = false;
                next_addr = meta.cv_addr;
                remaining = meta.remaining;

            } while (remaining != 0);
        }


        out_stream.write(COO_Element{DMP_ROW_SENTINEL, 0, 0});


        cout << "\n  Results from PMCU " << p << ":" << endl;
        while (!out_stream.empty()) {
            COO_Element elem = out_stream.read();
            if (is_coo_sentinel(elem)) {
                break;
            }
            cout << "    C(" << elem.row << "," << elem.col << ") = "
                 << (unsigned int)elem.val << endl;
        }
    }

    cout << "\n=== Test Complete ===" << endl;
    return 0;
}