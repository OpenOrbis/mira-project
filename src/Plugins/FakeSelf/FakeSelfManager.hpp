#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>
#include <Utils/Hook.hpp>

#include "FakeSelfStructs.hpp"

extern "C"
{
    #include <sys/elf.h>
    #include <vm/vm.h>
};



namespace Mira
{
    namespace Plugins
    {
        class FakeSelfManager : 
            public Mira::Utils::IModule
        {
        private:
            enum
            {
                LoadSelfSegment = 2,
                LoadSelfBlock = 6,

                SelfMagic = 0x1D3D154F,
                ElfMagic = 0x464C457F,

                SelfPtypeFake = 1,
            };

            Utils::Hook* m_SceSblACMgrGetPathIdHook;
            Utils::Hook* m_SceSblServiceMailboxHook;
            Utils::Hook* m_SceSblAuthMgrVerifyHeaderHook;
            Utils::Hook* m_SceSblAuthMgrIsLoadable2Hook;

            uint8_t m_MailboxRequest[0x80];

        public:
            FakeSelfManager();
            virtual ~FakeSelfManager();

            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

        private:
            int AuthSelfHeader(struct self_context* p_Context);

            // This calls the original sceSblServiceMailbox disabling the hooks calling, reenabling hooks
            static int SceSblServiceMailbox(uint32_t p_ServiceId, void* p_Request, void* p_Response);
            static int SceSblAuthMgrSmLoadSelfBlock(uint32_t p_ServiceId, void* p_Request, void* p_Response);
            static int SceSblAuthMgrSmLoadSelfSegment(uint32_t p_ServiceId, void* p_Request, void* p_Response);

            static SblMapListEntry* SceSblDriverFindMappedPageListByGpuVa(vm_offset_t p_GpuVa);
            static vm_offset_t SceSblDriverGpuVaToCpuVa(vm_offset_t p_GpuVa, size_t* p_NumPageGroups);
            static bool IsFakeSelf(SelfContext* p_Context);
            static int SceSblAuthMgrGetElfHeader(SelfContext* p_Context, Elf64_Ehdr** p_OutElfHeader);
            static int BuildFakeSelfAuthInfo(SelfContext* p_Context, SelfAuthInfo* p_ParentAuthInfo, SelfAuthInfo* p_AuthInfo);

            static int SceSblAuthMgrGetSelfAuthInfoFake(SelfContext* p_Context, SelfAuthInfo* p_Info);
            
        protected:
            static int OnSceSblACMgrGetPathId(const char* p_Path);

            static int OnSceSblServiceMailbox(uint32_t p_ServiceId, void* p_Request, void* p_Response);
            static int OnSceSblAuthMgrVerifyHeader(struct self_context* p_Context);
            static int OnSceSblAuthMgrIsLoadable2(struct self_context* p_Context, struct self_auth_info* p_OldAuthInfo, int32_t p_PathId, struct self_auth_info* p_NewAuthInfo);
            //static int OnAuthSelfHeader(struct self_context* p_Context);
        };
    }
}
