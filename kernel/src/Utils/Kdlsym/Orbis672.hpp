#pragma once

#include <Boot/Config.hpp>

#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_672
/*
    These are the required functions in order for the Oni Framework to operate properly
    These are all offsets into the base of the kernel. They expect all standard FreeBSD 9 prototypes

    The reason we do not hardcode offsets here, is due to the different platforms that are supported, and
    for the platforms that do enable kernel ASLR (Address Space Layout Randomization?)
*/
#pragma clang diagnostic push
//#pragma clang diagnostic warning "-Wunused-macros"

#define kdlsym_addr_Xfast_syscall                                      (0x000001C0)
#define kdlsym_addr_malloc                                             (0x0000D7A0)
#define kdlsym_addr_free                                               (0x0000D9A0)
#define kdlsym_addr_sigqueue_take                                      (0x00028420)
#define kdlsym_addr_kern_psignal                                       (0x0002D0F0)
#define kdlsym_addr__sx_slock                                          (0x000424E0)
#define kdlsym_addr__sx_xlock                                          (0x000426C0)
#define kdlsym_addr__sx_sunlock                                        (0x000427C0)
#define kdlsym_addr__sx_xunlock                                        (0x00042880)
#define kdlsym_addr_pmap_activate                                      (0x0005A280)
#define kdlsym_addr_kproc_create                                       (0x0008A0A0)
#define kdlsym_addr_kproc_exit                                         (0x0008A310)
#define kdlsym_addr_kthread_add                                        (0x0008A600)
#define kdlsym_addr_kthread_exit                                       (0x0008A8F0)
#define kdlsym_addr_spinlock_enter                                     (0x000A35B0)
#define kdlsym_addr_spinlock_exit                                      (0x000A35F0)
#define kdlsym_addr_vm_mmap                                            (0x000AC520)
#define kdlsym_addr_map_self_patch3                                    (0x000AD2E4)
#define kdlsym_addr_vm_object_reference                                (0x000D71A0)
#define kdlsym_addr_sys_setlogin_patch1                                (0x0010EC1C)
#define kdlsym_addr_proc_rwmem                                         (0x0010EE10)
#define kdlsym_addr_kern_ptrace_patch1                                 (0x0010F892)
#define kdlsym_addr_aslr_gen_pseudo_random_number                      (0x00118A00)
#define kdlsym_addr_cloneuio                                           (0x002D3060)
#define kdlsym_addr_console_cdev                                       (0x022C0CD8)
#define kdlsym_addr_console_write                                      (0x003F8710)
#define kdlsym_addr_disable_aslr_patch1                                (0x00119A20)
#define kdlsym_addr_disable_aslr_patch2                                (0x00119ACA)
#define kdlsym_addr_uprintf                                            (0x00122AC0)
#define kdlsym_addr_printf                                             (0x00123280)
#define kdlsym_addr_snprintf                                           (0x00123590)
#define kdlsym_addr_memset                                             (0x001687D0)
#define kdlsym_addr_devclass_get_softc                                 (0x0015A9A0)
#define kdlsym_addr_panic                                              (0x002074C0)
#define kdlsym_addr_memcmp                                             (0x00207E40)
#define kdlsym_addr_pause                                              (0x0022A080)
#define kdlsym_addr_wakeup                                             (0x0022A0A0)
#define kdlsym_addr_setrunnable                                        (0x0022A2A0)
#define kdlsym_addr_sceSblACMgrIsAllowedSystemLevelDebugging           (0x00233BD0)
#define kdlsym_addr_sceSblACMgrIsAllowedCoredump                       (0x00233BF0)
#define kdlsym_addr_sceSblACMgrHasMmapSelfCapability                   (0x00233C40)
#define kdlsym_addr_sceSblACMgrIsAllowedToMmapSelf                     (0x00233C50)
#define kdlsym_addr_sceSblACMgrGetDeviceAccessType                     (0x00233F90)
#define kdlsym_addr_strlen                                             (0x002433E0)
#define kdlsym_addr_kmem_alloc                                         (0x00250730)
#define kdlsym_addr_kmem_alloc_patch1                                  (0x002507F5)
#define kdlsym_addr_kmem_free                                          (0x00250900)
#define kdlsym_addr_kmem_alloc_patch2                                  (0x00250803)
#define kdlsym_addr_thread_unsuspend                                   (0x00267E30)
#define kdlsym_addr_tdfind                                             (0x002681F0)
#define kdlsym_addr_kernel_sysctl                                      (0x00277DF0)
#define kdlsym_addr_critical_enter                                     (0x002AA0A0)
#define kdlsym_addr_critical_exit                                      (0x002AA0B0)
#define kdlsym_addr_deci_tty_write                                     (0x004A97E0)
#define kdlsym_addr_trap_fatal_return_patch1                           (0x002ED2DA)
#define kdlsym_addr_trap_fatal                                         (0x002ED2E0)
#define kdlsym_addr_trap_fatal_return_patch2                           (0x002ED57C)
#define kdlsym_addr_dblfault_handler                                   (0x002ED580)
#define kdlsym_addr_rindex                                             (0x00326130)
#define kdlsym_addr_pfind                                              (0x0033DC90)
#define kdlsym_addr_strcmp                                             (0x00341810)
#define kdlsym_addr_strncmp                                            (0x0039B6E0)
#define kdlsym_addr_kern_readv                                         (0x0039B790)
#define kdlsym_addr_vrele                                              (0x003B7DD0)
#define kdlsym_addr_bzero                                              (0x003C14F0)
#define kdlsym_addr_memcpy                                             (0x003C15B0)
#define kdlsym_addr_memcpy_patch1                                      (0x003C15BD)
#define kdlsym_addr_copyout                                            (0x003C16B0)
#define kdlsym_addr_copyout_patch1                                     (0x003C1702)
#define kdlsym_addr_copyout_patch2                                     (0x003C170E)
#define kdlsym_addr_copyin                                             (0x003C17A0)
#define kdlsym_addr_copyin_patch1                                      (0x003C17F7)
#define kdlsym_addr_copyin_patch2                                      (0x003C1803)
#define kdlsym_addr_copyinstr                                          (0x003C1C50)
#define kdlsym_addr_copyinstr_patch1                                   (0x003C1CA3)
#define kdlsym_addr_copyinstr_patch2                                   (0x003C1CAF)
#define kdlsym_addr_faultin                                            (0x003E0410)
#define kdlsym_addr_eventhandler_register                              (0x00402E80)
#define kdlsym_addr_eventhandler_deregister                            (0x00403220)
#define kdlsym_addr_eventhandler_find_list                             (0x00403420)
#define kdlsym_addr_proc_reparent                                      (0x004066F0)
#define kdlsym_addr_sched_sleep                                        (0x00445440)
#define kdlsym_addr_index                                              (0x0044C010)
#define kdlsym_addr_vmspace_alloc                                      (0x0044C710)
#define kdlsym_addr_vmspace_free                                       (0x0044C9C0)
#define kdlsym_addr_vmspace_acquire_ref                                (0x0044CB90)
#define kdlsym_addr__vm_map_lock                                       (0x0044CBF0)
#define kdlsym_addr__vm_map_unlock                                     (0x0044CC60)
#define kdlsym_addr__vm_map_lock_read                                  (0x0044CD40)
#define kdlsym_addr__vm_map_unlock_read                                (0x0044CD90)
#define kdlsym_addr_vm_map_insert                                      (0x0044DEF0)
#define kdlsym_addr_vm_map_fixed                                       (0x0044FF80)
#define kdlsym_addr_vm_map_stack                                       (0x004565E0)
#define kdlsym_addr_icc_nvs_write                                      (0x00464290)
#define kdlsym_addr_icc_nvs_read                                       (0x00464450)
#define kdlsym_addr_namei                                              (0x0048F630)
#define kdlsym_addr_NDFREE                                             (0x004910D0)
#define kdlsym_addr__mtx_lock_flags                                    (0x00496540)
#define kdlsym_addr__mtx_lock_sleep                                    (0x004965E0)
#define kdlsym_addr__mtx_unlock_flags                                  (0x00496810)
#define kdlsym_addr__mtx_unlock_sleep                                  (0x00496910)
#define kdlsym_addr__mtx_lock_spin_flags                               (0x00496970)
#define kdlsym_addr__mtx_unlock_spin_flags                             (0x00496B30)
#define kdlsym_addr__thread_lock_flags                                 (0x00496C90)
#define kdlsym_addr_mtx_init                                           (0x00496FE0)
#define kdlsym_addr_sceRegMgrSetInt                                    (0x005077D0)
#define kdlsym_addr_sceRegMgrGetInt                                    (0x00508A60)
#define kdlsym_addr_sceSblDriverMapPages                               (0x00638310)
#define kdlsym_addr_sceSblDriverUnmapPages                             (0x00638A00)
#define kdlsym_addr_sceSblServiceCrypt                                 (0x0063E680)
#define kdlsym_addr_sceSblPfsSaveDataSignHeader                        (0x00641450)
#define kdlsym_addr_sceSblPfsSetKeys                                   (0x00641520)
#define kdlsym_addr_sceSblPfsKeymgrGenKeys                             (0x00649440)
#define kdlsym_addr_sceSblKeymgrClearKey                               (0x00649B80)
#define kdlsym_addr_sceSblKeymgrHoldKey                                (0x00649EC0)
#define kdlsym_addr_sceSblKeymgrDropKey                                (0x0064A2F0)
#define kdlsym_addr_sceSblKeymgrAllocateKeyId                          (0x0064A500)
#define kdlsym_addr_sceSblKeymgrFreeKeyId                              (0x0064A5A0)
#define kdlsym_addr_sceSblAuthMgrAddEEkc3                              (0xE065C5F0)
#define kdlsym_addr_sceSblAuthMgrDeleteEEkc                            (0x0065C870)
#define kdlsym_addr_sceSblAuthMgrAddEEkc                               (0x0065CB40)
#define kdlsym_addr_sceSblAuthMgrAddEEkc2                              (0x0065CDA0)
#define kdlsym_addr_sceSblNpDrmDecryptDiscRif                          (0x006693A0)
#define kdlsym_addr_sceSblNpDrmDecryptRifNew                           (0x0066A180)
#define kdlsym_addr_avcontrol_sleep                                    (0x00700E50)
#define kdlsym_addr_vm_object_deallocate                               (0x0076F160)
#define kdlsym_addr_sysdump_perform_dump_on_fatal_trap                 (0x00784120)
#define kdlsym_addr_kernel_max_text_size                               (0x0078BA90)
#define kdlsym_addr_prison0                                            (0x0113E518)
#define kdlsym_addr_M_TEMP                                             (0x01540EB0)
#define kdlsym_addr_M_IOV                                              (0x01A87AD0)
#define kdlsym_addr_self_orbis_sysvec                                  (0x01A8A398)
#define kdlsym_addr_maxtsiz                                            (0x01BB6A60)
#define kdlsym_addr_maxssiz                                            (0x01BB6A80)
#define kdlsym_addr_sgrowsiz                                           (0x01BB6A88)
#define kdlsym_addr_global_settings_base                               (0x01BD7FD0)
#define kdlsym_addr_kernel_map                                         (0x0220DFC0)
#define kdlsym_addr_allproc_lock                                       (0x022BBE20)
#define kdlsym_addr_proctree_lock                                      (0x022BBE40)
#define kdlsym_addr_allproc                                            (0x022BBE80)
#define kdlsym_addr_usb_devclass_ptr                                   (0x022BF2D8)
#define kdlsym_addr_initproc                                           (0x022C0818)
#define kdlsym_addr_rootvnode                                          (0x02300320)
#define kdlsym_addr_vsnprintf                                          (0x00123630)
#define kdlsym_addr_destroy_dev                                        (0x003BDCB0)
#define kdlsym_addr_make_dev_p                                         (0x003BD770)
#define kdlsym_addr_gdt                                                (0x01BBCA10)
#define kdlsym_addr_vm_fault_disable_pagefaults                        (0x000C0BB0)
#define kdlsym_addr_vm_fault_enable_pagefaults                         (0x000C0BE0)
#define kdlsym_addr_mtx_destroy                                        (0x00497050)
#define kdlsym_addr_proc0                                              (0x022BFA40)
#define kdlsym_addr_kthread_suspend                                    (0x0008AA40)
#define kdlsym_addr_kern_openat                                        (0x0049E9F0)
#define kdlsym_addr_kern_ioctl                                         (0x0039C5C0)
#define kdlsym_addr_sceSblRngGetRandomNumber                           (0x00665230)
#define kdlsym_addr_SceSblSysVeriThread                                (0x0063C950)
#define kdlsym_addr_lim_cur                                            (0x002CD510)
#define kdlsym_addr_icc_nvs_write                                      (0x00464290)
#define kdlsym_addr_sceSblAuthMgrIsLoadable2                           (0x0065D7A0)
#define kdlsym_addr_sceSblServiceMailbox                               (0x0064CC20)
#define kdlsym_addr_sceSblAuthMgrVerifyHeader                          (0x0065D800)
#define kdlsym_addr_gpu_va_page_list                                   (0x0266AC68)
#define kdlsym_addr__sceSblAuthMgrGetSelfInfo                          (0x0065E010)
#define kdlsym_addr__sceSblAuthMgrSmStart                              (0x0065E490)
#define kdlsym_addr__sceSblAuthMgrSmLoadSelfBlock                      (0x00660AF0)
#define kdlsym_addr__sceSblAuthMgrSmLoadSelfSegment                    (0x006606A0)
#define kdlsym_addr_mini_syscore_self_binary                           (0x0156A588)
#define kdlsym_addr_ctxStatus                                          (0x026B4130)
#define kdlsym_addr_ctxTable                                           (0x026B4140)
#define kdlsym_addr_s_taskId                                           (0x026B4310)
#define kdlsym_addr__sceSblAuthMgrSmFinalize                           (0x0065FF70)
#define kdlsym_addr_s_sm_sxlock                                        (0x026B40C8)
#define kdlsym_addr_kern_thr_create                                    (0x004A6FB0)
#define kdlsym_addr_killproc                                           (0x0002DC80)
#define kdlsym_addr_kernel_mount                                       (0x00442F90)
#define kdlsym_addr_mount_argf                                         (0x00442DE0)
#define kdlsym_addr_sbl_keymgr_key_rbtree                              (0x02694580)
#define kdlsym_addr_sbl_keymgr_key_slots                               (0x02694570)
#define kdlsym_addr_sceSblKeymgrSetKeyStorage                          (0x00646E00)
#define kdlsym_addr_sbl_keymgr_buf_va                                  (0x02698000)
#define kdlsym_addr_sbl_keymgr_buf_gva                                 (0x02698808)
#define kdlsym_addr_strstr                                             (0x004817F0)
#define kdlsym_addr_vn_fullpath                                        (0x002F0C40)
#define kdlsym_addr_dynlib_do_dlsym                                    (0x00417960)
#define kdlsym_addr_name_to_nids                                       (0x00417C40)
#define kdlsym_addr_strdup                                             (0x002504C0)
#define kdlsym_addr_realloc                                            (0x0000DAD0)
#define kdlsym_addr_M_MOUNT                                            (0x01A90CA0)
#define kdlsym_addr__sx_init_flags                                     (0x00042450)
#define kdlsym_addr__mtx_unlock_flags                                  (0x00496810)
#define kdlsym_addr__mtx_lock_flags                                    (0x00496540)
#define kdlsym_addr_sched_prio                                         (0x004453C0)

// Kernel Hooks
#define kdlsym_addr_printf_hook                                        (0x01A9FE98)

// self function patches
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookA                    0x659AC6
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookB                    0x65A758
#define kdlsym_addr_sceSblAuthMgrIsLoadable2_hook                      0x65930F
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook        0x661571
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook      0x66092A

// fake package patches
#define kdlsym_addr_fpu_kern_enter                                     (0x0036B6E0)
#define kdlsym_addr_fpu_kern_ctx                                       (0x02694080)
#define kdlsym_addr_Sha256Hmac                                         (0x00335B70)
#define kdlsym_addr_fpu_kern_leave                                     (0x0036B7D0)
#define kdlsym_addr_AesCbcCfb128Decrypt                                (0x003C0550)
#define kdlsym_addr_AesCbcCfb128Encrypt                                (0x003C0320)

#define kdlsym_addr_npdrm_decrypt_isolated_rif__sceSblKeymgrSmCallfunc_hook        0x669500
#define kdlsym_addr_npdrm_decrypt_rif_new__sceSblKeymgrSmCallfunc_hook             0x66A313
#define kdlsym_addr_sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg_hook            0x646EA5
#define kdlsym_addr_sceSblKeymgrInvalidateKey__sx_xlock_hook                  0x0064AA3D
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookA                              0x6CDF15
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookB                              0x6CE141
#define kdlsym_addr_RsaesPkcs1v15Dec2048CRT                            0x1D6050
#define kdlsym_addr_sbl_pfs_sx                                         0x2679040
#define kdlsym_addr_sceSblKeymgrSetKeyForPfs                           0x649800
#define kdlsym_addr_sceSblKeymgrSmCallfunc                             0x6493D0
#define kdlsym_addr_sceSblDriverSendMsg                                0x637AE0
#define kdlsym_addr_memmove                                             0x003EBB00
#define kdlsym_addr_sceRegMgrGetStr                                             0x00509220
#define kdlsym_addr_sceRegMgrSetBin                                             0x005092F0
#define kdlsym_addr_sceRegMgrGetBin                                            0x005093A0
//#define kdlsym_addr_sceRegMgrSetStr                                             0x00//3EBB00
#define kdlsym_addr__thread_lock_flags                                 (0x00496C90)
#define kdlsym_addr_spinlock_exit                                      (0x000A35F0)
#define kdlsym_addr_setidt                                             (0x000A1B90)
#define kdlsym_addr_map_chunk_table                                    (0x0064BB50)
#define kdlsym_addr_make_chunk_table_system                            (0x0064BD20)
#define kdlsym_addr_sbl_drv_msg_mtx                                    (0x0266AC70)
#define kdlsym_addr__thread_lock_flags                                 (0x00496C90)
// user land offsets
#define	SceSysCore_addr_ptrace_patch                                   (0x00018D6B)

// SceShellCore patches - call sceKernelIsGenuineCEX
#define ssc_sceKernelIsGenuineCEX_patchA                   0x189602
#define ssc_sceKernelIsGenuineCEX_patchB                   0x835642
#define ssc_sceKernelIsGenuineCEX_patchC                   0x880492
#define ssc_sceKernelIsGenuineCEX_patchD                   0xA12B92


// SceShellCore patches - call sceKernelIsAssistMode for Testkits
#define ssc_sceKernelIsAssistMode_patchA                  0x189630 



// SceShellCore patches - call nidf_libSceDipsw
#define ssc_nidf_libSceDipsw_patchA                        0x189630
#define ssc_nidf_libSceDipsw_patchB                        0x254107
#define ssc_nidf_libSceDipsw_patchC                        0x835670
#define ssc_nidf_libSceDipsw_patchD                        0xA12BC0

// SceShellCore patches - enable VR without spoof
#define ssc_enable_vr                                      0x00DDDD70

// SceShellCore patches - use free prefix instead fake
#define ssc_fake_to_free_patch                             0xFD2BF1

#pragma clang diagnostic pop

#endif
