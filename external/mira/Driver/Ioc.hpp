#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_KERNEL)
#include <sys/ioccom.h>
#else
#include <sys/ioctl.h>
#define	IOC_VOID	0x20000000	/* no parameters */
#define	IOC_OUT		0x40000000	/* copy out parameters */
#define	IOC_IN		0x80000000	/* copy in parameters */
#define	IOC_INOUT	(IOC_IN|IOC_OUT)
#endif

#include <stdint.h>

#if !defined(_MAX_PATH)
#define _MAX_PATH 260
#endif

#define MIRA_IOCTL_BASE 'M'

#include "DriverCmds.hpp"

#if defined(__cplusplus)
};
#endif