#include "memlayout.h"
.intel_syntax noprefix
.text
.align 64

.globl round18
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
//r13 u64ItemMaskAnd
//r14 fromthread
//r15 u32InitWordCount

round18:
push rbx
push rbp
push r12
push r13
push r14
push r15
sub rsp, 0x18

mov eax, CK_THREADSEP
mov [rsp], rax
mov ecx, eax
shl rcx, LOGTHREADS
mov [rsp+0x08], rcx
imul rax, rsi
lea r10, [rdi+rax+CK_BITMAP]
mov eax, [rdi+rax+CK_RENMAX0+4]
mov [rsp+0x10], rax
mov ebx, esi
shl ebx, (8-LOGTHREADS)
imul ebp, ebx, CK_BUCKETSIZE15
add ebp, CK_BUCKET15-CK_COUNTER1+CK_BUCKETSIZE15
lea rsi, [rdi+CK_COUNTER1]
mov r13, 0x3ffffffff
xor r15d, r15d

lea rax, [r10+CK_COUNTER0-CK_BITMAP]
mov r11, rax
vmovdqa ymm0, ymmword ptr yInitDest16[rip]
vpbroadcastd ymm1, dword ptr u32initbucket16inc[rip]

mov ecx, 32
_LoopInitDstCtr:
vmovdqa [rax], ymm0
vpaddd ymm0, ymm0, ymm1
add rax, 32
sub ecx, 1
jnz _LoopInitDstCtr

_LoopSrcBucket:
mov rdi, r10
xor eax, eax
mov ecx, [rsp+0x10]
rep stosq

mov ecx, THREADS
_LoopCountSide1FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopCountSide1:
mov eax, [r8]
add r8, 5
mov edi, eax
shr eax, 5
and eax, 0xfff
mov edx, [r10+rax*4]
bts edx, edi
mov [r10+rax*4], edx
cmp r8, r9
jb _LoopCountSide1
add rsi, [rsp]
sub ecx, 1
jnz _LoopCountSide1FromThread

add r10, 0x4000
mov rdi, r10
xor eax, eax
mov ecx, [rsp+0x10]
rep stosq

sub rsi, [rsp+0x08]
sub ebp, CK_BUCKETSIZE15
xor edx, edx
_LoopTrimSide0FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopTrimSide0:
mov r14, [r8]
add r8, 5
mov rcx, r14
shr r14, 34
and r14d, 0x3f
or r14d, edx
mov edi, [r11+r14*4]
mov eax, ecx
and rcx, r13
or rcx, r15
mov [r11+rdi], rcx
mov ecx, eax
shr eax, 5
and eax, 0xfff
mov r12d, [r10+rax*4-0x4000]
bt r12d, ecx
lea r12d, [rdi+5]
cmovc edi, r12d
mov [r11+r14*4], edi
mov edi, [r10+rax*4]
bts edi, ecx
mov [r10+rax*4], edi
cmp r8, r9
jb _LoopTrimSide0
add rsi, [rsp]
add edx, (256/THREADS)
cmp edx, 256
jb _LoopTrimSide0FromThread

sub rsi, [rsp+0x08]
add ebp, CK_BUCKETSIZE15
add ebx, 1
lea r15, [r15+r13+1]
xor edx, edx
_LoopTrimSide1FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopTrimSide1:
mov r14, [r8]
add r8, 5
mov rcx, r14
shr r14, 34
and r14d, 0x3f
or r14d, edx
mov edi, [r11+r14*4]
mov eax, ecx
and rcx, r13
or rcx, r15
mov [r11+rdi], rcx
mov ecx, eax
shr eax, 5
and eax, 0xfff
mov eax, [r10+rax*4]
bt eax, ecx
lea eax, [rdi+5]
cmovc edi, eax
mov [r11+r14*4], edi
cmp r8, r9
jb _LoopTrimSide1
add rsi, [rsp]
add edx, (256/THREADS)
cmp edx, 256
jb _LoopTrimSide1FromThread

add ebx, 1
lea r15, [r15+r13+1]
sub r10, 0x4000
sub rsi, [rsp+0x08]
add ebp, CK_BUCKETSIZE15*2
test ebx, (256/THREADS-2)
jnz _LoopSrcBucket

_Epilog:
add rsp, 0x18
pop r15
pop r14
pop r13
pop r12
pop rbp
pop rbx
ret
