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

extern "C"
{
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
    #include <sys/ptrace.h>
    #include <machine/reg.h>
    #include <sys/wait.h>
}

#include "gdbstub.hpp"
#include "ptrace.hpp"
#include "kdl.h"
#include "log.h"
#include "syscalls.hpp"

extern int GDB_SERVER_PID;
extern int gdb_server_socket;

/************************************************************************
 *
 * external low-level support routines
 */

int gdb_strlen(char *str);
int gdb_vm_fault_disable_pagefaults();
void gdb_vm_fault_enable_pagefaults(int val);

void putDebugChar(char c)
{
    unsigned char tmpStr[2];
    tmpStr[1] = 0;
    tmpStr[0] = c;
    LOG_SIMPLE("%s",tmpStr);
    gdb_putDebugChar((unsigned char)c);
}

unsigned char getDebugChar()
{
    unsigned char tmpStr[2];
    tmpStr[1] = 0;

    unsigned char c = gdb_getDebugChar();
    tmpStr[0] = c;
    LOG_SIMPLE("%s",tmpStr);
    return c;
}

/************************************************************************/
/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 0x4000 //4096*4

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
int hex (char ch)
{
    if ((ch >= 'a') && (ch <= 'f'))
        return (ch - 'a' + 10);
    if ((ch >= '0') && (ch <= '9'))
        return (ch - '0');
    if ((ch >= 'A') && (ch <= 'F'))
        return (ch - 'A' + 10);
    return (-1);
}

/* scan for the sequence $<data>#<checksum>     */
char *remcomInBuffer = NULL;

unsigned char *getpacket (void)
{
	PS4GDB_kmalloc();
	PS4GDB_kM_TEMP();

    if(remcomInBuffer == NULL)
    {
        remcomInBuffer = (char*)kmalloc(4096*4,kM_TEMP,0x102);
        LOG_DBG("remcomInBuffer allocated at 0x%llx\n", remcomInBuffer);
    }

    LOG_SIMPLE("\n<-");

    unsigned char *buffer = (unsigned char *)remcomInBuffer;
    unsigned char checksum;
    unsigned char xmitcsum;
    int count;
    char ch;

    while (1)
    {
        /* wait around for the start character, ignore all other characters */
        while ((ch = getDebugChar ()) != '$')
            ;

retry:
        checksum = 0;
        xmitcsum = -1;
        count = 0;

        /* now, read until a # or end of buffer is found */
        while (count < BUFMAX - 1)
        {
            ch = getDebugChar ();
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
            ch = getDebugChar ();
            xmitcsum = hex (ch) << 4;
            ch = getDebugChar ();
            xmitcsum += hex (ch);

            if (checksum != xmitcsum)
            {

                putDebugChar ('-');	/* failed checksum */
            }
            else
            {
                putDebugChar ('+');	/* successful transfer */

                /* if a sequence char is present, reply the sequence ID */
                if (buffer[2] == ':')
                {
                    putDebugChar (buffer[0]);
                    putDebugChar (buffer[1]);

                    return &buffer[3];
                }

                return &buffer[0];
            }
        }
    }
}

/* send the packet in buffer.  */
void putpacket (unsigned char *buffer)
{
    unsigned char checksum;
    int count;
    char ch;

    LOG_SIMPLE("\n->", buffer);

    /*  $<packet info>#<checksum>.  */
    do
    {
        putDebugChar ('$');
        checksum = 0;
        count = 0;

        while ((ch = buffer[count]))
        {
            putDebugChar (ch);
            checksum += ch;
            count += 1;
        }

        putDebugChar ('#');
        putDebugChar (hexchars[checksum >> 4]);
        putDebugChar (hexchars[checksum % 16]);

    }
    while (getDebugChar () != '+');
}

/* Address of a routine to RTE to if we get a memory fault.  */
static void (*volatile mem_fault_routine) () = NULL;

/* Indicate to caller of mem2hex or hex2mem that there has been an
   error.  */
static volatile int mem_err = 0;

void set_mem_err (void)
{
    mem_err = 1;
}

/* These are separate functions so that they are so short and sweet
   that the compiler won't save any registers (if there is a fault
   to mem_fault, they won't get restored, so there better not be any
   saved).  */
int get_char (char *addr)
{
    return *addr;
}

void set_char (char *addr, int val)
{
    *addr = val;
}

/* convert the memory pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf (null) */
/* If MAY_FAULT is non-zero, then we should set mem_err in response to
   a fault; if zero treat a fault like any other fault in the stub.  */
char *mem2hex_ptrace (char *mem, char *buf, int count, int may_fault)
{
   	PS4GDB_kmalloc();
	PS4GDB_kfree();
	PS4GDB_kM_TEMP();

    int i;
    unsigned char ch;
    mem_err = 0;

    uint8_t *readAddr;
    size_t allocSize;
    if(count > 4096) {
        allocSize = count;
    } else {
        allocSize = 4096;
    }

    readAddr = (uint8_t *)kmalloc(allocSize,kM_TEMP,0x102);

    // Read memory address using ptrace
    int r = ptrace_io(GDB_SERVER_PID, PIOD_READ_I, mem, readAddr, count);
    if (r == 0) {
        mem = (char*)readAddr;

        for (i = 0; i < count; i++)
        {
            ch = get_char (mem++);
            if(mem_err == 1)
                ch = 0x00;

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

char *mem2hex (char *mem, char *buf, int count, int may_fault)
{
    int i;
    unsigned char ch;
    mem_err = 0;

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

    return (buf);
}

/* convert the hex array pointed to by buf into binary to be placed in mem */
/* return a pointer to the character AFTER the last byte written */
char *hex2mem (char *buf, char *mem, int count, int may_fault)
{
    int i;
    unsigned char ch;

    if (may_fault)
        mem_fault_routine = set_mem_err;
    for (i = 0; i < count; i++)
    {
        ch = hex (*buf++) << 4;
        ch = ch + hex (*buf++);
        set_char (mem++, ch);
        if (may_fault && mem_err)
            return (mem);
    }
    if (may_fault)
        mem_fault_routine = NULL;
    return (mem);
}

char *hex2mem_ptrace (char *buf, char *mem, int count, int may_fault)
{
   	PS4GDB_kmalloc();
	PS4GDB_kfree();
	PS4GDB_kM_TEMP();

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

    // Write bytes into process memory
    int r = (int)ptrace_io(GDB_SERVER_PID,PT_WRITE_I,mem,writeAddr,count);

    if(r != 0)
        mem_err = 1;
    else
        mem_err = 0;

    kfree((void*)writeAddr,kM_TEMP);
    return (mem);
}

/* this function takes the 386 exception vector and attempts to
   translate this number into a unix compatible signal value */
int computeSignal (int exceptionVector)
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
int hexToInt (char **ptr, unsigned long long *intValue)
{
    int numChars = 0;
    int hexValue;

    *intValue = 0;

    while (**ptr)
    {
        hexValue = hex (**ptr);
        if (hexValue >= 0)
        {
            *intValue = (*intValue << 4) | hexValue;
            numChars++;
        }
        else
            break;

        (*ptr)++;
    }

    return (numChars);
}

void createTempStr(char *src, char *dst, char separator)
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


unsigned long long hextoull(char *ptr)
{
    unsigned long long value = ps4gdb_strtoul(ptr, NULL, 16);
    return value;
}

static uint64_t registers[100];

static char tmpBuff[1024];

void update_registers() {
    struct reg regs;
    ptrace_get_regs(GDB_SERVER_PID,&regs);
    registers[AMD64_RAX_REGNUM] = regs.r_rax;
    registers[AMD64_RBX_REGNUM] = regs.r_rbx;
    registers[AMD64_RCX_REGNUM] = regs.r_rcx;
    registers[AMD64_RDX_REGNUM] = regs.r_rdx;
    registers[AMD64_RDI_REGNUM] = regs.r_rdi;
    registers[AMD64_RSI_REGNUM] = regs.r_rsi;
    registers[AMD64_RSP_REGNUM] = regs.r_rsp;
    registers[AMD64_RBP_REGNUM] = regs.r_rbp;
    registers[AMD64_R8_REGNUM] = regs.r_r8;
    registers[AMD64_R9_REGNUM] = regs.r_r9;
    registers[AMD64_R10_REGNUM] = regs.r_r10;
    registers[AMD64_R11_REGNUM] = regs.r_r11;
    registers[AMD64_R12_REGNUM] = regs.r_r12;
    registers[AMD64_R13_REGNUM] = regs.r_r13;
    registers[AMD64_R14_REGNUM] = regs.r_r14;
    registers[AMD64_R15_REGNUM] = regs.r_r15;
    registers[AMD64_RIP_REGNUM] = regs.r_rip;
    registers[AMD64_CS_REGNUM] = regs.r_cs;
    registers[AMD64_SS_REGNUM] = regs.r_ss;
    registers[AMD64_DS_REGNUM] = regs.r_ds;
    registers[AMD64_ES_REGNUM] = regs.r_es;
    registers[AMD64_FS_REGNUM] = regs.r_fs;
    registers[AMD64_GS_REGNUM] = regs.r_gs;
    registers[AMD64_EFLAGS_REGNUM] = regs.r_rflags;
}

void write_registers() {
    struct reg regs;
    ptrace_get_regs(GDB_SERVER_PID,&regs);
    regs.r_rax = registers[AMD64_RAX_REGNUM];
    regs.r_rbx = registers[AMD64_RBX_REGNUM];
    regs.r_rcx = registers[AMD64_RCX_REGNUM];
    regs.r_rdx = registers[AMD64_RDX_REGNUM];
    regs.r_rdi = registers[AMD64_RDI_REGNUM];
    regs.r_rsi = registers[AMD64_RSI_REGNUM];
    regs.r_rsp = registers[AMD64_RSP_REGNUM];
    regs.r_rbp = registers[AMD64_RBP_REGNUM];
    regs.r_r8 = registers[AMD64_R8_REGNUM];
    regs.r_r9 = registers[AMD64_R9_REGNUM];
    regs.r_r10 = registers[AMD64_R10_REGNUM];
    regs.r_r11 = registers[AMD64_R11_REGNUM];
    regs.r_r12 = registers[AMD64_R12_REGNUM];
    regs.r_r13 = registers[AMD64_R13_REGNUM];
    regs.r_r14 = registers[AMD64_R14_REGNUM];
    regs.r_r15 = registers[AMD64_R15_REGNUM];
    regs.r_rip = registers[AMD64_RIP_REGNUM];
    regs.r_cs = registers[AMD64_CS_REGNUM];
    regs.r_ss = registers[AMD64_SS_REGNUM];
    regs.r_ds = registers[AMD64_DS_REGNUM];
    regs.r_es = registers[AMD64_ES_REGNUM];
    regs.r_fs = registers[AMD64_FS_REGNUM];
    regs.r_gs = registers[AMD64_GS_REGNUM];
    regs.r_rflags = registers[AMD64_EFLAGS_REGNUM];
    ptrace_set_regs(GDB_SERVER_PID,&regs);
}

void print_register_info(int gdb_i386vector, int errorCode)
{
    update_registers();

    LOG_DBG("received interrupt %02d - errorCode: 0x%llx\n", gdb_i386vector, errorCode);
    LOG_DBG("RAX: 0x%016llx\t\tRBX:  0x%016llx\n",registers[AMD64_RAX_REGNUM],registers[AMD64_RBX_REGNUM]);
    LOG_DBG("RCX: 0x%016llx\t\tRDX:  0x%016llx\n",registers[AMD64_RCX_REGNUM],registers[AMD64_RDX_REGNUM]);
    LOG_DBG("RSI: 0x%016llx\t\tRDI:  0x%016llx\n",registers[AMD64_RSI_REGNUM],registers[AMD64_RDI_REGNUM]);
    LOG_DBG("RBP: 0x%016llx\t\tRSP:  0x%016llx\n",registers[AMD64_RBP_REGNUM],registers[AMD64_RSP_REGNUM]);
    LOG_DBG("R8:  0x%016llx\t\tR9:   0x%016llx\n",registers[AMD64_R8_REGNUM],registers[AMD64_R9_REGNUM]);
    LOG_DBG("R10: 0x%016llx\t\tR11:  0x%016llx\n",registers[AMD64_R10_REGNUM],registers[AMD64_R11_REGNUM]);
    LOG_DBG("R12: 0x%016llx\t\tR13:  0x%016llx\n",registers[AMD64_R12_REGNUM],registers[AMD64_R13_REGNUM]);
    LOG_DBG("R14: 0x%016llx\t\tR15:  0x%016llx\n",registers[AMD64_R14_REGNUM],registers[AMD64_R15_REGNUM]);
    LOG_DBG("RIP: 0x%016llx\t\tFLAGS:0x%016llx\n",registers[AMD64_RIP_REGNUM],registers[AMD64_EFLAGS_REGNUM]);
    LOG_DBG("CS:  0x%016llx\t\tSS:   0x%016llx\n",registers[AMD64_CS_REGNUM],registers[AMD64_SS_REGNUM]);
    LOG_DBG("DS:  0x%016llx\t\tES:   0x%016llx\n",registers[AMD64_DS_REGNUM],registers[AMD64_ES_REGNUM]);
    LOG_DBG("FS:  0x%016llx\t\tGS:   0x%016llx\n",registers[AMD64_FS_REGNUM],registers[AMD64_GS_REGNUM]);
}

/*
 * This function does all command procesing for interfacing to gdb.
 */
void handle_exception (int gdb_i386vector, unsigned long long *registersOld, int errorCode)
{
    PS4GDB_kavcontrol_sleep();
    PS4GDB_ksnprintf();
    PS4GDB_kmalloc();
	PS4GDB_kM_TEMP();
    LOG_DBG("gdb_stub: handle exception start...\n");

    char *remcomOutBuffer = (char*)kmalloc(4096*4,kM_TEMP,0x102);

    LOG_DBG("remcomOutBuffer allocated at 0x%llx\n", remcomOutBuffer);

    int Doexit = 0;

    print_register_info(gdb_i386vector,errorCode);

    int sigval;
    unsigned long long addr, length;
    char *ptr;
    int stat = 0;

    /* reply to host that an exception has occurred */
    sigval = computeSignal (gdb_i386vector);
    ptr = remcomOutBuffer;

    LOG_DBG("gdb_stub: Entering main loop...\n");

    while (1 == 1)
    {
        remcomOutBuffer[0] = 0;
        ptr = (char*)getpacket ();

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
                update_registers();
                mem2hex ((char *) registers, remcomOutBuffer, NUMREGBYTES, 0);
                break;
            case 'G':		/* set the value of the CPU registers - return OK */
                hex2mem (ptr, (char *) registers, NUMREGBYTES, 0);
                gdb_strcpy (remcomOutBuffer, "OK");
                break;
            case 'P':		/* set the value of a single CPU register - return OK */
            {
                int regno;
                regno = (int)ps4gdb_strtoul(ptr,NULL,16);
                ksnprintf(tmpBuff,1024,"%llx",regno);
                ptr += gdb_strlen(tmpBuff)+1;
                hex2mem (ptr, (char *) &registers[regno], 8, 0);
                write_registers();
                gdb_strcpy (remcomOutBuffer, "OK");
                break;
            }

            /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
            case 'm':
                //LOG_DBG("got m packet! Ignoring...\n");
                //return;
                /* TRY TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
                //LOG_DBG("COMMAND####%s######\n",ptr);
                addr = ps4gdb_strtoul(ptr, NULL, 16);
                ksnprintf(tmpBuff,1024,"%llx",addr);
                ptr += gdb_strlen(tmpBuff);
                if (*(ptr++) == ',')
                {
                    if (hexToInt (&ptr, &length))
                    {
                        ptr = 0;
                        mem_err = 0;
                        mem2hex_ptrace((char *) addr, remcomOutBuffer, length, 1);
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
                                hex2mem_ptrace (ptr, (char *) addr, length, 1);
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
            case 'c':
                ptrace_clearstep(GDB_SERVER_PID);
                ptrace_continue(GDB_SERVER_PID);
                kavcontrol_sleep(100000);
                stat = 0;
                ps4gdb_sys_wait4(GDB_SERVER_PID,&stat,WUNTRACED,0);
                update_registers();
                remcomOutBuffer[0] = 'S';
                remcomOutBuffer[1] = hexchars[5 >> 4];
                remcomOutBuffer[2] = hexchars[5 % 16];
                remcomOutBuffer[3] = 0;
                break;
            case 'p':
                addr = ps4gdb_strtoul(ptr,NULL,16);
                mem2hex ((char *) &registers[addr], remcomOutBuffer, 8, 0);
                break;
            /* kill the program */
            case 'k':		/* do nothing */
                ptrace_clearstep(GDB_SERVER_PID);
                ptrace_detach(GDB_SERVER_PID);
                ps4gdb_sys_shutdown(gdb_server_socket,SHUT_RDWR);
                ps4gdb_sys_close(gdb_server_socket);
                gdb_server_socket = -1;
                return;
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
                    ptrace_setstep(GDB_SERVER_PID);
                    ptrace_step(GDB_SERVER_PID);
                    kavcontrol_sleep(100000);
                    stat = 0;
                    ps4gdb_sys_wait4(GDB_SERVER_PID,&stat,WUNTRACED,0);

                    update_registers();
                    remcomOutBuffer[0] = 'S';
                    remcomOutBuffer[1] = hexchars[5 >> 4];
                    remcomOutBuffer[2] = hexchars[5 % 16];
                    remcomOutBuffer[3] = 0;
                } else if (startsWith("Cont;c",ptr) == 1) {
                    ptrace_clearstep(GDB_SERVER_PID);
                    ptrace_continue(GDB_SERVER_PID);
                    stat = 0;
                    ps4gdb_sys_wait4(GDB_SERVER_PID,&stat,WUNTRACED,0);
                    update_registers();
                    // If we restart here, we probably have hit a breakpoint
                    // So we need to restore the original opcode
                    //ps4gdb_restore_breakpoint_opcode(registers[AMD64_RIP_REGNUM]);
                    remcomOutBuffer[0] = 'S';
                    remcomOutBuffer[1] = hexchars[5 >> 4];
                    remcomOutBuffer[2] = hexchars[5 % 16];
                    remcomOutBuffer[3] = 0;
                } else {
                    gdb_strcpy (remcomOutBuffer, "");
                }
                break;
        }			/* switch */

        /* reply to the request */
        putpacket ((unsigned char*)remcomOutBuffer);
        if(Doexit == 1)
            return;
    }
}

