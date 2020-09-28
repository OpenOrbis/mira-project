#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>

namespace Mira
{
	namespace Plugins
	{
		class FanController : public Mira::Utils::IModule
		{
			private:
				int fanController_orig = 79;
				int fanController_desired = 79;
			public:
				FanController();
				virtual ~FanController();

				virtual const char* GetName() override { return "FanController"; }
				virtual bool OnLoad() override;
				virtual bool OnUnload() override;
				virtual bool OnSuspend() override;
				virtual bool OnResume() override;
				void SetFanThreshhold(int fanController_input);
		};
	}
}
