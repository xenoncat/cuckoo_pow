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
//rsi pu32SrcCtr
//rdi
//r8  pRead
//r9  pReadEnd
//r10 pu8Bitmap
//r11 u32NodeIdHigh
//r12 pu16Ren2
//r13 links
//r14
//r15

//B-type
.globl roundpr1
roundpr1:
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
imul ebp, ebx, CK_BUCKETSIZE40
add ebp, CK_BUCKET40-CK_COUNTER0
mov edx, esi
lea rsi, [rdi+CK_COUNTER0]
lea r13, [rdi+CK_LINKS]
shl edx, (18-LOGTHREADS)
mov r11d, edx

lea rdi, [rdi+rdx*4+CK_ADJLIST]
xor eax, eax
mov ecx, (131072/THREADS)
rep stosq

mov qword ptr [r13+rdx*8], 0
add edx, 1

_LoopSrcBucket:
xor edi, edi
_LoopCountFromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopCount:
mov rax, [r8]
add r8, 8
mov r14, rax
shr eax, 7
test byte ptr [r10+rax], 0x01
jnz _Skip
movzx eax, word ptr [r12+rax*2]
or eax, r11d
mov [r13+rax*4+CK_UNREN2-CK_LINKS], r14d
mov ecx, [r13+rax*4+CK_ADJLIST-CK_LINKS]
mov [r13+rdx*8], ecx
mov [r13+rax*4+CK_ADJLIST-CK_LINKS], edx
shr r14, 32
or r14d, edi
mov [r13+rdx*8+4], r14d
add edx, 1
_Skip:
cmp r8, r9
jb _LoopCount
add rsi, CK_THREADSEP
add edi, (0x40000/THREADS)
cmp edi, 0x40000
jnz _LoopCountFromThread

add ebx, 1
add r10, 0x8000
add r12, 0x10000
mov eax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE40
#if THREADS < 4
lea eax, [r11+0x10000]
test ebx, 0x1f
cmovz r11d, eax
#endif
test ebx, (128/THREADS-1)
jnz _LoopSrcBucket

mov eax, 0xffffffff
mov [r13+rdx*8], rax

pop r15
pop r14
pop r13
pop r12
pop rbp
pop rbx
ret
