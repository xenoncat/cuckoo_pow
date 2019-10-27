#include <stdio.h>
#include <stdint.h>
//#include <string.h>
#include <x86intrin.h>
#include "memlayout.h"

void round4(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 8192);
    uint32_t *pu32DstCtr = ctx + (tid * CK_THREADSEP) + CK_COUNTER0;
    uint32_t *pu32Bitmap = ctx + (tid * CK_THREADSEP) + CK_BITMAP;
    int32_t i32SrcBucket = tid * (2048/THREADS);
    uint32_t rel32ReadStart = CK_BUCKET3 - CK_COUNTER1 + i32SrcBucket * CK_BUCKETSIZE3;
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER1;
    uint32_t u32Bucket;
    uint64_t const u64ItemMaskAnd = 0xfffffffff;
    uint64_t u64ItemMaskOr = 0;
    uint64_t dummy1, dummy2;

    uint32_t rel32CtrInit = CK_BUCKET2 - CK_COUNTER0;
    for (int i=0; i<2048; i++) {
        pu32DstCtr[i] = rel32CtrInit;
        rel32CtrInit += CK_BUCKETSIZE2;
    }

    do {
        asm volatile ("rep stosq" : "=D" (dummy1), "=c" (dummy2) : "D" (pu32Bitmap), "a" (0), "c" (4096) );

        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            void *pRead = (void *) pu32SrcCtr + rel32ReadStart + 2;    //no interesting data on first 2 bytes of a 5 byte slot
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[i32SrcBucket];
            do {
                uint32_t u32ReadVal = *(uint32_t *) pRead;
                pRead += 5;
                *(uint32_t *) ((void *) pu32Bitmap + (u32ReadVal & 0x7ffc)) |= 1 << ((u32ReadVal >> 15) & 0x1f);
            } while (pRead < pReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        pu32Bitmap += 0x2000;
        asm volatile ("rep stosq" : "=D" (dummy1), "=c" (dummy2) : "D" (pu32Bitmap), "a" (0), "c" (4096) );

        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE3;
        u32Bucket = 0;
        pu32DstCtr += 1;
        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            void *pRead = (void *) pu32SrcCtr + rel32ReadStart;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[i32SrcBucket+1];
            do {
                uint64_t u64ReadVal = *(uint64_t *) pRead;
                pRead += 5;
                uint32_t u32tmp = (pu32Bitmap[((u64ReadVal >> 18) & 0x1fff) - 0x2000] >> ((u64ReadVal >> 31) & 0x1f)) & 1;
                pu32Bitmap[((u64ReadVal >> 18) & 0x1fff)] |= (1 << ((u64ReadVal >> 31) & 0x1f));
                u32Bucket += (((u64ReadVal >> 35) - u32Bucket) & 0x1e);
                uint32_t rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write - 4) = (u64ReadVal & u64ItemMaskAnd) | u64ItemMaskOr;
                pu32DstCtr[u32Bucket] = rel32Write + (u32tmp * 5);
            } while (pRead < pReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        if ((u32Bucket & -32) != 0x7e0)
            goto _DensityError;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart -= CK_BUCKETSIZE3;
        u32Bucket = 0;
        pu32DstCtr -= 1;
        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            void *pRead = (void *) pu32SrcCtr + rel32ReadStart;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[i32SrcBucket];
            do {
                uint64_t u64ReadVal = *(uint64_t *) pRead;
                pRead += 5;
                uint32_t u32tmp = (pu32Bitmap[((u64ReadVal >> 18) & 0x1fff)] >> ((u64ReadVal >> 31) & 0x1f)) & 1;
                u32Bucket += (((u64ReadVal >> 35) - u32Bucket) & 0x1e);
                uint32_t rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = (u64ReadVal & u64ItemMaskAnd) | u64ItemMaskOr;
                pu32DstCtr[u32Bucket] = rel32Write + (u32tmp * 5);
            } while (pRead < pReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        if ((u32Bucket & -32) != 0x7e0)
            goto _DensityError;
        i32SrcBucket += 2;
        pu32Bitmap -= 0x2000;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE3 * 2;
        u64ItemMaskOr += 0x1000000000;
    } while (i32SrcBucket & (2048/THREADS-2));

    return;

_DensityError:
    printf("Density Error %u %x\n", i32SrcBucket, u32Bucket);
    return;
}
