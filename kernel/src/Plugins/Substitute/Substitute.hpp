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

struct dynlib_dlsym_args {
    int id;
    char* name;
    uint64_t* result;
};

struct entrypointhook_header {
    uint32_t magic;
    uint64_t entrypoint;
    uint32_t epdone;
    uint64_t sceSysmodulePreloadModuleForLibkernel;
    uint64_t fakeReturnAddress;
} __attribute__((packed));


/////////////////////////////////////////
// Substitute Parameter (Don't forget to update library !)
/////////////////////////////////////////

#define SUBSTITUTE_IOCTL_BASE   'S'
#define SUBSTITUTE_MAX_NAME 255 // Max lenght for name
#define SUBSTITUTE_MAIN_MODULE "" // Define the main module
#define SUBSTITUTE_MAX_HOOKS 100 // Max possible hooks (all process)
#define SUBSTITUTE_MAX_CHAINS 50 // Max possible function in chains

/////////////////////////////////////////
// Enumeration
/////////////////////////////////////////

enum HookType {
    HOOKTYPE_IAT,
    HOOKTYPE_JMP
};

enum {
    SUBSTITUTE_IAT_NAME = 0,
    SUBSTITUTE_IAT_NIDS = 1
};

enum {
    SUBSTITUTE_STATE_DISABLE = 0,
    SUBSTITUTE_STATE_ENABLE = 1,
    SUBSTITUTE_STATE_UNHOOK = 2
};

enum SubstituteError {
    SUBSTITUTE_OK       =  0,
    SUBSTITUTE_BAD_ARGS = -1,
    SUBSTITUTE_INVALID  = -2,
    SUBSTITUTE_BADLOGIC = -3,
    SUBSTITUTE_NOTFOUND = -4,
    SUBSTITUTE_NOMEM    = -5
};

/////////////////////////////////////////
// Kernel structure
/////////////////////////////////////////

struct SubstituteJmp {
    void* orig_addr;
    void* jmpto;
    char* backupData;
    size_t backupSize;
    bool enable;
};

struct SubstituteIAT {
    void* jmpslot_address; 
    void* original_function;
    uint64_t* uap_chains;
};

struct SubstituteHook {
    struct proc* process;
    HookType hook_type;
    struct SubstituteJmp jmp;
    struct SubstituteIAT iat;
};

/////////////////////////////////////////
// IOCTL Wrapper
/////////////////////////////////////////

struct substitute_state_hook {
    int hook_id;
    int state;
    struct substitute_hook_uat* chain;
    int result;
};

struct substitute_hook_iat {
    int hook_id;
    char name[SUBSTITUTE_MAX_NAME];
    char module_name[SUBSTITUTE_MAX_NAME];
    int flags;
    void* hook_function;
    struct substitute_hook_uat* chain;
};

struct substitute_hook_jmp {
    int hook_id;
    void* original_function;
    void* hook_function;
};

/////////////////////////////////////////
// Userland structure Information & Chain
/////////////////////////////////////////

struct substitute_hook_uat {
    int hook_id;
    void* hook_function;
    void* original_function;
    struct substitute_hook_uat* next;
};

/////////////////////////////////////////
// IOCTL Definition
/////////////////////////////////////////

#define SUBSTITUTE_HOOK_IAT _IOC(IOC_INOUT, SUBSTITUTE_IOCTL_BASE, 1, sizeof(struct substitute_hook_iat))
#define SUBSTITUTE_HOOK_JMP _IOC(IOC_INOUT, SUBSTITUTE_IOCTL_BASE, 2, sizeof(struct substitute_hook_jmp))
#define SUBSTITUTE_HOOK_STATE _IOC(IOC_INOUT, SUBSTITUTE_IOCTL_BASE, 3, sizeof(struct substitute_state_hook))

/////////////////////////////////////////
// Class Definition
/////////////////////////////////////////

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
            void* sys_dynlib_dlsym_p;

            // Plugin Base
            Substitute();
            virtual ~Substitute();
            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;

            static Substitute* GetPlugin();

            // Hook memory utility
            SubstituteHook* GetHookByID(int hook_id);
            SubstituteHook* AllocateHook(int* hook_id);
            void FreeHook(int hook_id);
            
            // Chain Utility
            bool  FindJmpslotSimilarity(struct proc* p, void* jmpslot_addr, int* hook_id);
            int   FindPositionByChain(uint64_t* chains, uint64_t chain);
            void* FindLastChainOccurence(uint64_t* chains, int* position);

            // Hook mechanics
            int DisableHook(struct proc* p, int hook_id);
            int EnableHook(struct proc* p, int hook_id);
            int Unhook(struct proc* p, int hook_id, struct substitute_hook_uat* chain);
            int HookJmp(struct proc* p, void* original_address, void* hook_function);
            int HookIAT(struct proc* p, struct substitute_hook_uat* chain, const char* module_name, const char* name, int32_t flags, void* hook_function);
            void CleanupProcessHook(struct proc* p);
            void CleanupAllHook();

            // SCE Module Utility
            uint64_t FindJmpslotAddress(struct proc* p, const char* module_name, const char* name, int32_t flags);
            void*    FindOriginalAddress(struct proc* p, const char* name, int32_t flags);

            // IOCTL Function
            static int OnIoctl_HookIAT(struct thread* td, struct substitute_hook_iat* uap);
            static int OnIoctl_HookJMP(struct thread* td, struct substitute_hook_jmp* uap);
            static int OnIoctl_StateHook(struct thread* td, struct substitute_state_hook* uap);
            static int OnIoctl(struct cdev* p_Device, u_long p_Command, caddr_t p_Data, int32_t p_FFlag, struct thread* p_Thread);

        protected:
            // Event to trigger
            static int Sys_dynlib_dlsym_hook(struct thread* td, struct dynlib_dlsym_args* uap);
            static void OnProcessStart(void *arg, struct proc *p);
            static void OnProcessExit(void *arg, struct proc *p);
        };
    }
}
