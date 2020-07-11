#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>

namespace Mira
{
		namespace Plugins
		{
				class MorpheusEnabler : public Mira::Utils::IModule
				{
				public:
						MorpheusEnabler();
						virtual ~MorpheusEnabler();

						virtual const char* GetName() override { return "MorpheusEnabler"; }
						virtual bool OnLoad() override;
						virtual bool OnUnload() override;
						virtual bool OnSuspend() override;
						virtual bool OnResume() override;
				};
		}
}
