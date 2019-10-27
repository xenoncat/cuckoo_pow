#include <stdint.h>
#include <x86intrin.h>

uint8_t blake2sigma[] = {
    0,2,4,6,1,3,5,7,8,10,12,14,9,11,13,15,
    14,4,9,13,10,8,15,6,1,0,11,5,12,2,7,3,
    11,12,5,15,8,0,2,13,10,3,7,9,14,6,1,4,
    7,3,13,11,9,1,12,14,2,5,4,15,6,10,0,8,
    9,5,2,10,0,7,4,15,14,11,6,3,1,12,8,13,
    2,6,0,8,12,10,11,3,4,7,15,1,13,5,14,9,
    12,1,14,4,5,15,13,10,0,6,9,8,7,3,2,11,
    13,7,12,3,11,14,1,9,5,15,8,2,0,4,6,10,
    6,14,11,0,15,9,3,8,12,13,1,10,2,7,4,5,
    10,8,7,1,2,4,6,5,15,9,3,13,11,14,12,0,
    0,2,4,6,1,3,5,7,8,10,12,14,9,11,13,15,
    14,4,9,13,10,8,15,6,1,0,11,5,12,2,7,3
};

void blake2b_compress(__m256i *state, uint64_t *msg)
{
    const __m256i yconstror16 = _mm256_set_epi64x(0x09080f0e0d0c0b0a, 0x0100070605040302, 0x09080f0e0d0c0b0a, 0x0100070605040302);
    const __m256i yconstror24 = _mm256_set_epi64x(0x0a09080f0e0d0c0b, 0x0201000706050403, 0x0a09080f0e0d0c0b, 0x0201000706050403);
    uint64_t m[16] __attribute__((aligned (64)));
    __m256i v0 = state[0];
    __m256i v1 = state[1];
    __m256i v2 = state[2];
    __m256i v3 = state[3];
    __m256i v4;

    for (int i=0; i<12; i++) {
        for (int j=0; j<16; j++) {
            m[j] = msg[blake2sigma[i*16+j]];
        }
        v0 = _mm256_add_epi64(v0, v1);
        v0 = _mm256_add_epi64(v0, ((__m256i *) m)[0]);
        v3 = _mm256_xor_si256(v3, v0);
        v3 = _mm256_shuffle_epi32(v3, 0xb1);
        v2 = _mm256_add_epi64(v2, v3);
        v1 = _mm256_xor_si256(v1, v2);
        v1 = _mm256_shuffle_epi8(v1, yconstror24);
        v0 = _mm256_add_epi64(v0, v1);
        v0 = _mm256_add_epi64(v0, ((__m256i *) m)[1]);
        v3 = _mm256_xor_si256(v3, v0);
        v3 = _mm256_shuffle_epi8(v3, yconstror16);
        v2 = _mm256_add_epi64(v2, v3);
        v1 = _mm256_xor_si256(v1, v2);
        v4 = _mm256_add_epi64(v1, v1);
        v1 = _mm256_srli_epi64(v1, 63);
        v1 = _mm256_xor_si256(v4, v1);

        v1 = _mm256_permute4x64_epi64(v1, 0x39);
        v2 = _mm256_permute4x64_epi64(v2, 0x4e);
        v3 = _mm256_permute4x64_epi64(v3, 0x93);

        v0 = _mm256_add_epi64(v0, v1);
        v0 = _mm256_add_epi64(v0, ((__m256i *) m)[2]);
        v3 = _mm256_xor_si256(v3, v0);
        v3 = _mm256_shuffle_epi32(v3, 0xb1);
        v2 = _mm256_add_epi64(v2, v3);
        v1 = _mm256_xor_si256(v1, v2);
        v1 = _mm256_shuffle_epi8(v1, yconstror24);
        v0 = _mm256_add_epi64(v0, v1);
        v0 = _mm256_add_epi64(v0, ((__m256i *) m)[3]);
        v3 = _mm256_xor_si256(v3, v0);
        v3 = _mm256_shuffle_epi8(v3, yconstror16);
        v2 = _mm256_add_epi64(v2, v3);
        v1 = _mm256_xor_si256(v1, v2);
        v4 = _mm256_add_epi64(v1, v1);
        v1 = _mm256_srli_epi64(v1, 63);
        v1 = _mm256_xor_si256(v4, v1);

        v1 = _mm256_permute4x64_epi64(v1, 0x93);
        v2 = _mm256_permute4x64_epi64(v2, 0x4e);
        v3 = _mm256_permute4x64_epi64(v3, 0x39);
    }
    v0 = _mm256_xor_si256(v0, v2);
    v1 = _mm256_xor_si256(v1, v3);
    state[0] = _mm256_xor_si256(v0, state[0]);
    state[1] = _mm256_xor_si256(v1, state[1]);
}
