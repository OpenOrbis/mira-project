#pragma once
#include "../Ioc.hpp"

// WE already have a definition of this in external/mira/MiraConfig.hpp
/*typedef struct _MiraConfig
{

} MiraConfig;*/
#define MIRA_READ_CONFIG _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_MiraReadConfig), sizeof(MiraConfig))
#define MIRA_WRITE_CONFIG _IOC(IOC_INOUT, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_MiraWriteConfig), sizeof(MiraConfig))

typedef struct __attribute__((packed)) _MiraMountInSandbox
{
    int32_t Permissions;
    char HostPath[_MAX_PATH];
    char SandboxPath[_MAX_PATH];
} MiraMountInSandbox;
#define MIRA_MOUNT_IN_SANDBOX _IOC(IOC_IN, MIRA_IOCTL_BASE, (uint32_t)(MiraIoctlCmds::CMD_MiraMountInSandbox), sizeof(MiraMountInSandbox))
