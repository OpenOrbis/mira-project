.intel_syntax noprefix
.text

.global _start, _g_rdi, _g_rsi

.org 0
_start:
	jmp		loader_entry

.org 0x10
_padding:
	nop
	nop
	nop
	nop


_g_rdi: .quad 0
_g_rsi: .quad 0

#_mira_elf_start:
#.incbin MIRA_ELF_PATH
#_mira_elf_end: