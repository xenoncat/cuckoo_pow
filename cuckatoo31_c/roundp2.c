#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>
#include "memlayout.h"

typedef struct {
    uint32_t next;
    uint32_t to;
} link;

void roundp2(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 32768);
    link *const links = ctx + CK_LINKS;
    uint32_t u32LinkIdBegin = tid * (1048576/THREADS);
    uint32_t u32LinkId = links[u32LinkIdBegin].next;
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER1;
    uint32_t u32SrcBucket = tid * (128/THREADS);
    uint32_t rel32ReadStart = CK_BUCKET97 - CK_COUNTER1 + u32SrcBucket * CK_BUCKETSIZE97;
    uint32_t *const pu32Adjlist = ctx + CK_ADJLIST;

    do {
        for (uint32_t fromthread=0; fromthread<THREADS; fromthread++) {
            uint64_t *pu64Read = (void *) pu32SrcCtr + rel32ReadStart;
            uint64_t *pu64ReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket];
            do {
                uint64_t u64ReadVal = *(pu64Read++);
                uint32_t u32NodeId = u64ReadVal >> 32;
                links[u32LinkId].next = pu32Adjlist[u32NodeId];
                pu32Adjlist[u32NodeId] = u32LinkId;
                links[u32LinkId].to = (uint32_t) u64ReadVal ^ 1;
                u32LinkId++;
            } while (pu64Read < pu64ReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        u32SrcBucket += 1;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE97;
    } while (u32SrcBucket & (128/THREADS-1));

    links[u32LinkId].next = 0xffffffff;
    links[u32LinkId].to = 0;
    links[u32LinkIdBegin].next = u32LinkId;
    links[u32LinkIdBegin].to = 0;
}
