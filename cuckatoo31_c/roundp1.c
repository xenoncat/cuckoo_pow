#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>
#include "memlayout.h"

typedef struct {
    uint32_t next;
    uint32_t to;
} link;

void roundp1(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 32768);
    uint32_t *pu32DstCtr = ctx + (tid * CK_THREADSEP) + CK_COUNTER1;
    link *const links = ctx + CK_LINKS;
    uint32_t u32LinkId = tid * (1048576/THREADS);
    uint32_t u32LinkIdBegin = u32LinkId;
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER0;
    uint32_t u32SrcBucket = tid * (128/THREADS);
    uint32_t rel32ReadStart = CK_BUCKET96 - CK_COUNTER0 + u32SrcBucket * CK_BUCKETSIZE96;
    uint32_t *const pu32Adjlist = ctx + CK_ADJLIST;

    uint32_t rel32CtrInit = CK_BUCKET97 - CK_COUNTER1;
    for (int i=0; i<128; i++) {
        pu32DstCtr[i] = rel32CtrInit;
        rel32CtrInit += CK_BUCKETSIZE97;
    }
    u32LinkId++;

    do {
        for (uint32_t fromthread=0; fromthread<THREADS; fromthread++) {
            uint64_t *pu64Read = (void *) pu32SrcCtr + rel32ReadStart;
            uint64_t *pu64ReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket];
            do {
                uint64_t u64ReadVal = *(pu64Read++);
                u64ReadVal &= 0x000fffff000fffffUL;
                uint32_t u32Bucket = (u64ReadVal >> 32) & 0x7f;
                uint32_t rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = u64ReadVal;
                pu32DstCtr[u32Bucket] = rel32Write + 8;
                uint32_t u32NodeId = (uint32_t) u64ReadVal;
                links[u32LinkId].next = pu32Adjlist[u32NodeId];
                pu32Adjlist[u32NodeId] = u32LinkId;
                links[u32LinkId].to = (u64ReadVal >> 32) ^ 1;
                u32LinkId++;
            } while (pu64Read < pu64ReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        u32SrcBucket += 1;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE96;
    } while (u32SrcBucket & (128/THREADS-1));

    links[u32LinkIdBegin].next = u32LinkId;
    links[u32LinkIdBegin].to = 0;
}
