#pragma once
#include <Boot/Config.hpp>
#if MIRA_PLATFORM==ONI_UNKNOWN_PLATFORM

/*
These are the required functions in order for the Oni Framework to operate properly
These are all offsets into the base of the kernel. They expect all standard FreeBSD 9 prototypes

The reason we do not hardcode offsets here, is due to the different platforms that are supported, and
for the platforms that do enable kernel ASLR (Address Space Layout Randomization?)
*/

#define kdlsym_addr__mtx_lock_flags                        0xDEADCODE
#define kdlsym_addr__mtx_lock_sleep                        0xDEADCODE
#define kdlsym_addr__mtx_lock_spin_flags                   0xDEADCODE
#define kdlsym_addr__mtx_unlock_flags                      0xDEADCODE
#define kdlsym_addr__mtx_unlock_sleep                      0xDEADCODE
#define kdlsym_addr__mtx_unlock_spin_flags                 0xDEADCODE
#define kdlsym_addr__sceSblAuthMgrGetSelfInfo              0xDEADCODE
#define kdlsym_addr__sceSblAuthMgrSmStart                  0xDEADCODE
#define kdlsym_addr__sx_init_flags                         0xDEADCODE
#define kdlsym_addr__sx_slock                              0xDEADCODE
#define kdlsym_addr__sx_sunlock                            0xDEADCODE
#define kdlsym_addr__sx_xlock                              0xDEADCODE
#define kdlsym_addr__sx_xunlock                            0xDEADCODE
#define kdlsym_addr__thread_lock_flags                     0xDEADCODE
#define kdlsym_addr__vm_map_lock_read                      0xDEADCODE
#define kdlsym_addr__vm_map_unlock_read                    0xDEADCODE
#define kdlsym_addr_AesCbcCfb128Decrypt                    0xDEADCODE
#define kdlsym_addr_AesCbcCfb128Encrypt                    0xDEADCODE
#define kdlsym_addr_allproc                                0xDEADCODE
#define kdlsym_addr_allproc_lock                           0xDEADCODE
#define kdlsym_addr_avcontrol_sleep                        0xDEADCODE
#define kdlsym_addr_contigfree                             0xDEADCODE
#define kdlsym_addr_contigmalloc                           0xDEADCODE
#define kdlsym_addr_copyin                                 0xDEADCODE
#define kdlsym_addr_copyinstr                              0xDEADCODE
#define kdlsym_addr_copyout                                0xDEADCODE
#define kdlsym_addr_critical_enter                         0xDEADCODE
#define kdlsym_addr_critical_exit                          0xDEADCODE
#define kdlsym_addr_destroy_dev                            0xDEADCODE
#define kdlsym_addr_dmem_start_app_process                 0xDEADCODE
#define kdlsym_addr_dynlib_do_dlsym                        0xDEADCODE
#define kdlsym_addr_dynlib_find_obj_by_handle              0xDEADCODE
#define kdlsym_addr_eventhandler_deregister                0xDEADCODE
#define kdlsym_addr_eventhandler_find_list                 0xDEADCODE
#define kdlsym_addr_eventhandler_register                  0xDEADCODE
#define kdlsym_addr_exec_new_vmspace                       0xDEADCODE
#define kdlsym_addr_faultin                                0xDEADCODE
#define kdlsym_addr_fget_unlocked                          0xDEADCODE
#define kdlsym_addr_fpu_kern_ctx                           0xDEADCODE
#define kdlsym_addr_fpu_kern_enter                         0xDEADCODE
#define kdlsym_addr_fpu_kern_leave                         0xDEADCODE
#define kdlsym_addr_free                                   0xDEADCODE
#define kdlsym_addr_gdt                                    0xDEADCODE
#define kdlsym_addr_gpu_va_page_list                       0xDEADCODE
#define kdlsym_addr_icc_nvs_read                           0xDEADCODE
#define kdlsym_addr_kern_close                             0xDEADCODE
#define kdlsym_addr_kern_ioctl                             0xDEADCODE
#define kdlsym_addr_kern_mkdirat                           0xDEADCODE
#define kdlsym_addr_kern_open                              0xDEADCODE
#define kdlsym_addr_kern_openat                            0xDEADCODE
#define kdlsym_addr_kern_readv                             0xDEADCODE
#define kdlsym_addr_kern_reboot                            0xDEADCODE
#define kdlsym_addr_kern_sysents                           0xDEADCODE
#define kdlsym_addr_kern_thr_create                        0xDEADCODE
#define kdlsym_addr_kernel_map                             0xDEADCODE
#define kdlsym_addr_kernel_mount                           0xDEADCODE
#define kdlsym_addr_killproc                               0xDEADCODE
#define kdlsym_addr_kmem_alloc                             0xDEADCODE
#define kdlsym_addr_kmem_free                              0xDEADCODE
#define kdlsym_addr_kproc_create                           0xDEADCODE
#define kdlsym_addr_kproc_exit                             0xDEADCODE
#define kdlsym_addr_kthread_add                            0xDEADCODE
#define kdlsym_addr_kthread_exit                           0xDEADCODE
#define kdlsym_addr_M_LINKER                               0xDEADCODE
#define kdlsym_addr_M_MOUNT                                0xDEADCODE
#define kdlsym_addr_M_TEMP                                 0xDEADCODE
#define kdlsym_addr_make_dev_p                             0xDEADCODE
#define kdlsym_addr_malloc                                 0xDEADCODE
#define kdlsym_addr_memcmp                                 0xDEADCODE
#define kdlsym_addr_memcpy                                 0xDEADCODE
#define kdlsym_addr_memmove                                0xDEADCODE
#define kdlsym_addr_memset                                 0xDEADCODE
#define kdlsym_addr_mini_syscore_self_binary               0xDEADCODE
#define kdlsym_addr_mount_arg                              0xDEADCODE
#define kdlsym_addr_mount_argb                             0xDEADCODE
#define kdlsym_addr_mount_argf                             0xDEADCODE
#define kdlsym_addr_mtx_destroy                            0xDEADCODE
#define kdlsym_addr_mtx_init                               0xDEADCODE
#define kdlsym_addr_mtx_lock_sleep                         0xDEADCODE
#define kdlsym_addr_mtx_unlock_sleep                       0xDEADCODE
#define kdlsym_addr_name_to_nids                           0xDEADCODE
#define kdlsym_addr_pause                                  0xDEADCODE
#define kdlsym_addr_pfind                                  0xDEADCODE
#define kdlsym_addr_pmap_activate                          0xDEADCODE
#define kdlsym_addr_printf                                 0xDEADCODE
#define kdlsym_addr_prison0                                0xDEADCODE
#define kdlsym_addr_proc0                                  0xDEADCODE
#define kdlsym_addr_proc_reparent                          0xDEADCODE
#define kdlsym_addr_proc_rwmem                             0xDEADCODE
#define kdlsym_addr_realloc                                0xDEADCODE
#define kdlsym_addr_rootvnode                              0xDEADCODE
#define kdlsym_addr_RsaesPkcs1v15Dec2048CRT                0xDEADCODE
#define kdlsym_addr_sbl_eap_internal_partition_key         0xDEADCODE
#define kdlsym_addr_sbl_keymgr_buf_gva                     0xDEADCODE
#define kdlsym_addr_sbl_keymgr_buf_va                      0xDEADCODE
#define kdlsym_addr_sbl_keymgr_key_rbtree                  0xDEADCODE
#define kdlsym_addr_sbl_keymgr_key_slots                   0xDEADCODE
#define kdlsym_addr_sbl_pfs_sx                             0xDEADCODE
#define kdlsym_addr_sceSblACMgrGetPathId                   0xDEADCODE
#define kdlsym_addr_sceSblAuthMgrIsLoadable2               0xDEADCODE
#define kdlsym_addr_sceSblAuthMgrSmVerifyHeader            0xDEADCODE
#define kdlsym_addr_sceSblAuthMgrVerifyHeader              0xDEADCODE
#define kdlsym_addr_sceSblDriverSendMsg                    0xDEADCODE
#define kdlsym_addr_sceSblGetEAPInternalPartitionKey       0xDEADCODE
#define kdlsym_addr_sceSblKeymgrClearKey                   0xDEADCODE
#define kdlsym_addr_sceSblKeymgrSetKeyForPfs               0xDEADCODE
#define kdlsym_addr_sceSblKeymgrSetKeyStorage              0xDEADCODE
#define kdlsym_addr_sceSblKeymgrSmCallfunc                 0xDEADCODE
#define kdlsym_addr_sceSblPfsSetKeys                       0xDEADCODE
#define kdlsym_addr_sceSblRngGetRandomNumber               0xDEADCODE
#define kdlsym_addr_sceSblServiceMailbox                   0xDEADCODE
#define kdlsym_addr_self_orbis_sysvec                      0xDEADCODE
#define kdlsym_addr_Sha256Hmac                             0xDEADCODE
#define kdlsym_addr_snprintf                               0xDEADCODE
#define kdlsym_addr_spinlock_exit                          0xDEADCODE
#define kdlsym_addr_sprintf                                0xDEADCODE
#define kdlsym_addr_sscanf                                 0xDEADCODE
#define kdlsym_addr_strcmp                                 0xDEADCODE
#define kdlsym_addr_strdup                                 0xDEADCODE
#define kdlsym_addr_strlen                                 0xDEADCODE
#define kdlsym_addr_strncmp                                0xDEADCODE
#define kdlsym_addr_strstr                                 0xDEADCODE
#define kdlsym_addr_sys_accept                             0xDEADCODE
#define kdlsym_addr_sys_bind                               0xDEADCODE
#define kdlsym_addr_sys_close                              0xDEADCODE
#define kdlsym_addr_sys_dup2                               0xDEADCODE
#define kdlsym_addr_sys_fstat                              0xDEADCODE
#define kdlsym_addr_sys_getdents                           0xDEADCODE
#define kdlsym_addr_sys_kill                               0xDEADCODE
#define kdlsym_addr_sys_listen                             0xDEADCODE
#define kdlsym_addr_sys_lseek                              0xDEADCODE
#define kdlsym_addr_sys_mkdir                              0xDEADCODE
#define kdlsym_addr_sys_mlock                              0xDEADCODE
#define kdlsym_addr_sys_mlockall                           0xDEADCODE
#define kdlsym_addr_sys_mmap                               0xDEADCODE
#define kdlsym_addr_sys_munmap                             0xDEADCODE
#define kdlsym_addr_sys_nmount                             0xDEADCODE
#define kdlsym_addr_sys_open                               0xDEADCODE
#define kdlsym_addr_sys_ptrace                             0xDEADCODE
#define kdlsym_addr_sys_read                               0xDEADCODE
#define kdlsym_addr_sys_recvfrom                           0xDEADCODE
#define kdlsym_addr_sys_rmdir                              0xDEADCODE
#define kdlsym_addr_sys_sendto                             0xDEADCODE
#define kdlsym_addr_sys_setuid                             0xDEADCODE
#define kdlsym_addr_sys_shutdown                           0xDEADCODE
#define kdlsym_addr_sys_socket                             0xDEADCODE
#define kdlsym_addr_sys_stat                               0xDEADCODE
#define kdlsym_addr_sys_unlink                             0xDEADCODE
#define kdlsym_addr_sys_unmount                            0xDEADCODE
#define kdlsym_addr_sys_wait4                              0xDEADCODE
#define kdlsym_addr_sys_write                              0xDEADCODE
#define kdlsym_addr_trap_fatal                             0xDEADCODE
#define kdlsym_addr_utilUSleep                             0xDEADCODE
#define kdlsym_addr_vm_fault_disable_pagefaults            0xDEADCODE
#define kdlsym_addr_vm_fault_enable_pagefaults             0xDEADCODE
#define kdlsym_addr_vm_map_lookup_entry                    0xDEADCODE
#define kdlsym_addr_vmspace_acquire_ref                    0xDEADCODE
#define kdlsym_addr_vmspace_alloc                          0xDEADCODE
#define kdlsym_addr_vmspace_free                           0xDEADCODE
#define kdlsym_addr_vn_fullpath                            0xDEADCODE
#define kdlsym_addr_vsnprintf                              0xDEADCODE
#define kdlsym_addr_wakeup                                 0xDEADCODE
#define kdlsym_addr_Xfast_syscall                          0xDEADCODE

// FakeSelf hooks
#define kdlsym_addr_sceSblAuthMgrIsLoadable__sceSblACMgrGetPathId_hook        0xDEADCODE
#define kdlsym_addr_sceSblAuthMgrIsLoadable2_hook                             0xDEADCODE
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook 0xDEADCODE
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook   0xDEADCODE
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookA                           0xDEADCODE
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookB                           0xDEADCODE

// FakePkg hooks
#define kdlsym_addr_sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg_hook       0xDEADCODE
#define kdlsym_addr_sceSblKeymgrInvalidateKey__sx_xlock_hook                  0xDEADCODE
#define kdlsym_addr_npdrm_decrypt_isolated_rif__sceSblKeymgrSmCallfunc_hook   0xDEADCODE
#define kdlsym_addr_npdrm_decrypt_rif_new__sceSblKeymgrSmCallfunc_hook        0xDEADCODE
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookA                          0xDEADCODE
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookB                          0xDEADCODE

// SceShellCore patches - call sceKernelIsGenuineCEX
#define ssc_sceKernelIsGenuineCEX_patchA                   0xDEADCODE
#define ssc_sceKernelIsGenuineCEX_patchB                   0xDEADCODE
#define ssc_sceKernelIsGenuineCEX_patchC                   0xDEADCODE
#define ssc_sceKernelIsGenuineCEX_patchD                   0xDEADCODE

// SceShellCore patches - call nidf_libSceDipsw
#define ssc_nidf_libSceDipsw_patchA                        0xDEADCODE
#define ssc_nidf_libSceDipsw_patchB                        0xDEADCODE
#define ssc_nidf_libSceDipsw_patchC                        0xDEADCODE
#define ssc_nidf_libSceDipsw_patchD                        0xDEADCODE

#define ssc_enable_fakepkg_patch                           0xDEADCODE

// SceShellCore patches - use free prefix instead fake
#define ssc_fake_to_free_patch                             0xDEADCODE

// SceShellUI patches - debug patches
#define ssu_sceSblRcMgrIsAllowDebugMenuForSettings_patch   0xDEADCODE
#define ssu_sceSblRcMgrIsStoreMode_patch                   0xDEADCODE

#endif
