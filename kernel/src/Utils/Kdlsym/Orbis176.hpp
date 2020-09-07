#pragma once
#include <Boot/Config.hpp>
#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_176

/*
These are the required functions in order for the Oni Framework to operate properly
These are all offsets into the base of the kernel. They expect all standard FreeBSD 9 prototypes

The reason we do not hardcode offsets here, is due to the different platforms that are supported, and
for the platforms that do enable kernel ASLR (Address Space Layout Randomization?)
*/

#define kdlsym_addr__mtx_lock_flags                        0x00225D20
#define kdlsym_addr__mtx_lock_sleep                        0x00225D80
#define kdlsym_addr__mtx_unlock_flags                      0x00225F70
#define kdlsym_addr__mtx_unlock_sleep                      0x00225FF0
#define kdlsym_addr__sceSblAuthMgrCheckSelfIsLoadable      0x005C7AC0 // !!! not 100% sure that is correct !!!5.00 sceSblAuthMgrIsLoadable2 0x63C110
#define kdlsym_addr__sceSblAuthMgrGetSelfInfo              0x005C8520
#define kdlsym_addr__sceSblAuthMgrVerifySelfHeader         0x005C7B70 // 5.00 sceSblAuthMgrVerifyHeader (actually 5.00 correct name also seems to be _sceSblAuthMgrVerifySelfHeader)
#define kdlsym_addr__sx_slock                              0x0023E320
#define kdlsym_addr__sx_sunlock                            0x0023E490
#define kdlsym_addr__sx_xlock                              0x0023E3D0
#define kdlsym_addr__sx_xunlock                            0x0023E500
#define kdlsym_addr__vm_map_lock_read                      0x003ACA70
#define kdlsym_addr__vm_map_unlock_read                    0x003ACAA0
#define kdlsym_addr_AesCbcCfb128Decrypt                    0x004593F0
#define kdlsym_addr_AesCbcCfb128Encrypt                    0x00459250
#define kdlsym_addr_allproc                                0x010CC350
#define kdlsym_addr_allproc_lock                           0x010CC2F0
#define kdlsym_addr_contigfree                             0x003A65B0
#define kdlsym_addr_contigmalloc                           0x003A6280
#define kdlsym_addr_copyin                                 0x00413CC0
#define kdlsym_addr_copyinstr                              0x00414010
#define kdlsym_addr_critical_enter                         0x0023DC30
#define kdlsym_addr_critical_exit                          0x0023DC40
#define kdlsym_addr_dmem_start_app_process                 0x0 
#define kdlsym_addr_target_id                              0x0
#define kdlsym_addr_eventhandler_register                  0x00267250
#define kdlsym_addr_exec_new_vmspace                       0x00206A10
#define kdlsym_addr_faultin                                0x003AA000
#define kdlsym_addr_fget_unlocked                          0x001FB130
#define kdlsym_addr_fpu_kern_ctx                           0x013E2840
#define kdlsym_addr_fpu_kern_enter                         0x004009D0
#define kdlsym_addr_fpu_kern_leave                         0x00400AC0
#define kdlsym_addr_free                                   0x00222820
#define kdlsym_addr_gpu_va_page_list                       0x013E2210
#define kdlsym_addr_icc_nvs_read                           0x00439CD0
#define kdlsym_addr_kern_close                             0x001F8990
#define kdlsym_addr_kern_mkdirat                           0x002CEEE0
#define kdlsym_addr_kern_open                              0x002C9DB0
#define kdlsym_addr_kern_openat                            0x002C9E10
#define kdlsym_addr_kern_readv                             0x0027AD30
#define kdlsym_addr_kern_reboot                            0x002354B0
#define kdlsym_addr_kern_sysents                           0x0102DC70
#define kdlsym_addr_kern_thr_create                        0x00243F40
#define kdlsym_addr_kernel_map                             0x010F6D70
#define kdlsym_addr_kmem_alloc                             0x003AB260
#define kdlsym_addr_kmem_free                              0x003AB410
#define kdlsym_addr_kproc_create                           0x00217080
#define kdlsym_addr_kthread_add                            0x00217700
#define kdlsym_addr_kthread_exit                           0x002179A0
#define kdlsym_addr_M_LINKER                               0x01038FE0
#define kdlsym_addr_M_MOUNT                                0x01049B50
#define kdlsym_addr_M_TEMP                                 0x010396E0
#define kdlsym_addr_malloc                                 0x00222660
#define kdlsym_addr_memcmp                                 0x002D6570
#define kdlsym_addr_memcpy                                 0x00413BC0
#define kdlsym_addr_memmove                                0x0041EB10
#define kdlsym_addr_memset                                 0x0041EB30
#define kdlsym_addr_mini_syscore_self_binary               0x00D2CC98 // 1.76 (b2ca78 phys addr)
#define kdlsym_addr_mtx_destroy                            0x00226670
#define kdlsym_addr_mtx_init                               0x00226600
#define kdlsym_addr_mtx_lock_sleep                         0x00225D80 //  '_' doubled
#define kdlsym_addr_mtx_unlock_sleep                       0x00225FF0 //  ----||----
#define kdlsym_addr_pfind                                  0x00228B10
#define kdlsym_addr_pmap_activate                          0x00413390
#define kdlsym_addr_printf                                 0x0026E340
#define kdlsym_addr_prison0                                0x01037250
#define kdlsym_addr_proc0                                  0x010B2708
#define kdlsym_addr_proc_reparent                          0x00208C60
#define kdlsym_addr_proc_rwmem                             0x002805E0
#define kdlsym_addr_realloc                                0x00222930
#define kdlsym_addr_rootvnode                              0x010EF920
#define kdlsym_addr_RsaesPkcs1v15Dec2048CRT                0x00465030 
#define kdlsym_addr_sbl_eap_internal_partition_key         0x014BC010 
#define kdlsym_addr_sbl_pfs_sx                             0x0
#define kdlsym_addr_sceSblAuthMgrSmStart                   0x005C8970
#define kdlsym_addr_sceSblAuthMgrSmVerifyHeader            0x005C9750
#define kdlsym_addr_sceSblDriverSendMsg                    0x005AFCC0 // Require some work. No 'xor     edx, edx, with jmp' , only direct function available
#define kdlsym_addr_sceSblGetEAPInternalPartitionKey       0x005B1B00 
#define kdlsym_addr_sceSblKeymgrClearKey                   0x005B3590 
#define kdlsym_addr_sceSblKeymgrSetKey                     0x005B33B0 // 5.00 sceSblKeymgrSetKeyForPfs
#define kdlsym_addr_sceSblKeymgrSmCallfunc                 0x005B3CF0 
#define kdlsym_addr_sceSblPfsSetKey                        0x005B2510 // 5.00 sceSblPfsSetKeys
#define kdlsym_addr_sceSblServiceMailbox                   0x005BF190 
#define kdlsym_addr_sceSblACMgrGetPathId                   0x003F5230
#define kdlsym_addr_self_orbis_sysvec                      0x01063E38 // found by label
#define kdlsym_addr_Sha256Hmac                             0x0045EFC0
#define kdlsym_addr_snprintf                               0x0026E560
#define kdlsym_addr_sscanf                                 0x00271D60 
#define kdlsym_addr_strcmp                                 0x002D7A20
#define kdlsym_addr_strdup                                 0x002D7B30
#define kdlsym_addr_strlen                                 0x002D7C80
#define kdlsym_addr_strncmp                                0x002D7CA0
#define kdlsym_addr_strstr                                 0x002D7E40
#define kdlsym_addr_trap_fatal                             0x00415A30
#define kdlsym_addr_utilUSleep                             0x005DBAF0 
#define kdlsym_addr_vm_map_lookup_entry                    0x003AD090
#define kdlsym_addr_vmspace_acquire_ref                    0x003AC930
#define kdlsym_addr_vmspace_alloc                          0x003AC520
#define kdlsym_addr_vmspace_free                           0x003AC780
#define kdlsym_addr_vsnprintf                              0x0026E600
#define kdlsym_addr_Xfast_syscall                          0x003FF260
#define kdlsym_addr_wakeup                                 0x0023F5A0
#define kdlsym_addr_eventhandler_register                  0x00267250
#define kdlsym_addr_eventhandler_deregister                0x00267540
#define kdlsym_addr_eventhandler_find_list                 0x00267660
#define kdlsym_addr_vm_fault_disable_pagefaults            0x003A9320
#define kdlsym_addr_vm_fault_enable_pagefaults             0x003A9350
#define kdlsym_addr_gdt                                    0x01114E60
#define kdlsym_addr_make_dev_p                             0x001EE5F0
#define kdlsym_addr_destroy_dev                            0x001EEB80

// FakeSelf hooks
#define kdlsym_addr_sceSblAuthMgrIsLoadable__sceSblACMgrGetPathId_hook        0x005C67BA
#define kdlsym_addr_sceSblAuthMgrIsLoadable2_hook                             0x0
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook 0x0
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook   0x0
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookA                           0x0
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookB                           0x0

// FakePkg hooks
#define kdlsym_addr_sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg_hook       0x0
#define kdlsym_addr_sceSblKeymgrInvalidateKey__sx_xlock_hook                  0x0
#define kdlsym_addr_npdrm_decrypt_isolated_rif__sceSblKeymgrSmCallfunc_hook   0x0
#define kdlsym_addr_npdrm_decrypt_rif_new__sceSblKeymgrSmCallfunc_hook        0x0
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookA                          0x0
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookB                          0x0

// SceShellCore patches - call sceKernelIsGenuineCEX
#define ssc_sceKernelIsGenuineCEX_patchA                   0x0
#define ssc_sceKernelIsGenuineCEX_patchB                   0x0
#define ssc_sceKernelIsGenuineCEX_patchC                   0x0
#define ssc_sceKernelIsGenuineCEX_patchD                   0x0

// SceShellCore patches - call nidf_libSceDipsw
#define ssc_nidf_libSceDipsw_patchA                        0x0
#define ssc_nidf_libSceDipsw_patchB                        0x0
#define ssc_nidf_libSceDipsw_patchC                        0x0
#define ssc_nidf_libSceDipsw_patchD                        0x0

// SceShellCore patches - use free prefix instead fake
#define ssc_fake_to_free_patch                             0x0
#endif