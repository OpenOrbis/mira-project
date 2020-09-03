// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "MorpheusEnabler.hpp"
#include <Utils/Kdlsym.hpp>
#include <Utils/Logger.hpp>

#include <Mira.hpp>
#include <OrbisOS/Utilities.hpp>

extern "C"
{
	#include <sys/mman.h>
    #include <sys/eventhandler.h>
};

using namespace Mira::Plugins;
using namespace Mira::OrbisOS;

MorpheusEnabler::MorpheusEnabler()
{

}

MorpheusEnabler::~MorpheusEnabler()
{

}

void MorpheusEnabler::ProcessStartEvent(void *arg, struct ::proc *p)
{
    auto strncmp = (int(*)(const char *, const char *, size_t))kdlsym(strncmp);

    if (!p)
        return;

    char* s_TitleId = (char*)((uint64_t)p + 0x390);
    if (strncmp(s_TitleId, "NPXS20001", 9) == 0) {
       
if(Utilities::isAssistMode() == IS_TESTKIT || Utilities::isTestkit() == IS_TESTKIT){
}
else
 DoPatch(); 
  }

    return;
}

void MorpheusEnabler::ResumeEvent()
{
    DoPatch();
    WriteLog(LL_Debug, "InstallEventHandlers finished");
    return;
}

bool MorpheusEnabler::DoPatch()
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

bool MorpheusEnabler::OnLoad()
{
	auto s_MainThread = Mira::Framework::GetFramework()->GetMainThread();
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "could not get main mira thread");
        return false;
    }

    // Initialize the event handlers
    auto eventhandler_register = (eventhandler_tag(*)(struct eventhandler_list *list, const char *name, void *func, void *arg, int priority))kdlsym(eventhandler_register);

    m_processStartEvent = eventhandler_register(NULL, "process_exec_end", reinterpret_cast<void*>(MorpheusEnabler::ProcessStartEvent), NULL, EVENTHANDLER_PRI_LAST);
    m_resumeEvent = eventhandler_register(NULL, "system_resume_phase4", reinterpret_cast<void*>(MorpheusEnabler::ResumeEvent), NULL, EVENTHANDLER_PRI_LAST);


if(Utilities::isAssistMode() == IS_TESTKIT || Utilities::isTestkit() == IS_TESTKIT){
     WriteLog(LL_Debug, "Testkit Detected, No patches will be applied\n");
return true;
}
else{
    return DoPatch();
}
}

bool MorpheusEnabler::OnUnload()
{
    auto eventhandler_deregister = (void(*)(struct eventhandler_list* a, struct eventhandler_entry* b))kdlsym(eventhandler_deregister);
    auto eventhandler_find_list = (struct eventhandler_list * (*)(const char *name))kdlsym(eventhandler_find_list);

    if (m_processStartEvent) {
        EVENTHANDLER_DEREGISTER(process_exec_end, m_processStartEvent);
        m_processStartEvent = nullptr;
    }

    if (m_resumeEvent) {
        EVENTHANDLER_DEREGISTER(process_exit, m_resumeEvent);
        m_resumeEvent = nullptr;
    }

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
