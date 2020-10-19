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
				class MorpheusActivator : public Mira::Utils::IModule
				{
			    private:
			    		static bool DoPatch();

				public:
						MorpheusActivator();
						virtual ~MorpheusActivator();

						virtual const char* GetName() override { return "MorpheusActivator"; }
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
