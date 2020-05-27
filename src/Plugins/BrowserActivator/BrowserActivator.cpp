#include "BrowserActivator.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>

using namespace Mira::Plugins;

BrowserActivator::BrowserActivator()
{

}

BrowserActivator::~BrowserActivator()
{

}

bool BrowserActivator::OnLoad()
{
	auto sceRegMgrGetInt = (uint32_t(*)(uint32_t p_Id, int32_t* p_OutValue))kdlsym(sceRegMgrGetInt);
	auto sceRegMgrSetInt = (uint32_t(*)(uint32_t p_Id, int32_t p_Value))kdlsym(sceRegMgrSetInt);

	int32_t rtv;

	auto s_Ret = sceRegMgrGetInt(0x3C040000, &rtv);

	if (s_Ret != 0)
	{
		WriteLog(LL_Error, "could not get browser activation status");
		return false;
	} else if (rtv == 0) {
		WriteLog(LL_Info, "web browser already activated");
		return true;
	}

	WriteLog(LL_Warn, "activating web browser");

	s_Ret = sceRegMgrSetInt(0x3C040000, 0);

	if (s_Ret != 0)
	{
		WriteLog(LL_Error, "could not activate browser");
		return false;
	}

	WriteLog(LL_Info, "activated web browser");
	return true;
}

bool BrowserActivator::OnUnload()
{
		return true;
}

bool BrowserActivator::OnSuspend()
{
		return true;
}

bool BrowserActivator::OnResume()
{
		return true;
}
