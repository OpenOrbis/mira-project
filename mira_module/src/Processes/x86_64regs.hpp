#pragma once

namespace Mira
{
    namespace Processes
    {
       typedef struct _reg 
       {
            register_t	r_r15;
            register_t	r_r14;
            register_t	r_r13;
            register_t	r_r12;
            register_t	r_r11;
            register_t	r_r10;
            register_t	r_r9;
            register_t	r_r8;
            register_t	r_rdi;
            register_t	r_rsi;
            register_t	r_rbp;
            register_t	r_rbx;
            register_t	r_rdx;
            register_t	r_rcx;
            register_t	r_rax;
            uint32_t	r_trapno;
            uint16_t	r_fs;
            uint16_t	r_gs;
            uint32_t	r_err;
            uint16_t	r_es;
            uint16_t	r_ds;
            register_t	r_rip;
            register_t	r_cs;
            register_t	r_rflags;
            register_t	r_rsp;
            register_t	r_ss;
        } GPRegisters;

        /*
        * Register set accessible via /proc/$pid/fpregs.
        */
        typedef struct _fpreg 
        {
            /*
            * XXX should get struct from fpu.h.  Here we give a slightly
            * simplified struct.  This may be too much detail.  Perhaps
            * an array of unsigned longs is best.
            */
            unsigned long	fpr_env[4];
            unsigned char	fpr_acc[8][16];
            unsigned char	fpr_xacc[16][16];
            unsigned long	fpr_spare[12];
        } FPRegisters;

        /*
        * Register set accessible via /proc/$pid/dbregs.
        */
        typedef struct _dbreg 
        {
            unsigned long  dr[16];	/* debug registers */
                        /* Index 0-3: debug address registers */
                        /* Index 4-5: reserved */
                        /* Index 6: debug status */
                        /* Index 7: debug control */
                        /* Index 8-15: reserved */
        } DBRegisters;
    }
}