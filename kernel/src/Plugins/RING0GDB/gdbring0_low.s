.intel_syntax noprefix
.text
.global return_to_prog
.global catch_exception_03
.global catch_exception_01
.extern getRegistersAddress
.extern set_mem_err
.extern print_register_info
.extern gdb_close_client_socket


return_to_prog:
	// Restore RIP	
	mov rdi, [rsp+0x40]
	mov [rsp+0x300], rdi

	// Restore flags
	mov rdi, [rsp+0x48]
	mov [rsp+0x310], rdi

	// Restore rsp
	mov rdi, [rsp+0x30]
	mov [rsp+0x318], rdi	

	// Restore registers
	mov rdi, [rsp+0x00]
	mov rax, [rsp+0x08]
	mov rbx, [rsp+0x10]
	mov rcx, [rsp+0x18]
	mov rdx, [rsp+0x20]
	mov rsi, [rsp+0x28]
	mov rbp, [rsp+0x38]
	mov r8,  [rsp+0x50]
	mov r9,  [rsp+0x58]
	mov r10, [rsp+0x60]
	mov r11, [rsp+0x68]
	mov r12, [rsp+0x70]
	mov r13, [rsp+0x78]
	mov r14, [rsp+0x80]
	mov r15, [rsp+0x88]

	// Adjust stack
	add rsp,0x300
	// Return from interrupt
	iretq

catch_exception_01:
	// Make some space on stack
	sub rsp,0x300

	// Store registers	
	mov [rsp+0x00],rdi
	mov [rsp+0x08],rax
	mov [rsp+0x10],rbx
	mov [rsp+0x18],rcx
	mov [rsp+0x20],rdx
	mov [rsp+0x28],rsi
	mov [rsp+0x38],rbp
	mov [rsp+0x50],r8
	mov [rsp+0x58],r9
	mov [rsp+0x60],r10
	mov [rsp+0x68],r11
	mov [rsp+0x70],r12
	mov [rsp+0x78],r13
	mov [rsp+0x80],r14
	mov [rsp+0x88],r15
	mov [rsp+0xA0],ds
	mov [rsp+0xA8],es
	mov [rsp+0xB0],fs
	mov [rsp+0xB8],gs

	// this int has no error code
	xor rsi,rsi
	mov [rsp+0xC0],rsi

    // Store old rsp value
	mov rsi,[rsp+0x318]
    mov [rsp+0x30],rsi
	
    // store return address
	mov rsi,[rsp+0x300]
	mov [rsp+0x40],rsi

	// store eflags
	mov rsi,[rsp+0x310]
   	mov [rsp+0x48], rsi	
	
	// store cs
	mov rsi,[rsp+0x308]
	mov [rsp+0x90],rsi

	// store ss
	mov rsi,[rsp+0x320]
	mov [rsp+0x98],rsi
	
    // Call handler
	mov rdi,0x01
	xor rsi,rsi
	xor rdx,rdx
	mov rcx,rsp
	call handle_exceptionRing0
	jmp return_to_prog


catch_exception_03:
	// Make some space on stack
	sub rsp,0x300

	// Store registers	
	mov [rsp+0x00],rdi
	mov [rsp+0x08],rax
	mov [rsp+0x10],rbx
	mov [rsp+0x18],rcx
	mov [rsp+0x20],rdx
	mov [rsp+0x28],rsi
	mov [rsp+0x38],rbp
	mov [rsp+0x50],r8
	mov [rsp+0x58],r9
	mov [rsp+0x60],r10
	mov [rsp+0x68],r11
	mov [rsp+0x70],r12
	mov [rsp+0x78],r13
	mov [rsp+0x80],r14
	mov [rsp+0x88],r15
	mov [rsp+0xA0],ds
	mov [rsp+0xA8],es
	mov [rsp+0xB0],fs
	mov [rsp+0xB8],gs

	// this int has no error code
	xor rsi,rsi
	mov [rsp+0xC0],rsi

    // Store old rsp value
	mov rsi,[rsp+0x318]
    mov [rsp+0x30],rsi
	
    // store return address
	mov rsi,[rsp+0x300]
	mov [rsp+0x40],rsi

	// store eflags
	mov rsi,[rsp+0x310]
   	mov [rsp+0x48], rsi	
	
	// store cs
	mov rsi,[rsp+0x308]
	mov [rsp+0x90],rsi

	// store ss
	mov rsi,[rsp+0x320]
	mov [rsp+0x98],rsi
	
    // Call handler
	mov rdi,0x03
	xor rsi,rsi
	xor rdx,rdx
	mov rcx,rsp
	call handle_exceptionRing0
	jmp return_to_prog
