#include "memlayout.h"
.intel_syntax noprefix
.text
.align 64

//Bitmap 32768 bytes
//Dest index 65536 bytes
//Temp edges 262144 bytes

.globl round96
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
//r13
//r14 fromthread
//r15 u32InitWordCount

round96:
push rbx
push rbp
push r12
push r13
push r14
push r15
push rsi

mov eax, CK_THREADSEP
imul rax, rsi
lea r10, [rdi+rax+CK_BITMAP]
mov r15d, [rdi+rax+CK_RENMAX0]
add r15d, 64
and r15d, -2
mov eax, 4094
cmp r15d, eax
cmova r15d, eax
mov ebx, esi
shl ebx, (10-LOGTHREADS)
imul ebp, ebx, CK_BUCKETSIZE95
add ebp, CK_BUCKET95-CK_COUNTER1+CK_BUCKETSIZE95
lea rsi, [rdi+CK_COUNTER1]

lea rax, [r10+CK_COUNTER0-CK_BITMAP]
mov r11, rax
vmovdqa ymm0, ymmword ptr yInitDest96[rip]
vpbroadcastd ymm1, dword ptr u32initbucket96inc[rip]

mov ecx, 16
_LoopInitDstCtr:
vmovdqa [rax], ymm0
vpaddd ymm0, ymm0, ymm1
add rax, 32
sub ecx, 1
jnz _LoopInitDstCtr

vpxor xmm7, xmm7, xmm7
mov eax, 0x0f
vmovd xmm6, eax
vpbroadcastb xmm6, xmm6

_LoopSrcBucket:
mov rdi, r10
xor eax, eax
mov ecx, r15d
rep stosq

mov ecx, THREADS
_LoopCountSide1FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopCountSide1:
mov eax, [r8]
add r8, 5
shr eax, 2
and eax, 0x7fff
add byte ptr [r10+rax], 0x10
cmp r8, r9
jb _LoopCountSide1
mov eax, CK_THREADSEP
add rsi, rax
sub ecx, 1
jnz _LoopCountSide1FromThread

mov rax, CK_THREADSEP*THREADS
sub rsi, rax
sub ebp, CK_BUCKETSIZE95
mov ecx, THREADS
_LoopCountSide0FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopCountSide0:
mov eax, [r8]
add r8, 5
shr eax, 2
and eax, 0x7fff
add byte ptr [r10+rax], 0x01
cmp r8, r9
jb _LoopCountSide0
mov eax, CK_THREADSEP
add rsi, rax
sub ecx, 1
jnz _LoopCountSide0FromThread

mov rdx, r10
lea rdi, [r10+0x8000]
mov dword ptr [rdi-4], 0
vpxor xmm5, xmm5, xmm5
mov ecx, r15d
_LoopXmm:
vmovdqa xmm0, [rdx]
vpsrlw xmm1, xmm0, 4
vpand xmm0, xmm0, xmm6
vpand xmm1, xmm1, xmm6
vpcmpeqb xmm2, xmm0, xmm7
vpcmpeqb xmm3, xmm1, xmm7
vpor xmm2, xmm2, xmm3
vpaddb xmm0, xmm0, xmm1
vpandn xmm0, xmm2, xmm0
vpslldq xmm1, xmm0, 1
vpaddb xmm0, xmm0, xmm1
vpslldq xmm1, xmm0, 2
vpaddb xmm0, xmm0, xmm1
vpslldq xmm1, xmm0, 4
vpaddb xmm0, xmm0, xmm1
vpslldq xmm1, xmm0, 8
vpaddb xmm0, xmm0, xmm1
vpmovzxbw ymm0, xmm0
vpbroadcastw ymm4, xmm5
vpaddw ymm0, ymm0, ymm4
vmovd eax, xmm2
cbw
vpsrldq xmm2, xmm2, 1
vpmovsxbw ymm2, xmm2
or [rdi-2], ax
vextracti128 xmm5, ymm0, 1
vpor ymm0, ymm0, ymm2
vmovdqa [rdi], ymm0
vpsrldq xmm5, xmm5, 14
add rdx, 16
add rdi, 32
sub ecx, 2
jnz _LoopXmm

add r10, 32768-2
mov rax, CK_THREADSEP*THREADS
sub rsi, rax
mov r14d, THREADS
_LoopMoveSide0FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopMoveSide0:
mov rax, [r8]
add r8, 5
mov rdi, rax
shr eax, 2
and eax, 0x7fff
movzx ecx, word ptr [r10+rax*2]
shr rdi, 17
and edi, 0xfffff
mov [r10+rcx*4+65536+2], edi
add cx, 1
sbb cx, 0
mov [r10+rax*2], cx
cmp r8, r9
jb _LoopMoveSide0
mov eax, CK_THREADSEP
add rsi, rax
sub r14d, 1
jnz _LoopMoveSide0FromThread

mov rax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE95
mov r14d, THREADS
_LoopMoveSide1FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopMoveSide1:
mov rax, [r8]
add r8, 5
mov rdi, rax
shr eax, 2
and eax, 0x7fff
movzx ecx, word ptr [r10+rax*2]
shr rdi, 17
and edi, 0xfffff
or edi, 0x80000000
mov [r10+rcx*4+65536+2], edi
add cx, 1
sbb cx, 0
mov [r10+rax*2], cx
cmp r8, r9
jb _LoopMoveSide1
mov eax, CK_THREADSEP
add rsi, rax
sub r14d, 1
jnz _LoopMoveSide1FromThread

lea r8, [r10+65536+2]
vpextrw eax, xmm5, 0
lea r9, [r8+rax*4]
mov dword ptr [r9], 0
_LoopDistribute:
mov rax, [r8]
add r8, 8
test rax, rax
jns _PairMulti1
mov ecx, eax
and ecx, 0x7f
mov edx, [r11+rcx*4]
mov [r11+rdx], rax
add edx, 8
mov [r11+rcx*4], edx
cmp r8, r9
jae _EndDistribute
mov edi, [r8]
test edi, edi
jns _LoopDistribute
mov eax, eax
_Loop1toM:
shl rdi, 32
add r8, 4
or rdi, rax
mov [r11+rdx], rdi
add edx, 8
mov edi, [r8]
test edi, edi
js _Loop1toM
mov [r11+rcx*4], edx
cmp r8, r9
jb _LoopDistribute
jmp _EndDistribute

_PairMulti1:
lea r12, [r8-8]
_FindOtherSide:
mov edi, [r8]
add r8, 4
test edi, edi
jns _FindOtherSide
lea r13, [r8-4]
_MultiOuter:
mov eax, [r12]
add r12, 4
mov ecx, eax
and ecx, 0x7f
mov edx, [r11+rcx*4]
mov r8, r13
mov edi, [r13]
_MultiInner:
add r8, 4
shl rdi, 32
or rdi, rax
mov [r11+rdx], rdi
add edx, 8
mov edi, [r8]
test edi, edi
js _MultiInner
mov [r11+rcx*4], edx
cmp r12, r13
jb _MultiOuter
cmp r8, r9
jb _LoopDistribute
_EndDistribute:

add ebx, 2
sub r10, 32768-2
mov rax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE95*2
test ebx, (1024/THREADS-2)
jnz _LoopSrcBucket

pop rax
shl eax, (22-LOGTHREADS)
lea rdi, [rsi+rax+CK_ADJLIST-CK_COUNTER1]
xor eax, eax
mov ecx, (524288/THREADS)
rep stosq

pop r15
pop r14
pop r13
pop r12
pop rbp
pop rbx
ret

.section .rodata.cst32,"aM",@progbits,32
.align 64
.set a, CK_BUCKET96-CK_COUNTER0
.globl yInitDest96
yInitDest96:
.int a, a+CK_BUCKETSIZE96, a+CK_BUCKETSIZE96*2, a+CK_BUCKETSIZE96*3
.int a+CK_BUCKETSIZE96*4, a+CK_BUCKETSIZE96*5, a+CK_BUCKETSIZE96*6, a+CK_BUCKETSIZE96*7
.globl u32initbucket96inc
u32initbucket96inc:
.int CK_BUCKETSIZE96*8
