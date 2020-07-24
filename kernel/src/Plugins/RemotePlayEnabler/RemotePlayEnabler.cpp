// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "RemotePlayEnabler.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>

#include <OrbisOS/Utilities.hpp>

using namespace Mira::Plugins;
using namespace Mira::OrbisOS;

extern "C"
{
	#include <sys/mman.h>
};

RemotePlayEnabler::RemotePlayEnabler()
{

}

RemotePlayEnabler::~RemotePlayEnabler()
{

}

bool RemotePlayEnabler::ShellUIPatch()
{
	WriteLog(LL_Debug, "patching SceShellUI");

	struct ::proc* s_Process = Utilities::FindProcessByName("SceShellUI");
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not find SceShellUI");
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

	uint8_t* s_ShellUITextStart = nullptr;
	for (auto i = 0; i < s_NumEntries; ++i)
	{
		if (!memcmp(s_Entries[i].name, "executable", 10) && s_Entries[i].prot >= (PROT_READ | PROT_EXEC))
		{
			s_ShellUITextStart = (uint8_t*)s_Entries[i].start;
			break;
		}
	}

	if (s_ShellUITextStart == nullptr)
	{
		WriteLog(LL_Error, "could not find SceShellUI text start");
		return false;
	}

	WriteLog(LL_Debug, "SceShellUI .text: (%p)", s_ShellUITextStart);

	uint8_t* s_ShellUIAppTextStart = nullptr;
	for (auto i = 0; i < s_NumEntries; ++i)
	{
#if MIRA_PLATFORM<MIRA_PLATFORM_ORBIS_BSD_500
		if (!memcmp(s_Entries[i].name, "libSceVsh_aot.sprx", 18) && s_Entries[i].prot >= (PROT_READ | PROT_EXEC))
		{
			s_ShellUIAppTextStart = (uint8_t*)s_Entries[i].start;
			break;
		}
#else
		if (!memcmp(s_Entries[i].name, "app.exe.sprx", 10) && s_Entries[i].prot >= (PROT_READ | PROT_EXEC))
		{
			s_ShellUIAppTextStart = (uint8_t*)s_Entries[i].start;
			break;
		}
#endif
	}

	if (s_ShellUIAppTextStart == nullptr)
	{
		WriteLog(LL_Error, "could not find SceShellUI App text start");
		return false;
	}

	WriteLog(LL_Debug, "SceShellUI App .text: (%p)", s_ShellUIAppTextStart);

	// Free the entries we got returned
	delete [] s_Entries;
	s_Entries = nullptr;

	// `/system_ex/app/NPXS20001/eboot.bin`
	s_Ret = Utilities::ProcessReadWriteMemory(s_Process, (void*)(s_ShellUITextStart + ssu_CreateUserForIDU_patch), 4, (void*)"\x48\x31\xC0\xC3", nullptr, true);
	if (s_Ret < 0)
	{
		WriteLog(LL_Error, "ssu_CreateUserForIDU_patch");
		return false;
	}

#if MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_405
	// `/system_ex/app/NPXS20001/libSceVsh_aot.sprx`
	s_Ret = Utilities::ProcessReadWriteMemory(s_Process, (void*)(s_ShellUIAppTextStart + ssu_remote_play_menu_patch), 5, (void*)"\xE9\x64\x02\x00\x00", nullptr, true);
#elif MIRA_PLATFORM>=MIRA_PLATFORM_ORBIS_BSD_455 && MIRA_PLATFORM<=MIRA_PLATFORM_ORBIS_BSD_474
	// `/system_ex/app/NPXS20001/libSceVsh_aot.sprx`
	s_Ret = Utilities::ProcessReadWriteMemory(s_Process, (void*)(s_ShellUIAppTextStart + ssu_remote_play_menu_patch), 5, (void*)"\xE9\x22\x02\x00\x00", nullptr, true);
#elif MIRA_PLATFORM>=MIRA_PLATFORM_ORBIS_BSD_500 && MIRA_PLATFORM<=MIRA_PLATFORM_ORBIS_BSD_505
	// `/system_ex/app/NPXS20001/psm/Application/app.exe.sprx`
	s_Ret = Utilities::ProcessReadWriteMemory(s_Process, (void*)(s_ShellUIAppTextStart + ssu_remote_play_menu_patch), 5, (void*)"\xE9\x82\x02\x00\x00", nullptr, true);
#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_672
	// `/system_ex/app/NPXS20001/psm/Application/app.exe.sprx`
	s_Ret = Utilities::ProcessReadWriteMemory(s_Process, (void*)(s_ShellUIAppTextStart + ssu_remote_play_menu_patch), 5, (void*)"\xE9\xBA\x02\x00\x00", nullptr, true);
#else
	s_Ret = -1;
#endif
	if (s_Ret < 0)
	{
		WriteLog(LL_Error, "ssu_remote_play_menu_patch");
		return false;
	}

	WriteLog(LL_Debug, "SceShellUI successfully patched");

	return true;
}

bool RemotePlayEnabler::RemotePlayPatch()
{
	WriteLog(LL_Debug, "patching SceRemotePlay");

	struct ::proc* s_Process = Utilities::FindProcessByName("SceRemotePlay");
	if (s_Process == nullptr)
	{
		WriteLog(LL_Error, "could not find SceRemotePlay");
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

	uint8_t* s_RemotePlayTextStart = nullptr;
	for (auto i = 0; i < s_NumEntries; ++i)
	{
		if (!memcmp(s_Entries[i].name, "executable", 10) && s_Entries[i].prot >= (PROT_READ | PROT_EXEC))
		{
			s_RemotePlayTextStart = (uint8_t*)s_Entries[i].start;
			break;
		}
	}

	if (s_RemotePlayTextStart == nullptr)
	{
		WriteLog(LL_Error, "could not find SceRemotePlay text start");
		return false;
	}

	WriteLog(LL_Debug, "SceRemotePlay .text: (%p)", s_RemotePlayTextStart);

	// Free the entries we got returned
	delete [] s_Entries;
	s_Entries = nullptr;

	// `/system/vsh/app/NPXS21006/eboot.bin`
	s_Ret = Utilities::ProcessReadWriteMemory(s_Process, (void*)(s_RemotePlayTextStart + srp_enabler_patchA), 1, (void*)"\x01", nullptr, true);
	if (s_Ret < 0)
	{
		WriteLog(LL_Error, "srp_enabler_patchA");
		return false;
	}

	// `/system/vsh/app/NPXS21006/eboot.bin`
	s_Ret = Utilities::ProcessReadWriteMemory(s_Process, (void*)(s_RemotePlayTextStart + srp_enabler_patchB), 2, (void*)"\xEB\x1E", nullptr, true);
	if (s_Ret < 0)
	{
		WriteLog(LL_Error, "srp_enabler_patchB");
		return false;
	}

	WriteLog(LL_Debug, "SceRemotePlay successfully patched");

	return true;
}

bool RemotePlayEnabler::OnLoad()
{
	auto s_Ret = ShellUIPatch();
	if (s_Ret == false) {
		WriteLog(LL_Error, "could not patch SceShellUI");
		return false;
	}

	s_Ret = RemotePlayPatch();
	if (s_Ret == false) {
		WriteLog(LL_Error, "could not patch SceRemotePlay");
		return false;
	}

	return true;
}

bool RemotePlayEnabler::OnUnload()
{
	return true;
}

bool RemotePlayEnabler::OnSuspend()
{
	return true;
}

bool RemotePlayEnabler::OnResume()
{
	return true;
}
