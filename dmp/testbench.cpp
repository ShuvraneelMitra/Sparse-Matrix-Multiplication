#include "hls_stream.h"
#include "mata_distributor.hpp"
#include "matb_loader.hpp"
#include "pmcu.hpp"
#include "post_processing.hpp"
#include <iostream>
#include <vector>
#include <map>

using namespace std;

int main() {
    
    vector<unsigned int> a_rowptr = {0, 2, 3, 5};
    vector<unsigned short> a_col = {0, 2, 1, 0, 2};
    vector<A_dtype> a_val = {1, 2, 3, 4, 5};
    unsigned int a_rows = 3;
    unsigned int a_nnz = 5;


    vector<unsigned int> b_rowptr_0 = {0, 2, 2, 4};
    vector<unsigned short> b_col_0 = {0, 2, 0, 2};
    vector<B_dtype> b_val_0 = {1, 2, 4, 5};


    vector<unsigned int> b_rowptr_1 = {0, 0, 1, 1};
    vector<unsigned short> b_col_1 = {1};
    vector<B_dtype> b_val_1 = {3};

    const unsigned int* b_rowptrs[DMP_NUM_BANKS] = {b_rowptr_0.data(), b_rowptr_1.data()};
    const unsigned short* b_cols[DMP_NUM_BANKS] = {b_col_0.data(), b_col_1.data()};
    const B_dtype* b_vals[DMP_NUM_BANKS] = {b_val_0.data(), b_val_1.data()};


    hls::stream<unsigned int> a_rowptr_stream("a_rowptr");
    hls::stream<unsigned short> a_col_stream("a_col");
    hls::stream<A_dtype> a_val_stream("a_val");
    hls::stream<MatA_Element> pmcu_in[DMP_NUM_PMCUS];
    hls::stream<MatB_Request> pmcu_req[DMP_NUM_PMCUS];
    hls::stream<MatB_Metadata> pmcu_meta[DMP_NUM_PMCUS];
    hls::stream<unsigned short> pmcu_col[DMP_NUM_PMCUS];
    hls::stream<B_dtype> pmcu_val[DMP_NUM_PMCUS];
    hls::stream<COO_Element> pmcu_out[DMP_NUM_PMCUS];
    hls::stream<CSR_Element> csr_stream("csr");

    for (unsigned int i = 0; i <= a_rows; ++i) {
        a_rowptr_stream.write(a_rowptr[i]);
    }
    for (unsigned int i = 0; i < a_nnz; ++i) {
        a_col_stream.write(a_col[i]);
        a_val_stream.write(a_val[i]);
    }

    MatADistributor<DMP_NUM_PMCUS>(a_rowptr_stream, a_col_stream, a_val_stream,
                                    pmcu_in, a_rows, a_nnz);
    map<pair<unsigned short, unsigned short>, C_dtype> final_results;


    for (unsigned short p = 0; p < DMP_NUM_PMCUS; p++) {
        vector<MatA_Element> a_elements;
        while (!pmcu_in[p].empty()) {
            MatA_Element elem = pmcu_in[p].read();
            if (elem.last) {
                break;
            }
            a_elements.push_back(elem);
        }


        for (auto& a : a_elements) {
            unsigned short bank_id = a.col % DMP_NUM_BANKS;

            const unsigned int* rowptr = b_rowptrs[bank_id];
            const unsigned short* cols = b_cols[bank_id];
            const B_dtype* vals = b_vals[bank_id];

            unsigned int start = rowptr[a.col];
            unsigned int end = rowptr[a.col + 1];

            for (unsigned int i = start; i < end; i++) {
                unsigned short b_col = cols[i];
                B_dtype b_val = vals[i];
                C_dtype prod = (C_dtype)a.val * (C_dtype)b_val;

                pmcu_out[p].write(COO_Element{a.row, b_col, prod});

                auto key = make_pair(a.row, b_col);
                final_results[key] += prod;
            }
        }


        pmcu_out[p].write(COO_Element{DMP_ROW_SENTINEL, 0, 0});
    }
    PostProcessing<DMP_NUM_PMCUS>(pmcu_out, csr_stream);
    unsigned int nz_count = 0;
    unsigned int current_rowptr = 0;
    bool first_in_row = true;

    while (true) {
        CSR_Element elem = csr_stream.read();
        if (elem.rowptr == UINT_MAX) {
            break;
        }

        if (elem.rowptr != current_rowptr) {
            current_rowptr = elem.rowptr;
            if (!first_in_row) cout << endl;
            first_in_row = false;
        }

        nz_count++;
    }

    cout << "\n\nTotal nonzeros from CSR: " << nz_count << endl;

    cout << "\nComputed results from accumulation:" << endl;
    bool success = true;
    for (auto& kv : final_results) {
        cout << "  C(" << kv.first.first << "," << kv.first.second << ") = "
             << (unsigned int)kv.second << "\n";


        if ((kv.first.first == 0 && kv.first.second == 0 && kv.second == 9) ||
            (kv.first.first == 0 && kv.first.second == 2 && kv.second == 12) ||
            (kv.first.first == 1 && kv.first.second == 1 && kv.second == 9) ||
            (kv.first.first == 2 && kv.first.second == 0 && kv.second == 24) ||
            (kv.first.first == 2 && kv.first.second == 2 && kv.second == 33)) {
        } else {
            success = false;
        }
    }


    vector<pair<unsigned short, unsigned short>> expected_keys = {
        {0, 0}, {0, 2}, {1, 1}, {2, 0}, {2, 2}
    };
    for (auto& key : expected_keys) {
        if (final_results.find(key) == final_results.end()) {
            cout << "  C(" << key.first << "," << key.second << ") = 0 ✗ (missing)" << endl;
            success = false;
        }
    }

    cout << (success? "SUCCESS" : "FAILURE") << "\n";

    return 0;
}