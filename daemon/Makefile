# CHANGEME: Default Orbis Version
ifeq ($(OO_PS4_TOOLCHAIN),)
$(error OO_PS4_TOOLCHAIN not set)
endif

# Check to see if we are building in CI/Checking mode
# Leave empty (erase TRUE) if disabling
ifeq ($(MIRA_CHECKS),)
MIRA_CHECKS :=
endif

# Project name
PROJ_NAME := Mira

# C++ compiler
CPPC	:=	clang++

# Linker
LNK		:= ld.lld

# C compiler
CC		:=	clang

# Archiver
AS		:=	llvm-ar

# cppcheck
CPPCHECK := cppcheck

# Output directory, by default is build
ifeq ($(OUT_DIR),)
OUT_DIR	:=	x64/Debug
endif

# Source directory
SRC_DIR	:=	src

# Include directory paths
I_DIRS	:=	-I$(SRC_DIR) -I$(SRC_DIR)/External/include -I"$(OO_PS4_TOOLCHAIN)/include" -I$(OO_PS4_TOOLCHAIN)/include/c++/v1

# Library directory paths
L_DIRS	:=	-L"$(OO_PS4_TOOLCHAIN)/lib"

# Included libraries
LIBS	:= -lc -lkernel -lc++

# C Defines
C_DEFS	:= -D_DEBUG -DPS4

# C++ Flags
CFLAGS	:= -cc1 -triple x86_64-pc-freebsd-elf -munwind-tables $(I_DIRS) $(C_DEFS) -fuse-init-array -emit-obj

# Assembly flags
SFLAGS	:= -m64 -nodefaultlibs -nostdlib

# Linker flags
# -Wl,--build-id=none -T $(SRC_DIR)/link.x --emit-relocs -gc-sections -nmagic --relocatable
LFLAGS	:= -m elf_x86_64 -pie --script $(OO_PS4_TOOLCHAIN)/link.x --eh-frame-hdr $(L_DIRS) $(LIBS) $(OO_PS4_TOOLCHAIN)/lib/crt1.o

# Calculate the listing of all file paths
ALL_CPP := $(sort $(shell find $(SRC_DIR)/ -type f -name '*.cpp'))
ALL_C	:= $(sort $(shell find $(SRC_DIR)/ -type f -name '*.c'))
ALL_S	:= $(sort $(shell find $(SRC_DIR)/ -type f -name '*.s'))

ALL_SOURCES :=  $(ALL_S) $(ALL_C) $(ALL_CPP)
TO_BUILD := $(ALL_S:$(SRC_DIR)%=$(OUT_DIR)/$(SRC_DIR)%) $(ALL_C:$(SRC_DIR)%=$(OUT_DIR)/$(SRC_DIR)%) $(ALL_CPP:$(SRC_DIR)%=$(OUT_DIR)/$(SRC_DIR)%)
ALL_OBJ_CPP := $(TO_BUILD:.cpp=.o)
ALL_OBJ_C := $(ALL_OBJ_CPP:.c=.o)
ALL_OBJ := $(ALL_OBJ_C:.s=.o)

# Target elf name
TARGET = eboot.bin

#Make Rules

$(TARGET): pre-run $(OUT_DIR) $(ALL_OBJ)
	
pre-run:
	@mkdir -p $(shell find '$(SRC_DIR)/' -type d -printf '$(OUT_DIR)/%p\n')

$(OUT_DIR)/$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $< ..."
ifneq ($(strip $(MIRA_CHECKS)),)
	@clang-tidy -checks=clang-analyzer-*,bugprone-*,portability-*,cert-* $< -- $(I_DIRS) $(C_DEFS)
endif
	$(CC) $(CFLAGS) -o $@ $<

$(OUT_DIR)/$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $< ..."
ifneq ($(strip $(MIRA_CHECKS)),)
	@clang-tidy -checks=clang-analyzer-*,bugprone-*,portability-*,cert-* $< -- $(I_DIRS) $(C_DEFS)
endif
	$(CPPC) $(CFLAGS) -o $@ $<

$(OUT_DIR)/$(SRC_DIR)/%.o: $(SRC_DIR)/%.s
	@echo "Assembling $< ..."
	$(CC) $(SFLAGS) -o $@ $< 

$(OUT_DIR):
	@mkdir $@

clean:
	@echo "Cleaning project..."
	@rm -f $(OUT_DIR)/$(TARGET) $(shell find $(OUT_DIR)/ -type f -name '*.o')