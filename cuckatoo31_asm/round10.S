#include "memlayout.h"
.intel_syntax noprefix
.text
.align 64

.globl round10
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

round10:
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
imul ebp, ebx, CK_BUCKETSIZE9
add ebp, CK_BUCKET9-CK_COUNTER1+CK_BUCKETSIZE9
lea rsi, [rdi+CK_COUNTER1]
mov r13, 0xfffffffff
xor r12d, r12d

lea rax, [r10+CK_COUNTER0-CK_BITMAP]
mov r11, rax
vmovdqa ymm0, ymmword ptr yInitDest8[rip]
vpbroadcastd ymm1, dword ptr u32initbucket8inc[rip]
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
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopCountSide1:
mov eax, [r8]
add r8, 6
mov edi, eax
shr eax, 5
and eax, 0x1fff
mov edx, [r10+rax*4]
bts edx, edi
mov [r10+rax*4], edx
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
sub ebp, CK_BUCKETSIZE9
xor edx, edx
_LoopTrimSide0FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopTrimSide0:
mov rax, [r8]
add r8, 6
mov r14, rax
shr eax, 5
and eax, 0x1fff
mov ecx, [r10+rax*4]
shrx edi, [r10+rax*4-0x8000], r14d
bts ecx, r14d
mov [r10+rax*4], ecx
mov r15, r14
shr r15, 36
and r15d, 0xfff
or r15d, edx
mov eax, [r11+r15*4]
and r14, r13
or r14, r12
mov [r11+rax], r14
and edi, 1
lea edi, [rdi+rdi*2]
lea eax, [rax+rdi*2]
mov [r11+r15*4], eax
cmp r8, r9
jb _LoopTrimSide0
mov eax, CK_THREADSEP
add rsi, rax
add edx, (8192/THREADS)
cmp edx, 8192
jb _LoopTrimSide0FromThread

mov rax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE9
xor edx, edx
lea r12, [r12+r13+1]
_LoopTrimSide1FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopTrimSide1:
mov rax, [r8]
add r8, 6
mov r14, rax
shr eax, 5
and eax, 0x1fff
shrx ecx, [r10+rax*4], r14d
mov r15, r14
shr r15, 36
and r15d, 0xfff
or r15d, edx
mov edi, [r11+r15*4]
and r14, r13
or r14, r12
mov [r11+rdi], r14
and ecx, 1
lea ecx, [rcx+rcx*2]
lea edi, [rdi+rcx*2]
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
add ebp, CK_BUCKETSIZE9*2
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
