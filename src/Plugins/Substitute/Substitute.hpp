#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>
#include <Utils/Hook.hpp>

extern "C"
{
    #include <sys/eventhandler.h>
    #include <sys/module.h>
    #include <sys/sx.h>

    static __inline __pure2 void*
    __getds0(void)
    {
        void* ret;

        __asm("movq %%gs:0,%0" : "=r" (ret));
        return (ret);
    }
};

struct proc;
struct sx;

enum HookType {
    HOOKTYPE_IAT,
    HOOKTYPE_JMP
};

typedef struct {
    int id;
    int hook_type;
    struct proc* process;
    void* jmpslot_address; // For IAT
    void* original_function;
    void* hook_function;
    char* backupData; // For JMP
    size_t backupSize; // For JMP
    bool hook_enable;
} SubstituteHook;

namespace Mira
{
    namespace Plugins
    {
        class Substitute : public Utils::IModule
        {
        private:
            // Start / Stop process
            eventhandler_entry* m_processStartHandler;
            eventhandler_entry* m_processEndHandler;

            // Hook management
            struct sx hook_sx;
            SubstituteHook* hook_list;
            int hook_nbr;

        public:
            Substitute();
            virtual ~Substitute();
            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

            int FindAvailableHookID();
            SubstituteHook* GetHookByID(int hook_id);
            SubstituteHook* AllocateNewHook();
            void FreeOldHook(int hook_id);
            int DisableHook(int hook_id);
            int EnableHook(int hook_id);
            int Unhook(int hook_id);
            int HookJmp(struct proc* p, void* original_address, void* hook_function);
            int HookIAT(struct proc* p, const char* nids, void* hook_function);
            void CleanupProcessHook(struct proc* p);

            uint64_t FindOffsetFromNids(struct proc* p, const char* nids_to_find);
            void DebugImportTable(struct proc* p);

        protected:
            static void OnProcessStart(void *arg, struct proc *p);
            static void OnProcessExit(void *arg, struct proc *p);
            static Substitute* GetPlugin();
        };
    }
}