#include "memlayout.h"
.intel_syntax noprefix
.text
.align 64

.globl round95
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

round95:
push rbx
push rbp
push r12
push r13
push r14
push r15

mov eax, CK_THREADSEP
imul rax, rsi
lea r10, [rdi+rax+CK_REN2]
mov r13d, [rdi+rax+CK_RENMAX1+4]
mov dword ptr [rdi+rax+CK_RENMAX2], 0
mov ebx, esi
shl ebx, (8-LOGTHREADS)
imul ebp, ebx, CK_BUCKETSIZE16
add ebp, CK_BUCKET16-CK_COUNTER0+CK_BUCKETSIZE16
lea rsi, [rdi+CK_COUNTER0]
mov r12d, ebx
shl r12d, 17

lea rax, [r10+CK_COUNTER1-CK_REN2]
mov r11, rax
vmovdqa ymm0, ymmword ptr yInitDest95[rip]
vpbroadcastd ymm1, dword ptr u32initbucket95inc[rip]
mov dword ptr [r10+CK_RENMAX2-CK_REN2], 0
mov ecx, 128
_LoopInitDstCtr:
vmovdqa [rax], ymm0
vpaddd ymm0, ymm0, ymm1
add rax, 32
sub ecx, 1
jnz _LoopInitDstCtr

_LoopSrcBucket:
mov rdi, r10
xor eax, eax
mov ecx, 2048
rep stosq

mov ecx, THREADS
_LoopCountSide1FromThread:
lea r8, [rsi+rbp+2]
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopCountSide1:
mov eax, [r8]
add r8, 5
mov edi, eax
shr eax, 6
and eax, 0xfff
mov edx, [r10+rax*4]
shr edi, 1
bts edx, edi
mov [r10+rax*4], edx
cmp r8, r9
jb _LoopCountSide1
mov eax, CK_THREADSEP
add rsi, rax
sub ecx, 1
jnz _LoopCountSide1FromThread

add r10, 0x4000
mov rdi, r10
xor eax, eax
mov ecx, 2048
rep stosq

mov rax, CK_THREADSEP*THREADS
sub rsi, rax
sub ebp, CK_BUCKETSIZE16
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
shr rax, 22
and eax, 0xfff
mov ecx, [r10+rax*4-0x4000]
shr edi, 17
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
mov ecx, 0x200
_LoopAnding:
vmovdqa ymm0, [rax-0x4000]
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
mov [r10-0x4000], ecx
or rdx, rax
mov rax, rdx
not rdx
tzcnt rcx, rdx
shrx rax, rax, rcx
add edi, ecx
_WordIsZero:
add r10, 4
test r10, 0x3fff
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
mov eax, [r11+CK_RENMAX2-CK_COUNTER1]
cmp edi, eax
cmova eax, edi
mov [r11+CK_RENMAX2-CK_COUNTER1], eax

mov rax, CK_THREADSEP*THREADS
sub rsi, rax
sub r10, 0x8000
xor edx, edx
_LoopDistributeSide0FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopDistributeSide0:
mov rax, [r8]
add r8, 5
mov edi, eax
mov rcx, rax
shr rax, 22
and eax, 0xfff
shr edi, 17
and edi, 0x1f
add edi, [r10+rax*4]
shl rdi, 25
mov rax, rcx
and ecx, 0x1ffff
or rdi, rcx
shr rax, 34
and eax, 0x3f
or eax, edx
and ecx, 3
shl ecx, 8
or ecx, eax
mov eax, [r11+rcx*4]
or rdi, r12
mov [r11+rax], rdi
add eax, 5
mov [r11+rcx*4], eax
cmp r8, r9
jb _LoopDistributeSide0
mov eax, CK_THREADSEP
add rsi, rax
add edx, (256/THREADS)
cmp edx, 256
jb _LoopDistributeSide0FromThread

mov rax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE16
xor edx, edx
add r12d, 0x20000
_LoopTrimSide1FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopTrimSide1:
mov rax, [r8]
add r8, 5
mov edi, eax
mov rcx, rax
shr rax, 22
and eax, 0xfff
shr edi, 17
and edi, 0x1f
mov r14d, [r10+rax*4]
add r14d, edi
shl r14, 25
shrx edi, [r10+rax*4+0x4000], edi
mov rax, rcx
and ecx, 0x1ffff
or r14, rcx
shr rax, 34
and eax, 0x3f
or eax, edx
and ecx, 3
shl ecx, 8
or ecx, eax
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
add edx, (256/THREADS)
cmp edx, 256
jb _LoopTrimSide1FromThread

add ebx, 2
add r10, 0x8000
add r12d, 0x20000
mov rax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE16*2
test ebx, (256/THREADS-2)
jnz _LoopSrcBucket

pop r15
pop r14
pop r13
pop r12
pop rbp
pop rbx
ret

.section .rodata.cst32,"aM",@progbits,32
.align 64
.set a, CK_BUCKET95-CK_COUNTER1
.globl yInitDest95
yInitDest95:
.int a, a+CK_BUCKETSIZE95, a+CK_BUCKETSIZE95*2, a+CK_BUCKETSIZE95*3
.int a+CK_BUCKETSIZE95*4, a+CK_BUCKETSIZE95*5, a+CK_BUCKETSIZE95*6, a+CK_BUCKETSIZE95*7
.globl u32initbucket95inc
u32initbucket95inc:
.int CK_BUCKETSIZE95*8
