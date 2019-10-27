#include <stdio.h>
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

static const uint32_t PhaseLookup1[] =
#if THREADS == 4
{240, 0, 366, 960, 568, 2424, 874, 4696};
#elif THREADS == 8
{108, 0, 160, 864, 240, 2144, 258, 4064, 258, 6128};
#endif

static uint32_t InitDest1[] = {CK_BUCKET1A-CK_COUNTER1, CK_BUCKET1B-(CK_COUNTER1+32768), CK_BUCKET1C-(CK_COUNTER1+65536), CK_BUCKET1D-(CK_COUNTER1+98304)
#if R1PHASES > 4
, CK_BUCKET1E-(CK_COUNTER1+131072)
#endif
};
static uint32_t InitDestInc1[] = {CK_BUCKETSIZE1A, CK_BUCKETSIZE1B, CK_BUCKETSIZE1C, CK_BUCKETSIZE1D
#if R1PHASES > 4
, CK_BUCKETSIZE1E
#endif
};

void sipseed1(void *context, uint32_t tid, uint32_t phase)
{
    void *const ctx = __builtin_assume_aligned(context, 8192);
    const __m256i yconstff = _mm256_set_epi64x(0xff, 0xff, 0xff, 0xff);
    const __m256i yconstedgemask = _mm256_set_epi64x(0x3ffffe, 0x3ffffe, 0x3ffffe, 0x3ffffe);
    const __m256i yconstbucketmask = _mm256_set_epi64x(0x1fff, 0x1fff, 0x1fff, 0x1fff);
    const __m256i yconstbitmapmask = _mm256_set_epi64x(0x3ffff, 0x3ffff, 0x3ffff, 0x3ffff);
    const __m256i yconstrol16 = _mm256_set_epi64x(0x0d0c0b0a09080f0e, 0x0504030201000706, 0x0d0c0b0a09080f0e, 0x0504030201000706);
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
    __m256i vnonce;
    uint32_t *pu32DstCtr = ctx + (tid * CK_THREADSEP) + (phase * 32768UL) + CK_COUNTER1;
    uint32_t *const pu32Bitmap = ctx + (tid * CK_THREADSEP) + CK_BITMAP;
    uint32_t u32SrcBucketIterations = PhaseLookup1[phase*2UL];
    int32_t i32SrcBucket = PhaseLookup1[phase*2UL+1] + (tid * u32SrcBucketIterations);
    uint32_t rel32ReadStart = CK_BUCKET0 - CK_COUNTER0 + CK_BUCKETSIZE0 + (i32SrcBucket * CK_BUCKETSIZE0);
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER0;
    uint32_t u32ItemMaskOr = i32SrcBucket << 18;
    uint32_t pu32ReadEndBuf[THREADS];
    uint32_t u32Bucket, u32ReadVal, u32Remaining, rel32Write;
    uint64_t dummy1, dummy2;

    uint32_t rel32CtrInit = InitDest1[phase];
    uint32_t u32CtrInitInc = InitDestInc1[phase];
    for (int i=0; i<8192; i++) {
        pu32DstCtr[i] = rel32CtrInit;
        rel32CtrInit += u32CtrInitInc;
    }

    do {
        //memset(pu32Bitmap, 0, 32768);
        asm volatile ("rep stosq" : "=D" (dummy1), "=c" (dummy2) : "D" (pu32Bitmap), "a" (0), "c" (4096) );

        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            uint32_t u32tmp = pu32SrcCtr[i32SrcBucket] + 3;
            u32tmp = (u32tmp > rel32ReadStart) ? u32tmp : rel32ReadStart;
            void *pRead = (void *) pu32SrcCtr + u32tmp;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[i32SrcBucket+1];
            do {
                u32ReadVal = *(uint32_t *) pRead;
                pRead += 5;
                pu32Bitmap[u32ReadVal & 0x1fff] |= 1 << ((u32ReadVal >> 13) & 0x1f);
            } while (pRead < pReadEnd);

            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        rel32ReadStart -= CK_BUCKETSIZE0;
        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            pu32SrcCtr = (void *) pu32SrcCtr - CK_THREADSEP;
            uint32_t u32tmp = pu32SrcCtr[i32SrcBucket-1] + 3;
            u32tmp = (u32tmp > rel32ReadStart) ? u32tmp : rel32ReadStart;
            void *pRead = (void *) pu32SrcCtr + u32tmp;
            void *pWrite = pRead;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[i32SrcBucket];
            do {
                uint64_t u64ReadVal = *(uint64_t *) pRead;
                pRead += 5;
                *(uint64_t *) pWrite = u64ReadVal;
                u32tmp = (pu32Bitmap[u64ReadVal & 0x1fff] >> ((u64ReadVal >> 13) & 0x1f)) & 1;
                pWrite += (u32tmp * 5UL);
            } while (pRead < pReadEnd);
            pu32ReadEndBuf[fromthread] = pWrite - (void *) pu32SrcCtr;
        }
        vnonce = _mm256_set_epi64x(0, 0, 0, 0);
        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            uint32_t u32tmp = pu32SrcCtr[i32SrcBucket-1] + 3;
            u32tmp = (u32tmp > rel32ReadStart) ? u32tmp : rel32ReadStart;
            void *pRead = (void *) pu32SrcCtr + u32tmp;
            void *pReadEnd = (void *) pu32SrcCtr + pu32ReadEndBuf[THREADS-1-fromthread];
            vnonce = _mm256_broadcastq_epi64(_mm256_extracti128_si256(vnonce, 0));
            goto _EnterDistributeSide0;
            do {
                u32Bucket = _mm256_extract_epi32(v2, 0);
                rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = _mm256_extract_epi64(v1, 0);
                pu32DstCtr[u32Bucket] = rel32Write + 5;
                u32ReadVal = _mm256_extract_epi32(v0, 0);
                pu32Bitmap[u32ReadVal & 0x1fff] &= ~(1 << ((u32ReadVal >> 13) & 0x1f));
                u32Bucket = _mm256_extract_epi32(v2, 2);
                rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = _mm256_extract_epi64(v1, 1);
                pu32DstCtr[u32Bucket] = rel32Write + 5;
                u32ReadVal = _mm256_extract_epi32(v0, 2);
                pu32Bitmap[u32ReadVal & 0x1fff] &= ~(1 << ((u32ReadVal >> 13) & 0x1f));
                u32Bucket = _mm256_extract_epi32(v2, 4);
                rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = _mm256_extract_epi64(v1, 2);
                pu32DstCtr[u32Bucket] = rel32Write + 5;
                u32ReadVal = _mm256_extract_epi32(v0, 4);
                pu32Bitmap[u32ReadVal & 0x1fff] &= ~(1 << ((u32ReadVal >> 13) & 0x1f));
                u32Bucket = _mm256_extract_epi32(v2, 6);
                rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = _mm256_extract_epi64(v1, 3);
                pu32DstCtr[u32Bucket] = rel32Write + 5;
                u32ReadVal = _mm256_extract_epi32(v0, 6);
                pu32Bitmap[u32ReadVal & 0x1fff] &= ~(1 << ((u32ReadVal >> 13) & 0x1f));
_EnterDistributeSide0:
                ;
                __m128i x0 = _mm_set_epi64x(*(uint64_t *) (pRead+12), *(uint64_t *) (pRead+2));
                __m128i x2 = _mm_slli_epi64(x0, 8);
                x0 = _mm_blend_epi32(x0, x2, 0x5);
                x0 = _mm_srli_epi64(x0, 10);
//printf("%x %x %x %x\n", _mm_extract_epi32(x0, 0), _mm_extract_epi32(x0, 1), _mm_extract_epi32(x0, 2), _mm_extract_epi32(x0, 3));
//return;
                v0 = _mm256_cvtepu32_epi64(x0);
//printf("%llx %llx %llx %llx\n", _mm256_extract_epi64(v0, 0), _mm256_extract_epi64(v0, 1), _mm256_extract_epi64(v0, 2), _mm256_extract_epi64(v0, 3));
//return;
                v0 = _mm256_sub_epi64(v0, vnonce);
                v0 = _mm256_and_si256(v0, yconstedgemask);
                vnonce = _mm256_add_epi64(vnonce, v0);
//printf("%llx %llx %llx %llx\n", _mm256_extract_epi64(vnonce, 0), _mm256_extract_epi64(vnonce, 1), _mm256_extract_epi64(vnonce, 2), _mm256_extract_epi64(vnonce, 3));
//return;

                SIPBEGIN
                SIPROUND
                v0 = _mm256_xor_si256(vnonce, v0);
                v2 = _mm256_xor_si256(yconstff, v2);
                SIPROUND
                SIPROUND
                SIPROUND
                SIPEND
                x0 = _mm_set_epi64x(*(uint64_t *) (pRead+10), *(uint64_t *) pRead);
                x2 = _mm_srli_epi64(x0, 8);
                x0 = _mm_blend_epi32(x2, x0, 0x5);
                v0 = _mm256_cvtepu32_epi64(x0);
                v2 = _mm256_set_epi64x(u32ItemMaskOr, u32ItemMaskOr, u32ItemMaskOr, u32ItemMaskOr);
                v0 = _mm256_and_si256(yconstbitmapmask, v0);
                v3 = _mm256_or_si256(v0, v2);
                v3 = _mm256_slli_epi64(v3, 18);
                v2 = _mm256_and_si256(yconstbucketmask, v1);
                v1 = _mm256_srli_epi64(v1, 13);
                v1 = _mm256_and_si256(yconstbitmapmask, v1);
                v1 = _mm256_or_si256(v1, v3);
                pRead += 20;
            } while (pRead < pReadEnd);
            u32Remaining = pReadEnd + 20 - pRead;
            u32Bucket = _mm256_extract_epi32(v2, 0);
            rel32Write = pu32DstCtr[u32Bucket];
            *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = _mm256_extract_epi64(v1, 0);
            pu32DstCtr[u32Bucket] = rel32Write + 5;
            u32ReadVal = _mm256_extract_epi32(v0, 0);
            pu32Bitmap[u32ReadVal & 0x1fff] &= ~(1 << ((u32ReadVal >> 13) & 0x1f));
            u32Remaining -= 5;
            if (!u32Remaining) goto _DoneDistributeSide0;
            u32Bucket = _mm256_extract_epi32(v2, 2);
            rel32Write = pu32DstCtr[u32Bucket];
            *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = _mm256_extract_epi64(v1, 1);
            pu32DstCtr[u32Bucket] = rel32Write + 5;
            u32ReadVal = _mm256_extract_epi32(v0, 2);
            pu32Bitmap[u32ReadVal & 0x1fff] &= ~(1 << ((u32ReadVal >> 13) & 0x1f));
            u32Remaining -= 5;
            if (!u32Remaining) goto _DoneDistributeSide0;
            u32Bucket = _mm256_extract_epi32(v2, 4);
            rel32Write = pu32DstCtr[u32Bucket];
            *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = _mm256_extract_epi64(v1, 2);
            pu32DstCtr[u32Bucket] = rel32Write + 5;
            u32ReadVal = _mm256_extract_epi32(v0, 4);
            pu32Bitmap[u32ReadVal & 0x1fff] &= ~(1 << ((u32ReadVal >> 13) & 0x1f));
            u32Remaining -= 5;
            if (!u32Remaining) goto _DoneDistributeSide0;
            u32Bucket = _mm256_extract_epi32(v2, 6);
            rel32Write = pu32DstCtr[u32Bucket];
            *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = _mm256_extract_epi64(v1, 3);
            pu32DstCtr[u32Bucket] = rel32Write + 5;
            u32ReadVal = _mm256_extract_epi32(v0, 6);
            pu32Bitmap[u32ReadVal & 0x1fff] &= ~(1 << ((u32ReadVal >> 13) & 0x1f));
_DoneDistributeSide0:
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        if ((_mm256_extract_epi32(vnonce, 0) & 0xffc00001) != 0xffc00000)
            goto _DensityError;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE0;
        u32ItemMaskOr += 0x40000;
        vnonce = _mm256_set_epi64x(0, 0, 0, 0);
        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            uint32_t u32tmp = pu32SrcCtr[i32SrcBucket] + 3;
            u32tmp = (u32tmp > rel32ReadStart) ? u32tmp : rel32ReadStart;
            void *pRead = (void *) pu32SrcCtr + u32tmp;
            void *pWrite = pRead;
            void *pRead2 = pRead;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[i32SrcBucket+1];
            do {
                uint64_t u64ReadVal = *(uint64_t *) pRead;
                pRead += 5;
                *(uint64_t *) pWrite = u64ReadVal;
                u32tmp = (pu32Bitmap[u64ReadVal & 0x1fff] >> ((u64ReadVal >> 13) & 0x1f)) & 1;
                pWrite += ((u32tmp ^ 1) * 5UL);
            } while (pRead < pReadEnd);
            vnonce = _mm256_broadcastq_epi64(_mm256_extracti128_si256(vnonce, 0));
            goto _EnterDistributeSide1;
            do {
                u32Bucket = _mm256_extract_epi32(v2, 0);
                rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = _mm256_extract_epi64(v1, 0);
                pu32DstCtr[u32Bucket] = rel32Write + 5;
                u32Bucket = _mm256_extract_epi32(v2, 2);
                rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = _mm256_extract_epi64(v1, 1);
                pu32DstCtr[u32Bucket] = rel32Write + 5;
                u32Bucket = _mm256_extract_epi32(v2, 4);
                rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = _mm256_extract_epi64(v1, 2);
                pu32DstCtr[u32Bucket] = rel32Write + 5;
                u32Bucket = _mm256_extract_epi32(v2, 6);
                rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = _mm256_extract_epi64(v1, 3);
                pu32DstCtr[u32Bucket] = rel32Write + 5;
_EnterDistributeSide1:
                ;
                __m128i x0 = _mm_set_epi64x(*(uint64_t *) (pRead2+12), *(uint64_t *) (pRead2+2));
                __m128i x2 = _mm_slli_epi64(x0, 8);
                x0 = _mm_blend_epi32(x0, x2, 0x5);
                x0 = _mm_srli_epi64(x0, 10);
                v0 = _mm256_cvtepu32_epi64(x0);
                v0 = _mm256_sub_epi64(v0, vnonce);
                v0 = _mm256_and_si256(v0, yconstedgemask);
                vnonce = _mm256_add_epi64(vnonce, v0);
                SIPBEGIN
                SIPROUND
                v0 = _mm256_xor_si256(vnonce, v0);
                v2 = _mm256_xor_si256(yconstff, v2);
                SIPROUND
                SIPROUND
                SIPROUND
                SIPEND
                x0 = _mm_set_epi64x(*(uint64_t *) (pRead2+10), *(uint64_t *) pRead2);
                x2 = _mm_srli_epi64(x0, 8);
                x0 = _mm_blend_epi32(x2, x0, 0x5);
                v0 = _mm256_cvtepu32_epi64(x0);
                v2 = _mm256_set_epi64x(u32ItemMaskOr, u32ItemMaskOr, u32ItemMaskOr, u32ItemMaskOr);
                v0 = _mm256_and_si256(yconstbitmapmask, v0);
                v3 = _mm256_or_si256(v0, v2);
                v3 = _mm256_slli_epi64(v3, 18);
                v2 = _mm256_and_si256(yconstbucketmask, v1);
                v1 = _mm256_srli_epi64(v1, 13);
                v1 = _mm256_and_si256(yconstbitmapmask, v1);
                v1 = _mm256_or_si256(v1, v3);
                pRead2 += 20;
            } while (pRead2 < pWrite);
            u32Remaining = pWrite + 20 - pRead2;
            u32Bucket = _mm256_extract_epi32(v2, 0);
            rel32Write = pu32DstCtr[u32Bucket];
            *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = _mm256_extract_epi64(v1, 0);
            pu32DstCtr[u32Bucket] = rel32Write + 5;
            u32Remaining -= 5;
            if (!u32Remaining) goto _DoneDistributeSide1;
            u32Bucket = _mm256_extract_epi32(v2, 2);
            rel32Write = pu32DstCtr[u32Bucket];
            *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = _mm256_extract_epi64(v1, 1);
            pu32DstCtr[u32Bucket] = rel32Write + 5;
            u32Remaining -= 5;
            if (!u32Remaining) goto _DoneDistributeSide1;
            u32Bucket = _mm256_extract_epi32(v2, 4);
            rel32Write = pu32DstCtr[u32Bucket];
            *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = _mm256_extract_epi64(v1, 2);
            pu32DstCtr[u32Bucket] = rel32Write + 5;
            u32Remaining -= 5;
            if (!u32Remaining) goto _DoneDistributeSide1;
            u32Bucket = _mm256_extract_epi32(v2, 6);
            rel32Write = pu32DstCtr[u32Bucket];
            *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = _mm256_extract_epi64(v1, 3);
            pu32DstCtr[u32Bucket] = rel32Write + 5;
_DoneDistributeSide1:
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        if ((_mm256_extract_epi32(vnonce, 0) & 0xffc00001) != 0xffc00000)
            goto _DensityError;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        i32SrcBucket += 2;
        rel32ReadStart += CK_BUCKETSIZE0 * 2;
        u32ItemMaskOr += 0x40000;
        u32SrcBucketIterations -= 2;
    } while (u32SrcBucketIterations);

    return;

_DensityError:
    printf("Density Error %u %x\n", i32SrcBucket, _mm256_extract_epi32(vnonce, 0));
    return;
}
