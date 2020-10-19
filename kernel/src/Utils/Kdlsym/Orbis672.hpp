#pragma once
#include <Boot/Config.hpp>

#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_672
/*
    These are the required functions in order for the Oni Framework to operate properly
    These are all offsets into the base of the kernel. They expect all standard FreeBSD 9 prototypes

    The reason we do not hardcode offsets here, is due to the different platforms that are supported, and
    for the platforms that do enable kernel ASLR (Address Space Layout Randomization?)
*/

#define kdlsym_addr__mtx_lock_flags                        0x00496540
#define kdlsym_addr__mtx_lock_sleep                        0x004965E0
#define kdlsym_addr__mtx_lock_spin_flags                   0x00496970
#define kdlsym_addr__mtx_unlock_flags                      0x00496810
#define kdlsym_addr__mtx_unlock_sleep                      0x00496910
#define kdlsym_addr__mtx_unlock_spin_flags                 0x00496B30
#define kdlsym_addr__sceSblAuthMgrGetSelfInfo              0x0065E010
#define kdlsym_addr__sceSblAuthMgrSmStart                  0x0065E490
#define kdlsym_addr__sleep                                 0x00229BF0
#define kdlsym_addr__sx_init_flags                         0x00042450
#define kdlsym_addr__sx_slock                              0x000424E0
#define kdlsym_addr__sx_sunlock                            0x000427C0
#define kdlsym_addr__sx_xlock                              0x000426C0
#define kdlsym_addr__sx_xunlock                            0x00042880
#define kdlsym_addr__thread_lock_flags                     0x00496C90
#define kdlsym_addr__vm_map_lock_read                      0x0044CD40
#define kdlsym_addr__vm_map_unlock_read                    0x0044CD90
#define kdlsym_addr_AesCbcCfb128Decrypt                    0x003C0550
#define kdlsym_addr_AesCbcCfb128Encrypt                    0x003C0320
#define kdlsym_addr_allproc                                0x022BBE80
#define kdlsym_addr_allproc_lock                           0x022BBE20
#define kdlsym_addr_avcontrol_sleep                        0x00700E50
#define kdlsym_addr_cloneuio                               0x002D3060
#define kdlsym_addr_console_cdev                           0x022C0CD8
#define kdlsym_addr_console_write                          0x003F8710
#define kdlsym_addr_contigfree                             0x000B7510
#define kdlsym_addr_contigmalloc                           0x000B7150
#define kdlsym_addr_copyin                                 0x003C17A0
#define kdlsym_addr_copyinstr                              0x003C1C50
#define kdlsym_addr_copyout                                0x003C16B0
#define kdlsym_addr_critical_enter                         0x002AA0A0
#define kdlsym_addr_critical_exit                          0x002AA0B0
#define kdlsym_addr_deci_tty_write                         0x004A97E0
#define kdlsym_addr_destroy_dev                            0x003BDCB0
#define kdlsym_addr_dmem_start_app_process                 0x0007CC90
#define kdlsym_addr_dynlib_do_dlsym                        0x00417960
#define kdlsym_addr_dynlib_find_obj_by_handle              0x00418AC0
#define kdlsym_addr_eventhandler_deregister                0x00403220
#define kdlsym_addr_eventhandler_find_list                 0x00403420
#define kdlsym_addr_eventhandler_register                  0x00402E80
#define kdlsym_addr_exec_new_vmspace                       0x0011AE30
#define kdlsym_addr_faultin                                0x003E0410
#define kdlsym_addr_fget_unlocked                          0x0024BAA0
#define kdlsym_addr_fpu_kern_ctx                           0x02694080
#define kdlsym_addr_fpu_kern_enter                         0x0036B6E0
#define kdlsym_addr_fpu_kern_leave                         0x0036B7D0
#define kdlsym_addr_free                                   0x0000D9A0
#define kdlsym_addr_gdt                                    0x01BBCA10
#define kdlsym_addr_gpu_va_page_list                       0x0266AC68
#define kdlsym_addr_icc_nvs_read                           0x00464450
#define kdlsym_addr_kern_close                             0x00249400
#define kdlsym_addr_kern_ioctl                             0x0039C5C0
#define kdlsym_addr_kern_mkdirat                           0x004A3AE0
#define kdlsym_addr_kern_open                              0x0049E990
#define kdlsym_addr_kern_openat                            0x0049E9F0
#define kdlsym_addr_kern_readv                             0x0039B790
#define kdlsym_addr_kern_reboot                            0x00206D50
#define kdlsym_addr_kern_sysents                           0x0111E000
#define kdlsym_addr_kern_thr_create                        0x004A6FB0
#define kdlsym_addr_kernel_map                             0x0220DFC0
#define kdlsym_addr_kernel_mount                           0x00442F90
#define kdlsym_addr_killproc                               0x0002DC80
#define kdlsym_addr_kmem_alloc                             0x00250730
#define kdlsym_addr_kmem_free                              0x00250900
#define kdlsym_addr_kproc_create                           0x0008A0A0
#define kdlsym_addr_kproc_exit                             0x0008A310
#define kdlsym_addr_kthread_add                            0x0008A600
#define kdlsym_addr_kthread_exit                           0x0008A8F0
#define kdlsym_addr_M_IOV                                  0x01A87AD0
#define kdlsym_addr_M_LINKER                               0x01A8EDB0
#define kdlsym_addr_M_MOUNT                                0x01A90CA0
#define kdlsym_addr_M_TEMP                                 0x01540EB0
#define kdlsym_addr_make_dev_p                             0x003BD770
#define kdlsym_addr_malloc                                 0x0000D7A0
#define kdlsym_addr_memcmp                                 0x00207E40
#define kdlsym_addr_memcpy                                 0x003C15B0
#define kdlsym_addr_memmove                                0x003EBB00
#define kdlsym_addr_memset                                 0x001687D0
#define kdlsym_addr_mini_syscore_self_binary               0x0156A588
#define kdlsym_addr_mount_arg                              0x00442CF0
#define kdlsym_addr_mount_argb                             0x004412B0
#define kdlsym_addr_mount_argf                             0x00442DE0
#define kdlsym_addr_mtx_destroy                            0x00497050
#define kdlsym_addr_mtx_init                               0x00496FE0
#define kdlsym_addr_mtx_lock_sleep                         0x004965E0
#define kdlsym_addr_mtx_unlock_sleep                       0x00496910
#define kdlsym_addr_name_to_nids                           0x00417C40
#define kdlsym_addr_pause                                  0x0022A080
#define kdlsym_addr_pfind                                  0x0033DC90
#define kdlsym_addr_pmap_activate                          0x0005A280
#define kdlsym_addr_printf                                 0x00123280
#define kdlsym_addr_prison0                                0x0113E518
#define kdlsym_addr_proc0                                  0x022BFA40
#define kdlsym_addr_proc_reparent                          0x004066F0
#define kdlsym_addr_proc_rwmem                             0x0010EE10
#define kdlsym_addr_realloc                                0x0000DAD0
#define kdlsym_addr_rootvnode                              0x02300320
#define kdlsym_addr_RsaesPkcs1v15Dec2048CRT                0x001D6050
#define kdlsym_addr_sbl_eap_internal_partition_key         0x026DCCD0
#define kdlsym_addr_sbl_keymgr_buf_gva                     0x02698808
#define kdlsym_addr_sbl_keymgr_buf_va                      0x02698000
#define kdlsym_addr_sbl_keymgr_key_rbtree                  0x02694580
#define kdlsym_addr_sbl_keymgr_key_slots                   0x02694570
#define kdlsym_addr_sbl_pfs_sx                             0x02679040
#define kdlsym_addr_sbl_drv_msg_mtx                        0x0266AC70
#define kdlsym_addr_sceSblACMgrGetPathId                   0x00233C70
#define kdlsym_addr_sceSblAuthMgrIsLoadable2               0x0065D7A0
#define kdlsym_addr_sceSblAuthMgrSmVerifyHeader            0x00660260
#define kdlsym_addr_sceSblAuthMgrVerifyHeader              0x0065D800
#define kdlsym_addr_sceSblDriverSendMsg                    0x00637AE0
#define kdlsym_addr_sceSblGetEAPInternalPartitionKey       0x0063F550
#define kdlsym_addr_sceSblKeymgrClearKey                   0x00649B80
#define kdlsym_addr_sceSblKeymgrSetKeyForPfs               0x00649800
#define kdlsym_addr_sceSblKeymgrSetKeyStorage              0x00646E00
#define kdlsym_addr_sceSblKeymgrSmCallfunc                 0x006493D0
#define kdlsym_addr_sceSblPfsSetKeys                       0x00641520
#define kdlsym_addr_sceSblRngGetRandomNumber               0x00665230
#define kdlsym_addr_sceSblServiceMailbox                   0x0064CC20
#define kdlsym_addr_sched_prio                             0x004453C0
#define kdlsym_addr_self_orbis_sysvec                      0x01A8A398
#define kdlsym_addr_Sha256Hmac                             0x00335B70
#define kdlsym_addr_snprintf                               0x00123590
#define kdlsym_addr_spinlock_exit                          0x000A35F0
#define kdlsym_addr_sprintf                                0x001234C0
#define kdlsym_addr_sscanf                                 0x00243810
#define kdlsym_addr_strcmp                                 0x00341810
#define kdlsym_addr_strdup                                 0x002504C0
#define kdlsym_addr_strlen                                 0x002433E0
#define kdlsym_addr_strncmp                                0x0039B6E0
#define kdlsym_addr_strstr                                 0x004817F0
#define kdlsym_addr_sys_accept                             0x001C6540
#define kdlsym_addr_sys_bind                               0x001C5BD0
#define kdlsym_addr_sys_close                              0x002493F0
#define kdlsym_addr_sys_dup2                               0x002475D0
#define kdlsym_addr_sys_fstat                              0x00249990
#define kdlsym_addr_sys_getdents                           0x004A4280
#define kdlsym_addr_sys_kill                               0x0002B5C0
#define kdlsym_addr_sys_listen                             0x001C5E10
#define kdlsym_addr_sys_lseek                              0x004A0950
#define kdlsym_addr_sys_mkdir                              0x004A3A60
#define kdlsym_addr_sys_mlock                              0x000AC1E0
#define kdlsym_addr_sys_mlockall                           0x000AC290
#define kdlsym_addr_sys_mmap                               0x000AB1A0
#define kdlsym_addr_sys_munmap                             0x000AB8F0
#define kdlsym_addr_sys_nmount                             0x0043F9C0
#define kdlsym_addr_sys_open                               0x0049E970
#define kdlsym_addr_sys_ptrace                             0x0010F4B0
#define kdlsym_addr_sys_read                               0x0039B720
#define kdlsym_addr_sys_recvfrom                           0x001C77E0
#define kdlsym_addr_sys_rmdir                              0x004A3DE0
#define kdlsym_addr_sys_sendto                             0x001C70B0
#define kdlsym_addr_sys_setuid                             0x0010BDB0
#define kdlsym_addr_sys_shutdown                           0x001C7A30
#define kdlsym_addr_sys_socket                             0x001C52B0
#define kdlsym_addr_sys_stat                               0x004A0F30
#define kdlsym_addr_sys_unlink                             0x004A0310
#define kdlsym_addr_sys_unmount                            0x004412D0
#define kdlsym_addr_sys_wait4                              0x00406830
#define kdlsym_addr_sys_write                              0x0039BCF0
#define kdlsym_addr_trap_fatal                             0x002ED2E0
#define kdlsym_addr_utilUSleep                             0x0069B2A0
#define kdlsym_addr_vm_fault_disable_pagefaults            0x000C0BB0
#define kdlsym_addr_vm_fault_enable_pagefaults             0x000C0BE0
#define kdlsym_addr_vm_map_lookup_entry                    0x0044D330
#define kdlsym_addr_vmspace_acquire_ref                    0x0044CB90
#define kdlsym_addr_vmspace_alloc                          0x0044C710
#define kdlsym_addr_vmspace_free                           0x0044C9C0
#define kdlsym_addr_vn_fullpath                            0x002F0C40
#define kdlsym_addr_vsnprintf                              0x00123630
#define kdlsym_addr_wakeup                                 0x0022A0A0
#define kdlsym_addr_Xfast_syscall                          0x000001C0

// Kernel Hooks
#define kdlsym_addr_printf_hook                            0x01A9FE98

// FakeSelf Hooks
#define kdlsym_addr_sceSblAuthMgrIsLoadable__sceSblACMgrGetPathId_hook        0x006591BC
#define kdlsym_addr_sceSblAuthMgrIsLoadable2_hook                             0x0065930F
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook   0x00661571
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook 0x0066092A
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookA                           0x00659AC6
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookB                           0x0065A758

// FakePkg Hooks
#define kdlsym_addr_sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg_hook       0x00646EA5
#define kdlsym_addr_sceSblKeymgrInvalidateKey__sx_xlock_hook                  0x0064AA3D
#define kdlsym_addr_npdrm_decrypt_isolated_rif__sceSblKeymgrSmCallfunc_hook   0x00669500
#define kdlsym_addr_npdrm_decrypt_rif_new__sceSblKeymgrSmCallfunc_hook        0x0066A313
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookA                          0x006CDF15
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookB                          0x006CE141

// sceRegMgr
#define kdlsym_addr_sceRegMgrGetInt                        0x00508A60
#define kdlsym_addr_sceRegMgrSetInt                        0x005077D0
#define kdlsym_addr_sceRegMgrGetBin                        0x005093A0
#define kdlsym_addr_sceRegMgrSetBin                        0x005092F0
#define kdlsym_addr_sceRegMgrGetStr                        0x00509220
#define kdlsym_addr_sceRegMgrSetStr                        0x00509060

// SceShellCore patches - call sceKernelIsGenuineCEX
#define ssc_sceKernelIsGenuineCEX_patchA                   0x00189602
#define ssc_sceKernelIsGenuineCEX_patchB                   0x00835642
#define ssc_sceKernelIsGenuineCEX_patchC                   0x00880492
#define ssc_sceKernelIsGenuineCEX_patchD                   0x00A12B92

// SceShellCore patches - call nidf_libSceDipsw
#define ssc_nidf_libSceDipsw_patchA                        0x00189630
#define ssc_nidf_libSceDipsw_patchB                        0x00254107
#define ssc_nidf_libSceDipsw_patchC                        0x00835670
#define ssc_nidf_libSceDipsw_patchD                        0x00A12BC0

#define ssc_enable_fakepkg_patch                           0x003EFCF0

// SceShellCore patches - use free prefix instead fake
#define ssc_fake_to_free_patch                             0x00FD2BF1

// SceShellCore patches - enable remote pkg installer
#define ssc_enable_data_mount_patch                        0x0033943E

// SceShellCore patches - enable VR without spoof
#define ssc_enable_vr_patch                                0x00DDDD70

// SceShellCore patches - enable official external HDD support (Support added in 4.50
#define ssc_external_hdd_pkg_installer_patch               0x009FB311
#define ssc_external_hdd_version_patchA                    0x00606A0D
#define ssc_external_hdd_version_patchB                    0x00149BF0

// SceShellUI patches - debug patches
#define ssu_sceSblRcMgrIsAllowDebugMenuForSettings_patch   0x0001D670
#define ssu_sceSblRcMgrIsStoreMode_patch                   0x0001D9D0

// SceShellUI - remote play related patching
#define ssu_CreateUserForIDU_patch                         0x001A0900
#define ssu_remote_play_menu_patch                         0x00EC8291

// SceRemotePlay - enabler patches
#define srp_enabler_patchA                                 0x0010C6D4
#define srp_enabler_patchB                                 0x0010C6EF

#endif
