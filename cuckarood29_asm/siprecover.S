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

.byte 0x0f, 0x1f, 0x84, 0x00, 0, 0, 0, 0
.globl siprecover
//rdi = ctx
//rsi = tid
siprecover:
//rax
//rcx inner loop counter
//rdx pu32BitmapFilter0
//rbx u32NumTargets2
//rbp
//rsi pScratch
//rdi pu32Header
//r11 outer loop counter
//r12 pScratchBegin
push rbx
push rbp
push r12
push r15
mov rax, rsp
sub rsp, 72
and rsp, -64
mov [rsp+64], rax

vpbroadcastq ymm12, [rdi+CK_BLAKE2BSTATE]
vpbroadcastq ymm13, [rdi+CK_BLAKE2BSTATE+8]
vpbroadcastq ymm14, [rdi+CK_BLAKE2BSTATE+16]
vpbroadcastq ymm15, [rdi+CK_BLAKE2BSTATE+24]

imul ebx, [rdi], 42
imul eax, esi, CK_THREADSEP
lea r12, [rdi+rax+CK_SIPSCRATCH]
lea rdx, [rdi+CK_BITMAP]
add rdi, CK_RECOVERWRITE

shl esi, (29-LOGTHREADS)
vmovd xmm4, esi
vpbroadcastq ymm4, xmm4
vpaddq ymm4, ymm4, ymmword ptr yinit[rip]
vpbroadcastq ymm5, qconst1[rip]
vpbroadcastq ymm9, qconstff[rip]
vpbroadcastq ymm11, qconstfilteraddrmask[rip]
vmovdqa ymm10, ymmword ptr yconstrol16[rip]
mov r15, 0x0fffffff0fffffff

SIPBEGIN0
mov rsi, r12
mov ecx, 64
jmp _EnterLoopFirstSip

_LoopFirstSipInner:
SIPEND1
vmovdqa [rsi], ymm8
add rsi, 32
vpaddq ymm4, ymm4, ymm5
SIPINNER0
_EnterLoopFirstSip:
SIPBEGIN1
SIPROUND
vpxor ymm0, ymm4, ymm0
vpxor ymm2, ymm9, ymm2
SIPROUND
SIPROUND
SIPROUND
SIPEND0
sub ecx, 1
jnz _LoopFirstSipInner

mov r11d, (2097152 / THREADS)
jmp _EnterLoopOuterSip

//Outer loop starts here
_LoopSipInner:
SIPEND1

vpxor ymm7, ymm6, [rsi]
vmovdqa [rsi], ymm8
add rsi, 32
vpsrlq ymm8, ymm7, 5
vpand ymm8, ymm11, ymm8
vmovdqa [rsp], ymm8
vmovdqa [rsp+0x20], ymm7
vmovd eax, xmm8
vmovq rbp, xmm7
mov eax, [rdx+rax*4]
bt eax, ebp
jnc _NoA0
vmovd r9d, xmm4
call _ProcAddEdge
_NoA0:
mov eax, [rsp+0x08]
mov rbp, [rsp+0x28]
mov eax, [rdx+rax*4]
bt eax, ebp
jnc _NoA1
vmovd r9d, xmm4
add r9d, 1 << (27-LOGTHREADS)
call _ProcAddEdge
_NoA1:
mov eax, [rsp+0x10]
mov rbp, [rsp+0x30]
mov eax, [rdx+rax*4]
bt eax, ebp
jnc _NoA2
vmovd r9d, xmm4
add r9d, 2 << (27-LOGTHREADS)
call _ProcAddEdge
_NoA2:
mov eax, [rsp+0x18]
mov rbp, [rsp+0x38]
mov eax, [rdx+rax*4]
bt eax, ebp
jnc _NoA3
vmovd r9d, xmm4
add r9d, 3 << (27-LOGTHREADS)
call _ProcAddEdge
_NoA3:

vpaddq ymm4, ymm4, ymm5
SIPINNER0
_EnterLoopSip:
SIPBEGIN1
SIPROUND
vpxor ymm0, ymm4, ymm0
vpxor ymm2, ymm9, ymm2
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
vpsrlq ymm7, ymm6, 5
vpand ymm7, ymm11, ymm7
vmovdqa [rsp], ymm7
vmovdqa [rsp+0x20], ymm6
vmovd eax, xmm7
vmovq rbp, xmm6
mov eax, [rdx+rax*4]
bt eax, ebp
jnc _NoB0
vmovd r9d, xmm4
call _ProcAddEdge
_NoB0:
mov eax, [rsp+0x08]
mov rbp, [rsp+0x28]
mov eax, [rdx+rax*4]
bt eax, ebp
jnc _NoB1
vmovd r9d, xmm4
add r9d, 1 << (27-LOGTHREADS)
call _ProcAddEdge
_NoB1:
mov eax, [rsp+0x10]
mov rbp, [rsp+0x30]
mov eax, [rdx+rax*4]
bt eax, ebp
jnc _NoB2
vmovd r9d, xmm4
add r9d, 2 << (27-LOGTHREADS)
call _ProcAddEdge
_NoB2:
mov eax, [rsp+0x18]
mov rbp, [rsp+0x38]
mov eax, [rdx+rax*4]
bt eax, ebp
jnc _NoB3
vmovd r9d, xmm4
add r9d, 3 << (27-LOGTHREADS)
call _ProcAddEdge
_NoB3:

vpaddq ymm4, ymm4, ymm5
SIPBEGIN0
mov rsi, r12
mov ecx, 64
sub r11d, 1
jnz _EnterLoopSip
//End of outer loop

mov ecx, 63
_LoopWrite:
vpxor ymm7, ymm6, [rsi]
add rsi, 32
vpsrlq ymm0, ymm7, 5
vpand ymm0, ymm11, ymm0
vmovdqa [rsp], ymm0
vmovdqa [rsp+0x20], ymm7
vmovd eax, xmm0
vmovq rbp, xmm7
mov eax, [rdx+rax*4]
bt eax, ebp
jnc _NoC0
vmovd r9d, xmm4
call _ProcAddEdge
_NoC0:
mov eax, [rsp+0x08]
mov rbp, [rsp+0x28]
mov eax, [rdx+rax*4]
bt eax, ebp
jnc _NoC1
vmovd r9d, xmm4
add r9d, 1 << (27-LOGTHREADS)
call _ProcAddEdge
_NoC1:
mov eax, [rsp+0x10]
mov rbp, [rsp+0x30]
mov eax, [rdx+rax*4]
bt eax, ebp
jnc _NoC2
vmovd r9d, xmm4
add r9d, 2 << (27-LOGTHREADS)
call _ProcAddEdge
_NoC2:
mov eax, [rsp+0x18]
mov rbp, [rsp+0x38]
mov eax, [rdx+rax*4]
bt eax, ebp
jnc _NoC3
vmovd r9d, xmm4
add r9d, 3 << (27-LOGTHREADS)
call _ProcAddEdge
_NoC3:

vpaddq ymm4, ymm4, ymm5
sub ecx, 1
jnz _LoopWrite

_Epilog:
mov rsp, [rsp+64]
pop r15
pop r12
pop rbp
pop rbx
ret

.align 64
_ProcAddEdge:
//rbp NodePair
//r9d Edge
and rbp, r15
mov eax, ebp
shr eax, 15
mov eax, [rdx+rax*4+0x8000]
bt eax, ebp
jc _FilterOK
ret
_FilterOK:
mov r8, rbp
xor ebp, ebp
//and r8, r15
mov r10, r8
shr r10, 32
_LoopMatchTarget:
cmp r8d, [rdx+rbp*4+8-CK_BITMAP]
je _FoundTarget
add ebp, 2
cmp ebp, ebx
jb _LoopMatchTarget
ret
_FoundTarget:
mov eax, r9d
and eax, 1
or ebp, eax
lea eax, [rbx+rbx]
lock xadd [rdi+rbp*4], eax
mov [rdi+rax*4], r10d
mov [rdi+rax*4+4], r9d
and ebp, -2
add ebp, 2
cmp ebp, ebx
jb _LoopMatchTarget
ret

.section .rodata.cst32,"aM",@progbits,32
.align 64
qconstfilteraddrmask:
.quad 0x1fff
