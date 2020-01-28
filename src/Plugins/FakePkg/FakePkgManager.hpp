#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>
#include <Utils/Hook.hpp>

#include <OrbisOS/FakeStructs.hpp>

#if defined(MIRA_UNSUPPORTED_PLATFORMS)
#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_176
    #include "Patches/Patches176.hpp"
    #elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_405
    #include "Patches/Patches405.hpp"
    #elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_455
    #include "Patches/Patches455.hpp"
    #elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_474
    #include "Patches/Patches474.hpp"
    #elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_501
    #include "Patches/Patches501.hpp"
    #elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_505
    #include "Patches/Patches505.hpp"
    #elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_620
    #include "Patches/Patches620.hpp"
    #elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_650
    #include "Patches/Patches650.hpp"
    #endif
#endif

namespace Mira
{
    namespace Plugins
    {
        class FakePkgManager :
            public Mira::Utils::IModule
        {
        private:
            Utils::Hook* m_NpdrmDecryptIsolatedRifHook;
            Utils::Hook* m_NpdrmDecryptRifNewHook;
            Utils::Hook* m_SceSblDriverSendMsgHook;
            Utils::Hook* m_SceSblKeymgrInvalidateKey;
            Utils::Hook* m_SceSblPfsSetKeysHook;

            static const uint8_t g_ypkg_p[0x80];
            static const uint8_t g_ypkg_q[0x80];
            static const uint8_t g_ypkg_dmp1[0x80];
            static const uint8_t g_ypkg_dmq1[0x80];
            static const uint8_t g_ypkg_iqmp[0x80];

            static const uint8_t g_RifDebugKey[0x10];
            static const uint8_t g_FakeKeySeed[0x10];
            
        public:
            FakePkgManager();
            virtual ~FakePkgManager();

            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

        private:
            // Helper functions
            static void GenPfsCryptoKey(uint8_t* p_EncryptionKeyPFS, uint8_t p_Seed[OrbisOS::PFS_SEED_SIZE], uint32_t p_Index, uint8_t p_Key[OrbisOS::PFS_FINAL_KEY_SIZE]);
            static void GenPfsEncKey(uint8_t* p_EncryptionKeyPFS, uint8_t p_Seed[OrbisOS::PFS_SEED_SIZE], uint8_t p_Key[OrbisOS::PFS_FINAL_KEY_SIZE]);
            static void GenPfsSignKey(uint8_t* p_EncryptionKeyPFS, uint8_t p_Seed[OrbisOS::PFS_SEED_SIZE], uint8_t p_Key[OrbisOS::PFS_FINAL_KEY_SIZE]);
            static int DecryptNpdrmDebugRif(uint32_t p_Type, uint8_t* p_Data);
            static OrbisOS::SblMapListEntry* SceSblDriverFindMappedPageListByGpuVa(vm_offset_t p_GpuVa);
            static vm_offset_t SceSblDriverGpuVaToCpuVa(vm_offset_t p_GpuVa, size_t* p_NumPageGroups);

            static int SceSblPfsSetKeys(uint32_t* p_Ekh, uint32_t* p_Skh, uint8_t* p_EekPfs, OrbisOS::Ekc* p_Eekc, uint32_t p_PubKeyVersion, uint32_t p_KeyVersion, OrbisOS::PfsHeader* p_Header, size_t p_HeaderSize, uint32_t p_Type, uint32_t p_Finalized, uint32_t p_IsDisc);

        protected:
            // Hooked Functions
            static int OnSceSblPfsSetKeys(uint32_t* p_Ekh, uint32_t* p_Skh, uint8_t* p_EekPfs, OrbisOS::Ekc* p_Eekc, uint32_t p_PubKeyVersion, uint32_t p_KeyVersion, OrbisOS::PfsHeader* p_Header, size_t p_HeaderSize, uint32_t p_Type, uint32_t p_Finalized, uint32_t p_IsDisc);
            static int OnNpdrmDecryptIsolatedRif(OrbisOS::KeymgrPayload* p_Payload);
            static int OnNpdrmDecryptRifNew(OrbisOS::KeymgrPayload* p_Payload);
            static int OnSceSblDriverSendMsg(OrbisOS::SblMsg* p_Message, size_t p_Size);
        };
    }
}