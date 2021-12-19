#include "SubstituteHook.hpp"

#include <stdio.h>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

void* Mira::SubstituteHook::FindJmpslotAddress(char* p_Module, char* p_Function, bool p_isNid)
{
    // Open mira device
    auto s_Device = open("/dev/mira", O_RDWR);
    if (s_Device == -1)
        return NULL;

    // Cast the header and set the needed values
    MiraFindJMPSlot s_Request;

    // Zero out the entire buffer to prevent garbage from being read back
    memset(&s_Request, 0, sizeof(MiraFindJMPSlot));

    void* result = NULL;

    snprintf(s_Request.module, _MAX_PATH, "%s", p_Module);
    snprintf(s_Request.function, _MAX_PATH, "%s", p_Function);

    s_Request.is_nid = p_isNid;
    s_Request.value = (void*)&result;

    // Call Substitute
    auto s_Ret = ioctl(s_Device, MIRA_FIND_JMPSLOT, s_Request);
    if (s_Ret != 0)
    {
        close(s_Device);
        return NULL;
    }

    close(s_Device);
    return result;
}