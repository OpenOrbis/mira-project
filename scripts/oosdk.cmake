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
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)  # for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Set all of the default CFLAGS
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_AR llvm-ar)

#message("ree: ${CMAKE_CXX_LINK_EXECUTABLE}")

#SET(CMAKE_C_LINK_EXECUTABLE "ld.lld -m elf_x86_64 -pie --eh-frame-hdr --script=\"${OO_PS4_TOOLCHAIN}/link.x\" -L${OO_PS4_TOOLCHAIN}/lib <OBJECTS> -o <TARGET> \"${OO_PS4_TOOLCHAIN}/lib/crt1.o\" <LINK_LIBRARIES> ")
#SET(CMAKE_CXX_LINK_EXECUTABLE "ld.lld -m elf_x86_64 -pie --eh-frame-hdr --script=\"${OO_PS4_TOOLCHAIN}/link.x\" -L${OO_PS4_TOOLCHAIN}/lib <OBJECTS> -o <TARGET> \"${OO_PS4_TOOLCHAIN}/lib/crt1.o\" <LINK_LIBRARIES> ")

# Set the C standard to c11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Set the C++ standard to c++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Additional include directories
include_directories("${OO_PS4_TOOLCHAIN}/include" "${OO_PS4_TOOLCHAIN}/include/c++/v1" "${PROJECT_SOURCE_DIR}")

add_link_options(-Wl,-nostdlib)
#add_link_options(-Wl,-pie)
add_link_options(-Wl,--eh-frame-hdr)
add_link_options(-Wl,-T ${OO_PS4_TOOLCHAIN}/link.x)

link_libraries("${OO_PS4_TOOLCHAIN}/lib/crt1.o")

# Additional library directories
link_directories(${OO_PS4_TOOLCHAIN}/lib)

# Set the C/C++ compiler flags
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fuse-ld=lld -funwind-tables -fuse-init-array -Xclang -debug-info-kind=limited -Xclang -debugger-tuning=gdb -Xclang -emit-obj")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-ld=lld -funwind-tables -fuse-init-array -Xclang -debug-info-kind=limited -Xclang -debugger-tuning=gdb -Xclang -emit-obj")

# Add the default compiler defines
add_compile_definitions(PS4=1 __BSD_VISIBLE=1 _BSD_SOURCE=1)

# If we are compiling for debug mode enable the flag
if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
    add_compile_definitions(_DEBUG=1)
endif()
