.intel_syntax noprefix
.text

.global cpu_sidtRing01

cpu_sidtRing01:
  sidt [rdi]
  xor rax,rax
  ret