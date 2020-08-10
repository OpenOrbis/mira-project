.intel_syntax noprefix
.text

.global kernelRdmsr
.global cpu_enable_wp
.global cpu_disable_wp

# This is needed for freestanding modules
.global memcpy
.global memmove
.global memset
.global memcmp
.global strlen
.global strcmp

kernelRdmsr:
	mov ecx, edi
	rdmsr
	shl rdx, 32
	or rax, rdx
	ret

cpu_enable_wp:
	mov rax, cr0
	or rax, 0x10000
	mov cr0, rax
	ret

cpu_disable_wp:
	mov rax, cr0
	and rax, ~0x10000
	mov cr0, rax
	ret

memcpy:
	# Preserve destination address because we have to return it.
	mov rax, rdi

	# Do a byte-by-byte move. On modern x86 chips, there is not a significant
	# performance difference between byte-wise and word-wise moves. In fact,
	# sometimes movsb is faster. See instlatx64.atw.hu for benchmarks.
	mov rcx, rdx
	rep movsb

	ret

memmove:
	# Preserve destination address because we have to return it.
	mov rax, rdi

	# If dest < src, we can always do a fast pointer-incrementing move.
	# If dest == src, do nothing.
	cmp rdi, rsi
	je .done
	jb .fast

	# If dest > src and there are no overlapping regions (dest >= src+num), we
	# can still do a fast pointer-incrementing move.
	mov rcx, rsi
	add rcx, rdx
	cmp rdi, rcx
	jae .fast

	# If dest > src and dest < src+num, we have to do a right-to-left move to
	# preserve overlapping data.
	.slow:

		# Set the direction flag so copying is right-to-left.
		std

		# Set the move count register.
		mov rcx, rdx

		# Update pointers to the right-hand side (minus one).
		dec rdx
		add rsi, rdx
		add rdi, rdx

		# Do a byte-by-byte move.
		rep movsb

		# Reset the direction flag.
		cld

		ret

	.fast:

		# Do a byte-by-byte move.
		mov rcx, rdx
		rep movsb

	.done:

		ret

memset:
	# Preserve the original destination address.
	mov r8, rdi

	# The value to store is the second parameter (rsi).
	mov rax, rsi

	# Do a byte-by-byte store.
	mov rcx, rdx
	rep stosb

	# Return the original destination address.
	mov rax, r8
	ret

memcmp:																	# @memcmp
		test rdx, rdx
		je .LBB0_5
		xor ecx, ecx
.LBB0_2:																# =>This Inner Loop Header: Depth=1
		movzx eax, byte ptr [rdi + rcx]
		movzx r8d, byte ptr [rsi + rcx]
		cmp al, r8b
		jne .LBB0_3
		add rcx, 1
		cmp rdx, rcx
		jne .LBB0_2
.LBB0_5:
		xor eax, eax
		ret
.LBB0_3:
		sub eax, r8d
		ret

strlen:
	# save rdi
	push rdi

	# *al = '\0'
	xor al, al
	# clear rcx
	xor rcx, rcx
	# *rcx = ~*rcx
	not rcx
	# while (*al /* '\0' */ != *rdi) ++rdi, --*rcx#
	cld
	repne scasb
	# *rcx = ~*rcx
	not rcx
	dec rcx

	# set return value
	mov rax, rcx
	# restore rdi
	pop rdi

	ret

strcmp:
myasm_strcmp:
	mov r8, 0 #i = 0
	mov r10, 0
myasm_loop:
	mov r9b, [rdi + r8]
	mov r10b, [rsi + r8]

	cmp r9b, r10b
	jne myasm_end

	cmp r9b, 0
	je myasm_end

	inc r8 #i++
	jmp myasm_loop

myasm_end:
	mov rax, 0
	mov al, r9b
	sub rax, r10
	ret
