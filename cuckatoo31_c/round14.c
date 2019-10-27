#include <stdint.h>
//#include <string.h>
#include <x86intrin.h>
#include "memlayout.h"

void round14(void *context, uint32_t tid)
{
    void *const ctx = __builtin_assume_aligned(context, 32768);
    uint32_t *pu32DstCtr = ctx + (tid * CK_THREADSEP) + CK_COUNTER0;
    uint32_t *pu32Bitmap = ctx + (tid * CK_THREADSEP) + CK_BITMAP;
    uint32_t u32SrcBucket = tid * (8192/THREADS);
    uint32_t rel32ReadStart = CK_BUCKET9 - CK_COUNTER1 + CK_BUCKETSIZE9 + u32SrcBucket * CK_BUCKETSIZE9;
    uint32_t *pu32SrcCtr = ctx + CK_COUNTER1;
    uint64_t u64ItemMaskOr = (uint64_t) u32SrcBucket << 30;
    uint32_t *pu32RenMax0 = ctx + (tid * CK_THREADSEP) + CK_RENMAX0;
    uint32_t u32Bucket;
    uint64_t dummy1, dummy2;

    *pu32RenMax0 = 0;
    uint32_t rel32CtrInit = CK_BUCKET14 - CK_COUNTER0;
    for (int i=0; i<8192; i++) {
        pu32DstCtr[i] = rel32CtrInit;
        rel32CtrInit += CK_BUCKETSIZE14;
    }

    do {
        asm volatile ("rep stosq" : "=D" (dummy1), "=c" (dummy2) : "D" (pu32Bitmap), "a" (0), "c" (4096) );

        for (uint32_t fromthread=0; fromthread<THREADS; fromthread++) {
            void *pRead = (void *) pu32SrcCtr + rel32ReadStart;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket+1];
            do {
                uint32_t u32ReadVal = *(uint32_t *) pRead;
                pRead += 6;
                pu32Bitmap[u32ReadVal & 0x1fff] |= 1 << ((u32ReadVal >> 13) & 0x1f);
            } while (pRead < pReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        pu32Bitmap += 0x2000;
        asm volatile ("rep stosq" : "=D" (dummy1), "=c" (dummy2) : "D" (pu32Bitmap), "a" (0), "c" (4096) );
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart -= CK_BUCKETSIZE9;
        for (uint32_t fromthread=0; fromthread<THREADS; fromthread++) {
            void *pRead = (void *) pu32SrcCtr + rel32ReadStart;
            void *pWrite = pRead;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket];
            do {
                uint64_t u64ReadVal = *(uint64_t *) pRead;
                pRead += 6;
                *(uint64_t *) pWrite = u64ReadVal;
                uint32_t u32tmp2 = (pu32Bitmap[(u64ReadVal & 0x1fff) - 0x2000] >> ((u64ReadVal >> 13) & 0x1f)) & 1;
                pWrite += (u32tmp2 * 6);
                pu32Bitmap[u64ReadVal & 0x1fff] |= (1 << ((u64ReadVal >> 13) & 0x1f));
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

            uint32_t u32tmp = *pu32RenMax0;
            u32tmp = (base > u32tmp) ? base : u32tmp;
            *pu32RenMax0 = u32tmp;
            //End rename
        }
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        pu32Bitmap -= 0x4000;
        for (uint32_t fromthread=0; fromthread<8192; fromthread+=(8192/THREADS)) {
            void *pRead = (void *) pu32SrcCtr + rel32ReadStart;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket];
            do {
                uint64_t u64ReadVal = *(uint64_t *) pRead;
                pRead += 6;
                uint32_t u32tmp = pu32Bitmap[u64ReadVal & 0x1fff] + ((u64ReadVal >> 13) & 0x1f);
                u32Bucket = ((u64ReadVal >> 36) & 0xfff) | fromthread;
                u32tmp |= (u64ReadVal >> 6) & 0x3ffff000;
                uint32_t rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = u64ItemMaskOr | u32tmp;
                pu32DstCtr[u32Bucket] = rel32Write + 5;
            } while (pRead < pReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE9;
        u64ItemMaskOr += 0x40000000;
        for (uint32_t fromthread=0; fromthread<8192; fromthread+=(8192/THREADS)) {
            void *pRead = (void *) pu32SrcCtr + rel32ReadStart;
            void *pReadEnd = (void *) pu32SrcCtr + pu32SrcCtr[u32SrcBucket+1];
            do {
                uint64_t u64ReadVal = *(uint64_t *) pRead;
                pRead += 6;
                uint32_t u32tmp = pu32Bitmap[u64ReadVal & 0x1fff] + ((u64ReadVal >> 13) & 0x1f);
                uint32_t u32tmp2 = (pu32Bitmap[(u64ReadVal & 0x1fff) + 0x2000] >> ((u64ReadVal >> 13) & 0x1f)) & 1;
                u32Bucket = ((u64ReadVal >> 36) & 0xfff) | fromthread;
                u32tmp |= (u64ReadVal >> 6) & 0x3ffff000;
                uint32_t rel32Write = pu32DstCtr[u32Bucket];
                *(uint64_t *) ((void *) pu32DstCtr + rel32Write) = u64ItemMaskOr | u32tmp;
                pu32DstCtr[u32Bucket] = rel32Write + (u32tmp2 * 5);
            } while (pRead < pReadEnd);
            pu32SrcCtr = (void *) pu32SrcCtr + CK_THREADSEP;
        }
        u32SrcBucket += 2;
        pu32SrcCtr = (void *) pu32SrcCtr - (THREADS * CK_THREADSEP);
        rel32ReadStart += CK_BUCKETSIZE9 * 2;
        u64ItemMaskOr += 0x40000000;
    } while (u32SrcBucket & (8192/THREADS-2));

    uint32_t u32tmp = pu32RenMax0[0] + (63+16);
    u32tmp &= -16;
    u32tmp >>= 1;
    pu32RenMax0[1] = (u32tmp > 2048) ? 2048 : u32tmp;
}
