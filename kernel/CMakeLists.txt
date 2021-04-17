cmake_minimum_required(VERSION 3.10)

# Set the project to be a C CXX ASM
project(kernel)

set(CMAKE_VERBOSE_MAKEFILE OFF)

# Include all of the OOSDK defaults
include(${CMAKE_SOURCE_DIR}/scripts/kernel.cmake)

set(MIRA_TARGET ${PROJECT_NAME}_Orbis_${MIRA_PLATFORM})
enable_language(ASM)

# Set the include directories
include_directories(${CMAKE_SOURCE_DIR}/external ${CMAKE_SOURCE_DIR}/external/freebsd-headers/include ${CMAKE_SOURCE_DIR}/external/mira/Kernel ${PROJECT_SOURCE_DIR}/src)

set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "")

# Get all of the source files that need to be compiled
file(GLOB_RECURSE MIRA_MODULE_SRC_FILES ${PROJECT_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/external/mira/Kernel/*.cpp ${PROJECT_SOURCE_DIR}/*.cc ${PROJECT_SOURCE_DIR}/*.cxx ${PROJECT_SOURCE_DIR}/*.h ${PROJECT_SOURCE_DIR}/*.hpp ${PROJECT_SOURCE_DIR}/*.hxx)
file(GLOB_RECURSE MIRA_MODULE_S_FILES ${PROJECT_SOURCE_DIR}/*.s ${PROJECT_SOURCE_DIR}/*.S)

# Set all assembler flags
set_source_files_properties(${MIRA_MODULE_S_FILES} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

# Set the linker options for this project
add_link_options(-Wl,-entry=mira_entry)
add_link_options(-Wl,-T ${PROJECT_SOURCE_DIR}/src/link.ld)
add_link_options(-Wl,-v)

set(TRAINER_LOADER_BIN ${CMAKE_BINARY_DIR}/trainer_loader/libtrainer_loader.so)
add_compile_definitions(TRAINER_LOADER_BIN_PATH="${TRAINER_LOADER_BIN}")

add_library(kernel SHARED ${MIRA_MODULE_S_FILES} ${MIRA_MODULE_SRC_FILES})