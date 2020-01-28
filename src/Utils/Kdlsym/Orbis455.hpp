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
#define kdlsym_addr__mtx_unlock_flags                      0x0030D940
#define kdlsym_addr__mtx_unlock_sleep                      0x0030DA10
#define kdlsym_addr__sceSblAuthMgrGetSelfInfo              0x00626490
#define kdlsym_addr__sx_slock                              0x0038F980
#define kdlsym_addr__sx_sunlock                            0x0038FB00
#define kdlsym_addr__sx_xlock                              0x0038FA30
#define kdlsym_addr__sx_xunlock                            0x0038FBC0
#define kdlsym_addr__vm_map_lock_read                      0x003920B0
#define kdlsym_addr__vm_map_unlock_read                    0x00392100
#define kdlsym_addr_AesCbcCfb128Decrypt                    0x0017A6F0
#define kdlsym_addr_AesCbcCfb128Encrypt                    0x0017A4C0
#define kdlsym_addr_allproc                                0x01AD7718
#define kdlsym_addr_allproc_lock                           0x01AD76B8
#define kdlsym_addr_copyin                                 0x0014A890
#define kdlsym_addr_copyinstr                              0x0014AD00
#define kdlsym_addr_critical_enter                         0x0023D560
#define kdlsym_addr_critical_exit                          0x0023D570
//#define kdlsym_addr_dmem_start_app_process                 0x0
#define kdlsym_addr_eventhandler_register                  0x003C97F0
#define kdlsym_addr_exec_new_vmspace                       0x002E8850
#define kdlsym_addr_faultin                                0x0030EE10
#define kdlsym_addr_fget_unlocked                          0x0042D160
#define kdlsym_addr_fpu_kern_ctx                           0x0251CCC0
#define kdlsym_addr_fpu_kern_enter                         0x00059580
#define kdlsym_addr_fpu_kern_leave                         0x00059680
#define kdlsym_addr_free                                   0x003F7930
#define kdlsym_addr_gpu_va_page_list                       0x02519DD0
#define kdlsym_addr_icc_nvs_read                           0x0001B850
#define kdlsym_addr_kern_close                             0x0042AC10
#define kdlsym_addr_kern_mkdirat                           0x00444E60
#define kdlsym_addr_kern_open                              0x0043FD00
#define kdlsym_addr_kern_openat                            0x0043FD60
#define kdlsym_addr_kern_readv                             0x0005ED30
#define kdlsym_addr_kern_reboot                            0x000998A0
#define kdlsym_addr_kern_sysents                           0x0102B690
#define kdlsym_addr_kern_thr_create                        0x002ECCD0
#define kdlsym_addr_kernel_map                             0x01B31218
#define kdlsym_addr_kmem_alloc                             0x0016ECD0
#define kdlsym_addr_kmem_free                              0x0016EEA0
#define kdlsym_addr_kproc_create                           0x00464700
#define kdlsym_addr_kthread_add                            0x00464C90
#define kdlsym_addr_kthread_exit                           0x00464F60
#define kdlsym_addr_M_MOUNT                                0x0144F2A0
#define kdlsym_addr_M_TEMP                                 0x01993B30
#define kdlsym_addr_malloc                                 0x003F7750
#define kdlsym_addr_memcmp                                 0x00242A60
#define kdlsym_addr_memcpy                                 0x0014A6B0
#define kdlsym_addr_memmove                                0x002EE740
#define kdlsym_addr_memset                                 0x00302BD0
#define kdlsym_addr_mini_syscore_self_binary               0x01471468
#define kdlsym_addr_mtx_init                               0x0030E0C0
#define kdlsym_addr_mtx_lock_sleep                         0x0030D710
#define kdlsym_addr_mtx_unlock_sleep                       0x0030DA10
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
#define kdlsym_addr_sbl_pfs_sx                             0x0
#define kdlsym_addr_sceSblAuthMgrIsLoadable2               0x00625C50
#define kdlsym_addr__sceSblAuthMgrSmStart                  0x00622020
#define kdlsym_addr_sceSblAuthMgrSmVerifyHeader            0x00623250
#define kdlsym_addr_sceSblAuthMgrVerifyHeader              0x00625CB0
#define kdlsym_addr_sceSblDriverSendMsg                    0x00601670
#define kdlsym_addr_sceSblGetEAPInternalPartitionKey       0x00609D20
#define kdlsym_addr_sceSblKeymgrClearKey                   0x0060DF40
#define kdlsym_addr_sceSblKeymgrSetKeyForPfs               0x0060DD70
#define kdlsym_addr_sceSblKeymgrSmCallfunc                 0x0060E680
#define kdlsym_addr_sceSblPfsSetKeys                       0x00606E00
#define kdlsym_addr_sceSblServiceMailbox                   0x006146C0
#define kdlsym_addr_sceSblACMgrGetPathId                   0x0016A5E0
#define kdlsym_addr_self_orbis_sysvec                      0x01460610
#define kdlsym_addr_Sha256Hmac                             0x002D5C50
#define kdlsym_addr_snprintf                               0x00018230
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
#define kdlsym_addr_vm_map_lookup_entry                    0x00392C70
#define kdlsym_addr_vmspace_acquire_ref                    0x00391EE0
#define kdlsym_addr_vmspace_alloc                          0x00391A70
#define kdlsym_addr_vmspace_free                           0x00391D10
#define kdlsym_addr_vsnprintf                              0x000182D0
#define kdlsym_addr_Xfast_syscall                          0x003095D0
#define kdlsym_addr_wakeup                                 0x00261140

// FakeSelf hooks
#define kdlsym_addr_sceSblAuthMgrIsLoadable__sceSblACMgrGetPathId_hook        0x0061F0FC
#define kdlsym_addr_sceSblAuthMgrIsLoadable2_hook                             0x0
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfSegment__sceSblServiceMailbox_hook 0x0
#define kdlsym_addr_sceSblAuthMgrSmLoadSelfBlock__sceSblServiceMailbox_hook   0x0
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookA                           0x0
#define kdlsym_addr_sceSblAuthMgrVerifyHeader_hookB                           0x0

// FakePkg hooks
#define kdlsym_addr_sceSblKeymgrSetKeyStorage__sceSblDriverSendMsg_hook       0x0
#define kdlsym_addr_sceSblKeymgrInvalidateKey__sx_xlock_hook                  0x0
#define kdlsym_addr_npdrm_decrypt_isolated_rif__sceSblKeymgrSmCallfunc_hook   0x0
#define kdlsym_addr_npdrm_decrypt_rif_new__sceSblKeymgrSmCallfunc_hook        0x0
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookA                          0x0
#define kdlsym_addr_mountpfs__sceSblPfsSetKeys_hookB                          0x0

#endif
