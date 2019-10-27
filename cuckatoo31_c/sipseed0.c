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
    vt = _mm256_slli_epi64(v3, 21); \
    v3 = _mm256_srli_epi64(v3, 43); \
    vt = _mm256_xor_si256(vt, v0); \
    v2 = _mm256_shuffle_epi32(v2, 0xb1); \
    v3 = _mm256_xor_si256(vt, v3);

#define SIPBEGIN \
    v3 = _mm256_xor_si256(vi3, vnonce); \
    v2 = _mm256_add_epi64(vi2, v3); \
    v3 = _mm256_shuffle_epi8(v3, yconstrol16); \
    v3 = _mm256_xor_si256(v3, v2); \
    v2 = _mm256_add_epi64(vi1, v2); \
    v0 = _mm256_add_epi64(vi0, v3); \
    v1 = _mm256_xor_si256(vi4, v2); \
    v2 = _mm256_shuffle_epi32(v2, 0xb1); \
    vt = _mm256_slli_epi64(v3, 21); \
    v3 = _mm256_srli_epi64(v3, 43); \
    vt = _mm256_xor_si256(vt, v0); \
    v3 = _mm256_xor_si256(vt, v3);

#define SIPEND \
    v0 = _mm256_add_epi64(v0, v1); \
    vt = _mm256_slli_epi64(v1, 13); \
    v1 = _mm256_srli_epi64(v1, 51); \
    vt = _mm256_xor_si256(vt, v0); \
    v2 = _mm256_add_epi64(v2, v3); \
    v3 = _mm256_shuffle_epi8(v3, yconstrol16); \
    v1 = _mm256_xor_si256(vt, v1); \
    v3 = _mm256_xor_si256(v3, v2); \
    v2 = _mm256_add_epi64(v2, v1); \
    vt = _mm256_slli_epi64(v1, 17); \
    v1 = _mm256_srli_epi64(v1, 47); \
    vt = _mm256_xor_si256(vt, v2); \
    v1 = _mm256_xor_si256(vt, v1); \
    vt = _mm256_slli_epi64(v3, 21); \
    v3 = _mm256_srli_epi64(v3, 43); \
    v2 = _mm256_shuffle_epi32(v2, 0xb1); \
    v3 = _mm256_xor_si256(vt, v3); \
    v1 = _mm256_xor_si256(v1, v2); \
    v1 = _mm256_xor_si256(v1, v3);

void sipseed0(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 8192);
    const __m256i yconst8 = _mm256_set_epi64x(8, 8, 8, 8);
    const __m256i yconstff = _mm256_set_epi64x(0xff, 0xff, 0xff, 0xff);
    const __m256i yconstbucketmask = _mm256_set_epi64x(0x1fff, 0x1fff, 0x1fff, 0x1fff);
    const __m256i yconstbitmapmask = _mm256_set_epi64x(0x3ffff, 0x3ffff, 0x3ffff, 0x3ffff);
    const __m256i yconstrol16 = _mm256_set_epi64x(0x0d0c0b0a09080f0e, 0x0504030201000706, 0x0d0c0b0a09080f0e, 0x0504030201000706);
    const __m256i yconstnonceinit = _mm256_set_epi64x(7, 5, 3, 1);
    const uint64_t u64init0 = *(uint64_t*) (ctx+CK_BLAKE2BSTATE);
    const uint64_t u64init1 = *(uint64_t*) (ctx+CK_BLAKE2BSTATE+8);
    const uint64_t u64init2 = *(uint64_t*) (ctx+CK_BLAKE2BSTATE+16);
    const uint64_t u64init3 = *(uint64_t*) (ctx+CK_BLAKE2BSTATE+24);
    const uint64_t u64init4 = *(uint64_t*) (ctx+CK_BLAKE2BSTATE+32);
    const __m256i vi0 = _mm256_set_epi64x(u64init0, u64init0, u64init0, u64init0);
    const __m256i vi1 = _mm256_set_epi64x(u64init1, u64init1, u64init1, u64init1);
    const __m256i vi2 = _mm256_set_epi64x(u64init2, u64init2, u64init2, u64init2);
    const __m256i vi3 = _mm256_set_epi64x(u64init3, u64init3, u64init3, u64init3);
    const __m256i vi4 = _mm256_set_epi64x(u64init4, u64init4, u64init4, u64init4);
    __m256i v0, v1, v2, v3, vt;
    const __m256i yshiftedtid = _mm256_set_epi64x(tid << (32-LOGTHREADS), tid << (32-LOGTHREADS), tid << (32-LOGTHREADS), tid << (32-LOGTHREADS));
    __m256i vnonce = _mm256_add_epi64(yconstnonceinit, yshiftedtid);
    uint32_t *pu32DstCtr = ctx + (tid * CK_THREADSEP) + CK_COUNTER0;
    void *pDstCtrRef = pu32DstCtr;
    int32_t i;
    uint32_t u32Bucket, rel32Write;

    uint32_t rel32CtrInit = CK_BUCKET0 - CK_COUNTER0;
    *(uint32_t *) (ctx + (tid * CK_THREADSEP) + CK_COUNTER1 - 4) = 0;
    pu32DstCtr[-1] = 0;
    for (i=0; i<8192; i++) {
        pu32DstCtr[i] = rel32CtrInit;
        rel32CtrInit += CK_BUCKETSIZE0;
    }

    for (i=0; i<(536870912/THREADS); i++) {
        SIPBEGIN
        SIPROUND
        v0 = _mm256_xor_si256(vnonce, v0);
        v2 = _mm256_xor_si256(yconstff, v2);
        SIPROUND
        SIPROUND
        SIPROUND
        SIPEND

        //21edge, 1x, 18v
        v0 = _mm256_slli_epi64(vnonce, 18);
        v2 = _mm256_and_si256(yconstbucketmask, v1);
        v1 = _mm256_srli_epi64(v1, 13);
        v1 = _mm256_and_si256(yconstbitmapmask, v1);
        v0 = _mm256_or_si256(v0, v1);
        vnonce = _mm256_add_epi64(vnonce, yconst8);

        u32Bucket = _mm256_extract_epi32(v2, 0);
        rel32Write = pu32DstCtr[u32Bucket];
        *(uint64_t *) (pDstCtrRef + rel32Write) = _mm256_extract_epi64(v0, 0);
        pu32DstCtr[u32Bucket] = rel32Write + 5;
        u32Bucket = _mm256_extract_epi32(v2, 2);
        rel32Write = pu32DstCtr[u32Bucket];
        *(uint64_t *) (pDstCtrRef + rel32Write) = _mm256_extract_epi64(v0, 1);
        pu32DstCtr[u32Bucket] = rel32Write + 5;
        u32Bucket = _mm256_extract_epi32(v2, 4);
        rel32Write = pu32DstCtr[u32Bucket];
        *(uint64_t *) (pDstCtrRef + rel32Write) = _mm256_extract_epi64(v0, 2);
        pu32DstCtr[u32Bucket] = rel32Write + 5;
        u32Bucket = _mm256_extract_epi32(v2, 6);
        rel32Write = pu32DstCtr[u32Bucket];
        *(uint64_t *) (pDstCtrRef + rel32Write) = _mm256_extract_epi64(v0, 3);
        pu32DstCtr[u32Bucket] = rel32Write + 5;
    }
}
