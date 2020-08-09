#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>

namespace Mira
{
		namespace Plugins
		{
				class RemotePlayEnabler : public Mira::Utils::IModule
				{
				public:
						RemotePlayEnabler();
						virtual ~RemotePlayEnabler();

						virtual const char* GetName() override { return "RemotePlayEnabler"; }
						virtual bool OnLoad() override;
						virtual bool OnUnload() override;
						virtual bool OnSuspend() override;
						virtual bool OnResume() override;

				protected:
				    static bool ShellUIPatch();
            static bool RemotePlayPatch();
				};
		}
}
