# Mira Loader Component

MiraLoader is designed to embed multi-stage elf's or payloads internally and start their execution from *anywhere* in memory. The code has been compiled in a way that as long as the payload is loaded into memory at any random address with proper execution permission flags that it will run.

## Support for
  - Usermode ELF's
  - Usermode payloads
  - Kernel ELF's (change interpreter string to begin with `kern` to execute entrypoint as new kernel process)

### Behavior
When the loader first starts depending on how it was compiled. If it was compiled with `_SOCKET_LOADER` defined it will wait on port `9021` for a payload (first instruction at :0 is `jmp`) or ELF. If the ELF header is found in the first 4 bytes, the ElfLoader will check for the .interp section in the ELF then check the data to see if it begins with `kern`. If it does it will execute as a kernel process, otherwise it will relocate the ELF in current memory space in the current process and jump to it.

If the loader was not compiled with `_SOCKET_LOADER` defined, then it will use some linker and compiler magic to embed trainer_loader and mira's elf into itself for a all-in-one loader binary file that you can use with any payload loader. No need for sockets or waiting for payloads!

The entrypoint is `mira_entry` which is determined by the linker script. Using MiraLoader you *MUST NOT* use any `static` variables, they will not be relocated and cause out of memory errors or kernel panics.
