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
//rbp rel32ReadStart
//rsi pu32SrcCtr
//rdi
//r8  pRead
//r9  pReadEnd
//r10 pu32Bitmap
//r11 pu32DstCtr
//r12 fromthread
//r13
//r14
//r15

.globl rounda1
rounda1:
push rbx
push rbp
push r12

imul eax, esi, CK_THREADSEP
lea r10, [rdi+rax+CK_BITMAP]
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
_A1LoopInitDstCtr:
vmovdqa [rax], ymm0
vpaddd ymm0, ymm0, ymm1
add rax, 32
sub ecx, 1
jnz _A1LoopInitDstCtr

_A1LoopSrcBucket:
mov r12d, THREADS
_A1LoopCountFromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_A1LoopCount:
mov rax, [r8]
add r8, 8
mov rdx, rax
shr rax, 32
and eax, 0x7f
mov edi, [r11+rax*4]
mov [r11+rdi], rdx
shr edx, 7
movzx ecx, byte ptr [r10+rdx]
add ecx, 1
and ecx, 2
mov [r10+rdx], cl
lea edi, [rdi+rcx*4]
mov [r11+rax*4], edi
cmp r8, r9
jb _A1LoopCount
add rsi, CK_THREADSEP
sub r12d, 1
jnz _A1LoopCountFromThread

add ebx, 1
add r10, 0x8000
mov eax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE40
test ebx, (128/THREADS-1)
jnz _A1LoopSrcBucket

pop r12
pop rbp
pop rbx
ret

.align 64
.globl roundb1
roundb1:
push rbx
push rbp
push r12

imul eax, esi, CK_THREADSEP
lea r10, [rdi+rax+CK_BITMAP]
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
_B1LoopInitDstCtr:
vmovdqa [rax], ymm0
vpaddd ymm0, ymm0, ymm1
add rax, 32
sub ecx, 1
jnz _B1LoopInitDstCtr

_B1LoopSrcBucket:
mov r12d, THREADS
_B1LoopCountFromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_B1LoopCount:
mov rax, [r8]
add r8, 8
mov rdx, rax
shr rax, 32
and eax, 0x7f
mov edi, [r11+rax*4]
mov [r11+rdi], rdx
shr edx, 7
movzx ecx, byte ptr [r10+rdx]
and ecx, 1
mov [r10+rdx], cl
xor ecx, 1
lea edi, [rdi+rcx*8]
mov [r11+rax*4], edi
cmp r8, r9
jb _B1LoopCount
add rsi, CK_THREADSEP
sub r12d, 1
jnz _B1LoopCountFromThread

add ebx, 1
add r10, 0x8000
mov eax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE40
test ebx, (128/THREADS-1)
jnz _B1LoopSrcBucket

pop r12
pop rbp
pop rbx
ret

.align 64
.globl roundc1
roundc1:
push rbx
push rbp
push r12

imul eax, esi, CK_THREADSEP
lea r10, [rdi+rax+CK_BITMAP]
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
_C1LoopInitDstCtr:
vmovdqa [rax], ymm0
vpaddd ymm0, ymm0, ymm1
add rax, 32
sub ecx, 1
jnz _C1LoopInitDstCtr

_C1LoopSrcBucket:
mov r12d, THREADS
_C1LoopCountFromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_C1LoopCount:
mov rax, [r8]
add r8, 8
mov rdx, rax
shr rax, 32
and eax, 0x7f
mov edi, [r11+rax*4]
mov [r11+rdi], rdx
shr edx, 7
movzx ecx, byte ptr [r10+rdx]
cmp ecx, 1
adc ecx, 0
mov [r10+rdx], cl
and ecx, 1
lea edi, [rdi+rcx*8]
mov [r11+rax*4], edi
cmp r8, r9
jb _C1LoopCount
add rsi, CK_THREADSEP
sub r12d, 1
jnz _C1LoopCountFromThread

add ebx, 1
add r10, 0x8000
mov eax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE40
test ebx, (128/THREADS-1)
jnz _C1LoopSrcBucket

pop r12
pop rbp
pop rbx
ret
