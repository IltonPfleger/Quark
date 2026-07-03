include ../../Makedefs.mk

PAYLOAD_TARGET       := $(BUILD)/$(PAYLOAD).elf
PAYLOAD_BUILD        := $(BUILD)/$(PAYLOAD)
PAYLOAD_SOURCES      := $(shell find . -name '*.cpp')
PAYLOAD_OBJECTS      := $(PAYLOAD_SOURCES:%=$(PAYLOAD_BUILD)/%.o)
PAYLOAD_DEPENDENCIES := $(PAYLOAD_OBJECTS:.o=.d)
LINUX                := ./linux
INITRD               := ./initrd.cpio

all: $(PAYLOAD_TARGET)

$(PAYLOAD_TARGET): $(PAYLOAD_OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) -e main --just-symbols $(SYSTEM).elf -Ttext=$(Payload_Address) --image-base=$(Payload_Address) -o $@ $^
	$(OBJCOPY) --update-section .__linux__=$(LINUX) $@
	$(OBJCOPY) --update-section .__initrd__=$(INITRD) $@

$(PAYLOAD_BUILD)/%.cpp.o: %.cpp $(CONFIG)
	@mkdir -p $(dir $@)
	$(CC) $(MACH_CCFLAGS) -I../include -MMD -MP -c $< -o $@

-include $(PAYLOAD_DEPENDENCIES)
