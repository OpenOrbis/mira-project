/*
    Implemented from: https://github.com/xvortex/ps4-hen-vtx
    Ported by: kiwidog (@kd_tech_)

    Bugfixes: SiSTRo (https://github.com/SiSTR0), SocraticBliss (https://github.com/SocraticBliss)
*/

#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>
#include <Utils/Hook.hpp>

#include <OrbisOS/FakeStructs.hpp>

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
            //Utils::Hook* m_SceSblServiceMailboxHook;
            //Utils::Hook* m_SceSblAuthMgrVerifyHeaderHook;
            //Utils::Hook* m_SceSblAuthMgrIsLoadable2Hook;

            //Utils::Hook* m__SceSblAuthMgrSmLoadSelfSegmentHook;
            //Utils::Hook* m__SceSblAuthMgrSmLoadSelfBlockHook;

            //static SelfContext* m_LastContext;

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
            static int AuthSelfHeader(OrbisOS::SelfContext* p_Context);
            static int SceSblAuthMgrSmLoadSelfBlock_Mailbox(uint64_t p_ServiceId, uint8_t* p_Request, void* p_Response);
            static int SceSblAuthMgrSmLoadSelfSegment_Mailbox(uint64_t service_id, void* p_Request, void* p_Response);
            static OrbisOS::SblMapListEntry* SceSblDriverFindMappedPageListByGpuVa(vm_offset_t p_GpuVa);
            static vm_offset_t SceSblDriverGpuVaToCpuVa(vm_offset_t p_GpuVa, size_t* p_NumPageGroups);
            static bool IsFakeSelf(OrbisOS::SelfContext* p_Context);
            static int BuildFakeSelfAuthInfo(OrbisOS::SelfContext* p_Context, OrbisOS::SelfAuthInfo* p_ParentAuthInfo, OrbisOS::SelfAuthInfo* p_AuthInfo);
            static int SceSblAuthMgrGetSelfAuthInfoFake(OrbisOS::SelfContext* p_Context, OrbisOS::SelfAuthInfo* p_Info);
            static int SceSblAuthMgrGetElfHeader(OrbisOS::SelfContext* p_Context, Elf64_Ehdr** p_OutElfHeader);
            static int SceSblAuthMgrIsLoadable_sceSblACMgrGetPathId(const char* path);

            //
            // Hook Helper Functions
            // This calls the original functions disabling the hooks, calling, reenabling hooks
            //static int SceSblServiceMailbox(uint32_t p_ServiceId, void* p_Request, void* p_Response);
            //static int SceSblAuthMgrIsLoadable2(SelfContext* p_Context, SelfAuthInfo* p_OldAuthInfo, int32_t p_PathId, SelfAuthInfo* p_NewAuthInfo);
            //static int SceSblAuthMgrVerifyHeader(SelfContext* p_Context);

            // 1.76 __int64 __fastcall sceSblAuthMgrSmLoadSelfBlock(__int64 a1, unsigned int a2, unsigned int a3, char *a4, char *a5, char *a6, void* a7, void* a8, void* a9, void* a10, int (__fastcall *a11)(signed __int64, __int64), __int64 a12)
            // 6.72 __int64 __fastcall sceSblAuthMgrSmLoadSelfBlock(SelfContext* a1, unsigned int a2, unsigned int a3, char *a4, char *a5, char* a6, void* a7, void* a8, void* a9, void* a10, void* a11)
            // 5.05 __int64 __fastcall sceSblAuthMgrSmLoadSelfBlock(__int64 a1, unsigned int a2, unsigned int a3, char *a4, char *a5, char* a6, void* a7, void* a8, void* a9, void* a10, void* a11)
            // Fixed thanks to the almighty flatz =]
            //static int _SceSblAuthMgrSmLoadSelfBlock(SelfContext* p_Context, uint32_t p_SegmentIndex, uint32_t p_BlockIndex, uint8_t* p_Data, size_t p_Size, int(*p_ReadCallback)(uint64_t /*p_Offset*/, uint8_t* /*p_Data*/, size_t /*p_Size*/), void* p_CallbackArg, void* unk0, void* unk1, void* unk2, void* unk3);
            //static int _SceSblAuthMgrSmLoadSelfSegment(SelfContext *p_Context, uint32_t p_SegmentIndex, bool p_IsBlockTable, uint8_t* p_Data, size_t p_Size, int(*p_ReadCallback)(uint64_t /*p_Offset*/, uint8_t* /*p_Data*/, size_t /*p_Size*/), void* p_CallbackArg, void* unk0, void* unk1);

            //void HookFunctionCall(uint8_t* p_HookTrampoline, void* p_Function, void* p_Address);

        protected:
            //
            // Hooked function callbacks
            //static int OnSceSblACMgrGetPathId(const char* p_Path);
            //static int OnSceSblServiceMailbox(uint32_t p_ServiceId, void* p_Request, void* p_Response);
            static int OnSceSblAuthMgrVerifyHeader(OrbisOS::SelfContext* p_Context);
            static int OnSceSblAuthMgrIsLoadable2(OrbisOS::SelfContext* p_Context, OrbisOS::SelfAuthInfo* p_OldAuthInfo, int32_t p_PathId, OrbisOS::SelfAuthInfo* p_NewAuthInfo);

            //static int On_SceSblAuthMgrSmLoadSelfBlock(SelfContext* p_Context, uint32_t p_SegmentIndex, uint32_t p_BlockIndex, uint8_t* p_Data, size_t p_Size, int(*p_ReadCallback)(uint64_t /*p_Offset*/, uint8_t* /*p_Data*/, size_t /*p_Size*/), void* p_CallbackArg, void* unk0, void* unk1, void* unk2, void* unk3);
            //static int On_SceSblAuthMgrSmLoadSelfSegment(SelfContext *p_Context, uint32_t p_SegmentIndex, bool p_IsBlockTable, uint8_t* p_Data, size_t p_Size, int(*p_ReadCallback)(uint64_t /*p_Offset*/, uint8_t* /*p_Data*/, size_t /*p_Size*/), void* p_CallbackArg, void* unk0, void* unk1);
        };
    }
}
