#pragma once
#include <Boot/Config.hpp>

// Offsets ported by SiSTRo & Joonie

#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_405
/*
These are the required functions in order for the Oni Framework to operate properly
These are all offsets into the base of the kernel. They expect all standard FreeBSD 9 prototypes

The reason we do not hardcode offsets here, is due to the different platforms that are supported, and
for the platforms that do enable kernel ASLR (Address Space Layout Randomization?)
*/

#define kdlsym_addr__mtx_lock_flags                        0x0036E260
#define kdlsym_addr__mtx_lock_sleep                        0x0036E2D0
#define kdlsym_addr__mtx_unlock_flags                      0x0036E510
#define kdlsym_addr__mtx_unlock_sleep                      0x0036E5E0
#define kdlsym_addr__sceSblAuthMgrGetSelfInfo              0x00615360
#define kdlsym_addr__sx_slock                              0x0027AB70
#define kdlsym_addr__sx_sunlock                            0x0027ACF0
#define kdlsym_addr__sx_xlock                              0x0027AC20
#define kdlsym_addr__sx_xunlock                            0x0027ADB0
#define kdlsym_addr__vm_map_lock_read                      0x0043DF10
#define kdlsym_addr__vm_map_unlock_read                    0x0043DF60
#define kdlsym_addr_AesCbcCfb128Decrypt                    0x0019D880
#define kdlsym_addr_AesCbcCfb128Encrypt                    0x0019D650
#define kdlsym_addr_allproc                                0x01FE3498
#define kdlsym_addr_allproc_lock                           0x01FE3438
#define kdlsym_addr_copyin                                 0x00286DF0
#define kdlsym_addr_copyinstr                              0x00287160
#define kdlsym_addr_critical_enter                         0x001E2660
#define kdlsym_addr_critical_exit                          0x001E2670
//#define kdlsym_addr_dmem_start_app_process                 0x0
#define kdlsym_addr_eventhandler_register                  0x004591B0
#define kdlsym_addr_exec_new_vmspace                       0x00214A00
#define kdlsym_addr_faultin                                0x002164A0
#define kdlsym_addr_fget_unlocked                          0x0045E8A0
#define kdlsym_addr_fpu_ctx                                0x0235C6C0
#define kdlsym_addr_fpu_kern_enter                         0x0039A120
#define kdlsym_addr_fpu_kern_leave                         0x0039A220
#define kdlsym_addr_free                                   0x001D18D0
#define kdlsym_addr_gpu_va_page_list                       0x0234ED68
#define kdlsym_addr_icc_nvs_read                           0x003C8490
#define kdlsym_addr_kern_close                             0x0045C440
#define kdlsym_addr_kern_mkdirat                           0x00029330
#define kdlsym_addr_kern_open                              0x00024400
#define kdlsym_addr_kern_openat                            0x00024460
#define kdlsym_addr_kern_readv                             0x00166900
#define kdlsym_addr_kern_reboot                            0x0025FC10
#define kdlsym_addr_kern_sysents                           0x00F17790
#define kdlsym_addr_kern_thr_create						   0x002ECCD0  
#define kdlsym_addr_kernel_map                             0x001D3700 
#define kdlsym_addr_kmem_alloc                             0x00369500
#define kdlsym_addr_kmem_free                              0x003696B0
#define kdlsym_addr_kproc_create                           0x001C92F0
#define kdlsym_addr_kthread_add                            0x001C9890
#define kdlsym_addr_kthread_exit                           0x001C9B60
#define kdlsym_addr_M_MOUNT                                0x01356480
#define kdlsym_addr_M_TEMP                                 0x0134B730
#define kdlsym_addr_malloc                                 0x001D1700
#define kdlsym_addr_memcmp                                 0x0029CD10
#define kdlsym_addr_memcpy                                 0x00286CF0
#define kdlsym_addr_memmove								   0x00339440
#define kdlsym_addr_memset                                 0x001ECB10
#define kdlsym_addr_mini_syscore_self_binary               0x0136B3E8
#define kdlsym_addr_mtx_init                               0x0036EC70
#define kdlsym_addr_mtx_lock_sleep                         0x0036E2D0
#define kdlsym_addr_mtx_unlock_sleep                       0x0036E5E0
#define kdlsym_addr_pfind                                  0x002FA310
#define kdlsym_addr_pmap_activate                          0x003F7B40
#define kdlsym_addr_printf                                 0x00347580
#define kdlsym_addr_prison0                                0x00F26010
#define kdlsym_addr_proc0                                  0x019536A0
#define kdlsym_addr_proc_reparent                          0x00049A60
#define kdlsym_addr_proc_rwmem                             0x000ABBB0
#define kdlsym_addr_realloc                                0x001D1A00
#define kdlsym_addr_rootvnode                              0x0206D250
#define kdlsym_addr_RsaesPkcs1v15Dec2048CRT                0x001E82A0
#define kdlsym_addr_sbl_eap_internal_partition_key         0x023DC000
#define kdlsym_addr_sbl_pfs_sx                             0x0
#define kdlsym_addr_sceSblAuthMgrIsLoadable2               0x00614A80
#define kdlsym_addr__sceSblAuthMgrSmStart                  0x006153F0
#define kdlsym_addr_sceSblAuthMgrSmVerifyHeader            0x006163C0
#define kdlsym_addr_sceSblAuthMgrVerifyHeader              0x00614AE0
#define kdlsym_addr_sceSblDriverSendMsg                    0x005F6400
#define kdlsym_addr_sceSblGetEAPInternalPartitionKey       0x00605330
#define kdlsym_addr_sceSblKeymgrClearKey                   0x005FEE10
#define kdlsym_addr_sceSblKeymgrSetKeyForPfs               0x005FEC40
#define kdlsym_addr_sceSblKeymgrSmCallfunc                 0x005FF500
#define kdlsym_addr_sceSblPfsSetKeys                       0x00600640
#define kdlsym_addr_sceSblServiceMailbox                   0x00606F40
#define kdlsym_addr_sceSblACMgrGetPathId                   0x00360620
#define kdlsym_addr_self_orbis_sysvec                      0x01862F08
#define kdlsym_addr_Sha256Hmac                             0x003369B0
#define kdlsym_addr_snprintf                               0x00347860
#define kdlsym_addr_sscanf                                 0x003813B0
#define kdlsym_addr_strcmp                                 0x00274970
#define kdlsym_addr_strdup                                 0x00021640
#define kdlsym_addr_strlen                                 0x001D3640
#define kdlsym_addr_strncmp								   0x00251250
#define kdlsym_addr_strstr                                 0x0032DAB0
#define kdlsym_addr_sys_accept                             0x00122620
#define kdlsym_addr_sys_bind                               0x00121C80
#define kdlsym_addr_sys_close                              0x0045C430
#define kdlsym_addr_sys_dup2                               0x0045A670
#define kdlsym_addr_sys_fstat                              0x0045C9C0
#define kdlsym_addr_sys_getdents                           0x00029AD0
#define kdlsym_addr_sys_kill                               0x0040C0E0
#define kdlsym_addr_sys_listen                             0x00121E90
#define kdlsym_addr_sys_lseek                              0x00026280
#define kdlsym_addr_sys_mkdir                              0x000292B0
#define kdlsym_addr_sys_mlock                              0x0031DC60
#define kdlsym_addr_sys_mlockall                           0x0031DD10
#define kdlsym_addr_sys_mmap                               0x0031CCF0
#define kdlsym_addr_sys_munmap                             0x0031D460
#define kdlsym_addr_sys_nmount                             0x00200C80
#define kdlsym_addr_sys_open                               0x000243E0
#define kdlsym_addr_sys_ptrace                             0x000AC020
#define kdlsym_addr_sys_read                               0x00166820
#define kdlsym_addr_sys_recvfrom                           0x00123810
#define kdlsym_addr_sys_rmdir                              0x00029630
#define kdlsym_addr_sys_sendto                             0x00123150
#define kdlsym_addr_sys_setuid                             0x00085B20
#define kdlsym_addr_sys_shutdown                           0x001239E0
#define kdlsym_addr_sys_socket                             0x00121340
#define kdlsym_addr_sys_stat                               0x00026880
#define kdlsym_addr_sys_unlink                             0x00025C90
#define kdlsym_addr_sys_unmount                            0x00202560
#define kdlsym_addr_sys_wait4                              0x00049BA0
#define kdlsym_addr_sys_write                              0x00166D70
#define kdlsym_addr_trap_fatal                             0x000EC770
#define kdlsym_addr_utilUSleep                             0x0062A3E0
#define kdlsym_addr_vm_map_lookup_entry                    0x0043E470
#define kdlsym_addr_vmspace_acquire_ref                    0x0043DD40
#define kdlsym_addr_vmspace_alloc                          0x0043D8C0
#define kdlsym_addr_vmspace_free                           0x0043DB70
#define kdlsym_addr_vsnprintf                              0x00347900
#define kdlsym_addr_Xfast_syscall                          0x0030EB30
#define kdlsym_addr_wakeup                                 0x0008A7C0

// FakeSelf hooks
#define kdlsym_addr_sceSblAuthMgrIsLoadable__sceSblACMgrGetPathId_hook        0x0061185C
#define kdlsym_addr_sceSblAuthMgrIsLoadable2_hook                             0x0
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook 0x0
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook   0x0
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookA                           0x0
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookB                           0x0

#endif
