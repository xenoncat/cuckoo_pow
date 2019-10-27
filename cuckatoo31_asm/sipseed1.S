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

.globl sipseed1
//rdi = ctx
//rsi = tid
//rdx = phase

//rax
//rcx fromthread
//rdx pu32DstCtr
//rbx i32SrcBucket
//rbp rel32ReadStart
//rsi pu32SrcCtr
//rdi
//r8  pRead
//r9  pReadEnd
//r10 pu32Bitmap
//r11
//r12 u64ItemMaskOr
//r13
//r14
//r15 pWrite

sipseed1:
push rbx
push rbp
push r12
push r13
push r14
push r15
mov rax, rsp
sub rsp, 0x90
and rsp, -64
mov [rsp+0x68], rax

mov eax, CK_THREADSEP
imul rax, rsi
lea r10, [rdi+rax+CK_BITMAP]
lea rax, PhaseLookup1[rip]
mov ebx, [rax+rdx*8]
mov [rsp+0x60], rbx
imul ebx, esi
add ebx, [rax+rdx*8+4]
imul ebp, ebx, CK_BUCKETSIZE0
add ebp, CK_BUCKET0-CK_COUNTER0+CK_BUCKETSIZE0
lea rsi, [rdi+CK_COUNTER0]
mov r12d, ebx
shl r12d, 18

vpbroadcastq ymm12, [rdi+CK_BLAKE2BSTATE]
vpbroadcastq ymm13, [rdi+CK_BLAKE2BSTATE+8]
vpbroadcastq ymm14, [rdi+CK_BLAKE2BSTATE+16]
vpbroadcastq ymm15, [rdi+CK_BLAKE2BSTATE+24]
vpbroadcastq ymm11, [rdi+CK_BLAKE2BSTATE+32]
vpbroadcastq ymm9, qconstff[rip]
vpbroadcastq ymm7, qconstbucketmask[rip]
vpbroadcastq ymm6, qconstbitmapmask[rip]
vpbroadcastq ymm5, qconst21mask[rip]
vmovdqa ymm10, ymmword ptr yconstrol16[rip]

lea rax, u32initbucket1inc[rip]
vpbroadcastd ymm1, dword ptr [rax+rdx*4]
shl edx, 5
vmovdqa ymm0, [rax+rdx+yInitDest1-u32initbucket1inc]
shl edx, 10
lea rax, [r10+rdx+CK_COUNTER1-CK_BITMAP]
mov rdx, rax
mov ecx, 1024
_LoopInitDstCtr:
vmovdqa [rax], ymm0
vpaddd ymm0, ymm0, ymm1
add rax, 32
sub ecx, 1
jnz _LoopInitDstCtr

_LoopSrcBucket:
mov rdi, r10
xor eax, eax
mov ecx, 0x1000
rep stosq

mov ecx, THREADS
_LoopCountSide1FromThread:
mov eax, [rsi+rbx*4]
add eax, 3
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax]
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopCountSide1:
mov eax, [r8]
add r8, 5
mov edi, eax
and eax, 0x1fff
mov r11d, [r10+rax*4]
shr edi, 13
bts r11d, edi
mov [r10+rax*4], r11d
cmp r8, r9
jb _LoopCountSide1
mov eax, CK_THREADSEP
add rsi, rax
sub ecx, 1
jnz _LoopCountSide1FromThread

sub ebp, CK_BUCKETSIZE0
mov ecx, THREADS
_LoopFilterSide0FromThread:
mov eax, CK_THREADSEP
sub rsi, rax
mov eax, [rsi+rbx*4-4]
add eax, 3
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax]
mov r15, r8
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopFilterSide0:
mov rax, [r8]
add r8, 5
mov [r15], rax
mov edi, eax
and eax, 0x1fff
mov eax, [r10+rax*4]
shr edi, 13
bt eax, edi
lea rax, [r15+5]
cmovc r15, rax
cmp r8, r9
jb _LoopFilterSide0
sub r15d, esi
mov eax, THREADS
sub eax, ecx
mov [rsp+rax*4+0x70], r15d
sub ecx, 1
jnz _LoopFilterSide0FromThread

vpxor xmm4, xmm4, xmm4
mov r14d, THREADS
_LoopDistributeSide0FromThread:
mov eax, [rsi+rbx*4-4]
add eax, 3
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax]
mov eax, [rsp+r14*4+0x6c]
lea r9, [rsi+rax]
vpbroadcastq ymm4, xmm4
jmp _EnterDistributeSide0
_LoopDistributeSide0:
vmovdqa [rsp], ymm2
vmovdqa [rsp+0x20], ymm1
vmovd eax, xmm2
mov edi, [rdx+rax*4]
vmovq [rdx+rdi], xmm1
add edi, 5
mov [rdx+rax*4], edi
vmovdqa [rsp+0x40], ymm0
vmovd eax, xmm0
mov ecx, eax
and eax, 0x1fff
mov edi, [r10+rax*4]
shr ecx, 13
btr edi, ecx
mov [r10+rax*4], edi

mov eax, [rsp+0x08]
mov edi, [rdx+rax*4]
mov rcx, [rsp+0x28]
mov [rdx+rdi], rcx
add edi, 5
mov [rdx+rax*4], edi
mov eax, [rsp+0x48]
mov ecx, eax
and eax, 0x1fff
mov edi, [r10+rax*4]
shr ecx, 13
btr edi, ecx
mov [r10+rax*4], edi

mov eax, [rsp+0x10]
mov edi, [rdx+rax*4]
mov rcx, [rsp+0x30]
mov [rdx+rdi], rcx
add edi, 5
mov [rdx+rax*4], edi
mov eax, [rsp+0x50]
mov ecx, eax
and eax, 0x1fff
mov edi, [r10+rax*4]
shr ecx, 13
btr edi, ecx
mov [r10+rax*4], edi

mov eax, [rsp+0x18]
mov edi, [rdx+rax*4]
mov rcx, [rsp+0x38]
mov [rdx+rdi], rcx
add edi, 5
mov [rdx+rax*4], edi
mov eax, [rsp+0x58]
mov ecx, eax
and eax, 0x1fff
mov edi, [r10+rax*4]
shr ecx, 13
btr edi, ecx
mov [r10+rax*4], edi

_EnterDistributeSide0:
vmovq xmm0, [r8+2]
vpinsrq xmm0, xmm0, [r8+12], 1
vpsllq xmm1, xmm0, 8
vpblendd xmm0, xmm0, xmm1, 0x5
vpsrld xmm0, xmm0, 10
vpmovzxdq ymm0, xmm0
vpsubq ymm0, ymm0, ymm4
vpand ymm0, ymm0, ymm5
vpaddq ymm4, ymm4, ymm0

SIPBEGIN
SIPROUND
vpxor ymm0, ymm4, ymm0
vpxor ymm2, ymm9, ymm2
SIPROUND
SIPROUND
SIPROUND
SIPEND

vmovq xmm0, [r8]
vpinsrq xmm0, xmm0, [r8+10], 1
vpsrlq xmm2, xmm0, 8
vpblendd xmm0, xmm2, xmm0, 0x5
vmovq xmm2, r12
vpmovzxdq ymm0, xmm0
vpbroadcastq ymm2, xmm2
vpand ymm0, ymm6, ymm0
vpor ymm3, ymm0, ymm2
vpsllq ymm3, ymm3, 18
vpand ymm2, ymm7, ymm1
vpsrlq ymm1, ymm1, 13
vpand ymm1, ymm6, ymm1
vpor ymm1, ymm1, ymm3
add r8, 20
cmp r8, r9
jb _LoopDistributeSide0

sub r9d, r8d
add r9d, 20

vmovdqa [rsp], ymm2
vmovdqa [rsp+0x20], ymm1
vmovd eax, xmm2
mov edi, [rdx+rax*4]
vmovq [rdx+rdi], xmm1
add edi, 5
mov [rdx+rax*4], edi
vmovdqa [rsp+0x40], ymm0
vmovd eax, xmm0
mov ecx, eax
and eax, 0x1fff
mov edi, [r10+rax*4]
shr ecx, 13
btr edi, ecx
mov [r10+rax*4], edi
sub r9d, 5
jz _DoneDistributeSide0

mov eax, [rsp+0x08]
mov edi, [rdx+rax*4]
mov rcx, [rsp+0x28]
mov [rdx+rdi], rcx
add edi, 5
mov [rdx+rax*4], edi
mov eax, [rsp+0x48]
mov ecx, eax
and eax, 0x1fff
mov edi, [r10+rax*4]
shr ecx, 13
btr edi, ecx
mov [r10+rax*4], edi
sub r9d, 5
jz _DoneDistributeSide0

mov eax, [rsp+0x10]
mov edi, [rdx+rax*4]
mov rcx, [rsp+0x30]
mov [rdx+rdi], rcx
add edi, 5
mov [rdx+rax*4], edi
mov eax, [rsp+0x50]
mov ecx, eax
and eax, 0x1fff
mov edi, [r10+rax*4]
shr ecx, 13
btr edi, ecx
mov [r10+rax*4], edi
sub r9d, 5
jz _DoneDistributeSide0

mov eax, [rsp+0x18]
mov edi, [rdx+rax*4]
mov rcx, [rsp+0x38]
mov [rdx+rdi], rcx
add edi, 5
mov [rdx+rax*4], edi
mov eax, [rsp+0x58]
mov ecx, eax
and eax, 0x1fff
mov edi, [r10+rax*4]
shr ecx, 13
btr edi, ecx
mov [r10+rax*4], edi

_DoneDistributeSide0:
mov eax, CK_THREADSEP
add rsi, rax
sub r14d, 1
jnz _LoopDistributeSide0FromThread
vmovd eax, xmm4
and eax, 0xffc00001
cmp eax, 0xffc00000
jne _DensityError

mov rax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE0
add r12d, 0x40000
vpxor xmm4, xmm4, xmm4
mov r11d, THREADS
_LoopTrimSide1FromThread:
mov eax, [rsi+rbx*4]
add eax, 3
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax]
mov r15, r8
mov r14, r8
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopFilterSide1:
mov rax, [r8]
add r8, 5
mov [r15], rax
mov edi, eax
and eax, 0x1fff
mov eax, [r10+rax*4]
shr edi, 13
bt eax, edi
lea rax, [r15+5]
cmovnc r15, rax
cmp r8, r9
jb _LoopFilterSide1
vpbroadcastq ymm4, xmm4
jmp _EnterDistributeSide1
_LoopDistributeSide1:
vmovdqa [rsp], ymm2
vmovdqa [rsp+0x20], ymm1
vmovd eax, xmm2
mov edi, [rdx+rax*4]
vmovq [rdx+rdi], xmm1
add edi, 5
mov [rdx+rax*4], edi

mov eax, [rsp+0x08]
mov edi, [rdx+rax*4]
mov rcx, [rsp+0x28]
mov [rdx+rdi], rcx
add edi, 5
mov [rdx+rax*4], edi

mov eax, [rsp+0x10]
mov edi, [rdx+rax*4]
mov rcx, [rsp+0x30]
mov [rdx+rdi], rcx
add edi, 5
mov [rdx+rax*4], edi

mov eax, [rsp+0x18]
mov edi, [rdx+rax*4]
mov rcx, [rsp+0x38]
mov [rdx+rdi], rcx
add edi, 5
mov [rdx+rax*4], edi

_EnterDistributeSide1:
vmovq xmm0, [r14+2]
vpinsrq xmm0, xmm0, [r14+12], 1
vpsllq xmm1, xmm0, 8
vpblendd xmm0, xmm0, xmm1, 0x5
vpsrld xmm0, xmm0, 10
vpmovzxdq ymm0, xmm0
vpsubq ymm0, ymm0, ymm4
vpand ymm0, ymm0, ymm5
vpaddq ymm4, ymm4, ymm0

SIPBEGIN
SIPROUND
vpxor ymm0, ymm4, ymm0
vpxor ymm2, ymm9, ymm2
SIPROUND
SIPROUND
SIPROUND
SIPEND

vmovq xmm0, [r14]
vpinsrq xmm0, xmm0, [r14+10], 1
vpsrlq xmm2, xmm0, 8
vpblendd xmm0, xmm2, xmm0, 0x5
vmovq xmm2, r12
vpmovzxdq ymm0, xmm0
vpbroadcastq ymm2, xmm2
vpand ymm0, ymm6, ymm0
vpor ymm3, ymm0, ymm2
vpsllq ymm3, ymm3, 18
vpand ymm2, ymm7, ymm1
vpsrlq ymm1, ymm1, 13
vpand ymm1, ymm6, ymm1
vpor ymm1, ymm1, ymm3
add r14, 20
cmp r14, r15
jb _LoopDistributeSide1

sub r15d, r14d
add r15d, 20

vmovdqa [rsp], ymm2
vmovdqa [rsp+0x20], ymm1
vmovd eax, xmm2
mov edi, [rdx+rax*4]
vmovq [rdx+rdi], xmm1
add edi, 5
mov [rdx+rax*4], edi
sub r15d, 5
jz _DoneDistributeSide1

mov eax, [rsp+0x08]
mov edi, [rdx+rax*4]
mov rcx, [rsp+0x28]
mov [rdx+rdi], rcx
add edi, 5
mov [rdx+rax*4], edi
sub r15d, 5
jz _DoneDistributeSide1

mov eax, [rsp+0x10]
mov edi, [rdx+rax*4]
mov rcx, [rsp+0x30]
mov [rdx+rdi], rcx
add edi, 5
mov [rdx+rax*4], edi
sub r15d, 5
jz _DoneDistributeSide1

mov eax, [rsp+0x18]
mov edi, [rdx+rax*4]
mov rcx, [rsp+0x38]
mov [rdx+rdi], rcx
add edi, 5
mov [rdx+rax*4], edi

_DoneDistributeSide1:
mov eax, CK_THREADSEP
add rsi, rax
sub r11d, 1
jnz _LoopTrimSide1FromThread
vmovd eax, xmm4
and eax, 0xffc00001
cmp eax, 0xffc00000
jne _DensityError

mov rax, CK_THREADSEP*THREADS
sub rsi, rax
add ebx, 2
add ebp, CK_BUCKETSIZE0*2
add r12d, 0x40000
sub dword ptr [rsp+0x60], 2
jnz _LoopSrcBucket

_Epilog:
mov rsp, [rsp+0x68]
pop r15
pop r14
pop r13
pop r12
pop rbp
pop rbx
ret

_DensityError:
lea rdi, szDebug[rip]
mov esi, ebx
vmovd edx, xmm4
xor eax, eax
call printf@PLT
//ud2
jmp _Epilog

.section .rodata.cst32,"aM",@progbits,32
.align 64
yInitDest1:
.set a, CK_BUCKET1A-CK_COUNTER1
.int a, a+CK_BUCKETSIZE1A, a+CK_BUCKETSIZE1A*2, a+CK_BUCKETSIZE1A*3
.int a+CK_BUCKETSIZE1A*4, a+CK_BUCKETSIZE1A*5, a+CK_BUCKETSIZE1A*6, a+CK_BUCKETSIZE1A*7
.set a, CK_BUCKET1B-(CK_COUNTER1+32768)
.int a, a+CK_BUCKETSIZE1B, a+CK_BUCKETSIZE1B*2, a+CK_BUCKETSIZE1B*3
.int a+CK_BUCKETSIZE1B*4, a+CK_BUCKETSIZE1B*5, a+CK_BUCKETSIZE1B*6, a+CK_BUCKETSIZE1B*7
.set a, CK_BUCKET1C-(CK_COUNTER1+65536)
.int a, a+CK_BUCKETSIZE1C, a+CK_BUCKETSIZE1C*2, a+CK_BUCKETSIZE1C*3
.int a+CK_BUCKETSIZE1C*4, a+CK_BUCKETSIZE1C*5, a+CK_BUCKETSIZE1C*6, a+CK_BUCKETSIZE1C*7
.set a, CK_BUCKET1D-(CK_COUNTER1+98304)
.int a, a+CK_BUCKETSIZE1D, a+CK_BUCKETSIZE1D*2, a+CK_BUCKETSIZE1D*3
.int a+CK_BUCKETSIZE1D*4, a+CK_BUCKETSIZE1D*5, a+CK_BUCKETSIZE1D*6, a+CK_BUCKETSIZE1D*7
#if R1PHASES>4
.set a, CK_BUCKET1E-(CK_COUNTER1+131072)
.int a, a+CK_BUCKETSIZE1E, a+CK_BUCKETSIZE1E*2, a+CK_BUCKETSIZE1E*3
.int a+CK_BUCKETSIZE1E*4, a+CK_BUCKETSIZE1E*5, a+CK_BUCKETSIZE1E*6, a+CK_BUCKETSIZE1E*7
#endif

qconst21mask:
.quad 0x3ffffe
PhaseLookup1:
//must be even number
#if THREADS == 4
.int 240, 0, 366, 960, 568, 2424, 874, 4696
#elif THREADS == 8
.int 108, 0, 160, 864, 240, 2144, 258, 4064, 258, 6128
#endif
u32initbucket1inc:
.int CK_BUCKETSIZE1A*8, CK_BUCKETSIZE1B*8, CK_BUCKETSIZE1C*8, CK_BUCKETSIZE1D*8
#if R1PHASES>4
.int CK_BUCKETSIZE1E*8
#endif
szDebug:
.string "%lx %lx\n"
