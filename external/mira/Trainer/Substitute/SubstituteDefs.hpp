#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

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
// Substitute Parameter (Don't forget to update library !)
/////////////////////////////////////////

#define SUBSTITUTE_IOCTL_BASE   'S'
#define SUBSTITUTE_MAX_NAME 255 // Max lenght for name
#define SUBSTITUTE_MAIN_MODULE "" // Define the main module
#define SUBSTITUTE_MAX_HOOKS 100 // Max possible hooks (all process)
#define SUBSTITUTE_MAX_CHAINS 50 // Max possible function in chains

/////////////////////////////////////////
// Shared structures
/////////////////////////////////////////
typedef struct _TrainerSubstituteHook
{
    // Function to call
    void* hookFn;

    // Trampoline function start REFERENCE (do not free)
    void* trampolineFn;

    // Stop executing and force returning this value
    bool stopExecution;

    // Next entry in the substitute chain
    struct _SubstituteHook* next;
} TrainerSubstituteHook;

typedef struct _TrainerSubstituteHookChain
{
    // Hook type
    enum HookType type;

    // The original function address in code (this will be the same across all hooks)
    void* originalFn;

    // Trampoline function start (this calls the original as if nothing happened)
    void* trampolineFn;

    // Start of the chain head
    struct _SubstituteHook* head;
} TrainerSubstituteHookChain;

#if defined(__cplusplus)
}   // extern "C"
#endif
/*
int open_hook(const char* path, const char* type)
{
    // printf("open called (%s).\n", path);

    return CallChain<int, int(*)(const char*, const char*)>(nullptr, path, type);
}

template<typename RetVal, typename... Args>
inline RetVal o_open(TrainerSubstituteHook* p_Hook, Args... p_Args)
{
    if (p_Hook == nullptr)
        return;
    
    RetVal (*s_open)(p_Args...) = (RetVal(*)(p_Args...))p_Hook->trampolineFn;

    return s_open(p_Args...);
}

inline void InstallHook(void* p_OriginalAddress, void* p_HookedAddress)
{
    TrainerSubstituteHookChain s_Chain;

    auto s_NewHead = new TrainerSubstituteHook;
    if (s_NewHead == nullptr)
        return;
    
    s_NewHead->stopExecution = false;
    s_NewHead->next = nullptr;
    s_NewHead->hookFn = p_HookedAddress;

    // TODO: Patches the address to fall into?
}

template<typename RetType, typename FnType, typename... Args>
inline RetType CallChain(TrainerSubstituteHookChain* p_Chain, Args... p_Args)
{
    if (p_Chain == nullptr)
        return;
    
    FnType s_TrampolineFn = reinterpret_cast<FnType>(p_Chain->originalFn);

    SubstituteHook* s_Current = p_Chain->head;
    if (s_Current == nullptr)
        return;
    
    RetType s_RetVal;
    
    while (s_Current != nullptr)
    {
        FnType s_Callback = reinterpret_cast<FnType>(s_Current->hookFn);
        if (s_Callback == nullptr)
        {
            s_Current = s_Current->next;
            continue;
        }
        
        s_RetVal = s_Callback(s_Current, p_Args...);
        if (s_Current->stop_execution)
        {
            s_Current->stop_execution = 0;
            return s_RetVal;
        }
        
        // Iterate forward
        s_Current = s_Current->next;
    }

    return s_RetVal;
}

void test_func()
{

}*/