MAKEFLAGS += --warn-undefined-variables

HERE := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

INCLUDE        := $(HERE)/include
BUILD          := $(HERE)/build
PAYLOADS       := $(HERE)/payload
TOOLS          := $(HERE)/tools

SYSTEM         := $(BUILD)/QUARK
IMAGE          := $(BUILD)/Image
CONFIG         := $(BUILD)/Config
KERNEL_ELF     := $(BUILD)/QUARK.elf
KERNEL_BINARY  := $(BUILD)/QUARK.bin

CONFIGURATOR   := $(TOOLS)/TraitsLoggerGenerator
TRAITS         := $(shell find $(HERE) -name "Traits.hpp")
HASH           := $(BUILD)/Traits.hash
MAPPER         := $(BUILD)/Mapper

SIZE           := size
TOOL           := riscv64-linux-gnu
CC             := $(TOOL)-g++
LD             := $(TOOL)-ld
NM             := $(TOOL)-nm
SIZE           := $(TOOL)-size
OBJCOPY        := $(TOOL)-objcopy
GDB            := $(TOOL)-gdb
CAT            := cat
MV             := mv
MKDIR          := mkdir
DD             := dd
MAKE           := make
RM             := rm
TRUNCATE       := truncate
QEMU           := qemu-system-riscv64

ARCH           ?= riscv64
MACHINE        ?= virt
PAYLOAD        ?= HelloWorld

CCFLAGS        := -std=c++23
CCFLAGS        += -I$(HERE) -I$(INCLUDE) -I$(HERE)/architecture/$(ARCH) -I$(HERE)/machine/$(ARCH)/$(MACHINE)
CCFLAGS        += -Wall -Wextra -Werror -pedantic
CCFLAGS        += -D__PAYLOAD=$(PAYLOAD) -O3

build: $(IMAGE).img

$(HASH): $(TRAITS)
	@$(MKDIR) -p $(dir $@)
	@cat $^ | sha256sum > $@

MACH_CCFLAGS := $(CCFLAGS)

$(CONFIG): $(HASH)
	$(MKDIR) -p $(dir $@)
	g++ $(CCFLAGS) -E $(HERE)/include/Traits.hpp -w -Wno-error=pragma-once-outside-header | $(CONFIGURATOR) > $(CONFIG).cpp
	g++ $(CCFLAGS) $(CONFIG).cpp -o $(CONFIG).elf
	$(CONFIG).elf > $@

-include $(CONFIG)
include $(HERE)/machine/$(ARCH)/$(MACHINE)/Makedefs.mk
