#pragma once
#include <Boot/Config.hpp>

#if MIRA_PLATFORM == MIRA_PLATFORM_ORBIS_BSD_702
/*
    These are the required functions in order for the Oni Framework to operate properly
    These are all offsets into the base of the kernel. They expect all standard FreeBSD 9 prototypes

    The reason we do not hardcode offsets here, is due to the different platforms that are supported, and
    for the platforms that do enable kernel ASLR (Address Space Layout Randomization?)
*/

#define kdlsym_addr__mtx_lock_flags                        0x000BB060
#define kdlsym_addr__mtx_lock_sleep                        0x000BB100
#define kdlsym_addr__mtx_lock_spin_flags                   0x000BB490
#define kdlsym_addr__mtx_unlock_flags                      0x000BB330
#define kdlsym_addr__mtx_unlock_sleep                      0x000BB430
#define kdlsym_addr__mtx_unlock_spin_flags                 0x000BB650
#define kdlsym_addr__sceSblAuthMgrGetSelfInfo              0x00660A90
#define kdlsym_addr__sceSblAuthMgrSmStart                  0x0065A560
#define kdlsym_addr__sx_init_flags                         0x001ADDB0
#define kdlsym_addr__sx_slock                              0x001ADE50
#define kdlsym_addr__sx_sunlock                            0x001AE130
#define kdlsym_addr__sx_xlock                              0x001AE030
#define kdlsym_addr__sx_xunlock                            0x001AE1F0
#define kdlsym_addr__thread_lock_flags                     0x000BB7B0
#define kdlsym_addr__vm_map_lock_read                      0x0025FB90
#define kdlsym_addr__vm_map_unlock_read                    0x0025FBE0
#define kdlsym_addr_AesCbcCfb128Decrypt                    0x001DA640
#define kdlsym_addr_AesCbcCfb128Encrypt                    0x001DA410
#define kdlsym_addr_allproc                                0x01B48318
#define kdlsym_addr_allproc_lock                           0x01B482B8
#define kdlsym_addr_avcontrol_sleep                        0x00704E00
#define kdlsym_addr_cloneuio                               0x0020E7D0
#define kdlsym_addr_console_cdev                           0x021F0778
#define kdlsym_addr_console_write                          0x0021D3E0
#define kdlsym_addr_contigfree                             0x00430AD0
#define kdlsym_addr_contigmalloc                           0x00430710
#define kdlsym_addr_copyin                                 0x0002F230
#define kdlsym_addr_copyinstr                              0x0002F6E0
#define kdlsym_addr_copyout                                0x0002F140
#define kdlsym_addr_critical_enter                         0x003832B0
#define kdlsym_addr_critical_exit                          0x003832D0
#define kdlsym_addr_deci_tty_write                         0x004AA3A0
#define kdlsym_addr_destroy_dev                            0x00422710
#define kdlsym_addr_dmem_start_app_process                 0x00245B80
#define kdlsym_addr_dynlib_do_dlsym                        0x002F02A0
#define kdlsym_addr_dynlib_find_obj_by_handle              0x002F1410
#define kdlsym_addr_eventhandler_deregister                0x00483BB0
#define kdlsym_addr_eventhandler_find_list                 0x00483DB0
#define kdlsym_addr_eventhandler_register                  0x00483810
#define kdlsym_addr_exec_new_vmspace                       0x0008E310
#define kdlsym_addr_faultin                                0x002C9D70
#define kdlsym_addr_fget_unlocked                          0x00324270
#define kdlsym_addr_fpu_kern_ctx                           0x0267B640
#define kdlsym_addr_fpu_kern_enter                         0x002CEBF0
#define kdlsym_addr_fpu_kern_leave                         0x002CECE0
#define kdlsym_addr_free                                   0x00301A40
#define kdlsym_addr_gdt                                    0x022E37E0
#define kdlsym_addr_gpu_va_page_list                       0x02669E48
#define kdlsym_addr_icc_nvs_read                           0x00348AA0
#define kdlsym_addr_kern_close                             0x00321B90
#define kdlsym_addr_kern_ioctl                             0x00192060
#define kdlsym_addr_kern_mkdirat                           0x0035AAC0
#define kdlsym_addr_kern_open                              0x00355960
#define kdlsym_addr_kern_openat                            0x003559C0
#define kdlsym_addr_kern_readv                             0x00191230
#define kdlsym_addr_kern_reboot                            0x002CD780
#define kdlsym_addr_kern_sysents                           0x01125660
#define kdlsym_addr_kern_thr_create                        0x000842E0
#define kdlsym_addr_kernel_map                             0x021C8EE0
#define kdlsym_addr_kernel_mount                           0x00299080
#define kdlsym_addr_killproc                               0x00313B90
#define kdlsym_addr_kmem_alloc                             0x001170F0
#define kdlsym_addr_kmem_free                              0x001172C0
#define kdlsym_addr_kproc_create                           0x000C4170
#define kdlsym_addr_kproc_exit                             0x000C43E0
#define kdlsym_addr_kthread_add                            0x000C46D0
#define kdlsym_addr_kthread_exit                           0x000C49C0
#define kdlsym_addr_M_IOV                                  0x01A64270
#define kdlsym_addr_M_LINKER                               0x01A7B690
#define kdlsym_addr_M_MOUNT                                0x01A71A70
#define kdlsym_addr_M_TEMP                                 0x01A7AE50
#define kdlsym_addr_make_dev_p                             0x004221E0
#define kdlsym_addr_malloc                                 0x00301840
#define kdlsym_addr_memcmp                                 0x00207500
#define kdlsym_addr_memcpy                                 0x0002F040
#define kdlsym_addr_memmove                                0x002B9EF0
#define kdlsym_addr_memset                                 0x002DFC20
#define kdlsym_addr_mini_syscore_self_binary               0x01555BD8
#define kdlsym_addr_mount_arg                              0x00298DE0
#define kdlsym_addr_mount_argb                             0x002973B0
#define kdlsym_addr_mount_argf                             0x00298ED0
#define kdlsym_addr_mtx_destroy                            0x000BBB80
#define kdlsym_addr_mtx_init                               0x000BBB10
#define kdlsym_addr_mtx_lock_sleep                         0x000BB100
#define kdlsym_addr_mtx_unlock_sleep                       0x000BB430
#define kdlsym_addr_name_to_nids                           0x002F0580
#define kdlsym_addr_pause                                  0x0016EEE0
#define kdlsym_addr_pfind                                  0x00015AC0
#define kdlsym_addr_pmap_activate                          0x003EAB30
#define kdlsym_addr_printf                                 0x000BC730
#define kdlsym_addr_prison0                                0x0113E398
#define kdlsym_addr_proc0                                  0x021EF890
#define kdlsym_addr_proc_reparent                          0x001AFCB0
#define kdlsym_addr_proc_rwmem                             0x00043E80
#define kdlsym_addr_realloc                                0x00301B70
#define kdlsym_addr_rootvnode                              0x022C5750
#define kdlsym_addr_RsaesPkcs1v15Dec2048CRT                0x001DD540
#define kdlsym_addr_sbl_eap_internal_partition_key         0x026E0CD0
#define kdlsym_addr_sbl_keymgr_buf_gva                     0x0269C808
#define kdlsym_addr_sbl_keymgr_buf_va                      0x0269C000
#define kdlsym_addr_sbl_keymgr_key_rbtree                  0x02698858
#define kdlsym_addr_sbl_keymgr_key_slots                   0x02698848
#define kdlsym_addr_sbl_pfs_sx                             0x026945C0
#define kdlsym_addr_sbl_drv_msg_mtx                        0x02669E50
#define kdlsym_addr_sceSblACMgrGetPathId                   0x001CB930
#define kdlsym_addr_sceSblAuthMgrIsLoadable2               0x00660210
#define kdlsym_addr_sceSblAuthMgrSmVerifyHeader            0x0065C340
#define kdlsym_addr_sceSblAuthMgrVerifyHeader              0x00660270
#define kdlsym_addr_sceSblDriverSendMsg                    0x006376A0
#define kdlsym_addr_sceSblGetEAPInternalPartitionKey       0x00645810
#define kdlsym_addr_sceSblKeymgrClearKey                   0x006489D0
#define kdlsym_addr_sceSblKeymgrSetKeyForPfs               0x00648650
#define kdlsym_addr_sceSblKeymgrSetKeyStorage              0x0063E230
#define kdlsym_addr_sceSblKeymgrSmCallfunc                 0x00648220
#define kdlsym_addr_sceSblPfsSetKeys                       0x00647000
#define kdlsym_addr_sceSblRngGetRandomNumber               0x00664190
#define kdlsym_addr_sceSblServiceMailbox                   0x0064C110
#define kdlsym_addr_sched_prio                             0x003281F0
#define kdlsym_addr_self_orbis_sysvec                      0x01A4F460
#define kdlsym_addr_Sha256Hmac                             0x00205F50
#define kdlsym_addr_snprintf                               0x000BCA30
#define kdlsym_addr_spinlock_exit                          0x00493FB0
#define kdlsym_addr_sprintf                                0x000BC970
#define kdlsym_addr_sscanf                                 0x002077A0
#define kdlsym_addr_strcmp                                 0x0043B5F0
#define kdlsym_addr_strdup                                 0x000382B0
#define kdlsym_addr_strlen                                 0x00093FF0
#define kdlsym_addr_strncmp                                0x003DABE0
#define kdlsym_addr_strstr                                 0x00005740
#define kdlsym_addr_sys_accept                             0x002902A0
#define kdlsym_addr_sys_bind                               0x0028F930
#define kdlsym_addr_sys_close                              0x00321B80
#define kdlsym_addr_sys_dup2                               0x0031FD50
#define kdlsym_addr_sys_fstat                              0x00322100
#define kdlsym_addr_sys_getdents                           0x0035B270
#define kdlsym_addr_sys_kill                               0x00311490
#define kdlsym_addr_sys_listen                             0x0028FB70
#define kdlsym_addr_sys_lseek                              0x00357940
#define kdlsym_addr_sys_mkdir                              0x0035AA40
#define kdlsym_addr_sys_mlock                              0x001D2F80
#define kdlsym_addr_sys_mlockall                           0x001D3030
#define kdlsym_addr_sys_mmap                               0x001D1F50
#define kdlsym_addr_sys_munmap                             0x001D26A0
#define kdlsym_addr_sys_nmount                             0x00295AC0
#define kdlsym_addr_sys_open                               0x00355940
#define kdlsym_addr_sys_ptrace                             0x00044510
#define kdlsym_addr_sys_read                               0x001911C0
#define kdlsym_addr_sys_recvfrom                           0x00291550
#define kdlsym_addr_sys_rmdir                              0x0035ADC0
#define kdlsym_addr_sys_sendto                             0x00290E20
#define kdlsym_addr_sys_setuid                             0x00087A50
#define kdlsym_addr_sys_shutdown                           0x002917A0
#define kdlsym_addr_sys_socket                             0x0028F010
#define kdlsym_addr_sys_stat                               0x00357F20
#define kdlsym_addr_sys_unlink                             0x00357310
#define kdlsym_addr_sys_unmount                            0x002973D0
#define kdlsym_addr_sys_wait4                              0x001AFDF0
#define kdlsym_addr_sys_write                              0x00191790
#define kdlsym_addr_trap_fatal                             0x0013A450
#define kdlsym_addr_utilUSleep                             0x00679E30
#define kdlsym_addr_vm_fault_disable_pagefaults            0x002C3AC0
#define kdlsym_addr_vm_fault_enable_pagefaults             0x002C3AF0
#define kdlsym_addr_vm_map_lookup_entry                    0x00260190
#define kdlsym_addr_vmspace_acquire_ref                    0x0025F9F0
#define kdlsym_addr_vmspace_alloc                          0x0025F570
#define kdlsym_addr_vmspace_free                           0x0025F820
#define kdlsym_addr_vn_fullpath                            0x0015F470
#define kdlsym_addr_vsnprintf                              0x000BCAD0
#define kdlsym_addr_wakeup                                 0x0016EF00
#define kdlsym_addr_Xfast_syscall                          0x000001C0

// Kernel Hooks
#define kdlsym_addr_printf_hook                            0x01AA0058

// FakeSelf Hooks
#define kdlsym_addr_sceSblAuthMgrIsLoadable__sceSblACMgrGetPathId_hook        0x0065E97C
#define kdlsym_addr_sceSblAuthMgrIsLoadable2_hook                             0x0065EACF
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook   0x0065D669
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook 0x0065CA0D
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookA                           0x0065F256
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookB                           0x0065FEF8

// FakePkg Hooks
#define kdlsym_addr_sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg_hook       0x0063E2D5
#define kdlsym_addr_sceSblKeymgrInvalidateKey__sx_xlock_hook                  0x0064989D
#define kdlsym_addr_npdrm_decrypt_isolated_rif__sceSblKeymgrSmCallfunc_hook   0x00668A50
#define kdlsym_addr_npdrm_decrypt_rif_new__sceSblKeymgrSmCallfunc_hook        0x0066985E
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookA                          0x006B534B
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookB                          0x006B557C

// sceRegMgr
#define kdlsym_addr_sceRegMgrGetInt                        0x00502650
#define kdlsym_addr_sceRegMgrSetInt                        0x005013B0
#define kdlsym_addr_sceRegMgrGetBin                        0x00502FB0
#define kdlsym_addr_sceRegMgrSetBin                        0x00502F00
#define kdlsym_addr_sceRegMgrGetStr                        0x00502E30
#define kdlsym_addr_sceRegMgrSetStr                        0x00502C70

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

#define ssc_enable_fakepkg_patch                           0x0

// SceShellCore patches - use free prefix instead fake
#define ssc_fake_to_free_patch                             0x0

// SceShellCore patches - enable remote pkg installer
#define ssc_enable_data_mount_patch                        0x0

// SceShellCore patches - enable VR without spoof
#define ssc_enable_vr_patch                                0x0

// SceShellCore patches - enable official external HDD support (Support added in 4.50)
#define ssc_external_hdd_pkg_installer_patch               0x0
#define ssc_external_hdd_version_patchA                    0x0
#define ssc_external_hdd_version_patchB                    0x0

// SceShellUI patches - debug patches
#define ssu_sceSblRcMgrIsAllowDebugMenuForSettings_patch   0x0
#define ssu_sceSblRcMgrIsStoreMode_patch                   0x0

// SceShellUI - remote play related patching
#define ssu_CreateUserForIDU_patch                         0x0
#define ssu_remote_play_menu_patch                         0x0

// SceRemotePlay - enabler patches
#define srp_enabler_patchA                                 0x0
#define srp_enabler_patchB                                 0x0

#endif
