#pragma once
#include <Boot/Config.hpp>

#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_501
/*
These are the required functions in order for the Oni Framework to operate properly
These are all offsets into the base of the kernel. They expect all standard FreeBSD 9 prototypes

The reason we do not hardcode offsets here, is due to the different platforms that are supported, and
for the platforms that do enable kernel ASLR (Address Space Layout Randomization?)
*/

#define kdlsym_addr__mtx_lock_flags                        0x00401900
#define kdlsym_addr__mtx_lock_sleep                        0x004019A0
#define kdlsym_addr__mtx_lock_spin_flags                   0x00401D30
#define kdlsym_addr__mtx_unlock_flags                      0x00401BD0
#define kdlsym_addr__mtx_unlock_sleep                      0x00401CD0
#define kdlsym_addr__mtx_unlock_spin_flags                 0x00401EF0
#define kdlsym_addr__sceSblAuthMgrGetSelfInfo              0x0063C960
#define kdlsym_addr__sceSblAuthMgrSmStart                  0x00641500
#define kdlsym_addr__sx_init_flags                         0x000F5AA0
#define kdlsym_addr__sx_slock                              0x000F5B20
#define kdlsym_addr__sx_sunlock                            0x000F5E00
#define kdlsym_addr__sx_xlock                              0x000F5D00
#define kdlsym_addr__sx_xunlock                            0x000F5EC0
#define kdlsym_addr__thread_lock_flags                     0x00402050
#define kdlsym_addr__vm_map_lock_read                      0x0019F030
#define kdlsym_addr__vm_map_unlock_read                    0x0019F080
#define kdlsym_addr_AesCbcCfb128Decrypt                    0x003A2A30
#define kdlsym_addr_AesCbcCfb128Encrypt                    0x003A2800
#define kdlsym_addr_allproc                                0x02382FF8
#define kdlsym_addr_allproc_lock                           0x02382F98
#define kdlsym_addr_avcontrol_sleep                        0x006EABD0
#define kdlsym_addr_cloneuio                               0x002A8010
#define kdlsym_addr_console_cdev                           0x01AC5158
#define kdlsym_addr_console_write                          0x000ECB40
#define kdlsym_addr_contigfree                             0x000F1EE0
#define kdlsym_addr_contigmalloc                           0x000F1B10
#define kdlsym_addr_copyin                                 0x001EA600
#define kdlsym_addr_copyinstr                              0x001EAA30
#define kdlsym_addr_copyout                                0x001EA520
#define kdlsym_addr_critical_enter                         0x0028E4D0
#define kdlsym_addr_critical_exit                          0x0028E4E0
#define kdlsym_addr_deci_tty_write                         0x0049CB20
#define kdlsym_addr_destroy_dev                            0x001B9C40
#define kdlsym_addr_dmem_start_app_process                 0x002468E0
#define kdlsym_addr_dynlib_do_dlsym                        0x002AF7B0
#define kdlsym_addr_dynlib_find_obj_by_handle              0x002B0B40
#define kdlsym_addr_eventhandler_deregister                0x001EC680
#define kdlsym_addr_eventhandler_find_list                 0x001EC870
#define kdlsym_addr_eventhandler_register                  0x001EC2F0
#define kdlsym_addr_exec_new_vmspace                       0x0038A940
#define kdlsym_addr_faultin                                0x00006DD0
#define kdlsym_addr_fget_unlocked                          0x000C3530
#define kdlsym_addr_fpu_kern_ctx                           0x0274C040
#define kdlsym_addr_fpu_kern_enter                         0x001BFE80
#define kdlsym_addr_fpu_kern_leave                         0x001BFF80
#define kdlsym_addr_free                                   0x0010E350
#define kdlsym_addr_gdt                                    0x01CB90F0
#define kdlsym_addr_gpu_va_page_list                       0x0271E208
#define kdlsym_addr_icc_nvs_read                           0x00395460
#define kdlsym_addr_kern_close                             0x000C0F40
#define kdlsym_addr_kern_ioctl                             0x00153880
#define kdlsym_addr_kern_mkdirat                           0x00340800
#define kdlsym_addr_kern_open                              0x0072AB50
#define kdlsym_addr_kern_openat                            0x0033B640
#define kdlsym_addr_kern_readv                             0x00152A10
#define kdlsym_addr_kern_reboot                            0x0010D280
#define kdlsym_addr_kern_sysents                           0x0107C610
#define kdlsym_addr_kern_thr_create                        0x001BE0E0
#define kdlsym_addr_kernel_map                             0x01AC60E0
#define kdlsym_addr_kernel_mount                           0x001E1810
#define kdlsym_addr_killproc                               0x000D4240
#define kdlsym_addr_kmem_alloc                             0x000FCB70
#define kdlsym_addr_kmem_free                              0x000FCD40
#define kdlsym_addr_kproc_create                           0x00137CE0
#define kdlsym_addr_kproc_exit                             0x00137F50
#define kdlsym_addr_kthread_add                            0x00138250
#define kdlsym_addr_kthread_exit                           0x00138530
#define kdlsym_addr_M_IOV                                  0x014B5E80
#define kdlsym_addr_M_LINKER                               0x019F2E10
#define kdlsym_addr_M_MOUNT                                0x019BF300
#define kdlsym_addr_M_TEMP                                 0x014B4110
#define kdlsym_addr_make_dev_p                             0x001B9700
#define kdlsym_addr_malloc                                 0x0010E140
#define kdlsym_addr_memcmp                                 0x00050AC0
#define kdlsym_addr_memcpy                                 0x001EA420
#define kdlsym_addr_memmove                                0x00073BA0
#define kdlsym_addr_memset                                 0x003201F0
#define kdlsym_addr_mini_syscore_self_binary               0x014C9D48
#define kdlsym_addr_mount_arg                              0x001E1590
#define kdlsym_addr_mount_argb                             0x001DFB40
#define kdlsym_addr_mount_argf                             0x001E1670
#define kdlsym_addr_mtx_destroy                            0x00402420
#define kdlsym_addr_mtx_init                               0x004023B0
#define kdlsym_addr_mtx_lock_sleep                         0x004019a0
#define kdlsym_addr_mtx_unlock_sleep                       0x00401bd0
#define kdlsym_addr_name_to_nids                           0x002AFA90
#define kdlsym_addr_pause                                  0x003FB550
#define kdlsym_addr_pfind                                  0x00403110
#define kdlsym_addr_pmap_activate                          0x002EAC40
#define kdlsym_addr_printf                                 0x00435C70
#define kdlsym_addr_prison0                                0x010986A0
#define kdlsym_addr_proc0                                  0x01AA4600
#define kdlsym_addr_proc_reparent                          0x00035330
#define kdlsym_addr_proc_rwmem                             0x0030CDC0
#define kdlsym_addr_realloc                                0x0010E480
#define kdlsym_addr_rootvnode                              0x022C19F0
#define kdlsym_addr_RsaesPkcs1v15Dec2048CRT                0x001FD6C0
#define kdlsym_addr_sbl_eap_internal_partition_key         0x02790C90
#define kdlsym_addr_sbl_keymgr_buf_gva                     0x02748800
#define kdlsym_addr_sbl_keymgr_buf_va                      0x02748000
#define kdlsym_addr_sbl_keymgr_key_rbtree                  0x02744558
#define kdlsym_addr_sbl_keymgr_key_slots                   0x02744548
#define kdlsym_addr_sbl_pfs_sx                             0x0271E5D8
#define kdlsym_addr_sbl_drv_msg_mtx                        0x0271E210
#define kdlsym_addr_sceSblACMgrGetPathId                   0x000117E0
#define kdlsym_addr_sceSblAuthMgrIsLoadable2               0x0063C110
#define kdlsym_addr_sceSblAuthMgrSmVerifyHeader            0x00642760
#define kdlsym_addr_sceSblAuthMgrVerifyHeader              0x0063C170
#define kdlsym_addr_sceSblDriverSendMsg                    0x0061D410
#define kdlsym_addr_sceSblGetEAPInternalPartitionKey       0x00625300
#define kdlsym_addr_sceSblKeymgrClearKey                   0x0062D730
#define kdlsym_addr_sceSblKeymgrSetKeyForPfs               0x0062D3A0
#define kdlsym_addr_sceSblKeymgrSetKeyStorage              0x00623BE0
#define kdlsym_addr_sceSblKeymgrSmCallfunc                 0x0062DEC0
#define kdlsym_addr_sceSblPfsSetKeys                       0x0061EBC0
#define kdlsym_addr_sceSblRngGetRandomNumber               0x00647670
#define kdlsym_addr_sceSblServiceMailbox                   0x00632160
#define kdlsym_addr_self_orbis_sysvec                      0x019BBCD0
#define kdlsym_addr_Sha256Hmac                             0x002D52E0
#define kdlsym_addr_snprintf                               0x00435F80
#define kdlsym_addr_spinlock_exit                          0x002349A0
#define kdlsym_addr_sprintf                                0x00435EB0
#define kdlsym_addr_sscanf                                 0x001757F0
#define kdlsym_addr_strcmp                                 0x001D0EC0
#define kdlsym_addr_strdup                                 0x001C1B20
#define kdlsym_addr_strlen                                 0x003B6DD0
#define kdlsym_addr_strncmp                                0x001B8ED0
#define kdlsym_addr_strstr                                 0x0017DEA0
#define kdlsym_addr_sys_accept                             0x00319DA0
#define kdlsym_addr_sys_bind                               0x00319450
#define kdlsym_addr_sys_close                              0x000C0F30
#define kdlsym_addr_sys_dup2                               0x000BF0D0
#define kdlsym_addr_sys_fstat                              0x000C14B0
#define kdlsym_addr_sys_getdents                           0x00340FC0
#define kdlsym_addr_sys_kill                               0x000D1A50
#define kdlsym_addr_sys_listen                             0x00319690
#define kdlsym_addr_sys_lseek                              0x0033D620
#define kdlsym_addr_sys_mkdir                              0x00340780
#define kdlsym_addr_sys_mlock                              0x0013E140
#define kdlsym_addr_sys_mlockall                           0x0013E1F0
#define kdlsym_addr_sys_mmap                               0x0013D120
#define kdlsym_addr_sys_munmap                             0x0013D890
#define kdlsym_addr_sys_nmount                             0x001DE1D0
#define kdlsym_addr_sys_open                               0x0033B5C0
#define kdlsym_addr_sys_ptrace                             0x0030D250
#define kdlsym_addr_sys_read                               0x001529A0
#define kdlsym_addr_sys_recvfrom                           0x0031B090
#define kdlsym_addr_sys_rmdir                              0x00340B00
#define kdlsym_addr_sys_sendto                             0x0031A940
#define kdlsym_addr_sys_setuid                             0x00054950
#define kdlsym_addr_sys_shutdown                           0x0031B2D0
#define kdlsym_addr_sys_socket                             0x00318B10
#define kdlsym_addr_sys_stat                               0x0033DC10
#define kdlsym_addr_sys_unlink                             0x0033D000
#define kdlsym_addr_sys_unmount                            0x001DFB60
#define kdlsym_addr_sys_wait4                              0x00035470
#define kdlsym_addr_sys_write                              0x00152EB0
#define kdlsym_addr_trap_fatal                             0x00171470
#define kdlsym_addr_utilUSleep                             0x00658850
#define kdlsym_addr_vm_fault_disable_pagefaults            0x002A6950
#define kdlsym_addr_vm_fault_enable_pagefaults             0x002A6C50
#define kdlsym_addr_vm_map_lookup_entry                    0x0019F650
#define kdlsym_addr_vmspace_acquire_ref                    0x0019EE80
#define kdlsym_addr_vmspace_alloc                          0x0019EA10
#define kdlsym_addr_vmspace_free                           0x0019ECB0
#define kdlsym_addr_vn_fullpath                            0x000A1220
#define kdlsym_addr_vsnprintf                              0x00436020
#define kdlsym_addr_wakeup                                 0x003FB570
#define kdlsym_addr_Xfast_syscall                          0x000001C0
#define kdlsym_addr_target_id                              0x0
// Kernel Hooks
#define kdlsym_addr_printf_hook                            0x019FC168

// FakeSelf hooks
#define kdlsym_addr_sceSblAuthMgrIsLoadable__sceSblACMgrGetPathId_hook        0x0063DE7D
#define kdlsym_addr_sceSblAuthMgrIsLoadable2_hook                             0x0063DFC1
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook 0x00642DAB
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook   0x006439C2
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookA                           0x0063E71C
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookB                           0x0063F338

// FakePkg hooks
#define kdlsym_addr_sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg_hook       0x00623C85
#define kdlsym_addr_sceSblKeymgrInvalidateKey__sx_xlock_hook                  0x0062E58D
#define kdlsym_addr_npdrm_decrypt_isolated_rif__sceSblKeymgrSmCallfunc_hook   0x0064C340
#define kdlsym_addr_npdrm_decrypt_rif_new__sceSblKeymgrSmCallfunc_hook        0x0064D11F
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookA                          0x006AA6F5
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookB                          0x006AA924

// sceRegMgr
#define kdlsym_addr_sceRegMgrGetInt                        0x004F9A80
#define kdlsym_addr_sceRegMgrSetInt                        0x004F8940
#define kdlsym_addr_sceRegMgrGetBin                        0x004FA300
#define kdlsym_addr_sceRegMgrSetBin                        0x004FA250
#define kdlsym_addr_sceRegMgrGetStr                        0x004FA180
#define kdlsym_addr_sceRegMgrSetStr                        0x004F9FC0

// SceShellCore patches - call sceKernelIsGenuineCEX
#define ssc_sceKernelIsGenuineCEX_patchA                   0x0016D05B
#define ssc_sceKernelIsGenuineCEX_patchB                   0x0079941B
#define ssc_sceKernelIsGenuineCEX_patchC                   0x007E5623
#define ssc_sceKernelIsGenuineCEX_patchD                   0x00946D5B

// SceShellCore patches - call nidf_libSceDipsw
#define ssc_nidf_libSceDipsw_patchA                        0x0016D087
#define ssc_nidf_libSceDipsw_patchB                        0x0023747B
#define ssc_nidf_libSceDipsw_patchC                        0x00799447
#define ssc_nidf_libSceDipsw_patchD                        0x00946D87

#define ssc_enable_fakepkg_patch                           0x003E0602

// SceShellCore patches - use free prefix instead fake
#define ssc_fake_to_free_patch                             0x00EA7A47

// SceShellCore patches - enable remote pkg installer
#define ssc_enable_data_mount_patch                        0x00319A53

// SceShellCore patches - enable VR without spoof
#define ssc_enable_vr_patch                                0x00C788D0

// SceShellCore patches - enable official external HDD support (Support added in 4.50)
#define ssc_external_hdd_pkg_installer_patch               0x00930D81
#define ssc_external_hdd_version_patchA                    0x005937DD
#define ssc_external_hdd_version_patchB                    0x00130A71

// SceShellUI patches - debug patches
#define ssu_sceSblRcMgrIsAllowDebugMenuForSettings_patch   0x0001BD90
#define ssu_sceSblRcMgrIsStoreMode_patch                   0x0001C090

// SceShellUI - remote play related patching
#define ssu_CreateUserForIDU_patch                         0x001A8C50
#define ssu_remote_play_menu_patch                         0x00EE606E

// SceRemotePlay - enabler patches
#define srp_enabler_patchA                                 0x0003C33F
#define srp_enabler_patchB                                 0x0003C35A

#endif
