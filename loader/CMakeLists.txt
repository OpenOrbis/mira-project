cmake_minimum_required(VERSION 3.10)

# Set the project to be a C CXX ASM
project(loader)

# Include all of the OOSDK defaults
include(${CMAKE_SOURCE_DIR}/scripts/payload.cmake)

enable_language(ASM)

# Set the include directories
#message("${PROJECT_SOURCE_DIR}/src")
include_directories(${CMAKE_SOURCE_DIR}/external ${CMAKE_SOURCE_DIR}/external/mira ${CMAKE_SOURCE_DIR}/external/mira/Kernel ${CMAKE_SOURCE_DIR}/external/freebsd-headers/include ${PROJECT_SOURCE_DIR}/src)

add_compile_definitions(_KERNEL)
add_link_options(-T ${PROJECT_SOURCE_DIR}/src/link.ld)

# Get all of the source files that need to be compiled
file(GLOB_RECURSE MIRA_MODULE_SRC_FILES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.cc ${PROJECT_SOURCE_DIR}/*.cxx ${CMAKE_SOURCE_DIR}/external/mira/Kernel/*.cpp ${PROJECT_SOURCE_DIR}/*.h ${PROJECT_SOURCE_DIR}/*.hpp ${PROJECT_SOURCE_DIR}/*.hxx)
file(GLOB_RECURSE MIRA_MODULE_S_FILES ${PROJECT_SOURCE_DIR}/*.s)

# Set all assembler flags
set_source_files_properties(${PROJECT_SOURCE_DIR}/src/Utils/__Crt0.S PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")
set_source_files_properties(${MIRA_MODULE_S_FILES} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

# Automatic elf embedding, the kernel module should be built at this point
set(MIRA_ELF_PATH ${CMAKE_BINARY_DIR}/kernel/kernel)
add_compile_definitions(MIRA_ELF_PATH="${MIRA_ELF_PATH}")

add_executable(loader ${PROJECT_SOURCE_DIR}/src/Utils/__Crt0.S ${MIRA_MODULE_S_FILES} ${MIRA_MODULE_SRC_FILES})

# Payload creation with objcopy
set(LOADER_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/loader)
add_custom_command(TARGET loader POST_BUILD COMMAND objcopy -O binary ${LOADER_OUTPUT_PATH} ${LOADER_OUTPUT_PATH}.bin)