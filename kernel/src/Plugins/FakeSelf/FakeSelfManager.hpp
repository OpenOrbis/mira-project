/*
    Implemented from: https://github.com/xvortex/ps4-hen-vtx
    Ported by: kiwidog (@kd_tech_)

    Bugfixes: SiSTRo (https://github.com/SiSTR0), SocraticBliss (https://github.com/SocraticBliss)
*/

#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>

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

        protected:
            //
            // Hooked function callbacks
            static int OnSceSblAuthMgrVerifyHeader(OrbisOS::SelfContext* p_Context);
            static int OnSceSblAuthMgrIsLoadable2(OrbisOS::SelfContext* p_Context, OrbisOS::SelfAuthInfo* p_OldAuthInfo, int32_t p_PathId, OrbisOS::SelfAuthInfo* p_NewAuthInfo);
        };
    }
}
