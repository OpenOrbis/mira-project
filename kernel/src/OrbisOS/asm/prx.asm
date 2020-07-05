; prx.s
; Mira
; Use NASM for compile and edit Utilities

BITS 64
DEFAULT REL

magic: db 'MIRA'
entrypoint: dq loadprx
prxdone: db 0
prx_path: dq 0
sceKernelLoadStartModule: dq 0

str_rpcstub: db 'prxstub', 0

loadprx:
	;load prx
	mov rdi, qword [prx_path]
	xor rsi, rsi
	xor rdx, rdx
	xor rcx, rcx
	xor r8, r8
	xor r9, r9
	mov r12, qword [sceKernelLoadStartModule]
	call r12
	xor eax, eax
	ret
