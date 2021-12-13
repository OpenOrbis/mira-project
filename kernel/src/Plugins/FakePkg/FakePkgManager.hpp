/*
    Implemented from: https://github.com/xvortex/ps4-hen-vtx
    Ported by: kiwidog (@kd_tech_)

    Bugfixes: SiSTRo (https://github.com/SiSTR0), SocraticBliss (https://github.com/SocraticBliss)
*/

#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>
#include <External/subhook/subhook.h>

#include <OrbisOS/FakeStructs.hpp>

extern "C"
{
    #include <sys/eventhandler.h>
};

namespace Mira
{
    namespace Plugins
    {
        class FakePkgManager :
            public Mira::Utils::IModule
        {
        private:
            subhook_t m_NpdrmDecryptIsolatedRifHook;
            subhook_t m_NpdrmDecryptRifNewHook;
            subhook_t m_SceSblDriverSendMsgHook;
            subhook_t m_SceSblKeymgrInvalidateKey;
            subhook_t m_SceSblPfsSetKeysHook;

            eventhandler_entry* m_processStartEvent;
            eventhandler_entry* m_resumeEvent;

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
            virtual bool OnLoad();

        private:
            // Helper functions
            static void GenPfsCryptoKey(uint8_t* p_EncryptionKeyPFS, uint8_t p_Seed[OrbisOS::PFS_SEED_SIZE], uint32_t p_Index, uint8_t p_Key[OrbisOS::PFS_FINAL_KEY_SIZE]);
            static void GenPfsEncKey(uint8_t* p_EncryptionKeyPFS, uint8_t p_Seed[OrbisOS::PFS_SEED_SIZE], uint8_t p_Key[OrbisOS::PFS_FINAL_KEY_SIZE]);
            static void GenPfsSignKey(uint8_t* p_EncryptionKeyPFS, uint8_t p_Seed[OrbisOS::PFS_SEED_SIZE], uint8_t p_Key[OrbisOS::PFS_FINAL_KEY_SIZE]);
            static int DecryptNpdrmDebugRif(uint32_t p_Type, uint8_t* p_Data);
            static OrbisOS::SblMapListEntry* SceSblDriverFindMappedPageListByGpuVa(vm_offset_t p_GpuVa);
            static vm_offset_t SceSblDriverGpuVaToCpuVa(vm_offset_t p_GpuVa, size_t* p_NumPageGroups);

            static int SceSblPfsSetKeys(uint32_t* p_Ekh, uint32_t* p_Skh, uint8_t* p_EekPfs, OrbisOS::Ekc* p_Eekc, uint32_t p_PubKeyVersion, uint32_t p_KeyVersion, OrbisOS::PfsHeader* p_Header, size_t p_HeaderSize, uint32_t p_Type, uint32_t p_Finalized, uint32_t p_IsDisc);
            static OrbisOS::SblKeyRbtreeEntry* sceSblKeymgrGetKey(unsigned int p_Handle);

        protected:
            // Hooked Functions
            static int OnSceSblPfsSetKeys(uint32_t* p_Ekh, uint32_t* p_Skh, uint8_t* p_EekPfs, OrbisOS::Ekc* p_Eekc, uint32_t p_PubKeyVersion, uint32_t p_KeyVersion, OrbisOS::PfsHeader* p_Header, size_t p_HeaderSize, uint32_t p_Type, uint32_t p_Finalized, uint32_t p_IsDisc);
            static int OnNpdrmDecryptIsolatedRif(OrbisOS::KeymgrPayload* p_Payload);
            static int OnNpdrmDecryptRifNew(OrbisOS::KeymgrPayload* p_Payload);
            static int OnSceSblDriverSendMsg(OrbisOS::SblMsg* p_Message, size_t p_Size) __attribute__ ((optnone));
            static int OnSceSblKeymgrInvalidateKeySxXlock(struct sx* p_Sx, int p_Opts, const char* p_File, int p_Line);

            static bool ShellCorePatch();
            static bool ShellUIPatch();
            static void ResumeEvent();
            static void ProcessStartEvent(void *arg, struct ::proc *p);

            };
    }
}