.intel_syntax noprefix
.text

.global syscall1, syscall2, syscall3, syscall4, syscall5, syscall6, _mmap
.extern __error

syscall:
    mov rax,rdi
    syscall
    jb _error
	ret

syscall1:
    mov rax,rdi
    mov rdi,rsi
    syscall
    jb _error
	ret

syscall2:
    mov rax,rdi
    mov rdi,rsi
    mov rsi,rdx
    syscall
    jb _error
	ret

syscall3:
    mov rax,rdi
    mov rdi,rsi
    mov rsi,rdx
    mov rdx,rcx
    syscall
    jb _error
	ret

syscall4:
    mov rax,rdi
    mov rdi,rsi
    mov rsi,rdx
    mov rdx,rcx
    mov r10,r8
    syscall
    jb _error
	ret

syscall5:
    mov rax,rdi
    mov rdi,rsi
    mov rsi,rdx
    mov rdx,rcx
    mov r10,r8
    mov r8,r9
    syscall
    jb _error
	ret

syscall6:
	mov rax,rdi
    mov rdi,rsi
    mov rsi,rdx
    mov rdx,rcx
    mov r10,r8
    mov r8,r9
	mov r9, [rsp]
    syscall
    jb _error
	ret

_mmap:
	mov rax, 477
	mov r10, rcx
	syscall
	ret

# Copy pasted from ps4-payload-sdk
_error:
	cmp qword ptr __error[rip], 0
	jz _end
	push rax
	call __error[rip]
	pop rcx
	mov [rax], ecx
	mov rax, -1
	mov rdx, -1

_end:
	ret