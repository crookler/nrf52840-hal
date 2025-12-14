# Makefile for embedded kernel with user-space application support

###
### build variables that can be modified to create a separate project build
###

# FLOAT indicates hard (default) or soft floating point libraries
FLOAT       ?= hard
# OPTFLGS indicates any desired compiler optimization flags
OPTFLGS     ?= -O0
# DBGFLGS indicates any desired compiler debug flags
DBGFLGS     ?= -DDEBUG -g
# OBJARC identifies the .a object file archive to include in the build (OBJARC = X --> archive name `libX.a`)
OBJARC      ?=
# USERAPP is the name of the user-space application to link into the kernel
USERAPP     ?= default
# USERARG is a string of arguments passed to the user application
USERARG     ?=

# build identifier based on above
HASH_BUILD  := $(shell echo -n "$(shell pwd)$(FLOAT)$(OPTFLGS)$(DBGFLGS)$(OBJARC)" | md5sum | cut -d' ' -f1)
HASH_USER   := $(shell echo -n "$(shell pwd)$(FLOAT)$(OPTFLGS)$(DBGFLGS)$(OBJARC)$(USERARG)" | md5sum | cut -d' ' -f1)

###
### usage variables that can be changed as needed
###

# PORT indicates the tty port to access the GDB server on the debugger
PORT        ?= /dev/ttyBmpGdb
# RTTID provides the identifier used in the RTT control block
RTTID       ?= INI642RTT

# project directory structure
K_ASM_DIR   := kernel/asm
K_INC_DIR   := kernel/include
K_LIB_DIR   := kernel/lib
K_SRC_DIR   := kernel/src
K_OBJ_DIR   := build/kernel_$(HASH_BUILD)
U_ASM_DIR   := user/asm
U_INC_DIR   := user/include
U_LIB_DIR   := user/lib
U_SRC_DIR   := user/src
A_ASM_DIR   := app/$(USERAPP)/asm
A_INC_DIR   := app/$(USERAPP)/include
A_SRC_DIR   := app/$(USERAPP)/src
U_OBJ_DIR 	:= build/$(USERAPP)_$(HASH_USER)
BIN_DIR     := build/bin
BINARY 		:= $(BIN_DIR)/kernel_$(HASH_BUILD)_$(USERAPP)_$(HASH_USER).elf

# project directory contents
K_ASM       = $(wildcard $(K_ASM_DIR)/*.s)
K_SRC       = $(wildcard $(K_SRC_DIR)/*.c)
K_OBJ       = $(wildcard $(K_OBJ_DIR)/*.o)
U_ASM       = $(wildcard $(U_ASM_DIR)/*.s)
U_SRC       = $(wildcard $(U_SRC_DIR)/*.c)
A_ASM       = $(wildcard $(A_ASM_DIR)/*.s)
A_SRC       = $(wildcard $(A_SRC_DIR)/*.c)
U_OBJ       = $(wildcard $(U_OBJ_DIR)/*.o)

# used to include the archive contents in the build
ifeq ($(OBJARC),)
    K_LIB :=
    TKLIB := /<K_LIB>/d
else
    K_LIB := $(K_LIB_DIR)/lib$(OBJARC).a
    TKLIB := s|<K_LIB>|$(K_LIB_DIR)/lib$(OBJARC).a|g
endif

# arm toolchain
ARMTC       := arm-none-eabi-
AS          := $(ARMTC)as
GCC         := $(ARMTC)gcc
LD          := $(ARMTC)ld.bfd
GDB         := $(ARMTC)gdb

# build options
ifeq ($(FLOAT), soft)
    U_LIBS := $(U_LIB_DIR)/soft_float/libc.a $(U_LIB_DIR)/soft_float/libgcc.a
    FLOAT_ARCH := -mfloat-abi=soft
else
    U_LIBS := $(U_LIB_DIR)/hard_float/libc.a $(U_LIB_DIR)/hard_float/libm.a $(U_LIB_DIR)/soft_float/libgcc.a
    FLOAT_ARCH := -mfloat-abi=hard -mfpu=fpv4-sp-d16
endif
ARCH        := $(FLOAT_ARCH) -march=armv7e-m -mcpu=cortex-m4 -mlittle-endian -mthumb
ERROR_FLAGS := -Wall -Werror -Wshadow -Wextra -Wunused
GCCFLAGS    := $(ARCH) -std=gnu99 -mslow-flash-data -ffreestanding $(ERROR_FLAGS) $(OPTFLGS) $(DBGFLGS)
K_GCCFLAGS  := $(GCCFLAGS) -nostdlib -nostartfiles
U_GCCFLAGS  := $(GCCFLAGS)

# ANSI color codes for output highlighting -- edit as desired
EM          = \e[1;33m
NO          = \e[0m

###
### make rules
###

# building rules -- these all work together to create the output binary
# and do not require the target board to be attached/ready to flash

.PHONY: build
build: setup $(BINARY)
	@cp util/init_template.gdb /tmp/init.gdb
	@sed -i -e 's|<template-file>|$(BINARY)|g' /tmp/init.gdb
	@sed -i -e 's|<template-port>|$(PORT)|g' /tmp/init.gdb
	@sed -i -e 's|<template-rttid>|$(RTTID)|g' /tmp/init.gdb
	@printf "\n$(EM)Built $(BINARY) using $(FLOAT) float with $(OPTFLGS) and $(DBGFLGS) flags$(NO)\n"

.PHONY: setup
setup:
	@mkdir -p build
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(K_OBJ_DIR)
	@mkdir -p $(U_OBJ_DIR)

$(K_OBJ_DIR)/%.o: $(K_SRC_DIR)/%.c
	@printf "\n$(EM)Compiling: $<$(NO)\n"
	$(GCC) -I$(K_INC_DIR) $(K_GCCFLAGS) -c $< -o $@

$(K_OBJ_DIR)/%.o: $(K_ASM_DIR)/%.s
	@printf "\n$(EM)Assembling: $<$(NO)\n"
	$(AS) $(ARCH) $< -o $@

$(U_OBJ_DIR)/%.o: $(U_SRC_DIR)/%.c
	@printf "\n$(EM)Compiling: $<$(NO)\n"
	@if [ ! -f $(U_INC_DIR)/usrarg.h ]; then util/gen_argv.sh $(U_INC_DIR)/usrarg.h $(USERARG); fi
	$(GCC) $(U_GCCFLAGS) -I$(U_INC_DIR) -c $< -o $@

$(U_OBJ_DIR)/%.o: $(U_ASM_DIR)/%.s
	@printf "\n$(EM)Assembling: $<$(NO)\n"
	$(AS) $(ARCH) $< -o $@

$(U_OBJ_DIR)/%.o: $(A_SRC_DIR)/%.c
	@printf "\n$(EM)Compiling: $<$(NO)\n"
	$(GCC) $(U_GCCFLAGS) -I$(U_INC_DIR) -I$(A_INC_DIR) -c $< -o $@

$(U_OBJ_DIR)/%.o: $(A_ASM_DIR)/%.s
	@printf "\n$(EM)Assembling: $<$(NO)\n"
	$(AS) $(ARCH) $< -o $@

# helpers for binary rule
K_SRC_RULE = $(K_SRC:$(K_SRC_DIR)/%.c=$(K_OBJ_DIR)/%.o)
K_ASM_RULE = $(K_ASM:$(K_ASM_DIR)/%.s=$(K_OBJ_DIR)/%.o)
U_SRC_RULE = $(U_SRC:$(U_SRC_DIR)/%.c=$(U_OBJ_DIR)/%.o)
U_ASM_RULE = $(U_ASM:$(U_ASM_DIR)/%.s=$(U_OBJ_DIR)/%.o)
A_SRC_RULE = $(A_SRC:$(A_SRC_DIR)/%.c=$(U_OBJ_DIR)/%.o)
A_ASM_RULE = $(A_ASM:$(A_ASM_DIR)/%.s=$(U_OBJ_DIR)/%.o)

.PHONY: $(BINARY)
.SILENT: $(BINARY)
$(BINARY): $(K_SRC_RULE) $(K_ASM_RULE) $(U_SRC_RULE) $(U_ASM_RULE) $(A_SRC_RULE) $(A_ASM_RULE)
	@cp util/linker_template.lds /tmp/linker.lds
	@sed -i -e 's|<K_OBJ_DIR>|$(K_OBJ_DIR)|g' /tmp/linker.lds
	@sed -i -e 's|<U_OBJ_DIR>|$(U_OBJ_DIR)|g' /tmp/linker.lds
	@sed -i -e '$(TKLIB)' /tmp/linker.lds
	@printf "\n$(EM)Linking $(BINARY)...$(NO)\n"
	$(LD) -T /tmp/linker.lds -o $(BINARY) $(K_OBJ) $(K_LIB) $(U_OBJ) $(U_LIBS)

# run rule -- this makes sure the binary is built, then flashes it to the
# target board and runs it using gdb

.PHONY: run
.SILENT: run
run: build
	@printf "\n$(EM)Running $(BINARY) on target using GDB...$(NO)\n"
	$(GDB) -x /tmp/init.gdb

# doc rule -- this invokes doxygen to create code documentation
.PHONY: doc
doc:
	doxygen util/doxygen.conf

# clean rule -- this deletes all files created using the above build rules
.PHONY: clean
clean:
	@rm -rf $(BIN_DIR)
	@rm -rf $(K_OBJ_DIR)
	@rm -rf $(U_OBJ_DIR)
	@rm -f $(U_INC_DIR)/usrarg.h

# cleandoc rule -- this deletes everything created by doxygen
.PHONY: cleandoc
cleandoc:
	@rm -f doxygen.warn
	@rm -rf docs

# veryclean rule -- this deletes the entire build directory and all doxygen content
.PHONY: veryclean
veryclean: clean cleandoc
	@rm -rf build
