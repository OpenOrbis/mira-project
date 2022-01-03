#pragma once

extern "C"
{
    #include <sys/param.h>
    #include <sys/types.h>
    #include <sys/stdint.h>
}

void gdb_exceptionHandler(int exception_number, void *exception_address);
int gdb_start_server(int port, int pid);
void gdb_strcpy(char *dest, const char *src);
unsigned char gdb_getDebugChar();
int gdb_read(void *buff, int len);
void gdb_memset(void *buff, uint8_t value, int len);
void gdb_putDebugChar(unsigned char c);
int gdb_write(void *buff, int len);
void gdbstub_set_debug_traps (void);
int startsWith(const char *pre, const char *str);
unsigned long ps4gdb_strtoul(const char *nptr, char **endptr, int base);
int ps4gdb_start_cmd_server(int cmd_port, int gdb_port);

