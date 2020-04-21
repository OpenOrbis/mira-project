#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>
#include <Utils/Hook.hpp>
#include <Driver/CtrlDriver.hpp>

extern "C"
{
    #include <sys/eventhandler.h>
    #include <sys/module.h>
    #include <sys/ioccom.h>
};

struct proc;
struct mtx;

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

// IOCTL Wrapper

#define SUBSTITUTE_IOCTL_BASE   'S'

struct substitute_state_hook {
    int hook_id;
    int result;
    int state;
};

struct substitute_hook_iat {
    int hook_id;
    char nids[0xA00];
    void* hook_function;
};

struct substitute_hook_jmp {
    int hook_id;
    void* original_function;
    void* hook_function;
};

#define SUBSTITUTE_HOOK_IAT _IOC(IOC_INOUT, SUBSTITUTE_IOCTL_BASE, 1, sizeof(struct substitute_hook_iat))
#define SUBSTITUTE_HOOK_JMP _IOC(IOC_INOUT, SUBSTITUTE_IOCTL_BASE, 2, sizeof(struct substitute_hook_jmp))
#define SUBSTITUTE_HOOK_STATE _IOC(IOC_INOUT, SUBSTITUTE_IOCTL_BASE, 3, sizeof(struct substitute_state_hook))


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
            struct mtx hook_mtx;
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
            int DisableHook(struct proc* p, int hook_id);
            int EnableHook(struct proc* p, int hook_id);
            int Unhook(struct proc* p, int hook_id);
            int HookJmp(struct proc* p, void* original_address, void* hook_function);
            int HookIAT(struct proc* p, const char* nids, void* hook_function);
            void CleanupProcessHook(struct proc* p);
            void CleanupAllHook();

            uint64_t FindOffsetFromNids(struct proc* p, const char* nids_to_find);
            void DebugImportTable(struct proc* p);
            void* FindOriginalByNids(struct proc* p, const char* nids);

            static int OnIoctl_HookIAT(struct thread* td, struct substitute_hook_iat* uap);
            static int OnIoctl_HookJMP(struct thread* td, struct substitute_hook_jmp* uap);
            static int OnIoctl_StateHook(struct thread* td, struct substitute_state_hook* uap);
            static int OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);

        protected:
            static void OnProcessStart(void *arg, struct proc *p);
            static void OnProcessExit(void *arg, struct proc *p);
            static Substitute* GetPlugin();
        };
    }
}