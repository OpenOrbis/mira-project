; rpcldr.s
; From golden a.k.a jogolden
; Edited for Mira
; Use NASM for compile and edit Utilities

BITS 64
DEFAULT REL

magic: db 'MIRA'
entrypoint: dq rpcldr
ldrdone: db 0
stubentry: dq 0
scePthreadAttrInit: dq 0
scePthreadAttrSetstacksize: dq 0
scePthreadCreate: dq 0
thr_initial: dq 0

hthread: dq 0
scePthreadAttr: dq 0
str_rpcstub: db 'rpcstub', 0

rpcldr:
	; load pthread data into fs
	mov rdi, qword [thr_initial]
	mov rsi, qword [rdi]
	mov rdi, qword [rsi + 0x1E0]
	call amd64_set_fsbase

	; create attr
	lea rdi, [scePthreadAttr]
	mov r12, qword [scePthreadAttrInit]
	call r12

	; set stack size
	mov rsi, 0x80000 ; 512 kb
	lea rdi, [scePthreadAttr]
	mov r12, qword [scePthreadAttrSetstacksize]
	call r12

	; create thread
	lea r8, [str_rpcstub]
	mov rcx, 0
	mov rdx, qword [stubentry]
	lea rsi, [scePthreadAttr]
	lea rdi, [hthread]
	mov r12, qword [scePthreadCreate]
	call r12

	mov byte [ldrdone], 1

	mov rdi, 0
	call sys_thr_exit
	retn

sys_thr_exit:
	mov rax, 431
	mov r10, rcx
	syscall
	retn

sys_sysarch:
	mov rax, 165
	mov r10, rcx
	syscall
	retn

amd64_set_fsbase:
	push rbp
	mov rbp, rsp
	push rbx
	sub rsp, 0x18

	mov [rbp - 0x18], rdi

	lea rsi, [rbp - 0x18]
	mov edi, 129
	call sys_sysarch

	add rsp, 0x18
	pop rbx
	pop rbp
	retn
