#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>
#include "memlayout.h"

#define SIPROUND \
    v0 = _mm256_add_epi64(v0, v1); \
    vt = _mm256_slli_epi64(v1, 13); \
    v1 = _mm256_srli_epi64(v1, 51); \
    vt = _mm256_xor_si256(vt, v0); \
    v2 = _mm256_add_epi64(v2, v3); \
    v3 = _mm256_shuffle_epi8(v3, yconstrol16); \
    v1 = _mm256_xor_si256(vt, v1); \
    v3 = _mm256_xor_si256(v3, v2); \
    v0 = _mm256_shuffle_epi32(v0, 0xb1); \
    v2 = _mm256_add_epi64(v2, v1); \
    vt = _mm256_slli_epi64(v1, 17); \
    v1 = _mm256_srli_epi64(v1, 47); \
    vt = _mm256_xor_si256(vt, v2); \
    v0 = _mm256_add_epi64(v0, v3); \
    v1 = _mm256_xor_si256(vt, v1); \
    vt = _mm256_slli_epi64(v3, 21); \
    v3 = _mm256_srli_epi64(v3, 43); \
    vt = _mm256_xor_si256(vt, v0); \
    v2 = _mm256_shuffle_epi32(v2, 0xb1); \
    v3 = _mm256_xor_si256(vt, v3);

#define SIPBEGIN \
    v3 = _mm256_xor_si256(vi3, vnonce); \
    v2 = _mm256_add_epi64(vi2, v3); \
    v3 = _mm256_shuffle_epi8(v3, yconstrol16); \
    v3 = _mm256_xor_si256(v3, v2); \
    v2 = _mm256_add_epi64(vi1, v2); \
    v0 = _mm256_add_epi64(vi0, v3); \
    v1 = _mm256_xor_si256(vi4, v2); \
    v2 = _mm256_shuffle_epi32(v2, 0xb1); \
    vt = _mm256_slli_epi64(v3, 21); \
    v3 = _mm256_srli_epi64(v3, 43); \
    vt = _mm256_xor_si256(vt, v0); \
    v3 = _mm256_xor_si256(vt, v3);

#define SIPEND \
    v0 = _mm256_add_epi64(v0, v1); \
    vt = _mm256_slli_epi64(v1, 13); \
    v1 = _mm256_srli_epi64(v1, 51); \
    vt = _mm256_xor_si256(vt, v0); \
    v2 = _mm256_add_epi64(v2, v3); \
    v3 = _mm256_shuffle_epi8(v3, yconstrol16); \
    v1 = _mm256_xor_si256(vt, v1); \
    v3 = _mm256_xor_si256(v3, v2); \
    v2 = _mm256_add_epi64(v2, v1); \
    vt = _mm256_slli_epi64(v1, 17); \
    v1 = _mm256_srli_epi64(v1, 47); \
    vt = _mm256_xor_si256(vt, v2); \
    v1 = _mm256_xor_si256(vt, v1); \
    vt = _mm256_slli_epi64(v3, 21); \
    v3 = _mm256_srli_epi64(v3, 43); \
    v2 = _mm256_shuffle_epi32(v2, 0xb1); \
    v3 = _mm256_xor_si256(vt, v3); \
    v1 = _mm256_xor_si256(v1, v2); \
    v1 = _mm256_xor_si256(v1, v3);

void siprecover2(void *ctx)
{
    uint32_t u32NumSols = *(uint32_t *) ctx;
    const __m256i yconstff = _mm256_set_epi64x(0xff, 0xff, 0xff, 0xff);
    const __m256i yconstrol16 = _mm256_set_epi64x(0x0d0c0b0a09080f0e, 0x0504030201000706, 0x0d0c0b0a09080f0e, 0x0504030201000706);
    const uint64_t u64init0 = *(uint64_t*) (ctx+CK_BLAKE2BSTATE);
    const uint64_t u64init1 = *(uint64_t*) (ctx+CK_BLAKE2BSTATE+8);
    const uint64_t u64init2 = *(uint64_t*) (ctx+CK_BLAKE2BSTATE+16);
    const uint64_t u64init3 = *(uint64_t*) (ctx+CK_BLAKE2BSTATE+24);
    const uint64_t u64init4 = *(uint64_t*) (ctx+CK_BLAKE2BSTATE+32);
    const __m256i vi0 = _mm256_set_epi64x(u64init0, u64init0, u64init0, u64init0);
    const __m256i vi1 = _mm256_set_epi64x(u64init1, u64init1, u64init1, u64init1);
    const __m256i vi2 = _mm256_set_epi64x(u64init2, u64init2, u64init2, u64init2);
    const __m256i vi3 = _mm256_set_epi64x(u64init3, u64init3, u64init3, u64init3);
    const __m256i vi4 = _mm256_set_epi64x(u64init4, u64init4, u64init4, u64init4);
    __m256i v0, v1, v2, v3, vt, vnonce;

    int32_t *pi32Counter = ctx + CK_RECOVERWRITE;
    int32_t i32CounterMax = 0;
    uint32_t u32NumSols42 = u32NumSols * 42;
    int32_t offset = (u32NumSols42 * 2 - 2);
    for (uint32_t i=0; i<u32NumSols42; i++) {
        int32_t val = pi32Counter[i] - offset;
        i32CounterMax = (val > i32CounterMax) ? val : i32CounterMax;
    }
    __m256i *p256Ptr = ctx + CK_RECOVERWRITE + 1024;
    void *pEnd = ctx + CK_RECOVERWRITE + (i32CounterMax * 4);

    do {
        vnonce = *p256Ptr;
        vnonce = _mm256_slli_epi64(vnonce, 32);
        vnonce = _mm256_srli_epi64(vnonce, 31);
        SIPBEGIN
        SIPROUND
        v0 = _mm256_xor_si256(vnonce, v0);
        v2 = _mm256_xor_si256(yconstff, v2);
        SIPROUND
        SIPROUND
        SIPROUND
        SIPEND
        v1 = _mm256_slli_epi64(v1, 33);
        v1 = _mm256_or_si256(v1, vnonce);
        v1 = _mm256_srli_epi64(v1, 1);
        *p256Ptr = v1;
        p256Ptr++;
    } while ((void *) p256Ptr < pEnd);

    uint32_t *pu32sol = (ctx + 8);
    uint32_t u32NumSols84 = u32NumSols * 84;
    uint32_t found, u0, u1, u1b, u2, u3, ulimit0, ulimit1;
    /*for (u0=0; u0<u32NumSols; u0++) {
        for (u1=u0*42; u1<(u0*42+42); u1+=1) {
            ulimit0 = pi32Counter[u1];
            printf("[%d] %d: ", u1, ulimit0);
            for (u2=u1*2+256; u2<ulimit0; u2+=u32NumSols84) {
                printf("%8x %8x, ", pi32Counter[u2], pi32Counter[u2+1]);
            }
            puts("");
        }
    }*/

    for (u0=0; u0<u32NumSols; u0++) {
        u1=u0*42+40;
        u1b=u0*42+1;
        ulimit0 = pi32Counter[u1];
        found = 0;
        for (u2=u1*2+256; (!found) && (u2<ulimit0); u2+=u32NumSols84) {
            uint32_t target = pi32Counter[u2+1] ^ 1;
            ulimit1 = pi32Counter[u1b];
            for (u3=u1b*2+256; (!found) && (u3<ulimit1); u3+=u32NumSols84) {
                if (target == pi32Counter[u3+1]) {
                    pu32sol[u1] = pi32Counter[u2];
                    pu32sol[u1b] = pi32Counter[u3];
                    found = 1;
                }
            }
        }
        if (found) {
            printf("debug info 1\n");
            for (u1=u0*42; u1<u0*42+40; u1+=2) {
                u1b=u1+3;
                ulimit0 = pi32Counter[u1];
                found = 0;
                for (u2=u1*2+256; (!found) && (u2<ulimit0); u2+=u32NumSols84) {
                    uint32_t target = pi32Counter[u2+1] ^ 1;
                    ulimit1 = pi32Counter[u1b];
                    for (u3=u1b*2+256; (!found) && (u3<ulimit1); u3+=u32NumSols84) {
                        if (target == pi32Counter[u3+1]) {
                            pu32sol[u1] = pi32Counter[u2];
                            pu32sol[u1b] = pi32Counter[u3];
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
        else {
            printf("debug info 2\n");
            for (u1=u0*42+1; u1<=u0*42+41; u1+=2) {
                u1b = (u1 == u0*42+41) ? u1-41 : u1+1;
                ulimit0 = pi32Counter[u1];
                found = 0;
                for (u2=u1*2+256; (!found) && (u2<ulimit0); u2+=u32NumSols84) {
                    uint32_t target = pi32Counter[u2+1] ^ 1;
                    ulimit1 = pi32Counter[u1b];
                    for (u3=u1b*2+256; (!found) && (u3<ulimit1); u3+=u32NumSols84) {
                        if (target == pi32Counter[u3+1]) {
                            pu32sol[u1] = pi32Counter[u2];
                            pu32sol[u1b] = pi32Counter[u3];
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
    }
}
