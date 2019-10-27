#include "memlayout.h"
.intel_syntax noprefix
.text
.align 64

.globl round1
//rdi = ctx
//rsi = tid
//rdx = phase

//rax
//rcx fromthread
//rdx pu32DstCtr
//rbx i32SrcBucket
//rbp rel32ReadStart
//rsi pu32SrcCtr
//rdi
//r8  pRead
//r9  pReadEnd
//r10 pu32Bitmap
//r11
//r12 u64ItemMaskOr
//r13 u64ItemMaskAnd
//r14
//r15 pWrite

round1:
push rbx
push rbp
push r12
push r13
push r14
push r15
mov rax, rsp
sub rsp, 32+8
and rsp, -64
mov [rsp+32], rax

imul eax, esi, CK_THREADSEP
lea r10, [rdi+rax+CK_BITMAP]
mov ebx, edx
shl ebx, 10
shl esi, (10-LOGTHREADS)
add ebx, esi
imul ebp, ebx, CK_BUCKETSIZE0
add ebp, CK_BUCKET0-CK_COUNTER0
lea rsi, [rdi+CK_COUNTER0]
mov r13, 0xfffffffff
xor r12d, r12d

lea rax, LookupCtrInit[rip]
mov eax, [rax+rdx*4]
vmovd xmm0, eax
vpbroadcastd ymm0, xmm0
shl edx, 13
lea rax, [r10+rdx+CK_COUNTER1-CK_BITMAP]
mov rdx, rax
vpaddd ymm0, ymm0, ymmword ptr yInitDest1[rip]
vpbroadcastd ymm1, dword ptr u32initbucket1inc[rip]

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
add eax, 2
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopCountSide0:
mov eax, [r8]
add r8, 6
mov edi, eax
and eax, 0x1fff
shr edi, 13
mov r11d, [r10+rax*4]
bts r11d, edi
mov [r10+rax*4], r11d
cmp r8, r9
jb _LoopCountSide0
add rsi, CK_THREADSEP
sub ecx, 1
jnz _LoopCountSide0FromThread

add ebp, CK_BUCKETSIZE0
mov ecx, THREADS
_LoopFilterSide1FromThread:
sub rsi, CK_THREADSEP
mov eax, [rsi+rbx*4]
add eax, 2
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax]
mov r15, r8
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopFilterSide1:
mov rax, [r8]
add r8, 6
mov [r15], rax
mov edi, eax
and eax, 0x1fff
shr edi, 13
mov eax, [r10+rax*4]
bt eax, edi
lea rax, [r15+6]
cmovc r15, rax
cmp r8, r9
jb _LoopFilterSide1
sub r15d, esi
mov eax, THREADS
sub eax, ecx
mov [rsp+rax*4], r15d
sub ecx, 1
jnz _LoopFilterSide1FromThread

xor rdx, 4
mov r14d, THREADS
_LoopDistributeSide1FromThread:
mov eax, [rsi+rbx*4]
add eax, 2
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax]
mov eax, [rsp+r14*4-4]
lea r9, [rsi+rax]
_LoopDistributeSide1:
mov rax, [r8]
add r8, 6
mov r11, rax
mov ecx, eax
shr rax, 36
and eax, 0x3ff
mov edi, [rdx+rax*8]
and r11, r13
or r11, r12
mov [rdx+rdi-4], r11
add edi, 5
mov [rdx+rax*8], edi
mov eax, ecx
and eax, 0x1fff
shr ecx, 13
mov edi, [r10+rax*4]
btr edi, ecx
mov [r10+rax*4], edi
cmp r8, r9
jb _LoopDistributeSide1
add rsi, CK_THREADSEP
sub r14d, 1
jnz _LoopDistributeSide1FromThread

mov eax, CK_THREADSEP*THREADS
sub rsi, rax
sub ebp, CK_BUCKETSIZE0
xor rdx, 4
mov r14d, THREADS
_LoopTrimSide0FromThread:
mov eax, [rsi+rbx*4-4]
add eax, 2
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopTrimSide0:
mov rax, [r8]
add r8, 6
mov r11, rax
mov ecx, eax
shr rax, 36
and eax, 0x3ff
mov edi, [rdx+rax*8]
and r11, r13
or r11, r12
mov [rdx+rdi], r11
mov r11d, ecx
and ecx, 0x1fff
shr r11d, 13
mov ecx, [r10+rcx*4]
bt ecx, r11d
lea ecx, [rdi+5]
cmovnc edi, ecx
mov [rdx+rax*8], edi
cmp r8, r9
jb _LoopTrimSide0
add rsi, CK_THREADSEP
sub r14d, 1
jnz _LoopTrimSide0FromThread

add ebx, 2
mov eax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE0*2
lea r12, [r12+r13+1]
test ebx, (1024/THREADS-2)
jnz _LoopSrcBucket

mov rsp, [rsp+32]
pop r15
pop r14
pop r13
pop r12
pop rbp
pop rbx
ret

.section .rodata.cst32,"aM",@progbits,32
.align 64
yInitDest1:
.int 0, CK_BUCKETSIZE1, CK_BUCKETSIZE1*2, CK_BUCKETSIZE1*3
.int CK_BUCKETSIZE1*4, CK_BUCKETSIZE1*5, CK_BUCKETSIZE1*6, CK_BUCKETSIZE1*7
LookupCtrInit:
.int CK_BUCKET1A-CK_COUNTER1, CK_BUCKET1B-CK_COUNTER1B
u32initbucket1inc:
.int CK_BUCKETSIZE1*8
