SOURCES := $(shell find $(PAYLOAD) -name '*.cpp')
OBJECTS     := $(SOURCES:%=$(BUILD)/$(PAYLOAD)/%.o)
DEPENDENCIES := $(OBJECTS:.o=.d)

$(BUILD)/$(PAYLOAD).o: $(OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) -r -o $@ $^

$(BUILD)/$(PAYLOAD)/%.cpp.o: %.cpp $(CONFIG)
	@mkdir -p $(dir $@)
	$(CC) $(MACH_CCFLAGS) -I../include -MMD -MP -c $< -o $@

-include $(DEPENDENCIES)
