#pragma once
#include <Boot/Config.hpp>

#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_555
/*
These are the required functions in order for the Oni Framework to operate properly
These are all offsets into the base of the kernel. They expect all standard FreeBSD 9 prototypes

The reason we do not hardcode offsets here, is due to the different platforms that are supported, and
for the platforms that do enable kernel ASLR (Address Space Layout Randomization?)
*/

#define kdlsym_addr__mtx_lock_flags                        0x00123190
#define kdlsym_addr__mtx_lock_sleep                        0x00123230
#define kdlsym_addr__mtx_lock_spin_flags                   0x001235C0
#define kdlsym_addr__mtx_unlock_flags                      0x00123460
#define kdlsym_addr__mtx_unlock_sleep                      0x00123560
#define kdlsym_addr__mtx_unlock_spin_flags                 0x00123780
#define kdlsym_addr__sceSblAuthMgrGetSelfInfo              0x0065CD80
#define kdlsym_addr__sceSblAuthMgrSmStart                  0x00657120
#define kdlsym_addr__sx_init_flags                         0x00482680
#define kdlsym_addr__sx_slock                              0x00482700
#define kdlsym_addr__sx_sunlock                            0x004829E0
#define kdlsym_addr__sx_xlock                              0x004828E0
#define kdlsym_addr__sx_xunlock                            0x00482AA0
#define kdlsym_addr__thread_lock_flags                     0x001238E0
#define kdlsym_addr__vm_map_lock_read                      0x00029E40
#define kdlsym_addr__vm_map_unlock_read                    0x00029E90
#define kdlsym_addr_AesCbcCfb128Decrypt                    0x0045AF10
#define kdlsym_addr_AesCbcCfb128Encrypt                    0x0045ACE0
#define kdlsym_addr_allproc                                0x021910E8
#define kdlsym_addr_allproc_lock                           0x02191088
#define kdlsym_addr_avcontrol_sleep                        0x00702350
#define kdlsym_addr_cloneuio                               0x0026DE80
#define kdlsym_addr_console_cdev                           0x022618E8
#define kdlsym_addr_console_write                          0x001C3990
#define kdlsym_addr_contigfree                             0x002CA800
#define kdlsym_addr_contigmalloc                           0x002CA430
#define kdlsym_addr_copyin                                 0x00405E70
#define kdlsym_addr_copyinstr                              0x00406320
#define kdlsym_addr_copyout                                0x00405D80
#define kdlsym_addr_critical_enter                         0x001EE530
#define kdlsym_addr_critical_exit                          0x001EE540
#define kdlsym_addr_deci_tty_write                         0x004AA3C0
#define kdlsym_addr_destroy_dev                            0x002077F0
#define kdlsym_addr_dmem_start_app_process                 0x000947C0
#define kdlsym_addr_dynlib_do_dlsym                        0x003FD840
#define kdlsym_addr_dynlib_find_obj_by_handle              0x003FEBE0
#define kdlsym_addr_eventhandler_deregister                0x0022DA40
#define kdlsym_addr_eventhandler_find_list                 0x0022DC50
#define kdlsym_addr_eventhandler_register                  0x0022D6A0
#define kdlsym_addr_exec_new_vmspace                       0x0020D3B0
#define kdlsym_addr_faultin                                0x0015DE80
#define kdlsym_addr_fget_unlocked                          0x00229240
#define kdlsym_addr_fpu_kern_ctx                           0x0265B040
#define kdlsym_addr_fpu_kern_enter                         0x0022CC00
#define kdlsym_addr_fpu_kern_leave                         0x0022CD00
#define kdlsym_addr_free                                   0x00466FA0
#define kdlsym_addr_gdt                                    0x022A89A0
#define kdlsym_addr_global_settings_base                   0x022BFF50
#define kdlsym_addr_gpu_va_page_list                       0x026536C8
#define kdlsym_addr_icc_nvs_read                           0x00421870
#define kdlsym_addr_kern_close                             0x00226B30
#define kdlsym_addr_kern_ioctl                             0x00139010
#define kdlsym_addr_kern_mkdirat                           0x0030E770
#define kdlsym_addr_kern_open                              0x003092F0
#define kdlsym_addr_kern_openat                            0x00309350
#define kdlsym_addr_kern_readv                             0x001388C8
#define kdlsym_addr_kern_reboot                            0x00288BF0
#define kdlsym_addr_kern_sysents                           0x0111ACC0
#define kdlsym_addr_kern_thr_create                        0x000D4FA0
#define kdlsym_addr_kernel_map                             0x0217FBB0
#define kdlsym_addr_kernel_mount                           0x0019f3A0
#define kdlsym_addr_killproc                               0x0017C390
#define kdlsym_addr_kmem_alloc                             0x001422E0
#define kdlsym_addr_kmem_free                              0x001424B0
#define kdlsym_addr_kproc_create                           0x0032DAA0
#define kdlsym_addr_kproc_exit                             0x0032DD20
#define kdlsym_addr_kthread_add                            0x0032E020
#define kdlsym_addr_kthread_exit                           0x0032E300
#define kdlsym_addr_M_IOV                                  0x0154D160
#define kdlsym_addr_M_LINKER                               0x01551A10
#define kdlsym_addr_M_MOUNT                                0x0155CCE0
#define kdlsym_addr_M_TEMP                                 0x01A92FF0
#define kdlsym_addr_make_dev_p                             0x002072B0
#define kdlsym_addr_malloc                                 0x00466DA0
#define kdlsym_addr_memcmp                                 0x0005E270
#define kdlsym_addr_memcpy                                 0x00405C80
#define kdlsym_addr_memmove                                0x0032D970
#define kdlsym_addr_memset                                 0x00108B50
#define kdlsym_addr_mini_syscore_self_binary               0x0156B618
#define kdlsym_addr_mount_arg                              0x0019F100
#define kdlsym_addr_mount_argb                             0x0019D6C0
#define kdlsym_addr_mount_argf                             0x0019F1F0
#define kdlsym_addr_mtx_destroy                            0x00123CB0
#define kdlsym_addr_mtx_init                               0x00123C40
#define kdlsym_addr_mtx_lock_sleep                         0x00123230
#define kdlsym_addr_mtx_unlock_sleep                       0x00123560
#define kdlsym_addr_name_to_nids                           0x003fDB20
#define kdlsym_addr_pause                                  0x00466650
#define kdlsym_addr_pfind                                  0x0015FDB0
#define kdlsym_addr_pmap_activate                          0x00305C30
#define kdlsym_addr_printf                                 0x0011B150
#define kdlsym_addr_prison0                                0x01139180
#define kdlsym_addr_proc0                                  0x0218FFB0
#define kdlsym_addr_proc_reparent                          0x0044F7E0
#define kdlsym_addr_proc_rwmem                             0x00393470
#define kdlsym_addr_realloc                                0x004670D0
#define kdlsym_addr_rootvnode                              0x022F3570
#define kdlsym_addr_RsaesPkcs1v15Dec2048CRT                0x002EA6B0
#define kdlsym_addr_sbl_eap_internal_partition_key         0x026C4CF0
#define kdlsym_addr_sbl_keymgr_buf_gva                     0x0265C808
#define kdlsym_addr_sbl_keymgr_buf_va                      0x0265C000
#define kdlsym_addr_sbl_keymgr_key_rbtree                  0x0265B710
#define kdlsym_addr_sbl_keymgr_key_slots                   0x0265B700
#define kdlsym_addr_sbl_pfs_sx                             0x02668080
#define kdlsym_addr_sbl_drv_msg_mtx                        0x026536D0
#define kdlsym_addr_sceSblACMgrGetPathId                   0x001B4C30
#define kdlsym_addr_sceSblAuthMgrIsLoadable2               0x0065C520
#define kdlsym_addr_sceSblAuthMgrSmVerifyHeader            0x006586D0
#define kdlsym_addr_sceSblAuthMgrVerifyHeader              0x0065C580
#define kdlsym_addr_sceSblDriverSendMsg                    0x00635EA0
#define kdlsym_addr_sceSblGetEAPInternalPartitionKey       0x00646F90
#define kdlsym_addr_sceSblKeymgrClearKey                   0x0063F080
#define kdlsym_addr_sceSblKeymgrSetKeyForPfs               0x0063ED00
#define kdlsym_addr_sceSblKeymgrSetKeyStorage              0x0063C7C0
#define kdlsym_addr_sceSblKeymgrSmCallfunc                 0x0063E8D0
#define kdlsym_addr_sceSblPfsSetKeys                       0x00641810
#define kdlsym_addr_sceSblRngGetRandomNumber               0x006607B0
#define kdlsym_addr_sceSblServiceMailbox                   0x0064A6A0
#define kdlsym_addr_sched_prio                             0x001F9F30
#define kdlsym_addr_self_orbis_sysvec						           0x015424E8
#define kdlsym_addr_Sha256Hmac                             0x0031D470
#define kdlsym_addr_snprintf                               0x0011B460
#define kdlsym_addr_spinlock_exit                          0x00381AE1
#define kdlsym_addr_sprintf                                0x0011B390
#define kdlsym_addr_sscanf                                 0x001DF530
#define kdlsym_addr_strcmp                                 0x001C0000
#define kdlsym_addr_strdup                                 0x0047FA60
#define kdlsym_addr_strlen                                 0x002A6F30
#define kdlsym_addr_strncmp                                0x002f2BC0
#define kdlsym_addr_strstr                                 0x000E4D90
#define kdlsym_addr_sys_accept                             0x00153A40
#define kdlsym_addr_sys_bind                               0x001530B0
#define kdlsym_addr_sys_close                              0x00226B20
#define kdlsym_addr_sys_dup2                               0x00224D00
#define kdlsym_addr_sys_fstat                              0x002270C0
#define kdlsym_addr_sys_getdents                           0x0030EF20
#define kdlsym_addr_sys_kill                               0x00179C40
#define kdlsym_addr_sys_listen                             0x001532F0
#define kdlsym_addr_sys_lseek                              0x0030B5E0
#define kdlsym_addr_sys_mkdir                              0x0030E6F0
#define kdlsym_addr_sys_mlock                              0x003C3500
#define kdlsym_addr_sys_mlockall                           0x003C35B0
#define kdlsym_addr_sys_mmap                               0x0032C4C0
#define kdlsym_addr_sys_munmap                             0x003C2C20
#define kdlsym_addr_sys_nmount                             0x0019BDD0
#define kdlsym_addr_sys_open				                       0x003092D0
#define kdlsym_addr_sys_ptrace                             0x00393B10
#define kdlsym_addr_sys_read                               0x00138130
#define kdlsym_addr_sys_recvfrom                           0x00154D10
#define kdlsym_addr_sys_rmdir                              0x0030EA70
#define kdlsym_addr_sys_sendto                             0x001545E0
#define kdlsym_addr_sys_setuid                             0x000106A0
#define kdlsym_addr_sys_shutdown                           0x00154F60
#define kdlsym_addr_sys_socket                             0x00152790
#define kdlsym_addr_sys_stat                               0x0030BBA0
#define kdlsym_addr_sys_unlink                             0x0030AE80
#define kdlsym_addr_sys_unmount                            0x0019D6E0
#define kdlsym_addr_sys_wait4                              0x0044F920
#define kdlsym_addr_sys_write                              0x00138640
#define kdlsym_addr_target_id                              0x022BFF8D
#define kdlsym_addr_trap_fatal                             0x000A1780
#define kdlsym_addr_utilUSleep                             0x00694090
#define kdlsym_addr_vm_fault_disable_pagefaults            0x00218190
#define kdlsym_addr_vm_fault_enable_pagefaults             0x002181C0
#define kdlsym_addr_vm_map_lookup_entry                    0x0002A470
#define kdlsym_addr_vmspace_acquire_ref                    0x00029C90
#define kdlsym_addr_vmspace_alloc                          0x00029810
#define kdlsym_addr_vmspace_free                           0x00029AC0
#define kdlsym_addr_vn_fullpath                            0x00272810
#define kdlsym_addr_vsnprintf                              0x0011B500
#define kdlsym_addr_wakeup                                 0x00466650
#define kdlsym_addr_Xfast_syscall                          0x000001C0

// Kernel Hooks
#define kdlsym_addr_printf_hook                            0x0019C288

// FakeSelf hooks
#define kdlsym_addr_sceSblAuthMgrIsLoadable__sceSblACMgrGetPathId_hook        0x0065588D
#define kdlsym_addr_sceSblAuthMgrIsLoadable2_hook                             0x006559D1
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook 0x00658D13
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook   0x00659966
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookA                           0x0065612C
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookB                           0x00656DD8

// FakePkg hooks
#define kdlsym_addr_sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg_hook       0x0063C865
#define kdlsym_addr_sceSblKeymgrInvalidateKey__sx_xlock_hook                  0x0063FF2A
#define kdlsym_addr_npdrm_decrypt_isolated_rif__sceSblKeymgrSmCallfunc_hook   0x00664130
#define kdlsym_addr_npdrm_decrypt_rif_new__sceSblKeymgrSmCallfunc_hook        0x00664F13
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookA                          0x006B1A68
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookB                          0x006B1C97

// SceShellCore patches - call sceKernelIsGenuineCEX
#define ssc_sceKernelIsGenuineCEX_patchA                   0x00177AFB
#define ssc_sceKernelIsGenuineCEX_patchB                   0x007BA80B
#define ssc_sceKernelIsGenuineCEX_patchC                   0x008052A3
#define ssc_sceKernelIsGenuineCEX_patchD                   0x0099520B

// SceShellCore patches - call nidf_libSceDipsw
#define ssc_nidf_libSceDipsw_patchA                        0x00177B27
#define ssc_nidf_libSceDipsw_patchB                        0x0024A9DD
#define ssc_nidf_libSceDipsw_patchC                        0x007BA837
#define ssc_nidf_libSceDipsw_patchD                        0x00995237

#define ssc_enable_fakepkg_patch                           0x003DE982

// SceShellCore patches - use free prefix instead fake
#define ssc_fake_to_free_patch                             0x00F196B0

// SceShellCore patches - enable VR without spoof
#define ssc_enable_vr                                      0x00CE7790

// SceShellUI patches - debug patches
#define ssu_sceSblRcMgrIsAllowDebugMenuForSettings_patch   0x0001D4D0
#define ssu_sceSblRcMgrIsStoreMode_patch                   0x0001D830

// SceShellUI - remote play related patching
#define ssu_CreateUserForIDU_patch                         0x001A3350
#define ssu_remote_play_menu_patch                         0x00E86151

// SceRemotePlay - enabler patches
#define srp_enabler_patchA                                 0x0003C0B6
#define srp_enabler_patchB                                 0x0003C0D1

// sceRegMgr
#define kdlsym_addr_sceRegMgrGetInt                        0x0050A830
#define kdlsym_addr_sceRegMgrSetInt                        0x00509520
#define kdlsym_addr_sceRegMgrGetBin                        0x0050B170
#define kdlsym_addr_sceRegMgrSetBin                        0x0050B0C0
#define kdlsym_addr_sceRegMgrGetStr                        0x0050AFF0
#define kdlsym_addr_sceRegMgrSetStr                        0x0050AE30

// Debug (Not needed to port)
#define kdlsym_addr_g_obi_create                           0x00473AD0
#define kdlsym_addr_g_obi_destroy                          0x00473BD0
#define kdlsym_addr_g_obi_read                             0x00473C30
#define kdlsym_addr_g_part_ox_get_bank                     0x002f0D20
#define kdlsym_addr_hexdump                                0x0011CA30

#endif
