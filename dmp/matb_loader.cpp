#include "matb_loader.hpp"
#include "types.hpp"
#include "hls_stream.h"
#include "hls_task.h"

#define ROWPTR_CACHE_SIZE 256
#define COLIDXVAL_CACHE_SIZE 512


struct MatB_Request {
    unsigned short src;     
    unsigned short rowidx;   
    bool first;             
    unsigned int cv_addr;   
    unsigned short burst_len;
};

struct MatB_Metadata {
    unsigned short src;
    unsigned int cv_addr;    
    unsigned short burst_len;
    bool has_more;         
};

struct RowPtr_Request {
    unsigned int addr;
    unsigned short src;
};

struct ColIdxVal_Request {
    unsigned int addr;
    unsigned short burst_len;
    unsigned short src;
};

struct RowPtr_CacheEntry {
    unsigned short tag;
    unsigned int rowptr;
    bool valid;
};

bool rowptr_cache_lookup(RowPtr_CacheEntry cache[ROWPTR_CACHE_SIZE],
                         unsigned short rowidx,
                         unsigned int& out_ptr) {
    unsigned short idx = rowidx % ROWPTR_CACHE_SIZE; // simple hashing
    if (cache[idx].valid && cache[idx].tag == rowidx) {
        out_ptr = cache[idx].rowptr;
        return true; 
    }
    return false;
}

void rowptr_cache_insert(RowPtr_CacheEntry cache[ROWPTR_CACHE_SIZE],
                         unsigned short rowidx,
                         unsigned int ptr) {
    unsigned short idx = rowidx % ROWPTR_CACHE_SIZE;// simple hashing
    cache[idx] = {rowidx, ptr, true};
}

struct ColIdxVal_CacheEntry {
    unsigned int tag;   
    unsigned short col;
    B_dtype val;
    bool valid;
};

bool colidxval_cache_lookup(ColIdxVal_CacheEntry cache[COLIDXVAL_CACHE_SIZE],
                             unsigned int addr,
                             unsigned short burst_len,
                             hls::stream<unsigned short>& pmcu_col_out,
                             hls::stream<B_dtype>& pmcu_val_out,
                             unsigned short& blocks_served) {
    blocks_served = 0;
    for (unsigned short i = 0; i < burst_len; i++) {
        #pragma HLS pipeline II=1
        unsigned int lookup_addr = addr + i;
        unsigned short idx = lookup_addr % COLIDXVAL_CACHE_SIZE;
        if (cache[idx].valid && cache[idx].tag == lookup_addr) {
            pmcu_col_out.write(cache[idx].col);
            pmcu_val_out.write(cache[idx].val);
            blocks_served++;
        } else {
            return false;
        }
    }
    return true;
}

void MatB_Manager(
    hls::stream<MatB_Request>& reqFIFO,
    
    // REQUEST STREAMS
    hls::stream<RowPtr_Request>& rowptr_load_req,
    hls::stream<ColIdxVal_Request>& colidxval_load_req,

    // INPUT STREAMS
    hls::stream<unsigned int>& rowptr_fill,   
    hls::stream<unsigned short>& colidxval_fill_col,
    hls::stream<B_dtype>& colidxval_fill_val,

    // OUTPUT STREAMS
    hls::stream<MatB_Metadata>& meta_out,
    hls::stream<unsigned short>& pmcu_col_out,
    hls::stream<B_dtype>& pmcu_val_out
){
    RowPtr_CacheEntry    rp_cache[ROWPTR_CACHE_SIZE]       = {};
    ColIdxVal_CacheEntry cv_cache[COLIDXVAL_CACHE_SIZE]    = {};

    #pragma HLS bind_storage variable=rp_cache type=ram_1p impl=bram
    #pragma HLS bind_storage variable=cv_cache type=ram_1p impl=bram

    while (true) {
        MatB_Request req = reqFIFO.read();

        unsigned int rp_start = 0, rp_end = 0;

        if (req.first) {
            bool hit_start = rowptr_cache_lookup(rp_cache, req.rowidx, rp_start);
            bool hit_end   = rowptr_cache_lookup(rp_cache, req.rowidx + 1, rp_end);

            if (!hit_start || !hit_end) {
                rowptr_load_req.write({req.rowidx, req.src});
                rp_start = rowptr_fill.read();
                rp_end   = rowptr_fill.read();

                rowptr_cache_insert(rp_cache, req.rowidx, rp_start);
                rowptr_cache_insert(rp_cache, req.rowidx + 1, rp_end);
            }
        }
        unsigned int   cv_start = req.first ? rp_start       : req.cv_addr;
        unsigned int   row_nz   = req.first ? (rp_end - rp_start) : req.burst_len;
        unsigned short to_serve = (unsigned short)((row_nz < req.burst_len) ? row_nz : req.burst_len);

        unsigned short blocks_served = 0;
        bool full_hit = colidxval_cache_lookup(
            cv_cache, cv_start, to_serve,
            pmcu_col_out,
            pmcu_val_out,
            blocks_served
        );

        if (!full_hit) {
            colidxval_load_req.write({cv_start, to_serve, req.src});
            for (unsigned short i = 0; i < to_serve; i++) {
                #pragma HLS pipeline II=1
                unsigned short col = colidxval_fill_col.read();
                B_dtype v = colidxval_fill_val.read();

                unsigned int fill_addr = cv_start + i;
                unsigned short idx = fill_addr % COLIDXVAL_CACHE_SIZE;
                cv_cache[idx] = {fill_addr, col, v, true};

                pmcu_col_out.write(col);
                pmcu_val_out.write(v);
            }
            blocks_served = to_serve;
        }

        unsigned int next_addr  = cv_start + blocks_served;
        bool has_more = req.first ? (next_addr < rp_end) : false;
        meta_out.write(MatB_Metadata{next_addr, blocks_served, has_more});
    }
}

void serve_row_req(
    hls::stream<RowPtr_Request>& rowptr_load_req,
    hls::stream<unsigned int>& rowptr_fill,
    unsigned int* dram 
) {
    while (true) {
        RowPtr_Request req = rowptr_load_req.read();
        rowptr_fill.write(dram[req.addr]);
        rowptr_fill.write(dram[req.addr + 1]);
    }
}

void serve_cv_req(
    hls::stream<ColIdxVal_Request>& colidxval_load_req,
    hls::stream<unsigned short>& colidxval_fill_col,
    hls::stream<B_dtype>& colidxval_fill_val,
    unsigned short* dram_col,  
    B_dtype* dram_val        
) {
    while (true) {
        ColIdxVal_Request req = colidxval_load_req.read();

        for (unsigned short i = 0; i < req.burst_len; i++) {
            #pragma HLS pipeline II=1
            colidxval_fill_col.write(dram_col[req.addr + i]);
            colidxval_fill_val.write(dram_val[req.addr + i]);
        }
    }
}

void Loader(
    // INPUT STREAMS
    hls::stream<RowPtr_Request>&    rowptr_load_req,
    hls::stream<ColIdxVal_Request>& colidxval_load_req,

    // OUTPUT STREAMS
    hls::stream<unsigned int>&      rowptr_fill,
    hls::stream<unsigned short>&    colidxval_fill_col,
    hls::stream<B_dtype>&           colidxval_fill_val,

    // MEMORY ARGUMENTS
    unsigned int*                   dram_rowptr,
    unsigned short*                 dram_col,
    B_dtype*                        dram_val
) {
    hls_thread_local hls::task ROW(serve_row_req,
        rowptr_load_req, rowptr_fill, dram_rowptr);

    hls_thread_local hls::task CV(serve_cv_req,
        colidxval_load_req, colidxval_fill_col, colidxval_fill_val,
        dram_col, dram_val);
}

template<unsigned short p, unsigned short b>
void RequestRouter(
    hls::stream<MatB_Request> pmcu_req[p],      
    hls::stream<MatB_Request> bank_req[b]       
) {
    while (true) {
        for (unsigned short i = 0; i < p; i++) {
            #pragma HLS pipeline II=1
            if (!pmcu_req[i].empty()) {
                MatB_Request req = pmcu_req[i].read();
                req.src = i;                      
                unsigned short target = req.rowidx % b;
                bank_req[target].write(req);
            }
        }
    }
}

template<unsigned short p, unsigned short b>
void Xbar(
    hls::stream<MatB_Metadata> bank_meta[b],
    hls::stream<unsigned short> bank_col[b],
    hls::stream<B_dtype> bank_val[b],

    hls::stream<MatB_Metadata> pmcu_meta[p],
    hls::stream<unsigned short> pmcu_col[p],
    hls::stream<B_dtype> pmcu_val[p]
) {
    while (true) {
        for (unsigned short i = 0; i < b; i++) {
            #pragma HLS pipeline II=1
            if (!bank_meta[i].empty()) {
                MatB_Metadata meta = bank_meta[i].read();
                unsigned short col = bank_col[i].read();
                B_dtype        val = bank_val[i].read();

                pmcu_meta[meta.src].write(meta);   
                pmcu_col[meta.src].write(col);
                pmcu_val[meta.src].write(val);
            }
        }
    }
}

void Bank(
    hls::stream<MatB_Request>&   reqFIFO,
    hls::stream<MatB_Metadata>&  meta_out,
    hls::stream<unsigned short>& pmcu_col_out,
    hls::stream<B_dtype>&        pmcu_val_out,
    unsigned int*                dram_rowptr,
    unsigned short*              dram_col,
    B_dtype*                     dram_val
) {
    hls_thread_local hls::stream<RowPtr_Request, 4> rowptr_load_req;
    hls_thread_local hls::stream<ColIdxVal_Request, 4> colidxval_load_req;
    hls_thread_local hls::stream<unsigned int, 4> rowptr_fill;
    hls_thread_local hls::stream<unsigned short, 4> colidxval_fill_col;
    hls_thread_local hls::stream<B_dtype, 4> colidxval_fill_val;

    hls_thread_local hls::task manager(MatB_Manager,
        reqFIFO,
        rowptr_load_req, colidxval_load_req,
        rowptr_fill, colidxval_fill_col, colidxval_fill_val,
        meta_out, pmcu_col_out, pmcu_val_out
    );

    hls_thread_local hls::task loader(Loader,
        rowptr_load_req, colidxval_load_req,
        rowptr_fill, colidxval_fill_col, colidxval_fill_val,
        dram_rowptr, dram_col, dram_val
    );
}

template<unsigned short p, unsigned short b>
void MatBLoader(
    hls::stream<MatB_Request> pmcu_req[p],

    hls::stream<MatB_Metadata>  pmcu_meta[p],
    hls::stream<unsigned short> pmcu_col[p],
    hls::stream<B_dtype>        pmcu_val[p],

    unsigned int*   dram_rowptr[b],
    unsigned short* dram_col[b],
    B_dtype*        dram_val[b]
) {
    hls_thread_local hls::stream<MatB_Request,   4> bank_req[b];
    hls_thread_local hls::stream<MatB_Metadata,  4> bank_meta[b];
    hls_thread_local hls::stream<unsigned short, 4> bank_col[b];
    hls_thread_local hls::stream<B_dtype,        4> bank_val[b];

    hls_thread_local hls::task router(RequestRouter<p,b>,
        pmcu_req, bank_req
    );
    for (unsigned short i = 0; i < b; i++) {
        #pragma HLS unroll
        hls_thread_local hls::task bank_inst(Bank,
            bank_req[i],
            bank_meta[i], bank_col[i], bank_val[i],
            dram_rowptr[i], dram_col[i], dram_val[i]
        );
    }

    hls_thread_local hls::task xbar(Xbar<p,b>,
        bank_meta, bank_col, bank_val,
        pmcu_meta, pmcu_col, pmcu_val
    );
}