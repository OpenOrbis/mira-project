# Default cmake version
cmake_minimum_required(VERSION 3.13)

# Enable verbose makefile
set(CMAKE_VERBOSE_MAKEFILE OFF)

# Set the target configuration, credits znullptr & specter
set(CMAKE_SYSTEM_NAME FreeBSD)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_SYSTEM_VERSION 12)
set(TARGET x86_64-pc-freebsd-elf)

# Check to see if the OO_PS4_TOOLCHAIN environment variable exists
if(DEFINED ENV{OO_PS4_TOOLCHAIN})
    set(OO_PS4_TOOLCHAIN $ENV{OO_PS4_TOOLCHAIN})
else()
    message(FATAL "OO_PS4_TOOLCHAIN environment variable not found")
endif()

LIST(APPEND CMAKE_PROGRAM_PATH ${OO_PS4_TOOLCHAIN})

# Specify the cross compiler
# The target triple needs to match the prefix of the binutils exactly
# (e.g. CMake looks for arm-none-eabi-ar)
set(CLANG_TARGET_TRIPLE x86_64-pc-freebsd-elf)

# C Compiler
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET ${CLANG_TARGET_TRIPLE})


# C++ Compiler
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_COMPILER_TARGET ${CLANG_TARGET_TRIPLE})

# ASM Compiler
set(CMAKE_ASM_COMPILER /usr/bin/clang)
set(CMAKE_ASM_COMPILER_TARGET ${CLANG_TARGET_TRIPLE})

# Don't run the linker on compiler check
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Specify compiler flags
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(ARCH_FLAGS "-funwind-tables")
set(CMAKE_C_FLAGS "-Wall -std=c11 ${ARCH_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -std=c++17  ${ARCH_FLAGS}")
set(CMAKE_ASM_FLAGS "-Wall ${ARCH_FLAGS} -x assembler-with-cpp")

# Specify linker flags
#add_link_options(-T ${OO_PS4_TOOLCHAIN}/link.x)
#add_link_options(-L${OO_PS4_TOOLCHAIN}/lib)
#add_link_options(--eh-frame-hdr)

set(CMAKE_EXE_LINKER_FLAGS "-m elf_x86_64 -pie --script ${OO_PS4_TOOLCHAIN}/link.x --eh-frame-hdr -L${OO_PS4_TOOLCHAIN}/lib")
set(CMAKE_CXX_LINK_EXECUTABLE "ld <LINK_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
set(CMAKE_C_LINK_EXECUTABLE "ld <LINK_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

# C/C++ toolchain
set(CMAKE_SYSROOT "${OO_PS4_TOOLCHAIN}")
set(CMAKE_FIND_ROOT_PATH ${GCC_ARM_SYSROOT})

# Set default include directories
include_directories(SYSTEM ${OO_PS4_TOOLCHAIN}/include ${OO_PS4_TOOLCHAIN}/include/c++/v1)

# Set default library directories
# NOTE: We do not use this, because it passes the clang/clang++ -Wl,rpath=/lib instead of just plain -L/lib
# it is taken care of above in the CMAKE_EXE_LINKER_FLAGS
#link_directories(${OO_PS4_TOOLCHAIN}/lib)

# Add C compiler definitions
add_compile_definitions(PS4=1 __BSD_VISIBLE=1 _BSD_SOURCE=1)

# If we are compiling for debug mode enable the flag
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_DEBUG=1)
endif()

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)

# For libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
