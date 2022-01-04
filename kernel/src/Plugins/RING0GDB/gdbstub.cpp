/****************************************************************************

		THIS SOFTWARE IS NOT COPYRIGHTED

   HP offers the following for use in the public domain.  HP makes no
   warranty with regard to the software or it's performance and the
   user accepts the software "AS IS" with all faults.

   HP DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD
   TO THIS SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

****************************************************************************/

/****************************************************************************
 *  Header: remcom.c,v 1.34 91/03/09 12:29:49 glenne Exp $
 *
 *  Module name: remcom.c $
 *  Revision: 1.34 $
 *  Date: 91/03/09 12:29:49 $
 *  Contributor:     Lake Stevens Instrument Division$
 *
 *  Description:     low level support for gdb debugger. $
 *
 *  Considerations:  only works on target hardware $
 *
 *  Written by:      Glenn Engel $
 *  ModuleState:     Experimental $
 *
 *  NOTES:           See Below $
 *
 *  Modified for 386 by Jim Kingdon, Cygnus Support.
 *
 *  To enable debugger support, two things need to happen.  One, a
 *  call to set_debug_traps() is necessary in order to allow any breakpoints
 *  or error conditions to be properly intercepted and reported to gdb.
 *  Two, a breakpoint needs to be generated to begin communication.  This
 *  is most easily accomplished by a call to breakpoint().  Breakpoint()
 *  simulates a breakpoint by executing a trap #1.
 *
 *  The external function exceptionHandler() is
 *  used to attach a specific handler to a specific 386 vector number.
 *  It should use the same privilege level it runs at.  It should
 *  install it as an interrupt gate so that interrupts are masked
 *  while the handler runs.
 *
 *  Because gdb will sometimes write to the stack area to execute function
 *  calls, this program cannot rely on using the supervisor stack so it
 *  uses it's own stack area reserved in the int array remcomStack.
 *
 *************
 *
 *    The following gdb commands are supported:
 *
 * command          function                               Return value
 *
 *    g             return the value of the CPU registers  hex data or ENN
 *    G             set the value of the CPU registers     OK or ENN
 *
 *    mAA..AA,LLLL  Read LLLL bytes at address AA..AA      hex data or ENN
 *    MAA..AA,LLLL: Write LLLL bytes at address AA.AA      OK or ENN
 *
 *    c             Resume at current address              SNN   ( signal NN)
 *    cAA..AA       Continue at address AA..AA             SNN
 *
 *    s             Step one instruction                   SNN
 *    sAA..AA       Step one instruction from AA..AA       SNN
 *
 *    k             kill
 *
 *    ?             What was the last sigval ?             SNN   (signal NN)
 *
 * All commands and responses are sent with a packet which includes a
 * checksum.  A packet consists of
 *
 * $<packet info>#<checksum>.
 *
 * where
 * <packet info> :: <characters representing the command or response>
 * <checksum>    :: < two hex digits computed as modulo 256 sum of <packetinfo>>
 *
 * When a packet is received, it is first acknowledged with either '+' or '-'.
 * '+' indicates a successful transfer.  '-' indicates a failed transfer.
 *
 * Example:
 *
 * Host:                  Reply:
 * $m0,10#2a               +$00010203040506070809101112131415#42
 *
 ****************************************************************************/

extern "C" {
#include <sys/param.h>
#include <sys/types.h>
#include <sys/filedesc.h>
#include <sys/proc.h>
#include <sys/stdint.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <sys/unistd.h>
#include <vm/vm.h>
#include <sys/malloc.h>
}

#include "kdl.h"
#include "log.h"
#include "syscalls.hpp"

int startsWith(const char *pre, const char *str);
int gdb_strlen(char *str);
int gdb_vm_fault_disable_pagefaults();
void gdb_vm_fault_enable_pagefaults(int val);
int get_gdb_server_socket();
void set_gdb_server_socket(int val);
extern "C"
{
    extern void gdb_exceptionHandlerRing0(int exception_number, uint64_t func);
}
int gdb_start_server(int port, int pid);
void gdb_strcpy(char *dest, const char *src);
unsigned char gdb_getDebugCharRing0();
int gdb_readRing0(void *buff, int len);
void gdb_memset(void *buff, uint8_t value, int len);
void gdb_putDebugCharRing0(unsigned char c);
int gdb_writeRing0(void *buff, int len);
void gdbstub_set_debug_traps (void);
int startsWith(const char *pre, const char *str);


extern "C"
{
	void cpu_sidtRing0(void*);
	void cpu_enable_wp();
	void cpu_disable_wp();
	struct thread *cpu_get_gs0();
}

void putDebugCharRing0(char c)
{
	PS4GDB_kprintf();

	unsigned char tmpStr[2];
	tmpStr[1] = 0;
	tmpStr[0] = c;
	kprintf("%s",tmpStr);

	gdb_putDebugCharRing0((unsigned char)c);
}

extern void exceptionHandler();	/* assign an exception handler   */

unsigned char getDebugCharRing0()
{
	PS4GDB_kprintf();
	
	unsigned char tmpStr[2];
	tmpStr[1] = 0;

	unsigned char c = gdb_getDebugCharRing0();
  tmpStr[0] = c;
	kprintf("%s",tmpStr);

	return c; 
}

/************************************************************************/
/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 0x1000 //4096*4

struct trap_registers{
  uint64_t rdi;       // 0x00
  uint64_t rax;       // 0x08
  uint64_t rbx;       // 0x10
  uint64_t rcx;       // 0x18
  uint64_t rdx;       // 0x20
  uint64_t rsi;       // 0x28
  uint64_t rsp;       // 0x30
  uint64_t rbp;       // 0x38
  uint64_t rip;       // 0x40
  uint64_t eflags;    // 0x48
  uint64_t r8;        // 0x50
  uint64_t r9;        // 0x58
  uint64_t r10;       // 0x60
  uint64_t r11;       // 0x68
  uint64_t r12;       // 0x70
  uint64_t r13;       // 0x78
  uint64_t r14;       // 0x80
  uint64_t r15;       // 0x88
  uint64_t cs;        // 0x90
  uint64_t ss;        // 0x98
  uint64_t ds;        // 0xA0
  uint64_t es;        // 0xA8
  uint64_t fs;        // 0xB0
  uint64_t gs;        // 0xB8
  uint64_t ErrCode;   // 0xC0
};

static struct trap_registers *tregs;

enum amd64_regnum
{
    AMD64_RAX_REGNUM,
    AMD64_RBX_REGNUM,
    AMD64_RCX_REGNUM,
    AMD64_RDX_REGNUM,
    AMD64_RSI_REGNUM,
    AMD64_RDI_REGNUM,
    AMD64_RBP_REGNUM,
    AMD64_RSP_REGNUM,
    AMD64_R8_REGNUM,
    AMD64_R9_REGNUM,
    AMD64_R10_REGNUM,
    AMD64_R11_REGNUM,
    AMD64_R12_REGNUM,
    AMD64_R13_REGNUM,
    AMD64_R14_REGNUM,
    AMD64_R15_REGNUM,
    AMD64_RIP_REGNUM,
    AMD64_EFLAGS_REGNUM,
    AMD64_CS_REGNUM,
    AMD64_SS_REGNUM,
    AMD64_DS_REGNUM,
    AMD64_ES_REGNUM,
    AMD64_FS_REGNUM,
    AMD64_GS_REGNUM,
    AMD64_ST0_REGNUM = 24,
    AMD64_ST1_REGNUM,
    AMD64_FCTRL_REGNUM = AMD64_ST0_REGNUM + 8,
    AMD64_FSTAT_REGNUM = AMD64_ST0_REGNUM + 9,
    AMD64_FTAG_REGNUM = AMD64_ST0_REGNUM + 10,
    AMD64_XMM0_REGNUM = 40,
    AMD64_XMM1_REGNUM,
    AMD64_MXCSR_REGNUM = AMD64_XMM0_REGNUM + 16,
    AMD64_YMM0H_REGNUM,
    AMD64_YMM15H_REGNUM = AMD64_YMM0H_REGNUM + 15,
    AMD64_BND0R_REGNUM = AMD64_YMM15H_REGNUM + 1,
    AMD64_BND3R_REGNUM = AMD64_BND0R_REGNUM + 3,
    AMD64_BNDCFGU_REGNUM,
    AMD64_BNDSTATUS_REGNUM,
    AMD64_XMM16_REGNUM,
    AMD64_XMM31_REGNUM = AMD64_XMM16_REGNUM + 15,
    AMD64_YMM16H_REGNUM,
    AMD64_YMM31H_REGNUM = AMD64_YMM16H_REGNUM + 15,
    AMD64_K0_REGNUM,
    AMD64_K7_REGNUM = AMD64_K0_REGNUM + 7,
    AMD64_ZMM0H_REGNUM,
    AMD64_ZMM31H_REGNUM = AMD64_ZMM0H_REGNUM + 31,
    AMD64_PKRU_REGNUM,
    AMD64_FSBASE_REGNUM,
    AMD64_GSBASE_REGNUM
};

/*  debug >  0 prints ill-formed commands in valid packets & checksum errors */

static const char hexchars[]="0123456789abcdef";

/* Number of registers.  */
#define NUMREGS 14



/* Number of bytes of registers.  */
#define NUMREGBYTES ((NUMREGS) * 8)

/*
 * these should not be static cuz they can be used outside this module
 */

/***************************  ASSEMBLY CODE MACROS *************************/


#define BREAKPOINT() asm("   int $3");

/* Put the error code here just in case the user cares.  */

/* Likewise, the vector number here (since GDB only gets the signal
   number through the usual means, and that's not very specific).  */


/* GDB stores segment registers in 32-bit words (that's just the way
   m-i386v.h is written).  So zero the appropriate areas in registers.  */
/*
 * remcomHandler is a front end for handle_exception.  It moves the
 * stack pointer into an area reserved for debugger use.
 */
int hex (char ch);

/* scan for the sequence $<data>#<checksum>     */

char *remcomInBufferRing0 = NULL;

unsigned char *getpacketRing0 (void)
{
    PS4GDB_kmalloc();
    PS4GDB_kM_TEMP();

    if(remcomInBufferRing0 == NULL)
    {
        remcomInBufferRing0 = (char*)kmalloc(4096*4,kM_TEMP,0x102);
        LOG_DBG("remcomInBufferRing0 allocated at 0x%llx\n", remcomInBufferRing0);
    }
  
  

  LOG_SIMPLE("\n<-");  

  unsigned char *buffer = (unsigned char *)remcomInBufferRing0;
  unsigned char checksum;
  unsigned char xmitcsum;
  int count;
  char ch;

  while (1)
    {
      /* wait around for the start character, ignore all other characters */
      while ((ch = getDebugCharRing0 ()) != '$')
	;

    retry:
      checksum = 0;
      xmitcsum = -1;
      count = 0;

      /* now, read until a # or end of buffer is found */
      while (count < BUFMAX - 1)
	{
	  ch = getDebugCharRing0 ();
	  if (ch == '$')
	    goto retry;
	  if (ch == '#')
	    break;
	  checksum = checksum + ch;
	  buffer[count] = ch;
	  count = count + 1;
	}
      buffer[count] = 0;

      if (ch == '#')
	{
	  ch = getDebugCharRing0 ();
	  xmitcsum = hex (ch) << 4;
	  ch = getDebugCharRing0 ();
	  xmitcsum += hex (ch);

	  if (checksum != xmitcsum)
	    {
	      
	      putDebugCharRing0 ('-');	/* failed checksum */
	    }
	  else
	    {
	      putDebugCharRing0 ('+');	/* successful transfer */

	      /* if a sequence char is present, reply the sequence ID */
	      if (buffer[2] == ':')
		{
		  putDebugCharRing0 (buffer[0]);
		  putDebugCharRing0 (buffer[1]);

		  return &buffer[3];
		}

	      return &buffer[0];
	    }
	}
    }
}

/* send the packet in buffer.  */

void putpacketRing0 (unsigned char *buffer)
{
  PS4GDB_kprintf();

  unsigned char checksum;
  int count;
  char ch;

  kprintf("\n->", buffer);

  /*  $<packet info>#<checksum>.  */
  do
    {
      putDebugCharRing0 ('$');
      checksum = 0;
      count = 0;

      while ((ch = buffer[count]))
	{
	  putDebugCharRing0 (ch);
	  checksum += ch;
	  count += 1;
	}

      putDebugCharRing0 ('#');
      putDebugCharRing0 (hexchars[checksum >> 4]);
      putDebugCharRing0 (hexchars[checksum % 16]);

    }
  while (getDebugCharRing0 () != '+');
}

void debug_error (char *format, char *parm)     
{
	PS4GDB_kprintf();
	kprintf(format,parm);
}

/* Indicate to caller of mem2hex or hex2mem that there has been an
   error.  */
static volatile int mem_err = 0;

void set_mem_errRing0 (void)
{
  mem_err = 1;
}

/* These are separate functions so that they are so short and sweet
   that the compiler won't save any registers (if there is a fault
   to mem_fault, they won't get restored, so there better not be any
   saved).  */
int get_char (char *addr);
void set_char (char *addr, int val);

/* convert the memory pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf (null) */
/* If MAY_FAULT is non-zero, then we should set mem_err in response to
   a fault; if zero treat a fault like any other fault in the stub.  */
char *mem2hexRing0 (char *mem,  char *buf, int count, int may_fault)
{
  int i;
  unsigned char ch;
  mem_err = 0;

    PS4GDB_kvm_fault_disable_pagefaults();
    PS4GDB_kvm_fault_enable_pagefaults();
    PS4GDB_kmalloc();
    PS4GDB_kfree();
    PS4GDB_kM_TEMP();
    PS4GDB_kcopyin();

  uint8_t *readAddr;
  size_t allocSize;
  if(count > 4096) {
      allocSize = count;
  } else {
      allocSize = 4096;
  }

  readAddr = (uint8_t *)kmalloc(allocSize,kM_TEMP,0x102);

  // Disable pagefaults
  int save = kvm_fault_disable_pagefaults();
  // try to copy the selected memory region
  int res = kcopyin(mem, readAddr, count);
  // Enable pagefaults
  kvm_fault_enable_pagefaults(save);

  if (res == 0) {
        mem = (char*)readAddr;

        for (i = 0; i < count; i++)
        {
            ch = get_char (mem++);
            if(mem_err == 1)
            {
                ch = 0x00;
            }

            *buf++ = hexchars[ch >> 4];
            *buf++ = hexchars[ch % 16];
        }
        *buf = 0;
    }
    else {
        mem_err = 1;
    }
    kfree((void*)readAddr,kM_TEMP);
    return (buf);
}

/* convert the hex array pointed to by buf into binary to be placed in mem */
/* return a pointer to the character AFTER the last byte written */
char * hex2memRing0 (char *buf, char *mem, int count, int may_fault)
{
  PS4GDB_kmalloc();
  PS4GDB_kfree();
  PS4GDB_kM_TEMP();
  PS4GDB_kvm_fault_disable_pagefaults();
  PS4GDB_kvm_fault_enable_pagefaults();
  PS4GDB_kcopyin();

  int i;
  unsigned char ch;

  // alloc temp buffer
  char *writeAddr;
  size_t allocSize;
  if(count > 4096) {
      allocSize = count;
  } else {
      allocSize = 4096;
  }

  writeAddr = (char*)kmalloc(allocSize,kM_TEMP,0x102);
  char *helper = writeAddr;

  // Write received hex bytes into allocated buffer
  for (i = 0; i < count; i++)
  {
      ch = hex (*buf++) << 4;
      ch = ch + hex (*buf++);
      set_char (helper++, ch);
  }

  // Disable pagefaults and write protection
  int save = kvm_fault_disable_pagefaults();
  cpu_disable_wp();
  // try to copy the selected memory region
  int r = kcopyin(writeAddr, mem, count);
  // Enable pagefaults and writeprotection
  kvm_fault_enable_pagefaults(save);
  cpu_enable_wp();

  if(r != 0)
      mem_err = 1;
  else
      mem_err = 0;

  kfree((void*)writeAddr,kM_TEMP);
  return (mem);
}


/* this function takes the 386 exception vector and attempts to
   translate this number into a unix compatible signal value */
int computeSignalRing0 (int exceptionVector)
{
  int sigval;
  switch (exceptionVector)
    {
    case 0:
      sigval = 8;
      break;			/* divide by zero */
    case 1:
      sigval = 5;
      break;			/* debug exception */
    case 3:
      sigval = 5;
      break;			/* breakpoint */
    case 4:
      sigval = 16;
      break;			/* into instruction (overflow) */
    case 5:
      sigval = 16;
      break;			/* bound instruction */
    case 6:
      sigval = 4;
      break;			/* Invalid opcode */
    case 7:
      sigval = 8;
      break;			/* coprocessor not available */
    case 8:
      sigval = 7;
      break;			/* double fault */
    case 9:
      sigval = 11;
      break;			/* coprocessor segment overrun */
    case 10:
      sigval = 11;
      break;			/* Invalid TSS */
    case 11:
      sigval = 11;
      break;			/* Segment not present */
    case 12:
      sigval = 11;
      break;			/* stack exception */
    case 13:
      sigval = 11;
      break;			/* general protection */
    case 14:
      sigval = 11;
      break;			/* page fault */
    case 16:
      sigval = 7;
      break;			/* coprocessor error */
    default:
      sigval = 7;		/* "software generated" */
    }
  return (sigval);
}

/**********************************************/
/* WHILE WE FIND NICE HEX CHARS, BUILD AN INT */
/* RETURN NUMBER OF CHARS PROCESSED           */
/**********************************************/
int hexToInt (char **ptr, unsigned long long *intValue);

void createTempStrRing0(char *src, char *dst, char separator)
{
	int i = 0;

	for(i = 0; ; i++)
		if(src[i] == separator)
		{
			dst[i] = 0x00;
			break;
		}
		else
			dst[i] = src[i];	

	return;
}


unsigned long ps4gdb_strtoul(const char *nptr, char **endptr, int base);

unsigned long hextoullRing0(char *ptr)
{
	return ps4gdb_strtoul(ptr, NULL, 16);
}

/*
 * This function does all command procesing for interfacing to gdb.
 */

static uint64_t registers[300];

uint64_t getRegistersAddress()
{
  return (uint64_t)&tregs;
}

void update_registersRing0() {
    registers[AMD64_RAX_REGNUM] = tregs->rax;
    registers[AMD64_RBX_REGNUM] = tregs->rbx;
    registers[AMD64_RCX_REGNUM] = tregs->rcx;
    registers[AMD64_RDX_REGNUM] = tregs->rdx;
    registers[AMD64_RDI_REGNUM] = tregs->rdi;
    registers[AMD64_RSI_REGNUM] = tregs->rsi;
    registers[AMD64_RSP_REGNUM] = tregs->rsp;
    registers[AMD64_RBP_REGNUM] = tregs->rbp;
    registers[AMD64_R8_REGNUM] = tregs->r8;
    registers[AMD64_R9_REGNUM] = tregs->r9;
    registers[AMD64_R10_REGNUM] = tregs->r10;
    registers[AMD64_R11_REGNUM] = tregs->r11;
    registers[AMD64_R12_REGNUM] = tregs->r12;
    registers[AMD64_R13_REGNUM] = tregs->r13;
    registers[AMD64_R14_REGNUM] = tregs->r14;
    registers[AMD64_R15_REGNUM] = tregs->r15;
    registers[AMD64_RIP_REGNUM] = tregs->rip;
    registers[AMD64_CS_REGNUM] = tregs->cs;
    registers[AMD64_SS_REGNUM] = tregs->ss;
    registers[AMD64_DS_REGNUM] = tregs->ds;
    registers[AMD64_ES_REGNUM] = tregs->es;
    registers[AMD64_FS_REGNUM] = tregs->fs;
    registers[AMD64_GS_REGNUM] = tregs->gs;
    registers[AMD64_EFLAGS_REGNUM] = tregs->eflags;
}

void write_registersRing0(){
  tregs->rax = registers[AMD64_RAX_REGNUM];
  tregs->rbx = registers[AMD64_RBX_REGNUM];
  tregs->rcx = registers[AMD64_RCX_REGNUM];
  tregs->rdx = registers[AMD64_RDX_REGNUM];
  tregs->rdi = registers[AMD64_RDI_REGNUM];
  tregs->rsi = registers[AMD64_RSI_REGNUM];
  tregs->rsp = registers[AMD64_RSP_REGNUM];
  tregs->rbp = registers[AMD64_RBP_REGNUM];
  tregs->r8 = registers[AMD64_R8_REGNUM];
  tregs->r9 = registers[AMD64_R9_REGNUM];
  tregs->r10 = registers[AMD64_R10_REGNUM];
  tregs->r11 = registers[AMD64_R11_REGNUM];
  tregs->r12 = registers[AMD64_R12_REGNUM];
  tregs->r13 = registers[AMD64_R13_REGNUM];
  tregs->r14 = registers[AMD64_R14_REGNUM];
  tregs->r15 = registers[AMD64_R15_REGNUM];
  tregs->rip = registers[AMD64_RIP_REGNUM];
  tregs->cs = registers[AMD64_CS_REGNUM];
  tregs->ss = registers[AMD64_SS_REGNUM];
  tregs->ds = registers[AMD64_DS_REGNUM];
  tregs->es = registers[AMD64_ES_REGNUM];
  tregs->fs = registers[AMD64_FS_REGNUM];
  tregs->gs = registers[AMD64_GS_REGNUM];
  tregs->eflags = registers[AMD64_EFLAGS_REGNUM];
}

static char tmpBuff[1024];

void print_register_infoRing0(int gdb_i386vector, int errorCode)
{
  PS4GDB_kprintf();

  kprintf("received interrupt %02d - errorCode: 0x%llx\n", gdb_i386vector, errorCode);
  kprintf("RAX: 0x%016llx\t\tRBX:  0x%016llx\n",registers[AMD64_RAX_REGNUM],registers[AMD64_RBX_REGNUM]);
  kprintf("RCX: 0x%016llx\t\tRDX:  0x%016llx\n",registers[AMD64_RCX_REGNUM],registers[AMD64_RDX_REGNUM]);
  kprintf("RSI: 0x%016llx\t\tRDI:  0x%016llx\n",registers[AMD64_RSI_REGNUM],registers[AMD64_RDI_REGNUM]);
  kprintf("RBP: 0x%016llx\t\tRSP:  0x%016llx\n",registers[AMD64_RBP_REGNUM],registers[AMD64_RSP_REGNUM]);
  kprintf("R8:  0x%016llx\t\tR9:   0x%016llx\n",registers[AMD64_R8_REGNUM],registers[AMD64_R9_REGNUM]);
  kprintf("R10: 0x%016llx\t\tR11:  0x%016llx\n",registers[AMD64_R10_REGNUM],registers[AMD64_R11_REGNUM]);
  kprintf("R12: 0x%016llx\t\tR13:  0x%016llx\n",registers[AMD64_R12_REGNUM],registers[AMD64_R13_REGNUM]);
  kprintf("R14: 0x%016llx\t\tR15:  0x%016llx\n",registers[AMD64_R14_REGNUM],registers[AMD64_R15_REGNUM]);
  kprintf("RIP: 0x%016llx\t\tFLAGS:0x%016llx\n",registers[AMD64_RIP_REGNUM],registers[AMD64_EFLAGS_REGNUM]);
  kprintf("CS:  0x%016llx\t\tSS:   0x%016llx\n",registers[AMD64_CS_REGNUM],registers[AMD64_SS_REGNUM]);
  kprintf("DS:  0x%016llx\t\tES:   0x%016llx\n",registers[AMD64_DS_REGNUM],registers[AMD64_ES_REGNUM]);
  kprintf("FS:  0x%016llx\t\tGS:   0x%016llx\n",registers[AMD64_FS_REGNUM],registers[AMD64_GS_REGNUM]);
  kprintf("kernelbase: 0x%llx\n", gKernelBase);
}

extern "C"
{

void handle_exceptionRing0 (int gdb_i386vector, unsigned long long *registersOld, int errorCode, struct trap_registers *regs)
{
    PS4GDB_ksnprintf();
    PS4GDB_kmalloc();
	  PS4GDB_kM_TEMP();
    PS4GDB_kfree();
    tregs = regs;
    int stepping = 0;
    int sigval;
    unsigned long long addr, length;
    char *ptr;

    if(get_gdb_server_socket() == -1)
      LOG_DBG("gdb_stub: handle exception start...\n");

    char *remcomOutBuffer = (char*)kmalloc(4096*4,kM_TEMP,0x102);

    if(get_gdb_server_socket() == -1)
      LOG_DBG("remcomOutBuffer allocated at 0x%llx\n", remcomOutBuffer);

    int Doexit = 0;

    update_registersRing0();
    if(get_gdb_server_socket() == -1)
      print_register_infoRing0(gdb_i386vector,errorCode);

    /* reply to host that an exception has occurred */
    sigval = computeSignalRing0 (gdb_i386vector);
    ptr = remcomOutBuffer;

    if(get_gdb_server_socket() == -1)
      LOG_DBG("gdb_stub: Entering main loop...\n");

    if(get_gdb_server_socket() != -1){
        remcomOutBuffer[0] = 'S';
        remcomOutBuffer[1] = hexchars[sigval >> 4];
        remcomOutBuffer[2] = hexchars[sigval % 16];
        remcomOutBuffer[3] = 0;
        putpacketRing0 ((unsigned char*)remcomOutBuffer);
    } 

    while (1 == 1)
    {
        remcomOutBuffer[0] = 0;
        ptr = (char*)getpacketRing0 ();

        switch (*ptr++)
        {
        case '?':
            remcomOutBuffer[0] = 'S';
            remcomOutBuffer[1] = hexchars[sigval >> 4];
            remcomOutBuffer[2] = hexchars[sigval % 16];
            remcomOutBuffer[3] = 0;
            break;
        case 'd':
            //remote_debug = !(remote_debug);	/* toggle debug flag */
            break;
        case 'g':		/* return the value of the CPU registers */
            update_registersRing0();
            mem2hexRing0 ((char *) registers, remcomOutBuffer, NUMREGBYTES, 0);
            break;
        case 'G':		/* set the value of the CPU registers - return OK */
            hex2memRing0 (ptr, (char *) registers, NUMREGBYTES, 0);
            gdb_strcpy (remcomOutBuffer, "OK");
            break;
        case 'P':		/* set the value of a single CPU register - return OK */
        {
            int regno;
            regno = (int)ps4gdb_strtoul(ptr,NULL,16);
            ksnprintf(tmpBuff,1024,"%llx",regno);
            ptr += gdb_strlen(tmpBuff)+1;
            hex2memRing0 (ptr, (char *) &registers[regno], 8, 0);
            write_registersRing0();
            gdb_strcpy (remcomOutBuffer, "OK");
            break;
        }

        /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
        case 'm':
            /* TRY TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
            addr = ps4gdb_strtoul(ptr, NULL, 16);
            ksnprintf(tmpBuff,1024,"%llx",addr);
            ptr += gdb_strlen(tmpBuff);
            if (*(ptr++) == ',')
            {
                if (hexToInt (&ptr, &length))
                {
                    ptr = 0;
                    mem_err = 0;
                    mem2hexRing0((char *) addr, remcomOutBuffer, length, 1);
                    if (mem_err)
                    {
                        gdb_strcpy (remcomOutBuffer, "E03");
                    }
                }
            }

            if (ptr)
            {
                gdb_strcpy (remcomOutBuffer, "E01");
            }

            break;

        /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
        case 'M':
            /* TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
            if (hexToInt (&ptr, &addr))
                if (*(ptr++) == ',')
                    if (hexToInt (&ptr, &length))
                        if (*(ptr++) == ':')
                        {
                            mem_err = 0;
                            hex2memRing0 (ptr, (char *) addr, length, 1);
                            if (mem_err)
                            {
                                gdb_strcpy (remcomOutBuffer, "E03");
                            }
                            else
                            {
                                gdb_strcpy (remcomOutBuffer, "OK");
                            }
                            ptr = 0;
                        }
            if (ptr)
            {
                gdb_strcpy (remcomOutBuffer, "E02");
            }
            break;
        /* cAA..AA    Continue at address AA..AA(optional) */
        /* sAA..AA   Step one instruction from AA..AA(optional) */
        case 's':
          stepping = 1;
        case 'c':
            /* clear the trace bit */
            registers[AMD64_EFLAGS_REGNUM] &= 0xfffffeff;
            /* set the trace bit if we're stepping */
            if(stepping == 1)
              registers[AMD64_EFLAGS_REGNUM] |= 0x100;

            Doexit = 1;
            gdb_strcpy (remcomOutBuffer, "OK");
            break;
        case 'p':
            addr = ps4gdb_strtoul(ptr,NULL,16);
            mem2hexRing0 ((char *) &registers[addr], remcomOutBuffer, 8, 0);
            break;
        /* kill the program */
        case 'k':		/* do nothing */
            set_gdb_server_socket(-1);
            /* clear the trace bit */
            registers[AMD64_EFLAGS_REGNUM] &= 0xfffffeff;
            goto exit_clean;
            break;
        case 'q':
            if(startsWith("Supported",ptr) == 1) {
                gdb_strcpy (remcomOutBuffer, "");
            } else if((startsWith("TStatus",ptr) == 1)) {
                gdb_strcpy (remcomOutBuffer, "T0");
            } else {
                gdb_strcpy (remcomOutBuffer, "");
            }
            break;
        case 'Z':
            gdb_strcpy (remcomOutBuffer, "");
            break;
        case 'v':
            if(startsWith("Cont?",ptr) == 1) {
                gdb_strcpy (remcomOutBuffer, "vCont;c;C;s;S;t;r");
            } else if (startsWith("Cont;s",ptr) == 1) {
              /* clear the trace bit */
              registers[AMD64_EFLAGS_REGNUM] &= 0xfffffeff;
              /* set the trace bit if we're stepping */
              registers[AMD64_EFLAGS_REGNUM] |= 0x100;

              Doexit = 1;
              gdb_strcpy (remcomOutBuffer, "OK");  
              break;
            } else if (startsWith("Cont;c",ptr) == 1) {
              /* clear the trace bit */
              registers[AMD64_EFLAGS_REGNUM] &= 0xfffffeff;
              Doexit = 1;
              gdb_strcpy (remcomOutBuffer, "OK");
              break;
            } else {
                gdb_strcpy (remcomOutBuffer, "");
            }
            break;
        }			/* switch */

        /* reply to the request */
        putpacketRing0 ((unsigned char*)remcomOutBuffer);
        if(Doexit == 1)
            goto exit_clean;
    }

exit_clean:
  write_registersRing0();
  kfree(remcomOutBuffer, kM_TEMP);
  return;
}
}

void *handle_mem_fault_exception () {
	PS4GDB_kprintf();
	kprintf("Received interrupt 14. set_mem_err: 0x%llx\n",set_mem_errRing0);

	return (void*)set_mem_errRing0;
}

extern "C"
{
    extern void catch_exception_03();
    extern void catch_exception_01();
}

/* this function is used to set up exception handlers for tracing and
   breakpoints */

void gdbstub_set_debug_traps (void) {
  PS4GDB_kcritical_enter();
  PS4GDB_kcritical_exit();

  kcritical_enter();

  asm("cli");
  gdb_exceptionHandlerRing0(1, (uint64_t)catch_exception_01);
  gdb_exceptionHandlerRing0(3, (uint64_t)catch_exception_03);  
  asm("sti");

  kcritical_exit();
}

/* This function will generate a breakpoint exception.  It is used at the
   beginning of a program to sync up with a debugger and can be used
   otherwise as a quick means to stop program execution and "break" into
   the debugger.  */

void breakpoint (void) {
  BREAKPOINT ();
}
