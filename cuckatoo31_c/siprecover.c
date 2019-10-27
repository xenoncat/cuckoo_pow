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

static void AddEdge(uint32_t *pu32TargetNodes, uint32_t *pu32Header, uint64_t u32Node, uint32_t u32Edge)
{
    uint32_t u32NumSols42 = pu32TargetNodes[0] * 42;
    uint32_t i = 0;
    while (i < u32NumSols42) {
        uint32_t u32Xor = u32Node ^ pu32TargetNodes[i+2];
        if ((u32Xor & -2) == 0) {
            uint32_t u32WrPtr = __sync_fetch_and_add(&(pu32Header[i | u32Xor]), u32NumSols42*2);
            pu32Header[u32WrPtr] = u32Edge >> 1;
            pu32Header[u32WrPtr + 1] = 0;
            //round up to multiple of 42
            i *= 6242;
            i >>= 18;
            i += 1;
            i *= 42;
            continue;
        }
        i += 2;
    }
}

void siprecover(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 8192);
    const __m256i yconst8 = _mm256_set_epi64x(8, 8, 8, 8);
    const __m256i yconstff = _mm256_set_epi64x(0xff, 0xff, 0xff, 0xff);
    const __m256i yconstfilteraddrmask = _mm256_set_epi64x(0x1fff, 0x1fff, 0x1fff, 0x1fff);
    const __m256i yconstnodemask = _mm256_set_epi64x(0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff);
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
    uint32_t *pu32BitmapFilter0 = ctx + CK_BITMAP;
    uint32_t *pu32BitmapFilter1 = ctx + CK_BITMAP + 32768;
    uint32_t *pu32TargetNodes = ctx;
    uint32_t *pu32Header = ctx + CK_RECOVERWRITE;
    //uint32_t *pu32DstCtr = ctx + (tid * CK_THREADSEP) + CK_COUNTER0;
    //void *pDstCtrRef = pu32DstCtr;
    uint32_t u32BitOffset, u32Node;
    int32_t i;

    for (i=0; i<(536870912/THREADS); i++) {
        SIPBEGIN
        SIPROUND
        v0 = _mm256_xor_si256(vnonce, v0);
        v2 = _mm256_xor_si256(yconstff, v2);
        SIPROUND
        SIPROUND
        SIPROUND
        SIPEND

        //30:1
        //18:6 5:1  filter 0
        //30:18 5:1 filter 1
        v0 = _mm256_srli_epi64(v1, 6);
        v0 = _mm256_and_si256(yconstfilteraddrmask, v0);
        v2 = _mm256_srli_epi64(v1, 1);
        v1 = _mm256_and_si256(yconstnodemask, v1);

        u32BitOffset = _mm256_extract_epi32(v2, 0);
        if ((pu32BitmapFilter0[(uint32_t)_mm256_extract_epi32(v0, 0)] >> (u32BitOffset & 0x1f)) & 1) {
            u32Node = _mm256_extract_epi32(v1, 0);
            if ((pu32BitmapFilter1[u32Node >> 18] >> (u32BitOffset & 0x1f)) & 1) {
                AddEdge(pu32TargetNodes, pu32Header, u32Node, _mm256_extract_epi32(vnonce, 0));
            }
        }
        u32BitOffset = _mm256_extract_epi32(v2, 2);
        if ((pu32BitmapFilter0[(uint32_t)_mm256_extract_epi32(v0, 2)] >> (u32BitOffset & 0x1f)) & 1) {
            u32Node = _mm256_extract_epi32(v1, 2);
            if ((pu32BitmapFilter1[u32Node >> 18] >> (u32BitOffset & 0x1f)) & 1) {
                AddEdge(pu32TargetNodes, pu32Header, u32Node, _mm256_extract_epi32(vnonce, 0)+2);
            }
        }
        u32BitOffset = _mm256_extract_epi32(v2, 4);
        if ((pu32BitmapFilter0[(uint32_t)_mm256_extract_epi32(v0, 4)] >> (u32BitOffset & 0x1f)) & 1) {
            u32Node = _mm256_extract_epi32(v1, 4);
            if ((pu32BitmapFilter1[u32Node >> 18] >> (u32BitOffset & 0x1f)) & 1) {
                AddEdge(pu32TargetNodes, pu32Header, u32Node, _mm256_extract_epi32(vnonce, 0)+4);
            }
        }
        u32BitOffset = _mm256_extract_epi32(v2, 6);
        if ((pu32BitmapFilter0[(uint32_t)_mm256_extract_epi32(v0, 6)] >> (u32BitOffset & 0x1f)) & 1) {
            u32Node = _mm256_extract_epi32(v1, 6);
            if ((pu32BitmapFilter1[u32Node >> 18] >> (u32BitOffset & 0x1f)) & 1) {
                AddEdge(pu32TargetNodes, pu32Header, u32Node, _mm256_extract_epi32(vnonce, 0)+6);
            }
        }

        vnonce = _mm256_add_epi64(yconst8, vnonce);
    }
}
