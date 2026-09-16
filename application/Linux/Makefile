include ../../Makedefs.mk

SOURCES      := $(wildcard *.cpp)
OBJECTS      := $(SOURCES:%=$(BUILD)/$(APPLICATION)/%.o)
DEPENDENCIES := $(OBJECTS:.o=.d)
LINUX        := ./linux
INITRD       := ./initrd.cpio

$(BUILD)/$(APPLICATION).o: $(OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) -r -o $@ $^
	$(OBJCOPY) --update-section .__linux__=$(LINUX) $@
	$(OBJCOPY) --update-section .__initrd__=$(INITRD) $@

$(BUILD)/$(APPLICATION)/%.cpp.o: %.cpp $(CONFIG)
	@mkdir -p $(dir $@)
	$(CC) $(MACH_CCFLAGS) -I../include -MMD -MP -c $< -o $@

-include $(DEPENDENCIES)
