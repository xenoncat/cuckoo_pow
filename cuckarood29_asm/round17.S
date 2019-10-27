#include "memlayout.h"
.intel_syntax noprefix
.text
.align 64

.globl round17
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
//r12 u64ItemMaskOr
//r13
//r14 fromthread
//r15 u32InitWordCount

round17:
push rbx
push rbp
push r12
push r13
push r14
push r15

imul eax, esi, CK_THREADSEP
lea r10, [rdi+rax+CK_BITMAP]
mov r15d, [rdi+rax+CK_RENMAX1+4]
mov ebx, esi
shl ebx, (8-LOGTHREADS)
imul ebp, ebx, CK_BUCKETSIZE16
add ebp, CK_BUCKET16-CK_COUNTER0
lea rsi, [rdi+CK_COUNTER0]
xor r12d, r12d

lea rax, [r10+CK_COUNTER1-CK_BITMAP]
mov r11, rax
vmovdqa ymm0, ymmword ptr yInitDest15[rip]
vpbroadcastd ymm1, dword ptr u32initbucket15inc[rip]

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
mov ecx, r15d
rep stosq

mov ecx, THREADS
_LoopCountSide0FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopCountSide0:
mov eax, [r8]
add r8, 4
and eax, 0x7fff
mov byte ptr [r10+rax], 0x01
cmp r8, r9
jb _LoopCountSide0
add rsi, CK_THREADSEP
sub ecx, 1
jnz _LoopCountSide0FromThread

mov eax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE16
xor edx, edx
xor r11, 4
mov r14d, THREADS
_LoopTrimSide1FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopTrimSide1:
mov eax, [r8]
add r8, 4
mov edi, eax
and eax, 0x7fff
movzx ecx, byte ptr [r10+rax]
or ecx, 2
mov [r10+rax], cl
mov eax, edi
shr edi, 30
sub edi, edx
and edi, 3
add edx, edi
mov edi, [r11+rdx*8]
and eax, 0x3fffffff
or eax, r12d
mov [r11+rdi-4], eax
and ecx, 1
lea edi, [rdi+rcx*4]
mov [r11+rdx*8], edi
cmp r8, r9
jb _LoopTrimSide1
add rsi, CK_THREADSEP
sub r14d, 1
jnz _LoopTrimSide1FromThread

and edx, -4
cmp edx, 0x7c
jne _DensityError

mov eax, CK_THREADSEP*THREADS
sub rsi, rax
sub ebp, CK_BUCKETSIZE16
xor edx, edx
xor r11, 4
mov r14d, THREADS
_LoopTrimSide0FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopTrimSide0:
mov eax, [r8]
add r8, 4
mov edi, eax
and eax, 0x7fff
movzx ecx, byte ptr [r10+rax]
mov eax, edi
shr edi, 30
sub edi, edx
and edi, 3
add edx, edi
mov edi, [r11+rdx*8]
and eax, 0x3fffffff
or eax, r12d
mov [r11+rdi], eax
and ecx, 2
lea edi, [rdi+rcx*2]
mov [r11+rdx*8], edi
cmp r8, r9
jb _LoopTrimSide0
add rsi, CK_THREADSEP
sub r14d, 1
jnz _LoopTrimSide0FromThread

and edx, -4
cmp edx, 0x7c
jne _DensityError

add ebx, 2
mov eax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE16*2
add r12d, 0x40000000
test ebx, (256/THREADS-2)
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
