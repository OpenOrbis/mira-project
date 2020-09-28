#pragma once
#include <Boot/Config.hpp>

#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_620
/*
    These are the required functions in order for the Oni Framework to operate properly
    These are all offsets into the base of the kernel. They expect all standard FreeBSD 9 prototypes

    The reason we do not hardcode offsets here, is due to the different platforms that are supported, and
    for the platforms that do enable kernel ASLR (Address Space Layout Randomization?)
*/

#define kdlsym_addr__mtx_lock_flags                        0x00007470
#define kdlsym_addr__mtx_lock_sleep                        0x00007510
#define kdlsym_addr__mtx_lock_spin_flags                   0x000078A0
#define kdlsym_addr__mtx_unlock_flags                      0x00007740
#define kdlsym_addr__mtx_unlock_sleep                      0x00007840
#define kdlsym_addr__mtx_unlock_spin_flags                 0x00007A60
#define kdlsym_addr__sceSblAuthMgrGetSelfInfo              0x006575B0
#define kdlsym_addr__sceSblAuthMgrSmStart                  0x0065B070
#define kdlsym_addr__sx_init_flags                         0x00083CA0
#define kdlsym_addr__sx_slock                              0x00083D20
#define kdlsym_addr__sx_sunlock                            0x00084000
#define kdlsym_addr__sx_xlock                              0x00083F00
#define kdlsym_addr__sx_xunlock                            0x000840C0
#define kdlsym_addr__thread_lock_flags                     0x00007BC0
#define kdlsym_addr__vm_map_lock_read                      0x0034D260
#define kdlsym_addr__vm_map_unlock_read                    0x0034D2B0
#define kdlsym_addr_AesCbcCfb128Decrypt                    0x002D4320
#define kdlsym_addr_AesCbcCfb128Encrypt                    0x002D40F0
#define kdlsym_addr_allproc                                0x022FBCD0
#define kdlsym_addr_allproc_lock                           0x022FBC70
#define kdlsym_addr_avcontrol_sleep                        0x00704200
#define kdlsym_addr_cloneuio                               0x00396310
#define kdlsym_addr_console_cdev                           0x021413E8
#define kdlsym_addr_console_write                          0x0007F230
#define kdlsym_addr_contigfree                             0x0009EF50
#define kdlsym_addr_contigmalloc                           0x0009EB90
#define kdlsym_addr_copyin                                 0x001148F0
#define kdlsym_addr_copyinstr                              0x00114DA0
#define kdlsym_addr_copyout                                0x00114800
#define kdlsym_addr_critical_enter                         0x00157850
#define kdlsym_addr_critical_exit                          0x00157860
#define kdlsym_addr_deci_tty_write                         0x004A8DB0
#define kdlsym_addr_destroy_dev                            0x00207730
#define kdlsym_addr_dmem_start_app_process                 0x000B5A50
#define kdlsym_addr_dynlib_do_dlsym                        0x00017A20
#define kdlsym_addr_dynlib_find_obj_by_handle              0x00019020
#define kdlsym_addr_eventhandler_deregister                0x00181320
#define kdlsym_addr_eventhandler_find_list                 0x00181530
#define kdlsym_addr_eventhandler_register                  0x00180F80
#define kdlsym_addr_exec_new_vmspace                       0x003D8300
#define kdlsym_addr_faultin                                0x001B38D0
#define kdlsym_addr_fget_unlocked                          0x0024BAA0
#define kdlsym_addr_fpu_kern_ctx                           0x0263A6C0
#define kdlsym_addr_fpu_kern_enter                         0x001E3990
#define kdlsym_addr_fpu_kern_leave                         0x001E3A90
#define kdlsym_addr_free                                   0x001D9260
#define kdlsym_addr_gdt                                    0x021425F0
#define kdlsym_addr_gpu_va_page_list                       0x02659E58
#define kdlsym_addr_icc_nvs_read                           0x00462340
#define kdlsym_addr_kern_close                             0x00068AC0
#define kdlsym_addr_kern_ioctl                             0x0030BAD0
#define kdlsym_addr_kern_mkdirat                           0x00349C40
#define kdlsym_addr_kern_open                              0x003447F0
#define kdlsym_addr_kern_openat                            0x00344850
#define kdlsym_addr_kern_readv                             0x0030AC70
#define kdlsym_addr_kern_reboot                            0x001D45E0
#define kdlsym_addr_kern_sysents                           0x0111F540
#define kdlsym_addr_kern_thr_create                        0x00470D40
#define kdlsym_addr_kernel_map                             0x021CBD88
#define kdlsym_addr_kernel_mount                           0x00011C80
#define kdlsym_addr_killproc                               0x00229AF0
#define kdlsym_addr_kmem_alloc                             0x00270420
#define kdlsym_addr_kmem_free                              0x002705F0
#define kdlsym_addr_kproc_create                           0x00197A20
#define kdlsym_addr_kproc_exit                             0x00197C90
#define kdlsym_addr_kthread_add                            0x00197F90
#define kdlsym_addr_kthread_exit                           0x00198270
#define kdlsym_addr_M_IOV                                  0x01A7C2F0
#define kdlsym_addr_M_LINKER                               0x01A82950
#define kdlsym_addr_M_MOUNT                                0x00D41270
#define kdlsym_addr_M_TEMP                                 0x0155D070
#define kdlsym_addr_make_dev_p                             0x002071F0
#define kdlsym_addr_malloc                                 0x001D9060
#define kdlsym_addr_memcmp                                 0x004A1740
#define kdlsym_addr_memcpy                                 0x00114700
#define kdlsym_addr_memmove                                0x00032C50
#define kdlsym_addr_memset                                 0x00394C60
#define kdlsym_addr_mini_syscore_self_binary               0x0157F648
#define kdlsym_addr_mount_arg                              0x000119E0
#define kdlsym_addr_mount_argb                             0x0000FFB0
#define kdlsym_addr_mount_argf                             0x00011AD0
#define kdlsym_addr_mtx_destroy                            0x00007FA0
#define kdlsym_addr_mtx_init                               0x00007F30
#define kdlsym_addr_mtx_lock_sleep                         0x00007510
#define kdlsym_addr_mtx_unlock_sleep                       0x00007840
#define kdlsym_addr_name_to_nids                           0x00017D00
#define kdlsym_addr_pause                                  0x000973D0
#define kdlsym_addr_pfind                                  0x004A1F00
#define kdlsym_addr_pmap_activate                          0x002F4BC0
#define kdlsym_addr_printf                                 0x00307E10
#define kdlsym_addr_prison0                                0x0113D458
#define kdlsym_addr_proc0                                  0x021C6F90
#define kdlsym_addr_proc_reparent                          0x00076450
#define kdlsym_addr_proc_rwmem                             0x0013E7A0
#define kdlsym_addr_realloc                                0x001D9390
#define kdlsym_addr_rootvnode                              0x021C3AC0
#define kdlsym_addr_RsaesPkcs1v15Dec2048CRT                0x002209B0
#define kdlsym_addr_sbl_eap_internal_partition_key         0x026CCC60
#define kdlsym_addr_sbl_keymgr_buf_gva                     0x02688808
#define kdlsym_addr_sbl_keymgr_buf_va                      0x02688000
#define kdlsym_addr_sbl_keymgr_key_rbtree                  0x02684A88
#define kdlsym_addr_sbl_keymgr_key_slots                   0x02684A78
#define kdlsym_addr_sbl_pfs_sx                             0x02677290
#define kdlsym_addr_sbl_drv_msg_mtx                        0x02659E60
#define kdlsym_addr_sceSblACMgrGetPathId                   0x004594E0
#define kdlsym_addr_sceSblAuthMgrIsLoadable2               0x00656D20
#define kdlsym_addr_sceSblAuthMgrSmVerifyHeader            0x0065CBD0
#define kdlsym_addr_sceSblAuthMgrVerifyHeader              0x00656D80
#define kdlsym_addr_sceSblDriverSendMsg                    0x006378E0
#define kdlsym_addr_sceSblGetEAPInternalPartitionKey       0x0063C940
#define kdlsym_addr_sceSblKeymgrClearKey                   0x00649280
#define kdlsym_addr_sceSblKeymgrSetKeyForPfs               0x00648EF0
#define kdlsym_addr_sceSblKeymgrSetKeyStorage              0x00646050
#define kdlsym_addr_sceSblKeymgrSmCallfunc                 0x00648AC0
#define kdlsym_addr_sceSblPfsSetKeys                       0x00640C90
#define kdlsym_addr_sceSblRngGetRandomNumber               0x00663000
#define kdlsym_addr_sceSblServiceMailbox                   0x0064B480
#define kdlsym_addr_sched_prio                             0x0040C740
#define kdlsym_addr_self_orbis_sysvec                      0x01568AE8
#define kdlsym_addr_Sha256Hmac                             0x00165D80
#define kdlsym_addr_snprintf                               0x00308120
#define kdlsym_addr_spinlock_exit                          0x000879F0
#define kdlsym_addr_sprintf                                0x00308050
#define kdlsym_addr_sscanf                                 0x000FEA40
#define kdlsym_addr_strcmp                                 0x0015E8C0
#define kdlsym_addr_strdup                                 0x0034AA20
#define kdlsym_addr_strlen                                 0x000D5AA0
#define kdlsym_addr_strncmp                                0x001FE650
#define kdlsym_addr_strstr                                 0x004247F0
#define kdlsym_addr_sys_accept                             0x00300A10
#define kdlsym_addr_sys_bind                               0x00300080
#define kdlsym_addr_sys_close                              0x00068AB0
#define kdlsym_addr_sys_dup2                               0x00066C70
#define kdlsym_addr_sys_fstat                              0x00069050
#define kdlsym_addr_sys_getdents                           0x0034A3F0
#define kdlsym_addr_sys_kill                               0x00227430
#define kdlsym_addr_sys_listen                             0x003002C0
#define kdlsym_addr_sys_lseek                              0x00346AB0
#define kdlsym_addr_sys_mkdir                              0x00349BC0
#define kdlsym_addr_sys_mlock                              0x00240F20
#define kdlsym_addr_sys_mlockall                           0x00240FD0
#define kdlsym_addr_sys_mmap                               0x0023FE90
#define kdlsym_addr_sys_munmap                             0x00240600
#define kdlsym_addr_sys_nmount                             0x0000E6A0
#define kdlsym_addr_sys_open                               0x003447D0
#define kdlsym_addr_sys_ptrace                             0x0013EE50
#define kdlsym_addr_sys_read                               0x0030AC00
#define kdlsym_addr_sys_recvfrom                           0x00301CE0
#define kdlsym_addr_sys_rmdir                              0x00349F40
#define kdlsym_addr_sys_sendto                             0x003015B0
#define kdlsym_addr_sys_setuid                             0x00028FC0
#define kdlsym_addr_sys_shutdown                           0x00301F30
#define kdlsym_addr_sys_socket                             0x002FF750
#define kdlsym_addr_sys_stat                               0x00347090
#define kdlsym_addr_sys_unlink                             0x00346350
#define kdlsym_addr_sys_unmount                            0x0000FFD0
#define kdlsym_addr_sys_wait4                              0x00076590
#define kdlsym_addr_sys_write                              0x0030B1E0
#define kdlsym_addr_target_id                              0x0215DB8D
#define kdlsym_addr_trap_fatal                             0x002E0DD0
#define kdlsym_addr_utilUSleep                             0x006864B0
#define kdlsym_addr_vm_fault_disable_pagefaults            0x003FEE40
#define kdlsym_addr_vm_fault_enable_pagefaults             0x003FEE70
#define kdlsym_addr_vm_map_lookup_entry                    0x0034D860
#define kdlsym_addr_vmspace_acquire_ref                    0x0034D0B0
#define kdlsym_addr_vmspace_alloc                          0x0034CC30
#define kdlsym_addr_vmspace_free                           0x0034CEE0
#define kdlsym_addr_vn_fullpath                            0x0024ABA0
#define kdlsym_addr_vsnprintf                              0x003081C0
#define kdlsym_addr_wakeup                                 0x000973F0
#define kdlsym_addr_Xfast_syscall                          0x000001C0

// Kernel Hooks
#define kdlsym_addr_printf_hook                            0x01A9FD58

// FakeSelf Hooks
#define kdlsym_addr_sceSblAuthMgrIsLoadable__sceSblACMgrGetPathId_hook        0x00658D2C
#define kdlsym_addr_sceSblAuthMgrIsLoadable2_hook                             0x00658E7F
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook   0x0065DEE1
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook 0x0065D29A
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookA                           0x00659636
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookB                           0x0065A2D8

// FakePkg Hooks
#define kdlsym_addr_sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg_hook       0x006460F5
#define kdlsym_addr_sceSblKeymgrInvalidateKey__sx_xlock_hook                  0x0064A11D
#define kdlsym_addr_npdrm_decrypt_isolated_rif__sceSblKeymgrSmCallfunc_hook   0x006685C0
#define kdlsym_addr_npdrm_decrypt_rif_new__sceSblKeymgrSmCallfunc_hook        0x006693D3
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookA                          0x0069FABA
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookB                          0x0069FCE6

// sceRegMgr
#define kdlsym_addr_sceRegMgrGetInt                        0x00503530
#define kdlsym_addr_sceRegMgrSetInt                        0x00502230
#define kdlsym_addr_sceRegMgrGetBin                        0x00503E70
#define kdlsym_addr_sceRegMgrSetBin                        0x00503DC0
#define kdlsym_addr_sceRegMgrGetStr                        0x00503CF0
#define kdlsym_addr_sceRegMgrSetStr                        0x00503B30

// SceShellCore patches - call sceKernelIsGenuineCEX
#define ssc_sceKernelIsGenuineCEX_patchA                   0x00186170
#define ssc_sceKernelIsGenuineCEX_patchB                   0x0081ED20
#define ssc_sceKernelIsGenuineCEX_patchC                   0x00869BA3
#define ssc_sceKernelIsGenuineCEX_patchD                   0x009F7550

// SceShellCore patches - call nidf_libSceDipsw
#define ssc_nidf_libSceDipsw_patchA                        0x0018619A
#define ssc_nidf_libSceDipsw_patchB                        0x0025C923
#define ssc_nidf_libSceDipsw_patchC                        0x0081ED4A
#define ssc_nidf_libSceDipsw_patchD                        0x009F757A

#define ssc_enable_fakepkg_patch                           0x003F7222

// SceShellCore patches - use free prefix instead fake
#define ssc_fake_to_free_patch                             0x00F9FB11

// SceShellCore patches - enable remote pkg installer
#define ssc_enable_data_mount_patch                        0x0033FF4C

// SceShellCore patches - enable VR without spoof
#define ssc_enable_vr_patch                                0x00DBABD0

// SceShellCore patches - enable official external HDD support (Support added in 4.50)
#define ssc_external_hdd_pkg_installer_patch               0x009E0031
#define ssc_external_hdd_version_patchA                    0x0060081D
#define ssc_external_hdd_version_patchB                    0x0014A731

// SceShellUI patches - debug patches
#define ssu_sceSblRcMgrIsAllowDebugMenuForSettings_patch   0x0001D6D0
#define ssu_sceSblRcMgrIsStoreMode_patch                   0x0001DA30

// SceShellUI - remote play related patching
#define ssu_CreateUserForIDU_patch                         0x001A0510
#define ssu_remote_play_menu_patch                         0x00E9F7B1

// SceRemotePlay - enabler patches
#define srp_enabler_patchA                                 0x0003CED6
#define srp_enabler_patchB                                 0x0003CEF1

#endif
