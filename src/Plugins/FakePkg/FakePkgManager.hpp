#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>
#include <Utils/Hook.hpp>

namespace Mira
{
    namespace Plugins
    {
        class FakePkgManager :
            public Mira::Utils::IModule
        {
        private:
            enum 
            { 
                // RIF
                RifKeySize = 0x10,

                // PFS
                PfsSeedSize = 0x10,
                PfsFinalKeySize = 0x20
            };

            typedef enum _KeyIndex
            {
                // This is by-default, when we ask the secure coprocessor to handle things
                SonyRifKey = -1,

                // This is the public homebrew key (used in hen, homebrew, fpkg)
                HomebrewRifKey,

                // This is a static homebrew rif key seed (used in hen, homebrew, fpkg)
                HomebrewRifKeySeed,

                // This is a downloadable key from the online backend for your account
                MiraRifKey,

                // This is a generated key from using console information (this never leaves the console)
                LocalRifKey,

                // Reserved for future use
                Reserved,

                // Total key counts
                KeyCount
            } KeyIndex;

            // The rif key is 16 bytes in length
            typedef struct _Key
            {
                uint8_t Bytes[RifKeySize];
            } Key;

            Key m_Keys[KeyIndex::KeyCount];

            Utils::Hook* m_NpdrmDecryptIsolatedRifHook;
            Utils::Hook* m_NpdrmDecryptRifNewHook;
            Utils::Hook* m_SceSblDriverSendMsgHook;
            Utils::Hook* m_SceSblKeymgrInvalidateKey;
            Utils::Hook* m_SceSblPfsSetKeysHook;
            
        public:
            FakePkgManager();
            virtual ~FakePkgManager();

            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

        protected:
            static inline void GeneratePfsCryptoKey(uint8_t* p_EKPfs, uint8_t p_Seed[PfsSeedSize], uint32_t p_Index, uint8_t p_Key[PfsFinalKeySize]);
            static inline void GeneratePfsEncKey(uint8_t* p_EKPfs, uint8_t p_Seed[PfsSeedSize], uint8_t p_Key[PfsFinalKeySize]);
            static inline void GeneratePfsSignKey(uint8_t* p_EKPfs, uint8_t p_Seed[PfsSeedSize], uint8_t p_Key[PfsFinalKeySize]);

            bool IsKeyFilled(KeyIndex p_Index);

            // Hooked Functions
            static int OnSceSblPfsSetKeys(uint32_t* p_Ekh, uint32_t* p_Skh, uint8_t* p_EekPfs, struct ekc* p_Eekc, uint32_t p_PubKeyVersion, uint32_t p_KeyVersion, struct pfs_header* p_Header, size_t p_HeaderSize, uint32_t p_Type, uint32_t p_Finalized, uint32_t p_IsDisc);
            static int OnNpdrmDecryptIsolatedRif(union keymgr_payload* p_Payload);
            static int OnNpdrmDecryptRifNew(union keymgr_payload* p_Payload);
            static int OnSceSblDriverSendMsg(struct sbl_msg* p_Message, size_t p_Size);
        };
    }
}