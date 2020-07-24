// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "MorpheusEnabler.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>

#include <OrbisOS/Utilities.hpp>

using namespace Mira::Plugins;
using namespace Mira::OrbisOS;

extern "C"
{
	#include <sys/mman.h>
};

MorpheusEnabler::MorpheusEnabler()
{

}

MorpheusEnabler::~MorpheusEnabler()
{

}

bool MorpheusEnabler::OnLoad()
{
	WriteLog(LL_Debug, "patching SceShellCore");

	struct ::proc* s_Process = Utilities::FindProcessByName("SceShellCore");
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not find SceShellCore");
		return false;
	}

	ProcVmMapEntry* s_Entries = nullptr;
	size_t s_NumEntries = 0;
	auto s_Ret = Utilities::GetProcessVmMap(s_Process, &s_Entries, &s_NumEntries);
	if (s_Ret < 0)
	{
		WriteLog(LL_Error, "could not get vm map");
		return false;
	}

	if (s_Entries == nullptr || s_NumEntries == 0)
	{
		WriteLog(LL_Error, "invalid entries (%p) or numEntries (%d)", s_Entries, s_NumEntries);
		return false;
	}

	uint8_t* s_TextStart = nullptr;
	for (auto i = 0; i < s_NumEntries; ++i)
	{
		if (s_Entries[i].prot == (PROT_READ | PROT_EXEC))
		{
			s_TextStart = (uint8_t*)s_Entries[i].start;
			break;
		}
	}

	if (s_TextStart == nullptr)
	{
		WriteLog(LL_Error, "could not find SceShellCore text start");
		return false;
	}

	WriteLog(LL_Debug, "SceShellCore .text: (%p)", s_TextStart);

	// Free the entries we got returned
	delete [] s_Entries;
	s_Entries = nullptr;

	s_Ret = Utilities::ProcessReadWriteMemory(s_Process, (void*)(s_TextStart + ssc_enable_vr_patch), 3, (void*)"\x31\xC0\xC3", nullptr, true);
	if (s_Ret < 0)
	{
			WriteLog(LL_Error, "ssc_enable_vr");
			return false;
	}

	return true;
}

bool MorpheusEnabler::OnUnload()
{
	return true;
}

bool MorpheusEnabler::OnSuspend()
{
	return true;
}

bool MorpheusEnabler::OnResume()
{
	return true;
}
