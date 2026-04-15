#pragma once

#include "types.hpp"

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
         unsigned int* c_nnz_out);
