#include <stdio.h>
#include <stdint.h>
//#include <string.h>
#include <x86intrin.h>
#include "memlayout.h"

void round2(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 8192);
    uint32_t *pu32DstCtr = ctx + (tid * CK_THREADSEP) + CK_COUNTER0;
    uint32_t *const pu32Bitmap = ctx + (tid * CK_THREADSEP) + CK_BITMAP;
    int32_t i32SrcBucket = tid * (2048/THREADS);
    uint32_t rel32ReadStart = CK_BUCKET1A - CK_COUNTER1 + i32SrcBucket * CK_BUCKETSIZE1;
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER1;
    uint32_t u32Bucket;
    uint64_t const u64ItemMaskAnd = 0xfffffffff;
    uint64_t u64ItemMaskOr = 0;
    uint64_t dummy1, dummy2;
    uint32_t pu32ReadEndBuf[THREADS*2];

    uint32_t rel32CtrInit = CK_BUCKET2 - CK_COUNTER0;
    for (int i=0; i<2048; i++) {
        pu32DstCtr[i] = rel32CtrInit;
        rel32CtrInit += CK_BUCKETSIZE2;
    }

    do {
        asm volatile ("rep stosq" : "=D" (dummy1), "=c" (dummy2) : "D" (pu32Bitmap), "a" (0), "c" (4096) );
        int fromthread = THREADS*2;

        do {
            do {
                uint32_t u32tmp = pu32SrcCtr[i32SrcBucket-1] + 8;
                u32tmp = (u32tmp > rel32ReadStart) ? u32tmp : rel32ReadStart;
                void *pRead = (void *) pu32SrcCtr + u32tmp + 2;    //no interesting data on first 2 bytes of a 5 byte slot
                void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[i32SrcBucket];
                do {
                    uint32_t u32ReadVal = *(uint32_t *) pRead;
                    pRead += 5;
                    *(uint32_t *) ((void *) pu32Bitmap + (u32ReadVal & 0x7ffc)) |= 1 << ((u32ReadVal >> 15) & 0x1f);
                } while (pRead < pReadEnd);
                pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
            } while (--fromthread & (THREADS-1));
            pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP) + 8192;
            rel32ReadStart += (CK_BUCKET1B-CK_BUCKET1A-8192);
        } while (fromthread);
        rel32ReadStart += CK_BUCKETSIZE1;
        fromthread = THREADS*2;
        do {
            pu32SrcCtr = (void *) pu32SrcCtr + (THREADS * CK_THREADSEP) - 8192;
            rel32ReadStart -= (CK_BUCKET1B-CK_BUCKET1A-8192);
            do {
                pu32SrcCtr = (void *) pu32SrcCtr - CK_THREADSEP;
                uint32_t u32tmp = pu32SrcCtr[i32SrcBucket] + 8;
                u32tmp = (u32tmp > rel32ReadStart) ? u32tmp : rel32ReadStart;
                void *pRead = (void *) pu32SrcCtr + u32tmp;
                void *pWrite = pRead;
                void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[i32SrcBucket+1];
                do {
                    uint64_t u64ReadVal = *(uint64_t *) pRead;
                    pRead += 5;
                    *(uint64_t *) pWrite = u64ReadVal;
                    u64ReadVal >>= 18;
                    u32tmp = (pu32Bitmap[u64ReadVal & 0x1fff] >> ((u64ReadVal >> 13) & 0x1f)) & 1;
                    pWrite += (u32tmp * 5UL);
                } while (pRead < pReadEnd);
                pu32ReadEndBuf[fromthread-1] = pWrite - (void *) pu32SrcCtr;
            } while (--fromthread & (THREADS-1));
        } while (fromthread);

        pu32DstCtr += 1;
        u32Bucket = 0;
        fromthread = THREADS*2;
        do {
            do {
                uint32_t u32tmp = pu32SrcCtr[i32SrcBucket] + 8;
                u32tmp = (u32tmp > rel32ReadStart) ? u32tmp : rel32ReadStart;
                void *pRead = (void *) pu32SrcCtr + u32tmp;
                void *pReadEnd = (void *) pu32SrcCtr + pu32ReadEndBuf[THREADS*2-fromthread];
                do {
                    uint64_t u64ReadVal = *(uint64_t *) pRead;
                    pRead += 5;
                    u32Bucket += (((u64ReadVal >> 35) - u32Bucket) & 0x1e);
                    uint32_t rel32Write = pu32DstCtr[u32Bucket];
                    pu32DstCtr[u32Bucket] = rel32Write + 5;
                    *(uint64_t *) ((void *) pu32DstCtr + rel32Write - 4) = (u64ReadVal & u64ItemMaskAnd) | u64ItemMaskOr;
                    u64ReadVal >>= 18;
                    pu32Bitmap[u64ReadVal & 0x1fff] &= ~(1 << ((u64ReadVal >> 13) & 0x1f));
                } while (pRead < pReadEnd);
                pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
            } while (--fromthread & (THREADS-1));
            pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP) + 8192;
            rel32ReadStart += (CK_BUCKET1B-CK_BUCKET1A-8192);
        } while (fromthread);
        if ((u32Bucket & -32) != 0x7e0)
            goto _DensityError;

        pu32DstCtr -= 1;
        pu32SrcCtr = (void *) pu32SrcCtr - 16384;
        rel32ReadStart -= 2*(CK_BUCKET1B-CK_BUCKET1A-8192) + CK_BUCKETSIZE1;
        u32Bucket = 0;
        fromthread = THREADS*2;
        do {
            do {
                uint32_t u32tmp = pu32SrcCtr[i32SrcBucket-1] + 8;
                u32tmp = (u32tmp > rel32ReadStart) ? u32tmp : rel32ReadStart;
                void *pRead = (void *) pu32SrcCtr + u32tmp;
                void *pWrite = pRead;
                void *pRead2 = pRead;
                void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[i32SrcBucket];
                do {
                    uint64_t u64ReadVal = *(uint64_t *) pRead;
                    pRead += 5;
                    *(uint64_t *) pWrite = u64ReadVal;
                    u64ReadVal >>= 18;
                    u32tmp = (pu32Bitmap[u64ReadVal & 0x1fff] >> ((u64ReadVal >> 13) & 0x1f)) & 1;
                    pWrite += ((u32tmp^1) * 5UL);
                } while (pRead < pReadEnd);
                do {
                    uint64_t u64ReadVal = *(uint64_t *) pRead2;
                    pRead2 += 5;
                    u32Bucket += (((u64ReadVal >> 35) - u32Bucket) & 0x1e);
                    uint32_t rel32Write = pu32DstCtr[u32Bucket];
                    pu32DstCtr[u32Bucket] = rel32Write + 5;
                    *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = (u64ReadVal & u64ItemMaskAnd) | u64ItemMaskOr;
                } while (pRead2 < pWrite);
                pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
            } while (--fromthread & (THREADS-1));
            pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP) + 8192;
            rel32ReadStart += (CK_BUCKET1B-CK_BUCKET1A-8192);
        } while (fromthread);
        if ((u32Bucket & -32) != 0x7e0)
            goto _DensityError;

        i32SrcBucket += 2;
        pu32SrcCtr = (void *) pu32SrcCtr - 16384;
        rel32ReadStart -= (2*(CK_BUCKET1B-CK_BUCKET1A-8192)) - 2*CK_BUCKETSIZE1;
        u64ItemMaskOr += 0x1000000000;

    } while (i32SrcBucket & (2048/THREADS-2));

    return;

_DensityError:
    printf("Density Error %u %x\n", i32SrcBucket, u32Bucket);
    return;
}
