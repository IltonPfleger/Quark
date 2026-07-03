include ../../Makedefs.mk

TARGET_BUILD_DIR := $(BUILD)/$(PAYLOAD)

ELF := $(BUILD)/$(PAYLOAD).elf

SOURCES := $(shell find . -name '*.cpp')
OBJECTS := $(patsubst %.cpp,$(TARGET_BUILD_DIR)/%.o,$(SOURCES))
DEPENDENCIES := $(OBJECTS:.o=.d)

LINUX = ./Image
INITRAMFS = ./initramfs.cpio
KERNEL_BLOB = $(TARGET_BUILD_DIR)/$(KERNEL).inc
INITRAMFS_BLOB = $(TARGET_BUILD_DIR)/$(INITRAMFS).inc

all: $(ELF)

$(ELF): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) -e main --just-symbols $(SYSTEM).elf -Ttext=$(Payload_Address) --image-base=$(Payload_Address) -o $@ $^
	$(OBJCOPY) --update-section .__linux__=$(LINUX) $(ELF)
	$(OBJCOPY) --update-section .__initrd__=$(INITRAMFS) $(ELF)

$(TARGET_BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) -I$(TARGET_BUILD_DIR) -I../include -MMD -MP -c $< -o $@ $(MACH_CCFLAGS)

-include $(DEPENDENCIES)
