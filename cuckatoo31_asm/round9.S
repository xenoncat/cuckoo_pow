#include "memlayout.h"
.intel_syntax noprefix
.text
.align 64

.globl round9
//rdi = ctx
//rsi = tid

//rax
//rcx
//rdx
//rbx i32SrcBucket
//rbp rel32ReadStart
//rsi pu32SrcCtr
//rdi
//r8  pRead
//r9  pReadEnd
//r10 pu32Bitmap
//r11 pu32DstCtr
//r12 u64ItemMaskOr
//r13 u64ItemMaskAnd
//r14
//r15 fromthread

.byte 0x0f, 0x1f, 0x84, 0x00, 0,0,0,0
round9:
push rbx
push rbp
push r12
push r13
push r14
push r15

mov eax, CK_THREADSEP
imul rax, rsi
lea r10, [rdi+rax+CK_BITMAP]
mov ebx, esi
shl ebx, (13-LOGTHREADS)
imul ebp, ebx, CK_BUCKETSIZE8
add ebp, CK_BUCKET8-CK_COUNTER0+CK_BUCKETSIZE8
lea rsi, [rdi+CK_COUNTER0]
mov r13, 0xfffffffff
xor r12d, r12d

lea rax, [r10+CK_COUNTER1-CK_BITMAP]
mov r11, rax
vmovdqa ymm0, ymmword ptr yInitDest9[rip]
vpbroadcastd ymm1, dword ptr u32initbucket9inc[rip]
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
lea r8, [rsi+rbp+2]
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopCountSide1:
mov eax, [r8]
add r8, 6
mov edi, eax
and eax, 0x7ffc
mov edx, [r10+rax]
shr edi, 15
bts edx, edi
mov [r10+rax], edx
cmp r8, r9
jb _LoopCountSide1
mov eax, CK_THREADSEP
add rsi, rax
sub ecx, 1
jnz _LoopCountSide1FromThread

add r10, 0x8000
mov rdi, r10
xor eax, eax
mov ecx, 0x1000
rep stosq

mov rax, CK_THREADSEP*THREADS
sub rsi, rax
sub ebp, CK_BUCKETSIZE8
xor edx, edx
_LoopTrimSide0FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopTrimSide0:
mov r15, [r8]
add r8, 6
mov rcx, r15
mov r14, r15
shr r15, 36
and r15d, 0xfff
or r15d, edx
mov edi, [r11+r15*4]
mov eax, ecx
shr eax, 18
and rcx, r13
or rcx, r12
mov [r11+rdi], rcx
and eax, 0x1fff
mov ecx, [r10+rax*4-0x8000]
shr r14, 31
bt ecx, r14d
lea ecx, [rdi+6]
cmovc edi, ecx
mov [r11+r15*4], edi
mov ecx, [r10+rax*4]
bts ecx, r14d
mov [r10+rax*4], ecx
cmp r8, r9
jb _LoopTrimSide0
mov eax, CK_THREADSEP
add rsi, rax
add edx, (8192/THREADS)
cmp edx, 8192
jb _LoopTrimSide0FromThread

mov rax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE8
xor edx, edx
lea r12, [r12+r13+1]
_LoopTrimSide1FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopTrimSide1:
mov r15, [r8]
add r8, 6
mov r14, r15
mov rcx, r15
shr r15, 36
and r15d, 0xfff
or r15d, edx
mov edi, [r11+r15*4]
and r14, r13
or r14, r12
mov [r11+rdi], r14
mov rax, rcx
shr ecx, 18
and ecx, 0x1fff
mov ecx, [r10+rcx*4]
shr rax, 31
bt ecx, eax
lea ecx, [rdi+6]
cmovc edi, ecx
mov [r11+r15*4], edi
cmp r8, r9
jb _LoopTrimSide1
mov eax, CK_THREADSEP
add rsi, rax
add edx, (8192/THREADS)
cmp edx, 8192
jb _LoopTrimSide1FromThread

add ebx, 2
sub r10, 0x8000
mov rax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE8*2
lea r12, [r12+r13+1]
test ebx, (8192/THREADS-2)
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
.set a, CK_BUCKET9-CK_COUNTER1
yInitDest9:
.int a, a+CK_BUCKETSIZE9, a+CK_BUCKETSIZE9*2, a+CK_BUCKETSIZE9*3
.int a+CK_BUCKETSIZE9*4, a+CK_BUCKETSIZE9*5, a+CK_BUCKETSIZE9*6, a+CK_BUCKETSIZE9*7
u32initbucket9inc:
.int CK_BUCKETSIZE9*8
