# CHANGEME: Default Orbis Version
ifeq ($(MIRA_PLATFORM),)
$(error No firmware version set for MIRA_PLATFORM)
endif

# Check to see if we are building in CI/Checking mode
# Leave empty (erase TRUE) if disabling
ifeq ($(MIRA_CHECKS),)
MIRA_CHECKS :=
endif

# Should the Git hash be included in the build?
ifeq ($(ADD_GIT_HASH),)
ADD_GIT_HASH :=
endif

# Project name
PROJ_NAME := Mira

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
CPPC	:=	clang++	# C++ compiler
LNK		:= 	ld		# Linker
CC		:=	clang 	# C compiler
AS		:=	llvm-ar # Archiver
endif
ifeq ($(UNAME_S),Darwin)
CPPC	:=	/usr/local/opt/llvm/bin/clang++
LNK		:= /usr/local/opt/llvm/bin/ld.lld
CC		:=	/usr/local/opt/llvm/bin/clang
AS		:=	/usr/local/opt/llvm/bin/llvm-ar
endif

# Objcopy
OBJCOPY	:=	objcopy

# cppcheck
CPPCHECK := cppcheck

# Output directory, by default is build
ifeq ($(OUT_DIR),)
OUT_DIR	:=	build
endif

# Source directory
SRC_DIR	:=	src

# If the FREEBSD headers path is not set we will try to use the relative path
ifeq ($(BSD_INC),)
BSD_INC := ../external/freebsd-headers/include
endif

# Set up a path for the external directory
ifeq ($(EXTERNAL_INC),)
EXTERNAL_INC := ../external
endif

# Include directory paths
I_DIRS	:=	-I. -I$(SRC_DIR) -I"$(BSD_INC)" -I$(EXTERNAL_INC)

# Library directory paths
L_DIRS	:=	-L.	-Llib

# Included libraries
LIBS	:=

# C Defines
C_DEFS	:= -D_KERNEL=1 -D_DEBUG -D_STANDALONE -D"MIRA_PLATFORM=${MIRA_PLATFORM}" -DMIRA_UNSUPPORTED_PLATFORMS -D__LP64__ -D_M_X64 -D__amd64__ -D__BSD_VISIBLE

# C++ Flags, -02 Optimizations break shit badly
CFLAGS	:= $(I_DIRS) $(C_DEFS) -fpic -m64 -O0 -fno-builtin -nodefaultlibs -nostdlib -nostdinc -fcheck-new -ffreestanding -fno-strict-aliasing -fno-exceptions -fno-asynchronous-unwind-tables -Wall -Werror -Wno-unknown-pragmas

# Assembly flags
SFLAGS	:= -m64 -nodefaultlibs -nostdlib

# Linker flags
# -Wl,--build-id=none -T $(SRC_DIR)/link.x --emit-relocs -gc-sections -nmagic --relocatable
LFLAGS	:= -v $(L_DIRS) -nostdlib -entry="mira_entry" -pie -T $(SRC_DIR)/link.ld

# Calculate the listing of all file paths
ALL_CPP 	:= $(sort $(shell find $(SRC_DIR)/ -type f -name '*.cpp'))
ALL_C		:= $(sort $(shell find $(SRC_DIR)/ -type f -name '*.c'))
ALL_S		:= $(sort $(shell find $(SRC_DIR)/ -type f -name '*.s'))
ALL_CAPS	:= $(sort $(shell find $(SRC_DIR)/ -type f -name '*.S'))

ALL_SOURCES :=  $(ALL_S) $(ALL_C) $(ALL_CPP)
TO_BUILD := $(ALL_S:$(SRC_DIR)%=$(OUT_DIR)/$(SRC_DIR)%) $(ALL_CAPS:$(SRC_DIR)%=$(OUT_DIR)/$(SRC_DIR)%) $(ALL_C:$(SRC_DIR)%=$(OUT_DIR)/$(SRC_DIR)%) $(ALL_CPP:$(SRC_DIR)%=$(OUT_DIR)/$(SRC_DIR)%)
ALL_OBJ_CPP := $(TO_BUILD:.cpp=.o)
ALL_OBJ_C := $(ALL_OBJ_CPP:.c=.o)
ALL_OBJ_CAPS	:= $(ALL_OBJ_C:.s=.o)
ALL_OBJ := $(ALL_OBJ_CAPS:.S=.o)

# Target elf name
TARGET = "$(PROJ_NAME)_Orbis_${MIRA_PLATFORM}.elf"

# Target payload name (data + text only, no elf)
PAYLOAD = "$(PROJ_NAME)_Orbis_${MIRA_PLATFORM}.bin"

#trainer_loader_Orbis_MIRA_PLATFORM_ORBIS_BSD_672.bin
#trainer_loader_Orbis_MIRA_PLATFORM_ORBIS_BSD_672.bin

# Embed the Mira elf inside of the loader to prevent sending 2 payloads
TRAINER_LOADER_BIN_NAME = trainer_loader_Orbis_${MIRA_PLATFORM}.bin
TRAINER_LOADER_BIN_PATH = $(abspath ${CURDIR}/../trainer_loader/$(OUT_DIR)/trainer_loader_Orbis_${MIRA_PLATFORM}.bin)
#TRAINER_LOADER_BIN_PATH = \"../trainer_loader/$(OUT_DIR)/$(TRAINER_LOADER_BIN_NAME)\"

SFLAGS += -DTRAINER_LOADER_BIN_PATH=\"${TRAINER_LOADER_BIN_PATH}\"

.PHONY: all clean

all: post-build

pre-build:
	@echo "Pre-Build"
ifneq ($(strip $(MIRA_CHECKS)),)
	@cppcheck $(SRC_DIR) $(I_DIRS) $(C_DEFS) --enable=information --check-config
endif
	@$(MAKE) --no-print-directory create
	@$(MAKE) --no-print-directory clean

post-build: main-build
	@echo "Post-Build"
	@echo "Linking $(PROJ_NAME)..."
	$(LNK) $(ALL_OBJ) -o $(OUT_DIR)/$(TARGET) $(LFLAGS) $(LIBS)

main-build: pre-build
	@echo "Building for Firmware $(MIRA_PLATFORM)..."
ifneq ($(strip $(ADD_GIT_HASH)),)
	echo  -n '#ifndef GIT_HASH\n#define GIT_HASH "' > $(SRC_DIR)/GitHash.hpp && \
	git rev-parse HEAD | tr -d "\n" >> $(SRC_DIR)/GitHash.hpp && \
	echo  '"\n#endif' >> $(SRC_DIR)/GitHash.hpp
endif
ifneq ($(strip $(MIRA_CHECKS)),)
	@scan-build $(MAKE) --no-print-directory $(ALL_OBJ)
else
	echo external $(EXTERNAL_INC)
	@$(MAKE) --no-print-directory $(ALL_OBJ)
endif

$(OUT_DIR)/$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $< ..."
ifneq ($(strip $(MIRA_CHECKS)),)
	@clang-tidy -checks=clang-analyzer-*,bugprone-*,portability-*,cert-* $< -- $(I_DIRS) $(C_DEFS)
endif
	@$(CC) $(CFLAGS) $(I_DIRS) -c $< -o $@

$(OUT_DIR)/$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $< ..."
ifneq ($(strip $(MIRA_CHECKS)),)
	@clang-tidy -checks=clang-analyzer-*,bugprone-*,portability-*,cert-* $< -- $(I_DIRS) $(C_DEFS)
endif
	@$(CPPC) $(CFLAGS) -std=c++17 -fno-rtti $(I_DIRS) -c $< -o $@

$(OUT_DIR)/$(SRC_DIR)/%.o: $(SRC_DIR)/%.s
	@echo "Assembling $< ..."
	@$(CC) $(SFLAGS) -c -o $@ $<

$(OUT_DIR)/$(SRC_DIR)/%.o: $(SRC_DIR)/%.S
	@echo "Assembling $< ..."
	@$(CC) $(SFLAGS) -c -o $@ $< 

clean:
	@echo "Cleaning project..."
	@rm -f $(SRC_DIR)/GitHash.hpp
	@rm -f $(OUT_DIR)/$(TARGET) $(OUT_DIR)/$(PAYLOAD) $(shell find $(OUT_DIR)/ -type f -name '*.o')

create:
	@echo "Creating directories..."
	@mkdir -p $(shell find '$(SRC_DIR)/' -type d -printf '$(OUT_DIR)/%p\n')

pushLoader:
	@nc -q 0 192.168.1.2 9020 < loader/build/MiraLoader_Orbis_${MIRA_PLATFORM}.bin

pushLoader2:
	@nc -q 0 192.168.1.2 9022 < loader/build/MiraLoader_Orbis_${MIRA_PLATFORM}.bin

pushMira:
	@nc -q 0 192.168.1.2 9020 < $(OUT_DIR)/$(TARGET)

pushMira2:
	@nc -q 0 192.168.1.2 9023 < $(OUT_DIR)/$(TARGET)
