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
		class RemotePlayActivator : 
			public Mira::Utils::IModule
		{
		private:
			static bool RemotePlayPatch();
			static bool ShellUIPatch();

		public:
			RemotePlayActivator();
			virtual ~RemotePlayActivator();

			virtual const char* GetName() override { return "RemotePlayActivator"; }
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
