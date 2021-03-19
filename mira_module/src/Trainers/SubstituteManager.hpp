#pragma once
#include <mira/Trainers/Substitute/SubstituteDefs.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>

namespace Mira
{
    typedef struct _SubstituteHook
    {
        // Hook id provided by the manager, each hook gets a unique one
        uint64_t Id;

        std::vector<uint64_t> Chain;

    } SubstituteHook;

    class SubstituteManager
    {
    private:
        std::mutex m_Mutex;
        
    protected:
        std::map<uint64_t, std::shared_ptr<SubstituteHook>> m_Hooks;

    public:
        SubstituteManager();

    protected:
        template <typename FunctionSignature>
        std::function<FunctionSignature> FunctionAs(void* p_FunctionPointer)
        {
            return static_cast<FunctionSignature*>(p_FunctionPointer);
        }

    public:
        virtual ~SubstituteManager();
        virtual std::shared_ptr<SubstituteHook> GetHookById(uint64_t p_HookId);
        virtual bool GetHookIdByJumpSlot(void* p_JumpSlotAddress, uint64_t& p_Id);
        
        virtual std::shared_ptr<SubstituteHook> AllocateHook(HookType p_HookType);
        virtual void FreeHook(uint64_t p_HookId);
        virtual bool EnableHook(uint64_t p_HookId);
        virtual bool DisableHook(uint64_t p_HookId);

        virtual bool HookJmp(void* p_OriginalAddress, void* p_HookAddress);
        virtual bool HookIAT(std::string p_ModuleName, std::string p_FunctionName, void* p_HookAddress);

        virtual void RemoveAllHooks();
    };
}