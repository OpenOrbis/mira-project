cmake_minimum_required(VERSION 3.12)

# Set verbosity
set(CMAKE_VERBOSE_MAKEFILE OFF)

# set the project name
project(mira-project)

set(MIRA_PLATFORM MIRA_PLATFORM_ORBIS_BSD_672)

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


# Build the userland common module which gets injected into each process
#add_subdirectory(mira_module)

# Build the daemon
#add_subdirectory(daemon)

# This is required for all other steps to succeeed
#add_subdirectory(trainer_loader)

# Kernel depends on trainer_loader
#add_subdirectory(kernel)

# Make trainer_loader a dependency of kernel
#add_dependencies(kernel trainer_loader)

# Loader depends on kernel
#add_subdirectory(loader)

# Make kernel a dependency of loader
#add_dependencies(loader kernel)

# Make the standalone tests
add_subdirectory(tests)

# Add the dependency on mira_module
#\add_dependencies(tests mira_module)