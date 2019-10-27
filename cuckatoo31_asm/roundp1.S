#include "memlayout.h"
.intel_syntax noprefix
.text
.align 64

//rdi = ctx
//rsi = tid

//rax
//rcx
//rdx u32LinkId
//rbx u32SrcBucket
//rbp rel32ReadStart
//rsi
//rdi links
//r8  pRead
//r9  pReadEnd
//r10 pu32SrcCtr
//r11 const
//r12 pu32DstCtr
//r13 fromthread
//r14
//r15 const

.globl roundp1
roundp1:
push rbx
push rbp
push r12
push r13
push r14
push r15

mov eax, CK_THREADSEP
mov r15d, eax
imul rax, rsi
lea rax, [rdi+rax+CK_COUNTER1]
mov r12, rax
vmovdqa ymm0, ymmword ptr yInitDest97[rip]
vpbroadcastd ymm1, dword ptr u32initbucket97inc[rip]
mov ecx, 16
_LoopInitDstCtr:
vmovdqa [rax], ymm0
vpaddd ymm0, ymm0, ymm1
add rax, 32
sub ecx, 1
jnz _LoopInitDstCtr

mov ebx, esi
shl ebx, (7-LOGTHREADS)
imul ebp, ebx, CK_BUCKETSIZE96
add ebp, CK_BUCKET96-CK_COUNTER0
lea r10, [rdi+CK_COUNTER0]
add rdi, CK_LINKS
mov edx, esi
shl edx, (20-LOGTHREADS)
push rdx
add edx, 1
lea r11, [r15*THREADS]

_LoopSrcBucket:
mov r13d, THREADS
_LoopPopulateLinksFromThread:
lea r8, [r10+rbp]
mov eax, [r10+rbx*4]
lea r9, [r10+rax]
_LoopPopulateLinks:
mov rax, [r8]
add r8, 8
btr rax, 63
mov rsi, rax
shr rax, 32
mov ecx, eax
and ecx, 0x7f
mov r14d, [r12+rcx*4]
mov [r12+r14], rsi
add r14d, 8
mov [r12+rcx*4], r14d
mov esi, esi
mov ecx, [rdi+rsi*4+CK_ADJLIST-CK_LINKS]
mov [rdi+rdx*8], ecx
mov [rdi+rsi*4+CK_ADJLIST-CK_LINKS], edx
xor eax, 1
mov [rdi+rdx*8+4], eax
add edx, 1
cmp r8, r9
jb _LoopPopulateLinks
add r10, r15
sub r13d, 1
jnz _LoopPopulateLinksFromThread

add ebx, 1
sub r10, r11
add ebp, CK_BUCKETSIZE96
test ebx, (128/THREADS-1)
jnz _LoopSrcBucket

pop rax
mov [rdi+rax*8], rdx

pop r15
pop r14
pop r13
pop r12
pop rbp
pop rbx
ret

.section .rodata.cst32,"aM",@progbits,32
.align 64
.set a, CK_BUCKET97-CK_COUNTER1
yInitDest97:
.int a, a+CK_BUCKETSIZE97, a+CK_BUCKETSIZE97*2, a+CK_BUCKETSIZE97*3
.int a+CK_BUCKETSIZE97*4, a+CK_BUCKETSIZE97*5, a+CK_BUCKETSIZE97*6, a+CK_BUCKETSIZE97*7
u32initbucket97inc:
.int CK_BUCKETSIZE97*8
