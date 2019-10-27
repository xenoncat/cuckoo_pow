#include "memlayout.h"
.intel_syntax noprefix
.text
.align 64

.globl round15
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
//r13
//r14
//r15 fromthread

.byte 0x66, 0x90
round15:
push rbx
push rbp
push r12
push r13
push r14
push r15

mov eax, CK_THREADSEP
imul rax, rsi
lea r10, [rdi+rax+CK_REN1]
mov ebx, esi
shl ebx, (13-LOGTHREADS)
imul ebp, ebx, CK_BUCKETSIZE14
add ebp, CK_BUCKET14-CK_COUNTER0+CK_BUCKETSIZE14
lea rsi, [rdi+CK_COUNTER0]

lea rax, [r10+CK_COUNTER1-CK_REN1]
mov r11, rax
vmovdqa ymm0, ymmword ptr yInitDest15[rip]
vpbroadcastd ymm1, dword ptr u32initbucket15inc[rip]
mov dword ptr [r10+CK_RENMAX1-CK_REN1], 0
mov ecx, 32
_LoopInitDstCtr:
vmovdqa [rax], ymm0
vpaddd ymm0, ymm0, ymm1
add rax, 32
sub ecx, 1
jnz _LoopInitDstCtr

_LoopSrcBucket:
mov eax, ebx
mov r12d, ebx
and eax, 0x1f00
and r12d, 0x3f
shl eax, 9
shl r12, 34
or r12, rax

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
shr eax, 17
and eax, 0x1fff
mov edx, [r10+rax*4]
shr edi, 12
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
sub ebp, CK_BUCKETSIZE14
mov r15d, THREADS
_LoopFilterSide0FromThread:
lea r8, [rsi+rbp]
mov rdx, r8
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopFilterSide0:
mov rax, [r8]
add r8, 5
mov [rdx], rax
mov edi, eax
shr eax, 17
and eax, 0x1fff
mov ecx, [r10+rax*4-0x8000]
shr edi, 12
bt ecx, edi
lea rcx, [rdx+5]
cmovc rdx, rcx
mov ecx, [r10+rax*4]
bts ecx, edi
mov [r10+rax*4], ecx
cmp r8, r9
jb _LoopFilterSide0
sub edx, esi
mov [rsi+rbx*4], edx
mov eax, CK_THREADSEP
add rsi, rax
sub r15d, 1
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
mov eax, [r11+CK_RENMAX1-CK_COUNTER1]
cmp edi, eax
cmova eax, edi
mov [r11+CK_RENMAX1-CK_COUNTER1], eax

mov rax, CK_THREADSEP*THREADS
sub rsi, rax
sub r10, 0x10000
xor edx, edx
mov r15d, THREADS
_LoopDistributeSide0FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopDistributeSide0:
mov rax, [r8]
add r8, 5
mov edi, eax
mov rcx, rax
shr eax, 17
and eax, 0x1fff
shr edi, 12
and edi, 0x1f
add edi, [r10+rax*4]
shl rdi, 22
mov rax, rcx
and ecx, 0xfff
shl ecx, 5
or rdi, rcx
shr rax, 30
sub eax, edx
and eax, 0x3ff
add edx, eax
mov ecx, edx
shr edx, 8
or rdi, rdx
mov edx, ecx
and ecx, 0xff
mov eax, [r11+rcx*4]
or rdi, r12
mov [r11+rax], rdi
add eax, 5
mov [r11+rcx*4], eax
cmp r8, r9
jb _LoopDistributeSide0
mov eax, CK_THREADSEP
add rsi, rax
sub r15d, 1
jnz _LoopDistributeSide0FromThread

mov rax, CK_THREADSEP*THREADS
sub rsi, rax
and edx, -1024
cmp edx, 0x1c00
jne _DensityError

add ebp, CK_BUCKETSIZE14
xor edx, edx
mov rax, 0x400000000
or r12, rax
mov r15d, THREADS
_LoopTrimSide1FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopTrimSide1:
mov rax, [r8]
add r8, 5
mov edi, eax
mov rcx, rax
shr eax, 17
and eax, 0x1fff
shr edi, 12
and edi, 0x1f
mov r14d, [r10+rax*4]
add r14d, edi
shl r14, 22
shrx edi, [r10+rax*4+0x8000], edi
mov rax, rcx
and ecx, 0xfff
shl ecx, 5
or r14, rcx
shr rax, 30
sub eax, edx
and eax, 0x3ff
add edx, eax
mov ecx, edx
shr edx, 8
or r14, rdx
mov edx, ecx
and ecx, 0xff
mov eax, [r11+rcx*4]
or r14, r12
mov [r11+rax], r14
and edi, 1
lea edi, [rdi+rdi*4]
add eax, edi
mov [r11+rcx*4], eax
cmp r8, r9
jb _LoopTrimSide1
mov eax, CK_THREADSEP
add rsi, rax
sub r15d, 1
jnz _LoopTrimSide1FromThread

mov rax, CK_THREADSEP*THREADS
sub rsi, rax
and edx, -1024
cmp edx, 0x1c00
jne _DensityError

add ebx, 2
add r10, 0x10000
add ebp, CK_BUCKETSIZE14*2
test ebx, (8192/THREADS-2)
jnz _LoopSrcBucket

_Epilog:
mov eax, [r11+CK_RENMAX1-CK_COUNTER1]
add eax, 63+16
and eax, -16
shr eax, 1
mov ecx, 2048
cmp eax, ecx
cmova eax, ecx
mov [r11+CK_RENMAX1-CK_COUNTER1+4], eax

pop r15
pop r14
pop r13
pop r12
pop rbp
pop rbx
ret

_DensityError:
mov dword ptr [rsi+CK_ABORT-CK_COUNTER0], 1
push r11
lea rdi, szDebug[rip]
mov esi, ebx
//mov edx, edx
xor eax, eax
call printf@PLT
pop r11
jmp _Epilog

.section .rodata.cst32,"aM",@progbits,32
.align 64
.set a, CK_BUCKET15-CK_COUNTER1
.globl yInitDest15
yInitDest15:
.int a, a+CK_BUCKETSIZE15, a+CK_BUCKETSIZE15*2, a+CK_BUCKETSIZE15*3
.int a+CK_BUCKETSIZE15*4, a+CK_BUCKETSIZE15*5, a+CK_BUCKETSIZE15*6, a+CK_BUCKETSIZE15*7
.globl u32initbucket15inc
u32initbucket15inc:
.int CK_BUCKETSIZE15*8
szDebug:
.string "Density Error %x %x\n"
