#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>
#include <sys/mman.h>
#include <pthread.h>
#include <time.h>
#include "memlayout.h"

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
void round1(void *ctx, uint32_t tid, uint32_t phase);
void round2(void *ctx, uint32_t tid);
void round3(void *ctx, uint32_t tid);
void round4(void *ctx, uint32_t tid);
void round14(void *ctx, uint32_t tid);
void round15(void *ctx, uint32_t tid);
void round16(void *ctx, uint32_t tid);
void round17(void *ctx, uint32_t tid);
void round40(void *ctx, uint32_t tid);
void roundpinit(void *ctx, uint32_t tid);
void rounda0(void *ctx, uint32_t tid);
void roundb0(void *ctx, uint32_t tid);
void roundc0(void *ctx, uint32_t tid);
void rounda1(void *ctx, uint32_t tid);
void roundb1(void *ctx, uint32_t tid);
void roundc1(void *ctx, uint32_t tid);
void roundpr0(void *ctx, uint32_t tid);
void roundpr1(void *ctx, uint32_t tid);
void FindCycles(void *ctx);
void Unrename(void *ctx);
void siprecover(void *ctx, uint32_t tid);

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
    uint64_t q0, q1;
    int32_t roundnum;
    uint32_t numgraphs = *(uint32_t *) (ctx+CK_NUMGRAPHS);
    struct timespec tp0, tp1;
    uint32_t timems;

    while (numgraphs--) {
        if (tid == 0) {
            clock_gettime(CLOCK_MONOTONIC, &tp0);
            *(__m256i *) (ctx+CK_BLAKE2BSTATE) = *(__m256i *) (ctx+CK_BLAKE2BSTATE_INIT);
            *(__m256i *) (ctx+CK_BLAKE2BSTATE+32) = *(__m256i *) (ctx+CK_BLAKE2BSTATE_INIT+32);
            printf("nonce=%d\n", *(uint32_t *) (ctx+CK_BLAKE2BMSG+76));
            blake2b_compress(sipinit, ctx+CK_BLAKE2BMSG);
            q0 = sipinit[0];
            q1 = sipinit[1];
            //printf("nonce=%d (k0 k1 k2 k3)=(%016lx %016lx %016lx %016lx)\n", *(uint32_t *) (ctx+CK_BLAKE2BMSG+76), q0, q1, sipinit[2], sipinit[3]);
            q0 += q1;
            q1 = (q1<<13) | (q1>>51);
            q1 ^= q0;
            sipinit[0] = (q0<<32) | (q0>>32);
            sipinit[1] = q1;
            *(uint32_t *) (ctx + CK_ABORT) = 0;
            *pu32solcount = 0;
        }
        pthread_barrier_wait(barrier);

        sipseed0(ctx, tid);
        pthread_barrier_wait(barrier);
        round1(ctx, tid, 0);
        pthread_barrier_wait(barrier);
        round1(ctx, tid, 1);
        pthread_barrier_wait(barrier);
        round2(ctx, tid);
        pthread_barrier_wait(barrier);

        for (roundnum=3; roundnum<13; roundnum++) {
            round3(ctx, tid);
            pthread_barrier_wait(barrier);
            roundnum++;
            round4(ctx, tid);
            pthread_barrier_wait(barrier);
        }
        round3(ctx, tid);
        pthread_barrier_wait(barrier);
        roundnum++;

        round14(ctx, tid);
        pthread_barrier_wait(barrier);
        roundnum++;
        round15(ctx, tid);
        pthread_barrier_wait(barrier);
        roundnum++;

        while (roundnum < 40) {
            round16(ctx, tid);
            pthread_barrier_wait(barrier);
            roundnum++;
            round17(ctx, tid);
            pthread_barrier_wait(barrier);
            roundnum++;
        }

        round40(ctx, tid);
        pthread_barrier_wait(barrier);
        roundnum++;

        roundpinit(ctx, tid);
        pthread_barrier_wait(barrier);
        roundnum++;

        while (roundnum < 66) {
            rounda0(ctx, tid);
            pthread_barrier_wait(barrier);
            roundnum++;
            roundb1(ctx, tid);
            pthread_barrier_wait(barrier);
            roundnum++;
            roundc0(ctx, tid);
            pthread_barrier_wait(barrier);
            roundnum++;
            rounda1(ctx, tid);
            pthread_barrier_wait(barrier);
            roundnum++;
            roundb0(ctx, tid);
            pthread_barrier_wait(barrier);
            roundnum++;
            roundc1(ctx, tid);
            pthread_barrier_wait(barrier);
            roundnum++;
        }
        roundpr0(ctx, tid);
        pthread_barrier_wait(barrier);
        roundnum++;
        roundpr1(ctx, tid);
        pthread_barrier_wait(barrier);
        roundnum++;

        if (tid == 0) {
            FindCycles(ctx);
            if (*pu32solcount) {
                Unrename(ctx);
            }
        }

        pthread_barrier_wait(barrier);
        if (*pu32solcount) {
            siprecover(ctx, tid);
            pthread_barrier_wait(barrier);
            uint32_t *pu32sol = (ctx + 8);
            uint32_t u32NumSol = *(uint32_t *) (ctx);
            uint32_t u32NumTargets = u32NumSol * 21;
            uint32_t u32NumTargets2 = u32NumTargets * 2;

            uint32_t found, edge;
            uint32_t u0, u1, u2, u3, u1b;
            if (tid == 0) {
                for (u0=0; u0<u32NumSol; u0++) {
                    for (u1=u0*42+1; u1<=u0*42+41; u1+=2) {
                        u1b = (u1 == u0*42+41) ? u1-41 : u1+1;
                        found = 0;
                        uint32_t *pu32Header = ctx + CK_RECOVERWRITE;
                        uint32_t u32Begin = u32NumTargets2 + u1*2;
                        uint32_t u32End = pu32Header[u1];
                        for (u2=u32Begin; (!found) && (u2<u32End); u2+=u32NumTargets2*2) {
                            uint32_t target = pu32Header[u2];
                            uint32_t u32BeginInner = u32NumTargets2 + (u1b)*2;
                            uint32_t u32EndInner = pu32Header[u1b];
                            for (u3=u32BeginInner; (!found) && (u3<u32EndInner); u3+=u32NumTargets2*2) {
                                if (target == pu32Header[u3]) {
                                    edge = pu32Header[u2+1];
                                    edge = ((edge & 0x3f) == 0x3f) ? edge : edge - 0x40;
                                    pu32sol[u1] = edge;
                                    edge = pu32Header[u3+1];
                                    edge = ((edge & 0x3f) == 0x3f) ? edge : edge - 0x40;
                                    pu32sol[u1b] = edge;
                                    found = 1;
                                }
                            }
                        }
                        if (!found) {
                            printf("BUG: Post-siprecover cycle recovery failed\n");
                            break;
                        }
                    }
                }
                for (u0=0; u0<u32NumSol; u0++) {
                    qsort(pu32sol+u0*42, 42, 4, cmpfunc);
                    printf("Solution ");
                    for (u1=0; u1<42; u1++) {
                        printf("%x ", pu32sol[u0*42+u1]);
                    }
                    printf("\n");
                }
            }
        }
        if (tid == 0) {
            clock_gettime(CLOCK_MONOTONIC, &tp1);
            timems = 1000*(tp1.tv_sec - tp0.tv_sec) + (tp1.tv_nsec - tp0.tv_nsec)/1000000;
            printf("Time: %u ms\n", timems);
            *(uint32_t *) (ctx+CK_BLAKE2BMSG+76) += 1;    //Increment nonce
        }
    } //while numgraphs
    return 0;
}

void cuckarood29(void *ctx)
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
    uint32_t numgraphs = *(uint32_t *) (ctx+CK_NUMGRAPHS);
    printf("Total time: %u ms, %.3f gps\n", timems, 1000*numgraphs/(double) timems);
}

int main(int argc, char **argv)
{

    void *ctx;
    void *barrier;
    int32_t i;
    uint32_t *pu32a;
    int32_t pagesize = 30;
    uint32_t nonce = 0;
    uint32_t numgraphs = 21;

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
        if (strcmp(argv[i], "-r") == 0) {
            if ((i+1)<argc) {
                sscanf(argv[++i], "%u", &numgraphs);
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

    if (pagesize == 30) {
        ctx = mmap(0, 4096*1048576UL, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | (30 << MAP_HUGE_SHIFT), -1, 0);
        if (ctx == MAP_FAILED) {
            printf("mmap failed\n");
            exit(1);
        }
        printf("1GiB page, ctx = %lx\n", (uint64_t) ctx);
        pu32a = ctx;
        for (i=0; i<4; i++) {
            *pu32a = 0;
            pu32a += (1073741824/4);
        }
    }
    else if (pagesize == 21) {
        ctx = mmap(0, 4096*1048576UL, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | (21 << MAP_HUGE_SHIFT), -1, 0);
        if (ctx == MAP_FAILED) {
            printf("mmap failed\n");
            exit(1);
        }
        printf("2MiB page, ctx = %lx\n", (uint64_t) ctx);
        pu32a = ctx;
        for (i=0; i<2048; i++) {
            *pu32a = 0;
            pu32a += (2097152/4);
        }
    }
    else {
        ctx = aligned_alloc(65536, 4096*1048576UL);
        if (ctx == 0) {
            printf("aligned_alloc failed\n");
            exit(1);
        }
        printf("4KiB page, ctx = %lx\n", (uint64_t) ctx);
        pu32a = ctx;
        for (i=0; i<1048576; i++) {
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
    *(uint32_t *) (ctx+CK_NUMGRAPHS) = numgraphs;

    barrier = ctx + CK_BARRIER;
    pthread_barrier_init(barrier, 0, THREADS);

    cuckarood29(ctx);

    printf("Normal exit\n");
}
