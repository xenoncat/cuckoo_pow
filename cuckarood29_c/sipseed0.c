#include <stdint.h>
#include <x86intrin.h>
#include "memlayout.h"

#define SIPROUND \
    v0 = _mm256_add_epi64(v0, v1); \
    vt = _mm256_slli_epi64(v1, 13); \
    v1 = _mm256_srli_epi64(v1, 51); \
    vt = _mm256_xor_si256(vt, v0); \
    v2 = _mm256_add_epi64(v2, v3); \
    v3 = _mm256_shuffle_epi8(v3, yconstrol16); \
    v1 = _mm256_xor_si256(vt, v1); \
    v3 = _mm256_xor_si256(v3, v2); \
    v0 = _mm256_shuffle_epi32(v0, 0xb1); \
    v2 = _mm256_add_epi64(v2, v1); \
    vt = _mm256_slli_epi64(v1, 17); \
    v1 = _mm256_srli_epi64(v1, 47); \
    vt = _mm256_xor_si256(vt, v2); \
    v0 = _mm256_add_epi64(v0, v3); \
    v1 = _mm256_xor_si256(vt, v1); \
    vt = _mm256_slli_epi64(v3, 25); \
    v3 = _mm256_srli_epi64(v3, 39); \
    vt = _mm256_xor_si256(vt, v0); \
    v2 = _mm256_shuffle_epi32(v2, 0xb1); \
    v3 = _mm256_xor_si256(vt, v3);

#define SIPBEGIN0 \
    v3 = _mm256_xor_si256(vi3, vnonce); \
    v2 = _mm256_add_epi64(vi2, v3); \
    v3 = _mm256_shuffle_epi8(v3, yconstrol16); \
    v3 = _mm256_xor_si256(v3, v2); \
    v2 = _mm256_add_epi64(vi1, v2); \
    v0 = _mm256_add_epi64(vi0, v3); \
    vt = _mm256_slli_epi64(vi1, 17); \
    v1 = _mm256_srli_epi64(vi1, 47); \
    vt = _mm256_xor_si256(vt, v2); \
    v1 = _mm256_xor_si256(vt, v1);

#define SIPBEGIN1 \
    vt = _mm256_slli_epi64(v3, 25); \
    v3 = _mm256_srli_epi64(v3, 39); \
    vt = _mm256_xor_si256(vt, v0); \
    v2 = _mm256_shuffle_epi32(v2, 0xb1); \
    v3 = _mm256_xor_si256(vt, v3);

#define SIPEND0 \
    v0 = _mm256_add_epi64(v0, v1); \
    vt = _mm256_slli_epi64(v1, 13); \
    v1 = _mm256_srli_epi64(v1, 51); \
    vt = _mm256_xor_si256(vt, v0); \
    v2 = _mm256_add_epi64(v2, v3); \
    v3 = _mm256_shuffle_epi8(v3, yconstrol16); \
    v1 = _mm256_xor_si256(vt, v1); \
    v3 = _mm256_xor_si256(v3, v2);

#define SIPEND1 \
    v0 = _mm256_shuffle_epi32(v0, 0xb1); \
    v2 = _mm256_add_epi64(v2, v1); \
    vt = _mm256_slli_epi64(v1, 17); \
    v1 = _mm256_srli_epi64(v1, 47); \
    vt = _mm256_xor_si256(vt, v2); \
    v0 = _mm256_add_epi64(v0, v3); \
    v1 = _mm256_xor_si256(vt, v1); \
    vt = _mm256_slli_epi64(v3, 25); \
    v3 = _mm256_srli_epi64(v3, 39); \
    vt = _mm256_xor_si256(vt, v3); \
    v2 = _mm256_shuffle_epi32(v2, 0xb1); \
    v3 = _mm256_xor_si256(vt, v0); \
    vt = _mm256_xor_si256(vt, v1); \
    vt = _mm256_xor_si256(vt, v2);

#define SIPINNER0 \
    v3 = _mm256_xor_si256(vnonce, v3); \
    v0 = _mm256_add_epi64(v0, v1); \
    vt = _mm256_slli_epi64(v1, 13); \
    v1 = _mm256_srli_epi64(v1, 51); \
    vt = _mm256_xor_si256(vt, v0); \
    v2 = _mm256_add_epi64(v2, v3); \
    v3 = _mm256_shuffle_epi8(v3, yconstrol16); \
    v1 = _mm256_xor_si256(vt, v1); \
    v3 = _mm256_xor_si256(v3, v2); \
    v0 = _mm256_shuffle_epi32(v0, 0xb1); \
    v2 = _mm256_add_epi64(v2, v1); \
    vt = _mm256_slli_epi64(v1, 17); \
    v1 = _mm256_srli_epi64(v1, 47); \
    vt = _mm256_xor_si256(vt, v2); \
    v0 = _mm256_add_epi64(v0, v3); \
    v1 = _mm256_xor_si256(vt, v1);

void sipseed0(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 8192);
    const __m256i yconst1 = _mm256_set_epi64x(1, 1, 1, 1);
    const __m256i yconstff = _mm256_set_epi64x(0xff, 0xff, 0xff, 0xff);
    const __m256i yconstbucketmask = _mm256_set_epi64x(0x3ff, 0x3ff, 0x3ff, 0x3ff);
    const __m256i yconstrol16 = _mm256_set_epi64x(0x0d0c0b0a09080f0e, 0x0504030201000706, 0x0d0c0b0a09080f0e, 0x0504030201000706);
    const __m256i yconstnonceinit = _mm256_set_epi64x(3 << (27-LOGTHREADS), 2 << (27-LOGTHREADS), 1 << (27-LOGTHREADS), 0);
    const uint64_t u64init0 = *(uint64_t*) (ctx+CK_BLAKE2BSTATE);
    const uint64_t u64init1 = *(uint64_t*) (ctx+CK_BLAKE2BSTATE+8);
    const uint64_t u64init2 = *(uint64_t*) (ctx+CK_BLAKE2BSTATE+16);
    const uint64_t u64init3 = *(uint64_t*) (ctx+CK_BLAKE2BSTATE+24);
    const __m256i vi0 = _mm256_set_epi64x(u64init0, u64init0, u64init0, u64init0);
    const __m256i vi1 = _mm256_set_epi64x(u64init1, u64init1, u64init1, u64init1);
    const __m256i vi2 = _mm256_set_epi64x(u64init2, u64init2, u64init2, u64init2);
    const __m256i vi3 = _mm256_set_epi64x(u64init3, u64init3, u64init3, u64init3);
    __m256i v0, v1, v2, v3, vt, vu, vv;
    const __m256i yshiftedtid = _mm256_set_epi64x(tid << (29-LOGTHREADS), tid << (29-LOGTHREADS), tid << (29-LOGTHREADS), tid << (29-LOGTHREADS));
    __m256i vnonce = _mm256_add_epi64(yconstnonceinit, yshiftedtid);
    __m256i *pScratchBegin = ctx + (tid * CK_THREADSEP) + CK_SIPSCRATCH;
    uint32_t *pu32DstCtr = ctx + (tid * CK_THREADSEP) + CK_COUNTER0;
    int32_t i, j;
    uint32_t u32Bucket, u32Ctr;
    void *pDstCtrRef = pu32DstCtr;

    uint32_t rel32CtrInit = CK_BUCKET0 - CK_COUNTER0;
    *(uint32_t *) (ctx + (tid * CK_THREADSEP) + CK_COUNTER1 - 4) = 0;
    pu32DstCtr[-1] = 0;
    for (i=0; i<2048; i++) {
        pu32DstCtr[i] = rel32CtrInit;
        rel32CtrInit += CK_BUCKETSIZE0;
    }

    SIPBEGIN0
    __m256i *pScratch = pScratchBegin;
    j = 64;
    goto _EnterLoopFirstSip;
    do {
        SIPEND1
        _mm256_store_si256(pScratch, vt);
        pScratch++;

        vnonce = _mm256_add_epi64(vnonce, yconst1);
        SIPINNER0
_EnterLoopFirstSip:
        SIPBEGIN1
        SIPROUND

        v0 = _mm256_xor_si256(vnonce, v0);
        v2 = _mm256_xor_si256(yconstff, v2);

        SIPROUND
        SIPROUND
        SIPROUND
        SIPEND0
    } while (--j);

    i = (2097152 / THREADS);
    goto _EnterLoopOuterSip;

    do {
        SIPEND1
        pu32DstCtr = (uint32_t *) ((uint64_t) pu32DstCtr ^ 4);
        vv = _mm256_xor_si256(vu, *pScratch);
        _mm256_store_si256(pScratch, vt);
        pScratch++;
        vt = _mm256_slli_epi64(vv, 4);
        vt = _mm256_blend_epi32(vv, vt, 0x55);
        vv = _mm256_and_si256(yconstbucketmask, vv);
        vt = _mm256_srli_epi64(vt, 14);
        u32Bucket = _mm256_extract_epi32(vv, 0);
        u32Ctr = pu32DstCtr[u32Bucket*2UL];
        *(uint64_t *) (pDstCtrRef + u32Ctr) = _mm256_extract_epi64(vt, 0);
        pu32DstCtr[u32Bucket*2UL] = u32Ctr + 6;
        u32Bucket = _mm256_extract_epi32(vv, 2);
        u32Ctr = pu32DstCtr[u32Bucket*2UL];
        *(uint64_t *) (pDstCtrRef + u32Ctr) = _mm256_extract_epi64(vt, 1);
        pu32DstCtr[u32Bucket*2UL] = u32Ctr + 6;
        u32Bucket = _mm256_extract_epi32(vv, 4);
        u32Ctr = pu32DstCtr[u32Bucket*2UL];
        *(uint64_t *) (pDstCtrRef + u32Ctr) = _mm256_extract_epi64(vt, 2);
        pu32DstCtr[u32Bucket*2UL] = u32Ctr + 6;
        u32Bucket = _mm256_extract_epi32(vv, 6);
        u32Ctr = pu32DstCtr[u32Bucket*2UL];
        *(uint64_t *) (pDstCtrRef + u32Ctr) = _mm256_extract_epi64(vt, 3);
        pu32DstCtr[u32Bucket*2UL] = u32Ctr + 6;

        vnonce = _mm256_add_epi64(vnonce, yconst1);
        SIPINNER0
_EnterLoopSip:
        SIPBEGIN1
        SIPROUND

        v0 = _mm256_xor_si256(vnonce, v0);
        v2 = _mm256_xor_si256(yconstff, v2);

        SIPROUND
        SIPROUND
        SIPROUND
        SIPEND0
    } while (--j);

_EnterLoopOuterSip:
    v2 = _mm256_add_epi64(v2, v1);
    vt = _mm256_slli_epi64(v1, 17);
    v1 = _mm256_srli_epi64(v1, 47);
    vt = _mm256_xor_si256(vt, v2);
    v1 = _mm256_xor_si256(vt, v1);
    vt = _mm256_slli_epi64(v3, 25);
    v3 = _mm256_srli_epi64(v3, 39);
    vt = _mm256_xor_si256(vt, v3);
    v2 = _mm256_shuffle_epi32(v2, 0xb1);
    vt = _mm256_xor_si256(vt, v1);
    vu = _mm256_xor_si256(vt, v2);
    pu32DstCtr = (uint32_t *) ((uint64_t) pu32DstCtr ^ 4);
    v0 = _mm256_slli_epi64(vu, 4);
    v0 = _mm256_blend_epi32(vu, v0, 0x55);
    v1 = _mm256_and_si256(yconstbucketmask, vu);
    v0 = _mm256_srli_epi64(v0, 14);
    u32Bucket = _mm256_extract_epi32(v1, 0);
    u32Ctr = pu32DstCtr[u32Bucket*2UL];
    *(uint64_t *) (pDstCtrRef + u32Ctr) = _mm256_extract_epi64(v0, 0);
    pu32DstCtr[u32Bucket*2UL] = u32Ctr + 6;
    u32Bucket = _mm256_extract_epi32(v1, 2);
    u32Ctr = pu32DstCtr[u32Bucket*2UL];
    *(uint64_t *) (pDstCtrRef + u32Ctr) = _mm256_extract_epi64(v0, 1);
    pu32DstCtr[u32Bucket*2UL] = u32Ctr + 6;
    u32Bucket = _mm256_extract_epi32(v1, 4);
    u32Ctr = pu32DstCtr[u32Bucket*2UL];
    *(uint64_t *) (pDstCtrRef + u32Ctr) = _mm256_extract_epi64(v0, 2);
    pu32DstCtr[u32Bucket*2UL] = u32Ctr + 6;
    u32Bucket = _mm256_extract_epi32(v1, 6);
    u32Ctr = pu32DstCtr[u32Bucket*2UL];
    *(uint64_t *) (pDstCtrRef + u32Ctr) = _mm256_extract_epi64(v0, 3);
    pu32DstCtr[u32Bucket*2UL] = u32Ctr + 6;

    vnonce = _mm256_add_epi64(vnonce, yconst1);
    SIPBEGIN0
    j = 64;
    pScratch = pScratchBegin;
    if (--i)
        goto _EnterLoopSip;

    j = 63;
    do {
        pu32DstCtr = (uint32_t *) ((uint64_t) pu32DstCtr ^ 4);
        vv = _mm256_xor_si256(vu, *pScratch);
        pScratch++;
        vt = _mm256_slli_epi64(vv, 4);
        vt = _mm256_blend_epi32(vv, vt, 0x55);
        vv = _mm256_and_si256(yconstbucketmask, vv);
        vt = _mm256_srli_epi64(vt, 14);
        u32Bucket = _mm256_extract_epi32(vv, 0);
        u32Ctr = pu32DstCtr[u32Bucket*2UL];
        *(uint64_t *) (pDstCtrRef + u32Ctr) = _mm256_extract_epi64(vt, 0);
        pu32DstCtr[u32Bucket*2UL] = u32Ctr + 6;
        u32Bucket = _mm256_extract_epi32(vv, 2);
        u32Ctr = pu32DstCtr[u32Bucket*2UL];
        *(uint64_t *) (pDstCtrRef + u32Ctr) = _mm256_extract_epi64(vt, 1);
        pu32DstCtr[u32Bucket*2UL] = u32Ctr + 6;
        u32Bucket = _mm256_extract_epi32(vv, 4);
        u32Ctr = pu32DstCtr[u32Bucket*2UL];
        *(uint64_t *) (pDstCtrRef + u32Ctr) = _mm256_extract_epi64(vt, 2);
        pu32DstCtr[u32Bucket*2UL] = u32Ctr + 6;
        u32Bucket = _mm256_extract_epi32(vv, 6);
        u32Ctr = pu32DstCtr[u32Bucket*2UL];
        *(uint64_t *) (pDstCtrRef + u32Ctr) = _mm256_extract_epi64(vt, 3);
        pu32DstCtr[u32Bucket*2UL] = u32Ctr + 6;
    } while (--j);
}
