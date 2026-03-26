#include <ap_int.h>

typedef ap_uint<8> in_type;
typedef ap_uint<9> out_add_type;
typedef ap_uint<8> A_dtype;

/*
The max number of bits isn the output matrix C is 
the max number of bits in the aggregate of a number of 
8b x 8b multiplications, which is potentially 16b long.
Thus the max number of bits in C_dtype should be 
#bits in n_col(A) + 16, which is at most 24 since we are
representing n_cols with a short.
*/
typedef ap_uint<24> C_dtype;

struct Index {
  unsigned short row;
  unsigned short col;  
};

template<short N_ROWS, short N_COLS, int N_NZ>
struct CSR {
    unsigned int rowptr[N_ROWS + 1];
    unsigned short col[N_NZ];
    A_dtype val[N_NZ];
};

struct CSR_Element {
    unsigned int rowptr;
    unsigned short col;
    C_dtype val;
};

struct COO_Element {
    unsigned short row;
    unsigned short col;
    C_dtype val;
};
