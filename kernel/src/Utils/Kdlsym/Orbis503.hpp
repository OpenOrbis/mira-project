#pragma once
#include <Boot/Config.hpp>

#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_503
/*
These are the required functions in order for the Oni Framework to operate properly
These are all offsets into the base of the kernel. They expect all standard FreeBSD 9 prototypes

The reason we do not hardcode offsets here, is due to the different platforms that are supported, and
for the platforms that do enable kernel ASLR (Address Space Layout Randomization?)
*/

#define kdlsym_addr__mtx_lock_flags                        0x00401C90
#define kdlsym_addr__mtx_lock_sleep                        0x00401D30
#define kdlsym_addr__mtx_lock_spin_flags                   0x004020C0
#define kdlsym_addr__mtx_unlock_flags                      0x00401F60
#define kdlsym_addr__mtx_unlock_sleep                      0x00402060
#define kdlsym_addr__mtx_unlock_spin_flags                 0x00402280
#define kdlsym_addr__sceSblAuthMgrGetSelfInfo              0x0063CD00
#define kdlsym_addr__sceSblAuthMgrSmStart                  0x006418A0
#define kdlsym_addr__sx_init_flags                         0x000F5BB0
#define kdlsym_addr__sx_slock                              0x000F5C30
#define kdlsym_addr__sx_sunlock                            0x000F5F10
#define kdlsym_addr__sx_xlock                              0x000F5E10
#define kdlsym_addr__sx_xunlock                            0x000F5FD0
#define kdlsym_addr__thread_lock_flags                     0x004023E0
#define kdlsym_addr__vm_map_lock_read                      0x0019F140
#define kdlsym_addr__vm_map_unlock_read                    0x0019F190
#define kdlsym_addr_AesCbcCfb128Decrypt                    0x003A2DC0
#define kdlsym_addr_AesCbcCfb128Encrypt                    0x003A2B90
#define kdlsym_addr_allproc                                0x02382FF8
#define kdlsym_addr_allproc_lock                           0x02382F98
#define kdlsym_addr_avcontrol_sleep                        0x006EAF70
#define kdlsym_addr_cloneuio                               0x002A82E0
#define kdlsym_addr_console_cdev                           0x01AC5158
#define kdlsym_addr_console_write                          0x000ECAC0
#define kdlsym_addr_contigfree                             0x000F1FF0
#define kdlsym_addr_contigmalloc                           0x000F1C20
#define kdlsym_addr_copyin                                 0x001EA710
#define kdlsym_addr_copyinstr                              0x001EAB40
#define kdlsym_addr_copyout                                0x001EA630
#define kdlsym_addr_critical_enter                         0x0028E7A0
#define kdlsym_addr_critical_exit                          0x0028E7B0
#define kdlsym_addr_deci_tty_write                         0x0049CEB0
#define kdlsym_addr_destroy_dev                            0x001B9D50
#define kdlsym_addr_dmem_start_app_process                 0x002469F0
#define kdlsym_addr_dynlib_do_dlsym                        0x002AFA80
#define kdlsym_addr_dynlib_find_obj_by_handle              0x002B0E10
#define kdlsym_addr_eventhandler_deregister                0x001EC790
#define kdlsym_addr_eventhandler_find_list                 0x001EC980
#define kdlsym_addr_eventhandler_register                  0x001EC400
#define kdlsym_addr_exec_new_vmspace                       0x0038ACD0
#define kdlsym_addr_faultin                                0x00006DD0
#define kdlsym_addr_fget_unlocked                          0x000C34B0
#define kdlsym_addr_fpu_kern_ctx                           0x0274C040
#define kdlsym_addr_fpu_kern_enter                         0x001BFF90
#define kdlsym_addr_fpu_kern_leave                         0x001C0090
#define kdlsym_addr_free                                   0x0010E460
#define kdlsym_addr_gdt                                    0x01CB90F0
#define kdlsym_addr_gpu_va_page_list                       0x0271E208
#define kdlsym_addr_icc_nvs_read                           0x003957F0
#define kdlsym_addr_kern_close                             0x000C0EC0
#define kdlsym_addr_kern_ioctl                             0x00153990
#define kdlsym_addr_kern_mkdirat                           0x00340B90
#define kdlsym_addr_kern_open                              0x0033B970
#define kdlsym_addr_kern_openat                            0x0033B9D0
#define kdlsym_addr_kern_readv                             0x00153248
#define kdlsym_addr_kern_reboot                            0x0010D390
#define kdlsym_addr_kern_sysents                           0x0107C610
#define kdlsym_addr_kern_thr_create                        0x001BE1F0
#define kdlsym_addr_kernel_map                             0x01AC60E0
#define kdlsym_addr_kernel_mount                           0x001E1920
#define kdlsym_addr_killproc                               0x000D41C0
#define kdlsym_addr_kmem_alloc                             0x000FCC80
#define kdlsym_addr_kmem_free                              0x000FCE50
#define kdlsym_addr_kproc_create                           0x00137DF0
#define kdlsym_addr_kproc_exit                             0x00138060
#define kdlsym_addr_kthread_add                            0x00138360
#define kdlsym_addr_kthread_exit                           0x00138640
#define kdlsym_addr_M_IOV                                  0x014B5E80
#define kdlsym_addr_M_LINKER                               0x019F2E10
#define kdlsym_addr_M_MOUNT                                0x019BF300
#define kdlsym_addr_M_TEMP                                 0x014B4110
#define kdlsym_addr_make_dev_p                             0x001B9810
#define kdlsym_addr_malloc                                 0x0010E250
#define kdlsym_addr_memcmp                                 0x00050AC0
#define kdlsym_addr_memcpy                                 0x001EA530
#define kdlsym_addr_memmove                                0x00073BA0
#define kdlsym_addr_memset                                 0x00320580
#define kdlsym_addr_mini_syscore_self_binary               0x014C9D48
#define kdlsym_addr_mount_arg                              0x001E16A0
#define kdlsym_addr_mount_argb                             0x001DFC50
#define kdlsym_addr_mount_argf                             0x001E1780
#define kdlsym_addr_mtx_destroy                            0x004027B0
#define kdlsym_addr_mtx_init                               0x00402740
#define kdlsym_addr_mtx_lock_sleep                         0x00401D30
#define kdlsym_addr_mtx_unlock_sleep                       0x00402060
#define kdlsym_addr_name_to_nids                           0x002AFD60
#define kdlsym_addr_pause                                  0x003FB8E0
#define kdlsym_addr_pfind                                  0x004034A0
#define kdlsym_addr_pmap_activate                          0x002EAFD0
#define kdlsym_addr_printf                                 0x00436000
#define kdlsym_addr_prison0                                0x010986A0
#define kdlsym_addr_proc0                                  0x01AA4600
#define kdlsym_addr_proc_reparent                          0x00035330
#define kdlsym_addr_proc_rwmem                             0x0030D150
#define kdlsym_addr_realloc                                0x0010E590
#define kdlsym_addr_rootvnode                              0x022C1A70
#define kdlsym_addr_RsaesPkcs1v15Dec2048CRT                0x001FD7D0
#define kdlsym_addr_sbl_eap_internal_partition_key         0x02790C90
#define kdlsym_addr_sbl_keymgr_buf_gva                     0x02748800
#define kdlsym_addr_sbl_keymgr_buf_va                      0x02748000
#define kdlsym_addr_sbl_keymgr_key_rbtree                  0x02744558
#define kdlsym_addr_sbl_keymgr_key_slots                   0x02744548
#define kdlsym_addr_sbl_pfs_sx                             0x0271E5D8
#define kdlsym_addr_sbl_drv_msg_mtx                        0x0271E210
#define kdlsym_addr_sceSblACMgrGetPathId                   0x000117E0
#define kdlsym_addr_sceSblAuthMgrIsLoadable2               0x0063C4B0
#define kdlsym_addr_sceSblAuthMgrSmVerifyHeader            0x00642B00
#define kdlsym_addr_sceSblAuthMgrVerifyHeader              0x0063C510
#define kdlsym_addr_sceSblDriverSendMsg                    0x0061D7B0
#define kdlsym_addr_sceSblGetEAPInternalPartitionKey       0x006256A0
#define kdlsym_addr_sceSblKeymgrClearKey                   0x0062DAD0
#define kdlsym_addr_sceSblKeymgrSetKeyForPfs               0x0062D740
#define kdlsym_addr_sceSblKeymgrSetKeyStorage              0x00623F80
#define kdlsym_addr_sceSblKeymgrSmCallfunc                 0x0062E260
#define kdlsym_addr_sceSblPfsSetKeys                       0x0061EF60
#define kdlsym_addr_sceSblRngGetRandomNumber               0x00647A10
#define kdlsym_addr_sceSblServiceMailbox                   0x00632500
#define kdlsym_addr_self_orbis_sysvec                      0x019BBCD0
#define kdlsym_addr_Sha256Hmac                             0x002D55B0
#define kdlsym_addr_snprintf                               0x00436310
#define kdlsym_addr_spinlock_exit                          0x00234AB0
#define kdlsym_addr_sprintf                                0x00436240
#define kdlsym_addr_sscanf                                 0x00175900
#define kdlsym_addr_strcmp                                 0x001D0FD0
#define kdlsym_addr_strdup                                 0x001C1C30
#define kdlsym_addr_strlen                                 0x003B7160
#define kdlsym_addr_strncmp                                0x001B8FE0
#define kdlsym_addr_strstr                                 0x0017DFB0
#define kdlsym_addr_sys_accept                             0x0031A130
#define kdlsym_addr_sys_bind                               0x003197E0
#define kdlsym_addr_sys_close                              0x000C0EB0
#define kdlsym_addr_sys_dup2                               0x000BF050
#define kdlsym_addr_sys_fstat                              0x000C1430
#define kdlsym_addr_sys_getdents                           0x00341350
#define kdlsym_addr_sys_kill                               0x000D19D0
#define kdlsym_addr_sys_listen                             0x00319A20
#define kdlsym_addr_sys_lseek                              0x0033D9B0
#define kdlsym_addr_sys_mkdir                              0x00340B10
#define kdlsym_addr_sys_mlock                              0x0013E250
#define kdlsym_addr_sys_mlockall                           0x0013E300
#define kdlsym_addr_sys_mmap                               0x0013D230
#define kdlsym_addr_sys_munmap                             0x0013D9A0
#define kdlsym_addr_sys_nmount                             0x001DE2E0
#define kdlsym_addr_sys_open                               0x0033B950
#define kdlsym_addr_sys_ptrace                             0x0030D5E0
#define kdlsym_addr_sys_read                               0x00152AB0
#define kdlsym_addr_sys_recvfrom                           0x0031B420
#define kdlsym_addr_sys_rmdir                              0x00340E90
#define kdlsym_addr_sys_sendto                             0x0031ACD0
#define kdlsym_addr_sys_setuid                             0x00054950
#define kdlsym_addr_sys_shutdown                           0x0031B660
#define kdlsym_addr_sys_socket                             0x00318EA0
#define kdlsym_addr_sys_stat                               0x0033DFA0
#define kdlsym_addr_sys_unlink                             0x0033D390
#define kdlsym_addr_sys_unmount                            0x001DFC70
#define kdlsym_addr_sys_wait4                              0x00035470
#define kdlsym_addr_sys_write                              0x00152FC0
#define kdlsym_addr_trap_fatal                             0x00171580
#define kdlsym_addr_utilUSleep                             0x00658BF0
#define kdlsym_addr_vm_fault_disable_pagefaults            0x002A6C20
#define kdlsym_addr_vm_fault_enable_pagefaults             0x002A6C50
#define kdlsym_addr_vm_map_lookup_entry                    0x0019F760
#define kdlsym_addr_vmspace_acquire_ref                    0x0019EF90
#define kdlsym_addr_vmspace_alloc                          0x0019EB20
#define kdlsym_addr_vmspace_free                           0x0019EDC0
#define kdlsym_addr_vn_fullpath                            0x000A11A0
#define kdlsym_addr_vsnprintf                              0x004363B0
#define kdlsym_addr_wakeup                                 0x003FB940
#define kdlsym_addr_Xfast_syscall                          0x000001C0

// Kernel Hooks
#define kdlsym_addr_printf_hook                            0x019FC168

// FakeSelf hooks
#define kdlsym_addr_sceSblAuthMgrIsLoadable__sceSblACMgrGetPathId_hook        0x0063E21D
#define kdlsym_addr_sceSblAuthMgrIsLoadable2_hook                             0x0063E361
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook 0x0064314B
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook   0x00643D62
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookA                           0x0063EABC
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookB                           0x0063F6D8

// FakePkg hooks
#define kdlsym_addr_sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg_hook       0x00624025
#define kdlsym_addr_sceSblKeymgrInvalidateKey__sx_xlock_hook                  0x0062E92D
#define kdlsym_addr_npdrm_decrypt_isolated_rif__sceSblKeymgrSmCallfunc_hook   0x0064C6E0
#define kdlsym_addr_npdrm_decrypt_rif_new__sceSblKeymgrSmCallfunc_hook        0x0064D4BF
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookA                          0x006AAA95
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookB                          0x006AACC4

// sceRegMgr
#define kdlsym_addr_sceRegMgrGetInt                        0x004F9E10
#define kdlsym_addr_sceRegMgrSetInt                        0x004F8CD0
#define kdlsym_addr_sceRegMgrGetBin                        0x004FA690
#define kdlsym_addr_sceRegMgrSetBin                        0x004FA5E0
#define kdlsym_addr_sceRegMgrGetStr                        0x004FA510
#define kdlsym_addr_sceRegMgrSetStr                        0x004FA350

// SceShellCore patches - call sceKernelIsGenuineCEX
#define ssc_sceKernelIsGenuineCEX_patchA                   0x0016D05B
#define ssc_sceKernelIsGenuineCEX_patchB                   0x0079980B
#define ssc_sceKernelIsGenuineCEX_patchC                   0x007E5A13
#define ssc_sceKernelIsGenuineCEX_patchD                   0x0094715B

// SceShellCore patches - call nidf_libSceDipsw
#define ssc_nidf_libSceDipsw_patchA                        0x0016D087
#define ssc_nidf_libSceDipsw_patchB                        0x0023747B
#define ssc_nidf_libSceDipsw_patchC                        0x00799837
#define ssc_nidf_libSceDipsw_patchD                        0x00947187

#define ssc_enable_fakepkg_patch                           0x003E0602

// SceShellCore patches - use free prefix instead fake
#define ssc_fake_to_free_patch                             0x00EA9667

// SceShellCore patches - enable VR without spoof
#define ssc_enable_vr_patch                                0x00C79170

// SceShellCore patches - enable official external HDD support (Support added in 4.50)
#define ssc_external_hdd_pkg_installer_patch               0x009312A1
#define ssc_external_hdd_version_patch                     0x00593C7D

// SceShellUI patches - debug patches
#define ssu_sceSblRcMgrIsAllowDebugMenuForSettings_patch   0x0001BD90
#define ssu_sceSblRcMgrIsStoreMode_patch                   0x0001C090

// SceShellUI - remote play related patching
#define ssu_CreateUserForIDU_patch                         0x001A8FA0
#define ssu_remote_play_menu_patch                         0x00EE638E

// SceRemotePlay - enabler patches
#define srp_enabler_patchA                                 0x0003C33F
#define srp_enabler_patchB                                 0x0003C35A

#endif
