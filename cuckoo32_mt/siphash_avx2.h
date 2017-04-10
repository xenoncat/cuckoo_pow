#define SIP_ROUND \
    t0 = _mm256_slli_epi64(v1, 13);\
    v0 = _mm256_add_epi64(v0, v1);\
    v1 = _mm256_srli_epi64(v1, 51);\
    v1 = _mm256_or_si256(v1, t0);\
    v2 = _mm256_add_epi64(v2, v3);\
    v3 = _mm256_shuffle_epi8(v3, yconstrol16);\
    v1 = _mm256_xor_si256(v1, v0);\
    t0 = _mm256_slli_epi64(v1, 17);\
    v3 = _mm256_xor_si256(v3, v2);\
    v0 = _mm256_shuffle_epi32(v0, 0xb1);\
    v2 = _mm256_add_epi64(v2, v1);\
    v1 = _mm256_srli_epi64(v1, 47);\
    v1 = _mm256_or_si256(v1, t0);\
    t0 = _mm256_slli_epi64(v3, 21);\
    v0 = _mm256_add_epi64(v0, v3);\
    v3 = _mm256_srli_epi64(v3, 43);\
    v3 = _mm256_or_si256(v3, t0);\
    v1 = _mm256_xor_si256(v1, v2);\
    v3 = _mm256_xor_si256(v3, v0);\
    v2 = _mm256_shuffle_epi32(v2, 0xb1)

#define FIRST_SIP_ROUND \
    v2 = _mm256_add_epi64(in2, v3);\
    v3 = _mm256_shuffle_epi8(v3, yconstrol16);\
    v3 = _mm256_xor_si256(v3, v2);\
    v2 = _mm256_add_epi64(v2, in1a);\
    t0 = _mm256_slli_epi64(v3, 21);\
    v0 = _mm256_add_epi64(in0, v3);\
    v3 = _mm256_srli_epi64(v3, 43);\
    v3 = _mm256_or_si256(v3, t0);\
    v1 = _mm256_xor_si256(in1, v2);\
    v3 = _mm256_xor_si256(v3, v0);\
    v2 = _mm256_shuffle_epi32(v2, 0xb1)

#define SIPHASH \
    v3 = _mm256_xor_si256(in3, nonce);\
    FIRST_SIP_ROUND;\
    SIP_ROUND;\
    v0 = _mm256_xor_si256(v0, nonce);\
    v2 = _mm256_xor_si256(v2, yconstff);\
    SIP_ROUND;\
    SIP_ROUND;\
    SIP_ROUND;\
    SIP_ROUND;\
    v0 = _mm256_xor_si256(v0, v1);\
    v2 = _mm256_xor_si256(v2, v3);\
    v0 = _mm256_xor_si256(v0, v2);
