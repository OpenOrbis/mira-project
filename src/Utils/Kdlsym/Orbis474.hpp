#pragma once
#include <Boot/Config.hpp>

// Offsets initially ported by SocraticBliss, and zecoxao, redone by kozarovv

#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_474
/*
These are the required functions in order for the Oni Framework to operate properly
These are all offsets into the base of the kernel. They expect all standard FreeBSD 9 prototypes

The reason we do not hardcode offsets here, is due to the different platforms that are supported, and
for the platforms that do enable kernel ASLR (Address Space Layout Randomization?)
*/

#define kdlsym_addr__mtx_lock_flags                        0x0030F8A0
#define kdlsym_addr__mtx_lock_sleep                        0x0030F910
#define kdlsym_addr__mtx_lock_spin_flags                   0x0030FC70
#define kdlsym_addr__mtx_unlock_flags                      0x0030FB40
#define kdlsym_addr__mtx_unlock_sleep                      0x0030FC10
#define kdlsym_addr__mtx_unlock_spin_flags                 0x0030FE30
#define kdlsym_addr__sceSblAuthMgrGetSelfInfo              0x00629880
#define kdlsym_addr__sceSblAuthMgrSmStart                  0x00625410
#define kdlsym_addr__sx_init_flags                         0x00390720
#define kdlsym_addr__sx_slock                              0x003907A0
#define kdlsym_addr__sx_sunlock                            0x00390920
#define kdlsym_addr__sx_xlock                              0x00390850
#define kdlsym_addr__sx_xunlock                            0x003909E0
#define kdlsym_addr__thread_lock_flags                     0x0030FF70
#define kdlsym_addr__vm_map_lock_read                      0x00392ED0
#define kdlsym_addr__vm_map_unlock_read                    0x00392F20
#define kdlsym_addr_AesCbcCfb128Decrypt                    0x00179950
#define kdlsym_addr_AesCbcCfb128Encrypt                    0x00179720
#define kdlsym_addr_allproc                                0x01ADF718
#define kdlsym_addr_allproc_lock                           0x01ADF6B8
#define kdlsym_addr_avcontrol_sleep                        0x006C8560
#define kdlsym_addr_contigfree                             0x00252AA0
#define kdlsym_addr_contigmalloc                           0x00252740
#define kdlsym_addr_copyin                                 0x00149F20
#define kdlsym_addr_copyinstr                              0x0014A390
#define kdlsym_addr_copyout                                0x00149E40
#define kdlsym_addr_critical_enter                         0x0023F9E0
#define kdlsym_addr_critical_exit                          0x0023F9F0
#define kdlsym_addr_destroy_dev                            0x0036F8F0
// #define kdlsym_addr_dmem_start_app_process                 0x0
#define kdlsym_addr_dynlib_do_dlsym                        0x00066000
#define kdlsym_addr_dynlib_find_obj_by_handle              0x000670B0
#define kdlsym_addr_eventhandler_deregister                0x003CAA10
#define kdlsym_addr_eventhandler_find_list                 0x003CAC00
#define kdlsym_addr_eventhandler_register                  0x003CA6A0
#define kdlsym_addr_exec_new_vmspace                       0x002EAA50
#define kdlsym_addr_faultin                                0x00311010
#define kdlsym_addr_fget_unlocked                          0x0042DFD0
#define kdlsym_addr_fpu_kern_ctx                           0x02528CC0
#define kdlsym_addr_fpu_kern_enter                         0x00058B60
#define kdlsym_addr_fpu_kern_leave                         0x00058C60
#define kdlsym_addr_free                                   0x003F87A0
#define kdlsym_addr_gdt                                    0x01B65AD0
#define kdlsym_addr_gpu_va_page_list                       0x02525DD0
#define kdlsym_addr_icc_nvs_read                           0x0001B850
#define kdlsym_addr_kern_close                             0x0042BA80
#define kdlsym_addr_kern_ioctl                             0x0005F000
#define kdlsym_addr_kern_mkdirat                           0x00445CD0
#define kdlsym_addr_kern_open                              0x00440B70
#define kdlsym_addr_kern_openat                            0x00440BD0
#define kdlsym_addr_kern_readv                             0x0005E310
#define kdlsym_addr_kern_reboot                            0x00098EE0
#define kdlsym_addr_kern_sysents                           0x01034790
#define kdlsym_addr_kern_thr_create                        0x002EEED0
#define kdlsym_addr_kernel_map                             0x01B39218
#define kdlsym_addr_kernel_mount                           0x000DC820
#define kdlsym_addr_killproc                               0x00025810
#define kdlsym_addr_kmem_alloc                             0x0016DF30
#define kdlsym_addr_kmem_free                              0x0016E100
#define kdlsym_addr_kproc_create                           0x00465590
#define kdlsym_addr_kproc_exit                             0x00465810
#define kdlsym_addr_kthread_add                            0x00465B20
#define kdlsym_addr_kthread_exit                           0x00465DF0
#define kdlsym_addr_M_LINKER                               0x01452320
#define kdlsym_addr_M_MOUNT                                0x014572A0
#define kdlsym_addr_M_TEMP                                 0x0199BB80
#define kdlsym_addr_make_dev_p                             0x0036F3C0
#define kdlsym_addr_malloc                                 0x003F85C0
#define kdlsym_addr_memcmp                                 0x00244EE0
#define kdlsym_addr_memcpy                                 0x00149D40
#define kdlsym_addr_memmove                                0x002F0940
#define kdlsym_addr_memset                                 0x00304DD0
#define kdlsym_addr_mini_syscore_self_binary               0x01479558
#define kdlsym_addr_mount_arg                              0x000DC5A0
#define kdlsym_addr_mount_argb                             0x000DAB40
#define kdlsym_addr_mount_argf                             0x000DC680
#define kdlsym_addr_mtx_destroy                            0x00310330
#define kdlsym_addr_mtx_init                               0x003102C0
#define kdlsym_addr_mtx_lock_sleep                         0x0030F910
#define kdlsym_addr_mtx_unlock_sleep                       0x0030FC10
#define kdlsym_addr_name_to_nids                           0x00068D10
#define kdlsym_addr_pause                                  0x002635A0
#define kdlsym_addr_pfind                                  0x00078DC0
#define kdlsym_addr_pmap_activate                          0x004283A0
#define kdlsym_addr_printf                                 0x00017F30
#define kdlsym_addr_prison0                                0x01042AB0
#define kdlsym_addr_proc0                                  0x01B37C00
#define kdlsym_addr_proc_reparent                          0x0015BA50
#define kdlsym_addr_proc_rwmem                             0x0017BDD0
#define kdlsym_addr_realloc                                0x003F88D0
#define kdlsym_addr_rootvnode                              0x021B89E0
#define kdlsym_addr_RsaesPkcs1v15Dec2048CRT                0x003F0070
#define kdlsym_addr_sbl_eap_internal_partition_key         0x0259CCD0
#define kdlsym_addr_sbl_eap_internal_partition_key         0x0259CCD0
#define kdlsym_addr_sbl_keymgr_buf_gva                     0x02548800
#define kdlsym_addr_sbl_keymgr_buf_va                      0x02548000
#define kdlsym_addr_sbl_keymgr_key_rbtree                  0x02544DE0
#define kdlsym_addr_sbl_keymgr_key_slots                   0x02544DD0
#define kdlsym_addr_sbl_pfs_sx                             0x02529310
#define kdlsym_addr_sceSblACMgrGetPathId                   0x00169840
#define kdlsym_addr_sceSblAuthMgrIsLoadable2               0x00629040
#define kdlsym_addr_sceSblAuthMgrSmVerifyHeader            0x00626640
#define kdlsym_addr_sceSblAuthMgrVerifyHeader              0x006290A0
#define kdlsym_addr_sceSblDriverSendMsg                    0x00603CA0
#define kdlsym_addr_sceSblGetEAPInternalPartitionKey       0x0060C6E0
#define kdlsym_addr_sceSblKeymgrClearKey                   0x00610D80
#define kdlsym_addr_sceSblKeymgrSetKeyForPfs               0x006109E0
#define kdlsym_addr_sceSblKeymgrSetKeyStorage              0x006093D0
#define kdlsym_addr_sceSblKeymgrSmCallfunc                 0x00611530
#define kdlsym_addr_sceSblPfsSetKeys                       0x006095E0
#define kdlsym_addr_sceSblRngGetRandomNumber               0x0062CCB0
#define kdlsym_addr_sceSblServiceMailbox                   0x00617AB0
#define kdlsym_addr_self_orbis_sysvec                      0x01468700
#define kdlsym_addr_Sha256Hmac                             0x002D7E00
#define kdlsym_addr_snprintf                               0x00018230
#define kdlsym_addr_spinlock_exit                          0x00284A80
#define kdlsym_addr_sprintf                                0x00018160
#define kdlsym_addr_sscanf                                 0x00304300
#define kdlsym_addr_strcmp                                 0x001DD150
#define kdlsym_addr_strdup                                 0x004538A0
#define kdlsym_addr_strlen                                 0x00353720
#define kdlsym_addr_strncmp                                0x003DC520
#define kdlsym_addr_strstr                                 0x00263B90
#define kdlsym_addr_sys_accept                             0x003ECD30
#define kdlsym_addr_sys_bind                               0x003EC3C0
#define kdlsym_addr_sys_close                              0x0042BA70
#define kdlsym_addr_sys_dup2                               0x00429CC0
#define kdlsym_addr_sys_fstat                              0x0042C020
#define kdlsym_addr_sys_getdents                           0x00446470
#define kdlsym_addr_sys_kill                               0x000232A0
#define kdlsym_addr_sys_listen                             0x003EC5D0
#define kdlsym_addr_sys_lseek                              0x00442B90
#define kdlsym_addr_sys_mkdir                              0x00445C50
#define kdlsym_addr_sys_mlock                              0x001420D0
#define kdlsym_addr_sys_mlockall                           0x00142180
#define kdlsym_addr_sys_mmap                               0x00141080
#define kdlsym_addr_sys_munmap                             0x00141820
#define kdlsym_addr_sys_nmount                             0x000D91A0
#define kdlsym_addr_sys_open                               0x00440B50
#define kdlsym_addr_sys_ptrace                             0x0017C260
#define kdlsym_addr_sys_read                               0x0005E230
#define kdlsym_addr_sys_recvfrom                           0x003EDEF0
#define kdlsym_addr_sys_rmdir                              0x00445FD0
#define kdlsym_addr_sys_sendto                             0x003ED830
#define kdlsym_addr_sys_setuid                             0x00113AE0
#define kdlsym_addr_sys_shutdown                           0x003EE0C0
#define kdlsym_addr_sys_socket                             0x003EBA90
#define kdlsym_addr_sys_stat                               0x00443190
#define kdlsym_addr_sys_unlink                             0x004425A0
#define kdlsym_addr_sys_unmount                            0x000DAB60
#define kdlsym_addr_sys_wait4                              0x0015BB90
#define kdlsym_addr_sys_write                              0x0005E780
#define kdlsym_addr_trap_fatal                             0x003DCBD0
#define kdlsym_addr_utilUSleep                             0x006626A0
#define kdlsym_addr_vm_fault_disable_pagefaults            0x002A3BA0
#define kdlsym_addr_vm_fault_enable_pagefaults             0x002A3BD0
#define kdlsym_addr_vm_map_lookup_entry                    0x00393A90
#define kdlsym_addr_vmspace_acquire_ref                    0x00392D00
#define kdlsym_addr_vmspace_alloc                          0x00392890
#define kdlsym_addr_vmspace_free                           0x00392B30
#define kdlsym_addr_vn_fullpath                            0x002FD7B0
#define kdlsym_addr_vsnprintf                              0x000182D0
#define kdlsym_addr_wakeup                                 0x002635C0
#define kdlsym_addr_Xfast_syscall                          0x0030B7D0

// FakeSelf hooks
#define kdlsym_addr_sceSblAuthMgrIsLoadable__sceSblACMgrGetPathId_hook        0x006224EC
#define kdlsym_addr_sceSblAuthMgrIsLoadable2_hook                             0x0062263F
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook 0x00626CAA
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook   0x006278D1
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookA                           0x00622D66
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookB                           0x00623989

// FakePkg hooks
#define kdlsym_addr_sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg_hook       0x00609475
#define kdlsym_addr_sceSblKeymgrInvalidateKey__sx_xlock_hook                  0x00611C0D
#define kdlsym_addr_npdrm_decrypt_isolated_rif__sceSblKeymgrSmCallfunc_hook   0x006312F0
#define kdlsym_addr_npdrm_decrypt_rif_new__sceSblKeymgrSmCallfunc_hook        0x006320CE
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookA                          0x0069AFE4
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookB                          0x0069B214

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

#define ssc_enable_fakepkg_patch                           0x00385032

// SceShellCore patches - use free prefix instead fake
#define ssc_fake_to_free_patch                             0x00D50208

// SceShellUI patches - debug patches
#define ssu_sceSblRcMgrIsAllowDebugMenuForSettings_patch   0x0
#define ssu_sceSblRcMgrIsStoreMode_patch                   0x0

#endif
