#include "memlayout.h"
.intel_syntax noprefix
.text
.align 64

.macro SIPROUND
vpaddq ymm0, ymm0, ymm1
vpsllq ymm8, ymm1, 13
vpsrlq ymm1, ymm1, 51
vpxor ymm8, ymm8, ymm0
vpaddq ymm2, ymm2, ymm3
vpshufb ymm3, ymm3, ymm10
vpxor ymm1, ymm8, ymm1
vpxor ymm3, ymm3, ymm2
vpshufd ymm0, ymm0, 0xb1
vpaddq ymm2, ymm2, ymm1
vpsllq ymm8, ymm1, 17
vpsrlq ymm1, ymm1, 47
vpxor ymm8, ymm8, ymm2
vpaddq ymm0, ymm0, ymm3
vpxor ymm1, ymm8, ymm1
vpsllq ymm8, ymm3, 21
vpsrlq ymm3, ymm3, 43
vpxor ymm8, ymm8, ymm0
vpshufd ymm2, ymm2, 0xb1
vpxor ymm3, ymm8, ymm3
.endm

.macro SIPBEGIN
vpxor ymm3, ymm15, ymm4
vpaddq ymm2, ymm14, ymm3
vpshufb ymm3, ymm3, ymm10
vpxor ymm3, ymm3, ymm2
vpaddq ymm2, ymm13, ymm2
vpaddq ymm0, ymm12, ymm3
vpxor ymm1, ymm11, ymm2
vpshufd ymm2, ymm2, 0xb1
vpsllq ymm8, ymm3, 21
vpsrlq ymm3, ymm3, 43
vpxor ymm8, ymm8, ymm0
vpxor ymm3, ymm8, ymm3
.endm

.macro SIPEND
vpaddq ymm0, ymm0, ymm1
vpsllq ymm8, ymm1, 13
vpsrlq ymm1, ymm1, 51
vpxor ymm8, ymm8, ymm0
vpaddq ymm2, ymm2, ymm3
vpshufb ymm3, ymm3, ymm10
vpxor ymm1, ymm8, ymm1
vpxor ymm3, ymm3, ymm2
vpaddq ymm2, ymm2, ymm1
vpsllq ymm8, ymm1, 17
vpsrlq ymm1, ymm1, 47
vpxor ymm8, ymm8, ymm2
vpxor ymm1, ymm8, ymm1
vpsllq ymm8, ymm3, 21
vpsrlq ymm3, ymm3, 43
vpshufd ymm2, ymm2, 0xb1
vpxor ymm3, ymm8, ymm3
vpxor ymm1, ymm1, ymm2
vpxor ymm1, ymm1, ymm3
.endm

.globl sipseed0
//rdi = ctx
//rsi = tid
sipseed0:
//rax
//rcx loop counter
//rdx
//rbx
//rbp
//rsi
//rdi ptr dest counters
//ymm4 nonce
//ymm5 nonce bump
//ymm6 qconstbitmapmask
//ymm7 qconstbucketmask
//ymm8 temp for rotate
//ymm9 constff
//ymm10 constrol16

mov rax, rsp
sub rsp, 72
and rsp, -64
mov [rsp+64], rax

vpbroadcastq ymm12, [rdi+CK_BLAKE2BSTATE]
vpbroadcastq ymm13, [rdi+CK_BLAKE2BSTATE+8]
vpbroadcastq ymm14, [rdi+CK_BLAKE2BSTATE+16]
vpbroadcastq ymm15, [rdi+CK_BLAKE2BSTATE+24]
vpbroadcastq ymm11, [rdi+CK_BLAKE2BSTATE+32]

mov eax, CK_THREADSEP
imul rax, rsi
lea rax, [rdi+rax+CK_COUNTER0]
mov rdi, rax
mov dword ptr [rax+CK_COUNTER1-CK_COUNTER0-4], 0
mov dword ptr [rax-4], 0
vmovdqa ymm0, ymmword ptr yinitbucket0[rip]
vpbroadcastd ymm1, u32initbucket0inc[rip]
mov ecx, 1024
_LoopInitBucket:
vmovdqa [rax], ymm0
vpaddd ymm0, ymm0, ymm1
add rax, 32
sub ecx, 1
jnz _LoopInitBucket

shl esi, (32-LOGTHREADS)
vmovd xmm4, esi
vpbroadcastq ymm4, xmm4
vpaddq ymm4, ymm4, ymmword ptr yinit[rip]
vpbroadcastq ymm5, qconst8[rip]
vpbroadcastq ymm9, qconstff[rip]
vpbroadcastq ymm7, qconstbucketmask[rip]
vpbroadcastq ymm6, qconstbitmapmask[rip]
vmovdqa ymm10, ymmword ptr yconstrol16[rip]

mov ecx, (536870912/THREADS)
_LoopSip:
SIPBEGIN
SIPROUND
vpxor ymm0, ymm4, ymm0
vpxor ymm2, ymm9, ymm2
SIPROUND
SIPROUND
SIPROUND
SIPEND

//21edge, 1x, 18v
vpsllq ymm0, ymm4, 18
vpand ymm2, ymm7, ymm1
vpsrlq ymm1, ymm1, 13
vpand ymm1, ymm6, ymm1
vpor ymm0, ymm0, ymm1
vpaddq ymm4, ymm4, ymm5

vmovdqa [rsp], ymm2
vmovdqa [rsp+0x20], ymm0
vmovd eax, xmm2
mov edx, [rdi+rax*4]
vmovq [rdi+rdx], xmm0
add edx, 5
mov [rdi+rax*4], edx

mov eax, [rsp+0x08]
mov rsi, [rsp+0x28]
mov edx, [rdi+rax*4]
mov [rdi+rdx], rsi
add edx, 5
mov [rdi+rax*4], edx

mov eax, [rsp+0x10]
mov rsi, [rsp+0x30]
mov edx, [rdi+rax*4]
mov [rdi+rdx], rsi
add edx, 5
mov [rdi+rax*4], edx

mov eax, [rsp+0x18]
mov rsi, [rsp+0x38]
mov edx, [rdi+rax*4]
mov [rdi+rdx], rsi
add edx, 5
mov [rdi+rax*4], edx

sub ecx, 1
jnz _LoopSip

mov rsp, [rsp+64]
ret

.section .rodata.cst32,"aM",@progbits,32
.align 64
.globl yinit
yinit:
.quad 1, 3, 5, 7
.globl yconstrol16
yconstrol16:
.quad 0x0504030201000706, 0x0d0c0b0a09080f0e, 0x0504030201000706, 0x0d0c0b0a09080f0e
.set a, CK_BUCKET0-CK_COUNTER0
.globl yinitbucket0
yinitbucket0:
.int a, a+CK_BUCKETSIZE0, a+CK_BUCKETSIZE0*2, a+CK_BUCKETSIZE0*3
.int a+CK_BUCKETSIZE0*4, a+CK_BUCKETSIZE0*5, a+CK_BUCKETSIZE0*6, a+CK_BUCKETSIZE0*7
.globl qconst8
qconst8:
.quad 8
.globl qconstff
qconstff:
.quad 0xff
.globl qconstbucketmask
qconstbucketmask:
.quad 0x1fff
.globl qconstbitmapmask
qconstbitmapmask:
.quad 0x3ffff
.globl u32initbucket0inc
u32initbucket0inc:
.int CK_BUCKETSIZE0*8
