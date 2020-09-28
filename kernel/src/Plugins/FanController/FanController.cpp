// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "FanController.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>
#include <Utils/SysWrappers.hpp>

using namespace Mira::Plugins;

FanController::FanController()
{
}

FanController::~FanController()
{
}

void FanController::SetFanThreshhold(int fanController_input)
{
	if (fanController_input < 59 || fanController_input > 79)
	{
		WriteLog(LL_Error, "Unsafe fan controller setting: %i°C", fanController_input);
		return;
	}

	auto s_Thread = curthread;
	if (s_Thread == nullptr)
	{
		WriteLog(LL_Error, "could not get current thread.");
		return;
	}

	if (fanController_orig == fanController_input)
	{
		WriteLog(LL_Info, "Fan controller already set to %i°C", fanController_input);
		return;
	}

	fanController_desired = fanController_input;

	int fd = kopen_t("/dev/icc_fan", 0x0000, 0, s_Thread); // O_RDONLY
	if (fd <= 0) {
		WriteLog(LL_Info, "unable to open \"/dev/icc_fan\"");
		return;
	}

	char data[10] = {0x00, 0x00, 0x00, 0x00, 0x00, (char)fanController_input, 0x00, 0x00, 0x00, 0x00};
	kioctl_t(fd, 0xC01C8F07, data, s_Thread);
	kclose_t(fd, s_Thread);

	WriteLog(LL_Info, "Successfully set fan controller to %i°C", fanController_input);
}

bool FanController::OnLoad()
{
	SetFanThreshhold(fanController_desired);
	return true;
}

bool FanController::OnUnload()
{
	SetFanThreshhold(fanController_orig);
	return true;
}

bool FanController::OnSuspend()
{
	return true;
}

bool FanController::OnResume()
{
	return true;
}
