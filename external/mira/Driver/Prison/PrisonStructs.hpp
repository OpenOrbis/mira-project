#pragma once
#include "../Ioc.hpp"

typedef struct _PrisonList
{
    uint32_t PrisonsCount;
    void* Prisons[0];
} PrisonList;
#define MIRA_PRISON_LIST _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_PrisonList), sizeof(PrisonList))

typedef struct _PrisonInfo
{
    // TODO: Implement
} PrisonInfo;
#define MIRA_PRISON_INFO _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_PrisonInfo), sizeof(PrisonInfo))
