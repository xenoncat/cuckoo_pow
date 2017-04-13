#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <x86intrin.h>
#include "siphash_avx2.h"
#include "memlayout20_st.h"

#define PRINTSTAT 1
#define PRINTBUCKETSTAT 1

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

static const __m256i yconst1 = {1, 1, 1, 1};
static const __m256i yconstff = {0xff, 0xff, 0xff, 0xff};
static const __m256i yconstrol16 = {0x0504030201000706, 0x0d0c0b0a09080f0e, 0x0504030201000706, 0x0d0c0b0a09080f0e};
static const __m256i yconstnonceinc = {8, 8, 8, 8};
static const __m256i yconstnoncemaske = {NONCEMASKE, NONCEMASKE, NONCEMASKE, NONCEMASKE};
static const __m256i yconstnodemask = {NODEMASK, NODEMASK, NODEMASK, NODEMASK};
static const __m256i yconstbucketmask = {B_BUCKETMASK, B_BUCKETMASK, B_BUCKETMASK, B_BUCKETMASK};
static const __m256i yconstslotedgeinc = {4<<B_SLOTPAYLOADSHIFT, 4<<B_SLOTPAYLOADSHIFT, 4<<B_SLOTPAYLOADSHIFT, 4<<B_SLOTPAYLOADSHIFT};

//2 2 12 42 96 (nonce=13)
static uint64_t k0def = 0x46bb8d7ff5f709ec;
static uint64_t k1def = 0x844d5c859087ab25;

static void TrimEdgeIter3(void * const context, void * const dst, void * const src, const uint32_t iter)
{
    void * const ctx = __builtin_assume_aligned(context, 64);
    uint32_t * const pu32dst = __builtin_assume_aligned(dst, 64);
    uint32_t * const pu32src = __builtin_assume_aligned(src, 64);
    void * const pvdstbody = pu32dst + B_BUCKETS;
    void * const pvsrcbody = pu32src + B_BUCKETS;
    uint8_t * const pu8deg = ctx + CK_RAM1;
    
    void *pvsrc, *pvread, *pvread2, *pvlimit;
    uint64_t rdtsc0, rdtsc1;
    uint32_t bucket, bucketshifted, edgecount, dbgctr=0;
    uint32_t d0, d1, d2, d3;
    
    rdtsc0 = __rdtsc();
    d0 = 0;
    for (bucket=0; bucket<B_BUCKETS; bucket++) {
        pu32dst[bucket] = d0;
        d0 += B3_SLOTS*B3_SLOTSIZE;
    }
    pvread = pvsrc = pvsrcbody;
    bucketshifted = 0;

    for (bucket=0; bucket<B_BUCKETS; bucket++) {
        memset(pu8deg, 0, R_ENTRIES);
        pvlimit = pvsrcbody + pu32src[bucket];
        pvread2 = pvread;
        while (pvread < pvlimit) {
            d0 = *(uint32_t*) pvread;
            pu8deg[d0 & (R_ENTRIES-1)]++;
            pvread += B2_SLOTSIZE;
        }
        while (pvread2 < pvlimit) {
            d0 = *(uint32_t*) pvread2;
            d1 = d0 & (R_ENTRIES-1);
            d2 = (d0 >> R_LOG) & (R_ENTRIES-1);
            d3 = (d0 >> (R_LOG + R_LOG)) & B_BUCKETMASK;     // mask 0xff optional?
            *(uint32_t*) (pvdstbody + pu32dst[d3]) = bucketshifted | (d1<<R_LOG) | d2;
            pu32dst[d3] += (pu8deg[d1] > 1) ? B3_SLOTSIZE : 0;
            pvread2 += B2_SLOTSIZE;
        }
        pvsrc += B2_SLOTS*B2_SLOTSIZE;  //advance to next bucket
        pvlimit += B2_SLOTSIZE;
        pvread = (pvlimit > pvsrc) ? pvlimit : pvsrc;
        bucketshifted += (1 << 22);
    }

    rdtsc1 = __rdtsc();
#if PRINTSTAT
    d2 = 0;
    edgecount = 0;
    dbgctr = 0;
    for (bucket=0; bucket<B_BUCKETS; bucket++) {
        edgecount += (pu32dst[bucket]-d2);
        dbgctr = ((pu32dst[bucket]-d2) > dbgctr) ? (pu32dst[bucket]-d2) : dbgctr;
        d2 += B3_SLOTS*B3_SLOTSIZE;
    }
    edgecount /= B3_SLOTSIZE;
    dbgctr /= B3_SLOTSIZE;
    printf("Iteration %d: %d; max: %d, rdtsc: %ld\n", iter, edgecount, dbgctr, rdtsc1-rdtsc0);
#endif
}

static uint32_t TrimEdgeIter4Rename(void * const context, void * const dst, void * const src, const uint32_t iter)
{
    void * const ctx = __builtin_assume_aligned(context, 64);
    uint32_t * const pu32dst = __builtin_assume_aligned(dst, 64);
    uint32_t * const pu32src = __builtin_assume_aligned(src, 64);
    void * const pvdstbody = pu32dst + B_BUCKETS;
    void * const pvsrcbody = pu32src + B_BUCKETS;
    uint16_t * const pu16deg = ctx + CK_RAM1;
    
    void *pvsrc, *pvread, *pvread2, *pvlimit;
    uint64_t rdtsc0, rdtsc1;
    uint32_t bucket, edgecount, dbgctr=0;
    uint32_t d0, d1, d2, d3;
    int32_t newnodeid=0, newnodeid_small;
    uint16_t w0;
    
    rdtsc0 = __rdtsc();
    d0 = 0;
    for (bucket=0; bucket<B_BUCKETS; bucket++) {
        pu32dst[bucket] = d0;
        d0 += B3_SLOTS*B3_SLOTSIZE;
    }
    pvread = pvsrc = pvsrcbody;

    for (bucket=0; bucket<B_BUCKETS; bucket++) {
        memset(pu16deg, 0, R_ENTRIES*2);
        newnodeid_small = 31;
        pvlimit = pvsrcbody + pu32src[bucket];
        pvread2 = pvread;
        while (pvread < pvlimit) {
            d0 = *(uint32_t*) pvread;
            pu16deg[d0 & (R_ENTRIES-1)]++;
            pvread += B3_SLOTSIZE;
        }
        while (pvread2 < pvlimit) {
            d0 = *(uint32_t*) pvread2;
            d1 = d0 & (R_ENTRIES-1);
            w0 = pu16deg[d1];
            if (w0 > 1) {
                newnodeid_small += (w0<32) ? 1 : 0;
                w0 = (w0<32) ? (uint16_t) newnodeid_small : w0;
                pu16deg[d1] = w0;
                d2 = (d0 >> R_LOG) & (R_ENTRIES-1);
                d3 = (d0 >> (R_LOG + R_LOG)) & B_BUCKETMASK;     // mask 0xff optional?
                d0 = ((uint32_t) w0) + newnodeid;
                d0 = d0 << R_LOG;
                *(uint32_t*) (pvdstbody + pu32dst[d3]) = d0 | d2;
                pu32dst[d3] += B3_SLOTSIZE;
            }
            pvread2 += B3_SLOTSIZE;
        }
        pvsrc += B3_SLOTS*B3_SLOTSIZE;  //advance to next bucket
        pvlimit += B3_SLOTSIZE;
        pvread = (pvlimit > pvsrc) ? pvlimit : pvsrc;
        newnodeid = newnodeid + (newnodeid_small - 31);
    }

    rdtsc1 = __rdtsc();
#if PRINTSTAT
    d2 = 0;
    edgecount = 0;
    dbgctr = 0;
    for (bucket=0; bucket<B_BUCKETS; bucket++) {
        edgecount += (pu32dst[bucket]-d2);
        dbgctr = ((pu32dst[bucket]-d2) > dbgctr) ? (pu32dst[bucket]-d2) : dbgctr;
        d2 += B3_SLOTS*B3_SLOTSIZE;
    }
    edgecount /= B3_SLOTSIZE;
    dbgctr /= B3_SLOTSIZE;
    printf("Iteration %d: %d; max: %d, rdtsc: %ld\n", iter, edgecount, dbgctr, rdtsc1-rdtsc0);
    printf("nodecount=%d\n", newnodeid);
#endif
    return newnodeid;
}

static void MakePairs(void * const context, void * const dst, void * const src)
{
    void * const ctx = __builtin_assume_aligned(context, 64);
    uint32_t * const pu32src = __builtin_assume_aligned(src, 64);
    void * const pvsrcbody = pu32src + B_BUCKETS;
    uint16_t * const pu16lastseen = ctx + CK_RAM1;
    uint8_t * const pbitfield = ctx + CK_PAIRSBITFIELD;
    
    uint32_t *pu32dst = dst;
    void *pvsrc, *pvread, *pvlimit;
    uint64_t rdtsc0, rdtsc1;
    uint64_t q0;
    uint32_t srcbucket, srcbucketshifted, dbgctr=0;
    uint32_t d0, d1, d2, d3;
    
    rdtsc0 = __rdtsc();
    pvread = pvsrc = pvsrcbody;
    srcbucketshifted = 0;

    for (srcbucket=0; srcbucket<B_BUCKETS; srcbucket++) {
        memset(pu16lastseen, 0, R_ENTRIES*2);
        pvlimit = pvsrcbody + pu32src[srcbucket];
        while (pvread < pvlimit) {
            d0 = *(uint32_t*) pvread;
            d1 = d0 & (R_ENTRIES-1);
            d2 = pu16lastseen[d1];
            d3 = (d0>>11);
            pu16lastseen[d1] = d3;
            pu32dst[0] = d2;    //15 bit u
            pu32dst[1] = d3;    //15 bit u
            pu32dst[2] = srcbucketshifted | d1;  //{v[7:0], v[18:8]}
            pu32dst += d2 ? 3 : 0;
            pvread += B3_SLOTSIZE;
        }
        pvsrc += B3_SLOTS*B3_SLOTSIZE;  //advance to next bucket
        pvlimit += B3_SLOTSIZE;
        pvread = (pvlimit > pvsrc) ? pvlimit : pvsrc;
        srcbucketshifted += 1 << 11;
    }
    d1 = ((void*) pu32dst - dst)/12;
    *(uint32_t*) (ctx+CK_PAIRSCOUNT) = d1;
    d2 = (d1+7)/8;
    memset(pbitfield, 0, d2);

    rdtsc1 = __rdtsc();
#if PRINTSTAT
    printf("MakePairs: pairs=%d, rdtsc: %ld\n", d1, rdtsc1-rdtsc0);
#endif
}

static void MakePairsIndex(void * const context)
{
    void * const ctx = __builtin_assume_aligned(context, 64);
    uint32_t * const pu32big = ctx + CK_BIGP;
    void * const pvbigbody = ctx + CK_BIGP +  B_BUCKETS * 4;
    uint32_t * const hashtab = ctx + CK_RAM1;
    uint32_t * const pu32pairsindex = ctx + CK_PAIRSINDEX;
    uint8_t * const pbitfield = ctx + CK_PAIRSBITFIELD;
    
    uint64_t rdtsc0, rdtsc1;
    uint32_t * pu32src = ctx + CK_PAIRSLIST;
    uint32_t *pu32src_end;
    void *pvbig, *pvread, *pvread2, *pvlimit;
    uint32_t d0, d1, d2, d3, d4, d5;
    uint32_t paircounter_shifted, bucket, pairsindexwrite, dbgctr=0;
    uint8_t b0;
    
    rdtsc0 = __rdtsc();
    d0 = 0;
    for (bucket=0; bucket<B_BUCKETS; bucket++) {
        pu32big[bucket] = d0;
        d0 += BP_SLOTS*BP_SLOTSIZE;
    }
    d0 = *(uint32_t*) (ctx + CK_PAIRSCOUNT);
    pu32src_end = (void*) pu32src + d0 * 12;
    paircounter_shifted = 0;
    while (pu32src < pu32src_end) {
        d0 = pu32src[0];    //15-bit u
        d1 = pu32src[1];    //15-bit u
        pu32src += 3;
        if (d0 == d1) {
            d2 = paircounter_shifted >> 7;
            pbitfield[d2/8] |= (1 << (d2%8));
            printf("2-cycle found\n");
            paircounter_shifted += (1 << 7);
            continue;
        }
        d2 = d0 & B_BUCKETMASK;
        d3 = d1 & B_BUCKETMASK;
        *(uint32_t*) (pvbigbody+pu32big[d2]) = (d0 >> B_LOG) | paircounter_shifted;
        pu32big[d2] += BP_SLOTSIZE;
        *(uint32_t*) (pvbigbody+pu32big[d3]) = (d1 >> B_LOG) | paircounter_shifted;
        pu32big[d3] += BP_SLOTSIZE;
        paircounter_shifted += (1 << 7);
    }

    pu32pairsindex[2] = 16; //first slot at offset 64 byte
    pu32pairsindex[3] = 16 + PI_SLOTS_2*2;
    pu32pairsindex[4] = 16 + PI_SLOTS_2*2 + PI_SLOTS_3*3;
    pu32pairsindex[5] = 16 + PI_SLOTS_2*2 + PI_SLOTS_3*3 + PI_SLOTS_4*4;
    pu32pairsindex[6] = 16 + PI_SLOTS_2*2 + PI_SLOTS_3*3 + PI_SLOTS_4*4 + PI_SLOTS_5*5;
    pu32pairsindex[7] = 16 + PI_SLOTS_2*2 + PI_SLOTS_3*3 + PI_SLOTS_4*4 + PI_SLOTS_5*5 + PI_SLOTS_6*6;
    
    pvread = pvbig = pvbigbody;
    
    for (bucket=0; bucket<B_BUCKETS; bucket++) {
        memset(hashtab, 0, 512);
        
        pvread2 = pvread;
        pvlimit = pvbigbody + pu32big[bucket];
        while (pvread < pvlimit) {
            d0 = *(uint32_t*) pvread;
            hashtab[d0 & 0x7f] += (1<<24);
            pvread += BP_SLOTSIZE;
        }
        
        while (pvread2 < pvlimit) {
            d0 = *(uint32_t*) pvread2;
            d1 = d0 & 0x7f;
            d2 = d0 >> 7;
            d3 = hashtab[d1];
            pvread2 += BP_SLOTSIZE;
            b0 = d3 >> 24;
            if ((b0<2) | (b0>7) ) {
                pbitfield[d2/8] |= (1 << (d2%8));
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
        pvbig += BP_SLOTS*BP_SLOTSIZE;  //advance to the next bucket
        pvlimit += BP_SLOTSIZE;
        pvread = (pvlimit > pvbig) ? pvlimit : pvbig;  //handle bucket overflow
    }
    
    rdtsc1 = __rdtsc();

#if PRINTSTAT
    printf("MakePairsIndex, rdtsc: %ld\n", rdtsc1-rdtsc0);
#endif

}

static void TrimPairs(void * const context)
{
    void * const ctx = __builtin_assume_aligned(context, 64);
    uint8_t * const pbitfield = ctx + CK_PAIRSBITFIELD;
    uint32_t * const pu32pairsindex = ctx + CK_PAIRSINDEX;
    uint32_t* pu32end;
    uint64_t *pu64read, *pu64end, *pu64write;
    uint32_t *pu32read, *pu32write, *pu32write2;
    uint64_t rdtsc0, rdtsc1;
    uint32_t d0, d1, d2, d3, d4, d5, innerloop, outerloop, moreiter;
    
    uint32_t const k2 = 16; //first slot at offset 64 byte
    uint32_t const k3 = 16 + PI_SLOTS_2*2;
    uint32_t const k4 = 16 + PI_SLOTS_2*2 + PI_SLOTS_3*3;
    uint32_t const k5 = 16 + PI_SLOTS_2*2 + PI_SLOTS_3*3 + PI_SLOTS_4*4;
    uint32_t const k6 = 16 + PI_SLOTS_2*2 + PI_SLOTS_3*3 + PI_SLOTS_4*4 + PI_SLOTS_5*5;
    uint32_t const k7 = 16 + PI_SLOTS_2*2 + PI_SLOTS_3*3 + PI_SLOTS_4*4 + PI_SLOTS_5*5 + PI_SLOTS_6*6;
    
#if PRINTSTAT
    d0 = (pu32pairsindex[2]-k2);
    d1 = (pu32pairsindex[3]-k3);
    d2 = (pu32pairsindex[4]-k4);
    d3 = (pu32pairsindex[5]-k5);
    d4 = (pu32pairsindex[6]-k6);
    d5 = (pu32pairsindex[7]-k7);
    printf("TrimPairs Start: %d %d %d %d %d %d\n", d0/2, d1/3, d2/4, d3/5, d4/6, d5/7);
#endif
    
    rdtsc0 = __rdtsc();
    for (outerloop=0; outerloop<12; outerloop++) {
        for (innerloop=0; innerloop<32; innerloop++) {
            moreiter = 0;
            pu64write = pu64read = (uint64_t*) &pu32pairsindex[k2];
            pu64end = (uint64_t*) &pu32pairsindex[pu32pairsindex[2]];
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
            pu32pairsindex[2] = ((uint32_t*) pu64write - pu32pairsindex);
            if (moreiter == 0)
                break;
        }

        if (innerloop==0)
            break;
        
        pu32write = pu32read = &pu32pairsindex[k7];
        pu32write2 = &pu32pairsindex[pu32pairsindex[6]];
        pu32end = &pu32pairsindex[pu32pairsindex[7]];
        while (pu32read < pu32end) {
            if (pbitfield[pu32read[0]/8] & (1 << (pu32read[0]%8))) {
                pu32write2[0] = pu32read[1];
                pu32write2[1] = pu32read[2];
                pu32write2[2] = pu32read[3];
                pu32write2[3] = pu32read[4];
                pu32write2[4] = pu32read[5];
                pu32write2[5] = pu32read[6];
                pu32write2 += 6;
            }
            else if (pbitfield[pu32read[1]/8] & (1 << (pu32read[1]%8))) {
                pu32write2[0] = pu32read[0];
                pu32write2[1] = pu32read[2];
                pu32write2[2] = pu32read[3];
                pu32write2[3] = pu32read[4];
                pu32write2[4] = pu32read[5];
                pu32write2[5] = pu32read[6];
                pu32write2 += 6;
            }
            else if (pbitfield[pu32read[2]/8] & (1 << (pu32read[2]%8))) {
                pu32write2[0] = pu32read[0];
                pu32write2[1] = pu32read[1];
                pu32write2[2] = pu32read[3];
                pu32write2[3] = pu32read[4];
                pu32write2[4] = pu32read[5];
                pu32write2[5] = pu32read[6];
                pu32write2 += 6;
            }
            else if (pbitfield[pu32read[3]/8] & (1 << (pu32read[3]%8))) {
                pu32write2[0] = pu32read[0];
                pu32write2[1] = pu32read[1];
                pu32write2[2] = pu32read[2];
                pu32write2[3] = pu32read[4];
                pu32write2[4] = pu32read[5];
                pu32write2[5] = pu32read[6];
                pu32write2 += 6;
            }
            else if (pbitfield[pu32read[4]/8] & (1 << (pu32read[4]%8))) {
                pu32write2[0] = pu32read[0];
                pu32write2[1] = pu32read[1];
                pu32write2[2] = pu32read[2];
                pu32write2[3] = pu32read[3];
                pu32write2[4] = pu32read[5];
                pu32write2[5] = pu32read[6];
                pu32write2 += 6;
            }
            else if (pbitfield[pu32read[5]/8] & (1 << (pu32read[5]%8))) {
                pu32write2[0] = pu32read[0];
                pu32write2[1] = pu32read[1];
                pu32write2[2] = pu32read[2];
                pu32write2[3] = pu32read[3];
                pu32write2[4] = pu32read[4];
                pu32write2[5] = pu32read[6];
                pu32write2 += 6;
            }
            else if (pbitfield[pu32read[6]/8] & (1 << (pu32read[6]%8))) {
                pu32write2[0] = pu32read[0];
                pu32write2[1] = pu32read[1];
                pu32write2[2] = pu32read[2];
                pu32write2[3] = pu32read[3];
                pu32write2[4] = pu32read[4];
                pu32write2[5] = pu32read[5];
                pu32write2 += 6;
            }
            else {
                pu32write[0] = pu32read[0];
                pu32write[1] = pu32read[1];
                pu32write[2] = pu32read[2];
                pu32write[3] = pu32read[3];
                pu32write[4] = pu32read[4];
                pu32write[5] = pu32read[5];
                pu32write[6] = pu32read[6];
                pu32write += 7;
            }
            pu32read += 7;
        }
        pu32pairsindex[7] = pu32write - pu32pairsindex;
        pu32end = pu32write2;
        
        pu32write = pu32read = &pu32pairsindex[k6];
        pu32write2 = &pu32pairsindex[pu32pairsindex[5]];
        while (pu32read < pu32end) {
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
        pu32pairsindex[6] = pu32write - pu32pairsindex;
        pu32end = pu32write2;
        
        pu32write = pu32read = &pu32pairsindex[k5];
        pu32write2 = &pu32pairsindex[pu32pairsindex[4]];
        while (pu32read < pu32end) {
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
        pu32pairsindex[5] = pu32write - pu32pairsindex;
        pu32end = pu32write2;
        
        pu32write = pu32read = &pu32pairsindex[k4];
        pu32write2 = &pu32pairsindex[pu32pairsindex[3]];
        while (pu32read < pu32end) {
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
        pu32pairsindex[4] = pu32write - pu32pairsindex;
        pu32end = pu32write2;
        
        pu32write = pu32read = &pu32pairsindex[k3];
        pu32write2 = &pu32pairsindex[pu32pairsindex[2]];
        while (pu32read < pu32end) {
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
        pu32pairsindex[3] = pu32write - pu32pairsindex;
        pu32pairsindex[2] = pu32write2 - pu32pairsindex;
    }
    
    rdtsc1 = __rdtsc();
    
#if PRINTSTAT
    d0 = (pu32pairsindex[2]-k2);
    d1 = (pu32pairsindex[3]-k3);
    d2 = (pu32pairsindex[4]-k4);
    d3 = (pu32pairsindex[5]-k5);
    d4 = (pu32pairsindex[6]-k6);
    d5 = (pu32pairsindex[7]-k7);
    printf("TrimPairs End: %d %d %d %d %d %d\n", d0/2, d1/3, d2/4, d3/5, d4/6, d5/7);
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
    uint32_t * const pu32edgesbegin = ctx + CK_EDGE1;
    uint32_t * const pu32edgei = ctx + CK_EDGE1;
    uint32_t * const pu32edges = pu32edgei + B_BUCKETS;
    void *pvedges, *pvend;
    uint32_t *solbuf;
    
    uint32_t nonceubuf[3][32];
    uint64_t q0;
    uint32_t nonceuctr, prevnonceuctr, savednonceuctr;
    uint32_t cyclepos, flatbucket, sipvnode, odd;
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
        
    for (cyclepos=0; cyclepos<halfcyclelen; cyclepos++) {
        odd = cyclepos & 1;
        d0 = listv[cyclepos];
        sipvnode = ((d0 << 8) & 0x7ff00) | ((d0 >> 11) & 0xff);   //undo {v[7:0], v[18:8]} caused by MakePairs()
        flatbucket = sipvnode & 0xff;
        
        d0 = (flatbucket!=0) ? (pu32edgei[flatbucket-1]) : (pu32edges - pu32edgesbegin);
        d1 = pu32edgei[flatbucket];
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
#if PRINTSTAT
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

int32_t cuckoo20_st(void *context, const uint64_t k0, const uint64_t k1)
{
    __m256i in0,in1,in2,in3,in1a;
    __m256i v0,v1,v2,v3,t0;
    __m256i vx, nonce, slotnonce, vpayload;
    __m128i x0, x1;
    void * const ctx = __builtin_assume_aligned(context, 64);
    uint64_t * const pu64sipstate = ctx + CK_SIPSTATE;
    uint32_t * const pu32edgesbegin = ctx + CK_EDGE1;
    uint32_t * const pu32abort = ctx + CK_ABORT;
    
    uint32_t *pu32big, *pu32edgei, *pu32edges, *pu32dst;
    uint16_t *pu16rtemp;
    uint8_t *pu8deg;
    void *pvbigbody, *pvbig, *pvread, *pvread2, *pvlimit, *pvedges_end, *pvdstbody;
    uint64_t rdtsc0, rdtsc1, rdtsc2;
    uint64_t q0, q1, q2, q3, q4;
    uint32_t bucket, edgecount, iter, dbgctr, bucketshifted;
    uint32_t d0, d1, d2, d3;
    
    q0 = k0 ^ 0x736f6d6570736575;
    q1 = k1 ^ 0x646f72616e646f6d;
    q2 = k0 ^ 0x6c7967656e657261;
    q3 = k1 ^ 0x7465646279746573;
    q0 += q1;
    q1 = (q1<<13) | (q1>>51);
    q1 ^= q0;
    pu64sipstate[4] = q1;
    in1a = _mm256_set_epi64x(q1, q1, q1, q1);
    q0 = (q0<<32) | (q0>>32);
    q1 = (q1<<17) | (q1>>47);
    pu64sipstate[0] = q0;
    pu64sipstate[1] = q1;
    pu64sipstate[2] = q2;
    pu64sipstate[3] = q3;
    *(void **) (ctx + CK_SOLPTR) = ctx;
    *(uint32_t *) (ctx + CK_ABORT) = 0;
    
    in0 = _mm256_set_epi64x(q0, q0, q0, q0);
    in1 = _mm256_set_epi64x(q1, q1, q1, q1);
    in2 = _mm256_set_epi64x(q2, q2, q2, q2);
    in3 = _mm256_set_epi64x(q3, q3, q3, q3);
    
    // Iteration 1A
    rdtsc0 = __rdtsc();
    pvbigbody = ctx + CK_BIG0BODY;
    pu32big = ctx + CK_BIG0HEAD;
    d0 = 0;
    for (bucket=0; bucket<B_BUCKETS; bucket++) {
        pu32big[bucket] = d0;
        d0 += B0_SLOTS*B0_SLOTSIZE;
    }
    nonce = _mm256_set_epi64x(7, 5, 3, 1);
    vpayload = _mm256_set_epi64x(3<<B_SLOTPAYLOADSHIFT, 2<<B_SLOTPAYLOADSHIFT, 1<<B_SLOTPAYLOADSHIFT, 0);
    for (d2=0; d2<HALFSIZE/4; d2++) {
        SIPHASH;
        nonce = _mm256_add_epi64(nonce, yconstnonceinc);
        v1 = v0 & yconstbucketmask;
        v0 = _mm256_srli_epi64(v0 & yconstnodemask, B_LOG) | vpayload;
        vpayload = _mm256_add_epi64(vpayload, yconstslotedgeinc);
        
        d0 = _mm256_extract_epi32(v1, 0);
        *(uint32_t*) (pvbigbody+pu32big[d0]) = _mm256_extract_epi32(v0, 0);
        pu32big[d0] += B0_SLOTSIZE;
        
        d0 = _mm256_extract_epi32(v1, 2);
        *(uint32_t*) (pvbigbody+pu32big[d0]) = _mm256_extract_epi32(v0, 2);
        pu32big[d0] += B0_SLOTSIZE;

        d0 = _mm256_extract_epi32(v1, 4);
        *(uint32_t*) (pvbigbody+pu32big[d0]) = _mm256_extract_epi32(v0, 4);
        pu32big[d0] += B0_SLOTSIZE;
        
        d0 = _mm256_extract_epi32(v1, 6);
        *(uint32_t*) (pvbigbody+pu32big[d0]) = _mm256_extract_epi32(v0, 6);
        pu32big[d0] += B0_SLOTSIZE;
    }
    rdtsc1 = __rdtsc();
    
#if PRINTBUCKETSTAT
    d2 = 0;
    edgecount = 0;
    dbgctr = 0;
    for (d0=0; d0<B_BUCKETS; d0++) {
        edgecount += (pu32big[d0]-d2);
        dbgctr = ((pu32big[d0]-d2) > dbgctr) ? (pu32big[d0]-d2) : dbgctr;
        d2 += B0_SLOTS*B0_SLOTSIZE;
    }
    edgecount /= B0_SLOTSIZE;
    dbgctr /= B0_SLOTSIZE;
    printf("BIG0 stat: total: %d, max: %d\n", edgecount, dbgctr);
#endif

    // Iteration 1B
    pu8deg = ctx + CK_RAM0;
    pu32edgei = ctx + CK_EDGE1;
    pu32edges = pu32edgei + B_BUCKETS;
    pu16rtemp = ctx + CK_BIG2;
    pvread = pvbig = pvbigbody;

    for (bucket=0; bucket<B_BUCKETS; bucket++) {
        memset(pu8deg, 0, R_ENTRIES);
        pvread2 = pvread;
        pvlimit = pvbigbody + pu32big[bucket];
        while (pvread < pvlimit) {
            d0 = *(uint32_t*) pvread;
            pu8deg[d0 & (R_ENTRIES-1)]++;
            pvread += B0_SLOTSIZE;
        }
        while (pvread2 < pvlimit) {
            d0 = *(uint32_t*) pvread2;
            *pu32edges = d0 >> 10;  //Mask 0xffffe omitted
            d0 = d0 & (R_ENTRIES-1);
            *pu16rtemp = (uint16_t) d0;
            d1 = (pu8deg[d0] > 1) ? 1 : 0;
            pu32edges += d1;
            pu16rtemp += d1;
            pvread2 += B0_SLOTSIZE;
        }
        pvbig += B0_SLOTS * B0_SLOTSIZE;    //advance to the next bucket
        pvlimit += B0_SLOTSIZE;
        pvread = (pvlimit > pvbig) ? pvlimit : pvbig;
        *(pu32edgei++) = pu32edges - pu32edgesbegin;
    }
    rdtsc2 = __rdtsc();

#if PRINTSTAT
    d1 = B_BUCKETS;
    edgecount = pu32edgesbegin[d1-1] - d1;
    printf("Iteration 1: %d; rdtsc: %ld+%ld\n", edgecount, rdtsc1-rdtsc0, rdtsc2-rdtsc1);
    printf("EDGE1 end: %d, BIG1 start: %d\n", pu32edgesbegin[d1-1], (uint32_t) (CK_BIG1 - CK_EDGE1)/4);
#endif
    d0 = pu32edges - pu32edgesbegin;
    d1 = (CK_BIG1-CK_EDGE1)/4;
    d0 = (d1 < d0) ? d1 : d0;
    *(--pu32edgei) = d0;
    pvedges_end = pu32edgesbegin + d0;
    
    // Iteration 2A
    rdtsc0 = __rdtsc();
    pvbigbody = ctx + (CK_BIG1 + B_BUCKETS * 4);
    pu32big = ctx + CK_BIG1;
    d0 = 0;
    for (bucket=0; bucket<B_BUCKETS; bucket++) {
        pu32big[bucket] = d0;
        d0 += B1_SLOTS*B1_SLOTSIZE;
    }
    pu32edgei = ctx + CK_EDGE1;
    pu32edges = pu32edgei + B_BUCKETS;
    pu16rtemp = ctx + CK_BIG2;
    vx = _mm256_set_epi64x(0, 0, 0, 0);
    x1 = _mm_set_epi64x(0, 0x0001000100010001);
    goto _EntryHash2A;
    
    do {
        d0 = _mm256_extract_epi32(v1, 0);
        *(uint32_t*) (pvbigbody+pu32big[d0]) = _mm256_extract_epi32(v0, 0);
        pu32big[d0] += B1_SLOTSIZE;
        
        d0 = _mm256_extract_epi32(v1, 2);
        *(uint32_t*) (pvbigbody+pu32big[d0]) = _mm256_extract_epi32(v0, 2);
        pu32big[d0] += B1_SLOTSIZE;
        
        d0 = _mm256_extract_epi32(v1, 4);
        *(uint32_t*) (pvbigbody+pu32big[d0]) = _mm256_extract_epi32(v0, 4);
        pu32big[d0] += B1_SLOTSIZE;
        
        d0 = _mm256_extract_epi32(v1, 6);
        *(uint32_t*) (pvbigbody+pu32big[d0]) = _mm256_extract_epi32(v0, 6);
        pu32big[d0] += B1_SLOTSIZE;
        _EntryHash2A:
        nonce = _mm256_cvtepu32_epi64(*(__m128i*) pu32edges) & yconstnoncemaske;
        d0 = (pu32edgesbegin + *pu32edgei) - pu32edges;
        pu32edgei += (d0 < 4) ? 1 : 0;
        pu32edges += 4;
        x0 = _mm_slli_epi64(x1, d0*16);
        v0 = _mm256_slli_epi64(_mm256_cvtepu16_epi64(x0), 22);
        q0 = _mm256_extract_epi64(vx, 3);
        vx = _mm256_set_epi64x(q0, q0, q0, q0);
        vx = _mm256_add_epi64(vx, v0);
        SIPHASH;
        vpayload = _mm256_slli_epi64(_mm256_cvtepu16_epi64(_mm_set_epi64x(0, *(uint64_t*) pu16rtemp)), 11) | vx;
        pu16rtemp += 4;
        v1 = v0 & yconstbucketmask;
        v0 = _mm256_srli_epi64(v0 & yconstnodemask, B_LOG) | vpayload;
    } while ((void*) pu32edges < pvedges_end);
    
    d2 = 4 - (pu32edges - (uint32_t*) pvedges_end); //remaining edges {1,2,3,4}

    d0 = _mm256_extract_epi32(v1, 0);
    d1 = pu32big[d0];
    *(uint32_t*) (pvbigbody+d1) = _mm256_extract_epi32(v0, 0);
    pu32big[d0] += B1_SLOTSIZE;
    if (--d2 == 0)
        goto _Done2A;
    
    d0 = _mm256_extract_epi32(v1, 2);
    d1 = pu32big[d0];
    *(uint32_t*) (pvbigbody+d1) = _mm256_extract_epi32(v0, 2);
    pu32big[d0] += B1_SLOTSIZE;
    if (--d2 == 0)
        goto _Done2A;
    
    d0 = _mm256_extract_epi32(v1, 4);
    d1 = pu32big[d0];
    *(uint32_t*) (pvbigbody+d1) = _mm256_extract_epi32(v0, 4);
    pu32big[d0] += B1_SLOTSIZE;
    if (--d2 == 0)
        goto _Done2A;
    
    d0 = _mm256_extract_epi32(v1, 6);
    d1 = pu32big[d0];
    *(uint32_t*) (pvbigbody+d1) = _mm256_extract_epi32(v0, 6);
    pu32big[d0] += B1_SLOTSIZE;

    _Done2A:
    d0 = *(uint32_t*) (pvbigbody+d1);
    d0 &= 0xffc00000;
    if (unlikely(d0 != 255 << 22)) {
        printf("Error 2A: %x\n", d0);
        goto _GiveUp;
    }
    
#if PRINTBUCKETSTAT
    d2 = 0;
    edgecount = 0;
    dbgctr = 0;
    for (bucket=0; bucket<B_BUCKETS; bucket++) {
        edgecount += (pu32big[bucket]-d2);
        dbgctr = ((pu32big[bucket]-d2) > dbgctr) ? (pu32big[bucket]-d2) : dbgctr;
        d2 += B1_SLOTS*B1_SLOTSIZE;
    }
    edgecount /= B1_SLOTSIZE;
    dbgctr /= B1_SLOTSIZE;
    printf("BIG1 stat: total: %d, max: %d\n", edgecount, dbgctr);
#endif

    // Iteration 2B
    rdtsc1 = __rdtsc();
    pu8deg = ctx + CK_RAM1;
    pu32dst = ctx + CK_BIG2;
    pvdstbody = ctx + (CK_BIG2 + B_BUCKETS * 4);
    d0 = 0;
    for (bucket=0; bucket<B_BUCKETS; bucket++) {
        pu32dst[bucket] = d0;
        d0 += B2_SLOTS*B2_SLOTSIZE;
    }
    pu32big = ctx + CK_BIG1;
    pvread = pvbig = pvbigbody;
    bucketshifted = 0;
    for (bucket=0; bucket<B_BUCKETS; bucket++) {
        memset(pu8deg, 0, R_ENTRIES);
        pvread2 = pvread;
        pvlimit = pvbigbody + pu32big[bucket];
        while (pvread < pvlimit) {
            d0 = *(uint32_t*) pvread;
            pu8deg[d0 & (R_ENTRIES-1)]++;
            pvread += B1_SLOTSIZE;
        }
        while (pvread2 < pvlimit) {
            d0 = *(uint32_t*) pvread2;
            d1 = d0 & (R_ENTRIES-1);
            d2 = (d0 >> R_LOG) & (R_ENTRIES-1);
            d3 = (d0 >> (R_LOG + R_LOG)) & B_BUCKETMASK;     // mask 0xff optional?
            *(uint32_t*) (pvdstbody + pu32dst[d3]) = bucketshifted | (d1<<R_LOG) | d2;
            pu32dst[d3] += (pu8deg[d1] > 1) ? B2_SLOTSIZE : 0;
            pvread2 += B1_SLOTSIZE;
        }
        pvbig += B1_SLOTS * B1_SLOTSIZE;    //advance to the next bucket
        pvlimit += B1_SLOTSIZE;
        pvread = (pvlimit > pvbig) ? pvlimit : pvbig;
        bucketshifted += 1 << 22;
    }
    rdtsc2 = __rdtsc();
    
#if PRINTSTAT
    d2 = 0;
    edgecount = 0;
    dbgctr = 0;
    for (bucket=0; bucket<B_BUCKETS; bucket++) {
        edgecount += (pu32dst[bucket]-d2);
        dbgctr = ((pu32dst[bucket]-d2) > dbgctr) ? (pu32dst[bucket]-d2) : dbgctr;
        d2 += B2_SLOTS*B2_SLOTSIZE;
    }
    edgecount /= B2_SLOTSIZE;
    dbgctr /= B2_SLOTSIZE;
    printf("Iteration 2: %d; max: %d, rdtsc: %ld+%ld\n", edgecount, dbgctr, rdtsc1-rdtsc0, rdtsc2-rdtsc1);
#endif
    
    TrimEdgeIter3(ctx, ctx+CK_BIG3, ctx+CK_BIG2, 3);
    TrimEdgeIter4Rename(ctx, ctx+CK_BIG2, ctx+CK_BIG3, 4);

    MakePairs(ctx, ctx+CK_PAIRSLIST, ctx+CK_BIG2);
    MakePairsIndex(ctx);
    TrimPairs(ctx);
    FindCycle(ctx);

    _GiveUp:
    return (*(void **) (ctx+CK_SOLPTR) - ctx) / (HALFCYCLELENGTH*2*4);
}

int main(int argc, char *argv[])
{
    struct timespec time0, time1;
    uint32_t *pu32;
    void *ctx_alloc, *ctx, *ctx_end;
    uint64_t rdtsc0, rdtsc1, timens, rdtscfreq;
    uint64_t k0, k1;
    uint32_t solutions, d0, d1;
    int32_t timems;
        
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
    
    printf("Init memory (%ld bytes)\n", CK_CONTEXT_SIZE);
    for (pu32=ctx; pu32<(uint32_t*)ctx_end; pu32+=1024) {
        *pu32=0;
    }
    
    printf("Running\n");

    clock_gettime(CLOCK_MONOTONIC, &time0);
    rdtsc0 = __rdtsc();
    solutions = cuckoo20_st(ctx, k0, k1);
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
