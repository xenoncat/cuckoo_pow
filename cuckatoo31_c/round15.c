#include <stdio.h>
#include <stdint.h>
//#include <string.h>
#include <x86intrin.h>
#include "memlayout.h"

void round15(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 32768);
    uint32_t *pu32DstCtr = ctx + (tid * CK_THREADSEP) + CK_COUNTER1;
    uint32_t *pu32Bitmap = ctx + (tid * CK_THREADSEP) + CK_REN1;
    uint32_t u32SrcBucket = tid * (8192/THREADS);
    uint32_t rel32ReadStart = CK_BUCKET14 - CK_COUNTER0 + CK_BUCKETSIZE14 + u32SrcBucket * CK_BUCKETSIZE14;
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER0;
    uint32_t u32BucketOld, u32Bucket;
    uint32_t *pu32RenMax1 = ctx + (tid * CK_THREADSEP) + CK_RENMAX1;
    uint64_t dummy1, dummy2;

    *pu32RenMax1 = 0;
    uint32_t rel32CtrInit = CK_BUCKET15 - CK_COUNTER1;
    for (int i=0; i<256; i++) {
        pu32DstCtr[i] = rel32CtrInit;
        rel32CtrInit += CK_BUCKETSIZE15;
    }

    do {
        asm volatile ("rep stosq" : "=D" (dummy1), "=c" (dummy2) : "D" (pu32Bitmap), "a" (0), "c" (4096) );

        uint64_t u64ItemMaskOr = (((uint64_t) u32SrcBucket & 0x3f) << 34) | ((u32SrcBucket & 0x1f00) << 9);

        for (uint32_t fromthread=0; fromthread<THREADS; fromthread++) {
            void *pRead = (void *) pu32SrcCtr + rel32ReadStart;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket+1];
            do {
                uint32_t u32ReadVal = *(uint32_t *) pRead;
                pRead += 5;
                pu32Bitmap[(u32ReadVal >> 17) & 0x1fff] |= 1 << ((u32ReadVal >> 12) & 0x1f);
            } while (pRead < pReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        pu32Bitmap += 0x2000;
        asm volatile ("rep stosq" :"=D" (dummy1), "=c" (dummy2) : "D" (pu32Bitmap), "a" (0), "c" (4096) );

        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart -= CK_BUCKETSIZE14;
        for (uint32_t fromthread=0; fromthread<THREADS; fromthread++) {
            void *pRead = (void *) pu32SrcCtr + rel32ReadStart;
            void *pWrite = pRead;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket];
            do {
                uint64_t u64ReadVal = *(uint64_t *) pRead;
                pRead += 5;
                *(uint64_t *) pWrite = u64ReadVal;
                uint32_t u32tmp2 = (pu32Bitmap[((u64ReadVal >> 17) & 0x1fff) - 0x2000] >> ((u64ReadVal >> 12) & 0x1f)) & 1;
                pWrite += (u32tmp2 * 5);
                pu32Bitmap[(u64ReadVal >> 17) & 0x1fff] |= (1 << ((u64ReadVal >> 12) & 0x1f));
            } while (pRead < pReadEnd);
            pu32SrcCtr[u32SrcBucket] = pWrite - (void *) pu32SrcCtr;
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        for (int i=0; i<0x2000; i++) {
            pu32Bitmap[i] &= pu32Bitmap[i-0x2000];
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
                pu32Bitmap[-0x2000] = count + base;
                shifter |= word;
                count = __builtin_ctz(~shifter);
                shifter >>= count;
                base += count;
                _WordIsZero:
                pu32Bitmap++;
            } while (((uint64_t) pu32Bitmap & 0x7fff) != 0);

            uint32_t u32tmp = *pu32RenMax1;
            u32tmp = (base > u32tmp) ? base : u32tmp;
            *pu32RenMax1 = u32tmp;
            //End rename
        }
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        pu32Bitmap -= 0x4000;
        u32BucketOld = 0;
        for (uint32_t fromthread=0; fromthread<THREADS; fromthread++) {
            void *pRead = (void *) pu32SrcCtr + rel32ReadStart;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket];
            do {
                uint64_t u64ReadVal = *(uint64_t *) pRead;
                pRead += 5;
                uint32_t u32tmp = pu32Bitmap[(u64ReadVal >> 17) & 0x1fff] + ((u64ReadVal >> 12) & 0x1f);
                uint64_t u64tmp = (uint64_t) u32tmp << 22;
                u64tmp |= (u64ReadVal & 0xfff) << 5;
                u32BucketOld += ((u64ReadVal >> 30) - u32BucketOld) & 0x3ff;
                u32Bucket = u32BucketOld & 0xff;
                uint32_t rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = u64ItemMaskOr | u64tmp | (u32BucketOld >> 8);
                pu32DstCtr[u32Bucket] = rel32Write + 5;
            } while (pRead < pReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        if ((u32BucketOld & -1024) != 0x1c00)
            goto _DensityError;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE14;
        u32BucketOld = 0;
        u64ItemMaskOr += 0x400000000;
        for (uint32_t fromthread=0; fromthread<THREADS; fromthread++) {
            void *pRead = (void *) pu32SrcCtr + rel32ReadStart;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket+1];
            do {
                uint64_t u64ReadVal = *(uint64_t *) pRead;
                pRead += 5;
                uint32_t u32tmp = pu32Bitmap[(u64ReadVal >> 17) & 0x1fff] + ((u64ReadVal >> 12) & 0x1f);
                uint32_t u32tmp2 = (pu32Bitmap[((u64ReadVal >> 17) & 0x1fff) + 0x2000] >> ((u64ReadVal >> 12) & 0x1f)) & 1;
                uint64_t u64tmp = (uint64_t) u32tmp << 22;
                u64tmp |= (u64ReadVal & 0xfff) << 5;
                u32BucketOld += ((u64ReadVal >> 30) - u32BucketOld) & 0x3ff;
                u32Bucket = u32BucketOld & 0xff;
                uint32_t rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = u64ItemMaskOr | u64tmp | (u32BucketOld >> 8);
                pu32DstCtr[u32Bucket] = rel32Write + (u32tmp2 * 5);
            } while (pRead < pReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        if ((u32BucketOld & -1024) != 0x1c00)
            goto _DensityError;
        u32SrcBucket += 2;
        pu32Bitmap += 0x4000;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE14 * 2;
    } while (u32SrcBucket & (8192/THREADS-2));

_Epilog:
    ;
    uint32_t u32tmp = pu32RenMax1[0] + (63+16);
    u32tmp &= -16;
    u32tmp >>= 1;
    pu32RenMax1[1] = (u32tmp > 2048) ? 2048 : u32tmp;
    return;

_DensityError:
    *(uint32_t *) (ctx+CK_ABORT) = 1;
    printf("Density Error %u %x\n", u32SrcBucket, u32Bucket);
    goto _Epilog;
}
