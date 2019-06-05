#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>
#include <Utils/Vector.hpp>

#include "ThreadContainer.hpp"

namespace Mira
{
    namespace Threading
    {
        // We need this otherwise threads run wild every fucking where >_>
        class ThreadManager : public Mira::Utils::IModule
        {
        private:
            struct mtx m_ThreadsLock;
            Vector<ThreadContainer> m_Threads;

        public:
            virtual bool OnLoad() override { return true; }
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;
            virtual bool OnUnload() override { return true; }

            virtual const wchar_t* GetName() override { return L"ThreadManager"; }
            virtual const wchar_t* GetDescription() override { return L"Mira threading manager"; }
            
            ThreadManager();
            virtual ~ThreadManager();

            void* CreateKernelThread(struct thread* p_OwningThread, void* p_EntryPoint, const char* p_ThreadName = nullptr, void* p_Arguments = nullptr);
            void* CreateUserThread(int32_t p_ProcessId, void* p_EntryPoint, void* p_Arguments = nullptr);

            uint32_t GetTotalThreads() { return m_Threads.size(); }
        };
    }
}