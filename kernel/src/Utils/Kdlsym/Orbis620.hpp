#pragma once
#include <Boot/Config.hpp>

#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_620
/*
These are the required functions in order for the Oni Framework to operate properly
These are all offsets into the base of the kernel. They expect all standard FreeBSD 9 prototypes

The reason we do not hardcode offsets here, is due to the different platforms that are supported, and
for the platforms that do enable kernel ASLR (Address Space Layout Randomization?)
*/

#define kdlsym_addr_Xfast_syscall                                      (0x000001C0)
#define kdlsym_addr__mtx_lock_flags                                    (0x00007470)
#define kdlsym_addr__mtx_lock_sleep                                    (0x00007510)
#define kdlsym_addr__mtx_unlock_flags                                  (0x00007740)
#define kdlsym_addr__mtx_unlock_sleep                                  (0x00007840)
#define kdlsym_addr__mtx_lock_spin_flags                               (0x000078A0)
#define kdlsym_addr__mtx_unlock_spin_flags                             (0x00007A60)
#define kdlsym_addr__thread_lock_flags                                 (0x00007BC0)
#define kdlsym_addr_mtx_init                                           (0x00007F30)
#define kdlsym_addr_index                                              (0x0001F740)
#define kdlsym_addr_sys_setlogin_patch1                                (0x0002BE6C)
#define kdlsym_addr_proc_reparent                                      (0x00076450)
#define kdlsym_addr__sx_slock                                          (0x00083D20)
#define kdlsym_addr__sx_xlock                                          (0x00083F00)
#define kdlsym_addr__sx_sunlock                                        (0x00084000)
#define kdlsym_addr__sx_xunlock                                        (0x000840C0)
#define kdlsym_addr_spinlock_enter                                     (0x000879B0)
#define kdlsym_addr_spinlock_exit                                      (0x000879F0)
#define kdlsym_addr_pause                                              (0x000973D0)
#define kdlsym_addr_wakeup                                             (0x000973F0)
#define kdlsym_addr_setrunnable                                        (0x000975F0)
#define kdlsym_addr_strlen                                             (0x000D5AA0)
#define kdlsym_addr_bzero                                              (0x00114640)
#define kdlsym_addr_memcpy                                             (0x00114700)
#define kdlsym_addr_memcpy_patch1                                      (0x0011470D)
#define kdlsym_addr_copyout                                            (0x00114800)
#define kdlsym_addr_copyout_patch1                                     (0x00114852)
#define kdlsym_addr_copyout_patch2                                     (0x0011485E)
#define kdlsym_addr_copyin                                             (0x001148F0)
#define kdlsym_addr_copyin_patch1                                      (0x00114947)
#define kdlsym_addr_copyin_patch2                                      (0x00114953)
#define kdlsym_addr_copyinstr                                          (0x00114DA0)
#define kdlsym_addr_copyinstr_patch1                                   (0x00114DF3)
#define kdlsym_addr_copyinstr_patch2                                   (0x00114DFF)
#define kdlsym_addr_proc_rwmem                                         (0x0013E7A0)
#define kdlsym_addr_realloc											   (0x001D9390)
#define kdlsym_addr_critical_enter                                     (0x00157850)
#define kdlsym_addr_critical_exit                                      (0x00157860)
#define kdlsym_addr_strcmp                                             (0x0015E8C0)
#define kdlsym_addr_kproc_create                                       (0x00197A20)
#define kdlsym_addr_kproc_exit                                         (0x00197C90)
#define kdlsym_addr_kthread_add                                        (0x00197F90)
#define kdlsym_addr_kthread_exit                                       (0x00198270)
#define kdlsym_addr_rindex                                             (0x001B2D10)
#define kdlsym_addr_faultin                                            (0x001B38D0)
#define kdlsym_addr_panic                                              (0x001D4D50)
#define kdlsym_addr_malloc                                             (0x001D9060)
#define kdlsym_addr_free                                               (0x001D9260)
#define kdlsym_addr_strncmp                                            (0x001FE650)
#define kdlsym_addr_sigqueue_take                                      (0x00224280)
#define kdlsym_addr_kern_mkdirat									   (0x00349C40)
#define kdlsym_addr_kern_psignal                                       (0x00228F50)
#define kdlsym_addr_vm_mmap                                            (0x00241260)
#define kdlsym_addr_kmem_alloc                                         (0x00270420)
#define kdlsym_addr_kmem_free                                          (0x002705F0)
#define kdlsym_addr_vrele                                              (0x00284690)
#define kdlsym_addr_kernel_sysctl                                      (0x002DDC80)
#define kdlsym_addr_trap_fatal                                         (0x002E0DD0)
#define kdlsym_addr_dblfault_handler                                   (0x002E1080)
#define kdlsym_addr_pmap_activate                                      (0x002F4BC0)
#define kdlsym_addr_strcpy                                             (0x002FA000)
#define kdlsym_addr_uprintf                                            (0x00307650)
#define kdlsym_addr_printf                                             (0x00307E10)
#define kdlsym_addr_snprintf                                           (0x00308120)
#define kdlsym_addr_vmspace_alloc                                      (0x0034CC30)
#define kdlsym_addr_vmspace_free                                       (0x0034CEE0)
#define kdlsym_addr_vmspace_acquire_ref                                (0x0034D0B0)
#define kdlsym_addr__vm_map_lock                                       (0x0034D110)
#define kdlsym_addr__vm_map_unlock                                     (0x0034D180)
#define kdlsym_addr__vm_map_lock_read                                  (0x0034D260)
#define kdlsym_addr__vm_map_unlock_read                                (0x0034D2B0)
#define kdlsym_addr_vm_map_insert                                      (0x0034E450)
#define kdlsym_addr_vm_map_fixed                                       (0x00350470)
#define kdlsym_addr_vm_map_stack                                       (0x00356AC0)
#define kdlsym_addr_devclass_get_softc                                 (0x003895F0)
#define kdlsym_addr_memset                                             (0x00394C60)
#define kdlsym_addr_namei                                              (0x003A5830)
#define kdlsym_addr_NDFREE                                             (0x003A7270)
#define kdlsym_addr_aslr_gen_pseudo_random_number                      (0x003D5EC0)
#define kdlsym_addr_sched_sleep                                        (0x0040C7D0)
#define kdlsym_addr_vm_object_reference                                (0x00446770)
#define kdlsym_addr_sceSblACMgrIsAllowedSystemLevelDebugging           (0x00459440)
#define kdlsym_addr_sceSblACMgrIsAllowedCoredump                       (0x00459460)
#define kdlsym_addr_sceSblACMgrHasMmapSelfCapability                   (0x004594B0)
#define kdlsym_addr_sceSblACMgrIsAllowedToMmapSelf                     (0x004594C0)
#define kdlsym_addr_sceSblACMgrGetDeviceAccessType                     (0x00459800)
#define kdlsym_addr_icc_nvs_write                                      (0x00462180)
#define kdlsym_addr_icc_nvs_read                                       (0x00462340)
#define kdlsym_addr_thread_unsuspend                                   (0x00470600)
#define kdlsym_addr_tdfind                                             (0x004709C0)
#define kdlsym_addr_memcmp                                             (0x004A1740)
#define kdlsym_addr_pfind                                              (0x004A1F00)
#define kdlsym_addr_sceRegMgrSetInt                                    (0x00502230)
#define kdlsym_addr_sceRegMgrGetInt                                    (0x00503530)
#define kdlsym_addr_sceSblDriverMapPages                               (0x00635E30)
#define kdlsym_addr_sceSblDriverUnmapPages                             (0x00636520)
#define kdlsym_addr_sceSblServiceCrypt                                 (0x0063B130)
#define kdlsym_addr_sceSblPfsSaveDataSignHeader                        (0x00640BC0)
#define kdlsym_addr_sceSblPfsSetKeys                                   (0x00640C90)
#define kdlsym_addr_sceSblPfsKeymgrGenKeys                             (0x00648B30)
#define kdlsym_addr_sceSblKeymgrClearKey                               (0x00649280)
#define kdlsym_addr_sceSblKeymgrHoldKey                                (0x006495C0)
#define kdlsym_addr_sceSblKeymgrDropKey                                (0x006499D0)
#define kdlsym_addr_sceSblKeymgrAllocateKeyId                          (0x00649BE0)
#define kdlsym_addr_sceSblKeymgrFreeKeyId                              (0x00649C80)
#define kdlsym_addr_sceSblAuthMgrAddEEkc3                              (0x0065EEB0)
#define kdlsym_addr_sceSblAuthMgrDeleteEEkc                            (0x0065F130)
#define kdlsym_addr_sceSblAuthMgrAddEEkc                               (0x0065F400)
#define kdlsym_addr_sceSblAuthMgrAddEEkc2                              (0x0065F660)
#define kdlsym_addr_sceSblNpDrmDecryptDiscRif                          (0x00668460)
#define kdlsym_addr_sceSblNpDrmDecryptRifNew                           (0x00669240)
#define kdlsym_addr_avcontrol_sleep                                    (0x00704200)
#define kdlsym_addr_vm_object_deallocate                               (0x00765250)
#define kdlsym_addr_sysdump_perform_dump_on_fatal_trap                 (0x00788C40)
#define kdlsym_addr_kernel_max_text_size                               (0x0078A3C6)
#define kdlsym_addr_prison0                                            (0x0113D458)
#define kdlsym_addr_M_USB                                              (0x0154FFE0)
#define kdlsym_addr_M_TEMP                                             (0x0155D070)
#define kdlsym_addr_self_orbis_sysvec                                  (0x01568AE8)
#define kdlsym_addr_global_settings_base                               (0x0215DB50)
#define kdlsym_addr_rootvnode                                          (0x021C3AC0)
#define kdlsym_addr_initproc                                           (0x021C7D60)
#define kdlsym_addr_maxtsiz                                            (0x021C8938)
#define kdlsym_addr_maxssiz                                            (0x021C8958)
#define kdlsym_addr_sgrowsiz                                           (0x021C8960)
#define kdlsym_addr_kernel_map                                         (0x021CBD88)
#define kdlsym_addr_allproc_lock                                       (0x022FBC70)
#define kdlsym_addr_proctree_lock                                      (0x022FBC90)
#define kdlsym_addr_allproc                                            (0x022FBCD0)
#define kdlsym_addr_vsnprintf										   (0x003081C0)
#define kdlsym_addr_eventhandler_register							   (0x00180F80)
#define kdlsym_addr_strdup											   (0x0034AA20)
#define kdlsym_addr_M_MOUNT											   (0x00D41270)
#define kdlsym_addr_strstr											   (0x004247F0)
#define kdlsym_addr_exec_new_vmspace								   (0x003D8300)
#define kdlsym_addr_Sha256Hmac											0x00165D80
#define kdlsym_addr_fpu_kern_enter										0x001E3990
#define kdlsym_addr_fpu_kern_leave										0x001E3A90
#define kdlsym_addr_fpu_ctx												0x02681800
#define kdlsym_addr_AesCbcCfb128Decrypt									0x002D4320
#define kdlsym_addr_gpu_va_page_list									0x02659E58
#define kdlsym_addr_sbl_pfs_sx											0x02677290
#define kdlsym_addr_RsaesPkcs1v15Dec2048CRT								0x002209B0
#define kdlsym_addr_sceSblKeymgrSetKeyForPfs							0x00648EF0
#define kdlsym_addr_AesCbcCfb128Encrypt									0x002D40F0
#define kdlsym_addr__sceSblAuthMgrGetSelfInfo							0x006575B0
#define kdlsym_addr_mini_syscore_self_binary							0x00157F648
#define kdlsym_addr_sceSblAuthMgrSmStart								0x0065B070
#define kdlsym_addr_sceSblAuthMgrSmVerifyHeader							0x0065CBD0
#define kdlsym_addr_sceSblServiceMailbox								0x0064B480
#define kdlsym_addr_sceSblKeymgrSmCallfunc								0x00648AC0
#define kdlsym_addr_sceSblAuthMgrVerifyHeader							0x00656D80
#define kdlsym_addr_sceSblAuthMgrIsLoadable2							0x00656D20
#define kdlsym_addr_sceSblDriverSendMsg									0x06378E0//0x006348E0
#define kdlsym_addr_proc0												0x021C6F90
#define kdlsym_addr_kern_thr_create										0x00470D40
#define kdlsym_addr_sscanf												0x000FEA40
#define kdlsym_addr_trap_msg											0x0112B120
#define kdlsym_addr_eventhandler_find_list                              0x00181530
#define kdlsym_addr_eventhandler_deregister                             0x00181320
#define kdlsym_addr_uma_large_malloc                                    0x00202090
#define kdlsym_addr_uma_large_free                                      0x00202150
#define kdlsym_addr__sx_init_flags                                      0x00083CA0
#define kdlsym_addr_memmove                                             0x00032C50
#define kdlsym_addr_destroy_dev                                         0x00207730
#define kdlsym_addr_vn_fullpath                                         0x0024ABA0
#define kdlsym_addr_make_dev_p                                          0x002071F0
#define kdlsym_addr_mtx_destroy                                         0x00007FA0
#define kdlsym_addr_killproc                                            0x00229AF0
#define kdlsym_addr_kernel_mount                                        0x00011C80
#define kdlsym_addr_mount_argf                                          0x00011AD0
#define kdlsym_addr_vm_fault_disable_pagefaults                         0x003FEE40
#define kdlsym_addr_vm_fault_enable_pagefaults                          0x003FEE70
#define kdlsym_addr_gdt                                                 0x021425F0
#define kdlsym_addr_sceSblKeymgrSetKeyStorage                           0x00646050
#define kdlsym_addr_sbl_keymgr_key_slots                                0x02684A78
#define kdlsym_addr_sbl_keymgr_buf_gva                                  0x02688808
#define kdlsym_addr_sbl_keymgr_buf_va                                   0x02688000
#define kdlsym_addr_sbl_keymgr_key_rbtree                               0x02684A88
#define kdlsym_addr_fpu_kern_enter                                      0x001E3990
#define kdlsym_addr_fpu_kern_ctx                                        0x0263A6C0
#define kdlsym_addr_sbl_drv_msg_mtx                                     0x02659E60
#define kdlsym_addr_sceSblKeymgrInvalidateKey__sx_xlock_hook            0x0064A11D
#define kdlsym_addr_kdlsym_addr_sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg_hook 0x006460F5
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookB                    0x0069FCE6
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookA                    0x0069FABA
#define kdlsym_addr_sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg_hook 0x006460F5
#define kdlsym_addr_npdrm_decrypt_rif_new__sceSblKeymgrSmCallfunc_hook  0x006693D3
#define kdlsym_addr_npdrm_decrypt_isolated_rif__sceSblKeymgrSmCallfunc_hook 0x006685C0
#define kdlsym_addr__sceSblAuthMgrSmStart                                0x0065B070
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook 0x0065DEE1
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook 0x0065D29A
#define kdlsym_addr_sceSblAuthMgrIsLoadable2_hook                       0x00658E7F
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookB                     0x0065A2D8
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookA                     0x00659636
#define kdlsym_addr_dynlib_do_dlsym                                     0x00017A20
#define kdlsym_addr_name_to_nids                                        0x00017D00
#define kdlsym_addr_console_cdev                                        0x021413E8
#define kdlsym_addr_M_IOV                                               0x00302B33
#define kdlsym_addr_console_write                                       0x0007F230
#define kdlsym_addr_deci_tty_write                                      0x00185B00
#define kdlsym_addr_cloneuio                                            0x00396310
#define kdlsym_addr_printf_hook                                         0x01A9FD28

// SceShellCore patches - call sceKernelIsGenuineCEX
#define ssc_sceKernelIsGenuineCEX_patchA  0x00186170
#define ssc_sceKernelIsGenuineCEX_patchB  0x0081ED20
#define ssc_sceKernelIsGenuineCEX_patchC  0x00869BA3
#define ssc_sceKernelIsGenuineCEX_patchD  0x009F7550

// SceShellCore patches - call nidf_libSceDipsw
#define ssc_nidf_libSceDipsw_patchA   0x0018619A
#define ssc_nidf_libSceDipsw_patchB   0x0025C923
#define ssc_nidf_libSceDipsw_patchC   0x0081ED4A
#define ssc_nidf_libSceDipsw_patchD   0x009F757A

#define ssu_sceSblRcMgrIsAllowDebugMenuForSettings_patch 0x001D6D0
#define ssu_sceSblRcMgrIsStoreMode_patch            0x001DA30

// SceShellUI - remote play related patching
#define ssu_CreateUserForIDU_patch                         0x001A0510
#define ssu_remote_play_menu_patch                         0x00E9F7B1

// SceRemotePlay - enabler patches
#define srp_enabler_patchA                                 0x0003CED6
#define srp_enabler_patchB                                 0x0003CEF1

#define ssc_enable_vr_patch                                0x00DBABD0

// SceShellCore patches - use free prefix instead fake
#define ssc_fake_to_free_patch                             0x00F9FB11




#endif
