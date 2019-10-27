#include <stdint.h>
#include <x86intrin.h>
#include "memlayout.h"

//  0  1  2
//A 0d 2  2
//B 0  1d 0
//C 1  1  2d
void rounda0(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 32768);
    uint32_t *const pu32DstCtr = ctx + (tid * CK_THREADSEP) + CK_COUNTER0;
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER1;
    uint32_t u32SrcBucket = tid * (128/THREADS);
    uint32_t rel32ReadStart = CK_BUCKET41 - CK_COUNTER1 + u32SrcBucket * CK_BUCKETSIZE41;
    uint8_t *pu8Bitmap = ctx + (tid * CK_THREADSEP) + CK_BITMAP;

    uint32_t rel32Write = CK_BUCKET40 - CK_COUNTER0;
    for (int i=0; i<128; i++) {
        pu32DstCtr[i] = rel32Write;
        rel32Write += CK_BUCKETSIZE40;
    }

    do {
        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            uint64_t *pu64Read = (void *) pu32SrcCtr + rel32ReadStart;
            uint64_t *pu64ReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket];
            do {
                uint64_t u64ReadVal = *(pu64Read++);
                uint32_t u32Bucket = u64ReadVal & 0x7f;
                rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = u64ReadVal;
                uint32_t u32tmp = pu8Bitmap[u64ReadVal >> (32+7)];
                u32tmp = (u32tmp + 1) & 2;
                pu8Bitmap[u64ReadVal >> (32+7)] = (uint8_t) u32tmp;
                pu32DstCtr[u32Bucket] = rel32Write + (u32tmp * 4);
            } while (pu64Read < pu64ReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        u32SrcBucket += 1;
        pu8Bitmap += 0x8000;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE41;
    } while (u32SrcBucket & (128/THREADS-1));
}

void roundb0(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 32768);
    uint32_t *const pu32DstCtr = ctx + (tid * CK_THREADSEP) + CK_COUNTER0;
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER1;
    uint32_t u32SrcBucket = tid * (128/THREADS);
    uint32_t rel32ReadStart = CK_BUCKET41 - CK_COUNTER1 + u32SrcBucket * CK_BUCKETSIZE41;
    uint8_t *pu8Bitmap = ctx + (tid * CK_THREADSEP) + CK_BITMAP;

    uint32_t rel32Write = CK_BUCKET40 - CK_COUNTER0;
    for (int i=0; i<128; i++) {
        pu32DstCtr[i] = rel32Write;
        rel32Write += CK_BUCKETSIZE40;
    }

    do {
        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            uint64_t *pu64Read = (void *) pu32SrcCtr + rel32ReadStart;
            uint64_t *pu64ReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket];
            do {
                uint64_t u64ReadVal = *(pu64Read++);
                uint32_t u32Bucket = u64ReadVal & 0x7f;
                rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = u64ReadVal;
                uint32_t u32tmp = pu8Bitmap[u64ReadVal >> (32+7)];
                u32tmp &= 1;
                pu8Bitmap[u64ReadVal >> (32+7)] = (uint8_t) u32tmp;
                pu32DstCtr[u32Bucket] = rel32Write + ((u32tmp^1) * 8);
            } while (pu64Read < pu64ReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        u32SrcBucket += 1;
        pu8Bitmap += 0x8000;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE41;
    } while (u32SrcBucket & (128/THREADS-1));
}

void roundc0(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 32768);
    uint32_t *const pu32DstCtr = ctx + (tid * CK_THREADSEP) + CK_COUNTER0;
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER1;
    uint32_t u32SrcBucket = tid * (128/THREADS);
    uint32_t rel32ReadStart = CK_BUCKET41 - CK_COUNTER1 + u32SrcBucket * CK_BUCKETSIZE41;
    uint8_t *pu8Bitmap = ctx + (tid * CK_THREADSEP) + CK_BITMAP;

    uint32_t rel32Write = CK_BUCKET40 - CK_COUNTER0;
    for (int i=0; i<128; i++) {
        pu32DstCtr[i] = rel32Write;
        rel32Write += CK_BUCKETSIZE40;
    }

    do {
        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            uint64_t *pu64Read = (void *) pu32SrcCtr + rel32ReadStart;
            uint64_t *pu64ReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket];
            do {
                uint64_t u64ReadVal = *(pu64Read++);
                uint32_t u32Bucket = u64ReadVal & 0x7f;
                rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = u64ReadVal;
                uint32_t u32tmp = pu8Bitmap[u64ReadVal >> (32+7)];
                u32tmp += (u32tmp == 0);
                pu8Bitmap[u64ReadVal >> (32+7)] = (uint8_t) u32tmp;
                pu32DstCtr[u32Bucket] = rel32Write + ((u32tmp&1) * 8);
            } while (pu64Read < pu64ReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        u32SrcBucket += 1;
        pu8Bitmap += 0x8000;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE41;
    } while (u32SrcBucket & (128/THREADS-1));
}
