#pragma once

#include "hls_stream.h"
#include "hls_task.h"
#include "types.hpp"

struct MatB_Request {
    unsigned short src;
    unsigned short rowidx;
    bool           first;
    unsigned int   cv_addr;
    unsigned int   remaining;
    unsigned short burst_len;
};

struct MatB_Metadata {
    unsigned short src;
    unsigned int   cv_addr;
    unsigned short burst_len;
    unsigned int   remaining;
};

struct RowPtr_CacheEntry {
    unsigned short tag;
    unsigned int   rowptr;
    bool           valid;
};

struct ColIdxVal_CacheEntry {
    unsigned int   tag;
    unsigned short col;
    B_dtype        val;
    bool           valid;
};

inline bool rowptr_cache_lookup(RowPtr_CacheEntry cache[ROWPTR_CACHE_SIZE],
                                unsigned short rowidx,
                                unsigned int& out_ptr) {
    unsigned short idx = (unsigned short)(rowidx % ROWPTR_CACHE_SIZE);
    if (cache[idx].valid && cache[idx].tag == rowidx) {
        out_ptr = cache[idx].rowptr;
        return true;
    }
    return false;
}

inline void rowptr_cache_insert(RowPtr_CacheEntry cache[ROWPTR_CACHE_SIZE],
                                unsigned short rowidx,
                                unsigned int ptr) {
    unsigned short idx = (unsigned short)(rowidx % ROWPTR_CACHE_SIZE);
    cache[idx] = RowPtr_CacheEntry{rowidx, ptr, true};
}

inline bool colidxval_cache_lookup(ColIdxVal_CacheEntry cache[COLIDXVAL_CACHE_SIZE],
                                   unsigned int addr,
                                   unsigned short burst_len,
                                   hls::stream<unsigned short>& pmcu_col_out,
                                   hls::stream<B_dtype>& pmcu_val_out) {
    for (unsigned short i = 0; i < burst_len; ++i) {
        #pragma HLS pipeline II=1
        unsigned int lookup_addr = addr + i;
        unsigned short idx = (unsigned short)(lookup_addr % COLIDXVAL_CACHE_SIZE);
        if (!cache[idx].valid || cache[idx].tag != lookup_addr) {
            return false;
        }
    }

    for (unsigned short i = 0; i < burst_len; ++i) {
        #pragma HLS pipeline II=1
        unsigned int lookup_addr = addr + i;
        unsigned short idx = (unsigned short)(lookup_addr % COLIDXVAL_CACHE_SIZE);
        pmcu_col_out.write(cache[idx].col);
        pmcu_val_out.write(cache[idx].val);
    }
    return true;
}

inline void MatB_Manager(hls::stream<MatB_Request>& reqFIFO,
                         hls::stream<MatB_Metadata>& meta_out,
                         hls::stream<unsigned short>& pmcu_col_out,
                         hls::stream<B_dtype>& pmcu_val_out,
                         const unsigned int* dram_rowptr,
                         const unsigned short* dram_col,
                         const B_dtype* dram_val) {
    RowPtr_CacheEntry rp_cache[ROWPTR_CACHE_SIZE] = {};
    ColIdxVal_CacheEntry cv_cache[COLIDXVAL_CACHE_SIZE] = {};

    #pragma HLS bind_storage variable=rp_cache type=ram_1p impl=bram
    #pragma HLS bind_storage variable=cv_cache type=ram_1p impl=bram

    while (true) {
        // For simulation, break when no more requests and done
        #ifndef __SYNTHESIS__
        if (reqFIFO.empty()) {
            break;
        }
        #endif
        
        MatB_Request req = reqFIFO.read();

        unsigned int rp_start = 0;
        unsigned int rp_end = 0;
        if (req.first) {
            bool hit_start = rowptr_cache_lookup(rp_cache, req.rowidx, rp_start);
            bool hit_end = rowptr_cache_lookup(rp_cache, (unsigned short)(req.rowidx + 1), rp_end);
            if (!hit_start) {
                rp_start = dram_rowptr[req.rowidx];
                rowptr_cache_insert(rp_cache, req.rowidx, rp_start);
            }
            if (!hit_end) {
                rp_end = dram_rowptr[req.rowidx + 1];
                rowptr_cache_insert(rp_cache, (unsigned short)(req.rowidx + 1), rp_end);
            }
        } else {
            rp_start = req.cv_addr;
            rp_end = req.cv_addr + req.remaining;
        }

        unsigned int cv_start = req.first ? rp_start : req.cv_addr;
        unsigned int available = req.first ? (rp_end - rp_start) : req.remaining;
        unsigned short to_serve =
            (unsigned short)((available < req.burst_len) ? available : req.burst_len);

        bool full_hit = colidxval_cache_lookup(
            cv_cache, cv_start, to_serve, pmcu_col_out, pmcu_val_out
        );

        if (!full_hit) {
            for (unsigned short i = 0; i < to_serve; ++i) {
                #pragma HLS pipeline II=1
                unsigned int fill_addr = cv_start + i;
                unsigned short idx = (unsigned short)(fill_addr % COLIDXVAL_CACHE_SIZE);
                unsigned short col = dram_col[fill_addr];
                B_dtype val = dram_val[fill_addr];

                cv_cache[idx] = ColIdxVal_CacheEntry{fill_addr, col, val, true};
                pmcu_col_out.write(col);
                pmcu_val_out.write(val);
            }
        }

        meta_out.write(MatB_Metadata{
            req.src,
            cv_start + to_serve,
            to_serve,
            available - to_serve
        });
    }
}

template<unsigned short p, unsigned short b>
void RequestRouter(hls::stream<MatB_Request> pmcu_req[p],
                   hls::stream<MatB_Request> bank_req[b]) {
    #ifndef __SYNTHESIS__
    unsigned int processed = 0;
    // Count total requests (simplified for simulation)
    unsigned int total_requests = 0;
    for (unsigned short i = 0; i < p; ++i) {
        while (!pmcu_req[i].empty()) {
            pmcu_req[i].read();
            total_requests++;
        }
    }
    // Re-push? For simulation simplicity, we'll skip router
    #else
    while (true) {
        for (unsigned short i = 0; i < p; ++i) {
            #pragma HLS pipeline II=1
            if (!pmcu_req[i].empty()) {
                MatB_Request req = pmcu_req[i].read();
                req.src = i;
                unsigned short target = (unsigned short)(req.rowidx % b);
                bank_req[target].write(req);
            }
        }
    }
    #endif
}

template<unsigned short p, unsigned short b>
void Xbar(hls::stream<MatB_Metadata> bank_meta[b],
          hls::stream<unsigned short> bank_col[b],
          hls::stream<B_dtype> bank_val[b],
          hls::stream<MatB_Metadata> pmcu_meta[p],
          hls::stream<unsigned short> pmcu_col[p],
          hls::stream<B_dtype> pmcu_val[p]) {
    #ifdef __SYNTHESIS__
    while (true) {
        for (unsigned short i = 0; i < b; ++i) {
            #pragma HLS pipeline II=1
            if (!bank_meta[i].empty()) {
                MatB_Metadata meta = bank_meta[i].read();
                pmcu_meta[meta.src].write(meta);
                for (unsigned short j = 0; j < meta.burst_len; ++j) {
                    #pragma HLS pipeline II=1
                    pmcu_col[meta.src].write(bank_col[i].read());
                    pmcu_val[meta.src].write(bank_val[i].read());
                }
            }
        }
    }
    #endif
}

inline void Bank(hls::stream<MatB_Request>& reqFIFO,
                 hls::stream<MatB_Metadata>& meta_out,
                 hls::stream<unsigned short>& pmcu_col_out,
                 hls::stream<B_dtype>& pmcu_val_out,
                 const unsigned int* dram_rowptr,
                 const unsigned short* dram_col,
                 const B_dtype* dram_val) {
    MatB_Manager(
        reqFIFO, meta_out, pmcu_col_out, pmcu_val_out,
        dram_rowptr, dram_col, dram_val
    );
}

template<unsigned short p, unsigned short b>
void MatBLoader(hls::stream<MatB_Request> pmcu_req[p],
                hls::stream<MatB_Metadata> pmcu_meta[p],
                hls::stream<unsigned short> pmcu_col[p],
                hls::stream<B_dtype> pmcu_val[p],
                const unsigned int* dram_rowptr[b],
                const unsigned short* dram_col[b],
                const B_dtype* dram_val[b]) {
    #ifdef __SYNTHESIS__
    #pragma HLS dataflow
    #endif
    
    hls::stream<MatB_Request, STREAM_DEPTH> bank_req[b];
    hls::stream<MatB_Metadata, STREAM_DEPTH> bank_meta[b];
    hls::stream<unsigned short, STREAM_DEPTH> bank_col[b];
    hls::stream<B_dtype, STREAM_DEPTH> bank_val[b];

    RequestRouter<p, b>(pmcu_req, bank_req);

    for (unsigned short i = 0; i < b; ++i) {
        #pragma HLS unroll
        Bank(bank_req[i], bank_meta[i], bank_col[i], bank_val[i],
             dram_rowptr[i], dram_col[i], dram_val[i]);
    }

    #ifdef __SYNTHESIS__
    Xbar<p, b>(bank_meta, bank_col, bank_val, pmcu_meta, pmcu_col, pmcu_val);
    #endif
}