#pragma once
#include <Boot/Config.hpp>

// Offsets ported by Crazyvoid - Updated by SiSTRo

#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_455
/*
These are the required functions in order for the Oni Framework to operate properly
These are all offsets into the base of the kernel. They expect all standard FreeBSD 9 prototypes

The reason we do not hardcode offsets here, is due to the different platforms that are supported, and
for the platforms that do enable kernel ASLR (Address Space Layout Randomization?)
*/

#define kdlsym_addr__mtx_lock_flags                        0x0030D6A0
#define kdlsym_addr__mtx_lock_sleep                        0x0030D710
#define kdlsym_addr__mtx_lock_spin_flags                   0x0030DA70
#define kdlsym_addr__mtx_unlock_flags                      0x0030D940
#define kdlsym_addr__mtx_unlock_sleep                      0x0030DA10
#define kdlsym_addr__mtx_unlock_spin_flags                 0x0030DC30
#define kdlsym_addr__sceSblAuthMgrGetSelfInfo              0x00626490
#define kdlsym_addr__sceSblAuthMgrSmStart                  0x00622020
#define kdlsym_addr__sx_init_flags                         0x0038F900
#define kdlsym_addr__sx_slock                              0x0038F980
#define kdlsym_addr__sx_sunlock                            0x0038FB00
#define kdlsym_addr__sx_xlock                              0x0038FA30
#define kdlsym_addr__sx_xunlock                            0x0038FBC0
#define kdlsym_addr__thread_lock_flags                     0x0030DD70
#define kdlsym_addr__vm_map_lock_read                      0x003920B0
#define kdlsym_addr__vm_map_unlock_read                    0x00392100
#define kdlsym_addr_AesCbcCfb128Decrypt                    0x0017A6F0
#define kdlsym_addr_AesCbcCfb128Encrypt                    0x0017A4C0
#define kdlsym_addr_allproc                                0x01AD7718
#define kdlsym_addr_allproc_lock                           0x01AD76B8
#define kdlsym_addr_avcontrol_sleep                        0x006C42E0
#define kdlsym_addr_cloneuio                               0x00040420
#define kdlsym_addr_console_cdev                           0x01A6CB08
#define kdlsym_addr_console_write                          0x00030630
#define kdlsym_addr_contigfree                             0x00250620
#define kdlsym_addr_contigmalloc                           0x002502C0
#define kdlsym_addr_copyin                                 0x0014A890
#define kdlsym_addr_copyinstr                              0x0014AD00
#define kdlsym_addr_copyout                                0x0014A7B0
#define kdlsym_addr_critical_enter                         0x0023D560
#define kdlsym_addr_critical_exit                          0x0023D570
#define kdlsym_addr_deci_tty_write                         0x00474E40
#define kdlsym_addr_destroy_dev                            0x0036EAD0
//#define kdlsym_addr_dmem_start_app_process                 0x0
#define kdlsym_addr_dynlib_do_dlsym                        0x00066A20
#define kdlsym_addr_dynlib_find_obj_by_handle              0x00067AD0
#define kdlsym_addr_eventhandler_deregister                0x003C9B60
#define kdlsym_addr_eventhandler_find_list                 0x003C9D50
#define kdlsym_addr_eventhandler_register                  0x003C97F0
#define kdlsym_addr_exec_new_vmspace                       0x002E8850
#define kdlsym_addr_faultin                                0x0030EE10
#define kdlsym_addr_fget_unlocked                          0x0042D160
#define kdlsym_addr_fpu_kern_ctx                           0x0251CCC0
#define kdlsym_addr_fpu_kern_enter                         0x00059580
#define kdlsym_addr_fpu_kern_leave                         0x00059680
#define kdlsym_addr_free                                   0x003F7930
#define kdlsym_addr_gdt                                    0x01B55AD0
#define kdlsym_addr_gpu_va_page_list                       0x02519DD0
#define kdlsym_addr_icc_nvs_read                           0x0001B850
#define kdlsym_addr_kern_close                             0x0042AC10
#define kdlsym_addr_kern_ioctl                             0x0005FA20
#define kdlsym_addr_kern_mkdirat                           0x00444E60
#define kdlsym_addr_kern_open                              0x0043FD00
#define kdlsym_addr_kern_openat                            0x0043FD60
#define kdlsym_addr_kern_readv                             0x0005ED30
#define kdlsym_addr_kern_reboot                            0x000998A0
#define kdlsym_addr_kern_sysents                           0x0102B690
#define kdlsym_addr_kern_thr_create                        0x002ECCD0
#define kdlsym_addr_kernel_map                             0x01B31218
#define kdlsym_addr_kernel_mount                           0x000DD1C0
#define kdlsym_addr_killproc                               0x00026230
#define kdlsym_addr_kmem_alloc                             0x0016ECD0
#define kdlsym_addr_kmem_free                              0x0016EEA0
#define kdlsym_addr_kproc_create                           0x00464700
#define kdlsym_addr_kproc_exit                             0x00464980
#define kdlsym_addr_kthread_add                            0x00464C90
#define kdlsym_addr_kthread_exit                           0x00464F60
#define kdlsym_addr_M_IOV                                  0x014447F0
#define kdlsym_addr_M_LINKER                               0x0144A320
#define kdlsym_addr_M_MOUNT                                0x0144F2A0
#define kdlsym_addr_M_TEMP                                 0x01993B30
#define kdlsym_addr_make_dev_p                             0x0036E5A0
#define kdlsym_addr_malloc                                 0x003F7750
#define kdlsym_addr_memcmp                                 0x00242A60
#define kdlsym_addr_memcpy                                 0x0014A6B0
#define kdlsym_addr_memmove                                0x002EE740
#define kdlsym_addr_memset                                 0x00302BD0
#define kdlsym_addr_mini_syscore_self_binary               0x01471468
#define kdlsym_addr_mount_arg                              0x000DCF40
#define kdlsym_addr_mount_argb                             0x000DB4E0
#define kdlsym_addr_mount_argf                             0x000DD020
#define kdlsym_addr_mtx_destroy                            0x0030E130
#define kdlsym_addr_mtx_init                               0x0030E0C0
#define kdlsym_addr_mtx_lock_sleep                         0x0030D710
#define kdlsym_addr_mtx_unlock_sleep                       0x0030DA10
#define kdlsym_addr_name_to_nids                           0x00069730
#define kdlsym_addr_pause                                  0x00261120
#define kdlsym_addr_pfind                                  0x00079780
#define kdlsym_addr_pmap_activate                          0x00427530
#define kdlsym_addr_printf                                 0x00017F30
#define kdlsym_addr_prison0                                0x010399B0
#define kdlsym_addr_proc0                                  0x01B2FC00
#define kdlsym_addr_proc_reparent                          0x0015C3C0
#define kdlsym_addr_proc_rwmem                             0x0017CB70
#define kdlsym_addr_realloc                                0x003F7A60
#define kdlsym_addr_rootvnode                              0x021AFA30
#define kdlsym_addr_RsaesPkcs1v15Dec2048CRT                0x003EF200
#define kdlsym_addr_sbl_eap_internal_partition_key         0x0258CCD0
#define kdlsym_addr_sbl_keymgr_buf_gva                     0x02538200
#define kdlsym_addr_sbl_keymgr_buf_va                      0x02538000
#define kdlsym_addr_sbl_keymgr_key_rbtree                  0x02534DE0
#define kdlsym_addr_sbl_keymgr_key_slots                   0x02534DD0
#define kdlsym_addr_sbl_pfs_sx                             0x0
#define kdlsym_addr_sbl_drv_msg_mtx                        0x02519DD8
#define kdlsym_addr_sceSblACMgrGetPathId                   0x0016A5E0
#define kdlsym_addr_sceSblAuthMgrIsLoadable2               0x00625C50
#define kdlsym_addr_sceSblAuthMgrSmVerifyHeader            0x00623250
#define kdlsym_addr_sceSblAuthMgrVerifyHeader              0x00625CB0
#define kdlsym_addr_sceSblDriverSendMsg                    0x00601670
#define kdlsym_addr_sceSblGetEAPInternalPartitionKey       0x00609D20
#define kdlsym_addr_sceSblKeymgrClearKey                   0x0060DF40
#define kdlsym_addr_sceSblKeymgrSetKeyForPfs               0x0060DD70
#define kdlsym_addr_sceSblKeymgrSetKeyStorage              0x0
#define kdlsym_addr_sceSblKeymgrSmCallfunc                 0x0060E680
#define kdlsym_addr_sceSblPfsSetKeys                       0x00606E00
#define kdlsym_addr_sceSblRngGetRandomNumber               0x006298C0
#define kdlsym_addr_sceSblServiceMailbox                   0x006146C0
#define kdlsym_addr_self_orbis_sysvec                      0x01460610
#define kdlsym_addr_Sha256Hmac                             0x002D5C50
#define kdlsym_addr_snprintf                               0x00018230
#define kdlsym_addr_spinlock_exit                          0x002828D0
#define kdlsym_addr_sprintf                                0x00018160
#define kdlsym_addr_sscanf                                 0x00302100
#define kdlsym_addr_strcmp                                 0x001DAEF0
#define kdlsym_addr_strdup                                 0x00452A30
#define kdlsym_addr_strlen                                 0x003514F0
#define kdlsym_addr_strncmp                                0x003DB670
#define kdlsym_addr_strstr                                 0x00261710
#define kdlsym_addr_sys_accept                             0x003EBEC0
#define kdlsym_addr_sys_bind                               0x003EB550
#define kdlsym_addr_sys_close                              0x0042AC00
#define kdlsym_addr_sys_dup2                               0x00428E50
#define kdlsym_addr_sys_fstat                              0x0042B1B0
#define kdlsym_addr_sys_getdents                           0x00445600
#define kdlsym_addr_sys_kill                               0x00023CC0
#define kdlsym_addr_sys_listen                             0x003EB760
#define kdlsym_addr_sys_lseek                              0x00441D20
#define kdlsym_addr_sys_mkdir                              0x00444DE0
#define kdlsym_addr_sys_mlock                              0x00142A40
#define kdlsym_addr_sys_mlockall                           0x00142AF0
#define kdlsym_addr_sys_mmap                               0x001419F0
#define kdlsym_addr_sys_munmap                             0x00142190
#define kdlsym_addr_sys_nmount                             0x000D9B40
#define kdlsym_addr_sys_open                               0x0043FCE0
#define kdlsym_addr_sys_ptrace                             0x0017D000
#define kdlsym_addr_sys_read                               0x0005EC50
#define kdlsym_addr_sys_recvfrom                           0x003ED080
#define kdlsym_addr_sys_rmdir                              0x00445160
#define kdlsym_addr_sys_sendto                             0x003EC9C0
#define kdlsym_addr_sys_setuid                             0x00114450
#define kdlsym_addr_sys_shutdown                           0x003ED250
#define kdlsym_addr_sys_socket                             0x003EAC20
#define kdlsym_addr_sys_stat                               0x00442320
#define kdlsym_addr_sys_unlink                             0x00441730
#define kdlsym_addr_sys_unmount                            0x000DB500
#define kdlsym_addr_sys_wait4                              0x0015C500
#define kdlsym_addr_sys_write                              0x0005F1A0
#define kdlsym_addr_trap_fatal                             0x003DBD20
#define kdlsym_addr_utilUSleep                             0x0065F290
#define kdlsym_addr_vm_fault_disable_pagefaults            0x002A19F0
#define kdlsym_addr_vm_fault_enable_pagefaults             0x002A1A20
#define kdlsym_addr_vm_map_lookup_entry                    0x00392C70
#define kdlsym_addr_vmspace_acquire_ref                    0x00391EE0
#define kdlsym_addr_vmspace_alloc                          0x00391A70
#define kdlsym_addr_vmspace_free                           0x00391D10
#define kdlsym_addr_vn_fullpath                            0x002FB5B0
#define kdlsym_addr_vsnprintf                              0x000182D0
#define kdlsym_addr_wakeup                                 0x00261140
#define kdlsym_addr_Xfast_syscall                          0x003095D0

// Kernel Hooks
#define kdlsym_addr_printf_hook                            0x0199BDC8

// FakeSelf hooks
#define kdlsym_addr_sceSblAuthMgrIsLoadable__sceSblACMgrGetPathId_hook        0x0061F0FC
#define kdlsym_addr_sceSblAuthMgrIsLoadable2_hook                             0x0061F24F
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook 0x006238BA
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook   0x006244E1
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookA                           0x0061F976
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookB                           0x00620599

// FakePkg hooks
#define kdlsym_addr_sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg_hook       0x0
#define kdlsym_addr_sceSblKeymgrInvalidateKey__sx_xlock_hook                  0x0060ED5A
#define kdlsym_addr_npdrm_decrypt_isolated_rif__sceSblKeymgrSmCallfunc_hook   0x0062DF00
#define kdlsym_addr_npdrm_decrypt_rif_new__sceSblKeymgrSmCallfunc_hook        0x0062ECDE
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookA                          0x0
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookB                          0x0

// sceRegMgr
#define kdlsym_addr_sceRegMgrGetInt                        0x004D7DB0
#define kdlsym_addr_sceRegMgrSetInt                        0x004D6F00
#define kdlsym_addr_sceRegMgrGetBin                        0x004D8710
#define kdlsym_addr_sceRegMgrSetBin                        0x004D8640
#define kdlsym_addr_sceRegMgrGetStr                        0x004D8520
#define kdlsym_addr_sceRegMgrSetStr                        0x004D8170

// SceShellCore patches - call sceKernelIsGenuineCEX
#define ssc_sceKernelIsGenuineCEX_patchA                   0x001486BB
#define ssc_sceKernelIsGenuineCEX_patchB                   0x006E523B
#define ssc_sceKernelIsGenuineCEX_patchC                   0x00852C6B
#define ssc_sceKernelIsGenuineCEX_patchD                   0x0

// SceShellCore patches - call nidf_libSceDipsw
#define ssc_nidf_libSceDipsw_patchA                        0x001486E7
#define ssc_nidf_libSceDipsw_patchB                        0x006E5267
#define ssc_nidf_libSceDipsw_patchC                        0x00852C97
#define ssc_nidf_libSceDipsw_patchD                        0x0

#define ssc_enable_fakepkg_patch                           0x00379573

// SceShellCore patches - use free prefix instead fake
#define ssc_fake_to_free_patch                             0x00D40F28

// SceShellCore patches - enable VR without spoof
#define ssc_enable_vr_patch                                0x00B2D9B0

// SceShellCore patches - enable official external HDD support (Support added in 4.50)
#define ssc_external_hdd_pkg_installer_patch               0x00844821
#define ssc_external_hdd_7xx_patch                         0x004F9F5D

// SceShellUI patches - debug patches
#define ssu_sceSblRcMgrIsAllowDebugMenuForSettings_patch   0x00019EA0
#define ssu_sceSblRcMgrIsStoreMode_patch                   0x0001A1A0

// SceShellUI - remote play related patching
#define ssu_CreateUserForIDU_patch                         0x00195820
#define ssu_remote_play_menu_patch                         0x01296867

// SceRemotePlay - enabler patches
#define srp_enabler_patchA                                 0x0003C882
#define srp_enabler_patchB                                 0x0003C89D

#endif
