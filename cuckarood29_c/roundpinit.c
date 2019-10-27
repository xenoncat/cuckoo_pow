#include <stdint.h>
#include <stdio.h>
//#include <string.h>
#include <x86intrin.h>
#include "memlayout.h"

void roundpinit(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 32768);
    uint32_t *const pu32DstCtr = ctx + (tid * CK_THREADSEP) + CK_COUNTER1;
    uint8_t *pu8Bitmap = ctx + (tid * CK_THREADSEP) + CK_BITMAP;
    uint32_t const u32InitWordCount = *(uint32_t *) (ctx + (tid * CK_THREADSEP) + CK_RENMAX1 + 4);
    uint32_t u32SrcBucket = tid * (128/THREADS);
    uint32_t rel32ReadStart = CK_BUCKET40 - CK_COUNTER0 + u32SrcBucket * CK_BUCKETSIZE40;
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER0;
    uint64_t dummy1, dummy2;

    uint32_t rel32Write = CK_BUCKET41 - CK_COUNTER1;
    for (int i=0; i<128; i++) {
        pu32DstCtr[i] = rel32Write;
        rel32Write += CK_BUCKETSIZE41;
    }

    do {
        asm volatile ("rep stosq" :"=D" (dummy1), "=c" (dummy2) : "D" (pu8Bitmap), "a" (0), "c" (u32InitWordCount) );

        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            uint64_t *pu64Read = (void *) pu32SrcCtr + rel32ReadStart;
            uint64_t *pu64ReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket];
            do {
                uint64_t u64ReadVal = *(pu64Read++);
                uint32_t u32Bucket = (u64ReadVal >> 32) & 0x7f;
                rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = u64ReadVal & 0x7fffffffffffffffLL;
                pu32DstCtr[u32Bucket] = rel32Write + 8;
                pu8Bitmap[(uint32_t) u64ReadVal >> 7] = 0x01;
            } while (pu64Read < pu64ReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        u32SrcBucket += 1;
        pu8Bitmap += 0x8000;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE40;
    } while (u32SrcBucket & (128/THREADS-1));
}
