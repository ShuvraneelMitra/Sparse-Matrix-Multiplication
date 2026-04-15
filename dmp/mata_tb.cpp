#include "topfile.hpp"
#include "mata_distributor.hpp"
#include <iostream>
#include <vector>

int main() {
    std::cout << "Testing MatA Distributor..." << std::endl;


    std::vector<unsigned int> a_rowptr = {0, 2, 3, 5};
    std::vector<unsigned short> a_col = {0, 2, 1, 0, 2};
    std::vector<A_dtype> a_val = {1, 2, 3, 4, 5};
    unsigned int a_rows = 3;
    unsigned int a_nnz = 5;


    hls::stream<unsigned int> rowptr_stream;
    hls::stream<unsigned short> col_stream;
    hls::stream<A_dtype> val_stream;
    hls::stream<MatA_Element> pmcu_in[DMP_NUM_PMCUS];


    for (unsigned int i = 0; i <= a_rows; ++i) {
        rowptr_stream.write(a_rowptr[i]);
    }
    for (unsigned int i = 0; i < a_nnz; ++i) {
        col_stream.write(a_col[i]);
        val_stream.write(a_val[i]);
    }


    MatADistributor<DMP_NUM_PMCUS>(rowptr_stream, col_stream, val_stream,
                                    pmcu_in, a_rows, a_nnz);


    for (unsigned short p = 0; p < DMP_NUM_PMCUS; ++p) {
        std::cout << "\nPMCU " << p << " received:" << std::endl;
        while (!pmcu_in[p].empty()) {
            MatA_Element elem = pmcu_in[p].read();
            if (elem.last) {
                std::cout << "  LAST element" << std::endl;
            } else {
                std::cout << " row=" << elem.row
                          << " col=" << elem.col
                          << " val=" << (unsigned int)elem.val << std::endl;
            }
        }
    }

    return 0;
}