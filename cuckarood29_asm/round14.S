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
//r14
//r15 fromthread

round14:
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
mov r12d, esi
lea rsi, [rdi+CK_COUNTER1]
shl r12, (40-LOGTHREADS)

lea rax, [r10+CK_COUNTER0-CK_BITMAP]
mov r11, rax
vmovdqa ymm0, ymmword ptr yInitDest2[rip]
vpbroadcastd ymm1, dword ptr u32initbucket2inc[rip]
mov dword ptr [r10+CK_RENMAX0-CK_BITMAP], 0

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
xor ebx, 1
mov r15d, THREADS
_LoopFilterSide1FromThread:
lea r8, [rsi+rbp]
mov rdx, r8
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopFilterSide1:
mov rax, [r8]
add r8, 5
mov [rdx], rax
mov rcx, rax
shr eax, 18
and eax, 0x1fff
shr rcx, 31
mov edi, [r10+rax*4-0x8000]
bt edi, ecx
lea rdi, [rdx+5]
cmovc rdx, rdi
mov edi, [r10+rax*4]
bts edi, ecx
mov [r10+rax*4], edi
cmp r8, r9
jb _LoopFilterSide1
sub edx, esi
mov [rsi+rbx*4], edx
add rsi, CK_THREADSEP
sub r15d, 1
jnz _LoopFilterSide1FromThread

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

mov eax, CK_THREADSEP*THREADS
sub rsi, rax
xor edx, edx
sub r10, 0x10000
xor r11, 4
mov r15d, THREADS
_LoopDistributeSide1FromThread:
lea r8, [rsi+rbp]
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopDistributeSide1:
mov rax, [r8]
add r8, 5
mov rcx, rax
mov rdi, rax
shr eax, 18
and eax, 0x1fff
shr rcx, 31
and ecx, 0x1f
add ecx, [r10+rax*4]
mov rax, rdi
shr rax, 36
sub eax, edx
and eax, 0xf
add edx, eax
shl ecx, 18
and edi, 0x3ffff
or ecx, edi
mov edi, [r11+rdx*8]
or rcx, r12
mov [r11+rdi-4], rcx
add edi, 5
mov [r11+rdx*8], edi
cmp r8, r9
jb _LoopDistributeSide1
add rsi, CK_THREADSEP
sub r15d, 1
jnz _LoopDistributeSide1FromThread

and edx, -16
cmp edx, 0x3f0
jne _DensityError

mov eax, CK_THREADSEP*THREADS
sub rsi, rax
sub ebp, CK_BUCKETSIZE3
xor ebx, 1
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
mov rdi, rax
shr eax, 18
and eax, 0x1fff
shr r14, 31
shrx ecx, [r10+rax*4+0x8000], r14d
and r14d, 0x1f
add r14d, [r10+rax*4]
mov rax, rdi
shr rax, 36
sub eax, edx
and eax, 0xf
add edx, eax
shl r14d, 18
and edi, 0x3ffff
or r14d, edi
mov edi, [r11+rdx*8]
or r14, r12
mov [r11+rdi], r14
and ecx, 1
lea ecx, [rcx+rcx*4]
add edi, ecx
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
mov eax, CK_THREADSEP*THREADS
sub rsi, rax
add ebp, CK_BUCKETSIZE3*2
add r12, 0x40000000
test ebx, (2048/THREADS-2)
jnz _LoopSrcBucket

mov eax, [r11+CK_RENMAX0-CK_COUNTER0]
add eax, 63+1
mov ecx, 4096
cmp eax, ecx
cmova eax, ecx
mov [r11+CK_RENMAX0-CK_COUNTER0+4], eax

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
