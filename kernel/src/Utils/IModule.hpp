#pragma once

struct proc;

namespace Mira
{
    namespace Utils
    {
        class IModule
        {
        public:
            IModule() { }
            virtual ~IModule() { }

            virtual bool OnLoad() { return true; }
            virtual bool OnUnload() { return true; }
            virtual bool OnSuspend() { return true; }
            virtual bool OnResume() { return true; }

            virtual bool OnProcessExec(struct proc* p_Process) { return true; }
            virtual bool OnProcessExecEnd(struct proc* p_Process) { return true; }
            
            virtual bool OnProcessExit(struct proc* p_Process) { return true; }

            virtual const char* GetName() { return ""; };
            virtual const char* GetDescription() { return ""; };
        };
    }
}