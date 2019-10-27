#include <stdint.h>
//#include <string.h>
#include <x86intrin.h>
#include "memlayout.h"

void round18(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 8192);
    uint32_t *pu32DstCtr = ctx + (tid * CK_THREADSEP) + CK_COUNTER0;
    uint32_t *pu32Bitmap = ctx + (tid * CK_THREADSEP) + CK_BITMAP;
    uint32_t const u32InitWordCount = *(uint32_t *) (ctx + (tid * CK_THREADSEP) + CK_RENMAX0 + 4);
    int32_t i32SrcBucket = tid * (256/THREADS);
    uint32_t rel32ReadStart = CK_BUCKET15 - CK_COUNTER1 + CK_BUCKETSIZE15 + i32SrcBucket * CK_BUCKETSIZE15;
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER1;
    uint32_t u32Bucket;
    uint64_t const u64ItemMaskAnd = 0x3ffffffff;
    uint64_t u64ItemMaskOr = 0;
    uint64_t dummy1, dummy2;

    uint32_t rel32CtrInit = CK_BUCKET16 - CK_COUNTER0;
    for (int i=0; i<256; i++) {
        pu32DstCtr[i] = rel32CtrInit;
        rel32CtrInit += CK_BUCKETSIZE16;
    }

    do {
        asm volatile ("rep stosq" : "=D" (dummy1), "=c" (dummy2) : "D" (pu32Bitmap), "a" (0), "c" (u32InitWordCount) );

        for (uint32_t fromthread=0; fromthread<THREADS; fromthread++) {
            void *pRead = (void *) pu32SrcCtr + rel32ReadStart;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[i32SrcBucket+1];
            do {
                uint32_t u32ReadVal = *(uint32_t *) pRead;
                pRead += 5;
                pu32Bitmap[(u32ReadVal >> 5) & 0xfff] |= 1 << (u32ReadVal & 0x1f);
            } while (pRead < pReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        pu32Bitmap += 0x1000;
        asm volatile ("rep stosq" : "=D" (dummy1), "=c" (dummy2) : "D" (pu32Bitmap), "a" (0), "c" (u32InitWordCount) );

        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart -= CK_BUCKETSIZE15;
        for (uint32_t fromthread=0; fromthread<256; fromthread+=(256/THREADS)) {
            void *pRead = (void *) pu32SrcCtr + rel32ReadStart;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[i32SrcBucket];
            do {
                uint64_t u64ReadVal = *(uint64_t *) pRead;
                pRead += 5;
                uint32_t u32tmp = (pu32Bitmap[((u64ReadVal >> 5) & 0xfff) - 0x1000] >> (u64ReadVal & 0x1f)) & 1;
                pu32Bitmap[((u64ReadVal >> 5) & 0xfff)] |= (1 << (u64ReadVal & 0x1f));
                u32Bucket = ((u64ReadVal >> 34) & 0x3f) | fromthread;
                uint32_t rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = (u64ReadVal & u64ItemMaskAnd) | u64ItemMaskOr;
                pu32DstCtr[u32Bucket] = rel32Write + (u32tmp * 5);
            } while (pRead < pReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE15;
        u64ItemMaskOr += 0x400000000;
        for (uint32_t fromthread=0; fromthread<256; fromthread+=(256/THREADS)) {
            void *pRead = (void *) pu32SrcCtr + rel32ReadStart;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[i32SrcBucket+1];
            do {
                uint64_t u64ReadVal = *(uint64_t *) pRead;
                pRead += 5;
                uint32_t u32tmp = (pu32Bitmap[(u64ReadVal >> 5) & 0xfff] >> (u64ReadVal & 0x1f)) & 1;
                u32Bucket = ((u64ReadVal >> 34) & 0x3f) | fromthread;
                uint32_t rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = (u64ReadVal & u64ItemMaskAnd) | u64ItemMaskOr;
                pu32DstCtr[u32Bucket] = rel32Write + (u32tmp * 5);
            } while (pRead < pReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        i32SrcBucket += 2;
        pu32Bitmap -= 0x1000;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE15 * 2;
        u64ItemMaskOr += 0x400000000;
    } while (i32SrcBucket & (256/THREADS-2));
}
