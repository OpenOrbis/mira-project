# OOSDK Defaults
cmake_minimum_required(VERSION 3.12)

# Credits: znullptr
set(CMAKE_SYSTEM_NAME FreeBSD)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_SYSTEM_VERSION 9)

# If the OO_PS4_TOOLCHAIN environment variable is found, set a CMake variable with the same name
if (NOT "" STREQUAL "$ENV{OO_PS4_TOOLCHAIN}")
  file(TO_CMAKE_PATH $ENV{OO_PS4_TOOLCHAIN} OO_PS4_TOOLCHAIN)
endif()

# Verify that we have a cmake variable set
if ("" STREQUAL ${OO_PS4_TOOLCHAIN})
    message(FATAL_ERROR, "OO_PS4_TOOLCHAIN environment variable not set!")
endif()

#set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER) # search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)  # for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)

# Set all of the default CFLAGS
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_AR llvm-ar)

set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES "")
set(CMAKE_C_STANDARD_LIBRARIES "")

set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES "")
set(CMAKE_CXX_STANDARD_LIBRARIES "")

set(CMAKE_ASM_STANDARD_INCLUDE_DIRECTORIES "")
set(CMAKE_ASM_STANDARD_LIBRARIES "")

set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "")

set(CMAKE_C_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_C_IMPLICIT_LINK_DIRECTORIES "")

#set(CMAKE_CXX_LINK_EXECUTABLE "ld.lld -L${OO_PS4_TOOLCHAIN}/lib <LINK_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
#set(CMAKE_C_LINK_EXECUTABLE "ld.lld -L${OO_PS4_TOOLCHAIN}/lib <LINK_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")

# Set the C standard to c11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Set the C++ standard to c++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_SYSROOT ${OO_PS4_TOOLCHAIN})

# Additional include directories
include_directories("${OO_PS4_TOOLCHAIN}/include" "${OO_PS4_TOOLCHAIN}/include/c++/v1" "${PROJECT_SOURCE_DIR}")

# Force using lld
add_link_options(-fuse-ld=lld)

# Don't include system standard libraries
add_link_options(-Wl,-nostdlib)

# Make this a pie executable
add_link_options(-Wl,-pie)

# TODO: Ask specter why this is needed?
add_link_options(-Wl,--eh-frame-hdr)

# Linker script
add_link_options(-Wl,--script ${OO_PS4_TOOLCHAIN}/link.x)

# No dynamic linker is required to fix cmake issue
add_link_options(-Wl,--no-dynamic-linker)

add_link_options(-Wl,-nostdlib)

add_compile_options(-m64 -nodefaultlibs -nostdlib -funwind-tables -fuse-init-array -fno-builtin -ffreestanding -Xclang -emit-obj)

link_libraries("${OO_PS4_TOOLCHAIN}/lib/crtlib.o")
#link_libraries("${OO_PS4_TOOLCHAIN}/lib/crt1.o")

# Additional library directories
link_directories(${OO_PS4_TOOLCHAIN}/lib)

# Set the C/C++ compiler flags
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funwind-tables -fuse-init-array -Xclang -debug-info-kind=limited -Xclang -debugger-tuning=gdb -Xclang -emit-obj")

# Add the default compiler defines
add_compile_definitions(PS4=1 __BSD_VISIBLE=1 _BSD_SOURCE=1)

# If we are compiling for debug mode enable the flag
if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
    add_compile_definitions(_DEBUG=1)
endif()
