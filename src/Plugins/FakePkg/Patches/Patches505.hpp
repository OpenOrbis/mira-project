#pragma once

#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_505

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
#define ssc_fake_to_free_patch                             0x00EA96A7

#endif
