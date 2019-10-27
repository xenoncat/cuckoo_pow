#include <stdint.h>
//#include <string.h>
#include <x86intrin.h>
#include "memlayout.h"

//  0  1  2
//A 0d 2  2
//B 0  1d 0
//C 1  1  2d

//A-type
void roundpr0(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 32768);
    uint32_t *const pu32DstCtr = ctx + (tid * CK_THREADSEP) + CK_COUNTER0;
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER1;
    uint32_t u32SrcBucket = tid * (128/THREADS);
    uint32_t rel32ReadStart = CK_BUCKET41 - CK_COUNTER1 + u32SrcBucket * CK_BUCKETSIZE41;
    uint8_t *pu8Bitmap = ctx + (tid * CK_THREADSEP) + CK_BITMAP;
    uint16_t *pu16Ren2 = ctx + (tid * CK_THREADSEP) + CK_REN2;
    uint32_t u32NewNodeId = 1;

    uint32_t rel32Write = CK_BUCKET40 - CK_COUNTER0;
    for (int i=0; i<128; i++) {
        pu32DstCtr[i] = rel32Write;
        rel32Write += CK_BUCKETSIZE40;
    }

#if THREADS < 4
    uint64_t u64ItemMaskOr = 0;
#endif
    do {
        for (int fromthread=0; fromthread<THREADS; fromthread++) {
            uint64_t *pu64Read = (void *) pu32SrcCtr + rel32ReadStart;
            uint64_t *pu64ReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket];
            do {
                uint64_t u64ReadVal = *(pu64Read++);
                uint32_t u32BitmapAccess = u64ReadVal >> (32+7);
                uint32_t u32tmp = pu8Bitmap[u32BitmapAccess];
                //0: drop, 1: new, 2: reuse
                uint32_t u32Node = pu16Ren2[u32BitmapAccess];
                u32Node = (u32tmp & 1) ? u32NewNodeId : u32Node;
                pu16Ren2[u32BitmapAccess] = u32Node;
                u32NewNodeId += u32tmp & 1;
                u32tmp = (u32tmp + 1) & 2;  //0->0, 1->2, 2->2
                pu8Bitmap[u32BitmapAccess] = (uint8_t) u32tmp;

                uint32_t u32Bucket = u64ReadVal & 0x7f;
                rel32Write = pu32DstCtr[u32Bucket];
#if THREADS < 4
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = u64ItemMaskOr | ((uint64_t) u32Node << 32) | ((uint32_t) u64ReadVal);
#else
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = ((uint64_t) u32Node << 32) | ((uint32_t) u64ReadVal);
#endif
                pu32DstCtr[u32Bucket] = rel32Write + (u32tmp * 4);

            } while (pu64Read < pu64ReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        u32SrcBucket += 1;
        pu8Bitmap += 0x8000;
        pu16Ren2 += 0x8000;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE41;
#if THREADS < 4
        if (!(u32SrcBucket & 0x1f)) {
            u32NewNodeId = 1;
            u64ItemMaskOr += (1UL << 48);
        }
#endif
    } while (u32SrcBucket & (128/THREADS-1));

    //CK_RENMAXP0 for info only, not accurate for THREADS<4
    *(uint32_t *) ((void *) pu32DstCtr + CK_RENMAXP0 - CK_COUNTER0) = u32NewNodeId;
}
