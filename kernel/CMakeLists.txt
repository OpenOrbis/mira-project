cmake_minimum_required(VERSION 3.10)

# Set the project to be a C CXX ASM
project(kernel)

set(CMAKE_VERBOSE_MAKEFILE OFF)

# Include all of the OOSDK defaults
include(${CMAKE_SOURCE_DIR}/scripts/kernel.cmake)

enable_language(ASM)

# Set the include directories
include_directories(${CMAKE_SOURCE_DIR}/external ${CMAKE_SOURCE_DIR}/external/freebsd-headers/include ${CMAKE_SOURCE_DIR}/external/mira/Kernel ${PROJECT_SOURCE_DIR}/src)

# Get all of the source files that need to be compiled
file(GLOB_RECURSE MIRA_MODULE_SRC_FILES ${PROJECT_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/external/mira/Kernel/*.cpp ${PROJECT_SOURCE_DIR}/*.cc ${PROJECT_SOURCE_DIR}/*.cxx ${PROJECT_SOURCE_DIR}/*.h ${PROJECT_SOURCE_DIR}/*.hpp ${PROJECT_SOURCE_DIR}/*.hxx)
file(GLOB_RECURSE MIRA_MODULE_S_FILES ${PROJECT_SOURCE_DIR}/*.s ${PROJECT_SOURCE_DIR}/*.S)

# Set all assembler flags
set_source_files_properties(${MIRA_MODULE_S_FILES} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

# Set the linker options for this project
add_link_options(-entry=mira_entry)
add_link_options(-T ${PROJECT_SOURCE_DIR}/src/link.ld)
add_link_options(-v)

set(TRAINER_LOADER_BIN ${CMAKE_BINARY_DIR}/trainer_loader/trainer_loader)
add_compile_definitions(TRAINER_LOADER_BIN_PATH="${TRAINER_LOADER_BIN}")

add_executable(kernel ${MIRA_MODULE_S_FILES} ${MIRA_MODULE_SRC_FILES})