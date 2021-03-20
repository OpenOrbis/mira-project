#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Vector.hpp>

extern "C"
{
    #include <sys/param.h>
    #include <sys/mutex.h>
}

namespace Mira
{
    namespace Plugins
    {
        class PluginManager : public Mira::Utils::IModule
        {
        private:
            Vector<Mira::Utils::IModule*> m_Plugins;

        public:
            PluginManager();
            virtual ~PluginManager();

            virtual bool OnLoad() override;
            virtual bool OnUnload() override;
            virtual bool OnSuspend() override;
            virtual bool OnResume() override;
            
            virtual bool OnProcessExec(struct proc* p_Process) override;
            virtual bool OnProcessExecEnd(struct proc* p_Process) override;
            virtual bool OnProcessExit(struct proc* p_Process) override;
            
        private:
            Mira::Utils::IModule* m_Debugger;
            Mira::Utils::IModule* m_Logger;
            Mira::Utils::IModule* m_FakeSelfManager;
            Mira::Utils::IModule* m_FakePkgManager;
            Mira::Utils::IModule* m_EmuRegistry;
            Mira::Utils::IModule* m_SyscallGuard;
            Mira::Utils::IModule* m_PrivCheck;

        public:
            Mira::Utils::IModule* GetDebugger() { return m_Debugger; }
            Mira::Utils::IModule* GetFakeSelfManager() { return m_FakeSelfManager; }
            Mira::Utils::IModule* GetEmulatedRegistry() { return m_EmuRegistry; }
            Mira::Utils::IModule* GetSyscallGuard() { return m_SyscallGuard; }
            Mira::Utils::IModule* GetPrivCheck() { return m_PrivCheck; }
        };
    }
}
