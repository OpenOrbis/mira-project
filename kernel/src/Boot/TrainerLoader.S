.intel_syntax noprefix
.text

.global _trainer_loader_start, _trainer_loader_end

.org 0x10
_padding:
	nop
	nop
	nop
	nop

_trainer_loader_start:
.incbin TRAINER_LOADER_BIN_PATH
_trainer_loader_end: