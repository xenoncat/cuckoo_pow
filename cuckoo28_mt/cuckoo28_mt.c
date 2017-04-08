#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <x86intrin.h>
#include <pthread.h>
#include "siphash_avx2.h"
#include "memlayout28_mt.h"

#define PRINTSTAT 1
#define PRINTBUCKETSTAT 1

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

static const __m256i yconst1 = {1, 1, 1, 1};
static const __m256i yconstff = {0xff, 0xff, 0xff, 0xff};
static const __m256i yconstrol16 = {0x0504030201000706, 0x0d0c0b0a09080f0e, 0x0504030201000706, 0x0d0c0b0a09080f0e};
static const __m256i yconstnonceinc = {8, 8, 8, 8};
static const __m256i yconstnoncemask = {NONCEMASK, NONCEMASK, NONCEMASK, NONCEMASK};
static const __m256i yconstnoncemaske = {NONCEMASKE, NONCEMASKE, NONCEMASKE, NONCEMASKE};
static const __m256i yconstnodemask = {NODEMASK, NODEMASK, NODEMASK, NODEMASK};
static const __m256i yconstbucketmask = {B_BUCKETMASK, B_BUCKETMASK, B_BUCKETMASK, B_BUCKETMASK};
static const __m256i yconstslotedgeinc = {4<<B_SLOTPAYLOADSHIFT, 4<<B_SLOTPAYLOADSHIFT, 4<<B_SLOTPAYLOADSHIFT, 4<<B_SLOTPAYLOADSHIFT};

typedef struct {
    void *ctx;
    uint32_t tid;
    uint32_t pad;
} tctx_t;

//4 42 56 (nonce=74)
static uint64_t k0def = 0x63ddd08f6085f61c;
static uint64_t k1def = 0xac831091192e1473;

static void TrimEdgeIter3(void * const context, void * const dst, void * const src, const uint32_t tid, const uint32_t iter)
{
    void * const ctx = __builtin_assume_aligned(context, 64);
    uint32_t * const pu32dst = __builtin_assume_aligned(dst, 64) + B_BUCKETS * 4 * tid;
    void * const pvdstbody = __builtin_assume_aligned(dst, 64) +  B_BUCKETS * THREADS * 4;
    void * const pvbighead = __builtin_assume_aligned(src, 64);
    void * const pvbigbody = pvbighead + B_BUCKETS * THREADS * 4;
    uint8_t * const pu8deg = ctx + CK_PTS1 + PTS1_SIZE * tid;
    void * const pvsmall0 = pu8deg + R_ENTRIES;
    uint32_t * const pu32end = pvsmall0 + S_BUCKETS * 4;
    uint32_t * const pu32abort = ctx + CK_ABORT;
    
    uint64_t *pu64readbig, *pu64readsmall;
    uint32_t *pu32big, *pu32small, *pu32readsmall;
    void *pvbig, *pvlimit, *pvlimit2, *pvsmall;
    uint64_t rdtsc0, rdtsc1;
    uint64_t q0, q1, q2, q3, q4;
    uint32_t bigbucket, bigbucketend, fromthread, smallbucket, edgecount, dbgctr=0;
    uint32_t d0, d1, d2, d3;
    
    rdtsc0 = __rdtsc();
    d0 = B3_SLOTS * B3_SLOTSIZE * tid;
    for (d1=0; d1<B_BUCKETS; d1++) {
        pu32dst[d1] = d0;
        d0 += THREADS*B3_SLOTS*B3_SLOTSIZE;
    }
    q2 = 0;     //Technically needs tid here, but 5-byte slots truncates it anyway
    bigbucket = tid * BPT;
    bigbucketend = bigbucket + BPT;
    pu32big = pvbighead;
    pvbig = pvbigbody + bigbucket * (THREADS*B2_SLOTS*B2_SLOTSIZE);
    d0 = (bigbucket!=0) ? (pu32big[(THREADS-1)*B_BUCKETS+bigbucket-1] + B2_SLOTSIZE*2) : 0;
    pvlimit = pvbigbody + d0;
    pu64readbig = (pvlimit > pvbig) ? pvlimit : pvbig;
    
    for (; bigbucket<bigbucketend; bigbucket++) {
        pu32small = pvsmall0;
        d0 = S_BUCKETS*4;
        do {
            *(pu32small++) = d0;
            d0 += S1_SLOTS*S1_SLOTSIZE;
        } while (pu32small<pu32end);
        pu32small = pvsmall0;
        
        q3 = 0;
        q4 = 0;
        for (fromthread=0; fromthread<THREADS*B_BUCKETS; fromthread+=B_BUCKETS) {
            pvlimit = pvbigbody + pu32big[fromthread+bigbucket];
            pvbig += B2_SLOTS*B2_SLOTSIZE;  //pvbig is now the beginning of the next bucket
            pvlimit2 = (pvbig<pvlimit) ? pvbig : pvlimit;
            while ((void*) pu64readbig < pvlimit2) {
                q0 = *pu64readbig;
                d0 = q0 & S_BUCKETMASK;
                q1 = q0 & 0xffc0000000;
                q4 += (q1 < q3) ? 0x100000000 : 0;
                q3 = q1;
                d1 = pu32small[d0];
                d2 = q0 >> S_LOG;   //truncate to uint32_t is important, alternatively use AND mask
                *(uint64_t*) (pvsmall0+d1) = q4 | (uint64_t) d2;
                pu32small[d0] += S1_SLOTSIZE;
                pu64readbig = ((void*)pu64readbig) + B2_SLOTSIZE;
            }
            pvlimit += B2_SLOTSIZE*2;       //Two more spaces because 8-byte writes were used to write 5-byte slots
            pu64readbig = (pvlimit > pvbig) ? pvlimit : pvbig;  //handle big bucket overflow
        }
        if (unlikely(q4 != 0x3f00000000)) {
            printf("Error Iter3 %lx\n", q4);
            *pu32abort = 1;
            goto _GiveUpIt3;
        }
        pvsmall = pvsmall0 + S_BUCKETS*4;
        pu32readsmall = pvsmall;
        for (smallbucket=0; smallbucket<S_BUCKETS; smallbucket++) {
            pu64readsmall = (uint64_t*) pu32readsmall;
            memset(pu8deg, 0, R_ENTRIES);
            pvlimit = pvsmall0 + pu32small[smallbucket];
            while ((void*) pu32readsmall < pvlimit) {
                pu8deg[*pu32readsmall & (R_ENTRIES-1)]++;
                pu32readsmall = ((void*)pu32readsmall) + S1_SLOTSIZE;
            }
            while ((void*) pu64readsmall < pvlimit) {
                q0 = *pu64readsmall;
                d0 = (q0>>30) & 0xff;
                d1 = q0 & (R_ENTRIES-1);
                q1 = ((q0>>R_LOG) & (1ULL<<(LOGHALFSIZE-B_LOG))-1);
                q1 |= ((uint64_t) d1) << 19;
                q1 |= q2;
                *(uint64_t*) (pvdstbody + pu32dst[d0]) = q1;
                pu32dst[d0] += (pu8deg[d1]>1) ? B3_SLOTSIZE : 0;
                pu64readsmall = ((void*)pu64readsmall) + S1_SLOTSIZE;
            }
            pvsmall += S1_SLOTS*S1_SLOTSIZE;    //advance to the next bucket
            pvlimit += S1_SLOTSIZE*2;           //Two more spaces because 8-byte writes were used to write 5-byte slots
            pu32readsmall = (pvlimit > pvsmall) ? pvlimit : pvsmall;    //handle small bucket overflow
            q2 += (1ULL<<30);
        }
    }
    
    _GiveUpIt3:
    pthread_barrier_wait(ctx+CK_BARRIER);
    rdtsc1 = __rdtsc();
#if PRINTSTAT
    if (tid==0) {
        d2 = 0;
        edgecount = 0;
        dbgctr = 0;
        for (d0=0; d0<B_BUCKETS; d0++) {
            for (d1=0; d1<THREADS; d1++) {
                d3 = d1 * B_BUCKETS + d0;
                edgecount += (pu32dst[d3]-d2);
                dbgctr = ((pu32dst[d3]-d2) > dbgctr) ? (pu32dst[d3]-d2) : dbgctr;
                d2 += B3_SLOTS*B3_SLOTSIZE;
            }
        }
        edgecount /= B3_SLOTSIZE;
        dbgctr /= B3_SLOTSIZE;
        printf("Iteration %d: %d; max: %d, rdtsc: %ld\n", iter, edgecount, dbgctr, rdtsc1-rdtsc0);
    }
#endif
}

static uint32_t TrimRename1(void * const context, void * const dst, void * const src, const uint32_t tid, const uint32_t iter)
{
    void * const ctx = __builtin_assume_aligned(context, 64);
    uint32_t * const pu32dst = __builtin_assume_aligned(dst, 64) + B_BUCKETS * 4 * tid;
    void * const pvdstbody = __builtin_assume_aligned(dst, 64) +  B_BUCKETS * THREADS * 4;
    void * const pvbighead = __builtin_assume_aligned(src, 64);
    void * const pvbigbody = pvbighead + B_BUCKETS * THREADS * 4;
    uint8_t * const pu8deg = ctx + CK_PTS1 + PTS1_SIZE * tid;
    void * const pvsmall0 = pu8deg + R_ENTRIES;
    uint32_t * const pu32end = pvsmall0 + S_BUCKETS * 4;
    uint32_t * const pu32abort = ctx + CK_ABORT;
    
    uint64_t *pu64readbig, *pu64readsmall;
    uint32_t *pu32big, *pu32small, *pu32readsmall;
    void *pvbig, *pvlimit, *pvlimit2, *pvsmall;
    uint64_t rdtsc0, rdtsc1;
    uint64_t q0, q1, q2, q3, q4;
    uint32_t bigbucket, bigbucketend, fromthread, smallbucket, edgecount, ramsize, dbgctr=0;
    uint32_t d0, d1, d2, d3;
    int32_t newnodeid=tid, newnodeid_small;
    uint8_t b0;
    
    rdtsc0 = __rdtsc();
    d0 = BR_SLOTS * BR_SLOTSIZE * tid;
    for (d1=0; d1<B_BUCKETS; d1++) {
        pu32dst[d1] = d0;
        d0 += THREADS*BR_SLOTS*BR_SLOTSIZE;
    }
    bigbucket = tid * BPT;
    bigbucketend = bigbucket + BPT;
    pvbig = pvbigbody + bigbucket * (THREADS*B3_SLOTS*B3_SLOTSIZE);
    pu32big = pvbighead;
    d0 = (bigbucket!=0) ? (pu32big[(THREADS-1)*B_BUCKETS+bigbucket-1] + B3_SLOTSIZE*2) : 0;
    pvlimit = pvbigbody + d0;
    pu64readbig = (pvlimit > pvbig) ? pvlimit : pvbig;
    
    for (; bigbucket<bigbucketend; bigbucket++) {
        pu32small = pvsmall0;
        d0 = S_BUCKETS*4;
        do {
            *(pu32small++) = d0;
            d0 += SR_SLOTS*SR_SLOTSIZE;
        } while (pu32small<pu32end);
        pu32small = pvsmall0;
        
        q3 = 0;
        q4 = 0;
        for (fromthread=0; fromthread<THREADS*B_BUCKETS; fromthread+=B_BUCKETS) {
            pvlimit = pvbigbody + pu32big[fromthread+bigbucket];
            pvbig += B3_SLOTS*B3_SLOTSIZE;  //pvbig is now the beginning of the next bucket
            pvlimit2 = (pvbig<pvlimit) ? pvbig : pvlimit;
            while ((void*) pu64readbig < pvlimit2) {
                q0 = *pu64readbig;
                d0 = q0 & S_BUCKETMASK;
                q1 = q0 & 0xffc0000000;
                q4 += (q1 < q3) ? 0x100000000 : 0;
                q3 = q1;
                d1 = pu32small[d0];
                d2 = q0 >> S_LOG;   //truncate to uint32_t is important, alternatively use AND mask
                *(uint64_t*) (pvsmall0+d1) = q4 | (uint64_t) d2;
                pu32small[d0] += SR_SLOTSIZE;
                pu64readbig = ((void*)pu64readbig) + B3_SLOTSIZE;
            }
            pvlimit += B3_SLOTSIZE*2;       //Two more spaces because 8-byte writes were used to write 5-byte slots
            pu64readbig = (pvlimit > pvbig) ? pvlimit : pvbig;  //handle big bucket overflow
        }
        if (unlikely(q4 != 0x3f00000000)) {
            printf("Error Rename1 %lx\n", q4);
            *pu32abort = 1;
            goto _GiveUpRen1;
        }
        pvsmall = pvsmall0 + S_BUCKETS*4;
        pu32readsmall = pvsmall;
        for (smallbucket=0; smallbucket<S_BUCKETS; smallbucket++) {
            pu64readsmall = (uint64_t*) pu32readsmall;
            memset(pu8deg, 0, R_ENTRIES);
            pvlimit = pvsmall0 + pu32small[smallbucket];
            newnodeid_small = 31;
            while ((void*) pu32readsmall < pvlimit) {
                pu8deg[*pu32readsmall & (R_ENTRIES-1)]++;
                pu32readsmall = ((void*)pu32readsmall) + SR_SLOTSIZE;
            }
            while ((void*) pu64readsmall < pvlimit) {
                q0 = *pu64readsmall;
                d2 = q0 & (R_ENTRIES-1);
                b0 = pu8deg[d2]; 
                if (b0 > 1) {
                    newnodeid_small += (b0<32) ? 1 : 0;
                    b0 = (b0<32) ? (uint8_t) newnodeid_small : b0;
                    pu8deg[d2] = b0;
                    d0 = (q0>>30) & B_BUCKETMASK;
                    d1 = (uint32_t) b0 << LOGTHREADS;
                    q1 = ((uint64_t) (newnodeid+d1)) << (LOGHALFSIZE-B_LOG);
                    q1 |= ((q0>>R_LOG) & (1ULL<<(LOGHALFSIZE-B_LOG))-1);
                    *(uint64_t*) (pvdstbody + pu32dst[d0]) = q1;
                    pu32dst[d0] += BR_SLOTSIZE;
                }
                pu64readsmall = ((void*)pu64readsmall) + SR_SLOTSIZE;
            }
            newnodeid = newnodeid + ((newnodeid_small - 31) << LOGTHREADS);
            pvsmall += SR_SLOTS*SR_SLOTSIZE;    //advance to the next bucket
            pvlimit += SR_SLOTSIZE*2;           //Two more spaces because 8-byte writes were used to write 6-byte slots
            pu32readsmall = (pvlimit > pvsmall) ? pvlimit : pvsmall;    //handle small bucket overflow
        }
    }

    _GiveUpRen1:
    *(int32_t*) (ctx + CK_THREADRESULT0 + tid*4) = newnodeid;
    pthread_barrier_wait(ctx+CK_BARRIER);
    if (unlikely(*pu32abort))
        return 0xffffffff;
    
    d0 = 0;
    for (d1=0; d1<THREADS; d1++) {
        d2 = *(int32_t*) (ctx + CK_THREADRESULT0 + d1*4);
        d0 = (d2>d0) ? d2 : d0;
    }
    d0 = (d0+31) >> B_LOG;  //max address of ram array
    ramsize = (d0>>5) + 1;  //number of 32-byte blocks
        
    rdtsc1 = __rdtsc();
    
#if PRINTSTAT
    if (tid==0) {
        d2 = 0;
        edgecount = 0;
        dbgctr = 0;
        for (d0=0; d0<B_BUCKETS; d0++) {
            for (d1=0; d1<THREADS; d1++) {
                d3 = d1 * B_BUCKETS + d0;
                edgecount += (pu32dst[d3]-d2);
                dbgctr = ((pu32dst[d3]-d2) > dbgctr) ? (pu32dst[d3]-d2) : dbgctr;
                d2 += BR_SLOTS*BR_SLOTSIZE;
            }
        }
        edgecount /= BR_SLOTSIZE;
        dbgctr /= BR_SLOTSIZE;
        printf("Iteration %d: %d; max: %d, rdtsc: %ld\n", iter, edgecount, dbgctr, rdtsc1-rdtsc0);
        d0 = 0;
        for (d1=0; d1<THREADS; d1++) {
            d2 = *(int32_t*) (ctx + CK_THREADRESULT0 + d1*4);
            d0 += d2 >> LOGTHREADS;
        }
        printf("nodecount=%d\n", d0);
    }
#endif
    return ramsize;
}

static uint32_t TrimRename2(void * const context, void * const dst, void * const src, const uint32_t tid, const uint32_t iter)
{
    void * const ctx = __builtin_assume_aligned(context, 64);
    uint32_t * const pu32dst = __builtin_assume_aligned(dst, 64) + B_BUCKETS * 4 * tid;
    void * const pvdstbody = __builtin_assume_aligned(dst, 64) +  B_BUCKETS * THREADS * 4;
    void * const pvbighead = __builtin_assume_aligned(src, 64);
    void * const pvbigbody = pvbighead + B_BUCKETS * THREADS * 4;
    uint8_t * const pu8deg = ctx + CK_PTS1 + PTS1_SIZE * tid;
    void * const pvsmall0 = pu8deg + R_ENTRIES;
    uint32_t * const pu32end = pvsmall0 + S_BUCKETS * 4;
    
    uint64_t *pu64readbig, *pu64readsmall;
    uint32_t *pu32big, *pu32small, *pu32readsmall;
    uint32_t* pu32renamelist = ctx + CK_RENAMELIST + tid * (33554432/THREADS);
    void *pvbig, *pvlimit, *pvlimit2, *pvsmall;
    uint64_t rdtsc0, rdtsc1;
    uint64_t q0, q1;
    uint32_t bigbucket, bigbucketend, flatbucket, fromthread, smallbucket, edgecount, ramsize, dbgctr=0;
    uint32_t d0, d1, d2, d3;
    int32_t newnodeid=tid, newnodeid_small;
    uint8_t b0;
    
    rdtsc0 = __rdtsc();
    d0 = B3_SLOTS * B3_SLOTSIZE * tid;
    for (d1=0; d1<B_BUCKETS; d1++) {
        pu32dst[d1] = d0;
        d0 += THREADS*B3_SLOTS*B3_SLOTSIZE;
    }
    bigbucket = tid * BPT;
    bigbucketend = bigbucket + BPT;
    pvbig = pvbigbody + bigbucket * (THREADS*BR_SLOTS*BR_SLOTSIZE);
    pu32big = pvbighead;
    flatbucket = tid * (HALFSIZE/THREADS);
    d0 = (bigbucket!=0) ? (pu32big[(THREADS-1)*B_BUCKETS+bigbucket-1] + BR_SLOTSIZE*2) : 0;
    pvlimit = pvbigbody + d0;
    pu64readbig = (pvlimit > pvbig) ? pvlimit : pvbig;

    for (; bigbucket<bigbucketend; bigbucket++) {
        pu32small = pvsmall0;
        d0 = S_BUCKETS*4;
        do {
            *(pu32small++) = d0;
            d0 += S1_SLOTS*S1_SLOTSIZE;
        } while (pu32small<pu32end);
        pu32small = pvsmall0;
        
        for (fromthread=0; fromthread<THREADS*B_BUCKETS; fromthread+=B_BUCKETS) {
            pvlimit = pvbigbody + pu32big[fromthread+bigbucket];
            pvbig += BR_SLOTS*BR_SLOTSIZE;  //pvbig is now the beginning of the next bucket
            pvlimit2 = (pvbig<pvlimit) ? pvbig : pvlimit;
            while ((void*) pu64readbig < pvlimit2) {
                q0 = *pu64readbig;
                d0 = q0 & S_BUCKETMASK;
                d1 = pu32small[d0];
                *(uint64_t*) (pvsmall0+d1) = (q0 >> S_LOG); //Mask 0x03ffffffff omitted
                pu32small[d0] += S1_SLOTSIZE;
                pu64readbig = ((void*)pu64readbig) + BR_SLOTSIZE;
            }
            pvlimit += BR_SLOTSIZE*2;       //Two more spaces because 8-byte writes were used to write 6-byte slots
            pu64readbig = (pvlimit > pvbig) ? pvlimit : pvbig;  //handle big bucket overflow
        }
        pvsmall = pvsmall0 + S_BUCKETS*4;
        pu32readsmall = pvsmall;
        for (smallbucket=0; smallbucket<S_BUCKETS; smallbucket++) {
            pu64readsmall = (uint64_t*) pu32readsmall;
            memset(pu8deg, 0, R_ENTRIES);
            pvlimit = pvsmall0 + pu32small[smallbucket];
            newnodeid_small = 31;
            while ((void*) pu32readsmall < pvlimit) {
                pu8deg[*pu32readsmall & (R_ENTRIES-1)]++;
                pu32readsmall = ((void*)pu32readsmall) + S1_SLOTSIZE;
            }
            while ((void*) pu64readsmall < pvlimit) {
                q0 = *pu64readsmall;
                d2 = q0 & (R_ENTRIES-1);
                b0 = pu8deg[d2]; 
                if (b0 > 1) {
                    if (b0 < 32) {
                        newnodeid_small++;
                        b0 = (uint8_t) newnodeid_small;
                        pu8deg[d2] = b0;
                        pu32renamelist[b0] = flatbucket | d2;
                    }
                    d0 = (q0>>R_LOG) & B_BUCKETMASK;
                    d1 = (uint32_t) b0 << LOGTHREADS;
                    q1 = ((uint64_t) (newnodeid+d1)) << (23-B_LOG);
                    q1 |= (q0 >> (R_LOG + B_LOG)) & 0x7fff;
                    *(uint64_t*) (pvdstbody + pu32dst[d0]) = q1;
                    pu32dst[d0] += B3_SLOTSIZE;
                }
                pu64readsmall = ((void*)pu64readsmall) + S1_SLOTSIZE;
            }
            newnodeid = newnodeid + ((newnodeid_small - 31) << LOGTHREADS);
            pu32renamelist += newnodeid_small - 31;
            pvsmall += S1_SLOTS*S1_SLOTSIZE;    //advance to the next bucket
            pvlimit += S1_SLOTSIZE*2;           //Two more spaces because 8-byte writes were used to write 5-byte slots
            pu32readsmall = (pvlimit > pvsmall) ? pvlimit : pvsmall;    //handle small bucket overflow
            flatbucket += R_ENTRIES;
        }
    }
    *(int32_t*) (ctx + CK_THREADRESULT1 + tid*4) = newnodeid;
    pthread_barrier_wait(ctx+CK_BARRIER);
    
    d0 = 0;
    for (d1=0; d1<THREADS; d1++) {
        d2 = *(int32_t*) (ctx + CK_THREADRESULT1 + d1*4);
        d0 = (d2>d0) ? d2 : d0;
    }
    d0 = (d0+31) >> B_LOG;  //max address of ram array
    ramsize = (d0>>5) + 1;  //number of 32-byte blocks
    
    rdtsc1 = __rdtsc();
    
#if PRINTSTAT
    if (tid==0) {
        d2 = 0;
        edgecount = 0;
        dbgctr = 0;
        for (d0=0; d0<B_BUCKETS; d0++) {
            for (d1=0; d1<THREADS; d1++) {
                d3 = d1 * B_BUCKETS + d0;
                edgecount += (pu32dst[d3]-d2);
                dbgctr = ((pu32dst[d3]-d2) > dbgctr) ? (pu32dst[d3]-d2) : dbgctr;
                d2 += B3_SLOTS*B3_SLOTSIZE;
            }
        }
        edgecount /= B3_SLOTSIZE;
        dbgctr /= B3_SLOTSIZE;
        printf("Iteration %d: %d; max: %d, rdtsc: %ld\n", iter, edgecount, dbgctr, rdtsc1-rdtsc0);
        d0 = 0;
        for (d1=0; d1<THREADS; d1++) {
            d2 = *(int32_t*) (ctx + CK_THREADRESULT1 + d1*4);
            d0 += d2 >> LOGTHREADS;
        }
        printf("nodecount=%d\n", d0);
    }
#endif
    return ramsize;
}

static void TrimEdge1Stage(void * const context, void * const dst, void * const src, const uint32_t tid, const uint32_t ramsize, const uint32_t iter)
{
    void * const ctx = __builtin_assume_aligned(context, 64);
    uint32_t * const pu32dst = __builtin_assume_aligned(dst, 64) + B_BUCKETS * 4 * tid;
    void * const pvdstbody = __builtin_assume_aligned(dst, 64) +  B_BUCKETS * THREADS * 4;
    void * const pvsrchead = __builtin_assume_aligned(src, 64);
    void * const pvsrcbody = pvsrchead + B_BUCKETS * THREADS * 4;
    uint8_t * const pu8deg = ctx + CK_PTS1 + PTS1_SIZE * tid;
    
    uint64_t *pu64readsrc;
    uint32_t *pu32src, *pu32readsrc;
    void *pvsrc, *pvsrc2, *pvlimit, *pvlimit2;
    uint64_t rdtsc0, rdtsc1;
    uint64_t q0;
    uint32_t srcbucket, srcbucketend, fromthread, edgecount, dbgctr=0;
    uint32_t d0, d1, d2, d3;
    
    rdtsc0 = __rdtsc();
    d0 = B3_SLOTS * B3_SLOTSIZE * tid;
    for (d1=0; d1<B_BUCKETS; d1++) {
        pu32dst[d1] = d0;
        d0 += THREADS*B3_SLOTS*B3_SLOTSIZE;
    }
    srcbucket = tid * BPT;
    srcbucketend = srcbucket + BPT;
    pvsrc = pvsrcbody + srcbucket * (THREADS*B3_SLOTS*B3_SLOTSIZE);
    pu32src = pvsrchead;
    d3 = srcbucket << 15;
    d0 = (srcbucket!=0) ? (pu32src[(THREADS-1)*B_BUCKETS+srcbucket-1] + B3_SLOTSIZE*2) : 0;
    pvlimit = pvsrcbody + d0;
    pu32readsrc = (pvlimit > pvsrc) ? pvlimit : pvsrc;

    for (; srcbucket<srcbucketend; srcbucket++) {
        memset(pu8deg, 0, ramsize*32);
        
        pu64readsrc = (uint64_t*) pu32readsrc;
        pvsrc2 = pvsrc;
        for (fromthread=0; fromthread<THREADS*B_BUCKETS; fromthread+=B_BUCKETS) {
            pvlimit = pvsrcbody + pu32src[fromthread+srcbucket];
            pvsrc += B3_SLOTS*B3_SLOTSIZE;  //pvsrc is now the beginning of the next bucket
            pvlimit2 = (pvsrc<pvlimit) ? pvsrc : pvlimit;
            while ((void*) pu32readsrc < pvlimit2) {
                pu8deg[*pu32readsrc & 0x7fff]++;
                pu32readsrc = ((void*)pu32readsrc) + B3_SLOTSIZE;
            }
            pvlimit += B3_SLOTSIZE*2;       //Two more spaces because 8-byte writes were used to write 5-byte slots
            pu32readsrc = (pvlimit > pvsrc) ? pvlimit : pvsrc;  //handle bucket overflow
        }
        
        for (fromthread=0; fromthread<THREADS*B_BUCKETS; fromthread+=B_BUCKETS) {
            pvlimit = pvsrcbody + pu32src[fromthread+srcbucket];
            pvsrc2 += B3_SLOTS*B3_SLOTSIZE;  //pvsrc2 is now the beginning of the next bucket
            pvlimit2 = (pvsrc2<pvlimit) ? pvsrc2 : pvlimit;
            while ((void*) pu64readsrc < pvlimit2) {
                q0 = *pu64readsrc;
                d0 = q0 & 0x7fff;
                d1 = q0 >> 15;
                d2 = d1 & B_BUCKETMASK;
                d1 = (d1 >> 8) & 0x7fff;
                *(uint64_t*) (pvdstbody + pu32dst[d2]) = (((uint64_t) d0) << 23) | d3 | d1;
                pu32dst[d2] += (pu8deg[d0] > 1) ? B3_SLOTSIZE : 0;
                pu64readsrc = ((void*)pu64readsrc) + B3_SLOTSIZE;
            }
            pvlimit += B3_SLOTSIZE*2;       //Two more spaces because 8-byte writes were used to write 5-byte slots
            pu64readsrc = (pvlimit > pvsrc2) ? pvlimit : pvsrc2;  //handle bucket overflow
        }
        d3 += (1 << 15);
    }

    pthread_barrier_wait(ctx+CK_BARRIER);
    rdtsc1 = __rdtsc();
#if PRINTSTAT
    if (tid==0) {
        d2 = 0;
        edgecount = 0;
        dbgctr = 0;
        for (d0=0; d0<B_BUCKETS; d0++) {
            for (d1=0; d1<THREADS; d1++) {
                d3 = d1 * B_BUCKETS + d0;
                edgecount += (pu32dst[d3]-d2);
                dbgctr = ((pu32dst[d3]-d2) > dbgctr) ? (pu32dst[d3]-d2) : dbgctr;
                d2 += B3_SLOTS*B3_SLOTSIZE;
            }
        }
        edgecount /= B3_SLOTSIZE;
        dbgctr /= B3_SLOTSIZE;
        printf("Iteration %d: %d; max: %d, rdtsc: %ld\n", iter, edgecount, dbgctr, rdtsc1-rdtsc0);
    }
#endif
}

static void TrimRename3(void * const context, void * const dst, void * const src, const uint32_t tid, const uint32_t ramsize, const uint32_t iter)
{
    void * const ctx = __builtin_assume_aligned(context, 64);
    uint32_t * const pu32dst = __builtin_assume_aligned(dst, 64) + B_BUCKETS * 4 * tid;
    void * const pvdstbody = __builtin_assume_aligned(dst, 64) +  B_BUCKETS * THREADS * 4;
    void * const pvsrchead = __builtin_assume_aligned(src, 64);
    void * const pvsrcbody = pvsrchead + B_BUCKETS * THREADS * 4;
    uint16_t * const pu16deg = ctx + CK_PTS1 + PTS1_SIZE * tid;
    
    uint64_t *pu64readsrc;
    uint32_t *pu32src, *pu32readsrc;
    void *pvsrc, *pvsrc2, *pvlimit, *pvlimit2;
    uint64_t rdtsc0, rdtsc1;
    uint64_t q0;
    uint32_t srcbucket, srcbucketend, fromthread, edgecount, dbgctr=0;
    uint32_t d0, d1, d2, d3;
    int32_t newnodeid=tid, newnodeid_small;
    uint16_t w0;
    
    rdtsc0 = __rdtsc();
    d0 = B3_SLOTS * B3_SLOTSIZE * tid;
    for (d1=0; d1<B_BUCKETS; d1++) {
        pu32dst[d1] = d0;
        d0 += THREADS*B3_SLOTS*B3_SLOTSIZE;
    }
    srcbucket = tid * BPT;
    srcbucketend = srcbucket + BPT;
    pvsrc = pvsrcbody + srcbucket * (THREADS*B3_SLOTS*B3_SLOTSIZE);
    pu32src = pvsrchead;
    d3 = srcbucket << 15;
    d0 = (srcbucket!=0) ? (pu32src[(THREADS-1)*B_BUCKETS+srcbucket-1] + B3_SLOTSIZE*2) : 0;
    pvlimit = pvsrcbody + d0;
    pu32readsrc = (pvlimit > pvsrc) ? pvlimit : pvsrc;

    for (; srcbucket<srcbucketend; srcbucket++) {
        memset(pu16deg, 0, ramsize*64);
        newnodeid_small = 31;
        
        pu64readsrc = (uint64_t*) pu32readsrc;
        pvsrc2 = pvsrc;
        for (fromthread=0; fromthread<THREADS*B_BUCKETS; fromthread+=B_BUCKETS) {
            pvlimit = pvsrcbody + pu32src[fromthread+srcbucket];
            pvsrc += B3_SLOTS*B3_SLOTSIZE;  //pvsrc is now the beginning of the next bucket
            pvlimit2 = (pvsrc<pvlimit) ? pvsrc : pvlimit;
            while ((void*) pu32readsrc < pvlimit2) {
                pu16deg[*pu32readsrc & 0x7fff]++;
                pu32readsrc = ((void*)pu32readsrc) + B3_SLOTSIZE;
            }
            pvlimit += B3_SLOTSIZE*2;       //Two more spaces because 8-byte writes were used to write 5-byte slots
            pu32readsrc = (pvlimit > pvsrc) ? pvlimit : pvsrc;  //handle bucket overflow
        }
        
        for (fromthread=0; fromthread<THREADS*B_BUCKETS; fromthread+=B_BUCKETS) {
            pvlimit = pvsrcbody + pu32src[fromthread+srcbucket];
            pvsrc2 += B3_SLOTS*B3_SLOTSIZE;  //pvsrc2 is now the beginning of the next bucket
            pvlimit2 = (pvsrc2<pvlimit) ? pvsrc2 : pvlimit;
            while ((void*) pu64readsrc < pvlimit2) {
                q0 = *pu64readsrc;
                d0 = q0 & 0x7fff;
                w0 = pu16deg[d0];
                if (w0 > 1) {
                    newnodeid_small += (w0<32) ? 1 : 0;
                    w0 = (w0<32) ? (uint16_t) newnodeid_small : w0;
                    pu16deg[d0] = w0;
                    d1 = q0 >> 15;
                    d2 = d1 & B_BUCKETMASK;
                    d1 = (d1 >> 8) & 0x7fff;
                    q0 = ((uint32_t) w0 << LOGTHREADS) + newnodeid;
                    q0 = q0 << 15;
                    q0 |= d1;
                    *(uint64_t*) (pvdstbody + pu32dst[d2]) = q0;
                    pu32dst[d2] += B3_SLOTSIZE;
                }
                pu64readsrc = ((void*)pu64readsrc) + B3_SLOTSIZE;
            }
            pvlimit += B3_SLOTSIZE*2;       //Two more spaces because 8-byte writes were used to write 5-byte slots
            pu64readsrc = (pvlimit > pvsrc2) ? pvlimit : pvsrc2;  //handle bucket overflow
        }
        newnodeid = newnodeid + ((newnodeid_small - 31) << LOGTHREADS);
        d3 += (1 << 15);
    }
    *(int32_t*) (ctx + CK_THREADRESULT0 + tid*4) = newnodeid;
    pthread_barrier_wait(ctx+CK_BARRIER);
    rdtsc1 = __rdtsc();
#if PRINTSTAT
    if (tid==0) {
        d2 = 0;
        edgecount = 0;
        dbgctr = 0;
        for (d0=0; d0<B_BUCKETS; d0++) {
            for (d1=0; d1<THREADS; d1++) {
                d3 = d1 * B_BUCKETS + d0;
                edgecount += (pu32dst[d3]-d2);
                dbgctr = ((pu32dst[d3]-d2) > dbgctr) ? (pu32dst[d3]-d2) : dbgctr;
                d2 += B3_SLOTS*B3_SLOTSIZE;
            }
        }
        edgecount /= B3_SLOTSIZE;
        dbgctr /= B3_SLOTSIZE;
        printf("Iteration %d: %d; max: %d, rdtsc: %ld\n", iter, edgecount, dbgctr, rdtsc1-rdtsc0);
        d0 = 0;
        for (d1=0; d1<THREADS; d1++) {
            d2 = *(int32_t*) (ctx + CK_THREADRESULT0 + d1*4);
            d0 += d2 >> LOGTHREADS;
        }
        printf("nodecount=%d\n", d0);
    }
#endif
}

static void MakePairs(void * const context, void * const dst, void * const src, const uint32_t tid, const uint32_t ramsize)
{
    void * const ctx = __builtin_assume_aligned(context, 64);
    void * const pvdst = __builtin_assume_aligned(dst, 64) + tid * (PAIRSLIST_PT*12);
    void * const pvsrchead = __builtin_assume_aligned(src, 64);
    void * const pvsrcbody = pvsrchead + B_BUCKETS * THREADS * 4;
    uint32_t * const pu32lastseen = ctx + CK_PTS1 + PTS1_SIZE * tid;
    uint8_t * const pbitfield = ctx + CK_PAIRSBITFIELD;
    
    uint64_t *pu64readsrc;
    uint32_t *pu32src;
    uint32_t *pu32dst = pvdst;
    void *pvsrc, *pvlimit, *pvlimit2;
    uint64_t rdtsc0, rdtsc1;
    uint64_t q0;
    uint32_t srcbucket, srcbucketend, srcbucketshifted, fromthread, dbgctr=0;
    uint32_t d0, d1, d2, d3;
    
    rdtsc0 = __rdtsc();
    srcbucket = tid * BPT;
    srcbucketend = srcbucket + BPT;
    pvsrc = pvsrcbody + srcbucket * (THREADS*B3_SLOTS*B3_SLOTSIZE);
    pu32src = pvsrchead;
    srcbucketshifted = srcbucket << 15;
    d0 = (srcbucket!=0) ? (pu32src[(THREADS-1)*B_BUCKETS+srcbucket-1] + B3_SLOTSIZE*2) : 0;
    pvlimit = pvsrcbody + d0;
    pu64readsrc = (pvlimit > pvsrc) ? pvlimit : pvsrc;

    for (; srcbucket<srcbucketend; srcbucket++) {
        memset(pu32lastseen, 0, ramsize*128);
        
        for (fromthread=0; fromthread<THREADS*B_BUCKETS; fromthread+=B_BUCKETS) {
            pvlimit = pvsrcbody + pu32src[fromthread+srcbucket];
            pvsrc += B3_SLOTS*B3_SLOTSIZE;  //pvsrc is now the beginning of the next bucket
            pvlimit2 = (pvsrc<pvlimit) ? pvsrc : pvlimit;
            while ((void*) pu64readsrc < pvlimit2) {
                q0 = *pu64readsrc;
                d1 = q0 & 0x7fff;
                d2 = pu32lastseen[d1];
                d3 = (q0>>15) & 0x3ffff;
                pu32lastseen[d1] = d3;
                pu32dst[0] = d2;       //18 bits u
                pu32dst[1] = d3;       //18 bits u
                pu32dst[2] = d1 | srcbucketshifted;  //{v[7:0], v[22:8]}
                pu32dst += d2 ? 3 : 0;
                pu64readsrc = ((void*)pu64readsrc) + B3_SLOTSIZE;
            }
            pvlimit += B3_SLOTSIZE*2;       //Two more spaces because 8-byte writes were used to write 5-byte slots
            pu64readsrc = (pvlimit > pvsrc) ? pvlimit : pvsrc;  //handle bucket overflow
        }
        srcbucketshifted += 1 << 15;
    }
    d1 = ((void*) pu32dst - pvdst)/12;
    *(uint32_t*) (ctx + CK_THREADRESULT1 + tid*4) = d1;
    memset(pbitfield + tid*(32768/THREADS), 0, 32768/THREADS);
    pthread_barrier_wait(ctx+CK_BARRIER);

    rdtsc1 = __rdtsc();
#if PRINTSTAT
    if (tid==0) {
        pu32src = (uint32_t*) (ctx + CK_THREADRESULT1);
        d1 = *pu32src;
        pu32src++;
        for (fromthread=1; fromthread<THREADS; fromthread++) {
            d1 += *pu32src;
            pu32src++;
        }
        printf("MakePairs: pairs=%d, rdtsc: %ld\n", d1, rdtsc1-rdtsc0);
    }
#endif
}

static void MakePairsIndex(void * const context, uint32_t const tid)
{
    void * const ctx = __builtin_assume_aligned(context, 64);
    void * const pvbighead = ctx + CK_BIGP;
    void * const pvbigbody = ctx + CK_BIGP +  B_BUCKETS * THREADS * 4;
    uint32_t * const hashtab = ctx + CK_PTS1 + PTS1_SIZE * tid;
    uint32_t * const pu32pairsindex = ctx + CK_PAIRSINDEX + tid * PI_THREADSTRIDE;
    uint8_t * const pbitfield = ctx + CK_PAIRSBITFIELD;
    
    uint64_t rdtsc0, rdtsc1;
    uint32_t * pu32big = ctx + CK_BIGP + B_BUCKETS * 4 * tid;
    uint32_t * pu32src = ctx + CK_PAIRSLIST + tid * (PAIRSLIST_PT*12);
    uint32_t *pu32read, *pu32read2, *pu32src_end;
    void *pvlimit, *pvbig, *pvbig2;
    uint32_t d0, d1, d2, d3, d4, d5;
    uint32_t k2, k3, k4, k5, k6;
    uint32_t paircounter_shifted, bigbucket, bigbucketend, fromthread, pairsindexwrite, dbgctr=0;
    uint8_t b0;
    
    rdtsc0 = __rdtsc();
    d0 = BP_SLOTS * BP_SLOTSIZE * tid;
    for (d1=0; d1<B_BUCKETS; d1++) {
        pu32big[d1] = d0;
        d0 += THREADS*BP_SLOTS*BP_SLOTSIZE;
    }
    d0 = *(uint32_t*) (ctx + CK_THREADRESULT1 + tid*4);
    pu32src_end = (void*) pu32src + d0 * 12;
    paircounter_shifted = (tid * PAIRSLIST_PT) << 10;
    while (pu32src < pu32src_end) {
        d0 = pu32src[0];
        d1 = pu32src[1];
        pu32src += 3;
        if (d0 == d1) {
            d2 = paircounter_shifted >> 10;
            //pbitfield[d2/8] |= (1 << (d2%8));
            __atomic_or_fetch(&pbitfield[d2/8], (1 << (d2%8)), __ATOMIC_RELAXED);
            printf("2-cycle found\n");
            paircounter_shifted += (1 << 10);
            continue;
        }
        d2 = d0 & B_BUCKETMASK;
        d3 = d1 & B_BUCKETMASK;
        *(uint32_t*) (pvbigbody+pu32big[d2]) = (d0 >> B_LOG) | paircounter_shifted;
        pu32big[d2] += BP_SLOTSIZE;
        *(uint32_t*) (pvbigbody+pu32big[d3]) = (d1 >> B_LOG) | paircounter_shifted;
        pu32big[d3] += BP_SLOTSIZE;
        paircounter_shifted += (1 << 10);
    }
    pthread_barrier_wait(ctx+CK_BARRIER);
    
    k2 = pu32pairsindex[2] = 16; //first slot at offset 64 byte
    k3 = pu32pairsindex[3] = 16 + PI_SLOTS_2*2;
    k4 = pu32pairsindex[4] = 16 + PI_SLOTS_2*2 + PI_SLOTS_3*3;
    k5 = pu32pairsindex[5] = 16 + PI_SLOTS_2*2 + PI_SLOTS_3*3 + PI_SLOTS_4*4;
    k6 = pu32pairsindex[6] = 16 + PI_SLOTS_2*2 + PI_SLOTS_3*3 + PI_SLOTS_4*4 + PI_SLOTS_5*5;
    
    bigbucket = tid * BPT;
    bigbucketend = bigbucket + BPT;
    pvbig = pvbigbody + bigbucket * (THREADS*BP_SLOTS*BP_SLOTSIZE);
    pvbig2 = pvbig;
    pu32big = pvbighead;
    pu32read = pvbig;
    
    for (; bigbucket<bigbucketend; bigbucket++) {
        memset(hashtab, 0, 4096);
        
        pu32read2 = pu32read;
        for (fromthread=0; fromthread<THREADS*B_BUCKETS; fromthread+=B_BUCKETS) {
            pvlimit = pvbigbody + pu32big[fromthread+bigbucket];
            while ((void*) pu32read < pvlimit) {
                hashtab[*pu32read & 0x3ff] += (1<<24);
                pu32read = ((void*)pu32read) + BP_SLOTSIZE;
            }
            pvbig += BP_SLOTS*BP_SLOTSIZE;  //advance to the next bucket
            pvlimit += BP_SLOTSIZE;
            pu32read = (pvlimit > pvbig) ? pvlimit : pvbig;  //handle bucket overflow
        }
        
        for (fromthread=0; fromthread<THREADS*B_BUCKETS; fromthread+=B_BUCKETS) {
            pvlimit = pvbigbody + pu32big[fromthread+bigbucket];
            while ((void*) pu32read2 < pvlimit) {
                d0 = *pu32read2;
                d1 = d0 & 0x3ff;
                d2 = d0 >> 10;
                d3 = hashtab[d1];
                pu32read2 = ((void*)pu32read2) + BP_SLOTSIZE;
                b0 = d3 >> 24;
                if ((b0<2) | (b0>6) ) {
                    //pbitfield[d2/8] |= (1 << (d2%8));
                    __atomic_or_fetch(&pbitfield[d2/8], (1 << (d2%8)), __ATOMIC_RELAXED);
                    continue;
                }
                d4 = d3 & 0xffffff;
                d5 = pu32pairsindex[b0];
                pairsindexwrite = d4 ? d4 : d5;
                pu32pairsindex[b0] = d4 ? d5 : d5+b0;
                pu32pairsindex[pairsindexwrite] = d2;
                pairsindexwrite++;
                hashtab[d1] = (d3 & 0xff000000) | pairsindexwrite;
            } 
            pvbig2 += BP_SLOTS*BP_SLOTSIZE;  //advance to the next bucket
            pvlimit += BP_SLOTSIZE;
            pu32read2 = (pvlimit > pvbig2) ? pvlimit : pvbig2;  //handle bucket overflow
        }
    }
    
    pthread_barrier_wait(ctx+CK_BARRIER);
    rdtsc1 = __rdtsc();

#if PRINTSTAT
    if (tid==0) {
        printf("MakePairsIndex, rdtsc: %ld\n", rdtsc1-rdtsc0);
    }
#endif
}

static void TrimPairsST(void * const context)
{
    void * const ctx = __builtin_assume_aligned(context, 64);
    uint8_t * const pbitfield = ctx + CK_PAIRSBITFIELD;
    uint32_t* pu32pairsindex[THREADS];
    uint32_t* pu32end[THREADS];
    uint64_t *pu64read, *pu64end, *pu64write;
    uint32_t *pu32read, *pu32write, *pu32write2;
    uint64_t rdtsc0, rdtsc1;
    uint32_t d0, d1, d2, d3, d4, innerloop, outerloop, fromthread, moreiter;
    
    uint32_t const k2 = 16; //first slot at offset 64 byte
    uint32_t const k3 = 16 + PI_SLOTS_2*2;
    uint32_t const k4 = 16 + PI_SLOTS_2*2 + PI_SLOTS_3*3;
    uint32_t const k5 = 16 + PI_SLOTS_2*2 + PI_SLOTS_3*3 + PI_SLOTS_4*4;
    uint32_t const k6 = 16 + PI_SLOTS_2*2 + PI_SLOTS_3*3 + PI_SLOTS_4*4 + PI_SLOTS_5*5;
    
    for (fromthread=0; fromthread<THREADS; fromthread++) {
        pu32pairsindex[fromthread] = ctx + CK_PAIRSINDEX + fromthread * PI_THREADSTRIDE;
    }
#if PRINTSTAT
    d0 = (pu32pairsindex[0][2]-k2);
    d1 = (pu32pairsindex[0][3]-k3);
    d2 = (pu32pairsindex[0][4]-k4);
    d3 = (pu32pairsindex[0][5]-k5);
    d4 = (pu32pairsindex[0][6]-k6);
    for (fromthread=1; fromthread<THREADS; fromthread++) {
        d0 += (pu32pairsindex[fromthread][2]-k2);
        d1 += (pu32pairsindex[fromthread][3]-k3);
        d2 += (pu32pairsindex[fromthread][4]-k4);
        d3 += (pu32pairsindex[fromthread][5]-k5);
        d4 += (pu32pairsindex[fromthread][6]-k6);
    }
    printf("TrimPairs Start: %d %d %d %d %d\n", d0/2, d1/3, d2/4, d3/5, d4/6);
#endif
    rdtsc0 = __rdtsc();
    
    for (outerloop=0; outerloop<24; outerloop++) {
        for (innerloop=0; innerloop<20; innerloop++) {
            moreiter = 0;
            for (fromthread=0; fromthread<THREADS; fromthread++) {
                pu64write = pu64read = (uint64_t*) &pu32pairsindex[fromthread][k2];
                pu64end = (uint64_t*) &pu32pairsindex[fromthread][pu32pairsindex[fromthread][2]];
                while (pu64read < pu64end) {
                    pu32read = (uint32_t*) pu64read;
                    *pu64write = *pu64read;
                    d0 = pu32read[0];
                    d1 = pu32read[1];
                    d2 = pbitfield[d0/8] & (1 << (d0%8));
                    d3 = pbitfield[d1/8] & (1 << (d1%8));
                    d4 = (d2 | d3) ? 1 : 0;
                    pbitfield[d0/8] |= (d4 << (d0%8));
                    pbitfield[d1/8] |= (d4 << (d1%8));
                    pu64write += d4 ? 0 : 1;
                    pu64read++;
                }
                moreiter |= (pu64end != pu64write) ? 1 : 0;
                pu32pairsindex[fromthread][2] = ((uint32_t*) pu64write - pu32pairsindex[fromthread]);
            }
            if (moreiter == 0)
                break;
        }

        if (innerloop==0)
            break;
        
        for (fromthread=0; fromthread<THREADS; fromthread++) {
            pu32write = pu32read = &pu32pairsindex[fromthread][k6];
            pu32write2 = &pu32pairsindex[fromthread][pu32pairsindex[fromthread][5]];
            pu32end[fromthread] = &pu32pairsindex[fromthread][pu32pairsindex[fromthread][6]];
            while (pu32read < pu32end[fromthread]) {
                if (pbitfield[pu32read[0]/8] & (1 << (pu32read[0]%8))) {
                    pu32write2[0] = pu32read[1];
                    pu32write2[1] = pu32read[2];
                    pu32write2[2] = pu32read[3];
                    pu32write2[3] = pu32read[4];
                    pu32write2[4] = pu32read[5];
                    pu32write2 += 5;
                }
                else if (pbitfield[pu32read[1]/8] & (1 << (pu32read[1]%8))) {
                    pu32write2[0] = pu32read[0];
                    pu32write2[1] = pu32read[2];
                    pu32write2[2] = pu32read[3];
                    pu32write2[3] = pu32read[4];
                    pu32write2[4] = pu32read[5];
                    pu32write2 += 5;
                }
                else if (pbitfield[pu32read[2]/8] & (1 << (pu32read[2]%8))) {
                    pu32write2[0] = pu32read[0];
                    pu32write2[1] = pu32read[1];
                    pu32write2[2] = pu32read[3];
                    pu32write2[3] = pu32read[4];
                    pu32write2[4] = pu32read[5];
                    pu32write2 += 5;
                }
                else if (pbitfield[pu32read[3]/8] & (1 << (pu32read[3]%8))) {
                    pu32write2[0] = pu32read[0];
                    pu32write2[1] = pu32read[1];
                    pu32write2[2] = pu32read[2];
                    pu32write2[3] = pu32read[4];
                    pu32write2[4] = pu32read[5];
                    pu32write2 += 5;
                }
                else if (pbitfield[pu32read[4]/8] & (1 << (pu32read[4]%8))) {
                    pu32write2[0] = pu32read[0];
                    pu32write2[1] = pu32read[1];
                    pu32write2[2] = pu32read[2];
                    pu32write2[3] = pu32read[3];
                    pu32write2[4] = pu32read[5];
                    pu32write2 += 5;
                }
                else if (pbitfield[pu32read[5]/8] & (1 << (pu32read[5]%8))) {
                    pu32write2[0] = pu32read[0];
                    pu32write2[1] = pu32read[1];
                    pu32write2[2] = pu32read[2];
                    pu32write2[3] = pu32read[3];
                    pu32write2[4] = pu32read[4];
                    pu32write2 += 5;
                }
                else {
                    pu32write[0] = pu32read[0];
                    pu32write[1] = pu32read[1];
                    pu32write[2] = pu32read[2];
                    pu32write[3] = pu32read[3];
                    pu32write[4] = pu32read[4];
                    pu32write[5] = pu32read[5];
                    pu32write += 6;
                }
                pu32read += 6;
            }
            pu32pairsindex[fromthread][6] = pu32write - pu32pairsindex[fromthread];
            pu32end[fromthread] = pu32write2;
        }
        
        for (fromthread=0; fromthread<THREADS; fromthread++) {
            pu32write = pu32read = &pu32pairsindex[fromthread][k5];
            pu32write2 = &pu32pairsindex[fromthread][pu32pairsindex[fromthread][4]];
            while (pu32read < pu32end[fromthread]) {
                if (pbitfield[pu32read[0]/8] & (1 << (pu32read[0]%8))) {
                    pu32write2[0] = pu32read[1];
                    pu32write2[1] = pu32read[2];
                    pu32write2[2] = pu32read[3];
                    pu32write2[3] = pu32read[4];
                    pu32write2 += 4;
                }
                else if (pbitfield[pu32read[1]/8] & (1 << (pu32read[1]%8))) {
                    pu32write2[0] = pu32read[0];
                    pu32write2[1] = pu32read[2];
                    pu32write2[2] = pu32read[3];
                    pu32write2[3] = pu32read[4];
                    pu32write2 += 4;
                }
                else if (pbitfield[pu32read[2]/8] & (1 << (pu32read[2]%8))) {
                    pu32write2[0] = pu32read[0];
                    pu32write2[1] = pu32read[1];
                    pu32write2[2] = pu32read[3];
                    pu32write2[3] = pu32read[4];
                    pu32write2 += 4;
                }
                else if (pbitfield[pu32read[3]/8] & (1 << (pu32read[3]%8))) {
                    pu32write2[0] = pu32read[0];
                    pu32write2[1] = pu32read[1];
                    pu32write2[2] = pu32read[2];
                    pu32write2[3] = pu32read[4];
                    pu32write2 += 4;
                }
                else if (pbitfield[pu32read[4]/8] & (1 << (pu32read[4]%8))) {
                    pu32write2[0] = pu32read[0];
                    pu32write2[1] = pu32read[1];
                    pu32write2[2] = pu32read[2];
                    pu32write2[3] = pu32read[3];
                    pu32write2 += 4;
                }
                else {
                    pu32write[0] = pu32read[0];
                    pu32write[1] = pu32read[1];
                    pu32write[2] = pu32read[2];
                    pu32write[3] = pu32read[3];
                    pu32write[4] = pu32read[4];
                    pu32write += 5;
                }
                pu32read += 5;
            }
            pu32pairsindex[fromthread][5] = pu32write - pu32pairsindex[fromthread];
            pu32end[fromthread] = pu32write2;
        }
        
        for (fromthread=0; fromthread<THREADS; fromthread++) {
            pu32write = pu32read = &pu32pairsindex[fromthread][k4];
            pu32write2 = &pu32pairsindex[fromthread][pu32pairsindex[fromthread][3]];
            while (pu32read < pu32end[fromthread]) {
                if (pbitfield[pu32read[0]/8] & (1 << (pu32read[0]%8))) {
                    pu32write2[0] = pu32read[1];
                    pu32write2[1] = pu32read[2];
                    pu32write2[2] = pu32read[3];
                    pu32write2 += 3;
                }
                else if (pbitfield[pu32read[1]/8] & (1 << (pu32read[1]%8))) {
                    pu32write2[0] = pu32read[0];
                    pu32write2[1] = pu32read[2];
                    pu32write2[2] = pu32read[3];
                    pu32write2 += 3;
                }
                else if (pbitfield[pu32read[2]/8] & (1 << (pu32read[2]%8))) {
                    pu32write2[0] = pu32read[0];
                    pu32write2[1] = pu32read[1];
                    pu32write2[2] = pu32read[3];
                    pu32write2 += 3;
                }
                else if (pbitfield[pu32read[3]/8] & (1 << (pu32read[3]%8))) {
                    pu32write2[0] = pu32read[0];
                    pu32write2[1] = pu32read[1];
                    pu32write2[2] = pu32read[2];
                    pu32write2 += 3;
                }
                else {
                    pu32write[0] = pu32read[0];
                    pu32write[1] = pu32read[1];
                    pu32write[2] = pu32read[2];
                    pu32write[3] = pu32read[3];
                    pu32write += 4;
                }
                pu32read += 4;
            }
            pu32pairsindex[fromthread][4] = pu32write - pu32pairsindex[fromthread];
            pu32end[fromthread] = pu32write2;
        }
        
        for (fromthread=0; fromthread<THREADS; fromthread++) {
            pu32write = pu32read = &pu32pairsindex[fromthread][k3];
            pu32write2 = &pu32pairsindex[fromthread][pu32pairsindex[fromthread][2]];
            while (pu32read < pu32end[fromthread]) {
                d0 = pu32read[0];
                d1 = pu32read[1];
                d2 = pu32read[2];
                if (pbitfield[d0/8] & (1 << (d0%8))) {
                    pu32write2[0] = d1;
                    pu32write2[1] = d2;
                    pu32write2 += 2;
                }
                else if (pbitfield[d1/8] & (1 << (d1%8))) {
                    pu32write2[0] = d0;
                    pu32write2[1] = d2;
                    pu32write2 += 2;
                }
                else if (pbitfield[d2/8] & (1 << (d2%8))) {
                    pu32write2[0] = d0;
                    pu32write2[1] = d1;
                    pu32write2 += 2;
                }
                else {
                    pu32write[0] = d0;
                    pu32write[1] = d1;
                    pu32write[2] = d2;
                    pu32write += 3;
                }
                pu32read += 3;
            }
            pu32pairsindex[fromthread][3] = pu32write - pu32pairsindex[fromthread];
            pu32pairsindex[fromthread][2] = pu32write2 - pu32pairsindex[fromthread];
        }
    }
    
    // Consolidate all to thread 0
    for (fromthread=1; fromthread<THREADS; fromthread++) {
        d0 = pu32pairsindex[0][2];
        d1 = k2;
        d2 = pu32pairsindex[fromthread][2];
        while (d1<d2)
            pu32pairsindex[0][d0++] = pu32pairsindex[fromthread][d1++];
        pu32pairsindex[0][2] = d0;
        
        d0 = pu32pairsindex[0][3];
        d1 = k3;
        d2 = pu32pairsindex[fromthread][3];
        while (d1<d2)
            pu32pairsindex[0][d0++] = pu32pairsindex[fromthread][d1++];
        pu32pairsindex[0][3] = d0;
        
        d0 = pu32pairsindex[0][4];
        d1 = k4;
        d2 = pu32pairsindex[fromthread][4];
        while (d1<d2)
            pu32pairsindex[0][d0++] = pu32pairsindex[fromthread][d1++];
        pu32pairsindex[0][4] = d0;
        //TODO: deg 5+
    }

    rdtsc1 = __rdtsc();
    
#if PRINTSTAT
    d0 = (pu32pairsindex[0][2]-k2);
    d1 = (pu32pairsindex[0][3]-k3);
    d2 = (pu32pairsindex[0][4]-k4);
    d3 = (pu32pairsindex[0][5]-k5);
    d4 = (pu32pairsindex[0][6]-k6);
    for (fromthread=1; fromthread<THREADS; fromthread++) {
        //TODO: remove when deg 5+ added
        d3 += (pu32pairsindex[fromthread][5]-k5);
        d4 += (pu32pairsindex[fromthread][6]-k6);
    }
    printf("TrimPairs End: %d %d %d %d %d\n", d0/2, d1/3, d2/4, d3/5, d4/6);
    printf("TrimPairs, %d loops, rdtsc: %ld\n", outerloop, rdtsc1-rdtsc0);
#endif
}

static int compare_uint32(const void* p1, const void* p2)
{
    uint32_t a1 = *(uint32_t*) p1;
    uint32_t a2 = *(uint32_t*) p2;
    return (a1<a2) ? -1 : 1;
}

static void RecoverSolution(void * const context, uint32_t * listv, uint32_t halfcyclelen)
{
    __m256i in0,in1,in2,in3,in1a;
    __m256i v0,v1,v2,v3,t0, nonce;
    
    void * const ctx = __builtin_assume_aligned(context, 64);
    uint32_t * const pu32renamelist = ctx + CK_RENAMELIST;
    uint32_t * const pu32edgesbegin = ctx + CK_EDGE1;
    uint32_t * pu32edgei[THREADS];
    uint32_t * pu32edges[THREADS];
    void *pvedges, *pvend;
    uint32_t *solbuf;
    
    uint32_t nonceubuf[3][32];
    uint64_t q0;
    uint32_t nonceuctr, prevnonceuctr, savednonceuctr;
    uint32_t fromthread, cyclepos, flatbucket, sipvnode, odd;
    uint32_t d0, d1, d2, d3, d4, d5;
    
    q0 = *(uint64_t*) (ctx+CK_SIPSTATE);
    in0 = _mm256_set_epi64x(q0, q0, q0, q0);
    q0 = *(uint64_t*) (ctx+CK_SIPSTATE+8);
    in1 = _mm256_set_epi64x(q0, q0, q0, q0);
    q0 = *(uint64_t*) (ctx+CK_SIPSTATE+16);
    in2 = _mm256_set_epi64x(q0, q0, q0, q0);
    q0 = *(uint64_t*) (ctx+CK_SIPSTATE+24);
    in3 = _mm256_set_epi64x(q0, q0, q0, q0);
    q0 = *(uint64_t*) (ctx+CK_SIPSTATE+32);
    in1a = _mm256_set_epi64x(q0, q0, q0, q0);
    solbuf = *(void **) (ctx + CK_SOLPTR);
    
    for (fromthread=0; fromthread<THREADS; fromthread++) {
        pu32edgei[fromthread] = ctx + CK_EDGE1 + (THREADPLAN0 * THREADS * B0_SLOTS * B0_SLOTSIZE) * fromthread;
        pu32edges[fromthread] = pu32edgei[fromthread] + (BPT * S_BUCKETS);
    }
    
    for (cyclepos=0; cyclepos<halfcyclelen; cyclepos++) {
        odd = cyclepos & 1;
        d0 = listv[cyclepos];   //{v[7:0], v[22:8]} caused by MakePairs()
        d0 = ((d0 << 8) & 0x7fff00) | ((d0 >> 15) & 0xff);
        d1 = ((d0&(THREADS-1))<<(23-LOGTHREADS)) | (d0>>LOGTHREADS);    //Rename List use tid at msb, the renamed nodes have tid at lsb
        d2 = pu32renamelist[d1];
        d4 = ((d2 >> 19) & 0xff);
        fromthread = d4 / BPT;
        d5 = d4 % BPT;
        d4 = (d5 < THREADPLAN0) ? (fromthread*THREADPLAN0+d5) : (THREADS*THREADPLAN0+fromthread*THREADPLAN1+d5-THREADPLAN0);
        sipvnode = ((d2 << 13) & 0x07ffe000) | ((d2 >> 6) & 0x00001f00) | d4;
        flatbucket = ((sipvnode >> 8) & 0xff) | (d5 << 8);
//printf("%08x %08x %08x %08x %08x\n", listv[cyclepos], d0, d1, d2, sipvnode);
        
        d0 = (flatbucket!=0) ? (pu32edgei[fromthread][flatbucket-1]) : (pu32edges[fromthread] - pu32edgesbegin);
        d1 = pu32edgei[fromthread][flatbucket];
        pvedges = &pu32edgesbegin[d0 & -4];
        pvend = &pu32edgesbegin[d1];
        
        nonceuctr = 0;
        do {
            nonce = _mm256_cvtepu32_epi64(*(__m128i*) pvedges) & yconstnoncemaske;
            nonce |= yconst1;
            SIPHASH;
            v0 &= yconstnodemask;
            if (_mm256_extract_epi32(v0, 0) == sipvnode) {
                nonceubuf[odd][nonceuctr] = _mm256_extract_epi32(nonce, 0);
                nonceuctr += 2;
                if (nonceuctr >= 32)
                    break;
            }
            if (_mm256_extract_epi32(v0, 2) == sipvnode) {
                nonceubuf[odd][nonceuctr] = _mm256_extract_epi32(nonce, 2);
                nonceuctr += 2;
                if (nonceuctr >= 32)
                    break;
            }
            if (_mm256_extract_epi32(v0, 4) == sipvnode) {
                nonceubuf[odd][nonceuctr] = _mm256_extract_epi32(nonce, 4);
                nonceuctr += 2;
                if (nonceuctr >= 32)
                    break;
            }
            if (_mm256_extract_epi32(v0, 6) == sipvnode) {
                nonceubuf[odd][nonceuctr] = _mm256_extract_epi32(nonce, 6);
                nonceuctr += 2;
                if (nonceuctr >= 32)
                    break;
            }
            pvedges += 16;
        } while (pvedges < pvend);
        if (nonceuctr < 4)
            break;
        
        d0 = 0;
        do {
            nonce = *(__m256i*) &nonceubuf[odd][d0];
            nonce &= yconstnoncemaske;
            SIPHASH;
            v0 = v0 & yconstnodemask;
            nonceubuf[odd][d0+1] = _mm256_extract_epi32(v0, 0);
            nonceubuf[odd][d0+3] = _mm256_extract_epi32(v0, 2);
            nonceubuf[odd][d0+5] = _mm256_extract_epi32(v0, 4);
            nonceubuf[odd][d0+7] = _mm256_extract_epi32(v0, 6);
            d0 += 8;
        } while (d0 < nonceuctr);
        
        if (cyclepos != 0) {
            for (d0=1; d0<prevnonceuctr; d0+=2) {
                for (d1=1; d1<nonceuctr; d1+=2) {
                    if (nonceubuf[odd ^ 1][d0] == nonceubuf[odd][d1]) {
                        solbuf[cyclepos*2] = nonceubuf[odd ^ 1][d0-1] >> 1;
                        solbuf[cyclepos*2+1] = nonceubuf[odd][d1-1] >> 1;
                        break;
                    }
                }
                if (d1<nonceuctr)
                    break;
            }
            if (d0>=prevnonceuctr)
                break;  //failed to link
            prevnonceuctr = nonceuctr;
        }
        else {
            for (d0=0; d0<nonceuctr; d0++) {
                nonceubuf[2][d0] = nonceubuf[0][d0];    //For cyclepos==0, save a copy
            }
            prevnonceuctr = nonceuctr;
            savednonceuctr = nonceuctr;
        }
    }
    
    if (cyclepos != halfcyclelen) {
        printf("Failed to recover solution\n");
        return;
    }
    for (d0=1; d0<prevnonceuctr; d0+=2) {
        for (d1=1; d1<savednonceuctr; d1+=2) {
            if (nonceubuf[(halfcyclelen&1)^1][d0] == nonceubuf[2][d1]) {
                solbuf[0] = nonceubuf[(halfcyclelen&1)^1][d0-1] >> 1;
                solbuf[1] = nonceubuf[2][d1-1] >> 1;
                break;
            }
        }
        if (d1<savednonceuctr)
            break;
    }
    if (d0>=prevnonceuctr) {
        printf("Failed to recover solution 2\n");
        return;
    }
    qsort(solbuf, halfcyclelen*2, 4, compare_uint32);
    *(void **) (ctx + CK_SOLPTR) = solbuf + halfcyclelen*2;
}

static void FindCycle(void * const context)
{
    void * const ctx = __builtin_assume_aligned(context, 64);
    uint32_t * const pu32pairsindex = ctx + CK_PAIRSINDEX;
    uint32_t * const pu32pairslist = ctx + CK_PAIRSLIST;
    uint32_t * const pu32cyclebuf = ctx + CK_CYCLEBUF;
    uint32_t * const pu32junctions = ctx + CK_JUNCTIONS;
    uint64_t rdtsc0, rdtsc1;
    uint32_t d0, d1, d2, d3, d4, d5, d6;
    uint32_t jcount, jcount4;
    uint32_t pidx, pidx_end, cyclectr, prevv, curv, startedge, cyclebufpos, pathlenctr, dbgctr=0;
    uint32_t jnode, jroot, jbegin, jend, jentry, jexit, jmeet;
    
    uint32_t const k2 = 16; //first slot at offset 64 byte
    uint32_t const k3 = 16 + PI_SLOTS_2*2;
    uint32_t const k4 = 16 + PI_SLOTS_2*2 + PI_SLOTS_3*3;
    uint32_t const k5 = 16 + PI_SLOTS_2*2 + PI_SLOTS_3*3 + PI_SLOTS_4*4;
    uint32_t const k6 = 16 + PI_SLOTS_2*2 + PI_SLOTS_3*3 + PI_SLOTS_4*4 + PI_SLOTS_5*5;
    
    rdtsc0 = __rdtsc();
    d2 = 8;
    pidx_end = pu32pairsindex[4];
    for (pidx = k4; pidx < pidx_end; pidx++) {
        d0 =  pu32pairsindex[pidx] * 3;
        if (pu32pairslist[d0]) {
            pu32pairslist[d0+1] = d2 + 0x100000;
            pu32pairslist[d0] = 0;
            pu32junctions[d2+3] = d0;
        }
        else {
            pu32pairslist[d0] = d2 + 0x100000;
            pu32junctions[d2+3] = d0+1;
        }
        pu32junctions[d2] = 0;
        pu32junctions[d2+1] = pu32pairslist[d0+2];
        d2 += ((d2&0x1f)==20) ? (32-12) : 4;    // (d2&0x1f) cycles through {8, 12, 16, 20} 
    }
    if ((d2 & 0x1f) != 8) {
        printf("Bug FindCycle 4\n");
        return;
    }
    jcount4 = d2 & -32;
    for (d0=0; d0<jcount4; d0+=32) {
        pu32junctions[d0] = d0 + 8;
        pu32junctions[d0+1] = d0 + 24;
        pu32junctions[d0+2] = 0;
        pu32junctions[d0+3] = 0;
        pu32junctions[d0+4] = 0;
    }
    
    pidx_end = pu32pairsindex[3];
    for (pidx = k3; pidx < pidx_end; pidx++) {
        d0 =  pu32pairsindex[pidx] * 3;
        if (pu32pairslist[d0]) {
            pu32pairslist[d0+1] = d2 + 0x100000;
            pu32pairslist[d0] = 0;
            pu32junctions[d2+3] = d0;
        }
        else {
            pu32pairslist[d0] = d2 + 0x100000;
            pu32junctions[d2+3] = d0+1;
        }
        pu32junctions[d2] = 0;
        pu32junctions[d2+1] = pu32pairslist[d0+2];
        d2 += ((d2&0x1f)==16) ? (32-8) : 4;    // (d2&0x1f) cycles through {8, 12, 16} 
    }
    if ((d2 & 0x1f) != 8) {
        printf("Bug FindCycle 3\n");
        return;
    }
    jcount = d2 & -32;
    for (d0=jcount4; d0<jcount; d0+=32) {
        pu32junctions[d0] = d0 + 8;
        pu32junctions[d0+1] = d0 + 20;
        pu32junctions[d0+2] = 0;
        pu32junctions[d0+3] = 0;
        pu32junctions[d0+4] = 0;
    }
    
    pidx_end = pu32pairsindex[2];
    for (pidx = k2; pidx < pidx_end; pidx++) {
        d0 =  pu32pairsindex[pidx] * 3;
        pu32pairslist[d0+1] = pu32pairslist[d0] ? pidx : pu32pairslist[d0+1];
        pu32pairslist[d0] = pu32pairslist[d0] ? 0 : pidx;
    }
    
    for (d0=0; d0<jcount; d0+=32) {
        d1 = pu32junctions[d0+1];
        for (d2=pu32junctions[d0]; d2<d1; d2+=4) {
            if (pu32junctions[d2]==0) {
                prevv = pu32junctions[d2+1];
                d3 = pu32junctions[d2+3];
                d4 = pu32pairslist[d3];
                pathlenctr = 1;
                while ((d4 >= k2) && (d4 < k3)) {
                    d5 = d4 ^ 1;
                    d6 = pu32pairsindex[d5] & 0x7fffffff;
                    if (pu32pairsindex[d4 & ~1] & 0x80000000)
                        printf("Unexpected visited\n");
                    pu32pairsindex[d4 & ~1] |= 0x80000000;
                    d6 *= 3;
                    d4 = pu32pairslist[d6+1];
                    d4 = (d4 == d5) ? pu32pairslist[d6] : d4;
                    curv = pu32pairslist[d6+2];
                    pathlenctr += (prevv != curv) ? 1 : 0;
                    prevv = curv;
                }
                if ((d4 >= 0x100000) && (d4 < jcount+0x100000)) {
                    pu32junctions[d2] = pathlenctr;
                    pu32junctions[d2+2] = d4 - 0x100000;
                    pu32junctions[d4-0x100000] = pathlenctr;
                    pu32junctions[d4-0x100000+2] = d2;
                }
                else if (d4 == 0) {
                    printf("Junction links dead end\n");
                }
                else {
                    printf("Bug in junction links\n");
                }
            }
        }
    }
#if 0
    for (d0=0; d0<jcount; d0+=32) {
        d1 = pu32junctions[d0];
        d2 = pu32junctions[d0+1];
        printf("node: %04x %04x %04x %04x\n", d1, d2, pu32junctions[d0+2], pu32junctions[d0+3]);
        for (d3=d1; d3<d2; d3+=4) {
            printf("%4d %08x %08x %08x\n", pu32junctions[d3], pu32junctions[d3+1], pu32junctions[d3+2], pu32junctions[d3+3]);
        }
    }
#endif
    
    for (jroot=0; jroot<jcount; jroot+=32) {
        if (pu32junctions[jroot+4])
            continue;
        jnode = jroot;
        jbegin = pu32junctions[jnode];
        jentry = pu32junctions[jnode+1] - 4;
        jentry = (jbegin > jentry) ? jbegin : jentry;
        pu32junctions[jroot+2] = jentry;
        pu32junctions[jroot+3] = jentry;
        do {
            jbegin = pu32junctions[jnode];
            jend = pu32junctions[jnode+1];
            jentry = pu32junctions[jnode+2];
            jexit = pu32junctions[jnode+3];
            do {
                jexit += 4;
                jexit = (jexit >= jend) ? jbegin : jexit;
                if (jexit == jentry)
                    break;
            } while (pu32junctions[jexit]==0);

            if (jexit != jentry) {
                pu32junctions[jnode+3] = jexit;    //exiting current node
                d0 = pu32junctions[jexit+2];
                d1 = d0 & -32;
                d2 = pu32junctions[d1+3];
                if (d2) {    //if visited
                    if (d2 < d0) {
                        if (!(pu32junctions[d1+4])) {  //a cycle is found
                            pathlenctr = 0;
                            do {
                                pathlenctr += pu32junctions[d2];
                                pathlenctr -= (pu32junctions[d2+1] == pu32junctions[d0+1]) ? 1 : 0;
                                d3 = pu32junctions[d2+2] & -32;
                                d0 = pu32junctions[d3+2];
                                d2 = pu32junctions[d3+3];
                            } while (d3 != d1);
                            printf("%4d-cycle found\n", pathlenctr*2);
                            if (pathlenctr == HALFCYCLELENGTH) {
                                d2 = pu32junctions[d1+3];
                                prevv = pu32junctions[d2+1];
                                pu32cyclebuf[0] = prevv;
                                pathlenctr = 1;
                                do {
                                    d3 = pu32junctions[d2+3];
                                    d4 = pu32pairslist[d3];
                                    while ((d4 >= k2) && (d4 < k3)) {
                                        d5 = d4 ^ 1;
                                        d6 = pu32pairsindex[d5] & 0x7fffffff;
                                        d6 *= 3;
                                        d4 = pu32pairslist[d6+1];
                                        d4 = (d4 == d5) ? pu32pairslist[d6] : d4;
                                        curv = pu32pairslist[d6+2];
                                        pu32cyclebuf[pathlenctr] = curv;
                                        pathlenctr += (prevv != curv) ? 1 : 0;
                                        prevv = curv;
                                    }
                                    if ((d4 >= 0x100000) && (d4 < jcount+0x100000)) {
                                        d3 = (d4 - 0x100000) & -32;
                                        d2 = pu32junctions[d3+3];
                                        curv = pu32junctions[d2+1];
                                        pu32cyclebuf[pathlenctr] = curv;
                                        pathlenctr += (prevv != curv) ? 1 : 0;
                                        prevv = curv;
                                        if (pathlenctr > HALFCYCLELENGTH)
                                            break;
                                    }
                                    else {
                                        printf("Bug in FindCycle junction link rerun\n");
                                        break;
                                    }
                                } while (1);
                                RecoverSolution(ctx, pu32cyclebuf+1, HALFCYCLELENGTH);
                            }
                        }
                    }
                }
                else {
                    //Enter new node
                    jnode = d1;
                    pu32junctions[jnode+2] = d0;
                    pu32junctions[jnode+3] = d0;
                }
            }
            else {
                //Exit current node
                pu32junctions[jnode+2] = 0;
                pu32junctions[jnode+3] = 0;
                pu32junctions[jnode+4] = 1;     //Mark current node with "exited" status
                if (jnode == jroot)
                    break;  //cannot go back from root, we are done
                jnode = pu32junctions[jentry+2] & -32;
            }
        } while (1);
    }
#if 0
    for (d0=0; d0<jcount; d0+=32) {
        d1 = pu32junctions[d0];
        d2 = pu32junctions[d0+1];
        printf("node: %04x %04x %04x %04x %04x\n", d1, d2, pu32junctions[d0+2], pu32junctions[d0+3], pu32junctions[d0+4]);
        for (d3=d1; d3<d2; d3+=4) {
            printf("%4d %08x %08x %08x\n", pu32junctions[d3], pu32junctions[d3+1], pu32junctions[d3+2], pu32junctions[d3+3]);
        }
    }
#endif
#if 1
    printf("End of junction processing\n");
#endif
    
    pidx_end = pu32pairsindex[2];
    for (pidx = k2; pidx < pidx_end; pidx+=2) {
        d0 = pu32pairsindex[pidx];
        if (d0 & 0x80000000)
            continue;
        pu32pairsindex[pidx] |= 0x80000000;
        cyclectr = 0;
        prevv = -1;
        startedge = d0;
        d2 = pidx;
        do {
            d0 *= 3;
            d1 = pu32pairslist[d0+1];
            d1 = (d1 == d2) ? pu32pairslist[d0] : d1;
            curv = pu32pairslist[d0+2];
            cyclectr += (prevv != curv) ? 1 : 0;
            prevv = curv;
            cyclebufpos = (cyclectr<=HALFCYCLELENGTH) ? cyclectr : 0;
            pu32cyclebuf[cyclebufpos] = prevv;
            if (d1 == 0) {
                printf("dead end %08x\n", d0);
                goto _NoCycle;
            }
            if (d1 >= k3) {
                printf("degree>2\n");
                goto _NoCycle;
            }
            d2 = d1^1;
            d0 = pu32pairsindex[d2] & 0x7fffffff;
            d3 = pu32pairsindex[d1 & ~1];
            pu32pairsindex[d1 & ~1] |= 0x80000000;
        } while ((d3 & 0x80000000) == 0);
        if (d0 == startedge) {
            cyclectr -= (prevv == pu32pairslist[d0*3+2]) ? 1 : 0;
            printf("%4d-cycle found\n", cyclectr*2);
            if (cyclectr == HALFCYCLELENGTH) {
                RecoverSolution(ctx, pu32cyclebuf+1, HALFCYCLELENGTH);
            }
        }
        else {
            printf("FindCycle unexpected degree-2 traversal\n");
        }
        _NoCycle:
        ;
    }
    rdtsc1 = __rdtsc();
#if PRINTSTAT
    printf("FindCycle, rdtsc: %ld\n", rdtsc1-rdtsc0);
#endif
}

void* worker(void *tctx)
{
    __m256i in0,in1,in2,in3,in1a;
    __m256i v0,v1,v2,v3,t0;
    __m256i vx, nonce, slotnonce, vpayload;
    __m128i x0, x1;
    void * const ctx = ((tctx_t*) tctx)->ctx;
    uint32_t const tid = ((tctx_t*) tctx)->tid;
    uint32_t * const pu32edgesbegin = ctx + CK_EDGE1;
    uint32_t * const pu32abort = ctx + CK_ABORT;
    
    uint64_t *pu64readbig, *pu64readsmall;
    uint32_t *pu32big, *pu32small, *pu32readsmall, *pu32edgei, *pu32edges, *pu32dst;
    uint16_t *pu16rtemp;
    uint8_t *pu8deg, *pu8deg2;
    void *pvbig0, *pvbig, *pvlimit, *pvlimit2, *pvsmall0, *pvsmall, *pvedges_end, *pvdst0;
    uint64_t rdtsc0, rdtsc1, rdtsc2;
    uint64_t q0, q1, q2, q3, q4;
    uint32_t edgecount, fromthread;
    uint32_t d0, d1, d2, d3, dbgctr;
    uint32_t bigbucket, bigbucketend, smallbucket, iter, ramsize0, ramsize1;

    q0 = *(uint64_t*) (ctx+CK_SIPSTATE);
    in0 = _mm256_set_epi64x(q0, q0, q0, q0);
    q0 = *(uint64_t*) (ctx+CK_SIPSTATE+8);
    in1 = _mm256_set_epi64x(q0, q0, q0, q0);
    q0 = *(uint64_t*) (ctx+CK_SIPSTATE+16);
    in2 = _mm256_set_epi64x(q0, q0, q0, q0);
    q0 = *(uint64_t*) (ctx+CK_SIPSTATE+24);
    in3 = _mm256_set_epi64x(q0, q0, q0, q0);
    q0 = *(uint64_t*) (ctx+CK_SIPSTATE+32);
    in1a = _mm256_set_epi64x(q0, q0, q0, q0);
    
    // Iteration 1A
    rdtsc0 = __rdtsc();
    pvbig0 = ctx + CK_BIG0BODY;
    pu32big = ctx + CK_BIG0HEAD + B_BUCKETS * 4 * tid;
    d0 = B0_SLOTS * B0_SLOTSIZE * tid;
    for (d1=0; d1<B_BUCKETS; d1++) {
        pu32big[d1] = d0;
        d0 += THREADS*B0_SLOTS*B0_SLOTSIZE;
    }
    d0 = tid * (HALFSIZE/THREADS*2);
    nonce = _mm256_set_epi64x(d0+7, d0+5, d0+3, d0+1);
    vpayload = _mm256_set_epi64x(3<<B_SLOTPAYLOADSHIFT, 2<<B_SLOTPAYLOADSHIFT, 1<<B_SLOTPAYLOADSHIFT, 0);
    for (d2=0; d2<HALFSIZE/(4*THREADS); d2++) {
        SIPHASH;
        nonce = _mm256_add_epi64(nonce, yconstnonceinc);
        v1 = v0 & yconstbucketmask;
        v0 = _mm256_srli_epi64(v0 & yconstnodemask, B_LOG) | vpayload;
        vpayload = _mm256_add_epi64(vpayload, yconstslotedgeinc);
        
        d0 = _mm256_extract_epi32(v1, 0);
        *(uint64_t*) (pvbig0+pu32big[d0]) = _mm256_extract_epi64(v0, 0);
        pu32big[d0] += B0_SLOTSIZE;
        
        d0 = _mm256_extract_epi32(v1, 2);
        *(uint64_t*) (pvbig0+pu32big[d0]) = _mm256_extract_epi64(v0, 1);
        pu32big[d0] += B0_SLOTSIZE;

        d0 = _mm256_extract_epi32(v1, 4);
        *(uint64_t*) (pvbig0+pu32big[d0]) = _mm256_extract_epi64(v0, 2);
        pu32big[d0] += B0_SLOTSIZE;
        
        d0 = _mm256_extract_epi32(v1, 6);
        *(uint64_t*) (pvbig0+pu32big[d0]) = _mm256_extract_epi64(v0, 3);
        pu32big[d0] += B0_SLOTSIZE;
    }
    pthread_barrier_wait(ctx+CK_BARRIER);
    rdtsc1 = __rdtsc();
    
#if PRINTBUCKETSTAT
    if (tid==0) {
        pu32big = ctx + CK_BIG0HEAD;
        d2 = 0;
        edgecount = 0;
        dbgctr = 0;
        for (d0=0; d0<B_BUCKETS; d0++) {
            for (d1=0; d1<THREADS; d1++) {
                d3 = d1 * B_BUCKETS + d0;
                edgecount += (pu32big[d3]-d2);
                dbgctr = ((pu32big[d3]-d2) > dbgctr) ? (pu32big[d3]-d2) : dbgctr;
                d2 += B0_SLOTS*B0_SLOTSIZE;
            }
        }
        edgecount /= B0_SLOTSIZE;
        dbgctr /= B0_SLOTSIZE;
        printf("BIG0 stat: total: %d, max: %d\n", edgecount, dbgctr);
    }
#endif

    // Iteration 1B
    pu8deg = ctx + CK_PTS0 + PTS0_SIZE * tid;
    pu32small = pvsmall0 = pu8deg + R_ENTRIES;
    bigbucket = tid * THREADPLAN0;
    bigbucketend = bigbucket + THREADPLAN0;
    pu32edgei = ctx + CK_EDGE1 + (THREADPLAN0 * THREADS * B0_SLOTS * B0_SLOTSIZE) * tid;    //Assume (THREADS*B0_SLOTS) divisible by 16
    pu32edges = pu32edgei + (BPT * S_BUCKETS);
    pu16rtemp = ctx + CK_BIG2 + (THREADPLAN0 * THREADS * B0_SLOTS * B0_SLOTSIZE / 2) * tid;
    pu32big = ctx + CK_BIG0HEAD;
    for (uint32_t threadplan=0; threadplan<2; threadplan++) {
        pvbig = pvbig0 + bigbucket * (THREADS*B0_SLOTS*B0_SLOTSIZE);
        d0 = (bigbucket!=0) ? (pu32big[(THREADS-1)*B_BUCKETS+bigbucket-1] + B0_SLOTSIZE*2) : 0;
        pvlimit = pvbig0 + d0;
        pu64readbig = (pvlimit > pvbig) ? pvlimit : pvbig;
        do {
            d0 = S_BUCKETS*4;
            for (d1=0; d1<S_BUCKETS; d1++) {
                pu32small[d1] = d0;
                d0 += S0_SLOTS*S0_SLOTSIZE;
            }
            q3 = 0;
            q4 = 0;
            for (fromthread=0; fromthread<THREADS*B_BUCKETS; fromthread+=B_BUCKETS) {
                pvlimit = pvbig0 + pu32big[fromthread+bigbucket];
                pvbig += B0_SLOTS*B0_SLOTSIZE;  //pvbig is now the beginning of the next bucket
                pvlimit2 = (pvbig<pvlimit) ? pvbig : pvlimit;
                while ((void*) pu64readbig < pvlimit2) {
                    q0 = *pu64readbig;
                    d0 = q0 & S_BUCKETMASK;
                    q1 = q0 & 0xfffff80000;
                    q4 += (q1 < q3) ? 0x100000000 : 0;
                    q3 = q1;
                    d1 = pu32small[d0];
                    d2 = q0 >> S_LOG;   //truncate to uint32_t is important, alternatively use AND mask
                    *(uint64_t*) (pvsmall0+d1) = q4 | (uint64_t) d2;
                    pu32small[d0] += S0_SLOTSIZE;
                    pu64readbig = ((void*)pu64readbig) + B0_SLOTSIZE;
                }
                pvlimit += B0_SLOTSIZE*2;       //Two more spaces because 8-byte writes were used to write 5-byte slots
                pu64readbig = (pvlimit > pvbig) ? pvlimit : pvbig;  //handle big bucket overflow
            }
            if (unlikely(q4 != 0x3f00000000)) {
                printf("Error 1B %lx\n", q4);
                *pu32abort = 1;
                goto _GiveUp1B;
            }
            pvsmall = pvsmall0 + S_BUCKETS*4;
            pu32readsmall = pvsmall;
            for (smallbucket=0; smallbucket<S_BUCKETS; smallbucket++) {
                pu64readsmall = (uint64_t*) pu32readsmall;
                memset(pu8deg, 0, R_ENTRIES);
                pvlimit = pvsmall0 + pu32small[smallbucket];
                while ((void*) pu32readsmall < pvlimit) {
                    d0 = *pu32readsmall;
                    pu8deg[d0 & (R_ENTRIES-1)]++;
                    pu32readsmall = ((void*)pu32readsmall) + S0_SLOTSIZE;
                }
                while ((void*) pu64readsmall < pvlimit) {
                    q0 = *pu64readsmall;
                    d0 = (q0 >> 10);    //Mask 0x0ffffffe omitted
                    *pu32edges = d0;
                    d0 = q0 & (R_ENTRIES-1);
                    *pu16rtemp = (uint16_t) d0;
                    d1 = (pu8deg[d0] > 1) ? 1 : 0;
                    pu32edges += d1;
                    pu16rtemp += d1;
                    pu64readsmall = ((void*)pu64readsmall) + S0_SLOTSIZE;
                }
                pvsmall += S0_SLOTS*S0_SLOTSIZE;    //advance to the next bucket
                pvlimit += S0_SLOTSIZE*2;           //Two more spaces because 8-byte writes were used to write 5-byte slots
                pu32readsmall = (pvlimit > pvsmall) ? pvlimit : pvsmall;    //handle small bucket overflow
                *(pu32edgei++) = pu32edges - pu32edgesbegin;
            }
        } while (++bigbucket < bigbucketend);
        bigbucket = tid*THREADPLAN1 + THREADS*THREADPLAN0;
        bigbucketend = bigbucket + THREADPLAN1;
    }
    
    _GiveUp1B:
    pthread_barrier_wait(ctx+CK_BARRIER);
    if (unlikely(*pu32abort))
        goto _GiveUp;
    rdtsc2 = __rdtsc();

#if PRINTSTAT
    if (tid==0) {
        d1 = B_BUCKETS / THREADS * S_BUCKETS;
        edgecount = pu32edgesbegin[d1-1] - d1;
        for (d0=1; d0<THREADS; d0++) {
            d1 += THREADPLAN0 * THREADS * B0_SLOTS * B0_SLOTSIZE / 4;
            edgecount += pu32edgesbegin[d1-1] - d1;
        }
        printf("Iteration 1: %d; rdtsc: %ld+%ld\n", edgecount, rdtsc1-rdtsc0, rdtsc2-rdtsc1);
        printf("EDGE1 end: %d, BIG1 start: %d\n", pu32edgesbegin[d1-1], (uint32_t) (CK_BIG1 - CK_EDGE1)/4);
    }
#endif
    d0 = pu32edges - pu32edgesbegin;
    d1 = (tid == THREADS-1) ? (CK_BIG1-CK_EDGE1) : ((tid+1)*THREADPLAN0*THREADS*B0_SLOTS*B0_SLOTSIZE);
    d1 /= 4;
    d0 = (d1 < d0) ? d1 : d0;
    *(--pu32edgei) = d0;
    pvedges_end = pu32edgesbegin + d0;

    // Iteration 2A
    rdtsc0 = __rdtsc();
    pvbig0 = ctx + (CK_BIG1 + B_BUCKETS * THREADS * 4);
    pu32big = ctx + CK_BIG1 + B_BUCKETS * 4 * tid;
    d0 = B1_SLOTS * B1_SLOTSIZE * tid;
    for (d1=0; d1<B_BUCKETS; d1++) {
        pu32big[d1] = d0;
        d0 += THREADS*B1_SLOTS*B1_SLOTSIZE;
    }
    pu32edgei = ctx + CK_EDGE1 + (THREADPLAN0 * THREADS * B0_SLOTS * B0_SLOTSIZE) * tid;
    pu32edges = pu32edgei + (B_BUCKETS / THREADS * S_BUCKETS);
    pu16rtemp = ctx + CK_BIG2 + (THREADPLAN0 * THREADS * B0_SLOTS * B0_SLOTSIZE / 2) * tid;
    vx = _mm256_set_epi64x(0, 0, 0, 0);     //Technically needs tid, but truncated anyway
    x1 = _mm_set_epi64x(0, 0x0001000100010001);
    goto _EntryHash2A;
    
    do {
        d0 = _mm256_extract_epi32(v1, 0);
        *(uint64_t*) (pvbig0+pu32big[d0]) = _mm256_extract_epi64(v0, 0);
        pu32big[d0] += B1_SLOTSIZE;
        
        d0 = _mm256_extract_epi32(v1, 2);
        *(uint64_t*) (pvbig0+pu32big[d0]) = _mm256_extract_epi64(v0, 1);
        pu32big[d0] += B1_SLOTSIZE;
        
        d0 = _mm256_extract_epi32(v1, 4);
        *(uint64_t*) (pvbig0+pu32big[d0]) = _mm256_extract_epi64(v0, 2);
        pu32big[d0] += B1_SLOTSIZE;
        
        d0 = _mm256_extract_epi32(v1, 6);
        *(uint64_t*) (pvbig0+pu32big[d0]) = _mm256_extract_epi64(v0, 3);
        pu32big[d0] += B1_SLOTSIZE;
        _EntryHash2A:
        nonce = _mm256_cvtepu32_epi64(*(__m128i*) pu32edges) & yconstnoncemaske;
        d0 = (pu32edgesbegin + *pu32edgei) - pu32edges;
        pu32edgei += (d0 < 4) ? 1 : 0;
        pu32edges += 4;
        x0 = _mm_slli_epi64(x1, d0*16);
        v0 = _mm256_slli_epi64(_mm256_cvtepu16_epi64(x0), 30);
        q0 = _mm256_extract_epi64(vx, 3);
        vx = _mm256_set_epi64x(q0, q0, q0, q0);
        vx = _mm256_add_epi64(vx, v0);
        SIPHASH;
        //vpayload = _mm256_slli_epi64(_mm256_cvtepu16_epi64(*(__m128i*) pu16rtemp), 19) | vx;
        vpayload = _mm256_slli_epi64(_mm256_cvtepu16_epi64(_mm_set_epi64x(0, *(uint64_t*) pu16rtemp)), 19) | vx;
        pu16rtemp += 4;
        v1 = v0 & yconstbucketmask;
        v0 = _mm256_srli_epi64(v0 & yconstnodemask, B_LOG) | vpayload;
    } while ((void*) pu32edges < pvedges_end);
    
    d3 = 4 - (pu32edges - (uint32_t*) pvedges_end); //remaining edges {1,2,3,4}

    d0 = _mm256_extract_epi32(v1, 0);
    d1 = pu32big[d0];
    *(uint64_t*) (pvbig0+d1) = _mm256_extract_epi64(v0, 0);
    pu32big[d0] += B1_SLOTSIZE;
    if (--d3 == 0)
        goto _Done2A;
    
    d0 = _mm256_extract_epi32(v1, 2);
    d1 = pu32big[d0];
    *(uint64_t*) (pvbig0+d1) = _mm256_extract_epi64(v0, 1);
    pu32big[d0] += B1_SLOTSIZE;
    if (--d3 == 0)
        goto _Done2A;
    
    d0 = _mm256_extract_epi32(v1, 4);
    d1 = pu32big[d0];
    *(uint64_t*) (pvbig0+d1) = _mm256_extract_epi64(v0, 2);
    pu32big[d0] += B1_SLOTSIZE;
    if (--d3 == 0)
        goto _Done2A;
    
    d0 = _mm256_extract_epi32(v1, 6);
    d1 = pu32big[d0];
    *(uint64_t*) (pvbig0+d1) = _mm256_extract_epi64(v0, 3);
    pu32big[d0] += B1_SLOTSIZE;

    _Done2A:
    q1 = *(uint64_t*) (pvbig0+d1);
    q1 &= 0xffffffffc0000000;
    if (unlikely(q1 != (65536ULL/THREADS-1) << 30)) {
        printf("Error 2A: %lx\n", q1);
        *pu32abort = 1;
    }
    
    pthread_barrier_wait(ctx+CK_BARRIER);
    if (unlikely(*pu32abort))
        goto _GiveUp;
    
#if PRINTBUCKETSTAT
    if (tid==0) {
        pu32big = ctx + CK_BIG1;
        d2 = 0;
        edgecount = 0;
        dbgctr = 0;
        for (d0=0; d0<B_BUCKETS; d0++) {
            for (d1=0; d1<THREADS; d1++) {
                d3 = d1 * B_BUCKETS + d0;
                edgecount += (pu32big[d3]-d2);
                dbgctr = ((pu32big[d3]-d2) > dbgctr) ? (pu32big[d3]-d2) : dbgctr;
                d2 += B1_SLOTS*B1_SLOTSIZE;
            }
        }
        edgecount /= B1_SLOTSIZE;
        dbgctr /= B1_SLOTSIZE;
        printf("BIG1 stat: total: %d, max: %d\n", edgecount, dbgctr);
    }
#endif

    // Iteration 2B
    rdtsc1 = __rdtsc();
    pu8deg = ctx + CK_PTS1 + PTS1_SIZE * tid;
    pu32small = pvsmall0 = pu8deg + R_ENTRIES;
    
    pvdst0 = ctx + (CK_BIG2 + B_BUCKETS * THREADS * 4);
    pu32dst = ctx + CK_BIG2 + B_BUCKETS * 4 * tid;
    d0 = B2_SLOTS * B2_SLOTSIZE * tid;
    for (d1=0; d1<B_BUCKETS; d1++) {
        pu32dst[d1] = d0;
        d0 += THREADS*B2_SLOTS*B2_SLOTSIZE;
    }
    
    q2 = 0;     //Technically needs tid here, but 5-byte slots truncates it anyway
    bigbucket = tid * BPT;
    bigbucketend = bigbucket + BPT;
    pvbig = pvbig0 + bigbucket * (THREADS*B1_SLOTS*B1_SLOTSIZE);
    pu32big = ctx + CK_BIG1;
    d0 = (bigbucket!=0) ? (pu32big[(THREADS-1)*B_BUCKETS+bigbucket-1] + B1_SLOTSIZE*2) : 0;
    pvlimit = pvbig0 + d0;
    pu64readbig = (pvlimit > pvbig) ? pvlimit : pvbig;
    
    for (; bigbucket<bigbucketend; bigbucket++) {
        d0 = S_BUCKETS*4;
        for (d1=0; d1<S_BUCKETS; d1++) {
            pu32small[d1] = d0;
            d0 += S1_SLOTS*S1_SLOTSIZE;
        }
        
        q3 = 0;
        q4 = 0;
        for (fromthread=0; fromthread<THREADS*B_BUCKETS; fromthread+=B_BUCKETS) {
            pvlimit = pvbig0 + pu32big[fromthread+bigbucket];
            pvbig += B1_SLOTS*B1_SLOTSIZE;  //pvbig is now the beginning of the next bucket
            pvlimit2 = (pvbig<pvlimit) ? pvbig : pvlimit;
            while ((void*) pu64readbig < pvlimit2) {
                q0 = *pu64readbig;
                d0 = q0 & S_BUCKETMASK;
                q1 = q0 & 0xffc0000000;
                q4 += (q1 < q3) ? 0x100000000 : 0;
                q3 = q1;
                d1 = pu32small[d0];
                d2 = q0 >> S_LOG;   //truncate to uint32_t is important, alternatively use AND mask
                *(uint64_t*) (pvsmall0+d1) = q4 | (uint64_t) d2;
                pu32small[d0] += S1_SLOTSIZE;
                pu64readbig = ((void*)pu64readbig) + B1_SLOTSIZE;
            }
            pvlimit += B1_SLOTSIZE*2;       //Two more spaces because 8-byte writes were used to write 5-byte slots
            pu64readbig = (pvlimit > pvbig) ? pvlimit : pvbig;  //handle big bucket overflow
        }
        if (unlikely(q4 != 0x3f00000000)) {
            printf("Error 2B %lx\n", q4);
            *pu32abort = 1;
            goto _GiveUp2B;
        }
        pvsmall = pvsmall0 + S_BUCKETS*4;
        pu32readsmall = pvsmall;
        for (smallbucket=0; smallbucket<S_BUCKETS; smallbucket++) {
            pu64readsmall = (uint64_t*) pu32readsmall;
            memset(pu8deg, 0, R_ENTRIES);
            pvlimit = pvsmall0 + pu32small[smallbucket];
            while ((void*) pu32readsmall < pvlimit) {
                pu8deg[*pu32readsmall & (R_ENTRIES-1)]++;
                pu32readsmall = ((void*)pu32readsmall) + S1_SLOTSIZE;
            }
            while ((void*) pu64readsmall < pvlimit) {
                q0 = *pu64readsmall;
                d0 = (q0>>30) & 0xff;
                q1 = ((q0>>R_LOG) & (1ULL<<(LOGHALFSIZE-B_LOG))-1);
                d1 = q0 & (R_ENTRIES-1);
                q1 |= ((uint64_t) d1) << 19;
                q1 |= q2;
                *(uint64_t*) (pvdst0 + pu32dst[d0]) = q1;
                pu32dst[d0] += (pu8deg[d1]>1) ? B2_SLOTSIZE : 0;
                pu64readsmall = ((void*)pu64readsmall) + S1_SLOTSIZE;
            }
            pvsmall += S1_SLOTS*S1_SLOTSIZE;    //advance to the next bucket
            pvlimit += S1_SLOTSIZE*2;           //Two more spaces because 8-byte writes were used to write 5-byte slots
            pu32readsmall = (pvlimit > pvsmall) ? pvlimit : pvsmall;    //handle small bucket overflow
            q2 += (1ULL<<30);
        }
    }

    _GiveUp2B:
    pthread_barrier_wait(ctx+CK_BARRIER);
    if (unlikely(*pu32abort))
        goto _GiveUp;
    
    rdtsc2 = __rdtsc();
    
#if PRINTSTAT
    if (tid==0) {
        pu32big = ctx + CK_BIG2;
        d2 = 0;
        edgecount = 0;
        dbgctr = 0;
        for (d0=0; d0<B_BUCKETS; d0++) {
            for (d1=0; d1<THREADS; d1++) {
                d3 = d1 * B_BUCKETS + d0;
                edgecount += (pu32big[d3]-d2);
                dbgctr = ((pu32big[d3]-d2) > dbgctr) ? (pu32big[d3]-d2) : dbgctr;
                d2 += B2_SLOTS*B2_SLOTSIZE;
            }
        }
        edgecount /= B2_SLOTSIZE;
        dbgctr /= B2_SLOTSIZE;
        printf("Iteration 2: %d; max: %d, rdtsc: %ld+%ld\n", edgecount, dbgctr, rdtsc1-rdtsc0, rdtsc2-rdtsc1);
    }
#endif

    TrimEdgeIter3(ctx, ctx+CK_BIG3, ctx+CK_BIG2, tid, 3);
    if (unlikely(*pu32abort))
        goto _GiveUp;
    
    ramsize0 = TrimRename1(ctx, ctx+CK_BIG2, ctx+CK_BIG3, tid, 4);
    if (unlikely(ramsize0 > 1024))
        goto _GiveUp;
    ramsize1 = TrimRename2(ctx, ctx+CK_BIG3, ctx+CK_BIG2, tid, 5);
    if (unlikely(ramsize1 > 1024))
        goto _GiveUp;
    
    for (iter=6; iter<30; iter+=2) {
        TrimEdge1Stage(ctx, ctx+CK_BIG2, ctx+CK_BIG3, tid, ramsize0, iter);
        TrimEdge1Stage(ctx, ctx+CK_BIG3, ctx+CK_BIG2, tid, ramsize1, iter+1);
    }
    TrimRename3(ctx, ctx+CK_BIG2, ctx+CK_BIG3, tid, ramsize0, 30);
    d0 = 0;
    for (d1=0; d1<THREADS; d1++) {
        d2 = *(int32_t*) (ctx + CK_THREADRESULT0 + d1*4);
        d0 = (d2>d0) ? d2 : d0;
    }
    if (unlikely(d0>262112))
        goto _GiveUp;
    MakePairs(ctx, ctx+CK_PAIRSLIST, ctx+CK_BIG2, tid, ramsize1);
    MakePairsIndex(ctx, tid);
    
    if (tid==0) {
        TrimPairsST(ctx);
        FindCycle(ctx);
    }
    
    _GiveUp:
    pthread_exit(0);
    return 0;
}

int32_t cuckoo28_mt(tctx_t *ptctx, const uint64_t k0, const uint64_t k1)
{
    pthread_t threads[THREADS];
    void * const ctx = ptctx[0].ctx;
    uint64_t * const pu64sipstate = ctx + CK_SIPSTATE;
    uint64_t q0, q1, q2, q3;
    uint32_t tid;
    
    q0 = k0 ^ 0x736f6d6570736575;
    q1 = k1 ^ 0x646f72616e646f6d;
    q2 = k0 ^ 0x6c7967656e657261;
    q3 = k1 ^ 0x7465646279746573;
    q0 += q1;
    q1 = (q1<<13) | (q1>>51);
    q1 ^= q0;
    pu64sipstate[4] = q1;
    q0 = (q0<<32) | (q0>>32);
    q1 = (q1<<17) | (q1>>47);
    pu64sipstate[0] = q0;
    pu64sipstate[1] = q1;
    pu64sipstate[2] = q2;
    pu64sipstate[3] = q3;
    *(void **) (ctx + CK_SOLPTR) = ctx;
    *(uint32_t *) (ctx + CK_ABORT) = 0;

    for (tid=0; tid<THREADS; tid++) {
        pthread_create(threads+tid, 0, &worker, ptctx+tid);
    }
    for (tid=0; tid<THREADS; tid++) {
        pthread_join(threads[tid], 0);
    }
    
    return (*(void **) (ctx+CK_SOLPTR) - ctx) / (HALFCYCLELENGTH*2*4);
}

int main(int argc, char *argv[])
{
    tctx_t tctx[THREADS];
    struct timespec time0, time1;
    uint32_t *pu32;
    void *ctx_alloc, *ctx, *ctx_end;
    uint64_t rdtsc0, rdtsc1, timens, rdtscfreq;
    uint64_t k0, k1;
    uint32_t solutions, d0, d1;
    int32_t timems;
    
    if (sizeof(pthread_barrier_t) > 64) {
        printf("sizeof(pthread_barrier_t)=%u > 64", (uint32_t) sizeof(pthread_barrier_t));
        exit(1);
    }
    
    k0 = k0def;
    k1 = k1def;
    if (argc>=3) {
        sscanf(argv[1], "%lx", &k0);
        sscanf(argv[2], "%lx", &k1);
    }
    printf("k0=%016lx k1=%016lx\n", k0, k1);
    
    ctx_alloc = malloc(CK_CONTEXT_SIZE+4096);
    ctx = (void*) (((uint64_t) ctx_alloc+4095LL) & -4096LL);
    ctx_end = ctx + CK_CONTEXT_SIZE;

    pthread_barrier_init(ctx+CK_BARRIER, 0, THREADS);
    for (d0=0; d0<THREADS; d0++) {
        tctx[d0].ctx = ctx;
        tctx[d0].tid = d0;
    }
    
    printf("Init memory (%ld bytes)\n", CK_CONTEXT_SIZE);
    for (pu32=ctx; pu32<(uint32_t*)ctx_end; pu32+=1024) {
        *pu32=0;
    }
    
    printf("Running\n");

    clock_gettime(CLOCK_MONOTONIC, &time0);
    rdtsc0 = __rdtsc();
    solutions = cuckoo28_mt(tctx, k0, k1);
    clock_gettime(CLOCK_MONOTONIC, &time1);
    rdtsc1 = __rdtsc();

    printf("solutions: %d\n", solutions);
    for (d1=0; d1<solutions; d1++) {
        printf("Solution: ");
        for (d0=0; d0<HALFCYCLELENGTH*2; d0++)
            printf("%x ", ((uint32_t*) ctx)[d1*HALFCYCLELENGTH*2+d0]);
        printf("\n");
    }
    
    timems = (time1.tv_sec-time0.tv_sec)*1000 + (time1.tv_nsec-time0.tv_nsec)/1000000;
    printf("Time: %d ms\n", timems);
    timens = (time1.tv_sec-time0.tv_sec)*1000000000 + (time1.tv_nsec-time0.tv_nsec);
    printf("rdtsc frequency: %.3f MHz\n", (double) (rdtsc1 - rdtsc0)*1000/timens);
    
    free(ctx_alloc);

    return 0;
}
