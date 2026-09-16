SOURCES := $(shell find $(APPLICATION) -name '*.cpp')
OBJECTS     := $(SOURCES:%=$(BUILD)/$(APPLICATION)/%.o)
DEPENDENCIES := $(OBJECTS:.o=.d)

$(BUILD)/$(APPLICATION).o: $(OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) -r -o $@ $^

$(BUILD)/$(APPLICATION)/%.cpp.o: %.cpp $(CONFIG)
	@mkdir -p $(dir $@)
	$(CC) $(MACH_CCFLAGS) -I../include -MMD -MP -c $< -o $@

-include $(DEPENDENCIES)
