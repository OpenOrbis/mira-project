#pragma once
#include <Boot/Config.hpp>

#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_900
/*
    These are the required functions in order for the Oni Framework to operate properly
    These are all offsets into the base of the kernel. They expect all standard FreeBSD 9 prototypes

    The reason we do not hardcode offsets here, is due to the different platforms that are supported, and
    for the platforms that do enable kernel ASLR (Address Space Layout Randomization?)
*/

#define kdlsym_addr__mtx_lock_flags                        0x002EEEB0
#define kdlsym_addr__mtx_lock_sleep                        0x002EEF50
#define kdlsym_addr__mtx_lock_spin_flags                   0x002EF2D0
#define kdlsym_addr__mtx_unlock_flags                      0x002EF170
#define kdlsym_addr__mtx_unlock_sleep                      0x002EF270
#define kdlsym_addr__mtx_unlock_spin_flags                 0x002EF4A0
#define kdlsym_addr__sceSblAuthMgrGetSelfInfo              0x006441E0
#define kdlsym_addr__sceSblAuthMgrSmStart                  0x0063FEE0
#define kdlsym_addr__sx_init_flags                         0x0043E390
#define kdlsym_addr__sx_slock                              0x0043E1A0
#define kdlsym_addr__sx_sunlock                            0x0043E710
#define kdlsym_addr__sx_xlock                              0x0043E610
#define kdlsym_addr__sx_xunlock                            0x0043E7D0
#define kdlsym_addr__thread_lock_flags                     0x002EF610
#define kdlsym_addr__vm_map_lock_read                      0x0007BB80
#define kdlsym_addr__vm_map_unlock_read                    0x0007BBD0
#define kdlsym_addr_AesCbcCfb128Decrypt                    0x001FF500
#define kdlsym_addr_AesCbcCfb128Encrypt                    0x001FF2D0
#define kdlsym_addr_allproc                                0x01B946E0
#define kdlsym_addr_allproc_lock                           0x01B94680
#define kdlsym_addr_avcontrol_sleep                        0x006EAE30
#define kdlsym_addr_cloneuio                               0x0044E7E0
#define kdlsym_addr_console_cdev                           0x021F1128
#define kdlsym_addr_console_write                          0x002D6EB0
#define kdlsym_addr_contigfree                             0x00270BB0
#define kdlsym_addr_contigmalloc                           0x00270810
#define kdlsym_addr_copyin                                 0x002716A0
#define kdlsym_addr_copyinstr                              0x00271B50
#define kdlsym_addr_copyout                                0x002715B0
#define kdlsym_addr_critical_enter                         0x002C1980
#define kdlsym_addr_critical_exit                          0x002C19A0
#define kdlsym_addr_deci_tty_write                         0x0048CDE0
#define kdlsym_addr_destroy_dev                            0x001EFAB0
#define kdlsym_addr_dmem_start_app_process                 0x00116B60
#define kdlsym_addr_dynlib_do_dlsym                        0x0021F1E0
#define kdlsym_addr_dynlib_find_obj_by_handle              0x00220360
#define kdlsym_addr_eventhandler_deregister                0x000F8700
#define kdlsym_addr_eventhandler_find_list                 0x000F88F0
#define kdlsym_addr_eventhandler_register                  0x000F8370
#define kdlsym_addr_exec_new_vmspace                       0x00050E60
#define kdlsym_addr_faultin                                0x001D4910
#define kdlsym_addr_fget_unlocked                          0x0045D170
#define kdlsym_addr_fpu_kern_ctx                           0x026541C0
#define kdlsym_addr_fpu_kern_enter                         0x002196D0
#define kdlsym_addr_fpu_kern_leave                         0x00219790
#define kdlsym_addr_free                                   0x00301CE0
#define kdlsym_addr_gdt                                    0x021FB280 // sus
#define kdlsym_addr_gpu_va_page_list                       0x02646CA8
#define kdlsym_addr_icc_nvs_read                           0x0010B310
#define kdlsym_addr_kern_close                             0x0045AAC0
#define kdlsym_addr_kern_ioctl                             0x0044FA80
#define kdlsym_addr_kern_mkdirat                           0x001DF080
#define kdlsym_addr_kern_open                              0x001D9EE0
#define kdlsym_addr_kern_openat                            0x001D9F40
#define kdlsym_addr_kern_readv                             0x0044F350
#define kdlsym_addr_kern_reboot                            0x0029A380
#define kdlsym_addr_kern_sysents                           0x01100310
#define kdlsym_addr_kern_thr_create                        0x001ED670
#define kdlsym_addr_kernel_map                             0x02268D48
#define kdlsym_addr_kernel_mount                           0x0004DF50
#define kdlsym_addr_killproc                               0x00029780
#define kdlsym_addr_kmem_alloc                             0x0037BE70
#define kdlsym_addr_kmem_free                              0x0037C040
#define kdlsym_addr_kproc_create                           0x000969E0
#define kdlsym_addr_kproc_exit                             0x00096C50
#define kdlsym_addr_kthread_add                            0x00096F40
#define kdlsym_addr_kthread_exit                           0x00097230
#define kdlsym_addr_M_IOV                                  0x01A792C0
#define kdlsym_addr_M_LINKER                               0x0155C2D0
#define kdlsym_addr_M_MOUNT                                0x015279F0
#define kdlsym_addr_M_TEMP                                 0x015621E0
#define kdlsym_addr_make_dev_p                             0x001EF590
#define kdlsym_addr_malloc                                 0x00301B20
#define kdlsym_addr_memcmp                                 0x00271E20 //sus
#define kdlsym_addr_memcpy                                 0x002714B0
#define kdlsym_addr_memmove                                0x000AF850
#define kdlsym_addr_memset                                 0x001496C0
#define kdlsym_addr_mini_syscore_self_binary               0x01579DF8 // sus
#define kdlsym_addr_mount_arg                              0x0004DCC0
#define kdlsym_addr_mount_argb                             0x0004D460
#define kdlsym_addr_mount_argf                             0x0004DDB0
#define kdlsym_addr_mtx_destroy                            0x002EF9D0
#define kdlsym_addr_mtx_init                               0x002EF960
#define kdlsym_addr_mtx_lock_sleep                         0x002EEF50
#define kdlsym_addr_mtx_unlock_sleep                       0x002EF270
#define kdlsym_addr_name_to_nids                           0x0021F4C0
#define kdlsym_addr_pause                                  0x00453EA0
#define kdlsym_addr_pfind                                  0x00178960
#define kdlsym_addr_pmap_activate                          0x00138940
#define kdlsym_addr_printf                                 0x000B7A30
#define kdlsym_addr_prison0                                0x01A76580 // sus
#define kdlsym_addr_proc0                                  0x01B90E00
#define kdlsym_addr_proc_reparent                          0x00173E90
#define kdlsym_addr_proc_rwmem                             0x0041EB00
#define kdlsym_addr_realloc                                0x00301DE0
#define kdlsym_addr_rootvnode                              0x021EFF20
#define kdlsym_addr_RsaesPkcs1v15Dec2048CRT                0x003ED220
#define kdlsym_addr_sbl_eap_internal_partition_key         0x026C4C90
#define kdlsym_addr_sbl_keymgr_buf_gva                     0x0264C808
#define kdlsym_addr_sbl_keymgr_buf_va                      0x0264C000
#define kdlsym_addr_sbl_keymgr_key_rbtree                  0x02648248
#define kdlsym_addr_sbl_keymgr_key_slots                   0x02648238
#define kdlsym_addr_sbl_pfs_sx                             0x0264DB40
#define kdlsym_addr_sbl_drv_msg_mtx                        0x02646CB0
#define kdlsym_addr_sceSblACMgrGetPathId                   0x0008BCD0
#define kdlsym_addr_sceSblAuthMgrIsLoadable2               0x006439A0
#define kdlsym_addr_sceSblAuthMgrSmVerifyHeader            0x00641C60
#define kdlsym_addr_sceSblAuthMgrVerifyHeader              0x00643A00
#define kdlsym_addr_sceSblDriverSendMsg                    0x0061CED0
#define kdlsym_addr_sceSblGetEAPInternalPartitionKey       0x0061DC10
#define kdlsym_addr_sceSblKeymgrClearKey                   0x0061F9D0
#define kdlsym_addr_sceSblKeymgrSetKeyForPfs               0x0061F690
#define kdlsym_addr_sceSblKeymgrSetKeyStorage              0x00624970
#define kdlsym_addr_sceSblKeymgrSmCallfunc                 0x0061F260
#define kdlsym_addr_sceSblPfsSetKeys                       0x006252D0
#define kdlsym_addr_sceSblRngGetRandomNumber               0x006496A0
#define kdlsym_addr_sceSblServiceMailbox                   0x00630C40
#define kdlsym_addr_sched_prio                             0x001CBB90
#define kdlsym_addr_self_orbis_sysvec                      0x01528E30
#define kdlsym_addr_Sha256Hmac                             0x00445060
#define kdlsym_addr_snprintf                               0x000B7D30
#define kdlsym_addr_spinlock_exit                          0x00314780
#define kdlsym_addr_sprintf                                0x000B7C70
#define kdlsym_addr_sscanf                                 0x0026C8D0
#define kdlsym_addr_strcmp                                 0x0040E700
#define kdlsym_addr_strdup                                 0x00278540
#define kdlsym_addr_strlen                                 0x0030F450
#define kdlsym_addr_strncmp                                0x00124750
#define kdlsym_addr_strstr                                 0x00487AB0
#define kdlsym_addr_sys_accept                             0x004488A0
#define kdlsym_addr_sys_bind                               0x00447F30
#define kdlsym_addr_sys_close                              0x0045AAB0
#define kdlsym_addr_sys_dup2                               0x00458CB0
#define kdlsym_addr_sys_fstat                              0x0045B030
#define kdlsym_addr_sys_getdents                           0x001DF840
#define kdlsym_addr_sys_kill                               0x00027050
#define kdlsym_addr_sys_listen                             0x00448170
#define kdlsym_addr_sys_lseek                              0x001DBF10
#define kdlsym_addr_sys_mkdir                              0x001DF000
#define kdlsym_addr_sys_mlock                              0x00166F30
#define kdlsym_addr_sys_mlockall                           0x00166FE0
#define kdlsym_addr_sys_mmap                               0x00165F50
#define kdlsym_addr_sys_munmap                             0x00166660
#define kdlsym_addr_sys_nmount                             0x0004A960
#define kdlsym_addr_sys_open                               0x001D9EC0
#define kdlsym_addr_sys_ptrace                             0x0041F1A0
#define kdlsym_addr_sys_read                               0x0044EBC0
#define kdlsym_addr_sys_recvfrom                           0x00449B40
#define kdlsym_addr_sys_rmdir                              0x001DF380
#define kdlsym_addr_sys_sendto                             0x00449420
#define kdlsym_addr_sys_setuid                             0x000018E0
#define kdlsym_addr_sys_shutdown                           0x00449D90
#define kdlsym_addr_sys_socket                             0x00447530
#define kdlsym_addr_sys_stat                               0x001DC4F0
#define kdlsym_addr_sys_unlink                             0x001DB8D0
#define kdlsym_addr_sys_unmount                            0x0004C290
#define kdlsym_addr_sys_wait4                              0x00173FD0
#define kdlsym_addr_sys_write                              0x0044F0D0
#define kdlsym_addr_trap_fatal                             0x002DF710
#define kdlsym_addr_utilUSleep                             0x0065B170
#define kdlsym_addr_vm_fault_disable_pagefaults            0x00156C70
#define kdlsym_addr_vm_fault_enable_pagefaults             0x00156CA0
#define kdlsym_addr_vm_map_lookup_entry                    0x0007C1C0
#define kdlsym_addr_vmspace_acquire_ref                    0x0007B9E0
#define kdlsym_addr_vmspace_alloc                          0x0007B550
#define kdlsym_addr_vmspace_free                           0x0007B810
#define kdlsym_addr_vn_fullpath                            0x002648C0
#define kdlsym_addr_vsnprintf                              0x000B7DD0
#define kdlsym_addr_wakeup                                 0x00453EC0
#define kdlsym_addr_Xfast_syscall                          0x000001C0

// Kernel Hooks
#define kdlsym_addr_printf_hook                            0x01A7ED68

// FakeSelf Hooks
#define kdlsym_addr_sceSblAuthMgrIsLoadable__sceSblACMgrGetPathId_hook        0x0064473C
#define kdlsym_addr_sceSblAuthMgrIsLoadable2_hook                             0x0064488E
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook   0x0064232D
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook 0x00642F68
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookA                           0x00645026
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookB                           0x00645D09

// FakePkg Hooks
#define kdlsym_addr_sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg_hook       0x00624A15
#define kdlsym_addr_sceSblKeymgrInvalidateKey__sx_xlock_hook                  0x0062084D
#define kdlsym_addr_npdrm_decrypt_isolated_rif__sceSblKeymgrSmCallfunc_hook   0x0064E070
#define kdlsym_addr_npdrm_decrypt_rif_new__sceSblKeymgrSmCallfunc_hook        0x0064EE3E
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookA                          0x006C3EF9
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookB                          0x006C412A

// sceRegMgr
#define kdlsym_addr_sceRegMgrGetInt                        0x004E9DD0
#define kdlsym_addr_sceRegMgrSetInt                        0x004E8B10
#define kdlsym_addr_sceRegMgrGetBin                        0x004EA770
#define kdlsym_addr_sceRegMgrSetBin                        0x004EA6C0
#define kdlsym_addr_sceRegMgrGetStr                        0x004EA5F0
#define kdlsym_addr_sceRegMgrSetStr                        0x004F65B5

// SceShellCore patches - call sceKernelIsGenuineCEX
#define ssc_sceKernelIsGenuineCEX_patchA                   0x0016EAA4
#define ssc_sceKernelIsGenuineCEX_patchB                   0x008621D4
#define ssc_sceKernelIsGenuineCEX_patchC                   0x008AFBC2
#define ssc_sceKernelIsGenuineCEX_patchD                   0x00A27BD4

// SceShellCore patches - call nidf_libSceDipsw
#define ssc_nidf_libSceDipsw_patchA                        0x0016EAD2
#define ssc_nidf_libSceDipsw_patchB                        0x00249F7B
#define ssc_nidf_libSceDipsw_patchC                        0x00862202
#define ssc_nidf_libSceDipsw_patchD                        0x00A27C02

#define ssc_enable_fakepkg_patch                           0x003D7AFF

// SceShellCore patches - use free prefix instead fake
#define ssc_fake_to_free_patch                             0x00FD3211

// SceShellCore patches - enable remote pkg installer
#define ssc_enable_data_mount_patch                        0x0032079B

// SceShellCore patches - enable VR without spoof
#define ssc_enable_vr_patch                                0x00DB0B80

// SceShellCore patches - enable official external HDD support (Support added in 4.50
// #define ssc_external_hdd_pkg_installer_patch               0x00A10A80
// #define ssc_external_hdd_version_patchA                    0x006180FD
// #define ssc_external_hdd_version_patchB                    0xDEADC0DE

// SceShellUI patches - debug patches
#define ssu_sceSblRcMgrIsAllowDebugMenuForSettings_patch   0x0001D1C0
#define ssu_sceSblRcMgrIsStoreMode_patch                   0x0001D520

// SceShellUI - remote play related patching
// #define ssu_CreateUserForIDU_patch                         0xDEADC0DE
// #define ssu_remote_play_menu_patch                         0xDEADC0DE

// SceRemotePlay - enabler patches
// #define srp_enabler_patchA                                 0xDEADC0DE
// #define srp_enabler_patchB                                 0xDEADC0DE

#endif
