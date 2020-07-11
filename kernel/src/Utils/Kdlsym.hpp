#pragma once
#include <Utils/Types.hpp>
#include <Boot/Config.hpp>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MIRA_PLATFORM
#error MIRA_PLATFORM not set
#endif

#if MIRA_PLATFORM==ONI_UNKNOWN_PLATFORM
#include "Kdlsym/Default.hpp"
#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_176
#include "Kdlsym/Orbis176.hpp"
#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_405
#include "Kdlsym/Orbis405.hpp"
#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_455
#include "Kdlsym/Orbis455.hpp"
#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_474
#include "Kdlsym/Orbis474.hpp"
#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_501
#include "Kdlsym/Orbis501.hpp"
#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_503
#include "Kdlsym/Orbis503.hpp"
#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_505
#include "Kdlsym/Orbis505.hpp"
#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_620
#include "Kdlsym/Orbis620.hpp"
#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_650
#include "Kdlsym/Orbis650.hpp"
#elif MIRA_PLATFORM==MIRA_PLATFORM_ORBIS_BSD_672
#include "Kdlsym/Orbis672.hpp"
#elif MIRA_PLATFORM==MIRA_PLATFORM_RASPI_ZERO
#include "Kdlsym/RaspberryPiZero.hpp"
#elif MIRA_PLATFORM==MIRA_PLATFORM_STEAM_LINK2
#include "Kdlsym/SteamLink2.hpp"
#endif

// Kernel base address, this must be filled out on-startup (normally done in oni_initializeKernel)
extern uint8_t* gKernelBase;

// Define kdlsym macro
#ifndef kdlsym
#define kdlsym(x) ((void*)((uint8_t *)&gKernelBase[kdlsym_addr_ ## x]))
#endif

#ifdef __cplusplus
}
#endif
