#include "Substitute2.hpp"

#include <Utils/Logger.hpp>

extern "C"
{
    #include <sys/proc.h>
};

using namespace Mira::Plugins;

bool Substitute2::OnProcessExecEnd(struct proc* p_Process)
{
    // Validate the process
    if (p_Process == nullptr)
    {
        WriteLog(LL_Error, "invalid process.");
        return false;
    }

    // Check the process name
    if (strcmp("eboot.bin", p_Process->p_comm) != 0)
        return true;
    
    // Check the title id's against the supported
    const char* s_TitleId = &p_Process->p_unk348[0x58];
    if (s_TitleId == nullptr)
        return false;

    // Get the main thread
    auto s_MainThread = p_Process->p_threads.tqh_first;
    if (s_MainThread == nullptr)
    {
        WriteLog(LL_Error, "main thread not found.");
        return false;
    }

    // Get the process name

    // Mount sub folder to the game path

    return true;
}

bool Substitute2::OnProcessExit(struct proc* p_Process)
{
    return true;
}

bool Substitute2::GetTitlePath(SubstituteFlags p_Flags, const char* p_TitleId, const char*& p_OutPath)
{
    return false;
}