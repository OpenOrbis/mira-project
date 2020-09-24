#pragma once
#include <Utils/IModule.hpp>
#include <Utils/Types.hpp>

namespace Mira
{
	namespace Plugins
	{
		class TargetID : public Mira::Utils::IModule
		{
			private:
				char targetId_orig = 0x00;
				char targetId_desired = 0x82; // DEX
			public:
				TargetID();
				virtual ~TargetID();

				virtual const char* GetName() override { return "TargetID"; }
				virtual bool OnLoad() override;
				virtual bool OnUnload() override;
				virtual bool OnSuspend() override;
				virtual bool OnResume() override;
				void SpoofTo(char dex_id);
		};
	}
}
