# Leave empty (erase TRUE) if disabling
ifeq ($(OO_PS4_TOOLCHAIN),)
$error("OO_PS4_TOOLCHAIN not set in path.")
else
TOOLCHAIN 	:= $(OO_PS4_TOOLCHAIN)
endif

# Check to see if we are building in CI/Checking mode
# Leave empty (erase TRUE) if disabling
ifeq ($(MIRA_CHECKS),)
MIRA_CHECKS :=
endif

# Project name
PROJ_NAME := Debugger

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
CPPC	:=	clang++	# C++ compiler
LNK		:= 	ld.lld		# Linker
CC		:=	clang 	# C compiler
AS		:=	llvm-ar # Archiver
CDIR	:= 	linux
endif
ifeq ($(UNAME_S),Darwin)
CPPC	:=	/usr/local/opt/llvm/bin/clang++
LNK		:= 	/usr/local/opt/llvm/bin/ld.lld
CC		:=	/usr/local/opt/llvm/bin/clang
AS		:=	/usr/local/opt/llvm/bin/llvm-ar
CDIR	:= 	macos
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

# Set up a path for the external directory
ifeq ($(EXTERNAL_INC),)
EXTERNAL_INC := ../external/mira
endif

# Include directory paths
I_DIRS	:=	-I. -I$(SRC_DIR) -I"$(EXTERNAL_INC)" -I$(TOOLCHAIN)/include

# Library directory paths
L_DIRS	:=	-L.	-Llib -L$(TOOLCHAIN)/lib

# Included libraries
LIBS	:= -lSceLibcInternal -lkernel

# C Defines
C_DEFS	:= -DPS4

# C/C++ Flags
CFLAGS	:= -cc1 -triple x86_64-scei-ps4-elf -munwind-tables $(I_DIRS) -emit-obj -fno-builtin -ffreestanding

# Assembly flags
SFLAGS	:= -fPIE -m64 -nodefaultlibs -nostdlib

# Linker flags
LFLAGS	:= -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr $(L_DIRS) $(LIBS) $(TOOLCHAIN)/lib/crtlib.o -nostdlib

# Calculate the listing of all file paths
ALL_CPP 	:= $(sort $(shell find $(SRC_DIR)/ -type f -name '*.cpp'))
ALL_C		:= $(sort $(shell find $(SRC_DIR)/ -type f -name '*.c'))
ALL_S		:= $(sort $(shell find $(SRC_DIR)/ -type f -name '*.s'))
ALL_CAPS	:= $(sort $(shell find $(SRC_DIR)/ -type f -name '*.S'))

ALL_SOURCES 	:=  $(ALL_CAPS) $(ALL_S) $(ALL_C) $(ALL_CPP)
TO_BUILD 		:= $(ALL_CAPS:$(SRC_DIR)%=$(OUT_DIR)/$(SRC_DIR)%) $(ALL_S:$(SRC_DIR)%=$(OUT_DIR)/$(SRC_DIR)%) $(ALL_C:$(SRC_DIR)%=$(OUT_DIR)/$(SRC_DIR)%) $(ALL_CPP:$(SRC_DIR)%=$(OUT_DIR)/$(SRC_DIR)%)
ALL_OBJ_CPP 	:= $(TO_BUILD:.cpp=.o)
ALL_OBJ_C 		:= $(ALL_OBJ_CPP:.c=.o)
ALL_OBJ_CAPS	:= $(ALL_OBJ_C:.s=.o)
ALL_OBJ 		:= $(ALL_OBJ_CAPS:.S=.o)

# Target elf name
TARGET 		= $(PROJ_NAME).prx
TARGETSTUB 	= $(PROJ_NAME).so

.PHONY: all clean

all: post-build

pre-build:
	@echo "Pre-Build"
ifneq ($(strip $(MIRA_CHECKS)),)
	@cppcheck $(SRC_DIR) $(I_DIRS) $(C_DEFS) --enable=information --check-config
endif
	@$(MAKE) --no-print-directory clean

post-build: main-build
	@echo "Post-Build"
	@echo "Linking $(PROJ_NAME)..."
	$(LNK) $(ALL_OBJ) -o $(OUT_DIR)/$(TARGET) $(LFLAGS) $(LIBS)
	@echo "Creating prx..."
	$(TOOLCHAIN)/bin/$(CDIR)/create-lib -in=$(OUT_DIR)/$(TARGET) -out=$(OUT_DIR)/$(TARGET).oprx --paid 0x3800000000000011

main-build: pre-build
	@echo "Building $(PROJ_NAME)..."
ifneq ($(strip $(MIRA_CHECKS)),)
	@scan-build $(MAKE) --no-print-directory $(ALL_OBJ)
else
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
	@$(CPPC) $(CFLAGS) $(I_DIRS) -o $@ $<

$(OUT_DIR)/$(SRC_DIR)/%.o: $(SRC_DIR)/%.s
	@echo "Assembling $< ..."
	@$(CC) $(SFLAGS) -c -o $@ $< 

$(OUT_DIR)/$(SRC_DIR)/%.o: $(SRC_DIR)/%.S
	@echo "Assembling $< ..."
	@$(CC) $(SFLAGS) -c -o $@ $< 

clean:
	@echo "Cleaning project..."
	@rm -f $(OUT_DIR)/$(TARGET) $(shell find $(OUT_DIR)/ -type f -name '*.o')

create:
	@echo "Creating directories..."
	@mkdir -p $(shell find '$(SRC_DIR)/' -type d -printf '$(OUT_DIR)/%p\n')
