; entrypoint_trigger.s
; Mira - OpenOrbis Team
; Detect the end of preload module and execute action
; Use NASM for compile and edit Substitute

BITS 64
DEFAULT REL

magic: db 'MIRA'
entrypoint: dq preload_prx_hook
epdone: dd 0
sceSysmodulePreloadModuleForLibkernel: dq 0
fakeReturnAddress: dq 0

doSubstitute: db 'doSubstituteLoadPRX', 0

preload_prx_hook:
	; Save R9
	push r9

	; Call the preload module function
	mov r9, qword [sceSysmodulePreloadModuleForLibkernel]
	call r9

	; Restore R9
	pop r9

	; Save register
	push rdi
	push rsi
	push rdx
	push rax

	; Call dynlib_dlsym with custom name
	mov rdi, 0
	lea rsi, [doSubstitute]
	lea rdx, [fakeReturnAddress]
	mov rax, 591
	syscall

	; restore register
	pop rax
	pop rdx
	pop rsi
	pop rdi
	ret
