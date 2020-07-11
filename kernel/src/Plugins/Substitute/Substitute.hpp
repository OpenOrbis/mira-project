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
    #include <sys/sysproto.h>
    #include <sys/sysent.h>
};

struct proc;
struct mtx;

struct dynlib_load_prx_args_ex {
    char*    path;  // const char *
    uint64_t unk1;   // int *
    uint64_t pRes;   // int *
    uint64_t unk2;   // int *
};

enum HookType {
    HOOKTYPE_IAT,
    HOOKTYPE_JMP
};

#define SUBSTITUTE_MAX_NAME 255 // Max lenght for name
#define SUBSTITUTE_MAIN_MODULE "" // Define the main module
#define SUBSTITUTE_MAX_HOOKS 1000 // Max possible hooks (all process)

enum {
    SUBSTITUTE_IAT_NAME = 0,
    SUBSTITUTE_IAT_NIDS = 1
};

enum {
    SUBSTITUTE_STATE_DISABLE = 0,
    SUBSTITUTE_STATE_ENABLE = 1,
    SUBSTITUTE_STATE_UNHOOK = 2
};

typedef struct {
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
    char name[SUBSTITUTE_MAX_NAME];
    char module_name[SUBSTITUTE_MAX_NAME];
    int flags;
    void* hook_function;
    void* original_function;
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
            SubstituteHook hooks[SUBSTITUTE_MAX_HOOKS];

        public:
            // Syscall hook (Original pointer)
            void* sys_dynlib_load_prx_p;

            Substitute();
            virtual ~Substitute();
            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

            static Substitute* GetPlugin();

            bool IsProcessAlive(struct proc* p_alive);
            SubstituteHook* GetHookByID(int hook_id);
            SubstituteHook* AllocateHook(int* hook_id);
            void FreeHook(int hook_id);
            int DisableHook(struct proc* p, int hook_id);
            int EnableHook(struct proc* p, int hook_id);
            int Unhook(struct proc* p, int hook_id);
            int HookJmp(struct proc* p, void* original_address, void* hook_function);
            int HookIAT(struct proc* p, const char* module_name, const char* name, int32_t flags, void* hook_function, uint64_t* original_function_out);
            void CleanupProcessHook(struct proc* p);
            void CleanupAllHook();

            uint64_t FindJmpslotAddress(struct proc* p, const char* module_name, const char* name, int32_t flags);
            void* FindOriginalAddress(struct proc* p, const char* name, int32_t flags);

            static int OnIoctl_HookIAT(struct thread* td, struct substitute_hook_iat* uap);
            static int OnIoctl_HookJMP(struct thread* td, struct substitute_hook_jmp* uap);
            static int OnIoctl_StateHook(struct thread* td, struct substitute_state_hook* uap);
            static int OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);

        protected:
            static int Sys_dynlib_load_prx_hook(struct thread* td, struct dynlib_load_prx_args_ex* uap);
            static void OnProcessStart(void *arg, struct proc *p);
            static void OnProcessExit(void *arg, struct proc *p);
        };
    }
}