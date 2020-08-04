#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>

extern "C"
{
    #include <sys/eventhandler.h>
};

namespace Mira
{
		namespace Plugins
		{
				class MorpheusEnabler : public Mira::Utils::IModule
				{
				private:
			            eventhandler_entry* m_processStartEvent;
			            eventhandler_entry* m_resumeEvent;
			    private:
			    		static bool DoPatch();
				public:
						MorpheusEnabler();
						virtual ~MorpheusEnabler();

						virtual const char* GetName() override { return "MorpheusEnabler"; }
						virtual bool OnLoad() override;
						virtual bool OnUnload() override;
						virtual bool OnSuspend() override;
						virtual bool OnResume() override;
				protected:
						static void ProcessStartEvent(void *arg, struct ::proc *p);
						static void ResumeEvent();
				};
		}
}
