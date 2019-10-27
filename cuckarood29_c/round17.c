#include <stdio.h>
#include <stdint.h>
//#include <string.h>
#include <x86intrin.h>
#include "memlayout.h"

void round17(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 32768);
    uint32_t *pu32DstCtr = ctx + (tid * CK_THREADSEP) + CK_COUNTER1;
    uint8_t *const pu8Bitmap = ctx + (tid * CK_THREADSEP) + CK_BITMAP;
    uint32_t const u32InitWordCount = *(uint32_t *) (ctx + (tid * CK_THREADSEP) + CK_RENMAX1 + 4);
    uint32_t u32SrcBucket = tid * (256/THREADS);
    uint32_t rel32ReadStart = CK_BUCKET16 - CK_COUNTER0 + u32SrcBucket * CK_BUCKETSIZE16;
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER0;
    uint32_t u32Bucket;
    uint32_t u32ItemMaskOr = 0;
    uint64_t dummy1, dummy2;

    uint32_t rel32CtrInit = CK_BUCKET15 - CK_COUNTER1;
    for (int i=0; i<256; i++) {
        pu32DstCtr[i] = rel32CtrInit;
        rel32CtrInit += CK_BUCKETSIZE15;
    }

    do {
        asm volatile ("rep stosq" : "=D" (dummy1), "=c" (dummy2) : "D" (pu8Bitmap), "a" (0), "c" (u32InitWordCount) );
        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            uint32_t *pu32Read = (void *) pu32SrcCtr + rel32ReadStart;
            uint32_t *pu32ReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket];
            do {
                uint32_t u32ReadVal = *(pu32Read++);
                pu8Bitmap[u32ReadVal & 0x7fff] = 0x01;
            } while (pu32Read < pu32ReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE16;
        u32Bucket = 0;
        pu32DstCtr += 1;
        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            uint32_t *pu32Read = (void *) pu32SrcCtr + rel32ReadStart;
            uint32_t *pu32ReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket+1];
            do {
                uint32_t u32ReadVal = *(pu32Read++);
                uint8_t u8tmp = pu8Bitmap[u32ReadVal & 0x7fff];
                u8tmp |= 0x02;
                pu8Bitmap[u32ReadVal & 0x7fff] = (uint8_t) u8tmp;
                u32Bucket += (((u32ReadVal >> 29) - u32Bucket) & 0x6);
                uint32_t rel32Write = pu32DstCtr[u32Bucket];
                *(uint32_t *) ((void *) pu32DstCtr + rel32Write - 4) = (u32ReadVal & 0x3fffffff) | u32ItemMaskOr;
                pu32DstCtr[u32Bucket] = rel32Write + ((u8tmp & 1) * 4UL);
            } while (pu32Read < pu32ReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        if ((u32Bucket & -8) != 0xf8)
            goto _DensityError;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart -= CK_BUCKETSIZE16;
        u32Bucket = 0;
        pu32DstCtr -= 1;
        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            uint32_t *pu32Read = (void *) pu32SrcCtr + rel32ReadStart;
            uint32_t *pu32ReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket];
            do {
                uint32_t u32ReadVal = *(pu32Read++);
                uint8_t u8tmp = pu8Bitmap[u32ReadVal & 0x7fff];
                u32Bucket += (((u32ReadVal >> 29) - u32Bucket) & 0x6);
                uint32_t rel32Write = pu32DstCtr[u32Bucket];
                *(uint32_t *) ((void *) pu32DstCtr + rel32Write) = (u32ReadVal & 0x3fffffff) | u32ItemMaskOr;
                pu32DstCtr[u32Bucket] = rel32Write + ((u8tmp & 2) * 2UL);
            } while (pu32Read < pu32ReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        if ((u32Bucket & -8) != 0xf8)
            goto _DensityError;
        u32SrcBucket += 2;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE16 * 2;
        u32ItemMaskOr += 0x40000000;
    } while (u32SrcBucket & (256/THREADS-2));

    return;

_DensityError:
    printf("Density Error %u %x\n", u32SrcBucket, u32Bucket);
    return;
}
