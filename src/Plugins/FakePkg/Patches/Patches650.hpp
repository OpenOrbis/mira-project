#pragma once

#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_650

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

// SceShellCore patches - use free prefix instead fake
#define ssc_fake_to_free_patch                             0x0

#endif
