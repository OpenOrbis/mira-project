cmake_minimum_required(VERSION 3.20)

# Set verbosity
set(CMAKE_VERBOSE_MAKEFILE OFF)

# set the project name
project(mira-project)

# Set the firmware version
if (MIRA_PLATFORM)
else()
    message("MIRA_PLATFORM not defined, defaulting to 672")
    set(MIRA_PLATFORM MIRA_PLATFORM_ORBIS_BSD_672)
endif()

if (NOT "" STREQUAL "$ENV{MIRA_PLATFORM}")
    message("overriding build by environment variable for MIRA_PLATFORM=$ENV{MIRA_PLATFORM}")
    set(MIRA_PLATFORM ENV{MIRA_PLATFORM})
endif()

# Display some output to the user
message("Configuring for MIRA_PLATFORM=${MIRA_PLATFORM}")

# Enable static analysis checking
set(MIRA_CHECKS False)
set(OOSDK_FOUND False)


# mira-project's required build order
# There are dependencies that require certain things to be pre-built
#
# /kernel
# - Dep: trainer_loader
#
# /loader
# - Dep: kernel
#
# /mira_module (OOSDK dep, none local)
# /daemon (OOSDK dep, none local)

# If we have MIRA_CHECKS on our environment variable, then we enable it in CMAKE
if (EXISTS "$ENV{MIRA_CHECKS}")
    set(MIRA_CHECKS True)
endif()

# If the OO_PS4_TOOLCHAIN environment variable is found, set a CMake variable with the same name
if (EXISTS "$ENV{OO_PS4_TOOLCHAIN}")
  set(OO_PS4_TOOLCHAIN $ENV{OO_PS4_TOOLCHAIN})
  set(OOSDK_FOUND True)
endif()

# Spit out a message if we have static analysis enabled
if (MIRA_CHECKS EQUAL True)
    message("MIRA_CHECKS is enabled, enabling static analysis.")
endif()

# Check to see if we found the OpenOrbis Toolchain
if (EXISTS "$ENV{OO_PS4_TOOLCHAIN}")
    message("OpenOrbis SDK found, building extra libraries...")

    # Build the userland common module which gets injected into each process
    add_subdirectory(mira_module)

    # Trainer example
    add_subdirectory(example_trainer)

    # Make mira_module a dependency of example_trainer
    add_dependencies(example_trainer mira_module)

    # Build the daemon
    add_subdirectory(daemon)

    # Make the standalone tests
    add_subdirectory(tests)
else()
    message("OpenOrbis SDK is not found, extra libraries is skipped ...")
endif()

# This is required for all other steps to succeeed
add_subdirectory(trainer_loader)

# Kernel depends on trainer_loader
add_subdirectory(kernel)

# Make trainer_loader a dependency of kernel
add_dependencies(kernel trainer_loader)

# Loader depends on kernel
add_subdirectory(loader)

# Make kernel a dependency of loader
add_dependencies(loader kernel)
