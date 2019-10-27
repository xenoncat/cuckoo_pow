#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>
#include <sys/mman.h>
#include <pthread.h>
#include <time.h>
#include "memlayout.h"

#define PRINTSTAT 1

static const uint64_t blake2biv[8] = {
        0x6a09e667f3bcc908, 0xbb67ae8584caa73b,
        0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
        0x510e527fade682d1, 0x9b05688c2b3e6c1f,
        0x1f83d9abfb41bd6b, 0x5be0cd19137e2179
};

typedef struct {
    void *ctx;
    uint32_t tid;
    uint32_t pad;
} threaddata_t;

void blake2b_compress(uint64_t * state, uint8_t const * const msg);
void sipseed0(void *ctx, uint32_t tid);
void sipseed1(void *ctx, uint32_t tid, uint32_t phase);
void round2(void *ctx, uint32_t tid);
void round3(void *ctx, uint32_t tid);
void round4(void *ctx, uint32_t tid);
void round8(void *ctx, uint32_t tid);
void round9(void *ctx, uint32_t tid);
void round10(void *ctx, uint32_t tid);
void round14(void *ctx, uint32_t tid);
void round15(void *ctx, uint32_t tid);
void round16(void *ctx, uint32_t tid);
void round17(void *ctx, uint32_t tid);
void round18(void *ctx, uint32_t tid);
void round95(void *ctx, uint32_t tid);
void round96(void *ctx, uint32_t tid);
void roundp1(void *ctx, uint32_t tid);
void roundp2(void *ctx, uint32_t tid);
void FindCycles(void *ctx);
void Unrename(void *ctx);
void siprecover(void *ctx, uint32_t tid);
void siprecover2(void *ctx);

static int cmpfunc(const void * a, const void * b)
{
   if (*(uint32_t*) a < *(uint32_t*) b) return -1;
   if (*(uint32_t*) a > *(uint32_t*) b) return 1;
   return 0;
}

void *ThreadEntry(void *p1)
{
    threaddata_t *p = p1;
    void * const ctx = p->ctx;
    const uint32_t tid = p->tid;
    void * const barrier = ctx + CK_BARRIER;
    uint64_t * const sipinit = ctx + CK_BLAKE2BSTATE;
    uint32_t *pu32solcount = ctx;
    uint32_t *pu32sol = (ctx + 8);
    uint32_t *pu32Abort = (ctx +CK_ABORT);
    uint64_t rdtsc0, rdtsc1;
    uint64_t q0, q1;
    uint32_t *pu32stat;
    uint32_t beginbucket, bucketval, bucketmin, bucketmax, buckettotal;
    int32_t i, roundnum;

    if (tid == 0) {
        *(__m256i *) (ctx+CK_BLAKE2BSTATE) = *(__m256i *) (ctx+CK_BLAKE2BSTATE_INIT);
        *(__m256i *) (ctx+CK_BLAKE2BSTATE+32) = *(__m256i *) (ctx+CK_BLAKE2BSTATE_INIT+32);
        blake2b_compress(sipinit, ctx+CK_BLAKE2BMSG);
        q0 = sipinit[0];
        q1 = sipinit[1];
        printf("k0 k1 k2 k3=%016lx %016lx %016lx %016lx\n", q0, q1, sipinit[2], sipinit[3]);
        q0 += q1;
        q1 = (q1<<13) | (q1>>51);
        q1 ^= q0;
        sipinit[0] = (q0<<32) | (q0>>32);
        sipinit[1] = q1;
        sipinit[4] = (q1<<17) | (q1>>47);
        *pu32Abort = 0;
        *pu32solcount = 0;
    }
    pthread_barrier_wait(barrier);

    rdtsc0 = _rdtsc();
    sipseed0(ctx, tid);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER0;
    beginbucket = CK_BUCKET0 - CK_COUNTER0;
    bucketmin = -1;
    bucketmax = 0;
    buckettotal = 0;
    for (i=0; i<8192; i++) {
        bucketval = *pu32stat - beginbucket;
        bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
        bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
        buckettotal += bucketval;
        pu32stat++;
        beginbucket += CK_BUCKETSIZE0;
    }
    printf("R0 T%d %u %u %u %lu\n", tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
    #endif

    uint64_t stat1 = 0;
    rdtsc0 = _rdtsc();
    sipseed1(ctx, tid, 0);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER1;
    beginbucket = CK_BUCKET1A - CK_COUNTER1;
    bucketmin = -1;
    bucketmax = 0;
    buckettotal = 0;
    for (i=0; i<8192; i++) {
        bucketval = *pu32stat - beginbucket;
        bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
        bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
        buckettotal += bucketval;
        pu32stat++;
        beginbucket += CK_BUCKETSIZE1A;
    }
    stat1 += buckettotal;
    printf("R1A T%d %u %u %u %lu\n", tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
    #endif
    rdtsc0 = _rdtsc();
    sipseed1(ctx, tid, 1);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER1 + 32768;
    beginbucket = CK_BUCKET1B - (CK_COUNTER1 + 32768);
    bucketmin = -1;
    bucketmax = 0;
    buckettotal = 0;
    for (i=0; i<8192; i++) {
        bucketval = *pu32stat - beginbucket;
        bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
        bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
        buckettotal += bucketval;
        pu32stat++;
        beginbucket += CK_BUCKETSIZE1B;
    }
    stat1 += buckettotal;
    printf("R1B T%d %u %u %u %lu\n", tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
    #endif
    rdtsc0 = _rdtsc();
    sipseed1(ctx, tid, 2);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER1 + 65536;
    beginbucket = CK_BUCKET1C - (CK_COUNTER1 + 65536);
    bucketmin = -1;
    bucketmax = 0;
    buckettotal = 0;
    for (i=0; i<8192; i++) {
        bucketval = *pu32stat - beginbucket;
        bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
        bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
        buckettotal += bucketval;
        pu32stat++;
        beginbucket += CK_BUCKETSIZE1C;
    }
    stat1 += buckettotal;
    printf("R1C T%d %u %u %u %lu\n", tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
    #endif
    rdtsc0 = _rdtsc();
    sipseed1(ctx, tid, 3);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER1 + 98304;
    beginbucket = CK_BUCKET1D - (CK_COUNTER1 + 98304);
    bucketmin = -1;
    bucketmax = 0;
    buckettotal = 0;
    for (i=0; i<8192; i++) {
        bucketval = *pu32stat - beginbucket;
        bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
        bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
        buckettotal += bucketval;
        pu32stat++;
        beginbucket += CK_BUCKETSIZE1D;
    }
    stat1 += buckettotal;
    printf("R1D T%d %u %u %u %lu\n", tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
    #endif
#if R1PHASES > 4
    rdtsc0 = _rdtsc();
    sipseed1(ctx, tid, 4);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER1 + 131072;
    beginbucket = CK_BUCKET1E - (CK_COUNTER1 + 131072);
    bucketmin = -1;
    bucketmax = 0;
    buckettotal = 0;
    for (i=0; i<8192; i++) {
        bucketval = *pu32stat - beginbucket;
        bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
        bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
        buckettotal += bucketval;
        pu32stat++;
        beginbucket += CK_BUCKETSIZE1E;
    }
    stat1 += buckettotal;
    printf("R1E T%d %u %u %u %lu\n", tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
    #endif
#endif
    //printf("%lu\n", stat1);
    rdtsc0 = _rdtsc();
    round2(ctx, tid);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER0;
    beginbucket = CK_BUCKET2 - CK_COUNTER0;
    bucketmin = -1;
    bucketmax = 0;
    buckettotal = 0;
    for (i=0; i<8192; i++) {
        bucketval = *pu32stat - beginbucket;
        bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
        bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
        buckettotal += bucketval;
        pu32stat++;
        beginbucket += CK_BUCKETSIZE2;
    }
    printf("R2 T%d %u %u %u %lu\n", tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
    #endif

    for (roundnum=3; roundnum<7; roundnum++) {
        rdtsc0 = _rdtsc();
        round3(ctx, tid);
        rdtsc1 = _rdtsc();
        pthread_barrier_wait(barrier);
        #if PRINTSTAT
        pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER1;
        beginbucket = CK_BUCKET3 - CK_COUNTER1;
        bucketmin = -1;
        bucketmax = 0;
        buckettotal = 0;
        for (i=0; i<8192; i++) {
            bucketval = *pu32stat - beginbucket;
            bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
            bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
            buckettotal += bucketval;
            pu32stat++;
            beginbucket += CK_BUCKETSIZE3;
        }
        printf("R%d T%d %u %u %u %lu\n", roundnum, tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
        #endif
        roundnum++;

        rdtsc0 = _rdtsc();
        round4(ctx, tid);
        rdtsc1 = _rdtsc();
        pthread_barrier_wait(barrier);
        #if PRINTSTAT
        pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER0;
        beginbucket = CK_BUCKET2 - CK_COUNTER0;
        bucketmin = -1;
        bucketmax = 0;
        buckettotal = 0;
        for (i=0; i<8192; i++) {
            bucketval = *pu32stat - beginbucket;
            bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
            bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
            buckettotal += bucketval;
            pu32stat++;
            beginbucket += CK_BUCKETSIZE2;
        }
        printf("R%d T%d %u %u %u %lu\n", roundnum, tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
        #endif
    }

    rdtsc0 = _rdtsc();
    round3(ctx, tid);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER1;
    beginbucket = CK_BUCKET3 - CK_COUNTER1;
    bucketmin = -1;
    bucketmax = 0;
    buckettotal = 0;
    for (i=0; i<8192; i++) {
        bucketval = *pu32stat - beginbucket;
        bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
        bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
        buckettotal += bucketval;
        pu32stat++;
        beginbucket += CK_BUCKETSIZE3;
    }
    printf("R%d T%d %u %u %u %lu\n", roundnum, tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
    #endif
    roundnum++;

    rdtsc0 = _rdtsc();
    round8(ctx, tid);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER0;
    beginbucket = CK_BUCKET8 - CK_COUNTER0;
    bucketmin = -1;
    bucketmax = 0;
    buckettotal = 0;
    for (i=0; i<8192; i++) {
        bucketval = *pu32stat - beginbucket;
        bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
        bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
        buckettotal += bucketval;
        pu32stat++;
        beginbucket += CK_BUCKETSIZE8;
    }
    printf("R%d T%d %u %u %u %lu\n", roundnum, tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
    #endif
    if (*pu32Abort)
        goto _AbortGraph;
    roundnum++;

    while (roundnum < 13) {
        rdtsc0 = _rdtsc();
        round9(ctx, tid);
        rdtsc1 = _rdtsc();
        pthread_barrier_wait(barrier);
        #if PRINTSTAT
        pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER1;
        beginbucket = CK_BUCKET9 - CK_COUNTER1;
        bucketmin = -1;
        bucketmax = 0;
        buckettotal = 0;
        for (i=0; i<8192; i++) {
            bucketval = *pu32stat - beginbucket;
            bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
            bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
            buckettotal += bucketval;
            pu32stat++;
            beginbucket += CK_BUCKETSIZE9;
        }
        printf("R%d T%d %u %u %u %lu\n", roundnum, tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
        #endif
        roundnum++;

        rdtsc0 = _rdtsc();
        round10(ctx, tid);
        rdtsc1 = _rdtsc();
        pthread_barrier_wait(barrier);
        #if PRINTSTAT
        pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER0;
        beginbucket = CK_BUCKET8 - CK_COUNTER0;
        bucketmin = -1;
        bucketmax = 0;
        buckettotal = 0;
        for (i=0; i<8192; i++) {
            bucketval = *pu32stat - beginbucket;
            bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
            bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
            buckettotal += bucketval;
            pu32stat++;
            beginbucket += CK_BUCKETSIZE8;
        }
        printf("R%d T%d %u %u %u %lu\n", roundnum, tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
        #endif
        roundnum++;
    }

    rdtsc0 = _rdtsc();
    round9(ctx, tid);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER1;
    beginbucket = CK_BUCKET9 - CK_COUNTER1;
    bucketmin = -1;
    bucketmax = 0;
    buckettotal = 0;
    for (i=0; i<8192; i++) {
        bucketval = *pu32stat - beginbucket;
        bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
        bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
        buckettotal += bucketval;
        pu32stat++;
        beginbucket += CK_BUCKETSIZE9;
    }
    printf("R%d T%d %u %u %u %lu\n", roundnum, tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
    #endif
    roundnum++;

    rdtsc0 = _rdtsc();
    round14(ctx, tid);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER0;
    beginbucket = CK_BUCKET14 - CK_COUNTER0;
    bucketmin = -1;
    bucketmax = 0;
    buckettotal = 0;
    for (i=0; i<8192; i++) {
        bucketval = *pu32stat - beginbucket;
        bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
        bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
        buckettotal += bucketval;
        pu32stat++;
        beginbucket += CK_BUCKETSIZE14;
    }
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_RENMAX0;
    printf("R%d T%d %u %u %u renmax=%u %lu\n", roundnum, tid, bucketmin, bucketmax, buckettotal, *pu32stat, rdtsc1-rdtsc0);
    #endif
    roundnum++;

    rdtsc0 = _rdtsc();
    round15(ctx, tid);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER1;
    beginbucket = CK_BUCKET15 - CK_COUNTER1;
    bucketmin = -1;
    bucketmax = 0;
    buckettotal = 0;
    for (i=0; i<256; i++) {
        bucketval = *pu32stat - beginbucket;
        bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
        bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
        buckettotal += bucketval;
        pu32stat++;
        beginbucket += CK_BUCKETSIZE15;
    }
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_RENMAX1;
    printf("R%d T%d %u %u %u renmax=%u %lu\n", roundnum, tid, bucketmin, bucketmax, buckettotal, *pu32stat, rdtsc1-rdtsc0);
    #endif
    roundnum++;

    rdtsc0 = _rdtsc();
    round16(ctx, tid);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER0;
    beginbucket = CK_BUCKET16 - CK_COUNTER0;
    bucketmin = -1;
    bucketmax = 0;
    buckettotal = 0;
    for (i=0; i<256; i++) {
        bucketval = *pu32stat - beginbucket;
        bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
        bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
        buckettotal += bucketval;
        pu32stat++;
        beginbucket += CK_BUCKETSIZE16;
    }
    printf("R%d T%d %u %u %u %lu\n", roundnum, tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
    #endif
    if (*pu32Abort)
        goto _AbortGraph;
    roundnum++;

    rdtsc0 = _rdtsc();
    round17(ctx, tid);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER1;
    beginbucket = CK_BUCKET15 - CK_COUNTER1;
    bucketmin = -1;
    bucketmax = 0;
    buckettotal = 0;
    for (i=0; i<256; i++) {
        bucketval = *pu32stat - beginbucket;
        bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
        bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
        buckettotal += bucketval;
        pu32stat++;
        beginbucket += CK_BUCKETSIZE15;
    }
    printf("R%d T%d %u %u %u %lu\n", roundnum, tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
    #endif
    roundnum++;

    rdtsc0 = _rdtsc();
    round18(ctx, tid);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER0;
    beginbucket = CK_BUCKET16 - CK_COUNTER0;
    bucketmin = -1;
    bucketmax = 0;
    buckettotal = 0;
    for (i=0; i<256; i++) {
        bucketval = *pu32stat - beginbucket;
        bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
        bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
        buckettotal += bucketval;
        pu32stat++;
        beginbucket += CK_BUCKETSIZE16;
    }
    printf("R%d T%d %u %u %u %lu\n", roundnum, tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
    #endif
    roundnum++;

    while (roundnum < 95) {
        round17(ctx, tid);
        pthread_barrier_wait(barrier);
        roundnum++;

        round18(ctx, tid);
        pthread_barrier_wait(barrier);
        roundnum++;
    }

    rdtsc0 = _rdtsc();
    round95(ctx, tid);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER1;
    beginbucket = CK_BUCKET95 - CK_COUNTER1;
    bucketmin = -1;
    bucketmax = 0;
    buckettotal = 0;
    for (i=0; i<1024; i++) {
        bucketval = *pu32stat - beginbucket;
        bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
        bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
        buckettotal += bucketval;
        pu32stat++;
        beginbucket += CK_BUCKETSIZE95;
    }
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_RENMAX2;
    printf("R%d T%d %u %u %u renmax=%u %lu\n", roundnum, tid, bucketmin, bucketmax, buckettotal, *pu32stat, rdtsc1-rdtsc0);
    #endif
    roundnum++;

    rdtsc0 = _rdtsc();
    round96(ctx, tid);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER0;
    beginbucket = CK_BUCKET96 - CK_COUNTER0;
    bucketmin = -1;
    bucketmax = 0;
    buckettotal = 0;
    for (i=0; i<128; i++) {
        bucketval = *pu32stat - beginbucket;
        bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
        bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
        buckettotal += bucketval;
        pu32stat++;
        beginbucket += CK_BUCKETSIZE96;
    }
    printf("R%d T%d %u %u %u %lu\n", roundnum, tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
    #endif
    roundnum++;

    rdtsc0 = _rdtsc();
    roundp1(ctx, tid);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + (CK_THREADSEP*tid) + CK_COUNTER1;
    beginbucket = CK_BUCKET97 - CK_COUNTER1;
    bucketmin = -1;
    bucketmax = 0;
    buckettotal = 0;
    for (i=0; i<128; i++) {
        bucketval = *pu32stat - beginbucket;
        bucketmin = (bucketval < bucketmin) ? bucketval : bucketmin;
        bucketmax = (bucketval > bucketmax) ? bucketval : bucketmax;
        buckettotal += bucketval;
        pu32stat++;
        beginbucket += CK_BUCKETSIZE97;
    }
    printf("Rp1 T%d %u %u %u %lu\n", tid, bucketmin, bucketmax, buckettotal, rdtsc1-rdtsc0);
    #endif

    rdtsc0 = _rdtsc();
    roundp2(ctx, tid);
    rdtsc1 = _rdtsc();
    pthread_barrier_wait(barrier);
    #if PRINTSTAT
    pu32stat = ctx + ((8388608/THREADS)*tid) + CK_LINKS;
    printf("Rp2 T%d %u %lu\n", tid, *pu32stat, rdtsc1-rdtsc0);
    #endif

    if (tid == 0) {
        rdtsc0 = _rdtsc();
        FindCycles(ctx);
        rdtsc1 = _rdtsc();
        printf("FindCylces rdtsc=%lu\n", rdtsc1-rdtsc0);
        if (*pu32solcount) {
            rdtsc0 = _rdtsc();
            Unrename(ctx);
            rdtsc1 = _rdtsc();
            uint32_t *pu32sol1 = ctx + 8;
            for (i=0; i < *pu32solcount; i++) {
                printf("Nodes ");
                for (int j=0; j<21; j++) {
                    printf("%x ", *pu32sol1);
                    pu32sol1+=2;
                }
                printf("\n");
            }
            printf("Unrename rdtsc=%lu\n", rdtsc1-rdtsc0);
        }
    }

    pthread_barrier_wait(barrier);
    if (*pu32solcount) {
        rdtsc0 = _rdtsc();
        siprecover(ctx, tid);
        rdtsc1 = _rdtsc();
        pthread_barrier_wait(barrier);
        printf("SipRecover T%d %lu\n", tid, rdtsc1-rdtsc0);
        if (tid==0) {
            siprecover2(ctx);
            for (uint32_t u0=0; u0<*pu32solcount; u0++) {
                qsort(pu32sol+u0*42, 42, 4, cmpfunc);
                printf("Solution ");
                for (uint32_t u1=0; u1<42; u1++) {
                    printf("%x ", pu32sol[u0*42+u1]);
                }
                printf("\n");
            }
        }
    }
_AbortGraph:
    return 0;
}

void cuckatoo31(void *ctx)
{
    pthread_t threads[THREADS];
    threaddata_t *threaddata = ctx + CK_THREADDATA;
    uint32_t tid;
    int32_t i;
    pthread_attr_t attr;
    cpu_set_t cpus;
    struct timespec tp0, tp1;
    uint32_t timems;

    for (i=0; i<THREADS; i++) {
        threaddata[i].ctx = ctx;
        threaddata[i].tid = i;
    }

    pthread_attr_init(&attr);
    clock_gettime(CLOCK_MONOTONIC, &tp0);
    for (tid=0; tid<THREADS; tid++) {
        CPU_ZERO(&cpus);
        CPU_SET(tid, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(threads+tid, &attr, &ThreadEntry, threaddata+tid);
    }
    for (tid=0; tid<THREADS; tid++) {
        pthread_join(threads[tid], 0);
    }
    clock_gettime(CLOCK_MONOTONIC, &tp1);
    timems = 1000*(tp1.tv_sec - tp0.tv_sec) + (tp1.tv_nsec - tp0.tv_nsec)/1000000;
    printf("Time: %d ms\n", timems);
}

int main(int argc, char **argv)
{
    void *ctx;
    void *barrier;
    int32_t i;
    uint32_t *pu32a;
    int32_t pagesize = 30;
    uint32_t nonce = 99;

    if (sizeof(pthread_barrier_t) > 64) {
        printf("sizeof(pthread_barrier_t)=%u > 64", (uint32_t) sizeof(pthread_barrier_t));
        exit(1);
    }

    for (i=1; i<argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            if ((i+1)<argc) {
                sscanf(argv[++i], "%u", &nonce);
            }
            continue;
        }
        if (strcmp(argv[i], "--p1g") == 0) {
            pagesize = 30;
            continue;
        }
        if (strcmp(argv[i], "--p2m") == 0) {
            pagesize = 21;
            continue;
        }
        if (strcmp(argv[i], "--p4k") == 0) {
            pagesize = 12;
            continue;
        }
    }
    printf("nonce = %u\n", nonce);

    if (pagesize == 30) {
        ctx = mmap(0, 11264*1048576UL, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | (30 << MAP_HUGE_SHIFT), -1, 0);
        if (ctx == MAP_FAILED) {
            printf("mmap failed\n");
            exit(1);
        }
        printf("1GiB page, ctx = %lx\n", (uint64_t) ctx);
        pu32a = ctx;
        for (i=0; i<11; i++) {
            *pu32a = 0;
            pu32a += (1073741824/4);
        }
    }
    else if (pagesize == 21) {
        ctx = mmap(0, 11264*1048576UL, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | (21 << MAP_HUGE_SHIFT), -1, 0);
        if (ctx == MAP_FAILED) {
            printf("mmap failed\n");
            exit(1);
        }
        printf("2MiB page, ctx = %lx\n", (uint64_t) ctx);
        pu32a = ctx;
        for (i=0; i<5632; i++) {
            *pu32a = 0;
            pu32a += (2097152/4);
        }
    }
    else {
        ctx = aligned_alloc(65536, 11264*1048576UL);
        if (ctx == 0) {
            printf("aligned_alloc failed\n");
            exit(1);
        }
        printf("4KiB page, ctx = %lx\n", (uint64_t) ctx);
        pu32a = ctx;
        for (i=0; i<2883584; i++) {
            *pu32a = 0;
            pu32a += (4096/4);
        }
    }

    memset(ctx + CK_BLAKE2BMSG, 0, 128);
    *(uint32_t *) (ctx+CK_BLAKE2BMSG+76) = nonce;
    *(__m256i *) (ctx+CK_BLAKE2BSTATE+64) = *(__m256i *) blake2biv;
    *(uint64_t *) (ctx+CK_BLAKE2BSTATE+96) = blake2biv[4] ^ 80;
    *(uint64_t *) (ctx+CK_BLAKE2BSTATE+104) = blake2biv[5];
    *(uint64_t *) (ctx+CK_BLAKE2BSTATE+112) = ~blake2biv[6];
    *(uint64_t *) (ctx+CK_BLAKE2BSTATE+120) = blake2biv[7];
    *(uint64_t *) (ctx+CK_BLAKE2BSTATE_INIT) = blake2biv[0] ^ 0x01010020;
    *(uint64_t *) (ctx+CK_BLAKE2BSTATE_INIT+8) = blake2biv[1];
    *(uint64_t *) (ctx+CK_BLAKE2BSTATE_INIT+16) = blake2biv[2];
    *(uint64_t *) (ctx+CK_BLAKE2BSTATE_INIT+24) = blake2biv[3];
    *(__m256i *) (ctx+CK_BLAKE2BSTATE_INIT+32) = *(__m256i *) (blake2biv+4);

    barrier = ctx + CK_BARRIER;
    pthread_barrier_init(barrier, 0, THREADS);

    cuckatoo31(ctx);

    printf("Normal exit\n");

    return 0;
}
