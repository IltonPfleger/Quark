include Makedefs.mk

KERNEL_SOURCES       := $(shell find src -name '*.cpp' | grep -v '*architecture*' | grep -v '*machine*')
KERNEL_SOURCES 	     += $(shell find src/architecture/$(ARCH) -name '*.cpp')
KERNEL_OBJECTS       := $(patsubst src/%.cpp,$(BUILD)/%.o,$(KERNEL_SOURCES))
KERNEL_DEPENDENCIES  := $(KERNEL_OBJECTS:.o=.d)
PAYLOAD_ELF                  := $(BUILD)/$(PAYLOAD).elf
PAYLOAD_BINARY               := $(BUILD)/$(PAYLOAD).bin
MAP                  := $(BUILD)/MemoryMap

run: $(IMAGE).img
	-$(QEMU) -M $(MACHINE) -smp $(CPU_Count) -bios none -nographic -m $(Memory_Size)b -kernel $<

debug: $(IMAGE).img
	-$(QEMU) -M $(MACHINE) -smp $(CPU_Count) -bios none -nographic -m $(Memory_Size)b -kernel $< -S -gdb tcp::1234

gdb:
	$(GDB) -ex "file build/QUARK.elf" -ex "target extended-remote:1234"\

$(IMAGE).bin: $(SYSTEM_BINARY) $(PAYLOAD_ELF)
	$(DD) bs=1M conv=notrunc if=$(SYSTEM_BINARY) of=$@

$(SYSTEM_BINARY) : $(SYSTEM_ELF) $(PAYLOAD_ELF) #$(MAPPER)
	#$(MAPPER) $(SYSTEM_ELF) $(MAP)
	#$(OBJCOPY) --update-section .__kernel_mm__=$(MAP) $(SYSTEM_ELF)
	$(OBJCOPY) --update-section .__payload__=$(PAYLOAD_ELF) $(SYSTEM_ELF)
	$(OBJCOPY) -O binary $(SYSTEM_ELF) $(SYSTEM_BINARY)

$(PAYLOAD_ELF): $(SYSTEM_ELF)
	make PAYLOAD=$(PAYLOAD) -C $(PAYLOADS) all

$(SYSTEM_ELF): $(KERNEL_OBJECTS)
	$(LD) -T Linker.ld --defsym=__BOOT__=$(MemoryMap_BootStart) -o $@ $^

#$(BUILD)/%: $(TOOLS)/%.cpp 
#	mkdir -p $(dir $@)
#	g++ $(CCFLAGS) -o $@ $<

$(BUILD)/%.o: src/%.cpp 
	mkdir -p $(dir $@)
	$(CC) $(MACH_CCFLAGS) -MMD -MP -c $< -o $@

%.bin: %.elf 
	$(OBJCOPY) -O binary $< $@

$(KERNEL_OBJECTS): $(CONFIG)

clean:
	rm -rf build

-include $(KERNEL_DEPENDENCIES)
