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

.globl rounda0
rounda0:
push rbx
push rbp
push r12

imul eax, esi, CK_THREADSEP
lea r10, [rdi+rax+CK_BITMAP]
mov ebx, esi
shl ebx, (7-LOGTHREADS)
imul ebp, ebx, CK_BUCKETSIZE41
add ebp, CK_BUCKET41-CK_COUNTER1
lea rsi, [rdi+CK_COUNTER1]

lea rax, [r10+CK_COUNTER0-CK_BITMAP]
mov r11, rax
vmovdqa ymm0, ymmword ptr yInitDest40[rip]
vpbroadcastd ymm1, dword ptr u32initbucket40inc[rip]

mov ecx, 16
_A0LoopInitDstCtr:
vmovdqa [rax], ymm0
vpaddd ymm0, ymm0, ymm1
add rax, 32
sub ecx, 1
jnz _A0LoopInitDstCtr

_A0LoopSrcBucket:
mov r12d, THREADS
_A0LoopCountFromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_A0LoopCount:
mov rax, [r8]
add r8, 8
mov rdx, rax
and eax, 0x7f
mov edi, [r11+rax*4]
mov [r11+rdi], rdx
shr rdx, 32+7
movzx ecx, byte ptr [r10+rdx]
add ecx, 1
and ecx, 2
mov [r10+rdx], cl
lea edi, [rdi+rcx*4]
mov [r11+rax*4], edi
cmp r8, r9
jb _A0LoopCount
add rsi, CK_THREADSEP
sub r12d, 1
jnz _A0LoopCountFromThread

add ebx, 1
add r10, 0x8000
mov eax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE41
test ebx, (128/THREADS-1)
jnz _A0LoopSrcBucket

pop r12
pop rbp
pop rbx
ret

.align 64
.globl roundb0
roundb0:
push rbx
push rbp
push r12

imul eax, esi, CK_THREADSEP
lea r10, [rdi+rax+CK_BITMAP]
mov ebx, esi
shl ebx, (7-LOGTHREADS)
imul ebp, ebx, CK_BUCKETSIZE41
add ebp, CK_BUCKET41-CK_COUNTER1
lea rsi, [rdi+CK_COUNTER1]

lea rax, [r10+CK_COUNTER0-CK_BITMAP]
mov r11, rax
vmovdqa ymm0, ymmword ptr yInitDest40[rip]
vpbroadcastd ymm1, dword ptr u32initbucket40inc[rip]

mov ecx, 16
_B0LoopInitDstCtr:
vmovdqa [rax], ymm0
vpaddd ymm0, ymm0, ymm1
add rax, 32
sub ecx, 1
jnz _B0LoopInitDstCtr

_B0LoopSrcBucket:
mov r12d, THREADS
_B0LoopCountFromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_B0LoopCount:
mov rax, [r8]
add r8, 8
mov rdx, rax
and eax, 0x7f
mov edi, [r11+rax*4]
mov [r11+rdi], rdx
shr rdx, 32+7
movzx ecx, byte ptr [r10+rdx]
and ecx, 1
mov [r10+rdx], cl
xor ecx, 1
lea edi, [rdi+rcx*8]
mov [r11+rax*4], edi
cmp r8, r9
jb _B0LoopCount
add rsi, CK_THREADSEP
sub r12d, 1
jnz _B0LoopCountFromThread

add ebx, 1
add r10, 0x8000
mov eax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE41
test ebx, (128/THREADS-1)
jnz _B0LoopSrcBucket

pop r12
pop rbp
pop rbx
ret

.align 64
.globl roundc0
roundc0:
push rbx
push rbp
push r12

imul eax, esi, CK_THREADSEP
lea r10, [rdi+rax+CK_BITMAP]
mov ebx, esi
shl ebx, (7-LOGTHREADS)
imul ebp, ebx, CK_BUCKETSIZE41
add ebp, CK_BUCKET41-CK_COUNTER1
lea rsi, [rdi+CK_COUNTER1]

lea rax, [r10+CK_COUNTER0-CK_BITMAP]
mov r11, rax
vmovdqa ymm0, ymmword ptr yInitDest40[rip]
vpbroadcastd ymm1, dword ptr u32initbucket40inc[rip]

mov ecx, 16
_C0LoopInitDstCtr:
vmovdqa [rax], ymm0
vpaddd ymm0, ymm0, ymm1
add rax, 32
sub ecx, 1
jnz _C0LoopInitDstCtr

_C0LoopSrcBucket:
mov r12d, THREADS
_C0LoopCountFromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_C0LoopCount:
mov rax, [r8]
add r8, 8
mov rdx, rax
and eax, 0x7f
mov edi, [r11+rax*4]
mov [r11+rdi], rdx
shr rdx, 32+7
movzx ecx, byte ptr [r10+rdx]
cmp ecx, 1
adc ecx, 0
mov [r10+rdx], cl
and ecx, 1
lea edi, [rdi+rcx*8]
mov [r11+rax*4], edi
cmp r8, r9
jb _C0LoopCount
add rsi, CK_THREADSEP
sub r12d, 1
jnz _C0LoopCountFromThread

add ebx, 1
add r10, 0x8000
mov eax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE41
test ebx, (128/THREADS-1)
jnz _C0LoopSrcBucket

pop r12
pop rbp
pop rbx
ret
