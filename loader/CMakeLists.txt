cmake_minimum_required(VERSION 3.10)

# Set the project to be a C CXX ASM
project(loader)

# Include all of the OOSDK defaults
include(${CMAKE_SOURCE_DIR}/scripts/payload.cmake)

enable_language(ASM)

# Set the include directories
message("${PROJECT_SOURCE_DIR}/src")
include_directories(${CMAKE_SOURCE_DIR}/external ${CMAKE_SOURCE_DIR}/external/mira ${CMAKE_SOURCE_DIR}/external/mira/Kernel ${CMAKE_SOURCE_DIR}/external/freebsd-headers/include ${PROJECT_SOURCE_DIR}/src)

add_compile_definitions(_KERNEL)

# Get all of the source files that need to be compiled
file(GLOB_RECURSE MIRA_MODULE_SRC_FILES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.cc ${PROJECT_SOURCE_DIR}/*.cxx ${CMAKE_SOURCE_DIR}/external/mira/Kernel/*.cpp ${PROJECT_SOURCE_DIR}/*.h ${PROJECT_SOURCE_DIR}/*.hpp ${PROJECT_SOURCE_DIR}/*.hxx)
file(GLOB_RECURSE MIRA_MODULE_S_FILES ${PROJECT_SOURCE_DIR}/*.s ${PROJECT_SOURCE_DIR}/*.S)

# Set all assembler flags
set_source_files_properties(${MIRA_MODULE_S_FILES} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

add_library(loader SHARED ${MIRA_MODULE_SRC_FILES})