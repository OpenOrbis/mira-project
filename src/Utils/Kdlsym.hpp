#pragma once
#include <Utils/Types.hpp>
#include <Boot/Config.hpp>

#ifndef ONI_PLATFORM
#error ONI_PLATFORM not set
#endif

#if ONI_PLATFORM==ONI_UNKNOWN_PLATFORM
#include "Kdlsym/Default.hpp"
#elif ONI_PLATFORM==ONI_PLATFORM_STEAM_LINK2
#include "Kdlsym/SteamLink2.hpp"
#elif ONI_PLATFORM==ONI_PLATFORM_RASPI_ZERO
#include "Kdlsym/RaspberryPiZero.hpp"
#elif ONI_PLATFORM==ONI_PLATFORM_ORBIS_BSD_176
#include "Kdlsym/Orbis176.hpp"
#elif ONI_PLATFORM==ONI_PLATFORM_ORBIS_BSD_405
#include "Kdlsym/Orbis405.hpp"
#elif ONI_PLATFORM==ONI_PLATFORM_ORBIS_BSD_455
#include "Kdlsym/Orbis455.hpp"
#elif ONI_PLATFORM==ONI_PLATFORM_ORBIS_BSD_474
#include "Kdlsym/Orbis474.hpp"
#elif ONI_PLATFORM==ONI_PLATFORM_ORBIS_BSD_501
#include "Kdlsym/Orbis501.hpp"
#elif ONI_PLATFORM==ONI_PLATFORM_ORBIS_BSD_505
#include "Kdlsym/Orbis505.hpp"
#elif ONI_PLATFORM==ONI_PLATFORM_ORBIS_BSD_620
#include "Kdlsym/Orbis620.hpp"
#elif ONI_PLATFORM==ONI_PLATFORM_ORBIS_BSD_650
#include "Kdlsym/Orbis650.hpp"
#endif

// Kernel base address, this must be filled out on-startup (normally done in oni_initializeKernel)
extern "C" uint8_t* gKernelBase;

// Define kdlsym macro
#ifndef kdlsym
#define kdlsym(x) ((void*)((uint8_t *)&gKernelBase[kdlsym_addr_ ## x]))
#endif
