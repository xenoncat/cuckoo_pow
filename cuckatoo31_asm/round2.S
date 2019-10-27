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
sub rsp, 168
and rsp, -64
mov [rsp+160], rax

mov eax, CK_THREADSEP
imul rax, rsi
lea r10, [rdi+rax+CK_BITMAP]
mov ebx, esi
shl ebx, (13-LOGTHREADS)
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

mov ecx, THREADS*R1PHASES
_LoopCountSide1Phase:
lea rax, PhaseLookup2a[rip-8]
mov ebp, ebx
#if THREADS==4
imul ebp, [rax+rcx*2]
add ebp, [rax+rcx*2+4]
#elif THREADS==8
imul ebp, [rax+rcx]
add ebp, [rax+rcx+4]
#else
#error
#endif
_LoopCountSide1FromThread:
mov eax, [rsi+rbx*4]
add eax, 3
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
test ecx, (THREADS-1)
jnz _LoopCountSide1FromThread
mov rax, THREADS*CK_THREADSEP-32768
sub rsi, rax
test ecx, ecx
jnz _LoopCountSide1Phase

mov ecx, THREADS*R1PHASES
_LoopFilterSide0Phase:
mov rax, THREADS*CK_THREADSEP-32768
add rsi, rax
lea rax, PhaseLookup2b[rip-8]
mov ebp, ebx
#if THREADS==4
imul ebp, [rax+rcx*2]
add ebp, [rax+rcx*2+4]
#elif THREADS==8
imul ebp, [rax+rcx]
add ebp, [rax+rcx+4]
#else
#error
#endif
_LoopFilterSide0FromThread:
mov eax, CK_THREADSEP
sub rsi, rax
mov eax, [rsi+rbx*4-4]
add eax, 3
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax]
mov rdx, r8
mov eax, [rsi+rbx*4]
lea r9, [rsi+rax]
_LoopFilterSide0:
mov rax, [r8]
add r8, 5
mov [rdx], rax
mov edi, eax
and eax, 0x1fff
mov eax, [r10+rax*4]
shr edi, 13
bt eax, edi
lea rax, [rdx+5]
cmovc rdx, rax
cmp r8, r9
jb _LoopFilterSide0
sub edx, esi
mov eax, THREADS*R1PHASES
sub eax, ecx
mov [rsp+rax*4], edx
sub ecx, 1
test ecx, (THREADS-1)
jnz _LoopFilterSide0FromThread
test ecx, ecx
jnz _LoopFilterSide0Phase

xor edx, edx
mov ecx, THREADS*R1PHASES
_LoopDistributeSide0Phase:
lea rax, PhaseLookup2c[rip-8]
mov ebp, ebx
#if THREADS==4
imul ebp, [rax+rcx*2]
add ebp, [rax+rcx*2+4]
#elif THREADS==8
imul ebp, [rax+rcx]
add ebp, [rax+rcx+4]
#else
#error
#endif
_LoopDistributeSide0FromThread:
mov eax, [rsi+rbx*4-4]
add eax, 3
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax]
mov eax, [rsp+rcx*4-4]
lea r9, [rsi+rax]
_LoopDistributeSide0:
mov rax, [r8]
add r8, 5
mov edi, eax
mov r15, rax
and eax, 0x1fff
mov r14d, [r10+rax*4]
shr edi, 13
btr r14d, edi
mov [r10+rax*4], r14d
mov rax, r15
shr rax, 36
sub eax, edx
and eax, 0xf
add edx, eax
mov edi, [r11+rdx*4]
and r15, r13
or r15, r12
mov [r11+rdi], r15
add edi, 5
mov [r11+rdx*4], edi
cmp r8, r9
jb _LoopDistributeSide0
mov eax, CK_THREADSEP
add rsi, rax
sub ecx, 1
test ecx, (THREADS-1)
jnz _LoopDistributeSide0FromThread
mov rax, THREADS*CK_THREADSEP-32768
sub rsi, rax
test ecx, ecx
jnz _LoopDistributeSide0Phase
and edx, -16
cmp edx, 0x1ff0
jne _DensityError

sub rsi, 32768*R1PHASES
xor edx, edx
lea r12, [r12+r13+1]
mov ecx, THREADS*R1PHASES
_LoopTrimSide1Phase:
lea rax, PhaseLookup2d[rip-8]
mov ebp, ebx
#if THREADS==4
imul ebp, [rax+rcx*2]
add ebp, [rax+rcx*2+4]
#elif THREADS==8
imul ebp, [rax+rcx]
add ebp, [rax+rcx+4]
#else
#error
#endif
_LoopTrimSide1FromThread:
mov eax, [rsi+rbx*4]
add eax, 3
cmp eax, ebp
cmovb eax, ebp
lea r8, [rsi+rax]
mov r15, r8
mov r14, r8
mov eax, [rsi+rbx*4+4]
lea r9, [rsi+rax]
_LoopFilterSide1:
mov rax, [r8]
add r8, 5
mov [r15], rax
mov edi, eax
and eax, 0x1fff
mov eax, [r10+rax*4]
shr edi, 13
bt eax, edi
lea rax, [r15+5]
cmovnc r15, rax
cmp r8, r9
jb _LoopFilterSide1
_LoopDistributeSide1:
mov rax, [r14]
add r14, 5
mov rdi, rax
shr rax, 36
sub eax, edx
and eax, 0xf
add edx, eax
mov eax, [r11+rdx*4]
and rdi, r13
or rdi, r12
mov [r11+rax], rdi
add eax, 5
mov [r11+rdx*4], eax
cmp r14, r15
jb _LoopDistributeSide1
mov eax, CK_THREADSEP
add rsi, rax
sub ecx, 1
test ecx, (THREADS-1)
jnz _LoopTrimSide1FromThread
mov rax, THREADS*CK_THREADSEP-32768
sub rsi, rax
test ecx, ecx
jnz _LoopTrimSide1Phase

and edx, -16
cmp edx, 0x1ff0
jne _DensityError

add ebx, 2
sub rsi, 32768*R1PHASES
lea r12, [r12+r13+1]
test ebx, (8192/THREADS-2)
jnz _LoopSrcBucket

_Epilog:
mov rsp, [rsp+160]
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
.globl yInitDest2
yInitDest2:
.set a, CK_BUCKET2-CK_COUNTER0
.int a, a+CK_BUCKETSIZE2, a+CK_BUCKETSIZE2*2, a+CK_BUCKETSIZE2*3
.int a+CK_BUCKETSIZE2*4, a+CK_BUCKETSIZE2*5, a+CK_BUCKETSIZE2*6, a+CK_BUCKETSIZE2*7
PhaseLookup2a:
#if R1PHASES>4
.int CK_BUCKETSIZE1E, CK_BUCKET1E-(CK_COUNTER1+131072)+CK_BUCKETSIZE1E
#endif
.int CK_BUCKETSIZE1D, CK_BUCKET1D-(CK_COUNTER1+98304)+CK_BUCKETSIZE1D
.int CK_BUCKETSIZE1C, CK_BUCKET1C-(CK_COUNTER1+65536)+CK_BUCKETSIZE1C
.int CK_BUCKETSIZE1B, CK_BUCKET1B-(CK_COUNTER1+32768)+CK_BUCKETSIZE1B
.int CK_BUCKETSIZE1A, CK_BUCKET1A-(CK_COUNTER1)+CK_BUCKETSIZE1A
PhaseLookup2b:
.int CK_BUCKETSIZE1A, CK_BUCKET1A-(CK_COUNTER1)
.int CK_BUCKETSIZE1B, CK_BUCKET1B-(CK_COUNTER1+32768)
.int CK_BUCKETSIZE1C, CK_BUCKET1C-(CK_COUNTER1+65536)
.int CK_BUCKETSIZE1D, CK_BUCKET1D-(CK_COUNTER1+98304)
#if R1PHASES>4
.int CK_BUCKETSIZE1E, CK_BUCKET1E-(CK_COUNTER1+131072)
#endif
PhaseLookup2c:
#if R1PHASES>4
.int CK_BUCKETSIZE1E, CK_BUCKET1E-(CK_COUNTER1+131072)
#endif
.int CK_BUCKETSIZE1D, CK_BUCKET1D-(CK_COUNTER1+98304)
.int CK_BUCKETSIZE1C, CK_BUCKET1C-(CK_COUNTER1+65536)
.int CK_BUCKETSIZE1B, CK_BUCKET1B-(CK_COUNTER1+32768)
.int CK_BUCKETSIZE1A, CK_BUCKET1A-(CK_COUNTER1)
PhaseLookup2d:
#if R1PHASES>4
.int CK_BUCKETSIZE1E, CK_BUCKET1E-(CK_COUNTER1+131072)+CK_BUCKETSIZE1E
#endif
.int CK_BUCKETSIZE1D, CK_BUCKET1D-(CK_COUNTER1+98304)+CK_BUCKETSIZE1D
.int CK_BUCKETSIZE1C, CK_BUCKET1C-(CK_COUNTER1+65536)+CK_BUCKETSIZE1C
.int CK_BUCKETSIZE1B, CK_BUCKET1B-(CK_COUNTER1+32768)+CK_BUCKETSIZE1B
.int CK_BUCKETSIZE1A, CK_BUCKET1A-(CK_COUNTER1)+CK_BUCKETSIZE1A
.globl u32initbucket2inc
u32initbucket2inc:
.int CK_BUCKETSIZE2*8
szDebug:
.string "%lx %lx\n"
