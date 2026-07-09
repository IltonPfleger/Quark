MAKEFLAGS += --warn-undefined-variables

HERE := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

INCLUDE        := $(HERE)/include
BUILD          := $(HERE)/build
PAYLOADS       := $(HERE)/payload
TOOLS          := $(HERE)/tools

SYSTEM         := $(BUILD)/QUARK
IMAGE          := $(BUILD)/Image
CONFIG         := $(BUILD)/Config
SYSTEM_ELF     := $(BUILD)/QUARK.elf
SYSTEM_BINARY  := $(BUILD)/QUARK.bun

CONFIGURATOR   := $(TOOLS)/TraitsLoggerGenerator
TRAITS         := $(shell find $(HERE) -name "Traits.hpp")
HASH           := $(BUILD)/Traits.hash
MAPPER         := $(BUILD)/Mapper

TOOL           := riscv64-linux-gnu
CC             := $(TOOL)-g++
LD             := $(TOOL)-ld
NM             := $(TOOL)-nm
SIZE           := $(TOOL)-size
OBJCOPY        := $(TOOL)-objcopy
GDB            := $(TOOL)-gdb
DD             := dd
TRUNCATE       := truncate
QEMU           := qemu-system-riscv64

ARCH           ?= riscv64
MACHINE        ?= virt
PAYLOAD        ?= HelloWorld

CCFLAGS        := -std=c++23
CCFLAGS        += -I$(HERE) -I$(INCLUDE) -I$(HERE)/architecture/$(ARCH) -I$(HERE)/machine/$(MACHINE)
CCFLAGS        += -Wall -Wextra -Werror -pedantic
CCFLAGS        += -D__PAYLOAD=$(PAYLOAD) -g 

build: $(IMAGE).img

$(HASH): $(TRAITS)
	@mkdir -p $(dir $@)
	@cat $^ | sha256sum > $@

MACH_CCFLAGS := $(CCFLAGS)

$(CONFIG): $(HASH)
	mkdir -p $(dir $@)
	g++ $(CCFLAGS) -E $(HERE)/include/Traits.hpp -w -Wno-error=pragma-once-outside-header | $(CONFIGURATOR) > $(CONFIG).cpp
	g++ $(CCFLAGS) $(CONFIG).cpp -o $(CONFIG).elf
	$(CONFIG).elf > $@

-include $(CONFIG)
include $(HERE)/machine/$(MACHINE)/Makedefs.mk
