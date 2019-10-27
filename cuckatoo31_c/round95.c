#include <stdint.h>
//#include <string.h>
#include <x86intrin.h>
#include "memlayout.h"

void round95(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 32768);
    uint32_t *pu32DstCtr = ctx + (tid * CK_THREADSEP) + CK_COUNTER1;
    uint32_t *pu32Bitmap = ctx + (tid * CK_THREADSEP) + CK_REN2;
    //uint32_t const u32InitWordCount = *(uint32_t *) (ctx + (tid * CK_THREADSEP) + CK_RENMAX1 + 4);
    uint32_t u32SrcBucket = tid * (256/THREADS);
    uint32_t rel32ReadStart = CK_BUCKET16 - CK_COUNTER0 + CK_BUCKETSIZE16 + u32SrcBucket * CK_BUCKETSIZE16;
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER0;
    uint32_t u32BucketOld, u32Bucket;
    uint32_t *pu32RenMax2 = ctx + (tid * CK_THREADSEP) + CK_RENMAX2;
    uint32_t u32ItemMaskOr = u32SrcBucket << 17;
    uint64_t dummy1, dummy2;

    *pu32RenMax2 = 0;
    uint32_t rel32CtrInit = CK_BUCKET95 - CK_COUNTER1;
    for (int i=0; i<1024; i++) {
        pu32DstCtr[i] = rel32CtrInit;
        rel32CtrInit += CK_BUCKETSIZE95;
    }

    do {
        asm volatile ("rep stosq" : "=D" (dummy1), "=c" (dummy2) : "D" (pu32Bitmap), "a" (0), "c" (0x800) );

        for (uint32_t fromthread=0; fromthread<THREADS; fromthread++) {
            void *pRead = (void *) pu32SrcCtr + rel32ReadStart + 2;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket+1];
            do {
                uint32_t u32ReadVal = *(uint32_t *) pRead;
                pRead += 5;
                pu32Bitmap[(u32ReadVal >> 6) & 0xfff] |= 1 << ((u32ReadVal >> 1) & 0x1f);
            } while (pRead < pReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        pu32Bitmap += 0x1000;
        asm volatile ("rep stosq" :"=D" (dummy1), "=c" (dummy2) : "D" (pu32Bitmap), "a" (0), "c" (0x800) );

        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart -= CK_BUCKETSIZE16;
        for (uint32_t fromthread=0; fromthread<THREADS; fromthread++) {
            void *pRead = (void *) pu32SrcCtr + rel32ReadStart;
            void *pWrite = pRead;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket];
            do {
                uint64_t u64ReadVal = *(uint64_t *) pRead;
                pRead += 5;
                *(uint64_t *) pWrite = u64ReadVal;
                uint32_t u32tmp2 = (pu32Bitmap[((u64ReadVal >> 22) & 0xfff) - 0x1000] >> ((u64ReadVal >> 17) & 0x1f)) & 1;
                pWrite += (u32tmp2 * 5);
                pu32Bitmap[(u64ReadVal >> 22) & 0xfff] |= (1 << ((u64ReadVal >> 17) & 0x1f));
            } while (pRead < pReadEnd);
            pu32SrcCtr[u32SrcBucket] = pWrite - (void *) pu32SrcCtr;
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        for (int i=0; i<0x1000; i++) {
            pu32Bitmap[i] &= pu32Bitmap[i-0x1000];
        }
        {
            //Begin rename
            uint64_t shifter = 0;
            int32_t count;
            uint32_t word;
            uint32_t base = 1;
            do {
                word = pu32Bitmap[0];
                if (word == 0)
                    goto _WordIsZero;
                count = __builtin_ctz(word);
                word >>= count;
                count = -count;
                if ((shifter & word) == 0)
                    goto _Rename;
                while (!(word & 0x80000000)) {
                    word <<= 1;
                    count += 1;
                    if ((shifter & word) == 0)
                        goto _Rename;
                }
                do {
                    shifter >>= 1;
                    base += 1;
                } while ((shifter & word) != 0);
                _Rename:
                pu32Bitmap[-0x1000] = count + base;
                shifter |= word;
                count = __builtin_ctz(~shifter);
                shifter >>= count;
                base += count;
                _WordIsZero:
                pu32Bitmap++;
            } while (((uint64_t) pu32Bitmap & 0x3fff) != 0);

            uint32_t u32tmp = *pu32RenMax2;
            u32tmp = (base > u32tmp) ? base : u32tmp;
            *pu32RenMax2 = u32tmp;
            //End rename
        }
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        pu32Bitmap -= 0x2000;
        for (uint32_t fromthread=0; fromthread<256; fromthread+=256/THREADS) {
            void *pRead = (void *) pu32SrcCtr + rel32ReadStart;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket];
            do {
                uint64_t u64ReadVal = *(uint64_t *) pRead;
                pRead += 5;
                uint32_t u32tmp = pu32Bitmap[(u64ReadVal >> 22) & 0xfff] + ((u64ReadVal >> 17) & 0x1f);
                uint64_t u64tmp = (uint64_t) u32tmp << 25;
                u64tmp |= (u64ReadVal & 0x1ffff);
                u32BucketOld = ((u64ReadVal >> 34) & 0x3f) | fromthread;
                u32Bucket = ((u64ReadVal & 0x3) << 8) | u32BucketOld;
                uint32_t rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = u64tmp | u32ItemMaskOr;
                pu32DstCtr[u32Bucket] = rel32Write + 5;
            } while (pRead < pReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE16;
        u32ItemMaskOr += 0x20000;
        for (uint32_t fromthread=0; fromthread<256; fromthread+=256/THREADS) {
            void *pRead = (void *) pu32SrcCtr + rel32ReadStart;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket+1];
            do {
                uint64_t u64ReadVal = *(uint64_t *) pRead;
                pRead += 5;
                uint32_t u32tmp = pu32Bitmap[(u64ReadVal >> 22) & 0xfff] + ((u64ReadVal >> 17) & 0x1f);
                uint32_t u32tmp2 = (pu32Bitmap[((u64ReadVal >> 22) & 0xfff) + 0x1000] >> ((u64ReadVal >> 17) & 0x1f)) & 1;
                uint64_t u64tmp = (uint64_t) u32tmp << 25;
                u64tmp |= (u64ReadVal & 0x1ffff);
                u32BucketOld = ((u64ReadVal >> 34) & 0x3f) | fromthread;
                u32Bucket = ((u64ReadVal & 0x3) << 8) | u32BucketOld;
                uint32_t rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = u64tmp | u32ItemMaskOr;
                pu32DstCtr[u32Bucket] = rel32Write + (u32tmp2 * 5);
            } while (pRead < pReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        u32SrcBucket += 2;
        pu32Bitmap += 0x2000;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE16 * 2;
        u32ItemMaskOr += 0x20000;
    } while (u32SrcBucket & (256/THREADS-2));
}
