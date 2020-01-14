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

                AuthInfoSize = 136,
            };

            //Utils::Hook* m_SceSblACMgrGetPathIdHook;
            Utils::Hook* m_SceSblServiceMailboxHook;
            Utils::Hook* m_SceSblAuthMgrVerifyHeaderHook;
            Utils::Hook* m_SceSblAuthMgrIsLoadable2Hook;

            Utils::Hook* m__SceSblAuthMgrSmLoadSelfSegmentHook;
            Utils::Hook* m__SceSblAuthMgrSmLoadSelfBlockHook;

            static SelfContext* m_LastContext;

            static const uint8_t c_ExecAuthInfo[AuthInfoSize];
            static const uint8_t c_DynlibAuthInfo[AuthInfoSize];

        public:
            FakeSelfManager();
            virtual ~FakeSelfManager();

            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

        private:
            //
            // Helper Functions
            static int AuthSelfHeader(SelfContext* p_Context);
            static int SceSblAuthMgrSmLoadSelfBlock_Mailbox(uint32_t p_ServiceId, void* p_Request, void* p_Response);
            static int SceSblAuthMgrSmLoadSelfSegment_Mailbox(uint32_t p_ServiceId, void* p_Request, void* p_Response);
            static SblMapListEntry* SceSblDriverFindMappedPageListByGpuVa(vm_offset_t p_GpuVa);
            static vm_offset_t SceSblDriverGpuVaToCpuVa(vm_offset_t p_GpuVa, size_t* p_NumPageGroups);
            static bool IsFakeSelf(SelfContext* p_Context);
            static int BuildFakeSelfAuthInfo(SelfContext* p_Context, SelfAuthInfo* p_ParentAuthInfo, SelfAuthInfo* p_AuthInfo);
            static int SceSblAuthMgrGetSelfAuthInfoFake(SelfContext* p_Context, SelfAuthInfo* p_Info);
            static int SceSblAuthMgrGetElfHeader(SelfContext* p_Context, Elf64_Ehdr** p_OutElfHeader);

        

            //
            // Hook Helper Functions
            // This calls the original functions disabling the hooks, calling, reenabling hooks
            static int SceSblServiceMailbox(uint32_t p_ServiceId, void* p_Request, void* p_Response);
            static int SceSblAuthMgrIsLoadable2(SelfContext* p_Context, SelfAuthInfo* p_OldAuthInfo, int32_t p_PathId, SelfAuthInfo* p_NewAuthInfo);
            static int SceSblAuthMgrVerifyHeader(SelfContext* p_Context);

            // 1.76 __int64 __fastcall sceSblAuthMgrSmLoadSelfBlock(__int64 a1, unsigned int a2, unsigned int a3, char *a4, char *a5, char *a6, void* a7, void* a8, void* a9, void* a10, int (__fastcall *a11)(signed __int64, __int64), __int64 a12)
            // 6.72 __int64 __fastcall sceSblAuthMgrSmLoadSelfBlock(SelfContext* a1, unsigned int a2, unsigned int a3, char *a4, char *a5, char* a6, void* a7, void* a8, void* a9, void* a10, void* a11)
            // 5.05 __int64 __fastcall sceSblAuthMgrSmLoadSelfBlock(__int64 a1, unsigned int a2, unsigned int a3, char *a4, char *a5, char* a6, void* a7, void* a8, void* a9, void* a10, void* a11)
            static int _SceSblAuthMgrSmLoadSelfBlock(SelfContext* p_Context, unsigned int a2, unsigned int a3, char *a4, char *a5, char* a6, void* a7, void* a8, void* a9, void* a10, void* a11);
            static int _SceSblAuthMgrSmLoadSelfSegment(SelfContext *p_Context, uint32_t a2, int32_t a3, uint8_t *a4, uint64_t a5, int64_t (*a6)(uint64_t, char **, int64_t), int64_t a7, int64_t (*a8)(uint64_t, int64_t), int64_t a9);

            
        protected:
            //
            // Hooked function callbacks
            //static int OnSceSblACMgrGetPathId(const char* p_Path);
            static int OnSceSblServiceMailbox(uint32_t p_ServiceId, void* p_Request, void* p_Response);
            static int OnSceSblAuthMgrVerifyHeader(SelfContext* p_Context);
            static int OnSceSblAuthMgrIsLoadable2(SelfContext* p_Context, SelfAuthInfo* p_OldAuthInfo, int32_t p_PathId, SelfAuthInfo* p_NewAuthInfo);

            static int On_SceSblAuthMgrSmLoadSelfBlock(SelfContext* p_Context, unsigned int a2, unsigned int a3, char *a4, char *a5, char* a6, void* a7, void* a8, void* a9, void* a10, void* a11);
            static int On_SceSblAuthMgrSmLoadSelfSegment(SelfContext *p_Context, uint32_t a2, int32_t a3, uint8_t *a4, uint64_t a5, int64_t (*a6)(uint64_t, char **, int64_t), int64_t a7, int64_t (*a8)(uint64_t, int64_t), int64_t a9);
        };
    }
}
