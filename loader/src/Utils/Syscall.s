.intel_syntax noprefix
.text

.global syscall, syscall1, syscall2, syscall3, syscall4, syscall5, syscall6, _mmap

syscall:
    mov rax,0
    mov r10,rcx
    syscall
    ret


syscall1:
    mov rax,rdi
    mov rdi,rsi
    syscall
    ret

syscall2:
    mov rax,rdi
    mov rdi,rsi
    mov rsi,rdx
    syscall
    ret

syscall3:
    mov rax,rdi
    mov rdi,rsi
    mov rsi,rdx
    mov rdx,rcx
    syscall
    ret

syscall4:
    mov rax,rdi
    mov rdi,rsi
    mov rsi,rdx
    mov rdx,rcx
    mov r10,r8
    syscall
    ret

syscall5:
    mov rax,rdi
    mov rdi,rsi
    mov rsi,rdx
    mov rdx,rcx
    mov r10,r8
    mov r8,r9
    syscall
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
    ret

_mmap:
	mov rax, 477
	mov r10, rcx
	syscall
	ret
