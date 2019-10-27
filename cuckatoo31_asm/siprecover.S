#include "memlayout.h"
.intel_syntax noprefix
.text
.align 64

.macro SIPROUND
vpaddq ymm0, ymm0, ymm1
vpsllq ymm8, ymm1, 13
vpsrlq ymm1, ymm1, 51
vpxor ymm8, ymm8, ymm0
vpaddq ymm2, ymm2, ymm3
vpshufb ymm3, ymm3, ymm10
vpxor ymm1, ymm8, ymm1
vpxor ymm3, ymm3, ymm2
vpshufd ymm0, ymm0, 0xb1
vpaddq ymm2, ymm2, ymm1
vpsllq ymm8, ymm1, 17
vpsrlq ymm1, ymm1, 47
vpxor ymm8, ymm8, ymm2
vpaddq ymm0, ymm0, ymm3
vpxor ymm1, ymm8, ymm1
vpsllq ymm8, ymm3, 21
vpsrlq ymm3, ymm3, 43
vpxor ymm8, ymm8, ymm0
vpshufd ymm2, ymm2, 0xb1
vpxor ymm3, ymm8, ymm3
.endm

.macro SIPBEGIN
vpxor ymm3, ymm15, ymm4
vpaddq ymm2, ymm14, ymm3
vpshufb ymm3, ymm3, ymm10
vpxor ymm3, ymm3, ymm2
vpaddq ymm2, ymm13, ymm2
vpaddq ymm0, ymm12, ymm3
vpxor ymm1, ymm11, ymm2
vpshufd ymm2, ymm2, 0xb1
vpsllq ymm8, ymm3, 21
vpsrlq ymm3, ymm3, 43
vpxor ymm8, ymm8, ymm0
vpxor ymm3, ymm8, ymm3
.endm

.macro SIPEND
vpaddq ymm0, ymm0, ymm1
vpsllq ymm8, ymm1, 13
vpsrlq ymm1, ymm1, 51
vpxor ymm8, ymm8, ymm0
vpaddq ymm2, ymm2, ymm3
vpshufb ymm3, ymm3, ymm10
vpxor ymm1, ymm8, ymm1
vpxor ymm3, ymm3, ymm2
vpaddq ymm2, ymm2, ymm1
vpsllq ymm8, ymm1, 17
vpsrlq ymm1, ymm1, 47
vpxor ymm8, ymm8, ymm2
vpxor ymm1, ymm8, ymm1
vpsllq ymm8, ymm3, 21
vpsrlq ymm3, ymm3, 43
vpshufd ymm2, ymm2, 0xb1
vpxor ymm3, ymm8, ymm3
vpxor ymm1, ymm1, ymm2
vpxor ymm1, ymm1, ymm3
.endm

.globl siprecover
//rdi = ctx
//rsi = tid
.byte 0x0f, 0x1f, 0x80, 0,0,0,0
siprecover:
//rax
//rcx loop counter
//rdx pu32BitmapFilter0
//rbx u32NumSols42
//rbp
//rsi
//rdi pu32Header
//r8
//ymm4 nonce
//ymm5 nonce bump
//ymm6 qconstnodemask
//ymm7 qconstbucketmask = 0x1fff = 8192 uint32 words filter bitmap
//ymm8 temp for rotate
//ymm9 constff
//ymm10 constrol16

push rbx
push rbp
mov rax, rsp
sub rsp, 104
and rsp, -64
mov [rsp+96], rax

vpbroadcastq ymm12, [rdi+CK_BLAKE2BSTATE]
vpbroadcastq ymm13, [rdi+CK_BLAKE2BSTATE+8]
vpbroadcastq ymm14, [rdi+CK_BLAKE2BSTATE+16]
vpbroadcastq ymm15, [rdi+CK_BLAKE2BSTATE+24]
vpbroadcastq ymm11, [rdi+CK_BLAKE2BSTATE+32]

imul ebx, [rdi], 42
lea rdx, [rdi+CK_BITMAP]
add rdi, CK_RECOVERWRITE
shl esi, (32-LOGTHREADS)
vmovd xmm4, esi
vpbroadcastq ymm4, xmm4
vpaddq ymm4, ymm4, ymmword ptr yinit[rip]
vpbroadcastq ymm5, qconst8[rip]
vpbroadcastq ymm9, qconstff[rip]
vpbroadcastq ymm7, qconstbucketmask[rip]
vpbroadcastq ymm6, qconstnodemask[rip]
vmovdqa ymm10, ymmword ptr yconstrol16[rip]

mov ecx, (536870912/THREADS)
_LoopSip:
SIPBEGIN
SIPROUND
vpxor ymm0, ymm4, ymm0
vpxor ymm2, ymm9, ymm2
SIPROUND
SIPROUND
SIPROUND
SIPEND

//ymm1
//30:1
//18:6 5:1
//30:18 5:1
vpsrlq ymm0, ymm1, 6
vpand ymm0, ymm7, ymm0
vmovdqa [rsp], ymm0
vpsrlq ymm2, ymm1, 1
vmovdqa [rsp+0x20], ymm2
vpand ymm1, ymm6, ymm1
vmovdqa [rsp+0x40], ymm1

vmovd eax, xmm0
vmovd esi, xmm2
mov eax, [rdx+rax*4]
bt eax, esi
jnc _No0
vmovd eax, xmm1
mov r8d, eax
shr eax, 18
mov eax, [rdx+rax*4+0x8000]
bt eax, esi
jnc _No0
vmovd esi, xmm4
call _ProcAddEdge
_No0:
mov eax, [rsp+0x08]
mov esi, [rsp+0x28]
mov eax, [rdx+rax*4]
bt eax, esi
jnc _No1
mov eax, [rsp+0x48]
mov r8d, eax
shr eax, 18
mov eax, [rdx+rax*4+0x8000]
bt eax, esi
jnc _No1
vmovd esi, xmm4
add esi, 2
call _ProcAddEdge
_No1:
mov eax, [rsp+0x10]
mov esi, [rsp+0x30]
mov eax, [rdx+rax*4]
bt eax, esi
jnc _No2
mov eax, [rsp+0x50]
mov r8d, eax
shr eax, 18
mov eax, [rdx+rax*4+0x8000]
bt eax, esi
jnc _No2
vmovd esi, xmm4
add esi, 4
call _ProcAddEdge
_No2:
mov eax, [rsp+0x18]
mov esi, [rsp+0x38]
mov eax, [rdx+rax*4]
bt eax, esi
jnc _No3
mov eax, [rsp+0x58]
mov r8d, eax
shr eax, 18
mov eax, [rdx+rax*4+0x8000]
bt eax, esi
jnc _No3
vmovd esi, xmm4
add esi, 6
call _ProcAddEdge
_No3:
vpaddq ymm4, ymm4, ymm5
sub ecx, 1
jnz _LoopSip

mov rsp, [rsp+96]
pop rbp
pop rbx
ret

.align 64
_ProcAddEdge:
//r8d node
//esi {edge, 1}
xor ebp, ebp
_LoopMatchTarget:
mov eax, [rdx+rbp*4+8-CK_BITMAP]
xor eax, r8d
test eax, -2
jz _FoundTarget
add ebp, 2
cmp ebp, ebx
jb _LoopMatchTarget
ret
_FoundTarget:
or ebp, eax
lea eax, [rbx+rbx]
lock xadd [rdi+rbp*4], eax
shr esi, 1
mov [rdi+rax*4], rsi
add esi, esi
//round up to multiple of 42
imul ebp, ebp, 6242
shr ebp, 18
add ebp, 1
imul ebp, ebp, 42
cmp ebp, ebx
jb _LoopMatchTarget
ret

.section .rodata.cst32,"aM",@progbits,32
.align 64
qconstnodemask:
.quad 0x7fffffff
