PAYLOAD_TARGET := $(BUILD)/$(PAYLOAD).elf
PAYLOAD_BUILD  := $(BUILD)/$(PAYLOAD)
PAYLOAD_SOURCES := $(shell find $(PAYLOAD) -name '*.cpp')
PAYLOAD_OBJECTS     := $(PAYLOAD_SOURCES:%=$(PAYLOAD_BUILD)/%.o)
PAYLOAD_DEPENDENCIES := $(PAYLOAD_OBJECTS:.o=.d)

all: $(PAYLOAD_TARGET)

$(PAYLOAD_TARGET): $(PAYLOAD_OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) -e main --just-symbols $(SYSTEM).elf -Ttext=$(MemoryMap_Application) --image-base=$(MemoryMap_Application) -o $@ $^

$(PAYLOAD_BUILD)/%.cpp.o: %.cpp $(CONFIG)
	@mkdir -p $(dir $@)
	$(CC) $(MACH_CCFLAGS) -I../include -MMD -MP -c $< -o $@

-include $(PAYLOAD_DEPENDENCIES)
