MAKEFLAGS += --warn-undefined-variables

HERE := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

INCLUDE        := $(HERE)/include
BUILD          := $(HERE)/build
APPLICATIONS   := $(HERE)/application
TOOLS          := $(HERE)/tools

SYSTEM         := $(BUILD)/QUARK
IMAGE          := $(BUILD)/Image
CONFIG         := $(BUILD)/Config
ELF            := $(BUILD)/QUARK.elf

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
CONSOLE        := kgx -e

ARCH           ?= riscv64
MACHINE        ?= virt
APPLICATION    ?= HelloWorld

CCFLAGS        := -std=c++23
CCFLAGS        += -I$(HERE) -I$(INCLUDE) -I$(HERE)/architecture/$(ARCH)/include -I$(HERE)/machine/$(ARCH)/$(MACHINE)/include
#CCFLAGS        += -Wall -Wextra -Werror -pedantic
CCFLAGS        += -ffunction-sections -fdata-sections
CCFLAGS        += -D__APPLICATION=$(APPLICATION) -g

LDFLAGS        := --gc-sections

build: $(IMAGE)

$(HASH): $(TRAITS)
	@$(MKDIR) -p $(dir $@)
	@cat $^ | sha256sum > $@


$(CONFIG): $(HASH)
	$(MKDIR) -p $(dir $@)
	g++ $(CCFLAGS) -E $(HERE)/include/Traits.hpp -w -Wno-error=pragma-once-outside-header | $(CONFIGURATOR) > $(CONFIG).cpp
	g++ $(CCFLAGS) $(CONFIG).cpp -o $(CONFIG).elf
	$(CONFIG).elf > $@

ifneq ($(filter clean,$(MAKECMDGOALS)),clean)
-include $(CONFIG)
endif

MACH_CCFLAGS := $(CCFLAGS)
include $(HERE)/machine/$(ARCH)/$(MACHINE)/Makedefs.mk
