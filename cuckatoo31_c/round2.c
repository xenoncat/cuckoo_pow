#include <stdio.h>
#include <stdint.h>
//#include <string.h>
#include <x86intrin.h>
#include "memlayout.h"

static const uint32_t PhaseLookup2a[] = {
#if R1PHASES>4
    CK_BUCKETSIZE1E, CK_BUCKET1E-(CK_COUNTER1+131072)+CK_BUCKETSIZE1E,
#endif
    CK_BUCKETSIZE1D, CK_BUCKET1D-(CK_COUNTER1+98304)+CK_BUCKETSIZE1D,
    CK_BUCKETSIZE1C, CK_BUCKET1C-(CK_COUNTER1+65536)+CK_BUCKETSIZE1C,
    CK_BUCKETSIZE1B, CK_BUCKET1B-(CK_COUNTER1+32768)+CK_BUCKETSIZE1B,
    CK_BUCKETSIZE1A, CK_BUCKET1A-(CK_COUNTER1)+CK_BUCKETSIZE1A};

static const uint32_t PhaseLookup2b[] = {
    CK_BUCKETSIZE1A, CK_BUCKET1A-(CK_COUNTER1),
    CK_BUCKETSIZE1B, CK_BUCKET1B-(CK_COUNTER1+32768),
    CK_BUCKETSIZE1C, CK_BUCKET1C-(CK_COUNTER1+65536),
    CK_BUCKETSIZE1D, CK_BUCKET1D-(CK_COUNTER1+98304),
#if R1PHASES>4
    CK_BUCKETSIZE1E, CK_BUCKET1E-(CK_COUNTER1+131072)
#endif
};

static const uint32_t PhaseLookup2c[] = {
#if R1PHASES>4
    CK_BUCKETSIZE1E, CK_BUCKET1E-(CK_COUNTER1+131072),
#endif
    CK_BUCKETSIZE1D, CK_BUCKET1D-(CK_COUNTER1+98304),
    CK_BUCKETSIZE1C, CK_BUCKET1C-(CK_COUNTER1+65536),
    CK_BUCKETSIZE1B, CK_BUCKET1B-(CK_COUNTER1+32768),
    CK_BUCKETSIZE1A, CK_BUCKET1A-(CK_COUNTER1)};
static const uint32_t PhaseLookup2d[] = {
#if R1PHASES>4
    CK_BUCKETSIZE1E, CK_BUCKET1E-(CK_COUNTER1+131072)+CK_BUCKETSIZE1E,
#endif
    CK_BUCKETSIZE1D, CK_BUCKET1D-(CK_COUNTER1+98304)+CK_BUCKETSIZE1D,
    CK_BUCKETSIZE1C, CK_BUCKET1C-(CK_COUNTER1+65536)+CK_BUCKETSIZE1C,
    CK_BUCKETSIZE1B, CK_BUCKET1B-(CK_COUNTER1+32768)+CK_BUCKETSIZE1B,
    CK_BUCKETSIZE1A, CK_BUCKET1A-(CK_COUNTER1)+CK_BUCKETSIZE1A};

void round2(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 8192);
    uint32_t *pu32DstCtr = ctx + (tid * CK_THREADSEP) + CK_COUNTER0;
    uint32_t *const pu32Bitmap = ctx + (tid * CK_THREADSEP) + CK_BITMAP;
    int32_t i32SrcBucket = tid * (8192/THREADS);
    uint32_t rel32ReadStart;
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER1;
    uint32_t u32Bucket;
    uint64_t const u64ItemMaskAnd = 0xfffffffff;
    uint64_t u64ItemMaskOr = 0;
    uint64_t dummy1, dummy2;
    uint32_t pu32ReadEndBuf[THREADS*R1PHASES];

    uint32_t rel32CtrInit = CK_BUCKET2 - CK_COUNTER0;
    for (int i=0; i<8192; i++) {
        pu32DstCtr[i] = rel32CtrInit;
        rel32CtrInit += CK_BUCKETSIZE2;
    }

    do {
        asm volatile ("rep stosq" : "=D" (dummy1), "=c" (dummy2) : "D" (pu32Bitmap), "a" (0), "c" (4096) );
        int fromthread = THREADS*R1PHASES;
        do {
#if THREADS == 4
            rel32ReadStart = i32SrcBucket * PhaseLookup2a[(fromthread >> 1) - 2] + PhaseLookup2a[(fromthread >> 1) - 1];
#elif THREADS == 8
            rel32ReadStart = i32SrcBucket * PhaseLookup2a[(fromthread >> 2) - 2] + PhaseLookup2a[(fromthread >> 2) - 1];
#else
#error
#endif
            do {
                uint32_t u32tmp = pu32SrcCtr[i32SrcBucket] + 3;
                u32tmp = (u32tmp > rel32ReadStart) ? u32tmp : rel32ReadStart;
                void *pRead = (void *) pu32SrcCtr + u32tmp;
                void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[i32SrcBucket+1];
                do {
                    uint32_t u32ReadVal = *(uint32_t *) pRead;
                    pRead += 5;
                    pu32Bitmap[u32ReadVal & 0x1fff] |= 1 << ((u32ReadVal >> 13) & 0x1f);
                } while (pRead < pReadEnd);
                pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
            } while (--fromthread & (THREADS-1));
            pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP) + 32768;
        } while (fromthread);
        fromthread = THREADS*R1PHASES;
        do {
            pu32SrcCtr = (void *) pu32SrcCtr + (THREADS * CK_THREADSEP) - 32768;
#if THREADS == 4
            rel32ReadStart = i32SrcBucket * PhaseLookup2b[(fromthread >> 1) - 2] + PhaseLookup2b[(fromthread >> 1) - 1];
#elif THREADS == 8
            rel32ReadStart = i32SrcBucket * PhaseLookup2b[(fromthread >> 2) - 2] + PhaseLookup2b[(fromthread >> 2) - 1];
#else
#error
#endif
            do {
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
                pu32ReadEndBuf[THREADS*R1PHASES-fromthread] = pWrite - (void *) pu32SrcCtr;
            } while (--fromthread & (THREADS-1));
        } while (fromthread);
        u32Bucket = 0;
        fromthread = THREADS*R1PHASES;
        do {
#if THREADS == 4
            rel32ReadStart = i32SrcBucket * PhaseLookup2c[(fromthread >> 1) - 2] + PhaseLookup2c[(fromthread >> 1) - 1];
#elif THREADS == 8
            rel32ReadStart = i32SrcBucket * PhaseLookup2c[(fromthread >> 2) - 2] + PhaseLookup2c[(fromthread >> 2) - 1];
#else
#error
#endif
            do {
                uint32_t u32tmp = pu32SrcCtr[i32SrcBucket-1] + 3;
                u32tmp = (u32tmp > rel32ReadStart) ? u32tmp : rel32ReadStart;
                void *pRead = (void *) pu32SrcCtr + u32tmp;
                void *pReadEnd = (void *) pu32SrcCtr + pu32ReadEndBuf[fromthread-1];
                do {
                    uint64_t u64ReadVal = *(uint64_t *) pRead;
                    pRead += 5;
                    u32Bucket += (((u64ReadVal >> 36) - u32Bucket) & 0xf);
                    uint32_t rel32Write = pu32DstCtr[u32Bucket];
                    pu32DstCtr[u32Bucket] = rel32Write + 5;
                    *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = (u64ReadVal & u64ItemMaskAnd) | u64ItemMaskOr;
                    pu32Bitmap[u64ReadVal & 0x1fff] &= ~(1 << ((u64ReadVal >> 13) & 0x1f));
                } while (pRead < pReadEnd);
                pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
            } while (--fromthread & (THREADS-1));
            pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP) + 32768;
        } while (fromthread);
        if ((u32Bucket & -16) != 0x1ff0)
            goto _DensityError;

        pu32SrcCtr = (void *) pu32SrcCtr - (32768*R1PHASES);
        u32Bucket = 0;
        u64ItemMaskOr += 0x1000000000;
        fromthread = THREADS*R1PHASES;
        do {
#if THREADS == 4
            rel32ReadStart = i32SrcBucket * PhaseLookup2d[(fromthread >> 1) - 2] + PhaseLookup2d[(fromthread >> 1) - 1];
#elif THREADS == 8
            rel32ReadStart = i32SrcBucket * PhaseLookup2d[(fromthread >> 2) - 2] + PhaseLookup2d[(fromthread >> 2) - 1];
#else
#error
#endif
            do {
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
                    pWrite += ((u32tmp^1) * 5UL);
                } while (pRead < pReadEnd);
                do {
                    uint64_t u64ReadVal = *(uint64_t *) pRead2;
                    pRead2 += 5;
                    u32Bucket += (((u64ReadVal >> 36) - u32Bucket) & 0xf);
                    uint32_t rel32Write = pu32DstCtr[u32Bucket];
                    pu32DstCtr[u32Bucket] = rel32Write + 5;
                    *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = (u64ReadVal & u64ItemMaskAnd) | u64ItemMaskOr;
                } while (pRead2 < pWrite);
                pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
            } while (--fromthread & (THREADS-1));
            pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP) + 32768;
        } while (fromthread);
        if ((u32Bucket & -16) != 0x1ff0)
            goto _DensityError;

        i32SrcBucket += 2;
        pu32SrcCtr = (void *) pu32SrcCtr - (32768*R1PHASES);
        u64ItemMaskOr += 0x1000000000;
    } while (i32SrcBucket & (8192/THREADS-2));

    return;

_DensityError:
    printf("Density Error %u %x\n", i32SrcBucket, u32Bucket);
    return;
}
