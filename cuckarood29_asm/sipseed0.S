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
vpsllq ymm8, ymm3, 25
vpsrlq ymm3, ymm3, 39
vpxor ymm8, ymm8, ymm0
vpshufd ymm2, ymm2, 0xb1
vpxor ymm3, ymm8, ymm3
.endm

.macro SIPBEGIN0
vpxor ymm3, ymm15, ymm4
vpaddq ymm2, ymm14, ymm3
vpshufb ymm3, ymm3, ymm10
vpxor ymm3, ymm3, ymm2
vpaddq ymm2, ymm13, ymm2
vpaddq ymm0, ymm12, ymm3
vpsllq ymm8, ymm13, 17
vpsrlq ymm1, ymm13, 47
vpxor ymm8, ymm8, ymm2
vpxor ymm1, ymm8, ymm1
.endm

.macro SIPBEGIN1
vpsllq ymm8, ymm3, 25
vpsrlq ymm3, ymm3, 39
vpxor ymm8, ymm8, ymm0
vpshufd ymm2, ymm2, 0xb1
vpxor ymm3, ymm8, ymm3
.endm

.macro SIPEND0
vpaddq ymm0, ymm0, ymm1
vpsllq ymm8, ymm1, 13
vpsrlq ymm1, ymm1, 51
vpxor ymm8, ymm8, ymm0
vpaddq ymm2, ymm2, ymm3
vpshufb ymm3, ymm3, ymm10
vpxor ymm1, ymm8, ymm1
vpxor ymm3, ymm3, ymm2
.endm

.macro SIPEND1
vpshufd ymm0, ymm0, 0xb1
vpaddq ymm2, ymm2, ymm1
vpsllq ymm8, ymm1, 17
vpsrlq ymm1, ymm1, 47
vpxor ymm8, ymm8, ymm2
vpaddq ymm0, ymm0, ymm3
vpxor ymm1, ymm8, ymm1
vpsllq ymm8, ymm3, 25
vpsrlq ymm3, ymm3, 39
vpxor ymm8, ymm8, ymm3
vpshufd ymm2, ymm2, 0xb1
vpxor ymm3, ymm8, ymm0
vpxor ymm8, ymm8, ymm1
vpxor ymm8, ymm8, ymm2
.endm

.macro SIPINNER0
vpxor ymm3, ymm4, ymm3
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
.endm

.globl sipseed0
//rdi = ctx
//rsi = tid
sipseed0:
//rax
//rcx inner loop counter
//rdx ptr dest counters
//rbx
//rbp outer loop counter
//rsi ptr SIP scratchpad
//rdi ptr dest counters, may have xor 4 applied
push rbx
push rbp
mov rax, rsp
sub rsp, 72
and rsp, -64
mov [rsp+64], rax

vpbroadcastq ymm12, [rdi+CK_BLAKE2BSTATE]
vpbroadcastq ymm13, [rdi+CK_BLAKE2BSTATE+8]
vpbroadcastq ymm14, [rdi+CK_BLAKE2BSTATE+16]
vpbroadcastq ymm15, [rdi+CK_BLAKE2BSTATE+24]
	
imul eax, esi, CK_THREADSEP
lea rax, [rdi+rax+CK_COUNTER0]
mov rdi, rax
mov rdx, rax
mov dword ptr [rax-4], 0
mov dword ptr [rax+CK_COUNTER1-CK_COUNTER0-4], 0
vmovdqa ymm0, ymmword ptr yinitbucket0[rip]
vpbroadcastd ymm1, u32initbucket0inc[rip]
mov ecx, 256
_LoopInitBucket:
vmovdqa [rax], ymm0
vpaddd ymm0, ymm0, ymm1
add rax, 32
sub ecx, 1
jnz _LoopInitBucket

shl esi, (29-LOGTHREADS)
vmovd xmm4, esi
vpbroadcastq ymm4, xmm4
vpaddq ymm4, ymm4, ymmword ptr yinit[rip]
vpbroadcastq ymm5, qconst1[rip]
vpbroadcastq ymm9, qconstff[rip]
vpbroadcastq ymm11, qconstbucketmask[rip]
vmovdqa ymm10, ymmword ptr yconstrol16[rip]

SIPBEGIN0
lea rsi, [rdx+CK_SIPSCRATCH-CK_COUNTER0]
mov ecx, 64
jmp _EnterLoopFirstSip

_LoopFirstSipInner:
SIPEND1
vmovdqa [rsi], ymm8
add rsi, 32
SIPINNER0
_EnterLoopFirstSip:
SIPBEGIN1
SIPROUND
vpxor ymm0, ymm4, ymm0
vpxor ymm2, ymm9, ymm2
vpaddq ymm4, ymm4, ymm5
SIPROUND
SIPROUND
SIPROUND
SIPEND0
sub ecx, 1
jnz _LoopFirstSipInner

mov ebp, (2097152 / THREADS)
jmp _EnterLoopOuterSip

//Outer loop starts here
_LoopSipInner:
SIPEND1
//begin write
xor rdi, 4
vpxor ymm7, ymm6, [rsi]
vmovdqa [rsi], ymm8
add rsi, 32
vpslld ymm8, ymm7, 4
vpblendd ymm8, ymm7, ymm8, 0x55
vpand ymm7, ymm11, ymm7
vpsrlq ymm8, ymm8, 14
vmovdqa [rsp], ymm7
vmovdqa [rsp+0x20], ymm8
vmovd eax, xmm7
mov ebx, [rdi+rax*8]
vmovq [rdx+rbx], xmm8
add ebx, 6
mov [rdi+rax*8], ebx

mov eax, [rsp+0x08]
mov r8, [rsp+0x28]
mov ebx, [rdi+rax*8]
mov [rdx+rbx], r8
add ebx, 6
mov [rdi+rax*8], ebx

mov eax, [rsp+0x10]
mov r8, [rsp+0x30]
mov ebx, [rdi+rax*8]
mov [rdx+rbx], r8
add ebx, 6
mov [rdi+rax*8], ebx

mov eax, [rsp+0x18]
mov r8, [rsp+0x38]
mov ebx, [rdi+rax*8]
mov [rdx+rbx], r8
add ebx, 6
mov [rdi+rax*8], ebx
//end write

SIPINNER0
_EnterLoopSip:
SIPBEGIN1
SIPROUND
vpxor ymm0, ymm4, ymm0
vpxor ymm2, ymm9, ymm2
vpaddq ymm4, ymm4, ymm5
SIPROUND
SIPROUND
SIPROUND
SIPEND0
sub ecx, 1
jnz _LoopSipInner

_EnterLoopOuterSip:
vpaddq ymm2, ymm2, ymm1
vpsllq ymm8, ymm1, 17
vpsrlq ymm1, ymm1, 47
vpxor ymm8, ymm8, ymm2
vpxor ymm1, ymm8, ymm1
vpsllq ymm8, ymm3, 25
vpsrlq ymm3, ymm3, 39
vpxor ymm8, ymm8, ymm3
vpshufd ymm2, ymm2, 0xb1
vpxor ymm8, ymm8, ymm1
vpxor ymm6, ymm8, ymm2

//begin write
xor rdi, 4
vpslld ymm0, ymm6, 4
vpblendd ymm0, ymm6, ymm0, 0x55
vpand ymm7, ymm11, ymm6
vpsrlq ymm0, ymm0, 14
vmovdqa [rsp], ymm7
vmovdqa [rsp+0x20], ymm0
vmovd eax, xmm7
mov ebx, [rdi+rax*8]
vmovq [rdx+rbx], xmm0
add ebx, 6
mov [rdi+rax*8], ebx

mov eax, [rsp+0x08]
mov r8, [rsp+0x28]
mov ebx, [rdi+rax*8]
mov [rdx+rbx], r8
add ebx, 6
mov [rdi+rax*8], ebx

mov eax, [rsp+0x10]
mov r8, [rsp+0x30]
mov ebx, [rdi+rax*8]
mov [rdx+rbx], r8
add ebx, 6
mov [rdi+rax*8], ebx

mov eax, [rsp+0x18]
mov r8, [rsp+0x38]
mov ebx, [rdi+rax*8]
mov [rdx+rbx], r8
add ebx, 6
mov [rdi+rax*8], ebx
//end write

SIPBEGIN0
lea rsi, [rdx+CK_SIPSCRATCH-CK_COUNTER0]
mov ecx, 64
sub ebp, 1
jnz _EnterLoopSip
//End of outer loop

mov ecx, 63
_LoopWrite:
xor rdi, 4
vpxor ymm1, ymm6, [rsi]
add rsi, 32
vpslld ymm0, ymm1, 4
vpblendd ymm0, ymm1, ymm0, 0x55
vpand ymm1, ymm11, ymm1
vpsrlq ymm0, ymm0, 14
vmovdqa [rsp], ymm1
vmovdqa [rsp+0x20], ymm0
vmovd eax, xmm1
mov ebx, [rdi+rax*8]
vmovq [rdx+rbx], xmm0
add ebx, 6
mov [rdi+rax*8], ebx

mov eax, [rsp+0x08]
mov r8, [rsp+0x28]
mov ebx, [rdi+rax*8]
mov [rdx+rbx], r8
add ebx, 6
mov [rdi+rax*8], ebx

mov eax, [rsp+0x10]
mov r8, [rsp+0x30]
mov ebx, [rdi+rax*8]
mov [rdx+rbx], r8
add ebx, 6
mov [rdi+rax*8], ebx

mov eax, [rsp+0x18]
mov r8, [rsp+0x38]
mov ebx, [rdi+rax*8]
mov [rdx+rbx], r8
add ebx, 6
mov [rdi+rax*8], ebx

sub ecx, 1
jnz _LoopWrite

mov rsp, [rsp+64]
pop rbp
pop rbx
ret

.section .rodata.cst32,"aM",@progbits,32
.align 64
.globl yinit
yinit:
.quad 0, 1 << (27-LOGTHREADS), 2 << (27-LOGTHREADS), 3 << (27-LOGTHREADS)
.globl yconstrol16
yconstrol16:
.quad 0x0504030201000706, 0x0d0c0b0a09080f0e, 0x0504030201000706, 0x0d0c0b0a09080f0e
.set a, CK_BUCKET0-CK_COUNTER0
yinitbucket0:
.int a, a+CK_BUCKETSIZE0, a+CK_BUCKETSIZE0*2, a+CK_BUCKETSIZE0*3
.int a+CK_BUCKETSIZE0*4, a+CK_BUCKETSIZE0*5, a+CK_BUCKETSIZE0*6, a+CK_BUCKETSIZE0*7
.globl qconst1
qconst1:
.quad 1
.globl qconstff
qconstff:
.quad 0xff
.globl qconstbucketmask
qconstbucketmask:
.quad 0x3ff
u32initbucket0inc:
.int CK_BUCKETSIZE0*8
