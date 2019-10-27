#include "memlayout.h"
.intel_syntax noprefix
.text
.align 64

.globl round14
//rdi = ctx
//rsi = tid

//rax
//rcx
//rdx
//rbx i32SrcBucket
//rbp rel32ReadStart
//rsi
//rdi
//r8  pRead
//r9  pReadEnd
//r10 pu32Bitmap
//r11 pu32DstCtr
//r12 u64ItemMaskOr
//r13
//r14 pu32SrcCtr
//r15

round14:
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
imul ebp, ebx, CK_BUCKETSIZE9
add ebp, CK_BUCKET9-CK_COUNTER1+CK_BUCKETSIZE9
lea r14, [rdi+CK_COUNTER1]
mov r12d, ebx
shl r12, 30

lea rax, [r10+CK_COUNTER0-CK_BITMAP]
mov r11, rax
vmovdqa ymm0, ymmword ptr yInitDest14[rip]
vpbroadcastd ymm1, dword ptr u32initbucket14inc[rip]
mov dword ptr [r10+CK_RENMAX0-CK_BITMAP], 0
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

mov esi, THREADS
_LoopCountSide1FromThread:
lea r8, [r14+rbp]
mov eax, [r14+rbx*4+4]
lea r9, [r14+rax]
_LoopCountSide1:
mov eax, [r8]
add r8, 6
mov edi, eax
and eax, 0x1fff
mov edx, [r10+rax*4]
shr edi, 13
bts edx, edi
mov [r10+rax*4], edx
cmp r8, r9
jb _LoopCountSide1
mov eax, CK_THREADSEP
add r14, rax
sub esi, 1
jnz _LoopCountSide1FromThread

add r10, 0x8000
mov rdi, r10
xor eax, eax
mov ecx, 0x1000
rep stosq

mov rax, CK_THREADSEP*THREADS
sub r14, rax
sub ebp, CK_BUCKETSIZE9
mov esi, THREADS
_LoopFilterSide0FromThread:
lea r8, [r14+rbp]
mov rdx, r8
mov eax, [r14+rbx*4]
lea r9, [r14+rax]
_LoopFilterSide0:
mov rax, [r8]
add r8, 6
mov [rdx], rax
mov ecx, eax
and eax, 0x1fff
mov edi, [r10+rax*4-0x8000]
shr ecx, 13
bt edi, ecx
lea rdi, [rdx+6]
cmovc rdx, rdi
mov edi, [r10+rax*4]
bts edi, ecx
mov [r10+rax*4], edi
cmp r8, r9
jb _LoopFilterSide0
sub edx, r14d
mov [r14+rbx*4], edx
mov eax, CK_THREADSEP
add r14, rax
sub esi, 1
jnz _LoopFilterSide0FromThread

mov rax, r10
mov ecx, 0x400
_LoopAnding:
vmovdqa ymm0, [rax-0x8000]
vpand ymm0, ymm0, [rax]
vmovdqa [rax], ymm0
add rax, 32
sub ecx, 1
jnz _LoopAnding

//rax shifter
//ecx count
//edx word
//edi base
xor eax, eax
//Don't use node 0
mov edi, 1
jmp _EnterRename
_LoopRename:
add ecx, edi
mov [r10-0x8000], ecx
or rdx, rax
mov rax, rdx
not rdx
tzcnt rcx, rdx
shrx rax, rax, rcx
add edi, ecx
_WordIsZero:
add r10, 4
test r10, 0x7fff
jz _DoneRename
_EnterRename:
mov edx, [r10]
tzcnt ecx, edx
jc _WordIsZero
shrx edx, edx, ecx
neg ecx
test rax, rdx
jz _LoopRename
add edx, edx
jc _WordCarryOut
_LoopShiftWord:
add ecx, 1
test rax, rdx
jz _LoopRename
add edx, edx
jnc _LoopShiftWord
_WordCarryOut:
rcr edx, 1
_LoopShiftShifter:
shr rax, 1
add edi, 1
test rax, rdx
jz _LoopRename
jmp _LoopShiftShifter
_DoneRename:
mov eax, [r11+CK_RENMAX0-CK_COUNTER0]
cmp edi, eax
cmova eax, edi
mov [r11+CK_RENMAX0-CK_COUNTER0], eax

mov rax, CK_THREADSEP*THREADS
sub r14, rax
sub r10, 0x10000
xor edx, edx
_LoopDistributeSide0FromThread:
lea r8, [r14+rbp]
mov eax, [r14+rbx*4]
lea r9, [r14+rax]
_LoopDistributeSide0:
mov rax, [r8]
add r8, 6
mov ecx, eax
mov rsi, rax
mov rdi, rax
and eax, 0x1fff
shr ecx, 13
and ecx, 0x1f
add ecx, [r10+rax*4]
shr rsi, 36
and esi, 0xfff
or esi, edx
mov eax, [r11+rsi*4]
shr rdi, 6
and edi, 0x3ffff000
or ecx, edi
or rcx, r12
mov [r11+rax], rcx
add eax, 5
mov [r11+rsi*4], eax
cmp r8, r9
jb _LoopDistributeSide0
mov eax, CK_THREADSEP
add r14, rax
add edx, (8192/THREADS)
cmp edx, 8192
jb _LoopDistributeSide0FromThread

mov rax, CK_THREADSEP*THREADS
sub r14, rax
add ebp, CK_BUCKETSIZE9
xor edx, edx
add r12, 0x40000000
_LoopTrimSide1FromThread:
lea r8, [r14+rbp]
mov eax, [r14+rbx*4+4]
lea r9, [r14+rax]
_LoopTrimSide1:
mov rax, [r8]
add r8, 6
mov esi, eax
mov rdi, rax
mov r15, rax
and eax, 0x1fff
shr esi, 13
shrx ecx, [r10+rax*4+0x8000], esi
and esi, 0x1f
add esi, [r10+rax*4]
shr r15, 36
and r15d, 0xfff
or r15d, edx
shr rdi, 6
and edi, 0x3ffff000
or esi, edi
mov edi, [r11+r15*4]
or rsi, r12
mov [r11+rdi], rsi
and ecx, 1
lea ecx, [rcx+rcx*4]
add edi, ecx
mov [r11+r15*4], edi
cmp r8, r9
jb _LoopTrimSide1
mov eax, CK_THREADSEP
add r14, rax
add edx, (8192/THREADS)
cmp edx, 8192
jb _LoopTrimSide1FromThread

add ebx, 2
mov rax, CK_THREADSEP*THREADS
sub r14, rax
add ebp, CK_BUCKETSIZE9*2
add r12, 0x40000000
test ebx, (8192/THREADS-2)
jnz _LoopSrcBucket

mov eax, [r11+CK_RENMAX0-CK_COUNTER0]
add eax, 63+16
and eax, -16
shr eax, 1
mov ecx, 2048
cmp eax, ecx
cmova eax, ecx
mov [r11+CK_RENMAX0-CK_COUNTER0+4], eax

pop r15
pop r14
pop r13
pop r12
pop rbp
pop rbx
ret

.section .rodata.cst32,"aM",@progbits,32
.align 64
.set a, CK_BUCKET14-CK_COUNTER0
yInitDest14:
.int a, a+CK_BUCKETSIZE14, a+CK_BUCKETSIZE14*2, a+CK_BUCKETSIZE14*3
.int a+CK_BUCKETSIZE14*4, a+CK_BUCKETSIZE14*5, a+CK_BUCKETSIZE14*6, a+CK_BUCKETSIZE14*7
u32initbucket14inc:
.int CK_BUCKETSIZE14*8
