#include "memlayout.h"
.intel_syntax noprefix
.text
.align 64

//rdi = ctx
//rsi = tid

//rax
//rcx
//rdx
//rbx u32SrcBucket
//rbp rel32ReadStart, u32ItemMaskOr
//rsi pu32SrcCtr
//rdi u32NewNodeId
//r8  pRead
//r9  pReadEnd
//r10 pu32Bitmap
//r11 pu32DstCtr
//r12 pu16Ren2
//r13
//r14 rel32Write
//r15 fromthread

//A-type
.globl roundpr0
roundpr0:
push rbx
push rbp
push r12
push r13
push r14
push r15

imul eax, esi, CK_THREADSEP
lea r10, [rdi+rax+CK_BITMAP]
lea r12, [rdi+rax+CK_REN2]
mov ebx, esi
shl ebx, (7-LOGTHREADS)
#if THREADS < 4
    xor ebp, ebp
#else
    imul ebp, ebx, CK_BUCKETSIZE41
    add ebp, CK_BUCKET41-CK_COUNTER1
#endif
lea rsi, [rdi+CK_COUNTER1]
mov edi, 1

lea rax, [r10+CK_COUNTER0-CK_BITMAP]
mov r11, rax
vmovdqa ymm0, ymmword ptr yInitDest40[rip]
vpbroadcastd ymm1, dword ptr u32initbucket40inc[rip]

mov ecx, 16
_LoopInitDstCtr:
vmovdqa [rax], ymm0
vpaddd ymm0, ymm0, ymm1
add rax, 32
sub ecx, 1
jnz _LoopInitDstCtr

_LoopSrcBucket:
mov r15d, THREADS
_LoopCountFromThread:
#if THREADS < 4
    imul eax, ebx, CK_BUCKETSIZE41
    lea r8, [rsi+rax+CK_BUCKET41-CK_COUNTER1]
#else
    lea r8, [rsi+rbp]
#endif
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopCount:
mov rax, [r8]
add r8, 8
mov r13, rax
shr rax, 32+7
movzx ecx, byte ptr [r10+rax]
//0: drop, 1: new, 2: reuse
movzx edx, word ptr [r12+rax*2]
bt ecx, 0
cmovc edx, edi
adc edi, 0
mov [r12+rax*2], dx
add ecx, 1
and ecx, 2
mov [r10+rax], cl
mov eax, r13d
and r13d, 0x7f
mov r14d, [r11+r13*4]
#if THREADS < 4
    or edx, ebp
#endif
shl rdx, 32
or rax, rdx
mov [r11+r14], rax
lea r14d, [r14+rcx*4]
mov [r11+r13*4], r14d
cmp r8, r9
jb _LoopCount
add rsi, CK_THREADSEP
sub r15d, 1
jnz _LoopCountFromThread

add ebx, 1
add r10, 0x8000
add r12, 0x10000
mov eax, CK_THREADSEP*THREADS
sub rsi, rax
#if THREADS < 4
    test ebx, 0x1f
    jnz _LoopSrcBucket
    mov edi, 1
    add ebp, 0x10000
#else
    add ebp, CK_BUCKETSIZE41
#endif
test ebx, (128/THREADS-1)
jnz _LoopSrcBucket

mov [r11+CK_RENMAXP0-CK_COUNTER0], edi

pop r15
pop r14
pop r13
pop r12
pop rbp
pop rbx
ret
