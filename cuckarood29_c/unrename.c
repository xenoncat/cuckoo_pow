#include <stdio.h>
#include <stdint.h>
//#include <string.h>
#include <x86intrin.h>
#include "memlayout.h"

void Unrename(void *ctx)
{
    uint32_t u32rencount = (*(uint32_t *) ctx) * 21;
    uint32_t *pu32sol = ctx + 8;
    uint32_t *pu32unren2 = ctx + CK_UNREN2;

    for (uint32_t i=0; i<u32rencount; i++) {
        *pu32sol = pu32unren2[*pu32sol];
        pu32sol += 2;
    }

    //ren1 format: u1[(11:0)(27:10)], u[2:0], u[9:3]
    pu32sol = ctx + 8;
    for (uint32_t i=0; i<u32rencount; i++) {
        uint32_t ren1 = *pu32sol;
        uint32_t ren1a = ((ren1 & 0x7f) << 3) | ((ren1 >> 7) & 0x7);
        uint32_t ren1tid = ren1a >> (10-LOGTHREADS);
        uint32_t ren1table = ren1a & (1024/THREADS-1);
        uint32_t *pu32basemap = ctx + (ren1tid * CK_THREADSEP) + (ren1table * 0x10000) + CK_REN1;
        uint32_t *pu32bitmap = pu32basemap + 0x2000;
        int32_t target = (ren1 >> 10) & 0xfff;
        int32_t guess0 = (161222*target) >> 16; // 8192/3330
        guess0 = (guess0>8191) ? 8191 : guess0;

        //printf("ren %d ren1a=%x target=%d guess0=%d\n", i, ren1a, target, guess0);

        uint32_t bitmapval;
        int32_t baseval, base0min, base0max, clz, ctz;
        int32_t deviation = 0;
        bitmapval = pu32bitmap[guess0];
        do {
            if ((guess0+deviation) < 8192) {
                bitmapval = pu32bitmap[guess0+deviation];
                if (bitmapval) {
                    baseval = pu32basemap[guess0+deviation];
                    ctz = __builtin_ctz(bitmapval);
                    clz = __builtin_clz(bitmapval);
                    base0max = baseval + ctz;
                    base0min = (ctz == (31-clz)) ? base0max : baseval - clz;
                    //printf("dev=%d %x %d %d %d\n", deviation, bitmapval, baseval, base0min, base0max);
                    if (base0max < (target-31)) {
                        //continue up
                        deviation += 1;
                        break;
                    }
                    if (base0min > target) {
                        //flip down
                        deviation = ~deviation;
                        break;
                    }
                    do {
                        if (baseval + ctz == target) {
                            //printf("found0\n");
                            guess0 = guess0+deviation;
                            goto _Found;
                        }
                        bitmapval &= ~(1 << ctz);
                        ctz = __builtin_ctz(bitmapval);
                    } while (bitmapval);
                }
            }
            deviation = ~deviation;

            if ((guess0+deviation) >= 0) {
                bitmapval = pu32bitmap[guess0+deviation];
                if (bitmapval) {
                    baseval = pu32basemap[guess0+deviation];
                    ctz = __builtin_ctz(bitmapval);
                    clz = __builtin_clz(bitmapval);
                    base0max = baseval + ctz;
                    base0min = (ctz == (31-clz)) ? base0max : baseval - clz;
                    //printf("dev=%d %x %d %d %d\n", deviation, bitmapval, baseval, base0min, base0max);
                    if (base0max < (target-31)) {
                        //flip up
                        deviation = -deviation;
                        break;
                    }
                    if (base0min > target) {
                        //continue down;
                        deviation -= 1;
                        break;
                    }
                    do {
                        if (baseval + ctz == target) {
                            //printf("found1\n");
                            guess0 = guess0+deviation;
                            goto _Found;
                        }
                        bitmapval &= ~(1 << ctz);
                        ctz = __builtin_ctz(bitmapval);
                    } while (bitmapval);
                }
            }
            deviation = -deviation;
        } while (deviation < 256);

        guess0 += deviation;
        if (deviation >= 0) {
            while (guess0 < 8192) {
                bitmapval = pu32bitmap[guess0];
                if (bitmapval) {
                    baseval = pu32basemap[guess0];
                    do {
                        ctz = __builtin_ctz(bitmapval);
                        if (baseval + ctz == target) {
                            //printf("found2\n");
                            goto _Found;
                        }
                        bitmapval &= ~(1 << ctz);
                    } while (bitmapval);
                }
                guess0++;
            }
        }
        else {
            while (guess0 >= 0) {
                bitmapval = pu32bitmap[guess0];
                if (bitmapval) {
                    baseval = pu32basemap[guess0];
                    do {
                        ctz = __builtin_ctz(bitmapval);
                        if (baseval + ctz == target) {
                            //printf("found3\n");
                            goto _Found;
                        }
                        bitmapval &= ~(1 << ctz);
                    } while (bitmapval);
                }
                guess0--;
            }
        }
        printf("BUG unrename fail\n");
        *(uint32_t *) ctx = 0;
        return;

        _Found:
        //printf("sol %x\n", (((ctz<<13) | guess0) << 10) | ren1a);
        *pu32sol = (((ctz<<13) | guess0) << 10) | ren1a;
        pu32sol += 2;
    }

    uint32_t *pu32Bitmap = ctx + CK_BITMAP;
    uint64_t dummy1, dummy2;
    asm volatile ("rep stosq" : "=D" (dummy1), "=c" (dummy2) : "D" (pu32Bitmap), "a" (0), "c" (8192));
    pu32sol = ctx + 8;
    for (uint32_t i=0; i<u32rencount; i++) {
        uint32_t node = *pu32sol;
        pu32sol += 2;
        pu32Bitmap[(node>> 5)&0x1fff] |= 1 << (node & 0x1f);
        pu32Bitmap[(node>>15)+0x2000] |= 1 << (node & 0x1f);
    }

    uint32_t *pu32TargetNodes = ctx;
    uint32_t *pu32Header = ctx + CK_RECOVERWRITE;
    uint32_t u32NumTargets2 = pu32TargetNodes[0] * 42;
    for (uint32_t u0=0; u0<u32NumTargets2; u0++) {
        pu32Header[u0] = u32NumTargets2 + u0*2;
    }
}
