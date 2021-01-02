#include "TargetID.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>

using namespace Mira::Plugins;

TargetID::TargetID()
{

}

TargetID::~TargetID()
{

}

void TargetID::SpoofTo(char targetId_input)
{
	targetId_desired = targetId_input;

	if (targetId_orig == 0x00)
		targetId_orig = *(char *)kdlsym(target_id);

	if (targetId_orig == targetId_input)
	{
		WriteLog(LL_Error, "Target ID is already %02hhX", targetId_input);
		return;
	}

	WriteLog(LL_Warn, "spoofing target ID to %02hhX", targetId_input);
	*(char *)kdlsym(target_id) = targetId_input;
	WriteLog(LL_Info, "target ID spoofed to %02hhX", targetId_input);
}

bool TargetID::OnLoad()
{
	SpoofTo(targetId_desired);
	return true;
}

bool TargetID::OnUnload()
{
	SpoofTo(targetId_orig);
	return true;
}

bool TargetID::OnSuspend()
{
	SpoofTo(targetId_orig);
	return true;
}

bool TargetID::OnResume()
{
	SpoofTo(targetId_desired);
	return true;
}
