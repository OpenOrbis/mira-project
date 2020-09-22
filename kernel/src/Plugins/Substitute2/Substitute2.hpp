#pragma once
#include <Utils/IModule.hpp>

namespace Mira
{
    namespace Plugins
    {
        class Substitute2 : public Utils::IModule
        {
        private:
            typedef enum SubstituteFlags_t
            {
                SF_NONE = 0,
                SF_USB = 1 << 1,
                SF_NET = 1 << 2,
                SF_USER = 1 << 3,
            } SubstituteFlags;
        public:
            virtual bool OnProcessExecEnd(struct proc* p_Process) override;
            virtual bool OnProcessExit(struct proc* p_Process) override;

        protected:
            bool GetTitlePath(SubstituteFlags p_Flags, const char* p_TitleId, const char*& p_OutPath);
        };
    }
}