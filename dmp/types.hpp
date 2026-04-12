#include <ap_int.h>
#include <limits.h>

#define BUFFER_SIZE 10

typedef ap_uint<8> in_type;
typedef ap_uint<9> out_add_type;
typedef ap_uint<8> A_dtype;
typedef A_dtype B_dtype;

/*
The max number of bits in the output matrix C is 
the max number of bits in the aggregate of a number of 
8b x 8b multiplications, each of which is potentially 16b long.
Thus the max number of bits in C_dtype should be 
#bits in n_col(A) + 16, which in this case, is 
num_bits(short) + 16.
*/
typedef ap_uint<16 + sizeof(short) * CHAR_BIT> C_dtype;

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