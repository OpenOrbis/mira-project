#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>

namespace Mira
{
		namespace Plugins
		{
				class BrowserActivator : public Mira::Utils::IModule
				{
				public:
						BrowserActivator();
						virtual ~BrowserActivator();

						virtual const char* GetName() override { return "BrowserActivator"; }
						virtual bool OnLoad() override;
						virtual bool OnUnload() override;
						virtual bool OnSuspend() override;
						virtual bool OnResume() override;
				};
		}
}
