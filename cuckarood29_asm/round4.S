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

imul eax, esi, CK_THREADSEP
lea r10, [rdi+rax+CK_BITMAP]
mov ebx, esi
shl ebx, (11-LOGTHREADS)
imul ebp, ebx, CK_BUCKETSIZE3
add ebp, CK_BUCKET3-CK_COUNTER1
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

mov ecx, THREADS
_LoopCountSide0FromThread:
lea r8, [rsi+rbp+2]
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
jnz _LoopCountSide0FromThread

add r10, 0x8000
mov rdi, r10
xor eax, eax
mov ecx, 0x1000
rep stosq

mov eax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE3
xor edx, edx
xor r11, 4
mov r15d, THREADS
_LoopTrimSide1FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopTrimSide1:
mov rax, [r8]
add r8, 5
mov rcx, rax
mov r14, rax
shr rax, 36
sub eax, edx
and eax, 0xf
add edx, eax
mov edi, [r11+rdx*8]
mov eax, ecx
shr eax, 18
and rcx, r13
or rcx, r12
mov [r11+rdi-4], rcx
and eax, 0x1fff
shr r14, 31
mov ecx, [r10+rax*4-0x8000]
bt ecx, r14d
lea ecx, [rdi+5]
cmovc edi, ecx
mov [r11+rdx*8], edi
mov ecx, [r10+rax*4]
bts ecx, r14d
mov [r10+rax*4], ecx
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
sub ebp, CK_BUCKETSIZE3
xor edx, edx
xor r11, 4
mov r15d, THREADS
_LoopTrimSide0FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopTrimSide0:
mov rax, [r8]
add r8, 5
mov r14, rax
mov rcx, rax
shr rax, 36
sub eax, edx
and eax, 0xf
add edx, eax
mov edi, [r11+rdx*8]
and r14, r13
or r14, r12
mov [r11+rdi], r14
mov rax, rcx
shr ecx, 18
and ecx, 0x1fff
shr rax, 31
mov ecx, [r10+rcx*4]
bt ecx, eax
lea ecx, [rdi+5]
cmovc edi, ecx
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
add ebp, CK_BUCKETSIZE3*2
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
