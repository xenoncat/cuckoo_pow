#include "memlayout.h"
.intel_syntax noprefix
.text
.align 64

.globl round8
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

round8:
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
lea r8, [rsi+rbp]
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
lea edi, [rcx+6]
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

mov rax, CK_THREADSEP*THREADS
sub rsi, rax
and edx, -16
cmp edx, 0x1ff0
jne _DensityError

add ebp, CK_BUCKETSIZE3
xor edx, edx
lea r12, [r12+r13+1]
mov r15d, THREADS
_LoopTrimSide1FromThread:
lea r8, [rsi+rbp]
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
lea edi, [rcx+6]
cmovc ecx, edi
mov [r11+rdx*4], ecx
cmp r8, r9
jb _LoopTrimSide1
mov eax, CK_THREADSEP
add rsi, rax
sub r15d, 1
jnz _LoopTrimSide1FromThread

mov rax, CK_THREADSEP*THREADS
sub rsi, rax
and edx, -16
cmp edx, 0x1ff0
jne _DensityError

add ebx, 2
sub r10, 0x8000
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
mov dword ptr [rsi+CK_ABORT-CK_COUNTER1], 1
push rax
lea rdi, szDebug[rip]
mov esi, ebx
//mov edx, edx
xor eax, eax
call printf@PLT
pop rax
jmp _Epilog

.section .rodata.cst32,"aM",@progbits,32
.align 64
.set a, CK_BUCKET8-CK_COUNTER0
.globl yInitDest8
yInitDest8:
.int a, a+CK_BUCKETSIZE8, a+CK_BUCKETSIZE8*2, a+CK_BUCKETSIZE8*3
.int a+CK_BUCKETSIZE8*4, a+CK_BUCKETSIZE8*5, a+CK_BUCKETSIZE8*6, a+CK_BUCKETSIZE8*7
.globl u32initbucket8inc
u32initbucket8inc:
.int CK_BUCKETSIZE8*8
szDebug:
.string "Density Error %x %x\n"
