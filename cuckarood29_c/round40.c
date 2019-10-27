#include <stdio.h>
#include <stdint.h>
//#include <string.h>
#include <x86intrin.h>
#include "memlayout.h"

//Bitmap 32768 bytes
//Dest index 65536 bytes
//Temp edges 262144 bytes

void round40(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 32768);
    uint32_t *pu32DstCtr = ctx + (tid * CK_THREADSEP) + CK_COUNTER0;
    uint8_t *const pu8Bitmap = ctx + (tid * CK_THREADSEP) + CK_BITMAP;
    uint32_t u32SrcBucket = tid * (256/THREADS);
    uint32_t rel32ReadStart = CK_BUCKET15 - CK_COUNTER1 + u32SrcBucket * CK_BUCKETSIZE15;
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER1;
    uint16_t *const pu16DestIdx = ctx + (tid * CK_THREADSEP) + CK_BITMAP + 32766;
    uint32_t u32Bucket;
    uint64_t dummy1, dummy2;

    __m128i const xconst00 = _mm_set_epi32(0, 0, 0, 0);
    __m128i const xconst0f = _mm_set_epi32(0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f, 0x0f0f0f0f);

    uint32_t rel32CtrInit = CK_BUCKET40 - CK_COUNTER0;
    for (int i=0; i<128; i++) {
        pu32DstCtr[i] = rel32CtrInit;
        rel32CtrInit += CK_BUCKETSIZE40;
    }
    uint32_t u32InitWordCount = *(uint32_t *) (ctx + (tid * CK_THREADSEP) + CK_RENMAX0 + 4);
    u32InitWordCount = (u32InitWordCount + 1) & -2;

    do {
        asm volatile ("rep stosq" : "=D" (dummy1), "=c" (dummy2) : "D" (pu8Bitmap), "a" (0), "c" (u32InitWordCount) );
        for (uint32_t fromthread=0; fromthread<THREADS; fromthread++) {
            uint32_t *pu32Read = (void *) pu32SrcCtr + rel32ReadStart;
            uint32_t *pu32ReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket];
            do {
                uint32_t u32ReadVal = *(pu32Read++);
                pu8Bitmap[(u32ReadVal >> 15) & 0x7fff] += 0x01;
            } while (pu32Read < pu32ReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE15;
        for (uint32_t fromthread=0; fromthread<THREADS; fromthread++) {
            uint32_t *pu32Read = (void *) pu32SrcCtr + rel32ReadStart;
            uint32_t *pu32ReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket+1];
            do {
                uint32_t u32ReadVal = *(pu32Read++);
                pu8Bitmap[(u32ReadVal >> 15) & 0x7fff] += 0x10;
            } while (pu32Read < pu32ReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }

        __m128i *p128Src = (void *) pu8Bitmap;
        __m256i *p256Dst = (void *) pu8Bitmap + 32768;
        __m128i xmm0, xmm1, xmm2, xmm3, xmm5;
        __m256i ymm0, ymm2, ymm4;
        *(uint32_t *) (pu8Bitmap + 32764) = 0;
        xmm5 = _mm_set_epi32(0,0,0,0);
        int i = u32InitWordCount;
        do {
            xmm0 = *p128Src;
            xmm1 = _mm_srli_epi16(xmm0, 4);
            xmm0 = _mm_and_si128(xmm0, xconst0f);
            xmm1 = _mm_and_si128(xmm1, xconst0f);
            xmm2 = _mm_cmpeq_epi8(xmm0, xconst00);
            xmm3 = _mm_cmpeq_epi8(xmm1, xconst00);
            xmm2 = _mm_or_si128(xmm2, xmm3);
            xmm0 = _mm_add_epi8(xmm0, xmm1);
            xmm0 = _mm_andnot_si128(xmm2, xmm0);
            xmm1 = _mm_slli_si128(xmm0, 1);
            xmm0 = _mm_add_epi8(xmm0, xmm1);
            xmm1 = _mm_slli_si128(xmm0, 2);
            xmm0 = _mm_add_epi8(xmm0, xmm1);
            xmm1 = _mm_slli_si128(xmm0, 4);
            xmm0 = _mm_add_epi8(xmm0, xmm1);
            xmm1 = _mm_slli_si128(xmm0, 8);
            xmm0 = _mm_add_epi8(xmm0, xmm1);
            ymm0 = _mm256_cvtepu8_epi16(xmm0);
            ymm4 = _mm256_broadcastw_epi16(xmm5);
            ymm0 = _mm256_add_epi16(ymm0, ymm4);
            int8_t i8tmp = _mm_cvtsi128_si32(xmm2);
            xmm2 = _mm_srli_si128(xmm2, 1);
            ymm2 = _mm256_cvtepi8_epi16(xmm2);
            *((int16_t *) p256Dst - 1) |= (int16_t) i8tmp;
            xmm5 = _mm256_extracti128_si256(ymm0, 1);
            ymm0 = _mm256_or_si256(ymm0, ymm2);
            *p256Dst = ymm0;
            xmm5 = _mm_srli_si128(xmm5, 14);
            p128Src++;
            p256Dst++;
        } while (i-=2);

        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        u32Bucket = 0;
        for (uint32_t fromthread=0; fromthread<THREADS; fromthread++) {
            uint32_t *pu32Read = (void *) pu32SrcCtr + rel32ReadStart;
            uint32_t *pu32ReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket+1];
            do {
                uint32_t u32ReadVal = *(pu32Read++);
                uint32_t u32tmp = pu16DestIdx[(u32ReadVal >> 15) & 0x7fff];
                u32tmp += (u32tmp != 0xffff) ? 1 : 0;
                pu16DestIdx[(u32ReadVal >> 15) & 0x7fff] = u32tmp;
                u32Bucket += (((u32ReadVal >> 30) - u32Bucket) & 0x3);
                uint32_t u32WriteVal = ((u32ReadVal & 0x7fff) << 7) | u32Bucket;
                *(uint32_t *) ((void *) pu16DestIdx + (u32tmp*4UL) + 65534) = u32WriteVal;
            } while (pu32Read < pu32ReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        if ((u32Bucket & -4) != 0x7c)
            goto _DensityError;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart -= CK_BUCKETSIZE15;
        u32Bucket = 0;
        for (uint32_t fromthread=0; fromthread<THREADS; fromthread++) {
            uint32_t *pu32Read = (void *) pu32SrcCtr + rel32ReadStart;
            uint32_t *pu32ReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket];
            do {
                uint32_t u32ReadVal = *(pu32Read++);
                uint32_t u32tmp = pu16DestIdx[(u32ReadVal >> 15) & 0x7fff];
                u32tmp += (u32tmp != 0xffff) ? 1 : 0;
                pu16DestIdx[(u32ReadVal >> 15) & 0x7fff] = u32tmp;
                u32Bucket += (((u32ReadVal >> 30) - u32Bucket) & 0x3);
                uint32_t u32WriteVal = 0x80000000 | ((u32ReadVal & 0x7fff) << 7) | u32Bucket;
                *(uint32_t *) ((void *) pu16DestIdx + (u32tmp*4UL) + 65534) = u32WriteVal;
            } while (pu32Read < pu32ReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        if ((u32Bucket & -4) != 0x7c)
            goto _DensityError;

        uint32_t *pu32Read = (void *) pu16DestIdx + 65538;
        uint32_t *pu32ReadEnd = pu32Read + _mm_extract_epi16(xmm5, 0);
        *pu32ReadEnd = 0;
        do {
            uint32_t u32ReadVal;
            int64_t i64tmp = *(int64_t *) pu32Read;
            pu32Read += 2;
            if (i64tmp < 0) {
                u32Bucket = i64tmp & 0x7f;
                uint32_t rel32Write = pu32DstCtr[u32Bucket];
                *(int64_t *) ((void *) pu32DstCtr + rel32Write) = i64tmp;
                rel32Write += 8;
                pu32DstCtr[u32Bucket] = rel32Write;
                if (pu32Read >= pu32ReadEnd)
                    break;
                u32ReadVal = *pu32Read;
                if (!(u32ReadVal & 0x80000000))
                    continue;
                do {
                    pu32Read++;
                    *(int64_t *) ((void *) pu32DstCtr + rel32Write) = ((int64_t) u32ReadVal << 32) | (uint32_t) i64tmp;
                    rel32Write += 8;
                    u32ReadVal = *pu32Read;
                } while (u32ReadVal & 0x80000000);
                pu32DstCtr[u32Bucket] = rel32Write;
                if (pu32Read < pu32ReadEnd)
                    continue;
                break;
            }
            else {
                uint32_t *pu32Read2 = pu32Read - 2;
                do {
                    u32ReadVal = *(pu32Read++);
                } while (!(u32ReadVal & 0x80000000));
                uint32_t *pu32Read3 = pu32Read - 1;
                do {
                    uint32_t u32tmp = *(pu32Read2++);
                    u32Bucket = u32tmp & 0x7f;
                    uint32_t rel32Write = pu32DstCtr[u32Bucket];
                    pu32Read = pu32Read3;
                    u32ReadVal = *pu32Read;
                    do {
                        pu32Read++;
                        *(int64_t *) ((void *) pu32DstCtr + rel32Write) = ((int64_t) u32ReadVal << 32) | u32tmp;
                        rel32Write += 8;
                        u32ReadVal = *pu32Read;
                    } while (u32ReadVal & 0x80000000);
                    pu32DstCtr[u32Bucket] = rel32Write;
                } while (pu32Read2 < pu32Read3);
                if (pu32Read < pu32ReadEnd)
                    continue;
                break;
            }
            break;
        } while (1);

        u32SrcBucket += 2;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE15 * 2;
    } while (u32SrcBucket & (256/THREADS-2));

    return;

_DensityError:
    printf("Density Error %u %x\n", u32SrcBucket, u32Bucket);
    return;
}
