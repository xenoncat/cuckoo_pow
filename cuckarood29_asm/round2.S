#include "memlayout.h"
.intel_syntax noprefix
.text
.align 64

.globl round2
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

round2:
push rbx
push rbp
push r12
push r13
push r14
push r15
mov rax, rsp
sub rsp, 64+8
and rsp, -64
mov [rsp+64], rax

imul eax, esi, CK_THREADSEP
lea r10, [rdi+rax+CK_BITMAP]
mov ebx, esi
shl ebx, (11-LOGTHREADS)
imul ebp, ebx, CK_BUCKETSIZE1
add ebp, CK_BUCKET1A-CK_COUNTER1
lea rsi, [rdi+CK_COUNTER1]
mov r13, 0xfffffffff
xor r12d, r12d

lea rax, [r10+CK_COUNTER0-CK_BITMAP]
mov r11, rax
vmovdqa ymm0, ymmword ptr yInitDest2[rip]
vpbroadcastd ymm1, dword ptr u32initbucket2inc[rip]

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

mov ecx, THREADS*2
_LoopCountSide0FromThread:
mov eax, [rsi+rbx*4-4]
add eax, 8
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax+2]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopCountSide0:
mov eax, [r8]
add r8, 5
mov edi, eax
and eax, 0x7ffc
shr edi, 15
mov edx, [r10+rax]
bts edx, edi
mov [r10+rax], edx
cmp r8, r9
jb _LoopCountSide0
add rsi, CK_THREADSEP
sub ecx, 1
test ecx, THREADS-1
jnz _LoopCountSide0FromThread
mov eax, THREADS*CK_THREADSEP-8192
sub rsi, rax
add ebp, CK_BUCKET1B-CK_BUCKET1A-8192
test ecx, ecx
jnz _LoopCountSide0FromThread

add ebp, CK_BUCKETSIZE1
mov ecx, THREADS*2
_LoopFilterSide1Phase:
mov eax, THREADS*CK_THREADSEP-8192
add rsi, rax
sub ebp, CK_BUCKET1B-CK_BUCKET1A-8192
_LoopFilterSide1FromThread:
sub rsi, CK_THREADSEP
mov eax, [rsi+rbx*4]
add eax, 8
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax]
mov rdx, r8
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopFilterSide1:
mov rax, [r8]
add r8, 5
mov [rdx], rax
mov rdi, rax
shr eax, 18
shr rdi, 31
and eax, 0x1fff
mov eax, [r10+rax*4]
bt eax, edi
lea rax, [rdx+5]
cmovc rdx, rax
cmp r8, r9
jb _LoopFilterSide1
sub edx, esi
mov eax, THREADS*2
sub eax, ecx
mov [rsp+rax*4], edx
sub ecx, 1
test ecx, THREADS-1
jnz _LoopFilterSide1FromThread
test ecx, ecx
jnz _LoopFilterSide1Phase

xor edx, edx
xor r11, 4
mov ecx, THREADS*2
_LoopDistributeSide1FromThread:
mov eax, [rsi+rbx*4]
add eax, 8
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax]
mov eax, [rsp+rcx*4-4]
lea r9, [rsi+rax]
_LoopDistributeSide1:
mov rax, [r8]
add r8, 5
mov r14, rax
mov r15, rax
shr eax, 18
and eax, 0x1fff
shr r14, 31
mov edi, [r10+rax*4]
btr edi, r14d
mov [r10+rax*4], edi
mov rax, r15
shr rax, 36
sub eax, edx
and eax, 0xf
add edx, eax
mov edi, [r11+rdx*8]
and r15, r13
or r15, r12
mov [r11+rdi-4], r15
add edi, 5
mov [r11+rdx*8], edi
cmp r8, r9
jb _LoopDistributeSide1
add rsi, CK_THREADSEP
sub ecx, 1
test ecx, THREADS-1
jnz _LoopDistributeSide1FromThread
mov eax, THREADS*CK_THREADSEP-8192
sub rsi, rax
add ebp, CK_BUCKET1B-CK_BUCKET1A-8192
test ecx, ecx
jnz _LoopDistributeSide1FromThread

and edx, -16
cmp edx, 0x3f0
jne _DensityError

xor edx, edx
sub rsi, 8192*2
sub ebp, 2*(CK_BUCKET1B-CK_BUCKET1A-8192)+CK_BUCKETSIZE1
xor r11, 4
mov ecx, THREADS*2
_LoopTrimSide0FromThread:
mov eax, [rsi+rbx*4-4]
add eax, 8
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax]
mov r15, r8
mov r14, r8
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopFilterSide0:
mov rax, [r8]
add r8, 5
mov [r15], rax
mov rdi, rax
shr eax, 18
and eax, 0x1fff
shr rdi, 31
mov eax, [r10+rax*4]
bt eax, edi
lea rax, [r15+5]
cmovnc r15, rax
cmp r8, r9
jb _LoopFilterSide0
_LoopDistributeSide0:
mov rax, [r14]
add r14, 5
mov rdi, rax
shr rax, 36
sub eax, edx
and eax, 0xf
add edx, eax
mov eax, [r11+rdx*8]
and rdi, r13
or rdi, r12
mov [r11+rax], rdi
add eax, 5
mov [r11+rdx*8], eax
cmp r14, r15
jb _LoopDistributeSide0
add rsi, CK_THREADSEP
sub ecx, 1
test ecx, THREADS-1
jnz _LoopTrimSide0FromThread
mov eax, THREADS*CK_THREADSEP-8192
sub rsi, rax
add ebp, CK_BUCKET1B-CK_BUCKET1A-8192
test ecx, ecx
jnz _LoopTrimSide0FromThread

and edx, -16
cmp edx, 0x3f0
jne _DensityError

add ebx, 2
sub rsi, 8192*2
sub ebp, (2*(CK_BUCKET1B-CK_BUCKET1A-8192)) - 2*CK_BUCKETSIZE1
lea r12, [r12+r13+1]
test ebx, (2048/THREADS-2)
jnz _LoopSrcBucket

_Epilog:
mov rsp, [rsp+64]
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
.set a, CK_BUCKET2-CK_COUNTER0
.globl yInitDest2
yInitDest2:
.int a, a+CK_BUCKETSIZE2, a+CK_BUCKETSIZE2*2, a+CK_BUCKETSIZE2*3
.int a+CK_BUCKETSIZE2*4, a+CK_BUCKETSIZE2*5, a+CK_BUCKETSIZE2*6, a+CK_BUCKETSIZE2*7
.globl u32initbucket2inc
u32initbucket2inc:
.int CK_BUCKETSIZE2*8
