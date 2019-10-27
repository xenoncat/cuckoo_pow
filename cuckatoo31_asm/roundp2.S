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
//r11 linkid of unused link
//r12
//r13 fromthread
//r14 const
//r15 const

.globl roundp2
roundp2:
push rbx
push rbp
push r12
push r13
push r14
push r15

mov ebx, esi
shl ebx, (7-LOGTHREADS)
imul ebp, ebx, CK_BUCKETSIZE97
add ebp, CK_BUCKET97-CK_COUNTER1
lea r10, [rdi+CK_COUNTER1]
add rdi, CK_LINKS
mov edx, esi
shl edx, (20-LOGTHREADS)
mov r11d, edx
mov edx, [rdi+rdx*8]
mov r15d, CK_THREADSEP
lea r14, [r15*THREADS]

_LoopSrcBucket:
mov r13d, THREADS
_LoopPopulateLinksFromThread:
lea r8, [r10+rbp]
mov eax, [r10+rbx*4]
lea r9, [r10+rax]
_LoopPopulateLinks:
mov rax, [r8]
add r8, 8
mov esi, eax
shr rax, 32
mov ecx, [rdi+rax*4+CK_ADJLIST-CK_LINKS]
mov [rdi+rdx*8], ecx
mov [rdi+rax*4+CK_ADJLIST-CK_LINKS], edx
xor esi, 1
mov [rdi+rdx*8+4], esi
add edx, 1
cmp r8, r9
jb _LoopPopulateLinks
add r10, r15
sub r13d, 1
jnz _LoopPopulateLinksFromThread

add ebx, 1
sub r10, r14
add ebp, CK_BUCKETSIZE97
test ebx, (128/THREADS-1)
jnz _LoopSrcBucket

mov eax, 0xffffffff
mov [rdi+rdx*8], rax
mov [rdi+r11*8], rdx

pop r15
pop r14
pop r13
pop r12
pop rbp
pop rbx
ret
