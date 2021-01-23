#pragma once
#include <Boot/Config.hpp>

#if MIRA_PLATFORM == MIRA_PLATFORM_ORBIS_BSD_755
/*
    These are the required functions in order for the Oni Framework to operate properly
    These are all offsets into the base of the kernel. They expect all standard FreeBSD 9 prototypes

    The reason we do not hardcode offsets here, is due to the different platforms that are supported, and
    for the platforms that do enable kernel ASLR (Address Space Layout Randomization?)
*/

#define kdlsym_addr__mtx_lock_flags                        0x0030FB10
#define kdlsym_addr__mtx_lock_sleep                        0x0030FBB0
#define kdlsym_addr__mtx_lock_spin_flags                   0x0030FF30
#define kdlsym_addr__mtx_unlock_flags                      0x0030FDD0
#define kdlsym_addr__mtx_unlock_sleep                      0x0030FED0
#define kdlsym_addr__mtx_unlock_spin_flags                 0x003100F0
#define kdlsym_addr__sceSblAuthMgrGetSelfInfo              0x0065C8E0
#define kdlsym_addr__sceSblAuthMgrSmStart                  0x00655C50
#define kdlsym_addr__sx_init_flags                         0x000D1380
#define kdlsym_addr__sx_slock                              0x000D1420
#define kdlsym_addr__sx_sunlock                            0x000D1700
#define kdlsym_addr__sx_xlock                              0x000D1600
#define kdlsym_addr__sx_xunlock                            0x000D17C0
#define kdlsym_addr__thread_lock_flags                     0x00310260
#define kdlsym_addr__vm_map_lock_read                      0x002FC430
#define kdlsym_addr__vm_map_unlock_read                    0x002FC480
#define kdlsym_addr_AesCbcCfb128Decrypt                    0x0021FA40
#define kdlsym_addr_AesCbcCfb128Encrypt                    0x0021F810
#define kdlsym_addr_allproc                                0x0213C828
#define kdlsym_addr_allproc_lock                           0x0213C7C8
#define kdlsym_addr_avcontrol_sleep                        0x006FEA00
#define kdlsym_addr_cloneuio                               0x00388C70
#define kdlsym_addr_console_cdev                           0x0213C7C0
#define kdlsym_addr_console_write                          0x0012D8D0
#define kdlsym_addr_contigfree                             0x0049E280
#define kdlsym_addr_contigmalloc                           0x0049DED0
#define kdlsym_addr_copyin                                 0x0028F9F0
#define kdlsym_addr_copyinstr                              0x0028FEA0
#define kdlsym_addr_copyout                                0x0028F900
#define kdlsym_addr_critical_enter                         0x00347850
#define kdlsym_addr_critical_exit                          0x00347870
#define kdlsym_addr_deci_tty_write                         0x004A6140
#define kdlsym_addr_destroy_dev                            0x0009F680
#define kdlsym_addr_dmem_start_app_process                 0x000C85F0
#define kdlsym_addr_dynlib_do_dlsym                        0x000270E0
#define kdlsym_addr_dynlib_find_obj_by_handle              0x00028250
#define kdlsym_addr_eventhandler_deregister                0x000D3A00
#define kdlsym_addr_eventhandler_find_list                 0x000D3C00
#define kdlsym_addr_eventhandler_register                  0x000D3670
#define kdlsym_addr_exec_new_vmspace                       0x00261500
#define kdlsym_addr_faultin                                0x002B76D0
#define kdlsym_addr_fget_unlocked                          0x003556A0
#define kdlsym_addr_fpu_kern_ctx                           0x02689740
#define kdlsym_addr_fpu_kern_enter                         0x004A5260
#define kdlsym_addr_fpu_kern_leave                         0x004A5350
#define kdlsym_addr_free                                   0x001D6870
#define kdlsym_addr_gdt                                    0x0220D3C0
#define kdlsym_addr_gpu_va_page_list                       0x02662648
#define kdlsym_addr_icc_nvs_read                           0x0002F930
#define kdlsym_addr_kern_close                             0x00353000
#define kdlsym_addr_kern_ioctl                             0x002B4A70
#define kdlsym_addr_kern_mkdirat                           0x000F9B70
#define kdlsym_addr_kern_open                              0x000F49E0
#define kdlsym_addr_kern_openat                            0x000F4A40
#define kdlsym_addr_kern_readv                             0x002B3C30
#define kdlsym_addr_kern_reboot                            0x000D28E0
#define kdlsym_addr_kern_sysents                           0x01122340
#define kdlsym_addr_kern_thr_create                        0x0047AB60
#define kdlsym_addr_kernel_map                             0x021405B8
#define kdlsym_addr_kernel_mount                           0x000790D0
#define kdlsym_addr_killproc                               0x0045FF30
#define kdlsym_addr_kmem_alloc                             0x001753E0
#define kdlsym_addr_kmem_free                              0x001755B0
#define kdlsym_addr_kproc_create                           0x0000D8F0
#define kdlsym_addr_kproc_exit                             0x0000DB60
#define kdlsym_addr_kthread_add                            0x0000DE50
#define kdlsym_addr_kthread_exit                           0x0000E140
#define kdlsym_addr_M_IOV                                  0x01570A30
#define kdlsym_addr_M_LINKER                               0x01555910
#define kdlsym_addr_M_MOUNT                                0x01543A60
#define kdlsym_addr_M_TEMP                                 0x01556DA0
#define kdlsym_addr_make_dev_p                             0x0009F150
#define kdlsym_addr_malloc                                 0x001D6680
#define kdlsym_addr_memcmp                                 0x0031D250
#define kdlsym_addr_memcpy                                 0x0028F800
#define kdlsym_addr_memmove                                0x0038CD20
#define kdlsym_addr_memset                                 0x0008D6F0
#define kdlsym_addr_mini_syscore_self_binary               0x015A8FC8
#define kdlsym_addr_mount_arg                              0x00078E30
#define kdlsym_addr_mount_argb                             0x000773D0
#define kdlsym_addr_mount_argf                             0x00078F20
#define kdlsym_addr_mtx_destroy                            0x00310620
#define kdlsym_addr_mtx_init                               0x003105B0
#define kdlsym_addr_mtx_lock_sleep                         0x0030FBB0
#define kdlsym_addr_mtx_unlock_sleep                       0x0030FED0
#define kdlsym_addr_name_to_nids                           0x000273C0
#define kdlsym_addr_pause                                  0x00086E80
#define kdlsym_addr_pfind                                  0x0012E7F0
#define kdlsym_addr_pmap_activate                          0x001B27F0
#define kdlsym_addr_printf                                 0x0026F740
#define kdlsym_addr_prison0                                0x0158DC60
#define kdlsym_addr_proc0                                  0x021AF840
#define kdlsym_addr_proc_reparent                          0x0024A230
#define kdlsym_addr_proc_rwmem                             0x00361310
#define kdlsym_addr_realloc                                0x001D69A0
#define kdlsym_addr_rootvnode                              0x01B463E0
#define kdlsym_addr_RsaesPkcs1v15Dec2048CRT                0x001517F0
#define kdlsym_addr_sbl_eap_internal_partition_key         0x026D4C90
#define kdlsym_addr_sbl_keymgr_buf_gva                     0x02688808
#define kdlsym_addr_sbl_keymgr_buf_va                      0x02688000
#define kdlsym_addr_sbl_keymgr_key_rbtree                  0x02684248
#define kdlsym_addr_sbl_keymgr_key_slots                   0x02684238
#define kdlsym_addr_sbl_pfs_sx                             0x0267C040
#define kdlsym_addr_sbl_drv_msg_mtx                        0x02662650
#define kdlsym_addr_sceSblACMgrGetPathId                   0x00364D80
#define kdlsym_addr_sceSblAuthMgrIsLoadable2               0x0065C090
#define kdlsym_addr_sceSblAuthMgrSmVerifyHeader            0x00657A30
#define kdlsym_addr_sceSblAuthMgrVerifyHeader              0x0065C0F0
#define kdlsym_addr_sceSblDriverSendMsg                    0x00634A40
#define kdlsym_addr_sceSblGetEAPInternalPartitionKey       0x00642260
#define kdlsym_addr_sceSblKeymgrClearKey                   0x00643E80
#define kdlsym_addr_sceSblKeymgrSetKeyForPfs               0x00643B20
#define kdlsym_addr_sceSblKeymgrSetKeyStorage              0x0063E3E0
#define kdlsym_addr_sceSblKeymgrSmCallfunc                 0x006436F0
#define kdlsym_addr_sceSblPfsSetKeys                       0x0063F100
#define kdlsym_addr_sceSblRngGetRandomNumber               0x006621F0
#define kdlsym_addr_sceSblServiceMailbox                   0x0064A1A0
#define kdlsym_addr_sched_prio                             0x0018FDF0
#define kdlsym_addr_self_orbis_sysvec                      0x01564E80
#define kdlsym_addr_Sha256Hmac                             0x00274740
#define kdlsym_addr_snprintf                               0x0026FA40
#define kdlsym_addr_spinlock_exit                          0x003DAFE0
#define kdlsym_addr_sprintf                                0x0026F980
#define kdlsym_addr_sscanf                                 0x001C4840
#define kdlsym_addr_strcmp                                 0x00104BA0
#define kdlsym_addr_strdup                                 0x00110FA0
#define kdlsym_addr_strlen                                 0x002E8BC0
#define kdlsym_addr_strncmp                                0x000BF670
#define kdlsym_addr_strstr                                 0x003B0250
#define kdlsym_addr_sys_accept                             0x000D63B0
#define kdlsym_addr_sys_bind                               0x000D5A30
#define kdlsym_addr_sys_close                              0x00352FF0
#define kdlsym_addr_sys_dup2                               0x003511C0
#define kdlsym_addr_sys_fstat                              0x00353560
#define kdlsym_addr_sys_getdents                           0x000FA330
#define kdlsym_addr_sys_kill                               0x0045D800
#define kdlsym_addr_sys_listen                             0x000D5C70
#define kdlsym_addr_sys_lseek                              0x000F6A00
#define kdlsym_addr_sys_mkdir                              0x000F9AF0
#define kdlsym_addr_sys_mlock                              0x000DBD90
#define kdlsym_addr_sys_mlockall                           0x000DBE40
#define kdlsym_addr_sys_mmap                               0x000DADA0
#define kdlsym_addr_sys_munmap                             0x000DB4C0
#define kdlsym_addr_sys_nmount                             0x00075AF0
#define kdlsym_addr_sys_open                               0x000F49C0
#define kdlsym_addr_sys_ptrace                             0x003619B0
#define kdlsym_addr_sys_read                               0x002B3BB0
#define kdlsym_addr_sys_recvfrom                           0x000D7660
#define kdlsym_addr_sys_rmdir                              0x000F9E70
#define kdlsym_addr_sys_sendto                             0x000D6F30
#define kdlsym_addr_sys_setuid                             0x0037A200
#define kdlsym_addr_sys_shutdown                           0x000D78B0
#define kdlsym_addr_sys_socket                             0x000D5110
#define kdlsym_addr_sys_stat                               0x000F6FE0
#define kdlsym_addr_sys_unlink                             0x000F63C0
#define kdlsym_addr_sys_unmount                            0x000773F0
#define kdlsym_addr_sys_wait4                              0x0024A370
#define kdlsym_addr_sys_write                              0x002B40C0
#define kdlsym_addr_trap_fatal                             0x0046D0C0
#define kdlsym_addr_utilUSleep                             0x006730E0
#define kdlsym_addr_vm_fault_disable_pagefaults            0x003E35E0
#define kdlsym_addr_vm_fault_enable_pagefaults             0x003E3610
#define kdlsym_addr_vm_map_lookup_entry                    0x002FCA70
#define kdlsym_addr_vmspace_acquire_ref                    0x002FC290
#define kdlsym_addr_vmspace_alloc                          0x002FBE00
#define kdlsym_addr_vmspace_free                           0x002FC0C0
#define kdlsym_addr_vn_fullpath                            0x002C3570
#define kdlsym_addr_vsnprintf                              0x0026FAE0
#define kdlsym_addr_wakeup                                 0x00086EA0
#define kdlsym_addr_Xfast_syscall                          0x000001C0

// Kernel Hooks
#define kdlsym_addr_printf_hook                            0x01A9BF78

// FakeSelf Hooks
#define kdlsym_addr_sceSblAuthMgrIsLoadable__sceSblACMgrGetPathId_hook        0x0065A51C
#define kdlsym_addr_sceSblAuthMgrIsLoadable2_hook                             0x0065A66E
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook   0x00658D48
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook 0x006580FD
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookA                           0x0065AE06
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookB                           0x0065BAE9

// FakePkg Hooks
#define kdlsym_addr_sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg_hook       0x0063E485
#define kdlsym_addr_sceSblKeymgrInvalidateKey__sx_xlock_hook                  0x00644CFD
#define kdlsym_addr_npdrm_decrypt_isolated_rif__sceSblKeymgrSmCallfunc_hook   0x006667D0
#define kdlsym_addr_npdrm_decrypt_rif_new__sceSblKeymgrSmCallfunc_hook        0x0066759E
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookA                          0x006D9757
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookB                          0x006D9988

// sceRegMgr
#define kdlsym_addr_sceRegMgrGetInt                        0x00500280
#define kdlsym_addr_sceRegMgrSetInt                        0x004FEFC0
#define kdlsym_addr_sceRegMgrGetBin                        0x00500BD0
#define kdlsym_addr_sceRegMgrSetBin                        0x00500B20
#define kdlsym_addr_sceRegMgrGetStr                        0x00500A50
#define kdlsym_addr_sceRegMgrSetStr                        0x00500890

// SceShellCore patches - call sceKernelIsGenuineCEX
#define ssc_sceKernelIsGenuineCEX_patchA                   0x00168A90
#define ssc_sceKernelIsGenuineCEX_patchB                   0x007FBF00
#define ssc_sceKernelIsGenuineCEX_patchC                   0x0084AF42
#define ssc_sceKernelIsGenuineCEX_patchD                   0x009D3150

// SceShellCore patches - call nidf_libSceDipsw
#define ssc_nidf_libSceDipsw_patchA                        0x00168ABA
#define ssc_nidf_libSceDipsw_patchB                        0x00316BD3
#define ssc_nidf_libSceDipsw_patchC                        0x007FBF2A
#define ssc_nidf_libSceDipsw_patchD                        0x009D317A

#define ssc_enable_fakepkg_patch                           0x003C244F

// SceShellCore patches - use free prefix instead fake
#define ssc_fake_to_free_patch                             0x00F66831

// SceShellCore patches - enable remote pkg installer
#define ssc_enable_data_mount_patch                        0x00316BC3

// SceShellCore patches - enable VR without spoof
#define ssc_enable_vr_patch                                0x00D57E60

// SceShellCore patches - enable official external HDD support (Support added in 4.50)
#define ssc_external_hdd_pkg_installer_patch               0x009BC141
#define ssc_external_hdd_version_patchA                    0x005BCF2D
#define ssc_external_hdd_version_patchB                    0x00133080

// SceShellUI patches - debug patches
#define ssu_sceSblRcMgrIsAllowDebugMenuForSettings_patch   0x0001D140
#define ssu_sceSblRcMgrIsStoreMode_patch                   0x0001D4A0

// SceShellUI - remote play related patching
#define ssu_CreateUserForIDU_patch                         0x0018E120
#define ssu_remote_play_menu_patch                         0x00EC66E1

// SceRemotePlay - enabler patches
#define srp_enabler_patchA                                 0x0010A13A
#define srp_enabler_patchB                                 0x0010A155

#endif
