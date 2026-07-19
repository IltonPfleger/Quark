include ../../Makedefs.mk

ELF := $(BUILD)/$(PAYLOAD).elf
BUILD := $(BUILD)/$(PAYLOAD)

SOURCES := $(shell find . \( -name '*.cpp' -o -name '*.cc' \) -not -path '*/EPOS/*' | sed 's|^\./||')
OBJECTS := $(patsubst %.cc,$(BUILD)/%.o,$(filter %.cc,$(SOURCES)))
OBJECTS += $(patsubst %.cpp,$(BUILD)/%.o,$(filter %.cpp,$(SOURCES)))
DEPENDENCIES := $(OBJECTS:.o=.d)

.PHONY: EPOS

all: $(ELF) EPOS

$(ELF): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) -e main --just-symbols $(SYSTEM).elf -Ttext=$(Payload_Address) --image-base=$(Payload_Address) -o $@ $^

$(BUILD)/%.o: %.cpp $(CONFIG) 
	@mkdir -p $(dir $@)
	$(CC) $(MACH_CCFLAGS) -I. -Ismartdata/include -I../include -MMD -MP -c $< -o $@\
		-Wno-deprecated-enum-enum-conversion -Wno-overflow \
		-Wno-unused-parameter -Wno-pedantic \
		-Wno-tautological-compare -Wno-volatile \
		-Wno-ignored-qualifiers -Wno-deprecated-copy \
		-Wno-return-type -Wno-maybe-uninitialized\
		-Wno-unused-but-set-variable

$(BUILD)/%.o: %.cc $(CONFIG)
	@mkdir -p $(dir $@)
	$(CC) $(MACH_CCFLAGS) -Ismartdata/include -I../../include \
	      -MMD -MP -c $< -o $@ \
	      -Wno-deprecated-enum-enum-conversion -Wno-overflow \
	      -Wno-unused-parameter -Wno-pedantic \
	      -Wno-tautological-compare -Wno-volatile \
	      -Wno-ignored-qualifiers -Wno-deprecated-copy \
	      -Wno-return-type -Wno-maybe-uninitialized\
		  -Wno-unused-but-set-variable

EPOS:
	#./EPOS.sh

-include $(DEPENDENCIES)
