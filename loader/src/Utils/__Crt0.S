.intel_syntax noprefix
.text

.global _start, _mira_elf_start, _mira_elf_end

.org 0
_start:
	jmp		mira_entry

.org 0x10
_padding:
	nop
	nop
	nop
	nop

_mira_elf_start:
.incbin MIRA_ELF_PATH
_mira_elf_end: