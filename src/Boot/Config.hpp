#pragma once

// Set our default definitions for each used version number

#define ONI_PLATFORM_ORBIS_BSD_176	176

#define ONI_PLATFORM_ORBIS_BSD_355	355

#define ONI_PLATFORM_ORBIS_BSD_400	400

#define ONI_PLATFORM_ORBIS_BSD_405	405

#define ONI_PLATFORM_ORBIS_BSD_407	407

#define ONI_PLATFORM_ORBIS_BSD_455	455

#define ONI_PLATFORM_ORBIS_BSD_474	474

#define ONI_PLATFORM_ORBIS_BSD_500	500

#define ONI_PLATFORM_ORBIS_BSD_501	501

#define ONI_PLATFORM_ORBIS_BSD_505	505

#define ONI_PLATFORM_ORBIS_BSD_550	550

#define ONI_PLATFORM_ORBIS_BSD_553	553

#define ONI_PLATFORM_ORBIS_BSD_555	555

#define ONI_PLATFORM_RASPI_ZERO		998

#define ONI_PLATFORM_STEAM_LINK		999

// Unknown device
#define ONI_UNKNOWN_PLATFORM		-1

// Function address for kernel_execution
#ifndef ONI_KERN_EXEC
#define ONI_KERN_EXEC 0x4141414141414141
#endif

#ifndef KEXEC_SYSCALL_NUM
#define KEXEC_SYSCALL_NUM	11
#endif

// The current platform configured by oni
#ifndef ONI_PLATFORM
#error ONI_PLATFORM environment variable not set
#endif

// The maximum number of plugins for use with oni
#define PLUGINMANAGER_MAX_PLUGINS	256

#ifdef _DEBUG
#define ONI_THREAD_NAME	"oni_thread"
#define ONI_LOG_BUFFER_SIZE	0x1000
#define ONI_BASE_PATH "/user/oni"
#define RPC_SLEEP "rpc sleep"
#define ONI_RPC_PORT 9999
#else
#define ONI_THREAD_NAME	""
#define ONI_LOG_BUFFER_SIZE	0x100
#define ONI_BASE_PATH "/user/config"
#define RPC_SLEEP ""
#define ONI_RPC_PORT	9999
#endif
