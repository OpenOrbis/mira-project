.intel_syntax noprefix
.text

.global _start, _g_rdi, _g_rsi, _g_consoleHandle, _g_launchMode

.org 0
_start:
	jmp		loader_entry

# This is the launch mode
# 0 = normal
# 1 = injected
_g_launchMode: .byte 0x00

.org 0x10
_padding:
	nop
	nop
	nop
	nop


_g_rdi: .quad 0
_g_rsi: .quad 0
_g_consoleHandle: .int 0

#_mira_elf_start:
#.incbin MIRA_ELF_PATH
#_mira_elf_end: