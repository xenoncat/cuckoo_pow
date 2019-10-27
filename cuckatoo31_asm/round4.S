#include "memlayout.h"
.intel_syntax noprefix
.text
.align 64

.globl round4
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

round4:
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
imul ebp, ebx, CK_BUCKETSIZE3
add ebp, CK_BUCKET3-CK_COUNTER1+CK_BUCKETSIZE3
lea rsi, [rdi+CK_COUNTER1]
mov r13, 0xfffffffff
xor r12d, r12d

lea rax, [r10+CK_COUNTER0-CK_BITMAP]
mov r11, rax
vmovdqa ymm0, ymmword ptr yInitDest2[rip]
vpbroadcastd ymm1, dword ptr u32initbucket2inc[rip]
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
mov eax, [rsi+rbx*4]
add eax, 8
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax]
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopCountSide1:
mov eax, [r8]
add r8, 5
mov edi, eax
and eax, 0x1fff
mov edx, [r10+rax*4]
shr edi, 13
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
sub ebp, CK_BUCKETSIZE3
xor edx, edx
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
mov rcx, [r8]
add r8, 5
mov r14, rcx
mov eax, ecx
shr rcx, 36
sub ecx, edx
and ecx, 0xf
add edx, ecx
mov ecx, [r11+rdx*4]
and r14, r13
or r14, r12
mov [r11+rcx], r14
mov r14d, eax
and eax, 0x1fff
mov edi, [r10+rax*4-0x8000]
shr r14d, 13
bt edi, r14d
lea edi, [rcx+5]
cmovc ecx, edi
mov [r11+rdx*4], ecx
mov edi, [r10+rax*4]
bts edi, r14d
mov [r10+rax*4], edi
cmp r8, r9
jb _LoopTrimSide0
mov eax, CK_THREADSEP
add rsi, rax
sub r15d, 1
jnz _LoopTrimSide0FromThread

and edx, -16
cmp edx, 0x1ff0
jne _DensityError

mov rax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE3
xor edx, edx
lea r12, [r12+r13+1]
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
mov rcx, [r8]
add r8, 5
mov r14, rcx
mov eax, ecx
shr rcx, 36
sub ecx, edx
and ecx, 0xf
add edx, ecx
mov ecx, [r11+rdx*4]
and r14, r13
or r14, r12
mov [r11+rcx], r14
mov r14d, eax
and eax, 0x1fff
mov edi, [r10+rax*4]
shr r14d, 13
bt edi, r14d
lea edi, [rcx+5]
cmovc ecx, edi
mov [r11+rdx*4], ecx
cmp r8, r9
jb _LoopTrimSide1
mov eax, CK_THREADSEP
add rsi, rax
sub r15d, 1
jnz _LoopTrimSide1FromThread

and edx, -16
cmp edx, 0x1ff0
jne _DensityError

add ebx, 2
sub r10, 0x8000
mov rax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE3*2
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

_DensityError:
lea rdi, szDebug[rip]
mov esi, ebx
//mov edx, edx
xor eax, eax
call printf@PLT
jmp _Epilog

.section .rodata.cst32,"aM",@progbits,32
.align 64
szDebug:
.string "%lx %lx\n"
