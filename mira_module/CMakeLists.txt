cmake_minimum_required(VERSION 3.10)

# Set the project to be a C CXX ASM
project(mira_module)

set(CMAKE_VERBOSE_MAKEFILE ON)

# Include all of the OOSDK defaults
include(${CMAKE_SOURCE_DIR}/scripts/oosdk.cmake)

#enable_language(C)
#enable_language(CXX)
enable_language(ASM)

# Set the include directories
include_directories(${CMAKE_SOURCE_DIR}/external ${CMAKE_SOURCE_DIR}/external/mira src)

# Get all of the source files that need to be compiled
file(GLOB_RECURSE MIRA_MODULE_SRC_FILES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.cc ${PROJECT_SOURCE_DIR}/*.cxx ${PROJECT_SOURCE_DIR}/*.h ${PROJECT_SOURCE_DIR}/*.hpp ${PROJECT_SOURCE_DIR}/*.hxx)
file(GLOB_RECURSE MIRA_MODULE_S_FILES ${PROJECT_SOURCE_DIR}/*.s ${PROJECT_SOURCE_DIR}/*.S)

# Set all assembler flags
set_source_files_properties(${MIRA_MODULE_S_FILES} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

message("SrcFiles")
message("${MIRA_MODULE_SRC_FILES}")

message("SFiles")
message("${MIRA_MODULE_S_FILES}")

link_libraries(-lSceLibcInternal -lkernel -lc++)

add_library(mira_module SHARED ${MIRA_MODULE_SRC_FILES})