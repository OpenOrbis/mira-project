cmake_minimum_required(VERSION 3.12)

# Set verbosity
set(CMAKE_VERBOSE_MAKEFILE OFF)

# set the project name
project(mira-project)

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



add_subdirectory(mira_module)
#add_subdirectory(daemon)

# This is required for all other steps to succeeed
#add_subdirectory(trainer_loader)

# Kernel depends on trainer_loader
#add_subdirectory(kernel)

# Loader depends on kernel
#add_subdirectory(loader)