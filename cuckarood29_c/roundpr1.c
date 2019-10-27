#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>
#include "memlayout.h"

typedef struct {
    uint32_t next;
    uint32_t to;
} link;

//  0  1  2
//A 0d 2  2
//B 0  1d 0
//C 1  1  2d

//B-type
void roundpr1(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 32768);
    link *const links = ctx + CK_LINKS;
    uint32_t u32LinkId = tid * (262144/THREADS);
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER0;
    uint32_t u32SrcBucket = tid * (128/THREADS);
    uint32_t rel32ReadStart = CK_BUCKET40 - CK_COUNTER0 + u32SrcBucket * CK_BUCKETSIZE40;
    uint8_t *pu8Bitmap = ctx + (tid * CK_THREADSEP) + CK_BITMAP;
    uint32_t u32NodeIdHigh = tid * (262144/THREADS);
    uint16_t *pu16Ren2 = ctx + (tid * CK_THREADSEP) + CK_REN2;
    uint32_t *const pu32Unren2 = ctx + CK_UNREN2;
    uint32_t *const pu32Adjlist = ctx + CK_ADJLIST;

    uint64_t dummy1, dummy2;
    uint32_t *const pu32InitAdjList = ctx + (tid<<(20-LOGTHREADS)) + CK_ADJLIST;
    asm volatile ("rep stosq" : "=D" (dummy1), "=c" (dummy2) : "D" (pu32InitAdjList), "a" (0), "c" (131072/THREADS) );

    links[u32LinkId].next = 0;
    links[u32LinkId].to = 0;
    u32LinkId++;

    do {
        for (uint32_t fromthread=0; fromthread<0x40000; fromthread+=(0x40000/THREADS)) {
            uint64_t *pu64Read = (void *) pu32SrcCtr + rel32ReadStart;
            uint64_t *pu64ReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket];
            do {
                uint64_t u64ReadVal = *(pu64Read++);
                uint32_t u32BitmapIndex = (uint32_t) u64ReadVal >> 7;
                if (pu8Bitmap[u32BitmapIndex] & 0x01)
                    continue;
                uint32_t u32NodeId = u32NodeIdHigh | pu16Ren2[u32BitmapIndex];
                pu32Unren2[u32NodeId] = (uint32_t) u64ReadVal;
                links[u32LinkId].next = pu32Adjlist[u32NodeId];
                pu32Adjlist[u32NodeId] = u32LinkId;
//if (u32NodeId == (fromthread | (u64ReadVal >> 32))) printf("2-cycle\n");
                links[u32LinkId].to = fromthread | (u64ReadVal >> 32);
                u32LinkId++;
            } while (pu64Read < pu64ReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        u32SrcBucket += 1;
        pu8Bitmap += 0x8000;
        pu16Ren2 += 0x8000;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE40;
#if THREADS < 4
        if (!(u32SrcBucket & 0x1f)) {
            u32NodeIdHigh += 0x10000;
        }
#endif
    } while (u32SrcBucket & (128/THREADS-1));

    links[u32LinkId].next = 0xffffffff;
    links[u32LinkId].to = 0;
}
