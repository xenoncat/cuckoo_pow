#include "memlayout.h"
.intel_syntax noprefix
.text
.align 64

.globl roundpinit
//rdi = ctx
//rsi = tid

//rax
//rcx
//rdx
//rbx u32SrcBucket
//rbp rel32ReadStart
//rsi pu32SrcCtr
//rdi
//r8  pRead
//r9  pReadEnd
//r10 pu32Bitmap
//r11 pu32DstCtr
//r12
//r13
//r14
//r15 u32InitWordCount

roundpinit:
push rbx
push rbp
push r12
push r13
push r14
push r15

imul eax, esi, CK_THREADSEP
lea r10, [rdi+rax+CK_BITMAP]
mov r15d, [rdi+rax+CK_RENMAX1+4]
mov ebx, esi
shl ebx, (7-LOGTHREADS)
imul ebp, ebx, CK_BUCKETSIZE40
add ebp, CK_BUCKET40-CK_COUNTER0
lea rsi, [rdi+CK_COUNTER0]

lea rax, [r10+CK_COUNTER1-CK_BITMAP]
mov r11, rax
vmovdqa ymm0, ymmword ptr yInitDest41[rip]
vpbroadcastd ymm1, dword ptr u32initbucket41inc[rip]

mov ecx, 16
_LoopInitDstCtr:
vmovdqa [rax], ymm0
vpaddd ymm0, ymm0, ymm1
add rax, 32
sub ecx, 1
jnz _LoopInitDstCtr

_LoopSrcBucket:
mov rdi, r10
xor eax, eax
mov ecx, r15d
rep stosq

mov ecx, THREADS
_LoopCountFromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopCount:
mov rax, [r8]
add r8, 8
mov rdx, rax
shr rax, 32
and eax, 0x7f
mov edi, [r11+rax*4]
btr rdx, 63
mov [r11+rdi], rdx
add edi, 8
mov [r11+rax*4], edi
shr edx, 7
mov byte ptr [r10+rdx], 1
cmp r8, r9
jb _LoopCount
add rsi, CK_THREADSEP
sub ecx, 1
jnz _LoopCountFromThread

add ebx, 1
add r10, 0x8000
mov eax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE40
test ebx, (128/THREADS-1)
jnz _LoopSrcBucket

_Epilog:
pop r15
pop r14
pop r13
pop r12
pop rbp
pop rbx
ret

.section .rodata.cst32,"aM",@progbits,32
.align 64
.set a, CK_BUCKET41-CK_COUNTER1
.globl yInitDest41
yInitDest41:
.int a, a+CK_BUCKETSIZE41, a+CK_BUCKETSIZE41*2, a+CK_BUCKETSIZE41*3
.int a+CK_BUCKETSIZE41*4, a+CK_BUCKETSIZE41*5, a+CK_BUCKETSIZE41*6, a+CK_BUCKETSIZE41*7
.globl u32initbucket41inc
u32initbucket41inc:
.int CK_BUCKETSIZE41*8
