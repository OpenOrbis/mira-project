#pragma once
#include <Utils/Types.hpp>
#include <Boot/Config.hpp>

#ifndef MIRA_PLATFORM
#error MIRA_PLATFORM not set
#endif

#ifdef MIRA_UNSUPPORTED_PLATFORMS

#if MIRA_PLATFORM==ONI_UNKNOWN_PLATFORM
#include "Kdlsym/Default.hpp"
#elif MIRA_PLATFORM==MIRA_PLATFORM_STEAM_LINK2
#include "Kdlsym/SteamLink2.hpp"
#elif MIRA_PLATFORM==MIRA_PLATFORM_RASPI_ZERO
#include "Kdlsym/RaspberryPiZero.hpp"
#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_176
#include "Kdlsym/Orbis176.hpp"

#endif // MIRA_UNSUPPORTED_PLATFORMS

#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_405
#include "Kdlsym/Orbis405.hpp"
#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_455
#include "Kdlsym/Orbis455.hpp"
#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_474
#include "Kdlsym/Orbis474.hpp"

#ifdef MIRA_UNSUPPORTED_PLATFORMS
#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_501
#include "Kdlsym/Orbis501.hpp"
#endif

#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_505
#include "Kdlsym/Orbis505.hpp"

// Unsupported platforms
#ifdef MIRA_UNSUPPORTED_PLATFORMS
#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_620
#include "Kdlsym/Orbis620.hpp"
#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_650
#include "Kdlsym/Orbis650.hpp"
#endif

#endif

// Kernel base address, this must be filled out on-startup (normally done in oni_initializeKernel)
extern "C" uint8_t* gKernelBase;

// Define kdlsym macro
#ifndef kdlsym
#define kdlsym(x) ((void*)((uint8_t *)&gKernelBase[kdlsym_addr_ ## x]))
#endif
