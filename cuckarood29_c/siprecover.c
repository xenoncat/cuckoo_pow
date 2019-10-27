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

static void AddEdge(uint32_t *pu32TargetNodes, uint32_t *pu32Header, uint64_t u64NodePair, uint32_t u32Edge)
{
    uint32_t u32NumTargets2 = pu32TargetNodes[0] * 42;
    uint32_t u32Node0 = u64NodePair & 0x0fffffff;
    uint32_t u32Node1 = (u64NodePair >> 32) & 0x0fffffff;
    for (uint32_t i=0; i<u32NumTargets2; i+=2) {
        if (u32Node0 == pu32TargetNodes[i+2]) {
            uint32_t u32HdrPtr = i + (u32Edge&1);
            uint32_t u32WrPtr = __sync_fetch_and_add(&(pu32Header[u32HdrPtr]), u32NumTargets2*2);
            pu32Header[u32WrPtr] = u32Node1;
            pu32Header[u32WrPtr + 1] = u32Edge;
        }
    }
}

void siprecover(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 8192);
    const __m256i yconst1 = _mm256_set_epi64x(1, 1, 1, 1);
    const __m256i yconstff = _mm256_set_epi64x(0xff, 0xff, 0xff, 0xff);
    const __m256i yconstfilteraddrmask = _mm256_set_epi64x(0x1fff, 0x1fff, 0x1fff, 0x1fff);
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
    uint32_t *pu32BitmapFilter0 = ctx + CK_BITMAP;
    uint32_t *pu32BitmapFilter1 = ctx + CK_BITMAP + 32768;
    uint32_t *pu32TargetNodes = ctx;
    uint32_t *pu32Header = ctx + CK_RECOVERWRITE;
    uint64_t u64nodepair;
    uint32_t u32node;
    int32_t i, j;

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
        vv = _mm256_xor_si256(vu, *pScratch);
        _mm256_store_si256(pScratch, vt);
        pScratch++;
        vt = _mm256_srli_epi64(vv, 5);
        vt = _mm256_and_si256(yconstfilteraddrmask, vt);
        u64nodepair = _mm256_extract_epi64(vv, 0);
        u32node = (uint32_t) u64nodepair;
        if ((pu32BitmapFilter0[(uint32_t)_mm256_extract_epi32(vt, 0)] >> (u32node & 0x1f)) & 1) {
            if ((pu32BitmapFilter1[(u32node >> 15) & 0x1fff] >> (u32node & 0x1f)) & 1) {
                AddEdge(pu32TargetNodes, pu32Header, u64nodepair, _mm256_extract_epi32(vnonce, 0));
            }
        }
        u64nodepair = _mm256_extract_epi64(vv, 1);
        u32node = (uint32_t) u64nodepair;
        if ((pu32BitmapFilter0[(uint32_t)_mm256_extract_epi32(vt, 2)] >> (u32node & 0x1f)) & 1) {
            if ((pu32BitmapFilter1[(u32node >> 15) & 0x1fff] >> (u32node & 0x1f)) & 1) {
                AddEdge(pu32TargetNodes, pu32Header, u64nodepair, _mm256_extract_epi32(vnonce, 2));
            }
        }
        u64nodepair = _mm256_extract_epi64(vv, 2);
        u32node = (uint32_t) u64nodepair;
        if ((pu32BitmapFilter0[(uint32_t)_mm256_extract_epi32(vt, 4)] >> (u32node & 0x1f)) & 1) {
            if ((pu32BitmapFilter1[(u32node >> 15) & 0x1fff] >> (u32node & 0x1f)) & 1) {
                AddEdge(pu32TargetNodes, pu32Header, u64nodepair, _mm256_extract_epi32(vnonce, 4));
            }
        }
        u64nodepair = _mm256_extract_epi64(vv, 3);
        u32node = (uint32_t) u64nodepair;
        if ((pu32BitmapFilter0[(uint32_t)_mm256_extract_epi32(vt, 6)] >> (u32node & 0x1f)) & 1) {
            if ((pu32BitmapFilter1[(u32node >> 15) & 0x1fff] >> (u32node & 0x1f)) & 1) {
                AddEdge(pu32TargetNodes, pu32Header, u64nodepair, _mm256_extract_epi32(vnonce, 6));
            }
        }

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
    vt = _mm256_srli_epi64(vu, 5);
    vt = _mm256_and_si256(yconstfilteraddrmask, vt);
    u64nodepair = _mm256_extract_epi64(vu, 0);
    u32node = (uint32_t) u64nodepair;
    if ((pu32BitmapFilter0[(uint32_t)_mm256_extract_epi32(vt, 0)] >> (u32node & 0x1f)) & 1) {
        if ((pu32BitmapFilter1[(u32node >> 15) & 0x1fff] >> (u32node & 0x1f)) & 1) {
            AddEdge(pu32TargetNodes, pu32Header, u64nodepair, _mm256_extract_epi32(vnonce, 0));
        }
    }
    u64nodepair = _mm256_extract_epi64(vu, 1);
    u32node = (uint32_t) u64nodepair;
    if ((pu32BitmapFilter0[(uint32_t)_mm256_extract_epi32(vt, 2)] >> (u32node & 0x1f)) & 1) {
        if ((pu32BitmapFilter1[(u32node >> 15) & 0x1fff] >> (u32node & 0x1f)) & 1) {
            AddEdge(pu32TargetNodes, pu32Header, u64nodepair, _mm256_extract_epi32(vnonce, 2));
        }
    }
    u64nodepair = _mm256_extract_epi64(vu, 2);
    u32node = (uint32_t) u64nodepair;
    if ((pu32BitmapFilter0[(uint32_t)_mm256_extract_epi32(vt, 4)] >> (u32node & 0x1f)) & 1) {
        if ((pu32BitmapFilter1[(u32node >> 15) & 0x1fff] >> (u32node & 0x1f)) & 1) {
            AddEdge(pu32TargetNodes, pu32Header, u64nodepair, _mm256_extract_epi32(vnonce, 4));
        }
    }
    u64nodepair = _mm256_extract_epi64(vu, 3);
    u32node = (uint32_t) u64nodepair;
    if ((pu32BitmapFilter0[(uint32_t)_mm256_extract_epi32(vt, 6)] >> (u32node & 0x1f)) & 1) {
        if ((pu32BitmapFilter1[(u32node >> 15) & 0x1fff] >> (u32node & 0x1f)) & 1) {
            AddEdge(pu32TargetNodes, pu32Header, u64nodepair, _mm256_extract_epi32(vnonce, 6));
        }
    }

    vnonce = _mm256_add_epi64(vnonce, yconst1);
    SIPBEGIN0
    j = 64;
    pScratch = pScratchBegin;
    if (--i)
        goto _EnterLoopSip;

    j = 63;
    do {
        vv = _mm256_xor_si256(vu, *pScratch);
        pScratch++;
        vt = _mm256_srli_epi64(vv, 5);
        vt = _mm256_and_si256(yconstfilteraddrmask, vt);
        u64nodepair = _mm256_extract_epi64(vv, 0);
        u32node = (uint32_t) u64nodepair;
        if ((pu32BitmapFilter0[(uint32_t)_mm256_extract_epi32(vt, 0)] >> (u32node & 0x1f)) & 1) {
            if ((pu32BitmapFilter1[(u32node >> 15) & 0x1fff] >> (u32node & 0x1f)) & 1) {
                AddEdge(pu32TargetNodes, pu32Header, u64nodepair, _mm256_extract_epi32(vnonce, 0));
            }
        }
        u64nodepair = _mm256_extract_epi64(vv, 1);
        u32node = (uint32_t) u64nodepair;
        if ((pu32BitmapFilter0[(uint32_t)_mm256_extract_epi32(vt, 2)] >> (u32node & 0x1f)) & 1) {
            if ((pu32BitmapFilter1[(u32node >> 15) & 0x1fff] >> (u32node & 0x1f)) & 1) {
                AddEdge(pu32TargetNodes, pu32Header, u64nodepair, _mm256_extract_epi32(vnonce, 2));
            }
        }
        u64nodepair = _mm256_extract_epi64(vv, 2);
        u32node = (uint32_t) u64nodepair;
        if ((pu32BitmapFilter0[(uint32_t)_mm256_extract_epi32(vt, 4)] >> (u32node & 0x1f)) & 1) {
            if ((pu32BitmapFilter1[(u32node >> 15) & 0x1fff] >> (u32node & 0x1f)) & 1) {
                AddEdge(pu32TargetNodes, pu32Header, u64nodepair, _mm256_extract_epi32(vnonce, 4));
            }
        }
        u64nodepair = _mm256_extract_epi64(vv, 3);
        u32node = (uint32_t) u64nodepair;
        if ((pu32BitmapFilter0[(uint32_t)_mm256_extract_epi32(vt, 6)] >> (u32node & 0x1f)) & 1) {
            if ((pu32BitmapFilter1[(u32node >> 15) & 0x1fff] >> (u32node & 0x1f)) & 1) {
                AddEdge(pu32TargetNodes, pu32Header, u64nodepair, _mm256_extract_epi32(vnonce, 6));
            }
        }
        vnonce = _mm256_add_epi64(vnonce, yconst1);
    } while (--j);
}
