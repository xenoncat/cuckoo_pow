#include <stdio.h>
#include <stdint.h>
//#include <string.h>
#include <x86intrin.h>
#include "memlayout.h"

static uint32_t const LookupCtrInit[2] = {CK_BUCKET1A - CK_COUNTER1, CK_BUCKET1B - CK_COUNTER1B};

//4 thread case
//pu32DstCtr: phase0: ctx + tid * THREADSEP + COUNTER1
//            phase1: ctx + tid * THREADSEP + COUNTER1B
//i32SrcBucket: phase0: T0=0, T1=256, T2=512, T3=768; Loop step +=2; 128 loops
//              phase1: T0=1024, T1=1280, T2=1536, T3=1792
void round1(void *context, uint32_t tid, uint32_t phase)
{
    void *const ctx = __builtin_assume_aligned(context, 8192);
    uint32_t *pu32DstCtr = ctx + (tid * CK_THREADSEP) + (phase * 8192) + CK_COUNTER1;
    uint32_t *const pu32Bitmap = ctx + (tid * CK_THREADSEP) + CK_BITMAP;
    int32_t i32SrcBucket = (phase * 1024) + (tid * (1024/THREADS));
    uint32_t rel32ReadStart = CK_BUCKET0 - CK_COUNTER0 + i32SrcBucket * CK_BUCKETSIZE0;
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER0;
    uint64_t const u64ItemMaskAnd = 0xfffffffff;
    uint64_t u64ItemMaskOr = 0;
    uint32_t pu32ReadEndBuf[THREADS];
    uint64_t dummy1, dummy2;

    uint32_t rel32CtrInit = LookupCtrInit[phase];
    for (int i=0; i<2048; i++) {
        pu32DstCtr[i] = rel32CtrInit;
        rel32CtrInit += CK_BUCKETSIZE1;
    }

    do {
        //memset(pu32Bitmap, 0, 32768);
        asm volatile ("rep stosq" : "=D" (dummy1), "=c" (dummy2) : "D" (pu32Bitmap), "a" (0), "c" (4096) );

        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            uint32_t u32tmp = pu32SrcCtr[i32SrcBucket-1] + 2;
            u32tmp = (u32tmp > rel32ReadStart) ? u32tmp : rel32ReadStart;
            void *pRead = (void *) pu32SrcCtr + u32tmp;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[i32SrcBucket];
            do {
                uint32_t u32ReadVal = *(uint32_t *) pRead;
                pRead += 6;
                pu32Bitmap[u32ReadVal & 0x1fff] |= 1 << ((u32ReadVal >> 13) & 0x1f);
            } while (pRead < pReadEnd);

            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        rel32ReadStart += CK_BUCKETSIZE0;
        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            pu32SrcCtr = (void *) pu32SrcCtr - CK_THREADSEP;
            uint32_t u32tmp = pu32SrcCtr[i32SrcBucket] + 2;
            u32tmp = (u32tmp > rel32ReadStart) ? u32tmp : rel32ReadStart;
            void *pRead = (void *) pu32SrcCtr + u32tmp;
            void *pWrite = pRead;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[i32SrcBucket+1];
            do {
                uint64_t u64ReadVal = *(uint64_t *) pRead;
                pRead += 6;
                *(uint64_t *) pWrite = u64ReadVal;
                u32tmp = (pu32Bitmap[u64ReadVal & 0x1fff] >> ((u64ReadVal >> 13) & 0x1f)) & 1;
                pWrite += (u32tmp * 6UL);
            } while (pRead < pReadEnd);
            pu32ReadEndBuf[fromthread] = pWrite - (void *) pu32SrcCtr;
        }
        pu32DstCtr += 1;
        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            uint32_t u32tmp = pu32SrcCtr[i32SrcBucket] + 2;
            u32tmp = (u32tmp > rel32ReadStart) ? u32tmp : rel32ReadStart;
            void *pRead = (void *) pu32SrcCtr + u32tmp;
            void *pReadEnd = (void *) pu32SrcCtr + pu32ReadEndBuf[THREADS-1-fromthread];
            do {
                uint64_t u64ReadVal = *(uint64_t *) pRead;
                pRead += 6;
                uint32_t u32Bucket = (u64ReadVal >> 35) & 0x7fe;
                uint32_t rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write - 4) = (u64ReadVal & u64ItemMaskAnd) | u64ItemMaskOr;
                pu32DstCtr[u32Bucket] = rel32Write + 5;
                pu32Bitmap[u64ReadVal & 0x1fff] &= ~(1 << ((u64ReadVal >> 13) & 0x1f));
            } while (pRead < pReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        pu32DstCtr -= 1;
        rel32ReadStart -= CK_BUCKETSIZE0;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            uint32_t u32tmp = pu32SrcCtr[i32SrcBucket-1] + 2;
            u32tmp = (u32tmp > rel32ReadStart) ? u32tmp : rel32ReadStart;
            void *pRead = (void *) pu32SrcCtr + u32tmp;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[i32SrcBucket];
            do {
                uint64_t u64ReadVal = *(uint64_t *) pRead;
                pRead += 6;
                uint32_t u32Bucket = (u64ReadVal >> 35) & 0x7fe;
                uint32_t rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = (u64ReadVal & u64ItemMaskAnd) | u64ItemMaskOr;
                u32tmp = (pu32Bitmap[u64ReadVal & 0x1fff] >> ((u64ReadVal >> 13) & 0x1f)) & 1;
                pu32DstCtr[u32Bucket] = rel32Write + ((u32tmp^1) * 5UL);
            } while (pRead < pReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }

        i32SrcBucket += 2;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE0 * 2;
        u64ItemMaskOr += 0x1000000000;

    } while (i32SrcBucket & (1024/THREADS-2));
}
