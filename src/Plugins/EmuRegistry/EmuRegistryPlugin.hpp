#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>
#include <Utils/Hook.hpp>

namespace Mira
{
    namespace Plugins
    {
        class EmuRegistryPlugin : public Utils::IModule
        {
            public:
            struct _IntEntry
            {
                uint32_t key;
                int32_t value;
            };

            struct _BinEntry
            {
                uint32_t key;
                uint8_t* data;
                uint32_t dataSize;
            };

            struct _StrEntry
            {
                uint32_t key;
                char* str;
                uint32_t strSize;
            };

        private:
            Utils::Hook* m_GetIntHook;
            Utils::Hook* m_SetIntHook;
            Utils::Hook* m_GetBinHook;
            Utils::Hook* m_SetBinHook;
            Utils::Hook* m_GetStrHook;
            Utils::Hook* m_SetStrHook;

        public:
            EmuRegistryPlugin();
            virtual ~EmuRegistryPlugin();

            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

        protected:
            static uint32_t OnSceRegMgrGetInt(uint32_t p_Id, int32_t* p_OutValue);
            static uint32_t OnSceRegMgrSetInt(uint32_t p_Id, int32_t p_Value);

            static uint32_t OnSceRegMgrGetBin(uint32_t p_Id, void* p_Data, uint32_t p_Size);
            static uint32_t OnSceRegMgrSetBin(uint32_t p_Id, void* p_Data, uint32_t p_Size);

            static uint32_t OnSceRegMgrGetStr(uint32_t p_Id, char* p_String, uint32_t p_Size);
            static uint32_t OnSceRegMgrSetStr(uint32_t p_Id, char* p_String, uint32_t p_Size);
        };
    }
}