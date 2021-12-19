#pragma once

extern "C" void __stack_chk_fail(void);
extern "C" unsigned long __stack_chk_guard;
extern "C" void __stack_chk_guard_setup(void);