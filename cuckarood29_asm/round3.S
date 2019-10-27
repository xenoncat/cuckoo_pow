#include "memlayout.h"
.intel_syntax noprefix
.text
.align 64

.globl round3
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

round3:
push rbx
push rbp
push r12
push r13
push r14
push r15

imul eax, esi, CK_THREADSEP
lea r10, [rdi+rax+CK_BITMAP]
mov ebx, esi
shl ebx, (11-LOGTHREADS)
imul ebp, ebx, CK_BUCKETSIZE2
add ebp, CK_BUCKET2-CK_COUNTER0
lea rsi, [rdi+CK_COUNTER0]
mov r13, 0xfffffffff
xor r12d, r12d

lea rax, [r10+CK_COUNTER1-CK_BITMAP]
mov r11, rax
vmovdqa ymm0, ymmword ptr yInitDest3[rip]
vpbroadcastd ymm1, dword ptr u32initbucket3inc[rip]

mov ecx, 256
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
_LoopCountSide0FromThread:
mov eax, [rsi+rbx*4-4]
add eax, 8
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopCountSide0:
mov eax, [r8]
add r8, 5
mov edi, eax
shr eax, 5
and eax, 0x1fff
mov edx, [r10+rax*4]
bts edx, edi
mov [r10+rax*4], edx
cmp r8, r9
jb _LoopCountSide0
add rsi, CK_THREADSEP
sub ecx, 1
jnz _LoopCountSide0FromThread

add r10, 0x8000
mov rdi, r10
xor eax, eax
mov ecx, 0x1000
rep stosq

mov eax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE2
xor edx, edx
xor r11, 4
mov r15d, THREADS
_LoopTrimSide1FromThread:
mov eax, [rsi+rbx*4]
add eax, 8
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax]
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopTrimSide1:
mov rax, [r8]
add r8, 5
mov r14, rax
shr eax, 5
and eax, 0x1fff
mov ecx, [r10+rax*4]
shrx edi, [r10+rax*4-0x8000], r14d
bts ecx, r14d
mov [r10+rax*4], ecx
mov rax, r14
shr rax, 36
sub eax, edx
and eax, 0xf
add edx, eax
mov eax, [r11+rdx*8]
and r14, r13
or r14, r12
mov [r11+rax-4], r14
and edi, 1
lea edi, [rdi+rdi*4]
add eax, edi
mov [r11+rdx*8], eax
cmp r8, r9
jb _LoopTrimSide1
add rsi, CK_THREADSEP
sub r15d, 1
jnz _LoopTrimSide1FromThread

and edx, -16
cmp edx, 0x3f0
jne _DensityError

mov eax, CK_THREADSEP*THREADS
sub rsi, rax
sub ebp, CK_BUCKETSIZE2
xor edx, edx
xor r11, 4
mov r15d, THREADS
_LoopTrimSide0FromThread:
mov eax, [rsi+rbx*4-4]
add eax, 8
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopTrimSide0:
mov rax, [r8]
add r8, 5
mov r14, rax
mov ecx, eax
shr eax, 5
and eax, 0x1fff
shrx ecx, [r10+rax*4], ecx
mov rax, r14
shr rax, 36
sub eax, edx
and eax, 0xf
add edx, eax
mov edi, [r11+rdx*8]
and r14, r13
or r14, r12
mov [r11+rdi], r14
and ecx, 1
lea ecx, [rcx+rcx*4]
add edi, ecx
mov [r11+rdx*8], edi
cmp r8, r9
jb _LoopTrimSide0
add rsi, CK_THREADSEP
sub r15d, 1
jnz _LoopTrimSide0FromThread

and edx, -16
cmp edx, 0x3f0
jne _DensityError

add ebx, 2
sub r10, 0x8000
mov eax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE2*2
lea r12, [r12+r13+1]
test ebx, (2048/THREADS-2)
jnz _LoopSrcBucket

_Epilog:
pop r15
pop r14
pop r13
pop r12
pop rbp
pop rbx
ret

_DensityError:
ud2
jmp _Epilog

.section .rodata.cst32,"aM",@progbits,32
.align 64
.set a, CK_BUCKET3-CK_COUNTER1
.globl yInitDest3
yInitDest3:
.int a, a+CK_BUCKETSIZE3, a+CK_BUCKETSIZE3*2, a+CK_BUCKETSIZE3*3
.int a+CK_BUCKETSIZE3*4, a+CK_BUCKETSIZE3*5, a+CK_BUCKETSIZE3*6, a+CK_BUCKETSIZE3*7
.globl u32initbucket3inc
u32initbucket3inc:
.int CK_BUCKETSIZE3*8
