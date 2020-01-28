#pragma once

#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_501

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

// SceShellCore patches - use free prefix instead fake
#define ssc_fake_to_free_patch                             0x00EA7B67

#endif
